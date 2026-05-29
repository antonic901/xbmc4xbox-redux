/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogVideoOSD : public CGUIDialog
{
public:

  CGUIDialogVideoOSD(void);
  virtual ~CGUIDialogVideoOSD(void);

  virtual void FrameMove();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event);
};
