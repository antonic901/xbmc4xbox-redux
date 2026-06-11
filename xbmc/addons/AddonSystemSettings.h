/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonType.h" // AddonType
#include "settings/lib/ISettingCallback.h"

#include <map>
#include <memory>
#include <string>

namespace ADDON
{

const int AUTO_UPDATES_ON = 0;
const int AUTO_UPDATES_NOTIFY = 1;
const int AUTO_UPDATES_NEVER = 2;

namespace AddonRepoUpdateMode
{
enum Type
{
  OFFICIAL_ONLY = 0,
  ANY_REPOSITORY = 1
};
}

class CAddonInfo;
typedef boost::shared_ptr<CAddonInfo> AddonInfoPtr;

class IAddon;
typedef boost::shared_ptr<IAddon> AddonPtr;

class CAddonSystemSettings : public ISettingCallback
{
public:
  static CAddonSystemSettings& GetInstance();
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);

  bool GetActive(AddonType::Type type, AddonPtr& addon);
  bool SetActive(AddonType::Type type, const std::string& addonID);
  bool IsActive(const IAddon& addon);

  /*!
   * Gets Kodi addon auto update mode
   *
   * @return the autoupdate mode value
  */
  int GetAddonAutoUpdateMode() const;


  /*!
   * Gets Kodi preferred addon repository update mode
   *
   * @return the preferred mode value
   */
  AddonRepoUpdateMode::Type GetAddonRepoUpdateMode() const;

  /*!
   * Attempt to unset addon as active. Returns true if addon is no longer active,
   * false if it could not be unset (e.g. if the addon is the default)
   */
  bool UnsetActive(const AddonInfoPtr& addon);

private:
  CAddonSystemSettings();
  CAddonSystemSettings(const CAddonSystemSettings&);
  CAddonSystemSettings& operator=(const CAddonSystemSettings&);
  virtual ~CAddonSystemSettings() {}

  const std::map<AddonType::Type, std::string> m_activeSettings;
};
};
