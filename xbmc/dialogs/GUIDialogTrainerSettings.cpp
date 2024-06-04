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

#include "GUIDialogTrainerSettings.h"
#include "GUIPassword.h"
#include "ProgramDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "utils/Trainer.h"

using namespace std;

CGUIDialogTrainerSettings::CGUIDialogTrainerSettings(void)
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_TRAINER_SETTINGS, "TrainerSettings.xml"),
      m_database(NULL),
      m_iTrainer(0),
      m_iOldTrainer(0),
      m_bNeedSave(false),
      m_bCanceled(false)
{ }

CGUIDialogTrainerSettings::~CGUIDialogTrainerSettings(void)
{ }

void CGUIDialogTrainerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  // TODO: implement this
}

void CGUIDialogTrainerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  // TODO: implement this
}

void CGUIDialogTrainerSettings::Save()
{
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevelExpert))
    return;

  // TODO: implement this
}

void CGUIDialogTrainerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  // TODO: implement this
}

bool CGUIDialogTrainerSettings::ShowForTitle(unsigned int iTitleId, CProgramDatabase* database)
{
  // CGUIDialogTrainerSettings *dialog = (CGUIDialogTrainerSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_TRAINER_SETTINGS);
  // if (!dialog) return false;
  // dialog->m_iTitleId = iTitleId;
  // dialog->m_database = database;
  // dialog->Open();
  // if (dialog->m_bNeedSave)
  //   return true;

  return false;
}

