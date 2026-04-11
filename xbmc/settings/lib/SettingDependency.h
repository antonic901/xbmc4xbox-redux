/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SettingConditions.h"
#include "utils/BooleanLogic.h"
#include "utils/logtypes.h"

#include <list>
#include <set>
#include <string>

enum class SettingDependencyType {
  Unknown = 0,
  Enable,
  Update,
  Visible
};

enum class SettingDependencyOperator {
  Unknown = 0,
  Equals,
  LessThan,
  GreaterThan,
  Contains
};

enum class SettingDependencyTarget {
  Unknown = 0,
  Setting,
  Property
};

class CSettingDependencyCondition : public CSettingConditionItem
{
public:
  explicit CSettingDependencyCondition(CSettingsManager *settingsManager = NULL);
  CSettingDependencyCondition(const std::string &setting, const std::string &value,
                              SettingDependencyOperator op, bool negated = false,
                              CSettingsManager *settingsManager = NULL);
  CSettingDependencyCondition(const std::string &strProperty, const std::string &value,
                              const std::string &setting = "", bool negated = false,
                              CSettingsManager *settingsManager = NULL);
  virtual ~CSettingDependencyCondition() {}

  virtual bool Deserialize(const TiXmlNode *node);
  virtual bool Check() const;

  const std::string& GetName() const { return m_name; }
  const std::string& GetSetting() const { return m_setting; }
  SettingDependencyTarget GetTarget() const { return m_target; }
  SettingDependencyOperator GetOperator() const { return m_operator; }

private:
  CSettingDependencyCondition(CSettingsManager* settingsManager,
                              const std::string& strProperty,
                              const std::string& setting,
                              const std::string& value,
                              SettingDependencyTarget target = SettingDependencyTarget::Unknown,
                              SettingDependencyOperator op = SettingDependencyOperator::Equals,
                              bool negated = false);

  bool setTarget(const std::string &target);
  bool setOperator(const std::string &op);

  SettingDependencyTarget m_target = SettingDependencyTarget::Unknown;
  SettingDependencyOperator m_operator = SettingDependencyOperator::Equals;

  static Logger s_logger;
};

using CSettingDependencyConditionPtr = boost::shared_ptr<CSettingDependencyCondition>;

class CSettingDependencyConditionCombination;
using CSettingDependencyConditionCombinationPtr = boost::shared_ptr<CSettingDependencyConditionCombination>;

class CSettingDependencyConditionCombination : public CSettingConditionCombination
{
public:
  explicit CSettingDependencyConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  CSettingDependencyConditionCombination(BooleanLogicOperation op, CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  {
    SetOperation(op);
  }
  virtual ~CSettingDependencyConditionCombination() {}

  virtual bool Deserialize(const TiXmlNode *node);

  const std::set<std::string>& GetSettings() const { return m_settings; }

  CSettingDependencyConditionCombination* Add(const CSettingDependencyConditionPtr& condition);
  CSettingDependencyConditionCombination* Add(
      const CSettingDependencyConditionCombinationPtr& operation);

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingDependencyConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingDependencyCondition(m_settingsManager); }

  std::set<std::string> m_settings;
};

class CSettingDependency : public CSettingCondition
{
public:
  explicit CSettingDependency(CSettingsManager *settingsManager = NULL);
  CSettingDependency(SettingDependencyType type, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingDependency() {}

  virtual bool Deserialize(const TiXmlNode *node);

  SettingDependencyType GetType() const { return m_type; }
  std::set<std::string> GetSettings() const;

  CSettingDependencyConditionCombinationPtr And();
  CSettingDependencyConditionCombinationPtr Or();

private:
  bool setType(const std::string &type);

  SettingDependencyType m_type = SettingDependencyType::Unknown;

  static Logger s_logger;
};

using SettingDependencies = std::list<CSettingDependency>;
using SettingDependencyMap = std::map<std::string, SettingDependencies>;
