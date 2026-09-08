/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

class CGUIWindowStartup :
      public CGUIWindow
{
public:
  CGUIWindowStartup(void);
  virtual ~CGUIWindowStartup(void);

  // specialization of CGUIWindow
  virtual void OnDeinitWindow(int nextWindowID);
};
