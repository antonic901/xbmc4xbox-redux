/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/PlayerGUIInfo.h"

#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/MediaSettings.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/SeekHandler.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <boost/move/make_unique.hpp>

using namespace KODI::GUILIB::GUIINFO;

CPlayerGUIInfo::CPlayerGUIInfo()
  : m_playerShowTime(false),
    m_playerShowInfo(false),
    m_seekOffset(0),
    m_AfterSeekTimeout(0)
{
}

CPlayerGUIInfo::~CPlayerGUIInfo() {}

int CPlayerGUIInfo::GetTotalPlayTime() const
{
  return MathUtils::round_int(g_application.GetTotalTime());
}

int CPlayerGUIInfo::GetPlayTime() const
{
  int64_t lPTS = static_cast<int64_t>(g_application.GetTime() * 1000);
  return lPTS < 0 ? 0 : lPTS;
}

int CPlayerGUIInfo::GetPlayTimeRemaining() const
{
  int iReverse = GetTotalPlayTime() - MathUtils::round_int(g_application.GetTime());
  return iReverse > 0 ? iReverse : 0;
}

float CPlayerGUIInfo::GetSeekPercent() const
{
  int iTotal = GetTotalPlayTime();
  if (iTotal == 0)
    return 0.0f;

  float fPercentPlayTime = static_cast<float>(GetPlayTime()) / iTotal * 0.1f;
  float fPercentPerSecond = 100.0f / static_cast<float>(iTotal);
  float fPercent =
      fPercentPlayTime + fPercentPerSecond * CSeekHandler::GetInstance().GetSeekSize();
  fPercent = std::max(0.0f, std::min(fPercent, 100.0f));
  return fPercent;
}

std::string CPlayerGUIInfo::GetCurrentPlayTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;

  return StringUtils::SecondsToTimeString(MathUtils::round_int(GetPlayTime() / 1000.0), format);
}

std::string CPlayerGUIInfo::GetCurrentPlayTimeRemaining(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;

  int iTimeRemaining = GetPlayTimeRemaining();
  if (iTimeRemaining)
    return StringUtils::SecondsToTimeString(iTimeRemaining, format);

  return std::string();
}

std::string CPlayerGUIInfo::GetDuration(TIME_FORMAT format) const
{
  int iTotal = GetTotalPlayTime();
  if (iTotal > 0)
  {
    if (format == TIME_FORMAT_GUESS && iTotal >= 3600)
      format = TIME_FORMAT_HH_MM_SS;
    return StringUtils::SecondsToTimeString(iTotal, format);
  }
  return std::string();
}

std::string CPlayerGUIInfo::GetCurrentSeekTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;

  return StringUtils::SecondsToTimeString(
      g_application.GetTime() + CSeekHandler::GetInstance().GetSeekSize(), format);
}

std::string CPlayerGUIInfo::GetSeekTime(TIME_FORMAT format) const
{
  if (!CSeekHandler::GetInstance().HasTimeCode())
    return std::string();

  int iSeekTimeCode = CSeekHandler::GetInstance().GetTimeCodeSeconds();
  if (format == TIME_FORMAT_GUESS && iSeekTimeCode >= 3600)
    format = TIME_FORMAT_HH_MM_SS;

  return StringUtils::SecondsToTimeString(iSeekTimeCode, format);
}

void CPlayerGUIInfo::SetDisplayAfterSeek(unsigned int timeOut, int seekOffset)
{
  if (timeOut>0)
  {
    m_AfterSeekTimeout = CTimeUtils::GetFrameTime() +  timeOut;
    if (seekOffset)
      m_seekOffset = seekOffset;
  }
  else
    m_AfterSeekTimeout = 0;
}

bool CPlayerGUIInfo::GetDisplayAfterSeek()
{
  if (CTimeUtils::GetFrameTime() < m_AfterSeekTimeout)
    return true;
  m_seekOffset = 0;
  return false;
}

void CPlayerGUIInfo::SetShowInfo(bool showinfo)
{
  if (showinfo != m_playerShowInfo)
  {
    m_playerShowInfo = showinfo;
    m_events.Publish(PlayerShowInfoChangedEvent(m_playerShowInfo));
  }
}

