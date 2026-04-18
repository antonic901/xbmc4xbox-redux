/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingDependency.h"

#include "ServiceBroker.h"
#include "Setting.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <memory>
#include <set>
#include <stdlib.h>
#include <string>

CSettingDependencyCondition::CSettingDependencyCondition(
    CSettingsManager* settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager), m_target(SettingDependencyTarget::Unknown), m_operator(SettingDependencyOperator::Equals)
{
  m_name = "";
  m_setting = "";
  m_value = "";
  m_negated = false;
}

CSettingDependencyCondition::CSettingDependencyCondition(
    const std::string& setting,
    const std::string& value,
    SettingDependencyOperator::Type op,
    bool negated /* = false */,
    CSettingsManager* settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager), m_target(SettingDependencyTarget::Setting), m_operator(op)
{
  m_name = setting;
  m_setting = setting;
  m_value = value;
  m_negated = negated;
}

CSettingDependencyCondition::CSettingDependencyCondition(
    const std::string& strProperty,
    const std::string& value,
    const std::string& setting /* = "" */,
    bool negated /* = false */,
    CSettingsManager* settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager), m_target(SettingDependencyTarget::Property), m_operator(SettingDependencyOperator::Equals)
{
  m_name = strProperty;
  m_setting = setting;
  m_value = value;
  m_negated = negated;
}

CSettingDependencyCondition::CSettingDependencyCondition(
    CSettingsManager* settingsManager,
    const std::string& strProperty,
    const std::string& setting,
    const std::string& value,
    SettingDependencyTarget::Type target /* = SettingDependencyTarget::Unknown */,
    SettingDependencyOperator::Type op /* = SettingDependencyOperator::Equals */,
    bool negated /* = false */)
  : CSettingConditionItem(settingsManager), m_target(target), m_operator(op)
{
  m_name = strProperty;
  m_setting = setting;
  m_value = value;
  m_negated = negated;
}

bool CSettingDependencyCondition::Deserialize(const TiXmlNode *node)
{
  if (!CSettingConditionItem::Deserialize(node))
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  m_target = SettingDependencyTarget::Setting;
  const char *strTarget = elem->Attribute(SETTING_XML_ATTR_ON);
  if (strTarget != NULL && !setTarget(strTarget))
  {
    CLog::Log(LOGWARNING, "unknown target \"{}\"", strTarget);
    return false;
  }

  if (m_target != SettingDependencyTarget::Setting && m_name.empty())
  {
    CLog::Log(LOGWARNING, "missing name for dependency");
    return false;
  }

  if (m_target == SettingDependencyTarget::Setting)
  {
    if (m_setting.empty())
    {
      CLog::Log(LOGWARNING, "missing setting for dependency");
      return false;
    }

    m_name = m_setting;
  }

  m_operator = SettingDependencyOperator::Equals;
  const char *strOperator = elem->Attribute(SETTING_XML_ATTR_OPERATOR);
  if (strOperator != NULL && !setOperator(strOperator))
  {
    CLog::Log(LOGWARNING, "unknown operator \"{}\"", strOperator);
    return false;
  }

  return true;
}

bool CSettingDependencyCondition::Check() const
{
  if (m_name.empty() ||
      m_target == SettingDependencyTarget::Unknown ||
      m_operator == SettingDependencyOperator::Unknown ||
      m_settingsManager == NULL)
    return false;

  bool result = false;
  switch (m_target)
  {
    case SettingDependencyTarget::Setting:
    {
      if (m_setting.empty())
        return false;

      SettingPtr setting = m_settingsManager->GetSetting(m_setting);
      if (setting == NULL)
      {
        CLog::Log(LOGWARNING, "unable to check condition on unknown setting \"{}\"", m_setting);
        return false;
      }

      switch (m_operator)
      {
        case SettingDependencyOperator::Equals:
          result = setting->Equals(m_value);
          break;

        case SettingDependencyOperator::LessThan:
        {
          const std::string value = setting->ToString();
          if (StringUtils::IsInteger(m_value))
            result = strtol(value.c_str(), NULL, 0) < strtol(m_value.c_str(), NULL, 0);
          else
            result = value.compare(m_value) < 0;
          break;
        }

        case SettingDependencyOperator::GreaterThan:
        {
          const std::string value = setting->ToString();
          if (StringUtils::IsInteger(m_value))
            result = strtol(value.c_str(), NULL, 0) > strtol(m_value.c_str(), NULL, 0);
          else
            result = value.compare(m_value) > 0;
          break;
        }

        case SettingDependencyOperator::Contains:
          result = (setting->ToString().find(m_value) != std::string::npos);
          break;

        case SettingDependencyOperator::Unknown:
        default:
          break;
      }

      break;
    }

    case SettingDependencyTarget::Property:
    {
      SettingConstPtr setting;
      if (!m_setting.empty())
      {
        setting = m_settingsManager->GetSetting(m_setting);
        if (setting == NULL)
        {
          CLog::Log(LOGWARNING, "unable to check condition on unknown setting \"{}\"", m_setting);
          return false;
        }
      }
      result = m_settingsManager->GetConditions().Check(m_name, m_value, setting);
      break;
    }

    default:
      return false;
  }

  return result == !m_negated;
}

