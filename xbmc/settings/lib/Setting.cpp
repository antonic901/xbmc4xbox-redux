/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Setting.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <boost/make_shared.hpp>
#include <sstream>
#include <utility>

template<typename TKey, typename TValue>
bool CheckSettingOptionsValidity(const TValue& value, const std::vector<std::pair<TKey, TValue> >& options)
{
  for (std::vector<std::pair<TKey, TValue> >::const_iterator it = options.begin(); it != options.end(); ++it)
  {
    if (it->second == value)
      return true;
  }

  return false;
}

template<typename TKey, typename TValue>
bool CheckSettingOptionsValidity(const TValue& value, const std::vector<TKey>& options)
{
  for (std::vector<TKey>::const_iterator it = options.begin(); it != options.end(); ++it)
  {
    if (it->value == value)
      return true;
  }

  return false;
}

bool DeserializeOptionsSort(const TiXmlElement* optionsElement, SettingOptionsSort::Type& optionsSort)
{
  optionsSort = SettingOptionsSort::NoSorting;

  std::string sort;
  if (optionsElement->QueryStringAttribute("sort", &sort) != TIXML_SUCCESS)
    return true;

  if (StringUtils::EqualsNoCase(sort, "false") || StringUtils::EqualsNoCase(sort, "off") ||
    StringUtils::EqualsNoCase(sort, "no") || StringUtils::EqualsNoCase(sort, "disabled"))
    optionsSort = SettingOptionsSort::NoSorting;
  else if (StringUtils::EqualsNoCase(sort, "asc") || StringUtils::EqualsNoCase(sort, "ascending") ||
    StringUtils::EqualsNoCase(sort, "true") || StringUtils::EqualsNoCase(sort, "on") ||
    StringUtils::EqualsNoCase(sort, "yes") || StringUtils::EqualsNoCase(sort, "enabled"))
    optionsSort = SettingOptionsSort::Ascending;
  else if (StringUtils::EqualsNoCase(sort, "desc") || StringUtils::EqualsNoCase(sort, "descending"))
    optionsSort = SettingOptionsSort::Descending;
  else
    return false;

  return true;
}

CSetting::CSetting(const std::string& id, CSettingsManager* settingsManager /* = NULL */)
  : ISetting(id, settingsManager), m_callback(NULL), m_enabled(true), m_level(SettingLevel::Standard), m_changed(false)
{
}

CSetting::CSetting(const std::string& id, const CSetting& setting)
  : ISetting(id, setting.m_settingsManager), m_enabled(true)
{
  Copy(setting);
}

void CSetting::MergeBasics(const CSetting& other)
{
  // ISetting
  SetVisible(other.GetVisible());
  SetLabel(other.GetLabel());
  SetHelp(other.GetHelp());
  SetRequirementsMet(other.MeetsRequirements());
  // CSetting
  SetEnabled(other.GetEnabled());
  SetParent(other.GetParent());
  SetLevel(other.GetLevel());
  SetControl(const_cast<CSetting&>(other).GetControl());
  SetDependencies(other.GetDependencies());
}

bool CSetting::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  const char *parentSetting = element->Attribute(SETTING_XML_ATTR_PARENT);
  if (parentSetting != NULL)
    m_parentSetting = parentSetting;

  // get <enable>
  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_ENABLED, value))
    m_enabled = value;

  // get the <level>
  int level = -1;
  if (XMLUtils::GetInt(node, SETTING_XML_ELM_LEVEL, level))
    m_level = static_cast<SettingLevel::Type>(level);

  if (m_level < SettingLevel::Basic || m_level > SettingLevel::Internal)
    m_level = SettingLevel::Standard;

  const TiXmlNode *dependencies = node->FirstChild(SETTING_XML_ELM_DEPENDENCIES);
  if (dependencies != NULL)
  {
    const TiXmlNode *dependencyNode = dependencies->FirstChild(SETTING_XML_ELM_DEPENDENCY);
    while (dependencyNode != NULL)
    {
      CSettingDependency dependency(m_settingsManager);
      if (dependency.Deserialize(dependencyNode))
        m_dependencies.push_back(dependency);
      else
        CLog::Log(LOGWARNING, "error reading <{}> tag of \"{}\"", SETTING_XML_ELM_DEPENDENCY, m_id);

      dependencyNode = dependencyNode->NextSibling(SETTING_XML_ELM_DEPENDENCY);
    }
  }

  const TiXmlElement *control = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (control != NULL)
  {
    const char *controlType = control->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == NULL)
    {
      CLog::Log(LOGERROR, "error reading \"{}\" attribute of <control> tag of \"{}\"",
                      SETTING_XML_ATTR_TYPE, m_id);
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == NULL || !m_control->Deserialize(control, update))
    {
      CLog::Log(LOGERROR, "error reading <{}> tag of \"{}\"", SETTING_XML_ELM_CONTROL, m_id);
      return false;
    }
  }
  else if (!update && m_level < SettingLevel::Internal && !IsReference())
  {
    CLog::Log(LOGERROR, "missing <{}> tag of \"{}\"", SETTING_XML_ELM_CONTROL, m_id);
    return false;
  }

  const TiXmlNode *updates = node->FirstChild(SETTING_XML_ELM_UPDATES);
  if (updates != NULL)
  {
    const TiXmlElement *updateElem = updates->FirstChildElement(SETTING_XML_ELM_UPDATE);
    while (updateElem != NULL)
    {
      CSettingUpdate settingUpdate;
      if (settingUpdate.Deserialize(updateElem))
      {
        if (!m_updates.insert(settingUpdate).second)
          CLog::Log(LOGWARNING, "duplicate <{}> definition for \"{}\"", SETTING_XML_ELM_UPDATE, m_id);
      }
      else
        CLog::Log(LOGWARNING, "error reading <{}> tag of \"{}\"", SETTING_XML_ELM_UPDATE, m_id);

      updateElem = updateElem->NextSiblingElement(SETTING_XML_ELM_UPDATE);
    }
  }

  return true;
}

