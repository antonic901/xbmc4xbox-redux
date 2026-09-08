/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

class CGUIWindowWeather : public CGUIWindow
{
public:
  CGUIWindowWeather(void);
  virtual ~CGUIWindowWeather(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();

protected:
  virtual void OnInitWindow();

  void UpdateButtons();
  void UpdateLocations();
  void SetProperties();
  void ClearProperties();
  void SetLocation(int loc);

  unsigned int m_maxLocation;
};
