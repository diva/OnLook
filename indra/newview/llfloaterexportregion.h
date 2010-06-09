// <edit>
#ifndef LL_LLFLOATEREXPORTREGION_H
#define LL_LLFLOATEREXPORTREGION_H

#include "llfloater.h"
#include "llselectmgr.h"
#include "llvoavatar.h"

class LLFloaterExportRegion
: public LLFloater, public LLFloaterSingleton<LLFloaterExportRegion>
{
	friend class LLUISingleton<LLFloaterExportRegion, VisibilityPolicy<LLFloater> >;
public:
	LLFloaterExportRegion(const LLSD& unused);
	BOOL postBuild(void);
	void updateNamesProgress();
	void receivePrimName(LLViewerObject* object, std::string name);

	LLSD getLLSD();

	std::vector<U32> mPrimList;
	std::map<U32, std::string> mPrimNameMap;

	static LLFloaterExportRegion* sInstance;

	static void receiveObjectProperties(LLUUID fullid, std::string name, std::string desc);

	static void onClickSelectAll(void* user_data);
	static void onClickSaveAs(void* user_data);
	
private:
	virtual ~LLFloaterExportRegion();
	void addToPrimList(LLViewerObject* object);

	enum LIST_COLUMN_ORDER
	{
		LIST_CHECKED,
		LIST_TYPE,
		LIST_NAME,
		LIST_AVATARID
	};
	
	std::map<LLUUID, LLSD> mExportables;
};

#endif
// </edit>
