// <edit>

#include "llviewerprecompiledheaders.h"
#include "llao.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llfilepicker.h"
#include "llsdserialize.h"
#include "llagent.h"
#include "llvoavatar.h"

std::list<LLUUID> LLAO::mStandOverrides;
std::map<LLUUID,LLUUID> LLAO::mOverrides;
LLFloaterAO* LLFloaterAO::sInstance;
BOOL LLAO::mEnabled = FALSE;
F32 LLAO::mPeriod;
LLAOStandTimer* LLAO::mTimer = NULL;

LLAOStandTimer::LLAOStandTimer(F32 period) : LLEventTimer(period)
{
}
BOOL LLAOStandTimer::tick()
{
	if(!mPaused && LLAO::isEnabled() && !LLAO::mStandOverrides.empty())
	{
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
					avatarp->stopMotion(LLAO::mStandOverrides.back());
					avatarp->startMotion(LLAO::mStandOverrides.front());
					LLAO::mStandOverrides.push_back(LLAO::mStandOverrides.front());
					LLAO::mStandOverrides.pop_front();
					LLFloaterAO* ao = LLFloaterAO::sInstance;
					if(ao)
					{
						ao->mStandsCombo->setSimple(LLStringExplicit(LLAO::mStandOverrides.back().asString()));
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
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		gAgent.sendAnimationRequest(LLAO::mStandOverrides.back(), ANIM_REQUEST_STOP);
		avatarp->stopMotion(LLAO::mStandOverrides.back());
	}
	mEventTimer.reset();
	mEventTimer.stop();
	mPaused = TRUE;
}

void LLAOStandTimer::resume()
{
	if(!mPaused) return;
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		gAgent.sendAnimationRequest(LLAO::mStandOverrides.back(), ANIM_REQUEST_START);
		avatarp->startMotion(LLAO::mStandOverrides.back());
	}
	mEventTimer.reset();
	mEventTimer.start();
	mPaused = FALSE;
}

void LLAOStandTimer::reset()
{
	mEventTimer.reset();
}
//static
void LLAO::setup()
{
	mEnabled = gSavedSettings.getBOOL("AO.Enabled");
	mPeriod = gSavedSettings.getF32("AO.Period");
	if(mEnabled) mTimer = new LLAOStandTimer(mPeriod);
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
	if(mTimer)
		delete mTimer;
	if(mEnabled)
		mTimer = new LLAOStandTimer(mPeriod);
	else
		mTimer = NULL;
	return true;
}
//static
bool LLAO::handleAOEnabledChanged(const LLSD& newvalue)
{
	BOOL value = newvalue.asBoolean();
	mEnabled = value;
	runAnims(value);
	if(mTimer)
		delete mTimer;
	if(mEnabled)
		mTimer = new LLAOStandTimer(mPeriod);
	else
		mTimer = NULL;
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
//static
void LLAO::refresh()
{
	mOverrides.clear();
	mStandOverrides.clear();
	LLSD settings = gSavedPerAccountSettings.getLLSD("AO.Settings");
	//S32 version = (S32)settings["version"].asInteger();
	LLSD overrides = settings["overrides"];
	LLSD::map_iterator sd_it = overrides.beginMap();
	LLSD::map_iterator sd_end = overrides.endMap();
	for( ; sd_it != sd_end; sd_it++)
	{
		if(sd_it->first == "stands")
			for(LLSD::array_iterator itr = sd_it->second.beginArray();
				itr != sd_it->second.endArray(); ++itr)
					//list of listness
					mStandOverrides.push_back(itr->asUUID());
		// ignore if override is null key...
		if(sd_it->second.asUUID().isNull() 
			// don't allow override to be used as a trigger
			|| mOverrides.find(sd_it->second.asUUID()) != mOverrides.end())
			continue;
		else if(LLAO::isStand(LLUUID(sd_it->first)))
			//list of listness
			mStandOverrides.push_back(sd_it->second.asUUID());
		else
			//add to the list 
			mOverrides[LLUUID(sd_it->first)] = sd_it->second.asUUID();
	}
}

//static
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
	childSetAction("btn_save", onClickSave, this);
	childSetAction("btn_load", onClickLoad, this);

	childSetCommitCallback("line_walking", onCommitAnim, this);
	childSetCommitCallback("line_running", onCommitAnim, this);
	childSetCommitCallback("line_crouchwalk", onCommitAnim, this);
	childSetCommitCallback("line_flying", onCommitAnim, this);
	childSetCommitCallback("line_turn_left", onCommitAnim, this);
	childSetCommitCallback("line_turn_right", onCommitAnim, this);
	childSetCommitCallback("line_jumping", onCommitAnim, this);
	childSetCommitCallback("line_fly_up", onCommitAnim, this);
	childSetCommitCallback("line_crouching", onCommitAnim, this);
	childSetCommitCallback("line_fly_down", onCommitAnim, this);
	LLComboBox* combo = getChild<LLComboBox>( "combo_stands");
	combo->setAllowTextEntry(TRUE,36,TRUE);
	combo->setCommitCallback(onCommitStands);
	combo->setCallbackUserData(this);
	mStandsCombo = combo;
	childSetAction("combo_stands_add", onClickStandAdd, this);
	childSetAction("combo_stands_delete", onClickStandRemove, this);
	childSetCommitCallback("line_hover", onCommitAnim, this);
	childSetCommitCallback("line_sitting", onCommitAnim, this);
	childSetCommitCallback("line_prejump", onCommitAnim, this);
	childSetCommitCallback("line_falling", onCommitAnim, this);
	childSetCommitCallback("line_stride", onCommitAnim, this);
	childSetCommitCallback("line_soft_landing", onCommitAnim, this);
	childSetCommitCallback("line_medium_landing", onCommitAnim, this);
	childSetCommitCallback("line_hard_landing", onCommitAnim, this);
	childSetCommitCallback("line_flying_slow", onCommitAnim, this);
	childSetCommitCallback("line_sitting_on_ground", onCommitAnim, this);
	refresh();
	return TRUE;
}