bool CSetting::IsEnabled() const
{
  if (m_dependencies.empty() && m_parentSetting.empty())
    return m_enabled;

  // if the setting has a parent setting and that parent setting is disabled
  // the setting should automatically also be disabled
  if (!m_parentSetting.empty())
  {
    SettingPtr parentSetting = m_settingsManager->GetSetting(m_parentSetting);
    if (parentSetting != NULL && !parentSetting->IsEnabled())
      return false;
  }

  bool enabled = m_enabled;
  for (SettingDependencies::const_iterator dep = m_dependencies.begin(); dep != m_dependencies.end(); ++dep)
  {
    if (dep->GetType() != SettingDependencyType::Enable)
      continue;

    if (!dep->Check())
    {
      enabled = false;
      break;
    }
  }

  return enabled;
}

void CSetting::SetEnabled(bool enabled)
{
  if (!m_dependencies.empty() || m_enabled == enabled)
    return;

  m_enabled = enabled;
  OnSettingPropertyChanged(shared_from_this(), "enabled");
}

void CSetting::MakeReference(const std::string& referencedId /* = "" */)
{
  std::string tmpReferencedId = referencedId;
  if (referencedId.empty())
    tmpReferencedId = m_id;

  m_id = StringUtils::Format("#{}[{}]", tmpReferencedId, StringUtils::CreateUUID());
  m_referencedId = tmpReferencedId;
}

bool CSetting::IsVisible() const
{
  if (!ISetting::IsVisible())
    return false;

  bool visible = true;
  for (SettingDependencies::const_iterator dep = m_dependencies.begin(); dep != m_dependencies.end(); ++dep)
  {
    if (dep->GetType() != SettingDependencyType::Visible)
      continue;

    if (!dep->Check())
    {
      visible = false;
      break;
    }
  }

  return visible;
}

bool CSetting::OnSettingChanging(const boost::shared_ptr<const CSetting>& setting)
{
  if (m_callback == NULL)
    return true;

  return m_callback->OnSettingChanging(setting);
}

void CSetting::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingChanged(setting);
}

void CSetting::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingAction(setting);
}

bool CSetting::DeserializeIdentification(const TiXmlNode* node,
                                         std::string& identification,
                                         bool& isReference)
{
  isReference = false;

  // first check if we can simply retrieve the setting's identifier
  if (ISetting::DeserializeIdentification(node, identification))
    return true;

  // otherwise try to retrieve a reference to another setting's identifier
  if (!DeserializeIdentificationFromAttribute(node, SETTING_XML_ATTR_REFERENCE, identification))
    return false;

  isReference = true;
  return true;
}

bool CSetting::OnSettingUpdate(const boost::shared_ptr<CSetting>& setting,
                               const char* oldSettingId,
                               const TiXmlNode* oldSettingNode)
{
  if (m_callback == NULL)
    return false;

  return m_callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);
}

void CSetting::OnSettingPropertyChanged(const boost::shared_ptr<const CSetting>& setting,
                                        const char* propertyName)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingPropertyChanged(setting, propertyName);
}

void CSetting::Copy(const CSetting &setting)
{
  SetVisible(setting.IsVisible());
  SetLabel(setting.GetLabel());
  SetHelp(setting.GetHelp());
  SetRequirementsMet(setting.MeetsRequirements());
  m_callback = setting.m_callback;
  m_level = setting.m_level;

  if (setting.m_control != NULL)
  {
    m_control = m_settingsManager->CreateControl(setting.m_control->GetType());
    *m_control = *setting.m_control;
  }
  else
    m_control.reset();

  m_dependencies = setting.m_dependencies;
  m_updates = setting.m_updates;
  m_changed = setting.m_changed;
}

CSettingList::CSettingList(const std::string& id,
                           boost::shared_ptr<CSetting> settingDefinition,
                           CSettingsManager* settingsManager /* = NULL */)
  : CSetting(id, settingsManager), m_definition(boost::move(settingDefinition)), m_delimiter("|"), m_minimumItems(0), m_maximumItems(1)
{
}

CSettingList::CSettingList(const std::string& id,
                           boost::shared_ptr<CSetting> settingDefinition,
                           int label,
                           CSettingsManager* settingsManager /* = NULL */)
  : CSetting(id, settingsManager), m_definition(boost::move(settingDefinition)), m_delimiter("|"), m_minimumItems(0), m_maximumItems(1)
{
  SetLabel(label);
}

