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
#include "filesystem/ProgramDatabaseDirectory.h"
#include "programs/dialogs/GUIDialogProgramScan.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;
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

CStdString CGUIWindowProgramNav::GetQuickpathName(const CStdString& strPath) const
{
  CStdString path = strPath;
  if (path.Equals("programdb://games/genres/"))
    return "GameGenres";
  else if (path.Equals("programdb://games/titles/"))
    return "GameTitles";
  else if (path.Equals("programdb://games/years/"))
    return "GameYears";
  else if (path.Equals("programdb://games/developers/"))
    return "GameDevelopers";
  else if (path.Equals("programdb://games/publishers/"))
    return "GamePublishers";
  else if (path.Equals("programdb://games/descriptors/"))
    return "GameDescriptors";
  else if (path.Equals("programdb://games/generalfeatures/"))
    return "GameGeneralFeatures";
  else if (path.Equals("programdb://games/onlinefeatures/"))
    return "GameOnlineFeatures";
  else if (path.Equals("programdb://games/platforms/"))
    return "GamePlatforms";
  else if (path.Equals("programdb://games/"))
    return "Games";
  else if (path.Equals("programdb://applications/titles/"))
    return "ApplicationTitles";
  else if (path.Equals("programdb://applications/years/"))
    return "ApplicationYears";
  else if (path.Equals("programdb://recentlyaddedgames/"))
    return "RecentlyAddedGames";
  else if (path.Equals("programdb://recentlyplayedgames/"))
    return "RecentlyPlayedGames";
  else if (path.Equals("sources://programs/"))
    return "Files";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowProgramNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowProgramNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  items.ClearProperties();

  bool bResult = CGUIWindowProgramBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsProgramDb())
    {
      XFILE::CProgramDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(items.GetPath(),params);
      PROGRAMDATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

      items.SetThumbnailImage("");
      if (node == NODE_TYPE_TITLE_GAMES ||
          node == NODE_TYPE_RECENTLY_ADDED_GAMES ||
          node == NODE_TYPE_RECENTLY_PLAYED_GAMES)
        items.SetContent("games");
      else if (node == NODE_TYPE_TITLE_APPLICATIONS)
        items.SetContent("applications");
      else if (node == NODE_TYPE_DEVELOPER)
        items.SetContent("developers");
      else if (node == NODE_TYPE_PUBLISHER)
        items.SetContent("publishers");
      else if (node == NODE_TYPE_GENRE)
        items.SetContent("genres");
      else if (node == NODE_TYPE_DESCRIPTOR)
        items.SetContent("descriptors");
      else if (node == NODE_TYPE_GENERALFEATURE)
        items.SetContent("generalfeatures");
      else if (node == NODE_TYPE_ONLINEFEATURE)
        items.SetContent("onlinefeatures");
      else if (node == NODE_TYPE_PLATFORM)
        items.SetContent("platforms");
      else
        items.SetContent("");
    }
    else if (strDirectory.Equals("plugin://program/"))
    {
      items.SetContent("plugins");
      items.SetLabel(g_localizeStrings.Get(24001));
    }
    else if (strDirectory.IsEmpty())
      items.SetLabel("");
    else if (!items.IsVirtualDirectoryRoot())
    {
      CStdString label;
      if (items.GetLabel().IsEmpty() && m_rootDir.IsSource(items.GetPath(), g_settings.GetSourcesFromType("programs"), &label)) 
        items.SetLabel(label);
    }
    else
    { // load info from the database
      LoadProgramInfo(items);
    }
  }
  return bResult;
}

void CGUIWindowProgramNav::LoadProgramInfo(CFileItemList &items)
{
  // TODO: implement this
}

bool CGUIWindowProgramNav::OnClick(int iItem)
{
  return CGUIWindowProgramBase::OnClick(iItem);
}

bool CGUIWindowProgramNav::CanDelete(const CStdString& strPath)
{
  CQueryParams params;
  CProgramDatabaseDirectory::GetQueryParams(strPath,params);

  if (params.GetGameId()   != -1 ||
      params.GetApplicationId() != -1
              && !CProgramDatabaseDirectory::IsAllItem(strPath))
    return true;

  return false;
}

