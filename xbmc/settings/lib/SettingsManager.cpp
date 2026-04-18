/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsManager.h"

#include "ServiceBroker.h"
#include "Setting.h"
#include "SettingDefinitions.h"
#include "SettingSection.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <boost/make_shared.hpp>
#include <utility>

const uint32_t CSettingsManager::Version = 2;
const uint32_t CSettingsManager::MinimumSupportedVersion = 0;

bool ParseSettingIdentifier(const std::string& settingId, std::string& categoryTag, std::string& settingTag)
{
  static const std::string Separator = ".";

  if (settingId.empty())
    return false;

  std::vector<std::string> parts = StringUtils::Split(settingId, Separator);
  if (parts.size() < 1 || parts.at(0).empty())
    return false;

  if (parts.size() == 1)
  {
    settingTag = parts.at(0);
    return true;
  }

  // get the category tag and remove it from the parts
  categoryTag = parts.at(0);
  parts.erase(parts.begin());

  // put together the setting tag
  settingTag = StringUtils::Join(parts, Separator);

  return true;
}

CSettingsManager::CSettingsManager()
  : m_initialized(false), m_loaded(false)
{
}

CSettingsManager::~CSettingsManager()
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_settingCreators.clear();
  m_settingControlCreators.clear();

  Clear();
}

uint32_t CSettingsManager::ParseVersion(const TiXmlElement* root) const
{
  // try to get and check the version
  uint32_t version = 0;
  root->QueryUnsignedAttribute(SETTING_XML_ROOT_VERSION, &version);

  return version;
}

bool CSettingsManager::Initialize(const TiXmlElement *root)
{
  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);
  if (m_initialized || root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTING_XML_ROOT))
  {
    CLog::Log(LOGERROR, "error reading settings definition: doesn't contain <" SETTING_XML_ROOT
                    "> tag");
    return false;
  }

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    CLog::Log(LOGWARNING, "missing " SETTING_XML_ROOT_VERSION " attribute", SETTING_XML_ROOT_VERSION);

  if (MinimumSupportedVersion >= version+1)
  {
    CLog::Log(LOGERROR, "unable to read setting definitions from version {} (minimum version: {})",
                    version, MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    CLog::Log(LOGERROR, "unable to read setting definitions from version {} (current version: {})",
                    version, Version);
    return false;
  }

  const TiXmlNode *sectionNode = root->FirstChild(SETTING_XML_ELM_SECTION);
  while (sectionNode != NULL)
  {
    std::string sectionId;
    if (CSettingSection::DeserializeIdentification(sectionNode, sectionId))
    {
      SettingSectionPtr section = SettingSectionPtr();
      CSettingsManager::SettingSectionMap::iterator itSection = m_sections.find(sectionId);
      bool update = (itSection != m_sections.end());
      if (!update)
        section = boost::make_shared<CSettingSection>(sectionId, this);
      else
        section = itSection->second;

      if (section->Deserialize(sectionNode, update))
        AddSection(section);
      else
      {
        CLog::Log(LOGWARNING, "unable to read section \"{}\"", sectionId);
      }
    }

    sectionNode = sectionNode->NextSibling(SETTING_XML_ELM_SECTION);
  }

  return true;
}

bool CSettingsManager::Load(const TiXmlElement *root, bool &updated, bool triggerEvents /* = true */, std::map<std::string, SettingPtr> *loadedSettings /* = NULL */)
{
  CSharedLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);
  if (m_loaded || root == NULL)
    return false;

  if (triggerEvents && !OnSettingsLoading())
    return false;

  // try to get and check the version
  uint32_t version = ParseVersion(root);
  if (version == 0)
    CLog::Log(LOGWARNING, "missing {} attribute", SETTING_XML_ROOT_VERSION);

  if (MinimumSupportedVersion >= version+1)
  {
    CLog::Log(LOGERROR, "unable to read setting values from version {} (minimum version: {})", version,
                    MinimumSupportedVersion);
    return false;
  }
  if (version > Version)
  {
    CLog::Log(LOGERROR, "unable to read setting values from version {} (current version: {})", version,
                    Version);
    return false;
  }

  if (!Deserialize(root, updated, loadedSettings))
    return false;

  if (triggerEvents)
    OnSettingsLoaded();

  return true;
}

bool CSettingsManager::Save(
  const ISettingsValueSerializer* serializer, std::string& serializedValues) const
{
  if (serializer == NULL)
    return false;

  CSharedLock lock(m_critical);
  CSharedLock settingsLock(m_settingsCritical);
  if (!m_initialized)
    return false;

  if (!OnSettingsSaving())
    return false;

  serializedValues = serializer->SerializeValues(this);

  OnSettingsSaved();

  return true;
}

void CSettingsManager::Unload()
{
  CExclusiveLock lock(m_settingsCritical);
  if (!m_loaded)
    return;

  // needs to be set before calling CSetting::Reset() to avoid calls to
  // OnSettingChanging() and OnSettingChanged()
  m_loaded = false;

  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    setting->second.setting->Reset();

  OnSettingsUnloaded();
}

