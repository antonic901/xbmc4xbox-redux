/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Settings.h"

#include "Autorun.h"
#include "GUIPassword.h"
#include "LangInfo.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Skin.h"
#include "cores/VideoRenderers/XBoxRenderer.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "input/KeyboardLayoutManager.h"

#include "karaoke/CdgParser.h"
#include "network/NetworkServices.h"
#include "network/upnp/UPnPSettings.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "profiles/ProfileManager.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/lib/SettingsManager.h"
#include "utils/CharsetConverter.h"
#include "utils/FanController.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#include "platform/xbox/XBAudioConfig.h"
#include "platform/xbox/XBTimeZone.h"
#include "platform/xbox/XBVideoConfig.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"

using namespace KODI;
using namespace XFILE;

const char* CSettings::SETTING_LOOKANDFEEL_SKIN = "lookandfeel.skin";
const char* CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS = "lookandfeel.skinsettings";
const char* CSettings::SETTING_LOOKANDFEEL_SKINTHEME = "lookandfeel.skintheme";
const char* CSettings::SETTING_LOOKANDFEEL_SKINCOLORS = "lookandfeel.skincolors";
const char* CSettings::SETTING_LOOKANDFEEL_FONT = "lookandfeel.font";
const char* CSettings::SETTING_LOOKANDFEEL_SKINZOOM = "lookandfeel.skinzoom";
const char* CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW = "lookandfeel.startupwindow";
const char* CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN = "lookandfeel.soundskin";
const char* CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS = "lookandfeel.enablerssfeeds";
const char* CSettings::SETTING_LOOKANDFEEL_RSSEDIT = "lookandfeel.rssedit";
const char* CSettings::SETTING_LOCALE_LANGUAGE = "locale.language";
const char* CSettings::SETTING_LOCALE_COUNTRY = "locale.country";
const char* CSettings::SETTING_LOCALE_CHARSET = "locale.charset";
const char* CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS = "locale.keyboardlayouts";
const char* CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT = "locale.activekeyboardlayout";
const char* CSettings::SETTING_LOCALE_TIMEZONE = "locale.timezone";
const char* CSettings::SETTING_LOCALE_SHORTDATEFORMAT = "locale.shortdateformat";
const char* CSettings::SETTING_LOCALE_LONGDATEFORMAT = "locale.longdateformat";
const char* CSettings::SETTING_LOCALE_TIMEFORMAT = "locale.timeformat";
const char* CSettings::SETTING_LOCALE_USE24HOURCLOCK = "locale.use24hourclock";
const char* CSettings::SETTING_LOCALE_TEMPERATUREUNIT = "locale.temperatureunit";
const char* CSettings::SETTING_LOCALE_SPEEDUNIT = "locale.speedunit";
const char* CSettings::SETTING_LOCALE_USE_DST = "locale.usedst";
const char* CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS = "filelists.showparentdiritems";
const char* CSettings::SETTING_FILELISTS_SHOWEXTENSIONS = "filelists.showextensions";
const char* CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING = "filelists.ignorethewhensorting";
const char* CSettings::SETTING_FILELISTS_ALLOWFILEDELETION = "filelists.allowfiledeletion";
const char* CSettings::SETTING_FILELISTS_SHOWADDSOURCEBUTTONS = "filelists.showaddsourcebuttons";
const char* CSettings::SETTING_FILELISTS_SHOWHIDDEN = "filelists.showhidden";
const char* CSettings::SETTING_SCREENSAVER_MODE = "screensaver.mode";
const char* CSettings::SETTING_SCREENSAVER_SETTINGS = "screensaver.settings";
const char* CSettings::SETTING_SCREENSAVER_PREVIEW = "screensaver.preview";
const char* CSettings::SETTING_SCREENSAVER_TIME = "screensaver.time";
const char* CSettings::SETTING_SCREENSAVER_DISABLEFORAUDIO = "screensaver.disableforaudio";
const char* CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE = "screensaver.usedimonpause";
const char* CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS = "videolibrary.showunwatchedplots";
const char* CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS = "videolibrary.actorthumbs";
const char* CSettings::SETTING_MYVIDEOS_FLATTEN = "myvideos.flatten";
const char* CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS = "videolibrary.flattentvshows";
const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWSSELECTFIRSTUNWATCHEDITEM = "videolibrary.tvshowsselectfirstunwatcheditem";
const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWSINCLUDEALLSEASONSANDSPECIALS = "videolibrary.tvshowsincludeallseasonsandspecials";
const char* CSettings::SETTING_VIDEOLIBRARY_SHOWALLITEMS = "videolibrary.showallitems";
const char* CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS = "videolibrary.groupmoviesets";
const char* CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS = "videolibrary.groupsingleitemsets";
const char* CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP = "videolibrary.updateonstartup";
const char* CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE = "videolibrary.backgroundupdate";
const char* CSettings::SETTING_VIDEOLIBRARY_CLEANUP = "videolibrary.cleanup";
const char* CSettings::SETTING_VIDEOLIBRARY_EXPORT = "videolibrary.export";
const char* CSettings::SETTING_VIDEOLIBRARY_IMPORT = "videolibrary.import";
const char* CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS = "videolibrary.showemptytvshows";
const char* CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER = "videolibrary.moviesetsfolder";
const char* CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL = "videolibrary.artworklevel";
const char* CSettings::SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST = "videolibrary.movieartwhitelist";
const char* CSettings::SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST = "videolibrary.tvshowartwhitelist";
const char* CSettings::SETTING_VIDEOLIBRARY_EPISODEART_WHITELIST = "videolibrary.episodeartwhitelist";
const char* CSettings::SETTING_VIDEOLIBRARY_MUSICVIDEOART_WHITELIST = "videolibrary.musicvideoartwhitelist";
const char* CSettings::SETTING_VIDEOLIBRARY_SHOWPERFORMERS = "videolibrary.musicvideosallperformers";
const char* CSettings::SETTING_VIDEOLIBRARY_IGNOREVIDEOVERSIONS = "videolibrary.ignorevideoversions";
const char* CSettings::SETTING_VIDEOLIBRARY_IGNOREVIDEOEXTRAS = "videolibrary.ignorevideoextras";
const char* CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER = "videolibrary.showvideoversionsasfolder";
const char* CSettings::SETTING_LOCALE_AUDIOLANGUAGE = "locale.audiolanguage";
const char* CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG = "videoplayer.preferdefaultflag";
const char* CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM = "videoplayer.autoplaynextitem";
const char* CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS = "videoplayer.seeksteps";
const char* CSettings::SETTING_VIDEOPLAYER_SEEKDELAY = "videoplayer.seekdelay";
const char* CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT = "videoplayer.errorinaspect";
const char* CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD = "videoplayer.rendermethod";
const char* CSettings::SETTING_VIDEOPLAYER_DEFAULTPLAYER = "videoplayer.defaultplayer";
const char* CSettings::SETTING_VIDEOPLAYER_SOFTEN = "videoplayer.soften";
const char* CSettings::SETTING_VIDEOPLAYER_FLICKER = "videoplayer.flicker";
const char* CSettings::SETTING_MYVIDEOS_SELECTACTION = "myvideos.selectaction";
const char* CSettings::SETTING_MYVIDEOS_SELECTDEFAULTVERSION = "myvideos.selectdefaultversion";
const char* CSettings::SETTING_MYVIDEOS_PLAYACTION = "myvideos.playaction";
const char* CSettings::SETTING_MYVIDEOS_USETAGS = "myvideos.usetags";
const char* CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS = "myvideos.extractflags";
const char* CSettings::SETTING_MYVIDEOS_EXTRACTCHAPTERTHUMBS = "myvideos.extractchapterthumbs";
const char* CSettings::SETTING_MYVIDEOS_REPLACELABELS = "myvideos.replacelabels";
const char* CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB = "myvideos.extractthumb";
const char* CSettings::SETTING_MYVIDEOS_STACKVIDEOS = "myvideos.stackvideos";
const char* CSettings::SETTING_LOCALE_SUBTITLELANGUAGE = "locale.subtitlelanguage";
const char* CSettings::SETTING_SUBTITLES_FONTSIZE = "subtitles.fontsize";
const char* CSettings::SETTING_SUBTITLES_STYLE = "subtitles.style";
const char* CSettings::SETTING_SUBTITLES_CHARSET = "subtitles.charset";
const char* CSettings::SETTING_SUBTITLES_LANGUAGES = "subtitles.languages";
const char* CSettings::SETTING_SUBTITLES_STORAGEMODE = "subtitles.storagemode";
const char* CSettings::SETTING_SUBTITLES_CUSTOMPATH = "subtitles.custompath";
const char* CSettings::SETTING_SUBTITLES_PAUSEONSEARCH = "subtitles.pauseonsearch";
const char* CSettings::SETTING_SUBTITLES_DOWNLOADFIRST = "subtitles.downloadfirst";
const char* CSettings::SETTING_SUBTITLES_TV = "subtitles.tv";
const char* CSettings::SETTING_SUBTITLES_MOVIE = "subtitles.movie";
const char* CSettings::SETTING_DVDS_PLAYERREGION = "dvds.playerregion";
const char* CSettings::SETTING_DVDS_AUTOMENU = "dvds.automenu";
const char* CSettings::SETTING_SCRAPERS_MOVIESDEFAULT = "scrapers.moviesdefault";
const char* CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT = "scrapers.tvshowsdefault";
const char* CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT = "scrapers.musicvideosdefault";
const char* CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS = "musiclibrary.showcompilationartists";
const char* CSettings::SETTING_MUSICLIBRARY_SHOWDISCS = "musiclibrary.showdiscs";
const char* CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE = "musiclibrary.useoriginaldate";
const char* CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME = "musiclibrary.useartistsortname";
const char* CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO = "musiclibrary.downloadinfo";
const char* CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER = "musiclibrary.artistsfolder";
const char* CSettings::SETTING_MUSICLIBRARY_PREFERONLINEALBUMART = "musiclibrary.preferonlinealbumart";
const char* CSettings::SETTING_MUSICLIBRARY_ARTWORKLEVEL = "musiclibrary.artworklevel";
const char* CSettings::SETTING_MUSICLIBRARY_USEALLLOCALART = "musiclibrary.usealllocalart";
const char* CSettings::SETTING_MUSICLIBRARY_USEALLREMOTEART = "musiclibrary.useallremoteart";
const char* CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST = "musiclibrary.artistartwhitelist";
const char* CSettings::SETTING_MUSICLIBRARY_ALBUMART_WHITELIST = "musiclibrary.albumartwhitelist";
const char* CSettings::SETTING_MUSICLIBRARY_MUSICTHUMBS = "musiclibrary.musicthumbs";
const char* CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER = "musiclibrary.albumsscraper";
const char* CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER = "musiclibrary.artistsscraper";
const char* CSettings::SETTING_MUSICLIBRARY_OVERRIDETAGS = "musiclibrary.overridetags";
const char* CSettings::SETTING_MUSICLIBRARY_SHOWALLITEMS = "musiclibrary.showallitems";
const char* CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP = "musiclibrary.updateonstartup";
const char* CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE = "musiclibrary.backgroundupdate";
const char* CSettings::SETTING_MUSICLIBRARY_CLEANUP = "musiclibrary.cleanup";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT = "musiclibrary.export";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE = "musiclibrary.exportfiletype";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER = "musiclibrary.exportfolder";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS = "musiclibrary.exportitems";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED = "musiclibrary.exportunscraped";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE = "musiclibrary.exportoverwrite";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK = "musiclibrary.exportartwork";
const char* CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO = "musiclibrary.exportskipnfo";
const char* CSettings::SETTING_MUSICLIBRARY_IMPORT = "musiclibrary.import";
const char* CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM = "musicplayer.autoplaynextitem";
const char* CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT = "musicplayer.queuebydefault";
const char* CSettings::SETTING_MUSICPLAYER_SEEKSTEPS = "musicplayer.seeksteps";
const char* CSettings::SETTING_MUSICPLAYER_SEEKDELAY = "musicplayer.seekdelay";
const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE = "musicplayer.replaygaintype";
const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP = "musicplayer.replaygainpreamp";
const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP = "musicplayer.replaygainnogainpreamp";
const char* CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING = "musicplayer.replaygainavoidclipping";
const char* CSettings::SETTING_MUSICPLAYER_CROSSFADE = "musicplayer.crossfade";
const char* CSettings::SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS = "musicplayer.crossfadealbumtracks";
const char* CSettings::SETTING_MUSICPLAYER_VISUALISATION = "musicplayer.visualisation";
const char* CSettings::SETTING_MUSICPLAYER_DEFAULTPLAYER = "musicplayer.defaultplayer";
const char* CSettings::SETTING_MUSICFILES_SELECTACTION = "musicfiles.selectaction";
const char* CSettings::SETTING_MUSICFILES_USETAGS = "musicfiles.usetags";
const char* CSettings::SETTING_MUSICFILES_TRACKFORMAT = "musicfiles.trackformat";
const char* CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT = "musicfiles.nowplayingtrackformat";
const char* CSettings::SETTING_MUSICFILES_LIBRARYTRACKFORMAT = "musicfiles.librarytrackformat";
const char* CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS = "musicfiles.findremotethumbs";
const char* CSettings::SETTING_AUDIOCDS_AUTOACTION = "audiocds.autoaction";
const char* CSettings::SETTING_AUDIOCDS_USECDDB = "audiocds.usecddb";
const char* CSettings::SETTING_AUDIOCDS_RECORDINGPATH = "audiocds.recordingpath";
const char* CSettings::SETTING_AUDIOCDS_TRACKPATHFORMAT = "audiocds.trackpathformat";
const char* CSettings::SETTING_AUDIOCDS_ENCODER = "audiocds.encoder";
const char* CSettings::SETTING_AUDIOCDS_SETTINGS = "audiocds.settings";
const char* CSettings::SETTING_AUDIOCDS_EJECTONRIP = "audiocds.ejectonrip";
const char* CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS = "mymusic.songthumbinvis";
const char* CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW = "mymusic.defaultlibview";
const char* CSettings::SETTING_PICTURES_USETAGS = "pictures.usetags";
const char* CSettings::SETTING_PICTURES_GENERATETHUMBS = "pictures.generatethumbs";
const char* CSettings::SETTING_PICTURES_SHOWVIDEOS = "pictures.showvideos";
const char* CSettings::SETTING_PICTURES_DISPLAYRESOLUTION = "pictures.displayresolution";
const char* CSettings::SETTING_SLIDESHOW_STAYTIME = "slideshow.staytime";
const char* CSettings::SETTING_SLIDESHOW_DISPLAYEFFECTS = "slideshow.displayeffects";
const char* CSettings::SETTING_SLIDESHOW_SHUFFLE = "slideshow.shuffle";
const char* CSettings::SETTING_WEATHER_CURRENTLOCATION = "weather.currentlocation";
const char* CSettings::SETTING_WEATHER_ADDON = "weather.addon";
const char* CSettings::SETTING_WEATHER_ADDONSETTINGS = "weather.addonsettings";
const char* CSettings::SETTING_SERVICES_DEVICENAME = "services.devicename";
const char* CSettings::SETTING_SERVICES_UPNP = "services.upnp";
const char* CSettings::SETTING_SERVICES_UPNPSERVER = "services.upnpserver";
const char* CSettings::SETTING_SERVICES_UPNPRENDERER = "services.upnprenderer";
const char* CSettings::SETTING_SERVICES_WEBSERVER = "services.webserver";
const char* CSettings::SETTING_SERVICES_WEBSERVERPORT = "services.webserverport";
const char* CSettings::SETTING_SERVICES_WEBSERVERUSERNAME = "services.webserverusername";
const char* CSettings::SETTING_SERVICES_WEBSERVERPASSWORD = "services.webserverpassword";
const char* CSettings::SETTING_SERVICES_WEBSKIN = "services.webskin";
const char* CSettings::SETTING_SERVICES_ESENABLED = "services.esenabled";
const char* CSettings::SETTING_SERVICES_ESPORT = "services.esport";
const char* CSettings::SETTING_SERVICES_ESPORTRANGE = "services.esportrange";
const char* CSettings::SETTING_SERVICES_ESMAXCLIENTS = "services.esmaxclients";
const char* CSettings::SETTING_SERVICES_ESALLINTERFACES = "services.esallinterfaces";
const char* CSettings::SETTING_SERVICES_ESINITIALDELAY = "services.esinitialdelay";
const char* CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY = "services.escontinuousdelay";
const char* CSettings::SETTING_SERVICES_FTPSERVER = "services.ftpserver";
const char* CSettings::SETTING_SERVICES_FTPSERVER_USER = "services.ftpserveruser";
const char* CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD = "services.ftpserverpassword";
const char* CSettings::SETTING_SERVICES_TIMESERVER = "services.timeserver";
const char* CSettings::SETTING_SERVICES_TIMESERVER_ADDRESS = "services.timeserveraddress";
const char* CSettings::SETTING_SMB_WINSSERVER = "smb.winsserver";
const char* CSettings::SETTING_SMB_WORKGROUP = "smb.workgroup";
const char* CSettings::SETTING_VIDEOSCREEN_RESOLUTION = "videoscreen.resolution";
const char* CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION = "videoscreen.guicalibration";
const char* CSettings::SETTING_VIDEOSCREEN_FLICKERFILTER = "videoscreen.flickerfilter";
const char* CSettings::SETTING_VIDEOSCREEN_SOFTEN = "videoscreen.soften";
const char* CSettings::SETTING_VIDEOSCREEN_ASPECT = "videooutput.aspect";
const char* CSettings::SETTING_VIDEOSCREEN_HD480p = "videooutput.hd480p";
const char* CSettings::SETTING_VIDEOSCREEN_HD720p = "videooutput.hd720p";
const char* CSettings::SETTING_VIDEOSCREEN_HD1080i = "videooutput.hd1080i";
const char* CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH = "audiooutput.passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_AACPASSTHROUGH = "audiooutput.aacpassthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH = "audiooutput.ac3passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH = "audiooutput.dtspassthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_MP1PASSTHROUGH = "audiooutput.mp1passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_MP2PASSTHROUGH = "audiooutput.mp2passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_MP3PASSTHROUGH = "audiooutput.mp3passthrough";
const char* CSettings::SETTING_NETWORK_USEHTTPPROXY = "network.usehttpproxy";
const char* CSettings::SETTING_NETWORK_HTTPPROXYTYPE = "network.httpproxytype";
const char* CSettings::SETTING_NETWORK_HTTPPROXYSERVER = "network.httpproxyserver";
const char* CSettings::SETTING_NETWORK_HTTPPROXYPORT = "network.httpproxyport";
const char* CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME = "network.httpproxyusername";
const char* CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD = "network.httpproxypassword";
const char* CSettings::SETTING_NETWORK_BANDWIDTH = "network.bandwidth";
const char* CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME = "powermanagement.shutdowntime";
const char* CSettings::SETTING_DEBUG_SHOWLOGINFO = "debug.showloginfo";
const char* CSettings::SETTING_DEBUG_SCREENSHOTPATH = "debug.screenshotpath";
const char* CSettings::SETTING_MASTERLOCK_LOCKCODE = "masterlock.lockcode";
const char* CSettings::SETTING_MASTERLOCK_STARTUPLOCK = "masterlock.startuplock";
const char* CSettings::SETTING_MASTERLOCK_MAXRETRIES = "masterlock.maxretries";
const char* CSettings::SETTING_CACHE_HARDDISK = "cache.harddisk";
const char* CSettings::SETTING_CACHEVIDEO_DVDROM = "cachevideo.dvdrom";
const char* CSettings::SETTING_CACHEVIDEO_LAN = "cachevideo.lan";
const char* CSettings::SETTING_CACHEVIDEO_INTERNET = "cachevideo.internet";
const char* CSettings::SETTING_CACHEAUDIO_DVDROM = "cacheaudio.dvdrom";
const char* CSettings::SETTING_CACHEAUDIO_LAN = "cacheaudio.lan";
const char* CSettings::SETTING_CACHEAUDIO_INTERNET = "cacheaudio.internet";
const char* CSettings::SETTING_CACHEDVD_DVDROM = "cachedvd.dvdrom";
const char* CSettings::SETTING_CACHEDVD_LAN = "cachedvd.lan";
const char* CSettings::SETTING_CACHEUNKNOWN_INTERNET = "cacheunknown.internet";
const char* CSettings::SETTING_SYSTEM_PLAYLISTSPATH = "system.playlistspath";
const char* CSettings::SETTING_ADDONS_AUTOUPDATES = "general.addonupdates";
const char* CSettings::SETTING_ADDONS_NOTIFICATIONS = "general.addonnotifications";
const char* CSettings::SETTING_ADDONS_SHOW_RUNNING = "addons.showrunning";
const char* CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES = "addons.unknownsources";
const char* CSettings::SETTING_ADDONS_UPDATEMODE = "addons.updatemode";
const char* CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES = "addons.managedependencies";
const char* CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES = "addons.removeorphaneddependencies";
const char* CSettings::SETTING_GENERAL_ADDONFOREIGNFILTER = "general.addonforeignfilter";
const char* CSettings::SETTING_GENERAL_ADDONBROKENFILTER = "general.addonbrokenfilter";
const char* CSettings::SETTING_SOURCE_VIDEOS = "source.videos";
const char* CSettings::SETTING_SOURCE_MUSIC = "source.music";
const char* CSettings::SETTING_SOURCE_PICTURES = "source.pictures";
const char* CSettings::SETTING_FILECACHE_BUFFERMODE = "filecache.buffermode";
const char* CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN = "harddisk.remoteplayspindown";
const char* CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DURATION = "harddisk.remoteplayspindownminduration";
const char* CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DELAY = "harddisk.remoteplayspindowndelay";
const char* CSettings::SETTING_HDD_SPINDOWN_TIME = "harddisk.spindowntime";
const char* CSettings::SETTING_KARAOKE_ENABLED = "karaoke.enabled";
const char* CSettings::SETTING_KARAOKE_CHARSET = "karaoke.charset";
const char* CSettings::SETTING_KARAOKE_EXPORT = "karaoke.export";
const char* CSettings::SETTING_KARAOKE_IMPORT = "karaoke.importcsv";
const char* CSettings::SETTING_KARAOKE_PORT_ONE_VOICEMASK = "karaoke.port0voicemask";
const char* CSettings::SETTING_KARAOKE_PORT_TWO_VOICEMASK = "karaoke.port1voicemask";
const char* CSettings::SETTING_KARAOKE_PORT_THREE_VOICEMASK = "karaoke.port2voicemask";
const char* CSettings::SETTING_KARAOKE_PORT_FOUR_VOICEMASK = "karaoke.port3voicemask";
const char* CSettings::SETTING_HARDDISK_AAMLEVEL = "harddisk.aamlevel";
const char* CSettings::SETTING_HARDDISK_APMLEVEL = "harddisk.apmlevel";
const char* CSettings::SETTING_LCD_BACKLIGHT = "lcd.backlight";
const char* CSettings::SETTING_LCD_CONTRAST = "lcd.contrast";
const char* CSettings::SETTING_LCD_MODCHIP = "lcd.modchip";
const char* CSettings::SETTING_LCD_TYPE = "lcd.type";
const char* CSettings::SETTING_LCD_DISABLE_ON_PLAYBACK = "lcd.disableonplayback";
const char* CSettings::SETTING_TRAINER_SCAN = "myprograms.trainerscan";
const char* CSettings::SETTING_NETWORK_ASSIGNMENT = "network.assignment";
const char* CSettings::SETTING_NETWORK_IPADDRESS = "network.ipaddress";
const char* CSettings::SETTING_NETWORK_SUBNET = "network.subnet";
const char* CSettings::SETTING_NETWORK_GATEWAY = "network.gateway";
const char* CSettings::SETTING_NETWORK_DNS = "network.dns";
const char* CSettings::SETTING_NETWORK_DNS2 = "network.dns2";
const char* CSettings::SETTING_UPDATER_CHECK = "updater.check";
const char* CSettings::SETTING_XBOX_LED_COLOUR = "system.ledcolour";
const char* CSettings::SETTING_XBOX_LED_DISABLE_ON_PLAYBACK = "system.leddisableonplayback";
const char* CSettings::SETTING_XBOX_AUTO_TEMPERATURE = "system.autotemperature";
const char* CSettings::SETTING_XBOX_FANSPEED_CONTROL = "system.fanspeedcontrol";
const char* CSettings::SETTING_XBOX_FANSPEED = "system.fanspeed";
const char* CSettings::SETTING_XBOX_MIN_FANSPEED = "system.minfanspeed";
const char* CSettings::SETTING_XBOX_TARGET_TEMPERATURE = "system.targettemperature";

