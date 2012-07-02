/** 
 * @file llpaneleditwearable.h
 * @brief A LLPanel dedicated to the editing of wearables.
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

#ifndef LL_LLPANELEDITWEARABLE_H
#define LL_LLPANELEDITWEARABLE_H

#include "llpanel.h"
#include "llscrollingpanellist.h"
#include "llmodaldialog.h"
#include "llvoavatardefines.h"
#include "llwearabletype.h"

class LLAccordionCtrl;
class LLCheckBoxCtrl;
class LLWearable;
class LLTextBox;
class LLViewerInventoryItem;
class LLViewerVisualParam;
class LLVisualParamHint;
class LLViewerJointMesh;
class LLAccordionCtrlTab;
class LLJoint;
class LLLineEditor;
class LLSubpart;
class LLWearableSaveAsDialog;

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

class LLPanelEditWearable : public LLPanel
{
public:
	LLPanelEditWearable( LLWearableType::EType type );
	virtual ~LLPanelEditWearable();

	/*virtual*/ BOOL 		postBuild();
	/*virtual*/ BOOL		isDirty() const;	// LLUICtrl
	/*virtual*/ void		draw();	
	
	// changes camera angle to default for selected subpart
	void				changeCamera(U8 subpart);

	const std::string&	getLabel()	{ return LLWearableType::getTypeLabel( mType ); }
	LLWearableType::EType		getType() const{ return mType; }
	LLWearable* 		getWearable() 	const;

	void			onTabChanged(LLUICtrl* ctrl);
	bool			onTabPrecommit();

	void				setWearableIndex(S32 index);	
	void				refreshWearables(bool force_immediate);
	void 				wearablesChanged();

	void				saveChanges(bool force_save_as = false, std::string new_name = std::string());
	
	void 				setUIPermissions(U32 perm_mask, BOOL is_complete);

	void				hideTextureControls();
	void				revertChanges();

	void				showDefaultSubpart();

	void 				updateScrollingPanelList();

	static void			onRevertButtonClicked( void* userdata );
	void				onCommitSexChange();
		
	virtual void		setVisible( BOOL visible );

	typedef std::pair<BOOL, LLViewerVisualParam*> editable_param;
	typedef std::map<F32, editable_param> value_map_t;
	
	void				updateScrollingPanelUI();
	void				getSortedParams(value_map_t &sorted_params, const std::string &edit_group, bool editable);
	void				buildParamList(LLScrollingPanelList *panel_list, value_map_t &sorted_params);
		
	// Callbacks
	static void			onBtnSubpart( void* userdata );
	static void			onBtnTakeOff( void* userdata );
	static void			onBtnSave( void* userdata );

	static void			onBtnSaveAs( void* userdata );
	static void			onSaveAsCommit( LLWearableSaveAsDialog* save_as_dialog, void* userdata );

	static void			onBtnTakeOffDialog( S32 option, void* userdata );
	static void			onBtnCreateNew( void* userdata );
	static bool			onSelectAutoWearOption(const LLSD& notification, const LLSD& response);

	void				onColorSwatchCommit(const LLUICtrl*);
	void				onTexturePickerCommit(const LLUICtrl*);
	
	//alpha mask checkboxes
	void configureAlphaCheckbox(LLVOAvatarDefines::ETextureIndex te, const std::string& name);
	void onInvisibilityCommit(LLCheckBoxCtrl* checkbox_ctrl, LLVOAvatarDefines::ETextureIndex te);
	void updateAlphaCheckboxes();
	void initPreviousAlphaTextures();
	void initPreviousAlphaTextureEntry(LLVOAvatarDefines::ETextureIndex te);
	
private:

	LLWearableType::EType		mType;
	BOOL				mCanTakeOff;
	typedef std::map<std::string, LLVOAvatarDefines::ETextureIndex> string_texture_index_map_t;
	string_texture_index_map_t mAlphaCheckbox2Index;

	typedef std::map<LLVOAvatarDefines::ETextureIndex, LLUUID> s32_uuid_map_t;
	s32_uuid_map_t mPreviousAlphaTexture;
	U32					mCurrentSubpart;
	U32					mCurrentIndex;
	LLWearable*			mCurrentWearable;
	LLWearable*			mPendingWearable;	//For SaveAs. There's a period where the old wearable will be removed, but the new one will still be pending, 
											//so this is needed to retain focus on this wearables tab over the messy transition.
	bool				mPendingRefresh;	//LLAgentWearables::setWearableOutfit fires a buttload of remove/wear calls which spams wearablesChanged
											//a bazillion pointless (and not particularly valid) times. Deferring to draw effectively sorts it all out.
public:
	LLModalDialog*		mActiveModal;
};

#endif
