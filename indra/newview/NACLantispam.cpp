/* DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                   Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */

#include "llviewerprecompiledheaders.h"
#include "NACLantispam.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"
#include "llviewerobjectlist.h"
#include "llagent.h"
#include "lltrans.h"
#include <time.h>
#include <boost/lexical_cast.hpp>

bool can_block(const LLUUID& id)
{
	if (id.isNull() || gAgent.getID() == id) return false; //Can't block system or self.
	if (const LLViewerObject* obj = gObjectList.findObject(id)) //From an object,
		return !obj->permYouOwner(); //not own object.
	return true;
}

U32 NACLAntiSpamRegistry::globalAmount;
U32 NACLAntiSpamRegistry::globalTime;
bool NACLAntiSpamRegistry::bGlobalQueue;
NACLAntiSpamQueue* NACLAntiSpamRegistry::queues[NACLAntiSpamRegistry::QUEUE_MAX] = {0};
std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*> NACLAntiSpamRegistry::globalEntries;
std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*>::iterator NACLAntiSpamRegistry::it2;

// The following sounds will be ignored for purposes of spam protection. They have been gathered from wiki documentation of frequent official sounds.
const std::string COLLISION_SOUNDS[] ={"dce5fdd4-afe4-4ea1-822f-dd52cac46b08","51011582-fbca-4580-ae9e-1a5593f094ec","68d62208-e257-4d0c-bbe2-20c9ea9760bb","75872e8c-bc39-451b-9b0b-042d7ba36cba","6a45ba0b-5775-4ea8-8513-26008a17f873","992a6d1b-8c77-40e0-9495-4098ce539694","2de4da5a-faf8-46be-bac6-c4d74f1e5767","6e3fb0f7-6d9c-42ca-b86b-1122ff562d7d","14209133-4961-4acc-9649-53fc38ee1667","bc4a4348-cfcc-4e5e-908e-8a52a8915fe6","9e5c1297-6eed-40c0-825a-d9bcd86e3193","e534761c-1894-4b61-b20c-658a6fb68157","8761f73f-6cf9-4186-8aaa-0948ed002db1","874a26fd-142f-4173-8c5b-890cd846c74d","0e24a717-b97e-4b77-9c94-b59a5a88b2da","75cf3ade-9a5b-4c4d-bb35-f9799bda7fb2","153c8bf7-fb89-4d89-b263-47e58b1b4774","55c3e0ce-275a-46fa-82ff-e0465f5e8703","24babf58-7156-4841-9a3f-761bdbb8e237","aca261d8-e145-4610-9e20-9eff990f2c12","0642fba6-5dcf-4d62-8e7b-94dbb529d117","25a863e8-dc42-4e8a-a357-e76422ace9b5","9538f37c-456e-4047-81be-6435045608d4","8c0f84c3-9afd-4396-b5f5-9bca2c911c20","be582e5d-b123-41a2-a150-454c39e961c8","c70141d4-ba06-41ea-bcbc-35ea81cb8335","7d1826f4-24c4-4aac-8c2e-eff45df37783","063c97d3-033a-4e9b-98d8-05c8074922cb","00000000-0000-0000-0000-000000000120"};
const int COLLISION_SOUNDS_SIZE=29;

// NaClAntiSpamQueueEntry

NACLAntiSpamQueueEntry::NACLAntiSpamQueueEntry()
{
	entryTime=0;
	entryAmount=0;
	blocked=false;
}
void NACLAntiSpamQueueEntry::clearEntry()
{
	entryTime=0;
	entryAmount=0;
	blocked=false;
}
U32 NACLAntiSpamQueueEntry::getEntryAmount() const
{
	return entryAmount;
}
U32 NACLAntiSpamQueueEntry::getEntryTime() const
{
	return entryTime;
}
void NACLAntiSpamQueueEntry::updateEntryAmount()
{
	entryAmount++;
}
void NACLAntiSpamQueueEntry::updateEntryTime()
{
	entryTime=time(0);
}
void NACLAntiSpamQueueEntry::setBlocked()
{
	blocked=true;
}
bool NACLAntiSpamQueueEntry::getBlocked() const
{
	return blocked;
}

