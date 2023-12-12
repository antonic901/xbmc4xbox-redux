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

#include "programs/ProgramDatabase.h"
#include "DirectoryNodeOverview.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "LocalizeStrings.h"
#include "programs/ProgramDbUrl.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;
using namespace std;


Node OverviewChildren[] = {
                            { NODE_TYPE_GAMES_OVERVIEW,            "games",                   15016 },
                            { NODE_TYPE_RECENTLY_ADDED_GAMES,      "recentlyaddedgames",      35106 },
                            { NODE_TYPE_RECENTLY_PLAYED_GAMES,     "recentlyplayedgames",     35107 }
                          };

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName().Equals(OverviewChildren[i].id.c_str()))
      return OverviewChildren[i].node;

  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName().Equals(OverviewChildren[i].id.c_str()))
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CProgramDatabase database;
  database.Open();
  bool hasGames = database.HasContent(PROGRAMDB_CONTENT_GAMES);
  vector<pair<const char*, int> > vec;
  if (hasGames)
  {
    if (g_settings.m_bMyProgramNavFlatten)
      vec.push_back(make_pair("games/titles", 15016));
    else
      vec.push_back(make_pair("games", 15016));   // Games
  }
  CStdString path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformated(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
