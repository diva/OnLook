// <edit>
/** 
 * @file llimportobject.h
 */

#ifndef LL_LLIMPORTOBJECT_H
#define LL_LLIMPORTOBJECT_H

#include "llviewerobject.h"
#include "llfloater.h"


class LLImportWearable
{
public:
	std::string mName;
	int mType;
	std::string mData;

	LLImportWearable(LLSD sd);
};

class LLImportObject : public LLViewerObject
{
public:
	//LLImportObject(std::string id, std::string parentId);
	LLImportObject(std::string id, LLSD prim);

	std::string mId;
	std::string mParentId;
	std::string mPrimName;
	bool importIsAttachment;
	U32 importAttachPoint;
	LLVector3 importAttachPos;
	LLQuaternion importAttachRot;
};


class LLXmlImportOptions
{
public:
	LLXmlImportOptions(LLXmlImportOptions* options);
	LLXmlImportOptions(std::string filename);
	LLXmlImportOptions(LLSD llsd);
	void init(LLSD llsd);
	std::string mName;
	//LLSD mLLSD;
	std::vector<LLImportObject*> mRootObjects;
	std::vector<LLImportObject*> mChildObjects;
	std::vector<LLImportWearable*> mWearables;
	BOOL mKeepPosition;
	LLViewerObject* mSupplier;
};


class LLXmlImport
{
public:
	static void import(LLXmlImportOptions* import_options);
	static void onNewPrim(LLViewerObject* object);
	static void onNewItem(LLViewerInventoryItem* item);
	static void onNewAttachment(LLViewerObject* object);
	static void Cancel(void* user_data);
	static void rez_supply();

	static bool sImportInProgress;
	static bool sImportHasAttachments;
	static LLUUID sFolderID;
	static LLViewerObject* sSupplyParams;
	static int sPrimsNeeded;
	static std::vector<LLImportObject*> sPrims; // all prims being imported
	static std::map<U8, bool> sPt2watch; // attach points that need watching
	static std::map<std::string, U8> sId2attachpt; // attach points of all attachables
	static std::map<U8, LLVector3> sPt2attachpos; // positions of all attachables
	static std::map<U8, LLQuaternion> sPt2attachrot; // rotations of all attachables
	static int sPrimIndex;
	static int sAttachmentsDone;
	static std::map<std::string, U32> sId2localid;
	static std::map<U32, LLVector3> sRootpositions;
	static std::map<U32, LLQuaternion> sRootrotations;
	static LLXmlImportOptions* sXmlImportOptions;
};

#endif
// </edit>

