/**
 * @file aisyncclient.h
 * @brief Declaration of AISyncClient.
 *
 * Copyright (c) 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   05/12/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AI_SYNC_CLIENT_H
#define AI_SYNC_CLIENT_H

#ifdef SYNC_TESTSUITE
/*
 * To compile the testsuite, run:
 *
 * cd indra/llcommon
 * g++ -O3 -DCWDEBUG -DSYNC_TESTSUITE -I. -I../cwdebug aisyncclient.cpp -lcwd
 */

#include <stdint.h>
#include <cassert>
typedef uint32_t U32;
typedef uint64_t U64;
#define LL_COMMON_API
#define SHOW_ASSERT
#define ASSERT_ONLY_COMMA(...) , __VA_ARGS__
#define llassert assert

struct LLFrameTimer
{
  double mStartTime;
  double mExpiry;
  static double getCurrentTime(void);
  static U32 sFrameCount;
  static U32 getFrameCount() { return sFrameCount; }
  void reset(double expiration) { mStartTime = getCurrentTime(); mExpiry = mStartTime + expiration; }
  bool hasExpired(void) const { return getCurrentTime() > mExpiry; }
};

#else // !SYNC_TESTSUITE
#include "llpreprocessor.h"
#include "stdtypes.h"
#include "llerror.h"
#include "llframetimer.h"
#endif
#include <list>
#include <boost/intrusive_ptr.hpp>

class AISyncServer;

/*
 * AISyncClient
 *
 * This class allows to synchronize events between multiple client objects,
 * derived from AISyncClient<AISYNCSERVER>, where AISYNCSERVER must be
 * derived from AISyncServer.
 *
 * Client objects call ready(mask) (or ready(event, bool), where each bit can be
 * set or unset, which will result in alternating calls to event1_ready(mask) and
 * event1_not_ready(mask) for all registered clients at the same time depending on
 * when all clients are ready for syncevent1 or not (the other events can be
 * polled, see below).
 *
 * For example, let a and b be two clients:
 *
 * {
 *   AISyncClient<syncgroup_motions, AISyncServer> a;
 *   {
 *     AISyncClient<syncgroup_motions, AISyncServer> b;
 *
 *     a.ready(syncevent1, true);	// Calls a.event1_ready().
 *     a.ready(syncevent1, false);	// Calls a.event1_not_ready().
 *     b.ready(syncevent1, true);	// Nothing happens.
 *     b.ready(syncevent1, false);	// Nothing happens.
 *     b.ready(syncevent1, true);	// Nothing happens.
 *     a.ready(syncevent1, true);	// Calls a.event1_ready() and b.event1_ready(2).
 *     b.ready(syncevent1, false);	// Calls a.event1_not_ready() and b.event1_not_ready().
 *   }								// Calls a.event1_ready() because b was destructed and a is still ready.
 *   a.ready(syncevent1, false);	// Calls a.event1_not_ready(). Must set not-ready before destructing.
 * }
 *
 * Clients can access the server by calling server() and poll it:
 *
 *   AISyncServer* server = server();
 *
 *   if (server)
 *   {
 *     int n = server->number_of_clients();
 *     synceventset set1 = server->events_with_all_clients_ready();
 *     synceventset set2 = server->events_with_zero_clients_ready();
 *     synceventset set3 = server->events_with_at_least_one_client_ready();
 *     if (n == 0)
 *     {
 *       llassert(set1 == syncevents);			// We define a server with no clients to have all clients ready.
 *       llassert(set2 == syncevents);			// If there are no clients then every event has zero ready clients.
 *       llassert(set3 == 0);					// If there are no clients then obviously set3 is the empty set.
 *     }
 *     else
 *     {
 *       llassert((set3 & set1) == set1);		// set1 is a subset of set3.
 *     }
 *     llassert((set3 & (set1 & ~set2)) == (set1 & ~set2));
 *                                              // The set of events that have all clients ready, but not zero clients,
 *                                              // is a subset of the set of events with at least one client ready.
 *     llassert((set2 ^ set3) == syncevents);	// Each event belongs to either set2 or set3.
 *   }
 *
 * Clients can also unregister themselves without destruction, by calling unregister();
 * This might cause a call to event1_ready() for the remaining clients, if all of them
 * are ready for syncevent1.
 *
 * Clients must be set not-ready for all events before unregistering them.
 *
 * Finally, it is possible to cause a server-side unregister for all clients,
 * by calling server->dissolve(). This will cause an immediate call to event1_ready()
 * for all clients that are ready, unless all were already ready.
 */