void CSettingsManager::Clear()
{
  CExclusiveLock lock(m_critical);
  Unload();

  m_settings.clear();
  m_sections.clear();

  OnSettingsCleared();

  m_initialized = false;
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  bool updated = false;
  return LoadSetting(node, settingId, updated);
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, const std::string &settingId, bool &updated)
{
  updated = false;

  if (node == NULL)
    return false;

  SettingPtr setting = GetSetting(settingId);
  if (setting == NULL)
    return false;

  return LoadSetting(node, setting, updated);
}

void CSettingsManager::SetInitialized()
{
  CExclusiveLock lock(m_settingsCritical);
  if (m_initialized)
    return;

  m_initialized = true;

  // resolve any reference settings
  for (SettingSectionMap::const_iterator section = m_sections.begin(); section != m_sections.end(); ++section)
    ResolveReferenceSettings(section->second);

  // remove any incomplete settings
  CleanupIncompleteSettings();

  // figure out all the dependencies between settings
  for (SettingMap::const_iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    ResolveSettingDependencies(setting->second);
}

void CSettingsManager::AddSection(const SettingSectionPtr& section)
{
  if (section == NULL)
    return;

  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);

  section->CheckRequirements();
  m_sections[section->GetId()] = section;

  // get all settings and add them to the settings map
  std::set<SettingPtr> newSettings;
  SettingCategoryList categories = section->GetCategories();
  for (SettingCategoryList::const_iterator category = categories.begin(); category != categories.end(); ++category)
  {
    (*category)->CheckRequirements();
    SettingGroupList groups = (*category)->GetGroups();
    for (SettingGroupList::const_iterator group = groups.begin(); group != groups.end(); ++group)
    {
      (*group)->CheckRequirements();
      SettingList settings = (*group)->GetSettings();
      for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
      {
        AddSetting(*setting);

        newSettings.insert(*setting);
      }
    }
  }

  if (m_initialized && !newSettings.empty())
  {
    // resolve any reference settings in the new section
    ResolveReferenceSettings(section);

    // cleanup any newly added incomplete settings
    CleanupIncompleteSettings();

    // resolve any dependencies for the newly added settings
    for (std::set<SettingPtr>::const_iterator setting = newSettings.begin(); setting != newSettings.end(); ++setting)
      ResolveSettingDependencies(*setting);
  }
}

bool CSettingsManager::AddSetting(const boost::shared_ptr<CSetting>& setting,
                                  const boost::shared_ptr<CSettingSection>& section,
                                  const boost::shared_ptr<CSettingCategory>& category,
                                  const boost::shared_ptr<CSettingGroup>& group)
{
  if (setting == NULL || section == NULL || category == NULL || group == NULL)
    return false;

  CExclusiveLock lock(m_critical);
  CExclusiveLock settingsLock(m_settingsCritical);

  // check if a setting with the given ID already exists
  if (FindSetting(setting->GetId()) != m_settings.end())
    return false;

  // if the given setting has not been added to the group yet, do it now
  SettingList settings = group->GetSettings();
  if (std::find(settings.begin(), settings.end(), setting) == settings.end())
    group->AddSetting(setting);

  // if the given group has not been added to the category yet, do it now
  SettingGroupList groups = category->GetGroups();
  if (std::find(groups.begin(), groups.end(), group) == groups.end())
    category->AddGroup(group);

  // if the given category has not been added to the section yet, do it now
  SettingCategoryList categories = section->GetCategories();
  if (std::find(categories.begin(), categories.end(), category) == categories.end())
    section->AddCategory(category);

  // check if the given section exists and matches
  SettingSectionPtr sectionPtr = GetSection(section->GetId());
  if (sectionPtr != NULL && sectionPtr != section)
    return false;

  // if the section doesn't exist yet, add it
  if (sectionPtr == NULL)
    AddSection(section);
  else
  {
    // add the setting
    AddSetting(setting);

    if (m_initialized)
    {
      // cleanup any newly added incomplete setting
      CleanupIncompleteSettings();

      // resolve any dependencies for the newly added setting
      ResolveSettingDependencies(setting);
    }
  }

  return true;
}

void CSettingsManager::RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList)
{
  CExclusiveLock lock(m_settingsCritical);
  if (callback == NULL)
    return;

  for (std::set<std::string>::const_iterator setting = settingList.begin(); setting != settingList.end(); ++setting)
  {
    CSettingsManager::SettingMap::iterator itSetting = FindSetting(*setting);
    if (itSetting == m_settings.end())
    {
      if (m_initialized)
        continue;

      Setting tmpSetting = {};
      std::pair<SettingMap::iterator, bool> tmpIt = InsertSetting(*setting, tmpSetting);
      itSetting = tmpIt.first;
    }

    itSetting->second.callbacks.insert(callback);
  }
}

void CSettingsManager::UnregisterCallback(ISettingCallback *callback)
{
  CExclusiveLock lock(m_settingsCritical);
  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    setting->second.callbacks.erase(callback);
}

void CSettingsManager::RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator)
{
  CExclusiveLock lock(m_critical);
  if (settingType.empty() || settingCreator == NULL)
    return;

  CSettingsManager::SettingCreatorMap::iterator creatorIt = m_settingCreators.find(settingType);
  if (creatorIt == m_settingCreators.end())
    m_settingCreators.insert(std::make_pair(settingType, settingCreator));
}