CSettingList::CSettingList(const std::string &id, const CSettingList &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

SettingPtr CSettingList::Clone(const std::string &id) const
{
  if (m_definition == NULL)
    return SettingPtr();

  return boost::make_shared<CSettingList>(id, *this);
}

void CSettingList::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::List)
    return;

  const CSettingList &listSetting = static_cast<const CSettingList&>(other);
  if (m_definition == NULL && listSetting.m_definition != NULL)
    m_definition = listSetting.m_definition;
  if (m_defaults.empty() && !listSetting.m_defaults.empty())
    m_defaults = listSetting.m_defaults;
  if (m_values.empty() && !listSetting.m_values.empty())
    m_values = listSetting.m_values;
  if (m_delimiter == "|" && listSetting.m_delimiter != "|")
    m_delimiter = listSetting.m_delimiter;
  if (m_minimumItems == 0 && listSetting.m_minimumItems != 0)
    m_minimumItems = listSetting.m_minimumItems;
  if (m_maximumItems == -1 && listSetting.m_maximumItems != -1)
    m_maximumItems = listSetting.m_maximumItems;
}

bool CSettingList::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (m_definition == NULL)
    return false;

  if (!CSetting::Deserialize(node, update))
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
  {
    CLog::Log(LOGWARNING, "unable to read type of list setting of {}", m_id);
    return false;
  }

  // deserialize the setting definition in update mode because we don't care
  // about an invalid <default> value (which is never used)
  if (!m_definition->Deserialize(node, true))
    return false;

  const TiXmlNode *constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // read the delimiter
    std::string delimiter;
    if (XMLUtils::GetString(constraints, SETTING_XML_ELM_DELIMITER, delimiter) && !delimiter.empty())
      m_delimiter = delimiter;

    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MINIMUM_ITEMS, m_minimumItems);
    if (m_minimumItems < 0)
      m_minimumItems = 0;
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MAXIMUM_ITEMS, m_maximumItems);
    if (m_maximumItems <= 0)
      m_maximumItems = -1;
    else if (m_maximumItems < m_minimumItems)
    {
      CLog::Log(LOGWARNING, "invalid <{}> ({}) and/or <{}> ({}) of {}", SETTING_XML_ELM_MINIMUM_ITEMS,
                     m_minimumItems, SETTING_XML_ELM_MAXIMUM_ITEMS, m_maximumItems, m_id);
      return false;
    }
  }

  // read the default and initial values
  std::string values;
  if (XMLUtils::GetString(node, SETTING_XML_ELM_DEFAULT, values))
  {
    if (!fromString(values, m_defaults))
    {
      CLog::Log(LOGWARNING, "invalid <{}> definition \"{}\" of {}", SETTING_XML_ELM_DEFAULT, values, m_id);
      return false;
    }
    Reset();
  }

  return true;
}

SettingType::Type CSettingList::GetElementType() const
{
  CSharedLock lock(m_critical);

  if (m_definition == NULL)
    return SettingType::Unknown;

  return m_definition->GetType();
}

bool CSettingList::FromString(const std::string &value)
{
  SettingList values;
  if (!fromString(value, values))
    return false;

  return SetValue(values);
}

std::string CSettingList::ToString() const
{
  return toString(m_values);
}

bool CSettingList::Equals(const std::string &value) const
{
  SettingList values;
  if (!fromString(value, values) || values.size() != m_values.size())
    return false;

  bool ret = true;
  for (size_t index = 0; index < values.size(); index++)
  {
    if (!m_values[index]->Equals(values[index]->ToString()))
    {
      ret = false;
      break;
    }
  }

  return ret;
}

bool CSettingList::CheckValidity(const std::string &value) const
{
  SettingList values;
  return fromString(value, values);
}

void CSettingList::Reset()
{
  CExclusiveLock lock(m_critical);
  SettingList values;
  for (SettingList::const_iterator it = m_defaults.begin(); it != m_defaults.end(); ++it)
    values.push_back((*it)->Clone((*it)->GetId()));

  SetValue(values);
}

bool CSettingList::FromString(const std::vector<std::string> &value)
{
  SettingList values;
  if (!fromValues(value, values))
    return false;

  return SetValue(values);
}

bool CSettingList::SetValue(const SettingList &values)
{
  CExclusiveLock lock(m_critical);

  if ((int)values.size() < m_minimumItems ||
     (m_maximumItems > 0 && (int)values.size() > m_maximumItems))
    return false;

  bool equal = values.size() == m_values.size();
  for (size_t index = 0; index < values.size(); index++)
  {
    if (values[index]->GetType() != GetElementType())
      return false;

    if (equal &&
        !values[index]->Equals(m_values[index]->ToString()))
      equal = false;
  }

  if (equal)
    return true;

  SettingList oldValues = m_values;
  m_values.clear();
  m_values.insert(m_values.begin(), values.begin(), values.end());

  if (!OnSettingChanging(shared_from_base<CSettingList>()))
  {
    m_values = oldValues;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingList>());
    return false;
  }

  m_changed = toString(m_values) != toString(m_defaults);
  OnSettingChanged(shared_from_base<CSettingList>());
  return true;
}

void CSettingList::SetDefault(const SettingList &values)
{
  CExclusiveLock lock(m_critical);

  m_defaults.clear();
  m_defaults.insert(m_defaults.begin(), values.begin(), values.end());

  if (!m_changed)
  {
    m_values.clear();
    for (SettingList::const_iterator it = m_defaults.begin(); it != m_defaults.end(); ++it)
      m_values.push_back((*it)->Clone((*it)->GetId()));
  }
}

void CSettingList::copy(const CSettingList &setting)
{
  CSetting::Copy(setting);

  copy(setting.m_values, m_values);
  copy(setting.m_defaults, m_defaults);

  if (setting.m_definition != NULL)
  {
    SettingPtr definitionCopy = setting.m_definition->Clone(m_id + ".definition");
    if (definitionCopy != NULL)
      m_definition = definitionCopy;
  }

  m_delimiter = setting.m_delimiter;
  m_minimumItems = setting.m_minimumItems;
  m_maximumItems = setting.m_maximumItems;
}

