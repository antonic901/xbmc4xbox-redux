/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ContextMenus.h"

#include "addons/Addon.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/AddonsDirectory.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
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

  return URIUtils::HasExtension(item.GetPath(), g_advancedSettings.m_programExtensions);
}

bool CProgramInfoBase::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  std::string strRootPath = URIUtils::GetParentPath(item->GetPath());
  std::string strTempPath = URIUtils::AddFileToFolder(strRootPath, "_resources", "default.xml");
  CXBMCTinyXML doc;
  if (doc.LoadFile(strTempPath) && doc.RootElement())
  {
    const TiXmlElement* synopsis = doc.RootElement();
    std::string value;
    float fValue;
    int iValue;

    if (XMLUtils::GetString(synopsis, "developer", value))
      item->SetProperty("developer", value);

    if (XMLUtils::GetString(synopsis, "publisher", value))
      item->SetProperty("publisher", value);

    if (XMLUtils::GetString(synopsis, "features_general", value))
      item->SetProperty("features_general", value);

    if (XMLUtils::GetString(synopsis, "features_online", value))
      item->SetProperty("features_online", value);

    if (XMLUtils::GetString(synopsis, "esrb", value))
      item->SetProperty("esrb", value);

    if (XMLUtils::GetString(synopsis, "esrb_descriptors", value))
      item->SetProperty("esrb_descriptors", value);

    if (XMLUtils::GetString(synopsis, "genre", value))
      item->SetProperty("genre", value);

    if (XMLUtils::GetString(synopsis, "release_date", value))
      item->SetProperty("release_date", value);

    if (XMLUtils::GetInt(synopsis, "year", iValue))
      item->SetProperty("year", iValue);

    if (XMLUtils::GetFloat(synopsis, "rating", fValue))
      item->SetProperty("rating", fValue);

    if (XMLUtils::GetString(synopsis, "platform", value))
      item->SetProperty("platform", value);

    if (XMLUtils::GetString(synopsis, "exclusive", value))
      item->SetProperty("exclusive", value);
  }

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
}
