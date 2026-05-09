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
#include "application/AppParams.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "guilib/StereoscopicsManager.h"
#include "input/keyboard/KeyboardLayoutManager.h"

#include <mutex>
#if defined(TARGET_POSIX)
#include "platform/posix/PosixTimezone.h"
#endif // defined(TARGET_POSIX)
#include "network/upnp/UPnPSettings.h"
#include "network/WakeOnAccess.h"
#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
#include "platform/darwin/osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/TVOSSettingsHandler.h"
#endif // defined(TARGET_DARWIN_TVOS)
#if defined(TARGET_DARWIN_EMBEDDED)
#include "SettingAddon.h"
#endif
#include "DiscSettings.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "powermanagement/PowerTypes.h"
#include "profiles/ProfileManager.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/ServicesSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/SubtitlesSettings.h"
#include "settings/lib/SettingsManager.h"
#include "utils/CharsetConverter.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "view/ViewStateSettings.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"

using namespace KODI;
using namespace XFILE;

const char* CSettings::SETTING_LOOKANDFEEL_SKIN = "lookandfeel.skin";
const char* CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS = "lookandfeel.skinsettings";
const char* CSettings::SETTING_LOOKANDFEEL_SKINTHEME = "lookandfeel.skintheme";
const char* CSettings::SETTING_LOOKANDFEEL_SKINCOLORS = "lookandfeel.skincolors";
const char* CSettings::SETTING_LOOKANDFEEL_FONT = "lookandfeel.font";
const char* CSettings::SETTING_LOOKANDFEEL_SKINZOOM = "lookandfeel.skinzoom";
const char* CSettings::SETTING_LOOKANDFEEL_STARTUPACTION = "lookandfeel.startupaction";
const char* CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW = "lookandfeel.startupwindow";
const char* CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN = "lookandfeel.soundskin";
const char* CSettings::SETTING_LOOKANDFEEL_ENABLERSSFEEDS = "lookandfeel.enablerssfeeds";
const char* CSettings::SETTING_LOOKANDFEEL_RSSEDIT = "lookandfeel.rssedit";
const char* CSettings::SETTING_LOOKANDFEEL_STEREOSTRENGTH = "lookandfeel.stereostrength";
const char* CSettings::SETTING_LOCALE_LANGUAGE = "locale.language";
const char* CSettings::SETTING_LOCALE_COUNTRY = "locale.country";
const char* CSettings::SETTING_LOCALE_CHARSET = "locale.charset";
const char* CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS = "locale.keyboardlayouts";
const char* CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT = "locale.activekeyboardlayout";
const char* CSettings::SETTING_LOCALE_TIMEZONECOUNTRY = "locale.timezonecountry";
const char* CSettings::SETTING_LOCALE_TIMEZONE = "locale.timezone";
const char* CSettings::SETTING_LOCALE_SHORTDATEFORMAT = "locale.shortdateformat";
const char* CSettings::SETTING_LOCALE_LONGDATEFORMAT = "locale.longdateformat";
const char* CSettings::SETTING_LOCALE_TIMEFORMAT = "locale.timeformat";
const char* CSettings::SETTING_LOCALE_USE24HOURCLOCK = "locale.use24hourclock";
const char* CSettings::SETTING_LOCALE_TEMPERATUREUNIT = "locale.temperatureunit";
const char* CSettings::SETTING_LOCALE_SPEEDUNIT = "locale.speedunit";
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
const char* CSettings::SETTING_WINDOW_WIDTH = "window.width";
const char* CSettings::SETTING_WINDOW_HEIGHT = "window.height";
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
const char* CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE = "videoplayer.adjustrefreshrate";
const char* CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK = "videoplayer.usedisplayasclock";
const char* CSettings::SETTING_VIDEOPLAYER_ERRORINASPECT = "videoplayer.errorinaspect";
const char* CSettings::SETTING_VIDEOPLAYER_STRETCH43 = "videoplayer.stretch43";
const char* CSettings::SETTING_VIDEOPLAYER_TELETEXTENABLED = "videoplayer.teletextenabled";
const char* CSettings::SETTING_VIDEOPLAYER_TELETEXTSCALE = "videoplayer.teletextscale";
const char* CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE = "videoplayer.stereoscopicplaybackmode";
const char* CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP = "videoplayer.quitstereomodeonstop";
const char* CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD = "videoplayer.rendermethod";
const char* CSettings::SETTING_VIDEOPLAYER_HQSCALERS = "videoplayer.hqscalers";
const char* CSettings::SETTING_VIDEOPLAYER_USESUPERRESOLUTION = "videoplayer.usesuperresolution";
const char* CSettings::SETTING_VIDEOPLAYER_HIGHPRECISIONPROCESSING = "videoplayer.highprecision";
const char* CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC = "videoplayer.usemediacodec";
const char* CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE = "videoplayer.usemediacodecsurface";
const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAU = "videoplayer.usevdpau";
const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMIXER = "videoplayer.usevdpaumixer";
const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG2 = "videoplayer.usevdpaumpeg2";
const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4 = "videoplayer.usevdpaumpeg4";
const char* CSettings::SETTING_VIDEOPLAYER_USEVDPAUVC1 = "videoplayer.usevdpauvc1";
const char* CSettings::SETTING_VIDEOPLAYER_USEDXVA2 = "videoplayer.usedxva2";
const char* CSettings::SETTING_VIDEOPLAYER_USEVTB = "videoplayer.usevtb";
const char* CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER = "videoplayer.useprimedecoder";
const char* CSettings::SETTING_VIDEOPLAYER_USESTAGEFRIGHT = "videoplayer.usestagefright";
const char* CSettings::SETTING_VIDEOPLAYER_LIMITGUIUPDATE = "videoplayer.limitguiupdate";
const char* CSettings::SETTING_VIDEOPLAYER_SUPPORTMVC = "videoplayer.supportmvc";
const char* CSettings::SETTING_VIDEOPLAYER_CONVERTDOVI = "videoplayer.convertdovi";
const char* CSettings::SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS = "videoplayer.allowedhdrformats";
const char* CSettings::SETTING_VIDEOPLAYER_DEFAULTPLAYER = "videoplayer.defaultplayer";
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
const char* CSettings::SETTING_SUBTITLES_PARSECAPTIONS = "subtitles.parsecaptions";
const char* CSettings::SETTING_SUBTITLES_CAPTIONSALIGN = "subtitles.captionsalign";
const char* CSettings::SETTING_SUBTITLES_ALIGN = "subtitles.align";
const char* CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH = "subtitles.stereoscopicdepth";
const char* CSettings::SETTING_SUBTITLES_FONTNAME = "subtitles.fontname";
const char* CSettings::SETTING_SUBTITLES_FONTSIZE = "subtitles.fontsize";
const char* CSettings::SETTING_SUBTITLES_STYLE = "subtitles.style";
const char* CSettings::SETTING_SUBTITLES_COLOR = "subtitles.colorpick";
const char* CSettings::SETTING_SUBTITLES_BORDERSIZE = "subtitles.bordersize";
const char* CSettings::SETTING_SUBTITLES_BORDERCOLOR = "subtitles.bordercolorpick";
const char* CSettings::SETTING_SUBTITLES_OPACITY = "subtitles.opacity";
const char* CSettings::SETTING_SUBTITLES_BLUR = "subtitles.blur";
const char* CSettings::SETTING_SUBTITLES_BACKGROUNDTYPE = "subtitles.backgroundtype";
const char* CSettings::SETTING_SUBTITLES_SHADOWCOLOR = "subtitles.shadowcolor";
const char* CSettings::SETTING_SUBTITLES_SHADOWOPACITY = "subtitles.shadowopacity";
const char* CSettings::SETTING_SUBTITLES_SHADOWSIZE = "subtitles.shadowsize";
const char* CSettings::SETTING_SUBTITLES_BGCOLOR = "subtitles.bgcolorpick";
const char* CSettings::SETTING_SUBTITLES_BGOPACITY = "subtitles.bgopacity";
const char* CSettings::SETTING_SUBTITLES_MARGINVERTICAL = "subtitles.marginvertical";
const char* CSettings::SETTING_SUBTITLES_CHARSET = "subtitles.charset";
const char* CSettings::SETTING_SUBTITLES_OVERRIDEFONTS = "subtitles.overridefonts";
const char* CSettings::SETTING_SUBTITLES_OVERRIDESTYLES = "subtitles.overridestyles";
const char* CSettings::SETTING_SUBTITLES_LANGUAGES = "subtitles.languages";
const char* CSettings::SETTING_SUBTITLES_STORAGEMODE = "subtitles.storagemode";
const char* CSettings::SETTING_SUBTITLES_CUSTOMPATH = "subtitles.custompath";
const char* CSettings::SETTING_SUBTITLES_PAUSEONSEARCH = "subtitles.pauseonsearch";
const char* CSettings::SETTING_SUBTITLES_DOWNLOADFIRST = "subtitles.downloadfirst";
const char* CSettings::SETTING_SUBTITLES_TV = "subtitles.tv";
const char* CSettings::SETTING_SUBTITLES_MOVIE = "subtitles.movie";
const char* CSettings::SETTING_DVDS_AUTORUN = "dvds.autorun";
const char* CSettings::SETTING_DVDS_PLAYERREGION = "dvds.playerregion";
const char* CSettings::SETTING_DVDS_AUTOMENU = "dvds.automenu";
const char* CSettings::SETTING_DISC_PLAYBACK = "disc.playback";
const char* CSettings::SETTING_BLURAY_PLAYERREGION = "bluray.playerregion";
const char* CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL = "accessibility.audiovisual";
const char* CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING = "accessibility.audiohearing";
const char* CSettings::SETTING_ACCESSIBILITY_SUBHEARING = "accessibility.subhearing";
const char* CSettings::SETTING_SCRAPERS_MOVIESDEFAULT = "scrapers.moviesdefault";
const char* CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT = "scrapers.tvshowsdefault";
const char* CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT = "scrapers.musicvideosdefault";
const char* CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL = "pvrmanager.preselectplayingchannel";
const char* CSettings::SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER = "pvrmanager.backendchannelgroupsorder";
const char* CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER = "pvrmanager.backendchannelorder";
const char* CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS = "pvrmanager.usebackendchannelnumbers";
const char* CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS = "pvrmanager.usebackendchannelnumbersalways";
const char* CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE = "pvrmanager.startgroupchannelnumbersfromone";
const char* CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES = "pvrmanager.clientpriorities";
const char* CSettings::SETTING_PVRMANAGER_CHANNELMANAGER = "pvrmanager.channelmanager";
const char* CSettings::SETTING_PVRMANAGER_GROUPMANAGER = "pvrmanager.groupmanager";
const char* CSettings::SETTING_PVRMANAGER_CHANNELSCAN = "pvrmanager.channelscan";
const char* CSettings::SETTING_PVRMANAGER_RESETDB = "pvrmanager.resetdb";
const char* CSettings::SETTING_PVRMANAGER_ADDONS = "pvrmanager.addons";
const char* CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO = "pvrmenu.displaychannelinfo";
const char* CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH = "pvrmenu.closechannelosdonswitch";
const char* CSettings::SETTING_PVRMENU_ICONPATH = "pvrmenu.iconpath";
const char* CSettings::SETTING_PVRMENU_SEARCHICONS = "pvrmenu.searchicons";
const char* CSettings::SETTING_EPG_PAST_DAYSTODISPLAY = "epg.pastdaystodisplay";
const char* CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY = "epg.futuredaystodisplay";
const char* CSettings::SETTING_EPG_SELECTACTION = "epg.selectaction";
const char* CSettings::SETTING_EPG_HIDENOINFOAVAILABLE = "epg.hidenoinfoavailable";
const char* CSettings::SETTING_EPG_EPGUPDATE = "epg.epgupdate";
const char* CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV = "epg.preventupdateswhileplayingtv";
const char* CSettings::SETTING_EPG_RESETEPG = "epg.resetepg";
const char* CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES = "pvrplayback.switchtofullscreenchanneltypes";
const char* CSettings::SETTING_PVRPLAYBACK_SIGNALQUALITY = "pvrplayback.signalquality";
const char* CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH = "pvrplayback.confirmchannelswitch";
const char* CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT = "pvrplayback.channelentrytimeout";
const char* CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED = "pvrplayback.delaymarklastwatched";
const char* CSettings::SETTING_PVRPLAYBACK_FPS = "pvrplayback.fps";
const char* CSettings::SETTING_PVRPLAYBACK_AUTOPLAYNEXTPROGRAMME = "pvrplayback.autoplaynextprogramme";
const char* CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION = "pvrrecord.instantrecordaction";
const char* CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME = "pvrrecord.instantrecordtime";
const char* CSettings::SETTING_PVRRECORD_MARGINSTART = "pvrrecord.marginstart";
const char* CSettings::SETTING_PVRRECORD_MARGINEND = "pvrrecord.marginend";
const char* CSettings::SETTING_PVRRECORD_TIMERNOTIFICATIONS = "pvrrecord.timernotifications";
const char* CSettings::SETTING_PVRRECORD_GROUPRECORDINGS = "pvrrecord.grouprecordings";
const char* CSettings::SETTING_PVRREMINDERS_AUTOCLOSEDELAY = "pvrreminders.autoclosedelay";
const char* CSettings::SETTING_PVRREMINDERS_AUTORECORD = "pvrreminders.autorecord";
const char* CSettings::SETTING_PVRREMINDERS_AUTOSWITCH = "pvrreminders.autoswitch";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED = "pvrpowermanagement.enabled";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME = "pvrpowermanagement.backendidletime";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD = "pvrpowermanagement.setwakeupcmd";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_PREWAKEUP = "pvrpowermanagement.prewakeup";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUP = "pvrpowermanagement.dailywakeup";
const char* CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME = "pvrpowermanagement.dailywakeuptime";
const char* CSettings::SETTING_PVRPARENTAL_ENABLED = "pvrparental.enabled";
const char* CSettings::SETTING_PVRPARENTAL_PIN = "pvrparental.pin";
const char* CSettings::SETTING_PVRPARENTAL_DURATION = "pvrparental.duration";
const char* CSettings::SETTING_PVRCLIENT_MENUHOOK = "pvrclient.menuhook";
const char* CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS = "pvrtimers.hidedisabledtimers";
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
const char* CSettings::SETTING_SLIDESHOW_HIGHQUALITYDOWNSCALING = "slideshow.highqualitydownscaling";
const char* CSettings::SETTING_WEATHER_CURRENTLOCATION = "weather.currentlocation";
const char* CSettings::SETTING_WEATHER_ADDON = "weather.addon";
const char* CSettings::SETTING_WEATHER_ADDONSETTINGS = "weather.addonsettings";
const char* CSettings::SETTING_SERVICES_DEVICENAME = "services.devicename";
const char* CSettings::SETTING_SERVICES_DEVICEUUID = "services.deviceuuid";
const char* CSettings::SETTING_SERVICES_UPNP = "services.upnp";
const char* CSettings::SETTING_SERVICES_UPNPSERVER = "services.upnpserver";
const char* CSettings::SETTING_SERVICES_UPNPANNOUNCE = "services.upnpannounce";
const char* CSettings::SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES = "services.upnplookforexternalsubtitles";
const char* CSettings::SETTING_SERVICES_UPNPCONTROLLER = "services.upnpcontroller";
const char* CSettings::SETTING_SERVICES_UPNPPLAYERVOLUMESYNC = "services.upnpplayervolumesync";
const char* CSettings::SETTING_SERVICES_UPNPRENDERER = "services.upnprenderer";
const char* CSettings::SETTING_SERVICES_WEBSERVER = "services.webserver";
const char* CSettings::SETTING_SERVICES_WEBSERVERPORT = "services.webserverport";
const char* CSettings::SETTING_SERVICES_WEBSERVERAUTHENTICATION = "services.webserverauthentication";
const char* CSettings::SETTING_SERVICES_WEBSERVERUSERNAME = "services.webserverusername";
const char* CSettings::SETTING_SERVICES_WEBSERVERPASSWORD = "services.webserverpassword";
const char* CSettings::SETTING_SERVICES_WEBSERVERSSL = "services.webserverssl";
const char* CSettings::SETTING_SERVICES_WEBSKIN = "services.webskin";
const char* CSettings::SETTING_SERVICES_ESENABLED = "services.esenabled";
const char* CSettings::SETTING_SERVICES_ESPORT = "services.esport";
const char* CSettings::SETTING_SERVICES_ESPORTRANGE = "services.esportrange";
const char* CSettings::SETTING_SERVICES_ESMAXCLIENTS = "services.esmaxclients";
const char* CSettings::SETTING_SERVICES_ESALLINTERFACES = "services.esallinterfaces";
const char* CSettings::SETTING_SERVICES_ESINITIALDELAY = "services.esinitialdelay";
const char* CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY = "services.escontinuousdelay";
const char* CSettings::SETTING_SERVICES_ZEROCONF = "services.zeroconf";
const char* CSettings::SETTING_SERVICES_AIRPLAY = "services.airplay";
const char* CSettings::SETTING_SERVICES_AIRPLAYVOLUMECONTROL = "services.airplayvolumecontrol";
const char* CSettings::SETTING_SERVICES_USEAIRPLAYPASSWORD = "services.useairplaypassword";
const char* CSettings::SETTING_SERVICES_AIRPLAYPASSWORD = "services.airplaypassword";
const char* CSettings::SETTING_SERVICES_AIRPLAYVIDEOSUPPORT = "services.airplayvideosupport";
const char* CSettings::SETTING_SMB_WINSSERVER = "smb.winsserver";
const char* CSettings::SETTING_SMB_WORKGROUP = "smb.workgroup";
const char* CSettings::SETTING_SMB_MINPROTOCOL = "smb.minprotocol";
const char* CSettings::SETTING_SMB_MAXPROTOCOL = "smb.maxprotocol";
const char* CSettings::SETTING_SMB_LEGACYSECURITY = "smb.legacysecurity";
const char* CSettings::SETTING_SMB_CHUNKSIZE = "smb.chunksize";
const char* CSettings::SETTING_SERVICES_WSDISCOVERY = "services.wsdiscovery";
const char* CSettings::SETTING_VIDEOSCREEN_MONITOR = "videoscreen.monitor";
const char* CSettings::SETTING_VIDEOSCREEN_SCREEN = "videoscreen.screen";
const char* CSettings::SETTING_VIDEOSCREEN_WHITELIST = "videoscreen.whitelist";
const char* CSettings::SETTING_VIDEOSCREEN_RESOLUTION = "videoscreen.resolution";
const char* CSettings::SETTING_VIDEOSCREEN_SCREENMODE = "videoscreen.screenmode";
const char* CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN = "videoscreen.fakefullscreen";
const char* CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS = "videoscreen.blankdisplays";
const char* CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE = "videoscreen.stereoscopicmode";
const char* CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE = "videoscreen.preferedstereoscopicmode";
const char* CSettings::SETTING_VIDEOSCREEN_NOOFBUFFERS = "videoscreen.noofbuffers";
const char* CSettings::SETTING_VIDEOSCREEN_3DLUT = "videoscreen.cms3dlut";
const char* CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE = "videoscreen.displayprofile";
const char* CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION = "videoscreen.guicalibration";
const char* CSettings::SETTING_VIDEOSCREEN_TESTPATTERN = "videoscreen.testpattern";
const char* CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE = "videoscreen.limitedrange";
const char* CSettings::SETTING_VIDEOSCREEN_FRAMEPACKING = "videoscreen.framepacking";
const char* CSettings::SETTING_VIDEOSCREEN_10BITSURFACES = "videoscreen.10bitsurfaces";
const char* CSettings::SETTING_VIDEOSCREEN_USESYSTEMSDRPEAKLUMINANCE = "videoscreen.usesystemsdrpeakluminance";
const char* CSettings::SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE = "videoscreen.guipeakluminance";
const char* CSettings::SETTING_VIDEOSCREEN_DITHER = "videoscreen.dither";
const char* CSettings::SETTING_VIDEOSCREEN_DITHERDEPTH = "videoscreen.ditherdepth";
const char* CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE = "audiooutput.audiodevice";
const char* CSettings::SETTING_AUDIOOUTPUT_CHANNELS = "audiooutput.channels";
const char* CSettings::SETTING_AUDIOOUTPUT_CONFIG = "audiooutput.config";
const char* CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE = "audiooutput.samplerate";
const char* CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX = "audiooutput.stereoupmix";
const char* CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME = "audiooutput.maintainoriginalvolume";
const char* CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY = "audiooutput.processquality";
const char* CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD = "audiooutput.atempothreshold";
const char* CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE = "audiooutput.streamsilence";
const char* CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE = "audiooutput.streamnoise";
const char* CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE = "audiooutput.guisoundmode";
const char* CSettings::SETTING_AUDIOOUTPUT_GUISOUNDVOLUME = "audiooutput.guisoundvolume";
const char* CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH = "audiooutput.passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE = "audiooutput.passthroughdevice";
const char* CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH = "audiooutput.ac3passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE = "audiooutput.ac3transcode";
const char* CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH = "audiooutput.eac3passthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH = "audiooutput.dtspassthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH = "audiooutput.truehdpassthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH = "audiooutput.dtshdpassthrough";
const char* CSettings::SETTING_AUDIOOUTPUT_DTSHDCOREFALLBACK = "audiooutput.dtshdcorefallback";
const char* CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS = "audiooutput.volumesteps";
const char* CSettings::SETTING_INPUT_PERIPHERALS = "input.peripherals";
const char* CSettings::SETTING_INPUT_PERIPHERALLIBRARIES = "input.peripherallibraries";
const char* CSettings::SETTING_INPUT_ENABLEMOUSE = "input.enablemouse";
const char* CSettings::SETTING_INPUT_ASKNEWCONTROLLERS = "input.asknewcontrollers";
const char* CSettings::SETTING_INPUT_CONTROLLERCONFIG = "input.controllerconfig";
const char* CSettings::SETTING_INPUT_RUMBLENOTIFY = "input.rumblenotify";
const char* CSettings::SETTING_INPUT_TESTRUMBLE = "input.testrumble";
const char* CSettings::SETTING_INPUT_CONTROLLERPOWEROFF = "input.controllerpoweroff";
const char* CSettings::SETTING_INPUT_APPLEREMOTEMODE = "input.appleremotemode";
const char* CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON = "input.appleremotealwayson";
const char* CSettings::SETTING_INPUT_APPLEREMOTESEQUENCETIME = "input.appleremotesequencetime";
const char* CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED = "input.siriremoteidletimerenabled";
const char* CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME = "input.siriremoteidletime";
const char* CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY = "input.siriremotehorizontalsensitivity";
const char* CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY = "input.siriremoteverticalsensitivity";
const char* CSettings::SETTING_INPUT_TVOSUSEKODIKEYBOARD = "input.tvosusekodikeyboard";
const char* CSettings::SETTING_NETWORK_USEHTTPPROXY = "network.usehttpproxy";
const char* CSettings::SETTING_NETWORK_HTTPPROXYTYPE = "network.httpproxytype";
const char* CSettings::SETTING_NETWORK_HTTPPROXYSERVER = "network.httpproxyserver";
const char* CSettings::SETTING_NETWORK_HTTPPROXYPORT = "network.httpproxyport";
const char* CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME = "network.httpproxyusername";
const char* CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD = "network.httpproxypassword";
const char* CSettings::SETTING_NETWORK_BANDWIDTH = "network.bandwidth";
const char* CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF = "powermanagement.displaysoff";
const char* CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME = "powermanagement.shutdowntime";
const char* CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE = "powermanagement.shutdownstate";
const char* CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS = "powermanagement.wakeonaccess";
const char* CSettings::SETTING_POWERMANAGEMENT_WAITFORNETWORK = "powermanagement.waitfornetwork";
const char* CSettings::SETTING_DEBUG_SHOWLOGINFO = "debug.showloginfo";
const char* CSettings::SETTING_DEBUG_EXTRALOGGING = "debug.extralogging";
const char* CSettings::SETTING_DEBUG_SETEXTRALOGLEVEL = "debug.setextraloglevel";
const char* CSettings::SETTING_DEBUG_SCREENSHOTPATH = "debug.screenshotpath";
const char* CSettings::SETTING_DEBUG_SHARE_LOG = "debug.sharelog";
const char* CSettings::SETTING_EVENTLOG_ENABLED = "eventlog.enabled";
const char* CSettings::SETTING_EVENTLOG_ENABLED_NOTIFICATIONS = "eventlog.enablednotifications";
const char* CSettings::SETTING_EVENTLOG_SHOW = "eventlog.show";
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
const char* CSettings::SETTING_FILECACHE_MEMORYSIZE = "filecache.memorysize"; // in MBytes
const char* CSettings::SETTING_FILECACHE_READFACTOR = "filecache.readfactor"; // as integer (x100)
const char* CSettings::SETTING_FILECACHE_CHUNKSIZE = "filecache.chunksize"; // in Bytes

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
    CLog::Log(LOGERROR, "CSettings: unable to load settings from {}, creating new default settings",
              file);
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
  for (const auto& subSetting : m_subSettings)
  {
    if (!subSetting->Save(root))
      return false;
  }

  return true;
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return GetSettingsManager()->LoadSetting(node, settingId);
}

