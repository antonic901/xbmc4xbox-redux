/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AdvancedSettings.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "network/DNSNameCache.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/FileUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

using namespace ADDON;

CAdvancedSettings::CAdvancedSettings()
{
  m_initialized = false;

  m_bVideoLibraryImportWatchedState = true;
  m_bVideoLibraryImportResumePoint = true;
}

std::string GetPlayerName(const int& player)
{
  if (player == 0)
    return "mplayer";
  if (player == 1)
    return "dvdplayer";
  if (player == 2)
    return "paplayer";

  return "";
}

void CAdvancedSettings::OnSettingsLoaded()
{
  const boost::shared_ptr<CProfilesManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // load advanced settings
  Load(*profileManager);

  // default players?
  CLog::Log(LOGINFO, "Default Video Player: %s", GetPlayerName(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_DEFAULTPLAYER)).c_str());
  CLog::Log(LOGINFO, "Default Audio Player: %s", GetPlayerName(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MUSICPLAYER_DEFAULTPLAYER)).c_str());

  // setup any logging...
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO))
  {
    m_logLevel = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    CLog::Log(LOGINFO, "Enabled debug logging due to GUI setting ({})", m_logLevel);
  }
  else
  {
    m_logLevel = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGINFO, "Disabled debug logging due to GUI setting. Level {}.", m_logLevel);
  }
  CLog::SetLogLevel(m_logLevel);
}

void CAdvancedSettings::OnSettingsUnloaded()
{
  m_initialized = false;
}

void CAdvancedSettings::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_DEBUG_SHOWLOGINFO)
    SetDebugMode(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
}

void CAdvancedSettings::Initialize(CSettingsManager& settingsMgr)
{
  Initialize();

  settingsMgr.RegisterSettingsHandler(this, true);
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_DEBUG_SHOWLOGINFO);
  settingsMgr.RegisterCallback(this, settingSet);
}

void CAdvancedSettings::Uninitialize(CSettingsManager& settingsMgr)
{
  settingsMgr.UnregisterCallback(this);
  settingsMgr.UnregisterSettingsHandler(this);
  settingsMgr.UnregisterSettingOptionsFiller("loggingcomponents");

  Clear();

  m_initialized = false;
}

