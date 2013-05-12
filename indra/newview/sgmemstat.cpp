/* Copyright (C) 2012 Siana Gearz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */

#include "llviewerprecompiledheaders.h"
#include "sgmemstat.h"

#if (!LL_LINUX && !LL_USE_TCMALLOC)
bool SGMemStat::haveStat() {
	return false;
}

F32 SGMemStat::getMalloc() {
	return 0.f;
}

U32 SGMemStat::getNumObjects() {
	return 0;
}

std::string SGMemStat::getPrintableStat() {
	return std::string();
}

#else

extern "C" {
	typedef void (*MallocExtension_GetStats_t)(char* buffer, int buffer_length);
	typedef int (*MallocExtension_GetNumericProperty_t)(const char* property, size_t* value);
	//typedef int (*MallocExtension_SetNumericProperty_t)(const char* property, size_t value);
}

static MallocExtension_GetNumericProperty_t MallocExtension_GetNumericProperty = 0;
static MallocExtension_GetStats_t MallocExtension_GetStats = 0;

static void initialize() {
	static bool initialized = false;	
	if (!initialized) {
		apr_dso_handle_t* hprog = 0;
		LLAPRPool pool;
		pool.create();
#if LL_WINDOWS
		apr_dso_load(&hprog, "libtcmalloc_minimal.dll", pool());
#else
		apr_dso_load(&hprog, 0, pool());
#endif
		apr_dso_sym((apr_dso_handle_sym_t*)&MallocExtension_GetNumericProperty,
			hprog, "MallocExtension_GetNumericProperty");
		apr_dso_sym((apr_dso_handle_sym_t*)&MallocExtension_GetStats,
			hprog, "MallocExtension_GetStats");
		initialized = true;
	}	
}

bool SGMemStat::haveStat() {
	initialize();
	return MallocExtension_GetNumericProperty;
}

F32 SGMemStat::getMalloc() {
	if(MallocExtension_GetNumericProperty) {
		size_t value;
		MallocExtension_GetNumericProperty("generic.current_allocated_bytes", &value);
		return value;
	}
	else return 0.0f;
}

U32 SGMemStat::getNumObjects() {
	return 0;
}

std::string SGMemStat::getPrintableStat() {
	initialize();
	if (MallocExtension_GetStats) {
		char buffer[4096];
		buffer[4095] = 0;
		MallocExtension_GetStats(buffer, 4095);
		return std::string(buffer);
	}
	else return std::string();
}

#endif
