// <edit>
/* DOUBLE EDIT REACH AROUND
Rewritten by Hg Beeks

We will handle drag-and-drop in the future. Reminder - Look in llPreviewGesture for handleDragAndDrop. -HgB
*/

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llanimstatelabels.h"
#include "llao.h"
#include "llfilepicker.h"
#include "llinventorymodel.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llvoavatar.h"
//this is for debugging ;D
#define AO_DEBUG

//static variables
std::list<std::string> LLAO::mStandOverrides;
std::list<std::string> LLAO::mWalkOverrides;
std::list<std::string> LLAO::mRunOverrides;
std::list<std::string> LLAO::mCrouchwalkOverrides;
std::list<std::string> LLAO::mFlyOverrides;
std::list<std::string> LLAO::mTurnLeftOverrides;
std::list<std::string> LLAO::mTurnRightOverrides;
std::list<std::string> LLAO::mJumpOverrides;
std::list<std::string> LLAO::mFlyUpOverrides;
std::list<std::string> LLAO::mFlyDownOverrides;
std::list<std::string> LLAO::mCrouchOverrides;
std::list<std::string> LLAO::mHoverOverrides;
std::list<std::string> LLAO::mSitOverrides;
std::list<std::string> LLAO::mPreJumpOverrides;
std::list<std::string> LLAO::mFallOverrides;
std::list<std::string> LLAO::mStrideOverrides;
std::list<std::string> LLAO::mSoftLandOverrides;
std::list<std::string> LLAO::mMediumLandOverrides;
std::list<std::string> LLAO::mHardLandOverrides;
std::list<std::string> LLAO::mSlowFlyOverrides;
std::list<std::string> LLAO::mGroundSitOverrides;

std::map<LLUUID,LLUUID> LLAO::mOverrides;
LLFloaterAO* LLFloaterAO::sInstance;
BOOL LLAO::mEnabled = FALSE;
F32 LLAO::mPeriod;
LLAOStandTimer* LLAO::mTimer = NULL;

class ObjectNameMatches : public LLInventoryCollectFunctor
{
public:
	ObjectNameMatches(std::string name)
	{
		sName = name;
	}
	virtual ~ObjectNameMatches() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item)
		{
			return (item->getName() == sName);
		}
		return false;
	}
private:
	std::string sName;
};


LLUUID LLAO::getFrontUUID()
{
	if (!LLAO::mStandOverrides.empty())
		return LLUUID(LLAO::getAssetIDByName(LLAO::mStandOverrides.front()));
	else
		return LLUUID::null;
}

LLUUID LLAO::getBackUUID()
{
	if (!LLAO::mStandOverrides.empty())
		return LLUUID(LLAO::getAssetIDByName(LLAO::mStandOverrides.back()));
	else
		return LLUUID::null;
}

LLAOStandTimer::LLAOStandTimer(F32 period) : LLEventTimer(period)
{
}
BOOL LLAOStandTimer::tick()
{
	if(!mPaused && LLAO::isEnabled() && !LLAO::mStandOverrides.empty())
	{
#ifdef AO_DEBUG
		llinfos << "tick" << llendl;
#endif
		LLVOAvatar* avatarp = gAgent.getAvatarObject();
		if (avatarp)
		{
			for ( LLVOAvatar::AnimIterator anim_it =
					  avatarp->mPlayingAnimations.begin();
				  anim_it != avatarp->mPlayingAnimations.end();
				  anim_it++)
			{
				if(LLAO::isStand(anim_it->first))
				{
					//back is always last played, front is next
					avatarp->stopMotion(LLAO::getBackUUID());
#ifdef AO_DEBUG
					//llinfos << "Stopping " << LLAO::mStandOverrides.back() << llendl;
#endif
					avatarp->startMotion(LLAO::getFrontUUID());
#ifdef AO_DEBUG
					//llinfos << "Starting " << LLAO::mStandOverrides.front() << llendl;
#endif
					LLAO::mStandOverrides.push_back(LLAO::mStandOverrides.front());
					LLAO::mStandOverrides.pop_front();
					LLFloaterAO* ao = LLFloaterAO::sInstance;
					if(ao)
					{
						//ao->mStandsCombo->setSimple(LLStringExplicit(LLAO::mStandOverrides.back().asString()));
					}
					break;
				}
			}
		}
	}
	return FALSE;
}

