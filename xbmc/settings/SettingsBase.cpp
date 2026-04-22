/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsBase.h"

#include "settings/SettingUtils.h"
#include "settings/SettingsValueXmlSerializer.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"

#include <boost/move/unique_ptr.hpp>
#include <boost/move/make_unique.hpp>

#define SETTINGS_XML_ROOT   "settings"

CSettingsBase::CSettingsBase()
  : m_settingsManager(new CSettingsManager()), m_initialized(false)
{ }

CSettingsBase::~CSettingsBase()
{
  Uninitialize();

  delete m_settingsManager;
}

bool CSettingsBase::Initialize()
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  InitializeVisibility();
  InitializeDefaults();

  m_settingsManager->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

bool CSettingsBase::IsInitialized() const
{
  return m_initialized && m_settingsManager->IsInitialized();
}

bool CSettingsBase::LoadValuesFromXml(const CXBMCTinyXML& xml, bool& updated)
{
  const TiXmlElement* xmlRoot = xml.RootElement();
  if (xmlRoot == NULL || xmlRoot->ValueStr() != SETTINGS_XML_ROOT)
    return false;

  return m_settingsManager->Load(xmlRoot, updated);
}

bool CSettingsBase::LoadValuesFromXml(const TiXmlElement* root, bool& updated)
{
  if (root == NULL)
    return false;

  return m_settingsManager->Load(root, updated);
}

bool CSettingsBase::LoadHiddenValuesFromXml(const TiXmlElement* root)
{
  if (root == NULL)
    return false;

  std::map<std::string, boost::shared_ptr<CSetting> > loadedSettings;

  bool updated;
  // don't trigger events for hidden settings
  bool success = m_settingsManager->Load(root, updated, false, &loadedSettings);
  if (success)
  {
    for(std::map<std::string, boost::shared_ptr<CSetting> >::const_iterator setting = loadedSettings.begin(); setting != loadedSettings.end(); ++setting)
      setting->second->SetVisible(false);
  }

  return success;
}

void CSettingsBase::SetLoaded()
{
  m_settingsManager->SetLoaded();
}

bool CSettingsBase::IsLoaded() const
{
  return m_settingsManager->IsLoaded();
}

bool CSettingsBase::SaveValuesToXml(CXBMCTinyXML& xml) const
{
  std::string serializedSettings;
  boost::movelib::unique_ptr<CSettingsValueXmlSerializer> xmlSerializer = boost::movelib::make_unique<CSettingsValueXmlSerializer>();
  if (!m_settingsManager->Save(xmlSerializer.get(), serializedSettings))
    return false;

  if (!xml.Parse(serializedSettings))
    return false;

  return true;
}

void CSettingsBase::Unload()
{
  m_settingsManager->Unload();
}

void CSettingsBase::Uninitialize()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  // unregister setting option fillers
  UninitializeOptionFillers();

  // unregister setting conditions
  UninitializeConditions();

  // unregister ISettingCallback implementations
  UninitializeISettingCallbacks();

  // cleanup the settings manager
  m_settingsManager->Clear();

  // unregister ISubSettings implementations
  UninitializeISubSettings();
  // unregister ISettingsHandler implementations
  UninitializeISettingsHandlers();

  m_initialized = false;
}

void CSettingsBase::RegisterCallback(ISettingCallback* callback, const std::set<std::string>& settingList)
{
  m_settingsManager->RegisterCallback(callback, settingList);
}

void CSettingsBase::UnregisterCallback(ISettingCallback* callback)
{
  m_settingsManager->UnregisterCallback(callback);
}

SettingPtr CSettingsBase::GetSetting(const std::string& id) const
{
  if (id.empty())
    return SettingPtr();

  return m_settingsManager->GetSetting(id);
}

std::vector<boost::shared_ptr<CSettingSection> > CSettingsBase::GetSections() const
{
  return m_settingsManager->GetSections();
}

boost::shared_ptr<CSettingSection> CSettingsBase::GetSection(const std::string& section) const
{
  if (section.empty())
    return boost::shared_ptr<CSettingSection>();

  return m_settingsManager->GetSection(section);
}

bool CSettingsBase::GetBool(const std::string& id) const
{
  return m_settingsManager->GetBool(id);
}

bool CSettingsBase::SetBool(const std::string& id, bool value)
{
  return m_settingsManager->SetBool(id, value);
}

bool CSettingsBase::ToggleBool(const std::string& id)
{
  return m_settingsManager->ToggleBool(id);
}

int CSettingsBase::GetInt(const std::string& id) const
{
  return m_settingsManager->GetInt(id);
}

bool CSettingsBase::SetInt(const std::string& id, int value)
{
  return m_settingsManager->SetInt(id, value);
}

double CSettingsBase::GetNumber(const std::string& id) const
{
  return m_settingsManager->GetNumber(id);
}

bool CSettingsBase::SetNumber(const std::string& id, double value)
{
  return m_settingsManager->SetNumber(id, value);
}

std::string CSettingsBase::GetString(const std::string& id) const
{
  return m_settingsManager->GetString(id);
}

bool CSettingsBase::SetString(const std::string& id, const std::string& value)
{
  return m_settingsManager->SetString(id, value);
}

std::vector<CVariant> CSettingsBase::GetList(const std::string& id) const
{
  boost::shared_ptr<CSetting> setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::List)
    return std::vector<CVariant>();

  return CSettingUtils::GetList(boost::static_pointer_cast<CSettingList>(setting));
}

bool CSettingsBase::SetList(const std::string& id, const std::vector<CVariant>& value)
{
  boost::shared_ptr<CSetting> setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::List)
    return false;

  return CSettingUtils::SetList(boost::static_pointer_cast<CSettingList>(setting), value);
}

bool CSettingsBase::SetDefault(const std::string &id)
{
  return m_settingsManager->SetDefault(id);
}

void CSettingsBase::SetDefaults()
{
  m_settingsManager->SetDefaults();
}

bool CSettingsBase::InitializeDefinitionsFromXml(const CXBMCTinyXML& xml)
{
  const TiXmlElement* root = xml.RootElement();
  if (root == NULL)
    return false;

  return m_settingsManager->Initialize(root);
}