bool CSettings::GetBool(const std::string& id) const
{
  // Backward compatibility (skins use this setting)
  if (StringUtils::EqualsNoCase(id, "lookandfeel.enablemouse"))
    return CSettingsBase::GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE);

  return CSettingsBase::GetBool(id);
}

void CSettings::Clear()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  GetSettingsManager()->Clear();

  for (auto& subSetting : m_subSettings)
    subSetting->Clear();

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
  for (const auto& subSetting : m_subSettings)
    ok &= subSetting->Load(settings);

  return ok;
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from {}, Line {}\n{}", file,
              xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from {}", file);

  return InitializeDefinitionsFromXml(xmlDoc);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(TARGET_WINDOWS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "windows.xml") && !Initialize(SETTINGS_XML_FOLDER "windows.xml"))
    CLog::Log(LOGFATAL, "Unable to load windows-specific settings definitions");
#if defined(TARGET_WINDOWS_DESKTOP)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_WINDOWS_STORE)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win10.xml") && !Initialize(SETTINGS_XML_FOLDER "win10.xml"))
    CLog::Log(LOGFATAL, "Unable to load win10-specific settings definitions");
#endif
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#elif defined(TARGET_DARWIN_TVOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_tvos.xml") &&
      !Initialize(SETTINGS_XML_FOLDER "darwin_tvos.xml"))
    CLog::Log(LOGFATAL, "Unable to load tvos-specific settings definitions");
