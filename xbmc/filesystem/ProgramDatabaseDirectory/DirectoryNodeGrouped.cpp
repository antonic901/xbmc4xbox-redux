/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DirectoryNodeGrouped.h"
#include "QueryParams.h"
#include "programs/ProgramDatabase.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NODE_TYPE type, const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NODE_TYPE CDirectoryNodeGrouped::GetChildType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  if (params.GetContentType() == PROGRAMDB_CONTENT_GAMES)
    return NODE_TYPE_TITLE_GAMES;

  // this could matter in future roms/emulators integration
  return NODE_TYPE_TITLE_GAMES/*NODE_TYPE_TITLE_TVSHOWS*/;
}

CStdString CDirectoryNodeGrouped::GetLocalizedName() const
{
  CProgramDatabase db;
  if (db.Open())
    return db.GetItemById(GetContentType(), GetID());

  return "";
}

bool CDirectoryNodeGrouped::GetContent(CFileItemList& items) const
{
  CProgramDatabase programdatabase;
  if (!programdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  std::string itemType = GetContentType(params);
  if (itemType.empty())
    return false;

  return programdatabase.GetItems(BuildPath(), (PROGRAMDB_CONTENT_TYPE)params.GetContentType(), itemType, items);
}

std::string CDirectoryNodeGrouped::GetContentType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  return GetContentType(params);
}

std::string CDirectoryNodeGrouped::GetContentType(const CQueryParams &params) const
{
  switch (GetType())
  {
    case NODE_TYPE_GENRE:
      return "genres";
    case NODE_TYPE_YEAR:
      return "years";
    case NODE_TYPE_GAMES_OVERVIEW:
    case NODE_TYPE_NONE:
    case NODE_TYPE_OVERVIEW:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_TITLE_GAMES:
    default:
      break;
  }

  return "";
}
