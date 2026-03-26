/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/IGUIInfoProvider.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CGUIInfoProvider : public IGUIInfoProvider
{
public:
  CGUIInfoProvider() {}
  virtual ~CGUIInfoProvider() {}

  virtual bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const CGUIInfo& info,
                        std::string* fallback)
  {
    return false;
  }
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
