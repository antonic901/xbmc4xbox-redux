/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Player.h"

#include "AddonUtils.h"
#include "application/Application.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ListItem.h"
#include "PlayList.h"
#include "PlayListPlayer.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/IPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/MediaSettings.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"

#include <boost/make_shared.hpp>

using namespace KODI::MESSAGING;

namespace
{

boost::shared_ptr<const CApplicationPlayer> getAppPlayer()
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  boost::shared_ptr<const CApplicationPlayer> res = components.GetComponent<CApplicationPlayer>();
  return res;
}

boost::shared_ptr<CApplicationPlayer> getAppPlayerMut()
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  boost::shared_ptr<CApplicationPlayer> res = components.GetComponent<CApplicationPlayer>();
  return res;
}

} // namespace

namespace XBMCAddon
{
  namespace xbmc
  {
    PlayParameter Player::defaultPlayParameter;

    Player::Player(int _playerCore)
    {
      iPlayList = PLAYLIST::TYPE_MUSIC;

      // now that we're done, register hook me into the system
      if (languageHook)
      {
        DelayedCallGuard dc(languageHook);
        languageHook->RegisterPlayerCallback(this);
      }
    }

    Player::~Player()
    {
      deallocating();

      // we're shutting down so unregister me.
      if (languageHook)
      {
        DelayedCallGuard dc(languageHook);
        languageHook->UnregisterPlayerCallback(this);
      }
    }

    void Player::play(const Alternative<String, const PlayList* > & item,
                      const XBMCAddon::xbmcgui::ListItem* listitem, bool windowed, int startpos)
    {
      XBMC_TRACE;

      if (&item == &defaultPlayParameter)
        playCurrent(windowed);
      else if (item.which() == XBMCAddon::first)
        playStream(item.former(), listitem, windowed);
      else // item is a PlayListItem
        playPlaylist(item.later(),windowed,startpos);
    }

