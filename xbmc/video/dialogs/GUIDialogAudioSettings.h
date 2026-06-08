/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CGUIDialogAudioSettings
{
public:
  CGUIDialogAudioSettings();
  virtual ~CGUIDialogAudioSettings();

  static std::string FormatDelay(float value, float interval);
  static std::string FormatDecibel(float value);
};
