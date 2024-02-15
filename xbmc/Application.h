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
#include "Key.h"
#include "utils/GlobalsHandling.h"

class CFileItem;
class CFileItemList;
namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
}

#include "windows/GUIWindowPointer.h"   // Mouse pointer

#include "utils/Idle.h"
#include "utils/DelayController.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "settings/ISettingsHandler.h"
#include "settings/ISubSettings.h"
#include "storage/DetectDVDType.h"
#include "Autorun.h"
#include "video/Bookmark.h"
#include "utils/Stopwatch.h"
#include "ApplicationMessenger.h"

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

class CCdgParser;
class CApplicationMessenger;
class CProfile;
class CSplash;
class CGUITextLayout;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback,
                     public ISettingCallback, public ISettingsHandler, public ISubSettings
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
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
  bool LoadSkin(const CStdString& skinID);
  void UnloadSkin();
  bool LoadUserWindows();
  void ReloadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
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
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState(bool bForeground = false);
  void UpdateFileState();
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  bool IsPlaying() const;
  bool IsPaused() const;
  bool IsPlayingAudio() const;
  bool IsPlayingVideo() const;
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
  int GetPlaySpeed() const;
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void SetPlaySpeed(int iSpeed);
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow();
  double GetTotalTime() const;
  double GetTime() const;
  float GetPercentage() const;

  // Get the percentage of data currently cached/buffered (aq/vq + FileCache) from the input stream if applicable.
  float GetCachePercentage() const;

  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );
  void ResetPlayTime();

  void StopVideoScan();
  void StopMusicScan();
  bool IsMusicScanning() const;
  bool IsVideoScanning() const;

  void StartVideoCleanup();

  void StartVideoScan(const CStdString &path, bool scanAll = false);
  void StartMusicScan(const CStdString &path);
  void StartMusicAlbumScan(const CStdString& strDirectory);
  void StartMusicArtistScan(const CStdString& strDirectory);

  void UpdateLibraries();
  void CheckMusicPlaylist();

  CApplicationMessenger& getApplicationMessenger();
  CNetwork& getNetwork();

  bool ExecuteXBMCAction(std::string action);

  CGUIWindowPointer m_guiPointer;

  CIdleThread m_idleThread;
  MEDIA_DETECT::CAutorun m_Autorun;
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
  CDelayController m_ctrDpad;
  IPlayer* m_pPlayer;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  DWORD m_dwSpinDownTime;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  bool m_bIsPaused;
  bool m_bPlaybackStarting;
  bool m_128MBHack;

  CCdgParser* m_pCdgParser;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();

  bool SetLanguage(const CStdString &strLanguage);

  ReplayGainSettings& GetReplayGainSettings() { return m_replayGainSettings; }
protected:
  virtual bool OnSettingsSaving() const;

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

  void LoadSkin(const boost::shared_ptr<ADDON::CSkinInfo>& skin);

  friend class CApplicationMessenger;
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
  CStdString m_prevMedia;
  CSplash* m_splash;
  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  PLAYERCOREID m_eCurrentPlayer;
  bool m_bInitializing;

  CBookmark m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  CGUITextLayout *m_debugLayout;

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  VIDEO::CVideoInfoScanner *m_videoInfoScanner;
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

  bool PlayStack(const CFileItem& item, bool bRestart);
  bool SwitchToFullScreen();
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);

  bool ProcessJoystickEvent(const std::string& joystickName, int button, bool isAxis, float fAmount, unsigned int holdTime = 0);

  void CheckForDebugButtonCombo();
  float NavigationIdleTime();
  void CheckForTitleChange();
  static bool AlwaysProcess(const CAction& action);

  void SaveCurrentFileSettings();

  void InitDirectoriesXbox();

  CApplicationMessenger m_applicationMessenger;
  CNetwork    *m_network;
  
#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

  ReplayGainSettings m_replayGainSettings;
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
extern CStdString g_LoadErrorStr;
