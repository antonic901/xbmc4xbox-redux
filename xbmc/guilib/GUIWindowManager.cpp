/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowManager.h"

#include "Application.h" // IsCurrentThread
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "GUIInfoManager.h"
#include "GUIPassword.h"
#include "GUITexture.h"
#include "ServiceBroker.h"
#include "WindowIDs.h"
#include "addons/Skin.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "Application.h"
#include "dialogs/GUIDialogFavourites.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/dialogs/GUIDialogInfoProviderSettings.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "music/windows/GUIWindowMusicPlaylist.h"
#include "music/windows/GUIWindowMusicPlaylistEditor.h"
#include "music/windows/GUIWindowVisualisation.h"
#include "pictures/GUIWindowPictures.h"
#include "pictures/GUIWindowSlideShow.h"
#include "profiles/windows/GUIWindowSettingsProfile.h"
#include "programs/GUIWindowPrograms.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIWindowSettings.h"
#include "settings/windows/GUIWindowSettingsCategory.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/dialogs/GUIDialogVideoOSD.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "video/windows/GUIWindowVideoPlaylist.h"
#include "windows/GUIWindowWeather.h"
#include "windows/GUIWindowFileManager.h"
#include "windows/GUIWindowHome.h"
#include "windows/GUIWindowLoginScreen.h"
#include "windows/GUIWindowSplash.h"
#include "windows/GUIWindowStartup.h"
#include "windows/GUIWindowSystemInfo.h"

// Dialog includes
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "music/dialogs/GUIDialogVisualisationPresetList.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "addons/GUIDialogAddonInfo.h"
#include "addons/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSeekBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogSmartPlaylistRule.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "dialogs/GUIDialogYesNo.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "pictures/GUIDialogPictureInfo.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "settings/dialogs/GUIDialogLibExportSettings.h"
#include "video/dialogs/GUIDialogAudioSubtitleSettings.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#include "video/dialogs/GUIDialogVideoSettings.h"

#include "dialogs/GUIDialogSlider.h"
#ifdef HAS_OPTICAL_DRIVE
#include "dialogs/GUIDialogPlayEject.h"
#endif
#include "dialogs/GUIDialogMediaFilter.h"
#include "video/dialogs/GUIDialogSubtitles.h"

using namespace KODI;
using namespace MESSAGING;

CGUIWindowManager::CGUIWindowManager()
{
  m_pCallback = NULL;
  m_iNested = 0;
  m_initialized = false;
}

CGUIWindowManager::~CGUIWindowManager() {}

void CGUIWindowManager::Initialize()
{
  m_tracker.SelectAlgorithm();

  m_initialized = true;

  LoadNotOnDemandWindows();

  CServiceBroker::GetAppMessenger()->RegisterReceiver(this);
}

void CGUIWindowManager::CreateWindows()
{
  Add(new CGUIWindowHome);
  Add(new CGUIWindowPrograms);
  Add(new CGUIWindowPictures);
  Add(new CGUIWindowFileManager);
  Add(new CGUIWindowSettings);
  Add(new CGUIWindowSystemInfo);
  Add(new CGUIWindowSettingsCategory);
  Add(new CGUIWindowVideoNav);
  Add(new CGUIWindowVideoPlaylist);
  Add(new CGUIWindowLoginScreen);
  Add(new CGUIWindowSettingsProfile);
  Add(new CGUIWindow(WINDOW_SKIN_SETTINGS, "SkinSettings.xml"));
  Add(new CGUIWindowAddonBrowser);
  Add(new CGUIDialogYesNo);
  Add(new CGUIDialogProgress);
  Add(new CGUIDialogExtendedProgressBar);
  Add(new CGUIDialogKeyboardGeneric);
  Add(new CGUIDialogVolumeBar);
  Add(new CGUIDialogSeekBar);
  Add(new CGUIDialogSubMenu);
  Add(new CGUIDialogContextMenu);
  Add(new CGUIDialogKaiToast);
  Add(new CGUIDialogNumeric);
  Add(new CGUIDialogGamepad);
  Add(new CGUIDialogButtonMenu);
  Add(new CGUIDialogSlider);
  Add(new CGUIDialogMusicOSD);
  Add(new CGUIDialogVisualisationPresetList);
  Add(new CGUIDialogVideoSettings);
  Add(new CGUIDialogAudioSubtitleSettings);
  Add(new CGUIDialogVideoBookmarks);
  // Don't add the filebrowser dialog - it's created and added when it's needed
  Add(new CGUIDialogMediaSource);
  Add(new CGUIDialogProfileSettings);
  Add(new CGUIDialogFavourites);
  Add(new CGUIDialogSongInfo);
  Add(new CGUIDialogSmartPlaylistEditor);
  Add(new CGUIDialogSmartPlaylistRule);
  Add(new CGUIDialogBusy);
  Add(new CGUIDialogPictureInfo);
  Add(new CGUIDialogAddonInfo);
  Add(new CGUIDialogAddonSettings);

  Add(new CGUIDialogLockSettings);

  Add(new CGUIDialogContentSettings);

  Add(new CGUIDialogLibExportSettings);

  Add(new CGUIDialogInfoProviderSettings);

  Add(new CGUIDialogMediaFilter);
  Add(new CGUIDialogSubtitles);

  Add(new CGUIWindowMusicPlayList);
  Add(new CGUIWindowMusicNav);
  Add(new CGUIWindowMusicPlaylistEditor);

  Add(new CGUIDialogSelect);
  Add(new CGUIDialogMusicInfo);
  Add(new CGUIDialogOK);
  Add(new CGUIDialogVideoInfo);

  Add(new CGUIDialogTextViewer);
  Add(new CGUIWindowVisualisation);
  Add(new CGUIWindowSlideShow);

  Add(new CGUIDialogVideoOSD);
  Add(new CGUIWindowWeather);
  Add(new CGUIWindowStartup);
  Add(new CGUIWindowSplash);
}

