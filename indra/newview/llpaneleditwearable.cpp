/** 
 * @file llpaneleditwearable.cpp
 * @brief UI panel for editing of a particular wearable item.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneleditwearable.h"
#include "llpanel.h"
#include "llwearable.h"
#include "lluictrl.h"
#include "llscrollingpanellist.h"
#include "llvisualparam.h"
#include "lltoolmorph.h"
#include "llviewerjointmesh.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llvoavatarself.h"
#include "lltexteditor.h"
#include "lltextbox.h"
#include "llagentwearables.h"
#include "llscrollingpanelparam.h"
#include "llradiogroup.h"
#include "llnotificationsutil.h"

#include "llcolorswatch.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "llviewercontrol.h"    // gSavedSettings
#include "llviewertexturelist.h"
#include "llagentcamera.h"
#include "llmorphview.h"

#include "llcommandhandler.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llfloatercustomize.h"

#include "llwearablelist.h"

// subparts of the UI for focus, camera position, etc.
enum ESubpart {
        SUBPART_SHAPE_HEAD = 1, // avoid 0
        SUBPART_SHAPE_EYES,
        SUBPART_SHAPE_EARS,
        SUBPART_SHAPE_NOSE,
        SUBPART_SHAPE_MOUTH,
        SUBPART_SHAPE_CHIN,
        SUBPART_SHAPE_TORSO,
        SUBPART_SHAPE_LEGS,
        SUBPART_SHAPE_WHOLE,
        SUBPART_SHAPE_DETAIL,
        SUBPART_SKIN_COLOR,
        SUBPART_SKIN_FACEDETAIL,
        SUBPART_SKIN_MAKEUP,
        SUBPART_SKIN_BODYDETAIL,
        SUBPART_HAIR_COLOR,
        SUBPART_HAIR_STYLE,
        SUBPART_HAIR_EYEBROWS,
        SUBPART_HAIR_FACIAL,
        SUBPART_EYES,
        SUBPART_SHIRT,
        SUBPART_PANTS,
        SUBPART_SHOES,
        SUBPART_SOCKS,
        SUBPART_JACKET,
        SUBPART_GLOVES,
        SUBPART_UNDERSHIRT,
        SUBPART_UNDERPANTS,
        SUBPART_SKIRT,
        SUBPART_ALPHA,
        SUBPART_TATTOO,
        SUBPART_PHYSICS_BREASTS_UPDOWN,
        SUBPART_PHYSICS_BREASTS_INOUT,
        SUBPART_PHYSICS_BREASTS_LEFTRIGHT,
        SUBPART_PHYSICS_BELLY_UPDOWN,
        SUBPART_PHYSICS_BUTT_UPDOWN,
        SUBPART_PHYSICS_BUTT_LEFTRIGHT,
        SUBPART_PHYSICS_ADVANCED,
 };

using namespace LLVOAvatarDefines;

typedef std::vector<ESubpart> subpart_vec_t;

// Locally defined classes

class LLEditWearableDictionary : public LLSingleton<LLEditWearableDictionary>
{
        //--------------------------------------------------------------------
        // Constructors and Destructors
        //--------------------------------------------------------------------
public:
        LLEditWearableDictionary();
        virtual ~LLEditWearableDictionary();
        
        //--------------------------------------------------------------------
        // Wearable Types
        //--------------------------------------------------------------------
public:
        struct WearableEntry : public LLDictionaryEntry
        {
                WearableEntry(LLWearableType::EType type,
                                          const std::string &title,
                                          U8 num_color_swatches,  // number of 'color_swatches'
                                          U8 num_texture_pickers, // number of 'texture_pickers'
                                          U8 num_subparts, ... ); // number of subparts followed by a list of ETextureIndex and ESubparts


                const LLWearableType::EType mWearableType;
                subpart_vec_t           mSubparts;
                texture_vec_t           mColorSwatchCtrls;
                texture_vec_t           mTextureCtrls;
        };

        struct Wearables : public LLDictionary<LLWearableType::EType, WearableEntry>
        {
                Wearables();
        } mWearables;

        const WearableEntry*    getWearable(LLWearableType::EType type) const { return mWearables.lookup(type); }

        //--------------------------------------------------------------------
        // Subparts
        //--------------------------------------------------------------------
public:
        struct SubpartEntry : public LLDictionaryEntry
        {
                SubpartEntry(ESubpart part,
                                         const std::string &joint,
                                         const std::string &edit_group,
                                         const std::string &button_name,
                                         const LLVector3d  &target_offset,
                                         const LLVector3d  &camera_offset,
                                         const ESex        &sex);

			
			ESubpart			mSubpart;
			std::string			mTargetJoint;
			std::string			mEditGroup;
			std::string			mButtonName;
			LLVector3d			mTargetOffset;
			LLVector3d			mCameraOffset;
			ESex				mSex;
		};

        struct Subparts : public LLDictionary<ESubpart, SubpartEntry>
        {
                Subparts();
        } mSubparts;

        const SubpartEntry*  getSubpart(ESubpart subpart) const { return mSubparts.lookup(subpart); }

        //--------------------------------------------------------------------
        // Picker Control Entries
        //--------------------------------------------------------------------
public:
        struct PickerControlEntry : public LLDictionaryEntry
        {
                PickerControlEntry(ETextureIndex tex_index,
                                                   const std::string name,
                                                   const LLUUID default_image_id = LLUUID::null,
                                                   const bool allow_no_texture = false,
												   const std::string checkbox_name = LLStringUtil::null);
                ETextureIndex           mTextureIndex;
                const std::string       mControlName;
                const LLUUID            mDefaultImageId;
                const bool                      mAllowNoTexture;
				std::string				mCheckboxName;
        };

        struct ColorSwatchCtrls : public LLDictionary<ETextureIndex, PickerControlEntry>
        {
                ColorSwatchCtrls();
        } mColorSwatchCtrls;

        struct TextureCtrls : public LLDictionary<ETextureIndex, PickerControlEntry>
        {
                TextureCtrls();
        } mTextureCtrls;

        const PickerControlEntry* getTexturePicker(ETextureIndex index) const { return mTextureCtrls.lookup(index); }
        const PickerControlEntry* getColorSwatch(ETextureIndex index) const { return mColorSwatchCtrls.lookup(index); }
};

LLEditWearableDictionary::LLEditWearableDictionary()
{

}

//virtual 
LLEditWearableDictionary::~LLEditWearableDictionary()
{
}

LLEditWearableDictionary::Wearables::Wearables()
{
        // note the subpart that is listed first is treated as "default", regardless of what order is in enum.
        // Please match the order presented in XUI. -Nyx
        // this will affect what camera angle is shown when first editing a wearable
        addEntry(LLWearableType::WT_SHAPE,              new WearableEntry(LLWearableType::WT_SHAPE,"edit_shape_title",0,0,9,  SUBPART_SHAPE_WHOLE, SUBPART_SHAPE_HEAD,        SUBPART_SHAPE_EYES,     SUBPART_SHAPE_EARS,     SUBPART_SHAPE_NOSE,     SUBPART_SHAPE_MOUTH, SUBPART_SHAPE_CHIN, SUBPART_SHAPE_TORSO, SUBPART_SHAPE_LEGS));
        addEntry(LLWearableType::WT_SKIN,               new WearableEntry(LLWearableType::WT_SKIN,"edit_skin_title",0,3,4, TEX_HEAD_BODYPAINT, TEX_UPPER_BODYPAINT, TEX_LOWER_BODYPAINT, SUBPART_SKIN_COLOR, SUBPART_SKIN_FACEDETAIL, SUBPART_SKIN_MAKEUP, SUBPART_SKIN_BODYDETAIL));
        addEntry(LLWearableType::WT_HAIR,               new WearableEntry(LLWearableType::WT_HAIR,"edit_hair_title",0,1,4, TEX_HAIR, SUBPART_HAIR_COLOR,       SUBPART_HAIR_STYLE,     SUBPART_HAIR_EYEBROWS, SUBPART_HAIR_FACIAL));
        addEntry(LLWearableType::WT_EYES,               new WearableEntry(LLWearableType::WT_EYES,"edit_eyes_title",0,1,1, TEX_EYES_IRIS, SUBPART_EYES));
        addEntry(LLWearableType::WT_SHIRT,              new WearableEntry(LLWearableType::WT_SHIRT,"edit_shirt_title",1,1,1, TEX_UPPER_SHIRT, TEX_UPPER_SHIRT, SUBPART_SHIRT));
        addEntry(LLWearableType::WT_PANTS,              new WearableEntry(LLWearableType::WT_PANTS,"edit_pants_title",1,1,1, TEX_LOWER_PANTS, TEX_LOWER_PANTS, SUBPART_PANTS));
        addEntry(LLWearableType::WT_SHOES,              new WearableEntry(LLWearableType::WT_SHOES,"edit_shoes_title",1,1,1, TEX_LOWER_SHOES, TEX_LOWER_SHOES, SUBPART_SHOES));
        addEntry(LLWearableType::WT_SOCKS,              new WearableEntry(LLWearableType::WT_SOCKS,"edit_socks_title",1,1,1, TEX_LOWER_SOCKS, TEX_LOWER_SOCKS, SUBPART_SOCKS));
        addEntry(LLWearableType::WT_JACKET,     new WearableEntry(LLWearableType::WT_JACKET,"edit_jacket_title",1,2,1, TEX_UPPER_JACKET, TEX_UPPER_JACKET, TEX_LOWER_JACKET, SUBPART_JACKET));
        addEntry(LLWearableType::WT_GLOVES,     new WearableEntry(LLWearableType::WT_GLOVES,"edit_gloves_title",1,1,1, TEX_UPPER_GLOVES, TEX_UPPER_GLOVES, SUBPART_GLOVES));
        addEntry(LLWearableType::WT_UNDERSHIRT, new WearableEntry(LLWearableType::WT_UNDERSHIRT,"edit_undershirt_title",1,1,1, TEX_UPPER_UNDERSHIRT, TEX_UPPER_UNDERSHIRT, SUBPART_UNDERSHIRT));
        addEntry(LLWearableType::WT_UNDERPANTS, new WearableEntry(LLWearableType::WT_UNDERPANTS,"edit_underpants_title",1,1,1, TEX_LOWER_UNDERPANTS, TEX_LOWER_UNDERPANTS, SUBPART_UNDERPANTS));
        addEntry(LLWearableType::WT_SKIRT,              new WearableEntry(LLWearableType::WT_SKIRT,"edit_skirt_title",1,1,1, TEX_SKIRT, TEX_SKIRT, SUBPART_SKIRT));
        addEntry(LLWearableType::WT_ALPHA,              new WearableEntry(LLWearableType::WT_ALPHA,"edit_alpha_title",0,5,1, TEX_LOWER_ALPHA, TEX_UPPER_ALPHA, TEX_HEAD_ALPHA, TEX_EYES_ALPHA, TEX_HAIR_ALPHA, SUBPART_ALPHA));
        addEntry(LLWearableType::WT_TATTOO,     new WearableEntry(LLWearableType::WT_TATTOO,"edit_tattoo_title",1,3,1, TEX_HEAD_TATTOO, TEX_LOWER_TATTOO, TEX_UPPER_TATTOO, TEX_HEAD_TATTOO, SUBPART_TATTOO));
        addEntry(LLWearableType::WT_PHYSICS,    new WearableEntry(LLWearableType::WT_PHYSICS,"edit_physics_title",0,0,7, SUBPART_PHYSICS_BREASTS_UPDOWN, SUBPART_PHYSICS_BREASTS_INOUT, SUBPART_PHYSICS_BREASTS_LEFTRIGHT, SUBPART_PHYSICS_BELLY_UPDOWN, SUBPART_PHYSICS_BUTT_UPDOWN, SUBPART_PHYSICS_BUTT_LEFTRIGHT, SUBPART_PHYSICS_ADVANCED));
}

LLEditWearableDictionary::WearableEntry::WearableEntry(LLWearableType::EType type,
                                          const std::string &title,
                                          U8 num_color_swatches,
                                          U8 num_texture_pickers,
                                          U8 num_subparts, ... ) :
        LLDictionaryEntry(title),
        mWearableType(type)
{
        va_list argp;
        va_start(argp, num_subparts);

        for (U8 i = 0; i < num_color_swatches; ++i)
        {
                ETextureIndex index = (ETextureIndex)va_arg(argp,int);
                mColorSwatchCtrls.push_back(index);
        }

        for (U8 i = 0; i < num_texture_pickers; ++i)
        {
                ETextureIndex index = (ETextureIndex)va_arg(argp,int);
                mTextureCtrls.push_back(index);
        }

        for (U8 i = 0; i < num_subparts; ++i)
        {
                ESubpart part = (ESubpart)va_arg(argp,int);
                mSubparts.push_back(part);
        }
}

LLEditWearableDictionary::Subparts::Subparts()
{
	addEntry(SUBPART_SHAPE_WHOLE, new SubpartEntry(SUBPART_SHAPE_WHOLE, "mPelvis", "shape_body","Body", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
	const LLVector3d head_target(0.f, 0.f, 0.05f);
	const LLVector3d head_camera(-0.5f, 0.05f, 0.07f);
	addEntry(SUBPART_SHAPE_HEAD, new SubpartEntry(SUBPART_SHAPE_HEAD, "mHead", "shape_head","Head", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_EYES, new SubpartEntry(SUBPART_SHAPE_EYES, "mHead", "shape_eyes","Eyes", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_EARS, new SubpartEntry(SUBPART_SHAPE_EARS, "mHead", "shape_ears","Ears", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_NOSE, new SubpartEntry(SUBPART_SHAPE_NOSE, "mHead", "shape_nose","Nose", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_MOUTH, new SubpartEntry(SUBPART_SHAPE_MOUTH, "mHead", "shape_mouth","Mouth", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_CHIN, new SubpartEntry(SUBPART_SHAPE_CHIN, "mHead", "shape_chin","Chin", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SHAPE_TORSO, new SubpartEntry(SUBPART_SHAPE_TORSO, "mTorso", "shape_torso","Torso", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f), SEX_BOTH));
	addEntry(SUBPART_SHAPE_LEGS, new SubpartEntry(SUBPART_SHAPE_LEGS, "mPelvis", "shape_legs","Legs", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));

	addEntry(SUBPART_SKIN_COLOR, new SubpartEntry(SUBPART_SKIN_COLOR, "mHead", "skin_color","Skin Color", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SKIN_FACEDETAIL, new SubpartEntry(SUBPART_SKIN_FACEDETAIL, "mHead", "skin_facedetail","Face Detail", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SKIN_MAKEUP, new SubpartEntry(SUBPART_SKIN_MAKEUP, "mHead", "skin_makeup","Makeup", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_SKIN_BODYDETAIL, new SubpartEntry(SUBPART_SKIN_BODYDETAIL, "mPelvis", "skin_bodydetail","Body Detail", LLVector3d(0.f, 0.f, -0.2f), LLVector3d(-2.5f, 0.5f, 0.5f), SEX_BOTH));

	addEntry(SUBPART_HAIR_COLOR, new SubpartEntry(SUBPART_HAIR_COLOR, "mHead", "hair_color","Color", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
	addEntry(SUBPART_HAIR_STYLE, new SubpartEntry(SUBPART_HAIR_STYLE, "mHead", "hair_style","Style", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
	addEntry(SUBPART_HAIR_EYEBROWS, new SubpartEntry(SUBPART_HAIR_EYEBROWS, "mHead", "hair_eyebrows","Eyebrows", head_target, head_camera,SEX_BOTH));
	addEntry(SUBPART_HAIR_FACIAL, new SubpartEntry(SUBPART_HAIR_FACIAL, "mHead", "hair_facial","Facial", head_target, head_camera,SEX_MALE));

	addEntry(SUBPART_EYES, new SubpartEntry(SUBPART_EYES, "mHead", "eyes",LLStringUtil::null, head_target, head_camera,SEX_BOTH));

	addEntry(SUBPART_SHIRT, new SubpartEntry(SUBPART_SHIRT, "mTorso", "shirt",LLStringUtil::null, LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f), SEX_BOTH));
	addEntry(SUBPART_PANTS, new SubpartEntry(SUBPART_PANTS, "mPelvis", "pants",LLStringUtil::null, LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));
	addEntry(SUBPART_SHOES, new SubpartEntry(SUBPART_SHOES, "mPelvis", "shoes",LLStringUtil::null, LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));
	addEntry(SUBPART_SOCKS, new SubpartEntry(SUBPART_SOCKS, "mPelvis", "socks",LLStringUtil::null, LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));
	addEntry(SUBPART_JACKET, new SubpartEntry(SUBPART_JACKET, "mTorso", "jacket",LLStringUtil::null, LLVector3d(0.f, 0.f, 0.f), LLVector3d(-2.f, 0.1f, 0.3f), SEX_BOTH));
	addEntry(SUBPART_SKIRT, new SubpartEntry(SUBPART_SKIRT, "mPelvis", "skirt",LLStringUtil::null, LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));
	addEntry(SUBPART_GLOVES, new SubpartEntry(SUBPART_GLOVES, "mTorso", "gloves",LLStringUtil::null, LLVector3d(0.f, 0.f, 0.f), LLVector3d(-1.f, 0.15f, 0.f), SEX_BOTH));
	addEntry(SUBPART_UNDERSHIRT, new SubpartEntry(SUBPART_UNDERSHIRT, "mTorso", "undershirt",LLStringUtil::null, LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f), SEX_BOTH));
	addEntry(SUBPART_UNDERPANTS, new SubpartEntry(SUBPART_UNDERPANTS, "mPelvis", "underpants",LLStringUtil::null, LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f), SEX_BOTH));
	addEntry(SUBPART_ALPHA, new SubpartEntry(SUBPART_ALPHA, "mPelvis", "alpha",LLStringUtil::null, LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f), SEX_BOTH));
	addEntry(SUBPART_TATTOO, new SubpartEntry(SUBPART_TATTOO, "mPelvis", "tattoo", LLStringUtil::null, LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
	addEntry(SUBPART_PHYSICS_BREASTS_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BREASTS_UPDOWN, "mTorso", "physics_breasts_updown", "Breast Bounce", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-0.8f, 0.15f, 0.38),SEX_FEMALE));
	addEntry(SUBPART_PHYSICS_BREASTS_INOUT, new SubpartEntry(SUBPART_PHYSICS_BREASTS_INOUT, "mTorso", "physics_breasts_inout", "Breast Cleavage", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-0.8f, 0.15f, 0.38f),SEX_FEMALE));
	addEntry(SUBPART_PHYSICS_BREASTS_LEFTRIGHT, new SubpartEntry(SUBPART_PHYSICS_BREASTS_LEFTRIGHT, "mTorso", "physics_breasts_leftright", "Breast Sway", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-0.8f, 0.15f, 0.38f),SEX_FEMALE));
	addEntry(SUBPART_PHYSICS_BELLY_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BELLY_UPDOWN, "mTorso", "physics_belly_updown", "Belly Bounce", LLVector3d(0.f, 0.f, -.05f), LLVector3d(-0.8f, 0.15f, 0.38f),SEX_BOTH));
	addEntry(SUBPART_PHYSICS_BUTT_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BUTT_UPDOWN, "mTorso", "physics_butt_updown", "Butt Bounce", LLVector3d(0.f, 0.f,-.1f), LLVector3d(0.3f, 0.8f, -0.1f),SEX_BOTH));
	addEntry(SUBPART_PHYSICS_BUTT_LEFTRIGHT, new SubpartEntry(SUBPART_PHYSICS_BUTT_LEFTRIGHT, "mTorso", "physics_butt_leftright", "Butt Sway", LLVector3d(0.f, 0.f,-.1f), LLVector3d(0.3f, 0.8f, -0.1f),SEX_BOTH));
	addEntry(SUBPART_PHYSICS_ADVANCED, new SubpartEntry(SUBPART_PHYSICS_ADVANCED, "mTorso", "physics_advanced", "Advanced Parameters", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
}

LLEditWearableDictionary::SubpartEntry::SubpartEntry(ESubpart part,
                                         const std::string &joint,
                                         const std::string &edit_group,
                                         const std::string &button_name,
                                         const LLVector3d  &target_offset,
                                         const LLVector3d  &camera_offset,
                                         const ESex        &sex) :
        LLDictionaryEntry(edit_group),
        mSubpart(part),
        mTargetJoint(joint),
        mEditGroup(edit_group),
        mButtonName(button_name),
        mTargetOffset(target_offset),
        mCameraOffset(camera_offset),
        mSex(sex)
{
}

LLEditWearableDictionary::ColorSwatchCtrls::ColorSwatchCtrls()
{
        addEntry ( TEX_UPPER_SHIRT,  new PickerControlEntry (TEX_UPPER_SHIRT, "Color/Tint" ));
        addEntry ( TEX_LOWER_PANTS,  new PickerControlEntry (TEX_LOWER_PANTS, "Color/Tint" ));
        addEntry ( TEX_LOWER_SHOES,  new PickerControlEntry (TEX_LOWER_SHOES, "Color/Tint" ));
        addEntry ( TEX_LOWER_SOCKS,  new PickerControlEntry (TEX_LOWER_SOCKS, "Color/Tint" ));
        addEntry ( TEX_UPPER_JACKET, new PickerControlEntry (TEX_UPPER_JACKET, "Color/Tint" ));
        addEntry ( TEX_SKIRT,  new PickerControlEntry (TEX_SKIRT, "Color/Tint" ));
        addEntry ( TEX_UPPER_GLOVES, new PickerControlEntry (TEX_UPPER_GLOVES, "Color/Tint" ));
        addEntry ( TEX_UPPER_UNDERSHIRT, new PickerControlEntry (TEX_UPPER_UNDERSHIRT, "Color/Tint" ));
        addEntry ( TEX_LOWER_UNDERPANTS, new PickerControlEntry (TEX_LOWER_UNDERPANTS, "Color/Tint" ));
        addEntry ( TEX_HEAD_TATTOO, new PickerControlEntry(TEX_HEAD_TATTOO, "Color/Tint" ));
}

LLEditWearableDictionary::TextureCtrls::TextureCtrls()
{
        addEntry ( TEX_HEAD_BODYPAINT,  new PickerControlEntry (TEX_HEAD_BODYPAINT,  "Head Tattoos", LLUUID::null, TRUE ));
        addEntry ( TEX_UPPER_BODYPAINT, new PickerControlEntry (TEX_UPPER_BODYPAINT, "Upper Tattoos", LLUUID::null, TRUE ));
        addEntry ( TEX_LOWER_BODYPAINT, new PickerControlEntry (TEX_LOWER_BODYPAINT, "Lower Tattoos", LLUUID::null, TRUE ));
        addEntry ( TEX_HAIR, new PickerControlEntry (TEX_HAIR, "Texture", LLUUID( gSavedSettings.getString( "UIImgDefaultHairUUID" ) ), FALSE ));
        addEntry ( TEX_EYES_IRIS, new PickerControlEntry (TEX_EYES_IRIS, "Iris", LLUUID( gSavedSettings.getString( "UIImgDefaultEyesUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_SHIRT, new PickerControlEntry (TEX_UPPER_SHIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultShirtUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_PANTS, new PickerControlEntry (TEX_LOWER_PANTS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultPantsUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_SHOES, new PickerControlEntry (TEX_LOWER_SHOES, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultShoesUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_SOCKS, new PickerControlEntry (TEX_LOWER_SOCKS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultSocksUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_JACKET, new PickerControlEntry (TEX_UPPER_JACKET, "Upper Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultJacketUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_JACKET, new PickerControlEntry (TEX_LOWER_JACKET, "Lower Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultJacketUUID" ) ), FALSE ));
        addEntry ( TEX_SKIRT, new PickerControlEntry (TEX_SKIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultSkirtUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_GLOVES, new PickerControlEntry (TEX_UPPER_GLOVES, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultGlovesUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_UNDERSHIRT, new PickerControlEntry (TEX_UPPER_UNDERSHIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultUnderwearUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_UNDERPANTS, new PickerControlEntry (TEX_LOWER_UNDERPANTS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultUnderwearUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_ALPHA, new PickerControlEntry (TEX_LOWER_ALPHA, "Lower Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE, "lower alpha texture invisible" ));
        addEntry ( TEX_UPPER_ALPHA, new PickerControlEntry (TEX_UPPER_ALPHA, "Upper Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE, "upper alpha texture invisible" ));
        addEntry ( TEX_HEAD_ALPHA, new PickerControlEntry (TEX_HEAD_ALPHA, "Head Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE, "head alpha texture invisible" ));
        addEntry ( TEX_EYES_ALPHA, new PickerControlEntry (TEX_EYES_ALPHA, "Eye Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE, "eye alpha texture invisible" ));
        addEntry ( TEX_HAIR_ALPHA, new PickerControlEntry (TEX_HAIR_ALPHA, "Hair Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE, "hair alpha texture invisible" ));
        addEntry ( TEX_LOWER_TATTOO, new PickerControlEntry (TEX_LOWER_TATTOO, "Lower Tattoo", LLUUID::null, TRUE ));
        addEntry ( TEX_UPPER_TATTOO, new PickerControlEntry (TEX_UPPER_TATTOO, "Upper Tattoo", LLUUID::null, TRUE ));
        addEntry ( TEX_HEAD_TATTOO, new PickerControlEntry (TEX_HEAD_TATTOO, "Head Tattoo", LLUUID::null, TRUE ));
}

LLEditWearableDictionary::PickerControlEntry::PickerControlEntry(ETextureIndex tex_index,
                                         const std::string name,
                                         const LLUUID default_image_id,
                                         const bool allow_no_texture,
										 const std::string checkbox_name) :
        LLDictionaryEntry(name),
        mTextureIndex(tex_index),
        mControlName(name),
        mDefaultImageId(default_image_id),
        mAllowNoTexture(allow_no_texture),
		mCheckboxName(checkbox_name)
{
}

// Helper functions.
static const texture_vec_t null_texture_vec;

// Specializations of this template function return a vector of texture indexes of particular control type
// (i.e. LLColorSwatchCtrl or LLTextureCtrl) which are contained in given WearableEntry.
template <typename T>
const texture_vec_t&
get_pickers_indexes(const LLEditWearableDictionary::WearableEntry *wearable_entry) { return null_texture_vec; }

// Specializations of this template function return picker control entry for particular control type.
template <typename T>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry (const ETextureIndex index) { return NULL; }

typedef boost::function<void(LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry*)> function_t;

typedef struct PickerControlEntryNamePredicate
{
        PickerControlEntryNamePredicate(const std::string name) : mName (name) {};
        bool operator()(const LLEditWearableDictionary::PickerControlEntry* entry) const
        {
                return (entry && entry->mName == mName);
        }
private:
        const std::string mName;
} PickerControlEntryNamePredicate;

// A full specialization of get_pickers_indexes for LLColorSwatchCtrl
template <>
const texture_vec_t&
get_pickers_indexes<LLColorSwatchCtrl> (const LLEditWearableDictionary::WearableEntry *wearable_entry)
{
        if (!wearable_entry)
        {
                llwarns << "could not get LLColorSwatchCtrl indexes for null wearable entry." << llendl;
                return null_texture_vec;
        }
        return wearable_entry->mColorSwatchCtrls;
}

// A full specialization of get_pickers_indexes for LLTextureCtrl
template <>
const texture_vec_t&
get_pickers_indexes<LLTextureCtrl> (const LLEditWearableDictionary::WearableEntry *wearable_entry)
{
        if (!wearable_entry)
        {
                llwarns << "could not get LLTextureCtrl indexes for null wearable entry." << llendl;
                return null_texture_vec;
        }
        return wearable_entry->mTextureCtrls;
}

// A full specialization of get_picker_entry for LLColorSwatchCtrl
template <>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry<LLColorSwatchCtrl> (const ETextureIndex index)
{
        return LLEditWearableDictionary::getInstance()->getColorSwatch(index);
}

// A full specialization of get_picker_entry for LLTextureCtrl
template <>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry<LLTextureCtrl> (const ETextureIndex index)
{
        return LLEditWearableDictionary::getInstance()->getTexturePicker(index);
}

template <typename CtrlType, class Predicate>
const LLEditWearableDictionary::PickerControlEntry*
find_picker_ctrl_entry_if(LLWearableType::EType type, const Predicate pred)
{
        const LLEditWearableDictionary::WearableEntry *wearable_entry
                = LLEditWearableDictionary::getInstance()->getWearable(type);
        if (!wearable_entry)
        {
                llwarns << "could not get wearable dictionary entry for wearable of type: " << type << llendl;
                return NULL;
        }
        const texture_vec_t& indexes = get_pickers_indexes<CtrlType>(wearable_entry);
        for (texture_vec_t::const_iterator
                         iter = indexes.begin(),
                         iter_end = indexes.end();
                 iter != iter_end; ++iter)
        {
                const ETextureIndex te = *iter;
                const LLEditWearableDictionary::PickerControlEntry*     entry
                        = get_picker_entry<CtrlType>(te);
                if (!entry)
                {
                        llwarns << "could not get picker dictionary entry (" << te << ") for wearable of type: " << type << llendl;
                        continue;
                }
                if (pred(entry))
                {
                        return entry;
                }
        }
        return NULL;
}

template <typename CtrlType>
void
for_each_picker_ctrl_entry(LLPanel* panel, LLWearableType::EType type, function_t fun)
{
        if (!panel)
        {
                llwarns << "the panel wasn't passed for wearable of type: " << type << llendl;
                return;
        }
        const LLEditWearableDictionary::WearableEntry *wearable_entry
                = LLEditWearableDictionary::getInstance()->getWearable(type);
        if (!wearable_entry)
        {
                llwarns << "could not get wearable dictionary entry for wearable of type: " << type << llendl;
                return;
        }
        const texture_vec_t& indexes = get_pickers_indexes<CtrlType>(wearable_entry);
        for (texture_vec_t::const_iterator
                         iter = indexes.begin(),
                         iter_end = indexes.end();
                 iter != iter_end; ++iter)
        {
                const ETextureIndex te = *iter;
                const LLEditWearableDictionary::PickerControlEntry*     entry
                        = get_picker_entry<CtrlType>(te);
                if (!entry)
                {
                        llwarns << "could not get picker dictionary entry (" << te << ") for wearable of type: " << type << llendl;
                        continue;
                }
                fun (panel, entry);
        }
}

// The helper functions for pickers management
static void init_color_swatch_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
				color_swatch_ctrl->setCommitCallback(boost::bind(&LLPanelEditWearable::onColorSwatchCommit, self, _1));
                // Can't get the color from the wearable here, since the wearable may not be set when this is called.
            	color_swatch_ctrl->setOriginal(LLColor4::white);
        }
}

static void init_texture_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
				texture_ctrl->setCommitCallback(boost::bind(&LLPanelEditWearable::onTexturePickerCommit, self, _1));
                texture_ctrl->setDefaultImageAssetID(entry->mDefaultImageId);
                texture_ctrl->setAllowNoTexture(entry->mAllowNoTexture);
                // Don't allow (no copy) or (notransfer) textures to be selected.
				texture_ctrl->setImmediateFilterPermMask(PERM_NONE);//PERM_COPY | PERM_TRANSFER);
				texture_ctrl->setNonImmediateFilterPermMask(PERM_NONE);//PERM_COPY | PERM_TRANSFER);
        }
		if(!entry->mCheckboxName.empty())
		{
			LLCheckBoxCtrl* checkbox_ctrl = panel->getChild<LLCheckBoxCtrl>(entry->mCheckboxName, true, false);
			if(checkbox_ctrl)
			{
				checkbox_ctrl->setCommitCallback(LLPanelEditWearable::onInvisibilityCommit, self);
				self->initPreviousTextureListEntry(entry->mTextureIndex);
			}
		}
}

static void update_color_swatch_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
                color_swatch_ctrl->set(self->getWearable()->getClothesColor(entry->mTextureIndex));
        }
}

static void update_texture_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
                LLUUID new_id;
                LLLocalTextureObject *lto = self->getWearable()->getLocalTextureObject(entry->mTextureIndex);
                if( lto && (lto->getID() != IMG_DEFAULT_AVATAR) )
                {
                        new_id = lto->getID();
                }
                else
                {
                        new_id = LLUUID::null;
                }
                LLUUID old_id = texture_ctrl->getImageAssetID();
                if (old_id != new_id)
                {
                        // texture has changed, close the floater to avoid DEV-22461
                        texture_ctrl->closeFloater();
                }
                texture_ctrl->setImageAssetID(new_id);
        }
}

static void set_enabled_color_swatch_ctrl(bool enabled, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
                color_swatch_ctrl->setEnabled(enabled);
				color_swatch_ctrl->setVisible(enabled);
        }
}

static void set_enabled_texture_ctrl(bool enabled, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
                texture_ctrl->setEnabled(enabled);
				texture_ctrl->setVisible(enabled);
		}
}

static void set_enabled_invisibility_ctrl(bool enabled, LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        if(!entry->mCheckboxName.empty())
		{
			LLCheckBoxCtrl* checkbox_ctrl = panel->getChild<LLCheckBoxCtrl>(entry->mCheckboxName, true, false);
			if(checkbox_ctrl)
			{
                checkbox_ctrl->setEnabled(enabled);
				checkbox_ctrl->setVisible(enabled);
				checkbox_ctrl->set(!gAgentAvatarp->isTextureVisible(entry->mTextureIndex, self->getWearable()));
			}
		}
}

class LLWearableSaveAsDialog : public LLModalDialog
{
private:
	std::string	mItemName;
	void		(*mCommitCallback)(LLWearableSaveAsDialog*,void*);
	void*		mCallbackUserData;

public:
	LLWearableSaveAsDialog( const std::string& desc, void(*commit_cb)(LLWearableSaveAsDialog*,void*), void* userdata )
		: LLModalDialog( LLStringUtil::null, 240, 100 ),
		  mCommitCallback( commit_cb ),
		  mCallbackUserData( userdata )
	{
		LLUICtrlFactory::getInstance()->buildFloater(this, "floater_wearable_save_as.xml");
		
		childSetAction("Save", LLWearableSaveAsDialog::onSave, this );
		childSetAction("Cancel", LLWearableSaveAsDialog::onCancel, this );

		childSetTextArg("name ed", "[DESC]", desc);
	}

	virtual void startModal()
	{
		LLModalDialog::startModal();
		LLLineEditor* edit = getChild<LLLineEditor>("name ed");
		if (!edit) return;
		edit->setFocus(TRUE);
		edit->selectAll();
	}

	const std::string& getItemName() { return mItemName; }

	static void onSave( void* userdata )
	{
		LLWearableSaveAsDialog* self = (LLWearableSaveAsDialog*) userdata;
		self->mItemName = self->childGetValue("name ed").asString();
		LLStringUtil::trim(self->mItemName);
		if( !self->mItemName.empty() )
		{
			if( self->mCommitCallback )
			{
				self->mCommitCallback( self, self->mCallbackUserData );
			}
			self->close(); // destroys this object
		}
	}

	static void onCancel( void* userdata )
	{
		LLWearableSaveAsDialog* self = (LLWearableSaveAsDialog*) userdata;
		self->close(); // destroys this object
	}
};

LLPanelEditWearable::LLPanelEditWearable( LLWearableType::EType type )

	: LLPanel( LLWearableType::getTypeLabel( type ) ),
	  mType( type )
{
}
LLPanelEditWearable::~LLPanelEditWearable()
{
}

BOOL LLPanelEditWearable::postBuild()
{
	std::string icon_name = LLInventoryIcon::getIconName(LLWearableType::getAssetType( mType ),LLInventoryType::IT_WEARABLE,mType,FALSE);

	childSetValue("icon", icon_name);

	childSetAction("Create New", LLPanelEditWearable::onBtnCreateNew, this );

	// If PG, can't take off underclothing or shirt
	mCanTakeOff =
		LLWearableType::getAssetType( mType ) == LLAssetType::AT_CLOTHING &&
		!( gAgent.isTeen() && (mType == LLWearableType::WT_UNDERSHIRT || mType == LLWearableType::WT_UNDERPANTS) );
	childSetVisible("Take Off", mCanTakeOff);
	childSetAction("Take Off", LLPanelEditWearable::onBtnTakeOff, this );

	LLUICtrl* sex_radio = getChild<LLUICtrl>("sex radio", true, false);
	if(sex_radio)
	{
		sex_radio->setCommitCallback(boost::bind(&LLPanelEditWearable::onCommitSexChange,this));
	}

	childSetAction("Save",  &LLPanelEditWearable::onBtnSave, (void*)this );

	childSetAction("Save As", &LLPanelEditWearable::onBtnSaveAs, (void*)this );

	childSetAction("Revert", &LLPanelEditWearable::onRevertButtonClicked, (void*)this );

        {
                const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(mType);
                if (!wearable_entry)
                {
                        llwarns << "could not get wearable dictionary entry for wearable of type: " << mType << llendl;
                }
                U8 num_subparts = wearable_entry->mSubparts.size();
        
                for (U8 index = 0; index < num_subparts; ++index)
                {
                        // dive into data structures to get the panel we need
                        ESubpart subpart_e = wearable_entry->mSubparts[index];
                        const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);
        
                        if (!subpart_entry)
                        {
                                llwarns << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << llendl;
                                continue;
                        }

						if(!subpart_entry->mButtonName.empty())
						{
							llinfos << "Finding button " << subpart_entry->mButtonName << llendl;
							llassert_always(getChild<LLButton>(subpart_entry->mButtonName,true,false));
							childSetAction(subpart_entry->mButtonName, &LLPanelEditWearable::onBtnSubpart, (void*)subpart_e);
						}
                }	
				// initialize texture and color picker controls
    			for_each_picker_ctrl_entry <LLColorSwatchCtrl> (this, mType, boost::bind(init_color_swatch_ctrl, this, _1, _2));
				for_each_picker_ctrl_entry <LLTextureCtrl>     (this, mType, boost::bind(init_texture_ctrl, this, _1, _2));
	}
	return TRUE;
}

BOOL LLPanelEditWearable::isDirty() const
{
	LLWearable* wearable = getWearable();
	return wearable && wearable->isDirty();
}

void LLPanelEditWearable::draw()
{
	if( gFloaterCustomize->isMinimized() )
	{
		return;
	}

	LLVOAvatar* avatar = gAgentAvatarp;
	if( !avatar )
	{
		return;
	}

	LLWearable* wearable = gAgentWearables.getWearable( mType, 0 );	// TODO: MULTI-WEARABLE
	BOOL has_wearable = (wearable != NULL );
	BOOL is_dirty = isDirty();
	BOOL is_modifiable = FALSE;
	BOOL is_copyable = FALSE;
	BOOL is_complete = FALSE;
	LLViewerInventoryItem* item;
	item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(mType, 0);	// TODO: MULTI-WEARABLE
	if(item)
	{
		const LLPermissions& perm = item->getPermissions();
		is_modifiable = perm.allowModifyBy(gAgent.getID(), gAgent.getGroupID());
		is_copyable = perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID());
		is_complete = item->isComplete();
	}

	childSetEnabled("Save", is_modifiable && is_complete && has_wearable && is_dirty);
	childSetEnabled("Save As", is_copyable && is_complete && has_wearable);
	childSetEnabled("Revert", has_wearable && is_dirty );
	childSetEnabled("Take Off",  has_wearable );
	childSetVisible("Take Off", mCanTakeOff && has_wearable  );
	childSetVisible("Create New", !has_wearable );

	childSetVisible("not worn instructions",  !has_wearable );
	childSetVisible("no modify instructions",  has_wearable && !is_modifiable);


	const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(mType);
	if (wearable_entry)
	{
		U8 num_subparts = wearable_entry->mSubparts.size();

		for (U8 index = 0; index < num_subparts; ++index)
        {
			// dive into data structures to get the panel we need
			ESubpart subpart_e = wearable_entry->mSubparts[index];
			const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);
			
			if (!subpart_entry)
            {
                    llwarns << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << llendl;
                    continue;
            }

			childSetVisible(subpart_entry->mButtonName,has_wearable);
			if( has_wearable && is_complete && is_modifiable )
			{
				childSetEnabled(subpart_entry->mButtonName, subpart_entry->mSex & avatar->getSex() );
			}
			else
			{
				childSetEnabled(subpart_entry->mButtonName, FALSE );
			}
		}
	}

	childSetVisible("square", !is_modifiable);

	childSetVisible("title", FALSE);
	childSetVisible("title_no_modify", FALSE);
	childSetVisible("title_not_worn", FALSE);
	childSetVisible("title_loading", FALSE);

	childSetVisible("path", FALSE);

	LLTextBox *av_height = getChild<LLTextBox>("avheight",FALSE,FALSE);
	if(av_height) //Only display this if the element exists
	{
		// Display the shape's nominal height.
		//
		// The value for avsize is the best available estimate from
		// measuring against prims.
		float avsize = avatar->mBodySize.mV[VZ] + .195;
		int inches = (int)(avsize / .0254f);
		int feet = inches / 12;
		inches %= 12;

		std::ostringstream avheight(std::ostringstream::trunc);
		avheight << std::fixed << std::setprecision(2) << avsize << " m ("
			<< feet << "' " << inches << "\")";
		av_height->setVisible(TRUE);
		av_height->setTextArg("[AVHEIGHT]",avheight.str());		
	}
	
	if(has_wearable && !is_modifiable)
	{
		// *TODO:Translate
		childSetVisible("title_no_modify", TRUE);
		childSetTextArg("title_no_modify", "[DESC]", std::string(LLWearableType::getTypeLabel( mType )));
		
		hideTextureControls();
	}
	else if(has_wearable && !is_complete)
	{
		// *TODO:Translate
		childSetVisible("title_loading", TRUE);
		childSetTextArg("title_loading", "[DESC]", std::string(LLWearableType::getTypeLabel( mType )));
			
		std::string path;
		const LLUUID& item_id = gAgentWearables.getWearableItemID( wearable->getType(), 0 );	// TODO: MULTI-WEARABLE
		append_path(item_id, path);
		childSetVisible("path", TRUE);
		childSetTextArg("path", "[PATH]", path);

		hideTextureControls();
	}
	else if(has_wearable && is_modifiable)
	{
		childSetVisible("title", TRUE);
		childSetTextArg("title", "[DESC]", wearable->getName() );

		std::string path;
		const LLUUID& item_id = gAgentWearables.getWearableItemID( wearable->getType(), 0 );	// TODO: MULTI-WEARABLE
		append_path(item_id, path);
		childSetVisible("path", TRUE);
		childSetTextArg("path", "[PATH]", path);

		for_each_picker_ctrl_entry <LLColorSwatchCtrl>	(this, mType, boost::bind(update_color_swatch_ctrl, this, _1, _2));
		for_each_picker_ctrl_entry <LLTextureCtrl>    	(this, mType, boost::bind(update_texture_ctrl, this, _1, _2));
		for_each_picker_ctrl_entry <LLColorSwatchCtrl>	(this, mType, boost::bind(set_enabled_color_swatch_ctrl, is_complete, _1, _2));
        for_each_picker_ctrl_entry <LLTextureCtrl>		(this, mType, boost::bind(set_enabled_texture_ctrl, is_complete, _1, _2));
		for_each_picker_ctrl_entry <LLTextureCtrl>		(this, mType, boost::bind(set_enabled_invisibility_ctrl, is_copyable && is_complete, this, _1, _2));
	}
	else
	{
		// *TODO:Translate
		childSetVisible("title_not_worn", TRUE);
		childSetTextArg("title_not_worn", "[DESC]", std::string(LLWearableType::getTypeLabel( mType )));

		hideTextureControls();
	}
	
	childSetVisible("icon", has_wearable && is_modifiable);

	LLPanel::draw();
}

void LLPanelEditWearable::setVisible(BOOL visible)
{
	LLPanel::setVisible( visible );
	if( !visible )
	{
		for_each_picker_ctrl_entry <LLColorSwatchCtrl> (this, mType, boost::bind(set_enabled_color_swatch_ctrl, FALSE, _1, _2));
	}
}

// static
void LLPanelEditWearable::setWearable(LLWearable* wearable, U32 perm_mask, BOOL is_complete)
{
	if( wearable )
	{
		setUIPermissions(perm_mask, is_complete);
		if (mType == LLWearableType::WT_ALPHA)
		{
			initPreviousTextureList();
		}
	}
}

// static
void LLPanelEditWearable::onRevertButtonClicked( void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	gAgentWearables.revertWearable( self->mType, 0 );	// TODO: MULTI-WEARABLE
}

void LLPanelEditWearable::onBtnSave( void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	gAgentWearables.saveWearable( self->mType, 0 );	// TODO: MULTI-WEARABLE
}

// static
void LLPanelEditWearable::onBtnSaveAs( void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLWearable* wearable = gAgentWearables.getWearable( self->getType(), 0 );	// TODO: MULTI-WEARABLE
	if( wearable )
	{
		LLWearableSaveAsDialog* save_as_dialog = new LLWearableSaveAsDialog( wearable->getName(), onSaveAsCommit, self );
		save_as_dialog->startModal();
		// LLWearableSaveAsDialog deletes itself.
	}
}

// static
void LLPanelEditWearable::onSaveAsCommit( LLWearableSaveAsDialog* save_as_dialog, void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLVOAvatar* avatar = gAgentAvatarp;
	if( avatar )
	{
		gAgentWearables.saveWearableAs( self->getType(), 0, save_as_dialog->getItemName(), FALSE );	// TODO: MULTI-WEARABLE
	}
}

// static
void LLPanelEditWearable::onCommitSexChange()
{
	if (!isAgentAvatarValid()) return;

	LLWearableType::EType type = mType;	// TODO: MULTI-WEARABLE
    U32 index = 0;	// TODO: MULTI-WEARABLE

	if( !gAgentWearables.isWearableModifiable(type, index))
    {
		return;
    }
	
	LLViewerVisualParam* param = static_cast<LLViewerVisualParam*>(gAgentAvatarp->getVisualParam( "male" ));
	if( !param )
	{
		return;
	}

	bool is_new_sex_male = (gSavedSettings.getU32("AvatarSex") ? SEX_MALE : SEX_FEMALE) == SEX_MALE;
	LLWearable*     wearable = gAgentWearables.getWearable(type, index);
	if (wearable)
	{
		wearable->setVisualParamWeight(param->getID(), is_new_sex_male, FALSE);
	}
    param->setWeight( is_new_sex_male, FALSE );
	
	gAgentAvatarp->updateSexDependentLayerSets( FALSE );

	gAgentAvatarp->updateVisualParams();
		
	
	//if(!wearable)
	//{
	//	return;
	//}



	//wearable->setVisualParamWeight(param->getID(), (new_sex == SEX_MALE), TRUE);
	//wearable->writeToAvatar();
	//avatar->updateVisualParams();

	gFloaterCustomize->clearScrollingPanelList();

	// Assumes that we're in the "Shape" Panel.
	//self->setSubpart( SUBPART_SHAPE_WHOLE );
}


// static
void LLPanelEditWearable::onBtnCreateNew( void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLSD payload;
	payload["wearable_type"] = (S32)self->getType();
	LLNotificationsUtil::add("AutoWearNewClothing", LLSD(), payload, &onSelectAutoWearOption);
}

bool LLPanelEditWearable::onSelectAutoWearOption(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLVOAvatar* avatar = gAgentAvatarp;
	if(avatar)
	{
		// Create a new wearable in the default folder for the wearable's asset type.
		LLWearable* wearable = LLWearableList::instance().createNewWearable( (LLWearableType::EType)notification["payload"]["wearable_type"].asInteger() );
		LLAssetType::EType asset_type = wearable->getAssetType();

		LLUUID folder_id;
		// regular UI, items get created in normal folder
		folder_id = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(asset_type));

		// Only auto wear the new item if the AutoWearNewClothing checkbox is selected.
		LLPointer<LLInventoryCallback> cb = option == 0 ? 
			new WearOnAvatarCallback : NULL;
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
			folder_id, wearable->getTransactionID(), wearable->getName(), wearable->getDescription(),
			asset_type, LLInventoryType::IT_WEARABLE, wearable->getType(),
			wearable->getPermissions().getMaskNextOwner(), cb);
	}
	return false;
}


bool LLPanelEditWearable::textureIsInvisible(ETextureIndex te)
{
	const LLWearable* wearable = gAgentWearables.getWearable(mType, 0);	// TODO: MULTI-WEARABLE
	if(wearable)
	{
		const LLLocalTextureObject* lto = wearable->getLocalTextureObject(te);
		return (lto && lto->getID() == IMG_INVISIBLE);
	}
	return false;
}

LLWearable* LLPanelEditWearable::getWearable() const
{
	return gAgentWearables.getWearable(mType, 0);	// TODO: MULTI-WEARABLE
}


void LLPanelEditWearable::onTexturePickerCommit(const LLUICtrl* ctrl)
{
        const LLTextureCtrl* texture_ctrl = dynamic_cast<const LLTextureCtrl*>(ctrl);
        if (!texture_ctrl)
        {
                llwarns << "got commit signal from not LLTextureCtrl." << llendl;
                return;
        }

        if (getWearable())
        {
                LLWearableType::EType type = getWearable()->getType();
                const PickerControlEntryNamePredicate name_pred(texture_ctrl->getName());
                const LLEditWearableDictionary::PickerControlEntry* entry
                        = find_picker_ctrl_entry_if<LLTextureCtrl, PickerControlEntryNamePredicate>(type, name_pred);
                if (entry)
                {
                        // Set the new version
                        LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(texture_ctrl->getImageAssetID());
                        if( image->getID() == IMG_DEFAULT )
                        {
                                image = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT_AVATAR);
                        }
                        if (getWearable())
                        {
                                U32 index = gAgentWearables.getWearableIndex(getWearable());
                                gAgentAvatarp->setLocalTexture(entry->mTextureIndex, image, FALSE, index);
                                LLVisualParamHint::requestHintUpdates();
                                gAgentAvatarp->wearableUpdated(type, FALSE);
                        }
						if (mType == LLWearableType::WT_ALPHA && image->getID() != IMG_INVISIBLE)
						{
							mPreviousTextureList[entry->mTextureIndex] = image->getID();
						}	
				}
                else
                {
                        llwarns << "could not get texture picker dictionary entry for wearable of type: " << type << llendl;
                }
        }
}

void LLPanelEditWearable::onColorSwatchCommit(const LLUICtrl* base_ctrl )
{
		LLColorSwatchCtrl* ctrl = (LLColorSwatchCtrl*) base_ctrl;

        if (getWearable())
        {
                LLWearableType::EType type = getWearable()->getType();
                const PickerControlEntryNamePredicate name_pred(ctrl->getName());
                const LLEditWearableDictionary::PickerControlEntry* entry
                        = find_picker_ctrl_entry_if<LLColorSwatchCtrl, PickerControlEntryNamePredicate>(type, name_pred);
                if (entry)
                {
                        const LLColor4& old_color = getWearable()->getClothesColor(entry->mTextureIndex);
                        const LLColor4& new_color = LLColor4(ctrl->getValue());
                        if( old_color != new_color )
                        {
                                getWearable()->setClothesColor(entry->mTextureIndex, new_color, TRUE);
                                LLVisualParamHint::requestHintUpdates();
                                gAgentAvatarp->wearableUpdated(getWearable()->getType(), FALSE);
                        }
                }
                else
                {
                        llwarns << "could not get color swatch dictionary entry for wearable of type: " << type << llendl;
                }
        }
}

ESubpart LLPanelEditWearable::getDefaultSubpart()
{
	switch( mType )
	{
		case LLWearableType::WT_SHAPE:		return SUBPART_SHAPE_WHOLE;
		case LLWearableType::WT_SKIN:		return SUBPART_SKIN_COLOR;
		case LLWearableType::WT_HAIR:		return SUBPART_HAIR_COLOR;
		case LLWearableType::WT_EYES:		return SUBPART_EYES;
		case LLWearableType::WT_SHIRT:		return SUBPART_SHIRT;
		case LLWearableType::WT_PANTS:		return SUBPART_PANTS;
		case LLWearableType::WT_SHOES:		return SUBPART_SHOES;
		case LLWearableType::WT_SOCKS:		return SUBPART_SOCKS;
		case LLWearableType::WT_JACKET:		return SUBPART_JACKET;
		case LLWearableType::WT_GLOVES:		return SUBPART_GLOVES;
		case LLWearableType::WT_UNDERSHIRT:	return SUBPART_UNDERSHIRT;
		case LLWearableType::WT_UNDERPANTS:	return SUBPART_UNDERPANTS;
		case LLWearableType::WT_SKIRT:		return SUBPART_SKIRT;
		case LLWearableType::WT_ALPHA:		return SUBPART_ALPHA;
		case LLWearableType::WT_TATTOO:		return SUBPART_TATTOO;
		case LLWearableType::WT_PHYSICS:	return SUBPART_PHYSICS_BELLY_UPDOWN;

		default:	llassert(0);		return SUBPART_SHAPE_WHOLE;
	}
}




void LLPanelEditWearable::hideTextureControls()
{
	for_each_picker_ctrl_entry <LLTextureCtrl>     (this, mType, boost::bind(set_enabled_texture_ctrl, FALSE, _1, _2));
	for_each_picker_ctrl_entry <LLColorSwatchCtrl> (this, mType, boost::bind(set_enabled_color_swatch_ctrl, FALSE, _1, _2));
	for_each_picker_ctrl_entry <LLTextureCtrl>		(this, mType, boost::bind(set_enabled_invisibility_ctrl, FALSE, this, _1, _2));
}


void LLPanelEditWearable::switchToDefaultSubpart()
{
	setSubpart( getDefaultSubpart() );
}



void LLPanelEditWearable::setUIPermissions(U32 perm_mask, BOOL is_complete)
{
	BOOL is_copyable = (perm_mask & PERM_COPY) ? TRUE : FALSE;
	BOOL is_modifiable = (perm_mask & PERM_MODIFY) ? TRUE : FALSE;

	childSetEnabled("Save", is_modifiable && is_complete);
	childSetEnabled("Save As", is_copyable && is_complete);
	childSetEnabled("sex radio", is_modifiable && is_complete);
	for_each_picker_ctrl_entry <LLTextureCtrl>		(this, mType, boost::bind(set_enabled_texture_ctrl, is_copyable && is_modifiable && is_complete, _1, _2));
	for_each_picker_ctrl_entry <LLColorSwatchCtrl>	(this, mType, boost::bind(set_enabled_color_swatch_ctrl, is_modifiable && is_complete, _1, _2));
	for_each_picker_ctrl_entry <LLTextureCtrl>		(this, mType, boost::bind(set_enabled_invisibility_ctrl, is_copyable && is_modifiable && is_complete, this, _1, _2));
}

// static
void LLPanelEditWearable::onBtnSubpart(void* userdata)
{
	LLFloaterCustomize* floater_customize = gFloaterCustomize;
	if (!floater_customize) return;
	LLPanelEditWearable* self = floater_customize->getCurrentWearablePanel();
	if (!self) return;
	ESubpart subpart = (ESubpart) (intptr_t)userdata;
	self->setSubpart( subpart );
}

void LLPanelEditWearable::setSubpart( ESubpart subpart )
{
	mCurrentSubpart = subpart;

	const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(mType);
	if (wearable_entry)
	{
		U8 num_subparts = wearable_entry->mSubparts.size();

		for (U8 index = 0; index < num_subparts; ++index)
        {
			// dive into data structures to get the panel we need
			ESubpart subpart_e = wearable_entry->mSubparts[index];
			const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);
			
			if (subpart_entry)
            {
				LLButton* btn = getChild<LLButton>(subpart_entry->mButtonName);
				{
					btn->setToggleState( subpart == subpart_e );
				}
			}
		}
	}

	const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart);
	if( subpart_entry )
	{
		// Update the thumbnails we display
		LLFloaterCustomize::param_map sorted_params;
		LLVOAvatar* avatar = gAgentAvatarp;
		ESex avatar_sex = avatar->getSex();
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(mType,0);	// TODO: MULTI-WEARABLE
		U32 perm_mask = 0x0;
		BOOL is_complete = FALSE;
		bool can_export = false;
		bool can_import = false;
		if(item)
		{
			perm_mask = item->getPermissions().getMaskOwner();
			is_complete = item->isComplete();
			
			if (subpart <= 18) // body parts only
			{
				can_import = true;

				if (is_complete && 
					gAgent.getID() == item->getPermissions().getOwner() &&
					gAgent.getID() == item->getPermissions().getCreator() &&
					(PERM_ITEM_UNRESTRICTED &
					perm_mask) == PERM_ITEM_UNRESTRICTED)
				{
					can_export = true;
				}
			}
		}
		setUIPermissions(perm_mask, is_complete);
		BOOL editable = ((perm_mask & PERM_MODIFY) && is_complete) ? TRUE : FALSE;
		
		for(LLViewerVisualParam* param = (LLViewerVisualParam *)avatar->getFirstVisualParam(); 
			param; 
			param = (LLViewerVisualParam *)avatar->getNextVisualParam())
		{
			if (param->getID() == -1
				|| !param->isTweakable() 
				|| param->getEditGroup() != subpart_entry->mEditGroup 
				|| !(param->getSex() & avatar_sex))
			{
				continue;
			}

			// negative getDisplayOrder() to make lowest order the highest priority
			LLFloaterCustomize::param_map::value_type vt(-param->getDisplayOrder(), LLFloaterCustomize::editable_param(editable, param));
			llassert( sorted_params.find(-param->getDisplayOrder()) == sorted_params.end() );  // Check for duplicates
			sorted_params.insert(vt);
		}
		gFloaterCustomize->generateVisualParamHints(NULL, sorted_params, mType != LLWearableType::WT_PHYSICS);
		gFloaterCustomize->updateScrollingPanelUI();
		gFloaterCustomize->childSetEnabled("Export", can_export);
		gFloaterCustomize->childSetEnabled("Import", can_import);

		// Update the camera
		gMorphView->setCameraTargetJoint( gAgentAvatarp->getJoint( subpart_entry->mTargetJoint ) );
		gMorphView->setCameraTargetOffset( subpart_entry->mTargetOffset );
		gMorphView->setCameraOffset( subpart_entry->mCameraOffset );
		gMorphView->setCameraDistToDefault();
		if (gSavedSettings.getBOOL("AppearanceCameraMovement"))
		{
			gMorphView->updateCamera();
		}
	}
}

// static
void LLPanelEditWearable::onBtnTakeOff( void* userdata )
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	
	LLWearable* wearable = gAgentWearables.getWearable( self->mType, 0 );	// TODO: MULTI-WEARABLE
	if( !wearable )
	{
		return;
	}

	gAgentWearables.removeWearable( self->mType, false, 0 );	// TODO: MULTI-WEARABLE
}

// static
void LLPanelEditWearable::onInvisibilityCommit(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLCheckBoxCtrl* checkbox_ctrl = (LLCheckBoxCtrl*) ctrl;
	if (!gAgentAvatarp)
	{
		return;
	}

	const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(self->mType);
	if (wearable_entry)
	{
		U8 num_textures = wearable_entry->mTextureCtrls.size();

		for (U8 index = 0; index < num_textures; ++index)
        {
			// dive into data structures to get the panel we need
			ETextureIndex texindex = wearable_entry->mTextureCtrls[index];
				
			const LLEditWearableDictionary::PickerControlEntry *tex_ctrl = LLEditWearableDictionary::getInstance()->getTexturePicker(texindex);
			
			if (tex_ctrl && tex_ctrl->mCheckboxName == checkbox_ctrl->getName())
            {
				bool new_invis_state = checkbox_ctrl->get();
				if (new_invis_state)
				{
					LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(IMG_INVISIBLE);
		
					LLWearable* wearable = gAgentWearables.getWearable(self->mType,0);	// TODO: MULTI-WEARABLE
					const LLLocalTextureObject* lto = wearable ? wearable->getLocalTextureObject(texindex) : NULL;

					if(lto)
					{
						self->mPreviousTextureList[(S32)texindex] = lto->getID();
					}
					if(wearable)
					{
						LLLocalTextureObject new_lto(image, IMG_INVISIBLE);
						wearable->setLocalTextureObject(texindex, new_lto);
						wearable->writeToAvatar();
						gAgentAvatarp->wearableUpdated(self->mType, FALSE);
					}
		
				}
				else
				{
					// Try to restore previous texture, if any.
					LLUUID prev_id = self->mPreviousTextureList[(S32)texindex];
					if (prev_id.isNull() || (prev_id == IMG_INVISIBLE))
					{
						prev_id = LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID"));
					}
					if (prev_id.notNull())
					{
						LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(prev_id);
						LLWearable* wearable = gAgentWearables.getWearable(self->mType,0);	// TODO: MULTI-WEARABLE
						if(wearable)
						{
							LLLocalTextureObject new_lto(image, prev_id);
							wearable->setLocalTextureObject(texindex, new_lto);
							wearable->writeToAvatar();
							gAgentAvatarp->wearableUpdated(self->mType, FALSE);
						}
					}
				}
            }
		}
	}
}

void LLPanelEditWearable::initPreviousTextureList()
{
	initPreviousTextureListEntry(TEX_LOWER_ALPHA);
	initPreviousTextureListEntry(TEX_UPPER_ALPHA);
	initPreviousTextureListEntry(TEX_HEAD_ALPHA);
	initPreviousTextureListEntry(TEX_EYES_ALPHA);
	initPreviousTextureListEntry(TEX_LOWER_ALPHA);
}
void LLPanelEditWearable::initPreviousTextureListEntry(ETextureIndex te)
{
	if(!getWearable())
		return;
        LLLocalTextureObject *lto = getWearable()->getLocalTextureObject(te);
        if (lto)
        {
                mPreviousTextureList[te] = lto->getID();
        }
}
