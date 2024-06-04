#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CProgramDatabase;
class CTrainer;

class CGUIDialogTrainerSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogTrainerSettings();
  virtual ~CGUIDialogTrainerSettings();

  static bool ShowForTitle(unsigned int iTitleId, CProgramDatabase* database);

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

private:
  std::vector<CTrainer*> m_vecTrainers;
  std::vector<std::string> m_vecOptions;
  CProgramDatabase* m_database;

  int m_iTrainer;
  int m_iOldTrainer;
  unsigned int m_iTitleId;
  bool m_bNeedSave;
  bool m_bCanceled;
  std::string m_strActive; // active trainer at start - to save db work
};