bool CSettings::Initialize()
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  GetSettingsManager()->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

void CSettings::RegisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings* subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.erase(subSettings);
}

bool CSettings::Load()
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Load(profileManager->GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !Load(xmlDoc.RootElement(), updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from %s, creating new default settings",
              file.c_str());
    if (!Reset())
      return false;

    if (!Load(file))
      return false;
  }
  // if the settings had to be updated, we need to save the changes
  else if (updated)
    return Save(file);

  return true;
}

bool CSettings::Load(const TiXmlElement* root)
{
  bool updated = false;
  return Load(root, updated);
}

bool CSettings::Save()
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return Save(profileManager->GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!SaveValuesToXml(xmlDoc))
    return false;

  TiXmlElement* root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  if (!Save(root))
    return false;

  return xmlDoc.SaveFile(file);
}

bool CSettings::Save(TiXmlNode* root) const
{
  CSingleLock lock(m_critical);
  // save any ISubSettings implementations
  for (std::set<ISubSettings*>::const_iterator subSetting = m_subSettings.begin(); subSetting != m_subSettings.end(); ++subSetting)
  {
    if (!(*subSetting)->Save(root))
      return false;
  }

  return true;
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return GetSettingsManager()->LoadSetting(node, settingId);
}

void CSettings::Clear()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  GetSettingsManager()->Clear();

  for (std::set<ISubSettings*>::iterator subSetting = m_subSettings.begin(); subSetting != m_subSettings.end(); ++subSetting)
    (*subSetting)->Clear();

  m_initialized = false;
}

