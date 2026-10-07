/*
 *  Copyright (c) 2006 elupus (Joakim Plate)
 *  Copyright (C) 2006-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayer.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

#include <boost/move/unique_ptr.hpp>
#include <string>

class PLT_MediaController;

namespace UPNP
{

class CUPnPPlayerController;

class CUPnPPlayer : public IPlayer, public CThread
{
public:
  CUPnPPlayer(IPlayerCallback& callback, const char* uuid);
  virtual ~CUPnPPlayer();

  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options);
  virtual bool QueueNextFile(const CFileItem &file);
  virtual bool CloseFile(bool reopen = false);
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool HasVideo() const { return m_hasVideo; }
  virtual bool HasAudio() const { return m_hasAudio; }
  virtual void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride);
  virtual void SeekPercentage(float fPercent = 0);
  virtual void SetVolume(float volume);

  virtual int SeekChapter(int iChapter) { return -1; }

  virtual void SeekTime(int64_t iTime = 0);
  virtual void SetSpeed(float speed = 0);

  virtual bool IsCaching() const { return false; }
  virtual int GetCacheLevel() const { return -1; }
  virtual bool OnAction(const CAction &action);

  int PlayFile(const CFileItem& file,
               const CPlayerOptions& options,
               XbmcThreads::EndTime& timeout);

private:
  bool IsPaused() const;
  int64_t GetTime();
  int64_t GetTotalTime();
  float GetPercentage();

  // implementation of CThread
  virtual void Process();
  virtual void OnExit();

  PLT_MediaController* m_control;
  boost::movelib::unique_ptr<CUPnPPlayerController> m_delegate;
  std::string m_current_uri;
  std::string m_current_meta;
  bool m_started;
  bool m_stopremote;
  bool m_hasVideo;
  bool m_hasAudio;
  XbmcThreads::EndTime m_updateTimer;
};

} /* namespace UPNP */
