/**
 * @file dofloaterhex.h
 * @brief Hex Editor Floater made by Day
 * @author Day Oh
 * 
 * $LicenseInfo:firstyear=2009&license=WTFPLV2$
 *  
 */

// <edit>


#ifndef DO_DOFLOATERHEX_H
#define DO_DOFLOATERHEX_H

#include "llfloater.h"
#include "dohexeditor.h"
#include "llinventory.h"
#include "llviewerimage.h"

class DOFloaterHex
: public LLFloater
{
public:
	DOFloaterHex(LLInventoryItem* item);
	static void show(LLUUID item_id);
	BOOL postBuild(void);
	void close(bool app_quitting);
	static void imageCallback(BOOL success, 
					LLViewerImage *src_vi,
					LLImageRaw* src, 
					LLImageRaw* aux_src, 
					S32 discard_level,
					BOOL final,
					void* userdata);
	static void assetCallback(LLVFS *vfs,
				   const LLUUID& asset_uuid,
				   LLAssetType::EType type,
				   void* user_data, S32 status, LLExtStat ext_status);
	static void onClickSave(void* user_data);
	static void onClickUpload(void* user_data);
	static void onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	LLInventoryItem* mItem;
	DOHexEditor* mEditor;
	static std::list<DOFloaterHex*> sInstances;
private:
	virtual ~DOFloaterHex();
protected:
	static S32 sUploadAmount;
};

#endif
// </edit>