void LLAOStandTimer::pause()
{
	if(mPaused) return;
#ifdef AO_DEBUG
	llinfos << "Pausing AO Timer...." << llendl;
#endif
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
#ifdef AO_DEBUG
		//llinfos << "Stopping " << LLAO::mStandOverrides.back() << llendl;
#endif
		gAgent.sendAnimationRequest(LLAO::getBackUUID(), ANIM_REQUEST_STOP);
		avatarp->stopMotion(LLAO::getBackUUID());
	}
	mEventTimer.reset();
	mEventTimer.stop();
	mPaused = TRUE;
}

void LLAOStandTimer::resume()
{
	if(!mPaused) return;
#ifdef AO_DEBUG
	llinfos << "Unpausing AO Timer...." << llendl;
#endif
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
#ifdef AO_DEBUG
		//llinfos << "Starting " << LLAO::mStandOverrides.back() << llendl;
#endif
		gAgent.sendAnimationRequest(LLAO::getBackUUID(), ANIM_REQUEST_START);
		avatarp->startMotion(LLAO::getBackUUID());
	}
	mEventTimer.reset();
	mEventTimer.start();
	mPaused = FALSE;
}

// Used for sorting
struct SortItemPtrsByName
{
	bool operator()(const LLInventoryItem* i1, const LLInventoryItem* i2)
	{
		return (LLStringUtil::compareDict(i1->getName(), i2->getName()) < 0);
	}
};


void LLAOStandTimer::reset()
{
	mEventTimer.reset();
}

//static
void LLAO::setup()
{
	mEnabled = gSavedSettings.getBOOL("AO.Enabled");
	mPeriod = gSavedSettings.getF32("AO.Period");
	mTimer = new LLAOStandTimer(mPeriod);
	gSavedSettings.getControl("AO.Enabled")->getSignal()->connect(boost::bind(&handleAOEnabledChanged, _1));
	gSavedSettings.getControl("AO.Period")->getSignal()->connect(boost::bind(&handleAOPeriodChanged, _1));
}
//static
void LLAO::runAnims(BOOL enabled)
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		for ( LLVOAvatar::AnimIterator anim_it =
				  avatarp->mPlayingAnimations.begin();
			  anim_it != avatarp->mPlayingAnimations.end();
			  anim_it++)
		{
			if(LLAO::mOverrides.find(anim_it->first) != LLAO::mOverrides.end())
			{
				LLUUID anim_id = mOverrides[anim_it->first];
				// this is an override anim
				if(enabled)
				{
					// make override start
					avatarp->startMotion(anim_id);
				}
				else
				{
					avatarp->stopMotion(anim_id);
					gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
				}
			}
		}
		if(mTimer)
		{
			if(enabled)
				mTimer->resume();
			else
				mTimer->pause();
		}
	}
}
//static
bool LLAO::handleAOPeriodChanged(const LLSD& newvalue)
{
	F32 value = (F32)newvalue.asReal();
	mPeriod = value;
	return true;
}
//static
bool LLAO::handleAOEnabledChanged(const LLSD& newvalue)
{
	BOOL value = newvalue.asBoolean();
	mEnabled = value;
	runAnims(value);
	return true;
}
//static
BOOL LLAO::isStand(LLUUID _id)
{
	std::string id = _id.asString();
	//ALL KNOWN STANDS
	if(id == "2408fe9e-df1d-1d7d-f4ff-1384fa7b350f") return TRUE;
	if(id == "15468e00-3400-bb66-cecc-646d7c14458e") return TRUE;
	if(id == "370f3a20-6ca6-9971-848c-9a01bc42ae3c") return TRUE;
	if(id == "42b46214-4b44-79ae-deb8-0df61424ff4b") return TRUE;
	if(id == "f22fed8b-a5ed-2c93-64d5-bdd8b93c889f") return TRUE;
	return FALSE;
}

const LLUUID& LLAO::getAssetIDByName(const std::string& name)
{
	if (name.empty()) return LLUUID::null;

	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	ObjectNameMatches objectnamematches(name);
	gInventory.collectDescendentsIf(LLUUID::null,cats,items,FALSE,objectnamematches);

	if (items.count())
	{
		return items[0]->getAssetUUID();
	}
	return LLUUID::null;
};

