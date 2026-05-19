/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/SettingsBase.h"

#include <string>

class CSettingList;
class TiXmlNode;

/*!
 \brief Wrapper around CSettingsManager responsible for properly setting up
 the settings manager and registering all the callbacks, handlers and custom
 setting types.
 \sa CSettingsManager
 */
class CSettings : public CSettingsBase, public CSettingCreator, public CSettingControlCreator
                , private ISubSettings
{
public:
  static const char* SETTING_LOOKANDFEEL_SKIN;
  static const char* SETTING_LOOKANDFEEL_SKINSETTINGS;
  static const char* SETTING_LOOKANDFEEL_SKINTHEME;
  static const char* SETTING_LOOKANDFEEL_SKINCOLORS;
  static const char* SETTING_LOOKANDFEEL_FONT;
  static const char* SETTING_LOOKANDFEEL_SKINZOOM;
  static const char* SETTING_LOOKANDFEEL_STARTUPWINDOW;
  static const char* SETTING_LOOKANDFEEL_SOUNDSKIN;
  static const char* SETTING_LOOKANDFEEL_ENABLERSSFEEDS;
  static const char* SETTING_LOOKANDFEEL_RSSEDIT;
  static const char* SETTING_LOCALE_LANGUAGE;
  static const char* SETTING_LOCALE_COUNTRY;
  static const char* SETTING_LOCALE_CHARSET;
  static const char* SETTING_LOCALE_KEYBOARDLAYOUTS;
  static const char* SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT;
  static const char* SETTING_LOCALE_TIMEZONE;
  static const char* SETTING_LOCALE_SHORTDATEFORMAT;
  static const char* SETTING_LOCALE_LONGDATEFORMAT;
  static const char* SETTING_LOCALE_TIMEFORMAT;
  static const char* SETTING_LOCALE_USE24HOURCLOCK;
  static const char* SETTING_LOCALE_TEMPERATUREUNIT;
  static const char* SETTING_LOCALE_SPEEDUNIT;
  static const char* SETTING_FILELISTS_SHOWPARENTDIRITEMS;
  static const char* SETTING_FILELISTS_SHOWEXTENSIONS;
  static const char* SETTING_FILELISTS_IGNORETHEWHENSORTING;
  static const char* SETTING_FILELISTS_ALLOWFILEDELETION;
  static const char* SETTING_FILELISTS_SHOWADDSOURCEBUTTONS;
  static const char* SETTING_FILELISTS_SHOWHIDDEN;
  static const char* SETTING_SCREENSAVER_MODE;
  static const char* SETTING_SCREENSAVER_SETTINGS;
  static const char* SETTING_SCREENSAVER_PREVIEW;
  static const char* SETTING_SCREENSAVER_TIME;
  static const char* SETTING_SCREENSAVER_DISABLEFORAUDIO;
  static const char* SETTING_SCREENSAVER_USEDIMONPAUSE;
  static const char* SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS;
  static const char* SETTING_VIDEOLIBRARY_ACTORTHUMBS;
  static const char* SETTING_MYVIDEOS_FLATTEN;
  static const char* SETTING_VIDEOLIBRARY_FLATTENTVSHOWS;
  static const char* SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM;
  static const char* SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS;
  static const char* SETTING_VIDEOLIBRARY_SHOWALLITEMS;
  static const char* SETTING_VIDEOLIBRARY_GROUPMOVIESETS;
  static const char* SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS;
  static const char* SETTING_VIDEOLIBRARY_UPDATEONSTARTUP;
  static const char* SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE;
  static const char* SETTING_VIDEOLIBRARY_CLEANUP;
  static const char* SETTING_VIDEOLIBRARY_EXPORT;
  static const char* SETTING_VIDEOLIBRARY_IMPORT;
  static const char* SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS;
  static const char* SETTING_VIDEOLIBRARY_MOVIESETSFOLDER;
  static const char* SETTING_VIDEOLIBRARY_ARTWORK_LEVEL;
  static const char* SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST;
  static const char* SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST;
  static const char* SETTING_VIDEOLIBRARY_EPISODEART_WHITELIST;
  static const char* SETTING_VIDEOLIBRARY_MUSICVIDEOART_WHITELIST;
  static const char* SETTING_VIDEOLIBRARY_SHOWPERFORMERS;
  static const char* SETTING_VIDEOLIBRARY_IGNOREVIDEOVERSIONS;
  static const char* SETTING_VIDEOLIBRARY_IGNOREVIDEOEXTRAS;
  static const char* SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER;
  static const char* SETTING_LOCALE_AUDIOLANGUAGE;
  static const char* SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG;
  static const char* SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM;
  static const char* SETTING_VIDEOPLAYER_SEEKSTEPS;
  static const char* SETTING_VIDEOPLAYER_SEEKDELAY;
  static const char* SETTING_VIDEOPLAYER_ERRORINASPECT;
  static const char* SETTING_VIDEOPLAYER_RENDERMETHOD;
  static const char* SETTING_VIDEOPLAYER_DEFAULTPLAYER;
  static const char* SETTING_MYVIDEOS_SELECTACTION;
  static const char* SETTING_MYVIDEOS_SELECTDEFAULTVERSION;
  static const char* SETTING_MYVIDEOS_PLAYACTION;
  static const char* SETTING_MYVIDEOS_USETAGS;
  static const char* SETTING_MYVIDEOS_EXTRACTFLAGS;
  static const char* SETTING_MYVIDEOS_REPLACELABELS;
  static const char* SETTING_MYVIDEOS_EXTRACTTHUMB;
  static const char* SETTING_MYVIDEOS_STACKVIDEOS;
  static const char* SETTING_LOCALE_SUBTITLELANGUAGE;
  static const char* SETTING_SUBTITLES_FONTSIZE;
  static const char* SETTING_SUBTITLES_STYLE;
  static const char* SETTING_SUBTITLES_CHARSET;
  static const char* SETTING_SUBTITLES_LANGUAGES;
  static const char* SETTING_SUBTITLES_STORAGEMODE;
  static const char* SETTING_SUBTITLES_CUSTOMPATH;
  static const char* SETTING_SUBTITLES_PAUSEONSEARCH;
  static const char* SETTING_SUBTITLES_DOWNLOADFIRST;
  static const char* SETTING_SUBTITLES_TV;
  static const char* SETTING_SUBTITLES_MOVIE;
  static const char* SETTING_DVDS_PLAYERREGION;
  static const char* SETTING_DVDS_AUTOMENU;
  static const char* SETTING_SCRAPERS_MOVIESDEFAULT;
  static const char* SETTING_SCRAPERS_TVSHOWSDEFAULT;
  static const char* SETTING_SCRAPERS_MUSICVIDEOSDEFAULT;
  static const char* SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS;
  static const char* SETTING_MUSICLIBRARY_SHOWDISCS;
  static const char* SETTING_MUSICLIBRARY_USEORIGINALDATE;
  static const char* SETTING_MUSICLIBRARY_USEARTISTSORTNAME;
  static const char* SETTING_MUSICLIBRARY_DOWNLOADINFO;
  static const char* SETTING_MUSICLIBRARY_ARTISTSFOLDER;
  static const char* SETTING_MUSICLIBRARY_PREFERONLINEALBUMART;
  static const char* SETTING_MUSICLIBRARY_ARTWORKLEVEL;
  static const char* SETTING_MUSICLIBRARY_USEALLLOCALART;
  static const char* SETTING_MUSICLIBRARY_USEALLREMOTEART;
  static const char* SETTING_MUSICLIBRARY_ARTISTART_WHITELIST;
  static const char* SETTING_MUSICLIBRARY_ALBUMART_WHITELIST;
  static const char* SETTING_MUSICLIBRARY_MUSICTHUMBS;
  static const char* SETTING_MUSICLIBRARY_ALBUMSSCRAPER;
  static const char* SETTING_MUSICLIBRARY_ARTISTSSCRAPER;
  static const char* SETTING_MUSICLIBRARY_OVERRIDETAGS;
  static const char* SETTING_MUSICLIBRARY_SHOWALLITEMS;
  static const char* SETTING_MUSICLIBRARY_UPDATEONSTARTUP;
  static const char* SETTING_MUSICLIBRARY_BACKGROUNDUPDATE;
  static const char* SETTING_MUSICLIBRARY_CLEANUP;
  static const char* SETTING_MUSICLIBRARY_EXPORT;
  static const char* SETTING_MUSICLIBRARY_EXPORT_FILETYPE;
  static const char* SETTING_MUSICLIBRARY_EXPORT_FOLDER;
  static const char* SETTING_MUSICLIBRARY_EXPORT_ITEMS;
  static const char* SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED;
  static const char* SETTING_MUSICLIBRARY_EXPORT_OVERWRITE;
  static const char* SETTING_MUSICLIBRARY_EXPORT_ARTWORK;
  static const char* SETTING_MUSICLIBRARY_EXPORT_SKIPNFO;
  static const char* SETTING_MUSICLIBRARY_IMPORT;
  static const char* SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM;
  static const char* SETTING_MUSICPLAYER_QUEUEBYDEFAULT;
  static const char* SETTING_MUSICPLAYER_SEEKSTEPS;
  static const char* SETTING_MUSICPLAYER_SEEKDELAY;
  static const char* SETTING_MUSICPLAYER_REPLAYGAINTYPE;
  static const char* SETTING_MUSICPLAYER_REPLAYGAINPREAMP;
  static const char* SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP;
  static const char* SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING;
  static const char* SETTING_MUSICPLAYER_CROSSFADE;
  static const char* SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS;
  static const char* SETTING_MUSICPLAYER_VISUALISATION;
  static const char* SETTING_MUSICPLAYER_DEFAULTPLAYER;
  static const char* SETTING_MUSICFILES_SELECTACTION;
  static const char* SETTING_MUSICFILES_USETAGS;
  static const char* SETTING_MUSICFILES_TRACKFORMAT;
  static const char* SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT;
  static const char* SETTING_MUSICFILES_LIBRARYTRACKFORMAT;
  static const char* SETTING_MUSICFILES_FINDREMOTETHUMBS;
  static const char* SETTING_AUDIOCDS_AUTOACTION;
  static const char* SETTING_AUDIOCDS_USECDDB;
  static const char* SETTING_AUDIOCDS_RECORDINGPATH;
  static const char* SETTING_AUDIOCDS_TRACKPATHFORMAT;
  static const char* SETTING_AUDIOCDS_ENCODER;
  static const char* SETTING_AUDIOCDS_SETTINGS;
  static const char* SETTING_AUDIOCDS_EJECTONRIP;
  static const char* SETTING_MYMUSIC_SONGTHUMBINVIS;
  static const char* SETTING_MYMUSIC_DEFAULTLIBVIEW;
  static const char* SETTING_PICTURES_USETAGS;
  static const char* SETTING_PICTURES_GENERATETHUMBS;
  static const char* SETTING_PICTURES_SHOWVIDEOS;
  static const char* SETTING_PICTURES_DISPLAYRESOLUTION;
  static const char* SETTING_SLIDESHOW_STAYTIME;
  static const char* SETTING_SLIDESHOW_DISPLAYEFFECTS;
  static const char* SETTING_SLIDESHOW_SHUFFLE;
  static const char* SETTING_WEATHER_CURRENTLOCATION;
  static const char* SETTING_WEATHER_ADDON;
  static const char* SETTING_WEATHER_ADDONSETTINGS;
  static const char* SETTING_SERVICES_DEVICENAME;
  static const char* SETTING_SERVICES_UPNP;
  static const char* SETTING_SERVICES_UPNPSERVER;
  static const char* SETTING_SERVICES_UPNPRENDERER;
  static const char* SETTING_SERVICES_WEBSERVER;
  static const char* SETTING_SERVICES_WEBSERVERPORT;
  static const char* SETTING_SERVICES_WEBSERVERUSERNAME;
  static const char* SETTING_SERVICES_WEBSERVERPASSWORD;
  static const char* SETTING_SERVICES_WEBSKIN;
  static const char* SETTING_SERVICES_ESENABLED;
  static const char* SETTING_SERVICES_ESPORT;
  static const char* SETTING_SERVICES_ESPORTRANGE;
  static const char* SETTING_SERVICES_ESMAXCLIENTS;
  static const char* SETTING_SERVICES_ESALLINTERFACES;
  static const char* SETTING_SERVICES_ESINITIALDELAY;
  static const char* SETTING_SERVICES_ESCONTINUOUSDELAY;
  static const char* SETTING_SMB_WINSSERVER;
  static const char* SETTING_SMB_WORKGROUP;
  static const char* SETTING_VIDEOSCREEN_RESOLUTION;
  static const char* SETTING_VIDEOSCREEN_GUICALIBRATION;
  static const char* SETTING_AUDIOOUTPUT_AC3PASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_DTSPASSTHROUGH;
  static const char* SETTING_NETWORK_USEHTTPPROXY;
  static const char* SETTING_NETWORK_HTTPPROXYTYPE;
  static const char* SETTING_NETWORK_HTTPPROXYSERVER;
  static const char* SETTING_NETWORK_HTTPPROXYPORT;
  static const char* SETTING_NETWORK_HTTPPROXYUSERNAME;
  static const char* SETTING_NETWORK_HTTPPROXYPASSWORD;
  static const char* SETTING_NETWORK_BANDWIDTH;
  static const char* SETTING_POWERMANAGEMENT_SHUTDOWNTIME;
  static const char* SETTING_DEBUG_SHOWLOGINFO;
  static const char* SETTING_DEBUG_EXTRALOGGING;
  static const char* SETTING_DEBUG_SETEXTRALOGLEVEL;
  static const char* SETTING_DEBUG_SCREENSHOTPATH;
  static const char* SETTING_MASTERLOCK_LOCKCODE;
  static const char* SETTING_MASTERLOCK_STARTUPLOCK;
  static const char* SETTING_MASTERLOCK_MAXRETRIES;
  static const char* SETTING_CACHE_HARDDISK;
  static const char* SETTING_CACHEVIDEO_DVDROM;
  static const char* SETTING_CACHEVIDEO_LAN;
  static const char* SETTING_CACHEVIDEO_INTERNET;
  static const char* SETTING_CACHEAUDIO_DVDROM;
  static const char* SETTING_CACHEAUDIO_LAN;
  static const char* SETTING_CACHEAUDIO_INTERNET;
  static const char* SETTING_CACHEDVD_DVDROM;
  static const char* SETTING_CACHEDVD_LAN;
  static const char* SETTING_CACHEUNKNOWN_INTERNET;
  static const char* SETTING_SYSTEM_PLAYLISTSPATH;
  static const char* SETTING_ADDONS_AUTOUPDATES;
  static const char* SETTING_ADDONS_NOTIFICATIONS;
  static const char* SETTING_ADDONS_SHOW_RUNNING;
  static const char* SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES;
  static const char* SETTING_ADDONS_UPDATEMODE;
  static const char* SETTING_ADDONS_MANAGE_DEPENDENCIES;
  static const char* SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES;
  static const char* SETTING_GENERAL_ADDONFOREIGNFILTER;
  static const char* SETTING_GENERAL_ADDONBROKENFILTER;
  static const char* SETTING_SOURCE_VIDEOS;
  static const char* SETTING_SOURCE_MUSIC;
  static const char* SETTING_SOURCE_PICTURES;

  // values for SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES = 0;
  static const int VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES = 1;
  static const int VIDEOLIBRARY_THUMB_SHOW_UNWATCHED_EPISODE = 2;
  // values for SETTING_VIDEOLIBRARY_ARTWORK_LEVEL
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int VIDEOLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_MUSICLIBRARY_ARTWORKLEVEL
  static const int MUSICLIBRARY_ARTWORK_LEVEL_ALL = 0;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_BASIC = 1;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_CUSTOM = 2;
  static const int MUSICLIBRARY_ARTWORK_LEVEL_NONE = 3;

  // values for SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM
  static const int SETTING_AUTOPLAYNEXT_MUSICVIDEOS = 0;
  static const int SETTING_AUTOPLAYNEXT_TVSHOWS = 1;
  static const int SETTING_AUTOPLAYNEXT_EPISODES = 2;
  static const int SETTING_AUTOPLAYNEXT_MOVIES = 3;
  static const int SETTING_AUTOPLAYNEXT_UNCATEGORIZED = 4;

  // values for SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_DOLBY_VISION = 0;
  static const int VIDEOPLAYER_ALLOWED_HDR_TYPE_HDR10PLUS = 1;

  /*!
   \brief Creates a new settings wrapper around a new settings manager.

   For access to the "global" settings wrapper the static GetInstance() method should
   be used.
   */
  CSettings() {}
  virtual ~CSettings() {}

  CSettingsManager* GetSettingsManager() const { return m_settingsManager; }

  // specialization of CSettingsBase
  virtual bool Initialize();

  /*!
   \brief Registers the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void RegisterSubSettings(ISubSettings* subSettings);
  /*!
   \brief Unregisters the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void UnregisterSubSettings(ISubSettings* subSettings);

  // implementations of CSettingsBase
  virtual bool Load();
  virtual bool Save();

  /*!
   \brief Loads setting values from the given (XML) file.

   \param file Path to an XML file containing setting values
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const std::string &file);
  /*!
  \brief Loads setting values from the given XML element.

  \param root XML element containing setting values
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool Load(const TiXmlElement* root);
  /*!
   \brief Loads setting values from the given XML element.

   \param root XML element containing setting values
   \param hide Whether to hide the loaded settings or not
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool LoadHidden(const TiXmlElement *root) { return CSettingsBase::LoadHiddenValuesFromXml(root); }

  /*!
   \brief Saves the setting values to the given (XML) file.

   \param file Path to an XML file
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(const std::string &file);
  /*!
   \brief Saves the setting values to the given XML node.

   \param root XML node
   \return True if the setting values were successfully saved, false otherwise
   */
  virtual bool Save(TiXmlNode* root) const;

  /*!
   \brief Loads the setting being represented by the given XML node with the
   given identifier.

   \param node XML node representing the setting to load
   \param settingId Setting identifier
   \return True if the setting was successfully loaded from the given XML node, false otherwise
   */
  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);

  /*!
   \brief Clears the complete settings.

   This removes all initialized settings, groups, categories and sections and
   returns to the uninitialized state. Any registered callbacks or
   implementations stay registered.
   */
  virtual void Clear();

