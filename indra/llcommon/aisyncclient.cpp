/**
 * @file aisyncclient.cpp
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
 *   - Initial version, written by Aleric Inglewood @ SL
 */

#include "sys.h"
#include "aisyncclient.h"
#include <deque>
#include "debug.h"

typedef std::deque<boost::intrusive_ptr<AISyncServer> > servers_type;
static servers_type servers[syncgroup_size];

//static
template<syncgroups syncgroup>
void AISyncServer::register_client(AISyncClient_ServerPtr* client)
{
#ifdef DEBUG_SYNCOUTPUT
  DoutEntering(dc::notice, "AISyncServer::register_client(" << client << ")");
#endif

  // Determine which server to use.
  boost::intrusive_ptr<AISyncServer> server;
  int expired = 0;
  AISyncKey const sync_key(client->sync_key_hash());
  for (servers_type::iterator server_iter = servers[syncgroup].begin(); server_iter != servers[syncgroup].end(); ++server_iter)
  {
	boost::intrusive_ptr<AISyncServer>& server_ptr = *server_iter;
	if (server_ptr->mSyncKey.expired())
	{
	  ++expired;
	  // If the server only contains a single client, then unregister it and put the server (back) in the server cache.
	  if (server_ptr->get_refcount() == 2)
	  {
		server_ptr->unregister_last_client();
		AISyncServer::dispose_server<syncgroup>(server_ptr);
	  }
	  continue;
	}
	if (server_ptr->mSyncKey.matches(sync_key))
	{
	  server = server_ptr;
	  break;
	}
  }
  // Remove servers with expired keys.
  if (expired)
  {
	if (expired == servers[syncgroup].size())
	{
	  servers[syncgroup].clear();
	}
	else
	{
	  servers[syncgroup].erase(servers[syncgroup].begin(), servers[syncgroup].begin() + expired);
	}
  }
  if (!server)
  {
	AISyncServer::create_server<syncgroup>(server, sync_key);
	servers[syncgroup].push_back(server);
  }

  // Sanity check: the client should never already be registered.
  llassert(!client->mSyncServer);
  // Recover from assertion failure.
  if (client->mSyncServer)
  {
	if (client->mSyncServer == server)
	{
	  return;
	}
	client->mSyncServer->unregister_client(client, syncgroup);
	register_client<syncgroup>(client);
	return;
  }

  // Obviously...
  llassert(!client->mReadyEvents);

  // Check if the current clients are all ready: adding a new one will cause the group to become not-ready.
  bool all_old_clients_are_ready = server->mNrReady && !((server->mNrClients - server->mNrReady) & synccountmask1);
  // Add new client to the group.
  client->mSyncServer = server;
  server->mNrClients += syncevents;
  llassert((server->mNrClients & syncoverflowbits) == 0);
  if (all_old_clients_are_ready)
  {
	server->trigger_not_ready();		// Tell all old clients that the group is not ready.
  }
  server->mClients.push_back(client);	// Actually add the new client to the list.
}

void AISyncServer::unregister_client(AISyncClient_ServerPtr* client, syncgroups syncgroup)
{
#ifdef DEBUG_SYNCOUTPUT
  DoutEntering(dc::notice, "unregister_client(" << client << ", " << syncgroup << "), with this = " << this);
#endif

  // The client must be registered with this server.
  llassert(client->mSyncServer == this);
  // A client may only be unregistered after it was marked not-ready for all events.
  llassert(!client->mReadyEvents);
  // Run over all registered clients.
  for (client_list_type::iterator client_iter = mClients.begin(); client_iter != mClients.end(); ++client_iter)
  {
	// Found it?
	if (*client_iter == client)
	{
	  mClients.erase(client_iter);
	  // Are the remaining clients ready?
	  if (mNrReady && !((mNrClients - syncevents - mNrReady) & synccountmask1))
	  {
		trigger_ready();
	  }
	  mNrClients -= syncevents;
	  llassert((mNrClients & syncoverflowbits) == 0);
	  client->mSyncServer.reset();		// This might delete the current object.
	  break;
	}
  }
  // The client must have been found.
  llassert(!client->mSyncServer);
}

