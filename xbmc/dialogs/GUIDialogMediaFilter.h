/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQuery.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "settings/lib/SettingType.h"
#include "utils/DatabaseUtils.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CDbUrl;
class CSetting;
class CSmartPlaylist;
class CSmartPlaylistRule;
struct StringSettingOption;

class CGUIDialogMediaFilter : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogMediaFilter();
  virtual ~CGUIDialogMediaFilter();

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);

  static void ShowAndEditMediaFilter(const std::string &path, CSmartPlaylist &filter);

  struct Filter
  {
    std::string mediaType;
    Field field;
    uint32_t label;
    SettingType::Type settingType;
    std::string controlType;
    std::string controlFormat;
    CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator;
    boost::shared_ptr<CSetting> setting;
    CSmartPlaylistRule* rule;
    void* data;
  };

protected:
  // specializations of CGUIWindow
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save() { return true; }
  virtual unsigned int GetDelayMs() const { return 500; }

  // specialization of CGUIDialogSettingsManualBase
  virtual void SetupView();
  virtual void InitializeSettings();

  bool SetPath(const std::string &path);
  void UpdateControls();
  void TriggerFilter() const;
  void Reset(bool filtersOnly = false);

  int GetItems(const Filter &filter, std::vector<std::string> &items, bool countOnly = false);
  void GetRange(const Filter &filter, int &min, int &interval, int &max);
  void GetRange(const Filter &filter, float &min, float &interval, float &max);
  bool GetMinMax(const std::string &table, const std::string &field, int &min, int &max, const CDatabase::Filter &filter = CDatabase::Filter());

  CSmartPlaylistRule* AddRule(Field field, CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator = CDatabaseQueryRule::OPERATOR_CONTAINS);
  void DeleteRule(Field field);

  static void GetStringListOptions(const boost::shared_ptr<const CSetting>& setting,
                                   std::vector<StringSettingOption>& list,
                                   std::string& current,
                                   void* data);

  CDbUrl* m_dbUrl;
  std::string m_mediaType;
  CSmartPlaylist *m_filter;
  std::map<std::string, Filter> m_filters;
};
