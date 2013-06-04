/** 
 * @file llfloateractivespeakers.cpp
 * @brief Management interface for muting and controlling volume of residents currently speaking
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
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

#include "llviewerprecompiledheaders.h"

#include "llfloateractivespeakers.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "llmutelist.h"
#include "llscrolllistctrl.h"
#include "llspeakers.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"

// [RLVa:KB]
#include "rlvhandler.h"
// [/RLVa:KB]

using namespace LLOldEvents;

const F32 RESORT_TIMEOUT = 5.f; // seconds of mouse inactivity before it's ok to sort regardless of mouse-in-view.

//
// LLFloaterActiveSpeakers
//

LLFloaterActiveSpeakers::LLFloaterActiveSpeakers(const LLSD& seed) : mPanel(NULL)
{
	mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, NULL);
	// do not automatically open singleton floaters (as result of getInstance())
	BOOL no_open = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_active_speakers.xml", &getFactoryMap(), no_open);	
	//RN: for now, we poll voice client every frame to get voice amplitude feedback
	//LLVoiceClient::getInstance()->addObserver(this);
	mPanel->refreshSpeakers();
}

LLFloaterActiveSpeakers::~LLFloaterActiveSpeakers()
{
}

void LLFloaterActiveSpeakers::onOpen()
{
	gSavedSettings.setBOOL("ShowActiveSpeakers", TRUE);
}

void LLFloaterActiveSpeakers::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowActiveSpeakers", FALSE);
	}
	setVisible(FALSE);
}

void LLFloaterActiveSpeakers::draw()
{
	// update state every frame to get live amplitude feedback
	mPanel->refreshSpeakers();
	LLFloater::draw();
}

BOOL LLFloaterActiveSpeakers::postBuild()
{
	mPanel = getChild<LLPanelActiveSpeakers>("active_speakers_panel");
	return TRUE;
}

void LLFloaterActiveSpeakers::onParticipantsChanged()
{
	//refresh();
}

//static
void* LLFloaterActiveSpeakers::createSpeakersPanel(void* data)
{
	// don't show text only speakers
	return new LLPanelActiveSpeakers(LLActiveSpeakerMgr::getInstance(), FALSE);
}

//
// LLPanelActiveSpeakers::SpeakerMuteListener
//
bool LLPanelActiveSpeakers::SpeakerMuteListener::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (event->getValue().asString() == "voice")
	{
		mPanel->childSetValue("moderator_allow_voice", !speakerp->mModeratorMutedVoice);
	}
	if (event->getValue().asString() == "text")
	{
		mPanel->childSetValue("moderator_allow_text", !speakerp->mModeratorMutedText);
	}
	return true;
}


//
// LLPanelActiveSpeakers::SpeakerAddListener
//
bool LLPanelActiveSpeakers::SpeakerAddListener::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	mPanel->addSpeaker(event->getValue().asUUID());
	return true;
}


//
// LLPanelActiveSpeakers::SpeakerRemoveListener
//
bool LLPanelActiveSpeakers::SpeakerRemoveListener::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	mPanel->removeSpeaker(event->getValue().asUUID());
	return true;
}

//
// LLPanelActiveSpeakers::SpeakerClearListener
//
bool LLPanelActiveSpeakers::SpeakerClearListener::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	mPanel->mSpeakerList->clearRows();
	return true;
}


//
// LLPanelActiveSpeakers
//
LLPanelActiveSpeakers::LLPanelActiveSpeakers(LLSpeakerMgr* data_source, BOOL show_text_chatters) : 
	mSpeakerList(NULL),
	mMuteVoiceCtrl(NULL),
	mMuteTextCtrl(NULL),
	mNameText(NULL),
	mProfileBtn(NULL),
	mShowTextChatters(show_text_chatters),
	mSpeakerMgr(data_source)
{
	setMouseOpaque(FALSE);
	mSpeakerMuteListener = new SpeakerMuteListener(this);
	mSpeakerAddListener = new SpeakerAddListener(this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(this);
	mSpeakerClearListener = new SpeakerClearListener(this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
}

BOOL LLPanelActiveSpeakers::postBuild()
{
	std::string sort_column = gSavedSettings.getString(std::string("FloaterActiveSpeakersSortColumn"));
	BOOL sort_ascending     = gSavedSettings.getBOOL(  std::string("FloaterActiveSpeakersSortAscending"));

	mSpeakerList = getChild<LLScrollListCtrl>("speakers_list");
	mSpeakerList->sortByColumn(sort_column, sort_ascending);
	mSpeakerList->setDoubleClickCallback(boost::bind(&onDoubleClickSpeaker,this));
	mSpeakerList->setCommitOnSelectionChange(TRUE);
	mSpeakerList->setCommitCallback(boost::bind(&LLPanelActiveSpeakers::handleSpeakerSelect,this));
	mSpeakerList->setSortChangedCallback(boost::bind(&LLPanelActiveSpeakers::onSortChanged,this));
	mSpeakerList->setCallbackUserData(this);

	if ((mMuteTextCtrl = findChild<LLUICtrl>("mute_text_btn")))
		childSetCommitCallback("mute_text_btn", onClickMuteTextCommit, this);

	mMuteVoiceCtrl = getChild<LLUICtrl>("mute_btn");
	childSetCommitCallback("mute_btn", onClickMuteVoiceCommit, this);

	childSetCommitCallback("speaker_volume", onVolumeChange, this);

	mNameText = findChild<LLTextBox>("resident_name");
	
	if ((mProfileBtn = findChild<LLButton>("profile_btn")))
		childSetAction("profile_btn", onClickProfile, this);

	if (findChild<LLUICtrl>("moderator_allow_voice"))
		childSetCommitCallback("moderator_allow_voice", onModeratorMuteVoice, this);
	if (findChild<LLUICtrl>("moderator_allow_text"))
		childSetCommitCallback("moderator_allow_text", onModeratorMuteText, this);
	if (findChild<LLUICtrl>("moderator_mode"))
		childSetCommitCallback("moderation_mode", onChangeModerationMode, this);

	mVolumeSlider.connect(this,"speaker_volume");

	// update speaker UI
	handleSpeakerSelect();

	return TRUE;
}

void LLPanelActiveSpeakers::addSpeaker(const LLUUID& speaker_id)
{
	if (mSpeakerList->getItemIndex(speaker_id) >= 0)
	{
		// already have this speaker
		return;
	}

	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp)
	{
		// since we are forced to sort by text, encode sort order as string
		std::string speaking_order_sort_string = llformat("%010d", speakerp->mSortIndex);

		LLSD row;
		row["id"] = speaker_id;

		LLSD& columns = row["columns"];

		columns[0]["column"] = "icon_speaking_status";
		columns[0]["type"] = "icon";
		columns[0]["value"] = "icn_active-speakers-dot-lvl0.tga";

		std::string speaker_name;
		if (speakerp->mDisplayName.empty())
		{
			speaker_name = LLCacheName::getDefaultName();
		}
		else
		{
			speaker_name = speakerp->mDisplayName;
		}
		columns[1]["column"] = "speaker_name";
		columns[1]["type"] = "text";
		columns[1]["value"] = speaker_name;

		columns[2]["column"] = "speaking_status";
		columns[2]["type"] = "text";
		
		// print speaking ordinal in a text-sorting friendly manner
		columns[2]["value"] = speaking_order_sort_string;

		mSpeakerList->addElement(row);
	}
}

void LLPanelActiveSpeakers::removeSpeaker(const LLUUID& speaker_id)
{
	mSpeakerList->deleteSingleItem(mSpeakerList->getItemIndex(speaker_id));
}

void LLPanelActiveSpeakers::handleSpeakerSelect()
{
	LLUUID speaker_id = mSpeakerList->getValue().asUUID();
	LLPointer<LLSpeaker> selected_speakerp = mSpeakerMgr->findSpeaker(speaker_id);

	if (selected_speakerp.notNull())
	{
		// since setting these values is delayed by a round trip to the Vivox servers
		// update them only when selecting a new speaker or
		// asynchronously when an update arrives
		childSetValue("moderator_allow_voice", selected_speakerp ? !selected_speakerp->mModeratorMutedVoice : TRUE);
		childSetValue("moderator_allow_text", selected_speakerp ? !selected_speakerp->mModeratorMutedText : TRUE);

		mSpeakerMuteListener->clearDispatchers();
		selected_speakerp->addListener(mSpeakerMuteListener);
	}
}

void LLPanelActiveSpeakers::refreshSpeakers()
{
	// store off current selection and scroll state to preserve across list rebuilds
	LLUUID selected_id = mSpeakerList->getSelectedValue().asUUID();
	S32 scroll_pos = mSpeakerList->getScrollInterface()->getScrollPos();

	// decide whether it's ok to resort the list then update the speaker manager appropriately.
	// rapid resorting by activity makes it hard to interact with speakers in the list
	// so we freeze the sorting while the user appears to be interacting with the control.
	// we assume this is the case whenever the mouse pointer is within the active speaker
	// panel and hasn't been motionless for more than a few seconds. see DEV-6655 -MG
	LLRect screen_rect;
	localRectToScreen(getLocalRect(), &screen_rect);
	BOOL mouse_in_view = screen_rect.pointInRect(gViewerWindow->getCurrentMouseX(), gViewerWindow->getCurrentMouseY());
	F32 mouses_last_movement = gMouseIdleTimer.getElapsedTimeF32();
	BOOL sort_ok = ! (mouse_in_view && mouses_last_movement<RESORT_TIMEOUT);
	mSpeakerMgr->update(sort_ok);

	const std::string icon_image_0 = "icn_active-speakers-dot-lvl0.tga";
	const std::string icon_image_1 = "icn_active-speakers-dot-lvl1.tga";
	const std::string icon_image_2 = "icn_active-speakers-dot-lvl2.tga";

	std::vector<LLScrollListItem*> items = mSpeakerList->getAllData();

	std::string mute_icon_image = "mute_icon.tga";

	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, mShowTextChatters);
	for (std::vector<LLScrollListItem*>::iterator item_it = items.begin();
		item_it != items.end();
		++item_it)
	{
		LLScrollListItem* itemp = (*item_it);
		LLUUID speaker_id = itemp->getUUID();

		LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
		if (!speakerp)
		{
			continue;
		}

		// since we are forced to sort by text, encode sort order as string
		std::string speaking_order_sort_string = llformat("%010d", speakerp->mSortIndex);

		LLScrollListCell* icon_cell = itemp->getColumn(0);
		if (icon_cell)
		{

			std::string icon_image_id;

			S32 icon_image_idx = llmin(2, llfloor((speakerp->mSpeechVolume / LLVoiceClient::OVERDRIVEN_POWER_LEVEL) * 3.f));
			switch(icon_image_idx)
			{
			case 0:
				icon_image_id = icon_image_0;
				break;
			case 1:
				icon_image_id = icon_image_1;
				break;
			case 2:
				icon_image_id = icon_image_2;
				break;
			}

			LLColor4 icon_color;

			if (speakerp->mStatus == LLSpeaker::STATUS_MUTED)
			{
				icon_cell->setValue(mute_icon_image);
				if(speakerp->mModeratorMutedVoice)
				{
					icon_color.setVec(0.5f, 0.5f, 0.5f, 1.f);
				}
				else
				{
					icon_color.setVec(1.f, 71.f / 255.f, 71.f / 255.f, 1.f);
				}
			}
			else
			{
				icon_cell->setValue(icon_image_id);
				icon_color = speakerp->mDotColor;

				if (speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE) // if voice is disabled for this speaker
				{
					// non voice speakers have hidden icons, render as transparent
					icon_color.setVec(0.f, 0.f, 0.f, 0.f);
				}
			}

			icon_cell->setColor(icon_color);

			if (speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE && speakerp->mStatus != LLSpeaker::STATUS_MUTED) // if voice is disabled for this speaker
			{
				// non voice speakers have hidden icons, render as transparent
				icon_cell->setColor(LLColor4::transparent);
			}
		}

		// update name column
		LLScrollListCell* name_cell = itemp->getColumn(1);
		if (name_cell)
		{
			if (speakerp->mStatus == LLSpeaker::STATUS_NOT_IN_CHANNEL)	
			{
				// draw inactive speakers in different color
				static LLCachedControl<LLColor4> sSpeakersInactive(gColors, "SpeakersInactive");

				name_cell->setColor(sSpeakersInactive);
			}
			else
			{
				static LLCachedControl<LLColor4> sDefaultListText(gColors, "DefaultListText");

				name_cell->setColor(sDefaultListText);
			}
			// <edit>
			if(!mShowTextChatters && !(speakerp->mStatus == LLSpeaker::STATUS_NOT_IN_CHANNEL) && speakerp->mID != gAgent.getID())
			{
				bool found = false;
				for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
						iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
				{
					LLViewerRegion* regionp = *iter;
					// let us check to see if they are actually in the sim
					if(regionp)
					{
						if(regionp->mMapAvatarIDs.find(speakerp->mID) != -1)
						{
							found = true;
							break;
						}
					}
				}
				if(!found)
				{
					static LLCachedControl<LLColor4> sSpeakersGhost(gColors, "SpeakersGhost");

					name_cell->setColor(sSpeakersGhost);
				}
			}
			// </edit>

			std::string speaker_name;
			if (speakerp->mDisplayName.empty())
			{
				speaker_name = LLCacheName::getDefaultName();
			}
			else
			{
				speaker_name = speakerp->mDisplayName;
			}

			if (speakerp->mIsModerator)
			{
				speaker_name += std::string(" ") + getString("moderator_label");
			}
			
			name_cell->setValue(speaker_name);
			((LLScrollListText*)name_cell)->setFontStyle(speakerp->mIsModerator ? LLFontGL::BOLD : LLFontGL::NORMAL);
		}

		// update speaking order column
		LLScrollListCell* speaking_status_cell = itemp->getColumn(2);
		if (speaking_status_cell)
		{
			// print speaking ordinal in a text-sorting friendly manner
			speaking_status_cell->setValue(speaking_order_sort_string);
		}
	}

	// we potentially modified the sort order by touching the list items
	mSpeakerList->setNeedsSort();

	LLPointer<LLSpeaker> selected_speakerp = mSpeakerMgr->findSpeaker(selected_id);
	// update UI for selected participant
	if (mMuteVoiceCtrl)
	{
		mMuteVoiceCtrl->setValue(LLMuteList::getInstance()->isMuted(selected_id, LLMute::flagVoiceChat));
		mMuteVoiceCtrl->setEnabled(LLVoiceClient::getInstance()->voiceEnabled()
									&& LLVoiceClient::getInstance()->getVoiceEnabled(selected_id)
									&& selected_id.notNull() 
									&& selected_id != gAgent.getID() 
									&& (selected_speakerp.notNull() && (selected_speakerp->mType == LLSpeaker::SPEAKER_AGENT || selected_speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL)));

	}
	if (mMuteTextCtrl)
	{
		mMuteTextCtrl->setValue(LLMuteList::getInstance()->isMuted(selected_id, LLMute::flagTextChat));
		mMuteTextCtrl->setEnabled(selected_id.notNull() 
								&& selected_id != gAgent.getID() 
								&& selected_speakerp.notNull() 
								&& selected_speakerp->mType != LLSpeaker::SPEAKER_EXTERNAL
								&& !LLMuteList::getInstance()->isLinden(selected_id));
	}
	mVolumeSlider->setValue(LLVoiceClient::getInstance()->getUserVolume(selected_id));
	mVolumeSlider->setEnabled(LLVoiceClient::getInstance()->voiceEnabled()
					&& LLVoiceClient::getInstance()->getVoiceEnabled(selected_id)
					&& selected_id.notNull() 
					&& selected_id != gAgent.getID() 
					&& (selected_speakerp.notNull() && (selected_speakerp->mType == LLSpeaker::SPEAKER_AGENT || selected_speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL)));

	if (LLView* view = findChild<LLView>("moderator_controls_label"))
		view->setEnabled(selected_id.notNull());

	if (LLView* view = findChild<LLView>("moderator_allow_voice"))
		view->setEnabled(selected_id.notNull() && mSpeakerMgr->isVoiceActive() && LLVoiceClient::getInstance()->getVoiceEnabled(selected_id));

	if (LLView* view = findChild<LLView>("moderator_allow_text"))
		view->setEnabled(selected_id.notNull());

	if (mProfileBtn)
	{
		mProfileBtn->setEnabled(selected_id.notNull() && (selected_speakerp.notNull() && selected_speakerp->mType != LLSpeaker::SPEAKER_EXTERNAL) );
	}

	// show selected user name in large font
	if (mNameText)
	{
		if (selected_speakerp)
		{
			mNameText->setValue(selected_speakerp->mDisplayName);
		}
		else
		{
			mNameText->setValue(LLStringUtil::null);
		}
	}

	//update moderator capabilities
	LLPointer<LLSpeaker> self_speakerp = mSpeakerMgr->findSpeaker(gAgent.getID());
	if(self_speakerp)
	{
		if (LLView* view = findChild<LLView>("moderation_mode_panel"))
			view->setVisible(self_speakerp->mIsModerator && mSpeakerMgr->isVoiceActive());
		if (LLView* view = findChild<LLView>("moderator_controls"))
			view->setVisible(self_speakerp->mIsModerator);
	}

	// keep scroll value stable
	mSpeakerList->getScrollInterface()->setScrollPos(scroll_pos);
}

void LLPanelActiveSpeakers::setVoiceModerationCtrlMode(
	const BOOL& moderated_voice)
{
	LLUICtrl* voice_moderation_ctrl = getChild<LLUICtrl>("moderation_mode");

	if ( voice_moderation_ctrl )
	{
		std::string value;

		value = moderated_voice ? "moderated" : "unmoderated";
		voice_moderation_ctrl->setValue(value);
	}
}

//static
void LLPanelActiveSpeakers::onClickMuteTextCommit(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
	BOOL is_muted = LLMuteList::getInstance()->isMuted(speaker_id, LLMute::flagTextChat);
	std::string name;

	//fill in name using voice client's copy of name cache
	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}
	
	name = speakerp->mDisplayName;

	LLMute mute(speaker_id, name, speakerp->mType == LLSpeaker::SPEAKER_AGENT ? LLMute::AGENT : LLMute::OBJECT);

	if (!is_muted)
	{
		LLMuteList::getInstance()->add(mute, LLMute::flagTextChat);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, LLMute::flagTextChat);
	}
}


//static
void LLPanelActiveSpeakers::onClickMuteVoiceCommit(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
	BOOL is_muted = LLMuteList::getInstance()->isMuted(speaker_id, LLMute::flagVoiceChat);
	std::string name;

	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}

	name = speakerp->mDisplayName;

	// muting voice means we're dealing with an agent or an external voice client which won't stay muted between sessions
	LLMute mute(speaker_id, name, speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL ? LLMute::EXTERNAL : LLMute::AGENT);

	if (!is_muted)
	{
		LLMuteList::getInstance()->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, LLMute::flagVoiceChat);
	}
}


//static
void LLPanelActiveSpeakers::onVolumeChange(LLUICtrl* source, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();

	F32 new_volume = (F32)panelp->childGetValue("speaker_volume").asReal();
	LLVoiceClient::getInstance()->setUserVolume(speaker_id, new_volume);
}

//static 
void LLPanelActiveSpeakers::onClickProfile(void* user_data)
{
// [RLVa:KB] - Checked: 2009-07-10 (RLVa-1.0.0g) | Added: RLVa-1.0.0g
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
	{
		return;
	}
// [/RLVa:KB]

	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLAvatarActions::showProfile(panelp->mSpeakerList->getValue().asUUID());
}

//static
void LLPanelActiveSpeakers::onDoubleClickSpeaker(void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLAvatarActions::startIM(panelp->mSpeakerList->getValue().asUUID());
}

//static
void LLPanelActiveSpeakers::onSortChanged(void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	std::string sort_column = panelp->mSpeakerList->getSortColumnName();
	BOOL sort_ascending = panelp->mSpeakerList->getSortAscending();
	gSavedSettings.setString(std::string("FloaterActiveSpeakersSortColumn"), sort_column);
	gSavedSettings.setBOOL(  std::string("FloaterActiveSpeakersSortAscending"), sort_ascending);
}


//static 
void LLPanelActiveSpeakers::onModeratorMuteVoice(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	LLUICtrl* speakers_list = self->getChild<LLUICtrl>("speakers_list");
	if (!speakers_list || !gAgent.getRegion()) return;
	if (LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(self->mSpeakerMgr))
		mgr->moderateVoiceParticipant(speakers_list->getValue(), ctrl->getValue());
}

//static 
void LLPanelActiveSpeakers::onModeratorMuteText(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	LLUICtrl* speakers_list = self->getChild<LLUICtrl>("speakers_list");
	if (!speakers_list || !gAgent.getRegion()) return;
	if (LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(self->mSpeakerMgr))
		mgr->toggleAllowTextChat(speakers_list->getValue());
}

//static
void LLPanelActiveSpeakers::onChangeModerationMode(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	if (!gAgent.getRegion()) return;
	// Singu Note: moderateVoiceAllParticipants ends up flipping the boolean passed to it before the actual post
	if (LLIMSpeakerMgr* speaker_manager = dynamic_cast<LLIMSpeakerMgr*>(self->mSpeakerMgr))
	{
		if (ctrl->getValue().asString() == "unmoderated")
		{
			speaker_manager->moderateVoiceAllParticipants(true);
		}
		else if (ctrl->getValue().asString() == "moderated")
		{
			speaker_manager->moderateVoiceAllParticipants(false);
		}
	}
}

