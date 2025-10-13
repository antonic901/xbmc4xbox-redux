/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "programs/GUIWindowPrograms.h"

#include "dialogs/GUIDialogMediaSource.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"

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
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
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

bool CGUIWindowPrograms::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

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
