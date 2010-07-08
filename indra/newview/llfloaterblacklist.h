// <edit>
#ifndef LL_LLFLOATERBLACKLIST_H
#define LL_LLFLOATERBLACKLIST_H
#include "llfloater.h"
class LLFloaterBlacklist : LLFloater
{
public:
	LLFloaterBlacklist();
	~LLFloaterBlacklist();
	static void show();
	BOOL postBuild();
	void refresh();
	void add(LLUUID uuid);
	void clear();
	void setEditID(LLUUID edit_id);
	void removeEntry();
	static void onClickAdd(void* user_data);
	static void onClickClear(void* user_data);
	static void onCommitFileList(LLUICtrl* ctrl, void* user_data);
	static void onClickCopyUUID(void* user_data);
	static void onClickRemove(void* user_data);
	static void loadFromSave();
	static void onClickSave(void* user_data);
	static void onClickLoad(void* user_data);
protected:
	LLUUID mSelectID;
private:
	static LLFloaterBlacklist* sInstance;
	void setMassEnabled(bool enabled);
	void setEditEnabled(bool enabled);
};
#endif
// </edit>
