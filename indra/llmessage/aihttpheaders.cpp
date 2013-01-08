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

void AIHTTPReceivedHeaders::addHeader(std::string const& key, std::string const& value)
{
  if (!mContainer)
  {
	mContainer = new Container;
  }
  else if (equal(key, "set-cookie"))
  {
	// If a cookie with this name already exists, replace it.
	std::string const name = value.substr(0, value.find('='));
	container_t::iterator const end = mContainer->mKeyValuePairs.end();
	for (container_t::iterator header = mContainer->mKeyValuePairs.begin(); header != end; ++header)
	{
	  if (equal(header->first, "set-cookie") && header->second.substr(0, header->second.find('=')) == name)
	  {
		header->second = value;
		return;
	  }
	}
  }
  mContainer->mKeyValuePairs.insert(container_t::value_type(key, value));
}

bool AIHTTPReceivedHeaders::hasHeader(std::string const& key) const
{
  return !mContainer ? false : (mContainer->mKeyValuePairs.find(key) != mContainer->mKeyValuePairs.end());
}

bool AIHTTPReceivedHeaders::getFirstValue(std::string const& key, std::string& value_out) const
{
  AIHTTPReceivedHeaders::container_t::const_iterator iter;
  if (!mContainer || (iter = mContainer->mKeyValuePairs.find(key)) == mContainer->mKeyValuePairs.end())
	return false;
  value_out = iter->second;
  return true;
}

bool AIHTTPReceivedHeaders::getValues(std::string const& key, range_type& value_out) const
{
  if (!mContainer)
	return false;
  value_out = mContainer->mKeyValuePairs.equal_range(key);
  return value_out.first != value_out.second;
}

std::ostream& operator<<(std::ostream& os, AIHTTPReceivedHeaders const& headers)
{
  os << '{';
  if (headers.mContainer)
  {
	bool first = true;
	AIHTTPReceivedHeaders::container_t::const_iterator const end = headers.mContainer->mKeyValuePairs.end();
	for (AIHTTPReceivedHeaders::container_t::const_iterator iter = headers.mContainer->mKeyValuePairs.begin(); iter != end; ++iter)
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

//static
bool AIHTTPReceivedHeaders::equal(std::string const& key1, std::string const& key2)
{
  if (key1.length() != key2.length())
  {
	return false;
  }
  for (std::string::const_iterator i1 = key1.begin(), i2 = key2.begin(); i1 != key1.end(); ++i1, ++i2)
  {
	if (((*i1 ^ *i2) & 0xdf) != 0)
	{
	  return false;
	}
  }
  return true;
}

