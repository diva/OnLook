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

#ifndef NACLANTISPAM_H
#define NACLANTISPAM_H
#include <boost/tr1/unordered_map.hpp>
#include "stdtypes.h"
#include "lluuid.h"
class NACLAntiSpamQueueEntry
{
	friend class NACLAntiSpamQueue;
	friend class NACLAntiSpamRegistry;
protected:
	NACLAntiSpamQueueEntry();
	void clearEntry();
	U32 getEntryAmount() const;
	U32 getEntryTime() const;
	void updateEntryAmount();
	void updateEntryTime();
	bool getBlocked() const;
	void setBlocked();
private:
	U32 entryAmount;
	U32 entryTime;
	bool blocked;
};
class NACLAntiSpamQueue
{
	friend class NACLAntiSpamRegistry;
public:
	U32 getAmount() const;
	U32 getTime() const;
protected:
	NACLAntiSpamQueue(U32 time, U32 amount);
	void setAmount(U32 amount);
	void setTime(U32 time);
	void clearEntries();
	void purgeEntries();
	void blockEntry(LLUUID& source);
	int checkEntry(LLUUID& source, U32 multiplier);
private:
	std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*> entries;
	std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*>::iterator it;
	U32 queueAmount;
	U32 queueTime;
};
class NACLAntiSpamRegistry
{
public:
	NACLAntiSpamRegistry(U32 time=2, U32 amount=10);
	static void registerQueues(U32 time=2, U32 amount=10);
//	static void registerQueue(U32 name, U32 time, U32 amount);
	static void setRegisteredQueueTime(U32 name, U32 time);
	static void setRegisteredQueueAmount(U32 name,U32 amount);
	static void setAllQueueTimes(U32 amount);
	static void setAllQueueAmounts(U32 time);
	static bool checkQueue(U32 name, LLUUID& source, U32 multiplier=1);
	static bool handleNaclAntiSpamGlobalQueueChanged(const LLSD& newvalue);
	static bool handleNaclAntiSpamTimeChanged(const LLSD& newvalue);
	static bool handleNaclAntiSpamAmountChanged(const LLSD& newvalue);
	static void clearRegisteredQueue(U32 name);
	static void purgeRegisteredQueue(U32 name);
	static void clearAllQueues();
	static void purgeAllQueues();
	static void setGlobalQueue(bool value);
	static void setGlobalAmount(U32 amount);
	static void setGlobalTime(U32 time);
	static void blockOnQueue(U32 name,LLUUID& owner_id);
	enum {
		QUEUE_CHAT,
		QUEUE_INVENTORY,
		QUEUE_IM,
		QUEUE_CALLING_CARD,
		QUEUE_SOUND,
		QUEUE_SOUND_PRELOAD,
		QUEUE_SCRIPT_DIALOG,
		QUEUE_TELEPORT,
		QUEUE_MAX
	};
private:
	static const char* getQueueName(U32 queue_id);
	static NACLAntiSpamQueue* queues[QUEUE_MAX];
	static std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*> globalEntries;
	static std::tr1::unordered_map<std::string,NACLAntiSpamQueueEntry*>::iterator it2;
	static U32 globalTime;
	static U32 globalAmount;
	static bool bGlobalQueue;

	static int checkGlobalEntry(LLUUID& source, U32 multiplier);
	static void clearGlobalEntries();
	static void purgeGlobalEntries();
	static void blockGlobalEntry(LLUUID& source);
};

extern const std::string COLLISION_SOUNDS[];
extern const int COLLISION_SOUNDS_SIZE;

#endif //NACLANTISPAM_H