//static
void LLAO::refresh()
{
	mOverrides.clear();
	mStandOverrides.clear();
	mWalkOverrides.clear();
	mRunOverrides.clear();
	mCrouchwalkOverrides.clear();
	mFlyOverrides.clear();
	mTurnLeftOverrides.clear();
	mTurnRightOverrides.clear();
	mJumpOverrides.clear();
	mFlyUpOverrides.clear();
	mFlyDownOverrides.clear();
	mCrouchOverrides.clear();
	mHoverOverrides.clear();
	mSitOverrides.clear();
	mPreJumpOverrides.clear();
	mFallOverrides.clear();
	mStrideOverrides.clear();
	mSoftLandOverrides.clear();
	mMediumLandOverrides.clear();
	mHardLandOverrides.clear();
	mSlowFlyOverrides.clear();
	mGroundSitOverrides.clear();
	LLSD settings = gSavedPerAccountSettings.getLLSD("AO.Settings");
	//S32 version = (S32)settings["version"].asInteger();
	LLSD overrides = settings["overrides"];
	LLSD::map_iterator sd_it = overrides.beginMap();
	LLSD::map_iterator sd_end = overrides.endMap();
	for( ; sd_it != sd_end; sd_it++)
	{
		if(sd_it->second.asString() == "" )
		{
			continue;
		}
		if(sd_it->first == "stands")
		{
			for(LLSD::array_iterator itr = sd_it->second.beginArray();
				itr != sd_it->second.endArray(); ++itr)
			{
					//list of listness
				if(itr->asString() != "")
					mStandOverrides.push_back(itr->asString());
			}
		}
	}
}

std::list<std::string>* LLAO::getOverrideList(const std::string name)
{
	std::list<std::string> *anim_list;
	if (name == "Stand")
		anim_list = &mStandOverrides;
	else if (name == "Walk")
		anim_list = &mWalkOverrides;
	else if (name == "Run")
		anim_list = &mRunOverrides;
	else if (name == "Crouchwalk")
		anim_list = &mCrouchwalkOverrides;
	else if (name == "Fly")
		anim_list = &mFlyOverrides;
	else if (name == "TurnLeft")
		anim_list = &mTurnLeftOverrides;
	else if (name == "TurnRight")
		anim_list = &mTurnRightOverrides;
	else if (name == "Jump")
		anim_list = &mJumpOverrides;
	else if (name == "FlyUp")
		anim_list = &mFlyUpOverrides;
	else if (name == "FlyDown")
		anim_list = &mFlyDownOverrides;
	else if (name == "Crouch")
		anim_list = &mCrouchOverrides;
	else if (name == "Hover")
		anim_list = &mHoverOverrides;
	else if (name == "Sit")
		anim_list = &mSitOverrides;
	else if (name == "PreJump")
		anim_list = &mPreJumpOverrides;
	else if (name == "Fall")
		anim_list = &mFallOverrides;
	else if (name == "Stride")
		anim_list = &mStrideOverrides;
	else if (name == "SoftLand")
		anim_list = &mSoftLandOverrides;
	else if (name == "MediumLand")
		anim_list = &mMediumLandOverrides;
	else if (name == "HardLand")
		anim_list = &mHardLandOverrides;
	else if (name == "SlowFly")
		anim_list = &mSlowFlyOverrides;
	else
		anim_list = &mGroundSitOverrides;
	return anim_list;
}

void LLAO::pushTo(std::string name, std::string value)
{
	if (name == "Stand")
		mStandOverrides.push_back(value);
	else if (name == "Walk")
		mWalkOverrides.push_back(value);
	else if (name == "Run")
		mRunOverrides.push_back(value);
	else if (name == "Crouchwalk")
		mCrouchwalkOverrides.push_back(value);
	else if (name == "Fly")
		mFlyOverrides.push_back(value);
	else if (name == "TurnLeft")
		mTurnLeftOverrides.push_back(value);
	else if (name == "TurnRight")
		mTurnRightOverrides.push_back(value);
	else if (name == "Jump")
		mJumpOverrides.push_back(value);
	else if (name == "FlyUp")
		mFlyUpOverrides.push_back(value);
	else if (name == "FlyDown")
		mFlyDownOverrides.push_back(value);
	else if (name == "Crouch")
		mCrouchOverrides.push_back(value);
	else if (name == "Hover")
		mHoverOverrides.push_back(value);
	else if (name == "Sit")
		mSitOverrides.push_back(value);
	else if (name == "PreJump")
		mPreJumpOverrides.push_back(value);
	else if (name == "Fall")
		mFallOverrides.push_back(value);
	else if (name == "Stride")
		mStrideOverrides.push_back(value);
	else if (name == "SoftLand")
		mSoftLandOverrides.push_back(value);
	else if (name == "MediumLand")
		mMediumLandOverrides.push_back(value);
	else if (name == "HardLand")
		mHardLandOverrides.push_back(value);
	else if (name == "SlowFly")
		mSlowFlyOverrides.push_back(value);
	else
		mGroundSitOverrides.push_back(value);
}

