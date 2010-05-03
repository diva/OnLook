// <edit>
/** 
 * @file llimportobject.cpp
 */

#include "llviewerprecompiledheaders.h"
#include "llimportobject.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerobject.h"
#include "llagent.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llfloater.h"
#include "lllineeditor.h"
#include "llinventorymodel.h"
#include "lluictrlfactory.h"
#include "llscrolllistctrl.h"


// static vars
bool LLXmlImport::sImportInProgress = false;
bool LLXmlImport::sImportHasAttachments = false;
LLUUID LLXmlImport::sFolderID;
LLViewerObject* LLXmlImport::sSupplyParams;
int LLXmlImport::sPrimsNeeded;
std::vector<LLImportObject*> LLXmlImport::sPrims;
std::map<std::string, U8> LLXmlImport::sId2attachpt;
std::map<U8, bool> LLXmlImport::sPt2watch;
std::map<U8, LLVector3> LLXmlImport::sPt2attachpos;
std::map<U8, LLQuaternion> LLXmlImport::sPt2attachrot;
int LLXmlImport::sPrimIndex = 0;
int LLXmlImport::sAttachmentsDone = 0;
std::map<std::string, U32> LLXmlImport::sId2localid;
std::map<U32, LLVector3> LLXmlImport::sRootpositions;
LLXmlImportOptions* LLXmlImport::sXmlImportOptions;

LLFloaterImportProgress* LLFloaterImportProgress::sInstance;

LLXmlImportOptions::LLXmlImportOptions(LLXmlImportOptions* options)
{
	mName = options->mName;
	mRootObjects = options->mRootObjects;
	mChildObjects = options->mChildObjects;
	mWearables = options->mWearables;
	mSupplier = options->mSupplier;
	mKeepPosition = options->mKeepPosition;
}
LLXmlImportOptions::LLXmlImportOptions(std::string filename)
:	mSupplier(NULL),
	mKeepPosition(FALSE)
{
	mName = gDirUtilp->getBaseFileName(filename, true);
	llifstream in(filename);
	if(!in.is_open())
	{
		llwarns << "Couldn't open file..." << llendl;
		return;
	}
	LLSD llsd;
	if(LLSDSerialize::fromXML(llsd, in) < 1)
	{
		llwarns << "Messed up data?" << llendl;
		return;
	}
	init(llsd);
}
LLXmlImportOptions::LLXmlImportOptions(LLSD llsd)
:	mName("stuff"),
	mSupplier(NULL),
	mKeepPosition(FALSE)
{
	init(llsd);
}
void LLXmlImportOptions::init(LLSD llsd)
{
	mRootObjects.clear();
	mChildObjects.clear();
	mWearables.clear();
	// Separate objects and wearables
	std::vector<LLImportObject*> unsorted_objects;
	LLSD::map_iterator map_end = llsd.endMap();
	for(LLSD::map_iterator map_iter = llsd.beginMap() ; map_iter != map_end; ++map_iter)
	{
		std::string key((*map_iter).first);
		LLSD item = (*map_iter).second;
		if(item.has("type"))
		{
			if(item["type"].asString() == "wearable")
				mWearables.push_back(new LLImportWearable(item));
			else
				unsorted_objects.push_back(new LLImportObject(key, item));
		}
		else // assumed to be a prim
			unsorted_objects.push_back(new LLImportObject(key, item));
	}
	// Separate roots from children
	int total_objects = (int)unsorted_objects.size();
	for(int i = 0; i < total_objects; i++)
	{
		if(unsorted_objects[i]->mParentId == "")
			mRootObjects.push_back(unsorted_objects[i]);
		else
			mChildObjects.push_back(unsorted_objects[i]);
	}
}

