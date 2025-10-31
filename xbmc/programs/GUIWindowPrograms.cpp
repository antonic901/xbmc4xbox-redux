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
#include "dialogs/GUIDialogTrainerSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "programs/ProgramLibraryQueue.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/launchers/ProgramLauncher.h"
#include "programs/launchers/XBELauncher.h"
#include "messaging/ApplicationMessenger.h"
#include "Util.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

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

void CGUIWindowPrograms::OnItemInfo(CFileItemPtr item)
{
  std::string strRootPath = URIUtils::GetParentPath(item->GetPath());
  std::string strTempPath = URIUtils::AddFileToFolder(strRootPath, "_resources", "default.xml");
  CXBMCTinyXML doc;
  if (doc.LoadFile(strTempPath) && doc.RootElement())
  {
    const TiXmlElement* synopsis = doc.RootElement();
    std::string value;
    float fValue;
    int iValue;

    if (XMLUtils::GetString(synopsis, "developer", value))
      item->SetProperty("developer", value);

    if (XMLUtils::GetString(synopsis, "publisher", value))
      item->SetProperty("publisher", value);

    if (XMLUtils::GetString(synopsis, "features_general", value))
      item->SetProperty("features_general", value);

    if (XMLUtils::GetString(synopsis, "features_online", value))
      item->SetProperty("features_online", value);

    if (XMLUtils::GetString(synopsis, "esrb", value))
      item->SetProperty("esrb", value);

    if (XMLUtils::GetString(synopsis, "esrb_descriptors", value))
      item->SetProperty("esrb_descriptors", value);

    if (XMLUtils::GetString(synopsis, "genre", value))
      item->SetProperty("genre", value);

    if (XMLUtils::GetString(synopsis, "release_date", value))
      item->SetProperty("release_date", value);

    if (XMLUtils::GetInt(synopsis, "year", iValue))
      item->SetProperty("year", iValue);

    if (XMLUtils::GetFloat(synopsis, "rating", fValue))
      item->SetProperty("rating", fValue);

    if (XMLUtils::GetString(synopsis, "platform", value))
      item->SetProperty("platform", value);

    if (XMLUtils::GetString(synopsis, "exclusive", value))
      item->SetProperty("exclusive", value);
  }

  CGUIDialogProgramInfo *dialog = static_cast<CGUIDialogProgramInfo*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_INFO));
  dialog->SetProgram(item.get());
  dialog->Open();
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
  else if (item->IsHD() && !item->IsXBE())
  {
    buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
    CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
  }
  else if (item->IsXBE())
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 19033);
    buttons.Add(CONTEXT_BUTTON_LAUNCH_IN, 519);
    buttons.Add(CONTEXT_BUTTON_PLAY_TRAILER, StringUtils::Format("%s %s", g_localizeStrings.Get(208).c_str(), g_localizeStrings.Get(20410).c_str()));
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);
    buttons.Add(CONTEXT_BUTTON_TRAINER_OPTIONS, 38712);
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
  case CONTEXT_BUTTON_SCAN:
    {
      CProgramLibraryQueue::GetInstance().ScanLibrary(item->GetPath());
      return true;
    }
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
        }
        CUtil::DeleteProgramDatabaseDirectoryCache();
        int select = itemNumber >= m_vecItems->Size() - 1 ? itemNumber - 1 : itemNumber;
        Refresh(true);
        m_viewControl.SetSelectedItem(select);
      }
      return true;
    }
  case CONTEXT_BUTTON_INFO:
    {
      OnItemInfo(item);
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
