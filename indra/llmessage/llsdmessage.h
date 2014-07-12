/**
 * @file   llsdmessage.h
 * @author Nat Goodspeed
 * @date   2008-10-30
 * @brief  API intended to unify sending capability, UDP and TCP messages:
 *         https://wiki.lindenlab.com/wiki/Viewer:Messaging/Messaging_Notes
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

#if ! defined(LL_LLSDMESSAGE_H)
#define LL_LLSDMESSAGE_H

#include "llerror.h"                // LOG_CLASS()
#include "llevents.h"               // LLEventPumps
#include "llhttpclient.h"
#include <string>
#include <stdexcept>

class LLSD;
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy eventResponder_timeout;

/**
 * Class managing the messaging API described in
 * https://wiki.lindenlab.com/wiki/Viewer:Messaging/Messaging_Notes
 */
class LLSDMessage
{
    LOG_CLASS(LLSDMessage);

public:
    LLSDMessage();

    /// Exception if you specify arguments badly
    struct ArgError: public std::runtime_error
    {
        ArgError(const std::string& what):
            std::runtime_error(std::string("ArgError: ") + what) {}
    };

    /**
     * The response idiom used by LLSDMessage -- LLEventPump names on which to
     * post reply or error -- is designed for the case in which your
     * reply/error handlers are methods on the same class as the method
     * sending the message. Any state available to the sending method that
     * must be visible to the reply/error methods can conveniently be stored
     * on that class itself, if it's not already.
     *
     * The LLHTTPClient::ResponderBase idiom requires a separate instance of a
     * separate class so that it can dispatch to the code of interest by
     * calling canonical virtual methods. Interesting state must be copied
     * into that new object. 
     *
     * With some trepidation, because existing response code is packaged in
     * LLHTTPClient::ResponderWithResult subclasses, we provide this adapter class
     * <i>for transitional purposes only.</i> Instantiate a new heap
     * ResponderAdapter with your new LLHTTPClient::ResponderPtr. Pass
     * ResponderAdapter::getReplyName() and/or getErrorName() in your
     * LLSDMessage (or LLViewerRegion::getCapAPI()) request event. The
     * ResponderAdapter will call the appropriate Responder method, then
     * @c delete itself.
     *
     * Singularity note: I think this class/API is a bad idea that makes things
     * more complex, a lot slower and less OO. The idea to get methods called
     * on the same class that does the request is a nice idea, but should
     * be implemented through boost::bind and NOT use LLSD. Avoid.
     * Also note that this only works for ResponderWithResult derived classes,
     * not for responders derived from ResponderWithCompleted.
     * --Aleric
     */
    class ResponderAdapter
    {
    public:
        /**
         * Bind the new LLHTTPClient::ResponderWithResult subclass instance.
         *
         * Passing the constructor a name other than the default is only
         * interesting if you suspect some usage will lead to an exception or
         * log message.
         */
        ResponderAdapter(LLHTTPClient::ResponderWithResult* responder,
                         const std::string& name="ResponderAdapter");

        /// EventPump name on which LLSDMessage should post reply event
        std::string getReplyName() const { return mReplyPump.getName(); }
        /// EventPump name on which LLSDMessage should post error event
        std::string getErrorName() const { return mErrorPump.getName(); }
        /// Name of timeout policy to use.
        std::string getTimeoutPolicyName() const;

    private:
        // We have two different LLEventStreams, though we route them both to
        // the same listener, so that we can bind an extra flag identifying
        // which case (reply or error) reached that listener.
        bool listener(const LLSD&, bool success);

        LLHTTPClient::ResponderPtr mResponder;
        LLEventStream mReplyPump, mErrorPump;
    };

    /**
     * Force our implementation file to be linked with caller. The .cpp file
     * contains a static instance of this class, which must be linked into the
     * executable to support the canonical listener. But since the primary
     * interface to that static instance is via a named LLEventPump rather
     * than by direct reference, the linker doesn't necessarily perceive the
     * necessity to bring in the translation unit. Referencing this dummy
     * method forces the issue.
     */
    static void link();

private:
    friend class LLCapabilityListener;
    /// Responder used for internal purposes by LLSDMessage and
    /// LLCapabilityListener. Others should use higher-level APIs.
    class EventResponder: public LLHTTPClient::ResponderWithResult
    {
    public:
        /**
         * LLHTTPClient::ResponderWithResult that dispatches via named LLEventPump instances.
         * We bind LLEventPumps, even though it's an LLSingleton, for testability.
         * We bind the string names of the desired LLEventPump instances rather
         * than actually obtain()ing them so we only obtain() the one we're going
         * to use. If the caller doesn't bother to listen() on it, the other pump
         * may never materialize at all.
         * @a target and @a message are only to clarify error processing.
         * For a capability message, @a target should be the region description,
         * @a message should be the capability name.
         * For a service with a visible URL, pass the URL as @a target and the HTTP verb
         * (e.g. "POST") as @a message.
         */
        EventResponder(LLEventPumps& pumps,
                       const LLSD& request,
                       const std::string& target, const std::string& message,
                       const std::string& replyPump, const std::string& errorPump);

		void setTimeoutPolicy(std::string const& name);
    
        /*virtual*/ void httpSuccess(void);
        /*virtual*/ void httpFailure(void);

		/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return *mHTTPTimeoutPolicy; }
		/*virtual*/ char const* getName(void) const { return "EventResponder"; }

    private:
        LLEventPumps& mPumps;
        LLReqID mReqID;
        const std::string mTarget, mMessage, mReplyPump, mErrorPump;
        AIHTTPTimeoutPolicy const* mHTTPTimeoutPolicy;
    };

private:
    bool httpListener(const LLSD&);
    LLEventStream mEventPump;
};

#endif /* ! defined(LL_LLSDMESSAGE_H) */
