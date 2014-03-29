/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "lltoolbar.h"

#include "llflyoutbutton.h"
#include "llscrolllistitem.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloaterfriends.h"
#include "llfloaterinventory.h"
#include "llfloatermute.h"
#include "llimpanel.h"
#include "llimview.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "llviewerparcelmgr.h"
#include "llvoavatarself.h"

// [RLVa:KB]
#include "rlvhandler.h"
// [/RLVa:KB]

#if LL_DARWIN

	#include "llresizehandle.h"
	#include "llviewerwindow.h"

	// This class draws like an LLResizeHandle but has no interactivity.
	// It's just there to provide a cue to the user that the lower right corner of the window functions as a resize handle.
	class LLFakeResizeHandle : public LLResizeHandle
	{
	public:
		LLFakeResizeHandle(const LLResizeHandle::Params& p)
			: LLResizeHandle(p)
		{	
		}

		virtual BOOL	handleHover(S32 x, S32 y, MASK mask)   { return FALSE; };
		virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask)  { return FALSE; };
		virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask)   { return FALSE; };

	};

#endif // LL_DARWIN

//
// Globals
//

LLToolBar *gToolBar = NULL;
S32 TOOL_BAR_HEIGHT = 20;

//
// Statics
//
F32	LLToolBar::sInventoryAutoOpenTime = 1.f;

//
// Functions
//
void show_floater(const std::string& floater_name);

LLToolBar::LLToolBar()
:	LLLayoutPanel()
#if LL_DARWIN
	, mResizeHandle(NULL)
#endif // LL_DARWIN
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	mCommitCallbackRegistrar.add("ShowFloater", boost::bind(show_floater, _2));
}


BOOL LLToolBar::postBuild()
{
	mCommunicateBtn.connect(this, "communicate_btn");
	mCommunicateBtn->setCommitCallback(boost::bind(&LLToolBar::onClickCommunicate, this, _2));
	mFlyBtn.connect(this, "fly_btn");
	mBuildBtn.connect(this, "build_btn");
	mMapBtn.connect(this, "map_btn");
	mRadarBtn.connect(this, "radar_btn");
	mInventoryBtn.connect(this, "inventory_btn");

	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		LLButton* buttonp = dynamic_cast<LLButton*>(view);
		if(buttonp)
		{
			buttonp->setSoundFlags(LLView::SILENT);
		}
	}

#if LL_DARWIN
	if(mResizeHandle == NULL)
	{
		LLResizeHandle::Params p;
		p.rect(LLRect(0, 0, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT));
		p.name(std::string(""));
		p.min_width(RESIZE_HANDLE_WIDTH);
		p.min_height(RESIZE_HANDLE_HEIGHT);
		p.corner(LLResizeHandle::RIGHT_BOTTOM);
		mResizeHandle = new LLFakeResizeHandle(p);		this->addChildInBack(mResizeHandle);
		LLLayoutStack* toolbar_stack = getChild<LLLayoutStack>("toolbar_stack");
		toolbar_stack->reshape(toolbar_stack->getRect().getWidth() - RESIZE_HANDLE_WIDTH, toolbar_stack->getRect().getHeight());
	}
#endif // LL_DARWIN

	layoutButtons();

	return TRUE;
}

LLToolBar::~LLToolBar()
{
	// LLView destructor cleans up children
}


BOOL LLToolBar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	LLButton* inventory_btn = getChild<LLButton>("inventory_btn");
	if (!inventory_btn || !inventory_btn->getVisible()) return FALSE;

	LLInventoryView* active_inventory = LLInventoryView::getActiveInventory();

	if (active_inventory && active_inventory->getVisible())
	{
		mInventoryAutoOpen = FALSE;
	}
	else if (inventory_btn->getRect().pointInRect(x, y))
	{
		if (mInventoryAutoOpen)
		{
			if (!(active_inventory && active_inventory->getVisible()) && 
			mInventoryAutoOpenTimer.getElapsedTimeF32() > sInventoryAutoOpenTime)
			{
				LLInventoryView::showAgentInventory();
			}
		}
		else
		{
			mInventoryAutoOpen = TRUE;
			mInventoryAutoOpenTimer.reset();
		}
	}

	return LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
}

