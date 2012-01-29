/** 
 * @file llpanelmaininventory.cpp
 * @brief Implementation of llpanelmaininventory.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpanelmaininventory.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "lleconomy.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llsdserialize.h"
#include "llspinctrl.h"
#include "lltooldraganddrop.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llpanelinventory.h"
#include "llappviewer.h"

#include "rlvhandler.h"

const std::string FILTERS_FILENAME("filters.xml");

LLDynamicArray<LLInventoryView*> LLInventoryView::sActiveViews;
const S32 INV_MIN_WIDTH = 240;
const S32 INV_MIN_HEIGHT = 150;
const S32 INV_FINDER_WIDTH = 160;
const S32 INV_FINDER_HEIGHT = 408;

//BOOL LLInventoryView::sOpenNextNewItem = FALSE;
class LLInventoryViewFinder : public LLFloater
{
public:
	LLInventoryViewFinder(const std::string& name,
						const LLRect& rect,
						LLInventoryView* inventory_view);
	virtual void draw();
	/*virtual*/	BOOL	postBuild();
	virtual void onClose(bool app_quitting);
	void changeFilter(LLInventoryFilter* filter);
	void updateElementsFromFilter();
	BOOL getCheckShowEmpty();
	BOOL getCheckSinceLogoff();

	static void onTimeAgo(LLUICtrl*, void *);
	static void onCheckSinceLogoff(LLUICtrl*, void *);
	static void onCloseBtn(void* user_data);
	static void selectAllTypes(void* user_data);
	static void selectNoTypes(void* user_data);

protected:
	LLInventoryView*	mInventoryView;
	LLSpinCtrl*			mSpinSinceDays;
	LLSpinCtrl*			mSpinSinceHours;
	LLInventoryFilter*	mFilter;
};


///----------------------------------------------------------------------------
/// LLInventoryView
///----------------------------------------------------------------------------
// Default constructor
LLInventoryView::LLInventoryView(const std::string& name,
								 const std::string& rect,
								 LLInventoryModel* inventory) :
	LLFloater(name, rect, std::string("Inventory"), RESIZE_YES,
			  INV_MIN_WIDTH, INV_MIN_HEIGHT, DRAG_ON_TOP,
			  MINIMIZE_NO, CLOSE_YES),
	mActivePanel(NULL)
	//LLHandle<LLFloater> mFinderHandle takes care of its own initialization
{
	init(inventory);
}

LLInventoryView::LLInventoryView(const std::string& name,
								 const LLRect& rect,
								 LLInventoryModel* inventory) :
	LLFloater(name, rect, std::string("Inventory"), RESIZE_YES,
			  INV_MIN_WIDTH, INV_MIN_HEIGHT, DRAG_ON_TOP,
			  MINIMIZE_NO, CLOSE_YES),
	mActivePanel(NULL)
	//LLHandle<LLFloater> mFinderHandle takes care of its own initialization
{
	init(inventory);
	setRect(rect); // override XML
}


void LLInventoryView::init(LLInventoryModel* inventory)
{
	// Callbacks
	init_inventory_actions(this);

	// Controls
	addBoolControl("Inventory.ShowFilters", FALSE);
	addBoolControl("Inventory.SortByName", FALSE);
	addBoolControl("Inventory.SortByDate", TRUE);
	addBoolControl("Inventory.FoldersAlwaysByName", TRUE);
	addBoolControl("Inventory.SystemFoldersToTop", TRUE);
	updateSortControls();

	addBoolControl("Inventory.SearchName", TRUE);
	addBoolControl("Inventory.SearchDesc", FALSE);
	addBoolControl("Inventory.SearchCreator", FALSE);

	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);

	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inventory.xml", NULL);
}