LLFloaterXmlImportOptions::LLFloaterXmlImportOptions(LLXmlImportOptions* default_options)
:	mDefaultOptions(default_options)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_import_options.xml");
}
BOOL LLFloaterXmlImportOptions::postBuild()
{
	center();
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("import_list");
	// Add all wearables to list and keep an id:ptr map
	std::vector<LLImportWearable*>::iterator wearable_end = mDefaultOptions->mWearables.end();
	for(std::vector<LLImportWearable*>::iterator iter = mDefaultOptions->mWearables.begin();
		iter != wearable_end; ++iter)
	{
		LLImportWearable* wearablep = (*iter);
		LLUUID id; id.generate();
		mImportWearableMap[id] = wearablep;
		LLSD element;
		element["id"] = id;
		LLSD& check_column = element["columns"][LIST_CHECKED];
		check_column["column"] = "checked";
		check_column["type"] = "checkbox";
		check_column["value"] = true;
		LLSD& type_column = element["columns"][LIST_TYPE];
		type_column["column"] = "type";
		type_column["type"] = "icon";
		type_column["value"] = "inv_item_" + LLWearable::typeToTypeName((EWearableType)(wearablep->mType)) + ".tga";
		LLSD& name_column = element["columns"][LIST_NAME];
		name_column["column"] = "name";
		name_column["value"] = wearablep->mName;
		list->addElement(element, ADD_BOTTOM);
	}
	// Add all root objects to list and keep an id:ptr map
	std::vector<LLImportObject*>::iterator object_end = mDefaultOptions->mRootObjects.end();
	for(std::vector<LLImportObject*>::iterator iter = mDefaultOptions->mRootObjects.begin();
		iter != object_end; ++iter)
	{
		LLImportObject* objectp = (*iter);
		LLUUID id; id.generate();
		mImportObjectMap[id] = objectp;
		LLSD element;
		element["id"] = id;
		LLSD& check_column = element["columns"][LIST_CHECKED];
		check_column["column"] = "checked";
		check_column["type"] = "checkbox";
		check_column["value"] = true;
		LLSD& type_column = element["columns"][LIST_TYPE];
		type_column["column"] = "type";
		type_column["type"] = "icon";
		type_column["value"] = "inv_item_object.tga";
		LLSD& name_column = element["columns"][LIST_NAME];
		name_column["column"] = "name";
		name_column["value"] = objectp->mPrimName;
		list->addElement(element, ADD_BOTTOM);
	}
	// Callbacks
	childSetAction("select_all_btn", onClickSelectAll, this);
	childSetAction("select_objects_btn", onClickSelectObjects, this);
	childSetAction("select_wearables_btn", onClickSelectWearables, this);
	childSetAction("ok_btn", onClickOK, this);
	childSetAction("cancel_btn", onClickCancel, this);
	return TRUE;
}
void LLFloaterXmlImportOptions::onClickSelectAll(void* user_data)
{
	LLFloaterXmlImportOptions* floaterp = (LLFloaterXmlImportOptions*)user_data;
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("import_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
	std::vector<LLScrollListItem*>::iterator items_end = items.end();
	bool new_value = !((*item_iter)->getColumn(LIST_CHECKED)->getValue());
	for( ; item_iter != items_end; ++item_iter)
	{
		(*item_iter)->getColumn(LIST_CHECKED)->setValue(new_value);
	}
}
void LLFloaterXmlImportOptions::onClickSelectObjects(void* user_data)
{
	LLFloaterXmlImportOptions* floaterp = (LLFloaterXmlImportOptions*)user_data;
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("import_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
	std::vector<LLScrollListItem*>::iterator items_end = items.end();
	bool new_value = false;
	for( ; item_iter != items_end; ++item_iter)
	{
		if(((*item_iter)->getColumn(LIST_TYPE)->getValue()).asString() == "inv_item_object.tga")
		{
			new_value = !((*item_iter)->getColumn(LIST_CHECKED)->getValue());
			break;
		}
	}
	for(item_iter = items.begin(); item_iter != items_end; ++item_iter)
	{
		if(((*item_iter)->getColumn(LIST_TYPE)->getValue()).asString() == "inv_item_object.tga")
			(*item_iter)->getColumn(LIST_CHECKED)->setValue(new_value);
	}
}
void LLFloaterXmlImportOptions::onClickSelectWearables(void* user_data)
{
	LLFloaterXmlImportOptions* floaterp = (LLFloaterXmlImportOptions*)user_data;
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("import_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
	std::vector<LLScrollListItem*>::iterator items_end = items.end();
	bool new_value = false;
	for( ; item_iter != items_end; ++item_iter)
	{
		if(((*item_iter)->getColumn(LIST_TYPE)->getValue()).asString() != "inv_item_object.tga")
		{
			new_value = !((*item_iter)->getColumn(LIST_CHECKED)->getValue());
			break;
		}
	}
	for(item_iter = items.begin(); item_iter != items_end; ++item_iter)
	{
		if(((*item_iter)->getColumn(LIST_TYPE)->getValue()).asString() != "inv_item_object.tga")
			(*item_iter)->getColumn(LIST_CHECKED)->setValue(new_value);
	}
}
void LLFloaterXmlImportOptions::onClickOK(void* user_data)
{
	LLFloaterXmlImportOptions* floaterp = (LLFloaterXmlImportOptions*)user_data;
	LLXmlImportOptions* opt = new LLXmlImportOptions(floaterp->mDefaultOptions);
	opt->mRootObjects.clear();
	opt->mChildObjects.clear();
	opt->mWearables.clear();
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("import_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	std::vector<LLScrollListItem*>::iterator item_end = items.end();
	std::vector<LLImportObject*>::iterator child_end = floaterp->mDefaultOptions->mChildObjects.end();
	for(std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != item_end; ++iter)
	{
		if((*iter)->getColumn(LIST_CHECKED)->getValue())
		{ // checked
			LLUUID id = (*iter)->getUUID();
			if(floaterp->mImportWearableMap.find(id) != floaterp->mImportWearableMap.end())
			{
				opt->mWearables.push_back(floaterp->mImportWearableMap[id]);
			}
			else // object
			{
				LLImportObject* objectp = floaterp->mImportObjectMap[id];
				opt->mRootObjects.push_back(objectp);
				// Add child objects
				for(std::vector<LLImportObject*>::iterator child_iter = floaterp->mDefaultOptions->mChildObjects.begin();
					child_iter != child_end; ++child_iter)
				{
					if((*child_iter)->mParentId == objectp->mId)
						opt->mChildObjects.push_back((*child_iter));
				}
			}
		}
	}
	opt->mKeepPosition = floaterp->childGetValue("keep_position_check");
	LLXmlImport::import(opt);
	floaterp->close();
}
void LLFloaterXmlImportOptions::onClickCancel(void* user_data)
{
	LLFloaterXmlImportOptions* floaterp = (LLFloaterXmlImportOptions*)user_data;
	floaterp->close();
}





std::string terse_F32_string( F32 f )
{
	std::string r = llformat( "%.2f", f );
	// "1.20"  -> "1.2"
	// "24.00" -> "24."
	S32 len = r.length();
	while( len > 0 && '0' == r[len - 1] )
	{
		r.erase(len-1, 1);
		len--;
	}
	if( '.' == r[len - 1] )
	{
		// "24." -> "24"
		r.erase(len-1, 1);
	}
	else if( ('-' == r[0]) && ('0' == r[1]) )
	{
		// "-0.59" -> "-.59"
		r.erase(1, 1);
	}
	else if( '0' == r[0] )
	{
		// "0.59" -> ".59"
		r.erase(0, 1);
	}
	return r;
}
LLImportWearable::LLImportWearable(LLSD sd)
{
	mName = sd["name"].asString();
	mType = sd["wearabletype"].asInteger();

	LLSD params = sd["params"];
	LLSD textures = sd["textures"];

	mData = "LLWearable version 22\n" + 
			mName + "\n\n" + 
			"\tpermissions 0\n" + 
			"\t{\n" + 
			"\t\tbase_mask\t7fffffff\n" + 
			"\t\towner_mask\t7fffffff\n" + 
			"\t\tgroup_mask\t00000000\n" + 
			"\t\teveryone_mask\t00000000\n" + 
			"\t\tnext_owner_mask\t00082000\n" + 
			"\t\tcreator_id\t00000000-0000-0000-0000-000000000000\n" + 
			"\t\towner_id\t" + gAgent.getID().asString() + "\n" + 
			"\t\tlast_owner_id\t" + gAgent.getID().asString() + "\n" + 
			"\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n" + 
			"\t}\n" + 
			"\tsale_info\t0\n" + 
			"\t{\n" + 
			"\t\tsale_type\tnot\n" + 
			"\t\tsale_price\t10\n" + 
			"\t}\n" + 
			"type " + llformat("%d", mType) + "\n";

	mData += llformat("parameters %d\n", params.size());
	LLSD::map_iterator map_iter = params.beginMap();
	LLSD::map_iterator map_end = params.endMap();
	for( ; map_iter != map_end; ++map_iter)
	{
		mData += (*map_iter).first + " " + terse_F32_string((*map_iter).second.asReal()) + "\n";
	}

	mData += llformat("textures %d\n", textures.size());
	map_iter = textures.beginMap();
	map_end = textures.endMap();
	for( ; map_iter != map_end; ++map_iter)
	{
		mData += (*map_iter).first + " " + (*map_iter).second.asString() + "\n";
	}
}

//LLImportObject::LLImportObject(std::string id, std::string parentId)
//	:	LLViewerObject(LLUUID::null, 9, NULL, TRUE),
//		mId(id),
//		mParentId(parentId),
//		mPrimName("Object")
//{
//	importIsAttachment = false;
//}
LLImportObject::LLImportObject(std::string id, LLSD prim)
	:	LLViewerObject(LLUUID::null, 9, NULL, TRUE)
{
	importIsAttachment = false;
	mId = id;
	mParentId = "";
	mPrimName = "Primitive";
	if(prim.has("parent"))
	{
		mParentId = prim["parent"].asString();
	}
	// Stuff for attach
	if(prim.has("attach"))
	{
		importIsAttachment = true;
		importAttachPoint = (U8)prim["attach"].asInteger();
		importAttachPos = ll_vector3_from_sd(prim["position"]);
		importAttachRot = ll_quaternion_from_sd(prim["rotation"]);
	}
	// Transforms
	setPosition(ll_vector3_from_sd(prim["position"]), FALSE);
	setScale(ll_vector3_from_sd(prim["scale"]), FALSE);
	setRotation(ll_quaternion_from_sd(prim["rotation"]), FALSE);
	// Flags
	setFlags(FLAGS_CAST_SHADOWS, prim["shadows"].asInteger());
	setFlags(FLAGS_PHANTOM, prim["phantom"].asInteger());
	setFlags(FLAGS_USE_PHYSICS, prim["physical"].asInteger());
	// Volume params
	LLVolumeParams volume_params;
	volume_params.fromLLSD(prim["volume"]);
	
	setVolume(volume_params, 0, false);
	// Extra params
	if(prim.has("flexible"))
	{
		LLFlexibleObjectData* wat = new LLFlexibleObjectData();
		wat->fromLLSD(prim["flex"]);
		LLFlexibleObjectData flex = *wat;
		setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, flex, true);
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, true);
	}
	if(prim.has("light"))
	{
		LLLightParams* wat = new LLLightParams();
		wat->fromLLSD(prim["light"]);
		LLLightParams light = *wat;
		setParameterEntry(LLNetworkData::PARAMS_LIGHT, light, true);
		setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, TRUE, true);
	}
	if(prim.has("sculpt"))
	{
		LLSculptParams *wat = new LLSculptParams();
		wat->fromLLSD(prim["sculpt"]);
		LLSculptParams sculpt = *wat;
		setParameterEntry(LLNetworkData::PARAMS_SCULPT, sculpt, true);
		setParameterEntryInUse(LLNetworkData::PARAMS_SCULPT, TRUE, true);
	}
	// Textures
	LLSD textures = prim["textures"];
	LLSD::array_iterator array_iter = textures.beginArray();
	LLSD::array_iterator array_end = textures.endArray();
	int i = 0;
	for( ; array_iter != array_end; ++array_iter)
	{
		LLTextureEntry* wat = new LLTextureEntry();
		wat->fromLLSD(*array_iter);
		LLTextureEntry te = *wat;
		setTE(i, te);
		i++;
	}
	if(prim.has("name"))
	{
		mPrimName = prim["name"].asString();
	}
}



BuildingSupply::BuildingSupply() : LLEventTimer(0.1f)
{
}
		
BOOL BuildingSupply::tick()
{
	if(LLXmlImport::sImportInProgress && (LLXmlImport::sPrimsNeeded > 0))
	{
		LLXmlImport::sPrimsNeeded--;
		// Need moar prims
		if(LLXmlImport::sXmlImportOptions->mSupplier == NULL)
		{
			gMessageSystem->newMessageFast(_PREHASH_ObjectAdd);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU8Fast(_PREHASH_PCode, 9);
			gMessageSystem->addU8Fast(_PREHASH_Material, LL_MCODE_WOOD);
			gMessageSystem->addU32Fast(_PREHASH_AddFlags, 0);
			gMessageSystem->addU8Fast(_PREHASH_PathCurve, 16);
			gMessageSystem->addU8Fast(_PREHASH_ProfileCurve, 1);
			gMessageSystem->addU16Fast(_PREHASH_PathBegin, 0);
			gMessageSystem->addU16Fast(_PREHASH_PathEnd, 0);
			gMessageSystem->addU8Fast(_PREHASH_PathScaleX, 100);
			gMessageSystem->addU8Fast(_PREHASH_PathScaleY, 100);
			gMessageSystem->addU8Fast(_PREHASH_PathShearX, 0);
			gMessageSystem->addU8Fast(_PREHASH_PathShearY, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathTwist, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathTwistBegin, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathRadiusOffset, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathTaperX, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathTaperY, 0);
			gMessageSystem->addU8Fast(_PREHASH_PathRevolutions, 0);
			gMessageSystem->addS8Fast(_PREHASH_PathSkew, 0);
			gMessageSystem->addU16Fast(_PREHASH_ProfileBegin, 0);
			gMessageSystem->addU16Fast(_PREHASH_ProfileEnd, 0);
			gMessageSystem->addU16Fast(_PREHASH_ProfileHollow, 0);
			gMessageSystem->addU8Fast(_PREHASH_BypassRaycast, 1);
			LLVector3 rezpos = gAgent.getPositionAgent() + LLVector3(0.0f, 0.0f, 2.0f);
			gMessageSystem->addVector3Fast(_PREHASH_RayStart, rezpos);
			gMessageSystem->addVector3Fast(_PREHASH_RayEnd, rezpos);
			gMessageSystem->addUUIDFast(_PREHASH_RayTargetID, LLUUID::null);
			gMessageSystem->addU8Fast(_PREHASH_RayEndIsIntersection, 0);
			gMessageSystem->addVector3Fast(_PREHASH_Scale, LLXmlImport::sSupplyParams->getScale());
			gMessageSystem->addQuatFast(_PREHASH_Rotation, LLQuaternion::DEFAULT);
			gMessageSystem->addU8Fast(_PREHASH_State, 0);
			gMessageSystem->sendReliable(gAgent.getRegionHost());
		}
		else // have supplier
		{
			try
			{
				gMessageSystem->newMessageFast(_PREHASH_ObjectDuplicate);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
				gMessageSystem->nextBlockFast(_PREHASH_SharedData);
				LLVector3 rezpos = gAgent.getPositionAgent() + LLVector3(0.0f, 0.0f, 2.0f);
				rezpos -= LLXmlImport::sSupplyParams->getPositionRegion();
				gMessageSystem->addVector3Fast(_PREHASH_Offset, rezpos);
				gMessageSystem->addU32Fast(_PREHASH_DuplicateFlags, 0);
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
				gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, LLXmlImport::sXmlImportOptions->mSupplier->getLocalID());
				gMessageSystem->sendReliable(gAgent.getRegionHost());
			}
			catch(int)
			{
				llwarns << "Abort! Abort!" << llendl;
				return TRUE;
			}
		}
		LLFloaterImportProgress::update();
		return FALSE;
	}
	return TRUE;
}


