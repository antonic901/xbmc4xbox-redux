/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "GUIDialogProgramSettings.h"

#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "programs/ProgramDatabase.h"
#include "programs/launchers/ProgramLauncher.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "Util.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "xbox/xbeheader.h"


#define SETTING_EXECUTABLE            "programexecutable"
#define SETTING_FORCEREGION           "programforceregion"
#define SETTING_TRAINER_LIST          "trainerlist"
#define SETTING_TRAINER_HACKS         "trainerchoosehacks"

using namespace std;

CGUIDialogProgramSettings::CGUIDialogProgramSettings(void)
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PROGRAM_SETTINGS, "DialogSettings.xml"),
      m_trainer(nullptr),
      m_iTitleId(0)
{
  m_trainers.clear();
  m_trainerOptions.clear();
  m_selectedTrainerOptions.clear();
  m_strExecutable.clear();
}

CGUIDialogProgramSettings::~CGUIDialogProgramSettings(void)
{
}

bool CGUIDialogProgramSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Reset();
      break;
    }

    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_SETTINGS_CUSTOM_BUTTON)
      {
        SaveProgramSettings();
        return true;
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogProgramSettings::LoadSettings(const std::string& strExecutable, SProgramSettings& programSettings)
{
  CProgramDatabase database;
  if (database.Open())
  {
    bool isXBE = URIUtils::HasExtension(strExecutable, ".xbe");

    std::string strSettings;
    if (database.GetProgramSettings(strExecutable, strSettings) && !strSettings.empty())
    {
      CXBMCTinyXML xmlSettings;
      if (xmlSettings.Parse(strSettings) &&
          xmlSettings.RootElement() &&
          xmlSettings.RootElement()->ValueStr() == "settings")
      {
        TiXmlElement *element = xmlSettings.RootElement();
        XMLUtils::GetString(element, SETTING_EXECUTABLE, programSettings.strExecutable);
        if (isXBE)
          XMLUtils::GetInt(element, SETTING_FORCEREGION, programSettings.iForceRegion);
      }
    }
  }
  // set default values
  if (programSettings.strExecutable.empty())
    programSettings.strExecutable = URIUtils::GetFileName(strExecutable);
}

int CGUIDialogProgramSettings::GetXBERegion(const std::string& strExecutable, bool forceAllRegions /* = false */)
{
  CXBE xbe;
  int iRegion = xbe.ExtractGameRegion(strExecutable);
  if (iRegion < 1 || iRegion > 7)
    iRegion = 0;
  return CXBE::FilterRegion(iRegion, forceAllRegions);
}

void CGUIDialogProgramSettings::Reset()
{
  ResetTrainer(true);
  m_iTitleId = 0;
  m_strExecutable.clear();
  m_settings.Reset();
}

void CGUIDialogProgramSettings::ResetTrainer(bool bClearTrainers /* = false */)
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

void CGUIDialogProgramSettings::LoadProgramSettings()
{
  CProgramDatabase database;
  if (database.Open())
  {
    // Load general settings
    LoadSettings(m_strExecutable, m_settings);

    // Load trainer settings
    if (URIUtils::HasExtension(m_strExecutable, ".xbe"))
    {
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
    }
    database.Close();
  }
}

void CGUIDialogProgramSettings::SaveSettings(const std::string& strExecutable, const SProgramSettings& settings)
{
  CProgramDatabase database;
  if (database.Open())
  {
    bool isXBE = URIUtils::HasExtension(strExecutable, ".xbe");

    // save general settings
    TiXmlDocument xmlSettings;
    TiXmlElement xmlRootElement("settings");
    TiXmlNode *pRoot = xmlSettings.InsertEndChild(xmlRootElement);
    if (pRoot)
    {
      XMLUtils::SetString(pRoot, SETTING_EXECUTABLE, settings.strExecutable);
      if (isXBE)
        XMLUtils::SetInt(pRoot, SETTING_FORCEREGION, settings.iForceRegion);

      TiXmlPrinter printer;
      printer.SetIndent("");
      xmlSettings.Accept(&printer);
      database.SetProgramSettings(strExecutable, printer.CStr());
    }
  }
}

void CGUIDialogProgramSettings::SaveProgramSettings()
{
  CProgramDatabase database;
  if (database.Open())
  {
    // save general settings
    SaveSettings(m_strExecutable, m_settings);

    // save trainer settings
    if (URIUtils::HasExtension(m_strExecutable, ".xbe"))
      database.SetTrainer(m_iTitleId, m_trainer);
  }
}

void CGUIDialogProgramSettings::IntegerOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogProgramSettings *programSettings = static_cast<CGUIDialogProgramSettings*>(data);

  if (setting->GetId() == SETTING_FORCEREGION)
  {
    int iRegion = GetXBERegion(programSettings->m_strExecutable, true);
    list.push_back(make_pair(g_localizeStrings.Get(16316), 0));
    list.push_back(make_pair(iRegion == VIDEO_NTSCM ? "NTSC-M (default)" : "NTSC-M", VIDEO_NTSCM));
    list.push_back(make_pair(iRegion == VIDEO_NTSCJ ? "NTSC-J (default)" : "NTSC-J", VIDEO_NTSCJ));
    list.push_back(make_pair(iRegion == VIDEO_PAL50 ? "PAL (default)" : "PAL", VIDEO_PAL50));
    list.push_back(make_pair(iRegion == VIDEO_PAL60 ? "PAL60 (default)" : "PAL60", VIDEO_PAL60));
  }
  else if (setting->GetId() == SETTING_TRAINER_LIST)
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), 0));
    for (std::vector<CTrainer*>::const_iterator it = programSettings->m_trainers.begin(); it != programSettings->m_trainers.end(); ++it)
      list.push_back(make_pair((*it)->GetName(), (*it)->GetTrainerId()));
  }
}

