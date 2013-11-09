/**
 * @file aialert.cpp
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
 *   - Initial version, written by Aleric Inglewood @ SL
 *
 *   05/11/2013
 *   Moved everything in namespace AIAlert, except AIArgs.
 */

#include "aialert.h"

namespace AIAlert
{

Error::Error(Prefix const& prefix, modal_nt type,
             Error const& alert) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (prefix) mLines.push_front(Line(prefix));
}

Error::Error(Prefix const& prefix, modal_nt type,
             std::string const& xml_desc, AIArgs const& args) : mModal(type)
{
  if (prefix) mLines.push_back(Line(prefix));
  mLines.push_back(Line(xml_desc, args));
}

Error::Error(Prefix const& prefix, modal_nt type,
             Error const& alert,
             std::string const& xml_desc, AIArgs const& args) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (prefix) mLines.push_back(Line(prefix, !mLines.empty()));
  mLines.push_back(Line(xml_desc, args));
}

Error::Error(Prefix const& prefix, modal_nt type,
             std::string const& xml_desc,
             Error const& alert) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (!mLines.empty()) { mLines.front().set_newline(); }
  mLines.push_front(Line(xml_desc));
  if (prefix) mLines.push_front(Line(prefix));
}

Error::Error(Prefix const& prefix, modal_nt type,
             std::string const& xml_desc, AIArgs const& args,
             Error const& alert) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (!mLines.empty()) { mLines.front().set_newline(); }
  mLines.push_front(Line(xml_desc, args));
  if (prefix) mLines.push_front(Line(prefix));
}

} // namespace AIAlert

