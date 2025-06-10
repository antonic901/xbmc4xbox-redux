#pragma once

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

#include "system.h" // for HAS_DVD_DRIVE et. al.
#include "XBApplicationEx.h"

#include "IMsgTargetCallback.h"
#include "guilib/Key.h"
#include "utils/GlobalsHandling.h"
#include "messaging/IMessageTarget.h"
#include "ServiceManager.h"

class CFileItem;
class CFileItemList;
namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
}

#include "utils/Idle.h"
#include "utils/DelayController.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/ISubSettings.h"
#include "storage/DetectDVDType.h"
#include "Autorun.h"
#include "video/Bookmark.h"
#include "utils/Stopwatch.h"
#include "interfaces/IActionListener.h"

class CNetwork;

namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB

// replay gain settings struct for quick access by the player multiple
// times per second (saves doing settings lookup)
struct ReplayGainSettings
{
  int iPreAmp;
  int iNoGainPreAmp;
  int iType;
  bool bAvoidClipping;
};

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

#include "ApplicationPlayer.h"

class CSeekHandler;
class CCdgParser;
class CProfile;
class CSplash;
class CGUITextLayout;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback,
                     public ISettingCallback, public ISettingsHandler, public ISubSettings,
                     public KODI::MESSAGING::IMessageTarget
{
  friend class CApplicationPlayer;
public:

  enum ESERVERS
  {
    ES_WEBSERVER = 1,
    ES_AIRPLAYSERVER,
    ES_JSONRPCSERVER,
    ES_UPNPRENDERER,
    ES_UPNPSERVER,
    ES_EVENTSERVER,
    ES_ZEROCONF
  };

  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove(bool processEvents, bool processGUI = true);
  virtual void Render();
#ifndef HAS_XBOX_D3D
  virtual void RenderNoPresent();
#endif
  virtual HRESULT Create(HWND hWnd);
  virtual HRESULT Cleanup();
  void StartServices();
  void StopServices();
  void StartIdleThread();
  void StopIdleThread();
  void RefreshEventServer();
  void StartLEDControl(bool switchoff = false);
  void DimLCDOnPlayback(bool dim);
  bool IsCurrentThread() const;
  void PrintXBEToLCD(const char* xbePath);
  void CheckDate();
  DWORD GetThreadId() const { return m_threadID; };
  void Stop(bool bLCDStop = true);
  void RestartApp();
  void UnloadSkin(bool forReload = false);
  bool LoadUserWindows();
  void ReloadSkin(bool confirm = false);
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  boost::shared_ptr<CFileItem> CurrentFileItemPtr();
  CFileItem& CurrentUnstackedItem();
  virtual bool OnMessage(CGUIMessage& message);
  PLAYERCOREID GetCurrentPlayer();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  virtual void OnPlayBackSeek(int iTime, int seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnPlayBackSpeedChanged(int iSpeed);

  virtual int  GetMessageMask();
  virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  bool PlayMedia(const CFileItem& item, const std::string &player, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  PlayBackRet PlayFile(CFileItem item, const std::string& player, bool bRestart = false);
  void SaveFileState(bool bForeground = false);
  void UpdateFileState();
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  bool IsPlayingFullScreenVideo() const;
  bool IsStartingPlayback() const { return m_bPlaybackStarting; }
  bool IsFullScreen();
  bool OnKey(CKey& key);
  bool OnAction(CAction &action);
  void RenderMemoryStatus();
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();
  void CheckShutdown();
  void CheckScreenSaver();   // CB: SCREENSAVER PATCH
  void CheckPlayingProgress();
  void ActivateScreenSaver(bool forceType = false);

  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume(bool percentage = true) const;
  void SetVolume(long iValue, bool isPercentage = true);
  int GetDynamicRangeCompressionLevel() { return m_dynamicRangeCompressionLevel; };
  VOICE_MASK GetKaraokeVoiceMask(DWORD dwPort) { return m_karaokeVoiceMask[dwPort]; }
  bool IsMuted() const;
  void ToggleMute(void);
  void SetMute(bool mute);
  void ShowVolumeBar(const CAction *action = NULL);
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow(); // this is MAYBE WakeUpScreenSaverAndDPMS
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

  void StopVideoScan();
  void StopMusicScan();
  bool IsMusicScanning() const;
  bool IsVideoScanning() const;

  /*!
   \brief Starts a video library cleanup.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   */
  void StartVideoCleanup(bool userInitiated = true);

  /*!
   \brief Starts a video library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param scanAll Whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned).
   */
  void StartVideoScan(const std::string &path, bool userInitiated = true, bool scanAll = false);

  /*!
  \brief Starts a music library cleanup.
  \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
  */
  void StartMusicCleanup(bool userInitiated = true);

  /*!
   \brief Starts a music library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param flags Flags for controlling the scanning process.  See xbmc/music/infoscanner/MusicInfoScanner.h for possible values.
   */
  void StartMusicScan(const std::string &path, bool userInitiated = true, int flags = 0);
  void StartMusicAlbumScan(const std::string& strDirectory, bool refresh = false);
  void StartMusicArtistScan(const std::string& strDirectory, bool refresh = false);

  void UpdateLibraries();

  void UpdateCurrentPlayArt();

  CNetwork& getNetwork();

  bool ExecuteXBMCAction(std::string action, const CGUIListItemPtr &item = CGUIListItemPtr());

  CIdleThread m_idleThread;
  MEDIA_DETECT::CAutorun m_Autorun;
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
  CDelayController m_ctrDpad;
  CApplicationPlayer* m_pPlayer;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  DWORD m_dwSpinDownTime;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  bool m_bPlaybackStarting;
  typedef enum
  {
    PLAY_STATE_NONE = 0,
    PLAY_STATE_STARTING,
    PLAY_STATE_PLAYING,
    PLAY_STATE_STOPPED,
    PLAY_STATE_ENDED,
  } PlayState;
  PlayState m_ePlayState;
  CCriticalSection m_playStateMutex;

  bool m_bIsPaused;
  bool m_128MBHack;

  CCdgParser* m_pCdgParser;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();

  bool IsStandAlone()
  {
    return true;
  }

  bool SetLanguage(const std::string &strLanguage);
  bool LoadLanguage(bool reload);

  ReplayGainSettings& GetReplayGainSettings() { return m_replayGainSettings; }

  void SetLoggingIn(bool loggingIn) { m_loggingIn = loggingIn; }

  /*!
   \brief Register an action listener.
   \param listener The listener to register
   */
  void RegisterActionListener(IActionListener *listener);
  /*!
   \brief Unregister an action listener.
   \param listener The listener to unregister
   */
  void UnregisterActionListener(IActionListener *listener);

  boost::movelib::unique_ptr<CServiceManager> m_ServiceManager;

  bool SwitchToFullScreen(bool force = false);

protected:
  virtual bool OnSettingsSaving() const;

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

  bool LoadSkin(const std::string& skinID);

  /*!
   \brief Delegates the action to all registered action handlers.
   \param action The action
   \return true, if the action was taken by one of the action listener.
   */
  bool NotifyActionListeners(const CAction &action) const;

  bool m_skinReverting;
  bool m_ignoreSkinSettingChanges;

  bool m_loggingIn;

  // screensaver
  bool m_bScreenSave;
  ADDON::AddonPtr m_screenSaver;

  D3DGAMMARAMP m_OldRamp;

  // timer information
  CStopWatch m_idleTimer;
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_screenSaverTimer;
  CStopWatch m_shutdownTimer;

  CFileItemPtr m_itemCurrentFile;
  CFileItemList* m_currentStack;
  CFileItemPtr m_stackFileItemToUpdate;

  CStdString m_prevMedia;
  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  bool m_bInitializing;

  CBookmark m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_currentStackPosition;
  int m_nextPlaylistItem;

  CGUITextLayout *m_debugLayout;

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  MUSIC_INFO::CMusicInfoScanner *m_musicInfoScanner;

  bool m_muted;
  int m_volumeLevel;                     // measured in milliBels -60dB -> 0dB range.
  int m_dynamicRangeCompressionLevel;    // measured in milliBels  0dB -> 30dB range.

  VOICE_MASK m_karaokeVoiceMask[4];

  void Mute();
  void UnMute();

  void SetHardwareVolume(long hardwareVolume);
  void UpdateLCD();
  void FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  void InitBasicD3D();

  PlayBackRet PlayStack(const CFileItem& item, bool bRestart);
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);

  bool ProcessJoystickEvent(const std::string& joystickName, int button, bool isAxis, float fAmount, unsigned int holdTime = 0);
  bool ExecuteInputAction(CAction action);

  void CheckForDebugButtonCombo();
  float NavigationIdleTime();
  void CheckForTitleChange();
  static bool AlwaysProcess(const CAction& action);

  void SaveCurrentFileSettings();

  void InitDirectoriesXbox();
  void CreateUserDirs() const;

  CNetwork    *m_network;
  
#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

  ReplayGainSettings m_replayGainSettings;

  std::vector<IActionListener *> m_actionListeners;

  bool m_fallbackLanguageLoaded;

private:
  CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
