/** 
 * @file llimpanel.h
 * @brief LLIMPanel class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_IMPANEL_H
#define LL_IMPANEL_H

#include "llfloater.h"
#include "lllogchat.h"
#include "llstyle.h"

class LLAvatarName;
class LLButton;
class LLIMSpeakerMgr;
class LLIMInfo;
class LLInventoryCategory;
class LLInventoryItem;
class LLLineEditor;
class LLParticipantList;
class LLViewerTextEditor;
class LLVoiceChannel;

class LLFloaterIMPanel : public LLFloater
{
public:

	// The session id is the id of the session this is for. The target
	// refers to the user (or group) that where this session serves as
	// the default. For example, if you open a session though a
	// calling card, a new session id will be generated, but the
	// target_id will be the agent referenced by the calling card.
	LLFloaterIMPanel(const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 EInstantMessage dialog);
	LLFloaterIMPanel(const std::string& session_label,
					 const LLUUID& session_id,
					 const LLUUID& target_id,
					 const LLDynamicArray<LLUUID>& ids,
					 EInstantMessage dialog);
	virtual ~LLFloaterIMPanel();

	void lookupName();
	void onAvatarNameLookup(const LLUUID&, const LLAvatarName& avatar_name);

	/*virtual*/ BOOL postBuild();

	// Check typing timeout timer.
	/*virtual*/ void draw();
	/*virtual*/ void onClose(bool app_quitting = FALSE);
	/*virtual*/ void handleVisibilityChange(BOOL new_visibility);

	// add target ids to the session. 
	// Return TRUE if successful, otherwise FALSE.
	BOOL inviteToSession(const LLDynamicArray<LLUUID>& agent_ids);

	void addHistoryLine(const std::string &utf8msg, 
						LLColor4 incolor = LLColor4::white, 
						bool log_to_file = true,
						const LLUUID& source = LLUUID::null,
						const std::string& name = LLStringUtil::null);

	void setInputFocus( BOOL b );

	void selectAll();
	void selectNone();
	void setVisible(BOOL b);

	S32 getNumUnreadMessages() { return mNumUnreadMessages; }

	BOOL handleKeyHere(KEY key, MASK mask);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg);

	BOOL    focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );

	void            onFocusReceived();
	void			onInputEditorFocusReceived();
	void			onInputEditorFocusLost();
	void			onInputEditorKeystroke(LLLineEditor* caller);
	static void		onTabClick( void* userdata );

	void			onClickHistory();
	void			onFlyoutCommit(class LLComboBox* flyout, const LLSD& value);
	static void		onClickStartCall( void* userdata );
	static void		onClickEndCall( void* userdata );
	void			onClickToggleActiveSpeakers(const LLSD& value);
	static void*	createSpeakersPanel(void* data);

	//callbacks for P2P muting and volume control
	void onClickMuteVoice();

	const LLUUID& getSessionID() const { return mSessionUUID; }
	const LLUUID& getOtherParticipantID() const { return mOtherParticipantUUID; }
	void processSessionUpdate(const LLSD& update);
	LLVoiceChannel* getVoiceChannel() { return mVoiceChannel; }
	LLIMSpeakerMgr* getSpeakerManager() const { return mSpeakers; } // Singu TODO: LLIMModel::getSpeakerManager
	EInstantMessage getDialogType() const { return mDialog; }

	void sessionInitReplyReceived(const LLUUID& im_session_id);

	// Handle other participant in the session typing.
	void processIMTyping(const LLIMInfo* im_info, BOOL typing);
	static void chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata);

	//show error statuses to the user
	void showSessionStartError(const std::string& error_string);
	void showSessionEventError(
		const std::string& event_string,
		const std::string& error_string);
	void showSessionForceClose(const std::string& reason);

	static bool onConfirmForceCloseError(const LLSD& notification, const LLSD& response);

	typedef enum e_session_type
	{
		P2P_SESSION,
		GROUP_SESSION,
		ADHOC_SESSION
	} SType;
	bool isP2PSessionType() const { return mSessionType == P2P_SESSION;}
	bool isAdHocSessionType() const { return mSessionType == ADHOC_SESSION;}
	bool isGroupSessionType() const { return mSessionType == GROUP_SESSION;}
	SType mSessionType;

	// LLIMModel Functionality
	bool getSessionInitialized() const { return mSessionInitialized; }
	bool mStartCallOnInitialize;