bool CPlayerGUIInfo::ToggleShowInfo()
{
  SetShowInfo(!m_playerShowInfo);
  return m_playerShowInfo;
}

bool CPlayerGUIInfo::InitCurrentItem(CFileItem *item)
{
  if (item && g_application.m_pPlayer->IsPlaying())
  {
    CLog::Log(LOGDEBUG, "CPlayerGUIInfo::InitCurrentItem(%s)", CURL::GetRedacted(item->GetPath()).c_str());
    m_currentItem = boost::movelib::make_unique<CFileItem>(*item);
  }
  else
  {
    m_currentItem.reset();
  }
  return false;
}

bool CPlayerGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // PLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case PLAYER_SEEKOFFSET:
    {
      std::string seekOffset = StringUtils::SecondsToTimeString(abs(m_seekOffset / 1000), (TIME_FORMAT)info.GetData1());
      if (m_seekOffset < 0)
        value = "-" + seekOffset;
      if (m_seekOffset > 0)
        value = "+" + seekOffset;
      return true;
    }
    case PLAYER_PROGRESS:
      value = StringUtils::Format("%i", MathUtils::round_int(g_application.GetPercentage()));
      return true;
    case PLAYER_PROGRESS_CACHE:
      value = StringUtils::Format("%i", MathUtils::round_int(g_application.GetCachePercentage()));
      return true;
    case PLAYER_VOLUME:
      value =
          StringUtils::Format("%2.1f dB", (float)(g_application.GetVolume(false) + g_application.GetDynamicRangeCompressionLevel()) * 0.01f);
      return true;
    case PLAYER_SUBTITLE_DELAY:
      value = StringUtils::Format("%2.3f s", CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay);
      return true;
    case PLAYER_AUDIO_DELAY:
      value = StringUtils::Format("%2.3f s", CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay);
      return true;
    case PLAYER_CHAPTER:
      value = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapter());
      return true;
    case PLAYER_CHAPTERCOUNT:
      value = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapterCount());
      return true;
    case PLAYER_CHAPTERNAME:
      g_application.m_pPlayer->GetChapterName(value);
      return true;
    case PLAYER_PATH:
    case PLAYER_FILENAME:
    case PLAYER_FILEPATH:
      value = GUIINFO::GetFileInfoLabelValueFromPath(info.m_info, item->GetPath());
      return true;
    case PLAYER_TITLE:
      // use label or drop down to title from path
      value = item->GetLabel();
      if (value.empty())
        value = CUtil::GetTitleFromPath(item->GetPath());
      return true;
    case PLAYER_PLAYSPEED:
    {
      value = StringUtils::Format("%i", g_application.m_pPlayer->GetPlaySpeed());
      return true;
    }
    case PLAYER_TIME:
      value = GetCurrentPlayTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PLAYER_START_TIME:
    {
      CDateTime time = CDateTime::GetCurrentDateTime();
      time -= CDateTimeSpan(0, 0, 0, (int)GetPlayTime());
      value = time.GetAsLocalizedTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PLAYER_DURATION:
      value = GetDuration(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PLAYER_TIME_REMAINING:
      value = GetCurrentPlayTimeRemaining(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PLAYER_FINISH_TIME:
    {
      CDateTime time(CDateTime::GetCurrentDateTime());
      int playTimeRemaining = GetPlayTimeRemaining();
      time += CDateTimeSpan(0, 0, 0, playTimeRemaining);
      value = time.GetAsLocalizedTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    }
    case PLAYER_TIME_SPEED:
    {
      float speed = g_application.m_pPlayer->GetPlaySpeed();
      if (speed != 1.0f)
        value = StringUtils::Format("%s (%ix)",
                                    GetCurrentPlayTime(static_cast<TIME_FORMAT>(info.GetData1())).c_str(),
                                    static_cast<int>(speed));
      else
        value = GetCurrentPlayTime(TIME_FORMAT_GUESS);
      return true;
    }
    case PLAYER_SEEKTIME:
      value = GetCurrentSeekTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case PLAYER_SEEKSTEPSIZE:
    {
      int seekSize = CSeekHandler::GetInstance().GetSeekSize();
      std::string strSeekSize = StringUtils::SecondsToTimeString(abs(seekSize), static_cast<TIME_FORMAT>(info.GetData1()));
      if (seekSize < 0)
        value = "-" + strSeekSize;
      if (seekSize > 0)
        value = "+" + strSeekSize;
      return true;
    }
    case PLAYER_SEEKNUMERIC:
      value = GetSeekTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return !value.empty();
    case PLAYER_CACHELEVEL:
    {
      int iLevel = g_application.m_pPlayer->GetCacheLevel();
      if (iLevel >= 0)
      {
        value = std::to_string(iLevel);
        return true;
      }
      break;
    }
    case PLAYER_ITEM_ART:
      value = item->GetArt(info.GetData3());
      return true;
    case PLAYER_ICON:
      value = item->GetArt("thumb");
      if (value.empty())
        value = item->GetArt("icon");
      if (fallback)
        *fallback = item->GetArt("icon");
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // PLAYLIST_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case PLAYLIST_LENGTH:
    case PLAYLIST_POSITION:
    case PLAYLIST_RANDOM:
    case PLAYLIST_REPEAT:
      value = GUIINFO::GetPlaylistLabel(info.m_info, info.GetData1());
      return true;
  }

  return false;
}

bool CPlayerGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // PLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case PLAYER_VOLUME:
      value = g_application.GetVolume();
      return true;
    case PLAYER_PROGRESS:
      value = MathUtils::round_int(g_application.GetPercentage());
      return true;
    case PLAYER_PROGRESS_CACHE:
      value = MathUtils::round_int(g_application.GetCachePercentage());
      return true;
    case PLAYER_SEEKBAR:
      value = MathUtils::round_int(GetSeekPercent());
      return true;
    case PLAYER_CACHELEVEL:
      value = g_application.m_pPlayer->GetCacheLevel();
      return true;
    case PLAYER_CHAPTER:
      value = g_application.m_pPlayer->GetChapter();
      return true;
    case PLAYER_CHAPTERCOUNT:
      value = g_application.m_pPlayer->GetChapterCount();
      return true;
    case PLAYER_SUBTITLE_DELAY:
      value = g_application.GetSubtitleDelay();
      return true;
    case PLAYER_AUDIO_DELAY:
      value = g_application.GetAudioDelay();
      return true;
  }

  return false;
}

