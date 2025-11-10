/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ContextMenus.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/Scraper.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "programs/ProgramDatabase.h"
#include "programs/ProgramLibraryQueue.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"


namespace CONTEXTMENU
{

CProgramInfoBase::CProgramInfoBase()
  : CStaticContextMenuAction(19033)
{
}

bool CProgramInfoBase::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return false;

  return item.HasProgramInfoTag();
}

bool CProgramInfoBase::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CGUIDialogProgramInfo *dialog = static_cast<CGUIDialogProgramInfo*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_INFO));
  dialog->SetProgram(item.get());
  dialog->Open();
  return true;
}

CProgramSettings::CProgramSettings()
  : CStaticContextMenuAction(519)
{
}

bool CProgramSettings::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return false;

  return URIUtils::HasExtension(item.GetPath(), g_advancedSettings.m_programExtensions);
}

bool CProgramSettings::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CGUIDialogProgramSettings::ShowForTitle(item);
  return true;
}

CScriptLaunch::CScriptLaunch()
  : CStaticContextMenuAction(247)
{
}

bool CScriptLaunch::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return URIUtils::IsDOSPath(item.GetPath());

  return URIUtils::HasExtension(item.GetPath(), g_advancedSettings.m_programExtensions);
}

bool CScriptLaunch::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  ADDON::VECADDONS addons;
  if (XFILE::CAddonsDirectory::GetScriptsAndPlugins("executable", addons) && addons.size())
  {
    CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
    if (dialog)
    {
      dialog->SetHeading(247);
      dialog->Reset();
      for (ADDON::VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
      {
        std::string strOption = StringUtils::Format("%s (%s)", (*it)->Name().c_str(), (*it)->Author().c_str());
        dialog->Add(strOption);
      }
      dialog->Open();

      int iSelected = dialog->GetSelectedItem();
      if (!dialog->IsConfirmed() || iSelected < 0)
        return true;

      std::string strPath = item->GetPath();
      std::string strParentPath = item->m_bIsFolder ? item->GetPath() : URIUtils::GetParentPath(strPath);

      std::vector<std::string> argv;
      argv.push_back(strPath);
      argv.push_back(strParentPath);

      ADDON::AddonPtr addon = addons[iSelected];
      CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon, argv);

      return true;
    }
  }

  CGUIDialogKaiToast::QueueNotification(StringUtils::Format(g_localizeStrings.Get(13328).c_str(), g_localizeStrings.Get(247).c_str()), g_localizeStrings.Get(161));
  return false;
};

CScraperConfig::CScraperConfig()
  : CStaticContextMenuAction(10132)
{
}

bool CScraperConfig::IsVisible(const CFileItem& item) const
{
  return item.m_bIsFolder && URIUtils::IsDOSPath(item.GetPath());
}

bool CScraperConfig::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CProgramDatabase database;
  if (!database.Open())
    return false;

  std::string currentScraperId;
  ADDON::ScraperPtr scraper = database.GetScraperForPath(item->GetPath());
  if (scraper)
    currentScraperId = scraper->ID();
  std::string selectedAddonId = currentScraperId;

  if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::ADDON_SCRAPER_PROGRAMS, selectedAddonId, false) == 1
      && selectedAddonId != currentScraperId)
  {
    ADDON::AddonPtr scraperAddon;
    CServiceBroker::GetAddonMgr().GetAddon(selectedAddonId, scraperAddon);
    scraper = boost::dynamic_pointer_cast<ADDON::CScraper>(scraperAddon);
    database.SetScraperForPath(item->GetPath(), scraper);
  }

  return true;
};

CContentScan::CContentScan()
  : CStaticContextMenuAction(13349)
{
}

bool CContentScan::IsVisible(const CFileItem& item) const
{
  if (!item.m_bIsFolder || !item.IsHD())
    return false;

  CProgramDatabase database;
  if (!database.Open())
    return false;

  ADDON::ScraperPtr scraper = database.GetScraperForPath(item.GetPath());
  return scraper != NULL;
}

bool CContentScan::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CProgramLibraryQueue::GetInstance().ScanLibrary(item->GetPath());
  return true;
};
}