void CSettingList::copy(const SettingList &srcValues, SettingList &dstValues)
{
  dstValues.clear();

  for (SettingList::const_iterator value = srcValues.begin(); value != srcValues.end(); ++value)
  {
    if (*value == NULL)
      continue;

    SettingPtr valueCopy = (*value)->Clone((*value)->GetId());
    if (valueCopy == NULL)
      continue;

    dstValues.push_back(SettingPtr(valueCopy));
  }
}

bool CSettingList::fromString(const std::string &strValue, SettingList &values) const
{
  return fromValues(StringUtils::Split(strValue, m_delimiter), values);
}

bool CSettingList::fromValues(const std::vector<std::string> &strValues, SettingList &values) const
{
  if ((int)strValues.size() < m_minimumItems ||
     (m_maximumItems > 0 && (int)strValues.size() > m_maximumItems))
    return false;

  bool ret = true;
  int index = 0;
  for (std::vector<std::string>::const_iterator value = strValues.begin(); value != strValues.end(); ++value)
  {
    SettingPtr settingValue = m_definition->Clone(StringUtils::Format("{}.{}", m_id, index++));
    if (settingValue == NULL ||
        !settingValue->FromString(*value))
    {
      ret = false;
      break;
    }

    values.push_back(SettingPtr(settingValue));
  }

  if (!ret)
    values.clear();

  return ret;
}

std::string CSettingList::toString(const SettingList &values) const
{
  std::vector<std::string> strValues;
  for (SettingList::const_iterator value = values.begin(); value != values.end(); ++value)
  {
    if (value != NULL)
      strValues.push_back((*value)->ToString());
  }

  return StringUtils::Join(strValues, m_delimiter);
}

const CSettingBool::Value CSettingBool::DefaultValue = false;

CSettingBool::CSettingBool(const std::string& id, CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<bool, SettingType::Boolean>(id, settingsManager), m_value(CSettingBool::DefaultValue), m_default(CSettingBool::DefaultValue)
{
  SetLabel(CSettingBool::DefaultLabel);
}

CSettingBool::CSettingBool(const std::string& id, const CSettingBool& setting)
  : CTraitedSetting<bool, SettingType::Boolean>(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingBool::CSettingBool(const std::string& id,
                           int label,
                           bool value,
                           CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<bool, SettingType::Boolean>(id, settingsManager), m_value(value), m_default(value)
{
  SetLabel(label);
}

SettingPtr CSettingBool::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingBool>(id, *this);
}

void CSettingBool::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Boolean)
    return;

  const CSettingBool &boolSetting = static_cast<const CSettingBool&>(other);
  if (m_default == false && boolSetting.m_default == true)
    m_default = boolSetting.m_default;
  if (m_value == m_default && boolSetting.m_value != m_default)
    m_value = boolSetting.m_value;
}

bool CSettingBool::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  // get the default value
  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "error reading the default value of \"{}\"", m_id);
    return false;
  }

  return true;
}

bool CSettingBool::FromString(const std::string &value)
{
  bool bValue;
  if (!fromString(value, bValue))
    return false;

  return SetValue(bValue);
}

std::string CSettingBool::ToString() const
{
  return m_value ? "true" : "false";
}

bool CSettingBool::Equals(const std::string &value) const
{
  bool bValue;
  return (fromString(value, bValue) && m_value == bValue);
}

bool CSettingBool::CheckValidity(const std::string &value) const
{
  bool bValue;
  return fromString(value, bValue);
}

bool CSettingBool::SetValue(bool value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  bool oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingBool>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingBool>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingBool>());
  return true;
}

void CSettingBool::SetDefault(bool value)
{
  CExclusiveLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

void CSettingBool::copy(const CSettingBool &setting)
{
  CSetting::Copy(setting);

  m_value = setting.m_value;
  m_default = setting.m_default;
}

bool CSettingBool::fromString(const std::string &strValue, bool &value) const
{
  if (StringUtils::EqualsNoCase(strValue, "true"))
  {
    value = true;
    return true;
  }
  if (StringUtils::EqualsNoCase(strValue, "false"))
  {
    value = false;
    return true;
  }

  return false;
}

const CSettingInt::DefaultValue = 0;
const CSettingInt::DefaultMin = CSettingInt::DefaultValue;
const CSettingInt::DefaultStep = 1;
const CSettingInt::DefaultMax = CSettingInt::DefaultValue;

CSettingInt::CSettingInt(const std::string& id, CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<int, SettingType::Integer>(id, settingsManager),
    m_value(CSettingInt::DefaultValue),
    m_default(CSettingInt::DefaultValue),
    m_min(CSettingInt::DefaultMin),
    m_step(CSettingInt::DefaultStep),
    m_max(CSettingInt::DefaultMax),
    m_optionsFiller(NULL),
    m_optionsFillerData(NULL),
    m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(CSettingInt::DefaultLabel);
}

CSettingInt::CSettingInt(const std::string& id, const CSettingInt& setting)
  : CTraitedSetting<int, SettingType::Integer>(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<int, SettingType::Integer>(id, settingsManager),
    m_value(value),
    m_default(value),
    m_min(CSettingInt::DefaultMin),
    m_step(CSettingInt::DefaultStep),
    m_max(CSettingInt::DefaultMax),
    m_optionsFiller(NULL),
    m_optionsFillerData(NULL),
    m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(label);
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         int minimum,
                         int step,
                         int maximum,
                         CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<int, SettingType::Integer>(id, settingsManager),
    m_value(value),
    m_default(value),
    m_min(minimum),
    m_step(step),
    m_max(maximum),
    m_optionsFiller(NULL),
    m_optionsFillerData(NULL),
    m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(label);
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         const TranslatableIntegerSettingOptions& options,
                         CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<int, SettingType::Integer>(id, settingsManager),
    m_value(value),
    m_default(value),
    m_min(CSettingInt::DefaultMin),
    m_step(CSettingInt::DefaultStep),
    m_max(CSettingInt::DefaultMax),
    m_optionsFiller(NULL),
    m_optionsFillerData(NULL),
    m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(label);
  SetTranslatableOptions(options);
}

SettingPtr CSettingInt::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingInt>(id, *this);
}

