/** 
 * @file basic_plugin_base.h
 * @brief Basic plugin base class for Basic API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 * 
 * @endcond
 */

#ifndef BASIC_PLUGIN_BASE_H
#define BASIC_PLUGIN_BASE_H

#include <sstream>

#include "linden_common.h"

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"

class BasicPluginBase
{
public:
	BasicPluginBase(LLPluginInstance::sendMessageFunction send_message_function, LLPluginInstance* plugin_instance);
	//! Basic plugin destructor.
	virtual ~BasicPluginBase() {}

	//! Handle received message from plugin loader shell.
	virtual void receiveMessage(char const* message_string) = 0;

	// This function is actually called and then calls the member function above.
	static void staticReceiveMessage(char const* message_string, BasicPluginBase** self_ptr);

protected:
	void sendMessage(LLPluginMessage const& message);

	//! Message data being sent to plugin loader shell by mSendMessageFunction.
	LLPluginInstance* mPluginInstance;

	//! Function to send message from plugin to plugin loader shell.
	LLPluginInstance::sendMessageFunction mSendMessageFunction;

	//! Flag to delete plugin instance (self).
	bool mDeleteMe;
};

/** The plugin <b>must</b> define this function to create its instance.
 * It should look something like this: 
 * @code
 * {  
 *    *plugin_object = new FooPluginBar(send_message_function, plugin_instance); 
 *    return 0; 
 * }  
 * @endcode
 */
int create_plugin(
	LLPluginInstance::sendMessageFunction send_message_function, 
	LLPluginInstance* plugin_instance, 
	BasicPluginBase** plugin_object);

#endif // BASIC_PLUGIN_BASE

