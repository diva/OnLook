// <edit>

#include "llviewerprecompiledheaders.h"
#include "llfloaterexploreanimations.h"
#include "lluictrlfactory.h"
#include "llscrolllistctrl.h"
#include "llfloateranimpreview.h"
#include "llvoavatar.h"
#include "lllocalinventory.h"
// <stuff for jelly roll>
#include "llfloatertools.h"
#include "llselectmgr.h"
// </stuff for jelly roll>

std::map< LLUUID, std::list< LLAnimHistoryItem* > > LLFloaterExploreAnimations::animHistory;
LLFloaterExploreAnimations* LLFloaterExploreAnimations::sInstance;


LLAnimHistoryItem::LLAnimHistoryItem(LLUUID assetid)
{
	mAssetID = assetid;
}

LLFloaterExploreAnimations::LLFloaterExploreAnimations(LLUUID avatarid)
:	LLFloater()
{
	LLFloaterExploreAnimations::sInstance = this;
	mAvatarID = avatarid;
	mAnimPreview = new LLPreviewAnimation(256, 256);
	mAnimPreview->setZoom(2.0f);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_explore_animations.xml");
}

void LLFloaterExploreAnimations::close(bool app_quitting)
{
	LLFloater::close(app_quitting);
}

LLFloaterExploreAnimations::~LLFloaterExploreAnimations()
{
	delete mAnimPreview;
	LLFloaterExploreAnimations::sInstance = NULL;
}

BOOL LLFloaterExploreAnimations::postBuild(void)
{
	childSetCommitCallback("anim_list", onSelectAnimation, this);
	childSetAction("copy_uuid_btn", onClickCopyUUID, this);
	childSetAction("open_btn", onClickOpen, this);
	childSetAction("jelly_roll_btn", onClickJellyRoll, this);
	update();
	return TRUE;
}

void LLFloaterExploreAnimations::update()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("anim_list");
	LLUUID selection = list->getSelectedValue().asUUID();
	list->clearRows(); // do this differently probably

	std::list<LLAnimHistoryItem*> history = animHistory[mAvatarID];
	std::list<LLAnimHistoryItem*>::iterator iter = history.begin();
	std::list<LLAnimHistoryItem*>::iterator end = history.end();
	for( ; iter != end; ++iter)
	{
		LLAnimHistoryItem* item = (*iter);

		LLSD element;
		element["id"] = item->mAssetID;

		LLSD& name_column = element["columns"][0];
		name_column["column"] = "name";
		name_column["value"] = item->mAssetID.asString();

		LLSD& info_column = element["columns"][1];
		info_column["column"] = "info";
		if(item->mPlaying)
			info_column["value"] = "Playing";
		else
			info_column["value"] = llformat("%.1f min ago", (LLTimer::getElapsedSeconds() - item->mTimeStopped) / 60.f);

		list->addElement(element, ADD_BOTTOM);
	}

	list->selectByID(selection);
}

void LLFloaterExploreAnimations::draw()
{
	LLFloater::draw();

	LLRect r = getRect();

	gGL.color3f(1.f, 1.f, 1.f);

	gGL.getTexUnit(0)->bind(mAnimPreview->getTexture());

	gGL.begin( LLRender::QUADS );
	{
		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex2i(r.getWidth() - 266, r.getHeight() - 25);
		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex2i(r.getWidth() - 266, r.getHeight() - 256);
		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex2i(r.getWidth() - 10, r.getHeight() - 256);
		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex2i(r.getWidth() - 10, r.getHeight() - 25);
	}
	gGL.end();

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	//LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	//if (!avatarp->areAnimationsPaused())
	//{
	//	mAnimPreview->requestUpdate();
	//}
}





// static
void LLFloaterExploreAnimations::startAnim(LLUUID avatarid, LLUUID assetid)
{
	std::string asset_str = assetid.asString();
	if(asset_str.find("17132261-c061") != std::string::npos) return; // dog1
	else if(asset_str.find("fea558cb-8b9b") != std::string::npos) return; // dog2
	else if(asset_str.find("50cb5750-0743") != std::string::npos) return; // dog3
	else if(asset_str.find("-dead-") != std::string::npos) return; // emo

	LLAnimHistoryItem* item = NULL;

	std::list<LLAnimHistoryItem*> history = animHistory[avatarid];
	std::list<LLAnimHistoryItem*>::iterator iter = history.begin();
	std::list<LLAnimHistoryItem*>::iterator end = history.end();
	for( ; iter != end; ++iter)
	{
		if((*iter)->mAssetID == assetid)
		{
			item = (*iter);
			break;
		}
	}
	if(!item)
	{
		item = new LLAnimHistoryItem(assetid);
		item->mAvatarID = avatarid;
		item->mTimeStarted = LLTimer::getElapsedSeconds();
	}
	item->mPlaying = true;
	history.push_back(item);
	animHistory[avatarid] = history; // is this really necessary?
	handleHistoryChange();
}

