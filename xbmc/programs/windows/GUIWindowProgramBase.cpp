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
#include "programs/ProgramInfoDownloader.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/dialogs/GUIDialogProgramScan.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogTrainerSettings.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "filesystem/ProgramDatabaseDirectory.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/RarManager.h"
#include "filesystem/HDDirectory.h"
#include "settings/GUIDialogContentSettings.h"
#include "GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"
#include "utils/FileUtils.h"
#include "utils/EmulatorUtils.h"
#include "xbox/xbeheader.h"

using namespace std;
using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;
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
      m_iRegionSet = 0;
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
  if (!pItem)
    return;

  if (!scraper)
    return;

  if (pItem->IsParentFolder() || pItem->m_bIsShareOrDrive ||
      pItem->GetPath().Equals("add") || pItem->IsPlayList())
    return;

  // ShowIGDB can kill the item as this window can be closed while we do it,
  // so take a copy of the item now
  CFileItem item(*pItem);
  if (item.IsProgramDb() && item.HasProgramInfoTag())
  {
    if (item.GetProgramInfoTag()->m_strFileNameAndPath.IsEmpty())
      item.SetPath(item.GetProgramInfoTag()->m_strPath);
    else
      item.SetPath(item.GetProgramInfoTag()->m_strFileNameAndPath);
  }
  else
  {
    if (item.m_bIsFolder)
    {
      CFileItemList items;
      CDirectory::GetDirectory(item.GetPath(), items, g_settings.m_programExtensions);
      items.Stack();

      // check for media files
      bool bFoundFile(false);
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr item2 = items[i];

        if (item2->IsProgram() && !item2->IsPlayList() &&
            !CUtil::ExcludeFileOrFolder(item2->GetPath(), g_advancedSettings.m_gamesExcludeFromScanRegExps))
        {
          item.SetPath(item2->GetPath());
          item.m_bIsFolder = false;
          bFoundFile = true;
          break;
        }
      }

      // no program file in this folder
      if (!bFoundFile)
      {
        CGUIDialogOK::ShowAndGetInput(35003,35002,20022,20022);
        return;
      }
    }
  }

  // we need to also request any thumbs be applied to the folder item
  if (pItem->m_bIsFolder)
    item.SetProperty("set_folder_thumb", pItem->GetPath());

  bool modified = ShowIGDB(&item, scraper);
  if (modified)
  {
    int itemNumber = m_viewControl.GetSelectedItem();
    Refresh();
    m_viewControl.SetSelectedItem(itemNumber);
  }
}

