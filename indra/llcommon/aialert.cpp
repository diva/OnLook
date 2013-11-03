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
 */

#include "aialert.h"

AIAlert::AIAlert(AIAlertPrefix const& prefix, modal_nt type,
                 std::string const& xml_desc, AIArgs const& args) : mModal(type)
{
  if (prefix) mLines.push_back(AIAlertLine(prefix));
  mLines.push_back(AIAlertLine(xml_desc, args));
}

AIAlert::AIAlert(AIAlertPrefix const& prefix, modal_nt type,
                 AIAlert const& alert,
                 std::string const& xml_desc, AIArgs const& args) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (prefix) mLines.push_back(AIAlertLine(prefix, !mLines.empty()));
  mLines.push_back(AIAlertLine(xml_desc, args));
}

AIAlert::AIAlert(AIAlertPrefix const& prefix, modal_nt type,
                 std::string const& xml_desc,
                 AIAlert const& alert) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (!mLines.empty()) { mLines.front().set_newline(); }
  mLines.push_front(AIAlertLine(xml_desc));
  if (prefix) mLines.push_front(AIAlertLine(prefix));
}

AIAlert::AIAlert(AIAlertPrefix const& prefix, modal_nt type,
                 std::string const& xml_desc, AIArgs const& args,
                 AIAlert const& alert) : mLines(alert.mLines), mModal(type)
{
  if (alert.mModal == modal) mModal = modal;
  if (!mLines.empty()) { mLines.front().set_newline(); }
  mLines.push_front(AIAlertLine(xml_desc, args));
  if (prefix) mLines.push_front(AIAlertLine(prefix));
}