//static ------------- Floater
void LLFloaterAO::show()
{
	if(sInstance)
		sInstance->open();
	else
		(new LLFloaterAO())->open();
}

LLFloaterAO::LLFloaterAO()
:	LLFloater()
{
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_ao.xml");
}

LLFloaterAO::~LLFloaterAO()
{
	sInstance = NULL;
}

BOOL LLFloaterAO::postBuild(void)
{
	LLComboBox*			combo;
	LLScrollListCtrl*	list;

	childSetAction("btn_save", onClickSave, this);
	childSetAction("btn_load", onClickLoad, this);

	combo = getChild<LLComboBox>( "combo_anim_type");
	combo->setCommitCallback(onCommitType);
	combo->setCallbackUserData(this);
	mAnimTypeCombo = combo;
	
	combo = getChild<LLComboBox>( "combo_anim_list");
	mAnimListCombo = combo;
	childSetAction("combo_anim_add", onClickAnimAdd, this);
	childSetAction("combo_anim_delete", onClickAnimRemove, this);

	list = getChild<LLScrollListCtrl>("combo_anim_list");
	mAnimationList = list;

	addAnimations();
	refresh();
	return TRUE;
}

void LLFloaterAO::refresh()
{
	std::list<std::string> *animation_list = LLAO::getOverrideList(mCurrentAnimType);
	llinfos << "Refreshed, building animation list for " << mCurrentAnimType << ", Count:" << animation_list->size() << llendl;
	for(std::list<std::string>::iterator itr = animation_list->begin();itr != animation_list->end();
		itr++)
	{
		llinfos << "Adding " << itr->c_str() << llendl;
		mAnimationList->addElement((*itr), ADD_BOTTOM);
	}
}
// static
void LLFloaterAO::onCommitType(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	floater->mCurrentAnimType = floater->mAnimTypeCombo->getSimple();
	floater->refresh();
}

void LLFloaterAO::addAnimations()
{
	mAnimListCombo->removeall();
	
	std::string none_text = getString("none_text");
	mAnimListCombo->add(none_text, LLUUID::null);

	// Get all inventory items that are animations
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLIsTypeWithPermissions is_copyable_animation(LLAssetType::AT_ANIMATION,
													PERM_ITEM_UNRESTRICTED,
													gAgent.getID(),
													gAgent.getGroupID());
	gInventory.collectDescendentsIf(gAgent.getInventoryRootID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_copyable_animation);

	// Copy into something we can sort
	std::vector<LLInventoryItem*> animations;

	S32 count = items.count();
	for(i = 0; i < count; ++i)
	{
		animations.push_back( items.get(i) );
	}

	// Do the sort
	std::sort(animations.begin(), animations.end(), SortItemPtrsByName());

	// And load up the combobox
	std::vector<LLInventoryItem*>::iterator it;
	for (it = animations.begin(); it != animations.end(); ++it)
	{
		LLInventoryItem* item = *it;
		llinfos << "Adding " << item->getName() << " to animation list." << llendl;
		mAnimListCombo->add(item->getName(), item->getAssetUUID(), ADD_BOTTOM);
	}
}

void generateAnimationSet(std::string name, std::list<std::string> anim_list, LLSD& overrides)
{
	for(std::list<std::string>::iterator itr = anim_list.begin();
		itr != anim_list.end();
		itr++)
	{
		overrides[name].append((*itr));
	}
}