void CSettingsManager::RegisterSettingControl(const std::string &controlType, ISettingControlCreator *settingControlCreator)
{
  if (controlType.empty() || settingControlCreator == NULL)
    return;

  CExclusiveLock lock(m_critical);
  CSettingsManager::SettingControlCreatorMap::iterator creatorIt = m_settingControlCreators.find(controlType);
  if (creatorIt == m_settingControlCreators.end())
    m_settingControlCreators.insert(std::make_pair(controlType, settingControlCreator));
}

void CSettingsManager::RegisterSettingsHandler(ISettingsHandler *settingsHandler, bool bFront /* = false */)
{
  if (settingsHandler == NULL)
    return;

  CExclusiveLock lock(m_critical);
  if (find(m_settingsHandlers.begin(), m_settingsHandlers.end(), settingsHandler) == m_settingsHandlers.end())
  {
    if (bFront)
      m_settingsHandlers.insert(m_settingsHandlers.begin(), settingsHandler);
    else
      m_settingsHandlers.push_back(settingsHandler);
  }
}

void CSettingsManager::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CExclusiveLock lock(m_critical);
  CSettingsManager::SettingsHandlers::iterator it = std::find(m_settingsHandlers.begin(), m_settingsHandlers.end(), settingsHandler);
  if (it != m_settingsHandlers.end())
    m_settingsHandlers.erase(it);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, IntegerSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == NULL)
    return;

  RegisterSettingOptionsFiller(identifier, reinterpret_cast<void*>(optionsFiller), SettingOptionsFillerType::Integer);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, StringSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == NULL)
    return;

  RegisterSettingOptionsFiller(identifier, reinterpret_cast<void*>(optionsFiller), SettingOptionsFillerType::String);
}

void CSettingsManager::UnregisterSettingOptionsFiller(const std::string &identifier)
{
  CExclusiveLock lock(m_critical);
  m_optionsFillers.erase(identifier);
}

void* CSettingsManager::GetSettingOptionsFiller(const SettingConstPtr& setting)
{
  CSharedLock lock(m_critical);
  if (setting == NULL)
    return NULL;

  // get the option filler's identifier
  std::string filler;
  if (setting->GetType() == SettingType::Integer)
    filler = boost::static_pointer_cast<const CSettingInt>(setting)->GetOptionsFillerName();
  else if (setting->GetType() == SettingType::String)
    filler = boost::static_pointer_cast<const CSettingString>(setting)->GetOptionsFillerName();

  if (filler.empty())
    return NULL;

  // check if such an option filler is known
  CSettingsManager::SettingOptionsFillerMap::iterator fillerIt = m_optionsFillers.find(filler);
  if (fillerIt == m_optionsFillers.end())
    return NULL;

  if (fillerIt->second.filler == NULL)
    return NULL;

  // make sure the option filler's type matches the setting's type
  switch (fillerIt->second.type)
  {
    case SettingOptionsFillerType::Integer:
    {
      if (setting->GetType() != SettingType::Integer)
        return NULL;

      break;
    }

    case SettingOptionsFillerType::String:
    {
      if (setting->GetType() != SettingType::String)
        return NULL;

      break;
    }

    default:
      return NULL;
  }

  return fillerIt->second.filler;
}

bool CSettingsManager::HasSettings() const
{
  return !m_settings.empty();
}

SettingPtr CSettingsManager::GetSetting(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  if (id.empty())
    return SettingPtr();

  CSettingsManager::SettingMap::const_iterator setting = FindSetting(id);
  if (setting != m_settings.end())
  {
    if (setting->second.setting->IsReference())
      return GetSetting(setting->second.setting->GetReferencedId());
    return setting->second.setting;
  }

  CLog::Log(LOGDEBUG, "requested setting ({}) was not found.", id);
  return SettingPtr();
}

SettingSectionList CSettingsManager::GetSections() const
{
  CSharedLock lock(m_critical);
  SettingSectionList sections;
  for (SettingSectionMap::const_iterator section = m_sections.begin(); section != m_sections.end(); ++section)
    sections.push_back(section->second);

  return sections;
}

SettingSectionPtr CSettingsManager::GetSection(std::string section) const
{
  CSharedLock lock(m_critical);
  if (section.empty())
    return SettingSectionPtr();

  StringUtils::ToLower(section);

  CSettingsManager::SettingSectionMap::const_iterator sectionIt = m_sections.find(section);
  if (sectionIt != m_sections.end())
    return sectionIt->second;

  CLog::Log(LOGDEBUG, "requested setting section ({}) was not found.", section);
  return SettingSectionPtr();
}

SettingDependencyMap CSettingsManager::GetDependencies(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  CSettingsManager::SettingMap::const_iterator setting = FindSetting(id);
  if (setting == m_settings.end())
    return SettingDependencyMap();

  return setting->second.dependencies;
}

SettingDependencyMap CSettingsManager::GetDependencies(const SettingConstPtr& setting) const
{
  if (setting == NULL)
    return SettingDependencyMap();

  return GetDependencies(setting->GetId());
}

bool CSettingsManager::GetBool(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Boolean)
    return false;

  return boost::static_pointer_cast<CSettingBool>(setting)->GetValue();
}

bool CSettingsManager::SetBool(const std::string &id, bool value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Boolean)
    return false;

  return boost::static_pointer_cast<CSettingBool>(setting)->SetValue(value);
}