// static
void LLXmlImport::import(LLXmlImportOptions* import_options)
{
	sXmlImportOptions = import_options;
	if(sXmlImportOptions->mSupplier == NULL)
	{
		LLViewerObject* cube = new LLViewerObject(LLUUID::null, 9, NULL, TRUE);
		cube->setScale(LLVector3(.31337f, .31337f, .31337f), FALSE);
		sSupplyParams = cube;
	}
	else sSupplyParams = sXmlImportOptions->mSupplier;

	if(!(sXmlImportOptions->mKeepPosition) && sXmlImportOptions->mRootObjects.size())
	{ // Reposition all roots so that the first root is somewhere near the avatar
		// Find the root closest to the ground
		int num_roots = (int)sXmlImportOptions->mRootObjects.size();
		int lowest_root = 0;
		F32 lowest_z(65536.f);
		for(int i = 0; i < num_roots; i++)
		{
			F32 z = sXmlImportOptions->mRootObjects[i]->getPosition().mV[2];
			if(z < lowest_z)
			{
				lowest_root = i;
				lowest_z = z;
			}
		}
		// Move all roots
		LLVector3 old_pos = sXmlImportOptions->mRootObjects[lowest_root]->getPosition();
		LLVector3 new_pos = gAgent.getPositionAgent() + (gAgent.getAtAxis() * 2.0f);
		LLVector3 difference = new_pos - old_pos;
		for(int i = 0; i < num_roots; i++)
		{
			sXmlImportOptions->mRootObjects[i]->setPosition(sXmlImportOptions->mRootObjects[i]->getPosition() + difference, FALSE);
		}
	}

	// Make the actual importable list
	sPrims.clear();
	// Clear these attachment-related maps
	sPt2watch.clear();
	sId2attachpt.clear();
	sPt2attachpos.clear();
	sPt2attachrot.clear();
	// Go ahead and add roots first
	std::vector<LLImportObject*>::iterator root_iter = sXmlImportOptions->mRootObjects.begin();
	std::vector<LLImportObject*>::iterator root_end = sXmlImportOptions->mRootObjects.end();
	for( ; root_iter != root_end; ++root_iter)
	{
		sPrims.push_back(*root_iter);
		// Remember some attachment info
		if((*root_iter)->importIsAttachment)
		{
			sId2attachpt[(*root_iter)->mId] = (*root_iter)->importAttachPoint;
			sPt2watch[(*root_iter)->importAttachPoint] = true;
			sPt2attachpos[(*root_iter)->importAttachPoint] = (*root_iter)->importAttachPos;
			sPt2attachrot[(*root_iter)->importAttachPoint] = (*root_iter)->importAttachRot;
		}
	}
	// Then add children, nearest first
	std::vector<LLImportObject*> children(sXmlImportOptions->mChildObjects);
	for(root_iter = sXmlImportOptions->mRootObjects.begin() ; root_iter != root_end; ++root_iter)
	{
		while(children.size() > 0)
		{
			std::string rootid = (*root_iter)->mId;
			F32 lowest_mag = 65536.0f;
			std::vector<LLImportObject*>::iterator lowest_child_iter = children.begin();
			LLImportObject* lowest_child = (*lowest_child_iter);
			
			std::vector<LLImportObject*>::iterator child_end = children.end();
			for(std::vector<LLImportObject*>::iterator child_iter = children.begin() ; child_iter != child_end; ++child_iter)
			{
				if((*child_iter)->mParentId == rootid)
				{
					F32 mag = (*child_iter)->getPosition().magVec();
					if(mag < lowest_mag)
					{
						lowest_child_iter = child_iter;
						lowest_child = (*lowest_child_iter);
						lowest_mag = mag;
					}
				}
			}
			sPrims.push_back(lowest_child);
			children.erase(lowest_child_iter);
		}
	}
	
	sImportInProgress = true;
	sImportHasAttachments = (sId2attachpt.size() > 0);
	sPrimsNeeded = (int)sPrims.size();
	sPrimIndex = 0;
	sId2localid.clear();
	sRootpositions.clear();

	LLFloaterImportProgress::show();
	LLFloaterImportProgress::update();

	// Create folder
	if((sXmlImportOptions->mWearables.size() > 0) || (sId2attachpt.size() > 0))
	{
		sFolderID = gInventory.createNewCategory( gAgent.getInventoryRootID(), LLAssetType::AT_NONE, sXmlImportOptions->mName);
	}

	// Go ahead and upload wearables
	int num_wearables = sXmlImportOptions->mWearables.size();
	for(int i = 0; i < num_wearables; i++)
	{
		LLAssetType::EType at = LLAssetType::AT_CLOTHING;
		if(sXmlImportOptions->mWearables[i]->mType < 4) at = LLAssetType::AT_BODYPART;
		LLUUID tid;
		tid.generate();
		// Create asset
		gMessageSystem->newMessageFast(_PREHASH_AssetUploadRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AssetBlock);
		gMessageSystem->addUUIDFast(_PREHASH_TransactionID, tid);
		gMessageSystem->addS8Fast(_PREHASH_Type, (S8)at);
		gMessageSystem->addBOOLFast(_PREHASH_Tempfile, FALSE);
		gMessageSystem->addBOOLFast(_PREHASH_StoreLocal, FALSE);
		gMessageSystem->addStringFast(_PREHASH_AssetData, sXmlImportOptions->mWearables[i]->mData.c_str());
		gMessageSystem->sendReliable(gAgent.getRegionHost());
		// Create item
		gMessageSystem->newMessageFast(_PREHASH_CreateInventoryItem);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_InventoryBlock);
		gMessageSystem->addU32Fast(_PREHASH_CallbackID, 0);
		gMessageSystem->addUUIDFast(_PREHASH_FolderID, sFolderID);
		gMessageSystem->addUUIDFast(_PREHASH_TransactionID, tid);
		gMessageSystem->addU32Fast(_PREHASH_NextOwnerMask, 532480);
		gMessageSystem->addS8Fast(_PREHASH_Type, at);
		gMessageSystem->addS8Fast(_PREHASH_InvType, 18);
		gMessageSystem->addS8Fast(_PREHASH_WearableType, sXmlImportOptions->mWearables[i]->mType);
		gMessageSystem->addStringFast(_PREHASH_Name, sXmlImportOptions->mWearables[i]->mName.c_str());
		gMessageSystem->addStringFast(_PREHASH_Description, "");
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}

	new BuildingSupply();
}

