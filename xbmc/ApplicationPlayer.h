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

#include <memory>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "guilib/GraphicContext.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"

typedef enum
{
  PLAYBACK_CANCELED = -1,
  PLAYBACK_FAIL = 0,
  PLAYBACK_OK = 1,
} PlayBackRet;

class IAudioCallback;
class CAction;
class CPlayerOptions;
class CStreamDetails;

struct SPlayerAudioStreamInfo;
struct SPlayerVideoStreamInfo;
struct SPlayerSubtitleStreamInfo;
struct TextCacheStruct_t;

class CApplicationPlayer
{
  boost::shared_ptr<IPlayer> m_pPlayer;
  unsigned int m_iPlayerOPSeq;  // used to detect whether an OpenFile request on player is canceled by us.
  PLAYERCOREID m_eCurrentPlayer;

  CCriticalSection  m_player_lock;

  // cache player state
  XbmcThreads::EndTime m_audioStreamUpdate;
  int m_iAudioStream;
  XbmcThreads::EndTime m_videoStreamUpdate;
  int m_iVideoStream;
  XbmcThreads::EndTime m_subtitleStreamUpdate;
  int m_iSubtitleStream;

public:
  CApplicationPlayer();

  int m_iPlaySpeed;

  // player management
  void CloseFile(bool reopen = false);
  void ClosePlayer();
  void ClosePlayerGapless(PLAYERCOREID newCore);
  void CreatePlayer(PLAYERCOREID newCore, IPlayerCallback& callback);
  PLAYERCOREID GetCurrentPlayer() const { return m_eCurrentPlayer; }
  int  GetPlaySpeed() const;
  bool HasPlayer() const;
  PlayBackRet OpenFile(const CFileItem& item, const CPlayerOptions& options);
  void ResetPlayer() { m_eCurrentPlayer = EPC_NONE; }
  void SetPlaySpeed(int iSpeed, bool bApplicationMuted = false); // bApplicationMuted is not used in DVD/PAPlayer so value can be ignored

  void SetRenderViewMode(int mode);
  float GetRenderAspectRatio();
  bool IsRenderingVideo();
  bool IsExternalPlaying();

  // proxy calls
  int   AddSubtitle(const std::string& strSubPath);
  bool  CanPause();
  bool  CanRecord();
  bool  CanSeek();
  void  DoAudioWork();
  void  GetAudioInfo(CStdString& strAudioInfo);
  int   GetAudioStream();
  int   GetAudioStreamCount();
  void  GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info);
  int   GetCacheLevel() const;
  float GetCachePercentage() const;
  int   GetChapterCount();
  int   GetChapter();
  void  GetChapterName(std::string& strChapterName, int chapterIdx=-1);
  int64_t GetChapterPos(int chapterIdx=-1);
  bool  GetCurrentSubtitle(CStdString& strSubtitle);
  void  GetGeneralInfo(CStdString& strVideoInfo);
  float GetPercentage() const;
  std::string GetPlayerState();
  std::string GetPlayingTitle();
  int   GetPreferredPlaylist() const;
  bool  GetStreamDetails(CStreamDetails &details);
  int   GetSubtitle();
  int   GetSubtitleCount();
  void  GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info);
  bool  GetSubtitleVisible();
  std::string GetRadioText(unsigned int line);
  int64_t GetTime() const;
  int64_t GetTotalTime() const;
  void  GetVideoInfo(CStdString& strVideoInfo);
  void  GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo &info);
  bool  HasAudio() const;
  bool  HasMenu() const;
  bool  HasVideo() const;
  bool  HasRDS() const;
  bool  IsCaching() const;
  bool  IsInMenu() const;
  bool  IsPaused();
  bool  IsPausedPlayback();
  bool  IsPassthrough() const;
  bool  IsPlaying() const;
  bool  IsPlayingAudio() const;
  bool  IsPlayingVideo() const;
  bool  IsPlayingRDS() const;
  bool  IsRecording() const;
  bool  OnAction(const CAction &action);
  void  OnNothingToQueueNotify();
  void  Pause();
  bool  QueueNextFile(const CFileItem &file);
  bool  Record(bool bOnOff);
  void  RegisterAudioCallback(IAudioCallback* pCallback);
  void  Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false);
  int   SeekChapter(int iChapter);
  void  SeekPercentage(float fPercent = 0);
  bool  SeekScene(bool bPlus = true);
  void  SeekTime(int64_t iTime = 0);
  void  SeekTimeRelative(int64_t iTime = 0);
  void  SetAudioStream(int iStream);
  void  SetAVDelay(float fValue = 0.0f);
  void  SetDynamicRangeCompression(long drc);
  void  SetMute(bool bOnOff);
  bool  SetPlayerState(const std::string& state);
  void  SetSubtitle(int iStream);
  void  SetSubTitleDelay(float fValue = 0.0f);
  void  SetSubtitleVisible(bool bVisible);
  void  SetVolume(long volume);
  void  ToFFRW(int iSpeed = 0);
  bool SupportsTempo();
  void  UnRegisterAudioCallback();

#ifdef _XBOX
  float GetActualFPS();
  bool GetSubtitleExtension(std::string& strSubtitleExtension);
  bool SkipNext();
#endif

  protected:
    boost::shared_ptr<IPlayer> GetInternal() const;
};