bool CGUIWindowManager::DestroyWindows()
{
  try
  {
    DestroyWindow(WINDOW_SPLASH);
    DestroyWindow(WINDOW_MUSIC_PLAYLIST);
    DestroyWindow(WINDOW_MUSIC_PLAYLIST_EDITOR);
    DestroyWindow(WINDOW_MUSIC_NAV);
    DestroyWindow(WINDOW_DIALOG_MUSIC_INFO);
    DestroyWindow(WINDOW_DIALOG_VIDEO_INFO);
    DestroyWindow(WINDOW_VIDEO_PLAYLIST);
    DestroyWindow(WINDOW_VIDEO_NAV);
    DestroyWindow(WINDOW_FILES);
    DestroyWindow(WINDOW_DIALOG_YES_NO);
    DestroyWindow(WINDOW_DIALOG_PROGRESS);
    DestroyWindow(WINDOW_DIALOG_NUMERIC);
    DestroyWindow(WINDOW_DIALOG_GAMEPAD);
    DestroyWindow(WINDOW_DIALOG_SUB_MENU);
    DestroyWindow(WINDOW_DIALOG_BUTTON_MENU);
    DestroyWindow(WINDOW_DIALOG_CONTEXT_MENU);
    DestroyWindow(WINDOW_DIALOG_PLAYER_CONTROLS);
    DestroyWindow(WINDOW_DIALOG_MUSIC_OSD);
    DestroyWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
    DestroyWindow(WINDOW_DIALOG_SELECT);
    DestroyWindow(WINDOW_DIALOG_OK);
    DestroyWindow(WINDOW_DIALOG_KEYBOARD);
    DestroyWindow(WINDOW_FULLSCREEN_VIDEO);
    DestroyWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_LOCK_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_NETWORK_SETUP);
    DestroyWindow(WINDOW_DIALOG_MEDIA_SOURCE);
    DestroyWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    DestroyWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_INFOPROVIDER_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_LIBEXPORT_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_SONG_INFO);
    DestroyWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    DestroyWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    DestroyWindow(WINDOW_DIALOG_BUSY);
    DestroyWindow(WINDOW_DIALOG_PICTURE_INFO);
    DestroyWindow(WINDOW_DIALOG_ADDON_INFO);
    DestroyWindow(WINDOW_DIALOG_ADDON_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_SLIDER);
    DestroyWindow(WINDOW_DIALOG_MEDIA_FILTER);
    DestroyWindow(WINDOW_DIALOG_SUBTITLES);

    DestroyWindow(WINDOW_DIALOG_TEXT_VIEWER);
    DestroyWindow(WINDOW_STARTUP_ANIM);
    DestroyWindow(WINDOW_LOGIN_SCREEN);
    DestroyWindow(WINDOW_VISUALISATION);
    DestroyWindow(WINDOW_SETTINGS_MENU);
    DestroyWindow(WINDOW_SETTINGS_PROFILES);
    DestroyWindow(WINDOW_SCREEN_CALIBRATION);
    DestroyWindow(WINDOW_SYSTEM_INFORMATION);
    DestroyWindow(WINDOW_SCREENSAVER);
    DestroyWindow(WINDOW_DIALOG_VIDEO_OSD);
    DestroyWindow(WINDOW_SLIDESHOW);
    DestroyWindow(WINDOW_ADDON_BROWSER);
    DestroyWindow(WINDOW_SKIN_SETTINGS);

    DestroyWindow(WINDOW_HOME);
    DestroyWindow(WINDOW_PROGRAMS);
    DestroyWindow(WINDOW_PICTURES);
    DestroyWindow(WINDOW_WEATHER);
    DestroyWindow(WINDOW_FULLSCREEN_GAME);

    Remove(WINDOW_SETTINGS_SERVICE);
    Remove(WINDOW_SETTINGS_MYPVR);
    Remove(WINDOW_SETTINGS_PLAYER);
    Remove(WINDOW_SETTINGS_MEDIA);
    Remove(WINDOW_SETTINGS_INTERFACE);
    DestroyWindow(WINDOW_SETTINGS_SYSTEM);  // all the settings categories

    Remove(WINDOW_DIALOG_KAI_TOAST);
    Remove(WINDOW_DIALOG_SEEK_BAR);
    Remove(WINDOW_DIALOG_VOLUME_BAR);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CGUIWindowManager::DestroyWindows()");
    return false;
  }

  return true;
}

void CGUIWindowManager::DestroyWindow(int id)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    pWindow->FreeResources(true);
    delete pWindow;
  }
}

