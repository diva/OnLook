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
#include "lltrans.h"

#include "llnotifications.h"
#include "llsd.h"
#include "llxmlnode.h"	// apparently needed to call LLNotifications::instance()

namespace AIAlert
{

LLNotificationPtr add(Error const& error, unsigned int suppress_mask, modal_nt type)
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

std::string text(Error const& error, int suppress_mask)
{
	std::string alert_text;
	bool suppress_newlines = false;
	bool last_was_prefix = false;
	for (Error::lines_type::const_iterator line = error.lines().begin(); line != error.lines().end(); ++line)
	{
		// Even if a line is suppressed, we print its leading newline if requested, but never more than one.
		if (!suppress_newlines && line->prepend_newline())
		{
			alert_text += '\n';
			suppress_newlines = true;
		}
		if (!line->suppressed(suppress_mask))
		{
			if (last_was_prefix) alert_text += ' ';		// The translation system strips off spaces... add them back.
			alert_text += LLTrans::getString(line->getXmlDesc(), line->args());
			suppress_newlines = false;
			last_was_prefix = line->is_prefix();
		}
	}
	return alert_text;
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