enum syncgroups
{
#ifdef SYNC_TESTSUITE
  syncgroup_test1,
  syncgroup_test2,
#else
  syncgroup_motions,			// Syncgroup used for animations.
#endif
  syncgroup_size
};

typedef U32 sync_canonical_type;
typedef U64 sync_key_hash_type;

int const sync_canonical_type_bits = sizeof(sync_canonical_type) * 8;
int const sync_number_of_events = 4;
int const sync_bits_per_event_counter = sync_canonical_type_bits / (sync_number_of_events + 1);

// Each type exists of 4 event bytes.
typedef sync_canonical_type syncevent;			// A single event: the least significant bit in one of the bytes.
typedef sync_canonical_type synceventset;		// Zero or more events: the least significant bit in zero or more of the bytes.
typedef sync_canonical_type synccount;			// A count [0, 255] in each of the bytes.
typedef sync_canonical_type synccountmask;		// A single count mask: all eight bits set in one of the bytes.
typedef sync_canonical_type synccountmaskset;	// Zero or more count masks: all eight bits set in zero or more of the bytes.

syncevent const syncevent1 = 1;
syncevent const syncevent2 = syncevent1 << sync_bits_per_event_counter;
syncevent const syncevent3 = syncevent2 << sync_bits_per_event_counter;
syncevent const syncevent4 = syncevent3 << sync_bits_per_event_counter;
sync_canonical_type const syncrefcountunit = syncevent4 << sync_bits_per_event_counter;

synccountmask const synccountmask1 = syncevent2 - 1;
synccountmask const synccountmask2 = synccountmask1 << sync_bits_per_event_counter;
synccountmask const synccountmask3 = synccountmask2 << sync_bits_per_event_counter;
synccountmask const synccountmask4 = synccountmask3 << sync_bits_per_event_counter;
synccountmask const syncrefcountmask = ((synccountmask)-1) & ~(syncrefcountunit - 1);

synceventset const syncevents = syncevent1 | syncevent2 | syncevent3 | syncevent4;
synccountmask const syncoverflowbits = (syncevents << (sync_bits_per_event_counter - 1)) | ~(~(sync_canonical_type)0 >> 1);

// Convert an event set into its corresponding count mask set.
inline synccountmaskset synceventset2countmaskset(synceventset events)
{
  synccountmaskset tmp1 = events << sync_bits_per_event_counter;
  return (tmp1 & ~events) - (events & ~tmp1);
}

// Convert a count mask set into an event set.
inline synceventset synccountmask2eventset(synccountmask mask)
{
  return mask & syncevents;
}

// Interface class used to determined if a client and a server fit together.
class LL_COMMON_API AISyncKey : private LLFrameTimer
{
  private:
	sync_key_hash_type mHash;
	U32 mFrameCount;

  public:
	// A static hash used for clients with no specific requirement.
	static sync_key_hash_type const sNoRequirementHash;

	// A sync key object that returns the sNoRequirementHash.
	static AISyncKey const sNoRequirement;

  public:
	AISyncKey(sync_key_hash_type hash) : mHash(hash) { mFrameCount = getFrameCount(); reset(0.1); }
	virtual ~AISyncKey() { }

	// Derived class should return a hash that is the same for clients which
	// should be synchronized (assuming they meet the time slot requirement as well).
	// The hash does not need to depend on on clock or frame number thus.
	virtual sync_key_hash_type hash(void) const { return sNoRequirementHash; }

	// Return true if this key expired.
	bool expired(void) const { return getFrameCount() > mFrameCount + 1 || hasExpired(); }

