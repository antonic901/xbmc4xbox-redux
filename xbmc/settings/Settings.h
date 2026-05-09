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
  static const char* SETTING_LOOKANDFEEL_STARTUPACTION;
  static const char* SETTING_LOOKANDFEEL_STARTUPWINDOW;
  static const char* SETTING_LOOKANDFEEL_SOUNDSKIN;
  static const char* SETTING_LOOKANDFEEL_ENABLERSSFEEDS;
  static const char* SETTING_LOOKANDFEEL_RSSEDIT;
  static const char* SETTING_LOOKANDFEEL_STEREOSTRENGTH;
  static const char* SETTING_LOCALE_LANGUAGE;
  static const char* SETTING_LOCALE_COUNTRY;
  static const char* SETTING_LOCALE_CHARSET;
  static const char* SETTING_LOCALE_KEYBOARDLAYOUTS;
  static const char* SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT;
  static const char* SETTING_LOCALE_TIMEZONECOUNTRY;
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
  static const char* SETTING_WINDOW_WIDTH;
  static const char* SETTING_WINDOW_HEIGHT;
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
  static const char* SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE;
  static const char* SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK;
  static const char* SETTING_VIDEOPLAYER_ERRORINASPECT;
  static const char* SETTING_VIDEOPLAYER_STRETCH43;
  static const char* SETTING_VIDEOPLAYER_TELETEXTENABLED;
  static const char* SETTING_VIDEOPLAYER_TELETEXTSCALE;
  static const char* SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE;
  static const char* SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP;
  static const char* SETTING_VIDEOPLAYER_RENDERMETHOD;
  static const char* SETTING_VIDEOPLAYER_HQSCALERS;
  static const char* SETTING_VIDEOPLAYER_USESUPERRESOLUTION;
  static const char* SETTING_VIDEOPLAYER_HIGHPRECISIONPROCESSING;
  static const char* SETTING_VIDEOPLAYER_USEMEDIACODEC;
  static const char* SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE;
  static const char* SETTING_VIDEOPLAYER_USEVDPAU;
  static const char* SETTING_VIDEOPLAYER_USEVDPAUMIXER;
  static const char* SETTING_VIDEOPLAYER_USEVDPAUMPEG2;
  static const char* SETTING_VIDEOPLAYER_USEVDPAUMPEG4;
  static const char* SETTING_VIDEOPLAYER_USEVDPAUVC1;
  static const char* SETTING_VIDEOPLAYER_USEDXVA2;
  static const char* SETTING_VIDEOPLAYER_USEVTB;
  static const char* SETTING_VIDEOPLAYER_USEPRIMEDECODER;
  static const char* SETTING_VIDEOPLAYER_USESTAGEFRIGHT;
  static const char* SETTING_VIDEOPLAYER_LIMITGUIUPDATE;
  static const char* SETTING_VIDEOPLAYER_SUPPORTMVC;
  static const char* SETTING_VIDEOPLAYER_CONVERTDOVI;
  static const char* SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS;
  static const char* SETTING_VIDEOPLAYER_DEFAULTPLAYER;
  static const char* SETTING_MYVIDEOS_SELECTACTION;
  static const char* SETTING_MYVIDEOS_SELECTDEFAULTVERSION;
  static const char* SETTING_MYVIDEOS_PLAYACTION;
  static const char* SETTING_MYVIDEOS_USETAGS;
  static const char* SETTING_MYVIDEOS_EXTRACTFLAGS;
  static const char* SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS;
  static const char* SETTING_MYVIDEOS_REPLACELABELS;
  static const char* SETTING_MYVIDEOS_EXTRACTTHUMB;
  static const char* SETTING_MYVIDEOS_STACKVIDEOS;
  static const char* SETTING_LOCALE_SUBTITLELANGUAGE;
  static const char* SETTING_SUBTITLES_PARSECAPTIONS;
  static const char* SETTING_SUBTITLES_CAPTIONSALIGN;
  static const char* SETTING_SUBTITLES_ALIGN;
  static const char* SETTING_SUBTITLES_STEREOSCOPICDEPTH;
  static const char* SETTING_SUBTITLES_FONTNAME;
  static const char* SETTING_SUBTITLES_FONTSIZE;
  static const char* SETTING_SUBTITLES_STYLE;
  static const char* SETTING_SUBTITLES_COLOR;
  static const char* SETTING_SUBTITLES_BORDERSIZE;
  static const char* SETTING_SUBTITLES_BORDERCOLOR;
  static const char* SETTING_SUBTITLES_OPACITY;
  static const char* SETTING_SUBTITLES_BLUR;
  static const char* SETTING_SUBTITLES_BACKGROUNDTYPE;
  static const char* SETTING_SUBTITLES_SHADOWCOLOR;
  static const char* SETTING_SUBTITLES_SHADOWOPACITY;
  static const char* SETTING_SUBTITLES_SHADOWSIZE;
  static const char* SETTING_SUBTITLES_BGCOLOR;
  static const char* SETTING_SUBTITLES_BGOPACITY;
  static const char* SETTING_SUBTITLES_MARGINVERTICAL;
  static const char* SETTING_SUBTITLES_CHARSET;
  static const char* SETTING_SUBTITLES_OVERRIDEFONTS;
  static const char* SETTING_SUBTITLES_OVERRIDESTYLES;
  static const char* SETTING_SUBTITLES_LANGUAGES;
  static const char* SETTING_SUBTITLES_STORAGEMODE;
  static const char* SETTING_SUBTITLES_CUSTOMPATH;
  static const char* SETTING_SUBTITLES_PAUSEONSEARCH;
  static const char* SETTING_SUBTITLES_DOWNLOADFIRST;
  static const char* SETTING_SUBTITLES_TV;
  static const char* SETTING_SUBTITLES_MOVIE;
  static const char* SETTING_DVDS_AUTORUN;
  static const char* SETTING_DVDS_PLAYERREGION;
  static const char* SETTING_DVDS_AUTOMENU;
  static const char* SETTING_DISC_PLAYBACK;
  static const char* SETTING_BLURAY_PLAYERREGION;
  static const char* SETTING_ACCESSIBILITY_AUDIOVISUAL;
  static const char* SETTING_ACCESSIBILITY_AUDIOHEARING;
  static const char* SETTING_ACCESSIBILITY_SUBHEARING;
  static const char* SETTING_SCRAPERS_MOVIESDEFAULT;
  static const char* SETTING_SCRAPERS_TVSHOWSDEFAULT;
  static const char* SETTING_SCRAPERS_MUSICVIDEOSDEFAULT;
  static const char* SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL;
  static const char* SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER;
  static const char* SETTING_PVRMANAGER_BACKENDCHANNELORDER;
  static const char* SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS;
  static const char* SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS;
  static const char* SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE;
  static const char* SETTING_PVRMANAGER_CLIENTPRIORITIES;
  static const char* SETTING_PVRMANAGER_CHANNELMANAGER;
  static const char* SETTING_PVRMANAGER_GROUPMANAGER;
  static const char* SETTING_PVRMANAGER_CHANNELSCAN;
  static const char* SETTING_PVRMANAGER_RESETDB;
  static const char* SETTING_PVRMANAGER_ADDONS;
  static const char* SETTING_PVRMENU_DISPLAYCHANNELINFO;
  static const char* SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH;
  static const char* SETTING_PVRMENU_ICONPATH;
  static const char* SETTING_PVRMENU_SEARCHICONS;
  static const char* SETTING_EPG_PAST_DAYSTODISPLAY;
  static const char* SETTING_EPG_FUTURE_DAYSTODISPLAY;
  static const char* SETTING_EPG_SELECTACTION;
  static const char* SETTING_EPG_HIDENOINFOAVAILABLE;
  static const char* SETTING_EPG_EPGUPDATE;
  static const char* SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV;
  static const char* SETTING_EPG_RESETEPG;
  static const char* SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES;
  static const char* SETTING_PVRPLAYBACK_SIGNALQUALITY;
  static const char* SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH;
  static const char* SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT;
  static const char* SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED;
  static const char* SETTING_PVRPLAYBACK_FPS;
  static const char* SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME;
  static const char* SETTING_PVRRECORD_INSTANTRECORDACTION;
  static const char* SETTING_PVRRECORD_INSTANTRECORDTIME;
  static const char* SETTING_PVRRECORD_MARGINSTART;
  static const char* SETTING_PVRRECORD_MARGINEND;
  static const char* SETTING_PVRRECORD_TIMERNOTIFICATIONS;
  static const char* SETTING_PVRRECORD_GROUPRECORDINGS;
  static const char* SETTING_PVRREMINDERS_AUTOCLOSEDELAY;
  static const char* SETTING_PVRREMINDERS_AUTORECORD;
  static const char* SETTING_PVRREMINDERS_AUTOSWITCH;
  static const char* SETTING_PVRPOWERMANAGEMENT_ENABLED;
  static const char* SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME;
  static const char* SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD;
  static const char* SETTING_PVRPOWERMANAGEMENT_PREWAKEUP;
  static const char* SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP;
  static const char* SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME;
  static const char* SETTING_PVRPARENTAL_ENABLED;
  static const char* SETTING_PVRPARENTAL_PIN;
  static const char* SETTING_PVRPARENTAL_DURATION;
  static const char* SETTING_PVRCLIENT_MENUHOOK;
  static const char* SETTING_PVRTIMERS_HIDEDISABLEDTIMERS;
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
  static const char* SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING;
  static const char* SETTING_WEATHER_CURRENTLOCATION;
  static const char* SETTING_WEATHER_ADDON;
  static const char* SETTING_WEATHER_ADDONSETTINGS;
  static const char* SETTING_SERVICES_DEVICENAME;
  static const char* SETTING_SERVICES_DEVICEUUID;
  static const char* SETTING_SERVICES_UPNP;
  static const char* SETTING_SERVICES_UPNPSERVER;
  static const char* SETTING_SERVICES_UPNPANNOUNCE;
  static const char* SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES;
  static const char* SETTING_SERVICES_UPNPCONTROLLER;
  static const char* SETTING_SERVICES_UPNPPLAYERVOLUMESYNC;
  static const char* SETTING_SERVICES_UPNPRENDERER;
  static const char* SETTING_SERVICES_WEBSERVER;
  static const char* SETTING_SERVICES_WEBSERVERPORT;
  static const char* SETTING_SERVICES_WEBSERVERAUTHENTICATION;
  static const char* SETTING_SERVICES_WEBSERVERUSERNAME;
  static const char* SETTING_SERVICES_WEBSERVERPASSWORD;
  static const char* SETTING_SERVICES_WEBSERVERSSL;
  static const char* SETTING_SERVICES_WEBSKIN;
  static const char* SETTING_SERVICES_ESENABLED;
  static const char* SETTING_SERVICES_ESPORT;
  static const char* SETTING_SERVICES_ESPORTRANGE;
  static const char* SETTING_SERVICES_ESMAXCLIENTS;
  static const char* SETTING_SERVICES_ESALLINTERFACES;
  static const char* SETTING_SERVICES_ESINITIALDELAY;
  static const char* SETTING_SERVICES_ESCONTINUOUSDELAY;
  static const char* SETTING_SERVICES_ZEROCONF;
  static const char* SETTING_SERVICES_AIRPLAY;
  static const char* SETTING_SERVICES_AIRPLAYVOLUMECONTROL;
  static const char* SETTING_SERVICES_USEAIRPLAYPASSWORD;
  static const char* SETTING_SERVICES_AIRPLAYPASSWORD;
  static const char* SETTING_SERVICES_AIRPLAYVIDEOSUPPORT;
  static const char* SETTING_SMB_WINSSERVER;
  static const char* SETTING_SMB_WORKGROUP;
  static const char* SETTING_SMB_MINPROTOCOL;
  static const char* SETTING_SMB_MAXPROTOCOL;
  static const char* SETTING_SMB_LEGACYSECURITY;
  static const char* SETTING_SMB_CHUNKSIZE;
  static const char* SETTING_SERVICES_WSDISCOVERY;
  static const char* SETTING_VIDEOSCREEN_MONITOR;
  static const char* SETTING_VIDEOSCREEN_SCREEN;
  static const char* SETTING_VIDEOSCREEN_WHITELIST;
  static const char* SETTING_VIDEOSCREEN_RESOLUTION;
  static const char* SETTING_VIDEOSCREEN_SCREENMODE;
  static const char* SETTING_VIDEOSCREEN_FAKEFULLSCREEN;
  static const char* SETTING_VIDEOSCREEN_BLANKDISPLAYS;
  static const char* SETTING_VIDEOSCREEN_STEREOSCOPICMODE;
  static const char* SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE;
  static const char* SETTING_VIDEOSCREEN_NOOFBUFFERS;
  static const char* SETTING_VIDEOSCREEN_3DLUT;
  static const char* SETTING_VIDEOSCREEN_DISPLAYPROFILE;
  static const char* SETTING_VIDEOSCREEN_GUICALIBRATION;
  static const char* SETTING_VIDEOSCREEN_TESTPATTERN;
  static const char* SETTING_VIDEOSCREEN_LIMITEDRANGE;
  static const char* SETTING_VIDEOSCREEN_FRAMEPACKING;
  static const char* SETTING_VIDEOSCREEN_10BITSURFACES;
  static const char* SETTING_VIDEOSCREEN_USESYSTEMSDRPEAKLUMINANCE;
  static const char* SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE;
  static const char* SETTING_VIDEOSCREEN_DITHER;
  static const char* SETTING_VIDEOSCREEN_DITHERDEPTH;
  static const char* SETTING_AUDIOOUTPUT_AUDIODEVICE;
  static const char* SETTING_AUDIOOUTPUT_CHANNELS;
  static const char* SETTING_AUDIOOUTPUT_CONFIG;
  static const char* SETTING_AUDIOOUTPUT_SAMPLERATE;
  static const char* SETTING_AUDIOOUTPUT_STEREOUPMIX;
  static const char* SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME;
  static const char* SETTING_AUDIOOUTPUT_PROCESSQUALITY;
  static const char* SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD;
  static const char* SETTING_AUDIOOUTPUT_STREAMSILENCE;
  static const char* SETTING_AUDIOOUTPUT_STREAMNOISE;
  static const char* SETTING_AUDIOOUTPUT_GUISOUNDMODE;
  static const char* SETTING_AUDIOOUTPUT_GUISOUNDVOLUME;
  static const char* SETTING_AUDIOOUTPUT_PASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE;
  static const char* SETTING_AUDIOOUTPUT_AC3PASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_AC3TRANSCODE;
  static const char* SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_DTSPASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH;
  static const char* SETTING_AUDIOOUTPUT_DTSHDCOREFALLBACK;
  static const char* SETTING_AUDIOOUTPUT_VOLUMESTEPS;
  static const char* SETTING_INPUT_PERIPHERALS;
  static const char* SETTING_INPUT_PERIPHERALLIBRARIES;
  static const char* SETTING_INPUT_ENABLEMOUSE;
  static const char* SETTING_INPUT_ASKNEWCONTROLLERS;
  static const char* SETTING_INPUT_CONTROLLERCONFIG;
  static const char* SETTING_INPUT_RUMBLENOTIFY;
  static const char* SETTING_INPUT_TESTRUMBLE;
  static const char* SETTING_INPUT_CONTROLLERPOWEROFF;
  static const char* SETTING_INPUT_APPLEREMOTEMODE;
  static const char* SETTING_INPUT_APPLEREMOTEALWAYSON;
  static const char* SETTING_INPUT_APPLEREMOTESEQUENCETIME;
  static const char* SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED;
  static const char* SETTING_INPUT_SIRIREMOTEIDLETIME;
  static const char* SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY;
  static const char* SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY;
  static const char* SETTING_INPUT_TVOSUSEKODIKEYBOARD;
  static const char* SETTING_NETWORK_USEHTTPPROXY;
  static const char* SETTING_NETWORK_HTTPPROXYTYPE;
  static const char* SETTING_NETWORK_HTTPPROXYSERVER;
  static const char* SETTING_NETWORK_HTTPPROXYPORT;
  static const char* SETTING_NETWORK_HTTPPROXYUSERNAME;
  static const char* SETTING_NETWORK_HTTPPROXYPASSWORD;
  static const char* SETTING_NETWORK_BANDWIDTH;
  static const char* SETTING_POWERMANAGEMENT_DISPLAYSOFF;
  static const char* SETTING_POWERMANAGEMENT_SHUTDOWNTIME;
  static const char* SETTING_POWERMANAGEMENT_SHUTDOWNSTATE;
  static const char* SETTING_POWERMANAGEMENT_WAKEONACCESS;
  static const char* SETTING_POWERMANAGEMENT_WAITFORNETWORK;
  static const char* SETTING_DEBUG_SHOWLOGINFO;
  static const char* SETTING_DEBUG_EXTRALOGGING;
  static const char* SETTING_DEBUG_SETEXTRALOGLEVEL;
  static const char* SETTING_DEBUG_SCREENSHOTPATH;
  static const char* SETTING_DEBUG_SHARE_LOG;
  static const char* SETTING_EVENTLOG_ENABLED;
  static const char* SETTING_EVENTLOG_ENABLED_NOTIFICATIONS;
  static const char* SETTING_EVENTLOG_SHOW;
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
  static const char* SETTING_FILECACHE_BUFFERMODE;
  static const char* SETTING_FILECACHE_MEMORYSIZE; // in MBytes
  static const char* SETTING_FILECACHE_READFACTOR; // as integer (x100)
  static const char* SETTING_FILECACHE_CHUNKSIZE; // in Bytes

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

  // overwrite (not override) from CSettingsBase
  bool GetBool(const std::string& id) const;

  /*!
   \brief Clears the complete settings.

   This removes all initialized settings, groups, categories and sections and
   returns to the uninitialized state. Any registered callbacks or
   implementations stay registered.
   */
  virtual void Clear();

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