void CAdvancedSettings::Initialize()
{
  if (m_initialized)
    return;

  m_seekSteps.push_back(10);
  m_seekSteps.push_back(30);
  m_seekSteps.push_back(60);
  m_seekSteps.push_back(180);
  m_seekSteps.push_back(300);
  m_seekSteps.push_back(600);
  m_seekSteps.push_back(1800);

  m_audioPlayCountMinimumPercent = 90.0f;

  m_videoSubsDelayRange = 60;
  m_videoAudioDelayRange = 10;
  m_videoUseTimeSeeking = true;
  m_videoTimeSeekForward = 30;
  m_videoTimeSeekBackward = -30;
  m_videoTimeSeekForwardBig = 600;
  m_videoTimeSeekBackwardBig = -600;
  m_videoPercentSeekForward = 2;
  m_videoPercentSeekBackward = -2;
  m_videoPercentSeekForwardBig = 10;
  m_videoPercentSeekBackwardBig = -10;

  m_videoPPFFmpegPostProc = "ha:128:7,va,dr";
  m_videoIgnoreSecondsAtStart = 3*60;
  m_videoIgnorePercentAtEnd   = 8.0f;
  m_videoPlayCountMinimumPercent = 90.0f;

  m_musicUseTimeSeeking = true;
  m_musicTimeSeekForward = 10;
  m_musicTimeSeekBackward = -10;
  m_musicTimeSeekForwardBig = 60;
  m_musicTimeSeekBackwardBig = -60;
  m_musicPercentSeekForward = 1;
  m_musicPercentSeekBackward = -1;
  m_musicPercentSeekForwardBig = 10;
  m_musicPercentSeekBackwardBig = -10;

  m_slideshowPanAmount = 2.5f;
  m_slideshowZoomAmount = 5.0f;
  m_slideshowBlackBarCompensation = 20.0f;

  m_songInfoDuration = 10;

  m_cddbAddress = "gnudb.gnudb.org";

  m_fullScreenOnMovieStart = true;
  m_cachePath = "special://temp/";

  m_videoCleanDateTimeRegExp = "(.*[^ _\\,\\.\\(\\)\\[\\]\\-])[ _\\.\\(\\)\\[\\]\\-]+(19[0-9][0-9]|20[0-9][0-9])([ _\\,\\.\\(\\)\\[\\]\\-]|[^0-9]$)?";

  m_videoCleanStringRegExps.clear();
  m_videoCleanStringRegExps.push_back(
      "[ "
      "_\\,\\.\\(\\)\\[\\]\\-](10bit|480p|480i|576p|576i|720p|720i|1080p|1080i|2160p|3d|aac|ac3|"
      "aka|atmos|avi|bd5|bdrip|bdremux|bluray|brrip|cam|cd[1-9]|custom|dc|ddp|divx|divx5|"
      "dolbydigital|dolbyvision|dsr|dsrip|dts|dts-hdma|dts-hra|dts-x|dv|dvd|dvd5|dvd9|dvdivx|"
      "dvdrip|dvdscr|dvdscreener|extended|fragment|fs|h264|h265|hdr|hdr10|hevc|hddvd|hdrip|"
      "hdtv|hdtvrip|hrhd|hrhdtv|internal|limited|multisubs|nfofix|ntsc|ogg|ogm|pal|pdtv|proper|"
      "r3|r5|read.nfo|remastered|remux|repack|rerip|retail|screener|se|svcd|tc|telecine|"
      "telesync|truehd|ts|uhd|unrated|ws|x264|x265|xvid|xvidvd|xxx|web-dl|webrip|www.www|"
      "\\[.*\\])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  m_videoCleanStringRegExps.push_back("(\\[.*\\])");

  m_moviesExcludeFromScanRegExps.clear();
  m_moviesExcludeFromScanRegExps.push_back("-trailer");
  m_moviesExcludeFromScanRegExps.push_back("[!-._ \\\\/]sample[-._ \\\\/]");
  m_moviesExcludeFromScanRegExps.push_back("[\\/](proof|subs)[\\/]");


  m_tvshowExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.push_back("[!-._ \\\\/]sample[-._ \\\\/]");


  m_audioExcludeFromScanRegExps.clear();

  m_folderStackRegExps.clear();
  m_folderStackRegExps.push_back("((cd|dvd|dis[ck])[0-9]+)$");

  m_videoStackRegExps.clear();
  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[0-9]+)(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[a-d])(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.push_back("(.*?)([ ._-]*[a-d])(.*?)(\\.[^.]+)$");
  // This one is a bit too greedy to enable by default.  It will stack sequels
  // in a flat dir structure, but is perfectly safe in a dir-per-vid one.
  //m_videoStackRegExps.push_back("(.*?)([ ._-]*[0-9])(.*?)(\\.[^.]+)$");

  m_tvshowEnumRegExps.clear();
  // foo.s01.e01, foo.s01_e01, S01E02 foo, S01 - E02, S01xE02
  m_tvshowEnumRegExps.push_back(
      TVShowRegexp(false, "s([0-9]+)[ ._x-]*e([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // foo.ep01, foo.EP_01, foo.E01
  m_tvshowEnumRegExps.push_back(
      TVShowRegexp(false, "[\\._ -]?()e(?:p[ ._-]?)?([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // foo.yyyy.mm.dd.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true, "([0-9]{4})[\\.-]([0-9]{2})[\\.-]([0-9]{2})"));
  // foo.mm.dd.yyyy.* (byDate=true)
  m_tvshowEnumRegExps.push_back(TVShowRegexp(true, "([0-9]{2})[\\.-]([0-9]{2})[\\.-]([0-9]{4})"));
  // foo.1x09* or just /1x09*
  m_tvshowEnumRegExps.push_back(
      TVShowRegexp(false, "[\\\\/\\._ \\[\\(-]([0-9]+)x([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$"));
  // Part I, Pt.VI, Part 1
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false,
                                   "[\\/._ -]p(?:ar)?t[_. -]()([ivx]+|[0-9]+)([._ -][^\\/]*)$"));
  // This regexp is for matching special episodes by their title, e.g. foo.special.mp4
  m_tvshowEnumRegExps.push_back(TVShowRegexp(false, "[\\\\/]([^\\\\/]+)\\.special\\.[a-z0-9]+$", 0, true));

  // foo.103*, 103 foo
  // XXX: This regex is greedy and will match years in show names.  It should always be last.
  m_tvshowEnumRegExps.push_back(
      TVShowRegexp(false,
      "[\\\\/\\._ -]([0-9]+)([0-9][0-9](?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([\\._ -][^\\\\/]*)$"));

  m_tvshowMultiPartEnumRegExp = "^[-_ex]+([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)";

  m_playlistAsFolders = true;
  m_detectAsUdf = false;

  m_fanartRes = 480;
  m_imageRes = 384;

  m_sambaclienttimeout = 30;
  m_sambadoscodepage = "";
  m_sambastatfiles = true;

  m_bHTTPDirectoryStatFilesize = false;

  m_bFTPThumbs = false;

  m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg|thumb.jpg|Thumb.jpg|thumb.JPG|Thumb.JPG";

  m_bMusicLibraryAllItemsOnBottom = false;
  m_bMusicLibraryCleanOnUpdate = false;
  m_bMusicLibraryArtistSortOnUpdate = false;
  m_iMusicLibraryRecentlyAddedItems = 25;
  m_strMusicLibraryAlbumFormat = "";
  m_prioritiseAPEv2tags = false;
  m_musicItemSeparator = " / ";
  m_musicArtistSeparators.push_back(";");
  m_musicArtistSeparators.push_back(" feat. ");
  m_musicArtistSeparators.push_back(" ft. ");
  m_videoItemSeparator = " / ";
  m_iMusicLibraryDateAdded = 1; // prefer mtime over ctime and current time

  m_bVideoLibraryAllItemsOnBottom = false;
  m_iVideoLibraryRecentlyAddedItems = 25;
  m_bVideoLibraryCleanOnUpdate = false;
  m_bVideoLibraryUseFastHash = true;
  m_bVideoScannerIgnoreErrors = false;
  m_iVideoLibraryDateAdded = 1; // prefer mtime over ctime and current time

  m_bEdlMergeShortCommBreaks = false;      // Off by default
  m_iEdlMaxCommBreakLength = 8 * 30 + 10;  // Just over 8 * 30 second commercial break.
  m_iEdlMinCommBreakLength = 3 * 30;       // 3 * 30 second commercial breaks.
  m_iEdlMaxCommBreakGap = 4 * 30;          // 4 * 30 second commercial breaks.
  m_iEdlMaxStartGap = 5 * 60;              // 5 minutes.
  m_iEdlCommBreakAutowait = 0;             // Off by default
  m_iEdlCommBreakAutowind = 0;             // Off by default

  m_curlconnecttimeout = 30;
  m_curllowspeedtime = 20;
  m_curlretries = 2;
  m_curlDisableIPV6 = true;      //Certain hardware/OS combinations have trouble
                                  //with ipv6.

  m_showExitButton = true;
  m_splashImage = true;

  m_playlistRetries = 100;
  m_playlistTimeout = 20; // 20 seconds timeout
  m_bVirtualShares = true;

  m_addonPackageFolderSize = 200;

  m_jsonOutputCompact = true;

  m_guiVisualizeDirtyRegions = false;
  m_guiAlgorithmDirtyRegions = 3;
  m_guiSmartRedraw = false;

  m_databaseMusic.Reset();
  m_databaseVideo.Reset();

  m_pictureExtensions =
      ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.rss|.webp|.jp2|.apng|.avif";
  m_musicExtensions = ".b4s|.nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.gdm|.imf|.m15|.sfx|.uni|.ac3|.dts|.cue|.aif|.aiff|.wpl|.xspf|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.wv|.dsp|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.wtv|.mka|.tak|.opus|.dff|.dsf|.m4b|.dtshd";
  m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.tp|.ts|.ty|.strm|.pls|.rm|.rmvb|.mpd|.m3u|.m3u8|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.udf|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.mk3d|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.001|.wpl|.xspf|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml|.vc1|.h264|.rcv|.rss|.mpls|.mpl|.webm|.bdmv|.bdm|.wtv|.trp|.f4v";
  m_subtitlesExtensions = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|"
                          ".ass|.vtt|.idx|.ifo|.zip|.sup";
  m_discStubExtensions = ".disc";
  // internal music extensions
  m_musicExtensions += "|.cdda";
  // internal video extensions
  m_videoExtensions += "|.pvr";

  m_logLevelHint = m_logLevel = LOG_LEVEL_NORMAL;

  m_userAgent = g_sysinfo.GetUserAgent();

  m_initialized = true;
}

bool CAdvancedSettings::Load(const CProfilesManager &profileManager)
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  Initialize(); // In case of profile switch.
  ParseSettingsFile("special://xbmc/system/advancedsettings.xml");

  ParseSettingsFile(profileManager.GetUserDataItem("advancedsettings.xml"));

  // Add the list of disc stub extensions (if any) to the list of video extensions
  if (!m_discStubExtensions.empty())
    m_videoExtensions += "|" + m_discStubExtensions;

  return true;
}

void CAdvancedSettings::ParseSettingsFile(const std::string &file)
{
  CXBMCTinyXML advancedXML;
  if (!XFILE::CFile::Exists(file))
  {
    CLog::Log(LOGINFO, "No settings file to load ({})", file);
    return;
  }

  if (!advancedXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading {}, Line {}\n{}", file, advancedXML.ErrorRow(),
              advancedXML.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = advancedXML.RootElement();
  if (!pRootElement || StringUtils::CompareNoCase(pRootElement->Value(), "advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading {}, no <advancedsettings> node", file);
    return;
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGINFO, "Loaded settings file from {}", file);

  // Dump contents of copied AS.xml to debug log
  TiXmlPrinter printer;
  printer.SetLineBreak("\n");
  printer.SetIndent("  ");
  advancedXML.Accept(&printer);
  CLog::Log(LOGINFO, "Contents of %s are...\n%s", file.c_str(), printer.CStr());

  TiXmlElement *pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_audioPlayCountMinimumPercent, 0.0f, 101.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_musicUseTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_musicTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_musicTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_musicTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_musicTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_musicPercentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_musicPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_musicPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_musicPercentSeekBackwardBig, -100, 0);

    TiXmlElement* pAudioExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_audioExcludeFromListingRegExps);

    pAudioExcludes = pElement->FirstChildElement("excludefromscan");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_audioExcludeFromScanRegExps);
  }

  pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "subsdelayrange", m_videoSubsDelayRange, 10, 600);
    XMLUtils::GetFloat(pElement, "audiodelayrange", m_videoAudioDelayRange, 10, 600);
    XMLUtils::GetBoolean(pElement, "fullscreenonmoviestart", m_fullScreenOnMovieStart);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_videoPlayCountMinimumPercent, 0.0f, 101.0f);
    XMLUtils::GetInt(pElement, "ignoresecondsatstart", m_videoIgnoreSecondsAtStart, 0, 900);
    XMLUtils::GetFloat(pElement, "ignorepercentatend", m_videoIgnorePercentAtEnd, 0, 100.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_videoUseTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_videoTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_videoTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_videoTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_videoTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_videoPercentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_videoPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_videoPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_videoPercentSeekBackwardBig, -100, 0);

    TiXmlElement* pVideoExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_videoExcludeFromListingRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludefromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_moviesExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludetvshowsfromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_tvshowExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("cleanstrings");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_videoCleanStringRegExps);

    XMLUtils::GetString(pElement,"cleandatetime", m_videoCleanDateTimeRegExp);
    XMLUtils::GetString(pElement,"ppffmpegpostprocessing",m_videoPPFFmpegPostProc);
  }

  pElement = pRootElement->FirstChildElement("musiclibrary");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iMusicLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "prioritiseapetags", m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", m_bMusicLibraryCleanOnUpdate);
    XMLUtils::GetBoolean(pElement, "artistsortonupdate", m_bMusicLibraryArtistSortOnUpdate);
    XMLUtils::GetString(pElement, "albumformat", m_strMusicLibraryAlbumFormat);
    XMLUtils::GetString(pElement, "itemseparator", m_musicItemSeparator);
    XMLUtils::GetInt(pElement, "dateadded", m_iMusicLibraryDateAdded);
    //Music artist name separators
    TiXmlElement* separators = pElement->FirstChildElement("artistseparators");
    if (separators)
    {
      m_musicArtistSeparators.clear();
      TiXmlNode* separator = separators->FirstChild("separator");
      while (separator)
      {
        if (separator->FirstChild())
          m_musicArtistSeparators.push_back(separator->FirstChild()->ValueStr());
        separator = separator->NextSibling("separator");
      }
    }
  }

  pElement = pRootElement->FirstChildElement("videolibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetInt(pElement, "recentlyaddeditems", m_iVideoLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", m_bVideoLibraryCleanOnUpdate);
    XMLUtils::GetBoolean(pElement, "usefasthash", m_bVideoLibraryUseFastHash);
    XMLUtils::GetString(pElement, "itemseparator", m_videoItemSeparator);
    XMLUtils::GetBoolean(pElement, "importwatchedstate", m_bVideoLibraryImportWatchedState);
    XMLUtils::GetBoolean(pElement, "importresumepoint", m_bVideoLibraryImportResumePoint);
    XMLUtils::GetInt(pElement, "dateadded", m_iVideoLibraryDateAdded);
  }

  pElement = pRootElement->FirstChildElement("videoscanner");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "ignoreerrors", m_bVideoScannerIgnoreErrors);
  }

  // Backward-compatibility of ExternalPlayer config
  pElement = pRootElement->FirstChildElement("externalplayer");
  if (pElement)
  {
    CLog::Log(LOGWARNING, "External player configuration has been removed from advancedsettings.xml.  It can now be configured in userdata/playercorefactory.xml");
  }
  pElement = pRootElement->FirstChildElement("slideshow");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "panamount", m_slideshowPanAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(pElement, "zoomamount", m_slideshowZoomAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(pElement, "blackbarcompensation", m_slideshowBlackBarCompensation, 0.0f, 50.0f);
  }

  pElement = pRootElement->FirstChildElement("network");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "curlclienttimeout", m_curlconnecttimeout, 1, 1000);
    XMLUtils::GetInt(pElement, "curllowspeedtime", m_curllowspeedtime, 1, 1000);
    XMLUtils::GetInt(pElement, "curlretries", m_curlretries, 0, 10);
    XMLUtils::GetBoolean(pElement, "disableipv6", m_curlDisableIPV6);
  }

  pElement = pRootElement->FirstChildElement("jsonrpc");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "compactoutput", m_jsonOutputCompact);
  }

  pElement = pRootElement->FirstChildElement("samba");
  if (pElement)
  {
    XMLUtils::GetString(pElement,  "doscodepage",   m_sambadoscodepage);
    XMLUtils::GetInt(pElement, "clienttimeout", m_sambaclienttimeout, 5, 100);
    XMLUtils::GetBoolean(pElement, "statfiles", m_sambastatfiles);
  }

  pElement = pRootElement->FirstChildElement("httpdirectory");
  if (pElement)
    XMLUtils::GetBoolean(pElement, "statfilesize", m_bHTTPDirectoryStatFilesize);

  pElement = pRootElement->FirstChildElement("ftp");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "remotethumbs", m_bFTPThumbs);
  }

  pElement = pRootElement->FirstChildElement("loglevel");
  if (pElement)
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    XMLUtils::GetInt(pRootElement, "loglevel", m_logLevelHint, LOG_LEVEL_NONE, LOG_LEVEL_MAX);
    const char* hide = pElement->Attribute("hide");
    if (hide == NULL || StringUtils::CompareNoCase("false", hide, 5) != 0)
    {
      SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_DEBUG_SHOWLOGINFO);
      if (setting != NULL)
        setting->SetVisible(false);
    }
    m_logLevel = std::max(m_logLevel, m_logLevelHint);
    CLog::SetLogLevel(m_logLevel);
  }

  XMLUtils::GetString(pRootElement, "cddbaddress", m_cddbAddress);

  XMLUtils::GetBoolean(pRootElement, "splash", m_splashImage);
  XMLUtils::GetBoolean(pRootElement, "showexitbutton", m_showExitButton);

  XMLUtils::GetInt(pRootElement, "songinfoduration", m_songInfoDuration, 0, INT_MAX);
  XMLUtils::GetInt(pRootElement, "playlistretries", m_playlistRetries, -1, 5000);
  XMLUtils::GetInt(pRootElement, "playlisttimeout", m_playlistTimeout, 0, 5000);

  XMLUtils::GetBoolean(pRootElement,"virtualshares", m_bVirtualShares);
  XMLUtils::GetUInt(pRootElement, "packagefoldersize", m_addonPackageFolderSize);

  // EDL commercial break handling
  pElement = pRootElement->FirstChildElement("edl");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mergeshortcommbreaks", m_bEdlMergeShortCommBreaks);
    XMLUtils::GetInt(pElement, "maxcommbreaklength", m_iEdlMaxCommBreakLength, 0, 10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "mincommbreaklength", m_iEdlMinCommBreakLength, 0, 5 * 60);  // Between 0 and 5 minutes
    XMLUtils::GetInt(pElement, "maxcommbreakgap", m_iEdlMaxCommBreakGap, 0, 5 * 60);        // Between 0 and 5 minutes.
    XMLUtils::GetInt(pElement, "maxstartgap", m_iEdlMaxStartGap, 0, 10 * 60);               // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "commbreakautowait", m_iEdlCommBreakAutowait, -60, 60);        // Between -60 and 60 seconds
    XMLUtils::GetInt(pElement, "commbreakautowind", m_iEdlCommBreakAutowind, -60, 60);        // Between -60 and 60 seconds
  }

  // picture exclude regexps
  TiXmlElement* pPictureExcludes = pRootElement->FirstChildElement("pictureexcludes");
  if (pPictureExcludes)
    GetCustomRegexps(pPictureExcludes, m_pictureExcludeFromListingRegExps);

  // picture extensions
  TiXmlElement* pExts = pRootElement->FirstChildElement("pictureextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_pictureExtensions);

  // music extensions
  pExts = pRootElement->FirstChildElement("musicextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_musicExtensions);

  // video extensions
  pExts = pRootElement->FirstChildElement("videoextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_videoExtensions);

  // stub extensions
  pExts = pRootElement->FirstChildElement("discstubextensions");
  if (pExts)
    GetCustomExtensions(pExts, m_discStubExtensions);

  m_vecTokens.clear();
  CLangInfo::LoadTokens(pRootElement->FirstChild("sorttokens"),m_vecTokens);

  //! @todo Should cache path be given in terms of our predefined paths??
  //! Are we even going to have predefined paths??
  std::string tmp;
  if (XMLUtils::GetPath(pRootElement, "cachepath", tmp))
    m_cachePath = tmp;
  URIUtils::AddSlashAtEnd(m_cachePath);

  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

  // trailer matching regexps
  TiXmlElement* pTrailerMatching = pRootElement->FirstChildElement("trailermatching");
  if (pTrailerMatching)
    GetCustomRegexps(pTrailerMatching, m_trailerMatchRegExps);

  //everything that's a trailer is not a movie
  m_moviesExcludeFromScanRegExps.insert(m_moviesExcludeFromScanRegExps.end(),
                                        m_trailerMatchRegExps.begin(),
                                        m_trailerMatchRegExps.end());

  // video stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("moviestacking");
  if (pVideoStacking)
    GetCustomRegexps(pVideoStacking, m_videoStackRegExps);

  // folder stacking regexps
  TiXmlElement* pFolderStacking = pRootElement->FirstChildElement("folderstacking");
  if (pFolderStacking)
    GetCustomRegexps(pFolderStacking, m_folderStackRegExps);

  //tv stacking regexps
  TiXmlElement* pTVStacking = pRootElement->FirstChildElement("tvshowmatching");
  if (pTVStacking)
    GetCustomTVRegexps(pTVStacking, m_tvshowEnumRegExps);

  //tv multipart enumeration regexp
  XMLUtils::GetString(pRootElement, "tvmultipartmatching", m_tvshowMultiPartEnumRegExp);

  // path substitutions
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
    TiXmlNode* pSubstitute = pPathSubstitution->FirstChildElement("substitute");
    while (pSubstitute)
    {
      std::string strFrom, strTo;
      TiXmlNode* pFrom = pSubstitute->FirstChild("from");
      if (pFrom && !pFrom->NoChildren())
        strFrom = CSpecialProtocol::TranslatePath(pFrom->FirstChild()->Value()).c_str();
      TiXmlNode* pTo = pSubstitute->FirstChild("to");
      if (pTo && !pTo->NoChildren())
        strTo = pTo->FirstChild()->Value();

      if (!strFrom.empty() && !strTo.empty())
      {
        CLog::Log(LOGDEBUG,"  Registering substitution pair:");
        CLog::Log(LOGDEBUG, "    From: [{}]", CURL::GetRedacted(strFrom));
        CLog::Log(LOGDEBUG, "    To:   [{}]", CURL::GetRedacted(strTo));
        m_pathSubstitutions.push_back(std::make_pair(strFrom, strTo));
      }
      else
      {
        // error message about missing tag
        if (strFrom.empty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");
      }

      // get next one
      pSubstitute = pSubstitute->NextSiblingElement("substitute");
    }
  }

  XMLUtils::GetUInt(pRootElement, "fanartres", m_fanartRes, 0, 9999);
  XMLUtils::GetUInt(pRootElement, "imageres", m_imageRes, 0, 9999);
  XMLUtils::GetBoolean(pRootElement, "playlistasfolders", m_playlistAsFolders);
  XMLUtils::GetBoolean(pRootElement, "detectasudf", m_detectAsUdf);

  // music thumbs
  TiXmlElement* pThumbs = pRootElement->FirstChildElement("musicthumbs");
  if (pThumbs)
    GetCustomExtensions(pThumbs,m_musicThumbs);

  // music filename->tag filters
  TiXmlElement* filters = pRootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    TiXmlNode* filter = filters->FirstChild("filter");
    while (filter)
    {
      if (filter->FirstChild())
        m_musicTagsFromFileFilters.push_back(filter->FirstChild()->ValueStr());
      filter = filter->NextSibling("filter");
    }
  }

  TiXmlElement* pHostEntries = pRootElement->FirstChildElement("hosts");
  if (pHostEntries)
  {
    TiXmlElement* element = pHostEntries->FirstChildElement("entry");
    while(element)
    {
      if(!element->NoChildren())
      {
        std::string name  = XMLUtils::GetAttribute(element, "name");
        std::string value = element->FirstChild()->ValueStr();
        if (!name.empty())
          CDNSNameCache::Add(name, value);
      }
      element = element->NextSiblingElement("entry");
    }
  }

  TiXmlElement* pDatabase = pRootElement->FirstChildElement("videodatabase");
  if (pDatabase)
  {
    CLog::Log(LOGWARNING, "VIDEO database configuration is experimental.");
    XMLUtils::GetString(pDatabase, "type", m_databaseVideo.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseVideo.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseVideo.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseVideo.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseVideo.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseVideo.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseVideo.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseVideo.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseVideo.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseVideo.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseVideo.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseVideo.compression);
  }

  pDatabase = pRootElement->FirstChildElement("musicdatabase");
  if (pDatabase)
  {
    XMLUtils::GetString(pDatabase, "type", m_databaseMusic.type);
    XMLUtils::GetString(pDatabase, "host", m_databaseMusic.host);
    XMLUtils::GetString(pDatabase, "port", m_databaseMusic.port);
    XMLUtils::GetString(pDatabase, "user", m_databaseMusic.user);
    XMLUtils::GetString(pDatabase, "pass", m_databaseMusic.pass);
    XMLUtils::GetString(pDatabase, "name", m_databaseMusic.name);
    XMLUtils::GetString(pDatabase, "key", m_databaseMusic.key);
    XMLUtils::GetString(pDatabase, "cert", m_databaseMusic.cert);
    XMLUtils::GetString(pDatabase, "ca", m_databaseMusic.ca);
    XMLUtils::GetString(pDatabase, "capath", m_databaseMusic.capath);
    XMLUtils::GetString(pDatabase, "ciphers", m_databaseMusic.ciphers);
    XMLUtils::GetBoolean(pDatabase, "compression", m_databaseMusic.compression);
  }

  pElement = pRootElement->FirstChildElement("gui");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "visualizedirtyregions", m_guiVisualizeDirtyRegions);
    XMLUtils::GetInt(pElement, "algorithmdirtyregions",     m_guiAlgorithmDirtyRegions);
    XMLUtils::GetBoolean(pElement, "smartredraw", m_guiSmartRedraw);
  }

  std::string seekSteps;
  XMLUtils::GetString(pRootElement, "seeksteps", seekSteps);
  if (!seekSteps.empty())
  {
    m_seekSteps.clear();
    std::vector<std::string> steps = StringUtils::Split(seekSteps, ',');
    for(std::vector<std::string>::iterator it = steps.begin(); it != steps.end(); ++it)
      m_seekSteps.push_back(atoi((*it).c_str()));
  }

  // load in the settings overrides
  CServiceBroker::GetSettingsComponent()->GetSettings()->LoadHidden(pRootElement);
}

