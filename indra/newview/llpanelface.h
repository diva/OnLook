/** 
 * @file llpanelface.h
 * @brief Panel in the tools floater for editing face textures, colors, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLPANELFACE_H
#define LL_LLPANELFACE_H

#include "v4color.h"
#include "llpanel.h"
#include "llmaterial.h"
#include "llmaterialmgr.h"
#include "lltextureentry.h"
#include "llselectmgr.h"

class LLButton;
class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLSpinCtrl;
class LLTextBox;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerObject;
class LLFloater;
class LLMaterialID;

// Represents an edit for use in replicating the op across one or more materials in the selection set.
//
// The apply function optionally performs the edit which it implements
// as a functor taking Data that calls member func MaterialFunc taking SetValueType
// on an instance of the LLMaterial class.
//
// boost who?
//
template<
	typename DataType,
	typename SetValueType,
	void (LLMaterial::*MaterialEditFunc)(SetValueType data) >
class LLMaterialEditFunctor
{
public:
	LLMaterialEditFunctor(const DataType& data) : _data(data) {}
	virtual ~LLMaterialEditFunctor() {}
	virtual void apply(LLMaterialPtr& material) { (material->*(MaterialEditFunc))(_data); }
	DataType _data;
};

template<
	typename DataType,
	DataType (LLMaterial::*MaterialGetFunc)() >
class LLMaterialGetFunctor
{
public:
	LLMaterialGetFunctor() {}
	virtual DataType get(LLMaterialPtr& material) { return (material->*(MaterialGetFunc)); }
};

template<
	typename DataType,
	DataType (LLTextureEntry::*TEGetFunc)() >
class LLTEGetFunctor
{
public:
	LLTEGetFunctor() {}
	virtual DataType get(LLTextureEntry* entry) { return (entry*(TEGetFunc)); }
};

class LLPanelFace : public LLPanel
{
public:
	virtual BOOL	postBuild();
	LLPanelFace(const std::string& name);
	virtual ~LLPanelFace();

	void			refresh();
	void			setMediaURL(const std::string& url);
	void			setMediaType(const std::string& mime_type);

	LLMaterialPtr createDefaultMaterial(LLMaterialPtr current_material)
	{
		LLMaterialPtr new_material(!current_material.isNull() ? new LLMaterial(current_material->asLLSD()) : new LLMaterial());
		llassert_always(new_material);

		// Preserve old diffuse alpha mode or assert correct default blend mode as appropriate for the alpha channel content of the diffuse texture
		//
		new_material->setDiffuseAlphaMode(current_material.isNull() ? (isAlpha() ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE) : current_material->getDiffuseAlphaMode());
		return new_material;
	}

	LLRender::eTexIndex getTextureChannelToEdit();

protected:
	void			getState();

	void			sendTexture();			// applies and sends texture
	void			sendTextureInfo();		// applies and sends texture scale, offset, etc.
	void			sendColor();			// applies and sends color
	void			sendAlpha();			// applies and sends transparency
	void			sendBump(U32 bumpiness);				// applies and sends bump map
	void			sendTexGen();				// applies and sends bump map
	void			sendShiny(U32 shininess);			// applies and sends shininess
	void			sendFullbright();		// applies and sends full bright
	void			sendGlow();
	void			sendMedia();

	// this function is to return TRUE if the drag should succeed.
	static BOOL onDragTexture(LLUICtrl* ctrl, LLInventoryItem* item);

	void 	onCommitTexture(const LLSD& data);
	void 	onCancelTexture(const LLSD& data);
	void 	onSelectTexture(const LLSD& data);
	void 	onCommitSpecularTexture(const LLSD& data);
	void 	onCancelSpecularTexture(const LLSD& data);
	void 	onSelectSpecularTexture(const LLSD& data);
	void 	onCommitNormalTexture(const LLSD& data);
	void 	onCancelNormalTexture(const LLSD& data);
	void 	onSelectNormalTexture(const LLSD& data);
	void 	onCommitShinyColor(const LLSD& data);
	void 	onCancelColor(const LLSD& data);
	void 	onSelectColor(const LLSD& data);

	void 	onCloseTexturePicker(const LLSD& data);

	// Make UI reflect state of currently selected material (refresh)
	// and UI mode (e.g. editing normal map v diffuse map)
	//
	void updateUI();

	// Convenience func to determine if all faces in selection have
	// identical planar texgen settings during edits
	//
	bool isIdenticalPlanarTexgen();

	// Callback funcs for individual controls
	//
	void			onCommitTextureInfo();

	void			onCommitMaterialBumpyScaleX(const LLSD& value);
	void			onCommitMaterialBumpyScaleY(const LLSD& value);
	void			onCommitMaterialBumpyRot(const LLSD& value);

	void			onCommitMaterialShinyScaleX(const LLSD& value);
	void			onCommitMaterialShinyScaleY(const LLSD& value);
	void			onCommitMaterialShinyRot(const LLSD& value);


	void			onCommitMaterialsMedia();
	void			onCommitMaterialType();
	void			onCommitAlphaMode();
	void			onCommitPlanarAlign();
	void			onCommitRepeatsPerMeter(LLUICtrl* repeats_ctrl);
	void			onClickAutoFix();

	static F32      valueGlow(LLViewerObject* object, S32 face);
	void			onClickCopy();
	void			onClickPaste();
	// <FS:CR> Build tool enhancements
	void		onClickMapsSync();
	void		alignMaterialsProperties();
	void		onCommitFlip(bool flip_x);
	// </FS>

private:

	bool		isAlpha() { return mIsAlpha; }

	// Convenience funcs to keep the visual flack to a minimum
	//
	LLUUID	getCurrentNormalMap();
	LLUUID	getCurrentSpecularMap();
	U8			getCurrentDiffuseAlphaMode();

	// Update visibility of controls to match current UI mode
	// (e.g. materials vs media editing)
	//
	// Do NOT call updateUI from within this function.
	//
	void updateVisibility();

	// Make material(s) reflect current state of UI (apply edit)
	//
	void updateMaterial();

	// Hey look everyone, a type-safe alternative to copy and paste! :)
	//

	// Update material parameters by applying 'edit_func' to selected TEs
	//
	template<
		typename DataType,
		typename SetValueType,
		void (LLMaterial::*MaterialEditFunc)(SetValueType data) >
	static void edit(LLPanelFace* p, DataType data)
	{
		LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc > edit(data);
		struct LLSelectedTEEditMaterial : public LLSelectedTEMaterialFunctor
		{
			LLSelectedTEEditMaterial(LLPanelFace* panel, LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc >* editp) : _panel(panel), _edit(editp) {}
			virtual ~LLSelectedTEEditMaterial() {};
			virtual LLMaterialPtr apply(LLViewerObject* object, S32 face, LLTextureEntry* tep, LLMaterialPtr& current_material)
			{
				if (_edit)
				{
					LLMaterialPtr new_material = _panel->createDefaultMaterial(current_material);
					llassert_always(new_material);

					// Determine correct alpha mode for current diffuse texture
					// (i.e. does it have an alpha channel that makes alpha mode useful)
					//
					// _panel->isAlpha() "lies" when one face has alpha and the rest do not (NORSPEC-329)
					// need to get per-face answer to this question for sane alpha mode retention on updates.
					//
					bool is_alpha_face = object->isImageAlphaBlended(face);

					// need to keep this original answer for valid comparisons in logic below
					//
					U8 original_default_alpha_mode = is_alpha_face ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

					U8 default_alpha_mode = original_default_alpha_mode;

					if (!current_material.isNull())
					{
						default_alpha_mode = current_material->getDiffuseAlphaMode();
					}

					// Ensure we don't inherit the default of blend by accident...
					// this will be stomped by a legit request to change the alpha mode by the apply() below
					//
					new_material->setDiffuseAlphaMode(default_alpha_mode);

					// Do "It"!
					//
					_edit->apply(new_material);

					U32		new_alpha_mode			= new_material->getDiffuseAlphaMode();
					LLUUID	new_normal_map_id		= new_material->getNormalID();
					LLUUID	new_spec_map_id		= new_material->getSpecularID();

					if ((new_alpha_mode == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND) && !is_alpha_face)
					{
						new_alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
						new_material->setDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_NONE);
					}

					bool is_default_blend_mode		= (new_alpha_mode == original_default_alpha_mode);
					bool is_need_material			= !is_default_blend_mode || !new_normal_map_id.isNull() || !new_spec_map_id.isNull();

					if (!is_need_material)
					{
						LL_DEBUGS("Materials") << "Removing material from object " << object->getID() << " face " << face << LL_ENDL;
						LLMaterialMgr::getInstance()->remove(object->getID(),face);
						new_material = NULL;
					}
					else
					{
						LL_DEBUGS("Materials") << "Putting material on object " << object->getID() << " face " << face << ", material: " << new_material->asLLSD() << LL_ENDL;
						LLMaterialMgr::getInstance()->put(object->getID(),face,*new_material);
					}

					object->setTEMaterialParams(face, new_material);
					return new_material;
				}
				return NULL;
			}
			LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc >*	_edit;
			LLPanelFace*																			_panel;
		} editor(p, &edit);
		LLSelectMgr::getInstance()->selectionSetMaterialParams(&editor);
	}

	template<
		typename DataType,
		typename ReturnType,
		ReturnType (LLMaterial::* const MaterialGetFunc)() const  >
	static void getTEMaterialValue(DataType& data_to_return, bool& identical,DataType default_value)
	{
		DataType data_value = DataType();
		struct GetTEMaterialVal : public LLSelectedTEGetFunctor<DataType>
		{
			GetTEMaterialVal(DataType default_value) : _default(default_value) {}
			virtual ~GetTEMaterialVal() {}

			DataType get(LLViewerObject* object, S32 face)
			{
				DataType ret = _default;
				LLMaterialPtr material_ptr;
				LLTextureEntry* tep = object ? object->getTE(face) : NULL;
				if (tep)
				{
					material_ptr = tep->getMaterialParams();
					if (!material_ptr.isNull())
					{
						ret = (material_ptr->*(MaterialGetFunc))();
					}
				}
				return ret;
			}
			DataType _default;
		} GetFunc(default_value);
		identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &GetFunc, data_value);
		data_to_return = data_value;
	}

	template<
		typename DataType,
		typename ReturnType, // some kids just have to different...
		ReturnType (LLTextureEntry::* const TEGetFunc)() const >
	static void getTEValue(DataType& data_to_return, bool& identical, DataType default_value)
	{
		DataType data_value = DataType();
		struct GetTEVal : public LLSelectedTEGetFunctor<DataType>
		{
			GetTEVal(DataType default_value) : _default(default_value) {}
			virtual ~GetTEVal() {}

			DataType get(LLViewerObject* object, S32 face) {
				LLTextureEntry* tep = object ? object->getTE(face) : NULL;
				return tep ? ((tep->*(TEGetFunc))()) : _default;
			}
			DataType _default;
		} GetTEValFunc(default_value);
		identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &GetTEValFunc, data_value );
		data_to_return = data_value;
	}

// <alchemy>
	friend struct LLPanelFaceSetTEFunctor; // Must access some of these
	// UI Widgets
	LLView* mMediaInfo;
	LLView* mMediaAdd;
	LLView* mMediaDelete;
	LLView* mLabelGlossy;
	LLView* mLabelEnvironment;
	LLView* mLabelShinyColor;
	LLView* mLabelAlphaMode;
	LLView* mLabelMaskCutoff;
	LLView* mLabelBumpy;
	LLView* mLabelShiny;
	LLView* mLabelColor;
	LLView* mLabelGlow;
	LLView* mLabelTexGen;
	LLComboBox* mComboShiny;
	LLComboBox* mComboBumpy;
	LLComboBox* mComboAlpha;
	LLSpinCtrl* mCtrlTexScaleU;
	LLUICtrl* mCtrlFlipTexScaleU;
	LLSpinCtrl* mCtrlTexScaleV;
	LLUICtrl* mCtrlFlipTexScaleV;
	LLSpinCtrl* mCtrlTexRot;
	LLUICtrl* mCtrlRpt;
	LLUICtrl* mCtrlPlanar;
	LLSpinCtrl* mCtrlTexOffsetU;
	LLSpinCtrl* mCtrlTexOffsetV;
	LLUICtrl* mBumpyScaleU;
	LLUICtrl* mBumpyScaleV;
	LLUICtrl* mBumpyRot;
	LLUICtrl* mBumpyOffsetU;
	LLUICtrl* mBumpyOffsetV;
	LLUICtrl* mShinyScaleU;
	LLUICtrl* mShinyScaleV;
	LLUICtrl* mShinyRot;
	LLUICtrl* mShinyOffsetU;
	LLUICtrl* mShinyOffsetV;
	LLUICtrl* mGlossyCtrl;
	LLUICtrl* mEnvironmentCtrl;
	LLUICtrl* mCtrlMaskCutoff;
	LLUICtrl* mCtrlAlign;
	LLUICtrl* mCtrlMapsSync;
	LLUICtrl* mCtrlCopy;
	LLUICtrl* mCtrlPaste;
	LLTextureCtrl*	mTextureCtrl;
	LLTextureCtrl*	mShinyTextureCtrl;
	LLTextureCtrl*	mBumpyTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;
	LLColorSwatchCtrl*	mShinyColorSwatch;

	LLComboBox*		mComboTexGen;
	LLComboBox*		mComboMatMedia;
	LLComboBox*		mComboMatType;

	LLCheckBoxCtrl*	mCheckFullbright;

	LLTextBox*		mLabelColorTransp;
	LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	LLSpinCtrl*     mCtrlGlow;
// </alchemy>

	// Update vis and enabling of specific subsets of controls based on material params
	// (e.g. hide the spec controls if no spec texture is applied)
	//
	void updateShinyControls(bool is_setting_texture = false, bool mess_with_combobox = false);
	void updateBumpyControls(bool is_setting_texture = false, bool mess_with_combobox = false);
	void updateAlphaControls();

	/*
	 * Checks whether the selected texture from the LLFloaterTexturePicker can be applied to the currently selected object.
	 * If agent selects texture which is not allowed to be applied for the currently selected object,
	 * all controls of the floater texture picker which allow to apply the texture will be disabled.
	 */
	void onTextureSelectionChanged(LLInventoryItem* itemp);

	bool mIsAlpha;


	#if defined(DEF_GET_MAT_STATE)
		#undef DEF_GET_MAT_STATE
	#endif

	#if defined(DEF_GET_TE_STATE)
		#undef DEF_GET_TE_STATE
	#endif

	#if defined(DEF_EDIT_MAT_STATE)
		DEF_EDIT_MAT_STATE
	#endif

	// Accessors for selected TE material state
	//
	#define DEF_GET_MAT_STATE(DataType,ReturnType,MaterialMemberFunc,DefaultValue)												\
		static void MaterialMemberFunc(DataType& data, bool& identical)																\
		{																																					\
			getTEMaterialValue< DataType, ReturnType, &LLMaterial::MaterialMemberFunc >(data, identical,DefaultValue);	\
		}

	// Mutators for selected TE material
	//
	#define DEF_EDIT_MAT_STATE(DataType,ReturnType,MaterialMemberFunc)																\
		static void MaterialMemberFunc(LLPanelFace* p,DataType data)																	\
		{																																					\
			edit< DataType, ReturnType, &LLMaterial::MaterialMemberFunc >(p,data);													\
		}

	// Accessors for selected TE state proper (legacy settings etc)
	//
	#define DEF_GET_TE_STATE(DataType,ReturnType,TexEntryMemberFunc,DefaultValue)													\
		static void TexEntryMemberFunc(DataType& data, bool& identical)																\
		{																																					\
			getTEValue< DataType, ReturnType, &LLTextureEntry::TexEntryMemberFunc >(data, identical,DefaultValue);		\
		}

	class LLSelectedTEMaterial
	{
	public:
		static void getCurrent(LLMaterialPtr& material_ptr, bool& identical_material);
		static void getMaxSpecularRepeats(F32& repeats, bool& identical);
		static void getMaxNormalRepeats(F32& repeats, bool& identical);
		static void getCurrentDiffuseAlphaMode(U8& diffuse_alpha_mode, bool& identical, bool diffuse_texture_has_alpha);

		DEF_GET_MAT_STATE(LLUUID,const LLUUID&,getNormalID,LLUUID::null)
		DEF_GET_MAT_STATE(LLUUID,const LLUUID&,getSpecularID,LLUUID::null)
		DEF_GET_MAT_STATE(F32,F32,getSpecularRepeatX,1.0f)
		DEF_GET_MAT_STATE(F32,F32,getSpecularRepeatY,1.0f)
		DEF_GET_MAT_STATE(F32,F32,getSpecularOffsetX,0.0f)
		DEF_GET_MAT_STATE(F32,F32,getSpecularOffsetY,0.0f)
		DEF_GET_MAT_STATE(F32,F32,getSpecularRotation,0.0f)

		DEF_GET_MAT_STATE(F32,F32,getNormalRepeatX,1.0f)
		DEF_GET_MAT_STATE(F32,F32,getNormalRepeatY,1.0f)
		DEF_GET_MAT_STATE(F32,F32,getNormalOffsetX,0.0f)
		DEF_GET_MAT_STATE(F32,F32,getNormalOffsetY,0.0f)
		DEF_GET_MAT_STATE(F32,F32,getNormalRotation,0.0f)

		DEF_EDIT_MAT_STATE(U8,U8,setDiffuseAlphaMode);
		DEF_EDIT_MAT_STATE(U8,U8,setAlphaMaskCutoff);

		DEF_EDIT_MAT_STATE(F32,F32,setNormalOffsetX);
		DEF_EDIT_MAT_STATE(F32,F32,setNormalOffsetY);
		DEF_EDIT_MAT_STATE(F32,F32,setNormalRepeatX);
		DEF_EDIT_MAT_STATE(F32,F32,setNormalRepeatY);
		DEF_EDIT_MAT_STATE(F32,F32,setNormalRotation);

		DEF_EDIT_MAT_STATE(F32,F32,setSpecularOffsetX);
		DEF_EDIT_MAT_STATE(F32,F32,setSpecularOffsetY);
		DEF_EDIT_MAT_STATE(F32,F32,setSpecularRepeatX);
		DEF_EDIT_MAT_STATE(F32,F32,setSpecularRepeatY);
		DEF_EDIT_MAT_STATE(F32,F32,setSpecularRotation);

		DEF_EDIT_MAT_STATE(U8,U8,setEnvironmentIntensity);
		DEF_EDIT_MAT_STATE(U8,U8,setSpecularLightExponent);

		DEF_EDIT_MAT_STATE(LLUUID,const LLUUID&,setNormalID);
		DEF_EDIT_MAT_STATE(LLUUID,const LLUUID&,setSpecularID);
		DEF_EDIT_MAT_STATE(LLColor4U,	const LLColor4U&,setSpecularLightColor);
	};

	class LLSelectedTE
	{
	public:

		static void getFace(LLFace*& face_to_return, bool& identical_face);
		static void getImageFormat(LLGLenum& image_format_to_return, bool& identical_face);
		static void getTexId(LLUUID& id, bool& identical);
		static void getObjectScaleS(F32& scale_s, bool& identical);
		static void getObjectScaleT(F32& scale_t, bool& identical);
		static void getMaxDiffuseRepeats(F32& repeats, bool& identical);

		DEF_GET_TE_STATE(U8,U8,getBumpmap,0)
		DEF_GET_TE_STATE(U8,U8,getShiny,0)
		DEF_GET_TE_STATE(U8,U8,getFullbright,0)
		DEF_GET_TE_STATE(F32,F32,getRotation,0.0f)
		DEF_GET_TE_STATE(F32,F32,getOffsetS,0.0f)
		DEF_GET_TE_STATE(F32,F32,getOffsetT,0.0f)
		DEF_GET_TE_STATE(F32,F32,getScaleS,1.0f)
		DEF_GET_TE_STATE(F32,F32,getScaleT,1.0f)
		DEF_GET_TE_STATE(F32,F32,getGlow,0.0f)
		DEF_GET_TE_STATE(LLTextureEntry::e_texgen,LLTextureEntry::e_texgen,getTexGen,LLTextureEntry::TEX_GEN_DEFAULT)
		DEF_GET_TE_STATE(LLColor4,const LLColor4&,getColor,LLColor4::white)
	};
};

#endif
