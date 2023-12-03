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

#include "programs/windows/GUIWindowProgramBase.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "programs/dialogs/GUIDialogProgramScan.h"
#include "dialogs/GUIDialogYesNo.h"
#include "settings/GUIDialogContentSettings.h"
#include "GUIWindowManager.h"

using namespace std;
using namespace PROGRAM;
using namespace ADDON;

CGUIWindowProgramBase::CGUIWindowProgramBase(int id, const CStdString &xmlFile)
    : CGUIMediaWindow(id, xmlFile)
{
  m_thumbLoader.SetObserver(this);
  m_stackingAvailable = true;
}

CGUIWindowProgramBase::~CGUIWindowProgramBase()
{
}

bool CGUIWindowProgramBase::OnAction(const CAction &action)
{
  // TODO: implement this
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowProgramBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();

      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      return CGUIMediaWindow::OnMessage(message);
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowProgramBase::OnInfo(CFileItem* pItem, const ADDON::ScraperPtr& scraper)
{
  // TODO: implement this
}

bool CGUIWindowProgramBase::Update(const CStdString &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  // might already be running from GetGroupedItems
  if (!m_thumbLoader.IsLoading())
    m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowProgramBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);

  // TODO: implement this

  return bResult;
}

bool CGUIWindowProgramBase::OnClick(int iItem)
{
  return CGUIMediaWindow::OnClick(iItem);
}

bool CGUIWindowProgramBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_CONTENT:
    {
      OnAssignContent(item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strPath : item->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_INFO:
    {
      ADDON::ScraperPtr info;
      PROGRAM::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      OnInfo(item.get(),info);
      return true;
    }
  case CONTEXT_BUTTON_STOP_SCANNING:
    {
      CGUIDialogProgramScan *pScanDlg = (CGUIDialogProgramScan *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
      if (pScanDlg && pScanDlg->IsScanning())
        pScanDlg->StopScanning();
      return true;
    }
  case CONTEXT_BUTTON_SCAN:
    {
      if( !item)
        return false;
      ADDON::ScraperPtr info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      CStdString strPath = item->GetPath();
      if (item->IsProgramDb() && (!item->m_bIsFolder || item->GetProgramInfoTag()->m_strPath.IsEmpty()))
        return false;

      if (item->IsProgramDb())
        strPath = item->GetProgramInfoTag()->m_strPath;

      if (info->Content() == CONTENT_NONE)
        return false;

      if (item->m_bIsFolder)
      {
        m_database.SetPathHash(strPath,""); // to force scan
        OnScan(strPath);
      }
      else
        OnInfo(item.get(),info);

      return true;
    }
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

int CGUIWindowProgramBase::GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, SScanSettings& settings)
{
  if (!item)
    return 0;

  if (m_vecItems->IsPlugin() || m_vecItems->IsRSS())
  {
    info.reset();
    return 0;
  }
  else if(m_vecItems->IsLiveTV())
  {
    info.reset();
    return 0;
  }

  bool foundDirectly = false;
  info = m_database.GetScraperForPath(item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strPath : item->GetPath(), settings, foundDirectly);
  return foundDirectly ? 1 : 0;
}

void CGUIWindowProgramBase::OnScan(const CStdString& strPath, bool scanAll)
{
  CGUIDialogProgramScan* pDialog = (CGUIDialogProgramScan*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
  if (pDialog)
    pDialog->StartScanning(strPath, scanAll);
}

CStdString CGUIWindowProgramBase::GetStartFolder(const CStdString &dir)
{
  // TODO: implemet this
  return CGUIMediaWindow::GetStartFolder(dir);
}

bool CGUIWindowProgramBase::OnUnAssignContent(const CStdString &path, int label1, int label2, int label3)
{
  bool bCanceled;
  CProgramDatabase db;
  db.Open();
  if (CGUIDialogYesNo::ShowAndGetInput(label1,label2,label3,20022,bCanceled))
  {
    db.RemoveContentForPath(path);
    db.Close();
    CUtil::DeleteProgramDatabaseDirectoryCache();
    return true;
  }
  else
  {
    if (!bCanceled)
    {
      ADDON::ScraperPtr info;
      SScanSettings settings;
      settings.exclude = true;
      db.SetScraperForPath(path,info,settings);
    }
  }
  db.Close();

  return false;
}

void CGUIWindowProgramBase::OnAssignContent(const CStdString &path)
{
  if (!g_guiSettings.GetBool("programlibrary.enabled")) 
    return;
 
  bool bScan=false;
  CProgramDatabase db;
  db.Open();

  SScanSettings settings;
  ADDON::ScraperPtr info = db.GetScraperForPath(path, settings);

  ADDON::ScraperPtr info2(info);
  
  if (CGUIDialogContentSettings::Show(info, settings))
  {
    if(settings.exclude || (!info && info2))
    {
      OnUnAssignContent(path,20375,20340,20341);
    }
    else if (info != info2)
    {
      if (OnUnAssignContent(path,20442,20443,20444))
        bScan = true;
    }

    db.SetScraperForPath(path,info,settings);

    if (bScan)
    {
      CGUIDialogProgramScan* pDialog = (CGUIDialogProgramScan*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
      if (pDialog)
        pDialog->StartScanning(path, true);
    }
  }
}