#endif
#endif

#if defined(PLATFORM_SETTINGS_FILE)
  if (CFile::Exists(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)) && !Initialize(SETTINGS_XML_FOLDER DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE)))
    CLog::Log(LOGFATAL, "Unable to load platform-specific settings definitions ({})",
              DEF_TO_STR_VALUE(PLATFORM_SETTINGS_FILE));
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
#if defined(TARGET_DARWIN_EMBEDDED)
  boost::shared_ptr<CSettingString> timezonecountry = boost::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  boost::shared_ptr<CSettingString> timezone = boost::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE));

  timezonecountry->SetRequirementsMet(false);
  timezone->SetRequirementsMet(false);
#endif
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(TARGET_POSIX)
  boost::shared_ptr<CSettingString> timezonecountry = boost::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY));
  boost::shared_ptr<CSettingString> timezone = boost::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE));

  if (timezonecountry->IsVisible())
    timezonecountry->SetDefault(g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()));
  if (timezone->IsVisible())
    timezone->SetDefault(g_timezone.GetOSConfiguredTimezone());
#endif // defined(TARGET_POSIX)

#if defined(TARGET_WINDOWS)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
  {
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN);
    else
      boost::static_pointer_cast<CSettingBool>(setting)->SetDefault(false);
  }
#endif

  if (CServiceBroker::GetAppParams()->IsStandAlone())
  {
    auto setting =
        GetSettingsManager()->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    else
      boost::static_pointer_cast<CSettingInt>(setting)->SetDefault(POWERSTATE_SHUTDOWN);
  }

  // Initialize deviceUUID if not already set, used in zeroconf advertisements.
  boost::shared_ptr<CSettingString> deviceUUID = boost::static_pointer_cast<CSettingString>(GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID));
  if (deviceUUID->GetValue().empty())
  {
    const std::string& uuid = StringUtils::CreateUUID();
    auto setting = GetSettingsManager()->GetSetting(CSettings::SETTING_SERVICES_DEVICEUUID);
    if (!setting)
      CLog::Log(LOGERROR, "Failed to load setting for: {}", CSettings::SETTING_SERVICES_DEVICEUUID);
    else
      boost::static_pointer_cast<CSettingString>(setting)->SetValue(uuid);
  }
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_OPTICAL_DRIVE
  GetSettingsManager()->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "subtitlesfonts", SUBTITLES::CSubtitlesSettings::SettingOptionsSubtitleFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshchangedelays", CDisplaySettings::SettingOptionsRefreshChangeDelaysFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("refreshrates", CDisplaySettings::SettingOptionsRefreshRatesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("modes", CDisplaySettings::SettingOptionsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("screens", CDisplaySettings::SettingOptionsDispModeFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("stereoscopicmodes", CDisplaySettings::SettingOptionsStereoscopicModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("preferedstereoscopicviewmodes", CDisplaySettings::SettingOptionsPreferredStereoscopicViewModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("monitors", CDisplaySettings::SettingOptionsMonitorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsmodes", CDisplaySettings::SettingOptionsCmsModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmswhitepoints", CDisplaySettings::SettingOptionsCmsWhitepointsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsprimaries", CDisplaySettings::SettingOptionsCmsPrimariesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("cmsgammamodes", CDisplaySettings::SettingOptionsCmsGammaModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
#ifdef TARGET_LINUX
  GetSettingsManager()->RegisterSettingOptionsFiller("timezonecountries", CPosixTimezone::SettingOptionsTimezoneCountriesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller("timezones", CPosixTimezone::SettingOptionsTimezonesFiller);
#endif
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "keyboardlayouts", KEYBOARD::CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filechunksizes", CServicesSettings::SettingOptionsChunkSizesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachebuffermodes", CServicesSettings::SettingOptionsBufferModesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachememorysizes", CServicesSettings::SettingOptionsMemorySizesFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachereadfactors", CServicesSettings::SettingOptionsReadFactorsFiller);
  GetSettingsManager()->RegisterSettingOptionsFiller(
      "filecachechunksizes", CServicesSettings::SettingOptionsCacheChunkSizesFiller);
}

