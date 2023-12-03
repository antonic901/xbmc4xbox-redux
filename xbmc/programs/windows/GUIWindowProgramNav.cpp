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

#include "programs/windows/GUIWindowProgramNav.h"
#include "programs/dialogs/GUIDialogProgramScan.h"
#include "GUIWindowManager.h"
#include "FileItem.h"

using namespace std;

CGUIWindowProgramNav::CGUIWindowProgramNav(void)
    : CGUIWindowProgramBase(WINDOW_PROGRAM_NAV, "MyProgramNav.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowProgramNav::~CGUIWindowProgramNav(void)
{
}

bool CGUIWindowProgramNav::OnAction(const CAction &action)
{
  // TODO: implement this
  return CGUIWindowProgramBase::OnAction(action);
}

bool CGUIWindowProgramNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("");
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (!CGUIWindowProgramBase::OnMessage(message))
        return false;

      //  base class has opened the database, do our check
      m_database.Open();

      if (!m_database.HasContent() && m_vecItems->IsProgramDb())
      { // no library - make sure we default to the root.
        m_vecItems->SetPath("");
        SetHistoryForPath("");
        Update("");
      }

      m_database.Close();
      return true;
    }
    break;
  }
  return CGUIWindowProgramBase::OnMessage(message);
}

void CGUIWindowProgramNav::LoadProgramInfo(CFileItemList &items)
{
  // TODO: implement this
}

bool CGUIWindowProgramNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  items.ClearProperties();

  bool bResult = CGUIWindowProgramBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    // TODO: we got some results - implement this
  }
  return bResult;
}

bool CGUIWindowProgramNav::OnClick(int iItem)
{
  return CGUIWindowProgramBase::OnClick(iItem);
}

void CGUIWindowProgramNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (m_vecItems->GetPath().Equals("sources://programs/"))
  {
    // get the usual shares
    CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
    // add scan button somewhere here
    CGUIDialogProgramScan *pScanDlg = (CGUIDialogProgramScan *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
    if (pScanDlg && pScanDlg->IsScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
    if (!item->IsDVD() && item->GetPath() != "add" &&
        (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CProgramDatabase database;
      database.Open();
      ADDON::ScraperPtr info = database.GetScraperForPath(item->GetPath());

      if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
      {
        if (!item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath())
        {
          if (info && info->Content() != CONTENT_NONE)
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          else
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
        }
      }

      if (info && (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning())))
        buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
    }
  }
  else
  {
    if (!item->IsParentFolder())
    {
      ADDON::ScraperPtr info;
      PROGRAM::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      if (info && info->Content() == CONTENT_GAMES)
        buttons.Add(CONTEXT_BUTTON_INFO, 35003);
    }
  }
}

bool CGUIWindowProgramNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("programs", item, button))
  {
    Refresh();
    return true;
  }

  return CGUIWindowProgramBase::OnContextButton(itemNumber, button);
}

CStdString CGUIWindowProgramNav::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("GameGenres"))
    return "programdb://games/genres/";
  else if (dir.Equals("GameTitles"))
    return "programdb://games/titles/";
  else if (dir.Equals("GameYears"))
    return "programdb://games/years/";
  else if (dir.Equals("GameDevelopers"))
    return "programdb://games/developers/";
  else if (dir.Equals("GamePublishers"))
    return "programdb://games/publishers/";
  else if (dir.Equals("Games"))
    return "programdb://games/";
  else if (dir.Equals("RecentlyAddedGames"))
    return "programdb://recentlyaddedgames/";
  else if (dir.Equals("Files"))
    return "sources://programs/";
  return CGUIWindowProgramBase::GetStartFolder(dir);
}