// NaClAntiSpamQueue

NACLAntiSpamQueue::NACLAntiSpamQueue(U32 time, U32 amount)
{
	queueTime=time;
	queueAmount=amount;
}
void NACLAntiSpamQueue::setAmount(U32 amount)
{
	queueAmount=amount;
}
void NACLAntiSpamQueue::setTime(U32 time)
{
	queueTime=time;
}
U32 NACLAntiSpamQueue::getAmount() const
{
	return queueAmount;
}
U32 NACLAntiSpamQueue::getTime() const
{
	return queueTime;
}
void NACLAntiSpamQueue::clearEntries()
{
	for(it = entries.begin(); it != entries.end(); it++)
	{
		it->second->clearEntry();
	}
}
void NACLAntiSpamQueue::purgeEntries()
{
	for(it = entries.begin(); it != entries.end(); it++)
	{
		delete it->second;
	}
	entries.clear();
}
void NACLAntiSpamQueue::blockEntry(LLUUID& source)
{
	it=entries.find(source.asString());
	if(it == entries.end())
	{
		entries[source.asString()]=new NACLAntiSpamQueueEntry();
	}
	entries[source.asString()]->setBlocked();
}
int NACLAntiSpamQueue::checkEntry(LLUUID& name, U32 multiplier)
// Returns 0 if unblocked/disabled, 1 if check results in a new block, 2 if by an existing block
{
	static LLCachedControl<bool> enabled(gSavedSettings,"AntiSpamEnabled",false);
	if(!enabled) return 0;
	it=entries.find(name.asString());
	if(it != entries.end())
	{
		if(it->second->getBlocked()) return 2;
		U32 eTime=it->second->getEntryTime();
		U32 currentTime=time(0);
		if((currentTime-eTime) <= queueTime)
		{
			it->second->updateEntryAmount();
			U32 eAmount=it->second->getEntryAmount();
			if(eAmount > (queueAmount*multiplier))
			{
				it->second->setBlocked();
				return 1;
			}
			else
				return 0;
		}
		else
		{
			it->second->clearEntry();
			it->second->updateEntryAmount();
			it->second->updateEntryTime();
			return 0;
		}
	}
	else
	{
		//lldebugs << "[antispam] New queue entry:" << name.asString() << llendl;
		entries[name.asString()]=new NACLAntiSpamQueueEntry();
		entries[name.asString()]->updateEntryAmount();
		entries[name.asString()]->updateEntryTime();
		return 0;
	}
}

// NaClAntiSpamRegistry

static const char* QUEUE_NAME[NACLAntiSpamRegistry::QUEUE_MAX] = {
"Chat",
"Inventory",
"Instant Message",
"calling card",
"sound",
"Sound Preload",
"Script Dialog",
"Teleport"};

