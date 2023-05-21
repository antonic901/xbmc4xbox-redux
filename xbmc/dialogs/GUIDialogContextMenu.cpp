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

#include "system.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/GUIDialogContentSettings.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "video/windows/GUIWindowVideoFiles.h"
#include "Application.h"
#include "GUIPassword.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "settings/GUIDialogLockSettings.h"
#include "storage/MediaManager.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "pictures/Picture.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace MEDIA_DETECT;

#define BACKGROUND_IMAGE       999
#if PRE_SKIN_VERSION_11_COMPATIBILITY
#define BACKGROUND_BOTTOM      998
#define BACKGROUND_TOP         997
#define SPACE_BETWEEN_BUTTONS    2
#endif
#define GROUP_LIST             996
#define BUTTON_TEMPLATE       1000
#define BUTTON_START          1001
#define BUTTON_END            (BUTTON_START + (int)m_buttons.size() - 1)

void CContextButtons::Add(unsigned int button, const CStdString &label)
{
  push_back(pair<unsigned int, CStdString>(button, label));
}

void CContextButtons::Add(unsigned int button, int label)
{
  push_back(pair<unsigned int, CStdString>(button, g_localizeStrings.Get(label)));
}

CGUIDialogContextMenu::CGUIDialogContextMenu(void)
  : CGUIDialog(WINDOW_DIALOG_CONTEXT_MENU, "DialogContextMenu.xml")
{
  m_clickedButton = -1;
  m_backgroundImageSize = 0;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogContextMenu::~CGUIDialogContextMenu(void)
{
}

bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  { // someone has been clicked - deinit...
    if (message.GetSenderId() >= BUTTON_START && message.GetSenderId() <= BUTTON_END)
      m_clickedButton = (int)m_buttons[message.GetSenderId() - BUTTON_START].first;
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogContextMenu::OnInitWindow()
{
  m_clickedButton = -1;
  // set initial control focus
  m_lastControlID = BUTTON_START;
  CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::SetupButtons()
{
  if (!m_buttons.size())
    return;

  // disable the template button control
  CGUIButtonControl *pButtonTemplate = (CGUIButtonControl *)GetFirstFocusableControl(BUTTON_TEMPLATE);
  if (!pButtonTemplate) pButtonTemplate = (CGUIButtonControl *)GetControl(BUTTON_TEMPLATE);
  if (!pButtonTemplate)
    return;
  pButtonTemplate->SetVisible(false);

  CGUIControlGroupList* pGroupList = NULL;
  {
    const CGUIControl* pControl = GetControl(GROUP_LIST);
    if (pControl && pControl->GetControlType() == GUICONTROL_GROUPLIST)
      pGroupList = (CGUIControlGroupList*)pControl;
  }

  // add our buttons
  for (unsigned int i = 0; i < m_buttons.size(); i++)
  {
    CGUIButtonControl *pButton = new CGUIButtonControl(*pButtonTemplate);
    if (pButton)
    { // set the button's ID and position
      int id = BUTTON_START + i;
      pButton->SetID(id);
      pButton->SetVisible(true);
      pButton->SetLabel(m_buttons[i].second);
      if (pGroupList)
      {
        pButton->SetPosition(pButtonTemplate->GetXPosition(), pButtonTemplate->GetYPosition());
        pGroupList->AddControl(pButton);
      }
#if PRE_SKIN_VERSION_11_COMPATIBILITY
      else
      {
        pButton->SetPosition(pButtonTemplate->GetXPosition(), i*(pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));
        pButton->SetNavigation(id - 1, id + 1, id, id);
        AddControl(pButton);
      }
#endif
    }
  }
  CGUIControl *pControl = NULL;
#if PRE_SKIN_VERSION_11_COMPATIBILITY
  if (!pGroupList)
  {
    // if we don't have grouplist update the navigation of the first and last buttons
    pControl = (CGUIControl *)GetControl(BUTTON_START);
    if (pControl)
      pControl->SetNavigation(BUTTON_END, pControl->GetControlIdDown(), pControl->GetControlIdLeft(), pControl->GetControlIdRight());
    pControl = (CGUIControl *)GetControl(BUTTON_END);
    if (pControl)
      pControl->SetNavigation(pControl->GetControlIdUp(), BUTTON_START, pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  }
#endif

  // fix up background images placement and size
  pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
  {
    // first set size of background image
    if (pGroupList)
    {
      if (pGroupList->GetOrientation() == VERTICAL)
      {
        // keep gap between bottom edges of grouplist and background image
        float diff = m_backgroundImageSize - pGroupList->GetHeight();
        pGroupList->SetHeight(pGroupList->GetTotalSize());
        pControl->SetHeight(diff + pGroupList->GetHeight());
      }
      else
      {
        // keep gap between right edges of grouplist and background image
        float diff = m_backgroundImageSize - pGroupList->GetWidth();
        pGroupList->SetWidth(pGroupList->GetTotalSize());
        pControl->SetWidth(diff + pGroupList->GetWidth());
      }
    }
#if PRE_SKIN_VERSION_11_COMPATIBILITY
    else
      pControl->SetHeight(m_buttons.size() * (pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));

    if (pGroupList && pGroupList->GetOrientation() == HORIZONTAL)
    {
      // if there is grouplist control with horizontal orientation - adjust width of top and bottom background
      CGUIControl* pControl2 = (CGUIControl *)GetControl(BACKGROUND_TOP);
      if (pControl2)
        pControl2->SetWidth(pControl->GetWidth());

      pControl2 = (CGUIControl *)GetControl(BACKGROUND_BOTTOM);
      if (pControl2)
        pControl2->SetWidth(pControl->GetWidth());
    }
    else
    {
      // adjust position of bottom background
      CGUIControl* pControl2 = (CGUIControl *)GetControl(BACKGROUND_BOTTOM);
      if (pControl2)
        pControl2->SetPosition(pControl2->GetXPosition(), pControl->GetYPosition() + pControl->GetHeight());
    }
#endif
  }

  // update our default control
  if (m_defaultControl < BUTTON_START || m_defaultControl > BUTTON_END)
    m_defaultControl = BUTTON_START;
  while (m_defaultControl <= BUTTON_END && !(GetControl(m_defaultControl)->CanFocus()))
    m_defaultControl++;
}

void CGUIDialogContextMenu::SetPosition(float posX, float posY)
{
  if (posY + GetHeight() > g_settings.m_ResInfo[m_coordsRes].iHeight)
    posY = g_settings.m_ResInfo[m_coordsRes].iHeight - GetHeight();
  if (posY < 0) posY = 0;
  if (posX + GetWidth() > g_settings.m_ResInfo[m_coordsRes].iWidth)
    posX = g_settings.m_ResInfo[m_coordsRes].iWidth - GetWidth();
  if (posX < 0) posX = 0;
#if PRE_SKIN_VERSION_11_COMPATIBILITY
  // we currently hack the positioning of the buttons from y position 0, which
  // forces skinners to place the top image at a negative y value.  Thus, we offset
  // the y coordinate by the height of the top image.
  const CGUIControl *top = GetControl(BACKGROUND_TOP);
  if (top)
    posY += top->GetHeight();
#endif
  CGUIDialog::SetPosition(posX, posY);
}

float CGUIDialogContextMenu::GetHeight()
{
  const CGUIControl *backMain = GetControl(BACKGROUND_IMAGE);
  if (backMain)
#if PRE_SKIN_VERSION_11_COMPATIBILITY
  {
    float height = backMain->GetHeight();
    const CGUIControl *backBottom = GetControl(BACKGROUND_BOTTOM);
    if (backBottom)
      height += backBottom->GetHeight();
    const CGUIControl *backTop = GetControl(BACKGROUND_TOP);
    if (backTop)
      height += backTop->GetHeight();
    return height;
  }
#else
  return backMain->GetHeight();
#endif
  else
    return CGUIDialog::GetHeight();
}

float CGUIDialogContextMenu::GetWidth()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetWidth();
  else
    return CGUIDialog::GetWidth();
}

bool CGUIDialogContextMenu::SourcesMenu(const CStdString &strType, const CFileItemPtr item, float posX, float posY)
{
  // TODO: This should be callable even if we don't have any valid items
  if (!item)
    return false;

  // grab our context menu
  CContextButtons buttons;
  GetContextButtons(strType, item, buttons);

  int button = ShowAndGetChoice(buttons);
  if (button >= 0)
    return OnContextButton(strType, item, (CONTEXT_BUTTON)button);
  return false;
}

void CGUIDialogContextMenu::GetContextButtons(const CStdString &type, const CFileItemPtr item, CContextButtons &buttons)
{
  // Add buttons to the ContextMenu that should be visible for both sources and autosourced items
  if (item && (item->IsDVD() || item->IsCDDA()))
  {
    // We need to check if there is a detected is inserted!
    if ( CDetectDVDMedia::IsDiscInDrive() )
      buttons.Add(CONTEXT_BUTTON_PLAY_DISC, 341); // Play CD/DVD!
    buttons.Add(CONTEXT_BUTTON_EJECT_DISC, 13391);  // Eject/Load CD/DVD!
  }


  // Next, Add buttons to the ContextMenu that should ONLY be visible for sources and not autosourced items
  CMediaSource *share = GetShare(type, item.get());

  if (g_settings.GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser)
  {
    if (share)
    {
      if (!share->m_ignore)
        buttons.Add(CONTEXT_BUTTON_EDIT_SOURCE, 1027); // Edit Source
      buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // Set as Default
      if (!share->m_ignore)
        buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 522); // Remove Source

      buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);
    }
    if (!GetDefaultShareNameByType(type).IsEmpty())
      buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // Clear Default

    buttons.Add(CONTEXT_BUTTON_ADD_SOURCE, 1026); // Add Source
  }
  if (share && LOCK_MODE_EVERYONE != g_settings.GetMasterProfile().getLockMode())
  {
    if (share->m_iHasLock == 0 && (g_settings.GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser))
      buttons.Add(CONTEXT_BUTTON_ADD_LOCK, 12332);
    else if (share->m_iHasLock == 1)
      buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);
    else if (share->m_iHasLock == 2)
    {
      buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);

      bool maxRetryExceeded = false;
      if (g_guiSettings.GetInt("masterlock.maxretries") != 0)
        maxRetryExceeded = (share->m_iBadPwdCount >= g_guiSettings.GetInt("masterlock.maxretries"));
  
      if (maxRetryExceeded)
        buttons.Add(CONTEXT_BUTTON_RESET_LOCK, 12334);
      else
        buttons.Add(CONTEXT_BUTTON_CHANGE_LOCK, 12356);
    }
  }
  if (share && !g_passwordManager.bMasterUser && item->m_iHasLock == 1)
    buttons.Add(CONTEXT_BUTTON_REACTIVATE_LOCK, 12353);
}

