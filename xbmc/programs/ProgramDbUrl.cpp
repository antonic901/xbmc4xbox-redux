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
#include "filesystem/ProgramDatabaseDirectory.h"
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
  // the URL must start with programdb://
  if (m_url.GetProtocol() != "programdb" || m_url.GetFileName().empty())
    return false;

  CStdString path = m_url.Get();
  PROGRAMDATABASEDIRECTORY::NODE_TYPE dirType = CProgramDatabaseDirectory::GetDirectoryType(path);
  PROGRAMDATABASEDIRECTORY::NODE_TYPE childType = CProgramDatabaseDirectory::GetDirectoryChildType(path);

  switch (dirType)
  {
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_GAMES_OVERVIEW:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_TITLE_GAMES:
      m_type = "games";
      break;

    default:
      break;
  }

  switch (childType)
  {
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_GAMES_OVERVIEW:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_TITLE_GAMES:
      m_type = "games";
      m_itemType = "games";
      break;

    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_GENRE:
      m_itemType = "genres";
      break;

    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_YEAR:
      m_itemType = "years";
      break;

    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_ROOT:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_OVERVIEW:
    default:
      return false;
  }

  if (m_type.empty() || m_itemType.empty())
    return false;

  // parse query params
  PROGRAMDATABASEDIRECTORY::CQueryParams queryParams;
  if (!CProgramDatabaseDirectory::GetQueryParams(path, queryParams))
    return false;

  // retrieve and parse all options
  AddOptions(m_url.GetOptions());

  // add options based on the QueryParams
  if (queryParams.GetGenreId() != -1)
    AddOption("genreid", queryParams.GetGenreId());
  if (queryParams.GetGameId() != -1)
    AddOption("gameid", queryParams.GetGameId());
  if (queryParams.GetDeveloperId() != -1)
    AddOption("developerid", queryParams.GetDeveloperId());
  if (queryParams.GetPublisherId() != -1)
    AddOption("publisherid", queryParams.GetPublisherId());
  if (queryParams.GetDescriptorId() != -1)
    AddOption("descriptorid", queryParams.GetDescriptorId());
  if (queryParams.GetGeneralFeatureId() != -1)
    AddOption("generalfeatureid", queryParams.GetGeneralFeatureId());
  if (queryParams.GetOnlineFeatureId() != -1)
    AddOption("onlinefeatureid", queryParams.GetOnlineFeatureId());
  if (queryParams.GetPlatformId() != -1)
    AddOption("platformid", queryParams.GetPlatformId());
  if (queryParams.GetTagId() != -1)
    AddOption("tagid", queryParams.GetTagId());
  if (queryParams.GetYear() != -1)
    AddOption("year", queryParams.GetYear());

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
  return (xspFilter.GetType() == m_itemType || xspFilter.GetType() == "games");
}
