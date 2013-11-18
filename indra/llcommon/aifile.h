/**
 * @file aifile.h
 * @brief Declaration of AIFile.
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

#ifndef AIFILE_H
#define AIFILE_H

#include "llfile.h"

// As LLFile, but throws AIAlert instead of printing a warning.
class LL_COMMON_API AIFile
{
  private:
	LLFILE* mFp;

  public:
	// Scoped file (exception safe). Throws AIAlertCode with errno on failure.
	AIFile(std::string const& filename, char const* accessmode);
	~AIFile();

	operator LLFILE* () const { return mFp; }

	// All these functions take UTF8 path/filenames.
	static LLFILE* fopen(std::string const& filename, char const* accessmode);
	static void close(LLFILE* file);

	static void mkdir(std::string const& dirname, int perms = 0700);				// Does NOT throw when dirname already exists.
	static void rmdir(std::string const& dirname);									// Does NOT throw when dirname does not exist.
	static void remove(std::string const& filename);								// Does NOT throw when filename does not exist.
	static void rename(std::string const& filename, std::string const& newname);
};

#endif // AIFILE_H
