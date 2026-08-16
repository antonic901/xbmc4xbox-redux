/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsValueFlatJsonSerializer.h"

#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingType.h"
#include "settings/lib/SettingsManager.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"

CSettingsValueFlatJsonSerializer::CSettingsValueFlatJsonSerializer(bool compact /* = true */)
  : m_compact(compact)
{ }

std::string CSettingsValueFlatJsonSerializer::SerializeValues(
  const CSettingsManager* settingsManager) const
{
  if (settingsManager == NULL)
    return "";

  CVariant root(CVariant::VariantTypeObject);

  const SettingSectionList sections = settingsManager->GetSections();
  for (SettingSectionList::const_iterator section = sections.begin(); section != sections.end(); ++section)
    SerializeSection(root, *section);

  std::string result = CJSONVariantWriter::Write(root, m_compact);
  if (result.empty())
  {
    CLog::Log(LOGWARNING,
      "CSettingsValueFlatJsonSerializer: failed to serialize settings into JSON");
    return "";
  }

  return result;
}

void CSettingsValueFlatJsonSerializer::SerializeSection(
    CVariant& parent, const boost::shared_ptr<CSettingSection>& section) const
{
  if (section == NULL)
    return;

  const SettingCategoryList categories = section->GetCategories();
  for (SettingCategoryList::const_iterator category = categories.begin(); category != categories.end(); ++category)
    SerializeCategory(parent, *category);
}

void CSettingsValueFlatJsonSerializer::SerializeCategory(
    CVariant& parent, const boost::shared_ptr<CSettingCategory>& category) const
{
  if (category == NULL)
    return;

  const SettingGroupList groups = category->GetGroups();
  for (SettingGroupList::const_iterator group = groups.begin(); group != groups.end(); ++group)
    SerializeGroup(parent, *group);
}

void CSettingsValueFlatJsonSerializer::SerializeGroup(
    CVariant& parent, const boost::shared_ptr<CSettingGroup>& group) const
{
  if (group == NULL)
    return;

  const SettingList settings = group->GetSettings();
  for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
    SerializeSetting(parent, *setting);
}

void CSettingsValueFlatJsonSerializer::SerializeSetting(
    CVariant& parent, const boost::shared_ptr<CSetting>& setting) const
{
  if (setting == NULL)
    return;

  // ignore references and action settings (which don't have a value)
  if (setting->IsReference() || setting->GetType() == SettingType::Action)
    return;

  const CVariant valueObj = SerializeSettingValue(setting);
  if (valueObj.isNull())
    return;

  parent[setting->GetId()] = valueObj;
}

CVariant CSettingsValueFlatJsonSerializer::SerializeSettingValue(
    const boost::shared_ptr<CSetting>& setting) const
{
  switch (setting->GetType())
  {
    case SettingType::Action:
      return CVariant::ConstNullVariant;

    case SettingType::Boolean:
      return CVariant(boost::static_pointer_cast<CSettingBool>(setting)->GetValue());

    case SettingType::Integer:
      return CVariant(boost::static_pointer_cast<CSettingInt>(setting)->GetValue());

    case SettingType::Number:
      return CVariant(boost::static_pointer_cast<CSettingNumber>(setting)->GetValue());

    case SettingType::String:
      return CVariant(boost::static_pointer_cast<CSettingString>(setting)->GetValue());

    case SettingType::List:
    {
      const boost::shared_ptr<CSettingList> settingList = boost::static_pointer_cast<CSettingList>(setting);

      CVariant settingListValuesObj(CVariant::VariantTypeArray);
      const SettingList settingListValues = settingList->GetValue();
      for (SettingList::const_iterator settingListValue = settingListValues.begin(); settingListValue != settingListValues.end(); ++settingListValue)
      {
        const CVariant valueObj = SerializeSettingValue(*settingListValue);
        if (!valueObj.isNull())
          settingListValuesObj.push_back(valueObj);
      }

      return settingListValuesObj;
    }

    case SettingType::Unknown:
    default:
      CLog::Log(LOGWARNING,
        "CSettingsValueFlatJsonSerializer: failed to serialize setting \"%s\" with value \"%s\" " \
        "of unknown type", setting->GetId().c_str(), setting->ToString().c_str());
      return CVariant::ConstNullVariant;
  }
}