bool CSettings::Load(const TiXmlElement* root, bool& updated)
{
  if (root == NULL)
    return false;

  if (!CSettingsBase::LoadValuesFromXml(root, updated))
    return false;

  return Load(static_cast<const TiXmlNode*>(root));
}

bool CSettings::Load(const TiXmlNode* settings)
{
  bool ok = true;
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::iterator subSetting = m_subSettings.begin(); subSetting != m_subSettings.end(); ++subSetting)
    ok &= (*subSetting)->Load(settings);

  return ok;
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %i\n%s", file.c_str(),
              xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from %s", file.c_str());

  return InitializeDefinitionsFromXml(xmlDoc);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(_XBOX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "xbox.xml") && !Initialize(SETTINGS_XML_FOLDER "xbox.xml"))
    CLog::Log(LOGFATAL, "Unable to load xbox-specific settings definitions");
#endif

  // load any custom visibility and default values before loading the special
  // appliance.xml so that appliances are able to overwrite even those values
  InitializeVisibility();
  InitializeDefaults();

  if (CFile::Exists(SETTINGS_XML_FOLDER "appliance.xml") && !Initialize(SETTINGS_XML_FOLDER "appliance.xml"))
    CLog::Log(LOGFATAL, "Unable to load appliance-specific settings definitions");

  return true;
}

