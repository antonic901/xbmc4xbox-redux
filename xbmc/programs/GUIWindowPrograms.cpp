/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "programs/GUIWindowPrograms.h"

#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogTrainerSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"
#include "programs/ProgramLibraryQueue.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/launchers/ProgramLauncher.h"
#include "messaging/ApplicationMessenger.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/Trainer.h"

using namespace KODI::MESSAGING;

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
  m_thumbLoader.SetObserver(this);
  m_rootDir.AllowNonLocalSources(false); // no nonlocal shares for this window please
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();
      return CGUIMediaWindow::OnMessage(message);
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPrograms::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);

  if (!item)
  {
    // nothing to do here
  }
  else if (item->IsHD() && !item->IsXBE())
  {
    buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
    CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
  }
  else if (item->IsXBE())
  {
    buttons.Add(CONTEXT_BUTTON_LAUNCH_IN, 519);
    buttons.Add(CONTEXT_BUTTON_PLAY_TRAILER, StringUtils::Format("%s %s", g_localizeStrings.Get(208).c_str(), g_localizeStrings.Get(20410).c_str()));
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);
    buttons.Add(CONTEXT_BUTTON_TRAINER_OPTIONS, 38712);
  }
}

bool CGUIWindowPrograms::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(itemNumber);

  if (CGUIDialogContextMenu::OnContextButton("programs", item, button))
  {
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && CGUIDialogYesNo::ShowAndGetInput(20375, 20340))
    {
      CProgramLibraryQueue::GetInstance().CleanLibrary(item->GetPath());
    }
    Refresh();
    return true;
  }

  switch (button)
  {
  case CONTEXT_BUTTON_SCAN:
    {
      CProgramLibraryQueue::GetInstance().ScanLibrary(item->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_DELETE:
    {
      if (CGUIDialogYesNo::ShowAndGetInput(646, StringUtils::Format(g_localizeStrings.Get(433).c_str(), item->GetLabel().c_str())))
      {
        m_database.DeleteProgram(item->GetPath());
        CUtil::DeleteProgramDatabaseDirectoryCache();
        int select = itemNumber >= m_vecItems->Size() - 1 ? itemNumber - 1 : itemNumber;
        Refresh(true);
        m_viewControl.SetSelectedItem(select);
      }
      return true;
    }
  case CONTEXT_BUTTON_LAUNCH_IN:
    {
      CGUIDialogProgramSettings::ShowForTitle(item);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_TRAILER:
    {
      CFileItem playItem(*item);
      playItem.SetPath(item->GetProperty("trailer").asString());
      CApplicationMessenger::Get().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(playItem)));
      return true;
    }
  case CONTEXT_BUTTON_TRAINER_OPTIONS:
    {
      CGUIDialogTrainerSettings::ShowForTitle(item);
      return true;
    }
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPrograms::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("programs");
}

bool CGUIWindowPrograms::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowPrograms::OnPlayMedia(int iItem, const std::string& player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (StringUtils::StartsWithNoCase(pItem->GetPath(), "insignia://"))
  {
    g_windowManager.ActivateWindow(WINDOW_INSIGNIA);
    return true;
  }

  return LAUNCHERS::CProgramLauncher::LaunchProgram(pItem->GetPath());
}

bool CGUIWindowPrograms::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  std::string strDirectory1(strDirectory);
  if (!strDirectory.empty())
  {
    int idPath = m_database.GetPathId(strDirectory);
    if (idPath >= 0)
      strDirectory1 = StringUtils::Format("programdb://paths/%i/", idPath);
  }

  if (!CGUIMediaWindow::GetDirectory(strDirectory1, items))
    return false;

  // don't allow the view state to change these
  if (StringUtils::StartsWithNoCase(strDirectory, "addons://"))
  {
    for (int i=0;i<items.Size();++i)
    {
      items[i]->SetLabel2(items[i]->GetProperty("Addon.Version").asString());
      items[i]->SetLabelPreformated(true);
    }
  }

  if (items.IsVirtualDirectoryRoot())
  {
    CFileItemPtr pItem(new CFileItem());
    pItem->SetPath("insignia://");
    pItem->SetIconImage("insignia/logo.png");
    pItem->SetLabel(g_localizeStrings.Get(38901));
    pItem->SetLabelPreformated(true);
    pItem->SetProperty("overview", g_localizeStrings.Get(38902));
    pItem->SetSpecialSort(SortSpecialOnTop);
    items.Add(pItem);

    items.SetLabel("");
  }

  return true;
}

std::string CGUIWindowPrograms::GetStartFolder(const std::string &dir)
{
  if (dir == "Plugins" || dir == "Addons")
    return "addons://sources/executable/";

  return CGUIMediaWindow::GetStartFolder(dir);
}