void CGUIWindowProgramNav::OnDeleteItem(CFileItemPtr pItem)
{
  if (m_vecItems->IsParentFolder())
    return;

  if (!m_vecItems->IsProgramDb())
  {
    if (!pItem->GetPath().Equals("newsmartplaylist://program") &&
        !pItem->GetPath().Equals("special://programplaylists/") &&
        !pItem->GetPath().Equals("sources://programs/") &&
        !pItem->GetPath().Left(9).Equals("newtag://"))
      CGUIWindowProgramBase::OnDeleteItem(pItem);
  }
  // else if (m_vecItems->GetContent() == "tags" && pItem->m_bIsFolder)
  // {
  //   CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  //   pDialog->SetHeading(432);
  //   CStdString strLabel;
  //   strLabel.Format(g_localizeStrings.Get(433), pItem->GetLabel());
  //   pDialog->SetLine(1, strLabel);
  //   pDialog->SetLine(2, "");
  //   pDialog->DoModal();
  //   if (pDialog->IsConfirmed())
  //   {
  //     CVideoDatabaseDirectory dir;
  //     CQueryParams params;
  //     dir.GetQueryParams(pItem->GetPath(), params);
  //     m_database.DeleteTag(params.GetTagId(), (VIDEODB_CONTENT_TYPE)params.GetContentType());
  //   }
  // }
  else 
  {
    if (!DeleteItem(pItem.get()))
      return;

    CStdString strDeletePath;
    if (pItem->m_bIsFolder)
      strDeletePath=pItem->GetProgramInfoTag()->m_strPath;
    else
      strDeletePath=pItem->GetProgramInfoTag()->m_strFileNameAndPath;

    if (URIUtils::HasSlashAtEnd(strDeletePath))
      pItem->m_bIsFolder=true;

    if (g_guiSettings.GetBool("filelists.allowfiledeletion") &&
        CUtil::SupportsWriteFileOperations(strDeletePath))
    {
      pItem->SetPath(strDeletePath);
      CGUIWindowProgramBase::OnDeleteItem(pItem);
    }
  }

  CUtil::DeleteProgramDatabaseDirectoryCache();
}

bool CGUIWindowProgramNav::DeleteItem(CFileItem* pItem, bool bUnavailable /* = false */)
{
  if (!pItem->HasProgramInfoTag() || !CanDelete(pItem->GetPath()))
    return false;

  PROGRAMDB_CONTENT_TYPE iType = PROGRAMDB_CONTENT_GAMES;
  if (pItem->HasProgramInfoTag() && (pItem->GetProgramInfoTag()->m_type.Equals("application") || pItem->GetProgramInfoTag()->m_type.Equals("emulator")))
    iType = PROGRAMDB_CONTENT_APPLICATIONS;

  // dont allow update while scanning
  CGUIDialogProgramScan* pDialogScan = (CGUIDialogProgramScan*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return false;
  }

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  if (iType == PROGRAMDB_CONTENT_GAMES)
    pDialog->SetHeading(35115);
  if (iType == PROGRAMDB_CONTENT_APPLICATIONS)
    pDialog->SetHeading(35116);

  if(bUnavailable)
  {
    pDialog->SetLine(0, g_localizeStrings.Get(662));
    pDialog->SetLine(1, g_localizeStrings.Get(663));
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }
  else
  {
    CStdString strLine;
    strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
    pDialog->SetLine(0, strLine);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }

  if (!pDialog->IsConfirmed())
    return false;

  CStdString path;
  CProgramDatabase database;
  database.Open();

  database.GetFilePathById(pItem->GetProgramInfoTag()->m_iDbId, path, iType);
  if (path.IsEmpty())
    return false;
  if (iType == PROGRAMDB_CONTENT_GAMES)
    database.DeleteGame(path);
  if (iType == PROGRAMDB_CONTENT_APPLICATIONS)
    database.DeleteApplication(path);

  CStdString strDirectory;
  URIUtils::GetDirectory(path,strDirectory);
  database.SetPathHash(strDirectory,"");

  return true;
}

void CGUIWindowProgramNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CProgramDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems->GetPath());

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
      else if (info && info->Content() == CONTENT_APPLICATIONS)
        buttons.Add(CONTEXT_BUTTON_INFO, 35109);

      // can we update the database?
      if (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if(item->IsProgramDb() && item->HasProgramInfoTag())
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);
      }
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
    //TODO should we search DB for entries from plugins?
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin()
        && !item->IsLiveTV() &&!item->IsRSS())
    {
      OnUnAssignContent(item->GetPath(),20375,20340,20341);
    }
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
  else if (dir.Equals("GameDescriptors"))
    return "programdb://games/descriptors/";
  else if (dir.Equals("GameGeneralFeatures"))
    return "programdb://games/generalfeatures/";
  else if (dir.Equals("GameOnlineFeatures"))
    return "programdb://games/onlinefeatures/";
  else if (dir.Equals("GamePlatforms"))
    return "programdb://games/platforms/";
  else if (dir.Equals("Games"))
    return "programdb://games/";
  else if (dir.Equals("ApplicationTitles"))
    return "programdb://applications/titles/";
  else if (dir.Equals("ApplicationYears"))
    return "programdb://applications/years/";
  else if (dir.Equals("Applications"))
    return "programdb://applications/";
  else if (dir.Equals("RecentlyAddedGames"))
    return "programdb://recentlyaddedgames/";
  else if (dir.Equals("RecentlyPlayedGames"))
    return "programdb://recentlyplayedgames/";
  else if (dir.Equals("Files"))
    return "sources://programs/";
  return CGUIWindowProgramBase::GetStartFolder(dir);
}
