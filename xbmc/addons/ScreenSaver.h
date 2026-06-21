/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"

struct KODI_ADDON_SCREENSAVER_PROPS
{
  ADDON_HARDWARE_CONTEXT device;
  int x;
  int y;
  int width;
  int height;
  float pixelRatio;
};

namespace KODI
{
namespace ADDONS
{

class CScreenSaver : public ADDON::IAddonInstanceHandler
{
public:
  explicit CScreenSaver(const ADDON::AddonInfoPtr& addonInfo);
  virtual ~CScreenSaver();

  bool Start();
  void Stop();
  void Render();

  // Addon callback functions
  void GetProperties(struct KODI_ADDON_SCREENSAVER_PROPS* props);
};

} /* namespace ADDONS */
} /* namespace KODI */
