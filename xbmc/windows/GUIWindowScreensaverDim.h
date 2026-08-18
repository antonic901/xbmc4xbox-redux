/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIWindowScreensaverDim : public CGUIDialog
{
public:
  CGUIWindowScreensaverDim();

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();

protected:
  virtual void UpdateVisibility();

private:
  float m_dimLevel;
  float m_newDimLevel;
  bool m_visible;
};