private:
	// called by constructors
	void init(const std::string& session_label);

	// Called by UI methods.
	void onSendMsg();

	// for adding agents via the UI. Return TRUE if possible, do it if 
	BOOL dropCallingCard(LLInventoryItem* item, BOOL drop);
	BOOL dropCategory(LLInventoryCategory* category, BOOL drop);

	// test if local agent can add agents.
	BOOL isInviteAllowed() const;

	// Called whenever the user starts or stops typing.
	// Sends the typing state to the other user if necessary.
	void setTyping(BOOL typing);

	// Add the "User is typing..." indicator.
	void addTypingIndicator(const std::string &name);

	// Remove the "User is typing..." indicator.
	void removeTypingIndicator(const LLIMInfo* im_info);

	void sendTypingState(BOOL typing);

	const bool isModerator(const LLUUID& speaker_id);
	
private:
	LLLineEditor* mInputEditor;
	LLViewerTextEditor* mHistoryEditor;

	// The value of the mSessionUUID depends on how the IM session was started:
	//   one-on-one  ==> random id
	//   group ==> group_id
	//   inventory folder ==> folder item_id 
	//   911 ==> Gaurdian_Angel_Group_ID ^ gAgent.getID()
	LLUUID mSessionUUID;

	std::string mSessionLabel;
	LLVoiceChannel*	mVoiceChannel;

	BOOL mSessionInitialized;
	LLSD mQueuedMsgsForInit;

	// The value mOtherParticipantUUID depends on how the IM session was started:
	//   one-on-one = recipient's id
	//   group ==> group_id
	//   inventory folder ==> first target id in list
	//   911 ==> sender
	LLUUID mOtherParticipantUUID;
	LLDynamicArray<LLUUID> mSessionInitialTargetIDs;

	EInstantMessage mDialog;

	// Are you currently typing?
	BOOL mTyping;

	// Is other user currently typing?
	BOOL mOtherTyping;

	// name of other user who is currently typing
	std::string mOtherTypingName;

	// Where does the "User is typing..." line start?
	S32 mTypingLineStartIndex;
	// Where does the "Starting session..." line start?
	S32 mSessionStartMsgPos;
	
	S32 mNumUnreadMessages;

	BOOL mSentTypingState;

	BOOL mShowSpeakersOnConnect;

	BOOL mTextIMPossible;
	BOOL mProfileButtonEnabled;
	BOOL mCallBackEnabled;

	bool mDing; // Whether or not to play a ding on new messages
	bool mRPMode;

	LLIMSpeakerMgr* mSpeakers;
	LLParticipantList* mSpeakerPanel;
	
	// Optimization:  Don't send "User is typing..." until the
	// user has actually been typing for a little while.  Prevents
	// extra IMs for brief "lol" type utterences.
	LLFrameTimer mFirstKeystrokeTimer;

	// Timer to detect when user has stopped typing.
	LLFrameTimer mLastKeystrokeTimer;

	boost::signals2::connection mFocusLostSignal;


	CachedUICtrl<LLUICtrl> mVolumeSlider;
	CachedUICtrl<LLButton> mEndCallBtn;
	CachedUICtrl<LLButton> mStartCallBtn;
	CachedUICtrl<LLButton> mSendBtn;
	CachedUICtrl<LLButton> mMuteBtn;

	void disableWhileSessionStarting();

	typedef std::map<LLUUID, LLStyleSP> styleMap;
	static styleMap mStyleMap;
	
	static std::set<LLFloaterIMPanel*> sFloaterIMPanels;
};


#endif  // LL_IMPANEL_H
