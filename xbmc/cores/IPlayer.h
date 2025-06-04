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

#include "IAudioCallback.h"
#include "guilib/Geometry.h"
#include "Key.h"

class TiXmlElement; 
class CStreamDetails;

class CFileItem;

class IPlayerCallback
{
public:
  virtual ~IPlayerCallback() {}
  virtual void OnPlayBackEnded() = 0;
  virtual void OnPlayBackStarted() = 0;
  virtual void OnPlayBackPaused() {};
  virtual void OnPlayBackResumed() {};
  virtual void OnPlayBackStopped() = 0;
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackSeek(int iTime, int seekOffset) {};
  virtual void OnPlayBackSeekChapter(int iChapter) {};
  virtual void OnPlayBackSpeedChanged(int iSpeed) {};
};

class CPlayerOptions
{
public:
  CPlayerOptions()
  {
    starttime = 0LL;
    startpercent = 0LL;
    identify = false;
    fullscreen = false;
    video_only = false;
  }
  double  starttime; /* start time in seconds */
  double  startpercent; /* start time in percent */  
  bool    identify;  /* identify mode, used for checking format and length of a file */
  CStdString state;  /* potential playerstate to restore to */
  bool    fullscreen; /* player is allowed to switch to fullscreen */
  bool    video_only; /* player is not allowed to play audio streams, video streams only */
};

struct SPlayerAudioStreamInfo
{
  bool valid;
  int bitrate;
  int channels;
  int samplerate;
  int bitspersample;
  std::string language;
  std::string name;
  std::string audioCodecName;

  SPlayerAudioStreamInfo()
  {
    valid = false;
    bitrate = 0;
    channels = 0;
    samplerate = 0;
    bitspersample = 0;
  }
};

struct SPlayerSubtitleStreamInfo
{
  std::string language;
  std::string name;
};

struct SPlayerVideoStreamInfo
{
  bool valid;
  int bitrate;
  float videoAspectRatio;
  int height;
  int width;
  std::string language;
  std::string name;
  std::string videoCodecName;
  CRect SrcRect;
  CRect DestRect;
  std::string stereoMode;

  SPlayerVideoStreamInfo()
  {
    valid = false;
    bitrate = 0;
    videoAspectRatio = 1.0f;
    height = 0;
    width = 0;
  }
};

class IPlayer
{
public:
  IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer(){};
  virtual bool Initialize(TiXmlElement* pConfig) { return true; };
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {};
  virtual void UnRegisterAudioCallback() {};
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options){ return false;}
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile(bool reopen = false){ return true;}
  virtual bool IsPlaying() const { return false;}
  virtual bool CanPause() { return true; };
  virtual void Pause() = 0;
  virtual bool IsPaused() const = 0;
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetPercentage(){ return 0;}
  virtual float GetCachePercentage(){ return 0;}
  virtual void SetMute(bool bOnOff){}
  virtual void SetVolume(long nVolume){}
  virtual void SetDynamicRangeCompression(long drc){}
  virtual void GetAudioInfo( CStdString& strAudioInfo) = 0;
  virtual void GetVideoInfo( CStdString& strVideoInfo) = 0;
  virtual void GetGeneralInfo( CStdString& strVideoInfo) = 0;
  virtual void Update(bool bPauseDrawing = false) = 0;
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect) = 0;
  virtual void GetVideoAspectRatio(float& fAR) = 0;
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};

  virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  virtual float GetAVDelay()                    { return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay()    { return 0.0f; }
  virtual int  GetSubtitleCount()     { return 0; }
  virtual int  GetSubtitle()          { return -1; }
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName){};
  virtual void GetSubtitleLanguage(int iStream, CStdString &strStreamLang){};
  virtual void SetSubtitle(int iStream){};
  virtual bool GetSubtitleVisible(){ return false;};
  virtual void SetSubtitleVisible(bool bVisible){};
  virtual bool GetSubtitleExtension(std::string &strSubtitleExtension){ return false;};
  virtual int  AddSubtitle(const CStdString& strSubPath) {return -1;};

  virtual int  GetAudioStreamCount()  { return 0; }
  virtual int  GetAudioStream()       { return -1; }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName){};
  virtual void SetAudioStream(int iStream){};
  virtual void GetAudioStreamLanguage(int iStream, CStdString &strLanguage){};

  virtual std::string GetRadioText(unsigned int line) { return ""; };

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(std::string& strChapterName)     { return; }
  virtual int64_t GetChapterPos(int chapterIdx=-1)             { return -1; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }
//  virtual bool GetChapterInfo(int chapter, SChapterInfo &info) { return false; }

  virtual float GetActualFPS() { return 0.0f; };
  virtual void SeekTime(__int64 iTime = 0){};
  /*
   \brief seek relative to current time, returns false if not implemented by player
   \param iTime The time in milliseconds to seek. A positive value will seek forward, a negative backward.
   \return True if the player supports relative seeking, otherwise false
   */
  virtual bool SeekTimeRelative(int64_t iTime) { return false; }
  /*!
   \brief current time in milliseconds
   */
  virtual int64_t GetTime() { return 0; }
  /*!
   \brief total time in milliseconds
   */
  virtual int64_t GetTotalTime() { return 0; }
  virtual void ResetTime() {};
  virtual int GetAudioBitrate(){ return 0;}
  virtual int GetVideoBitrate(){ return 0;}
  virtual int GetSourceBitrate(){ return 0;}
  virtual int GetChannels(){ return 0;};
  virtual int GetBitsPerSample(){ return 0;};
  virtual int GetSampleRate(){ return 0;};
  virtual CStdString GetAudioCodecName(){ return "";}
  virtual CStdString GetVideoCodecName(){ return "";}
  virtual int GetPictureWidth(){ return 0;}
  virtual int GetPictureHeight(){ return 0;}
  virtual bool GetStreamDetails(CStreamDetails &details){ return false;}
  virtual void ToFFRW(int iSpeed = 0){};
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool IsCaching() const {return false;};
  //Cache filled in Percent
  virtual int GetCacheLevel() const {return -1;}; 

  virtual bool IsInMenu() const {return false;};
  virtual bool HasMenu() { return false; };

  virtual void DoAudioWork(){};
  virtual bool OnAction(const CAction &action) { return false; };

  virtual bool GetCurrentSubtitle(CStdString& strSubtitle) { strSubtitle = ""; return false; }
  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState() { return ""; };
  virtual bool SetPlayerState(CStdString state) { return false;};
  
  virtual std::string GetPlayingTitle() { return ""; };

  std::string m_name;
  std::string m_type;

protected:
  IPlayerCallback& m_callback;
};
