/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAudioSettings.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

CGUIDialogAudioSettings::CGUIDialogAudioSettings()
{ }

CGUIDialogAudioSettings::~CGUIDialogAudioSettings() {}

std::string CGUIDialogAudioSettings::FormatDelay(float value, float interval)
{
  if (fabs(value) < 0.5f * interval)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (value < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(value));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), value);
}

std::string CGUIDialogAudioSettings::FormatDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), value);
}