void CAdvancedSettings::Clear()
{
  m_videoCleanStringRegExps.clear();
  m_moviesExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.clear();
  m_videoExcludeFromListingRegExps.clear();
  m_videoStackRegExps.clear();
  m_folderStackRegExps.clear();
  m_audioExcludeFromScanRegExps.clear();
  m_audioExcludeFromListingRegExps.clear();
  m_pictureExcludeFromListingRegExps.clear();

  m_pictureExtensions.clear();
  m_musicExtensions.clear();
  m_videoExtensions.clear();
  m_discStubExtensions.clear();

  m_userAgent.clear();
}

void CAdvancedSettings::GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && StringUtils::CompareNoCase(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
        iAction = 1; // append
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        bool bByDate = false;
        bool byTitle = false;
        int iDefaultSeason = 1;
        if (pRegExp->ToElement())
        {
          std::string byDate = XMLUtils::GetAttribute(pRegExp->ToElement(), "bydate");
          if (byDate == "true")
          {
            bByDate = true;
          }
          std::string byTitleAttr = XMLUtils::GetAttribute(pRegExp->ToElement(), "bytitle");
          byTitle = (byTitleAttr == "true");
          std::string defaultSeason = XMLUtils::GetAttribute(pRegExp->ToElement(), "defaultseason");
          if(!defaultSeason.empty())
          {
            iDefaultSeason = atoi(defaultSeason.c_str());
          }
        }
        std::string regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
        {
          settings.insert(settings.begin() + i++, 1,
                          TVShowRegexp(bByDate, regExp, iDefaultSeason, byTitle));
        }
        else
        {
          settings.push_back(TVShowRegexp(bByDate, regExp, iDefaultSeason, byTitle));
        }
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}

