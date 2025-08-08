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
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"
#include "ProgramDatabase.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/Variant.h"
#include "utils/Trainer.h"
#include "utils/log.h"
#include "Util.h"
#include "xbox/xbeheader.h"

#define SETTING_TRAINER_LIST          "trainerlist"
#define SETTING_TRAINER_HACKS         "trainerchoosehacks"

using namespace std;

CGUIDialogTrainerSettings::CGUIDialogTrainerSettings(void)
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_TRAINER_SETTINGS, "DialogSettings.xml"),
      m_trainer(nullptr),
      m_iTitleId(0)
{
  m_trainers.clear();
  m_trainerOptions.clear();
  m_selectedTrainerOptions.clear();
  m_strExecutable.clear();
}

CGUIDialogTrainerSettings::~CGUIDialogTrainerSettings(void)
{
}

bool CGUIDialogTrainerSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Reset();
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogTrainerSettings::Reset()
{
  ResetTrainer(true);
  m_iTitleId = 0;
  m_strExecutable.clear();
}

void CGUIDialogTrainerSettings::ResetTrainer(bool bClearTrainers /* = false */)
{
  if (bClearTrainers)
  {
    for (unsigned int i = 0; i < m_trainers.size(); ++i)
      delete m_trainers[i];
    m_trainers.clear();
  }
  m_trainer = nullptr;
  m_trainerOptions.clear();
  m_selectedTrainerOptions.clear();
}

void CGUIDialogTrainerSettings::IntegerOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogTrainerSettings *programSettings = static_cast<CGUIDialogTrainerSettings*>(data);

  if (setting->GetId() == SETTING_TRAINER_LIST)
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), 0));
    for (std::vector<CTrainer*>::const_iterator it = programSettings->m_trainers.begin(); it != programSettings->m_trainers.end(); ++it)
      list.push_back(make_pair((*it)->GetName(), (*it)->GetTrainerId()));
  }
}

void CGUIDialogTrainerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TRAINER_LIST)
  {
    ResetTrainer();

    const int idTrainer = ((CSettingInt*)setting)->GetValue();
    if (idTrainer == 0)
      return;

    for (std::vector<CTrainer*>::iterator it = m_trainers.begin(); it != m_trainers.end(); ++it)
    {
      CTrainer *trainer = *it;
      if (trainer->GetTrainerId() == idTrainer)
      {
        m_trainer = trainer;
        m_trainer->GetOptionLabels(m_trainerOptions);
        break;
      }
    }
  }
}

void CGUIDialogTrainerSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(38712);

  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 190);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
}

void CGUIDialogTrainerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TRAINER_HACKS)
  {
    CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect *>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
    if (dialog)
    {
      dialog->Reset();
      dialog->SetMultiSelection(true);
      dialog->SetHeading(38711);
      for (std::vector<std::string>::const_iterator it = m_trainerOptions.begin(); it != m_trainerOptions.end(); ++it)
        dialog->Add((*it));
      dialog->SetSelected(m_selectedTrainerOptions);
      dialog->Open();

      if (!dialog->IsConfirmed())
        return

      // reset to initial state
      m_selectedTrainerOptions.clear();
      unsigned char* data = m_trainer->GetOptions();
      for (int i = 0; i < m_trainer->GetNumberOfOptions(); i++)
        data[i] = 0;

      // enable selected hacks
      std::vector<int> selectedItems = dialog->GetSelectedItems();
      for (unsigned int i = 0; i < selectedItems.size(); i++)
      {
        const int index = selectedItems[i];
        data[index] = 1;
        m_selectedTrainerOptions.push_back(m_trainerOptions[index]);
      }
    }
  }
}

void CGUIDialogTrainerSettings::Save()
{
  CProgramDatabase database;
  if (database.Open())
  {
    database.SetTrainer(m_iTitleId, m_trainer);
    database.Close();
  }
}

void CGUIDialogTrainerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("xbelauncher", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogTrainerSettings: unable to setup xbelauncher");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogTrainerSettings: unable to setup xbelauncher");
    return;
  }

  if (!m_iTitleId)
    m_iTitleId = CUtil::GetXbeID(m_strExecutable);

  CProgramDatabase database;
  if (database.Open())
  {
    // Load trainer settings
    CFileItemList items;
    database.GetTrainers(items, m_iTitleId);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      CTrainer *trainer = new CTrainer(item->GetProperty("idtrainer").asInteger32());
      if (trainer->Load(item->GetPath()))
      {
        database.GetTrainerOptions(trainer->GetTrainerId(), m_iTitleId, trainer->GetOptions(), trainer->GetNumberOfOptions());
        m_trainers.push_back(trainer);
        if (item->GetProperty("isactive").asBoolean())
        {
          m_trainer = trainer;
          trainer->GetOptionLabels(m_trainerOptions);
          for (unsigned int i = 0; i < m_trainerOptions.size(); i++)
          {
            if (m_trainer->GetOptions()[i] == 1)
              m_selectedTrainerOptions.push_back(m_trainerOptions[i]);
          }
        }
      }
      else
        delete trainer;
    }
    database.Close();
  }

  AddList(group, SETTING_TRAINER_LIST, 38710, 0, m_trainer ? m_trainer->GetTrainerId() : 0, IntegerOptionsFiller, 38710);
  CSettingAction *btnHacks = AddButton(group, SETTING_TRAINER_HACKS, 38711, 0);

  CSettingDependency dependencyHacks(SettingDependencyTypeEnable, m_settingsManager);
  dependencyHacks.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TRAINER_LIST, "0", SettingDependencyOperatorEquals, true, m_settingsManager)));

  SettingDependencies deps;
  deps.push_back(dependencyHacks);
  btnHacks->SetDependencies(deps);
  deps.clear();
}

void CGUIDialogTrainerSettings::ShowForTitle(const CFileItemPtr pItem)
{
  CGUIDialogTrainerSettings *dialog = static_cast<CGUIDialogTrainerSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_TRAINER_SETTINGS));
  if (dialog)
  {
    dialog->Initialize();
    dialog->SetExecutable(pItem->GetPath());
    dialog->Open();
  }
}

