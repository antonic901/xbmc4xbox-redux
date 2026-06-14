/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonManager.h"

#include "addons/addoninfo/AddonInfo.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <boost/make_shared.hpp>

using namespace ADDON;

BinaryAddonBasePtr CBinaryAddonManager::GetAddonBase(const AddonInfoPtr& addonInfo,
                                                     IAddonInstanceHandler* handler,
                                                     AddonDllPtr& addon)
{
  CSingleLock lock(m_critSection);

  BinaryAddonBasePtr addonBase;

  const std::map<std::string, ADDON::BinaryAddonBasePtr>::iterator &addonInstances = m_runningAddons.find(addonInfo->ID());
  if (addonInstances != m_runningAddons.end())
  {
    addonBase = addonInstances->second;
  }
  else
  {
    addonBase = boost::make_shared<CBinaryAddonBase>(addonInfo);

    m_runningAddons.insert(std::make_pair(addonInfo->ID(), addonBase));
  }

  if (addonBase)
  {
    addon = addonBase->GetAddon(handler);
  }
  if (!addon)
  {
    CLog::Log(LOGFATAL, "CBinaryAddonManager::{}: Tried to get add-on '{}' who not available!",
              __FUNCTION__, addonInfo->ID());
  }

  return addonBase;
}

void CBinaryAddonManager::ReleaseAddonBase(const BinaryAddonBasePtr& addonBase,
                                           IAddonInstanceHandler* handler)
{
  const std::map<std::string, ADDON::BinaryAddonBasePtr>::iterator &addon = m_runningAddons.find(addonBase->ID());
  if (addon == m_runningAddons.end())
    return;

  addonBase->ReleaseAddon(handler);

  if (addonBase->UsedInstanceCount() > 0)
    return;

  m_runningAddons.erase(addon);
}

BinaryAddonBasePtr CBinaryAddonManager::GetRunningAddonBase(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);

  const std::map<std::string, ADDON::BinaryAddonBasePtr>::const_iterator &addonInstances = m_runningAddons.find(addonId);
  if (addonInstances != m_runningAddons.end())
    return addonInstances->second;

  return BinaryAddonBasePtr();
}

AddonPtr CBinaryAddonManager::GetRunningAddon(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);

  const BinaryAddonBasePtr base = GetRunningAddonBase(addonId);
  if (base)
    return base->GetActiveAddon();

  return AddonPtr();
}
