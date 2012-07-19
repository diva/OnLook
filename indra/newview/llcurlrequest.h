/**
 * @file llcurlrequest.h
 * @brief Declaration of class Request
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
 *   17/03/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AICURLREQUEST_H
#define AICURLREQUEST_H

#include <string>
#include <vector>
#include <boost/intrusive_ptr.hpp>

// Things defined in this namespace are called from elsewhere in the viewer code.
namespace AICurlInterface {

// Forward declaration.
class Responder;
typedef boost::intrusive_ptr<Responder> ResponderPtr;

class Request {
  public:
	typedef std::vector<std::string> headers_t;
	
	bool get(std::string const& url, ResponderPtr responder);
	bool getByteRange(std::string const& url, headers_t const& headers, S32 offset, S32 length, ResponderPtr responder);
	bool post(std::string const& url, headers_t const& headers, std::string const& data, ResponderPtr responder, S32 time_out = 0);
	bool post(std::string const& url, headers_t const& headers,        LLSD const& data, ResponderPtr responder, S32 time_out = 0);
};

} // namespace AICurlInterface

#endif