void CSettings::InitializeSettingTypes()
{
  GetSettingsManager()->RegisterSettingType("addon", this);
  GetSettingsManager()->RegisterSettingType("date", this);
  GetSettingsManager()->RegisterSettingType("path", this);
  GetSettingsManager()->RegisterSettingType("time", this);
}

void CSettings::InitializeControls()
{
  GetSettingsManager()->RegisterSettingControl("toggle", this);
  GetSettingsManager()->RegisterSettingControl("spinner", this);
  GetSettingsManager()->RegisterSettingControl("edit", this);
  GetSettingsManager()->RegisterSettingControl("button", this);
  GetSettingsManager()->RegisterSettingControl("list", this);
  GetSettingsManager()->RegisterSettingControl("slider", this);
  GetSettingsManager()->RegisterSettingControl("range", this);
  GetSettingsManager()->RegisterSettingControl("title", this);
  GetSettingsManager()->RegisterSettingControl("colorbutton", this);
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(_XBOX)
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH))->GetValue() && !g_audioConfig.HasDigitalOutput())
    boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH))->SetDefault(false);
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH))->SetDefault(g_audioConfig.GetAC3Enabled());
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH))->SetDefault(g_audioConfig.GetDTSEnabled());

  if (g_videoConfig.HasLetterbox())
    boost::static_pointer_cast<CSettingInt>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_ASPECT))->SetDefault(VIDEO_LETTERBOX);
  else if (g_videoConfig.HasWidescreen())
    boost::static_pointer_cast<CSettingInt>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_ASPECT))->SetDefault(VIDEO_WIDESCREEN);
  else
    boost::static_pointer_cast<CSettingInt>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_ASPECT))->SetDefault(VIDEO_NORMAL);
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_HD480p))->SetDefault(g_videoConfig.Has480p());
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_HD720p))->SetDefault(g_videoConfig.Has720p());
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_HD1080i))->SetDefault(g_videoConfig.Has1080i());

  boost::static_pointer_cast<CSettingInt>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE))->SetDefault(g_timezone.GetTimeZoneIndex());
  boost::static_pointer_cast<CSettingBool>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_USE_DST))->SetDefault(g_timezone.GetDST());
