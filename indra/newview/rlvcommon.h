/** 
 *
 * Copyright (c) 2009-2011, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU General Public License, version 2.0, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the GPL can be found in doc/GPL-license.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/gpl-2.0.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */

#ifndef RLV_COMMON_H
#define RLV_COMMON_H

#include "llmemberlistener.h"
#include "llinventorymodel.h"
#include "llselectmgr.h"
#include "llview.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "rlvdefines.h"
#include "rlvviewer2.h"

// ============================================================================
// Forward declarations
//

class RlvCommand;

typedef std::vector<const LLViewerObject*> c_llvo_vec_t;

// ============================================================================
// RlvNotifications
//

class RlvNotifications
{
public:
	static void notifyBehaviour(ERlvBehaviour eBhvr, ERlvParamType eType);
	static void notifyBlockedTeleport()    { notifyBlocked("blocked_teleport"); }
	static void notifyBlockedViewNote()    { notifyBlockedViewXXX(LLAssetType::lookup(LLAssetType::AT_NOTECARD)); }
	static void notifyBlockedViewScript()  { notifyBlockedViewXXX(LLAssetType::lookup(LLAssetType::AT_SCRIPT)); }
	static void notifyBlockedViewTexture() { notifyBlockedViewXXX(LLAssetType::lookup(LLAssetType::AT_TEXTURE)); }

	static void warnGiveToRLV();
protected:
	static void notifyBlocked(const std::string& strRlvString);
	static void notifyBlockedViewXXX(const char* pstrAssetType);

	static void onGiveToRLVConfirmation(const LLSD& notification, const LLSD& response);
};

// ============================================================================
// RlvSettings
//

inline BOOL rlvGetSettingBOOL(const std::string& strSetting, BOOL fDefault)
{
	return (gSavedSettings.controlExists(strSetting)) ? gSavedSettings.getBOOL(strSetting) : fDefault;
}
inline BOOL rlvGetPerUserSettingsBOOL(const std::string& strSetting, BOOL fDefault)
{
	return (gSavedPerAccountSettings.controlExists(strSetting)) ? gSavedPerAccountSettings.getBOOL(strSetting) : fDefault;
}
inline std::string rlvGetSettingString(const std::string& strSetting, const std::string& strDefault)
{
	return (gSavedSettings.controlExists(strSetting)) ? gSavedSettings.getString(strSetting) : strDefault;
}

class RlvSettings
{
public:
	static BOOL getDebug()						{ return rlvGetSettingBOOL(RLV_SETTING_DEBUG, FALSE); }
	static BOOL getForbidGiveToRLV()			{ return rlvGetSettingBOOL(RLV_SETTING_FORBIDGIVETORLV, TRUE); }
	static BOOL getNoSetEnv()					{ return fNoSetEnv; }

	static std::string getWearAddPrefix()		{ return rlvGetSettingString(RLV_SETTING_WEARADDPREFIX, LLStringUtil::null); }
	static std::string getWearReplacePrefix()	{ return rlvGetSettingString(RLV_SETTING_WEARREPLACEPREFIX, LLStringUtil::null); }

	static bool getDebugHideUnsetDup()			{ return rlvGetSettingBOOL(RLV_SETTING_DEBUGHIDEUNSETDUP, FALSE); }
	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	static BOOL getEnableComposites()			{ return fCompositeFolders; }
	#endif // RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	static BOOL getEnableLegacyNaming()			{ return fLegacyNaming; }
	static BOOL getEnableWear()					{ return TRUE; }
	static BOOL getEnableSharedWear()			{ return rlvGetSettingBOOL(RLV_SETTING_ENABLESHAREDWEAR, FALSE); }
	static BOOL getHideLockedLayers()			{ return rlvGetSettingBOOL(RLV_SETTING_HIDELOCKEDLAYER, FALSE); }		
	static BOOL getHideLockedAttach()			{ return rlvGetSettingBOOL(RLV_SETTING_HIDELOCKEDATTACH, FALSE); }
	static BOOL getHideLockedInventory()		{ return rlvGetSettingBOOL(RLV_SETTING_HIDELOCKEDINVENTORY, FALSE); }
	static BOOL getSharedInvAutoRename()		{ return rlvGetSettingBOOL(RLV_SETTING_SHAREDINVAUTORENAME, TRUE); }
	static BOOL getShowNameTags()				{ return fShowNameTags; }