// static
void LLFloaterExploreAnimations::stopAnim(LLUUID avatarid, LLUUID assetid)
{
	std::string asset_str = assetid.asString();
	if(asset_str.find("17132261-c061") != std::string::npos) return; // dog1
	else if(asset_str.find("fea558cb-8b9b") != std::string::npos) return; // dog2
	else if(asset_str.find("50cb5750-0743") != std::string::npos) return; // dog3
	else if(asset_str.find("-dead-") != std::string::npos) return; // emo

	LLAnimHistoryItem* item = NULL;

	std::list<LLAnimHistoryItem*> history = animHistory[avatarid];
	std::list<LLAnimHistoryItem*>::iterator iter = history.begin();
	std::list<LLAnimHistoryItem*>::iterator end = history.end();
	for( ; iter != end; ++iter)
	{
		if((*iter)->mAssetID == assetid)
		{
			item = (*iter);
			break;
		}
	}
	if(!item)
	{
		item = new LLAnimHistoryItem(assetid);
		item->mAvatarID = avatarid;
		item->mTimeStarted = LLTimer::getElapsedSeconds();
		history.push_back(item);
	}
	item->mPlaying = false;
	item->mTimeStopped = LLTimer::getElapsedSeconds();
	handleHistoryChange();
}

class LLAnimHistoryItemCompare
{
public:
	bool operator() (LLAnimHistoryItem* first, LLAnimHistoryItem* second)
	{
		if(first->mPlaying)
		{
			if(second->mPlaying)
			{
				return (first->mTimeStarted > second->mTimeStarted);
			}
			else
			{
				return true;
			}
		}
		else if(second->mPlaying)
		{
			return false;
		}
		else
		{
			return (first->mTimeStopped > second->mTimeStopped);
		}
	}
};

// static
void LLFloaterExploreAnimations::handleHistoryChange()
{
	std::map< LLUUID, std::list< LLAnimHistoryItem* > >::iterator av_iter = animHistory.begin();
	std::map< LLUUID, std::list< LLAnimHistoryItem* > >::iterator av_end = animHistory.end();
	for( ; av_iter != av_end; ++av_iter)
	{
		std::list<LLAnimHistoryItem*> history = (*av_iter).second;

		// Sort it
		LLAnimHistoryItemCompare c;
		history.sort(c);

		// Remove dupes
		history.unique();

		// Trim it
		if(history.size() > 32)
		{
			history.resize(32);
		}

		animHistory[(*av_iter).first] = history;
	}

	// Update floater
	if(LLFloaterExploreAnimations::sInstance)
		LLFloaterExploreAnimations::sInstance->update();
}





// static
void LLFloaterExploreAnimations::onSelectAnimation(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterExploreAnimations* floater = (LLFloaterExploreAnimations*)user_data;
	LLPreviewAnimation* preview = (LLPreviewAnimation*)floater->mAnimPreview;
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("anim_list");
	LLUUID selection = list->getSelectedValue().asUUID();
	
	preview->getDummyAvatar()->deactivateAllMotions();
	preview->getDummyAvatar()->startMotion(selection, 0.f);
	preview->setZoom(2.0f);
}

// static
void LLFloaterExploreAnimations::onClickCopyUUID(void* data)
{
	LLFloaterExploreAnimations* floater = (LLFloaterExploreAnimations*)data;
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("anim_list");
	LLUUID selection = list->getSelectedValue().asUUID();
	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(selection.asString()));
}

void LLFloaterExploreAnimations::onClickOpen(void* data)
{
	LLFloaterExploreAnimations* floater = (LLFloaterExploreAnimations*)data;
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("anim_list");
	LLUUID selection = list->getSelectedValue().asUUID();
	LLUUID item = LLLocalInventory::addItem(selection.asString(), LLAssetType::AT_ANIMATION, selection, true);
}

void LLFloaterExploreAnimations::onClickJellyRoll(void* data)
{
	std::string hover_text;
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	LLObjectSelection::valid_iterator sel_it = selection->valid_begin();
	LLObjectSelection::valid_iterator sel_end = selection->valid_end();
	for( ; sel_it != sel_end; ++sel_it)
	{
		LLViewerObject* objectp = (*sel_it)->getObject();
		hover_text = objectp->getDebugText();
		if(hover_text != "")
		{
			break;
		}
	}

	LLFloaterExploreAnimations* floater = (LLFloaterExploreAnimations*)data;
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("anim_list");
	LLUUID anim_id = list->getSelectedValue().asUUID();
	
	LLFloaterNewLocalInventory* createy = new LLFloaterNewLocalInventory();
	createy->childSetText("name_line", hover_text);
	createy->childSetText("asset_id_line", anim_id.asString());
	createy->childSetValue("type_combo", "animatn");
	createy->childSetText("creator_id_line", LLFloaterNewLocalInventory::sLastCreatorId.asString());
}

// </edit>
