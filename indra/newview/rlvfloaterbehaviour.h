/** 
 *
 * Copyright (c) 2009-2010, Kitty Barnett
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

#ifndef RLV_FLOATERS_H
#define RLV_FLOATERS_H

#include "llfloater.h"

#include "rlvdefines.h"
#include "rlvcommon.h"

// ============================================================================
// RlvFloaterLocks class declaration
//

class RlvFloaterBehaviours : public LLFloater, public LLFloaterSingleton<RlvFloaterBehaviours>
{
	friend class LLUISingleton<RlvFloaterBehaviours, VisibilityPolicy<LLFloater> >;

	RlvFloaterBehaviours(const LLSD& sdKey);

	/*
	 * LLFloater overrides
	 */
public:
	/*virtual*/ void onOpen();
	/*virtual*/ void onClose(bool fQuitting);
	/*virtual*/ BOOL postBuild();
	
	static void toggle(void* a=NULL);
	static BOOL visible(void* a=NULL);

	/*
	 * Member functions
	 */
protected:
	void onAvatarNameLookup(const LLUUID& idAgent, const LLAvatarName& avName);
	void onBtnCopyToClipboard();
	void onCommand(const RlvCommand& rlvCmd, ERlvCmdRet eRet);
	void refreshAll();

	/*
	 * Member variables
	 */
protected:
	boost::signals2::connection	m_ConnRlvCommand;
	uuid_vec_t 					m_PendingLookup;
};

// ============================================================================
// RlvFloaterLocks class declaration
//

class RlvFloaterLocks : public LLFloater, public LLFloaterSingleton<RlvFloaterLocks>
{
	friend class LLUISingleton<RlvFloaterLocks, VisibilityPolicy<LLFloater> >;

	RlvFloaterLocks(const LLSD& sdKey);

	/*
	 * LLFloater overrides
	 */
public:
	virtual void onOpen();
	virtual void onClose(bool fQuitting);
	
	static void toggle(void* a=NULL);
	static BOOL visible(void* a=NULL);

	/*
	 * Event handlers
	 */
protected:
	void onRlvCommand(const RlvCommand& rlvCmd, ERlvCmdRet eRet);

	/*
	 * Member functions
	 */
protected:
	void refreshAll();

	/*
	 * Member variables
	 */
protected:
	boost::signals2::connection m_ConnRlvCommand;
};

// ============================================================================

#endif // RLV_FLOATERS_H