void CSettingInt::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Integer)
    return;

  const CSettingInt &intSetting = static_cast<const CSettingInt&>(other);
  if (m_default == 0.0 && intSetting.m_default != 0.0)
    m_default = intSetting.m_default;
  if (m_value == m_default && intSetting.m_value != m_default)
    m_value = intSetting.m_value;
  if (m_min == 0.0 && intSetting.m_min != 0.0)
    m_min = intSetting.m_min;
  if (m_step == 1.0 && intSetting.m_step != 1.0)
    m_step = intSetting.m_step;
  if (m_max == 0.0 && intSetting.m_max != 0.0)
    m_max = intSetting.m_max;
  if (m_translatableOptions.empty() && !intSetting.m_translatableOptions.empty())
    m_translatableOptions = intSetting.m_translatableOptions;
  if (m_options.empty() && !intSetting.m_options.empty())
    m_options = intSetting.m_options;
  if (m_optionsFillerName.empty() && !intSetting.m_optionsFillerName.empty())
    m_optionsFillerName = intSetting.m_optionsFillerName;
  if (m_optionsFiller == NULL && intSetting.m_optionsFiller != NULL)
    m_optionsFiller = intSetting.m_optionsFiller;
  if (m_optionsFillerData == NULL && intSetting.m_optionsFillerData != NULL)
    m_optionsFillerData = intSetting.m_optionsFillerData;
  if (m_dynamicOptions.empty() && !intSetting.m_dynamicOptions.empty())
    m_dynamicOptions = intSetting.m_dynamicOptions;
  if (m_optionsSort == SettingOptionsSort::NoSorting &&
      intSetting.m_optionsSort != SettingOptionsSort::NoSorting)
    m_optionsSort = intSetting.m_optionsSort;
}

bool CSettingInt::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  // get the default value
  int value;
  if (XMLUtils::GetInt(node, SETTING_XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "error reading the default value of \"{}\"", m_id);
    return false;
  }

  const TiXmlNode *constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get the entries
    const TiXmlElement *options = constraints->FirstChildElement(SETTING_XML_ELM_OPTIONS);
    if (options != NULL && options->FirstChild() != NULL)
    {
      if (!DeserializeOptionsSort(options, m_optionsSort))
        CLog::Log(LOGWARNING, "invalid \"sort\" attribute of <" SETTING_XML_ELM_OPTIONS "> for \"{}\"",
                       m_id);

      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<IntegerSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingInt>()));
        }
      }
      else
      {
        m_translatableOptions.clear();
        const TiXmlElement *optionElement = options->FirstChildElement(SETTING_XML_ELM_OPTION);
        while (optionElement != NULL)
        {
          TranslatableIntegerSettingOption entry;
          if (optionElement->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &entry.label) ==
                  TIXML_SUCCESS &&
              entry.label > 0)
          {
            entry.value = strtol(optionElement->FirstChild()->Value(), NULL, 10);
            m_translatableOptions.push_back(entry);
          }
          else
          {
            std::string label;
            if (optionElement->QueryStringAttribute(SETTING_XML_ATTR_LABEL, &label) ==
                TIXML_SUCCESS)
            {
              int value = strtol(optionElement->FirstChild()->Value(), NULL, 10);
              m_options.push_back(IntegerSettingOption(label, value));
            }
          }

          optionElement = optionElement->NextSiblingElement(SETTING_XML_ELM_OPTION);
        }
      }
    }

    // get minimum
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MINIMUM, m_min);
    // get step
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_STEP, m_step);
    // get maximum
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MAXIMUM, m_max);
  }

  return true;
}

bool CSettingInt::FromString(const std::string &value)
{
  int iValue;
  if (!fromString(value, iValue))
    return false;

  return SetValue(iValue);
}

std::string CSettingInt::ToString() const
{
  std::ostringstream oss;
  oss << m_value;

  return oss.str();
}

bool CSettingInt::Equals(const std::string &value) const
{
  int iValue;
  return (fromString(value, iValue) && m_value == iValue);
}

bool CSettingInt::CheckValidity(const std::string &value) const
{
  int iValue;
  if (!fromString(value, iValue))
    return false;

  return CheckValidity(iValue);
}

