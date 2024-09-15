#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
