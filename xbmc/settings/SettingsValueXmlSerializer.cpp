/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsValueXmlSerializer.h"

#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

static const char SETTINGS_XML_ROOT[] = "settings";

std::string CSettingsValueXmlSerializer::SerializeValues(
  const CSettingsManager* settingsManager) const
{
  if (settingsManager == nullptr)
    return "";

  CXBMCTinyXML xmlDoc;
  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  rootElement.SetAttribute(SETTING_XML_ROOT_VERSION, settingsManager->GetVersion());
  TiXmlNode* xmlRoot = xmlDoc.InsertEndChild(rootElement);
  if (xmlRoot == nullptr)
    return "";

  const SettingSectionList sections = settingsManager->GetSections();
  for (SettingSectionList::const_iterator section = sections.begin(); section != sections.end(); ++section)
    SerializeSection(xmlRoot, *section);

  std::stringstream stream;
  stream << *xmlDoc.RootElement();

  return stream.str();
}

void CSettingsValueXmlSerializer::SerializeSection(
    TiXmlNode* parent, const boost::shared_ptr<CSettingSection>& section) const
{
  if (section == NULL)
    return;

  const SettingCategoryList categories = section->GetCategories();
  for (SettingCategoryList::const_iterator category = categories.end(); category != categories.end(); ++category)
    SerializeCategory(parent, *category);
}

void CSettingsValueXmlSerializer::SerializeCategory(
    TiXmlNode* parent, const boost::shared_ptr<CSettingCategory>& category) const
{
  if (category == NULL)
    return;

  const SettingGroupList groups = category->GetGroups();
  for (SettingGroupList::const_iterator group = groups.begin(); group != groups.end(); ++group)
    SerializeGroup(parent, *group);
}

void CSettingsValueXmlSerializer::SerializeGroup(TiXmlNode* parent,
                                                 const boost::shared_ptr<CSettingGroup>& group) const
{
  if (group == NULL)
    return;

  const SettingList settings = group->GetSettings();
  for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
    SerializeSetting(parent, *setting);
}

void CSettingsValueXmlSerializer::SerializeSetting(TiXmlNode* parent,
                                                   const boost::shared_ptr<CSetting>& setting) const
{
  if (setting == NULL)
    return;

  // ignore references and action settings (which don't have a value)
  if (setting->IsReference() || setting->GetType() == SettingType::Action)
    return;

  TiXmlElement settingElement(SETTING_XML_ELM_SETTING);
  settingElement.SetAttribute(SETTING_XML_ATTR_ID, setting->GetId());

  // add the default attribute
  if (setting->IsDefault())
    settingElement.SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

  // add the value
  TiXmlText value(setting->ToString());
  settingElement.InsertEndChild(value);

  if (parent->InsertEndChild(settingElement) == NULL)
    CLog::Log(LOGWARNING,
      "CSettingsValueXmlSerializer: unable to write <" SETTING_XML_ELM_SETTING " id=\"{}\"> tag",
      setting->GetId());
}