    void Player::playStream(const String& item, const xbmcgui::ListItem* plistitem, bool windowed)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      if (!item.empty())
      {
        // set fullscreen or windowed
        CMediaSettings::GetInstance().SetMediaStartWindowed(windowed);

        const AddonClass::Ref<xbmcgui::ListItem> listitem(plistitem);

        if (listitem.isSet())
        {
          // set m_strPath to the passed url
          listitem->item->SetPath(item.c_str());
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*listitem->item)));
        }
        else
        {
          CFileItemList *l = new CFileItemList; //don't delete,
          l->Add(boost::make_shared<CFileItem>(item, false));
          CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
        }
      }
      else
        playCurrent(windowed);
    }

    void Player::playCurrent(bool windowed)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      // set fullscreen or windowed
      CMediaSettings::GetInstance().SetMediaStartWindowed(windowed);

      // play current file in playlist
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != iPlayList)
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PLAY, CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx());
    }

    void Player::playPlaylist(const PlayList* playlist, bool windowed, int startpos)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);
      if (playlist != NULL)
      {
        // set fullscreen or windowed
        CMediaSettings::GetInstance().SetMediaStartWindowed(windowed);

        // play a python playlist (a playlist from playlistplayer.cpp)
        iPlayList = playlist->getPlayListId();
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
        if (startpos > -1)
          CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(startpos);
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PLAY, startpos);
      }
      else
        playCurrent(windowed);
    }

    void Player::stop()
    {
      XBMC_TRACE;
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
    }

    void Player::pause()
    {
      XBMC_TRACE;
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
    }

    void Player::playnext()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_NEXT);
    }

    void Player::playprevious()
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PREV);
    }

    void Player::playselected(int selected)
    {
      XBMC_TRACE;
      DelayedCallGuard dc(languageHook);

      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != iPlayList)
      {
        CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlayList);
      }
      CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(selected);

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PLAY, selected);
      //CServiceBroker::GetPlaylistPlayer().Play(selected);
      //CLog::Log(LOGNOTICE, "Current Song After Play: %i", CServiceBroker::GetPlaylistPlayer().GetCurrentSong());
    }

    void Player::OnPlayBackStarted(const CFileItem &file)
    {
      // We only have fileItem due to us having to
      // implement the interface, we can't send it to python
      // as we're not able to serialize it.
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this, &Player::onPlayBackStarted));
    }

    void Player::OnPlayBackEnded()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackEnded));
    }

    void Player::OnPlayBackStopped()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackStopped));
    }

    void Player::OnPlayBackError()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackError));
    }

    void Player::OnPlayBackPaused()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackPaused));
    }

    void Player::OnPlayBackResumed()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onPlayBackResumed));
    }

    void Player::OnQueueNextItem()
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player>(this,&Player::onQueueNextItem));
    }

    void Player::OnPlayBackSpeedChanged(int speed)
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSpeedChanged,speed));
    }

    void Player::OnPlayBackSeek(int64_t time, int64_t seekOffset)
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int,int>(this,&Player::onPlayBackSeek,static_cast<int>(time),static_cast<int>(seekOffset)));
    }

    void Player::OnPlayBackSeekChapter(int chapter)
    {
      XBMC_TRACE;
      invokeCallback(new CallbackFunction<Player,int>(this,&Player::onPlayBackSeekChapter,chapter));
    }

    void Player::onPlayBackStarted()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackEnded()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackStopped()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackError()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackPaused()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackResumed()
    {
      XBMC_TRACE;
    }

    void Player::onQueueNextItem()
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackSpeedChanged(int speed)
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackSeek(int time, int seekOffset)
    {
      XBMC_TRACE;
    }

    void Player::onPlayBackSeekChapter(int chapter)
    {
      XBMC_TRACE;
    }

    bool Player::isPlaying()
    {
      XBMC_TRACE;
      return getAppPlayer()->IsPlaying();
    }

    bool Player::isPlayingAudio()
    {
      XBMC_TRACE;
      return getAppPlayer()->IsPlayingAudio();
    }

    bool Player::isPlayingVideo()
    {
      XBMC_TRACE;
      return getAppPlayer()->IsPlayingVideo();
    }

    bool Player::isPlayingRDS()
    {
      XBMC_TRACE;
      return false;
    }

    bool Player::isExternalPlayer()
    {
      XBMC_TRACE;
      return false;
    }

    String Player::getPlayingFile()
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlaying())
        throw PlayerException("XBMC is not playing any file");

      return g_application.CurrentFile();
    }

    InfoTagVideo* Player::getVideoInfoTag()
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlayingVideo())
        throw PlayerException("XBMC is not playing any videofile");

      const CVideoInfoTag* movie = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentMovieTag();
      if (movie)
        return new InfoTagVideo(*movie);

      return new InfoTagVideo();
    }

    InfoTagMusic* Player::getMusicInfoTag()
    {
      XBMC_TRACE;
      if (getAppPlayer()->IsPlayingVideo() || !getAppPlayer()->IsPlayingAudio())
        throw PlayerException("XBMC is not playing any music file");

      const MUSIC_INFO::CMusicInfoTag* tag = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
      if (tag)
        return new InfoTagMusic(*tag);

      return new InfoTagMusic();
    }

    void Player::updateInfoTag(const XBMCAddon::xbmcgui::ListItem* item)
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlaying())
        throw PlayerException("Kodi is not playing any file");

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, item->item);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }

    double Player::getTotalTime()
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTotalTime();
    }

    double Player::getTime()
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      return g_application.GetTime();
    }

    void Player::seekTime(double pTime)
    {
      XBMC_TRACE;
      if (!getAppPlayer()->IsPlaying())
        throw PlayerException("XBMC is not playing any media file");

      g_application.SeekTime( pTime );
    }

    void Player::setSubtitles(const char* cLine)
    {
      XBMC_TRACE;
      if (getAppPlayer()->HasPlayer())
      {
        int nStream = getAppPlayerMut()->AddSubtitle(cLine);
        if(nStream >= 0)
        {
          getAppPlayerMut()->SetSubtitle(nStream);
          getAppPlayerMut()->SetSubtitleVisible(true);
          CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay = 0.0f;
          getAppPlayerMut()->SetSubTitleDelay(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay);
        }
      }
    }

    void Player::showSubtitles(bool bVisible)
    {
      XBMC_TRACE;
      if (getAppPlayer()->HasPlayer())
      {
        getAppPlayerMut()->SetSubtitleVisible(bVisible != 0);
      }
    }

    String Player::getSubtitles()
    {
      XBMC_TRACE;
      if (getAppPlayer()->HasPlayer())
      {
        SPlayerSubtitleStreamInfo info;
        getAppPlayerMut()->GetSubtitleStreamInfo(getAppPlayerMut()->GetSubtitle(), info);

        if (info.language.length() > 0)
          return info.language;
        else
          return info.name;
      }

      return NULL;
    }

    std::vector<String> Player::getAvailableSubtitleStreams()
    {
      if (getAppPlayer()->HasPlayer())
      {
        int subtitleCount = getAppPlayerMut()->GetSubtitleCount();
        std::vector<String> ret(subtitleCount);
        for (int iStream=0; iStream < subtitleCount; iStream++)
        {
          SPlayerSubtitleStreamInfo info;
          getAppPlayerMut()->GetSubtitleStreamInfo(iStream, info);

          if (info.language.length() > 0)
            ret[iStream] = info.language;
          else
            ret[iStream] = info.name;
        }
        return ret;
      }

      return std::vector<String>();
    }

    void Player::setSubtitleStream(int iStream)
    {
      if (getAppPlayer()->HasPlayer())
      {
        int streamCount = getAppPlayerMut()->GetSubtitleCount();
        if(iStream < streamCount)
        {
          getAppPlayerMut()->SetSubtitle(iStream);
          getAppPlayerMut()->SetSubtitleVisible(true);
        }
      }
    }

    std::vector<String> Player::getAvailableAudioStreams()
    {
      if (getAppPlayer()->HasPlayer())
      {
        int streamCount = getAppPlayerMut()->GetAudioStreamCount();
        std::vector<String> ret(streamCount);
        for (int iStream=0; iStream < streamCount; iStream++)
        {
          SPlayerAudioStreamInfo info;
          getAppPlayerMut()->GetAudioStreamInfo(iStream, info);

          if (info.language.length() > 0)
            ret[iStream] = info.language;
          else
            ret[iStream] = info.name;
        }
        return ret;
      }

      return std::vector<String>();
    }

    void Player::setAudioStream(int iStream)
    {
      if (getAppPlayer()->HasPlayer())
      {
        int streamCount = getAppPlayerMut()->GetAudioStreamCount();
        if(iStream < streamCount)
          getAppPlayerMut()->SetAudioStream(iStream);
      }
    }
  }
}

