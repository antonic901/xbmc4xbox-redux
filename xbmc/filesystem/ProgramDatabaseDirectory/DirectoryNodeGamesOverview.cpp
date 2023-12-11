/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DirectoryNodeGamesOverview.h"
#include "FileItem.h"
#include "programs/ProgramDatabase.h"
#include "LocalizeStrings.h"
#include "programs/ProgramDbUrl.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;
using namespace std;

Node GameChildren[] = {
                        { NODE_TYPE_DEVELOPER,      "developers",     35100 },
                        { NODE_TYPE_PUBLISHER,      "publishers",     35101 },
                        { NODE_TYPE_GENRE,          "genres",         135   },
                        { NODE_TYPE_DESCRIPTOR,     "descriptors",    35102 },
                        { NODE_TYPE_GENERALFEATURE, "generalfeatures",35103 },
                        { NODE_TYPE_ONLINEFEATURE,  "onlinefeatures", 35104 },
                        { NODE_TYPE_PLATFORM,       "platforms",      35105 },
                        { NODE_TYPE_TITLE_GAMES,    "titles",         369   },
                        { NODE_TYPE_YEAR,           "years",          562   }
                       };

CDirectoryNodeGamesOverview::CDirectoryNodeGamesOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_GAMES_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeGamesOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(GameChildren) / sizeof(Node); ++i)
    if (GetName().Equals(GameChildren[i].id.c_str()))
      return GameChildren[i].node;

  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeGamesOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(GameChildren) / sizeof(Node); ++i)
    if (GetName().Equals(GameChildren[i].id.c_str()))
      return g_localizeStrings.Get(GameChildren[i].label);
  return "";
}

bool CDirectoryNodeGamesOverview::GetContent(CFileItemList& items) const
{
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(BuildPath()))
    return false;

  for (unsigned int i = 0; i < sizeof(GameChildren) / sizeof(Node); ++i)
  {
    CProgramDbUrl itemUrl = programUrl;
    CStdString strDir; strDir.Format("%s/", GameChildren[i].id);
    itemUrl.AppendPath(strDir);

    CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), true));
    pItem->SetLabel(g_localizeStrings.Get(GameChildren[i].label));
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
