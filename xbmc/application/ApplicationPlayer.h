/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SeekHandler.h"
#include "application/IApplicationComponent.h"
#include "cores/IPlayer.h"
#include "playlists/PlayListTypes.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"

#include <memory>
#include <string>
#include <vector>

class CAction;
class CPlayerCoreFactory;
class CPlayerOptions;
class CStreamDetails;

struct SPlayerAudioStreamInfo;
struct SPlayerVideoStreamInfo;
struct SPlayerSubtitleStreamInfo;

// TODO: remove this enum
typedef enum
{
  PLAYBACK_CANCELED = -1,
  PLAYBACK_FAIL = 0,
  PLAYBACK_OK = 1,
} PlayBackRet;

class CApplicationPlayer : public IApplicationComponent
{
public:
  CApplicationPlayer() : m_iPlaySpeed(0) {}

  // player management
  void ClosePlayer();
  void ResetPlayer();
  std::string GetCurrentPlayer() const;
  int GetPlaySpeed() const;
  bool HasPlayer() const;
  bool OpenFile(const CFileItem& item, const CPlayerOptions& options,
                const CPlayerCoreFactory &factory,
                const std::string &playerName, IPlayerCallback& callback);
  void OpenNext(const CPlayerCoreFactory &factory);
  void SetPlaySpeed(int speed);

  void SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch);
  float GetRenderAspectRatio() const;
  bool IsRenderingVideo() const;

  /*!
   * \brief Get the name of the player in use
   * \return the player name if a player is active, otherwise it returns an empty string
   */
  std::string GetName() const;

  // proxy calls
  void AddSubtitle(const std::string& strSubPath);
  bool CanPause() const;
  bool CanSeek() const;
  int GetAudioStream();
  int GetAudioStreamCount() const;
  void GetAudioStreamInfo(int index, SPlayerAudioStreamInfo& info);
  int GetCacheLevel() const;
  float GetCachePercentage() const;
  int GetChapterCount() const;
  int GetChapter() const;
  void GetChapterName(std::string& strChapterName, int chapterIdx = -1) const;
  int64_t GetChapterPos(int chapterIdx = -1) const;
  float GetPercentage() const;
  std::string GetPlayerState();
  PLAYLIST::Id GetPreferredPlaylist() const;
  int GetSubtitle();
  int GetSubtitleCount() const;
  void GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo& info);
  bool GetSubtitleVisible() const;
  int64_t GetTime() const;
  int64_t GetTotalTime() const;
  void GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo& info);
  bool HasAudio() const;

  bool HasVideo() const;
  bool IsCaching() const;
  bool IsInMenu() const;
  bool IsPaused() const;
  bool IsPausedPlayback() const;
  bool IsPlaying() const;
  bool IsPlayingAudio() const;
  bool IsPlayingVideo() const;
  bool OnAction(const CAction &action);
  void OnNothingToQueueNotify();
  void Pause();
  bool QueueNextFile(const CFileItem &file);
  void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false);
  int SeekChapter(int iChapter);
  void SeekPercentage(float fPercent = 0);
  bool SeekScene(bool bPlus = true);
  void SeekTime(int64_t iTime = 0);
  void SeekTimeRelative(int64_t iTime = 0);
  void SetAudioStream(int iStream);
  void SetAVDelay(float fValue = 0.0f);
  void SetDynamicRangeCompression(long drc);
  void SetMute(bool bOnOff);
  bool SetPlayerState(const std::string& state);
  void SetSubtitle(int iStream);
  void SetSubTitleDelay(float fValue = 0.0f);
  void SetSubtitleVisible(bool bVisible);

  void SetVolume(int volume);

  CVideoSettings GetVideoSettings() const;
  void SetVideoSettings(CVideoSettings& settings);

  CSeekHandler& GetSeekHandler();
  const CSeekHandler& GetSeekHandler() const;

private:
  boost::shared_ptr<const IPlayer> GetInternal() const;
  boost::shared_ptr<IPlayer> GetInternal();
  void CreatePlayer(const CPlayerCoreFactory &factory, const std::string &player, IPlayerCallback& callback);
  void CloseFile(bool reopen = false);

  boost::shared_ptr<IPlayer> m_pPlayer;
  mutable CCriticalSection m_playerLock;
  CSeekHandler m_seekHandler;

  // cache player state
  XbmcThreads::EndTime m_audioStreamUpdate;
  int m_iAudioStream;
  XbmcThreads::EndTime m_subtitleStreamUpdate;
  int m_iSubtitleStream;

  struct SNextItem
  {
    SNextItem() : callback(NULL) {}
    boost::shared_ptr<CFileItem> pItem;
    CPlayerOptions options;
    std::string playerName;
    IPlayerCallback *callback;
  } m_nextItem;

  int m_iPlaySpeed;
};