bool CGUIDialogContextMenu::OnContextButton(const CStdString &type, const CFileItemPtr item, CONTEXT_BUTTON button)
{
  // Add Source doesn't require a valid share
  if (button == CONTEXT_BUTTON_ADD_SOURCE)
  {
    if (g_settings.IsMasterUser())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else if (!g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;

    return CGUIDialogMediaSource::ShowAndAddMediaSource(type);
  }

  // buttons that are available on both sources and autosourced items
  if (!item) return false;
  switch (button)
  {
  case CONTEXT_BUTTON_PLAY_DISC:
    return CAutorun::PlayDisc();

  case CONTEXT_BUTTON_EJECT_DISC:
#ifdef _WIN32PC
    if( item->GetPath()[0] ) CIoSupport::EjectTray( true, item->GetPath()[0] ); // TODO: detect tray state
#else
    CIoSupport::ToggleTray();
#endif
    return true;
  default:
    break;
  }

  // the rest of the operations require a valid share
  CMediaSource *share = GetShare(type, item.get());
  if (!share) return false;
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT_SOURCE:
    if (g_settings.IsMasterUser())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else if (!g_passwordManager.IsProfileLockUnlocked())
      return false;

    return CGUIDialogMediaSource::ShowAndEditMediaSource(type, *share);
    
  case CONTEXT_BUTTON_REMOVE_SOURCE:
    if (g_settings.IsMasterUser())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else 
    {
      if (!g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsMasterLockUnlocked(false))
        return false;
      if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
    }
    // prompt user if they want to really delete the source
    if (CGUIDialogYesNo::ShowAndGetInput(751, 0, 750, 0))
    { // check default before we delete, as deletion will kill the share object
      CStdString defaultSource(GetDefaultShareNameByType(type));
      if (!defaultSource.IsEmpty())
      {
        if (share->strName.Equals(defaultSource))
          ClearDefault(type);
      }

      // delete this share
      g_settings.DeleteSource(type, share->strName, share->strPath);
      return true;
    }
    break;

  case CONTEXT_BUTTON_SET_DEFAULT:
    if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

    // make share default
    SetDefault(type, share->strName);
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;
    // remove share default
    ClearDefault(type);
    return true;

  case CONTEXT_BUTTON_SET_THUMB:
    {
      if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
      else if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      // setup our thumb list
      CFileItemList items;

      // add the current thumb, if available
      if (!share->m_strThumbnailImage.IsEmpty())
      {
        CFileItemPtr current(new CFileItem("thumb://Current", false));
        current->SetThumbnailImage(share->m_strThumbnailImage);
        current->SetLabel(g_localizeStrings.Get(20016));
        items.Add(current);
      }
      else if (item->HasThumbnail())
      { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
        CFileItemPtr current(new CFileItem("thumb://Current", false));
        current->SetThumbnailImage(item->GetThumbnailImage());
        current->SetLabel(g_localizeStrings.Get(20016));
        items.Add(current);
      }
      // see if there's a local thumb for this item
      CStdString folderThumb = item->GetFolderThumb();
      if (XFILE::CFile::Exists(folderThumb))
      { // cache it
        CPicture pic;
        if (pic.CreateThumbnail(folderThumb, item->GetCachedProgramThumb()))
        {
          CFileItemPtr local(new CFileItem("thumb://Local", false));
          local->SetThumbnailImage(item->GetCachedProgramThumb());
          local->SetLabel(g_localizeStrings.Get(20017));
          items.Add(local);
        }
      }
      // and add a "no thumb" entry as well
      CFileItemPtr nothumb(new CFileItem("thumb://None", false));
      nothumb->SetIconImage(item->GetIconImage());
      nothumb->SetLabel(g_localizeStrings.Get(20018));
      items.Add(nothumb);

      CStdString strThumb;
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares);
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), strThumb))
        return false;

      if (strThumb == "thumb://Current")
        return true;

      if (strThumb == "thumb://None")
        strThumb = "";

      if (!share->m_ignore)
      {
        g_settings.UpdateSource(type,share->strName,"thumbnail",strThumb);
        g_settings.SaveSources();
      }
      else if (!strThumb.IsEmpty())
      { // this is icky as we have to cache using a bunch of different criteria
        CStdString cachedThumb;
        if (type == "music")
        {
          cachedThumb = item->GetPath();
          URIUtils::RemoveSlashAtEnd(cachedThumb);
          cachedThumb = CUtil::GetCachedMusicThumb(cachedThumb);
        }
        else if (type == "video")
          cachedThumb = item->GetCachedVideoThumb();
        else if (type == "pictures")
          cachedThumb = item->GetCachedPictureThumb();
        else  // assume "programs"
          cachedThumb = item->GetCachedProgramThumb();
        XFILE::CFile::Cache(strThumb, cachedThumb);
      }

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_DISC:
    return CAutorun::PlayDisc();

  case CONTEXT_BUTTON_EJECT_DISC:
    if (CIoSupport::GetTrayState() == TRAY_OPEN)
      CIoSupport::CloseTray();
    else
      CIoSupport::EjectTray();
    return true;

  case CONTEXT_BUTTON_ADD_LOCK:
    {
      // prompt user for mastercode when changing lock settings) only for default user
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      CStdString strNewPassword = "";
      if (!CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPassword))
        return false;
      // password entry and re-entry succeeded, write out the lock data
      share->m_iHasLock = 2;
      g_settings.UpdateSource(type, share->strName, "lockcode", strNewPassword);
      strNewPassword.Format("%i",share->m_iLockMode);
      g_settings.UpdateSource(type, share->strName, "lockmode", strNewPassword);
      g_settings.UpdateSource(type, share->strName, "badpwdcount", "0");
      g_settings.SaveSources();

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_RESET_LOCK:
    {
      // prompt user for profile lock when changing lock settings
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      g_settings.UpdateSource(type, share->strName, "badpwdcount", "0");
      g_settings.SaveSources();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REMOVE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      if (!CGUIDialogYesNo::ShowAndGetInput(12335, 0, 750, 0))
        return false;

      share->m_iHasLock = 0;
      g_settings.UpdateSource(type, share->strName, "lockmode", "0");
      g_settings.UpdateSource(type, share->strName, "lockcode", "0");
      g_settings.UpdateSource(type, share->strName, "badpwdcount", "0");
      g_settings.SaveSources();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REACTIVATE_LOCK:
    {
      bool maxRetryExceeded = false;
      if (g_guiSettings.GetInt("masterlock.maxretries") != 0)
        maxRetryExceeded = (share->m_iBadPwdCount >= g_guiSettings.GetInt("masterlock.maxretries"));
      if (!maxRetryExceeded)
      {
        // don't prompt user for mastercode when reactivating a lock
        g_passwordManager.LockSource(type, share->strName, true);
        return true;
      }
      return false;
    }
  case CONTEXT_BUTTON_CHANGE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      CStdString strNewPW;
      CStdString strNewLockMode;
      if (CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPW))
        strNewLockMode.Format("%i",share->m_iLockMode);
      else
        return false;
      // password ReSet and re-entry succeeded, write out the lock data
      g_settings.UpdateSource(type, share->strName, "lockcode", strNewPW);
      g_settings.UpdateSource(type, share->strName, "lockmode", strNewLockMode);
      g_settings.UpdateSource(type, share->strName, "badpwdcount", "0");
      g_settings.SaveSources();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  default:
    break;
  }
  return false;
}