// static
void LLXmlImport::onNewPrim(LLViewerObject* object)
{
	
	
	int currPrimIndex = sPrimIndex++;
	
	if(currPrimIndex >= (int)sPrims.size())
	{
		if(sAttachmentsDone >= (int)sPt2attachpos.size())
		{
			// "stop calling me"
			sImportInProgress = false;
			return;
		}
	}
	
	LLImportObject* from = sPrims[currPrimIndex];

	// Flags
	// trying this first in case it helps when supply is physical...
	U32 flags = from->mFlags;
	flags = flags & (~FLAGS_USE_PHYSICS);
	object->setFlags(flags, TRUE);
	object->setFlags(~flags, FALSE); // Can I improve this lol?

	if(from->mParentId == "")
	{
		// this will be a root
		sId2localid[from->mId] = object->getLocalID();
		sRootpositions[object->getLocalID()] = from->getPosition();
		// If it's an attachment, set description
		if(from->importIsAttachment)
		{
			gMessageSystem->newMessageFast(_PREHASH_ObjectDescription);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_LocalID, object->getLocalID());
			gMessageSystem->addStringFast(_PREHASH_Description, from->mId);
			gMessageSystem->sendReliable(gAgent.getRegionHost());
		}
	}
	else
	{
		// Move it to its root before linking
		U32 parentlocalid = sId2localid[from->mParentId];
		LLVector3 rootpos = sRootpositions[parentlocalid];
		
		U8 data[256];
		S32 offset = 0;
		gMessageSystem->newMessageFast(_PREHASH_MultipleObjectUpdate);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->addU8Fast(_PREHASH_Type, 5);
		htonmemcpy(&data[offset], &(rootpos.mV), MVT_LLVector3, 12);
		offset += 12;
		htonmemcpy(&data[offset], &(from->getScale().mV), MVT_LLVector3, 12); 
		offset += 12;
		gMessageSystem->addBinaryDataFast(_PREHASH_Data, data, offset);
		gMessageSystem->sendReliable(gAgent.getRegionHost());

		// Link it up
		gMessageSystem->newMessageFast(_PREHASH_ObjectLink);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, sId2localid[from->mParentId]);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}
	// Volume params
	LLVolumeParams params = from->getVolume()->getParams();
	object->setVolume(params, 0, false);
	// Extra params
	if(from->isFlexible())
	{
		LLFlexibleObjectData flex = *((LLFlexibleObjectData*)from->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE));
		object->setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, flex, true);
		object->setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, true);
		object->parameterChanged(LLNetworkData::PARAMS_FLEXIBLE, true);
	}
	else
	{
		// send param not in use in case the supply prim has it
		object->setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, true);
		object->parameterChanged(LLNetworkData::PARAMS_FLEXIBLE, true);
	}
	if (from->getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT))
	{
		LLLightParams light = *((LLLightParams*)from->getParameterEntry(LLNetworkData::PARAMS_LIGHT));
		object->setParameterEntry(LLNetworkData::PARAMS_LIGHT, light, true);
		object->setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, TRUE, true);
		object->parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
	}
	else
	{
		// send param not in use in case the supply prim has it
		object->setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, FALSE, true);
		object->parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
	}
	if (from->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
	{
		LLSculptParams sculpt = *((LLSculptParams*)from->getParameterEntry(LLNetworkData::PARAMS_SCULPT));
		object->setParameterEntry(LLNetworkData::PARAMS_SCULPT, sculpt, true);
		object->setParameterEntryInUse(LLNetworkData::PARAMS_SCULPT, TRUE, true);
		object->parameterChanged(LLNetworkData::PARAMS_SCULPT, true);
	}
	else
	{
		// send param not in use in case the supply prim has it
		object->setParameterEntryInUse(LLNetworkData::PARAMS_SCULPT, FALSE, true);
		object->parameterChanged(LLNetworkData::PARAMS_SCULPT, true);
	}
	// Textures
	U8 te_count = from->getNumTEs();
	for (U8 i = 0; i < te_count; i++)
	{
		const LLTextureEntry* wat = from->getTE(i);
		LLTextureEntry te = *wat;
		object->setTE(i, te);
	}

	object->sendShapeUpdate();
	object->sendTEUpdate();
	// Flag update is already coming from somewhere
	//object->updateFlags();

	// Transforms
	object->setScale(from->getScale(), FALSE);
	object->setRotation(from->getRotation(), FALSE);
	object->setPosition(from->getPosition(), FALSE);

	U8 data[256];
	S32 offset = 0;
	// Position and rotation
	gMessageSystem->newMessageFast(_PREHASH_MultipleObjectUpdate);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
	gMessageSystem->addU8Fast(_PREHASH_Type, 3);
	htonmemcpy(&data[offset], &(object->getPosition().mV), MVT_LLVector3, 12);
	offset += 12;
	LLQuaternion quat = object->getRotation();
	LLVector3 vec = quat.packToVector3();
	htonmemcpy(&data[offset], &(vec.mV), MVT_LLQuaternion, 12); 
	offset += 12;
	gMessageSystem->addBinaryDataFast(_PREHASH_Data, data, offset);
	gMessageSystem->sendReliable(gAgent.getRegionHost());
	// Position and scale
	offset = 0;
	gMessageSystem->newMessageFast(_PREHASH_MultipleObjectUpdate);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
	gMessageSystem->addU8Fast(_PREHASH_Type, 5);
	htonmemcpy(&data[offset], &(object->getPosition().mV), MVT_LLVector3, 12); 
	offset += 12;
	htonmemcpy(&data[offset], &(object->getScale().mV), MVT_LLVector3, 12); 
	offset += 12;
	gMessageSystem->addBinaryDataFast(_PREHASH_Data, data, offset);
	gMessageSystem->sendReliable(gAgent.getRegionHost());

	// Name
	if(from->mPrimName != "")
	{
		gMessageSystem->newMessageFast(_PREHASH_ObjectName);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_LocalID, object->getLocalID());
		gMessageSystem->addStringFast(_PREHASH_Name, from->mPrimName);
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}
	if(currPrimIndex + 1 >= (int)sPrims.size())
	{
		if(sId2attachpt.size() == 0)
		{
			sImportInProgress = false;
			std::string msg = "Imported " + sXmlImportOptions->mName;
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
			LLFloaterImportProgress::update();
			return;
		}
		else
		{
			// Take attachables into inventory
			std::string msg = "Wait a few moments for the attachments to attach...";
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);

			sAttachmentsDone = 0;
			std::map<std::string, U8>::iterator at_iter = sId2attachpt.begin();
			std::map<std::string, U8>::iterator at_end = sId2attachpt.end();
			for( ; at_iter != at_end; ++at_iter)
			{
				LLUUID tid;
				tid.generate();
				U32 at_localid = sId2localid[(*at_iter).first];
				gMessageSystem->newMessageFast(_PREHASH_DeRezObject);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				gMessageSystem->nextBlockFast(_PREHASH_AgentBlock);
				gMessageSystem->addUUIDFast(_PREHASH_GroupID, LLUUID::null);
				gMessageSystem->addU8Fast(_PREHASH_Destination, DRD_TAKE_INTO_AGENT_INVENTORY);
				gMessageSystem->addUUIDFast(_PREHASH_DestinationID, sFolderID);
				gMessageSystem->addUUIDFast(_PREHASH_TransactionID, tid);
				gMessageSystem->addU8Fast(_PREHASH_PacketCount, 1);
				gMessageSystem->addU8Fast(_PREHASH_PacketNumber, 0);
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
				gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, at_localid);
				gMessageSystem->sendReliable(gAgent.getRegionHost());
			}
		}
	}
	
	LLFloaterImportProgress::update();
}