void AISyncServer::unregister_last_client(void)
{
#ifdef DEBUG_SYNCOUTPUT
  DoutEntering(dc::notice, "unregister_last_client(), with this = " << this);
#endif

  // This function may only be called for servers with exactly one client.
  llassert(mClients.size() == 1);
  AISyncClient_ServerPtr* client = *mClients.begin();
#ifdef DEBUG_SYNCOUTPUT
  Dout(dc::notice, "unregistering client " << client);
#endif
  mClients.clear();
  mNrClients -= syncevents;
  mNrReady = 0;
  llassert((mNrClients & syncoverflowbits) == 0);
  client->mSyncServer.reset();
  client->deregistered();
}

synceventset AISyncServer::events_with_all_clients_ready(void) const
{
  synccount nrNotReady = mNrClients - mNrReady;
  synceventset result1 = !(nrNotReady & synccountmask1) ? syncevent1 : 0;
  synceventset result2 = !(nrNotReady & synccountmask2) ? syncevent2 : 0;
  synceventset result3 = !(nrNotReady & synccountmask3) ? syncevent3 : 0;
  synceventset result4 = !(nrNotReady & synccountmask4) ? syncevent4 : 0;
  result1 |= result2;
  result3 |= result4;
  return result1 | result3;
}

synceventset AISyncServer::events_with_at_least_one_client_ready(void) const
{
  synceventset result1 = (mNrReady & synccountmask1) ? syncevent1 : 0;
  synceventset result2 = (mNrReady & synccountmask2) ? syncevent2 : 0;
  synceventset result3 = (mNrReady & synccountmask3) ? syncevent3 : 0;
  synceventset result4 = (mNrReady & synccountmask4) ? syncevent4 : 0;
  result1 |= result2;
  result3 |= result4;
  return result1 | result3;
}

#ifdef CWDEBUG
struct SyncEventSet {
  synceventset mBits;
  SyncEventSet(synceventset bits) : mBits(bits) { }
};

std::ostream& operator<<(std::ostream& os, SyncEventSet const& ses)
{
  os << ((ses.mBits & syncevent4) ? '1' : '0');
  os << ((ses.mBits & syncevent3) ? '1' : '0');
  os << ((ses.mBits & syncevent2) ? '1' : '0');
  os << ((ses.mBits & syncevent1) ? '1' : '0');
  return os;
}
#endif

bool AISyncServer::ready(synceventset events, synceventset yesno/*,*/ ASSERT_ONLY_COMMA(AISyncClient_ServerPtr* client))
{
#ifdef DEBUG_SYNCOUTPUT
  DoutEntering(dc::notice, "AISyncServer::ready(" << SyncEventSet(events) << ", " << SyncEventSet(yesno) << ", " << client << ")");
#endif

  synceventset added_events = events & yesno;
  synceventset removed_events = events & ~yesno;
  // May not add events that are already ready.
  llassert(!(client->mReadyEvents & added_events));
  // Cannot remove events that weren't ready.
  llassert((client->mReadyEvents & removed_events) == removed_events);
  // Were all clients ready for event 1?
  bool ready_before = !((mNrClients - mNrReady) & synccountmask1);
  // Update mNrReady counters.
  mNrReady += added_events;
  mNrReady -= removed_events;
  // Test for under and overflow, this limits the maximum number of clients to 127 instead of 255, but well :p.
  llassert((mNrReady & syncoverflowbits) == 0);
  // Are all clients ready for event 1?
  bool ready_after = !((mNrClients - mNrReady) & synccountmask1);
  if (ready_before && !ready_after)
  {
	trigger_not_ready();
  }
#ifdef SHOW_ASSERT
  // Update debug administration.
  client->mReadyEvents ^= events;
#ifdef DEBUG_SYNCOUTPUT
  Dout(dc::notice, "Client " << client << " now has ready: " << SyncEventSet(client->mReadyEvents));
#endif
#endif
  if (!ready_before && ready_after)
  {
	trigger_ready();
  }
}

void AISyncServer::trigger_ready(void)
{
  for (client_list_type::iterator client_iter = mClients.begin(); client_iter != mClients.end(); ++client_iter)
  {
	llassert(((*client_iter)->mReadyEvents & syncevent1));
	(*client_iter)->event1_ready();
  }
}

void AISyncServer::trigger_not_ready(void)
{
  for (client_list_type::iterator client_iter = mClients.begin(); client_iter != mClients.end(); ++client_iter)
  {
	llassert(((*client_iter)->mReadyEvents & syncevent1));
	(*client_iter)->event1_not_ready();
  }
}

void intrusive_ptr_add_ref(AISyncServer* server)
{
  server->mNrClients += syncrefcountunit;
  llassert((server->mNrClients & syncoverflowbits) == 0);
}