bool CSettingsManager::ToggleBool(const std::string &id)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Boolean)
    return false;

  return SetBool(id, !boost::static_pointer_cast<CSettingBool>(setting)->GetValue());
}

int CSettingsManager::GetInt(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Integer)
    return 0;

  return boost::static_pointer_cast<CSettingInt>(setting)->GetValue();
}

bool CSettingsManager::SetInt(const std::string &id, int value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Integer)
    return false;

  return boost::static_pointer_cast<CSettingInt>(setting)->SetValue(value);
}

double CSettingsManager::GetNumber(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Number)
    return 0.0;

  return boost::static_pointer_cast<CSettingNumber>(setting)->GetValue();
}

bool CSettingsManager::SetNumber(const std::string &id, double value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::Number)
    return false;

  return boost::static_pointer_cast<CSettingNumber>(setting)->SetValue(value);
}

std::string CSettingsManager::GetString(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::String)
    return "";

  return boost::static_pointer_cast<CSettingString>(setting)->GetValue();
}

bool CSettingsManager::SetString(const std::string &id, const std::string &value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::String)
    return false;

  return boost::static_pointer_cast<CSettingString>(setting)->SetValue(value);
}

std::vector< boost::shared_ptr<CSetting> > CSettingsManager::GetList(const std::string &id) const
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::List)
    return std::vector< boost::shared_ptr<CSetting> >();

  return boost::static_pointer_cast<CSettingList>(setting)->GetValue();
}

bool CSettingsManager::SetList(const std::string &id, const std::vector< boost::shared_ptr<CSetting> > &value)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingType::List)
    return false;

  return boost::static_pointer_cast<CSettingList>(setting)->SetValue(value);
}

bool CSettingsManager::SetDefault(const std::string &id)
{
  CSharedLock lock(m_settingsCritical);
  SettingPtr setting = GetSetting(id);
  if (setting == NULL)
    return false;

  setting->Reset();
  return true;
}

void CSettingsManager::SetDefaults()
{
  CSharedLock lock(m_settingsCritical);
  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    setting->second.setting->Reset();
}

void CSettingsManager::AddCondition(const std::string &condition)
{
  CExclusiveLock lock(m_critical);
  if (condition.empty())
    return;

  m_conditions.AddCondition(condition);
}

void CSettingsManager::AddDynamicCondition(const std::string &identifier, SettingConditionCheck condition, void *data /*= NULL*/)
{
  CExclusiveLock lock(m_critical);
  if (identifier.empty() || condition == NULL)
    return;

  m_conditions.AddDynamicCondition(identifier, condition, data);
}

void CSettingsManager::RemoveDynamicCondition(const std::string &identifier)
{
  CExclusiveLock lock(m_critical);
  if (identifier.empty())
    return;

  m_conditions.RemoveDynamicCondition(identifier);
}

bool CSettingsManager::Serialize(TiXmlNode *parent) const
{
  if (parent == NULL)
    return false;

  CSharedLock lock(m_settingsCritical);

  for (SettingMap::const_iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
  {
    if (setting->second.setting->IsReference() ||
        setting->second.setting->GetType() == SettingType::Action)
      continue;

    TiXmlElement settingElement(SETTING_XML_ELM_SETTING);
    settingElement.SetAttribute(SETTING_XML_ATTR_ID, setting->second.setting->GetId());

    // add the default attribute
    if (setting->second.setting->IsDefault())
      settingElement.SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

    // add the value
    TiXmlText value(setting->second.setting->ToString());
    settingElement.InsertEndChild(value);

    if (parent->InsertEndChild(settingElement) == NULL)
    {
      CLog::Log(LOGWARNING, "unable to write <" SETTING_XML_ELM_SETTING " id=\"{}\"> tag",
                     setting->second.setting->GetId());
      continue;
    }
  }

  return true;
}

bool CSettingsManager::Deserialize(const TiXmlNode *node, bool &updated, std::map<std::string, SettingPtr> *loadedSettings /* = NULL */)
{
  updated = false;

  if (node == NULL)
    return false;

  CSharedLock lock(m_settingsCritical);

  // TODO: ideally this would be done by going through all <setting> elements
  // in node but as long as we have to support the v1- format that's not possible
  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
  {
    bool settingUpdated = false;
    if (LoadSetting(node, setting->second.setting, settingUpdated))
    {
      updated |= settingUpdated;
      if (loadedSettings != NULL)
        loadedSettings->insert(make_pair(setting->first, setting->second.setting));
    }
  }

  return true;
}

bool CSettingsManager::OnSettingChanging(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return false;

  CSharedLock lock(m_settingsCritical);
  if (!m_loaded)
    return true;

  CSettingsManager::SettingMap::iterator settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin(); callback != settingData.callbacks.end(); ++callback)
  {
    if (!(*callback)->OnSettingChanging(setting))
      return false;
  }

  // if this is a reference setting apply the same change to the referenced setting
  if (setting->IsReference())
  {
    CSharedLock lock(m_settingsCritical);
    CSettingsManager::SettingMap::iterator referencedSettingIt = FindSetting(setting->GetReferencedId());
    if (referencedSettingIt != m_settings.end())
    {
      Setting referencedSettingData = referencedSettingIt->second;
      // now that we have a copy of the setting's data, we can leave the lock
      lock.Leave();

      referencedSettingData.setting->FromString(setting->ToString());
    }
  }
  else if (!settingData.references.empty())
  {
    // if the changed setting is referenced by other settings apply the same change to the referencing settings
    std::set<SettingPtr> referenceSettings;
    CSharedLock lock(m_settingsCritical);
    for (std::set<std::string>::const_iterator reference = settingData.references.begin(); reference != settingData.references.end(); ++reference)
    {
      CSettingsManager::SettingMap::iterator referenceSettingIt = FindSetting(*reference);
      if (referenceSettingIt != m_settings.end())
        referenceSettings.insert(referenceSettingIt->second.setting);
    }
    // now that we have a copy of the setting's data, we can leave the lock
    lock.Leave();

    for (std::set<SettingPtr>::iterator referenceSetting = referenceSettings.begin(); referenceSetting != referenceSettings.end(); ++referenceSetting)
      (*referenceSetting)->FromString(setting->ToString());
  }

  return true;
}