bool CGUIWindowManager::SendMessage(int message, int senderID, int destID, int param1, int param2)
{
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return SendMessage(msg);
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message)
{
  bool handled = false;
  // Send the message to all none window targets
  for (int i = 0; i < int(m_vecMsgTargets.size()); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

    for (std::vector<CGUIWindow*>::reverse_iterator it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
    {
      (*it)->OnMessage(message);
    }

    for (std::map<int, CGUIWindow *>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); ++it)
    {
      it->second->OnMessage(message);
    }

    return true;
  }

  // Normal messages are sent to:
  // 1. All active modeless dialogs
  // 2. The topmost dialog that accepts the message
  // 3. The underlying window (only if it is the sender or receiver if a modal dialog is active)

  bool hasModalDialog(false);
  bool modalAcceptedMessage(false);
  // don't use an iterator for this loop, as some messages mean that m_activeDialogs is altered,
  // which will invalidate any iterator
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  size_t topWindow = m_activeDialogs.size();
  while (topWindow)
  {
    CGUIWindow* dialog = m_activeDialogs[--topWindow];

    if (!modalAcceptedMessage && dialog->IsModalDialog())
    { // modal window
      hasModalDialog = true;
      if (!modalAcceptedMessage && dialog->OnMessage( message ))
      {
        modalAcceptedMessage = handled = true;
      }
    }
    else if (!dialog->IsModalDialog())
    { // modeless
      if (dialog->OnMessage( message ))
        handled = true;
    }

    if (topWindow > m_activeDialogs.size())
      topWindow = m_activeDialogs.size();
  }

  // now send to the underlying window
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
  {
    if (hasModalDialog)
    {
      // only send the message to the underlying window if it's the recipient
      // or sender (or we have no sender)
      if (message.GetSenderId() == window->GetID() ||
          message.GetControlId() == window->GetID() ||
          message.GetSenderId() == 0 )
      {
        if (window->OnMessage(message)) handled = true;
      }
    }
    else
    {
      if (window->OnMessage(message)) handled = true;
    }
  }
  return handled;
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message, int window)
{
  if (window == 0)
    // send to no specified windows.
    return SendMessage(message);
  CGUIWindow* pWindow = GetWindow(window);
  if(pWindow)
    return pWindow->OnMessage(message);
  else
    return false;
}

void CGUIWindowManager::AddUniqueInstance(CGUIWindow *window)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  // increment our instance (upper word of windowID)
  // until we get a window we don't have
  int instance = 0;
  while (GetWindow(window->GetID()))
    window->SetID(window->GetID() + (++instance << 16));
  Add(window);
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  if (!pWindow)
  {
    CLog::Log(LOGERROR, "Attempted to add a NULL window pointer to the window manager.");
    return;
  }
  // push back all the windows if there are more than one covered by this class
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  const std::vector<int>& idRange = pWindow->GetIDRange();
  for (std::vector<int>::const_iterator i = idRange.begin(); i != idRange.end() ; ++i)
  {
    std::map<int, CGUIWindow *>::iterator it = m_mapWindows.find(*i);
    if (it != m_mapWindows.end())
    {
      CLog::Log(LOGERROR,
                "Error, trying to add a second window with id %i "
                "to the window manager",
                *i);
      return;
    }

    m_mapWindows.insert(std::make_pair(*i, pWindow));
  }
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::RegisterDialog(CGUIWindow* dialog)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  // only add the window if it does not exists
  for (std::vector<CGUIWindow*>::const_iterator it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->GetID() == dialog->GetID())
      return;
  }
  m_activeDialogs.push_back(dialog);
}

struct MatchId {
  int id;
  MatchId(int i) : id(i) {}
  bool operator()(int winId) const { return winId == id; }
};
struct MatchWindow {
  CGUIWindow* window;
  MatchWindow(CGUIWindow* w) : window(w) {}
  bool operator()(CGUIWindow* w) const { return w == window; }
};
void CGUIWindowManager::Remove(int id)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  std::map<int, CGUIWindow *>::iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
  {
    CGUIWindow *window = it->second;
    m_windowHistory.erase(std::remove_if(m_windowHistory.begin(),
                                         m_windowHistory.end(),
                                         MatchId(id)),
                          m_windowHistory.end());
    m_activeDialogs.erase(std::remove_if(m_activeDialogs.begin(),
                                         m_activeDialogs.end(),
                                         MatchWindow(window)),
                          m_activeDialogs.end());
    m_mapWindows.erase(it);
  }
  else
  {
    CLog::Log(LOGWARNING,
              "Attempted to remove window {} "
              "from the window manager when it didn't exist",
              id);
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(int id)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    m_deleteWindows.push_back(pWindow);
  }
}

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Deactivate");
  int currentWindow = GetActiveWindow();
  CGUIWindow *pCurrentWindow = GetWindow(currentWindow);
  if (!pCurrentWindow)
    return;     // no windows or window history yet

  // check to see whether our current window has a <previouswindow> tag
  if (pCurrentWindow->GetPreviousWindow() != WINDOW_INVALID)
  {
    //! @todo we may need to test here for the
    //!       whether our history should be changed

    // don't reactivate the previouswindow if it is ourselves.
    if (currentWindow != pCurrentWindow->GetPreviousWindow())
      ActivateWindow(pCurrentWindow->GetPreviousWindow());
    return;
  }
  // get the previous window in our stack
  if (m_windowHistory.size() < 2)
  {
    // no previous window history yet - check if we should just activate home
    if (GetActiveWindow() != WINDOW_INVALID && GetActiveWindow() != WINDOW_HOME)
    {
      CloseWindowSync(pCurrentWindow);
      ClearWindowHistory();
      ActivateWindow(WINDOW_HOME);
    }
    return;
  }
  m_windowHistory.pop_back();
  int previousWindow = GetActiveWindow();
  m_windowHistory.push_back(currentWindow);

  CGUIWindow *pNewWindow = GetWindow(previousWindow);
  if (!pNewWindow)
  {
    CLog::Log(LOGERROR, "Unable to activate the previous window");
    CloseWindowSync(pCurrentWindow);
    ClearWindowHistory();
    ActivateWindow(WINDOW_HOME);
    return;
  }

  // ok to go to the previous window now

  // tell our info manager which window we are going to
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetNextWindow(previousWindow);

  // deinitialize our window
  CloseWindowSync(pCurrentWindow);

  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetNextWindow(WINDOW_INVALID);
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetPreviousWindow(currentWindow);

  // remove the current window off our window stack
  m_windowHistory.pop_back();

  // ok, initialize the new window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Activate new");
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pNewWindow->OnMessage(msg2);

  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(newWindow, params, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false);
}