BOOL LLInventoryView::postBuild()
{
	gInventory.addObserver(this);
	
	mFilterTabs = (LLTabContainer*)getChild<LLTabContainer>("inventory filter tabs");

	// Set up the default inv. panel/filter settings.
	mActivePanel = getChild<LLInventoryPanel>("All Items");
	if (mActivePanel)
	{
		// "All Items" is the previous only view, so it gets the InventorySortOrder
		mActivePanel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER));
		mActivePanel->getFilter()->markDefault();
		mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		mActivePanel->setSelectCallback(onSelectionChange, mActivePanel);
	}
	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		recent_items_panel->setSinceLogoff(TRUE);
		recent_items_panel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::RECENTITEMS_SORT_ORDER));
		recent_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		recent_items_panel->getFilter()->markDefault();
		recent_items_panel->setSelectCallback(onSelectionChange, recent_items_panel);
	}
	LLInventoryPanel* worn_items_panel = getChild<LLInventoryPanel>("Worn Items");
	if (worn_items_panel)
	{
		worn_items_panel->setSortOrder(gSavedSettings.getU32(LLInventoryPanel::WORNITEMS_SORT_ORDER));
		worn_items_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		worn_items_panel->getFilter()->markDefault();
		worn_items_panel->setFilterWorn(true);
		worn_items_panel->setSelectCallback(onSelectionChange, worn_items_panel);
	}

	// Now load the stored settings from disk, if available.
	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILTERS_FILENAME);
	llinfos << "LLInventoryView::init: reading from " << filterSaveName.str() << llendl;
	llifstream file(filterSaveName.str());
	LLSD savedFilterState;
	if (file.is_open())
	{
		LLSDSerialize::fromXML(savedFilterState, file);
		file.close();

		// Load the persistent "Recent Items" settings.
		// Note that the "All Items" and "Worn Items" settings do not persist per-account.
		if(recent_items_panel)
		{
			if(savedFilterState.has(recent_items_panel->getFilter()->getName()))
			{
				LLSD recent_items = savedFilterState.get(
					recent_items_panel->getFilter()->getName());
				recent_items_panel->getFilter()->fromLLSD(recent_items);
			}
		}
	}


	mSearchEditor = getChild<LLSearchEditor>("inventory search editor");
	if (mSearchEditor)
	{
		mSearchEditor->setSearchCallback(onSearchEdit, this);
	}

	mQuickFilterCombo = getChild<LLComboBox>("Quick Filter");

	if (mQuickFilterCombo)
	{
		mQuickFilterCombo->setCommitCallback(onQuickFilterCommit);
	}


	sActiveViews.put(this);

	childSetTabChangeCallback("inventory filter tabs", "All Items", onFilterSelected, this);
	childSetTabChangeCallback("inventory filter tabs", "Recent Items", onFilterSelected, this);
	childSetTabChangeCallback("inventory filter tabs", "Worn Items", onFilterSelected, this);

	childSetAction("Inventory.ResetAll",onResetAll,this);
	childSetAction("Inventory.ExpandAll",onExpandAll,this);
	childSetAction("collapse_btn", onCollapseAll, this);

	//panel->getFilter()->markDefault();
	return TRUE;
}

// Destroys the object
LLInventoryView::~LLInventoryView( void )
{
	// Save the filters state.
	LLSD filterRoot;
	LLInventoryPanel* all_items_panel = getChild<LLInventoryPanel>("All Items");
	if (all_items_panel)
	{
		LLInventoryFilter* filter = all_items_panel->getFilter();
		if (filter)
		{
			LLSD filterState;
			filter->toLLSD(filterState);
			filterRoot[filter->getName()] = filterState;
		}
	}

	LLInventoryPanel* recent_items_panel = getChild<LLInventoryPanel>("Recent Items");
	if (recent_items_panel)
	{
		LLInventoryFilter* filter = recent_items_panel->getFilter();
		if (filter)
		{
			LLSD filterState;
			filter->toLLSD(filterState);
			filterRoot[filter->getName()] = filterState;
		}
	}
	
	LLInventoryPanel* worn_items_panel = getChild<LLInventoryPanel>("Worn Items");
	if (worn_items_panel)
	{
		LLInventoryFilter* filter = worn_items_panel->getFilter();
		if (filter)
		{
			LLSD filterState;
			filter->toLLSD(filterState);
			filterRoot[filter->getName()] = filterState;
		}
	}

	std::ostringstream filterSaveName;
	filterSaveName << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "filters.xml");
	llofstream filtersFile(filterSaveName.str());
	if(!LLSDSerialize::toPrettyXML(filterRoot, filtersFile))
	{
		llwarns << "Could not write to filters save file " << filterSaveName << llendl;
	}
	else
		filtersFile.close();

	sActiveViews.removeObj(this);
	gInventory.removeObserver(this);
	delete mSavedFolderState;
}

void LLInventoryView::startSearch()
{
	// this forces focus to line editor portion of search editor
	if (mSearchEditor)
	{
		mSearchEditor->focusFirstItem(TRUE);
	}
}

// virtual, from LLView
void LLInventoryView::setVisible( BOOL visible )
{
	gSavedSettings.setBOOL("ShowInventory", visible);
	LLFloater::setVisible(visible);
}

// Destroy all but the last floater, which is made invisible.
void LLInventoryView::onClose(bool app_quitting)
{
//	S32 count = sActiveViews.count();
// [RLVa:KB] - Checked: 2009-07-10 (RLVa-1.0.0g)
	// See LLInventoryView::closeAll() on why we're doing it this way
	S32 count = 0;
	for (S32 idx = 0, cnt = sActiveViews.count(); idx < cnt; idx++)
	{
		if (!sActiveViews.get(idx)->isDead())
			count++;
	}
// [/RLVa:KB]

	if (count > 1)
	{
		destroy();
	}
	else
	{
		if (!app_quitting)
		{
			gSavedSettings.setBOOL("ShowInventory", FALSE);
		}
		// clear filters, but save user's folder state first
		if (!mActivePanel->getRootFolder()->isFilterModified())
		{
			mSavedFolderState->setApply(FALSE);
			mActivePanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		}
		
		// onClearSearch(this);

		// pass up
		LLFloater::setVisible(FALSE);
	}
}