void CSettingsManager::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == NULL)
    return;

  CSettingsManager::SettingMap::iterator settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin(); callback != settingData.callbacks.end(); ++callback)
    (*callback)->OnSettingChanged(setting);

  // now handle any settings which depend on the changed setting
  SettingDependencyMap dependencies = GetDependencies(setting);
  for (SettingDependencyMap::const_iterator deps = dependencies.begin(); deps != dependencies.end(); ++deps)
  {
    for (SettingDependencies::const_iterator dep = deps->second.begin(); dep != deps->second.end(); ++dep)
      UpdateSettingByDependency(deps->first, *dep);
  }
}

void CSettingsManager::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == NULL)
    return;

  CSettingsManager::SettingMap::iterator settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin(); callback != settingData.callbacks.end(); ++callback)
    (*callback)->OnSettingAction(setting);
}

bool CSettingsManager::OnSettingUpdate(const SettingPtr& setting,
                                       const char* oldSettingId,
                                       const TiXmlNode* oldSettingNode)
{
  CSharedLock lock(m_settingsCritical);
  if (setting == NULL)
    return false;

  CSettingsManager::SettingMap::iterator settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  bool ret = false;
  for (CallbackSet::iterator callback = settingData.callbacks.begin(); callback != settingData.callbacks.end(); ++callback)
    ret |= (*callback)->OnSettingUpdate(setting, oldSettingId, oldSettingNode);

  return ret;
}

void CSettingsManager::OnSettingPropertyChanged(const boost::shared_ptr<const CSetting>& setting,
                                                const char* propertyName)
{
  CSharedLock lock(m_settingsCritical);
  if (!m_loaded || setting == NULL)
    return;

  CSettingsManager::SettingMap::const_iterator settingIt = FindSetting(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin(); callback != settingData.callbacks.end(); ++callback)
    (*callback)->OnSettingPropertyChanged(setting, propertyName);

  // check the changed property and if it may have an influence on the
  // children of the setting
  SettingDependencyType::Type dependencyType = SettingDependencyType::Unknown;
  if (StringUtils::EqualsNoCase(propertyName, "enabled"))
    dependencyType = SettingDependencyType::Enable;
  else if (StringUtils::EqualsNoCase(propertyName, "visible"))
    dependencyType = SettingDependencyType::Visible;

  if (dependencyType != SettingDependencyType::Unknown)
  {
    for (std::set<std::string>::const_iterator child = settingIt->second.children.begin(); child != settingIt->second.children.end(); ++child)
      UpdateSettingByDependency(*child, dependencyType);
  }
}

SettingPtr CSettingsManager::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = NULL */) const
{
  if (StringUtils::EqualsNoCase(settingType, "boolean"))
    return boost::make_shared<CSettingBool>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "integer"))
    return boost::make_shared<CSettingInt>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "number"))
    return boost::make_shared<CSettingNumber>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "string"))
    return boost::make_shared<CSettingString>(settingId, const_cast<CSettingsManager*>(this));
  else if (StringUtils::EqualsNoCase(settingType, "action"))
    return boost::make_shared<CSettingAction>(settingId, const_cast<CSettingsManager*>(this));
  else if (settingType.size() > 6 &&
           StringUtils::StartsWith(settingType, "list[") &&
           StringUtils::EndsWith(settingType, "]"))
  {
    std::string elementType = StringUtils::Mid(settingType, 5, settingType.size() - 6);
    SettingPtr elementSetting = CreateSetting(elementType, settingId + ".definition", const_cast<CSettingsManager*>(this));
    if (elementSetting != NULL)
      return boost::make_shared<CSettingList>(settingId, elementSetting, const_cast<CSettingsManager*>(this));
  }

  CSharedLock lock(m_critical);
  CSettingsManager::SettingCreatorMap::const_iterator creator = m_settingCreators.find(settingType);
  if (creator != m_settingCreators.end())
    return creator->second->CreateSetting(settingType, settingId, const_cast<CSettingsManager*>(this));

  return SettingPtr();
}

