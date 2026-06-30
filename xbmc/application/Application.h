/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayerCallback.h"
#include "application/ApplicationSettingsHandling.h"
#include "guilib/IMsgTargetCallback.h"
#include "guilib/IWindowManagerCallback.h"
#include "messaging/IMessageTarget.h"
#include "playlists/PlayListTypes.h"
#include "threads/SystemClock.h"
#include "utils/GlobalsHandling.h"
#include "utils/Stopwatch.h"
#include "windowing/GraphicContext.h"

#include <memory>
#include <string>
#include <vector>

class CAction;
class CBookmark;
class CFileItem;
class CFileItemList;
class CGUIComponent;
class CKey;
class CSeekHandler;
class CServiceManager;
class CSettingsComponent;
class CSplash;
class CWinSystemBase;

namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
  class CAddonInfo;
}

namespace ANNOUNCEMENT
{
  class CAnnouncementManager;
}

namespace MEDIA_DETECT
{
  class CAutorun;
}

namespace PLAYLIST
{
  class CPlayList;
}

namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

class CApplication : public IWindowManagerCallback,
                     public IMsgTargetCallback,
                     public KODI::MESSAGING::IMessageTarget,
                     public CApplicationComponents,
                     public CApplicationPlayerCallback,
                     public CApplicationSettingsHandling
{
public:

  // If playback time of current item is greater than this value, ACTION_PREV_ITEM will seek to start
  // of currently playing item, otherwise it will seek to start of the previous item in playlist
  static const unsigned int ACTION_PREV_ITEM_THRESHOLD = 3; // seconds;

  CApplication(void);
  virtual ~CApplication(void);

  bool Create();
  bool Initialize();
  int Run();
  bool Cleanup();

  virtual void FrameMove(bool processEvents, bool processGUI = true);
  virtual void Render();

  bool IsInitialized() const { return !m_bInitializing; }
  bool IsStopping() const { return m_bStop; }

  bool CreateGUI();
  bool InitWindow(RESOLUTION res = RES_INVALID);

  bool Stop(int exitCode);
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  boost::shared_ptr<CFileItem> CurrentFileItemPtr();
  const CFileItem& CurrentUnstackedItem();
  virtual bool OnMessage(CGUIMessage& message);
  std::string GetCurrentPlayer();

  virtual int  GetMessageMask();
  virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  bool PlayMedia(CFileItem& item, const std::string& player, PLAYLIST::Id playlistId);
  bool ProcessAndStartPlaylist(const std::string& strPlayList,
                               PLAYLIST::CPlayList& playlist,
                               PLAYLIST::Id playlistId,
                               int track = 0);
  bool PlayFile(CFileItem item, const std::string& player, bool bRestart = false);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlayingFullScreenVideo() const;
  bool IsFullScreen();
  bool OnAction(const CAction &action);
  void CloseNetworkShares();

  void ConfigureAndEnableAddons();
  virtual void Process();
  void ProcessSlow();
  /*!
   \brief Returns the total time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTotalTime() returns milliseconds.
   */
  double GetTotalTime() const;
  /*!
   \brief Returns the current time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTime() returns milliseconds.
   */
  double GetTime() const;
  float GetPercentage() const;

  // Get the percentage of data currently cached/buffered (aq/vq + FileCache) from the input stream if applicable.
  float GetCachePercentage() const;

  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );

  void UpdateLibraries();

  void UpdateCurrentPlayArt();

  bool ExecuteXBMCAction(std::string action, const boost::shared_ptr<CGUIListItem>& item = boost::shared_ptr<CGUIListItem>());

#ifdef HAS_OPTICAL_DRIVE
  boost::movelib::unique_ptr<MEDIA_DETECT::CAutorun> m_Autorun;
#endif

  std::string m_strPlayListFile;

  virtual bool GetRenderGUI() const;

  bool SetLanguage(const std::string &strLanguage);
  bool LoadLanguage(bool reload);

  void SetLoggingIn(bool switchingProfiles);

  boost::movelib::unique_ptr<CServiceManager> m_ServiceManager;

  /*!
  \brief Locks calls from outside kodi (e.g. python) until framemove is processed.
  */
  void LockFrameMoveGuard();

  /*!
  \brief Unlocks calls from outside kodi (e.g. python).
  */
  void UnlockFrameMoveGuard();

protected:
  virtual bool OnSettingsSaving() const;
  void PlaybackCleanup();

  boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  boost::movelib::unique_ptr<CGUIComponent> m_pGUI;
  boost::movelib::unique_ptr<CWinSystemBase> m_pWinSystem;

  // timer information
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_slowTimer;
  XbmcThreads::EndTime m_guiRefreshTimer;

  std::string m_prevMedia;
  bool m_bInitializing;

  int m_nextPlaylistItem;

  unsigned int m_lastRenderTime;
  bool m_skipGuiRender;

  boost::movelib::unique_ptr<MUSIC_INFO::CMusicInfoScanner> m_musicInfoScanner;

  bool PlayStack(CFileItem& item, bool bRestart);

  void HandlePortEvents();

  std::vector<boost::shared_ptr<ADDON::CAddonInfo> >
      m_incompatibleAddons; /*!< Result of addon migration (incompatible addon infos) */

public:
  bool m_bStop;

private:
  void PrintStartupLog();
  void ResetCurrentItem();

  mutable CCriticalSection m_critSection; /*!< critical section for all changes to this class, except for changes to triggers */

  CCriticalSection m_frameMoveGuard;              /*!< critical section for synchronizing GUI actions from inside and outside (python) */
  int m_ExitCode;
  boost::shared_ptr<CFileItem> m_itemCurrentFile; //!< Currently playing file
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