#ifdef _XBOX
  inline std::string GetPlayerName(const int& player) const
  {
    if (player == 0)
      return "mplayer";
    if (player == 1)
      return "dvdplayer";
    if (player == 2)
      return "paplayer";

    return "";
  }

  inline std::string GetFFmpegDllFolder() const
  {
    std::string folder = "Q:\\system\\players\\dvdplayer\\";
    if (GetBool("videoplayer.allcodecs"))
      folder += "full\\";
    return folder;
  }

  inline std::string GetDefaultVideoPlayerName() const
  {
    return GetPlayerName(GetInt(CSettings::SETTING_VIDEOPLAYER_DEFAULTPLAYER));
  }

  inline std::string GetDefaultAudioPlayerName() const
  {
    return GetPlayerName(GetInt(CSettings::SETTING_MUSICPLAYER_DEFAULTPLAYER));
  }
#endif

protected:
  // specializations of CSettingsBase
  virtual void InitializeSettingTypes();
  virtual void InitializeControls();
  virtual void InitializeOptionFillers();
  virtual void UninitializeOptionFillers();
  virtual void InitializeConditions();
  virtual void UninitializeConditions();
  virtual void InitializeVisibility();
  virtual void InitializeDefaults();
  virtual void InitializeISettingsHandlers();
  virtual void UninitializeISettingsHandlers();
  virtual void InitializeISubSettings();
  virtual void UninitializeISubSettings();
  virtual void InitializeISettingCallbacks();
  virtual void UninitializeISettingCallbacks();

  // implementation of CSettingsBase
  virtual bool InitializeDefinitions();

private:
  CSettings(const CSettings&);
  CSettings const& operator=(CSettings const&);

  bool Load(const TiXmlElement* root, bool& updated);

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode* settings);

  bool Initialize(const std::string &file);
  bool Reset();

  std::set<ISubSettings*> m_subSettings;
};
