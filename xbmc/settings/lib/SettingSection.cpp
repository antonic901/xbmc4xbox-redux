/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingSection.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <algorithm>

template<class T>
void addISetting(const TiXmlNode* node, const T& item, std::vector<T>& items, bool toBegin = false)
{
  if (node != NULL)
  {
    const TiXmlElement *element = node->ToElement();
    if (element != NULL)
    {
      // check if there is a "before" or "after" attribute to place the setting at a specific position
      int position = -1; // -1 => end, 0 => before, 1 => after
      const char *positionId = element->Attribute(SETTING_XML_ATTR_BEFORE);
      if (positionId != NULL && strlen(positionId) > 0)
        position = 0;
      else if ((positionId = element->Attribute(SETTING_XML_ATTR_AFTER)) != NULL && strlen(positionId) > 0)
        position = 1;

      if (positionId != NULL && strlen(positionId) > 0 && position >= 0)
      {
        for (typename std::vector<T>::iterator it = items.begin(); it != items.end(); ++it)
        {
          if (!StringUtils::EqualsNoCase((*it)->GetId(), positionId))
            continue;

          typename std::vector<T>::iterator positionIt = it;
          if (position == 1)
            ++positionIt;

          items.insert(positionIt, item);
          return;
        }
      }
    }
  }

  if (!toBegin)
    items.push_back(item);
  else
    items.insert(items.begin(), item);
}

CSettingGroup::CSettingGroup(const std::string& id,
                             CSettingsManager* settingsManager /* = NULL */)
  : ISetting(id, settingsManager)
{
}

bool CSettingGroup::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlElement *controlElement = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (controlElement != NULL)
  {
    const char *controlType = controlElement->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == NULL || strlen(controlType) <= 0)
    {
      CLog::Log(LOGERROR, "unable to read control type");
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == NULL)
    {
      CLog::Log(LOGERROR, "unable to create new control \"{}\"", controlType);
      return false;
    }

    if (!m_control->Deserialize(controlElement))
    {
      CLog::Log(LOGWARNING, "unable to read control \"{}\"", controlType);
      m_control.reset();
    }
  }

  const TiXmlElement *settingElement = node->FirstChildElement(SETTING_XML_ELM_SETTING);
  while (settingElement != NULL)
  {
    std::string settingId;
    bool isReference;
    if (CSetting::DeserializeIdentification(settingElement, settingId, isReference))
    {
      SettingList::iterator settingIt = std::find_if(m_settings.begin(), m_settings.end(), boost::bind(&CSetting::GetId, _1) == settingId);

      SettingPtr setting;
      if (settingIt != m_settings.end())
        setting = *settingIt;

      update = (setting != NULL);
      if (!update)
      {
        const char *settingType = settingElement->Attribute(SETTING_XML_ATTR_TYPE);
        if (settingType == NULL || strlen(settingType) <= 0)
        {
          CLog::Log(LOGERROR, "unable to read setting type of \"{}\"", settingId);
          return false;
        }

        setting = m_settingsManager->CreateSetting(settingType, settingId, m_settingsManager);
        if (setting == NULL)
          CLog::Log(LOGERROR, "unknown setting type \"{}\" of \"{}\"", settingType, settingId);
      }

      if (setting == NULL)
        CLog::Log(LOGERROR, "unable to create new setting \"{}\"", settingId);
      else
      {
        if (!setting->Deserialize(settingElement, update))
          CLog::Log(LOGWARNING, "unable to read setting \"{}\"", settingId);
        else
        {
          // if the setting is a reference turn it into one
          if (isReference)
            setting->MakeReference();

          if (!update)
            addISetting(settingElement, setting, m_settings);
        }
      }
    }

    settingElement = settingElement->NextSiblingElement(SETTING_XML_ELM_SETTING);
  }

  return true;
}