boost::shared_ptr<ISettingControl> CSettingsManager::CreateControl(const std::string &controlType) const
{
  if (controlType.empty())
    return boost::shared_ptr<ISettingControl>();

  CSharedLock lock(m_critical);
  CSettingsManager::SettingControlCreatorMap::const_iterator creator = m_settingControlCreators.find(controlType);
  if (creator != m_settingControlCreators.end() && creator->second != NULL)
    return creator->second->CreateControl(controlType);

  return boost::shared_ptr<ISettingControl>();
}

bool CSettingsManager::OnSettingsLoading()
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
  {
    if (!(*settingsHandler)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsUnloaded()
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
    (*settingsHandler)->OnSettingsUnloaded();
}

void CSettingsManager::OnSettingsLoaded()
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
    (*settingsHandler)->OnSettingsLoaded();
}

bool CSettingsManager::OnSettingsSaving() const
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
  {
    if (!(*settingsHandler)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsSaved() const
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
    (*settingsHandler)->OnSettingsSaved();
}

void CSettingsManager::OnSettingsCleared()
{
  CSharedLock lock(m_critical);
  for (SettingsHandlers::const_iterator settingsHandler = m_settingsHandlers.begin(); settingsHandler != m_settingsHandlers.end(); ++settingsHandler)
    (*settingsHandler)->OnSettingsCleared();
}

bool CSettingsManager::LoadSetting(const TiXmlNode* node, const SettingPtr& setting, bool& updated)
{
  updated = false;

  if (node == NULL || setting == NULL)
    return false;

  if (setting->GetType() == SettingType::Action)
    return false;

  std::string settingId = setting->GetId();
  if (setting->IsReference())
    settingId = setting->GetReferencedId();

  const TiXmlElement* settingElement = NULL;
  // try to split the setting identifier into category and subsetting identifier (v1-)
  std::string categoryTag, settingTag;
  if (ParseSettingIdentifier(settingId, categoryTag, settingTag))
  {
    const TiXmlNode *categoryNode = node;
    if (!categoryTag.empty())
      categoryNode = node->FirstChild(categoryTag);

    if (categoryNode != NULL)
      settingElement = categoryNode->FirstChildElement(settingTag);
  }

  if (settingElement == NULL)
  {
    // check if the setting is stored using its full setting identifier (v2+)
    settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
    while (settingElement != NULL)
    {
      const char *const id = settingElement->Attribute(SETTING_XML_ATTR_ID);
      if (id != NULL && settingId.compare(id) == 0)
        break;

      settingElement = settingElement->NextSiblingElement(SETTING_XML_ELM_SETTING);
    }
  }

  if (settingElement == NULL)
    return false;

  // check if the default="true" attribute is set for the value
  const char *isDefaultAttribute = settingElement->Attribute(SETTING_XML_ELM_DEFAULT);
  bool isDefault = isDefaultAttribute != NULL && StringUtils::EqualsNoCase(isDefaultAttribute, "true");

  if (!setting->FromString(settingElement->FirstChild() != NULL ? settingElement->FirstChild()->ValueStr() : StringUtils::Empty))
  {
    CLog::Log(LOGWARNING, "unable to read value of setting \"{}\"", settingId);
    return false;
  }

  // check if we need to perform any update logic for the setting
  std::set<CSettingUpdate> updates = setting->GetUpdates();
  for (std::set<CSettingUpdate>::const_iterator update = updates.begin(); update != updates.end(); ++update)
    updated |= UpdateSetting(node, setting, *update);

  // the setting's value hasn't been updated and is the default value
  // so we can reset it to the default value (in case the default value has changed)
  if (!updated && isDefault)
    setting->Reset();

  return true;
}

bool CSettingsManager::UpdateSetting(const TiXmlNode* node,
                                     const SettingPtr& setting,
                                     const CSettingUpdate& update)
{
  if (node == NULL || setting == NULL || update.GetType() == SettingUpdateType::Unknown)
    return false;

  bool updated = false;
  const char *oldSetting = NULL;
  const TiXmlNode *oldSettingNode = NULL;
  if (update.GetType() == SettingUpdateType::Rename)
  {
    if (update.GetValue().empty())
      return false;

    oldSetting = update.GetValue().c_str();
    std::string categoryTag, settingTag;
    if (!ParseSettingIdentifier(oldSetting, categoryTag, settingTag))
      return false;

    const TiXmlNode *categoryNode = node;
    if (!categoryTag.empty())
    {
      categoryNode = node->FirstChild(categoryTag);
      if (categoryNode == NULL)
        return false;
    }

    oldSettingNode = categoryNode->FirstChild(settingTag);
    if (oldSettingNode == NULL)
      return false;

    if (setting->FromString(oldSettingNode->FirstChild() != NULL ? oldSettingNode->FirstChild()->ValueStr() : StringUtils::Empty))
      updated = true;
    else
      CLog::Log(LOGWARNING, "unable to update \"{}\" through automatically renaming from \"{}\"",
                     setting->GetId(), oldSetting);
  }

  updated |= OnSettingUpdate(setting, oldSetting, oldSettingNode);
  return updated;
}

void CSettingsManager::UpdateSettingByDependency(const std::string &settingId, const CSettingDependency &dependency)
{
  UpdateSettingByDependency(settingId, dependency.GetType());
}

void CSettingsManager::UpdateSettingByDependency(const std::string &settingId, SettingDependencyType::Type dependencyType)
{
  CSettingsManager::SettingMap::iterator settingIt = FindSetting(settingId);
  if (settingIt == m_settings.end())
    return;
  SettingPtr setting = settingIt->second.setting;
  if (setting == NULL)
    return;

  switch (dependencyType)
  {
    case SettingDependencyType::Enable:
      // just trigger the property changed callback and a call to
      // CSetting::IsEnabled() will automatically determine the new
      // enabled state
      OnSettingPropertyChanged(setting, "enabled");
      break;

    case SettingDependencyType::Update:
    {
      SettingType::Type type = setting->GetType();
      if (type == SettingType::Integer)
      {
        boost::shared_ptr<CSettingInt> settingInt = boost::static_pointer_cast<CSettingInt>(setting);
        if (settingInt->GetOptionsType() == SettingOptionsType::Dynamic)
          settingInt->UpdateDynamicOptions();
      }
      else if (type == SettingType::String)
      {
        boost::shared_ptr<CSettingString> settingString = boost::static_pointer_cast<CSettingString>(setting);
        if (settingString->GetOptionsType() == SettingOptionsType::Dynamic)
          settingString->UpdateDynamicOptions();
      }
      // when a setting depends on another, it might need to refresh its visible/enable status
      // after been updated. E.g. if it depends on some complex setting condition
      RefreshVisibilityAndEnableStatus(setting);
      break;
    }

    case SettingDependencyType::Visible:
      // just trigger the property changed callback and a call to
      // CSetting::IsVisible() will automatically determine the new
      // visible state
      OnSettingPropertyChanged(setting, "visible");
      break;

    case SettingDependencyType::Unknown:
    default:
      break;
  }
}

void CSettingsManager::RefreshVisibilityAndEnableStatus(
    const boost::shared_ptr<const CSetting>& setting)
{
  bool updateVisibility = false;
  bool updateEnableStatus = false;
  const SettingDependencies& dependencies = setting->GetDependencies();
  for (SettingDependencies::const_iterator dep = dependencies.begin(); dep != dependencies.end(); ++dep)
  {
    if (dep->GetType() == SettingDependencyType::Enable)
    {
      updateEnableStatus = true;
    }

    if (dep->GetType() == SettingDependencyType::Visible)
    {
      updateVisibility = true;
    }
  }

  if (updateVisibility)
  {
    OnSettingPropertyChanged(setting, "visible");
  }
  if (updateEnableStatus)
  {
    OnSettingPropertyChanged(setting, "enabled");
  }
}

void CSettingsManager::AddSetting(const boost::shared_ptr<CSetting>& setting)
{
  setting->CheckRequirements();

  CSettingsManager::SettingMap::iterator addedSetting = FindSetting(setting->GetId());
  if (addedSetting == m_settings.end())
  {
    Setting tmpSetting = {};
    std::pair<CSettingsManager::SettingMap::iterator, bool> tmpIt = InsertSetting(setting->GetId(), tmpSetting);
    addedSetting = tmpIt.first;
  }

  if (addedSetting->second.setting == NULL)
  {
    addedSetting->second.setting = setting;
    setting->SetCallback(this);
  }
}

namespace
{
  struct GroupedReferenceSettings
  {
    SettingPtr referencedSetting;
    std::set<SettingPtr> referenceSettings;
  };
}

void CSettingsManager::ResolveReferenceSettings(const boost::shared_ptr<CSettingSection>& section)
{
  std::map<std::string, GroupedReferenceSettings> groupedReferenceSettings;

  // collect and group all reference(d) settings
  SettingCategoryList categories = section->GetCategories();
  for (SettingCategoryList::const_iterator category = categories.begin(); category != categories.end(); ++category)
  {
    SettingGroupList groups = (*category)->GetGroups();
    for (SettingGroupList::iterator group = groups.begin(); group != groups.end(); ++group)
    {
      SettingList settings = (*group)->GetSettings();
      for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
      {
        if ((*setting)->IsReference())
        {
          std::string referencedSettingId = (*setting)->GetReferencedId();
          std::map<std::string, GroupedReferenceSettings>::iterator itGroupedReferenceSetting = groupedReferenceSettings.find(referencedSettingId);
          if (itGroupedReferenceSetting == groupedReferenceSettings.end())
          {
            SettingPtr referencedSetting = SettingPtr();
            CSettingsManager::SettingMap::iterator itReferencedSetting = FindSetting(referencedSettingId);
            if (itReferencedSetting == m_settings.end())
            {
              CLog::Log(LOGWARNING, "missing referenced setting \"{}\"", referencedSettingId);
              continue;
            }

            GroupedReferenceSettings groupedReferenceSetting;
            groupedReferenceSetting.referencedSetting = itReferencedSetting->second.setting;

            itGroupedReferenceSetting = groupedReferenceSettings.insert(
              std::make_pair(referencedSettingId, groupedReferenceSetting)).first;
          }

          itGroupedReferenceSetting->second.referenceSettings.insert(*setting);
        }
      }
    }
  }

  if (groupedReferenceSettings.empty())
    return;

  // merge all reference settings into the referenced setting
  for (std::map<std::string, GroupedReferenceSettings>::const_iterator groupedReferenceSetting = groupedReferenceSettings.begin(); groupedReferenceSetting != groupedReferenceSettings.end(); ++groupedReferenceSetting)
  {
    CSettingsManager::SettingMap::iterator itReferencedSetting = FindSetting(groupedReferenceSetting->first);
    if (itReferencedSetting == m_settings.end())
      continue;

    for (std::set<SettingPtr>::const_iterator referenceSetting = groupedReferenceSetting->second.referenceSettings.begin(); referenceSetting != groupedReferenceSetting->second.referenceSettings.end(); ++referenceSetting)
    {
      groupedReferenceSetting->second.referencedSetting->MergeDetails(**referenceSetting);

      itReferencedSetting->second.references.insert((*referenceSetting)->GetId());
    }
  }

  // resolve any reference settings
  for (SettingCategoryList::const_iterator category = categories.begin(); category != categories.end(); ++category)
  {
    SettingGroupList groups = (*category)->GetGroups();
    for (SettingGroupList::iterator group = groups.begin(); group != groups.end(); ++group)
    {
      SettingList settings = (*group)->GetSettings();
      for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
      {
        if ((*setting)->IsReference())
        {
          std::string referencedSettingId = (*setting)->GetReferencedId();
          std::map<std::string, GroupedReferenceSettings>::iterator itGroupedReferenceSetting = groupedReferenceSettings.find(referencedSettingId);
          if (itGroupedReferenceSetting != groupedReferenceSettings.end())
          {
            const SettingPtr referencedSetting = itGroupedReferenceSetting->second.referencedSetting;

            // clone the referenced setting and copy the general properties of the reference setting
            SettingPtr clonedReferencedSetting = referencedSetting->Clone((*setting)->GetId());
            clonedReferencedSetting->SetReferencedId(referencedSettingId);
            clonedReferencedSetting->MergeBasics(**setting);

            (*group)->ReplaceSetting(*setting, clonedReferencedSetting);

            // update the setting
            CSettingsManager::SettingMap::iterator itReferenceSetting = FindSetting((*setting)->GetId());
            if (itReferenceSetting != m_settings.end())
              itReferenceSetting->second.setting = clonedReferencedSetting;
          }
        }
      }
    }
  }
}

void CSettingsManager::CleanupIncompleteSettings()
{
  // remove any empty and reference settings
  for (CSettingsManager::SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); )
  {
    CSettingsManager::SettingMap::iterator tmpIterator = setting++;
    if (tmpIterator->second.setting == NULL)
    {
      CLog::Log(LOGWARNING, "removing empty setting \"{}\"", tmpIterator->first);
      m_settings.erase(tmpIterator);
    }
  }
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type)
{
  CExclusiveLock lock(m_critical);
  CSettingsManager::SettingOptionsFillerMap::iterator it = m_optionsFillers.find(identifier);
  if (it != m_optionsFillers.end())
    return;

  SettingOptionsFiller optionsFiller = { filler, type };
  m_optionsFillers.insert(make_pair(identifier, optionsFiller));
}

