#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include "ISetting.h"
#include "Setting.h"
#include "SettingCategoryAccess.h"

class CSettingsManager;

/*!
  \ingroup settings
  \brief Group of settings being part of a category
  \sa CSettingCategory
  \sa CSetting
  */
class CSettingGroup : public ISetting
{
public:
  CSettingGroup(const std::string &id, CSettingsManager *settingsManager = NULL);
  ~CSettingGroup();

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  const SettingList& GetSettings() const { return m_settings; }
  SettingList GetSettings(SettingLevel level) const;

private:
  SettingList m_settings;
};

typedef std::vector<CSettingGroup *> SettingGroupList;

/*!
  \ingroup settings
  \brief Category of groups of settings being part of a section
  \sa CSettingSection
  \sa CSettingGroup
  */
class CSettingCategory : public ISetting
{
public:
  CSettingCategory(const std::string &id, CSettingsManager *settingsManager = NULL);
  ~CSettingCategory();

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  const int GetLabel() const { return m_label; }
  /*!
   \brief Sets the localizeable label ID of the setting category.
   \param label Localizeable label ID of the setting category
   */
  void SetLabel(int label) { m_label = label; }
  const int GetHelp() const { return m_help; }
  /*!
   \brief Sets the localizeable help ID of the setting category.
   \param label Localizeable help ID of the setting category
   */
  void SetHelp(int help) { m_help = help; }
  const SettingGroupList& GetGroups() const { return m_groups; }
  SettingGroupList GetGroups(SettingLevel level) const;

  bool CanAccess() const;

private:
  int m_label;
  int m_help;
  SettingGroupList m_groups;
  CSettingCategoryAccess m_accessCondition;
};

typedef std::vector<CSettingCategory *> SettingCategoryList;

/*!
  \ingroup settings
  \brief Section of setting categories
  \sa CSettings
  \sa CSettingCategory
  */
class CSettingSection : public ISetting
{
public:
  CSettingSection(const std::string &id, CSettingsManager *settingsManager = NULL);
  ~CSettingSection();

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  const int GetLabel() const { return m_label; }
  /*!
   \brief Sets the localizeable label ID of the setting section.
   \param label Localizeable label ID of the setting section
   */
  void SetLabel(int label) { m_label = label; }
  const int GetHelp() const { return m_help; }
  /*!
   \brief Sets the localizeable help ID of the setting section.
   \param label Localizeable help ID of the setting section
   */
  void SetHelp(int help) { m_help = help; }
  const SettingCategoryList& GetCategories() const { return m_categories; }
  SettingCategoryList GetCategories(SettingLevel level) const;

private:
  int m_label;
  int m_help;
  SettingCategoryList m_categories;
};
