/**
 * @file llwlparamkey.h
 * @brief LLWLParamKey struct
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLWLPARAMKEY_H
#define LL_LLWLPARAMKEY_H

#include "lltrans.h"
#include "llenvmanager.h"

struct LLWLParamKey : LLEnvKey
{
public:
	// scope and source of a param set (WL sky preset)
	std::string name;
	EScope scope;

	// for conversion from LLSD
	static const int NAME_IDX = 0;
	static const int SCOPE_IDX = 1;

	inline LLWLParamKey(const std::string& n, EScope s)
		: name(n), scope(s)
	{
	}

	inline LLWLParamKey(LLSD llsd)
		: name(llsd[NAME_IDX].asString()), scope(EScope(llsd[SCOPE_IDX].asInteger()))
	{
	}

	inline LLWLParamKey() // NOT really valid, just so std::maps can return a default of some sort
		: name(""), scope(SCOPE_LOCAL)
	{
	}

	inline LLWLParamKey(std::string& stringVal)
	{
		size_t len = stringVal.length();
		if (len > 0)
		{
			name = stringVal.substr(0, len - 1);
			scope = (EScope) atoi(stringVal.substr(len - 1, len).c_str());
		}
	}

	inline std::string toStringVal() const
	{
		std::stringstream str;
		str << name << scope;
		return str.str();
	}

	inline LLSD toLLSD() const
	{
		LLSD llsd = LLSD::emptyArray();
		llsd.append(LLSD(name));
		llsd.append(LLSD(scope));
		return llsd;
	}

	inline void fromLLSD(const LLSD& llsd)
	{
		name = llsd[NAME_IDX].asString();
		scope = EScope(llsd[SCOPE_IDX].asInteger());
	}

	inline bool operator <(const LLWLParamKey other) const
	{
		if (name < other.name)
		{
			return true;
		}
		else if (name > other.name)
		{
			return false;
		}
		else
		{
			return scope < other.scope;
		}
	}

	inline bool operator ==(const LLWLParamKey other) const
	{
		return (name == other.name) && (scope == other.scope);
	}

	inline std::string toString() const
	{
		switch (scope)
		{
		case SCOPE_LOCAL:
			return name + std::string(" (") + LLTrans::getString("Local") + std::string(")");
			break;
		case SCOPE_REGION:
			return name + std::string(" (") + LLTrans::getString("Region") + std::string(")");
			break;
		default:
			return name + " (?)";
		}
	}
};

#endif
