/**
 * @file doinventorybackup.h
 * @brief DOInventoryBackup Floaters and Inventory Backup System
 * @author Day Oh
 * 
 * $LicenseInfo:firstyear=2009&license=WTFPLV2$
 *  
 */

// <edit>
#ifndef DO_DOINVENTORYBACKUP_H
#define DO_DOINVENTORYBACKUP_H

#include "llviewerinventory.h"
#include "llfolderview.h"
#include "llfilepicker.h"
#include "llviewerimage.h"
#include "llfloater.h"


class DOInventoryBackupOrder
{
public:
	DOInventoryBackupOrder();

	std::string mPath;
	std::vector<LLInventoryCategory*> mCats;
	std::vector<LLInventoryItem*> mItems;

	bool mDownloadTextures;
	bool mDownloadSounds;
	bool mDownloadCallingCards;
	bool mDownloadLandmarks;
	bool mDownloadScripts;
	bool mDownloadWearables;
	bool mDownloadObjects;
	bool mDownloadNotecards;
	bool mDownloadAnimations;
	bool mDownloadGestures;
	//bool mDownloadOthers;
};

class DOFloaterInventoryBackupSettings
: public LLFloater
{
public:
	DOFloaterInventoryBackupSettings(DOInventoryBackupOrder* order);
	BOOL postBuild(void);
	static void onClickNext(void* userdata);

	DOInventoryBackupOrder* mOrder;
	virtual ~DOFloaterInventoryBackupSettings();
};

class DOFloaterInventoryBackup
: public LLFloater
{
public:
	DOFloaterInventoryBackup(std::string path, std::vector<LLInventoryCategory*> cats, std::vector<LLInventoryItem*> items);
	BOOL postBuild(void);

	std::string mPath;
	std::vector<LLInventoryCategory*> mCats;
	std::vector<LLInventoryItem*> mItems;
	std::vector<LLInventoryItem*>::iterator mItemIter;
	int mBusy;

	static std::list<DOFloaterInventoryBackup*> sInstances;

private:
	virtual ~DOFloaterInventoryBackup();
	void setStatus(LLUUID itemid, std::string status);
	void finishItem(LLUUID itemid, std::string status);
	void advance();
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

	int mItemsTotal;
	int mItemsCompleted;

	enum LIST_COLUMN_ORDER
	{
		LIST_TYPE,
		LIST_NAME,
		LIST_STATUS
	};

};

class DOInventoryBackup
{
public:
	static LLFilePicker::ESaveFilter getSaveFilter(LLInventoryItem* item);
	static std::string getExtension(LLInventoryItem* item);
	static std::string getUniqueFilename(std::string filename, std::string extension);
	static std::string getUniqueDirname(std::string dirname);
	static bool itemIsFolder(LLInventoryItem* item);
	static void save(LLFolderView* folder);
	static void download(LLInventoryItem* item, LLFloater* floater, loaded_callback_func onImage, LLGetAssetCallback onAsset);
	static std::string getPath(LLInventoryCategory* cat, std::vector<LLInventoryCategory*> cats);

	struct callbackdata
	{
		LLFloater* floater;
		LLInventoryItem* item;
	};

private:
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
	static void climb(LLInventoryCategory* cat,
							  std::vector<LLInventoryCategory*>& cats,
							  std::vector<LLInventoryItem*>& items);
};



#endif
// </edit>