	#ifdef RLV_EXTENSION_STARTLOCATION
	static BOOL getLoginLastLocation()			{ return rlvGetPerUserSettingsBOOL(RLV_SETTING_LOGINLASTLOCATION, TRUE); }
	static void updateLoginLastLocation();
	#endif // RLV_EXTENSION_STARTLOCATION

	static void initClass();
protected:
	static bool onChangedSettingBOOL(const LLSD& newvalue, BOOL* pfSetting);

	#ifdef RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	static BOOL fCompositeFolders;
	#endif // RLV_EXPERIMENTAL_COMPOSITEFOLDERS
	static BOOL fLegacyNaming;
	static BOOL fNoSetEnv;
	static BOOL fShowNameTags;
};

// ============================================================================
// RlvStrings
//

class RlvStrings
{
public:
	static void initClass();

	static const std::string& getAnonym(const std::string& strName);		// @shownames
	static const std::string& getBehaviourNotificationString(ERlvBehaviour eBhvr, ERlvParamType eType);
	static const std::string& getString(const std::string& strStringName);
	static const char*        getStringFromReturnCode(ERlvCmdRet eRet);
	static std::string        getVersion(bool fLegacy = false);				// @version
	static std::string        getVersionAbout();							// Shown in Help / About
	static std::string        getVersionNum();								// @versionnum
	static bool               hasString(const std::string& strStringName);

protected:
	static std::vector<std::string> m_Anonyms;
	static std::map<std::string, std::string> m_StringMap;
	#ifdef RLV_EXTENSION_NOTIFY_BEHAVIOUR
	static std::map<ERlvBehaviour, std::string> m_BhvrAddMap;
	static std::map<ERlvBehaviour, std::string> m_BhvrRemMap;
	#endif // RLV_EXTENSION_NOTIFY_BEHAVIOUR
};

// ============================================================================
// RlvUtil - Collection of (static) helper functions
//

class RlvUtil
{
public:
	static bool isEmote(const std::string& strUTF8Text);
	static bool isNearbyAgent(const LLUUID& idAgent);								// @shownames
	static bool isNearbyRegion(const std::string& strRegion);						// @showloc

	static void filterLocation(std::string& strUTF8Text);							// @showloc
	static void filterNames(std::string& strUTF8Text, bool fFilterLegacy = true);	// @shownames

	static bool isForceTp()	{ return m_fForceTp; }
	static void forceTp(const LLVector3d& posDest);									// Ignores restrictions that might otherwise prevent tp'ing

	static void notifyFailedAssertion(const std::string& strAssert, const std::string& strFile, int nLine);

	static void sendBusyMessage(const LLUUID& idTo, const std::string& strMsg, const LLUUID& idSession = LLUUID::null);
	static bool isValidReplyChannel(S32 nChannel);
	static bool sendChatReply(S32 nChannel, const std::string& strUTF8Text);
	static bool sendChatReply(const std::string& strChannel, const std::string& strUTF8Text);

protected:
	static bool m_fForceTp;															// @standtp
};

// ============================================================================
// Extensibility classes
//

class RlvBehaviourObserver
{
public:
	virtual ~RlvBehaviourObserver() {}
	virtual void changed(const RlvCommand& rlvCmd, bool fInternal) = 0;
};

class RlvCommandHandler
{
public:
	virtual ~RlvCommandHandler() {}
	virtual bool onAddRemCommand(const RlvCommand& rlvCmd, ERlvCmdRet& cmdRet) { return false; }
	virtual bool onClearCommand(const RlvCommand& rlvCmd, ERlvCmdRet& cmdRet)  { return false; }
	virtual bool onReplyCommand(const RlvCommand& rlvCmd, ERlvCmdRet& cmdRet)  { return false; }
	virtual bool onForceCommand(const RlvCommand& rlvCmd, ERlvCmdRet& cmdRet)  { return false; }
};
typedef bool (RlvCommandHandler::*rlvCommandHandler)(const RlvCommand& rlvCmd, ERlvCmdRet& cmdRet);

// ============================================================================
// Generic menu enablers
//

class RlvEnableIfNot : public LLMemberListener<LLView>
{
	bool handleEvent(LLPointer<LLEvent>, const LLSD&);
};

// ============================================================================
// Selection functors
//

