/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayer.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/IPlayer.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StdString.h"

#include <boost/make_shared.hpp>

boost::shared_ptr<const IPlayer> CApplicationPlayer::GetInternal() const
{
  CSingleLock lock(m_playerLock);
  return m_pPlayer;
}

boost::shared_ptr<IPlayer> CApplicationPlayer::GetInternal()
{
  CSingleLock lock(m_playerLock);
  return m_pPlayer;
}

void CApplicationPlayer::ClosePlayer()
{
  m_nextItem.pItem.reset();
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CloseFile();
    ResetPlayer();
  }
}

void CApplicationPlayer::ResetPlayer()
{
  // we need to do this directly on the member
  CSingleLock lock(m_playerLock);
  m_pPlayer.reset();
}

void CApplicationPlayer::CloseFile(bool reopen)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->CloseFile(reopen);
  }
}

void CApplicationPlayer::CreatePlayer(const CPlayerCoreFactory &factory, const std::string &player, IPlayerCallback& callback)
{
  CSingleLock lock(m_playerLock);
  if (!m_pPlayer)
  {
    m_pPlayer = factory.CreatePlayer(player, callback);
  }
}

std::string CApplicationPlayer::GetCurrentPlayer() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->m_name;
  }
  return "";
}

bool CApplicationPlayer::OpenFile(const CFileItem& item, const CPlayerOptions& options,
                                  const CPlayerCoreFactory &factory,
                                  const std::string &playerName, IPlayerCallback& callback)
{
  // get player type
  std::string newPlayer;
  if (!playerName.empty())
    newPlayer = playerName;
  else
    newPlayer = factory.GetDefaultPlayer(item);

  // check if we need to close current player
  // VideoPlayer can open a new file while playing
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player && player->IsPlaying())
  {
    bool needToClose = false;

    if (item.IsDiscImage() || item.IsDVDFile())
      needToClose = true;

    if (player->m_name != newPlayer)
      needToClose = true;

    if (player->m_type != "video" && player->m_type != "remote")
      needToClose = true;

    if (needToClose)
    {
      m_nextItem.pItem = boost::make_shared<CFileItem>(item);
      m_nextItem.options = options;
      m_nextItem.playerName = newPlayer;
      m_nextItem.callback = &callback;

      CloseFile();
      if (player->m_name != newPlayer)
      {
        CSingleLock lock(m_playerLock);
        m_pPlayer.reset();
      }
      return true;
    }
  }
  else if (player && player->m_name != newPlayer)
  {
    CloseFile();
    {
      CSingleLock lock(m_playerLock);
      m_pPlayer.reset();
      player.reset();
    }
  }

  if (!player)
  {
    CreatePlayer(factory, newPlayer, callback);
    player = GetInternal();
    if (!player)
      return false;
  }

  bool ret = player->OpenFile(item, options);

  m_nextItem.pItem.reset();

  // reset caching timers
  m_audioStreamUpdate.SetExpired();
  m_subtitleStreamUpdate.SetExpired();

  return ret;
}

void CApplicationPlayer::OpenNext(const CPlayerCoreFactory &factory)
{
  if (m_nextItem.pItem)
  {
    OpenFile(*m_nextItem.pItem, m_nextItem.options,
             factory,
             m_nextItem.playerName, *m_nextItem.callback);
    m_nextItem.pItem.reset();
  }
}

bool CApplicationPlayer::HasPlayer() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return player != NULL;
}

int CApplicationPlayer::GetChapter() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapter();
  else
    return -1;
}

int CApplicationPlayer::GetChapterCount() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapterCount();
  else
    return 0;
}

void CApplicationPlayer::GetChapterName(std::string& strChapterName, int chapterIdx) const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetChapterName(strChapterName);
}

int64_t CApplicationPlayer::GetChapterPos(int chapterIdx) const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapterPos(chapterIdx);

  return -1;
}

bool CApplicationPlayer::HasAudio() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasAudio());
}

bool CApplicationPlayer::HasVideo() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasVideo());
}

PLAYLIST::Id CApplicationPlayer::GetPreferredPlaylist() const
{
  if (IsPlayingVideo())
    return PLAYLIST::TYPE_VIDEO;

  if (IsPlayingAudio())
    return PLAYLIST::TYPE_MUSIC;

  return PLAYLIST::TYPE_NONE;
}

bool CApplicationPlayer::IsPaused() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsPaused());
}

bool CApplicationPlayer::IsPlaying() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsPlaying());
}

bool CApplicationPlayer::IsPausedPlayback() const
{
  return (IsPlaying() && IsPaused());
}

bool CApplicationPlayer::IsPlayingAudio() const
{
  return (IsPlaying() && !HasVideo() && HasAudio());
}

bool CApplicationPlayer::IsPlayingVideo() const
{
  return (IsPlaying() && HasVideo());
}

void CApplicationPlayer::Pause()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->Pause();
  }
}

void CApplicationPlayer::SetMute(bool bOnOff)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetMute(bOnOff);
}

void CApplicationPlayer::SetVolume(int volume)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetVolume(volume);
}

void CApplicationPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->Seek(bPlus, bLargeStep, bChapterOverride);
}

void CApplicationPlayer::SeekPercentage(float fPercent)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekPercentage(fPercent);
}

bool CApplicationPlayer::CanSeek() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->CanSeek());
}

bool CApplicationPlayer::SeekScene(bool bPlus)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SeekScene(bPlus));
}

void CApplicationPlayer::SeekTime(int64_t iTime)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekTime(iTime);
}

void CApplicationPlayer::SeekTimeRelative(int64_t iTime)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    // use relative seeking if implemented by player
    if (!player->SeekTimeRelative(iTime))
    {
      int64_t abstime = GetTime() + iTime;
      player->SeekTime(abstime);
    }
  }
}

int64_t CApplicationPlayer::GetTime() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetTime();
  else
    return 0;
}

int64_t CApplicationPlayer::GetTotalTime() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->GetTotalTime();
  }
  else
    return 0;
}

bool CApplicationPlayer::IsCaching() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsCaching());
}

bool CApplicationPlayer::IsInMenu() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsInMenu());
}

int CApplicationPlayer::GetCacheLevel() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetCacheLevel();
  else
    return 0;
}

int CApplicationPlayer::GetSubtitleCount()
{
  const boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetSubtitleCount();
  else
    return 0;
}

int CApplicationPlayer::GetAudioStream()
{
  if (!m_audioStreamUpdate.IsTimePast())
    return m_iAudioStream;

  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iAudioStream = player->GetAudioStream();
    m_audioStreamUpdate.Set(1000);
    return m_iAudioStream;
  }
  else
    return 0;
}

int CApplicationPlayer::GetSubtitle()
{
  if (!m_subtitleStreamUpdate.IsTimePast())
    return m_iSubtitleStream;

  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iSubtitleStream = player->GetSubtitle();
    m_subtitleStreamUpdate.Set(1000);
    return m_iSubtitleStream;
  }
  else
    return 0;
}

bool CApplicationPlayer::GetSubtitleVisible()
{
  const boost::shared_ptr<IPlayer> player = GetInternal();
  return player && player->GetSubtitleVisible();
}

bool CApplicationPlayer::CanPause() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->CanPause());
}

float CApplicationPlayer::GetPercentage() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->GetPercentage();
  }
  else
    return 0.0;
}

float CApplicationPlayer::GetCachePercentage() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetCachePercentage();
  else
    return 0.0;
}

std::string CApplicationPlayer::GetPlayerState()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetPlayerState();
  else
    return "";
}

bool CApplicationPlayer::QueueNextFile(const CFileItem &file)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->QueueNextFile(file));
}

bool CApplicationPlayer::SetPlayerState(const std::string& state)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SetPlayerState(state));
}

void CApplicationPlayer::OnNothingToQueueNotify()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OnNothingToQueueNotify();
}

void CApplicationPlayer::GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo& info)
{
  const boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    RECT SrcRect, DestRect;
    player->GetVideoRect(SrcRect, DestRect);

    info.valid = true;
    info.videoCodecName = player->GetVideoCodecName();
    info.width = player->GetPictureWidth();
    info.height = player->GetPictureHeight();
    player->GetVideoAspectRatio(info.videoAspectRatio);
    info.stereoMode = "";
    info.SrcRect = CRect(static_cast<float>(SrcRect.left), static_cast<float>(SrcRect.top), static_cast<float>(SrcRect.right), static_cast<float>(SrcRect.bottom));
    info.DestRect = CRect(static_cast<float>(DestRect.left), static_cast<float>(DestRect.top), static_cast<float>(DestRect.right), static_cast<float>(DestRect.bottom));
  }
}

void CApplicationPlayer::GetAudioStreamInfo(int index, SPlayerAudioStreamInfo& info)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString strName, strLanguage;
    player->GetAudioStreamName(index, strName);
    player->GetAudioStreamLanguage(index, strLanguage);

    info.valid = true;
    info.bitrate = player->GetAudioBitrate();
    info.channels = player->GetChannels();
    info.samplerate = player->GetSampleRate();
    info.bitspersample = player->GetBitsPerSample();
    info.name = strName;
    info.language = strLanguage;
  }
}

bool CApplicationPlayer::OnAction(const CAction &action)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->OnAction(action));
}

int CApplicationPlayer::GetAudioStreamCount()
{
  const boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetAudioStreamCount();
  else
    return 0;
}

void CApplicationPlayer::SetAudioStream(int iStream)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetAudioStream(iStream);
    m_iAudioStream = iStream;
    m_audioStreamUpdate.Set(1000);
  }
}

void CApplicationPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo& info)
{
  const boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString strName, strLanguage;
    player->GetSubtitleName(index, strName);
    player->GetSubtitleLanguage(index, strLanguage);

    info.name = strName;
    info.language = strLanguage;
  }
}

void CApplicationPlayer::SetSubtitle(int iStream)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitle(iStream);
    m_iSubtitleStream = iStream;
    m_subtitleStreamUpdate.Set(1000);
  }
}

void CApplicationPlayer::SetSubtitleVisible(bool bVisible)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitleVisible(bVisible);
  }
}