bool CSettingInt::CheckValidity(int value) const
{
  if (!m_translatableOptions.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_translatableOptions))
      return false;
  }
  else if (!m_options.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_options))
      return false;
  }
  else if (m_optionsFillerName.empty() && m_optionsFiller == NULL &&
           m_min != m_max && (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingInt::SetValue(int value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  int oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingInt>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingInt>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingInt>());
  return true;
}

void CSettingInt::SetDefault(int value)
{
  CExclusiveLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType::Type CSettingInt::GetOptionsType() const
{
  CSharedLock lock(m_critical);
  if (!m_translatableOptions.empty())
    return SettingOptionsType::StaticTranslatable;
  if (!m_options.empty())
    return SettingOptionsType::Static;
  if (!m_optionsFillerName.empty() || m_optionsFiller != NULL)
    return SettingOptionsType::Dynamic;

  return SettingOptionsType::Unknown;
}

IntegerSettingOptions CSettingInt::UpdateDynamicOptions()
{
  CExclusiveLock lock(m_critical);
  IntegerSettingOptions options;
  if (m_optionsFiller == NULL &&
     (m_optionsFillerName.empty() || m_settingsManager == NULL))
    return options;

  if (m_optionsFiller == NULL)
  {
    m_optionsFiller = reinterpret_cast<IntegerSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingInt>()));
    if (m_optionsFiller == NULL)
    {
      CLog::Log(LOGWARNING, "unknown options filler \"{}\" of \"{}\"", m_optionsFillerName, m_id);
      return options;
    }
  }

  int bestMatchingValue = m_value;
  m_optionsFiller(shared_from_base<CSettingInt>(), options, bestMatchingValue, m_optionsFillerData);

  if (bestMatchingValue != m_value)
    SetValue(bestMatchingValue);

  bool changed = m_dynamicOptions.size() != options.size();
  if (!changed)
  {
    for (size_t index = 0; index < options.size(); index++)
    {
      if (options[index].label.compare(m_dynamicOptions[index].label) != 0 ||
          options[index].value != m_dynamicOptions[index].value)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    m_dynamicOptions = options;
    OnSettingPropertyChanged(shared_from_base<CSettingInt>(), "options");
  }

  return options;
}

void CSettingInt::copy(const CSettingInt &setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_min = setting.m_min;
  m_step = setting.m_step;
  m_max = setting.m_max;
  m_translatableOptions = setting.m_translatableOptions;
  m_options = setting.m_options;
  m_optionsFillerName = setting.m_optionsFillerName;
  m_optionsFiller = setting.m_optionsFiller;
  m_optionsFillerData = setting.m_optionsFillerData;
  m_dynamicOptions = setting.m_dynamicOptions;
}

bool CSettingInt::fromString(const std::string &strValue, int &value)
{
  if (strValue.empty())
    return false;

  char *end = NULL;
  value = (int)strtol(strValue.c_str(), &end, 10);
  if (end != NULL && *end != '\0')
    return false;

  return true;
}

const CSettingNumber::Value CSettingNumber::DefaultValue = 0.0;
const CSettingNumber::Value CSettingNumber::DefaultMin = CSettingNumber::DefaultValue;
const CSettingNumber::Value CSettingNumber::DefaultStep = 1.0;
const CSettingNumber::Value CSettingNumber::DefaultMax = CSettingNumber::DefaultValue;

CSettingNumber::CSettingNumber(const std::string& id,
                               CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<double, SettingType::Number>(id, settingsManager),
    m_value(CSettingNumber::DefaultValue),
    m_default(CSettingNumber::DefaultValue),
    m_min(CSettingNumber::DefaultMin),
    m_step(CSettingNumber::DefaultStep),
    m_max(CSettingNumber::DefaultMax)
{
  SetLabel(CSettingNumber::DefaultLabel);
}

CSettingNumber::CSettingNumber(const std::string& id, const CSettingNumber& setting)
  : CTraitedSetting<double, SettingType::Number>(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingNumber::CSettingNumber(const std::string& id,
                               int label,
                               float value,
                               CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<double, SettingType::Number>(id, settingsManager),
    m_value(static_cast<double>(value)),
    m_default(static_cast<double>(value)),
    m_min(CSettingNumber::DefaultMin),
    m_step(CSettingNumber::DefaultStep),
    m_max(CSettingNumber::DefaultMax)
{
  SetLabel(label);
}

CSettingNumber::CSettingNumber(const std::string& id,
                               int label,
                               float value,
                               float minimum,
                               float step,
                               float maximum,
                               CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<double, SettingType::Number>(id, settingsManager),
    m_value(static_cast<double>(value)),
    m_default(static_cast<double>(value)),
    m_min(static_cast<double>(minimum)),
    m_step(static_cast<double>(step)),
    m_max(static_cast<double>(maximum))
{
  SetLabel(label);
}

SettingPtr CSettingNumber::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingNumber>(id, *this);
}

void CSettingNumber::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Number)
    return;

  const CSettingNumber &numberSetting = static_cast<const CSettingNumber&>(other);
  if (m_default == 0.0 && numberSetting.m_default != 0.0)
    m_default = numberSetting.m_default;
  if (m_value == m_default && numberSetting.m_value != m_default)
    m_value = numberSetting.m_value;
  if (m_min == 0.0 && numberSetting.m_min != 0.0)
    m_min = numberSetting.m_min;
  if (m_step == 1.0 && numberSetting.m_step != 1.0)
    m_step = numberSetting.m_step;
  if (m_max == 0.0 && numberSetting.m_max != 0.0)
    m_max = numberSetting.m_max;
}

bool CSettingNumber::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  // get the default value
  double value;
  if (XMLUtils::GetDouble(node, SETTING_XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "error reading the default value of \"{}\"", m_id);
    return false;
  }

  const TiXmlNode *constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get the minimum value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_MINIMUM, m_min);
    // get the step value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_STEP, m_step);
    // get the maximum value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_MAXIMUM, m_max);
  }

  return true;
}

