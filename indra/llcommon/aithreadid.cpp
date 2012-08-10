/**
 * @file aithreadid.cpp
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
 *   - Initial version, written by Aleric Inglewood @ SL
 */

#include "sys.h"
#include <iostream>
#include <iomanip>
#include "aithreadid.h"

AIThreadID const AIThreadID::sNone(AIThreadID::none);
apr_os_thread_t AIThreadID::sMainThreadID;
apr_os_thread_t const AIThreadID::undefinedID = (apr_os_thread_t)-1;
#ifndef LL_DARWIN
apr_os_thread_t CWD_TLS AIThreadID::lCurrentThread;
#endif

void AIThreadID::set_main_thread_id(void)
{
	sMainThreadID = apr_os_thread_current();
}

void AIThreadID::set_current_thread_id(void)
{
#ifndef LL_DARWIN
	lCurrentThread = apr_os_thread_current();
#endif
}

std::ostream& operator<<(std::ostream& os, AIThreadID const& id)
{
	return os << id.mID;
}

std::ostream& operator<<(std::ostream& os, AIThreadID::dout_print_t)
{
	if (!AIThreadID::in_main_thread())
	{
#ifdef LL_DARWIN
		os << std::hex << (size_t)apr_os_thread_current() << std::dec << ' ';
#else
		os << std::hex << (size_t)AIThreadID::lCurrentThread << std::dec << ' ';
#endif
	}
	return os;
}

