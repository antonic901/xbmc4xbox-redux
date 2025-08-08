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

class CSetting;
class CTrainer;

class CGUIDialogTrainerSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogTrainerSettings();
  virtual ~CGUIDialogTrainerSettings();
  virtual bool OnMessage(CGUIMessage &message);

  static void ShowForTitle(const CFileItemPtr pItem);

protected:
  static void IntegerOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  // specialization of CGUIDialogSettingsManualBase
  virtual void SetupView();

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  void SetExecutable(const std::string strExecutable) { m_strExecutable = strExecutable; }

private:
  void Reset();
  void ResetTrainer(bool bClearTrainers = false);

  std::vector<CTrainer*> m_trainers;
  std::vector<std::string> m_trainerOptions;
  std::vector<std::string> m_selectedTrainerOptions;

  CTrainer* m_trainer;
  unsigned int m_iTitleId;
  std::string m_strExecutable;
};