// See ShowIMDB in GUIWindowVideoBase.cpp for more info
bool CGUIWindowProgramBase::ShowIGDB(CFileItem *item, const ScraperPtr &info2)
{
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogProgramInfo* pDlgInfo = (CGUIDialogProgramInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_INFO);

  ScraperPtr info(info2); // use this as nfo might change it..

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CProgramInfoTag gameDetails;
  if (info)
  {
    m_database.Open();

    if (info->Content() == CONTENT_GAMES)
    {
      if (m_database.HasGameInfo(item->GetPath()))
      {
        bHasInfo = true;
        m_database.GetGameInfo(item->GetPath(), gameDetails);
      }
    }
    else if (info->Content() == CONTENT_APPLICATIONS)
    {
      if (m_database.HasApplicationInfo(item->GetPath()))
      {
        bHasInfo = true;
        m_database.GetApplicationInfo(item->GetPath(), gameDetails);
      }
    }
    m_database.Close();
  }
  else if (item->HasProgramInfoTag())
  {
    bHasInfo = true;
    gameDetails = *item->GetProgramInfoTag();
  }

  bool needsRefresh = false;
  if (bHasInfo)
  {
    if (info->Content() == CONTENT_NONE) // disable refresh button
      gameDetails.m_strXBENumber = "xx"+gameDetails.m_strXBENumber;
    *item->GetProgramInfoTag() = gameDetails;
    pDlgInfo->SetGame(item);
    pDlgInfo->DoModal();
    needsRefresh = pDlgInfo->NeedRefresh();
    if (!needsRefresh)
      return pDlgInfo->HasUpdatedThumb();
  }

  // quietly return if Internet lookups are disabled
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  if(!info)
    return false;

  CGUIDialogProgramScan* pDialog = (CGUIDialogProgramScan*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
  if (pDialog && pDialog->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(35003,14057,-1,-1);
    return false;
  }

  m_database.Open();
  // 2. Look for a nfo File to get the search URL
  SScanSettings settings;
  info = m_database.GetScraperForPath(item->GetPath(),settings);

  if (!info)
    return false;

  // Get the correct game title
  CStdString gameName = item->GetGameName(settings.parent_name);

  CScraperUrl scrUrl;
  CProgramInfoScanner scanner;
  bool hasDetails = false;
  bool listNeedsUpdating = false;
  bool ignoreNfo = false;
  // 3. Run a loop so that if we Refresh we re-run this block
  do
  {
    if (!ignoreNfo)
    {
      CNfoFile::NFOResult nfoResult = scanner.CheckForNFOFile(item,settings.parent_name_root,info,scrUrl);
      if (nfoResult == CNfoFile::ERROR_NFO)
        ignoreNfo = true;
      else
      if (nfoResult != CNfoFile::NO_NFO)
        hasDetails = true;

      if (needsRefresh)
      {
        bHasInfo = true;
        if (nfoResult == CNfoFile::URL_NFO || nfoResult == CNfoFile::COMBINED_NFO || nfoResult == CNfoFile::FULL_NFO)
        {
          if (CGUIDialogYesNo::ShowAndGetInput(35003,20446,20447,20022))
          {
            hasDetails = false;
            ignoreNfo = true;
            scrUrl.Clear();
            info = info2;
          }
        }
      }
    }

    // 4. if we don't have an url, or need to refresh the search
    //    then do the web search
    GAMELIST gamelist;
    
    if (!hasDetails && (scrUrl.m_url.size() == 0 || needsRefresh))
    {
      // 4a. show dialog that we're busy querying www.igdb.com
      CStdString strHeading;
      strHeading.Format(g_localizeStrings.Get(197),info->Name().c_str());
      pDlgProgress->SetHeading(strHeading);
      pDlgProgress->SetLine(0, gameName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // 4b. do the websearch
      CProgramInfoDownloader igdb(info);
      int returncode = igdb.FindGame(gameName, gamelist, pDlgProgress);
      if (returncode > 0)
      {
        pDlgProgress->Close();
        if (gamelist.size() > 0)
        {
          int iString = 35004;
          pDlgSelect->SetHeading(iString);
          pDlgSelect->Reset();
          for (unsigned int i = 0; i < gamelist.size(); ++i)
            pDlgSelect->Add(gamelist[i].strTitle);
          pDlgSelect->EnableButton(true, 413); // manual
          pDlgSelect->DoModal();

          // and wait till user selects one
          int iSelectedGame = pDlgSelect->GetSelectedLabel();
          if (iSelectedGame >= 0)
          {
            scrUrl = gamelist[iSelectedGame];
            CLog::Log(LOGDEBUG, "%s: user selected game '%s' with URL '%s'",
              __FUNCTION__, scrUrl.strTitle.c_str(), scrUrl.m_url[0].m_url.c_str());
          }
          else if (!pDlgSelect->IsButtonPressed())
          {
            m_database.Close();
            return listNeedsUpdating; // user backed out
          }
        }
      }
      else if (returncode == -1 || !CProgramInfoScanner::DownloadFailed(pDlgProgress))
      {
        pDlgProgress->Close();
        return false;
      }
    }
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IGDb lookup failed or was cancelled.
    if (!hasDetails && scrUrl.m_url.size() == 0)
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
      {
        m_database.Close();
        return listNeedsUpdating;
      }

      // Prompt the user to input the gameName
      int iString = 35005;
      if (!CGUIDialogKeyboard::ShowAndGetInput(gameName, g_localizeStrings.Get(iString), false))
      {
        m_database.Close();
        return listNeedsUpdating; // user backed out
      }

      needsRefresh = true;
    }
    else
    {
      // 5. Download the game information
      // show dialog that we're downloading the game info
      CFileItemList list;
      CStdString strPath=item->GetPath();
      if (item->IsProgramDb())
      {
        CFileItemPtr newItem(new CFileItem(*item->GetProgramInfoTag()));
        list.Add(newItem);
        strPath = item->GetProgramInfoTag()->m_strPath;
      }
      else
      {
        CFileItemPtr newItem(new CFileItem(*item));
        list.Add(newItem);
      }

      if (item->m_bIsFolder)
        list.SetPath(URIUtils::GetParentPath(strPath));
      else
      {
        CStdString path;
        URIUtils::GetDirectory(strPath, path);
        list.SetPath(path);
      }

      int iString=35006;
      pDlgProgress->SetHeading(iString);
      pDlgProgress->SetLine(0, gameName);
      pDlgProgress->SetLine(1, scrUrl.strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();
      if (bHasInfo)
      {
        if (info->Content() == CONTENT_GAMES)
          m_database.DeleteGame(item->GetPath());
        else if (info->Content() == CONTENT_APPLICATIONS)
          m_database.DeleteApplication(item->GetPath());
      }
      if (scanner.RetrieveProgramInfo(list,settings.parent_name_root,info->Content(),!ignoreNfo,&scrUrl,pDlgProgress))
      {
        if (info->Content() == CONTENT_GAMES)
          m_database.GetGameInfo(item->GetPath(),gameDetails);
        else if (info->Content() == CONTENT_APPLICATIONS)
          m_database.GetApplicationInfo(item->GetPath(),gameDetails);

        // got all game details :-)
        OutputDebugString("got details\n");
        pDlgProgress->Close();

        // now show the igdb info
        OutputDebugString("show info\n");

        // remove directory caches and reload images
        CUtil::DeleteProgramDatabaseDirectoryCache();
        CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
        OnMessage(reload);

        *item->GetProgramInfoTag() = gameDetails;
        pDlgInfo->SetGame(item);
        pDlgInfo->DoModal();
        item->SetThumbnailImage(pDlgInfo->GetThumbnail());
        needsRefresh = pDlgInfo->NeedRefresh();
        listNeedsUpdating = true;
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled())
        {
          m_database.Close();
          return listNeedsUpdating; // user cancelled
        }
        CGUIDialogOK::ShowAndGetInput(195, gameName, 0, 0);
        m_database.Close();
        return listNeedsUpdating;
      }
    }
  // 6. Check for a refresh
  } while (needsRefresh);
  m_database.Close();
  return listNeedsUpdating;
}

//Add change a title's name
void CGUIWindowProgramBase::UpdateProgramTitle(const CFileItem* pItem)
{
  // dont allow update while scanning
  CGUIDialogProgramScan* pDialogScan = (CGUIDialogProgramScan*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CProgramInfoTag detail;
  CProgramDatabase database;
  database.Open();
  CProgramDatabaseDirectory dir;
  CQueryParams params;
  dir.GetQueryParams(pItem->GetPath(),params);
  int iDbId = pItem->GetProgramInfoTag()->m_iDbId;

  PROGRAMDB_CONTENT_TYPE iType=PROGRAMDB_CONTENT_GAMES;
  if (pItem->HasProgramInfoTag() && (pItem->GetProgramInfoTag()->m_type.Equals("application") || pItem->GetProgramInfoTag()->m_type.Equals("emulator")))
    iType = PROGRAMDB_CONTENT_APPLICATIONS;

  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false))
    return;
  
  database.UpdateGameTitle(iDbId, strInput, iType);
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

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://programplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem("newsmartplaylist://program", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  // TODO: implement this

  return bResult;
}

