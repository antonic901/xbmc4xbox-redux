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

#include <vector>
#include <map>
#include <set>

#include "settings/ISettingsHandler.h"
#include "settings/ISubSettings.h"
#include "settings/GUISettings.h"
#include "MediaSource.h"
#include "settings/AdvancedSettings.h"
#include "utils/CriticalSection.h"

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

class CGUISettings;
class TiXmlElement;
class TiXmlNode;

class CSettings : private ISettingsHandler, ISubSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  void RegisterSettingsHandler(ISettingsHandler *settingsHandler);
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);
  void RegisterSubSettings(ISubSettings *subSettings);
  void UnregisterSubSettings(ISubSettings *subSettings);

  void Initialize();

  bool Load();
  void Save() const;
  bool Reset();

  void Clear();

  struct AdvancedSettings
  {
public:
    // multipath testing
    bool m_useMultipaths;
    bool m_DisableModChipDetection;

    int m_audioHeadRoom;
    float m_ac3Gain;
    CStdString m_audioDefaultPlayer;
    float m_audioPlayCountMinimumPercent;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    CStdString m_videoPPFFmpegType;
    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_musicResample;
    int m_videoBlackBarColour;
    int m_videoIgnoreAtStart;
    int m_videoIgnoreAtEnd;
    CStdString m_audioHost;
    bool m_audioApplyDrc;

    CStdString m_videoDefaultPlayer;
    CStdString m_videoDefaultDVDPlayer;
    float m_videoPlayCountMinimumPercent;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;
    bool m_lcdHeartbeat;
    int m_lcdScrolldelay;

    int m_autoDetectPingTime;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    int m_logLevel;
    int m_logLevelHint;
    CStdString m_cddbAddress;
#ifdef HAS_HAL
    bool m_useHalMount;
#endif
    bool m_fullScreenOnMovieStart;
    bool m_noDVDROM;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdString m_videoCleanDateTimeRegExp;
    CStdStringArray m_videoCleanStringRegExps;
    CStdStringArray m_videoExcludeFromListingRegExps;
    CStdStringArray m_moviesExcludeFromScanRegExps;
    CStdStringArray m_tvshowExcludeFromScanRegExps;
    CStdStringArray m_audioExcludeFromListingRegExps;
    CStdStringArray m_audioExcludeFromScanRegExps;
    CStdStringArray m_pictureExcludeFromListingRegExps;
    CStdStringArray m_videoStackRegExps;
    SETTINGS_TVSHOWLIST m_tvshowStackRegExps;
    CStdString m_tvshowMultiPartStackRegExp;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;

    bool m_bMusicLibraryHideAllItems;
    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    std::vector<CStdString> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryMyMoviesCategoriesToGenres;

    bool m_bUseEvilB;
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;

    int m_iMythMovieLength;         // minutes

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds

    bool m_bFirstLoop;
    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;

    bool m_fullScreen;
    bool m_startFullScreen;
    bool m_alwaysOnTop;  /* makes xbmc to run always on top .. osx/win32 only .. */
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool m_osx_GLFullScreen;
    bool m_bVirtualShares;
    bool m_bNavVKeyboard; // if true we navigate the virtual keyboard using cursor keys

    float m_karaokeSyncDelayCDG; // seems like different delay is needed for CDG and MP3s
    float m_karaokeSyncDelayLRC;
    bool m_karaokeChangeGenreForKaraokeSongs;
    bool m_karaokeKeepDelay; // store user-changed song delay in the database
    int m_karaokeStartIndex; // auto-assign numbering start from this value
    bool m_karaokeAlwaysEmptyOnCdgs; // always have empty background on CDG files
    bool m_karaokeUseSongSpecificBackground; // use song-specific video or image if available instead of default
    CStdString m_karaokeDefaultBackgroundType; // empty string or "vis", "image" or "video"
    CStdString m_karaokeDefaultBackgroundFilePath; // only for "image" or "video" types above

    CStdString m_cpuTempCmd;
    CStdString m_gpuTempCmd;
    int m_bgInfoLoaderMaxThreads;
  };

  CStdString m_pictureExtensions;
  CStdString m_musicExtensions;
  CStdString m_videoExtensions;

  CStdString m_logFolder;

  bool m_bMyMusicSongInfoInVis;
  bool m_bMyMusicSongThumbInVis;

  bool m_bMyMusicPlaylistRepeat;
  bool m_bMyMusicPlaylistShuffle;
  int m_iMyMusicStartWindow;

  // for scanning
  bool m_bMyMusicIsScanning;

  float m_fZoomAmount;      // current zoom amount
  float m_fPixelRatio;      // current pixel ratio

  bool m_bMyVideoPlaylistRepeat;
  bool m_bMyVideoPlaylistShuffle;
  bool m_bMyVideoNavFlatten;
  bool m_bStartVideoWindowed;
  bool m_bAddonAutoUpdate;
  bool m_bAddonNotifications;
  bool m_bAddonForeignFilter;

  int m_iVideoStartWindow;

  bool m_videoStacking;

  int iAdditionalSubtitleDirectoryChecked;

  char szOnlineArenaPassword[32]; // private arena password
  char szOnlineArenaDescription[64]; // private arena description

  int m_HttpApiBroadcastPort;
  int m_HttpApiBroadcastLevel;
  int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
  int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.
  int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
  bool m_bMute;
  int m_iSystemTimeTotalUp;    // Uptime in minutes!

  VOICE_MASK m_karaokeVoiceMask[4];

  struct RssSet
  {
    bool rtl;
    std::vector<int> interval;
    std::vector<std::string> url;
  };

  std::map<int,RssSet> m_mapRssUrls;

  CStdString m_defaultMusicLibSource;

  CStdString GetFFmpegDllFolder() const;
  CStdString GetPlayerName(const int& player) const;
  CStdString GetDefaultVideoPlayerName() const;
  CStdString GetDefaultAudioPlayerName() const;

  CStdString GetAvpackSettingsFile() const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  void LoadRSSFeeds();
  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  static bool GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue);
  static bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
protected:
  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool SaveAvpackXML() const;
  bool SaveNewAvpackXML() const;
  bool SaveAvpackSettings(TiXmlNode *io_pRoot) const;
  bool LoadAvpackXML();

private:
  // implementation of ISettingsHandler
  virtual bool OnSettingsLoading();
  virtual void OnSettingsLoaded();
  virtual bool OnSettingsSaving() const;
  virtual void OnSettingsSaved() const;
  virtual void OnSettingsCleared();

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  CCriticalSection m_critical;
  typedef std::set<ISettingsHandler*> SettingsHandlers;
  SettingsHandlers m_settingsHandlers;
  typedef std::set<ISubSettings*> SubSettings;
  SubSettings m_subSettings;
};

extern class CSettings g_settings;