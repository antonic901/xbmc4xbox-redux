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

#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/ProgramInfoScanner.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/Builtins.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "storage/MediaManager.h"
#include "utils/AsyncFileCopy.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils2.h"
#include "Application.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_TRACKS           5
#define CONTROL_BTN_REFRESH          6
#define CONTROL_BTN_PLAY             8
#define CONTROL_BTN_GET_THUMB       10
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_GET_FANART      12

#define CONTROL_LIST                50

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml")
    , m_gameItem(new CFileItem)
{
  m_bRefreshAll = true;
  m_bRefresh = false;
  m_hasUpdatedThumb = false;
  m_patchList = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
  SET_CONTROL_HIDDEN(CONTROL_LIST);
  SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
  delete m_patchList;
}

bool CGUIDialogProgramInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
      {
        ClearPatchList();
      }
      break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_bRefresh = false;
      m_bRefreshAll = true;
      m_hasUpdatedThumb = false;

      CGUIDialog::OnMessage(message);
      m_bViewReview = true;

      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_REFRESH, g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_FANART, g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);

      if (!m_gameItem->IsXBE())
        SET_CONTROL_HIDDEN(CONTROL_BTN_TRACKS);
      else
        SET_CONTROL_VISIBLE(CONTROL_BTN_TRACKS);

      Update();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_BTN_PLAY)
        Play();
      else if (iControl == CONTROL_BTN_GET_THUMB)
        OnGetThumb();
      else if(iControl == CONTROL_BTN_PLAY_TRAILER)
        PlayTrailer();
      else if (iControl == CONTROL_BTN_GET_FANART)
        OnGetFanart();
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          OnMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= m_patchList->Size())
            break;
          
          Play(m_patchList->Get(iItem)->GetPath());
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogProgramInfo::SetGame(const CFileItem *item)
{
  *m_gameItem = *item;
  ClearPatchList();
  PROGRAMDB_CONTENT_TYPE type = GetContentType(m_gameItem.get());

  // Load patched XBEs. Patched XBEs must start with "patch_". Example: patch_720p_Widescreen.xbe will result in "720p + Widescreen"
  CFileItemList patches;
  CDirectory::GetDirectory(m_gameItem->GetProgramInfoTag()->m_basePath, patches, ".xbe");
  for (int i = 0; i < patches.Size(); i++)
  {
    CFileItemPtr patch = patches[i];
    CStdString label(patch->GetLabel());
    if (label.Find("patch_") > -1)
    {
      URIUtils::RemoveExtension(label);
      StringUtils2::Replace(label, "patch_", "");
      StringUtils2::Replace(label, "_", " + ");
      patch->SetLabel2(patch->GetLabel());
      patch->SetLabel(label);
      patch->GetProgramInfoTag()->m_type = "xbe_patch";
      m_patchList->Add(patch);
    }
  }

  // set fanart property for games
  if (m_gameItem->CacheLocalFanart())
    m_gameItem->SetProperty("fanart_image", m_gameItem->GetCachedFanart());
  if (type == PROGRAMDB_CONTENT_GAMES)
  {
    m_patchList->SetContent("games");
    if (m_gameItem->GetProgramInfoTag()->m_strTrailer.IsEmpty())
    {
      m_gameItem->GetProgramInfoTag()->m_strTrailer = m_gameItem->FindTrailer();
      if (!m_gameItem->GetProgramInfoTag()->m_strTrailer.IsEmpty())
      {
        CProgramDatabase database;
        if(database.Open())
        {
          database.SetDetail(m_gameItem->GetProgramInfoTag()->m_strTrailer,
                              m_gameItem->GetProgramInfoTag()->m_iDbId,
                              PROGRAMDB_ID_TRAILER, PROGRAMDB_CONTENT_GAMES);
          database.Close();
          CUtil::DeleteProgramDatabaseDirectoryCache();
        }
      }
    }
  }
  m_loader.LoadItem(m_gameItem.get());
}