#endif
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_OPTICAL_DRIVE
  GetSettingsManager()->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("fanspeeds", CFanController::SettingOptionsSpeedsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("framerateconversions", CDisplaySettings::SettingOptionsFramerateconversionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("rendermethods", CXBoxRenderer::SettingOptionsRenderMethodsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("targettemperatures", CFanController::SettingOptionsTemperaturesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timezones", XBTimeZone::SettingOptionsTimezonesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "keyboardlayouts", KEYBOARD::CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("voicemasks", CCdgParser::SettingOptionsVoiceMasksFiller);
}

void CSettings::UninitializeOptionFillers()
{
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdactions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("charsets");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fanspeeds");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("languagenames");
  GetSettingsManager()->UnregisterSettingOptionsFiller("framerateconversions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("regions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shortdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("longdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timeformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("24hourclockformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("speedunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("temperatureunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("rendermethods");
  GetSettingsManager()->UnregisterSettingOptionsFiller("resolutions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("videoseeksteps");
  GetSettingsManager()->UnregisterSettingOptionsFiller("startupwindows");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlestreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitledownloadlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("iso6391languages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skincolors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinthemes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("targettemperatures");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezones");
  GetSettingsManager()->UnregisterSettingOptionsFiller("keyboardlayouts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("voicemasks");
}

void CSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string> &simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator itCondition = simpleConditions.begin(); itCondition != simpleConditions.end(); ++itCondition)
    GetSettingsManager()->AddCondition(*itCondition);

  // add more complex conditions
  const std::map<std::string, SettingConditionCheck> &complexConditions = CSettingConditions::GetComplexConditions();
  for (std::map<std::string, SettingConditionCheck>::const_iterator itCondition = complexConditions.begin(); itCondition != complexConditions.end(); ++itCondition)
    GetSettingsManager()->AddDynamicCondition(itCondition->first, itCondition->second);
}

void CSettings::UninitializeConditions()
{
  CSettingConditions::Deinitialize();
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSourceSettings::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->RegisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&g_langInfo);
#ifdef _XBOX
  GetSettingsManager()->RegisterSettingsHandler(&g_audioConfig);
  GetSettingsManager()->RegisterSettingsHandler(&g_videoConfig);
  GetSettingsManager()->RegisterSettingsHandler(&g_timezone);
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSettings::GetInstance());
}

void CSettings::UninitializeISettingsHandlers()
{
  // unregister ISettingsHandler implementations
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->UnregisterSettingsHandler(&CRssManager::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->UnregisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
  GetSettingsManager()->UnregisterSettingsHandler(&g_audioConfig);
  GetSettingsManager()->UnregisterSettingsHandler(&g_videoConfig);
  GetSettingsManager()->UnregisterSettingsHandler(&g_timezone);
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSourceSettings::GetInstance());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  RegisterSubSettings(&CDisplaySettings::GetInstance());
  RegisterSubSettings(&CMediaSettings::GetInstance());
  RegisterSubSettings(&CSkinSettings::GetInstance());
  RegisterSubSettings(&g_sysinfo);
  RegisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::UninitializeISubSettings()
{
  // unregister ISubSettings implementations
  UnregisterSubSettings(&CDisplaySettings::GetInstance());
  UnregisterSubSettings(&CMediaSettings::GetInstance());
  UnregisterSubSettings(&CSkinSettings::GetInstance());
  UnregisterSubSettings(&g_sysinfo);
  UnregisterSubSettings(&CViewStateSettings::GetInstance());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_MUSICLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_FLATTENTVSHOWS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_CLEANUP);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_IMPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_EXPORT);
  settingSet.insert(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS);
  GetSettingsManager()->RegisterCallback(&CMediaSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_RESOLUTION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_FLICKERFILTER);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SOFTEN);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_ASPECT);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_HD480p);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_HD720p);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_HD1080i);
  GetSettingsManager()->RegisterCallback(&CDisplaySettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SUBTITLES_CHARSET);
  settingSet.insert(CSettings::SETTING_KARAOKE_CHARSET);
  settingSet.insert(CSettings::SETTING_LOCALE_CHARSET);
  GetSettingsManager()->RegisterCallback(&g_charsetConverter, settingSet);

