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

class CSettingCategoryAccessCondition : public CSettingConditionItem
{
public:
  explicit CSettingCategoryAccessCondition(CSettingsManager *settingsManager = NULL)
    : CSettingConditionItem(settingsManager)
  { }
  virtual ~CSettingCategoryAccessCondition() {}

  virtual bool Check() const;
};

class CSettingCategoryAccessConditionCombination : public CSettingConditionCombination
{
public:
  explicit CSettingCategoryAccessConditionCombination(CSettingsManager *settingsManager = NULL)
    : CSettingConditionCombination(settingsManager)
  { }
  virtual ~CSettingCategoryAccessConditionCombination() {}

  virtual bool Check() const;

private:
  virtual CBooleanLogicOperation* newOperation() { return new CSettingCategoryAccessConditionCombination(m_settingsManager); }
  virtual CBooleanLogicValue* newValue() { return new CSettingCategoryAccessCondition(m_settingsManager); }
};

class CSettingCategoryAccess : public CSettingCondition
{
public:
  explicit CSettingCategoryAccess(CSettingsManager *settingsManager = NULL);
  virtual ~CSettingCategoryAccess() {}
};