void CGUIDialogProgramInfo::Update()
{
  CStdString strTmp;

  // setup plot text area
  strTmp = m_gameItem->GetProgramInfoTag()->m_strPlot;
  strTmp.Trim();
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_patchList);
  OnMessage(msg);

  if (m_bViewReview)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 35114);

    SET_CONTROL_HIDDEN(CONTROL_LIST);
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 207);

    SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
    SET_CONTROL_VISIBLE(CONTROL_LIST);
  }

  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_gameItem->GetThumbnailImage());
  }
  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  if (m_hasUpdatedThumb)
  {
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    g_windowManager.SendMessage(reload);
  }
}

bool CGUIDialogProgramInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIDialogProgramInfo::RefreshAll() const
{
  return m_bRefreshAll;
}

PROGRAMDB_CONTENT_TYPE CGUIDialogProgramInfo::GetContentType(const CFileItem *pItem) const
{
  PROGRAMDB_CONTENT_TYPE type = PROGRAMDB_CONTENT_GAMES;
  return type;
}

void CGUIDialogProgramInfo::ClearPatchList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_patchList->Clear();
}

void CGUIDialogProgramInfo::Play(const CStdString& strPath /* = "" */)
{
  CStdString command;
  command.Format("RunProgram(%s)", strPath.IsEmpty() ? m_gameItem->GetProgramInfoTag()->m_strFileNameAndPath : strPath);
  CBuiltins::Execute(command);
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  IMDb thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)
void CGUIDialogProgramInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_gameItem->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_gameItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnails from the web
  int i=1;
  for (std::vector<CScraperUrl::SUrlEntry>::iterator iter=m_gameItem->GetProgramInfoTag()->m_strPictureURL.m_url.begin();iter != m_gameItem->GetProgramInfoTag()->m_strPictureURL.m_url.end();++iter)
  {
    if (iter->m_type == CScraperUrl::URL_TYPE_SEASON)
      continue;
    CStdString strItemPath;
    strItemPath.Format("thumb://Remote%i",i++);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetProgramInfoTag()->m_strPictureURL.m_url.push_back(*iter);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20015));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  CStdString cachedLocalThumb;
  CStdString localThumb(m_gameItem->GetUserProgramThumb());
  if (CFile::Exists(localThumb))
  {
    URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    CPicture pic;
    pic.CreateThumbnail(localThumb, cachedLocalThumb);
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetThumbnailImage(cachedLocalThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  { // no local thumb exists, so we are just using the IGDb thumb or cached thumb
    // which is probably the IGDb thumb.  These could be wrong, so allow the user
    // to delete the incorrect thumb
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetIconImage("DefaultProgram.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  CStdString result;
  VECSOURCES sources(g_settings.m_programSources);
  g_mediaManager.GetLocalDrives(sources);  
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20019), result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CFileItem item(*m_gameItem->GetProgramInfoTag());
  CStdString cachedThumb(item.GetCachedProgramThumb());

  if (result.Left(14) == "thumb://Remote")
  {
    CStdString strFile;
    CFileItem chosen(result, false);
    CStdString thumb = chosen.GetCachedPictureThumb();
    if (CFile::Exists(thumb))
    {
      // NOTE: This could fail if the thumbloader was too slow and the user too impatient
      CFile::Cache(thumb, cachedThumb);
    }
    else
      result = "thumb://None";
  }
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
  {
    CPicture pic;
    pic.CreateThumbnail(result, cachedThumb);
  }
  else 
    result = "thumb://None";

  if (result == "thumb://None")
  {
    CFile::Delete(m_gameItem->GetCachedProgramThumb());
    cachedThumb.Empty();
  }

  CUtil::DeleteProgramDatabaseDirectoryCache(); // to get them new thumbs to show
  m_gameItem->SetThumbnailImage(cachedThumb);
  if (m_gameItem->HasProperty("set_folder_thumb"))
  { // have a folder thumb to set as well
    PROGRAM::CProgramInfoScanner::ApplyThumbToFolder(m_gameItem->GetProperty("set_folder_thumb").asString(), cachedThumb);
  }
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

// Allow user to select a Fanart
void CGUIDialogProgramInfo::OnGetFanart()
{
  CFileItemList items;
  
  CFileItem item(*m_gameItem->GetProgramInfoTag());
  CStdString cachedThumb(item.GetCachedFanart());
  
  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // ensure the fanart is unpacked
  m_gameItem->GetProgramInfoTag()->m_fanart.Unpack();

  // Grab the thumbnails from the web
  CStdString strPath;
  URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,"fanartthumbs",strPath);
  CUtil::WipeDir(strPath);
  XFILE::CDirectory::Create(strPath);
  for (unsigned int i = 0; i < m_gameItem->GetProgramInfoTag()->m_fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetProgramInfoTag()->m_fanart = m_gameItem->GetProgramInfoTag()->m_fanart;
    item->SetProperty("fanart_number", (int)i);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20441));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }
  
  CStdString strLocal = item.GetLocalFanart();
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));
    // make sure any previously cached thumb is removed
    if (CFile::Exists(itemLocal->GetCachedPictureThumb()))
      CFile::Delete(itemLocal->GetCachedPictureThumb());
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultProgram.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  CStdString result;
  VECSOURCES sources(g_settings.m_programSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445) || result.Equals("fanart://Current"))
    return;   // user cancelled
    
  if (CFile::Exists(cachedThumb))
    CFile::Delete(cachedThumb);

  if (result.Equals("fanart://Local"))
    result = strLocal;

  if (result.Left(15) == "fanart://Remote")
  {
    int iFanart = atoi(result.Mid(15).c_str());
    // set new primary fanart, and update our database accordingly
    m_gameItem->GetProgramInfoTag()->m_fanart.SetPrimaryFanart(iFanart);
    CProgramDatabase db;
    if (db.Open())
    {
      db.UpdateFanart(*m_gameItem, GetContentType(m_gameItem.get()));
      db.Close();
    }

    // download the fullres fanart image
    CStdString tempFile = "special://temp/fanart_download.jpg";
    CAsyncFileCopy downloader;
    bool succeeded = downloader.Copy(m_gameItem->GetProgramInfoTag()->m_fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413));
    if (succeeded)
    {
      CPicture pic;
      if (flip)
        pic.ConvertFile(tempFile, cachedThumb,0,1920,-1,100,true);
      else
        pic.CacheFanart(tempFile, cachedThumb);
    }
    CFile::Delete(tempFile);
    if (!succeeded)
      return; // failed or cancelled download, so don't do anything
  }
  else if (CFile::Exists(result))
  { // local file
    CPicture pic;
    if (flip)
      pic.ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      pic.CacheFanart(result, cachedThumb);
  }

  CUtil::DeleteProgramDatabaseDirectoryCache(); // to get them new thumbs to show
  if (CFile::Exists(cachedThumb))
    m_gameItem->SetProperty("fanart_image", cachedThumb);
  else
    m_gameItem->ClearProperty("fanart_image");
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

