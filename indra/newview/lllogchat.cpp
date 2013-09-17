/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include <ctime>
#include "lllogchat.h"
#include "llappviewer.h"
#include "llfloaterchat.h"


//static
std::string LLLogChat::makeLogFileName(std::string filename)
{
	if (gSavedPerAccountSettings.getBOOL("LogFileNamewithDate"))
	{
		time_t now; 
		time(&now); 
		char dbuffer[100];               /* Flawfinder: ignore */
		if (filename == "chat") 
		{ 
			static const LLCachedControl<std::string> local_chat_date_format(gSavedPerAccountSettings, "LogFileLocalChatDateFormat", "-%Y-%m-%d");
			strftime(dbuffer, 100, local_chat_date_format().c_str(), localtime(&now));
		} 
		else 
		{ 
			static const LLCachedControl<std::string> ims_date_format(gSavedPerAccountSettings, "LogFileIMsDateFormat", "-%Y-%m");
			strftime(dbuffer, 100, ims_date_format().c_str(), localtime(&now));
		} 
		filename += dbuffer; 
	}
	filename = cleanFileName(filename);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS,filename);
	filename += ".txt";
	return filename;
}

std::string LLLogChat::cleanFileName(std::string filename)
{
	std::string invalidChars = "\"\'\\/?*:<>|[]{}~"; // Cannot match glob or illegal filename chars
	S32 position = filename.find_first_of(invalidChars);
	while (position != filename.npos)
	{
		filename[position] = '_';
		position = filename.find_first_of(invalidChars, position);
	}
	return filename;
}

std::string LLLogChat::timestamp(bool withdate)
{
	time_t utc_time;
	utc_time = time_corrected();

	// There's only one internal tm buffer.
	struct tm* timep;

	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	timep = utc_to_pacific_time(utc_time, gPacificDaylightTime);

	static LLCachedControl<bool> withseconds("SecondsInLog");
	std::string text;
	if (withdate)
		if (withseconds)
			text = llformat("[%d/%02d/%02d %02d:%02d:%02d]  ", (timep->tm_year-100)+2000, timep->tm_mon+1, timep->tm_mday, timep->tm_hour, timep->tm_min, timep->tm_sec);
		else
			text = llformat("[%d/%02d/%02d %02d:%02d]  ", (timep->tm_year-100)+2000, timep->tm_mon+1, timep->tm_mday, timep->tm_hour, timep->tm_min);
	else
		if (withseconds)
			text = llformat("[%02d:%02d:%02d]  ", timep->tm_hour, timep->tm_min, timep->tm_sec);
		else
			text = llformat("[%02d:%02d]  ", timep->tm_hour, timep->tm_min);

	return text;
}


//static
void LLLogChat::saveHistory(std::string const& filename, std::string line)
{
	if(!filename.size())
	{
		llinfos << "Filename is Empty!" << llendl;
		return;
	}

	LLFILE* fp = LLFile::fopen(LLLogChat::makeLogFileName(filename), "a"); 		/*Flawfinder: ignore*/
	if (!fp)
	{
		llinfos << "Couldn't open chat history log!" << llendl;
	}
	else
	{
		fprintf(fp, "%s\n", line.c_str());
		
		fclose (fp);
	}
}

const std::streamoff BUFFER_SIZE(4096);

// Read a chunk of size from pos in ifstr and prepend it to data
// return that chunk's newline count
U32 read_chunk(llifstream& ifstr, const std::streamoff& pos, U32 size, std::string& data)
{
	char buffer[BUFFER_SIZE];
	ifstr.seekg(pos);
	ifstr.read(buffer, size);
	data.insert(0, buffer, size);
	return std::count(buffer, buffer + size, '\n');
}

void LLLogChat::loadHistory(std::string const& filename , void (*callback)(ELogLineType,std::string,void*), void* userdata)
{
	if(!filename.size())
	{
		llwarns << "Filename is Empty!" << llendl;
		return ;
	}

	llifstream ifstr(makeLogFileName(filename));
	if (!ifstr.is_open())
	{
		callback(LOG_EMPTY,LLStringUtil::null,userdata);
	}
	else
	{
		static const LLCachedControl<U32> lines("LogShowHistoryLines", 32);
		ifstr.seekg(-1, std::ios_base::end);
		if (!lines || !ifstr)
		{
			callback(LOG_EMPTY,LLStringUtil::null,userdata);
			return;
		}

		std::string data;
		U32 nlines = 0;
		if (ifstr.get() != '\n') // in case file doesn't end with a newline
		{
			data.push_back('\n');
			++nlines;
		}

		// Read BUFFER_SIZE byte chunks until we have enough endlines accumulated
		for(std::streamoff pos = ifstr.tellg() - BUFFER_SIZE; nlines < lines+1; pos -= BUFFER_SIZE)
		{
			if (pos > 0)
			{
				nlines += read_chunk(ifstr, pos, BUFFER_SIZE, data);
			}
			else // Ran out of file read the remaining from the start
			{
				nlines += read_chunk(ifstr, 0, pos + BUFFER_SIZE, data);
				break;
			}
		}

		// Break data into lines
		std::istringstream sstr(data);
		for (std::string line; nlines > 0 && getline(sstr, line); --nlines)
		{
			if (nlines <= lines)
			{
				callback(LOG_LINE, line, userdata);
			}
		}
		callback(LOG_END,LLStringUtil::null,userdata);
	}
}
