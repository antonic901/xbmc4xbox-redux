/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramInfoScanner.h"

#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "windows/GUIWindowFileManager.h"

using namespace XFILE;

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
    return DoScraping(strDirectory);
  }

  bool CProgramInfoScanner::DoScraping(const std::string& strDirectory, bool recursive /* = false */)
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
        DoScraping(item->GetPath(), true);
        continue;
      }

      std::string strFilename = URIUtils::GetFileName(item->GetPath());
      URIUtils::RemoveExtension(strFilename);
      if (!StringUtils::EqualsNoCase(strFilename, "default"))
        continue;

      if (m_handle)
        m_handle->SetTitle(strDirectory);

      if (m_database.AddProgram(item->GetPath(), idPath) < 0)
        return false;

      std::string strRootPath = strDirectory;
      std::string strNFO = URIUtils::AddFileToFolder(strRootPath, "_resources", "default.xml");

      CXBMCTinyXML doc;
      if (doc.LoadFile(strNFO) && doc.RootElement())
      {
        const TiXmlElement* element = doc.RootElement();
        std::string value;

        CFileItemPtr pItem(new CFileItem());
        pItem->SetPath(item->GetPath());
        pItem->SetProperty("type", "game");
        pItem->SetProperty("system", "xbox");

        if (XMLUtils::GetString(element, "type", value))
          pItem->SetProperty("type", value);

        if (XMLUtils::GetString(element, "system", value))
          pItem->SetProperty("system", value);

        if (item->IsXBE())
        {
          unsigned int xbeID = CUtil::GetXbeID(item->GetPath());
          std::stringstream ss;
          ss << std::hex << std::uppercase << xbeID;
          pItem->SetProperty("uniqueid", ss.str());
        }

        if (XMLUtils::GetString(element, "title", value))
          pItem->SetProperty("title", value);
        else
        {
          if (item->IsXBE())
            CUtil::GetXBEDescription(item->GetPath(), value);
          else
            value = URIUtils::GetFileName(URIUtils::GetParentPath(item->GetPath()));
          pItem->SetProperty("title", value);
        }

        if (XMLUtils::GetString(element, "overview", value))
          pItem->SetProperty("overview", value);

        value = URIUtils::AddFileToFolder(strRootPath, "_resources", "media", "preview.mp4");
        if (!CFile::Exists(value))
        {
          value = URIUtils::AddFileToFolder(strRootPath, "_resources", "artwork", "preview.xmv");
          if (!CFile::Exists(value))
            value.clear();
        }
        if (!value.empty())
          pItem->SetProperty("trailer", value);

        value = URIUtils::AddFileToFolder(strRootPath, "_resources", "artwork", "poster.jpg");
        if (!CFile::Exists(value))
        {
          value = URIUtils::AddFileToFolder(strRootPath, "_resources", "artwork", "poster.png");
          if (!CFile::Exists(value))
            value.clear();
        }
        if (!value.empty())
          pItem->SetArt("poster", value);

        value = URIUtils::AddFileToFolder(strRootPath, "_resources", "artwork", "fanart.jpg");
        if (!CFile::Exists(value))
        {
          value = URIUtils::AddFileToFolder(strRootPath, "_resources", "artwork", "fanart.png");
          if (!CFile::Exists(value))
            value.clear();
        }
        if (!value.empty())
          pItem->SetArt("fanart", value);

        int64_t iSize = CGUIWindowFileManager::CalculateFolderSize(strRootPath);
        if (iSize > 0)
          pItem->SetProperty("size", iSize);

        m_database.SetDetailsForItem(*pItem);

        if (m_handle)
          m_handle->SetPercentage(i * 100.f / items.Size());
      }
    }

    return true;
  }
}