NACLAntiSpamRegistry::NACLAntiSpamRegistry(U32 time, U32 amount)
{
	globalTime=time;
	globalAmount=amount;
	static LLCachedControl<bool> _NACL_AntiSpamGlobalQueue(gSavedSettings,"_NACL_AntiSpamGlobalQueue");
	bGlobalQueue=_NACL_AntiSpamGlobalQueue;
	for(int queue = 0; queue < QUEUE_MAX; ++queue)
	{
		queues[queue] = new NACLAntiSpamQueue(time,amount);
	}
}
//static
const char* NACLAntiSpamRegistry::getQueueName(U32 queue_id)
{
	if(queue_id >= QUEUE_MAX)
		return "Unknown";
	return QUEUE_NAME[queue_id];
}
//static
void NACLAntiSpamRegistry::registerQueues(U32 time, U32 amount)
{
	globalTime=time;
	globalAmount=amount;
	static LLCachedControl<bool> _NACL_AntiSpamGlobalQueue(gSavedSettings,"_NACL_AntiSpamGlobalQueue");
	bGlobalQueue=_NACL_AntiSpamGlobalQueue;
	for(int queue = 0; queue < QUEUE_MAX; ++queue)
	{
		queues[queue] = new NACLAntiSpamQueue(time,amount);
	}
}
//static
/*void NACLAntiSpamRegistry::registerQueue(U32 name, U32 time, U32 amount)
{
	it=queues.find(name);
	if(it == queues.end())
	{
		queues[name]=new NACLAntiSpamQueue(time,amount);
	}
}*/
//static
void NACLAntiSpamRegistry::setRegisteredQueueTime(U32 name, U32 time)
{
	if(name >= QUEUE_MAX || queues[name] == 0)
	{
		LL_ERRS("AntiSpam") << "CODE BUG: Attempting to set time of antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
		return;
	}
	
	queues[name]->setTime(time);
}
//static
void NACLAntiSpamRegistry::setRegisteredQueueAmount(U32 name, U32 amount)
{
	if(name >= QUEUE_MAX || queues[name] == 0)
	{
		LL_ERRS("AntiSpam") << "CODE BUG: Attempting to set amount for antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
		return;
	}
	
	queues[name]->setAmount(amount);
}
//static
void NACLAntiSpamRegistry::setAllQueueTimes(U32 time)
{
	globalTime=time;
	for(int queue = 0; queue < QUEUE_MAX; ++queue)
		if( queues[queue] )
			queues[queue]->setTime(time);
}
//static
void NACLAntiSpamRegistry::setAllQueueAmounts(U32 amount)
{
	globalAmount=amount;
	for(int queue = 0; queue < QUEUE_MAX; ++queue)
	{
		if(!queues[queue])	continue;
		if(queue == QUEUE_SOUND || queue == QUEUE_SOUND_PRELOAD)
			queues[queue]->setAmount(amount*5);
		else
			queues[queue]->setAmount(amount);
	}
}
//static
void NACLAntiSpamRegistry::clearRegisteredQueue(U32 name)
{
	if(name >= QUEUE_MAX || queues[name] == 0)
	{
		LL_ERRS("AntiSpam") << "CODE BUG: Attempting to clear antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
		return;
	}
	
	queues[name]->clearEntries();
}
//static
void NACLAntiSpamRegistry::purgeRegisteredQueue(U32 name)
{
	if(name >= QUEUE_MAX || queues[name] == 0)
	{
		LL_ERRS("AntiSpam") << "CODE BUG: Attempting to purge antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
		return;
	}
	
	queues[name]->purgeEntries();
}
//static
void NACLAntiSpamRegistry::blockOnQueue(U32 name, LLUUID& source)
{
	if(bGlobalQueue)
	{
		NACLAntiSpamRegistry::blockGlobalEntry(source);
	}
	else
	{
		if(name >= QUEUE_MAX || queues[name] == 0)
		{
			LL_ERRS("AntiSpam") << "CODE BUG: Attempting to use a antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
			return;
		}
		queues[name]->blockEntry(source);
	}
}
//static
void NACLAntiSpamRegistry::blockGlobalEntry(LLUUID& source)
{
	it2=globalEntries.find(source.asString());
	if(it2 == globalEntries.end())
	{
		globalEntries[source.asString()]=new NACLAntiSpamQueueEntry();
	}
	globalEntries[source.asString()]->setBlocked();
}
//static
bool NACLAntiSpamRegistry::checkQueue(U32 name, LLUUID& source, U32 multiplier)
//returns true if blocked
{
	if (!can_block(source)) return false;

	int result;
	if(bGlobalQueue)
	{
		result=NACLAntiSpamRegistry::checkGlobalEntry(source,multiplier);
	}
	else
	{
		if(name >= QUEUE_MAX || queues[name] == 0)
		{
			LL_ERRS("AntiSpam") << "CODE BUG: Attempting to check antispam queue that was outside of the reasonable range of queues or not created. Queue: " << getQueueName(name) << llendl;
			return false;
		}
		result=queues[name]->checkEntry(source,multiplier);
	}
	if(result == 0) //Safe
		return false;
	else if(result == 2) //Previously blocked
	{
		return true;
	}
	else //Just blocked!
	{
		if(gSavedSettings.getBOOL("AntiSpamNotify"))
		{
			LLSD args;
			args["SOURCE"] = source.asString().c_str();
			args["TYPE"] = LLTrans::getString(getQueueName(name));
			args["AMOUNT"] = boost::lexical_cast<std::string>(multiplier * queues[name]->getAmount());
			args["TIME"] = boost::lexical_cast<std::string>(queues[name]->getTime());
			LLNotificationsUtil::add("AntiSpamBlock", args);
		}
		return true;
	}
}

