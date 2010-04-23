// <edit>
#include "llviewerprecompiledheaders.h"
#include "llbuildnewviewsscheduler.h"
#include "llinventorybridge.h"
#define BUILD_DELAY 0.1f
#define BUILD_PER_DELAY 512

LLBuildNewViewsScheduler* gBuildNewViewsScheduler;

std::list<LLBuildNewViewsScheduler::job> LLBuildNewViewsScheduler::sJobs;
LLBuildNewViewsScheduler::LLBuildNewViewsScheduler() : LLEventTimer(BUILD_DELAY)
{
}
void LLBuildNewViewsScheduler::addJob(LLInventoryPanel* inventory_panel, LLInventoryObject* inventory_object)
{
	LLBuildNewViewsScheduler::job j;
	j.mInventoryPanel = inventory_panel;
	j.mInventoryObject = inventory_object;
	sJobs.push_back(j);
}
void LLBuildNewViewsScheduler::cancel(LLInventoryPanel* inventory_panel)
{
	for(std::list<LLBuildNewViewsScheduler::job>::iterator iter = sJobs.begin();
		iter != sJobs.end(); )
	{
		LLInventoryPanel* job_panel = (*iter).mInventoryPanel;
		if(job_panel == inventory_panel)
		{
			iter = sJobs.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}
BOOL LLBuildNewViewsScheduler::tick()
{
	U32 i = 0;
	while(!sJobs.empty() && (i < BUILD_PER_DELAY))
	{
		LLBuildNewViewsScheduler::job j = sJobs.front();
		buildNewViews(j.mInventoryPanel, j.mInventoryObject);
		sJobs.pop_front();
		++i;
	}
	return FALSE;
}
void LLBuildNewViewsScheduler::buildNewViews(LLInventoryPanel* panelp, LLInventoryObject* objectp)
{
	LLFolderViewItem* itemp = NULL;

	if (objectp)
	{		
		if (objectp->getType() <= LLAssetType::AT_NONE ||
			objectp->getType() >= LLAssetType::AT_COUNT)
		{
			llwarns << "called with objectp->mType == " 
				<< ((S32) objectp->getType())
				<< " (shouldn't happen)" << llendl;
		}
		else if (objectp->getType() == LLAssetType::AT_CATEGORY) // build new view for category
		{
			LLInvFVBridge* new_listener = LLInvFVBridge::createBridge(objectp->getType(),
													LLInventoryType::IT_CATEGORY,
													panelp,
													objectp->getUUID());

			if (new_listener)
			{
				LLFolderViewFolder* folderp = new LLFolderViewFolder(new_listener->getDisplayName(),
													new_listener->getIcon(),
													panelp->getRootFolder(),
													new_listener);
				
				folderp->setItemSortOrder(panelp->getSortOrder());
				itemp = folderp;
			}
		}
		else // build new view for item
		{
			LLInventoryItem* item = (LLInventoryItem*)objectp;
			LLInvFVBridge* new_listener = LLInvFVBridge::createBridge(
				item->getType(),
				item->getInventoryType(),
				panelp,
				item->getUUID(),
				item->getFlags());
			if (new_listener)
			{
				itemp = new LLFolderViewItem(new_listener->getDisplayName(),
												new_listener->getIcon(),
												new_listener->getCreationDate(),
												panelp->getRootFolder(),
												new_listener);
			}
		}

		LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)panelp->getRootFolder()->getItemByID(objectp->getParentUUID());

		if (itemp)
		{
			itemp->mDelayedDelete = TRUE;
			if (parent_folder)
			{
				itemp->addToFolder(parent_folder, panelp->getRootFolder());
			}
			else
			{
				llwarns << "Couldn't find parent folder for child " << itemp->getLabel() << llendl;
				delete itemp;
			}
		}
	}

	if (!objectp || (objectp && (objectp->getType() == LLAssetType::AT_CATEGORY)))
	{
		LLViewerInventoryCategory::cat_array_t* categories;
		LLViewerInventoryItem::item_array_t* items;

		panelp->getModel()->lockDirectDescendentArrays((objectp != NULL) ? objectp->getUUID() : LLUUID::null, categories, items);
		if(categories)
		{
			S32 count = categories->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryCategory* cat = categories->get(i);
				addJob(panelp, cat);
			}
		}
		if(items)
		{
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryItem* item = items->get(i);
				addJob(panelp, item);
			}
		}
		panelp->getModel()->unlockDirectDescendentArrays(objectp->getUUID());
	}
}
// </edit>