void CAdvancedSettings::GetCustomRegexps(TiXmlElement *pRootElement, std::vector<std::string>& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && StringUtils::CompareNoCase(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
        iAction = 1; // append
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        std::string regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, regExp);
        else
          settings.push_back(regExp);
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}

void CAdvancedSettings::GetCustomExtensions(TiXmlElement *pRootElement, std::string& extensions)
{
  std::string extraExtensions;
  if (XMLUtils::GetString(pRootElement, "add", extraExtensions) && !extraExtensions.empty())
    extensions += "|" + extraExtensions;
  if (XMLUtils::GetString(pRootElement, "remove", extraExtensions) && !extraExtensions.empty())
  {
    std::vector<std::string> exts = StringUtils::Split(extraExtensions, '|');
    for (std::vector<std::string>::const_iterator i = exts.begin(); i != exts.end(); ++i)
    {
      size_t iPos = extensions.find(*i);
      if (iPos != std::string::npos)
        extensions.erase(iPos,i->size()+1);
    }
  }
}

void CAdvancedSettings::SetDebugMode(bool debug)
{
  if (debug)
  {
    int level = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    m_logLevel = level;
    CLog::SetLogLevel(level);
    CLog::Log(LOGINFO, "Enabled debug logging due to GUI setting. Level %i.", level);
  }
  else
  {
    int level = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGINFO, "Disabled debug logging due to GUI setting. Level %i.", level);
    m_logLevel = level;
    CLog::SetLogLevel(level);
  }
}
