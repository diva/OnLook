// <edit>
#include "llviewerprecompiledheaders.h"
#include "llfloaterblacklist.h"
#include "lluictrlfactory.h"
#include "llsdserialize.h"
#include "llscrolllistctrl.h"
#include "llcheckboxctrl.h"
#include "llfilepicker.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"

LLFloaterBlacklist* LLFloaterBlacklist::sInstance;
LLFloaterBlacklist::LLFloaterBlacklist()
:	LLFloater()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_blacklist.xml");
}
LLFloaterBlacklist::~LLFloaterBlacklist()
{
	sInstance = NULL;
}
// static
void LLFloaterBlacklist::show()
{
	if(sInstance)
		sInstance->open();
	else
	{
		sInstance = new LLFloaterBlacklist();
		sInstance->open();
	}
}
BOOL LLFloaterBlacklist::postBuild()
{
	childSetAction("add_btn", onClickAdd, this);
	childSetAction("clear_btn", onClickClear, this);
	childSetAction("copy_uuid_btn", onClickCopyUUID, this);
	childSetAction("remove_btn", onClickRemove, this);
	childSetAction("save_btn", onClickSave, this);
	childSetAction("load_btn", onClickLoad, this);
	refresh();
	return TRUE;
}
void LLFloaterBlacklist::refresh()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("file_list");
	list->clearRows();
	if(gAssetStorage) // just in case
	{
		LLSD settings;
		for(std::vector<LLUUID>::iterator iter = gAssetStorage->mBlackListedAsset.begin();
			iter != gAssetStorage->mBlackListedAsset.end(); ++iter)
		{
			LLSD element;
			element["id"] = (*iter);
			LLSD& name_column = element["columns"][0];
			name_column["column"] = "asset_id";
			name_column["value"] = (*iter).asString();
			list->addElement(element, ADD_BOTTOM);
			settings.append((*iter));
		}
		setMassEnabled(!gAssetStorage->mBlackListedAsset.empty());
		gSavedSettings.setLLSD("Blacklist.Settings",settings);
	}
	else
		setMassEnabled(FALSE);
	setEditID(mSelectID);
}
void LLFloaterBlacklist::add(LLUUID uuid)
{
	if(gAssetStorage)
		gAssetStorage->mBlackListedAsset.push_back(uuid);
	refresh();
}
void LLFloaterBlacklist::clear()
{
	if(gAssetStorage) // just in case
	{
		gAssetStorage->mBlackListedAsset.clear();
	}
	refresh();
}
void LLFloaterBlacklist::setEditID(LLUUID edit_id)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("file_list");
	bool found = false;
	if(gAssetStorage)
		found = std::find(gAssetStorage->mBlackListedAsset.begin(),
					gAssetStorage->mBlackListedAsset.end(),edit_id) != gAssetStorage->mBlackListedAsset.end();
	if(found)
	{
		mSelectID = edit_id;
		list->selectByID(edit_id);
		setEditEnabled(true);
	}
	else
	{
		mSelectID = LLUUID::null;
		list->deselectAllItems(TRUE);
		setEditEnabled(false);
	}
}
void LLFloaterBlacklist::removeEntry()
{
	if(gAssetStorage && mSelectID.notNull())
		std::remove(gAssetStorage->mBlackListedAsset.begin(),gAssetStorage->mBlackListedAsset.end(),mSelectID);
	refresh();
}
void LLFloaterBlacklist::setMassEnabled(bool enabled)
{
	childSetEnabled("clear_btn", enabled);
}
void LLFloaterBlacklist::setEditEnabled(bool enabled)
{
	childSetEnabled("copy_uuid_btn", enabled);
	childSetEnabled("remove_btn", enabled);
}
// static
void LLFloaterBlacklist::onClickAdd(void* user_data)
{
	LLFloaterBlacklist* floaterp = (LLFloaterBlacklist*)user_data;
	if(!floaterp) return;
	floaterp->add(LLUUID(floaterp->childGetValue("id_edit").asString()));
}
// static
void LLFloaterBlacklist::onClickClear(void* user_data)
{
	LLFloaterBlacklist* floaterp = (LLFloaterBlacklist*)user_data;
	if(!floaterp) return;
	floaterp->clear();
}
// static
void LLFloaterBlacklist::onCommitFileList(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterBlacklist* floaterp = (LLFloaterBlacklist*)user_data;
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("file_list");
	LLUUID selected_id;
	if(list->getFirstSelected())
		selected_id = list->getFirstSelected()->getUUID();
	floaterp->setEditID(selected_id);
}
// static
void LLFloaterBlacklist::onClickCopyUUID(void* user_data)
{
	LLFloaterBlacklist* floaterp = (LLFloaterBlacklist*)user_data;
	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(floaterp->mSelectID.asString()));
}
// static
void LLFloaterBlacklist::onClickRemove(void* user_data)
{
	LLFloaterBlacklist* floaterp = (LLFloaterBlacklist*)user_data;
	floaterp->removeEntry();
}
// static
void LLFloaterBlacklist::loadFromSave()
{
	if(!gAssetStorage) return;
	LLSD blacklist = gSavedSettings.getLLSD("Blacklist.Settings");
	for(LLSD::array_iterator itr = blacklist.beginArray(); itr != blacklist.endArray(); ++itr)
	{
		gAssetStorage->mBlackListedAsset.push_back(itr->asUUID());
	}
}
//static
void LLFloaterBlacklist::onClickSave(void* user_data)
{
	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getSaveFile( LLFilePicker::FFSAVE_BLACKLIST, LLDir::getScrubbedFileName("untitled.blacklist")))
	{
		std::string file_name = file_picker.getFirstFile();
		llofstream export_file(file_name);
		LLSDSerialize::toPrettyXML(gSavedSettings.getLLSD("Blacklist.Settings"), export_file);
		export_file.close();
	}
}

//static
void LLFloaterBlacklist::onClickLoad(void* user_data)
{
	LLFloaterBlacklist* floater = (LLFloaterBlacklist*)user_data;

	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getOpenFile(LLFilePicker::FFLOAD_BLACKLIST))
	{
		std::string file_name = file_picker.getFirstFile();
		llifstream xml_file(file_name);
		if(!xml_file.is_open()) return;
		LLSD data;
		if(LLSDSerialize::fromXML(data, xml_file) >= 1)
		{
			gSavedSettings.setLLSD("Blacklist.Settings", data);
			LLFloaterBlacklist::loadFromSave();
			floater->refresh();
		}
		xml_file.close();
	}
}
// </edit>
