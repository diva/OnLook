/**
 * @file aifile.cpp
 * @brief POSIX file operations that throw on error.
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
 *   03/11/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"
#include "aifile.h"
#include "aialert.h"

#if LL_WINDOWS
#include <windows.h>
#include <stdlib.h>                 // Windows errno
#else
#include <errno.h>
#endif

AIFile::AIFile(std::string const& filename, char const* accessmode)
{
  mFp = AIFile::fopen(filename, accessmode);
}

AIFile::~AIFile()
{
  AIFile::close(mFp);
}

// Like THROW_MALERTE but appends "LLFile::strerr(errn) << " (" << errn << ')'" as argument to replace [ERROR].
#define THROW_ERROR(...) \
  do { \
	int errn = errno; \
	std::ostringstream error; \
	error << LLFile::strerr(errn) << " (" << errn << ')'; \
	THROW_MALERT_CLASS(AIAlert::ErrorCode, errn, __VA_ARGS__ ("[ERROR]", error.str())); \
  } while(0)

//static
void AIFile::mkdir(std::string const& dirname, int perms)
{
	int rc = LLFile::mkdir_nowarn(dirname, perms);
	if (rc < 0 && errno != EEXIST)
	{
		THROW_ERROR("AIFile_mkdir_Failed_to_create_DIRNAME", AIArgs("[DIRNAME]", dirname));
	}
}

//static
void AIFile::rmdir(std::string const& dirname)
{
	int rc = LLFile::rmdir_nowarn(dirname);
	if (rc < 0 && errno != ENOENT)
	{
		THROW_ERROR("AIFile_rmdir_Failed_to_remove_DIRNAME", AIArgs("[DIRNAME]", dirname));
	}
}

//static
LLFILE*	AIFile::fopen(std::string const& filename, const char* mode)
{
	LLFILE* fp = LLFile::fopen(filename, mode);
	if (!fp)
	{
		THROW_ERROR("AIFile_fopen_Failed_to_open_FILENAME", AIArgs("[FILENAME]", filename));
	}
	return fp;
}

//static
void AIFile::close(LLFILE* file)
{
	if (LLFile::close(file) < 0)
	{
		THROW_ERROR("AIFile_close_Failed_to_close_file", AIArgs);
	}
}

//static
void AIFile::remove(std::string const& filename)
{
	int rc = LLFile::remove_nowarn(filename);
	if (rc < 0 && errno != ENOENT)
	{
		THROW_ERROR("AIFile_remove_Failed_to_remove_FILENAME", AIArgs("[FILENAME]", filename));
	}
}

//static
void AIFile::rename(std::string const& filename, std::string const& newname)
{
	if (LLFile::rename_nowarn(filename, newname) < 0)
	{
		THROW_ERROR("AIFile_rename_Failed_to_rename_FILE_to_NEWFILE", AIArgs("[FILE]", filename)("[NEWFILE]", newname));
	}
}