bool CSettingNumber::FromString(const std::string &value)
{
  double dValue;
  if (!fromString(value, dValue))
    return false;

  return SetValue(dValue);
}

std::string CSettingNumber::ToString() const
{
  std::ostringstream oss;
  oss << m_value;

  return oss.str();
}

bool CSettingNumber::Equals(const std::string &value) const
{
  double dValue;
  CSharedLock lock(m_critical);
  return (fromString(value, dValue) && m_value == dValue);
}

bool CSettingNumber::CheckValidity(const std::string &value) const
{
  double dValue;
  if (!fromString(value, dValue))
    return false;

  return CheckValidity(dValue);
}

bool CSettingNumber::CheckValidity(double value) const
{
  CSharedLock lock(m_critical);
  if (m_min != m_max &&
     (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingNumber::SetValue(double value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  double oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingNumber>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingNumber>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingNumber>());
  return true;
}

void CSettingNumber::SetDefault(double value)
{
  CExclusiveLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

void CSettingNumber::copy(const CSettingNumber &setting)
{
  CSetting::Copy(setting);
  CExclusiveLock lock(m_critical);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_min = setting.m_min;
  m_step = setting.m_step;
  m_max = setting.m_max;
}

bool CSettingNumber::fromString(const std::string &strValue, double &value)
{
  if (strValue.empty())
    return false;

  char *end = NULL;
  value = strtod(strValue.c_str(), &end);
  if (end != NULL && *end != '\0')
    return false;

  return true;
}

const CSettingString::Value CSettingString::DefaultValue;

CSettingString::CSettingString(const std::string& id,
                               CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<std::string, SettingType::String>(id, settingsManager), m_value(CSettingString::DefaultValue), m_default(CSettingString::DefaultValue), m_allowEmpty(false), m_allowNewOption(false), m_optionsFiller(NULL), m_optionsFillerData(NULL), m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(CSettingString::DefaultLabel);
}

CSettingString::CSettingString(const std::string& id, const CSettingString& setting)
  : CTraitedSetting<std::string, SettingType::String>(id, setting.m_settingsManager), m_optionsSort(SettingOptionsSort::NoSorting)
{
  copy(setting);
}

CSettingString::CSettingString(const std::string& id,
                               int label,
                               const std::string& value,
                               CSettingsManager* settingsManager /* = NULL */)
  : CTraitedSetting<std::string, SettingType::String>(id, settingsManager), m_value(value), m_default(value), m_allowEmpty(false), m_allowNewOption(false), m_optionsFiller(NULL), m_optionsFillerData(NULL), m_optionsSort(SettingOptionsSort::NoSorting)
{
  SetLabel(label);
}

SettingPtr CSettingString::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingString>(id, *this);
}

void CSettingString::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::String)
    return;

  const CSettingString &stringSetting = static_cast<const CSettingString&>(other);
  if (m_default.empty() && !stringSetting.m_default.empty())
    m_default = stringSetting.m_default;
  if (m_value == m_default && stringSetting.m_value != m_default)
    m_value = stringSetting.m_value;
  if (m_allowEmpty == false && stringSetting.m_allowEmpty == true)
    m_allowEmpty = stringSetting.m_allowEmpty;
  if (m_allowNewOption == false && stringSetting.m_allowNewOption == true)
    m_allowNewOption = stringSetting.m_allowNewOption;
  if (m_translatableOptions.empty() && !stringSetting.m_translatableOptions.empty())
    m_translatableOptions = stringSetting.m_translatableOptions;
  if (m_options.empty() && !stringSetting.m_options.empty())
    m_options = stringSetting.m_options;
  if (m_optionsFillerName.empty() && !stringSetting.m_optionsFillerName.empty())
    m_optionsFillerName = stringSetting.m_optionsFillerName;
  if (m_optionsFiller == NULL && stringSetting.m_optionsFiller != NULL)
    m_optionsFiller = stringSetting.m_optionsFiller;
  if (m_optionsFillerData == NULL && stringSetting.m_optionsFillerData != NULL)
    m_optionsFillerData = stringSetting.m_optionsFillerData;
  if (m_dynamicOptions.empty() && !stringSetting.m_dynamicOptions.empty())
    m_dynamicOptions = stringSetting.m_dynamicOptions;
  if (m_optionsSort == SettingOptionsSort::NoSorting &&
      stringSetting.m_optionsSort != SettingOptionsSort::NoSorting)
    m_optionsSort = stringSetting.m_optionsSort;
}

