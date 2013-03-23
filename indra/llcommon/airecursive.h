/**
 * @file airecursive.h
 * @brief Declaration of AIRecursive.
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
 *   05/01/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AI_RECURSIVE
#define AI_RECURSIVE

// Exception safe class to detect recursive calls.
//
// A unique, static bool must be passed (thread local if the function is
// called by more than one thread).
//
// Example usage:
//
// void f()
// {
//   static bool recursive;
//   if (recursive) return;
//   AIRecursive dummy(recursive);
//   ...
// }

class AIRecursive {
  private:
	bool& mFlag;

  public:
	AIRecursive(bool& flag) : mFlag(flag) { mFlag = true; }
	~AIRecursive() { mFlag = false; }
};

#endif // AI_RECURSIVE