void intrusive_ptr_release(AISyncServer* server)
{
  llassert(server->mNrClients >= syncrefcountunit);
  server->mNrClients -= syncrefcountunit;
  // If there are no more pointers pointing to this server, then obviously it can't have any registered clients.
  llassert(server->mNrClients >= syncrefcountunit || server->mNrClients == 0);
  if (server->mNrClients == 0)
  {
	delete server;
  }
}

//static
sync_key_hash_type const AISyncKey::sNoRequirementHash = 0x91b42f98a9fef15cULL;


//=============================================================================
//                             SYNC_TESTSUITE
//=============================================================================

#ifdef SYNC_TESTSUITE
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <boost/io/ios_state.hpp>

//static
U32 LLFrameTimer::sFrameCount;

double innerloop_count = 0;

double LLFrameTimer::getCurrentTime()
{
  return innerloop_count * 0.001;
}

class TestsuiteServerBase : public AISyncServer
{
  public:
	TestsuiteServerBase(AISyncKey const& sync_key) : AISyncServer(sync_key) { }
	client_list_type& clients(void) { return mClients; }
};

class TestsuiteServer1 : public TestsuiteServerBase
{
  public:
	TestsuiteServer1(AISyncKey const& sync_key) : TestsuiteServerBase(sync_key) { }
};

// Specialitations that link TestsuiteServer1 to syncgroup_test1.
//
//static
template<>
void AISyncServer::create_server<syncgroup_test1>(boost::intrusive_ptr<AISyncServer>& server, AISyncKey const& sync_key)
{
  AISyncServerCache<TestsuiteServer1, syncgroup_test1>::create_server(server, sync_key);
}
//static
template<>
void AISyncServer::dispose_server<syncgroup_test1>(boost::intrusive_ptr<AISyncServer>& server)
{
  AISyncServerCache<TestsuiteServer1, syncgroup_test1>::dispose_server(server);
}

class TestsuiteServer2 : public TestsuiteServerBase
{
  public:
	TestsuiteServer2(AISyncKey const& sync_key) : TestsuiteServerBase(sync_key) { }
};

// Specialitations that link TestsuiteServer2 to syncgroup_test2.
//
//static
template<>
void AISyncServer::create_server<syncgroup_test2>(boost::intrusive_ptr<AISyncServer>& server, AISyncKey const& sync_key)
{
  AISyncServerCache<TestsuiteServer2, syncgroup_test2>::create_server(server, sync_key);
}
//static
template<>
void AISyncServer::dispose_server<syncgroup_test2>(boost::intrusive_ptr<AISyncServer>& server)
{
  AISyncServerCache<TestsuiteServer2, syncgroup_test2>::dispose_server(server);
}

template<syncgroups syncgroup>
class TestsuiteClient : public AISyncClient<TestsuiteServerBase, syncgroup>
{
  private:
	int mIndex;
	bool mRequestedRegistered;
	synceventset mRequestedReady;
	bool mActualReady1;

  public:
	TestsuiteClient() : mIndex(-1), mRequestedRegistered(false), mRequestedReady(0), mActualReady1(false) { }
	~TestsuiteClient() { this->ready(mRequestedReady, (synceventset)0); }

	void setIndex(int index) { mIndex = index; }

  protected:
	/*virtual*/ void event1_ready(void)
	{
	  llassert(!mActualReady1);
	  mActualReady1 = true;
	}

	/*virtual*/ void event1_not_ready(void)
	{
	  llassert(mActualReady1);
	  mActualReady1 = false;
	}

	/*virtual*/ sync_key_hash_type sync_key_hash(void) const
	{
	  // Sync odd clients with eachother, and even clients with eachother.
	  return 0xb3c919ff + (mIndex & 1);
	}

	// This is called when the server expired and we're the only client on it.
	/*virtual*/ void deregistered(void)
	{
#ifdef DEBUG_SYNCOUTPUT
	  DoutEntering(dc::notice, "TestsuiteClient<" << syncgroup << ">::deregistered(), with this = " << this);
#endif
	  mRequestedRegistered = false;
	  mRequestedReady = 0;
	  mActualReady1 = false;
	  this->mReadyEvents = 0;
	}

  private:
	bool is_registered(void) const { return !!this->mSyncServer; }

  public:
	void change_state(unsigned long r);
	bool getRequestedRegistered(void) const { return mRequestedRegistered; }
	synceventset getRequestedReady(void) const { return mRequestedReady; }
};