void CGUIDialogProgramInfo::PlayTrailer()
{
  CFileItem item;
  item.SetPath(m_gameItem->GetProgramInfoTag()->m_strTrailer);
  *item.GetProgramInfoTag() = *m_gameItem->GetProgramInfoTag();
  item.GetVideoInfoTag()->m_streamDetails.Reset();
  item.GetVideoInfoTag()->m_strTitle.Format("%s (%s)",m_gameItem->GetProgramInfoTag()->m_strTitle.c_str(),g_localizeStrings.Get(20410));
  item.SetThumbnailImage(m_gameItem->GetThumbnailImage());
  item.GetVideoInfoTag()->m_iDbId = -1;
  item.GetVideoInfoTag()->m_iFileId = -1;

  // Close the dialog.
  Close(true);

  if (item.IsPlayList())
    g_application.getApplicationMessenger().MediaPlay(item);
  else
    g_application.getApplicationMessenger().PlayFile(item);
}

void CGUIDialogProgramInfo::SetLabel(int iControl, const CStdString &strLabel)
{
  if (strLabel.IsEmpty())
    SET_CONTROL_LABEL(iControl, 416);  // "Not available"
  else
    SET_CONTROL_LABEL(iControl, strLabel);
}

const CStdString& CGUIDialogProgramInfo::GetThumbnail() const 
{
  return m_gameItem->GetThumbnailImage();
}
