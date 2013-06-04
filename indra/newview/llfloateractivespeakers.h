/** 
 * @file llfloateractivespeakers.h
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

#ifndef LL_LLFLOATERACTIVESPEAKERS_H
#define LL_LLFLOATERACTIVESPEAKERS_H

#include "llfloater.h"
#include "llvoiceclient.h"
#include "llframetimer.h"
#include "llevent.h"

class LLScrollListCtrl;
class LLButton;
class LLPanelActiveSpeakers;
class LLSpeakerMgr;
class LLTextBox;


class LLFloaterActiveSpeakers : 
	public LLFloaterSingleton<LLFloaterActiveSpeakers>, 
	public LLFloater, 
	public LLVoiceClientParticipantObserver
{
	// friend of singleton class to allow construction inside getInstance() since constructor is protected
	// to enforce singleton constraint
	friend class LLUISingleton<LLFloaterActiveSpeakers, VisibilityPolicy<LLFloater> >;
public:
	virtual ~LLFloaterActiveSpeakers();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();

	/*virtual*/ void onParticipantsChanged();

	static void* createSpeakersPanel(void* data);

protected:
	LLFloaterActiveSpeakers(const LLSD& seed);

	LLPanelActiveSpeakers*	mPanel;
};

class LLPanelActiveSpeakers : public LLPanel
{
public:
	LLPanelActiveSpeakers(LLSpeakerMgr* data_source, BOOL show_text_chatters);

	/*virtual*/ BOOL postBuild();

	void handleSpeakerSelect();
	void refreshSpeakers();

	void setVoiceModerationCtrlMode(const BOOL& moderated_voice);
	
	static void onClickMuteVoiceCommit(LLUICtrl* ctrl, void* user_data);
	static void onClickMuteTextCommit(LLUICtrl* ctrl, void* user_data);
	static void onVolumeChange(LLUICtrl* source, void* user_data);
	static void onClickProfile(void* user_data);
	static void onDoubleClickSpeaker(void* user_data);
	static void onSortChanged(void* user_data);
	static void	onModeratorMuteVoice(LLUICtrl* ctrl, void* user_data);
	static void	onModeratorMuteText(LLUICtrl* ctrl, void* user_data);
	static void	onChangeModerationMode(LLUICtrl* ctrl, void* user_data);

protected:
	class SpeakerMuteListener : public LLOldEvents::LLSimpleListener
	{
	public:
		SpeakerMuteListener(LLPanelActiveSpeakers* panel) : mPanel(panel) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

		LLPanelActiveSpeakers* mPanel;
	};

	friend class SpeakerAddListener;
	class SpeakerAddListener : public LLOldEvents::LLSimpleListener
	{
	public:
		SpeakerAddListener(LLPanelActiveSpeakers* panel) : mPanel(panel) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

		LLPanelActiveSpeakers* mPanel;
	};

	friend class SpeakerRemoveListener;
	class SpeakerRemoveListener : public LLOldEvents::LLSimpleListener
	{
	public:
		SpeakerRemoveListener(LLPanelActiveSpeakers* panel) : mPanel(panel) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

		LLPanelActiveSpeakers* mPanel;
	};


	friend class SpeakerClearListener;
	class SpeakerClearListener : public LLOldEvents::LLSimpleListener
	{
	public:
		SpeakerClearListener(LLPanelActiveSpeakers* panel) : mPanel(panel) {}

		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

		LLPanelActiveSpeakers* mPanel;
	};

	void addSpeaker(const LLUUID& id);
	void removeSpeaker(const LLUUID& id);


	LLScrollListCtrl*	mSpeakerList;
	LLUICtrl*			mMuteVoiceCtrl;
	LLUICtrl*			mMuteTextCtrl;
	LLTextBox*			mNameText;
	LLButton*			mProfileBtn;
	BOOL				mShowTextChatters;
	LLSpeakerMgr*		mSpeakerMgr;
	LLFrameTimer		mIconAnimationTimer;
	LLPointer<SpeakerMuteListener> mSpeakerMuteListener;
	LLPointer<SpeakerAddListener> mSpeakerAddListener;
	LLPointer<SpeakerRemoveListener> mSpeakerRemoveListener;
	LLPointer<SpeakerClearListener> mSpeakerClearListener;

	CachedUICtrl<LLUICtrl> mVolumeSlider;
};


#endif // LL_LLFLOATERACTIVESPEAKERS_H
