/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "XBELauncher.h"

#include "FileItem.h"
#include "programs/ProgramDatabase.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "Util.h"
#include "utils/FilterFlickerPatch.h"
#include "utils/log.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "xbox/xbeheader.h"

using namespace LAUNCHERS;

CXBELauncher::CXBELauncher(std::string strExecutable)
{
  m_strExecutable = strExecutable;
  m_trainer = nullptr;
  m_database = new CProgramDatabase();
  m_settings = new SProgramSettings();
}

CXBELauncher::~CXBELauncher(void)
{
  if (m_trainer)
    delete m_trainer;
  delete m_database;
  delete m_settings;
}

bool CXBELauncher::LoadSettings()
{
  if (URIUtils::IsOnDVD(m_strExecutable))
    return true;

  CGUIDialogProgramSettings::LoadSettings(m_strExecutable, *m_settings);
  return true;
}

bool CXBELauncher::IsSupported()
{
  return URIUtils::HasExtension(m_strExecutable, ".xbe");
}

bool CXBELauncher::ApplyFFPatch(const std::string& strExecutable, std::string& strPatchedExecutable)
{
  RESOLUTION res = CDisplaySettings::Get().GetCurrentResolution();
  if (res == RES_HDTV_480p_4x3 ||
      res == RES_HDTV_480p_16x9 ||
      res == RES_HDTV_720p)
  {
    CLog::Log(LOGDEBUG, "%s - Progressive Mode detected: Skipping Filter Flicker Patching!", __FUNCTION__);
    return false;
  }

  if (URIUtils::IsOnDVD(strExecutable))
  {
    CLog::Log(LOGDEBUG, "%s - Source is DVD-ROM: Skipping Filter Flicker Patching!", __FUNCTION__);
    return false;
  }

  CXBE m_xbe;
  if((int)m_xbe.ExtractGameRegion(strExecutable.c_str()) <= 0) // Reading the GameRegion is enought to detect a Patchable xbe!
  {
    CLog::Log(LOGDEBUG, "%s - Not Patchable xbe detected (Homebrew?): Skipping Filter Flicker Patching!", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Starting Filter Flicker Patching...", __FUNCTION__);
  CGFFPatch m_ffp;
  if (!m_ffp.FFPatch(strExecutable, strPatchedExecutable))
  {
    CLog::Log(LOGERROR, "%s - Filter Flicker Patching failed!", __FUNCTION__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s - Filter Flicker Patching done. Saved to %s.", __FUNCTION__, strPatchedExecutable.c_str());
  return true;
}

CTrainer* CXBELauncher::LoadTrainer(unsigned int iTitleID)
{
  CProgramDatabase database;
  if (!database.Open())
    return nullptr;

  CFileItemList items;
  if (database.GetTrainers(items, iTitleID))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->GetProperty("isactive").asBoolean())
      {
        CTrainer* trainer = new CTrainer(items[i]->GetProperty("idtrainer").asInteger32());
        if (trainer->Load(items[i]->GetPath()) &&
            database.GetTrainerOptions(trainer->GetTrainerId(), iTitleID, trainer->GetOptions(), trainer->GetNumberOfOptions()))
          return trainer;
        else
        {
          delete trainer;
          return nullptr;
        }
      }
    }
  }

  return nullptr;
}

bool CXBELauncher::Launch()
{
  if (!m_database->Open())
    return false;

  if (!IsSupported())
    return false;

  LoadSettings();

  // install trainer if available
  m_trainer = LoadTrainer(CUtil::GetXbeID(m_strExecutable));
  if (m_trainer && !CTrainer::InstallTrainer(*m_trainer))
  {
    CLog::Log(LOGERROR, "%s - Trainer could not be installed: %s", __FUNCTION__, m_trainer->GetPath());
    return false;
  }

  std::string strExecutable = m_strExecutable;

  // apply flicker filter
  if (!URIUtils::IsOnDVD(m_strExecutable) && CSettings::GetInstance().GetBool("myprograms.autoffpatch"))
  {
    std::string strPatchedExecutable;
    if (ApplyFFPatch(m_strExecutable, strPatchedExecutable))
      strExecutable = strPatchedExecutable;
  }

  // apply video mode switching
  int iRegion = m_settings->iForceRegion;
  if (!iRegion && CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
    iRegion = CGUIDialogProgramSettings::GetXBERegion(m_strExecutable);

  // look for default executable
  if (!URIUtils::IsOnDVD(m_strExecutable) && !m_settings->strExecutable.empty() && !CSettings::GetInstance().GetBool("myprograms.autoffpatch"))
  {
    std::string strParentPath = URIUtils::GetParentPath(m_strExecutable);
    strExecutable = URIUtils::AddFileToFolder(strParentPath, m_settings->strExecutable);
  }

  if (!URIUtils::IsOnDVD(m_strExecutable))
    m_database->UpdateLastPlayed(m_strExecutable);

  CUtil::RunXBE(strExecutable.c_str(), NULL, F_VIDEO(iRegion));
  return true;
}