struct RlvSelectHasLockedAttach : public LLSelectedNodeFunctor
{
	RlvSelectHasLockedAttach() {}
	virtual bool apply(LLSelectNode* pNode);
};

// Filters out selected objects that can't be editable (i.e. getFirstNode() will return NULL if the selection is fully editable)
struct RlvSelectIsEditable : public LLSelectedNodeFunctor
{
	RlvSelectIsEditable() {}
	/*virtual*/ bool apply(LLSelectNode* pNode);
};

struct RlvSelectIsOwnedByOrGroupOwned : public LLSelectedNodeFunctor
{
	RlvSelectIsOwnedByOrGroupOwned(const LLUUID& uuid) : m_idAgent(uuid) {}
	virtual bool apply(LLSelectNode* pNode);
protected:
	LLUUID m_idAgent;
};

struct RlvSelectIsSittingOn : public LLSelectedNodeFunctor
{
	RlvSelectIsSittingOn(LLXform* pObject) : m_pObject(pObject) {}
	virtual bool apply(LLSelectNode* pNode);
protected:
	LLXform* m_pObject;
};

// ============================================================================
// Various public helper functions
//

BOOL rlvAttachToEnabler(void* pParam);

// ============================================================================
// Predicates
//

bool rlvPredCanWearItem(const LLViewerInventoryItem* pItem, ERlvWearMask eWearMask);
bool rlvPredCanNotWearItem(const LLViewerInventoryItem* pItem, ERlvWearMask eWearMask);
bool rlvPredCanRemoveItem(const LLViewerInventoryItem* pItem);
bool rlvPredCanNotRemoveItem(const LLViewerInventoryItem* pItem);

struct RlvPredCanWearItem
{
	RlvPredCanWearItem(ERlvWearMask eWearMask) : m_eWearMask(eWearMask) {}
	bool operator()(const LLViewerInventoryItem* pItem) { return rlvPredCanWearItem(pItem, m_eWearMask); }
protected:
	ERlvWearMask m_eWearMask;
};

struct RlvPredCanNotWearItem
{
	RlvPredCanNotWearItem(ERlvWearMask eWearMask) : m_eWearMask(eWearMask) {}
	bool operator()(const LLViewerInventoryItem* pItem) { return rlvPredCanNotWearItem(pItem, m_eWearMask); }
protected:
	ERlvWearMask m_eWearMask;
};

struct RlvPredIsEqualOrLinkedItem
{
	RlvPredIsEqualOrLinkedItem(const LLViewerInventoryItem* pItem) : m_pItem(pItem) {}
	RlvPredIsEqualOrLinkedItem(const LLUUID& idItem) { m_pItem = gInventory.getItem(idItem); }

	bool operator()(const LLViewerInventoryItem* pItem) const
	{
		return (m_pItem) && (pItem) && (m_pItem->getLinkedUUID() == pItem->getLinkedUUID());
	}
protected:
	const LLViewerInventoryItem* m_pItem;
};

template<typename T> struct RlvPredValuesEqual
{
	bool operator()(const T* pT2) const { return (pT1) && (pT2) && (*pT1 == *pT2); }
	const T* pT1;
};

// ============================================================================
// Inlined class member functions
//

// Checked: 2010-03-26 (RLVa-1.2.0b) | Modified: RLVa-1.0.2a
inline bool RlvUtil::isEmote(const std::string& strUTF8Text)
{
	return (strUTF8Text.length() > 4) && ( (strUTF8Text.compare(0, 4, "/me ") == 0) || (strUTF8Text.compare(0, 4, "/me'") == 0) );
}

// Checked: 2010-03-09 (RLVa-1.2.0b) | Added: RLVa-1.0.2a
inline bool RlvUtil::isValidReplyChannel(S32 nChannel)
{
	return (nChannel > 0) && (CHAT_CHANNEL_DEBUG != nChannel);
}

// Checked: 2009-08-05 (RLVa-1.0.1e) | Added: RLVa-1.0.0e
inline bool RlvUtil::sendChatReply(const std::string& strChannel, const std::string& strUTF8Text)
{
	S32 nChannel;
	return (LLStringUtil::convertToS32(strChannel, nChannel)) ? sendChatReply(nChannel, strUTF8Text) : false;
}

// ============================================================================

#endif // RLV_COMMON_H