void CGUIWindowManager::ForceActivateWindow(int iWindowID, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const std::vector<std::string>& params, bool swappingWindows /* = false */, bool force /* = false */)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(CServiceBroker::GetWinSystem()->GetGfxContext());
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTIVATE_WINDOW, iWindowID,
                                               swappingWindows ? 1 : 0, NULL, "", params);
  }
  else
  {
    CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
    ActivateWindow_Internal(iWindowID, params, swappingWindows, force);
  }
}

void CGUIWindowManager::ActivateWindow_Internal(int iWindowID, const std::vector<std::string>& params, bool swappingWindows, bool force /* = false */)
{
  // translate virtual windows
  if (iWindowID == WINDOW_START)
  { // virtual start window
    iWindowID = g_SkinInfo->GetStartWindow();
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

#if 0
  https://github.com/xbmc/xbmc/pull/17129
  // make sure we check mediasources from home
  if (GetActiveWindow() == WINDOW_HOME)
  {
    g_passwordManager.SetMediaSourcePath(!params.empty() ? params[0] : "");
  }
  else
  {
    g_passwordManager.SetMediaSourcePath("");
  }
#endif

  if (!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR,
              "MasterCode or MediaSource-code is wrong: Window with id {} will not be loaded! "
              "Enter a correct code!",
              iWindowID);
    if (GetActiveWindow() == WINDOW_INVALID && iWindowID != WINDOW_HOME)
      ActivateWindow(WINDOW_HOME);
    return;
  }

  // first check existence of the window we wish to activate.
  CGUIWindow *pNewWindow = GetWindow(iWindowID);
  if (!pNewWindow)
  { // nothing to see here - move along
    CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files",
              iWindowID - WINDOW_HOME);
    if (IsWindowActive(WINDOW_STARTUP_ANIM))
      ActivateWindow(WINDOW_HOME);
    return ;
  }
  else if (!pNewWindow->CanBeActivated())
  {
    if (IsWindowActive(WINDOW_STARTUP_ANIM))
      ActivateWindow(WINDOW_HOME);
    return;
  }
  else if (pNewWindow->IsDialog())
  { // if we have a dialog, we do a DoModal() rather than activate the window
    if (!pNewWindow->IsDialogRunning())
    {
      CSingleExit exitit(CServiceBroker::GetWinSystem()->GetGfxContext());
      static_cast<CGUIDialog *>(pNewWindow)->Open(params.size() > 0 ? params[0] : "");
      // Invalidate underlying windows after closing a modal dialog
      MarkDirty();
    }
    return;
  }

  // don't activate a window if there are active modal dialogs of type MODAL
  if (!force && HasModalDialog(true))
  {
    CLog::Log(LOGINFO, "Activate of window '%i' refused because there are active modal dialogs",
              iWindowID);
    CServiceBroker::GetGUI()->GetAudioManager().PlayActionSound(CAction(ACTION_ERROR));
    return;
  }

  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetNextWindow(iWindowID);

  // deactivate any window
  int currentWindow = GetActiveWindow();
  CGUIWindow *pWindow = GetWindow(currentWindow);
  if (pWindow)
    CloseWindowSync(pWindow, iWindowID);
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetNextWindow(WINDOW_INVALID);

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && !m_windowHistory.empty())
    m_windowHistory.pop_back();
  AddToWindowHistory(iWindowID);

  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetPreviousWindow(currentWindow);
  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, currentWindow, iWindowID);
  msg.SetStringParams(params);
  pNewWindow->OnMessage(msg);
//  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::CloseDialogs(bool forceClose) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  //This is to avoid an assert about out of bounds iterator
  //when m_activeDialogs happens to be empty
  if (m_activeDialogs.empty())
    return;

  std::vector<CGUIWindow*> activeDialogs = m_activeDialogs;
  for (std::vector<CGUIWindow*>::const_iterator it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
  {
    if ((*it)->IsModalDialog())
      (*it)->Close(forceClose);
  }
}