BOOL LLInventoryView::handleKeyHere(KEY key, MASK mask)
{
	LLFolderView* root_folder = mActivePanel ? mActivePanel->getRootFolder() : NULL;
	if (root_folder)
	{
		// first check for user accepting current search results
		if (mSearchEditor 
			&& mSearchEditor->hasFocus()
		    && (key == KEY_RETURN 
		    	|| key == KEY_DOWN)
		    && mask == MASK_NONE)
		{
			// move focus to inventory proper
			root_folder->setFocus(TRUE);
			root_folder->scrollToShowSelection();
			return TRUE;
		}

		if (root_folder->hasFocus() && key == KEY_UP)
		{
			startSearch();
		}
	}

	return LLFloater::handleKeyHere(key, mask);

}



// static
// *TODO: remove take_keyboard_focus param
LLInventoryView* LLInventoryView::showAgentInventory(BOOL take_keyboard_focus)
{
	if (gDisconnected || gNoRender)
	{
		return NULL;
	}

// [RLVa:KB] - Checked: 2009-07-10 (RLVa-1.0.0g)
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWINV))
	{
		return NULL;
	}
// [/RLVa:KB]

	LLInventoryView* iv = LLInventoryView::getActiveInventory();
#if 0 && !LL_RELEASE_FOR_DOWNLOAD
	if (sActiveViews.count() == 1)
	{
		delete iv;
		iv = NULL;
	}
#endif
	if(!iv && !gAgentCamera.cameraMouselook())
	{
		// create one.
		iv = new LLInventoryView(std::string("Inventory"),
								 std::string("FloaterInventoryRect"),
								 &gInventory);
		iv->open();
		// keep onscreen
		gFloaterView->adjustToFitScreen(iv, FALSE);

		gSavedSettings.setBOOL("ShowInventory", TRUE);
	}
	if(iv)
	{
		// Make sure it's in front and it makes a noise
		iv->setTitle(std::string("Inventory"));
		iv->open();		/*Flawfinder: ignore*/
	}
	//if (take_keyboard_focus)
	//{
	//	iv->startSearch();
	//	gFocusMgr.triggerFocusFlash();
	//}
	return iv;
}

// static
LLInventoryView* LLInventoryView::getActiveInventory()
{
	LLInventoryView* iv = NULL;
	S32 count = sActiveViews.count();
	if(count > 0)
	{
		iv = sActiveViews.get(0);
		S32 z_order = gFloaterView->getZOrder(iv);
		S32 z_next = 0;
		LLInventoryView* next_iv = NULL;
		for(S32 i = 1; i < count; ++i)
		{
			next_iv = sActiveViews.get(i);
			z_next = gFloaterView->getZOrder(next_iv);
			if(z_next < z_order)
			{
				iv = next_iv;
				z_order = z_next;
			}
		}
	}
	return iv;
}

// static
void LLInventoryView::toggleVisibility()
{
	S32 count = sActiveViews.count();
	if (0 == count)
	{
		showAgentInventory(TRUE);
	}
	else if (1 == count)
	{
		if (sActiveViews.get(0)->getVisible())
		{
			sActiveViews.get(0)->close();
			gSavedSettings.setBOOL("ShowInventory", FALSE);
		}
		else
		{
			showAgentInventory(TRUE);
		}
	}
	else
	{
		// With more than one open, we know at least one
		// is visible.

		// Close all the last one spawned.
		S32 last_index = sActiveViews.count() - 1;
		sActiveViews.get(last_index)->close();
	}
}

// static
void LLInventoryView::cleanup()
{
	S32 count = sActiveViews.count();
	for (S32 i = 0; i < count; i++)
	{
		sActiveViews.get(i)->destroy();
	}
}


void LLInventoryView::updateSortControls()
{
	U32 order = mActivePanel ? mActivePanel->getSortOrder() : gSavedSettings.getU32("InventorySortOrder");
	bool sort_by_date = order & LLInventoryFilter::SO_DATE;
	bool folders_by_name = order & LLInventoryFilter::SO_FOLDERS_BY_NAME;
	bool sys_folders_on_top = order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;

	getControl("Inventory.SortByDate")->setValue(sort_by_date);
	getControl("Inventory.SortByName")->setValue(!sort_by_date);
	getControl("Inventory.FoldersAlwaysByName")->setValue(folders_by_name);
	getControl("Inventory.SystemFoldersToTop")->setValue(sys_folders_on_top);
}

