/**
 * @file aihttpheaders.cpp
 * @brief Implementation of AIHTTPHeaders
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
 */

#include "sys.h"
#include "aihttpheaders.h"
#include <curl/curl.h>
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif

AIHTTPHeaders::AIHTTPHeaders(void)
{
}

AIHTTPHeaders::AIHTTPHeaders(std::string const& key, std::string const& value) : mContainer(new Container)
{
  addHeader(key, value);
}

bool AIHTTPHeaders::addHeader(std::string const& key, std::string const& value, op_type op)
{
  if (!mContainer)
  {
	mContainer = new Container;
  }
  insert_t res = mContainer->mKeyValuePairs.insert(container_t::value_type(key, value));
  bool key_already_exists = !res.second;
  if (key_already_exists)
  {
	llassert_always(op != new_header);
	if (op == replace_if_exists)
	  res.first->second = value;
  }
  return key_already_exists;
}

void AIHTTPHeaders::append_to(curl_slist*& slist) const
{
  if (!mContainer)
	return;
  container_t::const_iterator const end = mContainer->mKeyValuePairs.end();
  for (container_t::const_iterator iter = mContainer->mKeyValuePairs.begin(); iter != end; ++iter)
  {
	slist = curl_slist_append(slist, llformat("%s: %s", iter->first.c_str(), iter->second.c_str()).c_str());
  }
}

bool AIHTTPHeaders::hasHeader(std::string const& key) const
{
  return !mContainer ? false : (mContainer->mKeyValuePairs.find(key) != mContainer->mKeyValuePairs.end());
}

bool AIHTTPHeaders::getValue(std::string const& key, std::string& value_out) const
{
  AIHTTPHeaders::container_t::const_iterator iter;
  if (!mContainer || (iter = mContainer->mKeyValuePairs.find(key)) == mContainer->mKeyValuePairs.end())
	return false;
  value_out = iter->second;
  return true;
}

std::ostream& operator<<(std::ostream& os, AIHTTPHeaders const& headers)
{
  os << '{';
  if (headers.mContainer)
  {
	bool first = true;
	AIHTTPHeaders::container_t::const_iterator const end = headers.mContainer->mKeyValuePairs.end();
	for (AIHTTPHeaders::container_t::const_iterator iter = headers.mContainer->mKeyValuePairs.begin(); iter != end; ++iter)
	{
	  if (!first)
		os << ", ";
	  os << '"' << iter->first << ": " << iter->second << '"';
	  first = false;
	}
  }
  os << '}';
  return os;
}