void CGUIWindowManager::CloseInternalModalDialogs(bool forceClose) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  if (m_activeDialogs.empty())
    return;

  std::vector<CGUIWindow*> activeDialogs = m_activeDialogs;
  for (std::vector<CGUIWindow*>::const_iterator it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
  {
    if ((*it)->IsModalDialog() && !IsAddonWindow((*it)->GetID()) && !IsPythonWindow((*it)->GetID()))
      (*it)->Close(forceClose);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CGUIWindowManager::SwitchToFullScreen(bool force /* = false */)
{
  // don't switch if the slideshow is active
  if (IsWindowActive(WINDOW_SLIDESHOW))
    return false;

  // if playing from the video info window, close it first!
  if (IsModalDialogTopmost(WINDOW_DIALOG_VIDEO_INFO))
  {
    CGUIDialogVideoInfo* pDialog = GetWindow<CGUIDialogVideoInfo>(WINDOW_DIALOG_VIDEO_INFO);
    if (pDialog)
      pDialog->Close(true);
  }

  // if playing from the album info window, close it first!
  if (IsModalDialogTopmost(WINDOW_DIALOG_MUSIC_INFO))
  {
    CGUIDialogVideoInfo* pDialog = GetWindow<CGUIDialogVideoInfo>(WINDOW_DIALOG_MUSIC_INFO);
    if (pDialog)
      pDialog->Close(true);
  }

  // if playing from the song info window, close it first!
  if (IsModalDialogTopmost(WINDOW_DIALOG_SONG_INFO))
  {
    CGUIDialogVideoInfo* pDialog = GetWindow<CGUIDialogVideoInfo>(WINDOW_DIALOG_SONG_INFO);
    if (pDialog)
      pDialog->Close(true);
  }

  const int activeWindowID = GetActiveWindow();
  int windowID = WINDOW_INVALID;

  // See if we're playing a video
  if (activeWindowID != WINDOW_FULLSCREEN_VIDEO && g_application.m_pPlayer->IsPlayingVideo())
    windowID = WINDOW_FULLSCREEN_VIDEO;

  // See if we're playing an audio song
  if (activeWindowID != WINDOW_VISUALISATION && g_application.m_pPlayer->IsPlayingAudio())
    windowID = WINDOW_VISUALISATION;

  if (windowID != WINDOW_INVALID && (force || windowID != activeWindowID))
  {
    if (force)
      ForceActivateWindow(windowID);
    else
      ActivateWindow(windowID);

#ifdef _XBOX
    CServiceBroker::GetGUI()->GetTextureManager().Flush();
#endif

    return true;
  }

  return false;
}

void CGUIWindowManager::OnApplicationMessage(ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_DIALOG_OPEN:
  {
    if (pMsg->lpVoid)
      static_cast<CGUIDialog*>(pMsg->lpVoid)->Open(pMsg->param2, pMsg->strParam);
    else
    {
      CGUIDialog* pDialog = static_cast<CGUIDialog*>(GetWindow(pMsg->param1));
      if (pDialog)
        pDialog->Open(pMsg->strParam);
    }
  }
  break;

  case TMSG_GUI_WINDOW_CLOSE:
  {
    CGUIWindow *window = static_cast<CGUIWindow *>(pMsg->lpVoid);
    if (window)
      window->Close((pMsg->param1 & 0x1) ? true : false, pMsg->param1, (pMsg->param1 & 0x2) ? true : false);
  }
  break;

  case TMSG_GUI_ACTIVATE_WINDOW:
  {
    ActivateWindow(pMsg->param1, pMsg->params, pMsg->param2 > 0);
  }
  break;

  case TMSG_GUI_PREVIOUS_WINDOW:
  {
    PreviousWindow();
  }
  break;

  case TMSG_GUI_ADDON_DIALOG:
  {
#if 0
    if (pMsg->lpVoid)
    {
      static_cast<ADDON::CGUIAddonWindowDialog*>(pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
    }
#endif
  }
  break;

#ifdef HAS_PYTHON
  case TMSG_GUI_PYTHON_DIALOG:
  {
    // This hack is not much better but at least I don't need to make ApplicationMessenger
    //  know about Addon (Python) specific classes.
    CAction caction(pMsg->param1);
    static_cast<CGUIWindow*>(pMsg->lpVoid)->OnAction(caction);
  }
  break;
#endif

  case TMSG_GUI_ACTION:
  {
    if (pMsg->lpVoid)
    {
      CAction *action = static_cast<CAction *>(pMsg->lpVoid);
      if (pMsg->param1 == WINDOW_INVALID)
        g_application.OnAction(*action);
      else
      {
        CGUIWindow *pWindow = GetWindow(pMsg->param1);
        if (pWindow)
          pWindow->OnAction(*action);
        else
          CLog::Log(LOGWARNING, "Failed to get window with ID %i to send an action to",
                    pMsg->param1);
      }
      delete action;
    }
  }
  break;

  case TMSG_GUI_MESSAGE:
    if (pMsg->lpVoid)
    {
      CGUIMessage *message = static_cast<CGUIMessage *>(pMsg->lpVoid);
      SendMessage(*message, pMsg->param1);
      delete message;
    }
    break;

  case TMSG_GUI_DIALOG_YESNO:
  {

    if (!pMsg->lpVoid && pMsg->param1 < 0 && pMsg->param2 < 0)
      return;

    CGUIDialogYesNo *dialog = static_cast<CGUIDialogYesNo*>(GetWindow(WINDOW_DIALOG_YES_NO));
    if (!dialog)
      return;

    if (pMsg->lpVoid)
      pMsg->SetResult(dialog->ShowAndGetInput(*static_cast<HELPERS::DialogYesNoMessage*>(pMsg->lpVoid)));
    else
    {
      HELPERS::DialogYesNoMessage options;
      options.heading = pMsg->param1;
      options.text = pMsg->param2;
      pMsg->SetResult(dialog->ShowAndGetInput(options));
    }

  }
  break;

  case TMSG_GUI_DIALOG_OK:
  {

    if (!pMsg->lpVoid && pMsg->param1 < 0 && pMsg->param2 < 0)
      return;

    CGUIDialogOK *dialogOK = static_cast<CGUIDialogOK*>(GetWindow(WINDOW_DIALOG_OK));
    if (!dialogOK)
      return;

    if (pMsg->lpVoid)
      dialogOK->ShowAndGetInput(*static_cast<HELPERS::DialogOKMessage*>(pMsg->lpVoid));
    else
    {
      HELPERS::DialogOKMessage options;
      options.heading = pMsg->param1;
      options.text = pMsg->param2;
      dialogOK->ShowAndGetInput(options);
    }
    pMsg->SetResult(static_cast<int>(dialogOK->IsConfirmed()));
  }
  break;
  }
}

int CGUIWindowManager::GetMessageMask()
{
  return TMSG_MASK_WINDOWMANAGER;
}

bool CGUIWindowManager::OnAction(const CAction &action) const
{
  return HandleAction(action);
}

bool CGUIWindowManager::HandleAction(CAction const& action) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  size_t topmost = m_activeDialogs.size();
  while (topmost)
  {
    CGUIWindow *dialog = m_activeDialogs[--topmost];
    lock.unlock();
    if (dialog->IsModalDialog())
    { // we have the topmost modal dialog
      if (!dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      {
        bool fallThrough = (dialog->GetID() == WINDOW_DIALOG_FULLSCREEN_INFO);
        if (dialog->OnAction(action))
          return true;
        // dialog didn't want the action - we'd normally return false
        // but for some dialogs we want to drop the actions through
        if (fallThrough)
        {
          lock.lock();
          break;
        }
        return false;
      }
      CLog::Log(LOGWARNING,
                "CGUIWindowManager - {} - ignoring action {}, because topmost modal dialog closing "
                "animation is running",
                __FUNCTION__, action.GetID());
      return true; // do nothing with the action until the anim is finished
    }
    lock.lock();
    if (topmost > m_activeDialogs.size())
      topmost = m_activeDialogs.size();
  }
  lock.unlock();
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
    return window->OnAction(action);
  return false;
}

bool RenderOrderSortFunction(CGUIWindow *first, CGUIWindow *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::Process(unsigned int currentTime)
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  m_dirtyregions.clear();

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DoProcess(currentTime, m_dirtyregions);

  // process all dialogs - visibility may change etc.
  for (std::map<int, CGUIWindow*>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); ++it)
  {
    CGUIWindow *pWindow = it->second;
    if (pWindow && pWindow->IsDialog())
      pWindow->DoProcess(currentTime, m_dirtyregions);
  }

  for (CDirtyRegionList::iterator itr = m_dirtyregions.begin(); itr != m_dirtyregions.end(); ++itr)
    m_tracker.MarkDirtyRegion(*itr);
}

void CGUIWindowManager::MarkDirty()
{
  MarkDirty(CRect(0, 0, float(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()), float(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight())));
}

void CGUIWindowManager::MarkDirty(const CRect& rect)
{
  m_tracker.MarkDirtyRegion(CDirtyRegion(rect));

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->MarkDirtyRegion();

  // make copy of vector as we may remove items from it as we go
  std::vector<CGUIWindow*> activeDialogs = m_activeDialogs;
  for (std::vector<CGUIWindow*>::iterator it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
    if ((*it)->IsDialogRunning())
      (*it)->MarkDirtyRegion();
}

void CGUIWindowManager::RenderPass() const
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
  {
    pWindow->ClearBackground();
    pWindow->DoRender();
  }

  // we render the dialogs based on their render order.
  std::vector<CGUIWindow*> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  for (std::vector<CGUIWindow*>::iterator it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DoRender();
  }
}

void CGUIWindowManager::RenderEx() const
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->RenderEx();

  // We don't call RenderEx for now on dialogs since it is used
  // to trigger non gui video rendering. We can activate it later at any time.
  /*
  vector<CGUIWindow *> &activeDialogs = m_activeDialogs;
  for (iDialog it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->RenderEx();
  }
  */
}

