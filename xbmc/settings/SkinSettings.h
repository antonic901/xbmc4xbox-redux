/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Skin.h"
#include "settings/ISubSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <set>
#include <string>

class TiXmlNode;

class CSkinSettings : public ISubSettings
{
public:
  static CSkinSettings& GetInstance();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  void MigrateSettings(const boost::shared_ptr<ADDON::CSkinInfo>& skin);

  int TranslateString(const std::string &setting);
  const std::string& GetString(int setting) const;
  void SetString(int setting, const std::string &label);

  int TranslateBool(const std::string &setting);
  bool GetBool(int setting) const;
  void SetBool(int setting, bool set);

  /*! \brief Get the skin setting value as an integer value
   * \param setting - the setting id
   * \return the setting value as an integer, -1 if no conversion is possible
   */
  int GetInt(int setting) const;

  std::set<ADDON::CSkinSettingPtr> GetSettings() const;
  ADDON::CSkinSettingPtr GetSetting(const std::string& settingId);
  boost::shared_ptr<const ADDON::CSkinSetting> GetSetting(const std::string& settingId) const;

  void Reset(const std::string &setting);
  void Reset();

protected:
  CSkinSettings();
  CSkinSettings(const CSkinSettings&) = delete;
  CSkinSettings& operator=(CSkinSettings const&) = delete;
  virtual ~CSkinSettings();

private:
  CCriticalSection m_critical;
  std::set<ADDON::CSkinSettingPtr> m_settings;
};