TestsuiteClient<syncgroup_test1>* client1p;
TestsuiteClient<syncgroup_test2>* client2p;

int const number_of_clients_per_syncgroup = 8;

template<syncgroups syncgroup>
void TestsuiteClient<syncgroup>::change_state(unsigned long r)
{
  bool change_registered = r & 1;
  r >>= 1;
  synceventset toggle_events = ((r & 1) ? syncevent1 : 0) | ((r & 2) ? syncevent2 : 0) | ((r & 4) ? syncevent3 : 0) | ((r & 8) ? syncevent4 : 0);
  r >>= 4;
  if (change_registered)
  {
	if (mRequestedRegistered && !mRequestedReady)
	{
	  mRequestedRegistered = false;
	  this->unregister_client();
	}
  }
  else if (toggle_events)
  {
	mRequestedReady ^= toggle_events;
	mRequestedRegistered = true;
	this->ready(toggle_events, mRequestedReady & toggle_events);
  }
  llassert(mRequestedRegistered == is_registered());
  TestsuiteServerBase* server = this->server();
  llassert(!mRequestedRegistered || server->number_of_clients() == server->clients().size());
  if (mRequestedRegistered)
  {
	synceventset all_ready = syncevents;
	synceventset any_ready = 0;
	int nr = 0;
	for (int cl = 0; cl < number_of_clients_per_syncgroup; ++cl)
	{
	  if (syncgroup == syncgroup_test1)
	  {
		if (client1p[cl].server() != server)
		{
		  continue;
		}
		if (client1p[cl].getRequestedRegistered())
		{
		  ++nr;
		  all_ready &= client1p[cl].getRequestedReady();
		  any_ready |= client1p[cl].getRequestedReady();
		}
	  }
	  else
	  {
		if (client2p[cl].server() != server)
		{
		  continue;
		}
		if (client2p[cl].getRequestedRegistered())
		{
		  ++nr;
		  all_ready &= client2p[cl].getRequestedReady();
		  any_ready |= client2p[cl].getRequestedReady();
		}
	  }
	}
	llassert(nr == server->number_of_clients());
	llassert(!!(all_ready & syncevent1) == mActualReady1);
	llassert(this->server()->events_with_all_clients_ready() == all_ready);
	llassert(this->server()->events_with_at_least_one_client_ready() == any_ready);
	llassert(nr == 0 || (any_ready & all_ready) == all_ready);
  }
  llassert(mRequestedReady == this->mReadyEvents);
}

int main()
{
  Debug(libcw_do.on());
  Debug(dc::notice.on());
  Debug(libcw_do.set_ostream(&std::cout));
  Debug(list_channels_on(libcw_do));

  unsigned short seed16v[3] = { 0x1234, 0xfedc, 0x7091 };

  for (int k = 0;; ++k)
  {
	std::cout << "Loop: " << k << "; SEED: " << std::hex << seed16v[0] << ", " << seed16v[1] << ", " << seed16v[2] << std::dec << std::endl;
	++LLFrameTimer::sFrameCount;

	seed48(seed16v);
	seed16v[0] = lrand48() & 0xffff;
	seed16v[1] = lrand48() & 0xffff;
	seed16v[2] = lrand48() & 0xffff;

	TestsuiteClient<syncgroup_test1> client1[number_of_clients_per_syncgroup];
	TestsuiteClient<syncgroup_test2> client2[number_of_clients_per_syncgroup];
	client1p = client1;
	client2p = client2;

	for (int i = 0; i < number_of_clients_per_syncgroup; ++i)
	{
	  client1[i].setIndex(i);
	  client2[i].setIndex(i);
	}

	for (int j = 0; j < 1000000; ++j)
	{
	  innerloop_count += 1;

#ifdef DEBUG_SYNCOUTPUT
	  Dout(dc::notice, "Innerloop: " << j);
#endif
	  unsigned long r = lrand48();
	  syncgroups grp = (r & 1) ? syncgroup_test1 : syncgroup_test2;
	  r >>= 1;
	  int cl = (r & 255) % number_of_clients_per_syncgroup;
	  r >>= 8;
	  switch (grp)
	  {
		case syncgroup_test1:
		  client1[cl].change_state(r);
		  break;
		case syncgroup_test2:
		  client2[cl].change_state(r);
		  break;
	  }
	}
  }
}

#endif
