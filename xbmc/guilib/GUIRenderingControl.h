/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"

class IRenderingCallback;

class CGUIRenderingControl : public CGUIControl
{
public:
  CGUIRenderingControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIRenderingControl(const CGUIRenderingControl &from);
  virtual CGUIRenderingControl *Clone() const { return new CGUIRenderingControl(*this); }; //! @todo check for naughties

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void FreeResources(bool immediately = false);
  virtual bool CanFocus() const { return false; }
  virtual bool CanFocusFromPoint(const CPoint &point) const;
  bool InitCallback(IRenderingCallback *callback);

protected:
  CCriticalSection m_rendering;
  IRenderingCallback *m_callback;
};