bool CGUIWindowProgramBase::OnClick(int iItem)
{
  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowProgramBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  // contextual buttons
  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean() && !item->IsParentFolder())
  {
    if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
      buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);

    CStdString strXbePath = item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strFileNameAndPath : item->GetPath();
    if (!item->m_bIsFolder && URIUtils::GetExtension(strXbePath).Equals(".xbe"))
    { // XBE related buttons (save games, trainers, launch etc.)
      CStdString strLaunch = g_localizeStrings.Get(518);
      if (g_guiSettings.GetBool("myprograms.gameautoregion"))
      {
        int iRegion = GetRegion(itemNumber);
        if (iRegion == VIDEO_NTSCM)
          strLaunch += " (NTSC-M)";
        if (iRegion == VIDEO_NTSCJ)
          strLaunch += " (NTSC-J)";
        if (iRegion == VIDEO_PAL50)
          strLaunch += " (PAL)";
        if (iRegion == VIDEO_PAL60)
          strLaunch += " (PAL-60)";
        buttons.Add(CONTEXT_BUTTON_LAUNCH_IN, 519); // launch in video mode
      }
      buttons.Add(CONTEXT_BUTTON_LAUNCH, strLaunch); // Launch

      CStdString strTitleId;
      DWORD dwTitleId = CUtil::GetXbeID(strXbePath);
      strTitleId.Format("%08X", dwTitleId);

      CStdString strGameSavepath;
      URIUtils::AddFileToFolder("E:\\udata\\", strTitleId, strGameSavepath);
      if (CDirectory::Exists(strGameSavepath))
        buttons.Add(CONTEXT_BUTTON_GAMESAVES, 20322);

      if (m_database.ItemHasTrainer(dwTitleId))
        buttons.Add(CONTEXT_BUTTON_TRAINER_OPTIONS, 12015);

      buttons.Add(CONTEXT_BUTTON_SCAN_TRAINERS, 12012);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
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
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      CStdString playlist = m_vecItems->Get(itemNumber)->IsSmartPlayList() ? m_vecItems->Get(itemNumber)->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "program"))
        Refresh(true); // need to update
      return true;
    }
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_TRAINER_OPTIONS:
    {
      DWORD dwTitleId = CUtil::GetXbeID(item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strFileNameAndPath : item->GetPath());
      if (CGUIDialogTrainerSettings::ShowForTitle(dwTitleId, &m_database))
        Update(m_vecItems->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_SCAN_TRAINERS:
    {
      PopulateTrainersList();
      Update(m_vecItems->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_GAMESAVES:
    {
      CStdString strTitleID;
      CStdString strGameSavepath;
      strTitleID.Format("%08X", CUtil::GetXbeID(item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strFileNameAndPath : item->GetPath()));
      URIUtils::AddFileToFolder("E:\\udata\\", strTitleID, strGameSavepath);
      g_windowManager.ActivateWindow(WINDOW_GAMESAVES, strGameSavepath);
      return true;
    }
  case CONTEXT_BUTTON_LAUNCH:
    return OnPlayMedia(itemNumber);
  case CONTEXT_BUTTON_LAUNCH_IN:
    return OnChooseVideoModeAndLaunch(itemNumber);
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

/*
 TODO:
  1. Forcing video mode
  2. Applying trainer
  3. Launching roms
  4. Launching from DVD
*/
bool CGUIWindowProgramBase::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() )
    return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  
  if (pItem->HasProgramInfoTag() && m_database.Open())
  {
    m_database.IncrementPlayCount(*pItem);
    m_database.Close();
  }

  CFileItem file;
  file.SetPath(pItem->HasProgramInfoTag() ? pItem->GetProgramInfoTag()->m_strFileNameAndPath : pItem->GetPath());
  if (file.IsXBE())
  {
    DWORD dwTitleId = CUtil::GetXbeID(file.GetPath());
    
    // check if trainer is activated for this game
    CStdString strTrainer = m_database.GetActiveTrainer(dwTitleId);
    if (strTrainer != "")
    {
      CTrainer trainer;
      if (trainer.Load(strTrainer))
      {
        m_database.GetTrainerOptions(strTrainer, dwTitleId, trainer.GetOptions(), trainer.GetNumberOfOptions());
        CUtil::InstallTrainer(trainer);
      }
    }

    // Check for region override
    int iRegion = m_iRegionSet ? m_iRegionSet : GetRegion(iItem);
    CUtil::RunXBE(file.GetPath(), NULL, F_VIDEO(iRegion));
  }
  else if (file.IsROM())
    return EmulatorUtils::ChooseEmulatorAndLaunch(file.GetPath());

  return false;
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
  if (dir.Equals("$PLAYLISTS") || dir.Equals("Playlists"))
    return "special://programplaylists/";
  else if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/executable/";
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

void CGUIWindowProgramBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size())
    return;

  OnDeleteItem(m_vecItems->Get(iItem));

  Refresh(true);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowProgramBase::OnDeleteItem(CFileItemPtr item)
{
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;
  if (g_settings.GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      g_settings.GetCurrentProfile().filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }

  CFileUtils::DeleteItem(item);
}

bool CGUIWindowProgramBase::OnChooseVideoModeAndLaunch(int item)
{
  if (item < 0 || item >= m_vecItems->Size()) return false;

  int btn_PAL = 1;
  int btn_NTSCM = 2;
  int btn_NTSCJ = 3;
  int btn_PAL60 = 4;
  CStdString strPAL, strNTSCJ, strNTSCM, strPAL60;
  strPAL = "PAL";
  strNTSCM = "NTSC-M";
  strNTSCJ = "NTSC-J";
  strPAL60 = "PAL-60";
  int iRegion = GetRegion(item,true);

  if (iRegion == VIDEO_NTSCM)
    strNTSCM += " (default)";
  if (iRegion == VIDEO_NTSCJ)
    strNTSCJ += " (default)";
  if (iRegion == VIDEO_PAL50)
    strPAL += " (default)";

  // add the needed buttons
  CContextButtons choices;
  choices.Add(btn_PAL, strPAL);
  choices.Add(btn_NTSCM, strNTSCM);
  choices.Add(btn_NTSCJ, strNTSCJ);
  choices.Add(btn_PAL60, strPAL60);

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);

  if (btnid == btn_NTSCM)
  {
    m_iRegionSet = VIDEO_NTSCM;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),1);
  }
  if (btnid == btn_NTSCJ)
  {
    m_iRegionSet = VIDEO_NTSCJ;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),2);
  }
  if (btnid == btn_PAL)
  {
    m_iRegionSet = VIDEO_PAL50;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),4);
  }
  if (btnid == btn_PAL60)
  {
    m_iRegionSet = VIDEO_PAL60;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),8);
  }

  if (btnid > -1)
    return OnPlayMedia(item);

  return true;
}