// Global queue stoof
//static
void NACLAntiSpamRegistry::setGlobalQueue(bool value)
{
	NACLAntiSpamRegistry::purgeAllQueues();
	bGlobalQueue=value;
}
//static
void NACLAntiSpamRegistry::setGlobalAmount(U32 amount)
{
	globalAmount=amount;
}
//static
void NACLAntiSpamRegistry::setGlobalTime(U32 time)
{
	globalTime=time;
}
//static
void NACLAntiSpamRegistry::clearAllQueues()
{
	if(bGlobalQueue)
		NACLAntiSpamRegistry::clearGlobalEntries();
	else
	for(int queue = 0; queue < QUEUE_MAX; ++queue)
	{
		if(queues[queue])	queues[queue]->clearEntries();
	}
}
//static
void NACLAntiSpamRegistry::purgeAllQueues()
{
	if(bGlobalQueue)
		NACLAntiSpamRegistry::purgeGlobalEntries();
	else
		for(int queue = 0; queue < QUEUE_MAX; ++queue)
		{
			if(queues[queue])	queues[queue]->purgeEntries();
		}
	llinfos << "AntiSpam Queues Purged" << llendl;
}
//static
int NACLAntiSpamRegistry::checkGlobalEntry(LLUUID& name, U32 multiplier)
{
	static LLCachedControl<bool> enabled(gSavedSettings,"AntiSpamEnabled",false);
	if(!enabled) return 0;
	it2=globalEntries.find(name.asString());
	if(it2 != globalEntries.end())
	{
		if(it2->second->getBlocked()) return 2;
		U32 eTime=it2->second->getEntryTime();
		U32 currentTime=time(0);
		if((currentTime-eTime) <= globalTime)
		{
			it2->second->updateEntryAmount();
			U32 eAmount=it2->second->getEntryAmount();
			if(eAmount > (globalAmount*multiplier))
				return 1;
			else
				return 0;
		}
		else
		{
			it2->second->clearEntry();
			it2->second->updateEntryAmount();
			it2->second->updateEntryTime();
			return 0;
		}
	}
	else
	{
		globalEntries[name.asString()]=new NACLAntiSpamQueueEntry();
		globalEntries[name.asString()]->updateEntryAmount();
		globalEntries[name.asString()]->updateEntryTime();
		return 0;
	}
}
//static
void NACLAntiSpamRegistry::clearGlobalEntries()
{
	for(it2 = globalEntries.begin(); it2 != globalEntries.end(); it2++)
	{
		it2->second->clearEntry();
	}
}
//static
void NACLAntiSpamRegistry::purgeGlobalEntries()
{
	for(it2 = globalEntries.begin(); it2 != globalEntries.end(); it2++)
	{
		delete it2->second;
		it2->second = 0;
	}
	globalEntries.clear();
}

//Handlers
//static
bool NACLAntiSpamRegistry::handleNaclAntiSpamGlobalQueueChanged(const LLSD& newvalue)
{
    setGlobalQueue(newvalue.asBoolean());
    return true;
}
//static
bool NACLAntiSpamRegistry::handleNaclAntiSpamTimeChanged(const LLSD& newvalue)
{
    setAllQueueTimes(newvalue.asInteger());
    return true;
}
//static
bool NACLAntiSpamRegistry::handleNaclAntiSpamAmountChanged(const LLSD& newvalue)
{
    setAllQueueAmounts(newvalue.asInteger());
    return true;
}

