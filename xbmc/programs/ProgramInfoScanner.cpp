/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramInfoScanner.h"

#include "FileItem.h"
#include "TextureCache.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "programs/ProgramInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

using namespace XFILE;
using namespace ADDON;

namespace PROGRAM
{
  CProgramInfoScanner::CProgramInfoScanner()
  {
    m_bStop = false;
    m_strDirectory.clear();
  }

  CProgramInfoScanner::~CProgramInfoScanner()
  {
  }

  void CProgramInfoScanner::Process()
  {
    m_bStop = false;

    try
    {
      if (m_showDialog)
      {
        CGUIDialogExtendedProgressBar* dialog = static_cast<CGUIDialogExtendedProgressBar*>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
        if (dialog)
          m_handle = dialog->GetHandle(g_localizeStrings.Get(314));

        m_database.Open();
        m_bCanInterrupt = false;

        DoScan(m_strDirectory);
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s: Exception while scanning.", __FUNCTION__);
    }

    m_database.Close();
    m_bRunning = false;

    if (m_handle)
      m_handle->MarkFinished();
    m_handle = NULL;
  }

  void CProgramInfoScanner::Start(const std::string& strDirectory)
  {
    m_strDirectory = strDirectory;
    m_bRunning = true;
    Process();
  }

  void CProgramInfoScanner::Stop()
  {
    m_bStop = true;
  }

  bool CProgramInfoScanner::DoScan(const std::string& strDirectory)
  {
    ScraperPtr& scraper = m_database.GetScraperForPath(strDirectory);
    return DoScraping(strDirectory, scraper);
  }

  void GetArtwork(CFileItem *item)
  {
    std::string strArtworkPath = URIUtils::GetParentPath(item->GetPath());
    strArtworkPath = URIUtils::AddFileToFolder(strArtworkPath, "_resources", "artwork");

    CFileItemList items;
    CDirectory::GetDirectory(strArtworkPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_DEFAULTS);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];
      if (items[i]->m_bIsFolder)
        continue;

      std::string strFileName = URIUtils::GetFileName(pItem->GetPath());
      URIUtils::RemoveExtension(strFileName);

      if (StringUtils::EqualsNoCase(strFileName, "poster") || StringUtils::EqualsNoCase(strFileName, "fanart"))
        item->SetArt(strFileName, pItem->GetPath());
    }

    for (CGUIListItem::ArtMap::const_iterator it = item->GetArt().begin(); it != item->GetArt().end(); ++it)
      CTextureCache::Get().BackgroundCacheImage(it->second);
  }

  std::string GetNFO(const CFileItem *item)
  {
    std::string strNFO;

    std::string strPath = item->m_bIsFolder ? item->GetPath() : URIUtils::GetDirectory(item->GetPath());
    if (!item->m_bIsFolder)
    {
      strNFO = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
      if (CFile::Exists(strNFO))
        return strNFO;

      strNFO = URIUtils::AddFileToFolder(strPath, "program.nfo");
      if (CFile::Exists(strNFO))
        return strNFO;

      strNFO = URIUtils::AddFileToFolder(strPath, "_resources", "default.xml");
      if (CFile::Exists(strNFO))
        return strNFO;
    }

    return "";
  }

  bool CProgramInfoScanner::DoScraping(const std::string& strDirectory, const ScraperPtr& scraper, bool recursive /* = false */)
  {
    if (m_handle)
      m_handle->SetText(g_localizeStrings.Get(20415));

    int idPath = -1;
    if (recursive)
      idPath = m_database.GetPathId(URIUtils::GetParentPath(strDirectory));
    else
      idPath = m_database.AddPath(strDirectory);

    if (idPath < 0)
      return false;

    CFileItemList items;
    if(!CDirectory::GetDirectory(strDirectory, items, g_advancedSettings.m_programExtensions, DIR_FLAG_DEFAULTS))
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if (item->m_bIsFolder && recursive)
        continue;

      if (item->m_bIsFolder && !recursive)
      {
        DoScraping(item->GetPath(), scraper, true);
        if (m_handle)
          m_handle->SetPercentage(i * 100.f / items.Size());
        continue;
      }

      std::string strTemp = URIUtils::GetFileName(item->GetPath());
      URIUtils::RemoveExtension(strTemp);
      if (!StringUtils::EqualsNoCase(strTemp, "default"))
        continue;

      strTemp = strDirectory;
      URIUtils::RemoveSlashAtEnd(strTemp);
      strTemp = URIUtils::GetFileName(strTemp);

      if (m_handle)
        m_handle->SetTitle(strTemp);

      if (m_database.AddProgram(item->GetPath(), idPath) < 0)
        return false;

      CProgramInfoTag tag;
      tag.SetFileNameAndPath(item->GetPath());

      if (scraper->ID() == "metadata.local")
      {
        CXBMCTinyXML doc;
        std::string strNFO = GetNFO(item.get());
        if (!strNFO.empty() && doc.LoadFile(strNFO) && doc.RootElement())
          tag.Load(doc.RootElement());
      }
      else
      {
        scraper->GetProgramDetails(item->GetPath(), tag);
      }

      // set default values if not present
      if (tag.m_strFileNameAndPath.empty())
        tag.SetFileNameAndPath(item->GetPath());
      if (tag.m_type.empty())
        tag.SetType("game");
      if (tag.m_strSystem.empty())
        tag.SetSystem("xbox");
      if (tag.m_strTitle.empty())
        tag.SetTitle(strTemp);

      CFileItemPtr pItem(new CFileItem(tag));
      GetArtwork(pItem.get());
      m_database.SetDetailsForItem(pItem.get());
    }

    return true;
  }
}