SettingList CSettingGroup::GetSettings(SettingLevel::Type level) const
{
  SettingList settings;
  for (SettingList::const_iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
  {
    if ((*setting)->GetLevel() <= level && (*setting)->MeetsRequirements())
      settings.push_back(*setting);
  }

  return settings;
}

static bool IsVisibleSetting(const SettingPtr& setting, SettingLevel::Type level)
{
  return setting->GetLevel() <= level && setting->MeetsRequirements() && setting->IsVisible();
}

bool CSettingGroup::ContainsVisibleSettings(const SettingLevel::Type level) const
{
  return boost::algorithm::any_of(m_settings, boost::bind(IsVisibleSetting, _1, level));
}

void CSettingGroup::AddSetting(const SettingPtr& setting)
{
  addISetting(NULL, setting, m_settings);
}

void CSettingGroup::AddSettings(const SettingList &settings)
{
  for (SettingList::const_iterator setting = settings.begin(); setting != settings.end(); ++setting)
    addISetting(NULL, *setting, m_settings);
}

bool CSettingGroup::ReplaceSetting(const boost::shared_ptr<const CSetting>& currentSetting,
                                   const boost::shared_ptr<CSetting>& newSetting)
{
  for (SettingList::iterator itSetting = m_settings.begin(); itSetting != m_settings.end(); ++itSetting)
  {
    if (*itSetting == currentSetting)
    {
      if (newSetting == NULL)
        m_settings.erase(itSetting);
      else
        *itSetting = newSetting;

      return true;
    }
  }

  return false;
}

CSettingCategory::CSettingCategory(const std::string& id,
                                   CSettingsManager* settingsManager /* = NULL */)
  : ISetting(id, settingsManager),
    m_accessCondition(settingsManager)
{
}

bool CSettingCategory::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlNode *accessNode = node->FirstChild(SETTING_XML_ELM_ACCESS);
  if (accessNode != NULL && !m_accessCondition.Deserialize(accessNode))
    return false;

  const TiXmlNode *groupNode = node->FirstChild(SETTING_XML_ELM_GROUP);
  while (groupNode != NULL)
  {
    std::string groupId;
    if (CSettingGroup::DeserializeIdentification(groupNode, groupId))
    {
      SettingGroupList::iterator groupIt = std::find_if(m_groups.begin(), m_groups.end(), boost::bind(&CSettingGroup::GetId, _1) == groupId);

      SettingGroupPtr group;
      if (groupIt != m_groups.end())
        group = *groupIt;

      update = (group != NULL);
      if (!update)
        group = boost::make_shared<CSettingGroup>(groupId, m_settingsManager);

      if (group->Deserialize(groupNode, update))
      {
        if (!update)
          addISetting(groupNode, group, m_groups);
      }
      else
        CLog::Log(LOGWARNING, "unable to read group \"{}\"", groupId);
    }

    groupNode = groupNode->NextSibling(SETTING_XML_ELM_GROUP);
  }

  return true;
}

SettingGroupList CSettingCategory::GetGroups(SettingLevel::Type level) const
{
  SettingGroupList groups;
  for (SettingGroupList::const_iterator group = m_groups.begin(); group != m_groups.end(); ++group)
  {
    if ((*group)->MeetsRequirements() && (*group)->IsVisible() && (*group)->ContainsVisibleSettings(level))
      groups.push_back(*group);
  }

  return groups;
}

bool CSettingCategory::CanAccess() const
{
  return m_accessCondition.Check();
}

void CSettingCategory::AddGroup(const SettingGroupPtr& group)
{
  addISetting(NULL, group, m_groups, false);
}

void CSettingCategory::AddGroupToFront(const SettingGroupPtr& group)
{
  addISetting(NULL, group, m_groups, true);
}

void CSettingCategory::AddGroups(const SettingGroupList &groups)
{
  for (SettingGroupList::const_iterator group = groups.begin(); group != groups.end(); ++group)
    addISetting(NULL, *group, m_groups);
}

CSettingSection::CSettingSection(const std::string& id,
                                 CSettingsManager* settingsManager /* = NULL */)
  : ISetting(id, settingsManager)
{
}

bool CSettingSection::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlNode *categoryNode = node->FirstChild(SETTING_XML_ELM_CATEGORY);
  while (categoryNode != NULL)
  {
    std::string categoryId;
    if (CSettingCategory::DeserializeIdentification(categoryNode, categoryId))
    {
      SettingCategoryList::iterator categoryIt = std::find_if(m_categories.begin(), m_categories.end(), boost::bind(&CSettingCategory::GetId, _1) == categoryId);

      SettingCategoryPtr category;
      if (categoryIt != m_categories.end())
        category = *categoryIt;

      update = (category != NULL);
      if (!update)
        category = boost::make_shared<CSettingCategory>(categoryId, m_settingsManager);

      if (category->Deserialize(categoryNode, update))
      {
        if (!update)
          addISetting(categoryNode, category, m_categories);
      }
      else
        CLog::Log(LOGWARNING, "unable to read category \"{}\"", categoryId);
    }

    categoryNode = categoryNode->NextSibling(SETTING_XML_ELM_CATEGORY);
  }

  return true;
}

SettingCategoryList CSettingSection::GetCategories(SettingLevel::Type level) const
{
  SettingCategoryList categories;
  for (SettingCategoryList::const_iterator category = m_categories.begin(); category != m_categories.end(); ++category)
  {
    if ((*category)->MeetsRequirements() && (*category)->IsVisible() && (*category)->GetGroups(level).size() > 0)
      categories.push_back(*category);
  }

  return categories;
}

void CSettingSection::AddCategory(const SettingCategoryPtr& category)
{
  addISetting(NULL, category, m_categories);
}

void CSettingSection::AddCategories(const SettingCategoryList &categories)
{
  for (SettingCategoryList::const_iterator category = categories.begin(); category != categories.end(); ++category)
    addISetting(NULL, *category, m_categories);
}
