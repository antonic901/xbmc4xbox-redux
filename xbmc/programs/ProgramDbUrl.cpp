/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ProgramDbUrl.h"
#include "SmartPlayList.h"
#include "utils/StringUtils2.h"
#include "utils/Variant.h"

using namespace std;
using namespace XFILE;

CProgramDbUrl::CProgramDbUrl()
  : CDbUrl()
{ }

CProgramDbUrl::~CProgramDbUrl()
{ }

bool CProgramDbUrl::parse()
{
  // TODO: implement this
  return true;
}

bool CProgramDbUrl::validateOption(const std::string &key, const CVariant &value)
{
  if (!CDbUrl::validateOption(key, value))
    return false;

  // if the value is empty it will remove the option which is ok
  // otherwise we only care about the "filter" option here
  if (value.empty() || !StringUtils2::EqualsNoCase(key, "filter"))
    return true;

  if (!value.isString())
    return false;

  CSmartPlaylist xspFilter;
  if (!xspFilter.LoadFromJson(value.asString()))
    return false;
  
  // check if the filter playlist matches the item type
  return (xspFilter.GetType() == m_itemType ||
         (xspFilter.GetType() == "movies" && m_itemType == "sets"));
}