void LLInventoryView::resetFilters()
{
	LLInventoryViewFinder *finder = getFinder();
	getActivePanel()->getFilter()->resetDefault();
	if (finder)
	{
		finder->updateElementsFromFilter();
	}

	setFilterTextFromFilter();
}

// static
BOOL LLInventoryView::filtersVisible(void* user_data)
{
	LLInventoryView* self = (LLInventoryView*)user_data;
	if(!self) return FALSE;

	return self->getFinder() != NULL;
}

// static
void LLInventoryView::onClearSearch(void* user_data)
{
	LLInventoryView* self = (LLInventoryView*)user_data;
	if(!self) return;

	LLFloater *finder = self->getFinder();
	if (self->mActivePanel)
	{
		self->mActivePanel->setFilterSubString(LLStringUtil::null);
		self->mActivePanel->setFilterTypes(0xffffffff);
	}

	if (finder)
	{
		LLInventoryViewFinder::selectAllTypes(finder);
	}

	// re-open folders that were initially open
	if (self->mActivePanel)
	{
		self->mSavedFolderState->setApply(TRUE);
		self->mActivePanel->getRootFolder()->applyFunctorRecursively(*self->mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		self->mActivePanel->getRootFolder()->applyFunctorRecursively(opener);
		self->mActivePanel->getRootFolder()->scrollToShowSelection();
	}
}

//static
void LLInventoryView::onSearchEdit(const std::string& search_string, void* user_data )
{
	if (search_string == "")
	{
		onClearSearch(user_data);
	}
	LLInventoryView* self = (LLInventoryView*)user_data;
	if (!self->mActivePanel)
	{
		return;
	}

	LLInventoryModelBackgroundFetch::instance().start();

	std::string filter_text = search_string;
	std::string uppercase_search_string = filter_text;
	LLStringUtil::toUpper(uppercase_search_string);
	if (self->mActivePanel->getFilterSubString().empty() && uppercase_search_string.empty())
	{
			// current filter and new filter empty, do nothing
			return;
	}

	// save current folder open state if no filter currently applied
	if (!self->mActivePanel->getRootFolder()->isFilterModified())
	{
		self->mSavedFolderState->setApply(FALSE);
		self->mActivePanel->getRootFolder()->applyFunctorRecursively(*self->mSavedFolderState);
	}

	// set new filter string
	self->mActivePanel->setFilterSubString(uppercase_search_string);
}

//static
void LLInventoryView::onQuickFilterCommit(LLUICtrl* ctrl, void* user_data)
{

	LLComboBox* quickfilter = (LLComboBox*)ctrl;


	LLInventoryView* view = (LLInventoryView*)(quickfilter->getParent());
	if (!view->mActivePanel)
	{
		return;
	}


	std::string item_type = quickfilter->getSimple();
	U32 filter_type;

	if (view->getString("filter_type_animation") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_ANIMATION;
	}

	else if (view->getString("filter_type_callingcard") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_CALLINGCARD;
	}

	else if (view->getString("filter_type_wearable") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_WEARABLE;
	}

	else if (view->getString("filter_type_gesture") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_GESTURE;
	}

	else if (view->getString("filter_type_landmark") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_LANDMARK;
	}

	else if (view->getString("filter_type_notecard") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_NOTECARD;
	}

	else if (view->getString("filter_type_object") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_OBJECT;
	}

	else if (view->getString("filter_type_script") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_LSL;
	}

	else if (view->getString("filter_type_sound") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_SOUND;
	}

	else if (view->getString("filter_type_texture") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_TEXTURE;
	}

	else if (view->getString("filter_type_snapshot") == item_type)
	{
		filter_type = 0x1 << LLInventoryType::IT_SNAPSHOT;
	}

	else if (view->getString("filter_type_custom") == item_type)
	{
		// When they select custom, show the floater then return
		if( !(view->filtersVisible(view)) )
		{
			view->toggleFindOptions();
		}
		return;
	}

	else if (view->getString("filter_type_all") == item_type)
	{
		// Show all types
		filter_type = 0xffffffff;
	}

	else
	{
		llwarns << "Ignoring unknown filter: " << item_type << llendl;
		return;
	}

	view->mActivePanel->setFilterTypes( filter_type );


	// Force the filters window to update itself, if it's open.
	LLInventoryViewFinder* finder = view->getFinder();
	if( finder )
	{
		finder->updateElementsFromFilter();
	}

	// llinfos << "Quick Filter: " << item_type << llendl;

}



//static
void LLInventoryView::refreshQuickFilter(LLUICtrl* ctrl)
{

	LLInventoryView* view = (LLInventoryView*)(ctrl->getParent());
	if (!view->mActivePanel)
	{
		return;
	}

	LLComboBox* quickfilter = view->getChild<LLComboBox>("Quick Filter");
	if (!quickfilter)
	{
		return;
	}


	U32 filter_type = view->mActivePanel->getFilterTypes();


  // Mask to extract only the bit fields we care about.
  // *TODO: There's probably a cleaner way to construct this mask.
  U32 filter_mask = 0;
  filter_mask |= (0x1 << LLInventoryType::IT_ANIMATION);
  filter_mask |= (0x1 << LLInventoryType::IT_CALLINGCARD);
  filter_mask |= (0x1 << LLInventoryType::IT_WEARABLE);
  filter_mask |= (0x1 << LLInventoryType::IT_GESTURE);
  filter_mask |= (0x1 << LLInventoryType::IT_LANDMARK);
  filter_mask |= (0x1 << LLInventoryType::IT_NOTECARD);
  filter_mask |= (0x1 << LLInventoryType::IT_OBJECT);
  filter_mask |= (0x1 << LLInventoryType::IT_LSL);
  filter_mask |= (0x1 << LLInventoryType::IT_SOUND);
  filter_mask |= (0x1 << LLInventoryType::IT_TEXTURE);
  filter_mask |= (0x1 << LLInventoryType::IT_SNAPSHOT);


  filter_type &= filter_mask;


  //llinfos << "filter_type: " << filter_type << llendl;

	std::string selection;


	if (filter_type == filter_mask)
	{
		selection = view->getString("filter_type_all");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_ANIMATION))
	{
		selection = view->getString("filter_type_animation");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_CALLINGCARD))
	{
		selection = view->getString("filter_type_callingcard");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_WEARABLE))
	{
		selection = view->getString("filter_type_wearable");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_GESTURE))
	{
		selection = view->getString("filter_type_gesture");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_LANDMARK))
	{
		selection = view->getString("filter_type_landmark");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_NOTECARD))
	{
		selection = view->getString("filter_type_notecard");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_OBJECT))
	{
		selection = view->getString("filter_type_object");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_LSL))
	{
		selection = view->getString("filter_type_script");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_SOUND))
	{
		selection = view->getString("filter_type_sound");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_TEXTURE))
	{
		selection = view->getString("filter_type_texture");
	}

	else if (filter_type == (0x1 << LLInventoryType::IT_SNAPSHOT))
	{
		selection = view->getString("filter_type_snapshot");
	}

	else
	{
		selection = view->getString("filter_type_custom");
	}


	// Select the chosen item by label text
	BOOL result = quickfilter->setSimple( (selection) );

  if( !result )
  {
    llinfos << "The item didn't exist: " << selection << llendl;
  }

}



