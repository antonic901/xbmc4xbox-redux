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

#include "utils/log.h"
#include "ProgramDatabaseDirectory.h"
#include "utils/URIUtils.h"
#include "ProgramDatabaseDirectory/QueryParams.h"
#include "programs/ProgramDatabase.h"
#include "TextureManager.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "utils/Crc32.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

CProgramDatabaseDirectory::CProgramDatabaseDirectory(void)
{
}

CProgramDatabaseDirectory::~CProgramDatabaseDirectory(void)
{
}

bool CProgramDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return false;

  bool bResult = pNode->GetChilds(items);
  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr item = items[i];
    if (item->m_bIsFolder && !item->HasIcon() && !item->HasThumbnail())
    {
      CStdString strImage = GetIcon(item->GetPath());
      if (!strImage.IsEmpty() && g_TextureManager.HasTexture(strImage))
        item->SetIconImage(strImage);
    }
  }
  items.SetLabel(pNode->GetLocalizedName());

  return bResult;
}

NODE_TYPE CProgramDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetChildType();
}

NODE_TYPE CProgramDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetType();
}

NODE_TYPE CProgramDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  CDirectoryNode* pParentNode=pNode->GetParent();

  if (!pParentNode)
    return NODE_TYPE_NONE;

  return pParentNode->GetChildType();
}

bool CProgramDatabaseDirectory::GetQueryParams(const CStdString& strPath, CQueryParams& params)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return false;
  
  CDirectoryNode::GetDatabaseInfo(strPath,params);
  return true;
}

void CProgramDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
{
  CStdString path = strDirectory;
  URIUtils::RemoveSlashAtEnd(path);

  Crc32 crc;
  crc.ComputeFromLowerCase(path);

  CStdString strFileName;
  strFileName.Format("special://temp/%08x.fi", (unsigned __int32) crc);
  CFile::Delete(strFileName);
}

bool CProgramDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
  if (strDirectory.Right(4).Equals("/-1/"))
    return true;
  return false;
}

bool CProgramDatabaseDirectory::GetLabel(const CStdString& strDirectory, CStdString& strLabel)
{
  strLabel = "";

  CStdString path = strDirectory;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode.get() || path.IsEmpty())
    return false;

  // first see if there's any filter criteria
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(path, params);

  CProgramDatabase programdatabase;
  if (!programdatabase.Open())
    return false;

  // get genre
  if (params.GetGenreId() != -1)
    strLabel += programdatabase.GetGenreById(params.GetGenreId());

  // get year
  if (params.GetYear() != -1)
  {
    CStdString strTemp;
    strTemp.Format("%i",params.GetYear());
    if (!strLabel.IsEmpty())
      strLabel += " / ";
    strLabel += strTemp;
  }

  if (strLabel.IsEmpty())
  {
    switch (pNode->GetChildType())
    {
    case NODE_TYPE_TITLE_GAMES:
      strLabel = g_localizeStrings.Get(369); break;
    case NODE_TYPE_GAMES_OVERVIEW: // Games
      strLabel = g_localizeStrings.Get(35002); break;
    default:
      CLog::Log(LOGWARNING, "%s - Unknown nodetype requested %d", __FUNCTION__, pNode->GetChildType());
      return false;
    }
  }

  return true;
}

CStdString CProgramDatabaseDirectory::GetIcon(const CStdString &strDirectory)
{
  CStdString path = strDirectory;
  switch (GetDirectoryChildType(path))
  {
  case NODE_TYPE_TITLE_GAMES:
    if (path.Equals("programdb://games/titles/"))
    {
      if (g_settings.m_bMyProgramNavFlatten)
        return "DefaultGames.png";
      return "DefaultGameTitle.png";
    }
    return "";
  case NODE_TYPE_GAMES_OVERVIEW: // Games
    return "DefaultGames.png";
  default:
    CLog::Log(LOGWARNING, "%s - Unknown nodetype requested %s", __FUNCTION__, strDirectory.c_str());
    break;
  }

  return "";
}

bool CProgramDatabaseDirectory::ContainsGames(const CStdString &path)
{
  PROGRAMDATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
  if (type == PROGRAMDATABASEDIRECTORY::NODE_TYPE_TITLE_GAMES) return true;
  return false;
}

bool CProgramDatabaseDirectory::Exists(const char* strPath)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode.get())
    return false;

  if (pNode->GetChildType() == PROGRAMDATABASEDIRECTORY::NODE_TYPE_NONE)
    return false;

  return true;
}

bool CProgramDatabaseDirectory::CanCache(const CStdString& strPath)
{
  CStdString path = strPath;
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode.get())
    return false;
  return pNode->CanCache();
}
