// <edit>

#include "llviewerprecompiledheaders.h"
#include "llao.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llfilepicker.h"
#include "llsdserialize.h"

std::map<LLUUID,LLUUID> LLAO::mOverrides;
LLFloaterAO* LLFloaterAO::sInstance;

//static
void LLAO::refresh()
{
	mOverrides.clear();
	LLSD settings = gSavedPerAccountSettings.getLLSD("AO.Settings");
	LLSD overrides = settings["overrides"];
	LLSD::map_iterator sd_it = overrides.beginMap();
	LLSD::map_iterator sd_end = overrides.endMap();
	for( ; sd_it != sd_end; sd_it++)
	{
		// don't allow override to be used as a trigger
		if(mOverrides.find(sd_it->second.asUUID()) == mOverrides.end())
		{
			// ignore if override is null key...
			if(sd_it->second.asUUID().notNull())
			{
				mOverrides[LLUUID(sd_it->first)] = sd_it->second.asUUID();
			}
		}
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
	childSetCommitCallback("line_stand1", onCommitAnim, this);
	childSetCommitCallback("line_stand2", onCommitAnim, this);
	childSetCommitCallback("line_stand3", onCommitAnim, this);
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
	LLSD settings = gSavedPerAccountSettings.getLLSD("AO.Settings");
	LLSD overrides = settings["overrides"];
	childSetText("line_walking", idstr(overrides["6ed24bd8-91aa-4b12-ccc7-c97c857ab4e0"]));
	childSetText("line_running", idstr(overrides["05ddbff8-aaa9-92a1-2b74-8fe77a29b445"]));
	childSetText("line_crouchwalk", idstr(overrides["47f5f6fb-22e5-ae44-f871-73aaaf4a6022"]));
	childSetText("line_flying", idstr(overrides["aec4610c-757f-bc4e-c092-c6e9caf18daf"]));
	childSetText("line_turn_left", idstr(overrides["56e0ba0d-4a9f-7f27-6117-32f2ebbf6135"]));
	childSetText("line_turn_right", idstr(overrides["2d6daa51-3192-6794-8e2e-a15f8338ec30"]));
	childSetText("line_jumping", idstr(overrides["2305bd75-1ca9-b03b-1faa-b176b8a8c49e"]));
	childSetText("line_fly_up", idstr(overrides["62c5de58-cb33-5743-3d07-9e4cd4352864"]));
	childSetText("line_crouching", idstr(overrides["201f3fdf-cb1f-dbec-201f-7333e328ae7c"]));
	childSetText("line_fly_down", idstr(overrides["20f063ea-8306-2562-0b07-5c853b37b31e"]));
	childSetText("line_stand1", idstr(overrides["2408fe9e-df1d-1d7d-f4ff-1384fa7b350f"]));
	childSetText("line_stand2", idstr(overrides["15468e00-3400-bb66-cecc-646d7c14458e"]));
	childSetText("line_stand3", idstr(overrides["370f3a20-6ca6-9971-848c-9a01bc42ae3c"]));
	childSetText("line_hover", idstr(overrides["4ae8016b-31b9-03bb-c401-b1ea941db41d"]));
	childSetText("line_sitting", idstr(overrides["1a5fe8ac-a804-8a5d-7cbd-56bd83184568"]));
	childSetText("line_prejump", idstr(overrides["7a4e87fe-de39-6fcb-6223-024b00893244"]));
	childSetText("line_falling", idstr(overrides["666307d9-a860-572d-6fd4-c3ab8865c094"]));
	childSetText("line_stride", idstr(overrides["1cb562b0-ba21-2202-efb3-30f82cdf9595"]));
	childSetText("line_soft_landing", idstr(overrides["7a17b059-12b2-41b1-570a-186368b6aa6f"]));
	childSetText("line_medium_landing", idstr(overrides["f4f00d6e-b9fe-9292-f4cb-0ae06ea58d57"]));
	childSetText("line_hard_landing", idstr(overrides["3da1d753-028a-5446-24f3-9c9b856d9422"]));
	childSetText("line_flying_slow", idstr(overrides["2b5a38b2-5e00-3a97-a495-4c826bc443e6"]));
	childSetText("line_sitting_on_ground", idstr(overrides["1a2bd58e-87ff-0df8-0b4c-53e047b0bb6e"]));
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
	id = LLUUID(floater->childGetValue("line_stand1").asString());
	if(id.notNull()) overrides["2408fe9e-df1d-1d7d-f4ff-1384fa7b350f"] = id;
	id = LLUUID(floater->childGetValue("line_stand2").asString());
	if(id.notNull()) overrides["15468e00-3400-bb66-cecc-646d7c14458e"] = id;
	id = LLUUID(floater->childGetValue("line_stand3").asString());
	if(id.notNull()) overrides["370f3a20-6ca6-9971-848c-9a01bc42ae3c"] = id;
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
	LLSD settings;
	settings["version"] = 1;
	settings["overrides"] = overrides;
	gSavedPerAccountSettings.setLLSD("AO.Settings", settings);
	LLAO::refresh();
	floater->refresh();
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
			gSavedPerAccountSettings.setLLSD("AO.Settings", data);
			LLAO::refresh();
			floater->refresh();
		}
		xml_file.close();
	}
}

// </edit>
