/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/SortUtils.h"

#include <stdint.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

class CProfileManager;
class CSettingsManager;
class CVariant;
struct IntegerSettingOption;

class TiXmlElement;
namespace ADDON
{
  class IAddon;
}

class DatabaseSettings
{
public:
  DatabaseSettings() { Reset(); }
  void Reset()
  {
    type.clear();
    host.clear();
    port.clear();
    user.clear();
    pass.clear();
    name.clear();
    key.clear();
    cert.clear();
    ca.clear();
    capath.clear();
    ciphers.clear();
    compression = false;
  };
  std::string type;
  std::string host;
  std::string port;
  std::string user;
  std::string pass;
  std::string name;
  std::string key;
  std::string cert;
  std::string ca;
  std::string capath;
  std::string ciphers;
  bool compression;
};

struct TVShowRegexp
{
  bool byDate;
  bool byTitle;
  std::string regexp;
  int defaultSeason;
  TVShowRegexp(bool d, const std::string& r, int s = 1, bool t = false) : regexp(r)
  {
    byDate = d;
    defaultSeason = s;
    byTitle = t;
  }
};


typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

class CAdvancedSettings : public ISettingCallback, public ISettingsHandler
{
  public:
    CAdvancedSettings();

    virtual void OnSettingsLoaded();
    virtual void OnSettingsUnloaded();

    virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);

    void Initialize(CSettingsManager& settingsMgr);
    void Uninitialize(CSettingsManager& settingsMgr);
    bool Initialized() const { return m_initialized; }
    bool Load(const CProfileManager &profileManager);

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, std::vector<std::string> &settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, std::string& extensions);

    int m_audioHeadRoom;
    float m_karaokeSyncDelay;
    float m_audioPlayCountMinimumPercent;
    bool m_VideoPlayerIgnoreDTSinWAV;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    std::vector<int> m_seekSteps;
    std::string m_videoPPFFmpegDeint;
    std::string m_videoPPFFmpegPostProc;
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
    int m_videoBusyDialogDelay_ms;
    int m_videoIgnoreSecondsAtStart;
    float m_videoIgnorePercentAtEnd;

    float m_videoPlayCountMinimumPercent;

    unsigned int m_cacheMemSize;
    unsigned int m_cacheBufferMode;
    float m_cacheReadFactor;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;

    int m_autoDetectPingTime;

    int m_songInfoDuration;
    int m_logLevel;
    int m_logLevelHint;
    std::string m_cddbAddress;
    bool m_addSourceOnTop; //!< True to put 'add source' buttons on top

    bool m_fullScreenOnMovieStart;
    std::string m_cachePath;
    std::string m_videoCleanDateTimeRegExp;
    std::string m_videoFilenameIdentifierRegExp;
    std::vector<std::string> m_videoCleanStringRegExps;
    std::vector<std::string> m_videoExcludeFromListingRegExps;
    std::vector<std::string> m_moviesExcludeFromScanRegExps;
    std::vector<std::string> m_tvshowExcludeFromScanRegExps;
    std::vector<std::string> m_audioExcludeFromListingRegExps;
    std::vector<std::string> m_audioExcludeFromScanRegExps;
    std::vector<std::string> m_pictureExcludeFromListingRegExps;
    std::vector<std::string> m_videoStackRegExps;
    std::vector<std::string> m_folderStackRegExps;
    std::vector<std::string> m_trailerMatchRegExps;
    SETTINGS_TVSHOWLIST m_tvshowEnumRegExps;
    std::string m_tvshowMultiPartEnumRegExp;
    typedef std::vector< std::pair<std::string, std::string> > StringMapping;
    StringMapping m_pathSubstitutions;

    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;
    bool m_DisableModChipDetection;
    bool m_bPowerSave;
    bool m_displayRemoteCodes;
    bool m_noDVDROM;
    bool m_usePCDVDROM;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    unsigned int m_fanartRes; ///< \brief the maximal resolution to cache fanart at (assumes 16x9)
    unsigned int m_imageRes;  ///< \brief the maximal resolution to cache images at (assumes 16x9)
    bool m_useDDSFanart; ///< \brief support for DDS DXT1 generated thumbs

    int m_sambaclienttimeout;
    std::string m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    bool m_bFTPThumbs;

    int m_iMusicLibraryRecentlyAddedItems;
    int m_iMusicLibraryDateAdded;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryCleanOnUpdate;
    bool m_bMusicLibraryArtistSortOnUpdate;
    bool m_bMusicLibraryArtistNavigatesToSongs;
    std::string m_strMusicLibraryAlbumFormat;
    bool m_prioritiseAPEv2tags;
    std::string m_musicItemSeparator;
    std::vector<std::string> m_musicArtistSeparators;
    std::string m_videoItemSeparator;
    std::string m_programItemSeparator;
    std::vector<std::string> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryUseFastHash;
    bool m_bVideoLibraryImportWatchedState;
    bool m_bVideoLibraryImportResumePoint;

    bool m_bVideoScannerIgnoreErrors;
    int m_iVideoLibraryDateAdded;

    std::set<std::string> m_vecTokens;

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds

    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;
    bool m_curlDisableIPV6;

    bool m_showExitButton; /* Ideal for appliances to hide a 'useless' button */
    bool m_splashImage;
    int m_playlistRetries;
    int m_playlistTimeout;

    bool m_bVirtualShares;

    DatabaseSettings m_databaseMusic; // advanced music database setup
    DatabaseSettings m_databaseVideo; // advanced video database setup

    bool m_guiKeepInMemory;
    bool m_guiVisualizeDirtyRegions;
    int  m_guiAlgorithmDirtyRegions;
    bool m_guiSmartRedraw;
    unsigned int m_addonPackageFolderSize;

    bool m_jsonOutputCompact;

    void ParseSettingsFile(const std::string &file);

    bool m_initialized;

    void SetDebugMode(bool debug);

    //! \brief Toggles dirty-region visualization
    void ToggleDirtyRegionVisualization()
    {
      m_guiVisualizeDirtyRegions = !m_guiVisualizeDirtyRegions;
    }

    // runtime settings which cannot be set from advancedsettings.xml
    std::string m_videoExtensions;
    std::string m_discStubExtensions;
    std::string m_subtitlesExtensions;
    std::string m_musicExtensions;
    std::string m_pictureExtensions;
    std::string m_programExtensions;

    std::string m_userAgent;

  private:
    void Initialize();
    void Clear();
};