// static
// BOOL LLInventoryView::incrementalFind(LLFolderViewItem* first_item, const char *find_text, BOOL backward)
// {
// 	LLInventoryView* active_view = NULL;

// 	for (S32 i = 0; i < sActiveViews.count(); i++)
// 	{
// 		if (gFocusMgr.childHasKeyboardFocus(sActiveViews[i]))
// 		{
// 			active_view = sActiveViews[i];
// 			break;
// 		}
// 	}

// 	if (!active_view)
// 	{
// 		return FALSE;
// 	}

// 	std::string search_string(find_text);

// 	if (search_string.empty())
// 	{
// 		return FALSE;
// 	}

// 	if (active_view->mActivePanel &&
// 		active_view->mActivePanel->getRootFolder()->search(first_item, search_string, backward))
// 	{
// 		return TRUE;
// 	}

// 	return FALSE;
// }

void LLInventoryView::onResetAll(void* userdata)
{
	LLInventoryView* self = (LLInventoryView*) userdata;
	self->mActivePanel = (LLInventoryPanel*)self->childGetVisibleTab("inventory filter tabs");

	if (!self->mActivePanel)
	{
		return;
	}
	if (self->mActivePanel && self->mSearchEditor)
	{
		self->mSearchEditor->setText(LLStringUtil::null);
	}
	self->onSearchEdit("",userdata);
	self->mActivePanel->closeAllFolders();
}

//static
void LLInventoryView::onExpandAll(void* userdata)
{
	LLInventoryView* self = (LLInventoryView*) userdata;
	self->mActivePanel = (LLInventoryPanel*)self->childGetVisibleTab("inventory filter tabs");

	if (!self->mActivePanel)
	{
		return;
	}
	self->mActivePanel->openAllFolders();
}


//static
void LLInventoryView::onCollapseAll(void* userdata)
{
	LLInventoryView* self = (LLInventoryView*) userdata;
	self->mActivePanel = (LLInventoryPanel*)self->childGetVisibleTab("inventory filter tabs");

	if (!self->mActivePanel)
	{
		return;
	}
	self->mActivePanel->closeAllFolders();
}

