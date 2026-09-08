/*
 *  Copyright (C) 2023-2026 Team Xodi
 *  This file is part of Xodi - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#define CONTROL_GAMES_LIST  5000

class CGUIBaseContainer;

class CGUIWindowInsignia : public CGUIWindow
{
public:
  CGUIWindowInsignia(void);
  virtual ~CGUIWindowInsignia(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void InitializeGamesContainer(CGUIBaseContainer* container) { m_pGamesContainer = container; };
  CGUIBaseContainer* GetGamesContainer() { return m_pGamesContainer; };

protected:
  void SetProperties();
  void ClearProperties();

private:
  CGUIBaseContainer *m_pGamesContainer;
};