void CGUIDialogProgramSettings::StringOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogProgramSettings *programSettings = static_cast<CGUIDialogProgramSettings*>(data);

  if (setting->GetId() == SETTING_EXECUTABLE)
  {
    CFileItemList items;
    XFILE::CDirectory::GetDirectory(URIUtils::GetParentPath(programSettings->m_strExecutable), items, URIUtils::GetExtension(programSettings->m_strExecutable), XFILE::DIR_FLAG_DEFAULTS);
    for (int i = 0; i < items.Size(); ++i)
    {
      if (!items[i]->m_bIsFolder)
      {
        std::string strLabel = URIUtils::GetFileName(items[i]->GetPath());
        list.push_back(std::pair<std::string, std::string>(strLabel, strLabel));
      }
    }
  }
}

void CGUIDialogProgramSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_EXECUTABLE)
  {
    m_settings.strExecutable = ((CSettingString*)setting)->GetValue();
  }
  else if (settingId == SETTING_FORCEREGION)
  {
    m_settings.iForceRegion = ((CSettingInt*)setting)->GetValue();
  }
  else if (settingId == SETTING_TRAINER_LIST)
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

void CGUIDialogProgramSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(38996);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CANCEL_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 518);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CUSTOM_BUTTON, 190);
}

void CGUIDialogProgramSettings::OnSettingAction(const CSetting *setting)
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

void CGUIDialogProgramSettings::Save()
{
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevelExpert))
    return;

  SaveProgramSettings();

  LAUNCHERS::CProgramLauncher::LaunchProgram(m_strExecutable);
}

void CGUIDialogProgramSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("xbelauncher", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProgramSettings: unable to setup xbelauncher");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProgramSettings: unable to setup xbelauncher");
    return;
  }

  bool isXBE = URIUtils::HasExtension(m_strExecutable, ".xbe");
  if (!m_iTitleId)
    m_iTitleId = isXBE ? CUtil::GetXbeID(m_strExecutable) : 0;

  LoadProgramSettings();

  AddSpinner(group, SETTING_EXECUTABLE, 655, 0, m_settings.strExecutable, StringOptionsFiller, true);
  if (isXBE)
  { // Xbox (XBE) executable
    AddList(group, SETTING_FORCEREGION, 20026, 0, m_settings.iForceRegion, IntegerOptionsFiller, 20026);
    AddList(group, SETTING_TRAINER_LIST, 38710, 0, m_trainer ? m_trainer->GetTrainerId() : 0, IntegerOptionsFiller, 38710);
    CSettingAction *btnHacks = AddButton(group, SETTING_TRAINER_HACKS, 38711, 0);

    CSettingDependency dependencyHacks(SettingDependencyTypeEnable, m_settingsManager);
    dependencyHacks.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TRAINER_LIST, "0", SettingDependencyOperatorEquals, true, m_settingsManager)));

    SettingDependencies deps;
    deps.push_back(dependencyHacks);
    btnHacks->SetDependencies(deps);
    deps.clear();
  }
}

void CGUIDialogProgramSettings::ShowForTitle(const CFileItemPtr pItem)
{
  CGUIDialogProgramSettings *dialog = (CGUIDialogProgramSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SETTINGS);
  if (dialog == nullptr)
    return;

  // initialize and show the dialog
  dialog->Initialize();
  dialog->SetExecutable(pItem->GetPath());
  dialog->Open();
}

