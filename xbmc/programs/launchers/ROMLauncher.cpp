/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ROMLauncher.h"

#include "dialogs/GUIDialogSelect.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "programs/ProgramDatabase.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "settings/AdvancedSettings.h"
#include "Shortcut.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "Util.h"

using namespace LAUNCHERS;

#define CUSTOM_LAUNCH "special://temp/emu_launch.xbe"

SystemMapping Systems[] = {
                            {"Nintendo Entertainment System", "nes", ".nes"},
                            {"Sega Master System", "mastersystem", ".sms"},
                            {"Sega Megadrive / Genesis", "megadrive|genesis", ".md"},
                            {"Super Nintendo Entertainment System", "snes", ".sfc"}
                          };

CROMLauncher::CROMLauncher(std::string strExecutable)
{
  m_strExecutable = strExecutable;
  m_database = new CProgramDatabase();
  m_settings = new SProgramSettings();
}

CROMLauncher::~CROMLauncher(void)
{
  delete m_database;
  delete m_settings;
}

bool CROMLauncher::LoadSettings()
{
  if (URIUtils::IsOnDVD(m_strExecutable))
    return true;

  CGUIDialogProgramSettings::LoadSettings(m_strExecutable, *m_settings);
  return true;
}

bool CROMLauncher::IsSupported()
{
  if (URIUtils::HasExtension(m_strExecutable, ".xbe"))
    return false;

  return URIUtils::HasExtension(m_strExecutable, g_advancedSettings.m_programExtensions);
}

CFileItemPtr CROMLauncher::GetDefaultEmulator()
{
  if (m_settings->strEmulator.empty())
    return CFileItemPtr();

  if (!XFILE::CFile::Exists(m_settings->strEmulator))
    return CFileItemPtr();

  return CFileItemPtr(new CFileItem(m_settings->strEmulator));
}

bool CROMLauncher::FindEmulators(const std::string strRomFile, CFileItemList& emulators)
{
  CProgramDatabase database;
  if (!database.Open())
    return false;

  for (unsigned int i = 0; i < sizeof(Systems) / sizeof(SystemMapping); ++i)
  {
    std::vector<std::string> shortnames = StringUtils::Split(Systems[i].shortname, "|");
    for (std::vector<std::string>::iterator it = shortnames.begin(); it != shortnames.end(); ++it)
    {
      if (strRomFile.find(*it) != std::string::npos && URIUtils::HasExtension(strRomFile, Systems[i].extension))
        return database.GetEmulators(Systems[i].shortname, emulators);
    }
  }

  return false;
}

bool CROMLauncher::Launch()
{
  if (!m_database->Open())
    return false;

  if (!IsSupported())
    return false;

  LoadSettings();

  // Get emulator for this ROM
  CFileItemPtr emulator = GetDefaultEmulator();
  if (!emulator)
  {
    CFileItemList emulators;
    if (!FindEmulators(m_strExecutable, emulators))
    {
      CLog::Log(LOGINFO, "Emulator for '%s' is not installed", m_strExecutable.c_str());
      return false;
    }

    emulator = emulators[0];
    if (emulators.Size() > 1)
    { // let the user to choose if there is more then one
      CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
      dialog->Reset();
      dialog->SetHeading(22080);
      dialog->SetItems(emulators);
      dialog->Open();
      if (dialog->GetSelectedItem() < 0)
        return false;
      emulator = dialog->GetSelectedFileItem();
    }

    m_settings->strEmulator = emulator->GetPath();
    CGUIDialogProgramSettings::SaveSettings(m_strExecutable, *m_settings);
  }

  std::string strExecutable = m_strExecutable;

  // look for default executable
  if (!m_settings->strExecutable.empty())
  {
    std::string strParentPath = URIUtils::GetParentPath(m_strExecutable);
    strExecutable = URIUtils::AddFileToFolder(strParentPath, m_settings->strExecutable);
  }

  if (!URIUtils::IsOnDVD(m_strExecutable))
    m_database->UpdateLastPlayed(m_strExecutable);

  // Launch ROM
  CShortcut shortcut;
  shortcut.m_strPath = m_settings->strEmulator.c_str();
  shortcut.m_strCustomGame = strExecutable.c_str();
  shortcut.Save(CUSTOM_LAUNCH);
  CUtil::RunShortcut(CUSTOM_LAUNCH);
  return true;
}