bool CSettingString::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  const TiXmlNode *constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get allowempty (needs to be parsed before parsing the default value)
    XMLUtils::GetBoolean(constraints, SETTING_XML_ELM_ALLOWEMPTY, m_allowEmpty);

    // Values other than those in options constraints allowed to be added
    XMLUtils::GetBoolean(constraints, SETTING_XML_ELM_ALLOWNEWOPTION, m_allowNewOption);

    // get the entries
    const TiXmlElement *options = constraints->FirstChildElement(SETTING_XML_ELM_OPTIONS);
    if (options != NULL && options->FirstChild() != NULL)
    {
      if (!DeserializeOptionsSort(options, m_optionsSort))
        CLog::Log(LOGWARNING, "invalid \"sort\" attribute of <" SETTING_XML_ELM_OPTIONS "> for \"{}\"",
                       m_id);

      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<StringSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingString>()));
        }
      }
      else
      {
        m_translatableOptions.clear();
        const TiXmlElement *optionElement = options->FirstChildElement(SETTING_XML_ELM_OPTION);
        while (optionElement != NULL)
        {
          TranslatableStringSettingOption entry;
          if (optionElement->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &entry.first) == TIXML_SUCCESS && entry.first > 0)
          {
            entry.second = optionElement->FirstChild()->Value();
            m_translatableOptions.push_back(entry);
          }
          else
          {
            const std::string value = optionElement->FirstChild()->Value();
            // if a specific "label" attribute is present use it otherwise use the value as label
            std::string label = value;
            optionElement->QueryStringAttribute(SETTING_XML_ATTR_LABEL, &label);

            m_options.push_back(StringSettingOption(label, value));
          }

          optionElement = optionElement->NextSiblingElement(SETTING_XML_ELM_OPTION);
        }
      }
    }
  }

  // get the default value
  std::string value;
  if (XMLUtils::GetString(node, SETTING_XML_ELM_DEFAULT, value) &&
     (!value.empty() || m_allowEmpty))
    m_value = m_default = value;
  else if (!update && !m_allowEmpty)
  {
    CLog::Log(LOGERROR, "error reading the default value of \"{}\"", m_id);
    return false;
  }

  return true;
}

bool CSettingString::CheckValidity(const std::string &value) const
{
  CSharedLock lock(m_critical);
  if (!m_allowEmpty && value.empty())
    return false;

  if (!m_translatableOptions.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_translatableOptions))
      return false;
  }
  else if (!m_options.empty() && !m_allowNewOption)
  {
    if (!CheckSettingOptionsValidity(value, m_options))
      return false;
  }

  return true;
}

bool CSettingString::SetValue(const std::string &value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  std::string oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingString>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingString>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingString>());
  return true;
}

void CSettingString::SetDefault(const std::string &value)
{
  CSharedLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType::Type CSettingString::GetOptionsType() const
{
  CSharedLock lock(m_critical);
  if (!m_translatableOptions.empty())
    return SettingOptionsType::StaticTranslatable;
  if (!m_options.empty())
    return SettingOptionsType::Static;
  if (!m_optionsFillerName.empty() || m_optionsFiller != NULL)
    return SettingOptionsType::Dynamic;

  return SettingOptionsType::Unknown;
}

StringSettingOptions CSettingString::UpdateDynamicOptions()
{
  CExclusiveLock lock(m_critical);
  StringSettingOptions options;
  if (m_optionsFiller == NULL &&
     (m_optionsFillerName.empty() || m_settingsManager == NULL))
    return options;

  if (m_optionsFiller == NULL)
  {
    m_optionsFiller = reinterpret_cast<StringSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingString>()));
    if (m_optionsFiller == NULL)
    {
      CLog::Log(LOGERROR, "unknown options filler \"{}\" of \"{}\"", m_optionsFillerName, m_id);
      return options;
    }
  }

  std::string bestMatchingValue = m_value;
  m_optionsFiller(shared_from_base<CSettingString>(), options, bestMatchingValue, m_optionsFillerData);

  if (bestMatchingValue != m_value)
    SetValue(bestMatchingValue);

  // check if the list of items has changed
  bool changed = m_dynamicOptions.size() != options.size();
  if (!changed)
  {
    for (size_t index = 0; index < options.size(); index++)
    {
      if (options[index].label.compare(m_dynamicOptions[index].label) != 0 ||
          options[index].value.compare(m_dynamicOptions[index].value) != 0)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    m_dynamicOptions = options;
    OnSettingPropertyChanged(shared_from_base<CSettingString>(), "options");
  }

  return options;
}

void CSettingString::copy(const CSettingString &setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_value = setting.m_value;
  m_default = setting.m_default;
  m_allowEmpty = setting.m_allowEmpty;
  m_allowNewOption = setting.m_allowNewOption;
  m_translatableOptions = setting.m_translatableOptions;
  m_options = setting.m_options;
  m_optionsFillerName = setting.m_optionsFillerName;
  m_optionsFiller = setting.m_optionsFiller;
  m_optionsFillerData = setting.m_optionsFillerData;
  m_dynamicOptions = setting.m_dynamicOptions;
}

CSettingAction::CSettingAction(const std::string& id,
                               CSettingsManager* settingsManager /* = NULL */)
  : CSetting(id, settingsManager)
{
  SetLabel(DefaultLabel);
}

CSettingAction::CSettingAction(const std::string& id,
                               int label,
                               CSettingsManager* settingsManager /* = NULL */)
  : CSetting(id, settingsManager)
{
  SetLabel(label);
}

CSettingAction::CSettingAction(const std::string& id, const CSettingAction& setting)
  : CSetting(id, setting.m_settingsManager)
{
  copy(setting);
}

SettingPtr CSettingAction::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingAction>(id, *this);
}

void CSettingAction::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Action)
    return;

  const CSettingAction &actionSetting = static_cast<const CSettingAction&>(other);
  if (!HasData() && actionSetting.HasData())
    SetData(actionSetting.GetData());
}

bool CSettingAction::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSharedLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  m_data = XMLUtils::GetString(node, SETTING_XML_ELM_DATA);

  return true;
}

void CSettingAction::copy(const CSettingAction& setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_data = setting.m_data;
}