bool CGUIWindowManager::Render()
{
  assert(g_application.IsCurrentThread());
  CSingleExit lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  CDirtyRegionList dirtyRegions = m_tracker.GetDirtyRegions();

  bool hasRendered = false;
  // If we visualize the regions we will always render the entire viewport
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiVisualizeDirtyRegions || CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ALWAYS)
  {
    RenderPass();
    hasRendered = true;
  }
  else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ON_CHANGE)
  {
    if (!dirtyRegions.empty())
    {
      RenderPass();
      hasRendered = true;
    }
  }
  else
  {
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); ++i)
    {
      if (i->IsEmpty())
        continue;

      CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(*i);
      RenderPass();
      hasRendered = true;
    }
    CServiceBroker::GetWinSystem()->GetGfxContext().ResetScissors();
  }

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiVisualizeDirtyRegions)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(), false);
    const CDirtyRegionList &markedRegions  = m_tracker.GetMarkedRegions();
    for (CDirtyRegionList::const_iterator i = markedRegions.begin(); i != markedRegions.end(); ++i)
      CGUITexture::DrawQuad(*i, 0x0fff0000);
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); ++i)
      CGUITexture::DrawQuad(*i, 0x4c00ff00);
  }

  return hasRendered;
}

void CGUIWindowManager::AfterRender()
{
  m_tracker.CleanMarkedRegions();

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->AfterRender();

  // make copy of vector as we may remove items from it as we go
  std::vector<CGUIWindow*> activeDialogs = m_activeDialogs;
  for (std::vector<CGUIWindow*>::iterator it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
    {
      (*it)->AfterRender();
      // Dialog state can affect visibility states
      if (pWindow && (*it)->IsControlDirty())
        pWindow->MarkDirtyRegion();
    }
  }
}

void CGUIWindowManager::FrameMove()
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  if(m_iNested == 0)
  {
    // delete any windows queued for deletion
    for(std::vector<CGUIWindow*>::iterator it = m_deleteWindows.begin(); it != m_deleteWindows.end(); ++it)
    {
      // Free any window resources
      (*it)->FreeResources(true);
      delete (*it);
    }
    m_deleteWindows.clear();
  }

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->FrameMove();
  // update any dialogs - we take a copy of the vector as some dialogs may close themselves
  // during this call
  std::vector<CGUIWindow*> dialogs = m_activeDialogs;
  for (std::vector<CGUIWindow*>::iterator it = dialogs.begin(); it != dialogs.end(); ++it)
  {
    (*it)->FrameMove();
  }
}

CGUIDialog* CGUIWindowManager::GetDialog(int id) const
{
  CGUIWindow *window = GetWindow(id);
  if (window && window->IsDialog())
    return dynamic_cast<CGUIDialog*>(window);
  return NULL;
}

#ifdef HAS_XBOX_D3D
void CGUIWindowManager::RenderDialogs()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  // find the window with the lowest render order
  std::vector<CGUIWindow*> renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  // iterate through and render if they're running
  for (std::vector<CGUIWindow*>::iterator it = renderList.begin(); it != renderList.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DoRender();
  }

  // execute post rendering actions (finalize window closing)
  AfterRender();
}
#endif

CGUIWindow* CGUIWindowManager::GetWindow(int id) const
{
  if (id == 0 || id == WINDOW_INVALID)
    return NULL;

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  std::map<int, CGUIWindow*>::const_iterator it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
    return it->second;
  return NULL;
}

