/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "addons/addoninfo/AddonType.h" // AddonType
#include <boost/shared_ptr.hpp>

namespace ADDON
{
class IAddon;
typedef boost::shared_ptr<IAddon> AddonPtr;

class CAddonInfo;
typedef boost::shared_ptr<CAddonInfo> AddonInfoPtr;

class CAddonBuilder
{
public:
  static AddonPtr Generate(const AddonInfoPtr& info, AddonType::Type type);
};

} // namespace ADDON