void LLToolBar::layoutButtons()
{
#if LL_DARWIN
	const S32 FUDGE_WIDTH_OF_SCREEN = 4;                                    
	S32 width = gViewerWindow->getWindowWidth() + FUDGE_WIDTH_OF_SCREEN;   
	S32 pad = 2;

	// this function may be called before postBuild(), in which case mResizeHandle won't have been set up yet.
	if(mResizeHandle != NULL)
	{
		if(!gViewerWindow->getWindow()->getFullscreen())
		{
			// Only when running in windowed mode on the Mac, leave room for a resize widget on the right edge of the bar.
			width -= RESIZE_HANDLE_WIDTH;

			LLRect r;
			r.mLeft = width - pad;
			r.mBottom = 0;
			r.mRight = r.mLeft + RESIZE_HANDLE_WIDTH;
			r.mTop = r.mBottom + RESIZE_HANDLE_HEIGHT;
			mResizeHandle->setRect(r);
			mResizeHandle->setVisible(TRUE);
		}
		else
		{
			mResizeHandle->setVisible(FALSE);
		}
	}
#endif // LL_DARWIN
}


// virtual
void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);

	layoutButtons();
}


// Per-frame updates of visibility
void LLToolBar::refresh()
{
	if(!isAgentAvatarValid())
		return;

	static LLCachedControl<bool> show("ShowToolBar", true);
	BOOL mouselook = gAgentCamera.cameraMouselook();
	setVisible(show && !mouselook);

	static LLCachedControl<bool> continue_flying_on_unsit("LiruContinueFlyingOnUnsit");
	bool sitting = !continue_flying_on_unsit && gAgentAvatarp && gAgentAvatarp->isSitting();

	mFlyBtn->setEnabled((gAgent.canFly() || gAgent.getFlying()) && !sitting );
	static LLCachedControl<bool> ascent_build_always_enabled("AscentBuildAlwaysEnabled", true);
	mBuildBtn->setEnabled((LLViewerParcelMgr::getInstance()->allowAgentBuild() || ascent_build_always_enabled));

	// Check to see if we're in build mode
	// And not just clicking on a scripted object
	bool build_mode = LLToolMgr::getInstance()->inEdit() && !LLToolGrab::getInstance()->getHideBuildHighlight();
	static LLCachedControl<bool> build_btn_state("BuildBtnState",false);
	if (build_btn_state != build_mode)
		build_btn_state = build_mode;

// [RLVa:KB] - Version: 1.23.4 | Checked: 2009-07-10 (RLVa-1.0.0g)
	// Called per-frame so this really can't be slow
	if (rlv_handler_t::isEnabled())
	{
		// If we're rez-restricted, we can still edit => allow build floater
		// If we're edit-restricted, we can still rez => allow build floater
		mBuildBtn->setEnabled(!(gRlvHandler.hasBehaviour(RLV_BHVR_REZ) && gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)));

		mMapBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWWORLDMAP));
		mRadarBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWMINIMAP) && !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
		mInventoryBtn->setEnabled(!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWINV));
	}
// [/RLVa:KB]

	if (isInVisibleChain() && mCommunicateBtn->getVisible())
	{
		updateCommunicateList();
	}
}

void bold_if_equal(const LLFloater* f1, const LLFloater* f2, LLScrollListItem* itemp)
{
	if (f1 != f2) return;
	static_cast<LLScrollListText*>(itemp->getColumn(0))->setFontStyle(LLFontGL::BOLD);
}