//static
void LLInventoryView::onFilterSelected(void* userdata, bool from_click)
{
	LLInventoryView* self = (LLInventoryView*) userdata;
	LLInventoryFilter* filter;

	LLInventoryViewFinder *finder = self->getFinder();
	// Find my index
	self->mActivePanel = (LLInventoryPanel*)self->childGetVisibleTab("inventory filter tabs");

	if (!self->mActivePanel)
	{
		return;
	}
	filter = self->mActivePanel->getFilter();
	if (finder)
	{
		finder->changeFilter(filter);
	}
	if (filter->isActive())
	{
		// If our filter is active we may be the first thing requiring a fetch so we better start it here.
		LLInventoryModelBackgroundFetch::instance().start();
	}
	self->setFilterTextFromFilter();
	self->updateSortControls();
}

// static
void LLInventoryView::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data)
{
	LLInventoryPanel* panel = (LLInventoryPanel*)data;
	LLFolderView* fv = panel->getRootFolder();
	if (fv->needsAutoRename()) // auto-selecting a new user-created asset and preparing to rename
	{
		fv->setNeedsAutoRename(FALSE);
		if (items.size()) // new asset is visible and selected
		{
			fv->startRenamingSelectedItem();
		}
	}
}

const std::string LLInventoryView::getFilterSubString() 
{ 
	return mActivePanel->getFilterSubString(); 
}

void LLInventoryView::setFilterSubString(const std::string& string) 
{ 
	mActivePanel->setFilterSubString(string); 
}


BOOL LLInventoryView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	// Check to see if we are auto scrolling from the last frame
	LLInventoryPanel* panel = (LLInventoryPanel*)this->getActivePanel();
	BOOL needsToScroll = panel->getScrollableContainer()->needsToScroll(x, y, LLScrollableContainerView::VERTICAL);
	if(mFilterTabs)
	{
		if(needsToScroll)
		{
			mFilterTabs->startDragAndDropDelayTimer();
		}
	}
	
	BOOL handled = LLFloater::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	return handled;
}

void LLInventoryView::changed(U32 mask)
{
	std::ostringstream title;
	title << "Inventory";
 	if (LLInventoryModelBackgroundFetch::instance().backgroundFetchActive())
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		std::string item_count_string;
		LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());
		title << " (Fetched " << item_count_string << " items...)";
	}
	title << mFilterText;
	setTitle(title.str());

}

void LLInventoryView::draw()
{
 	if (LLInventoryModelBackgroundFetch::instance().isEverythingFetched())
	{
		LLLocale locale(LLLocale::USER_LOCALE);
		std::ostringstream title;
		title << "Inventory";
		std::string item_count_string;
		LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());
		title << " (" << item_count_string << " items)";
		title << mFilterText;
		setTitle(title.str());
	}
	if (mActivePanel && mSearchEditor)
	{
		mSearchEditor->setText(mActivePanel->getFilterSubString());
	}

        if (mActivePanel && mQuickFilterCombo)
        {
                refreshQuickFilter( mQuickFilterCombo );
        }

	LLFloater::draw();
}

void LLInventoryView::setFilterTextFromFilter() 
{ 
	mFilterText = mActivePanel->getFilter()->getFilterText(); 
}

void LLInventoryView::toggleFindOptions()
{
	LLFloater *floater = getFinder();
	if (!floater)
	{
		LLInventoryViewFinder * finder = new LLInventoryViewFinder(std::string("Inventory Finder"),
										LLRect(getRect().mLeft - INV_FINDER_WIDTH, getRect().mTop, getRect().mLeft, getRect().mTop - INV_FINDER_HEIGHT),
										this);
		mFinderHandle = finder->getHandle();
		finder->open();		/*Flawfinder: ignore*/
		addDependentFloater(mFinderHandle);

		// start background fetch of folders
		LLInventoryModelBackgroundFetch::instance().start();

		mFloaterControls[std::string("Inventory.ShowFilters")]->setValue(TRUE);
	}
	else
	{
		floater->close();

		mFloaterControls[std::string("Inventory.ShowFilters")]->setValue(FALSE);
	}
}
///----------------------------------------------------------------------------
/// LLInventoryViewFinder
///----------------------------------------------------------------------------
LLInventoryViewFinder* LLInventoryView::getFinder() 
{ 
	return (LLInventoryViewFinder*)mFinderHandle.get();
}


LLInventoryViewFinder::LLInventoryViewFinder(const std::string& name,
						const LLRect& rect,
						LLInventoryView* inventory_view) :
	LLFloater(name, rect, std::string("Filters"), RESIZE_NO,
				INV_FINDER_WIDTH, INV_FINDER_HEIGHT, DRAG_ON_TOP,
				MINIMIZE_NO, CLOSE_YES),
	mInventoryView(inventory_view),
	mFilter(inventory_view->mActivePanel->getFilter())
{

	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_inventory_view_finder.xml");
	updateElementsFromFilter();
}