CMediaSource *CGUIDialogContextMenu::GetShare(const CStdString &type, const CFileItem *item)
{
  VECSOURCES *shares = g_settings.GetSourcesFromType(type);
  if (!shares) return NULL;
  for (unsigned int i = 0; i < shares->size(); i++)
  {
    CMediaSource &testShare = shares->at(i);
    if (URIUtils::IsDVD(testShare.strPath))
    {
      if (!item->IsDVD())
        continue;
    }
    else
    {
      if (!testShare.strPath.Equals(item->GetPath()))
        continue;
    }
    // paths match, what about share name - only match the leftmost
    // characters as the label may contain other info (status for instance)
    if (item->GetLabel().Left(testShare.strName.size()).Equals(testShare.strName))
    {
      return &testShare;
    }
  }
  return NULL;
}

void CGUIDialogContextMenu::OnWindowLoaded()
{
  m_coordX = m_posX;
  m_coordY = m_posY;

  const CGUIControlGroupList* pGroupList = NULL;
  const CGUIControl* pControl = GetControl(GROUP_LIST);
  if (pControl && pControl->GetControlType() == GUICONTROL_GROUPLIST)
    pGroupList = (CGUIControlGroupList*)pControl;

  pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl && pGroupList)
  {
    if (pGroupList->GetOrientation() == VERTICAL)
      m_backgroundImageSize = pControl->GetHeight();
    else
      m_backgroundImageSize = pControl->GetWidth();
  }

  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogContextMenu::OnDeinitWindow(int nextWindowID)
{
  //we can't be sure that controls are removed on window unload
  //we have to remove them to be sure that they won't stay for next use of context menu
  for (unsigned int i = 0; i < m_buttons.size(); i++)
  {
    const CGUIControl *control = GetControl(BUTTON_START + i);
    if (control)
      RemoveControl(control);
  }

  m_buttons.clear();
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

CStdString CGUIDialogContextMenu::GetDefaultShareNameByType(const CStdString &strType)
{
  VECSOURCES *pShares = g_settings.GetSourcesFromType(strType);
  CStdString strDefault = g_settings.GetDefaultSourceFromType(strType);

  if (!pShares) return "";

  bool bIsSourceName(false);
  int iIndex = CUtil::GetMatchingSource(strDefault, *pShares, bIsSourceName);
  if (iIndex < 0 || iIndex >= (int)pShares->size())
    return "";

  return pShares->at(iIndex).strName;
}

void CGUIDialogContextMenu::SetDefault(const CStdString &strType, const CStdString &strDefault)
{
  if (strType == "programs")
    g_settings.m_defaultProgramSource = strDefault;
  else if (strType == "files")
    g_settings.m_defaultFileSource = strDefault;
  else if (strType == "music")
    g_settings.m_defaultMusicSource = strDefault;
  else if (strType == "video")
    g_settings.m_defaultVideoSource = strDefault;
  else if (strType == "pictures")
    g_settings.m_defaultPictureSource = strDefault;
  g_settings.SaveSources();
}

void CGUIDialogContextMenu::ClearDefault(const CStdString &strType)
{
  SetDefault(strType, "");
}

void CGUIDialogContextMenu::SwitchMedia(const CStdString& strType, const CStdString& strPath)
{
  // create menu
  CContextButtons choices;
  if (!strType.Equals("music"))
    choices.Add(WINDOW_MUSIC_FILES, 2);
  if (!strType.Equals("video"))
    choices.Add(WINDOW_VIDEO_FILES, 3);
  if (!strType.Equals("pictures"))
    choices.Add(WINDOW_PICTURES, 1);
  if (!strType.Equals("files"))
    choices.Add(WINDOW_FILES, 7);

  int window = ShowAndGetChoice(choices);
  if (window >= 0)
  {
    CUtil::DeleteDirectoryCache();
    g_windowManager.ChangeActiveWindow(window, strPath);
  }
}

int CGUIDialogContextMenu::ShowAndGetChoice(const CContextButtons &choices)
{
  if (choices.size() == 0)
    return -1;

  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    pMenu->m_buttons = choices;
    pMenu->Initialize();
    pMenu->SetupButtons();
    pMenu->PositionAtCurrentFocus();
    pMenu->DoModal();
    return pMenu->m_clickedButton;
  }
  return -1;
}

void CGUIDialogContextMenu::PositionAtCurrentFocus()
{
  CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
  if (window)
  {
    const CGUIControl *focusedControl = window->GetFocusedControl();
    if (focusedControl)
    {
      CPoint pos = focusedControl->GetRenderPosition() + CPoint(focusedControl->GetWidth() * 0.5f, focusedControl->GetHeight() * 0.5f)
                   + window->GetRenderPosition();
      SetPosition(m_coordX + pos.x - GetWidth() * 0.5f, m_coordY + pos.y - GetHeight() * 0.5f);
      return;
    }
  }
  // no control to center at, so just center the window
  CenterWindow();
}
