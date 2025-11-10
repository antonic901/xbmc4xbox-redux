/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "programs/GUIWindowPrograms.h"

#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "programs/ProgramLibraryQueue.h"
#include "programs/launchers/ProgramLauncher.h"
#include "programs/launchers/XBELauncher.h"
#include "settings/AdvancedSettings.h"
#include "Util.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"

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

bool CGUIWindowPrograms::OnClick(int iItem, const std::string &player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->GetPath() == "insignia://")
  {
    g_windowManager.ActivateWindow(WINDOW_INSIGNIA);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem, player);
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
  else if (URIUtils::IsProtocol(item->GetPath(), "gamesaves"))
  {
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);
  }
  else if (item->m_bIsFolder && item->IsHD())
  {
    CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
  }
  else if (URIUtils::HasExtension(item->GetPath(), g_advancedSettings.m_programExtensions))
  {
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);
    if (item->IsXBE())
      buttons.Add(CONTEXT_BUTTON_GAMESAVES, 38779);
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
  case CONTEXT_BUTTON_DELETE:
    {
      if (CGUIDialogYesNo::ShowAndGetInput(646, StringUtils::Format(g_localizeStrings.Get(433).c_str(), item->GetLabel().c_str())))
      {
        if (URIUtils::IsProtocol(item->GetPath(), "gamesaves"))
        {
          std::vector<std::string> Path = StringUtils::Split(item->GetPath(), "://");
          if (!CFileUtils::DeleteItem("E:\\UDATA\\" + Path.back() + "\\", true))
            return false;
        }
        else
        {
          m_database.DeleteProgram(item->GetPath());
          CFileUtils::DeleteItem(URIUtils::GetParentPath(item->GetPath()));
        }
        CUtil::DeleteProgramDatabaseDirectoryCache();
        int select = itemNumber >= m_vecItems->Size() - 1 ? itemNumber - 1 : itemNumber;
        Refresh(true);
        m_viewControl.SetSelectedItem(select);
      }
      return true;
    }
  case CONTEXT_BUTTON_GAMESAVES:
    {
      std::string strTitleId = LAUNCHERS::CXBELauncher::GetTitleID(item->GetPath(), true).asString();
      std::string strSaveGamePath = URIUtils::AddFileToFolder("E:\\UDATA\\", strTitleId);
      if (XFILE::CDirectory::Exists(strSaveGamePath))
        Update("gamesaves://" + strTitleId);
      else
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(38779), g_localizeStrings.Get(38772));
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

  return LAUNCHERS::CProgramLauncher::LaunchProgram(m_vecItems->Get(iItem)->GetPath());
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

    CFileItemPtr pItem2(new CFileItem("gamesaves://", true));
    pItem2->SetIconImage("DefaultGames.png");
    pItem2->SetLabel(g_localizeStrings.Get(38779));
    pItem2->SetLabelPreformated(true);
    pItem2->SetProperty("overview", g_localizeStrings.Get(38779));
    pItem2->SetSpecialSort(SortSpecialOnTop);
    items.Add(pItem2);

    CFileItemPtr pItem3(new CFileItem("programdb://games/recentlyplayed/", true));
    pItem3->SetIconImage("DefaultRecentlyAddedMovies.png");
    pItem3->SetLabel(g_localizeStrings.Get(38973));
    pItem3->SetLabelPreformated(true);
    pItem3->SetSpecialSort(SortSpecialOnTop);
    items.Add(pItem3);

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
