// <edit>

#include "llviewerprecompiledheaders.h"
#include "llcheats.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llagent.h"
#include "llinventorymodel.h"
#include "llmochascript.h"

std::map<std::string, LLCheatCode> LLCheats::cheatCodes;
KEY LLCheats::lastKeys[CHEAT_CODE_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// static
void LLCheats::init()
{
	KEY AcquireAssetID[CHEAT_CODE_SIZE] = {132, 132, 133, 133, 130, 131, 130, 131, 66, 65, 129, 0, 0, 0, 0, 0};
	createCode("AcquireAssetID", AcquireAssetID);
	KEY EverythingIsOk[CHEAT_CODE_SIZE] = {66, 65, 66, 65, 132, 133, 66, 65, 130, 131, 66, 65, 129, 0, 0, 0};
	createCode("EverythingIsOk", EverythingIsOk);
	KEY MochaScript[CHEAT_CODE_SIZE] = {'M', 'O', 'C', 'H', 'A', 'S', 'C', 'R', 'I', 'P', 'T', 0, 0, 0, 0, 0};
	createCode("MochaScript", MochaScript);
}

// static
void LLCheats::createCode(std::string name, KEY keys[CHEAT_CODE_SIZE])
{
	LLCheatCode code;
	code.entered = false;
	// find last non-zero key
	int s = CHEAT_CODE_SIZE - 1;
	while(!keys[s] && s)
		--s;
	// add keys in reverse order
	int i = 0;
	for( ; s >= 0; s--)
	{
		code.keySequence[i] = keys[s];
		++i;
	}
	// zero the rest
	for( ; i < CHEAT_CODE_SIZE; i++)
		code.keySequence[i] = 0;
	// register
	cheatCodes[name] = code;
}

// static
bool LLCheats::checkForCode(LLCheatCode code)
{
	for(int i = 0; i < CHEAT_CODE_SIZE; i++)
	{
		if(!code.keySequence[i]) return true;
		if(code.keySequence[i] != lastKeys[i]) return false;
	}
	return true;
}

// static
void LLCheats::pressKey(KEY key)
{
	//llwarns << "Pressed " << llformat("%d", key) << llendl;

	for(int i = (CHEAT_CODE_SIZE - 1); i > 0; i--)
		lastKeys[i] = lastKeys[i - 1];
	lastKeys[0] = key;

	std::map<std::string, LLCheatCode>::iterator iter = cheatCodes.begin();
	std::map<std::string, LLCheatCode>::iterator end = cheatCodes.end();
	for( ; iter != end; ++iter)
	{
		if(!(*iter).second.entered)
		{
			(*iter).second.entered = checkForCode((*iter).second);
			if((*iter).second.entered)
			{
				onCheatEnabled((*iter).first);
			}
		}
	}
}

void LLCheats::onCheatEnabled(std::string code_name)
{
	LLFloaterChat::addChat(LLChat(code_name + " code entered"));
	if(code_name == "MochaScript")
	{
		LLFloaterMochaScript::show();
		cheatCodes[code_name].entered = false;
	}
}


bool LLAssetIDAcquirer::mBusy = false;
std::vector<LLUUID> LLAssetIDAcquirer::mQueue;
LLUUID LLAssetIDAcquirer::mItemID;
LLUUID LLAssetIDAcquirer::mUnderwear;

// static
void LLAssetIDAcquirer::acquire(std::set<LLUUID> item_list)
{
	if(!LLCheats::cheatCodes["AcquireAssetID"].entered) return;

	// add to queue
	std::set<LLUUID>::iterator iter = item_list.begin();
	std::set<LLUUID>::iterator end = item_list.end();
	for( ; iter != end; ++iter)
		mQueue.push_back(*iter);

	work();
}

// static
void LLAssetIDAcquirer::work()
{
	if(mQueue.size())
	{
		if(mBusy)
		{
			// waiting
		}
		else
		{
			mBusy = true;
			mItemID = *(mQueue.begin());
			mUnderwear = gAgent.getWearableItem(WT_UNDERPANTS);
			
			gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			for(int i=0; i < WT_COUNT; ++i)
			{
				gMessageSystem->nextBlockFast(_PREHASH_WearableData);
				gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(i));
				if(i != WT_UNDERPANTS)
					gMessageSystem->addUUIDFast(_PREHASH_ItemID, gAgent.getWearableItem((EWearableType)i));
				else
					gMessageSystem->addUUIDFast(_PREHASH_ItemID, mItemID);
			}
			gAgent.sendReliableMessage();
			new LLAssetIDRequester();
		}
	}
	else
	{
		mBusy = false;

		// finished, so set back to normal			
		gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		for(int i = 0; i < WT_COUNT; ++i)
		{
			gMessageSystem->nextBlockFast(_PREHASH_WearableData);
			gMessageSystem->addU8Fast(_PREHASH_WearableType, U8(i));
			if(i != WT_UNDERPANTS)
				gMessageSystem->addUUIDFast(_PREHASH_ItemID, gAgent.getWearableItem((EWearableType)i));
			else
				gMessageSystem->addUUIDFast(_PREHASH_ItemID, mUnderwear);
		}
		gAgent.sendReliableMessage();
	}
}

// static
void LLAssetIDAcquirer::handle(LLMessageSystem* mesgsys)
{
	if(!mBusy) return;
	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if(agent_id != gAgent.getID()) return;
	S32 num_wearables = gMessageSystem->getNumberOfBlocksFast(_PREHASH_WearableData);
	for(int i = 0; i < num_wearables; i++)
	{
		U8 type_u8 = 0;
		gMessageSystem->getU8Fast(_PREHASH_WearableData, _PREHASH_WearableType, type_u8, i );
		if(type_u8 == WT_UNDERPANTS)
		{
			LLUUID item_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_ItemID, item_id, i );
			LLUUID asset_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_AssetID, asset_id, i );
			if(item_id == mItemID)
			{
				LLViewerInventoryItem* item = gInventory.getItem(item_id);
				if(item)
				{
					item->setAssetUUID(asset_id);
				}
			}
			// anyway
			// remove from queue
			std::vector<LLUUID>::iterator iter = std::find(mQueue.begin(), mQueue.end(), item_id);
			if(iter != mQueue.end())
				mQueue.erase(iter);

			// continue
			mBusy = false;
			work();
		}
	}
}

LLAssetIDRequester::LLAssetIDRequester() : LLEventTimer(0.25f)
{
}

BOOL LLAssetIDRequester::tick()
{
	gMessageSystem->newMessageFast(_PREHASH_AgentWearablesRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	gAgent.sendReliableMessage();
	return TRUE;
}

// </edit>