// static
void LLXmlImport::onNewItem(LLViewerInventoryItem* item)
{
	U8 attachpt = sId2attachpt[item->getDescription()];
	if(attachpt)
	{
		// clear description, part 1
		item->setDescription(std::string("(No Description)"));
		item->updateServer(FALSE);

		// Attach it
		gMessageSystem->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addUUIDFast(_PREHASH_ItemID, item->getUUID());
		gMessageSystem->addUUIDFast(_PREHASH_OwnerID, gAgent.getID());
		gMessageSystem->addU8Fast(_PREHASH_AttachmentPt, attachpt);
		gMessageSystem->addU32Fast(_PREHASH_ItemFlags, 0);
		gMessageSystem->addU32Fast(_PREHASH_GroupMask, 0);
		gMessageSystem->addU32Fast(_PREHASH_EveryoneMask, 0);
		gMessageSystem->addU32Fast(_PREHASH_NextOwnerMask, 0);
		gMessageSystem->addStringFast(_PREHASH_Name, item->getName());
		gMessageSystem->addStringFast(_PREHASH_Description, "");
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}
}

// static
void LLXmlImport::onNewAttachment(LLViewerObject* object)
{
	if(sPt2attachpos.size() == 0) return;

	U8 attachpt = (U8)object->getAttachmentPoint();
	if(sPt2watch[attachpt])
	{
		// clear description, part 2
		gMessageSystem->newMessageFast(_PREHASH_ObjectDescription);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_LocalID, object->getLocalID());
		gMessageSystem->addStringFast(_PREHASH_Description, "");
		gMessageSystem->sendReliable(gAgent.getRegionHost());

		// position and rotation
		LLVector3 pos = sPt2attachpos[attachpt];
		U8 data[256];
		S32 offset = 0;
		gMessageSystem->newMessageFast(_PREHASH_MultipleObjectUpdate);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->addU8Fast(_PREHASH_Type, 11); // link set this time
		htonmemcpy(&data[offset], &(pos.mV), MVT_LLVector3, 12);
		offset += 12;
		LLQuaternion quat = sPt2attachrot[attachpt];
		LLVector3 vec = quat.packToVector3();
		htonmemcpy(&data[offset], &(vec.mV), MVT_LLQuaternion, 12); 
		offset += 12;
		gMessageSystem->addBinaryDataFast(_PREHASH_Data, data, offset);
		gMessageSystem->sendReliable(gAgent.getRegionHost());

		// Select and deselect to make it send an update
		gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->sendReliable(gAgent.getRegionHost());

		gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->sendReliable(gAgent.getRegionHost());

		// Done?
		sAttachmentsDone++;
		if(sAttachmentsDone >= (int)sPt2attachpos.size())
		{
			sImportInProgress = false;
			std::string msg = "Imported " + sXmlImportOptions->mName;
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
			LLFloaterImportProgress::update();
			return;
		}
	}
}



