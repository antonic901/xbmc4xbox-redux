/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingUtils.h"

#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>

std::vector<CVariant> CSettingUtils::GetList(const boost::shared_ptr<const CSettingList>& settingList)
{
  return ListToValues(settingList, settingList->GetValue());
}

bool CSettingUtils::SetList(const boost::shared_ptr<CSettingList>& settingList,
                            const std::vector<CVariant>& value)
{
  SettingList newValues;
  if (!ValuesToList(settingList, value, newValues))
    return false;

  return settingList->SetValue(newValues);
}

std::vector<CVariant> CSettingUtils::ListToValues(
    const boost::shared_ptr<const CSettingList>& setting,
    const std::vector<boost::shared_ptr<CSetting> >& values)
{
  std::vector<CVariant> realValues;

  if (setting == NULL)
    return realValues;

  for (std::vector<boost::shared_ptr<CSetting> >::const_iterator value = values.begin(); value != values.end(); ++value)
  {
    switch (setting->GetElementType())
    {
      case SettingType::Boolean:
        realValues.push_back(boost::static_pointer_cast<const CSettingBool>(*value)->GetValue());
        break;

      case SettingType::Integer:
        realValues.push_back(boost::static_pointer_cast<const CSettingInt>(*value)->GetValue());
        break;

      case SettingType::Number:
        realValues.push_back(boost::static_pointer_cast<const CSettingNumber>(*value)->GetValue());
        break;

      case SettingType::String:
        realValues.push_back(boost::static_pointer_cast<const CSettingString>(*value)->GetValue());
        break;

      default:
        break;
    }
  }

  return realValues;
}

bool CSettingUtils::ValuesToList(const boost::shared_ptr<const CSettingList>& setting,
                                 const std::vector<CVariant>& values,
                                 std::vector<boost::shared_ptr<CSetting> >& newValues)
{
  if (setting == NULL)
    return false;

  int index = 0;
  bool ret = true;
  for (std::vector<CVariant>::const_iterator value = values.begin(); value != values.end(); ++value)
  {
    SettingPtr settingValue =
        setting->GetDefinition()->Clone(StringUtils::Format("{}.{}", setting->GetId(), index++));
    if (settingValue == NULL)
      return false;

    switch (setting->GetElementType())
    {
      case SettingType::Boolean:
        if (!value->isBoolean())
          ret = false;
        else
          ret = boost::static_pointer_cast<CSettingBool>(settingValue)->SetValue(value->asBoolean());
        break;

      case SettingType::Integer:
        if (!value->isInteger())
          ret = false;
        else
          ret = boost::static_pointer_cast<CSettingInt>(settingValue)->SetValue(static_cast<int>(value->asInteger()));
        break;

      case SettingType::Number:
        if (!value->isDouble())
          ret = false;
        else
          ret = boost::static_pointer_cast<CSettingNumber>(settingValue)->SetValue(value->asDouble());
        break;

      case SettingType::String:
        if (!value->isString())
          ret = false;
        else
          ret = boost::static_pointer_cast<CSettingString>(settingValue)->SetValue(value->asString());
        break;

      default:
        ret = false;
        break;
    }

    if (!ret)
      return false;

    newValues.push_back(boost::const_pointer_cast<CSetting>(settingValue));
  }

  return true;
}

struct FindMatchingValue
{
  int m_value;
  FindMatchingValue(int value) : m_value(value) {}

  bool operator()(const SettingPtr& setting) const
  {
    return boost::static_pointer_cast<CSettingInt>(setting)->GetValue() == m_value;
  }
};

bool CSettingUtils::FindIntInList(const boost::shared_ptr<const CSettingList>& settingList, int value)
{
  if (settingList == NULL || settingList->GetElementType() != SettingType::Integer)
    return false;

  const SettingList values = settingList->GetValue();
  const SettingList::const_iterator matchingValue =
      std::find_if(values.begin(), values.end(), FindMatchingValue(value));
  return matchingValue != values.end();
}