#ifdef _XBOX
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_XBOX_AUTO_TEMPERATURE);
  settingSet.insert(CSettings::SETTING_XBOX_FANSPEED_CONTROL);
  settingSet.insert(CSettings::SETTING_XBOX_FANSPEED);
  settingSet.insert(CSettings::SETTING_XBOX_MIN_FANSPEED);
  settingSet.insert(CSettings::SETTING_XBOX_TARGET_TEMPERATURE);
  GetSettingsManager()->RegisterCallback(CFanController::Instance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_AUDIOLANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_LANGUAGE);
  settingSet.insert(CSettings::SETTING_LOCALE_COUNTRY);
  settingSet.insert(CSettings::SETTING_LOCALE_SHORTDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_LONGDATEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEFORMAT);
  settingSet.insert(CSettings::SETTING_LOCALE_USE24HOURCLOCK);
  settingSet.insert(CSettings::SETTING_LOCALE_TEMPERATUREUNIT);
  settingSet.insert(CSettings::SETTING_LOCALE_SPEEDUNIT);
  GetSettingsManager()->RegisterCallback(&g_langInfo, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERUSERNAME);
  settingSet.insert(CSettings::SETTING_SERVICES_WEBSERVERPASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPRENDERER);
  settingSet.insert(CSettings::SETTING_SERVICES_ESENABLED);
  settingSet.insert(CSettings::SETTING_SERVICES_ESPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_ESALLINTERFACES);
  settingSet.insert(CSettings::SETTING_SERVICES_ESINITIALDELAY);
  settingSet.insert(CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER_USER);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_TIMESERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_TIMESERVER_ADDRESS);
  settingSet.insert(CSettings::SETTING_SMB_WINSSERVER);
  settingSet.insert(CSettings::SETTING_SMB_WORKGROUP);
  GetSettingsManager()->RegisterCallback(&CNetworkServices::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_MASTERLOCK_LOCKCODE);
  GetSettingsManager()->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_RSSEDIT);
  GetSettingsManager()->RegisterCallback(&CRssManager::GetInstance(), settingSet);

#ifdef _XBOX
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEZONE);
  settingSet.insert(CSettings::SETTING_LOCALE_USE_DST);
  GetSettingsManager()->RegisterCallback(&g_timezone, settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_SHOW_RUNNING);
  settingSet.insert(CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES);
  GetSettingsManager()->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);
}

void CSettings::UninitializeISettingCallbacks()
{
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(CFanController::Instance());
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&CNetworkServices::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_timezone);
}

bool CSettings::Reset()
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  const std::string settingsFile = profileManager->GetSettingsFile();

  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at %s", settingsFile.c_str());

  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to %s", settingsFile.c_str());
    return false;
  }

  return true;
}