void CSettings::UninitializeOptionFillers()
{
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdactions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiocdencoders");
  GetSettingsManager()->UnregisterSettingOptionsFiller("charsets");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fontheights");
  GetSettingsManager()->UnregisterSettingOptionsFiller("fonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlesfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("languagenames");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshchangedelays");
  GetSettingsManager()->UnregisterSettingOptionsFiller("refreshrates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("regions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shortdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("longdateformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timeformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("24hourclockformats");
  GetSettingsManager()->UnregisterSettingOptionsFiller("speedunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("temperatureunits");
  GetSettingsManager()->UnregisterSettingOptionsFiller("rendermethods");
  GetSettingsManager()->UnregisterSettingOptionsFiller("resolutions");
  GetSettingsManager()->UnregisterSettingOptionsFiller("screens");
  GetSettingsManager()->UnregisterSettingOptionsFiller("stereoscopicmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("preferedstereoscopicviewmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("monitors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsmodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmswhitepoints");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsprimaries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("cmsgammamodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("videoseeksteps");
  GetSettingsManager()->UnregisterSettingOptionsFiller("shutdownstates");
  GetSettingsManager()->UnregisterSettingOptionsFiller("startupwindows");
  GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitlestreamlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("subtitledownloadlanguages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("iso6391languages");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skincolors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinfonts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("skinthemes");
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezonecountries");
  GetSettingsManager()->UnregisterSettingOptionsFiller("timezones");
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingOptionsFiller("verticalsyncs");
  GetSettingsManager()->UnregisterSettingOptionsFiller("keyboardlayouts");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filechunksizes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachebuffermodes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachememorysizes");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachereadfactors");
  GetSettingsManager()->UnregisterSettingOptionsFiller("filecachechunksizes");
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
  GetSettingsManager()->RegisterSettingsHandler(&CWakeOnAccess::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->RegisterSettingsHandler(&g_langInfo);
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
  GetSettingsManager()->RegisterSettingsHandler(&g_timezone);
#endif
  GetSettingsManager()->RegisterSettingsHandler(&CMediaSettings::GetInstance());
}

void CSettings::UninitializeISettingsHandlers()
{
  // unregister ISettingsHandler implementations
  GetSettingsManager()->UnregisterSettingsHandler(&CMediaSettings::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingsHandler(&g_timezone);
#endif // defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterSettingsHandler(&g_langInfo);
  GetSettingsManager()->UnregisterSettingsHandler(&CRssManager::GetInstance());
  GetSettingsManager()->UnregisterSettingsHandler(&CWakeOnAccess::GetInstance());
#ifdef HAS_UPNP
  GetSettingsManager()->UnregisterSettingsHandler(&CUPnPSettings::GetInstance());
#endif
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
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREEN);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_RESOLUTION);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_SCREENMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_3DLUT);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_DISPLAYPROFILE);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_BLANKDISPLAYS);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_WHITELIST);
  settingSet.insert(CSettings::SETTING_VIDEOSCREEN_10BITSURFACES);
  GetSettingsManager()->RegisterCallback(&CDisplaySettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_SUBTITLES_CHARSET);
  settingSet.insert(CSettings::SETTING_LOCALE_CHARSET);
  GetSettingsManager()->RegisterCallback(&g_charsetConverter, settingSet);

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
  settingSet.insert(CSettings::SETTING_MASTERLOCK_LOCKCODE);
  GetSettingsManager()->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOOKANDFEEL_RSSEDIT);
  GetSettingsManager()->RegisterCallback(&CRssManager::GetInstance(), settingSet);

#if defined(TARGET_LINUX)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEZONE);
  settingSet.insert(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY);
  GetSettingsManager()->RegisterCallback(&g_timezone, settingSet);
#endif

#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEMODE);
  settingSet.insert(CSettings::SETTING_INPUT_APPLEREMOTEALWAYSON);
  GetSettingsManager()->RegisterCallback(&XBMCHelper::GetInstance(), settingSet);
#endif

#if defined(TARGET_DARWIN_TVOS)
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY);
  settingSet.insert(CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY);
  GetSettingsManager()->RegisterCallback(&CTVOSInputSettings::GetInstance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_ADDONS_SHOW_RUNNING);
  settingSet.insert(CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES);
  settingSet.insert(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES);
  GetSettingsManager()->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert(CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS);
  GetSettingsManager()->RegisterCallback(&CWakeOnAccess::GetInstance(), settingSet);

