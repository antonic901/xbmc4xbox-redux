/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/lib/Setting.h"

namespace ADDON
{
enum class AddonType;
}

class CSettingAddon : public CSettingString
{
public:
  CSettingAddon(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingAddon(const std::string &id, const CSettingAddon &setting);
  virtual ~CSettingAddon() {}

  virtual SettingPtr Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  ADDON::AddonType GetAddonType() const { return m_addonType; }
  void SetAddonType(ADDON::AddonType addonType) { m_addonType = addonType; }

private:
  void copyaddontype(const CSettingAddon &setting);

  ADDON::AddonType m_addonType{};
};