// static
void LLXmlImport::Cancel(void* user_data)
{
	sImportInProgress = false;
	LLFloaterImportProgress::sInstance->close(false);
}



LLFloaterImportProgress::LLFloaterImportProgress() 
:	LLFloater("ImportProgress", LLRect(0, 100, 400, 0), "Import progress")
{
	LLLineEditor* line = new LLLineEditor(
		std::string("Created"),
		LLRect(4, 80, 396, 60),
		std::string("Created prims"));
	line->setEnabled(FALSE);
	addChild(line);

	LLViewBorder* border = new LLViewBorder(
		"CreatedBorder",
		LLRect(4, 79, 395, 60));
	addChild(border);

	line = new LLLineEditor(
		std::string("Edited"),
		LLRect(4, 55, 396, 35),
		std::string("Edited prims"));
	line->setEnabled(FALSE);
	addChild(line);

	border = new LLViewBorder(
		"EditedBorder",
		LLRect(4, 54, 395, 35));
	addChild(border);

	LLButton* button = new LLButton(
		"CancelButton",
		LLRect(300, 28, 394, 8));
	button->setLabel(std::string("Cancel"));
	button->setEnabled(TRUE);
	addChild(button);
	childSetAction("CancelButton", LLXmlImport::Cancel, this);

	sInstance = this;
}