	// Return true if both keys are close enough to warrant synchronization.
	bool matches(AISyncKey const& key) const { return key.mHash == mHash; }
};

class LL_COMMON_API AISyncClient_ServerPtr
{
  protected:
	friend class AISyncServer;
	boost::intrusive_ptr<AISyncServer> mSyncServer;
	virtual ~AISyncClient_ServerPtr() { }

	// All-ready event.
	virtual void event1_ready(void) = 0;		// Called when, or after ready(), with the syncevent1 bit set, is called. Never called after a call to ready() with that bit unset.
	// Not all-ready event.
	virtual void event1_not_ready(void) = 0;	// A call to event1_not_ready() follows (at most once) the call to event1_ready(), at some unspecified moment.
	// Only client. Client was forcefully deregistered from expired server because it was the only client.
	virtual void deregistered(void)
	{
#ifdef SHOW_ASSERT
	  mReadyEvents = 0;
#endif
	}

	// Derived classes must return a hash that determines whether or not clients can be synchronized (should be synchronized when registered at the same time).
	virtual sync_key_hash_type sync_key_hash(void) const = 0;

#ifdef SHOW_ASSERT
	AISyncClient_ServerPtr(void) : mReadyEvents(0) { }

  protected:
	synceventset mReadyEvents;					// Used by AISyncServer for debugging only.
#endif
};

// AISyncServer
//
// This class represents a single group of AISyncClient's (a list of pointers to
// their AISyncClient_ServerPtr base class) that need to synchronize events.
//
class LL_COMMON_API AISyncServer
{
  public:
	typedef std::list<AISyncClient_ServerPtr*> client_list_type;

  protected:
	synccount mNrClients;						// Four times (once in each byte) the number of registered clients (the size of mClients).
	synccount mNrReady;							// The number of clients that are ready (four counts, one for each syncevent).
	client_list_type mClients;					// The registered clients.
	AISyncKey mSyncKey;							// The characteristic of clients belonging to this synchronization server.

  public:
	AISyncServer(AISyncKey const& sync_key) : mNrClients(0), mNrReady(0), mSyncKey(sync_key) { }
	virtual ~AISyncServer() { }

	// Default creation of a server for 'syncgroup'.
	template<syncgroups syncgroup>
	static void create_server(boost::intrusive_ptr<AISyncServer>& server, AISyncKey const& sync_key);
	// Default removal of a server for 'syncgroup'.
	template<syncgroups syncgroup>
	static void dispose_server(boost::intrusive_ptr<AISyncServer>& server);

	// Called from AISyncServerCache.
	void setSyncKey(AISyncKey const& sync_key) { mSyncKey = sync_key; }

	// Register client with a server from group syncgroup.
	template<syncgroups syncgroup>
	static void register_client(AISyncClient_ServerPtr* client);
	// Unregister the client (syncgroup must be the same as what was passed to register it).
	void unregister_client(AISyncClient_ServerPtr* client, syncgroups syncgroup);

	// Set readiness of all events at once.
	bool ready(synceventset events, synceventset yesno/*,*/ ASSERT_ONLY_COMMA(AISyncClient_ServerPtr* client));
	// One event became ready or not ready.
	bool ready(syncevent event, bool yesno/*,*/ ASSERT_ONLY_COMMA(AISyncClient_ServerPtr* client)) { return ready(event, yesno ? event : 0/*,*/ ASSERT_ONLY_COMMA(client)); }

	// Returns bitwise OR of syncevent1 through syncevent4 for
	//  events for which all clients are ready.
	synceventset events_with_all_clients_ready(void) const;
	//  events for which at least one client is ready.
	synceventset events_with_at_least_one_client_ready(void) const;
	//  events for which zero clients are ready.
	synceventset events_with_zero_clients_ready(void) const { return syncevents ^ events_with_at_least_one_client_ready(); }

	// Return the number registered clients.
	int number_of_clients(void) const { return (int)(mNrClients & synccountmask1); }
	int get_refcount(void) const { return (int)(mNrClients >> (sync_number_of_events * sync_bits_per_event_counter)); }

