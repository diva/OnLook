/**
 * @file llnotificationsutil.cpp
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"

#include "llnotificationsutil.h"

#include "llnotifications.h"
#include "llsd.h"
#include "llxmlnode.h"	// apparently needed to call LLNotifications::instance()

namespace AIAlert
{

LLNotificationPtr add(Error const& error, modal_nt type, unsigned int suppress_mask)
{
	return LLNotifications::instance().add(error, type, suppress_mask);
}

LLNotificationPtr add(std::string const& xml_desc, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, xml_desc, AIArgs()), type, 0);
}

LLNotificationPtr add(std::string const& xml_desc, AIArgs const& args, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, xml_desc, args), type, 0);
}

LLNotificationPtr add(Error const& error, std::string const& xml_desc, unsigned int suppress_mask, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, error, xml_desc, AIArgs()), type, suppress_mask);
}

LLNotificationPtr add(Error const& error, std::string const& xml_desc, AIArgs const& args, unsigned int suppress_mask, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, error, xml_desc, args), type, suppress_mask);
}

LLNotificationPtr add(std::string const& xml_desc, Error const& error, unsigned int suppress_mask, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, xml_desc, AIArgs(), error), type, suppress_mask);
}

LLNotificationPtr add(std::string const& xml_desc, AIArgs const& args, Error const& error, unsigned int suppress_mask, modal_nt type)
{
	return LLNotifications::instance().add(Error(Prefix(), type, xml_desc, args, error), type, suppress_mask);
}

} // namespace AIAlert

LLNotificationPtr LLNotificationsUtil::add(const std::string& name)
{
	return LLNotifications::instance().add(
		LLNotification::Params(name).substitutions(LLSD()).payload(LLSD()));	
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, 
					  const LLSD& substitutions)
{
	return LLNotifications::instance().add(
		LLNotification::Params(name).substitutions(substitutions).payload(LLSD()));		
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, 
					  const LLSD& substitutions, 
					  const LLSD& payload)
{
	return LLNotifications::instance().add(
		LLNotification::Params(name).substitutions(substitutions).payload(payload));	
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, 
					  const LLSD& substitutions, 
					  const LLSD& payload, 
					  const std::string& functor_name)
{
	return LLNotifications::instance().add(
		LLNotification::Params(name).substitutions(substitutions).payload(payload).functor_name(functor_name));	
}

LLNotificationPtr LLNotificationsUtil::add(const std::string& name, 
					  const LLSD& substitutions, 
					  const LLSD& payload, 
					  boost::function<void (const LLSD&, const LLSD&)> functor)
{
	return LLNotifications::instance().add(
		LLNotification::Params(name).substitutions(substitutions).payload(payload).functor(functor));	
}

S32 LLNotificationsUtil::getSelectedOption(const LLSD& notification, const LLSD& response)
{
	return LLNotification::getSelectedOption(notification, response);
}

void LLNotificationsUtil::cancel(LLNotificationPtr pNotif)
{
	LLNotifications::instance().cancel(pNotif);
}

LLNotificationPtr LLNotificationsUtil::find(LLUUID uuid)
{
	return LLNotifications::instance().find(uuid);
}
