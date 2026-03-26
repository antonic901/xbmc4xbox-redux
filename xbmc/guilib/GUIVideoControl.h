/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIVideoControl.h
\brief
*/

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIVideoControl :
      public CGUIControl
{
public:
  CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height);
  virtual ~CGUIVideoControl(void);
  virtual CGUIVideoControl* Clone() const { return new CGUIVideoControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool CanFocusFromPoint(const CPoint &point) const;
};

