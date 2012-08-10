/**
 * @file aithreadid.h
 * @brief Declaration of AIThreadID.
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
 *   08/08/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AI_THREAD_ID
#define AI_THREAD_ID

#include <apr_portable.h>	// apr_os_thread_t, apr_os_thread_current(), apr_os_thread_equal().
#include <iosfwd>			// std::ostream.
#include "llpreprocessor.h"	// LL_COMMON_API, LL_COMMON_API_TLS
#include "llerror.h"

// Lightweight wrapper around apr_os_thread_t.
// This class introduces no extra assembly code after optimization; it's only intend is to provide type-safety.
class AIThreadID
{
private:
	apr_os_thread_t mID;
	static LL_COMMON_API apr_os_thread_t sMainThreadID;
	static LL_COMMON_API apr_os_thread_t const undefinedID;
#ifndef LL_DARWIN
	static LL_COMMON_API_TLS apr_os_thread_t lCurrentThread;
#endif
public:
	static LL_COMMON_API AIThreadID const sNone;
	enum undefined_thread_t { none };
	enum dout_print_t { DoutPrint };

public:
	AIThreadID(void) : mID(apr_os_thread_current()) { }
	explicit AIThreadID(undefined_thread_t) : mID(undefinedID) { }		// Used for sNone.
	AIThreadID(AIThreadID const& id) : mID(id.mID) { }
	AIThreadID& operator=(AIThreadID const& id) { mID = id.mID; return *this; }
	bool is_main_thread(void) const { return apr_os_thread_equal(mID, sMainThreadID); }
	bool is_no_thread(void) const { return apr_os_thread_equal(mID, sNone.mID); }
	friend LL_COMMON_API bool operator==(AIThreadID const& id1, AIThreadID const& id2) { return apr_os_thread_equal(id1.mID, id2.mID); }
	friend LL_COMMON_API bool operator!=(AIThreadID const& id1, AIThreadID const& id2) { return !apr_os_thread_equal(id1.mID, id2.mID); }
	friend LL_COMMON_API std::ostream& operator<<(std::ostream& os, AIThreadID const& id);
	friend LL_COMMON_API std::ostream& operator<<(std::ostream& os, dout_print_t);
	static void set_main_thread_id(void);					// Called once to set sMainThreadID.
	static void set_current_thread_id(void);				// Called once for every thread to set lCurrentThread.
#ifdef LL_DARWIN
	void reset(void) { mID = apr_os_thread_current(); }
	bool equals_current_thread(void) const { return apr_os_thread_equal(mID, apr_os_thread_current()); }
	static bool in_main_thread(void) { return apr_os_thread_equal(apr_os_thread_current(), sMainThreadID); }
#else
	void reset(void) { mID = lCurrentThread; }
	bool equals_current_thread(void) const { return apr_os_thread_equal(mID, lCurrentThread); }
	static bool in_main_thread(void) { return apr_os_thread_equal(lCurrentThread, sMainThreadID); }
#endif
};

// Legacy function.
inline bool is_main_thread(void)
{
  return AIThreadID::in_main_thread();
}

#endif // AI_THREAD_ID