BOOL LLInventoryViewFinder::postBuild()
{
	childSetAction("All", selectAllTypes, this);
	childSetAction("None", selectNoTypes, this);

	mSpinSinceHours = getChild<LLSpinCtrl>("spin_hours_ago");
	childSetCommitCallback("spin_hours_ago", onTimeAgo, this);

	mSpinSinceDays = getChild<LLSpinCtrl>("spin_days_ago");
	childSetCommitCallback("spin_days_ago", onTimeAgo, this);

//	mCheckSinceLogoff   = getChild<LLSpinCtrl>("check_since_logoff");
	childSetCommitCallback("check_since_logoff", onCheckSinceLogoff, this);

	childSetAction("Close", onCloseBtn, this);

	updateElementsFromFilter();
	return TRUE;
}


void LLInventoryViewFinder::onCheckSinceLogoff(LLUICtrl *ctrl, void *user_data)
{
	LLInventoryViewFinder *self = (LLInventoryViewFinder *)user_data;
	if (!self) return;

	bool since_logoff= self->childGetValue("check_since_logoff");
	
	if (!since_logoff && 
	    !(  self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() ) )
	{
		self->mSpinSinceHours->set(1.0f);
	}	
}

void LLInventoryViewFinder::onTimeAgo(LLUICtrl *ctrl, void *user_data)
{
	LLInventoryViewFinder *self = (LLInventoryViewFinder *)user_data;
	if (!self) return;
	
	bool since_logoff=true;
	if ( self->mSpinSinceDays->get() ||  self->mSpinSinceHours->get() )
	{
		since_logoff = false;
	}
	self->childSetValue("check_since_logoff", since_logoff);
}

void LLInventoryViewFinder::changeFilter(LLInventoryFilter* filter)
{
	mFilter = filter;
	updateElementsFromFilter();
}

void LLInventoryViewFinder::updateElementsFromFilter()
{
	if (!mFilter)
		return;

	// Get data needed for filter display
	U32 filter_types = mFilter->getFilterTypes();
	std::string filter_string = mFilter->getFilterSubString();
	LLInventoryFilter::EFolderShow show_folders = mFilter->getShowFolderState();
	U32 hours = mFilter->getHoursAgo();

	// update the ui elements
	LLFloater::setTitle(mFilter->getName());
	childSetValue("check_animation", (S32) (filter_types & 0x1 << LLInventoryType::IT_ANIMATION));

	childSetValue("check_calling_card", (S32) (filter_types & 0x1 << LLInventoryType::IT_CALLINGCARD));
	childSetValue("check_clothing", (S32) (filter_types & 0x1 << LLInventoryType::IT_WEARABLE));
	childSetValue("check_gesture", (S32) (filter_types & 0x1 << LLInventoryType::IT_GESTURE));
	childSetValue("check_landmark", (S32) (filter_types & 0x1 << LLInventoryType::IT_LANDMARK));
	childSetValue("check_notecard", (S32) (filter_types & 0x1 << LLInventoryType::IT_NOTECARD));
	childSetValue("check_object", (S32) (filter_types & 0x1 << LLInventoryType::IT_OBJECT));
	childSetValue("check_script", (S32) (filter_types & 0x1 << LLInventoryType::IT_LSL));
	childSetValue("check_sound", (S32) (filter_types & 0x1 << LLInventoryType::IT_SOUND));
	childSetValue("check_texture", (S32) (filter_types & 0x1 << LLInventoryType::IT_TEXTURE));
	childSetValue("check_snapshot", (S32) (filter_types & 0x1 << LLInventoryType::IT_SNAPSHOT));
	childSetValue("check_show_empty", show_folders == LLInventoryFilter::SHOW_ALL_FOLDERS);
	childSetValue("check_since_logoff", mFilter->isSinceLogoff());
	mSpinSinceHours->set((F32)(hours % 24));
	mSpinSinceDays->set((F32)(hours / 24));
}