std::string LLFloaterAO::idstr(LLUUID id)
{
	if(id.notNull()) return id.asString();
	else return "";
}

void LLFloaterAO::refresh()
{
	childSetText("line_walking", idstr(LLAO::mOverrides[LLUUID("6ed24bd8-91aa-4b12-ccc7-c97c857ab4e0")]));
	childSetText("line_running", idstr(LLAO::mOverrides[LLUUID("05ddbff8-aaa9-92a1-2b74-8fe77a29b445")]));
	childSetText("line_crouchwalk", idstr(LLAO::mOverrides[LLUUID("47f5f6fb-22e5-ae44-f871-73aaaf4a6022")]));
	childSetText("line_flying", idstr(LLAO::mOverrides[LLUUID("aec4610c-757f-bc4e-c092-c6e9caf18daf")]));
	childSetText("line_turn_left", idstr(LLAO::mOverrides[LLUUID("56e0ba0d-4a9f-7f27-6117-32f2ebbf6135")]));
	childSetText("line_turn_right", idstr(LLAO::mOverrides[LLUUID("2d6daa51-3192-6794-8e2e-a15f8338ec30")]));
	childSetText("line_jumping", idstr(LLAO::mOverrides[LLUUID("2305bd75-1ca9-b03b-1faa-b176b8a8c49e")]));
	childSetText("line_fly_up", idstr(LLAO::mOverrides[LLUUID("62c5de58-cb33-5743-3d07-9e4cd4352864")]));
	childSetText("line_crouching", idstr(LLAO::mOverrides[LLUUID("201f3fdf-cb1f-dbec-201f-7333e328ae7c")]));
	childSetText("line_fly_down", idstr(LLAO::mOverrides[LLUUID("20f063ea-8306-2562-0b07-5c853b37b31e")]));
	mStandsCombo->clearRows();
	for(std::list<LLUUID>::iterator itr = LLAO::mStandOverrides.begin();itr != LLAO::mStandOverrides.end();
		itr++)
	{
		mStandsCombo->add((*itr).asString());
	}
	mStandsCombo->setSimple(LLStringExplicit(LLAO::mStandOverrides.back().asString()));
	childSetText("line_hover", idstr(LLAO::mOverrides[LLUUID("4ae8016b-31b9-03bb-c401-b1ea941db41d")]));
	childSetText("line_sitting", idstr(LLAO::mOverrides[LLUUID("1a5fe8ac-a804-8a5d-7cbd-56bd83184568")]));
	childSetText("line_prejump", idstr(LLAO::mOverrides[LLUUID("7a4e87fe-de39-6fcb-6223-024b00893244")]));
	childSetText("line_falling", idstr(LLAO::mOverrides[LLUUID("666307d9-a860-572d-6fd4-c3ab8865c094")]));
	childSetText("line_stride", idstr(LLAO::mOverrides[LLUUID("1cb562b0-ba21-2202-efb3-30f82cdf9595")]));
	childSetText("line_soft_landing", idstr(LLAO::mOverrides[LLUUID("7a17b059-12b2-41b1-570a-186368b6aa6f")]));
	childSetText("line_medium_landing", idstr(LLAO::mOverrides[LLUUID("f4f00d6e-b9fe-9292-f4cb-0ae06ea58d57")]));
	childSetText("line_hard_landing", idstr(LLAO::mOverrides[LLUUID("3da1d753-028a-5446-24f3-9c9b856d9422")]));
	childSetText("line_flying_slow", idstr(LLAO::mOverrides[LLUUID("2b5a38b2-5e00-3a97-a495-4c826bc443e6")]));
	childSetText("line_sitting_on_ground", idstr(LLAO::mOverrides[LLUUID("1a2bd58e-87ff-0df8-0b4c-53e047b0bb6e")]));
}
// static
void LLFloaterAO::onCommitStands(LLUICtrl* ctrl, void* user_data)
{
	//LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = LLUUID(ctrl->getValue());
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr != LLAO::mStandOverrides.end())
	{
		//back is always last played
		avatarp->stopMotion(LLAO::mStandOverrides.back());
		avatarp->startMotion(id);
		LLAO::mStandOverrides.push_back(id);
		LLAO::mStandOverrides.erase(itr);

		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);
}
// static
void LLFloaterAO::onCommitAnim(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;

	LLSD overrides;
	LLUUID id;
	id = LLUUID(floater->childGetValue("line_walking").asString());
	if(id.notNull()) overrides["6ed24bd8-91aa-4b12-ccc7-c97c857ab4e0"] = id;
	id = LLUUID(floater->childGetValue("line_running").asString());
	if(id.notNull()) overrides["05ddbff8-aaa9-92a1-2b74-8fe77a29b445"] = id;
	id = LLUUID(floater->childGetValue("line_crouchwalk").asString());
	if(id.notNull()) overrides["47f5f6fb-22e5-ae44-f871-73aaaf4a6022"] = id;
	id = LLUUID(floater->childGetValue("line_flying").asString());
	if(id.notNull()) overrides["aec4610c-757f-bc4e-c092-c6e9caf18daf"] = id;
	id = LLUUID(floater->childGetValue("line_turn_left").asString());
	if(id.notNull()) overrides["56e0ba0d-4a9f-7f27-6117-32f2ebbf6135"] = id;
	id = LLUUID(floater->childGetValue("line_turn_right").asString());
	if(id.notNull()) overrides["2d6daa51-3192-6794-8e2e-a15f8338ec30"] = id;
	id = LLUUID(floater->childGetValue("line_jumping").asString());
	if(id.notNull()) overrides["2305bd75-1ca9-b03b-1faa-b176b8a8c49e"] = id;
	id = LLUUID(floater->childGetValue("line_fly_up").asString());
	if(id.notNull()) overrides["62c5de58-cb33-5743-3d07-9e4cd4352864"] = id;
	id = LLUUID(floater->childGetValue("line_crouching").asString());
	if(id.notNull()) overrides["201f3fdf-cb1f-dbec-201f-7333e328ae7c"] = id;
	id = LLUUID(floater->childGetValue("line_fly_down").asString());
	if(id.notNull()) overrides["20f063ea-8306-2562-0b07-5c853b37b31e"] = id;
	id = LLUUID(floater->childGetValue("line_hover").asString());
	if(id.notNull()) overrides["4ae8016b-31b9-03bb-c401-b1ea941db41d"] = id;
	id = LLUUID(floater->childGetValue("line_sitting").asString());
	if(id.notNull()) overrides["1a5fe8ac-a804-8a5d-7cbd-56bd83184568"] = id;
	id = LLUUID(floater->childGetValue("line_prejump").asString());
	if(id.notNull()) overrides["7a4e87fe-de39-6fcb-6223-024b00893244"] = id;
	id = LLUUID(floater->childGetValue("line_falling").asString());
	if(id.notNull()) overrides["666307d9-a860-572d-6fd4-c3ab8865c094"] = id;
	id = LLUUID(floater->childGetValue("line_stride").asString());
	if(id.notNull()) overrides["1cb562b0-ba21-2202-efb3-30f82cdf9595"] = id;
	id = LLUUID(floater->childGetValue("line_soft_landing").asString());
	if(id.notNull()) overrides["7a17b059-12b2-41b1-570a-186368b6aa6f"] = id;
	id = LLUUID(floater->childGetValue("line_medium_landing").asString());
	if(id.notNull()) overrides["f4f00d6e-b9fe-9292-f4cb-0ae06ea58d57"] = id;
	id = LLUUID(floater->childGetValue("line_hard_landing").asString());
	if(id.notNull()) overrides["3da1d753-028a-5446-24f3-9c9b856d9422"] = id;
	id = LLUUID(floater->childGetValue("line_flying_slow").asString());
	if(id.notNull()) overrides["2b5a38b2-5e00-3a97-a495-4c826bc443e6"] = id;
	id = LLUUID(floater->childGetValue("line_sitting_on_ground").asString());
	if(id.notNull()) overrides["1a2bd58e-87ff-0df8-0b4c-53e047b0bb6e"] = id;
	for(std::list<LLUUID>::iterator itr = LLAO::mStandOverrides.begin();itr != LLAO::mStandOverrides.end();
		itr++)
	{
		overrides["stands"].append((*itr));
	}
	LLSD settings;
	settings["version"] = 2;
	settings["overrides"] = overrides;
	gSavedPerAccountSettings.setLLSD("AO.Settings", settings);
	LLAO::refresh();
	floater->refresh();
}

//static
void LLFloaterAO::onClickStandRemove(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = LLUUID(floater->mStandsCombo->getValue());
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
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
	onCommitAnim(NULL,user_data);
}
//static
void LLFloaterAO::onClickStandAdd(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = LLUUID(floater->mStandsCombo->getValue());
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr == LLAO::mStandOverrides.end())
	{
		//back is always last played
		avatarp->stopMotion(LLAO::mStandOverrides.back());
		avatarp->startMotion(id);
		LLAO::mStandOverrides.push_back(id);

		floater->refresh();
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
