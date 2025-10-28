/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "GameSavesDirectory.h"

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "Util.h"

using namespace XFILE;

CGameSavesDirectory::CGameSavesDirectory(void)
{
}

CGameSavesDirectory::~CGameSavesDirectory(void)
{
}

bool CGameSavesDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // URL in Format: gamesaves://{title_id}/{savegame_id}
  std::string strRootPath = "E:\\UDATA\\";
  if (!url.GetHostName().empty())
    strRootPath += url.GetHostName();
  URIUtils::AddSlashAtEnd(strRootPath);

  CFileItemList tempItems;
  if (!CDirectory::GetDirectory(strRootPath, tempItems, "", DIR_FLAG_BYPASS_CACHE))
    return false;

  for (int i = 0; i < tempItems.Size(); ++i)
  {
    CFileItemPtr pItem = tempItems[i];

    if (!pItem->m_bIsFolder)
      continue;

    std::string strTitlemetaXBX = URIUtils::AddFileToFolder(pItem->GetPath(), "titlemeta.xbx");
    std::string strSavemetaXBX = URIUtils::AddFileToFolder(pItem->GetPath(), "savemeta.xbx");

    CFile file;
    int mode = 0;
    if (CFile::Exists(strTitlemetaXBX) && file.Open(strTitlemetaXBX))
      mode = 1;
    else if (CFile::Exists(strSavemetaXBX) && file.Open(strSavemetaXBX))
      mode = 2;
    else
      continue;

    // Read title name
    WCHAR *yo = new WCHAR[(int)file.GetLength() + 1];
    file.Read(yo, file.GetLength());
    yo[file.GetLength()] = L'\0';
    std::string strLabel;
    g_charsetConverter.wToUTF8(yo, strLabel);
    int poss = strLabel.find("Name=");
    if (poss == -1)
    {
      wchar_t *chrtxt = new wchar_t[(int)file.GetLength() + 2];
      file.Seek(0);
      file.Read(chrtxt, file.GetLength());
      chrtxt[(int)file.GetLength() + 1] = '\n';
      g_charsetConverter.wToUTF8(chrtxt, strLabel);
      poss = strLabel.find("Name=");
    }
    file.Close();
    int pose = strLabel.find("\n", poss + 1);
    strLabel = strLabel.substr(poss+5, pose - poss-6);
    strLabel = CUtil::MakeLegalFileName(strLabel, LEGAL_NONE);

    // Format path in format: gamesaves://{title_id}/{savegame_id}
    std::vector<std::string> Path = StringUtils::Split(pItem->GetPath(), '\\');
    Path.pop_back();
    std::string strPath = URIUtils::AddFileToFolder(url.Get(), Path.back());

    CFileItemPtr item(new CFileItem(strPath, false));
    item->SetLabel(strLabel);
    item->SetLabelPreformated(true);
    item->SetIconImage("defaultProgram.png");

    if (mode == 1)
    {
      CFileItemList items2;
      if(CDirectory::GetDirectory(pItem->GetPath(), items2, "", DIR_FLAG_BYPASS_CACHE))
      {
        int total = 0;
        for (int j = 0; j < items2.Size(); ++j)
        {
          if (items2[j]->m_bIsFolder && CFile::Exists(URIUtils::AddFileToFolder(items2[j]->GetPath(), "savemeta.xbx")))
            total++;
        }
        if (total > 0)
          item->m_bIsFolder = true;
      }
    }

    items.Add(item);
  }

  return true;
}
