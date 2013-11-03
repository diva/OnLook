/**
 * @file aialert.h
 * @brief Declaration of AIArgs, AIAlertPrefix, AIAlertLine, AIAlert and AIAlertCode
 *
 * Copyright (c) 2013, Aleric Inglewood.
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
 *   02/11/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AI_ALERT
#define AI_ALERT

#include "llpreprocessor.h"
#include "llstring.h"
#include <deque>
#include <exception>

//===================================================================================================================================
// Facility to throw errors that can easily be converted to an informative pop-up floater for the user.

// Throw arbitrary class.
#define   THROW_ALERT_CLASS(Alert, ...) throw Alert(AIAlertPrefix(),                                                       AIAlert::not_modal, __VA_ARGS__)
#define  THROW_MALERT_CLASS(Alert, ...) throw Alert(AIAlertPrefix(),                                                           AIAlert::modal, __VA_ARGS__)
#define  THROW_FALERT_CLASS(Alert, ...) throw Alert(AIAlertPrefix(__PRETTY_FUNCTION__, alert_line_pretty_function_prefix), AIAlert::not_modal, __VA_ARGS__)
#define THROW_FMALERT_CLASS(Alert, ...) throw Alert(AIAlertPrefix(__PRETTY_FUNCTION__, alert_line_pretty_function_prefix),     AIAlert::modal, __VA_ARGS__)

// Shortcut to throw AIAlert.
#define   THROW_ALERT(...)   THROW_ALERT_CLASS(AIAlert, __VA_ARGS__)
#define  THROW_MALERT(...)  THROW_MALERT_CLASS(AIAlert, __VA_ARGS__)
#define  THROW_FALERT(...)  THROW_FALERT_CLASS(AIAlert, __VA_ARGS__)
#define THROW_FMALERT(...) THROW_FMALERT_CLASS(AIAlert, __VA_ARGS__)

// Shortcut to throw AIAlertCode.
#define   THROW_ALERTC(...)   THROW_ALERT_CLASS(AIAlertCode, __VA_ARGS__)
#define  THROW_MALERTC(...)  THROW_MALERT_CLASS(AIAlertCode, __VA_ARGS__)
#define  THROW_FALERTC(...)  THROW_FALERT_CLASS(AIAlertCode, __VA_ARGS__)
#define THROW_FMALERTC(...) THROW_FMALERT_CLASS(AIAlertCode, __VA_ARGS__)

// Shortcut to throw AIAlertCode with errno as code.
#define   THROW_ALERTE(...) do { int errn = errno;   THROW_ALERT_CLASS(AIAlertCode, errn, __VA_ARGS__); } while(0)
#define  THROW_MALERTE(...) do { int errn = errno;  THROW_MALERT_CLASS(AIAlertCode, errn, __VA_ARGS__); } while(0)
#define  THROW_FALERTE(...) do { int errn = errno;  THROW_FALERT_CLASS(AIAlertCode, errn, __VA_ARGS__); } while(0)
#define THROW_FMALERTE(...) do { int errn = errno; THROW_FMALERT_CLASS(AIAlertCode, errn, __VA_ARGS__); } while(0)

// Examples

#ifdef EXAMPLE_CODE

	//----------------------------------------------------------
	// To show the alert box:

	catch (AIAlert const& alert)
    {
	   LLNotificationsUtil::add(alert);		// Optionally pass alert_line_pretty_function_prefix as second parameter to *suppress* that output.
	}

    // or, for example

	catch (AIAlertCode const& alert)
    {
	   if (alert.getCode() != EEXIST)
	   {
		 LLNotificationsUtil::add(alert, alert_line_pretty_function_prefix);
	   }
	}
	//----------------------------------------------------------
    // To throw alerts:

	THROW_ALERT("ExampleKey");																// A) Lookup "ExampleKey" in strings.xml and show translation.
	THROW_ALERT("ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second)(...etc...));		// B) Same as A, but replace [FIRST] with first, [SECOND] with second, etc.
	THROW_ALERT("ExampleKey", alert);														// C) As A, but followed by a colon and a newline, and then the text of 'alert'.
	THROW_ALERT(alert, "ExampleKey");														// D) The text of 'alert', followed by a colon and a newline and then as A.
	THROW_ALERT("ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second), alert);			// E) As B, but followed by a colon and a newline, and then the text of 'alert'.
	THROW_ALERT(alert, "ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second));			// F) The text of 'alert', followed by a colon and a newline and then as B.
	// where 'alert' is a caught AIAlert object (as above) in a rethrow.
	// Prepend ALERT with M and/or F to make the alert box Modal and/or prepend the text with the current function name.
	// For example,
	THROW_MFALERT("ExampleKey", AIArgs("[FIRST]", first));	// Throw a Modal alert box that is prefixed with the current Function name.
	// Append E after ALERT to throw an AIAlertCode class that contains the current errno.
	// For example,
	THROW_FALERTE("ExampleKey", AIArgs("[FIRST]", first));	// Throw an alert box that is prefixed with the current Function name and pass errno to the catcher.

#endif // EXAMPLE_CODE

//
//===================================================================================================================================

enum alert_line_type_nt
{
  alert_line_normal = 0,
  alert_line_empty_prefix = 1,
  alert_line_pretty_function_prefix = 2
  // These must exist of single bits (a mask).
};

// An AIAlertPrefix currently comes only in two flavors:
//
// alert_line_empty_prefix           : An empty prefix.
// alert_line_pretty_function_prefix : A function name prefix, this is the function from which the alert was thrown.

class LL_COMMON_API AIAlertPrefix
{
  public:
	AIAlertPrefix(void) : mType(alert_line_empty_prefix) { }
	AIAlertPrefix(char const* str, alert_line_type_nt type) : mStr(str), mType(type) { }

	operator bool(void) const { return mType != alert_line_empty_prefix; }
	alert_line_type_nt type(void) const { return mType; }
	std::string const& str(void) const { return mStr; }

  private:
	std::string mStr;				// Literal text. For example a C++ function name.
	alert_line_type_nt mType;		// The type of this prefix.
};

// A wrapper around LLStringUtil::format_map_t to allow constructing a dictionary
// on one line by doing:
//
// AIArgs("[ARG1]", arg1)("[ARG2]", arg2)("[ARG3]", arg3)...

class LL_COMMON_API AIArgs
{
  private:
	LLStringUtil::format_map_t mArgs;	// The underlying replacement map.

  public:
	// Construct an empty map.
	AIArgs(void) { }
	// Construct a map with a single replacement.
	AIArgs(char const* key, std::string const& replacement) { mArgs[key] = replacement; }
	// Add another replacement.
	AIArgs& operator()(char const* key, std::string const& replacement) { mArgs[key] = replacement; return *this; }
	// The destructor may not throw.
	~AIArgs() throw() { }

	// Accessor.
	LLStringUtil::format_map_t const& operator*() const { return mArgs; }
};

// A class that represents one line with its replacements.
// The string mXmlDesc shall be looked up in strings.xml.
// This is not done as part of this class because LLTrans::getString
// is not part of llcommon.

class LL_COMMON_API AIAlertLine
{
  private:
	bool mNewline;					// Prepend this line with a newline if set.
	std::string mXmlDesc;			// The keyword to look up in string.xml.
	AIArgs mArgs;					// Replacement map.
	alert_line_type_nt mType;		// The type of this line: alert_line_normal for normal lines, other for prefixes.

  public:
	AIAlertLine(std::string const& xml_desc, bool newline = false) : mNewline(newline), mXmlDesc(xml_desc), mType(alert_line_normal) { }
	AIAlertLine(std::string const& xml_desc, AIArgs const& args, bool newline = false) : mNewline(newline), mXmlDesc(xml_desc), mArgs(args), mType(alert_line_normal) { }
	AIAlertLine(AIAlertPrefix const& prefix, bool newline = false) : mNewline(newline), mXmlDesc("AIPrefix"), mArgs("[PREFIX]", prefix.str()), mType(prefix.type()) { }
	// The destructor may not throw.
	~AIAlertLine() throw() { }

	// Prepend a newline before this line.
	void set_newline(void) { mNewline = true; }

	// These are to be used like: LLTrans::getString(line.getXmlDesc(), line.args()) and prepend with a \n if prepend_newline() returns true.
	std::string getXmlDesc(void) const { return mXmlDesc; }
	LLStringUtil::format_map_t const& args(void) const { return *mArgs; }
	bool prepend_newline(void) const { return mNewline; }

	// Accessors.
	bool suppressed(unsigned int suppress_mask) const { return (suppress_mask & mType) != 0; }
	bool is_prefix(void) const { return mType != alert_line_normal; }
};

// This class is used to throw an error that will cause
// an alert box to pop up for the user.
//
// An alert box only has text and an OK button.
// The alert box does not give feed back to the program; it is purely informational.

// The class represents multiple lines, each line is to be translated and catenated,
// separated by newlines, and then written to an alert box. This is not done as part
// of this class because LLTrans::getString and LLNotification is not part of llcommon.
// Instead call LLNotificationUtil::add(AIAlert const&).

class LL_COMMON_API AIAlert : public std::exception
{
  public:
	typedef std::deque<AIAlertLine> lines_type;
	enum modal_nt { not_modal, modal };

	// The destructor may not throw.
	~AIAlert() throw() { }

	// Accessors.
	lines_type const& lines(void) const { return mLines; }
	bool is_modal(void) const { return mModal == modal; }

	// A string with zero or more replacements.
	AIAlert(AIAlertPrefix const& prefix, modal_nt modal,
		    std::string const& xml_desc, AIArgs const& args = AIArgs());

	// Same as above bit prepending the message with the text of another alert.
	AIAlert(AIAlertPrefix const& prefix, modal_nt modal,
		    AIAlert const& alert,
		    std::string const& xml_desc, AIArgs const& args = AIArgs());

	// Same as above but appending the message with the text of another alert.
	// (no args)
	AIAlert(AIAlertPrefix const& prefix, modal_nt modal,
		    std::string const& xml_desc,
		    AIAlert const& alert);
	// (with args)
	AIAlert(AIAlertPrefix const& prefix, modal_nt modal,
		    std::string const& xml_desc, AIArgs const& args,
		    AIAlert const& alert);

  private:
	lines_type mLines;								// The lines (or prefixes) of text to be displayed, each consisting on a keyword (to be looked up in strings.xml) and a replacement map.
	modal_nt mModal;								// If true, make the alert box a modal floater.
};

// Same as AIAlert but allows to pass an additional error code.

class LL_COMMON_API AIAlertCode : public AIAlert
{
  private:
	int mCode;

  public:
	// The destructor may not throw.
	~AIAlertCode() throw() { }

	// Accessor.
	int getCode(void) const { return mCode; }

	// A string with zero or more replacements.
	AIAlertCode(AIAlertPrefix const& prefix, modal_nt modal, int code,
		        std::string const& xml_desc, AIArgs const& args = AIArgs()) :
	  AIAlert(prefix, modal, xml_desc, args) { }

	// Same as above bit prepending the message with the text of another alert.
	AIAlertCode(AIAlertPrefix const& prefix, modal_nt modal, int code,
		        AIAlert const& alert,
		        std::string const& xml_desc, AIArgs const& args = AIArgs()) :
	  AIAlert(prefix, modal, alert, xml_desc, args) { }

	// Same as above but appending the message with the text of another alert.
	// (no args)
	AIAlertCode(AIAlertPrefix const& prefix, modal_nt modal, int code,
		        std::string const& xml_desc,
		        AIAlert const& alert) :
	  AIAlert(prefix, modal, xml_desc, alert) { }
	// (with args)
	AIAlertCode(AIAlertPrefix const& prefix, modal_nt modal, int code,
		        std::string const& xml_desc, AIArgs const& args,
		        AIAlert const& alert) :
	  AIAlert(prefix, modal, xml_desc, args, alert) { }

};

#endif // AI_ALERT