int CGUIWindowProgramBase::GetRegion(int iItem, bool bReload)
{
  if (!g_guiSettings.GetBool("myprograms.gameautoregion"))
    return 0;

  int iRegion;
  if (bReload || m_vecItems->Get(iItem)->IsOnDVD() || !m_vecItems->Get(iItem)->HasProgramInfoTag())
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(m_vecItems->Get(iItem)->GetPath());
  }
  else
  {
    m_database.Open();
    iRegion = m_database.GetRegion(m_vecItems->Get(iItem)->GetPath());
    m_database.Close();
  }
  if (iRegion == -1)
  {
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(m_vecItems->Get(iItem)->GetPath());
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      m_database.SetRegion(m_vecItems->Get(iItem)->GetPath(),iRegion);
    }
    else
      iRegion = 0;
  }

  if (bReload)
    return CXBE::FilterRegion(iRegion,true);
  else
    return CXBE::FilterRegion(iRegion);
}

void CGUIWindowProgramBase::PopulateTrainersList()
{
  CDirectory directory;
  CFileItemList trainers;
  CFileItemList archives;
  CFileItemList inArchives;
  // first, remove any dead items
  std::vector<CStdString> vecTrainerPath;
  m_database.GetAllTrainers(vecTrainerPath);
  CGUIDialogProgress* m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_dlgProgress->SetLine(0,12023);
  m_dlgProgress->SetLine(1,"");
  m_dlgProgress->SetLine(2,"");
  m_dlgProgress->StartModal();
  m_dlgProgress->SetHeading(12012);
  m_dlgProgress->ShowProgressBar(true);
  m_dlgProgress->Progress();

  bool bBreak=false;
  bool bDatabaseState = m_database.IsOpen();
  if (!bDatabaseState)
    m_database.Open();
  m_database.BeginTransaction();
  for (unsigned int i=0;i<vecTrainerPath.size();++i)
  {
    m_dlgProgress->SetPercentage((int)((float)i/(float)vecTrainerPath.size()*100.f));
    CStdString strLine;
    strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,vecTrainerPath.size());
    m_dlgProgress->SetLine(1,strLine);
    m_dlgProgress->Progress();
    if (!CFile::Exists(vecTrainerPath[i]) || vecTrainerPath[i].find(g_guiSettings.GetString("myprograms.trainerpath",false)) == -1)
      m_database.RemoveTrainer(vecTrainerPath[i]);
    if (m_dlgProgress->IsCanceled())
    {
      bBreak = true;
      m_database.RollbackTransaction();
      break;
    }
  }
  if (!bBreak)
  {
    CLog::Log(LOGDEBUG,"trainerpath %s",g_guiSettings.GetString("myprograms.trainerpath",false).c_str());
    directory.GetDirectory(g_guiSettings.GetString("myprograms.trainerpath").c_str(),trainers,".xbtf|.etm");
    if (g_guiSettings.GetString("myprograms.trainerpath",false).IsEmpty())
    {
      m_database.RollbackTransaction();
      m_dlgProgress->Close();

      return;
    }

    directory.GetDirectory(g_guiSettings.GetString("myprograms.trainerpath").c_str(),archives,".rar|.zip",false); // TODO: ZIP SUPPORT
    for( int i=0;i<archives.Size();++i)
    {
      if (stricmp(URIUtils::GetExtension(archives[i]->GetPath()),".rar") == 0)
      {
        g_RarManager.GetFilesInRar(inArchives,archives[i]->GetPath(),false);
        CHDDirectory dir;
        dir.SetMask(".xbtf|.etm");
        for (int j=0;j<inArchives.Size();++j)
          if (dir.IsAllowed(inArchives[j]->GetPath()))
          {
            CFileItemPtr item(new CFileItem(*inArchives[j]));
            CStdString strPathInArchive = item->GetPath();
            CStdString path;
            URIUtils::CreateArchivePath(path, "rar", archives[i]->GetPath(), strPathInArchive,"");
            item->SetPath(path);
            trainers.Add(item);
          }
      }
      if (stricmp(URIUtils::GetExtension(archives[i]->GetPath()),".zip")==0)
      {
        // add trainers in zip
        CStdString strZipPath;
        URIUtils::CreateArchivePath(strZipPath,"zip",archives[i]->GetPath(),"");
        CFileItemList zipTrainers;
        directory.GetDirectory(strZipPath,zipTrainers,".etm|.xbtf");
        for (int j=0;j<zipTrainers.Size();++j)
        {
          CFileItemPtr item(new CFileItem(*zipTrainers[j]));
          trainers.Add(item);
        }
      }
    }
    if (!m_dlgProgress)
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    m_dlgProgress->SetPercentage(0);
    m_dlgProgress->ShowProgressBar(true);

    CLog::Log(LOGDEBUG,"# trainers %i",trainers.Size());
    m_dlgProgress->SetLine(1,"");
    int j=0;
    while (j < trainers.Size())
    {
      if (trainers[j]->m_bIsFolder)
        trainers.Remove(j);
      else
        j++;
    }
    for (int i=0;i<trainers.Size();++i)
    {
      CLog::Log(LOGDEBUG,"found trainer %s",trainers[i]->GetPath().c_str());
      m_dlgProgress->SetPercentage((int)((float)(i)/trainers.Size()*100.f));
      CStdString strLine;
      strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,trainers.Size());
      m_dlgProgress->SetLine(0,strLine);
      m_dlgProgress->SetLine(2,"");
      m_dlgProgress->Progress();
      if (m_database.HasTrainer(trainers[i]->GetPath())) // skip existing trainers
        continue;

      CTrainer trainer;
      if (trainer.Load(trainers[i]->GetPath()))
      {
        m_dlgProgress->SetLine(1,trainer.GetName());
        m_dlgProgress->SetLine(2,"");
        m_dlgProgress->Progress();
        unsigned int iTitle1, iTitle2, iTitle3;
        trainer.GetTitleIds(iTitle1,iTitle2,iTitle3);
        if (iTitle1)
          m_database.AddTrainer(iTitle1,trainers[i]->GetPath());
        if (iTitle2)
          m_database.AddTrainer(iTitle2,trainers[i]->GetPath());
        if (iTitle3)
          m_database.AddTrainer(iTitle3,trainers[i]->GetPath());
      }
      if (m_dlgProgress->IsCanceled())
      {
        m_database.RollbackTransaction();
        break;
      }
    }
  }
  m_database.CommitTransaction();
  m_dlgProgress->Close();

  if (!bDatabaseState)
    m_database.Close();
  else
    Update(m_vecItems->GetPath());
}
