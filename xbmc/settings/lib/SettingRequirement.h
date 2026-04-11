/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SettingConditions.h"

#include <set>
#include <string>

class CSettingRequirementCondition : public CSettingConditionItem
{
public:
  explicit CSettingRequirementCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingRequirementCondition() {}

  virtual bool Check() const;
};

class CSettingRequirementConditionCombination : public CSettingConditionCombination
{
public:
  explicit CSettingRequirementConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingRequirementConditionCombination() {}

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingRequirementConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingRequirementCondition(m_settingsManager); }
};

class CSettingRequirement : public CSettingCondition
{
public:
  explicit CSettingRequirement(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingRequirement() {}
};