void LLToolBar::updateCommunicateList()
{
	LLSD selected = mCommunicateBtn->getValue();

	mCommunicateBtn->removeall();

	LLFloater* frontmost_floater = LLFloaterChatterBox::getInstance()->getActiveFloater();
	bold_if_equal(LLFloaterMyFriends::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterMyFriends::getInstance()->getShortTitle(), LLSD("contacts"), ADD_TOP));
	bold_if_equal(LLFloaterChat::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterChat::getInstance()->getShortTitle(), LLSD("local chat"), ADD_TOP));
	mCommunicateBtn->addSeparator(ADD_TOP);
	mCommunicateBtn->add(getString("Redock Windows"), LLSD("redock"), ADD_TOP);
	mCommunicateBtn->addSeparator(ADD_TOP);
	bold_if_equal(LLFloaterMute::getInstance(), frontmost_floater, mCommunicateBtn->add(LLFloaterMute::getInstance()->getShortTitle(), LLSD("mute list"), ADD_TOP));

	if (gIMMgr->getIMFloaterHandles().size() > 0) mCommunicateBtn->addSeparator(ADD_TOP);
	for(std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it = gIMMgr->getIMFloaterHandles().begin(); floater_handle_it != gIMMgr->getIMFloaterHandles().end(); ++floater_handle_it)
	{
		if (LLFloaterIMPanel* im_floaterp = (LLFloaterIMPanel*)floater_handle_it->get())
		{
			S32 count = im_floaterp->getNumUnreadMessages();
			std::string floater_title;
			if (count > 0) floater_title = "*";
			floater_title.append(im_floaterp->getShortTitle());
			static LLCachedControl<bool> show_counts("ShowUnreadIMsCounts", true);
			if (show_counts && count > 0)
			{
				floater_title += " - ";
				if (count > 1)
				{
					LLStringUtil::format_map_t args;
					args["COUNT"] = llformat("%d", count);
					floater_title += getString("IMs", args);
				}
				else
				{
					floater_title += getString("IM");
				}
			}
			bold_if_equal(im_floaterp, frontmost_floater, mCommunicateBtn->add(floater_title, im_floaterp->getSessionID(), ADD_TOP));
		}
	}

	mCommunicateBtn->setToggleState(gSavedSettings.getBOOL("ShowCommunicate"));
	if (!selected.isUndefined()) mCommunicateBtn->setValue(selected);
}


// static
void LLToolBar::onClickCommunicate(const LLSD& selected_option)
{
	if (selected_option.asString() == "contacts")
	{
		LLFloaterMyFriends::showInstance();
	}
	else if (selected_option.asString() == "local chat")
	{
		LLFloaterChat::showInstance();
	}
	else if (selected_option.asString() == "redock")
	{
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterMyFriends::getInstance(), FALSE);
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterChat::getInstance(), FALSE);
		LLUUID session_to_show;
		
		std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it;
		for(floater_handle_it = gIMMgr->getIMFloaterHandles().begin(); floater_handle_it != gIMMgr->getIMFloaterHandles().end(); ++floater_handle_it)
		{
			LLFloater* im_floaterp = floater_handle_it->get();
			if (im_floaterp)
			{
				if (im_floaterp->isFrontmost())
				{
					session_to_show = ((LLFloaterIMPanel*)im_floaterp)->getSessionID();
				}
				LLFloaterChatterBox::getInstance()->addFloater(im_floaterp, FALSE);
			}
		}

		LLFloaterChatterBox::showInstance(session_to_show);
	}
	else if (selected_option.asString() == "mute list")
	{
		LLFloaterMute::showInstance();
	}
	else if (selected_option.isUndefined()) // user just clicked the communicate button, treat as toggle
	{
		if (LLFloaterChatterBox::getInstance()->getFloaterCount() == 0)
		{
			LLFloaterMyFriends::toggleInstance();
		}
		else
		{
			LLFloaterChatterBox::toggleInstance();
		}
	}
	else // otherwise selection_option is a specific IM session id
	{
		LLFloaterChatterBox::showInstance(selected_option);
	}
}
