/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "threads/CriticalSection.h"

class CGUIDialogVolumeBar : public CGUIDialog
{
public:
  CGUIDialogVolumeBar(void);
  virtual ~CGUIDialogVolumeBar(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
};
