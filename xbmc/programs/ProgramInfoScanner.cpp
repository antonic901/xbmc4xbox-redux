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
    if (m_handle)
      m_handle->SetText(g_localizeStrings.Get(20415));

    int idPath = m_database.AddPath(strDirectory);
    if (idPath < 0)
      return false;

    CFileItemList items;
    if(!CDirectory::GetDirectory(strDirectory, items, ".xbe", DIR_FLAG_DEFAULTS))
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if (!item->m_bIsFolder)
        continue;

      std::string strPath = URIUtils::AddFileToFolder(item->GetPath(), "default.xbe");
      if (!CFile::Exists(strPath))
        continue;

      if (m_handle)
        m_handle->SetTitle(item->GetPath());

      if (m_database.AddProgram(strPath, idPath) < 0)
        return false;

      std::string strRootPath = item->GetPath();
      std::string strNFO = URIUtils::AddFileToFolder(strRootPath, "_resources", "default.xml");

      CXBMCTinyXML doc;
      if (doc.LoadFile(strNFO) && doc.RootElement())
      {
        const TiXmlElement* element = doc.RootElement();
        std::string value;

        CFileItemPtr pItem(new CFileItem());
        pItem->SetPath(strPath);
        pItem->SetProperty("type", "game");

        unsigned int xbeID = CUtil::GetXbeID(strPath);
        std::stringstream ss;
        ss << std::hex << std::uppercase << xbeID;    
        pItem->SetProperty("uniqueid", ss.str());

        if (XMLUtils::GetString(element, "title", value))
          pItem->SetProperty("title", value);
        else
        {
          CUtil::GetXBEDescription(strPath, value);
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
