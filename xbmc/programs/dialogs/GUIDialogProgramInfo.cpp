/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "GUIDialogProgramInfo.h"

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIImage.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/launchers/ProgramLauncher.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace KODI::MESSAGING;

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_PLAY             8
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_SETTINGS        12

#define CONTROL_LIST                50

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml"),
    m_programItem(new CFileItem),
    m_screenshotList(new CFileItemList)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
  delete m_screenshotList;
}

bool CGUIDialogProgramInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearScreenshotList();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_PLAY)
      {
        return LAUNCHERS::CProgramLauncher::LaunchProgram(m_programItem->GetPath());
      }
      if (iControl == CONTROL_BTN_PLAY_TRAILER)
      {
        PlayTrailer();
      }
      if (iControl == CONTROL_BTN_SETTINGS)
      {
        CGUIDialogProgramSettings::ShowForTitle(m_programItem);
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogProgramInfo::OnInitWindow()
{
  Update();

  CGUIDialog::OnInitWindow();
}

void CGUIDialogProgramInfo::SetProgram(const CFileItem *item)
{
  *m_programItem = *item;

  // setup screenshot list
  ClearScreenshotList();

  CFileItemList items;
  std::string strScreenshots = URIUtils::AddFileToFolder(URIUtils::GetParentPath(item->GetPath()), "_resources", "screenshots");
  CDirectory::GetDirectory(strScreenshots, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_DEFAULTS);
  for (int i = 0; i < items.Size(); i++)
  {
    std::string strLabel = URIUtils::GetFileName(items[i]->GetPath());
    CFileItemPtr item(new CFileItem(strLabel));
    item->SetPath(items[i]->GetPath());
    item->SetArt("thumb", items[i]->GetPath());
    m_screenshotList->Add(item);
  }

  m_screenshotList->SetContent("game");
}

void CGUIDialogProgramInfo::Update()
{
  // setup plot text area
  std::string strTmp = m_programItem->GetProperty("overview").asString();
  StringUtils::Trim(strTmp);
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_screenshotList);
  OnMessage(msg);

  SET_CONTROL_VISIBLE(CONTROL_BTN_SETTINGS);

  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_programItem->GetArt("thumb"));
  }
}

void CGUIDialogProgramInfo::ClearScreenshotList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_screenshotList->Clear();
}

void CGUIDialogProgramInfo::PlayTrailer()
{
  std::string strTrailer = m_programItem->GetProperty("trailer").asString();
  if (!strTrailer.empty())
  {
    CFileItem item(strTrailer, false);
    item.SetLabel(StringUtils::Format("%s (%s)", m_programItem->GetLabel().c_str(), g_localizeStrings.Get(20410).c_str()));

    // Close the dialog.
    Close(true);

    CApplicationMessenger::Get().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(item)));
  }
}

void CGUIDialogProgramInfo::SetLabel(int iControl, const std::string &strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, 416);  // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}
