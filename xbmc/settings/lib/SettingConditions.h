/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SettingDefinitions.h"
#include "utils/BooleanLogic.h"

#include <map>
#include <set>
#include <string>
#include <utility>

class CSettingsManager;
class CSetting;

using SettingConditionCheck = bool (*)(const std::string& condition,
                                       const std::string& value,
                                       const boost::shared_ptr<const CSetting>& setting,
                                       void* data);

class ISettingCondition
{
public:
  explicit ISettingCondition(CSettingsManager *settingsManager)
    : m_settingsManager(settingsManager)
  { }
  virtual ~ISettingCondition() {}

  virtual bool Check() const = 0;

protected:
  CSettingsManager *m_settingsManager;
};

class CSettingConditionItem : public CBooleanLogicValue, public ISettingCondition
{
public:
  explicit CSettingConditionItem(CSettingsManager *settingsManager = NULL)
    : ISettingCondition(settingsManager)
  { }
  virtual ~CSettingConditionItem() {}

  virtual bool Deserialize(const TiXmlNode *node);
  virtual const char* GetTag() const { return SETTING_XML_ELM_CONDITION; }
  virtual bool Check() const;

protected:
  std::string m_name;
  std::string m_setting;
};

class CSettingConditionCombination : public CBooleanLogicOperation, public ISettingCondition
{
public:
  explicit CSettingConditionCombination(CSettingsManager *settingsManager = NULL)
    : ISettingCondition(settingsManager)
  { }
  virtual ~CSettingConditionCombination() {}

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingConditionItem(m_settingsManager); }
};

class CSettingCondition : public CBooleanLogic, public ISettingCondition
{
public:
  explicit CSettingCondition(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingCondition() {}

  virtual bool Check() const;
};

class CSettingConditionsManager
{
public:
  CSettingConditionsManager() {}
  CSettingConditionsManager(const CSettingConditionsManager&) = delete;
  CSettingConditionsManager const& operator=(CSettingConditionsManager const&) = delete;
  virtual ~CSettingConditionsManager() {}

  void AddCondition(std::string condition);
  void AddDynamicCondition(std::string identifier, SettingConditionCheck condition, void *data = NULL);
  void RemoveDynamicCondition(std::string identifier);

  bool Check(
      std::string condition,
      const std::string& value = "",
      const boost::shared_ptr<const CSetting>& setting = boost::shared_ptr<const CSetting>()) const;

private:
  using SettingConditionPair = std::pair<std::string, std::pair<SettingConditionCheck, void*>>;
  using SettingConditionMap = std::map<std::string, std::pair<SettingConditionCheck, void*>>;

  SettingConditionMap m_conditions;
  std::set<std::string> m_defines;
};