int CApplicationPlayer::AddSubtitle(const std::string& strSubPath)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->AddSubtitle(strSubPath);
  else
    return 0;
}

void CApplicationPlayer::SetSubTitleDelay(float fValue)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetSubTitleDelay(fValue);
}

void CApplicationPlayer::SetAVDelay(float fValue)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetAVDelay(fValue);
}

void CApplicationPlayer::SetDynamicRangeCompression(long drc)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetDynamicRangeCompression(drc);
}

int  CApplicationPlayer::SeekChapter(int iChapter)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->SeekChapter(iChapter);
  else
    return 0;
}

void CApplicationPlayer::SetPlaySpeed(int speed)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (!player)
    return;

  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;

  if (m_iPlaySpeed == speed)
    return ;
  if (!CanSeek())
    return;
  if (IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (speed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (speed < m_iPlaySpeed))
    )
    {
      speed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    Pause();
  }
  m_iPlaySpeed = speed;

  player->ToFFRW(m_iPlaySpeed);

  const boost::shared_ptr<CApplicationVolumeHandling> appVolume = CServiceBroker::GetAppComponents().GetComponent<CApplicationVolumeHandling>();

  if (m_iPlaySpeed == 1)
  { // restore volume
    player->SetVolume(appVolume->GetVolumeRatio());
  }
  else
  { // mute volume
    player->SetVolume(CApplicationVolumeHandling::VOLUME_MINIMUM);
  }
  player->SetMute(appVolume->IsMuted());
}

int CApplicationPlayer::GetPlaySpeed() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return m_iPlaySpeed;
  }
  else
    return 0;
}

void CApplicationPlayer::SetRenderViewMode(int mode)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    g_renderManager.SetViewMode(mode);
}

float CApplicationPlayer::GetRenderAspectRatio() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return g_renderManager.GetAspectRatio();
  else
    return 1.0;
}

bool CApplicationPlayer::IsRenderingVideo() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return g_renderManager.IsStarted();
  else
    return false;
}

std::string CApplicationPlayer::GetName() const
{
  const boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->m_name;
  }
  return std::string();
}

CVideoSettings CApplicationPlayer::GetVideoSettings() const
{
  boost::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return CMediaSettings::GetInstance().GetCurrentVideoSettings();
  }
  return CVideoSettings();
}

void CApplicationPlayer::SetVideoSettings(CVideoSettings& settings)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CMediaSettings::GetInstance().GetCurrentVideoSettings() = settings;
  }
}

CSeekHandler& CApplicationPlayer::GetSeekHandler()
{
  return m_seekHandler;
}

const CSeekHandler& CApplicationPlayer::GetSeekHandler() const
{
  return m_seekHandler;
}

int CApplicationPlayer::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  const boost::shared_ptr<CAdvancedSettings> &advSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const float delay = this->GetVideoSettings().m_SubtitleDelay;
  const float range = advSettings->m_videoSubsDelayRange;
  return static_cast<int>(0.5f + (delay + range) / (2.f * range) * 100.0f);
}

int CApplicationPlayer::GetAudioDelay() const
{
  // converts audio delay to a percentage
  const boost::shared_ptr<CAdvancedSettings> &advSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const float delay = this->GetVideoSettings().m_AudioDelay;
  const float range = advSettings->m_videoAudioDelayRange;
  return static_cast<int>(0.5f + (delay + range) / (2.f * range) * 100.0f);
}

bool CApplicationPlayer::CanRecord()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->CanRecord();
  return false;
}

bool CApplicationPlayer::IsRecording()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->IsRecording();
  return false;
}

bool CApplicationPlayer::Record(bool bOnOff)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->Record(bOnOff);
  return false;
}

bool CApplicationPlayer::HasMenu()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->HasMenu();
  return false;
}

void CApplicationPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->RegisterAudioCallback(pCallback);
}

void CApplicationPlayer::UnRegisterAudioCallback()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->UnRegisterAudioCallback();
}

void CApplicationPlayer::GetAudioInfo(std::string& strAudioInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString temp;
    player->GetAudioInfo(temp);
    strAudioInfo = temp;
  }
}

void CApplicationPlayer::GetVideoInfo(std::string& strVideoInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString temp;
    player->GetVideoInfo(temp);
    strVideoInfo = temp;
  }
}

void CApplicationPlayer::GetGeneralInfo(std::string& strVideoInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString temp;
    player->GetGeneralInfo(temp);
    strVideoInfo = temp;
  }
}

bool CApplicationPlayer::GetCurrentSubtitle(std::string& strSubtitle)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CStdString temp;
    bool ret = player->GetCurrentSubtitle(temp);
    strSubtitle = temp;
    return ret;
  }
  return false;
}

float CApplicationPlayer::GetActualFPS()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetActualFPS();

  return 0.0f;
}

bool CApplicationPlayer::GetSubtitleExtension(std::string& strSubtitleExtension)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->GetSubtitleExtension(strSubtitleExtension);
    return true;
  }

  return false;
}

bool CApplicationPlayer::SkipNext()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->SkipNext();

  return false;
}
