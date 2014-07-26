/**
 * @file aihttptimeoutpolicy.h
 * @brief Store the policy on timing out a HTTP curl transaction.
 *
 * Copyright (c) 2012, Aleric Inglewood.
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
 *   24/09/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AIHTTPTIMEOUTPOLICY_H
#define AIHTTPTIMEOUTPOLICY_H

#include "stdtypes.h"
#include <string>
#include <map>

class AIHTTPTimeoutPolicyBase;

namespace AIHTTPTimeoutPolicyOperators {

struct DNS;
struct Connect;
struct Reply;
struct Speed;
struct Transaction;
struct Total;

} // namespace AIHTTPTimeoutPolicyOperators

class AIHTTPTimeoutPolicy {
  protected:
	char const* const mName;				// The name of this policy, for debugging purposes.
	AIHTTPTimeoutPolicyBase* const mBase;	// Policy this policy was based on.
	static AIHTTPTimeoutPolicyBase sDebugSettingsCurlTimeout;	// CurlTimeout* debug settings.
	typedef std::map<std::string, AIHTTPTimeoutPolicy*> namemap_t;	// Type of sNameMap.
	static namemap_t sNameMap;				// Map of name of timeout policies (as returned by name()) to AIHTTPTimeoutPolicy* (this).

  private:
	U16 mDNSLookupGrace;			// Extra connect timeout the first time we connect to a host (this is to allow for DNS lookups).
	U16 mMaximumConnectTime;		// Connect timeouts any subsequent connects to the same host, assuming the DNS will be cached now.
	U16 mMaximumReplyDelay;			// Timeout for the period between sending data to the server and the HTTP header of the reply.
	U16 mLowSpeedTime;				// The time in seconds that a transfer should be below mLowSpeedLimit before to consider it too slow and abort.
	U32 mLowSpeedLimit;				// Transfer speed in bytes per second that a transfer should be below during mLowSpeedTime seconds to consider it too slow and abort.
	U16 mMaximumCurlTransaction;	// Timeout for the whole curl transaction (including connect and DNS lookup).
	U16 mMaximumTotalDelay;			// Timeout from moment of request (including the time a request is/was queued).

	friend struct AIHTTPTimeoutPolicyOperators::DNS;
	friend struct AIHTTPTimeoutPolicyOperators::Connect;
	friend struct AIHTTPTimeoutPolicyOperators::Reply;
	friend struct AIHTTPTimeoutPolicyOperators::Speed;
	friend struct AIHTTPTimeoutPolicyOperators::Transaction;
	friend struct AIHTTPTimeoutPolicyOperators::Total;

 public:
	// Construct a HTTP timeout policy object that mimics base, or Debug Settings if none given.
	AIHTTPTimeoutPolicy(
		char const* name,
		AIHTTPTimeoutPolicyBase& base = sDebugSettingsCurlTimeout);

	// Construct a HTTP timeout policy with exact specifications.
	AIHTTPTimeoutPolicy(
		char const* name,
		U16 dns_lookup_grace,
		U16 subsequent_connects,
		U16 reply_delay,
		U16 low_speed_time,
		U32 low_speed_limit,
		U16 curl_transaction,
		U16 total_delay);

	// Destructor.
	virtual ~AIHTTPTimeoutPolicy() { }

	void sanity_checks(void) const;

	// Accessors.
	char const* name(void) const { return mName ? mName : "AIHTTPTimeoutPolicyBase"; }
	U16 getConnectTimeout(std::string const& hostname) const;
	U16 getDNSLookup(void) const { return mDNSLookupGrace; }
	U16 getConnect(void) const { return mMaximumConnectTime; }
	U16 getReplyDelay(void) const { return mMaximumReplyDelay; }
	U16 getLowSpeedTime(void) const { return mLowSpeedTime; }
	U32 getLowSpeedLimit(void) const { return mLowSpeedLimit; }
	U16 getCurlTransaction(void) const { return mMaximumCurlTransaction; }
	U16 getTotalDelay(void) const { return mMaximumTotalDelay; }
	static AIHTTPTimeoutPolicy const& getDebugSettingsCurlTimeout(void);
	static AIHTTPTimeoutPolicy const* getTimeoutPolicyByName(std::string const& name);

	// Called once at start up of viewer to set a different default timeout policy than HTTPTimeoutPolicy_default.
	static void setDefaultCurlTimeout(AIHTTPTimeoutPolicy const& defaultCurlTimeout);

	// Called when a connect to a hostname timed out.
	static bool connect_timed_out(std::string const& hostname);

	// Called when the base that this policy was based on changed.
	virtual void base_changed(void);

	// Called when we ourselves changed.
	virtual void changed(void);

  protected:
	// Used by AIHTTPTimeoutPolicyBase::AIHTTPTimeoutPolicyBase(AIHTTPTimeoutPolicyBase&).
	AIHTTPTimeoutPolicy(AIHTTPTimeoutPolicy&);
	// Abused assigned operator (called by AIHTTPTimeoutPolicyBase::operator=).
	AIHTTPTimeoutPolicy& operator=(AIHTTPTimeoutPolicy const&);
};

class LLSD;

// Handlers for Debug Setting changes.
bool validateCurlTimeoutDNSLookup(LLSD const& newvalue);
bool handleCurlTimeoutDNSLookup(LLSD const& newvalue);
bool validateCurlTimeoutConnect(LLSD const& newvalue);
bool handleCurlTimeoutConnect(LLSD const& newvalue);
bool validateCurlTimeoutReplyDelay(LLSD const& newvalue);
bool handleCurlTimeoutReplyDelay(LLSD const& newvalue);
bool validateCurlTimeoutLowSpeedLimit(LLSD const& newvalue);
bool handleCurlTimeoutLowSpeedLimit(LLSD const& newvalue);
bool validateCurlTimeoutLowSpeedTime(LLSD const& newvalue);
bool handleCurlTimeoutLowSpeedTime(LLSD const& newvalue);
bool validateCurlTimeoutMaxTransaction(LLSD const& newvalue);
bool handleCurlTimeoutMaxTransaction(LLSD const& newvalue);
bool validateCurlTimeoutMaxTotalDelay(LLSD const& newvalue);
bool handleCurlTimeoutMaxTotalDelay(LLSD const& newvalue);

#endif // AIHTTPTIMEOUTPOLICY_H
