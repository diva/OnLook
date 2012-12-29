/** 
 * @file hbprefsinert.h
 * @brief  Ascent Viewer preferences panel
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Henri Beauchamp.
 * 
 * Second Life Viewer Source Code
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef ASCENTPREFSCHAT_H
#define ASCENTPREFSCHAT_H


#include "llpanel.h"
#include "lldroptarget.h"


class LLPrefsAscentChat : public LLPanel
{
public:
    LLPrefsAscentChat();
    ~LLPrefsAscentChat();

    void apply();
    void cancel();
    void refresh();
    void refreshValues();

protected:
    static void onSpellAdd(void* data);
    static void onSpellRemove(void* data);
    static void onSpellGetMore(void* data);
    static void onSpellEditCustom(void* data);
    static void onSpellBaseComboBoxCommit(LLUICtrl* ctrl, void* userdata);
    static void onCommitTimeDate(LLUICtrl* ctrl, void *userdata);
    static void onCommitAutoResponse(LLUICtrl* ctrl, void* user_data);
	static void onCommitResetAS(LLUICtrl*,void*);
	static void onCommitEnableAS(LLUICtrl*, void*);
	static void onCommitDialogBlock(LLUICtrl*, void*);
    static void onCommitKeywords(LLUICtrl* ctrl, void* user_data);

    //Chat/IM -----------------------------------------------------------------------------
    BOOL mWoLfVerticalIMTabs;
    BOOL mIMAnnounceIncoming;
    BOOL mHideTypingNotification;
    BOOL mShowGroupNameInChatIM;
    BOOL mPlayTypingSound;
    BOOL mHideNotificationsInChat;
    BOOL mEnableMUPose;
    BOOL mEnableOOCAutoClose;
    U32 mLinksForChattingObjects;
    U32 mTimeFormat;
    U32 mDateFormat;
        U32 tempTimeFormat;
        U32 tempDateFormat;
    BOOL mSecondsInChatAndIMs;

    BOOL mIMResponseAnyone;
    BOOL mIMResponseFriends;
    BOOL mIMResponseMuted;
    BOOL mIMShowOnTyping;
    BOOL mIMShowResponded;
    BOOL mIMResponseRepeat;
    BOOL mIMResponseItem;
    std::string mIMResponseText;

    //Spam --------------------------------------------------------------------------------
    BOOL mEnableAS;
    BOOL mGlobalQueue;
    U32  mChatSpamCount;
    U32  mChatSpamTime;
    BOOL mBlockDialogSpam;
    BOOL mBlockAlertSpam;
    BOOL mBlockFriendSpam;
    BOOL mBlockGroupNoticeSpam;
    BOOL mBlockGroupInviteSpam;
	BOOL mBlockGroupFeeInviteSpam;
    BOOL mBlockItemOfferSpam;
    BOOL mBlockScriptSpam;
    BOOL mBlockTeleportSpam;
    BOOL mNotifyOnSpam;
    BOOL mSoundMulti;
    U32  mNewLines;
    U32  mPreloadMulti;
	bool mEnableGestureSounds;

    //Text Options ------------------------------------------------------------------------
    BOOL mSpellDisplay;
    BOOL mKeywordsOn;
    std::string mKeywordsList;
    BOOL mKeywordsInChat;
    BOOL mKeywordsInIM;
    BOOL mKeywordsChangeColor;
    LLColor4 mKeywordsColor;
    BOOL mKeywordsPlaySound;
    LLUUID mKeywordsSound;
private:
	static LLPrefsAscentChat* sInst;
	static void SinguIMResponseItemDrop(LLViewerInventoryItem* item);
};

#endif