#ifdef HAVE_LIBBLURAY
  settingSet.clear();
  settingSet.insert(CSettings::SETTING_DISC_PLAYBACK);
  GetSettingsManager()->RegisterCallback(&CDiscSettings::GetInstance(), settingSet);
#endif
}

void CSettings::UninitializeISettingCallbacks()
{
  GetSettingsManager()->UnregisterCallback(&CMediaSettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&CDisplaySettings::GetInstance());
  GetSettingsManager()->UnregisterCallback(&g_charsetConverter);
  GetSettingsManager()->UnregisterCallback(&g_langInfo);
  GetSettingsManager()->UnregisterCallback(&g_passwordManager);
  GetSettingsManager()->UnregisterCallback(&CRssManager::GetInstance());
#if defined(TARGET_LINUX)
  GetSettingsManager()->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
#if defined(TARGET_DARWIN_OSX) and defined(HAS_XBMCHELPER)
  GetSettingsManager()->UnregisterCallback(&XBMCHelper::GetInstance());
#endif
  GetSettingsManager()->UnregisterCallback(&CWakeOnAccess::GetInstance());
#ifdef HAVE_LIBBLURAY
  GetSettingsManager()->UnregisterCallback(&CDiscSettings::GetInstance());
#endif
}

bool CSettings::Reset()
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  const std::string settingsFile = profileManager->GetSettingsFile();

  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at {}", settingsFile);

  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to {}", settingsFile);
    return false;
  }

  return true;
}