// static
void LLFloaterAO::onCommitAnim(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	//LLUUID animid(getAssetIDByName(stranim));  ------------------------------------------- IMPORTANT FOR LATER

	/*if (!(animid.notNull()))
	{
		cmdline_printchat(llformat("Warning: animation '%s' could not be found (Section: %s).",stranim.c_str(),strtoken.c_str()));
	}*/
	LLSD overrides;
	generateAnimationSet("stands",		LLAO::mStandOverrides,		overrides);
	generateAnimationSet("walks",		LLAO::mWalkOverrides,		overrides);
	generateAnimationSet("runs",		LLAO::mRunOverrides,		overrides);
	generateAnimationSet("crouchwalks", LLAO::mCrouchwalkOverrides,	overrides);
	generateAnimationSet("flies",		LLAO::mFlyOverrides,		overrides);
	generateAnimationSet("turnlefts",	LLAO::mTurnLeftOverrides,	overrides);
	generateAnimationSet("turnrights",	LLAO::mTurnRightOverrides,	overrides);
	generateAnimationSet("jumps",		LLAO::mJumpOverrides,		overrides);
	generateAnimationSet("flyups",		LLAO::mFlyUpOverrides,		overrides);
	generateAnimationSet("flydowns",	LLAO::mFlyDownOverrides,	overrides);
	generateAnimationSet("crouches",	LLAO::mCrouchOverrides,		overrides);
	generateAnimationSet("hovers",		LLAO::mHoverOverrides,		overrides);
	generateAnimationSet("sits",		LLAO::mSitOverrides,		overrides);
	generateAnimationSet("prejumps",	LLAO::mPreJumpOverrides,	overrides);
	generateAnimationSet("falls",		LLAO::mFallOverrides,		overrides);
	generateAnimationSet("strides",		LLAO::mStrideOverrides,		overrides);
	generateAnimationSet("softlands",	LLAO::mSoftLandOverrides,	overrides);
	generateAnimationSet("mediumlands", LLAO::mMediumLandOverrides, overrides);
	generateAnimationSet("hardlands",	LLAO::mHardLandOverrides,	overrides);
	generateAnimationSet("slowflies",	LLAO::mSlowFlyOverrides,	overrides);
	generateAnimationSet("groundsits",	LLAO::mGroundSitOverrides,	overrides);
	
	LLSD settings;
	settings["version"] = 2;
	settings["overrides"] = overrides;
	gSavedPerAccountSettings.setLLSD("AO.Settings", settings);
	LLAO::refresh();
	floater->refresh();
}


//static
void LLFloaterAO::onClickAnimRemove(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	std::vector<LLScrollListItem*> items = floater->mAnimationList->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item->getValue().asString() != "")
		{
			//avatar_names.push_back(item->getColumn(0)->getValue().asString());
			//avatar_ids.push_back(item->getUUID());
		}
	}
	/*std::list<std::string>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr != LLAO::mStandOverrides.end())
	{
		//back is always last played, front is next
		avatarp->stopMotion(id);
		LLAO::mStandOverrides.erase(itr);
		avatarp->startMotion(LLAO::mStandOverrides.front());
		LLAO::mStandOverrides.push_back(LLAO::mStandOverrides.front());
		LLAO::mStandOverrides.pop_front();

		floater->refresh();
		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);*/
}
//static
void LLFloaterAO::onClickAnimAdd(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	std::string anim_name = floater->mAnimListCombo->getSimple();
	LLUUID id(LLAO::getAssetIDByName(anim_name));
	std::list<std::string>* animation_list = LLAO::getOverrideList(floater->mCurrentAnimType);
	std::list<std::string>::iterator itr = std::find(animation_list->begin(),
		animation_list->end(),
		anim_name);
#ifdef AO_DEBUG
	llinfos << "Attempting to add " << anim_name << " (" << id << ") " << " to " << floater->mCurrentAnimType << llendl;
#endif
	if(id.notNull() && (itr == animation_list->end()))
	{
		llinfos << "Actually adding animation, this should be refreshed. Count:" << animation_list->size() << llendl;
		LLAO::pushTo(floater->mCurrentAnimType, anim_name);
		llinfos << "Added animation. Count:" << animation_list->size() << llendl;
		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);
}
//static
void LLFloaterAO::onClickSave(void* user_data)
{
	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getSaveFile( LLFilePicker::FFSAVE_AO, LLDir::getScrubbedFileName("untitled.ao")))
	{
		std::string file_name = file_picker.getFirstFile();
		llofstream export_file(file_name);
		LLSDSerialize::toPrettyXML(gSavedPerAccountSettings.getLLSD("AO.Settings"), export_file);
		export_file.close();
	}
}

//static
void LLFloaterAO::onClickLoad(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;

	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getOpenFile(LLFilePicker::FFLOAD_AO))
	{
		std::string file_name = file_picker.getFirstFile();
		llifstream xml_file(file_name);
		if(!xml_file.is_open()) return;
		LLSD data;
		if(LLSDSerialize::fromXML(data, xml_file) >= 1)
		{
			if(LLAO::isEnabled())
				LLAO::runAnims(FALSE);

			gSavedPerAccountSettings.setLLSD("AO.Settings", data);
			LLAO::refresh();

			if(LLAO::isEnabled())
				LLAO::runAnims(TRUE);

			floater->refresh();
		}
		xml_file.close();
	}
}
// </edit>