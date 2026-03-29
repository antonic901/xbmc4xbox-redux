/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogSeekBar : public CGUIDialog
{
public:
  CGUIDialogSeekBar(void);
  virtual ~CGUIDialogSeekBar(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();
private:
  int GetProgress() const;

  int m_lastProgress;
};