void LLInventoryViewFinder::draw()
{
	U32 filter = 0xffffffff;
	BOOL filtered_by_all_types = TRUE;

	if (!childGetValue("check_animation"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_ANIMATION);
		filtered_by_all_types = FALSE;
	}


	if (!childGetValue("check_calling_card"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_CALLINGCARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_clothing"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_WEARABLE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_gesture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_GESTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_landmark"))


	{
		filter &= ~(0x1 << LLInventoryType::IT_LANDMARK);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_notecard"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_NOTECARD);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_object"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_OBJECT);
		filter &= ~(0x1 << LLInventoryType::IT_ATTACHMENT);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_script"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_LSL);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_sound"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SOUND);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_texture"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_TEXTURE);
		filtered_by_all_types = FALSE;
	}

	if (!childGetValue("check_snapshot"))
	{
		filter &= ~(0x1 << LLInventoryType::IT_SNAPSHOT);
		filtered_by_all_types = FALSE;
	}

	if (!filtered_by_all_types)
	{
		// don't include folders in filter, unless I've selected everything
		filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
	}

	// update the panel, panel will update the filter
	mInventoryView->mActivePanel->setShowFolderState(getCheckShowEmpty() ?
		LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryView->mActivePanel->setFilterTypes(filter);
	if (getCheckSinceLogoff())
	{
		mSpinSinceDays->set(0);
		mSpinSinceHours->set(0);
	}
	U32 days = (U32)mSpinSinceDays->get();
	U32 hours = (U32)mSpinSinceHours->get();
	if (hours > 24)
	{
		days += hours / 24;
		hours = (U32)hours % 24;
		mSpinSinceDays->set((F32)days);
		mSpinSinceHours->set((F32)hours);
	}
	hours += days * 24;
	mInventoryView->mActivePanel->setHoursAgo(hours);
	mInventoryView->mActivePanel->setSinceLogoff(getCheckSinceLogoff());
	mInventoryView->setFilterTextFromFilter();

	LLFloater::draw();
}

void  LLInventoryViewFinder::onClose(bool app_quitting)
{
	if (mInventoryView) mInventoryView->getControl("Inventory.ShowFilters")->setValue(FALSE);
	// If you want to reset the filter on close, do it here.  This functionality was
	// hotly debated - Paulm
#if 0
	if (mInventoryView)
	{
		LLInventoryView::onResetFilter((void *)mInventoryView);
	}
#endif
	destroy();
}


BOOL LLInventoryViewFinder::getCheckShowEmpty()
{
	return childGetValue("check_show_empty");
}

BOOL LLInventoryViewFinder::getCheckSinceLogoff()
{
	return childGetValue("check_since_logoff");
}

void LLInventoryViewFinder::onCloseBtn(void* user_data)
{
	LLInventoryViewFinder* finderp = (LLInventoryViewFinder*)user_data;
	finderp->close();
}

// static
void LLInventoryViewFinder::selectAllTypes(void* user_data)
{
	LLInventoryViewFinder* self = (LLInventoryViewFinder*)user_data;
	if(!self) return;

	self->childSetValue("check_animation", TRUE);
	self->childSetValue("check_calling_card", TRUE);
	self->childSetValue("check_clothing", TRUE);
	self->childSetValue("check_gesture", TRUE);
	self->childSetValue("check_landmark", TRUE);
	self->childSetValue("check_notecard", TRUE);
	self->childSetValue("check_object", TRUE);
	self->childSetValue("check_script", TRUE);
	self->childSetValue("check_sound", TRUE);
	self->childSetValue("check_texture", TRUE);
	self->childSetValue("check_snapshot", TRUE);

/*
	self->mCheckCallingCard->set(TRUE);
	self->mCheckClothing->set(TRUE);
	self->mCheckGesture->set(TRUE);
	self->mCheckLandmark->set(TRUE);
	self->mCheckNotecard->set(TRUE);
	self->mCheckObject->set(TRUE);
	self->mCheckScript->set(TRUE);
	self->mCheckSound->set(TRUE);
	self->mCheckTexture->set(TRUE);
	self->mCheckSnapshot->set(TRUE);*/
}

//static
void LLInventoryViewFinder::selectNoTypes(void* user_data)
{
	LLInventoryViewFinder* self = (LLInventoryViewFinder*)user_data;
	if(!self) return;

	/*
	self->childSetValue("check_animation", FALSE);
	self->mCheckCallingCard->set(FALSE);
	self->mCheckClothing->set(FALSE);
	self->mCheckGesture->set(FALSE);
	self->mCheckLandmark->set(FALSE);
	self->mCheckNotecard->set(FALSE);
	self->mCheckObject->set(FALSE);
	self->mCheckScript->set(FALSE);
	self->mCheckSound->set(FALSE);
	self->mCheckTexture->set(FALSE);
	self->mCheckSnapshot->set(FALSE);*/


	self->childSetValue("check_animation", FALSE);
	self->childSetValue("check_calling_card", FALSE);
	self->childSetValue("check_clothing", FALSE);
	self->childSetValue("check_gesture", FALSE);
	self->childSetValue("check_landmark", FALSE);
	self->childSetValue("check_notecard", FALSE);
	self->childSetValue("check_object", FALSE);
	self->childSetValue("check_script", FALSE);
	self->childSetValue("check_sound", FALSE);
	self->childSetValue("check_texture", FALSE);
	self->childSetValue("check_snapshot", FALSE);
}