LLFloaterImportProgress::~LLFloaterImportProgress()
{
	sInstance = NULL;
}

void LLFloaterImportProgress::close(bool app_quitting)
{
	LLXmlImport::sImportInProgress = false;
	LLFloater::close(app_quitting);
}

// static
void LLFloaterImportProgress::show()
{
	if(!sInstance)
		sInstance = new LLFloaterImportProgress();
	sInstance->open();
	sInstance->center();
}

// static
void LLFloaterImportProgress::update()
{
	if(sInstance)
	{
		LLFloaterImportProgress* floater = sInstance;

		int create_goal = (int)LLXmlImport::sPrims.size();
		int create_done = create_goal - LLXmlImport::sPrimsNeeded;
		F32 create_width = F32(390.f / F32(create_goal));
		create_width *= F32(create_done);
		bool create_finished = create_done >= create_goal;

		int edit_goal = create_goal;
		int edit_done = LLXmlImport::sPrimIndex;
		F32 edit_width = F32(390.f / F32(edit_goal));
		edit_width *= edit_done;
		bool edit_finished = edit_done >= edit_goal;

		int attach_goal = (int)LLXmlImport::sId2attachpt.size();
		int attach_done = LLXmlImport::sAttachmentsDone;
		F32 attach_width = F32(390.f / F32(attach_goal));
		attach_width *= F32(attach_done);
		bool attach_finished = attach_done >= attach_goal;

		bool all_finished = create_finished && edit_finished && attach_finished;

		std::string title;
		title.assign(all_finished ? "Imported " : "Importing ");
		title.append(LLXmlImport::sXmlImportOptions->mName);
		if(!all_finished) title.append("...");
		floater->setTitle(title);

		std::string created_text = llformat("Created %d/%d prims", S32(create_done), S32(create_goal));
		std::string edited_text = llformat("Finished %d/%d prims", edit_done, edit_goal);

		LLLineEditor* text = floater->getChild<LLLineEditor>("Created");
		text->setText(created_text);

		text = floater->getChild<LLLineEditor>("Edited");
		text->setText(edited_text);

		LLViewBorder* border = floater->getChild<LLViewBorder>("CreatedBorder");
		border->setRect(LLRect(4, 79, 4 + create_width, 60));

		border = floater->getChild<LLViewBorder>("EditedBorder");
		border->setRect(LLRect(4, 54, 4 + edit_width, 35));

		LLButton* button = floater->getChild<LLButton>("CancelButton");
		button->setEnabled(!all_finished);
	}
}

// </edit>