bool CGUIWindowManager::ProcessRenderLoop(bool renderOnly)
{
  bool renderGui = true;

  if (g_application.IsCurrentThread() && m_pCallback)
  {
    renderGui = m_pCallback->GetRenderGUI();
    m_iNested++;
    if (!renderOnly)
      m_pCallback->Process();
    {
      CSingleExit leaveIt(CServiceBroker::GetWinSystem()->GetGfxContext());
      m_pCallback->FrameMove(!renderOnly);
    }
    m_pCallback->Render();
    m_iNested--;
  }
  if (g_application.m_bStop || !renderGui)
    return false;
  else
    return true;
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  // Need a copy because addon-dialogs removes itself on Close()
  std::map<int, CGUIWindow*> closeMap(m_mapWindows);
  for (std::map<int, CGUIWindow*>::iterator it = closeMap.begin(); it != closeMap.end(); ++it)
  {
    CGUIWindow* pWindow = it->second;
    if (IsWindowActive(it->first, false))
    {
      pWindow->DisableAnimations();
      pWindow->Close(true);
    }
    pWindow->ResetControlStates();
    pWindow->FreeResources(true);
  }
  UnloadNotOnDemandWindows();

  m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

  // destroy our custom windows...
  for (int i = 0; i < int(m_vecCustomWindows.size()); i++)
  {
    CGUIWindow *pWindow = m_vecCustomWindows[i];
    RemoveFromWindowHistory(pWindow->GetID());
    Remove(pWindow->GetID());
    delete pWindow;
  }

  // clear our vectors of windows
  m_vecCustomWindows.clear();
  m_activeDialogs.clear();

  m_initialized = false;
}

struct DialogIdMatcher
{
  int m_id;
  DialogIdMatcher(int id) : m_id(id) {}
  bool operator()(CGUIWindow* window) const { return window->GetID() == m_id; }
};
/// \brief Unroute window
/// \param id ID of the window routed
void CGUIWindowManager::RemoveDialog(int id)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_activeDialogs.erase(std::remove_if(m_activeDialogs.begin(),
                                       m_activeDialogs.end(),
                                       DialogIdMatcher(id)),
                         m_activeDialogs.end());
}

bool CGUIWindowManager::HasModalDialog(bool ignoreClosing) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::vector<CGUIWindow*>::const_iterator it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialog() &&
        (*it)->IsModalDialog() &&
        (!ignoreClosing || !(*it)->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
    {
      return true;
    }
  }
  return false;
}

bool CGUIWindowManager::HasVisibleModalDialog() const
{
  return HasModalDialog(false);
}

int CGUIWindowManager::GetTopmostDialog(bool modal, bool ignoreClosing) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::vector<CGUIWindow*>::const_reverse_iterator it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if ((!modal || dialog->IsModalDialog()) && (!ignoreClosing || !dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return dialog->GetID();
  }
  return WINDOW_INVALID;
}

int CGUIWindowManager::GetTopmostDialog(bool ignoreClosing /*= false*/) const
{
  return GetTopmostDialog(false, ignoreClosing);
}

int CGUIWindowManager::GetTopmostModalDialog(bool ignoreClosing /*= false*/) const
{
  return GetTopmostDialog(true, ignoreClosing);
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message, int window /*= 0*/)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back(std::make_pair(msg, window));
}

void CGUIWindowManager::DispatchThreadMessages()
{
  // This method only be called in the xbmc main thread.

  // XXX: for more info of this method
  //      check the pr here: https://github.com/xbmc/xbmc/pull/2253

  // As a thread message queue service, it should follow these rules:
  // 1. [Must] Thread safe, message can be pushed into queue in arbitrary thread context.
  // 2. Messages [must] be processed in dispatch message thread context with the same
  //    order as they be pushed into the queue.
  // 3. Dispatch function [must] support call itself during message process procedure,
  //    and do not break other rules listed here. to make it clear: in the
  //    SendMessage(), it could start another xbmc main thread loop, calling
  //    DispatchThreadMessages() in it's internal loop, this must be supported.
  // 4. During DispatchThreadMessages() processing, any new pushed message [should] not
  //    be processed by the current loop in DispatchThreadMessages(), prevent dead loop.
  // 5. If possible, queued messages can be removed by certain filter condition
  //    and not break above.

  CSingleLock lock(m_critSection);

  while (!m_vecThreadMessages.empty())
  {
    // pop up one message per time to make messages be processed by order.
    // this will ensure rule No.2 & No.3
    CGUIMessage *pMsg = m_vecThreadMessages.front().first;
    int window = m_vecThreadMessages.front().second;
    m_vecThreadMessages.pop_front();

    lock.unlock();

    // XXX: during SendMessage(), there could be a deeper 'xbmc main loop' inited by e.g. doModal
    //      which may loop there and callback to DispatchThreadMessages() multiple times.
    if (window)
      SendMessage( *pMsg, window );
    else
      SendMessage( *pMsg );
    delete pMsg;

    lock.lock();
  }
}

int CGUIWindowManager::RemoveThreadMessageByMessageIds(int *pMessageIDList)
{
  CSingleLock lock(m_critSection);
  int removedMsgCount = 0;
  for (std::list < std::pair<CGUIMessage*,int> >::iterator it = m_vecThreadMessages.begin();
       it != m_vecThreadMessages.end();)
  {
    CGUIMessage *pMsg = it->first;
    int *pMsgID;
    for(pMsgID = pMessageIDList; *pMsgID != 0; ++pMsgID)
      if (pMsg->GetMessage() == *pMsgID)
        break;
    if (*pMsgID)
    {
      it = m_vecThreadMessages.erase(it);
      delete pMsg;
      ++removedMsgCount;
    }
    else
    {
      ++it;
    }
  }
  return removedMsgCount;
}