  private:
	void trigger_ready(void);		// Calls event1_ready() for all clients.
	void trigger_not_ready(void);	// Calls event1_not_ready for all clients.

	void unregister_last_client(void);

	friend void intrusive_ptr_add_ref(AISyncServer* server);
	friend void intrusive_ptr_release(AISyncServer* server);
};

template<syncgroups syncgroup>
class LL_COMMON_API AISyncClient_SyncGroup : public AISyncClient_ServerPtr
{
  public:
	// Call 'ready' when you are ready (or not) to get a call to start().
	// Returns true if that call was made (immediately), otherwise it may happen later.

	bool ready(synceventset events, synceventset yesno)	// Set readiness of all events at once.
	{
	  if (!mSyncServer)
	  {
		AISyncServer::register_client<syncgroup>(this);
	  }
	  return mSyncServer->ready(events, yesno/*,*/ ASSERT_ONLY_COMMA(this));
	}
	bool ready(syncevent event, bool yesno = true)		// One event became ready or not ready.
	{
	  if (!mSyncServer)
	  {
		AISyncServer::register_client<syncgroup>(this);
	  }
	  return mSyncServer->ready(event, yesno/*,*/ ASSERT_ONLY_COMMA(this));
	}

	void unregister_client(void)
	{
	  if (mSyncServer)
	  {
		mSyncServer->unregister_client(this, syncgroup);
	  }
	}

  protected:
	virtual ~AISyncClient_SyncGroup() { unregister_client(); }
};

template<class AISYNCSERVER, syncgroups syncgroup>
class LL_COMMON_API AISyncClient : public AISyncClient_SyncGroup<syncgroup>
{
  public:
	AISYNCSERVER* server(void) { return static_cast<AISYNCSERVER*>(this->mSyncServer.get()); }
	AISYNCSERVER const* server(void) const { return static_cast<AISYNCSERVER const*>(this->mSyncServer.get()); }

	// By default return a general sync key that will cause synchronizing solely based on time.
	/*virtual*/ AISyncKey const& getSyncKey(void) const { return AISyncKey::sNoRequirement; }
};

template<class AISYNCSERVER, syncgroups syncgroup>
class AISyncServerCache
{
  private:
	// The server cache exists of a few pointers to unused server instances.
	// The cache is empty when sSize == 0 and full when sSize == 16;
	static int const cache_size = 16;
	static boost::intrusive_ptr<AISyncServer> sServerCache[cache_size];
	static int sSize;

  public:
	static void create_server(boost::intrusive_ptr<AISyncServer>& server, AISyncKey const& sync_key)
	{
	  if (sSize == 0)	// Cache empty?
	  {
		server.reset(new AISYNCSERVER(sync_key));
	  }
	  else
	  {
		sServerCache[--sSize].swap(server);
		server->setSyncKey(sync_key);
	  }
	}

	static void dispose_server(boost::intrusive_ptr<AISyncServer>& server)
	{
	  if (sSize < cache_size)
	  {
		sServerCache[sSize++].swap(server);
	  }
	  server.reset();
	}
};

template<class AISYNCSERVER, syncgroups syncgroup>
boost::intrusive_ptr<AISyncServer> AISyncServerCache<AISYNCSERVER, syncgroup>::sServerCache[cache_size];

template<class AISYNCSERVER, syncgroups syncgroup>
int AISyncServerCache<AISYNCSERVER, syncgroup>::sSize;

template<syncgroups syncgroup>
inline void AISyncServer::create_server(boost::intrusive_ptr<AISyncServer>& server, AISyncKey const& sync_key)
{
  AISyncServerCache<AISyncServer, syncgroup>::create_server(server, sync_key);
}

template<syncgroups syncgroup>
inline void AISyncServer::dispose_server(boost::intrusive_ptr<AISyncServer>& server)
{
  AISyncServerCache<AISyncServer, syncgroup>::dispose_server(server);
}

#endif // AI_SYNC_CLIENT_H

