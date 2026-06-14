/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <set>

namespace ADDON
{

  class IAddonInstanceHandler;

  class CAddonInfo;
  typedef boost::shared_ptr<CAddonInfo> AddonInfoPtr;

  class CAddonDll;
  typedef boost::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase : public boost::enable_shared_from_this<CBinaryAddonBase>
  {
  public:
    explicit CBinaryAddonBase(const AddonInfoPtr& addonInfo) : m_addonInfo(addonInfo) { }

    const std::string& ID() const;

    AddonDllPtr GetAddon(IAddonInstanceHandler* handler);
    void ReleaseAddon(IAddonInstanceHandler* handler);
    size_t UsedInstanceCount() const;

    AddonDllPtr GetActiveAddon();

    void OnPreInstall();
    void OnPostInstall(bool update, bool modal);
    void OnPreUnInstall();
    void OnPostUnInstall();

  private:
    AddonInfoPtr m_addonInfo;

    mutable CCriticalSection m_critSection;
    AddonDllPtr m_activeAddon;
    std::set<IAddonInstanceHandler*> m_activeAddonHandlers;
  };

} /* namespace ADDON */