void CSettingsManager::ResolveSettingDependencies(const boost::shared_ptr<CSetting>& setting)
{
  if (setting == NULL)
    return;

  ResolveSettingDependencies(FindSetting(setting->GetId())->second);
}

void CSettingsManager::ResolveSettingDependencies(const Setting& setting)
{
  if (setting.setting == NULL)
    return;

  // if the setting has a parent setting, add it to its children
  std::string parentSettingId = setting.setting->GetParent();
  if (!parentSettingId.empty())
  {
    CSettingsManager::SettingMap::iterator itParentSetting = FindSetting(parentSettingId);
    if (itParentSetting != m_settings.end())
      itParentSetting->second.children.insert(setting.setting->GetId());
  }

  // handle all dependencies of the setting
  const SettingDependencies &dependencies = setting.setting->GetDependencies();
  for (SettingDependencies::const_iterator deps = dependencies.begin(); deps != dependencies.end(); ++deps)
  {
    const std::set<std::string> settingIds = deps->GetSettings();
    for (std::set<std::string>::const_iterator settingId = settingIds.begin(); settingId != settingIds.end(); ++settingId)
    {
      CSettingsManager::SettingMap::iterator settingIt = FindSetting(*settingId);
      if (settingIt == m_settings.end())
        continue;

      bool newDep = true;
      SettingDependencies &settingDeps = settingIt->second.dependencies[setting.setting->GetId()];
      for (SettingDependencies::const_iterator dep = settingDeps.begin(); dep != settingDeps.end(); ++dep)
      {
        if (dep->GetType() == deps->GetType())
        {
          newDep = false;
          break;
        }
      }

      if (newDep)
        settingDeps.push_back(*deps);
    }
  }
}

CSettingsManager::SettingMap::const_iterator CSettingsManager::FindSetting(std::string settingId) const
{
  StringUtils::ToLower(settingId);
  return m_settings.find(settingId);
}

CSettingsManager::SettingMap::iterator CSettingsManager::FindSetting(std::string settingId)
{
  StringUtils::ToLower(settingId);
  return m_settings.find(settingId);
}

std::pair<CSettingsManager::SettingMap::iterator, bool> CSettingsManager::InsertSetting(std::string settingId, const Setting& setting)
{
  StringUtils::ToLower(settingId);
  return m_settings.insert(std::make_pair(settingId, setting));
}
