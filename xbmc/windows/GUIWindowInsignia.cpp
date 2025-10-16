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

#include "GUIWindowInsignia.h"

#include "GUIUserMessages.h"
#include "guilib/Key.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIStaticItem.h"
#include "guilib/GUIListItem.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "listproviders/StaticProvider.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/Insignia.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "programs/ProgramDatabase.h"
#include "programs/launchers/ProgramLauncher.h"

CGUIWindowInsignia::CGUIWindowInsignia(void)
    : CGUIWindow(WINDOW_INSIGNIA, "Insignia.xml"),
      m_pGamesContainer(nullptr)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowInsignia::~CGUIWindowInsignia(void)
{}

bool CGUIWindowInsignia::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      g_insigniaManager.Reset();
      return true;
    }
    else if (message.GetParam1() == GUI_MSG_INSIGNIA_FETCHED)
    {
      SetProperties();
    }
    break;
  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowInsignia::OnAction(const CAction &action)
{
  CGUIControl *focusedControl = GetFocusedControl();
  if (focusedControl && action.GetButtonCode() == KEY_BUTTON_A && focusedControl->GetID() == CONTROL_GAMES_LIST)
  {
    CGUIListItemPtr game = m_pGamesContainer->GetListItem(0);

    CProgramDatabase database;
    database.Open();

    std::string gamePath = database.GetXBEPathByTitleId(game->GetProperty("code").asString());
    if (gamePath.empty())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Insignia", g_localizeStrings.Get(38903));
    else
      LAUNCHERS::CProgramLauncher::LaunchProgram(gamePath);

    return true;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowInsignia::SetProperties()
{
  if (GetFocusedControlID() != CONTROL_GAMES_LIST)
    SET_CONTROL_FOCUS(CONTROL_GAMES_LIST, 0);

  // This is taken from Weather window and I think it seems broken.
  // CGUIWindowInsignia::SetProperties() will be called before CInsignia::OnJobComplete
  // meaning that m_info is still not assigned and that InfoLoader is in busy state (m_busy is true).
  // This will result that all properties which we set below will be "Busy".
  // POTENTIAL FIX: send GUI_MSG_INSIGNIA_FETCHED message inside OnJobComplete
  SetProperty("LastTimeUpdated", g_insigniaManager.GetLastUpdateTime());
  SetProperty("GamesSupported", g_insigniaManager.GetInfo(INSIGNIA_LABEL_GAMES_SUPPORTED));
  SetProperty("RegisteredUsers", g_insigniaManager.GetInfo(INSIGNIA_LABEL_REGISTERED_USERS));
  SetProperty("OnlineUsers", g_insigniaManager.GetInfo(INSIGNIA_LABEL_ONLINE_USERS));
}

void CGUIWindowInsignia::ClearProperties()
{
  SetProperty("LastTimeUpdated", "");
  SetProperty("GamesSupported", "");
  SetProperty("RegisteredUsers", "");
  SetProperty("OnlineUsers", "");

  if (m_pGamesContainer)
    m_pGamesContainer->SetListProvider(nullptr);
}
