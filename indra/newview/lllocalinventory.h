// <edit>
#ifndef LL_LLLOCALINVENTORY_H
#define LL_LLLOCALINVENTORY_H

#include "llviewerinventory.h"

#include "llfloater.h"

#include "llfolderview.h"
#include "llinventorymodel.h" // cat_array_t, item_array_t

class LLLocalInventory
{
public:
	static LLUUID addItem(std::string name, int type, LLUUID asset_id, bool open);
	static LLUUID addItem(std::string name, int type, LLUUID asset_id);
	static void addItem(LLViewerInventoryItem* item);
	static void open(LLUUID item_id);
	static void loadInvCache(std::string filename);
	static void saveInvCache(std::string filename, LLFolderView* folder);
	static void climb(LLInventoryCategory* cat,
							 LLInventoryModel::cat_array_t& cats,
							 LLInventoryModel::item_array_t& items);
};




class LLFloaterNewLocalInventory
: public LLFloater
{
public:
	LLFloaterNewLocalInventory();
	BOOL postBuild(void);

	static void onClickOK(void* user_data);
	static LLUUID sLastCreatorId;
	
private:
	virtual ~LLFloaterNewLocalInventory();

};

#endif
// </edit>
