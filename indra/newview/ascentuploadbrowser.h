/** 
 * @file ascentuploadbrowser.h
 * @Author Duncan Garrett
 * Meant as a replacement to using Windows' file browser for uploads.
 *
 * Created August 27 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * k ilu bye
 */

#ifndef ASCENT_UPLOAD_BROWSER
#define ASCENT_UPLOAD_BROWSER

#include "llfloater.h"
#include "llcombobox.h" 

class LLScrollListCtrl;

class ASFloaterUploadBrowser : public LLFloater
{
public:
	using LLFloater::handleDoubleClick;

	ASFloaterUploadBrowser();
	virtual ~ASFloaterUploadBrowser();
	//File list
	static void onClickFile(LLUICtrl* ctrl, void* user_data);
	static void onUpdateFilter(LLUICtrl* ctrl, void* user_data);
	static void onDoubleClick(void* user_data);
	static void onDirCommit (LLUICtrl* ctrl, void* data);
	static void onChangeDrives(LLUICtrl* ctrl, void* user_data);
	static void onClickFilepathGoto(void* data);

	void updateBrowser(void* data, std::string new_path);
	void refresh();
	void refreshUploadOptions();
	void handleDoubleClick();
	static void show(void*);

	std::vector<LLSD> datas;

private:
	static LLSD mUploaderSettings;
	static ASFloaterUploadBrowser* sInstance;
	enum FILE_COLUMN_ORDER
	{
		LIST_FILE_TYPE,
		LIST_ASSET_TYPE,
		LIST_INVENTORY_TYPE,
		LIST_FILE_NAME,
		LIST_DATA
	};
	enum FILE_TYPE_ORDER
	{
		LIST_TYPE_PARENT,
		LIST_TYPE_FOLDER,
		LIST_TYPE_FILE
	};
	enum FILE_TYPE
	{
		FOLDER,
		FILE_TEXTURE, 
		FILE_SOUND,
		FILE_ANIMATION
	};
	LLScrollListCtrl* mFileList;
	LLComboBox* mDriveCombo;
	LLComboBox* mBookmarkCombo;

	std::string mPathName;
	std::string mFilterType;
};


#endif // ASCENT_UPLOAD_BROWSER