bool CPlayerGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  const CFileItem *item = nullptr;
  if (gitem->IsFileItem())
    item = static_cast<const CFileItem*>(gitem);

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // PLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case PLAYER_SHOWINFO:
      value = m_playerShowInfo;
      return true;
    case PLAYER_SHOWTIME:
      value = m_playerShowTime;
      return true;
    case PLAYER_MUTED:
      value = (g_application.IsMuted() || g_application.GetVolume(false) <= VOLUME_MINIMUM);
      return true;
    case PLAYER_HAS_MEDIA:
      value = g_application.m_pPlayer->IsPlaying();
      return true;
    case PLAYER_HAS_AUDIO:
      value = g_application.m_pPlayer->IsPlayingAudio();
      return true;
    case PLAYER_HAS_VIDEO:
      value = g_application.m_pPlayer->IsPlayingVideo();
      return true;
    case PLAYER_IS_EXTERNAL:
      value = g_application.m_pPlayer->IsExternalPlaying();
      return true;
    case PLAYER_PLAYING:
      value = g_application.m_pPlayer->GetPlaySpeed() == 1.0f;
      return true;
    case PLAYER_PAUSED:
      value = g_application.m_pPlayer->IsPausedPlayback();
      return true;
    case PLAYER_REWINDING:
      value = g_application.m_pPlayer->GetPlaySpeed() < 0.0f;
      return true;
    case PLAYER_FORWARDING:
      value = g_application.m_pPlayer->GetPlaySpeed() > 1.5f;
      return true;
    case PLAYER_REWINDING_2x:
      value = g_application.m_pPlayer->GetPlaySpeed() == -2;
      return true;
    case PLAYER_REWINDING_4x:
      value = g_application.m_pPlayer->GetPlaySpeed() == -4;
      return true;
    case PLAYER_REWINDING_8x:
      value = g_application.m_pPlayer->GetPlaySpeed() == -8;
      return true;
    case PLAYER_REWINDING_16x:
      value = g_application.m_pPlayer->GetPlaySpeed() == -16;
      return true;
    case PLAYER_REWINDING_32x:
      value = g_application.m_pPlayer->GetPlaySpeed() == -32;
      return true;
    case PLAYER_FORWARDING_2x:
      value = g_application.m_pPlayer->GetPlaySpeed() == 2;
      return true;
    case PLAYER_FORWARDING_4x:
      value = g_application.m_pPlayer->GetPlaySpeed() == 4;
      return true;
    case PLAYER_FORWARDING_8x:
      value = g_application.m_pPlayer->GetPlaySpeed() == 8;
      return true;
    case PLAYER_FORWARDING_16x:
      value = g_application.m_pPlayer->GetPlaySpeed() == 16;
      return true;
    case PLAYER_FORWARDING_32x:
      value = g_application.m_pPlayer->GetPlaySpeed() == 32;
      return true;
    case PLAYER_CAN_PAUSE:
      value = g_application.m_pPlayer->CanPause();
      return true;
    case PLAYER_CAN_SEEK:
      value = g_application.m_pPlayer->CanSeek();
      return true;
    case PLAYER_SUPPORTS_TEMPO:
      value = g_application.m_pPlayer->SupportsTempo();
      return true;
    case PLAYER_IS_TEMPO:
    {
      float speed = (float)g_application.m_pPlayer->GetPlaySpeed();
      value = (speed >= 0.75 && speed <= 1.55 && speed != 1);
      return true;
    }
    case PLAYER_CACHING:
      value = g_application.m_pPlayer->IsCaching();
      return true;
    case PLAYER_SEEKBAR:
    {
      CGUIDialog *seekBar = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_SEEK_BAR);
      value = seekBar ? seekBar->IsDialogRunning() : false;
      return true;
    }
    case PLAYER_SEEKING:
      value = CSeekHandler::GetInstance().InProgress();
      return true;
    case PLAYER_PASSTHROUGH:
      value = g_application.m_pPlayer->IsPassthrough();
      return true;
    case PLAYER_ISINTERNETSTREAM:
      if (item)
      {
        value = URIUtils::IsInternetStream(item->GetPath());
        return true;
      }
      break;
    case PLAYER_HASDURATION:
      value = g_application.GetTotalTime() > 0;
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // PLAYLIST_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case PLAYLIST_ISRANDOM:
    {
      PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();
      PLAYLIST::Id playlistid = info.GetData1();
      if (info.GetData2() > 0 && playlistid != PLAYLIST::TYPE_NONE)
        value = player.IsShuffled(playlistid);
      else
        value = player.IsShuffled(player.GetCurrentPlaylist());
      return true;
    }
    case PLAYLIST_ISREPEAT:
    {
      PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();
      PLAYLIST::Id playlistid = info.GetData1();
      if (info.GetData2() > 0 && playlistid != PLAYLIST::TYPE_NONE)
        value = (player.GetRepeat(playlistid) == PLAYLIST::ALL);
      else
        value = player.GetRepeat(player.GetCurrentPlaylist()) == PLAYLIST::ALL;
      return true;
    }
    case PLAYLIST_ISREPEATONE:
    {
      PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();
      PLAYLIST::Id playlistid = info.GetData1();
      if (info.GetData2() > 0 && playlistid != PLAYLIST::TYPE_NONE)
        value = (player.GetRepeat(playlistid) == PLAYLIST::ONE);
      else
        value = player.GetRepeat(player.GetCurrentPlaylist()) == PLAYLIST::ONE;
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_ISPLAYING:
    {
      if (item)
      {
        if (item->HasProperty("playlistposition"))
        {
          value = static_cast<int>(item->GetProperty("playlisttype").asInteger()) ==
                      CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() &&
                  static_cast<int>(item->GetProperty("playlistposition").asInteger()) ==
                      CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
          return true;
        }
        else if (m_currentItem && !m_currentItem->GetPath().empty())
        {
          if (!g_application.m_strPlayListFile.empty())
          {
            //playlist file that is currently playing or the playlistitem that is currently playing.
            value = item->IsPath(g_application.m_strPlayListFile) || m_currentItem->IsSamePath(item);
          }
          else
          {
            value = m_currentItem->IsSamePath(item);
          }
          return true;
        }
      }
      break;
    }
  }

  return false;
}