bool CSettingDependencyCondition::setTarget(const std::string &target)
{
  if (StringUtils::EqualsNoCase(target, "setting"))
    m_target = SettingDependencyTarget::Setting;
  else if (StringUtils::EqualsNoCase(target, "property"))
    m_target = SettingDependencyTarget::Property;
  else
    return false;

  return true;
}

bool CSettingDependencyCondition::setOperator(const std::string &op)
{
  size_t length = 0;
  if (StringUtils::EndsWithNoCase(op, "is"))
  {
    m_operator = SettingDependencyOperator::Equals;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "lessthan"))
  {
    m_operator = SettingDependencyOperator::LessThan;
    length = 8;
  }
  else if (StringUtils::EndsWithNoCase(op, "lt"))
  {
    m_operator = SettingDependencyOperator::LessThan;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "greaterthan"))
  {
    m_operator = SettingDependencyOperator::GreaterThan;
    length = 11;
  }
  else if (StringUtils::EndsWithNoCase(op, "gt"))
  {
    m_operator = SettingDependencyOperator::GreaterThan;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "contains"))
  {
    m_operator = SettingDependencyOperator::Contains;
    length = 8;
  }

  if (op.size() > length + 1)
    return false;
  if (op.size() == length + 1)
  {
    if (!StringUtils::StartsWith(op, "!"))
      return false;
    m_negated = true;
  }

  return true;
}

bool CSettingDependencyConditionCombination::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  size_t numOperations = m_operations.size();
  size_t numValues = m_values.size();

  if (!CSettingConditionCombination::Deserialize(node))
    return false;

  if (numOperations < m_operations.size())
  {
    for (size_t i = numOperations; i < m_operations.size(); i++)
    {
      if (m_operations[i] == NULL)
        continue;

      CSettingDependencyConditionCombination *combination = static_cast<CSettingDependencyConditionCombination*>(m_operations[i].get());
      if (combination == NULL)
        continue;

      const std::set<std::string>& settings = combination->GetSettings();
      m_settings.insert(settings.begin(), settings.end());
    }
  }

  if (numValues < m_values.size())
  {
    for (size_t i = numValues; i < m_values.size(); i++)
    {
      if (m_values[i] == NULL)
        continue;

      CSettingDependencyCondition *condition = static_cast<CSettingDependencyCondition*>(m_values[i].get());
      if (condition == NULL)
        continue;

      std::string settingId = condition->GetSetting();
      if (!settingId.empty())
        m_settings.insert(settingId);
    }
  }

  return true;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(
    const CSettingDependencyConditionPtr& condition)
{
  if (condition != NULL)
  {
    m_values.push_back(condition);

    std::string settingId = condition->GetSetting();
    if (!settingId.empty())
      m_settings.insert(settingId);
  }

  return this;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(
    const CSettingDependencyConditionCombinationPtr& operation)
{
  if (operation != NULL)
  {
    m_operations.push_back(operation);

    const std::set<std::string> &settings = operation->GetSettings();
    m_settings.insert(settings.begin(), settings.end());
  }

  return this;
}

CSettingDependency::CSettingDependency(CSettingsManager* settingsManager /* = NULL */)
  : CSettingCondition(settingsManager), m_type(SettingDependencyType::Unknown)
{
}

CSettingDependency::CSettingDependency(SettingDependencyType::Type type,
                                       CSettingsManager* settingsManager /* = NULL */)
  : CSettingCondition(settingsManager), m_type(type)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));
}

bool CSettingDependency::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == NULL || strlen(strType) <= 0 || !setType(strType))
  {
    CLog::Log(LOGWARNING, "missing or unknown dependency type definition");
    return false;
  }

  return CSettingCondition::Deserialize(node);
}

std::set<std::string> CSettingDependency::GetSettings() const
{
  if (m_operation == NULL)
    return std::set<std::string>();

  CSettingDependencyConditionCombination *combination = static_cast<CSettingDependencyConditionCombination*>(m_operation.get());
  if (combination == NULL)
    return std::set<std::string>();

  return combination->GetSettings();
}

CSettingDependencyConditionCombinationPtr CSettingDependency::And()
{
  if (m_operation == NULL)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationAnd);

  return boost::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

CSettingDependencyConditionCombinationPtr CSettingDependency::Or()
{
  if (m_operation == NULL)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationOr);

  return boost::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

bool CSettingDependency::setType(const std::string &type)
{
  if (StringUtils::EqualsNoCase(type, "enable"))
    m_type = SettingDependencyType::Enable;
  else if (StringUtils::EqualsNoCase(type, "update"))
    m_type = SettingDependencyType::Update;
  else if (StringUtils::EqualsNoCase(type, "visible"))
    m_type = SettingDependencyType::Visible;
  else
    return false;

  return true;
}
