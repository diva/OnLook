/** 
 * @file llinventoryview.h
 * @brief LLInventoryView, LLInventoryFolder, and LLInventoryItem
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYPANEL_H
#define LL_LLINVENTORYPANEL_H

#include "llassetstorage.h"
#include "lldarray.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llfolderview.h"
#include "llinventoryfilter.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llmemberlistener.h"
#include "llcombobox.h"
#include "lluictrlfactory.h"
#include <set>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryView
//
// This is the controller class specific for handling agent
// inventory. It deals with the buttons and views used to navigate as
// well as controls the behavior of the overall object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryModel;
class LLInvFVBridge;
class LLMenuBarGL;
class LLCheckBoxCtrl;
class LLSpinCtrl;
class LLScrollableContainerView;
class LLTextBox;
class LLIconCtrl;
class LLSaveFolderState;
class LLSearchEditor;


class LLInventoryPanel : public LLPanel
{
public:
	LLInventoryPanel(const std::string& name,
			const std::string& sort_order_setting,
			const LLRect& rect,
			LLInventoryModel* inventory,
			BOOL allow_multi_select,
			LLView *parent_view = NULL);
	~LLInventoryPanel();

	LLInventoryModel* getModel() { return mInventory; }

	BOOL postBuild();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	// LLView methods
	void draw();
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);

	// Call this method to set the selection.
	void openAllFolders();
	void closeAllFolders();
	void openDefaultFolderForType(LLAssetType::EType);
	void setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void setSelectCallback(LLFolderView::SelectCallback callback, void* user_data);
	void clearSelection();
	LLInventoryFilter* getFilter();
	const LLInventoryFilter* getFilter() const;
	void setFilterTypes(U32 filter);
	U32 getFilterTypes() const;
	void setFilterPermMask(PermissionMask filter_perm_mask);
	U32 getFilterPermMask() const;
	void setFilterSubString(const std::string& string);
	const std::string getFilterSubString();
	void setFilterWorn(bool worn);
	bool getFilterWorn() const { return mFolderRoot->getFilterWorn(); }
	
	void setSinceLogoff(BOOL sl);
	void setHoursAgo(U32 hours);
	BOOL getSinceLogoff();
	
	void setShowFolderState(LLInventoryFilter::EFolderShow show);
	LLInventoryFilter::EFolderShow getShowFolderState();
	void setAllowMultiSelect(BOOL allow) { mFolderRoot->setAllowMultiSelect(allow); }
	// This method is called when something has changed about the inventory.
	void modelChanged(U32 mask);
	LLFolderView* getRootFolder();
	LLScrollableContainerView* getScrollableContainer() { return mScroller; }

	// DEBUG ONLY:
	static void dumpSelectionInformation(void* user_data);

	void openSelected();
	void unSelectAll();

protected:
	// Given the id and the parent, build all of the folder views.
	void rebuildViewsFor(const LLUUID& id, U32 mask);
	void buildNewViews(const LLUUID& id);

public:
	// TomY TODO: Move this elsewhere?
	// helper method which creates an item with a good description,
	// updates the inventory, updates the server, and pushes the
	// inventory update out to other observers.
	void createNewItem(const std::string& name,
					   const LLUUID& parent_id,
					   LLAssetType::EType asset_type,
					   LLInventoryType::EType inv_type,
					   U32 next_owner_perm = 0);

protected:
	LLInventoryModel*			mInventory;
	LLInventoryObserver*		mInventoryObserver;
	
	
	BOOL 						mAllowMultiSelect;
	LLFolderView*				mFolderRoot;
	LLScrollableContainerView*	mScroller;
	//--------------------------------------------------------------------
	// Sorting
	//--------------------------------------------------------------------
public:
	static const std::string DEFAULT_SORT_ORDER;
	static const std::string RECENTITEMS_SORT_ORDER;
	static const std::string WORNITEMS_SORT_ORDER;
	static const std::string INHERIT_SORT_ORDER;

	void setSortOrder(U32 order);
	U32 getSortOrder() const;
private:
	const std::string			mSortOrderSetting;
	LLUUID						mSelectThisID; // if non null, select this item
	
public:
	const LLUUID&		getRootFolderID() const;
protected:
	virtual LLFolderViewFolder*	createFolderViewFolder(LLInvFVBridge * bridge);
	virtual LLFolderViewItem*	createFolderViewItem(LLInvFVBridge * bridge);
};

class LLInventoryView;




///----------------------------------------------------------------------------
/// Function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

// useful functions with the inventory view
// *FIX: move these methods.

void init_inventory_actions(LLInventoryView *floater);
void init_inventory_panel_actions(LLInventoryPanel *panel);

class LLInventoryCategory;
class LLInventoryItem;
//void wear_inventory_category_on_avatar(LLInventoryCategory* category);

void wear_inventory_item_on_avatar(LLInventoryItem* item);
void wear_outfit_by_name(const std::string& name);
void wear_inventory_category(LLInventoryCategory* category, bool copy, bool append);

// These methods can open items without the inventory being visible
void open_notecard(LLViewerInventoryItem* inv_item, const std::string& title, const LLUUID& object_id, BOOL show_keep_discard, const LLUUID& source_id = LLUUID::null, BOOL take_focus = TRUE);
void open_landmark(LLViewerInventoryItem* inv_item, const std::string& title,                          BOOL show_keep_discard, const LLUUID& source_id = LLUUID::null, BOOL take_focus = TRUE);
void open_texture(const LLUUID& item_id, const std::string& title, BOOL show_keep_discard, const LLUUID& source_id = LLUUID::null, BOOL take_focus = TRUE);

const BOOL TAKE_FOCUS_YES = TRUE;
const BOOL TAKE_FOCUS_NO  = FALSE;

#endif // LL_LLINVENTORYPANEL_H