void CGUIWindowManager::AddMsgTarget(IMsgTargetCallback* pMsgTarget)
{
  m_vecMsgTargets.push_back(pMsgTarget);
}

void CGUIWindowManager::RemoveMsgTarget(IMsgTargetCallback* pMsgTarget)
{
  m_vecMsgTargets.erase(std::remove(m_vecMsgTargets.begin(), m_vecMsgTargets.end(), pMsgTarget),
                        m_vecMsgTargets.end());
}

int CGUIWindowManager::GetActiveWindow() const
{
  if (!m_windowHistory.empty())
    return m_windowHistory.back();
  return WINDOW_INVALID;
}

int CGUIWindowManager::GetActiveWindowOrDialog() const
{
  // if there is a dialog active get the dialog id instead
  int iWin = GetTopmostModalDialog() & WINDOW_ID_MASK;
  if (iWin != WINDOW_INVALID)
    return iWin;

  // get the currently active window
  return GetActiveWindow() & WINDOW_ID_MASK;
}

bool CGUIWindowManager::IsWindowActive(int id, bool ignoreClosing /* = true */) const
{
  // mask out multiple instances of the same window
  id &= WINDOW_ID_MASK;
  if ((GetActiveWindow() & WINDOW_ID_MASK) == id)
    return true;
  // run through the dialogs
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::vector<CGUIWindow*>::const_iterator it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if (((*it)->GetID() & WINDOW_ID_MASK) == id && (!ignoreClosing || !(*it)->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowActive(const std::string &xmlFile, bool ignoreClosing /* = true */) const
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CGUIWindow *window = GetWindow(GetActiveWindow());
  if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), xmlFile))
    return true;
  // run through the dialogs
  for (std::vector<CGUIWindow*>::const_iterator it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(URIUtils::GetFileName((*it)->GetProperty("xmlfile").asString()), xmlFile) &&
        (!ignoreClosing || !(*it)->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowVisible(int id) const
{
  return IsWindowActive(id, false);
}

bool CGUIWindowManager::IsWindowVisible(const std::string &xmlFile) const
{
  return IsWindowActive(xmlFile, false);
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::map<int, CGUIWindow*>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); ++it)
  {
    CGUIWindow *pWindow = it->second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT)
    {
      pWindow->FreeResources(true);
      pWindow->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::map<int, CGUIWindow*>::iterator it = m_mapWindows.begin(); it != m_mapWindows.end(); ++it)
  {
    CGUIWindow *pWindow = it->second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT ||
        pWindow->GetLoadType() == CGUIWindow::KEEP_IN_MEMORY)
    {
      pWindow->FreeResources(true);
    }
  }
}

void CGUIWindowManager::AddToWindowHistory(int newWindowID)
{
  // Check the window stack to see if this window is in our history,
  // and if so, pop all the other windows off the stack so that we
  // always have a predictable "Back" behaviour for each window
  std::deque<int> history = m_windowHistory;
  while (!history.empty())
  {
    if (history.back() == newWindowID)
      break;
    history.pop_back();
  }
  if (!history.empty())
  { // found window in history
    m_windowHistory.swap(history);
  }
  else
  {
    // didn't find window in history - add it to the stack
    m_windowHistory.push_back(newWindowID);
  }
}

void CGUIWindowManager::RemoveFromWindowHistory(int windowID)
{
  std::deque<int> history = m_windowHistory;

  // pop windows from stack until we found the window
  while (!history.empty())
  {
    if (history.back() == windowID)
      break;
    history.pop_back();
  }

  // found window in history
  if (!history.empty())
  {
    history.pop_back(); // remove window from stack
    m_windowHistory.swap(history);
  }
}

bool CGUIWindowManager::IsModalDialogTopmost(int id) const
{
  return IsDialogTopmost(id, true);
}

bool CGUIWindowManager::IsModalDialogTopmost(const std::string &xmlFile) const
{
  return IsDialogTopmost(xmlFile, true);
}

bool CGUIWindowManager::IsDialogTopmost(int id, bool modal /* = false */) const
{
  CGUIWindow *topmost = GetWindow(GetTopmostDialog(modal, false));
  if (topmost && (topmost->GetID() & WINDOW_ID_MASK) == id)
    return true;
  return false;
}

bool CGUIWindowManager::IsDialogTopmost(const std::string &xmlFile, bool modal /* = false */) const
{
  CGUIWindow *topmost = GetWindow(GetTopmostDialog(modal, false));
  if (topmost && StringUtils::EqualsNoCase(URIUtils::GetFileName(topmost->GetProperty("xmlfile").asString()), xmlFile))
    return true;
  return false;
}

bool CGUIWindowManager::HasVisibleControls()
{
  CSingleExit lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  if (m_activeDialogs.empty())
  {
    CGUIWindow *window(GetWindow(GetActiveWindow()));
    return !window || window->HasVisibleControls();
  }
  else
    return true;
}

void CGUIWindowManager::ClearWindowHistory()
{
  while (!m_windowHistory.empty())
    m_windowHistory.pop_back();
}

void CGUIWindowManager::CloseWindowSync(CGUIWindow *window, int nextWindowID /*= 0*/)
{
  window->Close(false, nextWindowID);

  bool renderLoopProcessed = true;
  while (window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE) && renderLoopProcessed)
    renderLoopProcessed = ProcessRenderLoop(true);
}

#ifdef _DEBUG
void CGUIWindowManager::DumpTextureUse()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DumpTextureUse();

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (std::vector<CGUIWindow*>::iterator it = m_activeDialogs.begin(); it != m_activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->DumpTextureUse();
  }
}
#endif
