/**
 * @file aihttpheaders.h
 * @brief Keep a list of HTTP headers.
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
 *   15/08/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *   21/10/2012
 *   Added AIHTTPReceivedHeaders
 */

#ifndef AIHTTPHEADERS_H
#define AIHTTPHEADERS_H

#include <string>
#include <map>
#include <iosfwd>
#include <algorithm>
#include "llpointer.h"
#include "llthread.h"			// LLThreadSafeRefCount

extern "C" struct curl_slist;

class AIHTTPHeaders {
  public:
	enum op_type
	{
	  new_header,				// The inserted header must be the first one.
	  replace_if_exists,		// If a header of this type already exists, replace it. Otherwise add the header.
	  keep_existing_header		// If a header of this type already exists, do nothing.
	};

	// Construct an empty container.
	AIHTTPHeaders(void) { }

	// Construct a container with a single header.
	AIHTTPHeaders(std::string const& key, std::string const& value);

	// Clear all headers.
	void clear(void) { if (mContainer) mContainer->mKeyValuePairs.clear(); }

	// Add a header. Returns true if the header already existed.
	bool addHeader(std::string const& key, std::string const& value, op_type op = new_header);

	// Return true if there are no headers associated with this object.
	bool empty(void) const { return !mContainer || mContainer->mKeyValuePairs.empty(); }

	// Return true if the header already exists.
	bool hasHeader(std::string const& key) const;

	// Return true if key exists and fill value_out with the value. Return false otherwise.
	bool getValue(std::string const& key, std::string& value_out) const;

	// Append the headers to slist.
	void append_to(curl_slist*& slist) const;

	// For debug purposes.
	friend std::ostream& operator<<(std::ostream& os, AIHTTPHeaders const& headers);

  private:
	typedef std::map<std::string, std::string> container_t;
	typedef std::pair<container_t::iterator, bool> insert_t;

	struct Container : public LLThreadSafeRefCount {
	  container_t mKeyValuePairs;
	};

	LLPointer<Container> mContainer;
};

// Functor that returns true if c1 is less than c2, ignoring bit 5.
// The effect is that characters in the range a-z equivalent ordering with A-Z.
// This function assumes UTF-8 or ASCII encoding!
//
// Note that other characters aren't important in the case of HTTP header keys;
// however if one considers all printable ASCII characters, then this functor
// also compares "@[\]^" equal to "`{|}~" (any other is either not printable or
// would be equal to a not printable character).
struct AIHTTPReceivedHeadersCharCompare {
  bool operator()(std::string::value_type c1, std::string::value_type c2) const
  {
	static std::string::value_type const bit5 = 0x20;
	return (c1 | bit5) < (c2 | bit5);
  }
};

// Functor to lexiographically compare two HTTP header keys using the above predicate.
// This means that for example "Content-Type" and "content-type" will have equivalent ordering.
struct AIHTTPReceivedHeadersCompare {
  bool operator()(std::string const& h1, std::string const& h2) const
  {
	static AIHTTPReceivedHeadersCharCompare predicate;
	return std::lexicographical_compare(h1.begin(), h1.end(), h2.begin(), h2.end(), predicate);
  }
};

class AIHTTPReceivedHeaders {
  private:
	typedef std::multimap<std::string, std::string, AIHTTPReceivedHeadersCompare> container_t;

  public:
	typedef container_t::const_iterator iterator_type;
	typedef std::pair<iterator_type, iterator_type> range_type;

	// Construct an empty container.
	AIHTTPReceivedHeaders(void) { }

	// Clear all headers.
	void clear(void) { if (mContainer) mContainer->mKeyValuePairs.clear(); }

	// Add a header.
	void addHeader(std::string const& key, std::string const& value);

	// Swap headers with another container.
	void swap(AIHTTPReceivedHeaders& headers) { LLPointer<Container>::swap(mContainer, headers.mContainer); }

	// Return true if there are no headers associated with this object.
	bool empty(void) const { return !mContainer || mContainer->mKeyValuePairs.empty(); }

	// Return true if the header exists.
	bool hasHeader(std::string const& key) const;

	// Return true if key exists and fill value_out with the value. Return false otherwise.
	bool getFirstValue(std::string const& key, std::string& value_out) const;

	// Return true if key exists and fill value_out with all values. Return false otherwise.
	bool getValues(std::string const& key, range_type& value_out) const;

	// For debug purposes.
	friend std::ostream& operator<<(std::ostream& os, AIHTTPReceivedHeaders const& headers);

	// Returns true if the two keys compare equal (ie, "Set-Cookie" == "set-cookie").
	static bool equal(std::string const& key1, std::string const& key2);

  private:
	struct Container : public LLThreadSafeRefCount {
	  container_t mKeyValuePairs;
	};

	LLPointer<Container> mContainer;
};

#endif // AIHTTPHEADERS_H
