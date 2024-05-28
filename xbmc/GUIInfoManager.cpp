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

#include "xbox/Network.h"
#include "system.h"
#include "windows/GUIMediaWindow.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogProgress.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "Util.h"
#include "utils/MathUtils.h"
#include "utils/SeekHandler.h"
#include "utils/TuxBoxUtil.h"
#include "Weather.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#include "addons/Visualisation.h"
#include "input/ButtonTranslator.h"
#include "music/MusicDatabase.h"
#include "utils/AlarmClock.h"
#include "CharsetConverter.h"
#ifdef HAS_LCD
#include "utils/LCD.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "FanController.h"
#include "xbox/XKHDD.h"
#endif
#include "SystemInfo.h"
#include "GUITextBox.h"
#include "GUIControlGroupList.h"
#include "GUIPassword.h"
#include "LangInfo.h"
#include "GUIInfoManager.h"
#include <stack>
#include "xbox/network.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/IGUIContainer.h"
#include "video/VideoDatabase.h"
#include "GUIWindowManager.h"
#include "filesystem/File.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/SkinSettings.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"

// stuff for current song
#ifdef HAS_FILESYSTEM
#include "filesystem/SndtrkDirectory.h"
#endif
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/MusicInfoLoader.h"
#include "LabelFormatter.h"
#include "TextureManager.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "addons/Skin.h"
#include "boost/make_shared.hpp"
#include "TimeUtils.h"
#include "utils/URIUtils.h"
#include "LocalizeStrings.h"
#include "utils/log.h"

#include "addons/AddonManager.h"
#include "interfaces/info/InfoBool.h"
#include "video/VideoThumbLoader.h"
#include "music/MusicThumbLoader.h"
#include "video/VideoDatabase.h"
#include "interfaces/info/InfoExpression.h"

using namespace std;
using namespace XFILE;
using namespace MEDIA_DETECT;
using namespace MUSIC_INFO;
using namespace ADDON;

CGUIInfoManager g_infoManager;

using namespace INFO;

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = 0;
  m_lastMusicBitrateTime = 0;
  m_fanSpeed = 0;
  m_AfterSeekTimeout = 0;
  m_seekOffset = 0;
  m_playerSeeking = false;
  m_performingSeek = false;
  m_nextWindowID = WINDOW_INVALID;
  m_prevWindowID = WINDOW_INVALID;
  m_stringParameters.push_back("__ZZZZ__");   // to offset the string parameters by 1 to assure that all entries are non-zero
  m_currentFile = new CFileItem;
  m_currentSlide = new CFileItem;
  m_frameCounter = 0;
  m_lastFPSTime = 0;
  ResetLibraryBools();
}

CGUIInfoManager::~CGUIInfoManager(void)
{
  delete m_currentFile;
  delete m_currentSlide;
}

bool CGUIInfoManager::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      CFileItemPtr item = boost::static_pointer_cast<CFileItem>(message.GetItem());
      if (item && m_currentFile->GetPath().Equals(item->GetPath()))
        *m_currentFile = *item;
      return true;
    }
  }
  return false;
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const CStdString &condition)
{
  // translate $LOCALIZE as required
  CStdString strCondition(CGUIInfoLabel::ReplaceLocalize(condition));
  strCondition = CGUIInfoLabel::ReplaceAddonStrings(strCondition);
  return TranslateSingleString(strCondition);
}

typedef struct
{
  const char *str;
  int  val;
} infomap;

const infomap string_bools[] =   {{ "isempty",          STRING_IS_EMPTY },
                                  { "isequal",          STRING_IS_EQUAL },
                                  { "startswith",       STRING_STARTS_WITH },
                                  { "endswith",         STRING_ENDS_WITH },
                                  { "contains",         STRING_CONTAINS }};

const infomap integer_bools[] =  {{ "isequal",          INTEGER_IS_EQUAL },
                                  { "isgreater",        INTEGER_GREATER_THAN },
                                  { "isgreaterorequal", INTEGER_GREATER_OR_EQUAL },
                                  { "isless",           INTEGER_LESS_THAN },
                                  { "islessorequal",    INTEGER_LESS_OR_EQUAL }};

const infomap player_labels[] =  {{ "hasmedia",         PLAYER_HAS_MEDIA },           // bools from here
                                  { "hasaudio",         PLAYER_HAS_AUDIO },
                                  { "hasvideo",         PLAYER_HAS_VIDEO },
                                  { "playing",          PLAYER_PLAYING },
                                  { "paused",           PLAYER_PAUSED },
                                  { "rewinding",        PLAYER_REWINDING },
                                  { "forwarding",       PLAYER_FORWARDING },
                                  { "rewinding2x",      PLAYER_REWINDING_2x },
                                  { "rewinding4x",      PLAYER_REWINDING_4x },
                                  { "rewinding8x",      PLAYER_REWINDING_8x },
                                  { "rewinding16x",     PLAYER_REWINDING_16x },
                                  { "rewinding32x",     PLAYER_REWINDING_32x },
                                  { "forwarding2x",     PLAYER_FORWARDING_2x },
                                  { "forwarding4x",     PLAYER_FORWARDING_4x },
                                  { "forwarding8x",     PLAYER_FORWARDING_8x },
                                  { "forwarding16x",    PLAYER_FORWARDING_16x },
                                  { "forwarding32x",    PLAYER_FORWARDING_32x },
                                  { "canrecord",        PLAYER_CAN_RECORD },
                                  { "recording",        PLAYER_RECORDING },
                                  { "displayafterseek", PLAYER_DISPLAY_AFTER_SEEK },
                                  { "caching",          PLAYER_CACHING },
                                  { "seekbar",          PLAYER_SEEKBAR },
                                  { "seeking",          PLAYER_SEEKING },
                                  { "showtime",         PLAYER_SHOWTIME },
                                  { "showcodec",        PLAYER_SHOWCODEC },
                                  { "showinfo",         PLAYER_SHOWINFO },
                                  { "muted",            PLAYER_MUTED },
                                  { "hasduration",      PLAYER_HASDURATION },
                                  { "cachelevel",       PLAYER_CACHELEVEL },          // labels from here
                                  { "progress",         PLAYER_PROGRESS },
                                  { "progresscache",    PLAYER_PROGRESS_CACHE },
                                  { "volume",           PLAYER_VOLUME },
                                  { "subtitledelay",    PLAYER_SUBTITLE_DELAY },
                                  { "audiodelay",       PLAYER_AUDIO_DELAY },
                                  { "chapter",          PLAYER_CHAPTER },
                                  { "chaptercount",     PLAYER_CHAPTERCOUNT },
                                  { "chaptername",      PLAYER_CHAPTERNAME },
                                  { "starrating",       PLAYER_STAR_RATING },
                                  { "folderpath",       PLAYER_PATH },
                                  { "filenameandpath",  PLAYER_FILEPATH }};

const infomap player_param[] =   {{ "art",              PLAYER_ITEM_ART }};

const infomap player_times[] =   {{ "seektime",         PLAYER_SEEKTIME },
                                  { "seekoffset",       PLAYER_SEEKOFFSET },
                                  { "timeremaining",    PLAYER_TIME_REMAINING },
                                  { "timespeed",        PLAYER_TIME_SPEED },
                                  { "time",             PLAYER_TIME },
                                  { "duration",         PLAYER_DURATION },
                                  { "finishtime",       PLAYER_FINISH_TIME }};

const infomap weather[] =        {{ "isfetched",        WEATHER_IS_FETCHED },
                                  { "conditions",       WEATHER_CONDITIONS },         // labels from here
                                  { "temperature",      WEATHER_TEMPERATURE },
                                  { "location",         WEATHER_LOCATION },
                                  { "fanartcode",       WEATHER_FANART_CODE },
                                  { "plugin",           WEATHER_PLUGIN }};

const infomap system_labels[] =  {{ "hasnetwork",           SYSTEM_ETHERNET_LINK_ACTIVE },
                                  { "hasmediadvd",          SYSTEM_MEDIA_DVD },
                                  { "dvdready",             SYSTEM_DVDREADY },
                                  { "trayopen",             SYSTEM_TRAYOPEN },
                                  { "haslocks",             SYSTEM_HASLOCKS },
                                  { "hasloginscreen",       SYSTEM_HAS_LOGINSCREEN },
                                  { "ismaster",             SYSTEM_ISMASTER },
                                  { "loggedon",             SYSTEM_LOGGEDON },
                                  { "hasdrivef",            SYSTEM_HAS_DRIVE_F },
                                  { "hasdriveg",            SYSTEM_HAS_DRIVE_G },
                                  { "hddtemperature",       SYSTEM_HDD_TEMPERATURE },
                                  { "hddinfomodel",         SYSTEM_HDD_MODEL },
                                  { "hddinfofirmware",      SYSTEM_HDD_FIRMWARE },
                                  { "hddinfoserial",        SYSTEM_HDD_SERIAL },
                                  { "hddinfopw",            SYSTEM_HDD_PASSWORD },
                                  { "hddinfolockstate",     SYSTEM_HDD_LOCKSTATE },
                                  { "hddlockkey",           SYSTEM_HDD_LOCKKEY },
                                  { "hddbootdate",          SYSTEM_HDD_BOOTDATE },
                                  { "hddcyclecount",        SYSTEM_HDD_CYCLECOUNT },
                                  { "dvdinfomodel",         SYSTEM_DVD_MODEL },
                                  { "dvdinfofirmware",      SYSTEM_DVD_FIRMWARE },
                                  { "mplayerversion",       SYSTEM_MPLAYER_VERSION },
                                  { "canpowerdown",         SYSTEM_CAN_POWERDOWN },
                                  { "canreboot",            SYSTEM_CAN_REBOOT },
                                  { "cputemperature",       SYSTEM_CPU_TEMPERATURE },     // labels from here
                                  { "cpuusage",             SYSTEM_CPU_USAGE },
                                  { "gputemperature",       SYSTEM_GPU_TEMPERATURE },
                                  { "fanspeed",             SYSTEM_FAN_SPEED },
                                  { "freespace",            SYSTEM_FREE_SPACE },
                                  { "usedspace",            SYSTEM_USED_SPACE },
                                  { "totalspace",           SYSTEM_TOTAL_SPACE },
                                  { "usedspacepercent",     SYSTEM_USED_SPACE_PERCENT },
                                  { "freespacepercent",     SYSTEM_FREE_SPACE_PERCENT },
                                  { "freespace(c)",         SYSTEM_FREE_SPACE_C },
                                  { "usedspace(c)",         SYSTEM_USED_SPACE_C },
                                  { "totalspace(c)",        SYSTEM_TOTAL_SPACE_C },
                                  { "usedspacepercent(c)",  SYSTEM_USED_SPACE_PERCENT_C },
                                  { "freespacepercent(c)",  SYSTEM_FREE_SPACE_PERCENT_C },
                                  { "freespace(e)",         SYSTEM_FREE_SPACE_E },
                                  { "usedspace(e)",         SYSTEM_USED_SPACE_E },
                                  { "totalspace(e)",        SYSTEM_TOTAL_SPACE_E },
                                  { "usedspacepercent(e)",  SYSTEM_USED_SPACE_PERCENT_E },
                                  { "freespacepercent(e)",  SYSTEM_FREE_SPACE_PERCENT_E },
                                  { "freespace(f)",         SYSTEM_FREE_SPACE_F },
                                  { "usedspace(f)",         SYSTEM_USED_SPACE_F },
                                  { "totalspace(f)",        SYSTEM_TOTAL_SPACE_F },
                                  { "usedspacepercent(f)",  SYSTEM_USED_SPACE_PERCENT_F },
                                  { "freespacepercent(f)",  SYSTEM_FREE_SPACE_PERCENT_F },
                                  { "freespace(g)",         SYSTEM_FREE_SPACE_G },
                                  { "usedspace(g)",         SYSTEM_USED_SPACE_G },
                                  { "totalspace(g)",        SYSTEM_TOTAL_SPACE_G },
                                  { "usedspacepercent(g)",  SYSTEM_USED_SPACE_PERCENT_G },
                                  { "freespacepercent(g)",  SYSTEM_FREE_SPACE_PERCENT_G },
                                  { "usedspace(x)",         SYSTEM_USED_SPACE_X },
                                  { "freespace(x)",         SYSTEM_FREE_SPACE_X },
                                  { "totalspace(x)",        SYSTEM_TOTAL_SPACE_X },
                                  { "usedspace(y)",         SYSTEM_USED_SPACE_Y },
                                  { "freespace(y)",         SYSTEM_FREE_SPACE_Y },
                                  { "totalspace(y)",        SYSTEM_TOTAL_SPACE_Y },
                                  { "system.usedspace(z)",  SYSTEM_USED_SPACE_Z },
                                  { "freespace(z)",         SYSTEM_FREE_SPACE_Z },
                                  { "totalspace(z)",        SYSTEM_TOTAL_SPACE_Z },
                                  { "buildversion",         SYSTEM_BUILD_VERSION },
                                  { "builddate",            SYSTEM_BUILD_DATE },
                                  { "fps",                  SYSTEM_FPS },
                                  { "dvdtraystate",         SYSTEM_DVD_TRAY_STATE },
                                  { "freememory",           SYSTEM_FREE_MEMORY },
                                  { "language",             SYSTEM_LANGUAGE },
                                  { "temperatureunits",     SYSTEM_TEMPERATURE_UNITS },
                                  { "screenmode",           SYSTEM_SCREEN_MODE },
                                  { "screenwidth",          SYSTEM_SCREEN_WIDTH },
                                  { "screenheight",         SYSTEM_SCREEN_HEIGHT },
                                  { "currentwindow",        SYSTEM_CURRENT_WINDOW },
                                  { "currentcontrol",       SYSTEM_CURRENT_CONTROL },
                                  { "xboxnickname",         SYSTEM_XBOX_NICKNAME },
                                  { "dvdlabel",             SYSTEM_DVD_LABEL },
                                  { "internetstate",        SYSTEM_INTERNET_STATE },
                                  { "kernelversion",        SYSTEM_KERNEL_VERSION },
                                  { "uptime",               SYSTEM_UPTIME },
                                  { "totaluptime",          SYSTEM_TOTALUPTIME },
                                  { "cpufrequency",         SYSTEM_CPUFREQUENCY },
                                  { "xboxversion",          SYSTEM_XBOX_VERSION },
                                  { "avpackinfo",           SYSTEM_AV_PACK_INFO },
                                  { "screenresolution",     SYSTEM_SCREEN_RESOLUTION },
                                  { "videoencoderinfo",     SYSTEM_VIDEO_ENCODER_INFO },
                                  { "xboxproduceinfo",      SYSTEM_XBOX_PRODUCE_INFO },
                                  { "xboxserial",           SYSTEM_XBOX_SERIAL },
                                  { "xberegion",            SYSTEM_XBE_REGION },
                                  { "dvdzone",              SYSTEM_DVD_ZONE },
                                  { "bios",                 SYSTEM_XBOX_BIOS },
                                  { "modchip",              SYSTEM_XBOX_MODCHIP },
                                  { "profilename",          SYSTEM_PROFILENAME },
                                  { "profilethumb",         SYSTEM_PROFILETHUMB },
                                  { "launchxbe",            SYSTEM_LAUNCHING_XBE },
                                  { "progressbar",          SYSTEM_PROGRESS_BAR },
                                  { "friendlyname",         SYSTEM_FRIENDLY_NAME },
                                  { "alarmpos",             SYSTEM_ALARM_POS },
                                  { "startupwindow",        SYSTEM_STARTUP_WINDOW }};

const infomap system_param[] =   {{ "hasalarm",         SYSTEM_HAS_ALARM },
                                  { "getbool",          SYSTEM_GET_BOOL },
                                  { "setting",          SYSTEM_SETTING },
                                  { "hasaddon",         SYSTEM_HAS_ADDON }};

const infomap lcd_labels[] =     {{ "playicon",         LCD_PLAY_ICON },
                                  { "progressbar",      LCD_PROGRESS_BAR },
                                  { "cputemperature",   LCD_CPU_TEMPERATURE },
                                  { "gputemperature",   LCD_GPU_TEMPERATURE },
                                  { "hddtemperature",   LCD_HDD_TEMPERATURE },
                                  { "fanspeed",         LCD_FAN_SPEED },
                                  { "date",             LCD_DATE },
                                  { "freespace(c)",     LCD_FREE_SPACE_C },
                                  { "freespace(e)",     LCD_FREE_SPACE_E },
                                  { "freespace(f)",     LCD_FREE_SPACE_F },
                                  { "freespace(g)",     LCD_FREE_SPACE_G },
                                  { "time21",           LCD_TIME_21 },
                                  { "time22",           LCD_TIME_22 },
                                  { "timewide21",       LCD_TIME_W21 },
                                  { "timewide22",       LCD_TIME_W22 },
                                  { "time41",           LCD_TIME_41 },
                                  { "time42",           LCD_TIME_42 },
                                  { "time43",           LCD_TIME_43 },
                                  { "time44",           LCD_TIME_44 }};

const infomap network_labels[] = {{ "isdhcp",            NETWORK_IS_DHCP },
                                  { "ipaddress",         NETWORK_IP_ADDRESS }, //labels from here
                                  { "linkstate",         NETWORK_LINK_STATE },
                                  { "macaddress",        NETWORK_MAC_ADDRESS },
                                  { "subnetaddress",     NETWORK_SUBNET_ADDRESS },
                                  { "gatewayaddress",    NETWORK_GATEWAY_ADDRESS },
                                  { "dns1address",       NETWORK_DNS1_ADDRESS },
                                  { "dns2address",       NETWORK_DNS2_ADDRESS },
                                  { "dhcpaddress",       NETWORK_DHCP_ADDRESS }};

const infomap musicpartymode[] = {{ "enabled",           MUSICPM_ENABLED },
                                  { "songsplayed",       MUSICPM_SONGSPLAYED },
                                  { "matchingsongs",     MUSICPM_MATCHINGSONGS },
                                  { "matchingsongspicked", MUSICPM_MATCHINGSONGSPICKED },
                                  { "matchingsongsleft", MUSICPM_MATCHINGSONGSLEFT },
                                  { "relaxedsongspicked",MUSICPM_RELAXEDSONGSPICKED },
                                  { "randomsongspicked", MUSICPM_RANDOMSONGSPICKED }};

const infomap musicplayer[] =    {{ "title",            MUSICPLAYER_TITLE },
                                  { "album",            MUSICPLAYER_ALBUM },
                                  { "artist",           MUSICPLAYER_ARTIST },
                                  { "albumartist",      MUSICPLAYER_ALBUM_ARTIST },
                                  { "year",             MUSICPLAYER_YEAR },
                                  { "genre",            MUSICPLAYER_GENRE },
                                  { "duration",         MUSICPLAYER_DURATION },
                                  { "tracknumber",      MUSICPLAYER_TRACK_NUMBER },
                                  { "cover",            MUSICPLAYER_COVER },
                                  { "bitrate",          MUSICPLAYER_BITRATE },
                                  { "playlistlength",   MUSICPLAYER_PLAYLISTLEN },
                                  { "playlistposition", MUSICPLAYER_PLAYLISTPOS },
                                  { "channels",         MUSICPLAYER_CHANNELS },
                                  { "bitspersample",    MUSICPLAYER_BITSPERSAMPLE },
                                  { "samplerate",       MUSICPLAYER_SAMPLERATE },
                                  { "codec",            MUSICPLAYER_CODEC },
                                  { "discnumber",       MUSICPLAYER_DISC_NUMBER },
                                  { "rating",           MUSICPLAYER_RATING },
                                  { "comment",          MUSICPLAYER_COMMENT },
                                  { "lyrics",           MUSICPLAYER_LYRICS },
                                  { "playlistplaying",  MUSICPLAYER_PLAYLISTPLAYING },
                                  { "exists",           MUSICPLAYER_EXISTS },
                                  { "hasprevious",      MUSICPLAYER_HASPREVIOUS },
                                  { "hasnext",          MUSICPLAYER_HASNEXT },
                                  { "playcount",        MUSICPLAYER_PLAYCOUNT },
                                  { "lastplayed",       MUSICPLAYER_LASTPLAYED }};

const infomap videoplayer[] =    {{ "title",            VIDEOPLAYER_TITLE },
                                  { "genre",            VIDEOPLAYER_GENRE },
                                  { "country",          VIDEOPLAYER_COUNTRY },
                                  { "originaltitle",    VIDEOPLAYER_ORIGINALTITLE },
                                  { "director",         VIDEOPLAYER_DIRECTOR },
                                  { "year",             VIDEOPLAYER_YEAR },
                                  { "cover",            VIDEOPLAYER_COVER },
                                  { "usingoverlays",    VIDEOPLAYER_USING_OVERLAYS },
                                  { "isfullscreen",     VIDEOPLAYER_ISFULLSCREEN },
                                  { "hasmenu",          VIDEOPLAYER_HASMENU },
                                  { "playlistlength",   VIDEOPLAYER_PLAYLISTLEN },
                                  { "playlistposition", VIDEOPLAYER_PLAYLISTPOS },
                                  { "plot",             VIDEOPLAYER_PLOT },
                                  { "plotoutline",      VIDEOPLAYER_PLOT_OUTLINE },
                                  { "episode",          VIDEOPLAYER_EPISODE },
                                  { "season",           VIDEOPLAYER_SEASON },
                                  { "rating",           VIDEOPLAYER_RATING },
                                  { "ratingandvotes",   VIDEOPLAYER_RATING_AND_VOTES },
                                  { "tvshowtitle",      VIDEOPLAYER_TVSHOW },
                                  { "premiered",        VIDEOPLAYER_PREMIERED },
                                  { "studio",           VIDEOPLAYER_STUDIO },
                                  { "mpaa",             VIDEOPLAYER_MPAA },
                                  { "top250",           VIDEOPLAYER_TOP250 },
                                  { "cast",             VIDEOPLAYER_CAST },
                                  { "castandrole",      VIDEOPLAYER_CAST_AND_ROLE },
                                  { "artist",           VIDEOPLAYER_ARTIST },
                                  { "album",            VIDEOPLAYER_ALBUM },
                                  { "writer",           VIDEOPLAYER_WRITER },
                                  { "tagline",          VIDEOPLAYER_TAGLINE },
                                  { "hasinfo",          VIDEOPLAYER_HAS_INFO },
                                  { "trailer",          VIDEOPLAYER_TRAILER },
                                  { "videocodec",       VIDEOPLAYER_VIDEO_CODEC },
                                  { "videoresolution",  VIDEOPLAYER_VIDEO_RESOLUTION },
                                  { "videoaspect",      VIDEOPLAYER_VIDEO_ASPECT },
                                  { "audiocodec",       VIDEOPLAYER_AUDIO_CODEC },
                                  { "audiochannels",    VIDEOPLAYER_AUDIO_CHANNELS },
                                  { "lastplayed",       VIDEOPLAYER_LASTPLAYED },
                                  { "playcount",        VIDEOPLAYER_PLAYCOUNT },
                                  { "hassubtitles",     VIDEOPLAYER_HASSUBTITLES },
                                  { "subtitlesenabled", VIDEOPLAYER_SUBTITLESENABLED }};

const infomap mediacontainer[] = {{ "hasfiles",         CONTAINER_HASFILES },
                                  { "hasfolders",       CONTAINER_HASFOLDERS },
                                  { "isstacked",        CONTAINER_STACKED },
                                  { "folderpath",       CONTAINER_FOLDERPATH },
                                  { "foldername",       CONTAINER_FOLDERNAME },
                                  { "pluginname",       CONTAINER_PLUGINNAME },
                                  { "viewmode",         CONTAINER_VIEWMODE },
                                  { "viewcount",        CONTAINER_VIEWCOUNT },
                                  { "totaltime",        CONTAINER_TOTALTIME },
                                  { "hasthumb",         CONTAINER_HAS_THUMB },
                                  { "sortmethod",       CONTAINER_SORT_METHOD },
                                  { "sortorder",        CONTAINER_SORT_ORDER },
                                  { "showplot",         CONTAINER_SHOWPLOT }};

const infomap container_bools[] ={{ "onnext",          CONTAINER_MOVE_NEXT },
                                  { "onprevious",       CONTAINER_MOVE_PREVIOUS },
                                  { "onscrollnext",     CONTAINER_SCROLL_NEXT },
                                  { "onscrollprevious", CONTAINER_SCROLL_PREVIOUS },
                                  { "numpages",         CONTAINER_NUM_PAGES },
                                  { "numitems",         CONTAINER_NUM_ITEMS },
                                  { "currentpage",      CONTAINER_CURRENT_PAGE },
                                  { "scrolling",        CONTAINER_SCROLLING },
                                  { "hasnext",          CONTAINER_HAS_NEXT },
                                  { "hasprevious",      CONTAINER_HAS_PREVIOUS },
                                  { "canfilter",        CONTAINER_CAN_FILTER },
                                  { "canfilteradvanced",CONTAINER_CAN_FILTERADVANCED },
                                  { "filtered",         CONTAINER_FILTERED }};

const infomap container_ints[] = {{ "row",              CONTAINER_ROW },
                                  { "column",           CONTAINER_COLUMN },
                                  { "position",         CONTAINER_POSITION },
                                  { "currentitem",      CONTAINER_CURRENT_ITEM },
                                  { "subitem",          CONTAINER_SUBITEM },
                                  { "hasfocus",         CONTAINER_HAS_FOCUS }};

const infomap container_str[]  = {{ "property",         CONTAINER_PROPERTY },
                                  { "content",          CONTAINER_CONTENT },
                                  { "art",              CONTAINER_ART }};

const infomap listitem_labels[]= {{ "thumb",            LISTITEM_THUMB },
                                  { "icon",             LISTITEM_ICON },
                                  { "actualicon",       LISTITEM_ACTUAL_ICON },
                                  { "overlay",          LISTITEM_OVERLAY },
                                  { "label",            LISTITEM_LABEL },
                                  { "label2",           LISTITEM_LABEL2 },
                                  { "title",            LISTITEM_TITLE },
                                  { "tracknumber",      LISTITEM_TRACKNUMBER },
                                  { "artist",           LISTITEM_ARTIST },
                                  { "album",            LISTITEM_ALBUM },
                                  { "albumartist",      LISTITEM_ALBUM_ARTIST },
                                  { "year",             LISTITEM_YEAR },
                                  { "genre",            LISTITEM_GENRE },
                                  { "director",         LISTITEM_DIRECTOR },
                                  { "filename",         LISTITEM_FILENAME },
                                  { "filenameandpath",  LISTITEM_FILENAME_AND_PATH },
                                  { "fileextension",    LISTITEM_FILE_EXTENSION },
                                  { "date",             LISTITEM_DATE },
                                  { "size",             LISTITEM_SIZE },
                                  { "rating",           LISTITEM_RATING },
                                  { "ratingandvotes",   LISTITEM_RATING_AND_VOTES },
                                  { "programcount",     LISTITEM_PROGRAM_COUNT },
                                  { "duration",         LISTITEM_DURATION },
                                  { "isselected",       LISTITEM_ISSELECTED },
                                  { "isplaying",        LISTITEM_ISPLAYING },
                                  { "plot",             LISTITEM_PLOT },
                                  { "plotoutline",      LISTITEM_PLOT_OUTLINE },
                                  { "episode",          LISTITEM_EPISODE },
                                  { "season",           LISTITEM_SEASON },
                                  { "tvshowtitle",      LISTITEM_TVSHOW },
                                  { "premiered",        LISTITEM_PREMIERED },
                                  { "comment",          LISTITEM_COMMENT },
                                  { "path",             LISTITEM_PATH },
                                  { "foldername",       LISTITEM_FOLDERNAME },
                                  { "folderpath",       LISTITEM_FOLDERPATH },
                                  { "picturepath",      LISTITEM_PICTURE_PATH },
                                  { "pictureresolution",LISTITEM_PICTURE_RESOLUTION },
                                  { "picturedatetime",  LISTITEM_PICTURE_DATETIME },
                                  { "picturecomment",   LISTITEM_PICTURE_COMMENT },
                                  { "picturecaption",   LISTITEM_PICTURE_CAPTION },
                                  { "picturedesc",      LISTITEM_PICTURE_DESC },
                                  { "picturekeywords",  LISTITEM_PICTURE_KEYWORDS },
                                  { "picturecammake",   LISTITEM_PICTURE_CAM_MAKE },
                                  { "picturecammodel",  LISTITEM_PICTURE_CAM_MODEL },
                                  { "pictureaperture",  LISTITEM_PICTURE_APERTURE },
                                  { "picturefocallen",  LISTITEM_PICTURE_FOCAL_LEN },
                                  { "picturefocusdist", LISTITEM_PICTURE_FOCUS_DIST },
                                  { "pictureexpmode",   LISTITEM_PICTURE_EXP_MODE },
                                  { "pictureexptime",   LISTITEM_PICTURE_EXP_TIME },
                                  { "pictureiso",       LISTITEM_PICTURE_ISO },
                                  { "picturegpslat",    LISTITEM_PICTURE_GPS_LAT },
                                  { "picturegpslon",    LISTITEM_PICTURE_GPS_LON },
                                  { "picturegpsalt",    LISTITEM_PICTURE_GPS_ALT },
                                  { "studio",           LISTITEM_STUDIO },
                                  { "country",          LISTITEM_COUNTRY },
                                  { "mpaa",             LISTITEM_MPAA },
                                  { "cast",             LISTITEM_CAST },
                                  { "castandrole",      LISTITEM_CAST_AND_ROLE },
                                  { "writer",           LISTITEM_WRITER },
                                  { "tagline",          LISTITEM_TAGLINE },
                                  { "top250",           LISTITEM_TOP250 },
                                  { "trailer",          LISTITEM_TRAILER },
                                  { "starrating",       LISTITEM_STAR_RATING },
                                  { "sortletter",       LISTITEM_SORT_LETTER },
                                  { "tag",              LISTITEM_TAG },
                                  { "set",              LISTITEM_SET },
                                  { "videocodec",       LISTITEM_VIDEO_CODEC },
                                  { "videoresolution",  LISTITEM_VIDEO_RESOLUTION },
                                  { "videoaspect",      LISTITEM_VIDEO_ASPECT },
                                  { "audiocodec",       LISTITEM_AUDIO_CODEC },
                                  { "audiochannels",    LISTITEM_AUDIO_CHANNELS },
                                  { "audiolanguage",    LISTITEM_AUDIO_LANGUAGE },
                                  { "subtitlelanguage", LISTITEM_SUBTITLE_LANGUAGE },
                                  { "isresumable",      LISTITEM_IS_RESUMABLE},
                                  { "percentplayed",    LISTITEM_PERCENT_PLAYED},
                                  { "isfolder",         LISTITEM_IS_FOLDER },
                                  { "originaltitle",    LISTITEM_ORIGINALTITLE },
                                  { "lastplayed",       LISTITEM_LASTPLAYED },
                                  { "playcount",        LISTITEM_PLAYCOUNT },
                                  { "lastplayed",       MUSICPLAYER_LASTPLAYED },
                                  { "discnumber",       LISTITEM_DISC_NUMBER },
                                  { "dateadded",        LISTITEM_DATE_ADDED },
                                  { "dbtype",           LISTITEM_DBTYPE },
                                  { "dbid",             LISTITEM_DBID }};

const infomap visualisation[] =  {{ "locked",           VISUALISATION_LOCKED },
                                  { "preset",           VISUALISATION_PRESET },
                                  { "name",             VISUALISATION_NAME },
                                  { "enabled",          VISUALISATION_ENABLED }};

const infomap fanart_labels[] =  {{ "color1",           FANART_COLOR1 },
                                  { "color2",           FANART_COLOR2 },
                                  { "color3",           FANART_COLOR3 },
                                  { "image",            FANART_IMAGE }};

const infomap skin_labels[] =    {{ "currenttheme",     SKIN_THEME },
                                  { "currentcolourtheme",SKIN_COLOUR_THEME },
                                  { "aspectratio",      SKIN_ASPECT_RATIO}};

const infomap window_bools[] =   {{ "ismedia",          WINDOW_IS_MEDIA },
                                  { "is",               WINDOW_IS },
                                  { "isactive",         WINDOW_IS_ACTIVE },
                                  { "istopmost",        WINDOW_IS_TOPMOST },
                                  { "isvisible",        WINDOW_IS_VISIBLE },
                                  { "previous",         WINDOW_PREVIOUS },
                                  { "next",             WINDOW_NEXT }};

const infomap control_labels[] = {{ "hasfocus",         CONTROL_HAS_FOCUS },
                                  { "isvisible",        CONTROL_IS_VISIBLE },
                                  { "isenabled",        CONTROL_IS_ENABLED },
                                  { "getlabel",         CONTROL_GET_LABEL }};

const infomap playlist[] =       {{ "length",           PLAYLIST_LENGTH },
                                  { "position",         PLAYLIST_POSITION },
                                  { "random",           PLAYLIST_RANDOM },
                                  { "repeat",           PLAYLIST_REPEAT },
                                  { "israndom",         PLAYLIST_ISRANDOM },
                                  { "isrepeat",         PLAYLIST_ISREPEAT },
                                  { "isrepeatone",      PLAYLIST_ISREPEATONE }};

const int picture_slide_map[]  = {/* LISTITEM_PICTURE_RESOLUTION => */ SLIDE_RESOLUTION,
                                  /* LISTITEM_PICTURE_DATE       => */ SLIDE_EXIF_DATE,
                                  /* LISTITEM_PICTURE_DATETIME   => */ SLIDE_EXIF_DATE_TIME,
                                  /* LISTITEM_PICTURE_COMMENT    => */ SLIDE_COMMENT,
                                  /* LISTITEM_PICTURE_CAPTION    => */ SLIDE_IPTC_CAPTION,
                                  /* LISTITEM_PICTURE_DESC       => */ SLIDE_EXIF_DESCRIPTION,
                                  /* LISTITEM_PICTURE_KEYWORDS   => */ SLIDE_IPTC_KEYWORDS,
                                  /* LISTITEM_PICTURE_CAM_MAKE   => */ SLIDE_EXIF_CAMERA_MAKE,
                                  /* LISTITEM_PICTURE_CAM_MODEL  => */ SLIDE_EXIF_CAMERA_MODEL,
                                  /* LISTITEM_PICTURE_APERTURE   => */ SLIDE_EXIF_APERTURE,
                                  /* LISTITEM_PICTURE_FOCAL_LEN  => */ SLIDE_EXIF_FOCAL_LENGTH,
                                  /* LISTITEM_PICTURE_FOCUS_DIST => */ SLIDE_EXIF_FOCUS_DIST,
                                  /* LISTITEM_PICTURE_EXP_MODE   => */ SLIDE_EXIF_EXPOSURE_MODE,
                                  /* LISTITEM_PICTURE_EXP_TIME   => */ SLIDE_EXIF_EXPOSURE_TIME,
                                  /* LISTITEM_PICTURE_ISO        => */ SLIDE_EXIF_ISO_EQUIV,
                                  /* LISTITEM_PICTURE_GPS_LAT    => */ SLIDE_EXIF_GPS_LATITUDE,
                                  /* LISTITEM_PICTURE_GPS_LON    => */ SLIDE_EXIF_GPS_LONGITUDE,
                                  /* LISTITEM_PICTURE_GPS_ALT    => */ SLIDE_EXIF_GPS_ALTITUDE };

const infomap bar[] =            {{ "gputemperature",  SYSTEM_GPU_TEMPERATURE },
                                  { "cputemperature",  SYSTEM_CPU_TEMPERATURE },
                                  { "cpuusage",        SYSTEM_CPU_USAGE },
                                  { "freememory",      SYSTEM_FREE_MEMORY },
                                  { "usedmemory",      SYSTEM_USED_MEMORY },
                                  { "fanspeed",        SYSTEM_FAN_SPEED },
                                  { "usedspace",       SYSTEM_USED_SPACE },
                                  { "freespace",       SYSTEM_FREE_SPACE },
                                  { "usedspace(c)",    SYSTEM_USED_SPACE_C },
                                  { "freespace(c)",    SYSTEM_FREE_SPACE_C },
                                  { "usedspace(e)",    SYSTEM_USED_SPACE_E }, 
                                  { "freespace(e)",    SYSTEM_FREE_SPACE_E },
                                  { "usedspace(f)",    SYSTEM_USED_SPACE_F },
                                  { "freespace(f)",    SYSTEM_FREE_SPACE_F },
                                  { "usedspace(g)",    SYSTEM_USED_SPACE_G },
                                  { "freespace(g)",    SYSTEM_FREE_SPACE_G },
                                  { "usedspace(x)",    SYSTEM_USED_SPACE_X },
                                  { "freespace(x)",    SYSTEM_FREE_SPACE_X },
                                  { "usedspace(y)",    SYSTEM_USED_SPACE_Y },
                                  { "freespace(y)",    SYSTEM_FREE_SPACE_Y },
                                  { "usedspace(z)",    SYSTEM_USED_SPACE_Z },
                                  { "freespace(z)",    SYSTEM_FREE_SPACE_Z },
                                  { "hddtemperature",  SYSTEM_HDD_TEMPERATURE }};

CGUIInfoManager::Property::Property(const CStdString &property, const CStdString &parameters)
: name(property)
{
  CUtil::SplitParams(parameters, params);
}

const CStdString &CGUIInfoManager::Property::param(unsigned int n /* = 0 */) const
{
  if (n < params.size())
    return params[n];
  return StringUtils::EmptyString;
}

unsigned int CGUIInfoManager::Property::num_params() const
{
  return params.size();
}

void CGUIInfoManager::SplitInfoString(const CStdString &infoString, vector<Property> &info)
{
  // our string is of the form:
  // category[(params)][.info(params).info2(params)] ...
  // so we need to split on . while taking into account of () pairs
  unsigned int parentheses = 0;
  CStdString property;
  CStdString param;
  for (size_t i = 0; i < infoString.size(); ++i)
  {
    if (infoString[i] == '(')
    {
      if (!parentheses++)
        continue;
    }
    else if (infoString[i] == ')')
    {
      if (!parentheses)
        CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
      else if (!--parentheses)
        continue;
    }
    else if (infoString[i] == '.' && !parentheses)
    {
      if (!property.IsEmpty()) // add our property and parameters
        info.push_back(Property(property.ToLower(), param));
      property.clear();
      param.clear();
      continue;
    }
    if (parentheses)
      param += infoString[i];
    else
      property += infoString[i];
  }
  if (parentheses)
    CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
  if (!property.IsEmpty())
    info.push_back(Property(property.ToLower(), param));
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const CStdString &strCondition)
{
  bool listItemDependent;
  return TranslateSingleString(strCondition, listItemDependent);
}

int CGUIInfoManager::TranslateSingleString(const CStdString &strCondition, bool &listItemDependent)
{
  /* We need to disable caching in INFO::InfoBool::Get if either of the following are true:
   *  1. if condition is between LISTITEM_START and LISTITEM_END
   *  2. if condition is STRING_IS_EMPTY, STRING_COMPARE, STRING_STR, INTEGER_GREATER_THAN and the
   *     corresponding label is between LISTITEM_START and LISTITEM_END
   *  This is achieved by setting the bool pointed at by listItemDependent, either here or in a recursive call
   */
  // trim whitespace, and convert to lowercase
  CStdString strTest = strCondition;
  strTest.TrimLeft(" \t\r\n");
  strTest.TrimRight(" \t\r\n");

  vector< Property> info;
  SplitInfoString(strTest, info);

  if (info.empty())
    return 0;

  const Property &cat = info[0];
  if (info.size() == 1)
  { // single category
    if (cat.name == "false" || cat.name == "no")
      return SYSTEM_ALWAYS_FALSE;
    else if (cat.name == "true" || cat.name == "yes")
      return SYSTEM_ALWAYS_TRUE;

    // deprecated begin
    if (cat.name == "isempty" && cat.num_params() == 1)
      return AddMultiInfo(GUIInfo(STRING_IS_EMPTY, TranslateSingleString(cat.param(), listItemDependent)));
    else if (cat.name == "stringcompare" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
      CStdString label = CGUIInfoLabel::GetLabel(cat.param(1)).ToLower();
      // 'true', 'false', 'yes', 'no' are valid strings, do not resolve them to SYSTEM_ALWAYS_TRUE or SYSTEM_ALWAYS_FALSE
      if (label != "true" && label != "false" && label != "yes" && label != "no")
      {
        int info2 = TranslateSingleString(cat.param(1), listItemDependent);
        if (info2 > 0)
          return AddMultiInfo(GUIInfo(STRING_COMPARE, info, -info2));
      }
      int compareString = ConditionalStringParameter(label);
      return AddMultiInfo(GUIInfo(STRING_COMPARE, info, compareString));
    }
    else if (cat.name == "integergreaterthan" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      int compareInt = atoi(cat.param(1).c_str());
      return AddMultiInfo(GUIInfo(INTEGER_GREATER_THAN, info, compareInt));
    }
    else if (cat.name == "substring" && cat.num_params() >= 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      CStdString label = CGUIInfoLabel::GetLabel(cat.param(1)).ToLower();
      int compareString = ConditionalStringParameter(label);
      if (cat.num_params() > 2)
      {
        if (cat.param(2).CompareNoCase("left") == 0)
          return AddMultiInfo(GUIInfo(STRING_STR_LEFT, info, compareString));
        else if (cat.param(2).CompareNoCase("right") == 0)
          return AddMultiInfo(GUIInfo(STRING_STR_RIGHT, info, compareString));
      }
      return AddMultiInfo(GUIInfo(STRING_STR, info, compareString));
    }
    else if (cat.name == "istrue" && cat.num_params() == 1)
    {
      return AddMultiInfo(GUIInfo(VALUE_IS_TRUE, TranslateSingleString(cat.param(0))));
    }
    // deprecated end
  }
  else if (info.size() == 2)
  {
    const Property &prop = info[1];
    if (cat.name == "string")
    {
      if (prop.name == "isempty")
      {
        return AddMultiInfo(GUIInfo(STRING_IS_EMPTY, TranslateSingleString(prop.param())));
      }
      else if (prop.num_params() == 2)
      {
        for (size_t i = 0; i < sizeof(string_bools) / sizeof(infomap); i++)
        {
          if (prop.name == string_bools[i].str)
          {
            int data1 = TranslateSingleString(prop.param(0));
            // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
            CStdString label = CGUIInfoLabel::GetLabel(prop.param(1)).ToLower();
            // 'true', 'false', 'yes', 'no' are valid strings, do not resolve them to SYSTEM_ALWAYS_TRUE or SYSTEM_ALWAYS_FALSE
            if (label != "true" && label != "false" && label != "yes" && label != "no")
            {
              int data2 = TranslateSingleString(prop.param(1));
              if (data2 > 0)
                return AddMultiInfo(GUIInfo(string_bools[i].val, data1, -data2));
            }
            return AddMultiInfo(GUIInfo(string_bools[i].val, data1, ConditionalStringParameter(label)));
          }
        }
      }
    }
    if (cat.name == "integer")
    {
      for (size_t i = 0; i < sizeof(integer_bools) / sizeof(infomap); i++)
      {
        if (prop.name == integer_bools[i].str)
        {
          int data1 = TranslateSingleString(prop.param(0));
          int data2 = atoi(prop.param(1).c_str());
          return AddMultiInfo(GUIInfo(integer_bools[i].val, data1, data2));
        }
      }
    }
    else if (cat.name == "player")
    {
      for (size_t i = 0; i < sizeof(player_labels) / sizeof(infomap); i++)
      {
        if (prop.name == player_labels[i].str)
          return player_labels[i].val;
      }
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++)
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.num_params() == 1)
      {
        for (size_t i = 0; i < sizeof(player_param) / sizeof(infomap); i++)
        {
          if (prop.name == player_param[i].str)
            return AddMultiInfo(GUIInfo(player_param[i].val, ConditionalStringParameter(prop.param())));
        }
      }
    }
    else if (cat.name == "weather")
    {
      for (size_t i = 0; i < sizeof(weather) / sizeof(infomap); i++)
      {
        if (prop.name == weather[i].str)
          return weather[i].val;
      }
    }
    else if (cat.name == "lcd")
    {
      for (size_t i = 0; i < sizeof(lcd_labels) / sizeof(infomap); i++)
      {
        if (prop.name == lcd_labels[i].str)
          return lcd_labels[i].val;
      }
    }
    else if (cat.name == "network")
    {
      for (size_t i = 0; i < sizeof(network_labels) / sizeof(infomap); i++)
      {
        if (prop.name == network_labels[i].str)
          return network_labels[i].val;
      }
    }
    else if (cat.name == "musicpartymode")
    {
      for (size_t i = 0; i < sizeof(musicpartymode) / sizeof(infomap); i++)
      {
        if (prop.name == musicpartymode[i].str)
          return musicpartymode[i].val;
      }
    }
    else if (cat.name == "system")
    {
      for (size_t i = 0; i < sizeof(system_labels) / sizeof(infomap); i++)
      {
        if (prop.name == system_labels[i].str)
          return system_labels[i].val;
      }
      if (prop.num_params() == 1)
      {
        const CStdString &param = prop.param();
        for (size_t i = 0; i < sizeof(system_param) / sizeof(infomap); i++)
        {
          if (prop.name == system_param[i].str)
            return AddMultiInfo(GUIInfo(system_param[i].val, ConditionalStringParameter(param)));
        }
        if (prop.name == "memory")
        {
          if (param == "free") return SYSTEM_FREE_MEMORY;
          else if (param == "free.percent") return SYSTEM_FREE_MEMORY_PERCENT;
          else if (param == "used") return SYSTEM_USED_MEMORY;
          else if (param == "used.percent") return SYSTEM_USED_MEMORY_PERCENT;
          else if (param == "total") return SYSTEM_TOTAL_MEMORY;
        }
        else if (prop.name == "idletime")
          return AddMultiInfo(GUIInfo(SYSTEM_IDLE_TIME, atoi(param.c_str())));
        else if (prop.name == "addontitle")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, infoLabel, 0));
          CStdString label = CGUIInfoLabel::GetLabel(param).ToLower();
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonicon")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, infoLabel, 0));
          CStdString label = CGUIInfoLabel::GetLabel(param).ToLower();
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonversion")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, infoLabel, 0));
          CStdString label = CGUIInfoLabel::GetLabel(param).ToLower();
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "controllerport")
        {
          int i_ControllerPort = atoi(param);
          if (i_ControllerPort == 1) return SYSTEM_CONTROLLER_PORT_1;
          else if (i_ControllerPort == 2) return SYSTEM_CONTROLLER_PORT_2;
          else if (i_ControllerPort == 3) return SYSTEM_CONTROLLER_PORT_3;
          else if (i_ControllerPort == 4)return SYSTEM_CONTROLLER_PORT_4;
          else return SYSTEM_CONTROLLER_PORT_1;
        }
      }
      else if (prop.name == "alarmlessorequal" && prop.num_params() == 2)
        return AddMultiInfo(GUIInfo(SYSTEM_ALARM_LESS_OR_EQUAL, ConditionalStringParameter(prop.param(0)), ConditionalStringParameter(prop.param(1))));
      else if (prop.name == "date")
      {
        if (prop.num_params() == 2)
          return AddMultiInfo(GUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000, StringUtils::DateStringToYYYYMMDD(prop.param(1)) % 10000));
        else if (prop.num_params() == 1)
          return AddMultiInfo(GUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000));
        return SYSTEM_DATE;
      }
      else if (prop.name == "time")
      {
        if (prop.num_params() == 0)
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, TIME_FORMAT_GUESS));
        if (prop.num_params() == 1)
        {
          TIME_FORMAT timeFormat = TranslateTimeFormat(prop.param(0));
          if (timeFormat == TIME_FORMAT_GUESS)
            return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0))));
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, timeFormat));
        }
        else
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0)), StringUtils::TimeStringToSeconds(prop.param(1))));
      }
    }
    else if (cat.name == "library")
    {
      if (prop.name == "isscanning") return LIBRARY_IS_SCANNING;
      else if (prop.name == "isscanningvideo") return LIBRARY_IS_SCANNING_VIDEO; // TODO: change to IsScanning(Video)
      else if (prop.name == "isscanningmusic") return LIBRARY_IS_SCANNING_MUSIC;
      else if (prop.name == "hascontent" && prop.num_params())
      {
        CStdString cat = prop.param(0); cat.ToLower();
        if (cat == "music") return LIBRARY_HAS_MUSIC;
        else if (cat == "video") return LIBRARY_HAS_VIDEO;
        else if (cat == "movies") return LIBRARY_HAS_MOVIES;
        else if (cat == "tvshows") return LIBRARY_HAS_TVSHOWS;
        else if (cat == "musicvideos") return LIBRARY_HAS_MUSICVIDEOS;
        else if (cat == "moviesets") return LIBRARY_HAS_MOVIE_SETS;
      }
    }
    else if (cat.name == "musicplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) // TODO: remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "property")
      {
        if (prop.param().Equals("fanart_image"))
          return AddMultiInfo(GUIInfo(PLAYER_ITEM_ART, ConditionalStringParameter("fanart")));
        return AddListItemProp(prop.param(), MUSICPLAYER_PROPERTY_OFFSET);
      }
      return TranslateMusicPlayerString(prop.name);
    }
    else if (cat.name == "videoplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) // TODO: remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(GUIInfo(VIDEOPLAYER_CONTENT, ConditionalStringParameter(prop.param()), 0));
      for (size_t i = 0; i < sizeof(videoplayer) / sizeof(infomap); i++)
      {
        if (prop.name == videoplayer[i].str)
          return videoplayer[i].val;
      }
    }
    else if (cat.name == "slideshow")
      return CPictureInfoTag::TranslateString(prop.name);
    else if (cat.name == "container")
    {
      for (size_t i = 0; i < sizeof(mediacontainer) / sizeof(infomap); i++) // these ones don't have or need an id
      {
        if (prop.name == mediacontainer[i].str)
          return mediacontainer[i].val;
      }
      int id = atoi(cat.param().c_str());
      for (size_t i = 0; i < sizeof(container_bools) / sizeof(infomap); i++) // these ones can have an id (but don't need to?)
      {
        if (prop.name == container_bools[i].str)
          return id ? AddMultiInfo(GUIInfo(container_bools[i].val, id)) : container_bools[i].val;
      }
      for (size_t i = 0; i < sizeof(container_ints) / sizeof(infomap); i++) // these ones can have an int param on the property
      {
        if (prop.name == container_ints[i].str)
          return AddMultiInfo(GUIInfo(container_ints[i].val, id, atoi(prop.param().c_str())));
      }
      for (size_t i = 0; i < sizeof(container_str) / sizeof(infomap); i++) // these ones have a string param on the property
      {
        if (prop.name == container_str[i].str)
          return AddMultiInfo(GUIInfo(container_str[i].val, id, ConditionalStringParameter(prop.param())));
      }
      if (prop.name == "sortdirection")
      {
        SortOrder order = SortOrderNone;
        if (prop.param().Equals("ascending"))
          order = SortOrderAscending;
        else if (prop.param().Equals("descending"))
          order = SortOrderDescending;
        return AddMultiInfo(GUIInfo(CONTAINER_SORT_DIRECTION, order));
      }
    }
    else if (cat.name == "listitem")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_WRAP));
      return ret;
    }
    else if (cat.name == "listitemposition")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_POSITION));
      return ret;
    }
    else if (cat.name == "listitemnowrap")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset));
      return ret;
    }
    else if (cat.name == "visualisation")
    {
      for (size_t i = 0; i < sizeof(visualisation) / sizeof(infomap); i++)
      {
        if (prop.name == visualisation[i].str)
          return visualisation[i].val;
      }
    }
    else if (cat.name == "fanart")
    {
      for (size_t i = 0; i < sizeof(fanart_labels) / sizeof(infomap); i++)
      {
        if (prop.name == fanart_labels[i].str)
          return fanart_labels[i].val;
      }
    }
    else if (cat.name == "skin")
    {
      for (size_t i = 0; i < sizeof(skin_labels) / sizeof(infomap); i++)
      {
        if (prop.name == skin_labels[i].str)
          return skin_labels[i].val;
      }
      if (prop.num_params())
      {
        if (prop.name == "string")
        {
          if (prop.num_params() == 2)
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::Get().TranslateString(prop.param(0)), ConditionalStringParameter(prop.param(1))));
          else
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::Get().TranslateString(prop.param(0))));
        }
        if (prop.name == "hassetting")
          return AddMultiInfo(GUIInfo(SKIN_BOOL, CSkinSettings::Get().TranslateBool(prop.param(0))));
        else if (prop.name == "hastheme")
          return AddMultiInfo(GUIInfo(SKIN_HAS_THEME, ConditionalStringParameter(prop.param(0))));
      }
    }
   else if (cat.name == "window")
    {
      if (prop.name == "property" && prop.num_params() == 1)
      { // TODO: this doesn't support foo.xml
        int winID = cat.param().IsEmpty() ? 0 : CButtonTranslator::TranslateWindow(cat.param());
        if (winID != WINDOW_INVALID)
          return AddMultiInfo(GUIInfo(WINDOW_PROPERTY, winID, ConditionalStringParameter(prop.param())));
      }
      for (size_t i = 0; i < sizeof(window_bools) / sizeof(infomap); i++)
      {
        if (prop.name == window_bools[i].str)
        { // TODO: The parameter for these should really be on the first not the second property
          if (prop.param().Find("xml") >= 0)
            return AddMultiInfo(GUIInfo(window_bools[i].val, 0, ConditionalStringParameter(prop.param())));
          int winID = prop.param().IsEmpty() ? 0 : CButtonTranslator::TranslateWindow(prop.param());
          if (winID != WINDOW_INVALID)
            return AddMultiInfo(GUIInfo(window_bools[i].val, winID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "control")
    {
      for (size_t i = 0; i < sizeof(control_labels) / sizeof(infomap); i++)
      {
        if (prop.name == control_labels[i].str)
        { // TODO: The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(GUIInfo(control_labels[i].val, controlID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "controlgroup" && prop.name == "hasfocus")
    {
      int groupID = atoi(cat.param().c_str());
      if (groupID)
        return AddMultiInfo(GUIInfo(CONTROL_GROUP_HAS_FOCUS, groupID, atoi(prop.param(0).c_str())));
    }
    else if (cat.name == "playlist")
    {
      for (size_t i = 0; i < sizeof(playlist) / sizeof(infomap); i++)
      {
        if (prop.name == playlist[i].str)
          return playlist[i].val;
      }
    }
    else if (cat.name == "bar")
    {
      for (size_t i = 0; i < sizeof(bar) / sizeof(infomap); i++)
      {
        if (prop.name == bar[i].str)
          return bar[i].val;
      }
    }
    /*else if (cat.name == "addon" && prop.name == "setting")
    {
      return AddMultiInfo(GUIInfo(WINDOW_PROPERTY, WINDOW_DIALOG_PLUGIN_SETTINGS, ConditionalStringParameter(prop.param(0))));
    }*/
  }
  else if (info.size() == 3)
  {
    if (info[0].name == "system" && info[1].name == "platform")
    { // TODO: replace with a single system.platform
      CStdString platform = info[2].name;
      if (platform == "linux") return SYSTEM_PLATFORM_LINUX;
      else if (platform == "windows") return SYSTEM_PLATFORM_WINDOWS;
      else if (platform == "osx") return SYSTEM_PLATFORM_OSX;
      else if (platform == "xbox") return SYSTEM_PLATFORM_XBOX;
    }
    if (info[0].name == "musicplayer")
    { // TODO: these two don't allow duration(foo) and also don't allow more than this number of levels...
      if (info[1].name == "position")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.position(foo).bar
        return AddMultiInfo(GUIInfo(value, 0, position));
      }
      else if (info[1].name == "offset")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.offset(foo).bar
        return AddMultiInfo(GUIInfo(value, 1, position));
      }
    }
    else if (info[0].name == "container")
    {
      int id = atoi(info[0].param().c_str());
      int offset = atoi(info[1].param().c_str());
      if (info[1].name == "listitemnowrap")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset));
      }
      else if (info[1].name == "listitemposition")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_POSITION));
      }
      else if (info[1].name == "listitem")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_WRAP));
      }
    }
  }

  return 0;
}

int CGUIInfoManager::TranslateListItem(const Property &info)
{
  for (size_t i = 0; i < sizeof(listitem_labels) / sizeof(infomap); i++) // these ones don't have or need an id
  {
    if (info.name == listitem_labels[i].str)
      return listitem_labels[i].val;
  }
  if (info.name == "property" && info.num_params() == 1)
  {
    if (info.param().Equals("fanart_image"))
      return AddListItemProp("fanart", LISTITEM_ART_OFFSET);
    return AddListItemProp(info.param());
  }
  if (info.name == "art" && info.num_params() == 1)
    return AddListItemProp(info.param(), LISTITEM_ART_OFFSET);
  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const CStdString &info) const
{
  for (size_t i = 0; i < sizeof(musicplayer) / sizeof(infomap); i++)
  {
    if (info == musicplayer[i].str)
      return musicplayer[i].val;
  }
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const CStdString &format)
{
  if (format.IsEmpty()) return TIME_FORMAT_GUESS;
  else if (format.Equals("hh")) return TIME_FORMAT_HH;
  else if (format.Equals("mm")) return TIME_FORMAT_MM;
  else if (format.Equals("ss")) return TIME_FORMAT_SS;
  else if (format.Equals("hh:mm")) return TIME_FORMAT_HH_MM;
  else if (format.Equals("mm:ss")) return TIME_FORMAT_MM_SS;
  else if (format.Equals("hh:mm:ss")) return TIME_FORMAT_HH_MM_SS;
  else if (format.Equals("h")) return TIME_FORMAT_H;
  else if (format.Equals("h:mm:ss")) return TIME_FORMAT_H_MM_SS;
  else if (format.Equals("xx")) return TIME_FORMAT_XX;
  return TIME_FORMAT_GUESS;
}

CStdString CGUIInfoManager::GetLabel(int info, int contextWindow, CStdString *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false);

  CStdString strLabel;
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= SLIDE_INFO_START && info <= SLIDE_INFO_END)
    return GetPictureLabel(info);

  if (info >= LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET &&
      info - (LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET) < (int)m_listitemProperties.size())
  { // grab the property
    if (!m_currentFile)
      return "";

    CStdString property = m_listitemProperties[info - LISTITEM_PROPERTY_START-MUSICPLAYER_PROPERTY_OFFSET];
    return m_currentFile->GetProperty(property).asString();
  }

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      strLabel = GetItemLabel(item.get(), info, fallback);
    }

    return strLabel;
  }

  switch (info)
  {
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    strLabel = strLabel.Trim();
    break;
  case WEATHER_TEMPERATURE:
    strLabel.Format("%s%s", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP), g_langInfo.GetTempUnitString().c_str());
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case WEATHER_FANART_CODE:
    strLabel = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
    URIUtils::RemoveExtension(strLabel);
    break;
  case WEATHER_PLUGIN:
    strLabel = CSettings::Get().GetString("weather.addon");
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case SYSTEM_LAUNCHING_XBE:
    strLabel = m_launchingXBE;
    break;
  case LCD_DATE:
    strLabel = GetDate(true);
    break;
  case SYSTEM_FPS:
    strLabel.Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel.Format("%2.1f dB", (float)(g_application.GetVolume(false) + g_application.GetDynamicRangeCompressionLevel()) * 0.01f);
    break;
  case PLAYER_SUBTITLE_DELAY:
    strLabel.Format("%2.3f s", CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay);
    break;
  case PLAYER_AUDIO_DELAY:
    strLabel.Format("%2.3f s", CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay);
    break;
  case PLAYER_CHAPTER:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapter());
    break;
  case PLAYER_CHAPTERCOUNT:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapterCount());
    break;
  case PLAYER_CHAPTERNAME:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      g_application.m_pPlayer->GetChapterName(strLabel);
    break;
  case PLAYER_CACHELEVEL:
    {
      int iLevel = 0;
      if(g_application.IsPlaying() && GetInt(iLevel, PLAYER_CACHELEVEL) && iLevel >= 0)
        strLabel.Format("%i", iLevel);
    }
    break;
  case PLAYER_TIME:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = GetCurrentPlayTime(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_DURATION:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = GetDuration(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_PATH:
  case PLAYER_FILEPATH:
     if (m_currentFile)
     {
       if (m_currentFile->HasMusicInfoTag())
         strLabel = m_currentFile->GetMusicInfoTag()->GetURL();
       else if (m_currentFile->HasVideoInfoTag())
         strLabel = m_currentFile->GetVideoInfoTag()->m_strFileNameAndPath;
       if (strLabel.IsEmpty())
         strLabel = m_currentFile->GetPath();
     }
     if (info == PLAYER_PATH)
     {
       // do this twice since we want the path outside the archive if this
       // is to be of use.
       if (URIUtils::IsInArchive(strLabel))
         strLabel = URIUtils::GetParentPath(strLabel);
       strLabel = URIUtils::GetParentPath(strLabel);
     }
     break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
  case MUSICPLAYER_ALBUM_ARTIST:
  case MUSICPLAYER_GENRE:
  case MUSICPLAYER_YEAR:
  case MUSICPLAYER_TRACK_NUMBER:
  case MUSICPLAYER_BITRATE:
  case MUSICPLAYER_PLAYLISTLEN:
  case MUSICPLAYER_PLAYLISTPOS:
  case MUSICPLAYER_CHANNELS:
  case MUSICPLAYER_BITSPERSAMPLE:
  case MUSICPLAYER_SAMPLERATE:
  case MUSICPLAYER_CODEC:
  case MUSICPLAYER_DISC_NUMBER:
  case MUSICPLAYER_RATING:
  case MUSICPLAYER_COMMENT:
  case MUSICPLAYER_LYRICS:
  case MUSICPLAYER_PLAYCOUNT:
  case MUSICPLAYER_LASTPLAYED:
    strLabel = GetMusicLabel(info);
  break;
  case VIDEOPLAYER_TITLE:
  case VIDEOPLAYER_ORIGINALTITLE:
  case VIDEOPLAYER_GENRE:
  case VIDEOPLAYER_DIRECTOR:
  case VIDEOPLAYER_YEAR:
  case VIDEOPLAYER_PLAYLISTLEN:
  case VIDEOPLAYER_PLAYLISTPOS:
  case VIDEOPLAYER_PLOT:
  case VIDEOPLAYER_PLOT_OUTLINE:
  case VIDEOPLAYER_EPISODE:
  case VIDEOPLAYER_SEASON:
  case VIDEOPLAYER_RATING:
  case VIDEOPLAYER_RATING_AND_VOTES:
  case VIDEOPLAYER_TVSHOW:
  case VIDEOPLAYER_PREMIERED:
  case VIDEOPLAYER_STUDIO:
  case VIDEOPLAYER_COUNTRY:
  case VIDEOPLAYER_MPAA:
  case VIDEOPLAYER_TOP250:
  case VIDEOPLAYER_CAST:
  case VIDEOPLAYER_CAST_AND_ROLE:
  case VIDEOPLAYER_ARTIST:
  case VIDEOPLAYER_ALBUM:
  case VIDEOPLAYER_WRITER:
  case VIDEOPLAYER_TAGLINE:
  case VIDEOPLAYER_TRAILER:
  case VIDEOPLAYER_PLAYCOUNT:
  case VIDEOPLAYER_LASTPLAYED:
    strLabel = GetVideoLabel(info);
  break;
  case VIDEOPLAYER_VIDEO_CODEC:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = g_application.m_pPlayer->GetVideoCodecName();
    break;
  case VIDEOPLAYER_VIDEO_RESOLUTION:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      return CStreamDetails::VideoDimsToResolutionDescription(g_application.m_pPlayer->GetPictureWidth(), g_application.m_pPlayer->GetPictureHeight());
    break;
  case VIDEOPLAYER_AUDIO_CODEC:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = g_application.m_pPlayer->GetAudioCodecName();
    break;
  case VIDEOPLAYER_VIDEO_ASPECT:
    if (g_application.IsPlaying() && g_application.m_pPlayer)
    {
      float aspect;
      g_application.m_pPlayer->GetVideoAspectRatio(aspect);
      strLabel = CStreamDetails::VideoAspectToAspectDescription(aspect);
    }
    break;
  case VIDEOPLAYER_AUDIO_CHANNELS:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%i", g_application.m_pPlayer->GetChannels());
    break;
  case PLAYLIST_LENGTH:
  case PLAYLIST_POSITION:
  case PLAYLIST_RANDOM:
  case PLAYLIST_REPEAT:
    strLabel = GetPlaylistLabel(info);
  break;
  case MUSICPM_SONGSPLAYED:
  case MUSICPM_MATCHINGSONGS:
  case MUSICPM_MATCHINGSONGSPICKED:
  case MUSICPM_MATCHINGSONGSLEFT:
  case MUSICPM_RELAXEDSONGSPICKED:
  case MUSICPM_RANDOMSONGSPICKED:
    strLabel = GetMusicPartyModeLabel(info);
  break;

  case SYSTEM_FREE_SPACE:
  case SYSTEM_FREE_SPACE_C:
  case SYSTEM_FREE_SPACE_E:
  case SYSTEM_FREE_SPACE_F:
  case SYSTEM_FREE_SPACE_G:
  case SYSTEM_USED_SPACE:
  case SYSTEM_USED_SPACE_C:
  case SYSTEM_USED_SPACE_E:
  case SYSTEM_USED_SPACE_F:
  case SYSTEM_USED_SPACE_G:
  case SYSTEM_TOTAL_SPACE:
  case SYSTEM_TOTAL_SPACE_C:
  case SYSTEM_TOTAL_SPACE_E:
  case SYSTEM_TOTAL_SPACE_F:
  case SYSTEM_TOTAL_SPACE_G:
  case SYSTEM_FREE_SPACE_PERCENT:
  case SYSTEM_FREE_SPACE_PERCENT_C:
  case SYSTEM_FREE_SPACE_PERCENT_E:
  case SYSTEM_FREE_SPACE_PERCENT_F:
  case SYSTEM_FREE_SPACE_PERCENT_G:
  case SYSTEM_USED_SPACE_PERCENT:
  case SYSTEM_USED_SPACE_PERCENT_C:
  case SYSTEM_USED_SPACE_PERCENT_E:
  case SYSTEM_USED_SPACE_PERCENT_F:
  case SYSTEM_USED_SPACE_PERCENT_G:
  case SYSTEM_USED_SPACE_X:
  case SYSTEM_FREE_SPACE_X:
  case SYSTEM_TOTAL_SPACE_X:
  case SYSTEM_USED_SPACE_Y:
  case SYSTEM_FREE_SPACE_Y:
  case SYSTEM_TOTAL_SPACE_Y:
  case SYSTEM_USED_SPACE_Z:
  case SYSTEM_FREE_SPACE_Z:
  case SYSTEM_TOTAL_SPACE_Z:
    return g_sysinfo.GetHddSpaceInfo(info);
  break;

  case LCD_FREE_SPACE_C:
  case LCD_FREE_SPACE_E:
  case LCD_FREE_SPACE_F:
  case LCD_FREE_SPACE_G:
    return g_sysinfo.GetHddSpaceInfo(info, true);
    break;

#ifdef HAS_XBOX_HARDWARE
  case SYSTEM_DVD_TRAY_STATE:
    return g_sysinfo.GetTrayState();
    break;
#endif

  case SYSTEM_CPU_TEMPERATURE:
  case SYSTEM_GPU_TEMPERATURE:
  case SYSTEM_FAN_SPEED:
  case LCD_CPU_TEMPERATURE:
  case LCD_GPU_TEMPERATURE:
  case LCD_FAN_SPEED:
  case SYSTEM_CPU_USAGE:
    return GetSystemHeatInfo(info);
    break;

#ifdef HAS_XBOX_HARDWARE
  case LCD_HDD_TEMPERATURE:
  case SYSTEM_HDD_MODEL:
  case SYSTEM_HDD_SERIAL:
  case SYSTEM_HDD_FIRMWARE:
  case SYSTEM_HDD_PASSWORD:
  case SYSTEM_HDD_LOCKSTATE:
  case SYSTEM_DVD_MODEL:
  case SYSTEM_DVD_FIRMWARE:
  case SYSTEM_HDD_TEMPERATURE:
  case SYSTEM_XBOX_MODCHIP:
  case SYSTEM_CPUFREQUENCY:
  case SYSTEM_XBOX_VERSION:
  case SYSTEM_AV_PACK_INFO:
  case SYSTEM_VIDEO_ENCODER_INFO:
  case NETWORK_MAC_ADDRESS:
  case SYSTEM_XBOX_SERIAL:
  case SYSTEM_XBE_REGION:
  case SYSTEM_DVD_ZONE:
  case SYSTEM_XBOX_PRODUCE_INFO:
  case SYSTEM_XBOX_BIOS:
  case SYSTEM_HDD_LOCKKEY:
  case SYSTEM_HDD_CYCLECOUNT:
  case SYSTEM_HDD_BOOTDATE:  
  case SYSTEM_MPLAYER_VERSION:
  case SYSTEM_KERNEL_VERSION:
#endif
  case SYSTEM_INTERNET_STATE:
  case SYSTEM_UPTIME:
  case SYSTEM_TOTALUPTIME:
    return g_sysinfo.GetInfo(info);
    break;

  case SYSTEM_SCREEN_RESOLUTION:
    strLabel.Format("%ix%i %s %02.2f fps.",
      CDisplaySettings::Get().GetCurrentResolutionInfo().iWidth,
      CDisplaySettings::Get().GetCurrentResolutionInfo().iHeight,
      CDisplaySettings::Get().GetCurrentResolutionInfo().strMode.c_str(),GetFPS());
    return strLabel;
    break;
#ifdef HAS_XBOX_HARDWARE
  case SYSTEM_CONTROLLER_PORT_1:
    return g_sysinfo.GetUnits(1);
    break;
  case SYSTEM_CONTROLLER_PORT_2:
    return g_sysinfo.GetUnits(2);
    break;
  case SYSTEM_CONTROLLER_PORT_3:
    return g_sysinfo.GetUnits(3);
    break;
  case SYSTEM_CONTROLLER_PORT_4:
    return g_sysinfo.GetUnits(4);
    break;
#endif  
  case CONTAINER_FOLDERPATH:
  case CONTAINER_FOLDERNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        if (info==CONTAINER_FOLDERNAME)
          strLabel = ((CGUIMediaWindow*)window)->CurrentDirectory().GetLabel();
        else
          strLabel = CURL(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath()).GetWithoutUserDetails();
      }
      break;
    }
  case CONTAINER_PLUGINNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        CURL url(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath());
        if (url.GetProtocol().Equals("plugin"))
        {
          strLabel = url.GetFileName();
          URIUtils::RemoveSlashAtEnd(strLabel);
        }
      }
      break;
    }
  case CONTAINER_VIEWCOUNT:
  case CONTAINER_VIEWMODE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIControl *control = window->GetControl(window->GetViewContainerID());
        if (control && control->IsContainer())
        {
          if (info == CONTAINER_VIEWMODE)
            strLabel = ((IGUIContainer *)control)->GetLabel();
          else if (info == CONTAINER_VIEWCOUNT)
            strLabel.Format("%i", window->GetViewCount());
        }
      }
      break;
    }
  case CONTAINER_SORT_METHOD:
  case CONTAINER_SORT_ORDER:
  {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
        if (viewState)
          if (info == CONTAINER_SORT_METHOD)
            strLabel = g_localizeStrings.Get(viewState->GetSortMethodLabel());
          else if (info == CONTAINER_SORT_ORDER)
            strLabel = g_localizeStrings.Get(viewState->GetSortOrderLabel());
      }
    }
    break;
  case CONTAINER_NUM_PAGES:
  case CONTAINER_NUM_ITEMS:
  case CONTAINER_CURRENT_ITEM:
  case CONTAINER_CURRENT_PAGE:
    return GetMultiInfoLabel(GUIInfo(info), contextWindow);
    break;
  case CONTAINER_SHOWPLOT:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("showplot").asString();
    }
    break;
  case CONTAINER_TOTALTIME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CFileItemList& items=((CGUIMediaWindow *)window)->CurrentDirectory();
        int duration=0;
        for (int i=0;i<items.Size();++i)
        {
          CFileItemPtr item=items.Get(i);
          if (item->HasMusicInfoTag())
            duration += item->GetMusicInfoTag()->GetDuration();
          else if (item->HasVideoInfoTag())
            duration += item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration();
        }
        if (duration > 0)
          return StringUtils::SecondsToTimeString(duration);
      }
    }
    break;
  case SYSTEM_BUILD_VERSION:
    strLabel = GetVersion();
    break;
  case SYSTEM_BUILD_DATE:
    strLabel = GetBuild();
    break;
  case SYSTEM_FREE_MEMORY:
  case SYSTEM_FREE_MEMORY_PERCENT:
  case SYSTEM_USED_MEMORY:
  case SYSTEM_USED_MEMORY_PERCENT:
  case SYSTEM_TOTAL_MEMORY:
    {
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      int iMemPercentFree = 100 - ((int)( 100.0f* (stat.dwTotalPhys - stat.dwAvailPhys)/stat.dwTotalPhys + 0.5f ));
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info == SYSTEM_FREE_MEMORY)
        strLabel.Format("%iMB", stat.dwAvailPhys /MB);      
      else if (info == SYSTEM_FREE_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentFree);
      else if (info == SYSTEM_USED_MEMORY)
        strLabel.Format("%iMB", (stat.dwTotalPhys - stat.dwAvailPhys)/MB);
      else if (info == SYSTEM_USED_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentUsed);
      else if (info == SYSTEM_TOTAL_MEMORY)
        strLabel.Format("%iMB", stat.dwTotalPhys/MB);
    }
    break;
  case SYSTEM_SCREEN_MODE:
    strLabel = CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution()).strMode;
    break;
  case SYSTEM_SCREEN_WIDTH:
    strLabel.Format("%i", CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution()).iWidth);
    break;
  case SYSTEM_SCREEN_HEIGHT:
    strLabel.Format("%i", CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution()).iHeight);
    break;
  case SYSTEM_CURRENT_WINDOW:
    return g_localizeStrings.Get(g_windowManager.GetFocusedWindow());
    break;
  case SYSTEM_STARTUP_WINDOW:
    strLabel.Format("%i", CSettings::Get().GetInt("lookandfeel.startupwindow"));
    break;
  case SYSTEM_CURRENT_CONTROL:
    {
      CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
          strLabel = control->GetDescription();
      }
    }
    break;
  case SYSTEM_XBOX_NICKNAME:
    {
      if (!CUtil::GetXBOXNickName(strLabel))
        strLabel=g_localizeStrings.Get(416); // 
      break;
    }
  case SYSTEM_DVD_LABEL:
    strLabel = CDetectDVDMedia::GetDVDLabel();
    break;
  case SYSTEM_ALARM_POS:
    if (g_alarmClock.GetRemaining("shutdowntimer") == 0.f)
      strLabel = "";
    else
    {
      double fTime = g_alarmClock.GetRemaining("shutdowntimer");
      if (fTime > 60.f)
        strLabel.Format("%2.0fm",g_alarmClock.GetRemaining("shutdowntimer")/60.f);
      else
        strLabel.Format("%2.0fs",g_alarmClock.GetRemaining("shutdowntimer"));
    }
    break;
  case SYSTEM_PROFILENAME:
    strLabel = CProfilesManager::Get().GetCurrentProfile().getName();
    break;
  case SYSTEM_LANGUAGE:
    strLabel = CSettings::Get().GetString("locale.language");
    break;
  case SYSTEM_TEMPERATURE_UNITS:
    strLabel = g_langInfo.GetTempUnitString();
    break;
  case SYSTEM_PROGRESS_BAR:
    {
      int percent;
      if (GetInt(percent, SYSTEM_PROGRESS_BAR) && percent > 0)
        strLabel.Format("%i", percent);
    }
    break;
  case SYSTEM_FRIENDLY_NAME:
    {
      CStdString friendlyName = CSettings::Get().GetString("services.devicename");
      if (friendlyName.Equals("XBMC"))
        strLabel.Format("%s (%s)", friendlyName.c_str(), "XBOX");
      else
        strLabel = friendlyName;
    }
    break;
  case LCD_PLAY_ICON:
    {
      int iPlaySpeed = g_application.GetPlaySpeed();
      if (g_application.IsPaused())
        strLabel.Format("\7");
      else if (iPlaySpeed < 1)
        strLabel.Format("\3:%ix", iPlaySpeed);
      else if (iPlaySpeed > 1)
        strLabel.Format("\4:%ix", iPlaySpeed);
      else
        strLabel.Format("\5");
    }
    break;
    
  case LCD_TIME_21:
  case LCD_TIME_22:
  case LCD_TIME_W21:
  case LCD_TIME_W22:
  case LCD_TIME_41:
  case LCD_TIME_42:
  case LCD_TIME_43:
  case LCD_TIME_44:
    //alternatively, set strLabel
    return GetLcdTime( info );
    break;

  case SKIN_THEME:
    strLabel = CSettings::Get().GetString("lookandfeel.skintheme");
    break;
  case SKIN_COLOUR_THEME:
    strLabel = CSettings::Get().GetString("lookandfeel.skincolors");
    break;
  case SKIN_ASPECT_RATIO:
    if (g_SkinInfo)
      strLabel = g_SkinInfo->GetCurrentAspect();
    break;
#ifdef HAS_LCD
  case LCD_PROGRESS_BAR:
    if (g_lcd) strLabel = g_lcd->GetProgressBar(g_application.GetTime(), g_application.GetTotalTime());
    break;
#endif
  case NETWORK_IP_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.ip;
    }
    break;
  case NETWORK_SUBNET_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.subnet;
    }
    break;
  case NETWORK_GATEWAY_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.gateway;
    }
    break;
  case NETWORK_DNS1_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.DNS1;
    }
    break;
  case NETWORK_DNS2_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.DNS2;
    }
    break;
  case NETWORK_DHCP_ADDRESS:
    {
      return g_application.getNetwork().m_networkinfo.dhcpserver;
    }
    break;
  case NETWORK_IS_DHCP:
    {
      if(g_application.getNetwork().m_networkinfo.DHCP)
        return g_localizeStrings.Get(148); // is dhcp ip
      else
        return g_localizeStrings.Get(147); // is fixed ip
    }
    break;
#ifdef HAS_XBOX_HARDWARE
  case NETWORK_LINK_STATE:
    {
      DWORD dwnetstatus = XNetGetEthernetLinkStatus();
      CStdString linkStatus;
      if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
      {
        if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
          linkStatus += "100mbps ";
        if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
          linkStatus += "10mbps ";
        if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
          linkStatus += g_localizeStrings.Get(153);
        if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
          linkStatus += g_localizeStrings.Get(152);
      }
      else
        linkStatus += g_localizeStrings.Get(159);
      return linkStatus;
    }
    break;
#endif
  case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_windowManager.SendMessage(msg);
      if (msg.GetPointer())
      {
        CVisualisation* viz = NULL;
        viz = (CVisualisation*)msg.GetPointer();
        if (viz)
        {
          strLabel = viz->GetPresetName();
          URIUtils::RemoveExtension(strLabel);
        }
      }
    }
    break;
  case VISUALISATION_NAME:
    strLabel = CSettings::Get().GetString("musicplayer.visualisation");
    break;
  case FANART_COLOR1:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color1").asString();
    }
    break;
  case FANART_COLOR2:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color2").asString();
    }
    break;
  case FANART_COLOR3:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color3").asString();
    }
    break;
  case FANART_IMAGE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt("fanart");
    }
    break;
  }

  return strLabel;
}

// tries to get a integer value for use in progressbars/sliders and such
bool CGUIInfoManager::GetInt(int &value, int info, int contextWindow, const CGUIListItem *item /* = NULL */) const
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoInt(value, m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= LISTITEM_START && info <= LISTITEM_END)
    return GetItemInt(value, item, info);

  switch( info )
  {
    case PLAYER_VOLUME:
      value = g_application.GetVolume();
      return true;
    case PLAYER_SUBTITLE_DELAY:
      value = g_application.GetSubtitleDelay();
      return true;
    case PLAYER_AUDIO_DELAY:
      value = g_application.GetAudioDelay();
      return true;
    case PLAYER_PROGRESS:
    case PLAYER_PROGRESS_CACHE:
    case PLAYER_SEEKBAR:
    case PLAYER_CACHELEVEL:
    case PLAYER_CHAPTER:
    case PLAYER_CHAPTERCOUNT:
      {
        if( g_application.IsPlaying() && g_application.m_pPlayer)
        {
          switch( info )
          {
          case PLAYER_PROGRESS:
            value = (int)(g_application.GetPercentage());
            return true;
          case PLAYER_PROGRESS_CACHE:
            value = (int)(g_application.GetCachePercentage());
            return true;
          case PLAYER_SEEKBAR:
            {
              value = (int)g_application.GetSeekHandler()->GetPercent();
              return true;
            }
          case PLAYER_CACHELEVEL:
            value = (int)(g_application.m_pPlayer->GetCacheLevel());
            return true;
          case PLAYER_CHAPTER:
            value = g_application.m_pPlayer->GetChapter();
            return true;
          case PLAYER_CHAPTERCOUNT:
            value = g_application.m_pPlayer->GetChapterCount();
            return true;
          }
        }
      }
      break;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
      {
        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        int memPercentUsed = (int)( 100.0f* (stat.dwTotalPhys - stat.dwAvailPhys)/stat.dwTotalPhys + 0.5f );
        if (info == SYSTEM_FREE_MEMORY)
          value = 100 - memPercentUsed;
        else
          value = memPercentUsed;
        return true;
      }
    case SYSTEM_PROGRESS_BAR:
      {
        CGUIDialogProgress *bar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (bar && bar->IsDialogRunning())
        {
          value = bar->GetPercentage();
          return true;
        }
      }
#ifdef HAS_XBOX_HARDWARE
    case SYSTEM_HDD_TEMPERATURE:
      value = atoi(g_sysinfo.GetInfo(LCD_HDD_TEMPERATURE));
      return true;
    case SYSTEM_CPU_TEMPERATURE:
      value = atoi(CFanController::Instance()->GetCPUTemp().ToString());
      return true;
    case SYSTEM_GPU_TEMPERATURE:
      value = atoi(CFanController::Instance()->GetGPUTemp().ToString());
      return true;
    case SYSTEM_FAN_SPEED:
      value = CFanController::Instance()->GetFanSpeed() * 2;
      return true;
#endif
    case SYSTEM_FREE_SPACE:
    case SYSTEM_FREE_SPACE_C:
    case SYSTEM_FREE_SPACE_E:
    case SYSTEM_FREE_SPACE_F:
    case SYSTEM_FREE_SPACE_G:
    case SYSTEM_USED_SPACE:
    case SYSTEM_USED_SPACE_C:
    case SYSTEM_USED_SPACE_E:
    case SYSTEM_USED_SPACE_F:
    case SYSTEM_USED_SPACE_G:
    case SYSTEM_FREE_SPACE_X:
    case SYSTEM_USED_SPACE_X:
    case SYSTEM_FREE_SPACE_Y:
    case SYSTEM_USED_SPACE_Y:
    case SYSTEM_FREE_SPACE_Z:
    case SYSTEM_USED_SPACE_Z:
      {
        g_sysinfo.GetHddSpaceInfo(value, info, true);
        return true;
      }
    case SYSTEM_CPU_USAGE:
      value = 100 - ((int)(100.0f *g_application.m_idleThread.GetRelativeUsage()));
      return true;
  }
  return false;
}

// functor for comparison InfoPtr's
struct InfoBoolFinder
{
  InfoBoolFinder(const std::string &expression, int context) : m_bool(expression, context) {};
  bool operator() (const InfoPtr &right) const { return m_bool == *right; };
  InfoBool m_bool;
};

INFO::InfoPtr CGUIInfoManager::Register(const std::string &expression, int context)
{
  std::string condition(CGUIInfoLabel::ReplaceLocalize(expression));
  StringUtils2::Trim(condition);
  StringUtils2::ToLower(condition);

  if (condition.empty())
    return INFO::InfoPtr();

  CSingleLock lock(m_critInfo);
  // do we have the boolean expression already registered?
  vector<InfoPtr>::const_iterator i = find_if(m_bools.begin(), m_bools.end(), InfoBoolFinder(condition, context));
  if (i != m_bools.end())
    return *i;

  if (condition.find_first_of("|+[]!") != condition.npos)
#ifndef _DEBUG // Why DEBUG build is throwing error here?
    m_bools.push_back(boost::make_shared<InfoExpression>(condition, context));
  else
    m_bools.push_back(boost::make_shared<InfoSingle>(condition, context));
#else
    m_bools.push_back(boost::shared_ptr<InfoExpression>(new InfoExpression(condition, context)));
  else
    m_bools.push_back(boost::shared_ptr<InfoSingle>(new InfoSingle(condition, context)));
#endif

  return m_bools.back();
}

bool CGUIInfoManager::EvaluateBool(const CStdString &expression, int contextWindow)
{
  bool result = false;
  INFO::InfoPtr info = Register(expression, contextWindow);
  if (info)
    result = info->Get();
  return result;
}

// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;

  int condition = abs(condition1);

  if (condition >= LISTITEM_START && condition < LISTITEM_END)
  {
    if (item)
      bReturn = GetItemBool(item, condition);
    else
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
      if (window)
      {
        CFileItemPtr item = window->GetCurrentListItem();
        bReturn = GetItemBool(item.get(), condition);
      }
    }
  }
  // Ethernet Link state checking
  // Will check if the Xbox has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  else if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
#ifdef HAS_XBOX_NETWORK
    bReturn = (XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE);
#else
    bReturn = true;
#endif
  else if (condition == WINDOW_IS_MEDIA)
  { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    bReturn = (pWindow && pWindow->IsMediaWindow());
  }
  else if (condition == PLAYER_MUTED)
    bReturn = g_application.IsMuted();
  else if (condition >= LIBRARY_HAS_MUSIC && condition <= LIBRARY_HAS_MUSICVIDEOS)
    bReturn = GetLibraryBool(condition);
  else if (condition == LIBRARY_IS_SCANNING)
  {
    if (g_application.IsMusicScanning() || g_application.IsVideoScanning())
      bReturn = true;
    else
      bReturn = false;
  }
  else if (condition == LIBRARY_IS_SCANNING_VIDEO)
  {
    bReturn = g_application.IsVideoScanning();
  }
  else if (condition == LIBRARY_IS_SCANNING_MUSIC)
  {
    bReturn = g_application.IsMusicScanning();
  }
  else if (condition == SYSTEM_PLATFORM_LINUX)
#if defined(_LINUX) && !defined(__APPLE__)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_WINDOWS)
#ifdef WIN32
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_OSX)
#ifdef __APPLE__
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_XBOX)
#ifdef HAS_XBOX_HARDWARE
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_MEDIA_DVD)
  {
    // we must: 1.  Check tray state.
    //          2.  Check that we actually have a disc in the drive (detection
    //              of disk type takes a while from a separate thread).

    int iTrayState = CIoSupport::GetTrayState();
    if ( iTrayState == DRIVE_CLOSED_MEDIA_PRESENT || iTrayState == TRAY_CLOSED_MEDIA_PRESENT )
      bReturn = CDetectDVDMedia::IsDiscInDrive();
    else 
      bReturn = false;
  }
  else if (condition == SYSTEM_HAS_DRIVE_F)
    bReturn = CIoSupport::DriveExists('F');
  else if (condition == SYSTEM_HAS_DRIVE_G)
    bReturn = CIoSupport::DriveExists('G');
  else if (condition == SYSTEM_DVDREADY)
    bReturn = CDetectDVDMedia::DriveReady() != DRIVE_NOT_READY;
  else if (condition == SYSTEM_TRAYOPEN)
    bReturn = CDetectDVDMedia::DriveReady() == DRIVE_OPEN;
  else if (condition == SYSTEM_CAN_POWERDOWN)
    bReturn = true;
  else if (condition == SYSTEM_CAN_REBOOT)
    bReturn = true;
  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_SHOWCODEC)
    bReturn = m_playerShowCodec;
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    return GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
  }
  else if (condition == SYSTEM_HASLOCKS)  
    bReturn = CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
  else if (condition == SYSTEM_ISMASTER)
    bReturn = CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
  else if (condition == SYSTEM_LOGGEDON)
    bReturn = !(g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN);
  else if (condition == SYSTEM_HAS_LOGINSCREEN)
    bReturn = CProfilesManager::Get().UsingLoginScreen();
  else if (condition == WEATHER_IS_FETCHED)
    bReturn = g_weatherManager.IsFetched();
  else if (condition == SYSTEM_INTERNET_STATE)
  {
    g_sysinfo.GetInfo(condition);
    bReturn = g_sysinfo.HasInternet();
  }
  else if (condition == SKIN_HAS_VIDEO_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.IsPlayingVideo();
  }
  else if (condition == SKIN_HAS_MUSIC_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.IsPlayingAudio();
  }
  else if (condition == CONTAINER_HASFILES || condition == CONTAINER_HASFOLDERS)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      const CFileItemList& items=((CGUIMediaWindow*)pWindow)->CurrentDirectory();
      for (int i=0;i<items.Size();++i)
      {
        CFileItemPtr item=items.Get(i);
        if (!item->m_bIsFolder && condition == CONTAINER_HASFILES)
        {
          bReturn=true;
          break;
        }
        else if (item->m_bIsFolder && !item->IsParentFolder() && condition == CONTAINER_HASFOLDERS)
        {
          bReturn=true;
          break;
        }
      }
    }
  }
  else if (condition == CONTAINER_STACKED)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().GetProperty("isstacked").asBoolean();
  }
  else if (condition == CONTAINER_HAS_THUMB)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().HasArt("thumb");
  }
  else if (condition == CONTAINER_CAN_FILTER)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = !((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_CAN_FILTERADVANCED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_FILTERED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->IsFiltered();
  }
  else if (condition == VIDEOPLAYER_HAS_INFO)
    bReturn = (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->IsEmpty());
  else if (condition >= CONTAINER_SCROLL_PREVIOUS && condition <= CONTAINER_SCROLL_NEXT)
  {
    // no parameters, so we assume it's just requested for a media window.  It therefore
    // can only happen if the list has focus.
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      map<int,int>::const_iterator it = m_containerMoves.find(pWindow->GetViewContainerID());
      if (it != m_containerMoves.end())
      {
        if (condition > CONTAINER_STATIC) // moving up
          bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
        else
          bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
      }
    }
  }
  else if (g_application.IsPlaying())
  {
    switch (condition)
    {
    case PLAYER_HAS_MEDIA:
      bReturn = true;
      break;
    case PLAYER_HAS_AUDIO:
      bReturn = g_application.IsPlayingAudio();
      break;
    case PLAYER_HAS_VIDEO:
      bReturn = g_application.IsPlayingVideo();
      break;
    case PLAYER_PLAYING:
      bReturn = !g_application.IsPaused() && (g_application.GetPlaySpeed() == 1);
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.IsPaused();
      break;
    case PLAYER_REWINDING:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() < 1;
      break;
    case PLAYER_FORWARDING:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() > 1;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 32;
      break;
    case PLAYER_CAN_RECORD:
      bReturn = g_application.m_pPlayer->CanRecord();
      break;
    case PLAYER_RECORDING:
      bReturn = g_application.m_pPlayer->IsRecording();
    break;
    case PLAYER_DISPLAY_AFTER_SEEK:
      bReturn = GetDisplayAfterSeek();
    break;
    case PLAYER_CACHING:
      bReturn = g_application.m_pPlayer->IsCaching();
    break;
    case PLAYER_SEEKBAR:
      {
        CGUIDialog *seekBar = (CGUIDialog*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
        bReturn = seekBar ? seekBar->IsDialogRunning() : false;
      }
    break;
    case PLAYER_SEEKING:
      bReturn = m_playerSeeking;
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
      break;
    case MUSICPM_ENABLED:
      bReturn = g_partyModeManager.IsEnabled();
    break;
    case MUSICPLAYER_HASPREVIOUS:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() > 0); // not first song
      }
      break;
    case MUSICPLAYER_HASNEXT:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() < (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() - 1)); // not last song
      }
      break;
    case MUSICPLAYER_PLAYLISTPLAYING:
      {
        bReturn = false;
        if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = true;
      }
      break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (CSettings::Get().GetInt("videoplayer.rendermethod") == RENDER_OVERLAYS);
    break;
    case VIDEOPLAYER_ISFULLSCREEN:
      bReturn = g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO;
    break;
    case VIDEOPLAYER_HASMENU:
      bReturn = g_application.m_pPlayer->HasMenu();
    break;
    case PLAYLIST_ISRANDOM:
      bReturn = g_playlistPlayer.IsShuffled(g_playlistPlayer.GetCurrentPlaylist());
    break;
    case PLAYLIST_ISREPEAT:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ALL;
    break;
    case PLAYLIST_ISREPEATONE:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ONE;
    break;
    case PLAYER_HASDURATION:
      bReturn = g_application.GetTotalTime() > 0;
      break;
    case VIDEOPLAYER_HASSUBTITLES:
      bReturn = g_application.m_pPlayer->GetSubtitleCount() > 0;
      break;
    case VIDEOPLAYER_SUBTITLESENABLED:
      bReturn = g_application.m_pPlayer->GetSubtitleVisible();
      break;
    case VISUALISATION_LOCKED:
      {
        CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
        g_windowManager.SendMessage(msg);
        if (msg.GetPointer())
        {
          CVisualisation *pVis = (CVisualisation *)msg.GetPointer();
          bReturn = pVis->IsLocked();
        }
      }
    break;
    case VISUALISATION_ENABLED:
      bReturn = !CSettings::Get().GetString("musicplayer.visualisation").empty();
    break;
    default: // default, use integer value different from 0 as true
      {
        int val;
        bReturn = GetInt(val, condition) && val != 0;
      }
    }
  }
  if (condition1 < 0)
    bReturn = !bReturn;
  return bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    // TODO: We currently don't use the item that is passed in to here, as these
    //       conditions only come from Container(id).ListItem(offset).* at this point.
    CGUIListItemPtr item;
    CGUIWindow *window = NULL;
    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = ((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag());
    }

    if (item) // If we got a valid item, do the lookup
      bReturn = GetItemBool(item.get(), condition); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else
  {
    switch (condition)
    {
      case VALUE_IS_TRUE:
        {
          CStdString value;
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
            value = GetItemImage((const CFileItem *)item, info.GetData1());
          else
            value = GetImage(info.GetData1(), contextWindow);

          bReturn = (value.Equals("true") || value.Equals("yes") || value.Equals("on"));
        }
        break;
      case SKIN_BOOL:
        {
          bReturn = CSkinSettings::Get().GetBool(info.GetData1());
        }
        break;
      case SKIN_STRING:
        {
          if (info.GetData2())
            bReturn = StringUtils2::EqualsNoCase(CSkinSettings::Get().GetString(info.GetData1()), m_stringParameters[info.GetData2()]);
          else
            bReturn = !CSkinSettings::Get().GetString(info.GetData1()).empty();
        }
        break;
      case SKIN_HAS_THEME:
        {
          CStdString theme = CSettings::Get().GetString("lookandfeel.skintheme");
          theme.ToLower();
          URIUtils::RemoveExtension(theme);
          bReturn = theme.Equals(m_stringParameters[info.GetData1()]);
        }
        break;
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).IsEmpty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).IsEmpty();
        break;
      case STRING_COMPARE: // STRING_COMPARE is deprecated
      case STRING_IS_EQUAL:
        {
          CStdString compare;
          if (info.GetData2() < 0) // info labels are stored with negative numbers
          {
            int info2 = -info.GetData2();
            if (item && item->IsFileItem() && info2 >= LISTITEM_START && info2 < LISTITEM_END)
              compare = GetItemImage((const CFileItem *)item, info2);
            else
              compare = GetImage(info2, contextWindow);
          }
          else if (info.GetData2() < (int)m_stringParameters.size())
          { // conditional string
            compare = m_stringParameters[info.GetData2()];
          }
        if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
            bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).Equals(compare);
        else
            bReturn = GetImage(info.GetData1(), contextWindow).Equals(compare);
        }
        break;
      case INTEGER_IS_EQUAL:
      case INTEGER_GREATER_THAN:
      case INTEGER_GREATER_OR_EQUAL:
      case INTEGER_LESS_THAN:
      case INTEGER_LESS_OR_EQUAL:
        {
          int integer;
          if (!GetInt(integer, info.GetData1(), contextWindow, item))
          {
            CStdString value;
            if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
              value = GetItemImage((const CFileItem *)item, info.GetData1());
            else
              value = GetImage(info.GetData1(), contextWindow);

            // Handle the case when a value contains time separator (:). This makes IntegerGreaterThan
            // useful for Player.Time* members without adding a separate set of members returning time in seconds
            if (value.find_first_of( ':' ) != value.npos)
              integer = StringUtils::TimeStringToSeconds(value);
            else
              integer = atoi(value.c_str());
          }

          // compare
          if (condition == INTEGER_IS_EQUAL)
            bReturn = integer == info.GetData2();
          else if (condition == INTEGER_GREATER_THAN)
            bReturn = integer > info.GetData2();
          else if (condition == INTEGER_GREATER_OR_EQUAL)
            bReturn = integer >= info.GetData2();
          else if (condition == INTEGER_LESS_THAN)
            bReturn = integer < info.GetData2();
          else if (condition == INTEGER_LESS_OR_EQUAL)
            bReturn = integer <= info.GetData2();
        }
        break;
      case STRING_STR:          // STRING_STR is deprecated
      case STRING_STR_LEFT:     // STRING_STR_LEFT is deprecated
      case STRING_STR_RIGHT:    // STRING_STR_RIGHT is deprecated
      case STRING_STARTS_WITH:
      case STRING_ENDS_WITH:
      case STRING_CONTAINS:
        {
          CStdString compare = m_stringParameters[info.GetData2()];
          // our compare string is already in lowercase, so lower case our label as well
          // as CStdString::Find() is case sensitive
          CStdString label;
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
            label = GetItemImage((const CFileItem *)item, info.GetData1()).ToLower();
          else
            label = GetImage(info.GetData1(), contextWindow).ToLower();
          if (condition == STRING_STR_LEFT || condition == STRING_STARTS_WITH)
            bReturn = label.Find(compare) == 0;
          else if (condition == STRING_STR_RIGHT || condition == STRING_ENDS_WITH)
            bReturn = label.Find(compare) == (int)(label.size()-compare.size());
          else
            bReturn = label.Find(compare) > -1;
        }
        break;
      case SYSTEM_ALARM_LESS_OR_EQUAL:
        {
          int time = lrint(g_alarmClock.GetRemaining(m_stringParameters[info.GetData1()]));
          int timeCompare = atoi(m_stringParameters[info.GetData2()]);
          if (time > 0)
            bReturn = timeCompare >= time;
          else
            bReturn = false;
        }
        break;
      case SYSTEM_HAS_ADDON:
        {
          AddonPtr addon;
          bReturn = CAddonMgr::Get().GetAddon(m_stringParameters[info.GetData1()],addon) && addon;
          break;
        }
      case SYSTEM_IDLE_TIME:
        bReturn = g_application.GlobalIdleTime() >= (int)info.GetData1();
        break;
      case CONTROL_GROUP_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = window->ControlGroupHasFocus(info.GetData1(), info.GetData2());
        }
        break;
      case CONTROL_IS_VISIBLE:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = control->IsVisible();
          }
        }
        break;
      case CONTROL_IS_ENABLED:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = !control->IsDisabled();
          }
        }
        break;
      case CONTROL_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = (window->GetFocusedControlID() == (int)info.GetData1());
        }
        break;
      case WINDOW_NEXT:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_nextWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_nextWindowID);
          if (window && URIUtils::GetFileName(window->GetProperty("xmlfile").asString()).Equals(m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_PREVIOUS:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_prevWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_prevWindowID);
          if (window && URIUtils::GetFileName(window->GetProperty("xmlfile").asString()).Equals(m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_IS:
        if (info.GetData1())
        {
          CGUIWindow *window = g_windowManager.GetWindow(contextWindow);
          bReturn = (window && window->GetID() == static_cast<int>(info.GetData1()));
        }
        else
          CLog::Log(LOGERROR, "The window id is required.");
        break;
      case WINDOW_IS_VISIBLE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowVisible(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowVisible(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_TOPMOST:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowTopMost(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowTopMost(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_ACTIVE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowActive(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowActive(m_stringParameters[info.GetData2()]);
        break;
      case SYSTEM_HAS_ALARM:
        bReturn = g_alarmClock.HasAlarm(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_GET_BOOL:
        bReturn = CSettings::Get().GetBool(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_SETTING:
        {
          if ( m_stringParameters[info.GetData1()].Equals("hidewatched") )
          {
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              bReturn = CMediaSettings::Get().GetWatchedMode(((CGUIMediaWindow *)window)->CurrentDirectory().GetContent()) == WatchedModeUnwatched;
          }
        }
        break;
      case CONTAINER_SCROLL_PREVIOUS:
      case CONTAINER_MOVE_PREVIOUS:
      case CONTAINER_MOVE_NEXT:
      case CONTAINER_SCROLL_NEXT:
        {
          map<int,int>::const_iterator it = m_containerMoves.find(info.GetData1());
          if (it != m_containerMoves.end())
          {
            if (condition > CONTAINER_STATIC) // moving up
              bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
            else
              bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
          }
        }
        break;
      case CONTAINER_CONTENT:
        {
          CStdString content;
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
              content = ((CGUIDialogMusicInfo *)window)->CurrentDirectory().GetContent();
            else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
              content = ((CGUIDialogVideoInfo *)window)->CurrentDirectory().GetContent();
          }
          if (content.IsEmpty())
          {
            window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              content = ((CGUIMediaWindow *)window)->CurrentDirectory().GetContent();
          }
          bReturn = m_stringParameters[info.GetData2()].Equals(content);
        }
        break;
      case CONTAINER_ROW:
      case CONTAINER_COLUMN:
      case CONTAINER_POSITION:
      case CONTAINER_HAS_NEXT:
      case CONTAINER_HAS_PREVIOUS:
      case CONTAINER_SCROLLING:
      case CONTAINER_SUBITEM:
        {
          const CGUIControl *control = NULL;
          if (info.GetData1())
          { // container specified
            CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
            if (window)
              control = window->GetControl(info.GetData1());
          }
          else
          { // no container specified - assume a mediawindow
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              control = window->GetControl(window->GetViewContainerID());
          }
          if (control)
            bReturn = control->GetCondition(condition, info.GetData2());
        }
        break;
      case CONTAINER_HAS_FOCUS:
        { // grab our container
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control && control->IsContainer())
            {
              CFileItemPtr item = boost::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(0));
              if (item && item->m_iprogramCount == info.GetData2())  // programcount used to store item id
                bReturn = true;
            }
          }
          break;
        }
      case VIDEOPLAYER_CONTENT:
        {
          CStdString strContent="files";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeMovie)
            strContent = "movies";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_iSeason > -1) // episode
            strContent = "episodes";
          if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_artist.empty())
            strContent = "musicvideos";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_strStatus == "livetv")
            strContent = "livetv";
          bReturn = m_stringParameters[info.GetData1()].Equals(strContent);
        }
        break;
      case CONTAINER_SORT_METHOD:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortMethod().sortBy == info.GetData1());
        }
        break;
      }
      case CONTAINER_SORT_DIRECTION:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetDisplaySortOrder() == info.GetData1());
        }
        break;
      }
      case SYSTEM_DATE:
        {
          CDateTime date = CDateTime::GetCurrentDateTime();
          int currentDate = date.GetMonth()*100+date.GetDay();
          int startDate = info.GetData1();
          int stopDate = info.GetData2();

          if (stopDate < startDate)
            bReturn = currentDate >= startDate || currentDate < stopDate;
          else
            bReturn = currentDate >= startDate && currentDate < stopDate;
        }
        break;
      case SYSTEM_TIME:
        {
          CDateTime time=CDateTime::GetCurrentDateTime();
          int currentTime = time.GetMinuteOfDay();
          int startTime = info.GetData1();
          int stopTime = info.GetData2();

          if (stopTime < startTime)
            bReturn = currentTime >= startTime || currentTime < stopTime;
          else
            bReturn = currentTime >= startTime && currentTime < stopTime;
        }
        break;
      case MUSICPLAYER_EXISTS:
        {
          int index = info.GetData2();
          if (info.GetData1() == 1)
          { // relative index
            if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
              return false;
            index += g_playlistPlayer.GetCurrentSong();
          }
          if (index >= 0 && index < g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size())
            return true;
          return false;
        }
        break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoInt(int &value, const GUIInfo &info, int contextWindow) const
{
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = boost::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemInt(value, item.get(), info.m_info);
  }

  return 0;
}

/// \brief Examines the multi information sent and returns the string as appropriate
CStdString CGUIInfoManager::GetMultiInfoLabel(const GUIInfo &info, int contextWindow, CStdString *fallback)
{
  if (info.m_info == SKIN_STRING)
  {
    return CSkinSettings::Get().GetString(info.GetData1());
  }
  else if (info.m_info == SKIN_BOOL)
  {
    bool bInfo = CSkinSettings::Get().GetBool(info.GetData1());
    if (bInfo)
      return g_localizeStrings.Get(20122);
  }
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = boost::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemImage(item.get(), info.m_info, fallback); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else if (info.m_info == PLAYER_TIME)
  {
    return GetCurrentPlayTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_REMAINING)
  {
    return GetCurrentPlayTimeRemaining((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_FINISH_TIME)
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    time += CDateTimeSpan(0, 0, 0, GetPlayTimeRemaining());
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_SPEED)
  {
    CStdString strTime;
    if (g_application.GetPlaySpeed() != 1)
      strTime.Format("%s (%ix)", GetCurrentPlayTime((TIME_FORMAT)info.GetData1()).c_str(), g_application.GetPlaySpeed());
    else
      strTime = GetCurrentPlayTime();
    return strTime;
  }
  else if (info.m_info == PLAYER_DURATION)
  {
    return GetDuration((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKTIME)
  {
    return GetCurrentSeekTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKOFFSET)
  {
    CStdString seekOffset = StringUtils::SecondsToTimeString(abs(m_seekOffset), (TIME_FORMAT)info.GetData1());
    if (m_seekOffset < 0)
      return "-" + seekOffset;
    if (m_seekOffset > 0)
      return "+" + seekOffset;
  }
  else if (info.m_info == PLAYER_ITEM_ART)
  {
    return m_currentFile->GetArt(m_stringParameters[info.GetData1()]);
  }
  else if (info.m_info == SYSTEM_TIME)
  {
    return GetTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == CONTAINER_NUM_PAGES || info.m_info == CONTAINER_CURRENT_PAGE ||
           info.m_info == CONTAINER_NUM_ITEMS || info.m_info == CONTAINER_POSITION ||
           info.m_info == CONTAINER_CURRENT_ITEM)
  {
    const CGUIControl *control = NULL;
    if (info.GetData1())
    { // container specified
      CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
      if (window)
        control = window->GetControl(info.GetData1());
    }
    else
    { // no container specified - assume a mediawindow
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        control = window->GetControl(window->GetViewContainerID());
    }
    if (control)
    {
      if (control->IsContainer())
        return ((IGUIContainer *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
        return ((CGUIControlGroupList *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
        return ((CGUITextBox *)control)->GetLabel(info.m_info);
    }
  }
  else if (info.m_info >= MUSICPLAYER_TITLE && info.m_info <= MUSICPLAYER_ALBUM_ARTIST)
    return GetMusicPlaylistInfo(info);
  else if (info.m_info == CONTAINER_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // container specified
      window = GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no container specified - assume a mediawindow
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    }
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == CONTAINER_ART)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt(m_stringParameters[info.GetData2()]);
  }
  else if (info.m_info == CONTROL_GET_LABEL)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
    if (window)
    {
      const CGUIControl *control = window->GetControl(info.GetData1());
      if (control)
        return control->GetDescription();
    }
  }
  else if (info.m_info == WINDOW_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // window specified
      window = g_windowManager.GetWindow(info.GetData1());//GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no window specified - assume active
      window = GetWindowWithCondition(contextWindow, 0);
    }

    if (window)
      return window->GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON ||
           info.m_info == SYSTEM_ADDON_VERSION)
  {
    // This logic does not check/care whether an addon has been disabled/marked as broken,
    // it simply retrieves it's name or icon that means if an addon is placed on the home screen it
    // will stay there even if it's disabled/marked as broken. This might need to be changed/fixed
    // in the future.
    AddonPtr addon;
    if (info.GetData2() == 0)
      CAddonMgr::Get().GetAddon(const_cast<CGUIInfoManager*>(this)->GetLabel(info.GetData1(), contextWindow),addon,ADDON_UNKNOWN,false);
    else
      CAddonMgr::Get().GetAddon(m_stringParameters[info.GetData1()],addon,ADDON_UNKNOWN,false);
    if (addon && info.m_info == SYSTEM_ADDON_TITLE)
      return addon->Name();
    if (addon && info.m_info == SYSTEM_ADDON_ICON)
      return addon->Icon();
    if (addon && info.m_info == SYSTEM_ADDON_VERSION)
      return addon->Version().asString();
  }

  return StringUtils::EmptyString;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(int info, int contextWindow, CStdString *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true);

  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow, fallback);
  }
  else if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == SYSTEM_PROFILETHUMB)
  {
    CStdString thumb = CProfilesManager::Get().GetCurrentProfile().getThumb();
    if (thumb.IsEmpty())
      thumb = "unknown-user.png";
    return thumb;
  }
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.IsPlayingAudio()) return "";
    if (fallback)
      *fallback = "DefaultAlbumCover.png";
    return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultAlbumCover.png";
  }
  else if (info == MUSICPLAYER_RATING)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return GetItemImage(m_currentFile, LISTITEM_RATING);
  }
  else if (info == PLAYER_STAR_RATING)
  {
    if (!g_application.IsPlaying()) return "";
    return GetItemImage(m_currentFile, LISTITEM_STAR_RATING);
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.IsPlayingVideo()) return "";
    if (fallback)
      *fallback = "DefaultVideoCover.png";
    if(m_currentMovieThumb.IsEmpty())
      return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultVideoCover.png";
    else return m_currentMovieThumb;
  }
  else if (info == LISTITEM_THUMB || info == LISTITEM_ICON || info == LISTITEM_ACTUAL_ICON ||
          info == LISTITEM_OVERLAY || info == LISTITEM_RATING || info == LISTITEM_STAR_RATING)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      if (item)
        return GetItemImage(item.get(), info, fallback);
    }
  }
  return GetLabel(info, contextWindow, fallback);
}

CStdString CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return time.GetAsLocalizedDate(!bNumbersOnly);
}

CStdString CGUIInfoManager::GetTime(TIME_FORMAT format) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return LocalizeTime(time, format);
}

CStdString CGUIInfoManager::GetLcdTime( int _eInfo ) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  CStdString strLcdTime;

#ifdef HAS_LCD

  UINT       nCharset;
  UINT       nLine;
  CStdString strTimeMarker;

  switch ( _eInfo )
  {
    case LCD_TIME_21:
      nCharset = 1; // CUSTOM_CHARSET_SMALLCHAR;
      nLine = 0;
      strTimeMarker = ".";
    break;
    case LCD_TIME_22:
      nCharset = 1; // CUSTOM_CHARSET_SMALLCHAR;
      nLine = 1;
      strTimeMarker = ".";
    break;

    case LCD_TIME_W21:
      nCharset = 2; // CUSTOM_CHARSET_MEDIUMCHAR;
      nLine = 0;
      strTimeMarker = ".";
    break;
    case LCD_TIME_W22:
      nCharset = 2; // CUSTOM_CHARSET_MEDIUMCHAR;
      nLine = 1;
      strTimeMarker = ".";
    break;

    case LCD_TIME_41:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 0;
      strTimeMarker = " ";
    break;
    case LCD_TIME_42:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 1;
      strTimeMarker = "o";
    break;
    case LCD_TIME_43:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 2;
      strTimeMarker = "o";
    break;
    case LCD_TIME_44:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 3;
      strTimeMarker = " ";
    break;
  }

  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetHour()  , nLine, 2, 2, true );
  strLcdTime += strTimeMarker;
  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetMinute(), nLine, 2, 2, false );
  strLcdTime += strTimeMarker;
  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetSecond(), nLine, 2, 2, false );

#endif
  
  return strLcdTime;
}

CStdString CGUIInfoManager::LocalizeTime(const CDateTime &time, TIME_FORMAT format) const
{
  const CStdString timeFormat = g_langInfo.GetTimeFormat();
  bool use12hourclock = timeFormat.Find('h') != -1;
  switch (format)
  {
  case TIME_FORMAT_GUESS:
    return time.GetAsLocalizedTime("", false);
  case TIME_FORMAT_SS:
    return time.GetAsLocalizedTime("ss", true);
  case TIME_FORMAT_MM:
    return time.GetAsLocalizedTime("mm", true);
  case TIME_FORMAT_MM_SS:
    return time.GetAsLocalizedTime("mm:ss", true);
  case TIME_FORMAT_HH:  // this forces it to a 12 hour clock
    return time.GetAsLocalizedTime(use12hourclock ? "h" : "HH", false);
  case TIME_FORMAT_HH_MM:
    return time.GetAsLocalizedTime(use12hourclock ? "h:mm" : "HH:mm", false);
  case TIME_FORMAT_HH_MM_SS:
    return time.GetAsLocalizedTime("", true);
  case TIME_FORMAT_H:
    return time.GetAsLocalizedTime("h", false);
  case TIME_FORMAT_H_MM_SS:
    return time.GetAsLocalizedTime("h:mm:ss", true);
  case TIME_FORMAT_XX:
    return use12hourclock ? time.GetAsLocalizedTime("xx", false) : "";
  default:
    break;
  }
  return time.GetAsLocalizedTime("", false);
}

CStdString CGUIInfoManager::GetDuration(TIME_FORMAT format) const
{
  if (g_application.IsPlayingAudio() && m_currentFile->HasMusicInfoTag())
  {
    const CMusicInfoTag& tag = *m_currentFile->GetMusicInfoTag();
    if (tag.GetDuration() > 0)
      return StringUtils::SecondsToTimeString(tag.GetDuration(), format);
  }
  if (g_application.IsPlayingVideo() && !m_currentMovieDuration.IsEmpty())
    return m_currentMovieDuration;  // for tuxbox
  unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
  if (iTotal > 0)
    return StringUtils::SecondsToTimeString(iTotal, format);
  return "";
}

CStdString CGUIInfoManager::GetMusicPartyModeLabel(int item)
{
  // get song counts
  if (item >= MUSICPM_SONGSPLAYED && item <= MUSICPM_RANDOMSONGSPICKED)
  {
    int iSongs = -1;
    switch (item)
    {
    case MUSICPM_SONGSPLAYED:
      {
        iSongs = g_partyModeManager.GetSongsPlayed();
        break;
      }
    case MUSICPM_MATCHINGSONGS:
      {
        iSongs = g_partyModeManager.GetMatchingSongs();
        break;
      }
    case MUSICPM_MATCHINGSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetMatchingSongsPicked();
        break;
      }
    case MUSICPM_MATCHINGSONGSLEFT:
      {
        iSongs = g_partyModeManager.GetMatchingSongsLeft();
        break;
      }
    case MUSICPM_RELAXEDSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRelaxedSongs();
        break;
      }
    case MUSICPM_RANDOMSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRandomSongs();
        break;
      }
    }
    if (iSongs < 0)
      return "";
    CStdString strLabel;
    strLabel.Format("%i", iSongs);
    return strLabel;
  }
  return "";
}

const CStdString CGUIInfoManager::GetMusicPlaylistInfo(const GUIInfo& info)
{
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.size() < 1)
    return "";
  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is PLAYLIST_MUSIC)
    if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
      return "";
    index = g_playlistPlayer.GetNextSong(index);
  }
  if (index < 0 || index >= playlist.size())
    return "";
  CFileItemPtr playlistItem = playlist[index];
  if (!playlistItem->GetMusicInfoTag()->Loaded())
  {
    playlistItem->LoadMusicTag();
    playlistItem->GetMusicInfoTag()->SetLoaded();
  }
  // try to set a thumbnail
  if (!playlistItem->HasArt("thumb"))
  {
    CMusicThumbLoader loader;
    loader.LoadItem(playlistItem.get());
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasArt("thumb"))
      playlistItem->SetArt("thumb", "DefaultAlbumCover.png");
  }
  if (info.m_info == MUSICPLAYER_PLAYLISTPOS)
  {
    CStdString strPosition = "";
    strPosition.Format("%i", index + 1);
    return strPosition;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
    return playlistItem->GetArt("thumb");
  return GetMusicTagLabel(info.m_info, playlistItem.get());
}

CStdString CGUIInfoManager::GetPlaylistLabel(int item) const
{
  if (!g_application.IsPlaying()) return "";
  int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
  switch (item)
  {
  case PLAYLIST_LENGTH:
    {
      CStdString strLength = "";
      strLength.Format("%i", g_playlistPlayer.GetPlaylist(iPlaylist).size());
      return strLength;
    }
  case PLAYLIST_POSITION:
    {
      CStdString strPosition = "";
      strPosition.Format("%i", g_playlistPlayer.GetCurrentSong() + 1);
      return strPosition;
    }
  case PLAYLIST_RANDOM:
    {
      if (g_playlistPlayer.IsShuffled(iPlaylist))
        return g_localizeStrings.Get(590); // 590: Random
      else
        return g_localizeStrings.Get(591); // 591: Off
    }
  case PLAYLIST_REPEAT:
    {
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);
      if (state == PLAYLIST::REPEAT_ONE)
        return g_localizeStrings.Get(592); // 592: One
      else if (state == PLAYLIST::REPEAT_ALL)
        return g_localizeStrings.Get(593); // 593: All
      else
        return g_localizeStrings.Get(594); // 594: Off
    }
  }
  return "";
}

CStdString CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.IsPlayingAudio() || !m_currentFile->HasMusicInfoTag()) return "";
  switch (item)
  {
  case MUSICPLAYER_PLAYLISTLEN:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_LENGTH);
    }
    break;
  case MUSICPLAYER_PLAYLISTPOS:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_POSITION);
    }
    break;
  case MUSICPLAYER_BITRATE:
    {
      float fTimeSpan = (float)(CTimeUtils::GetFrameTime() - m_lastMusicBitrateTime);
      if (fTimeSpan >= 500.0f)
      {
        m_MusicBitrate = g_application.m_pPlayer->GetAudioBitrate();
        m_lastMusicBitrateTime = CTimeUtils::GetFrameTime();
      }
      CStdString strBitrate = "";
      if (m_MusicBitrate > 0)
        strBitrate.Format("%i", MathUtils::round_int((double)m_MusicBitrate / 1000.0));
      return strBitrate;
    }
    break;
  case MUSICPLAYER_CHANNELS:
    {
      CStdString strChannels = "";
      if (g_application.m_pPlayer->GetChannels() > 0)
      {
        strChannels.Format("%i", g_application.m_pPlayer->GetChannels());
      }
      return strChannels;
    }
    break;
  case MUSICPLAYER_BITSPERSAMPLE:
    {
      CStdString strBitsPerSample = "";
      if (g_application.m_pPlayer->GetBitsPerSample() > 0)
      {
        strBitsPerSample.Format("%i", g_application.m_pPlayer->GetBitsPerSample());
      }
      return strBitsPerSample;
    }
    break;
  case MUSICPLAYER_SAMPLERATE:
    {
      CStdString strSampleRate = "";
      if (g_application.m_pPlayer->GetSampleRate() > 0)
      {
        strSampleRate.Format("%i",g_application.m_pPlayer->GetSampleRate());
      }
      return strSampleRate;
    }
    break;
  case MUSICPLAYER_CODEC:
    {
      CStdString strCodec;
      strCodec.Format("%s", g_application.m_pPlayer->GetAudioCodecName().c_str());
      return strCodec;
    }
    break;
  case MUSICPLAYER_LYRICS:
    return GetItemLabel(m_currentFile, AddListItemProp("lyrics"));
  }
  return GetMusicTagLabel(item, m_currentFile);
}

CStdString CGUIInfoManager::GetMusicTagLabel(int info, const CFileItem *item)
{
  if (!item->HasMusicInfoTag()) return "";
  const CMusicInfoTag &tag = *item->GetMusicInfoTag();
  switch (info)
  {
  case MUSICPLAYER_TITLE:
    if (tag.GetTitle().size()) { return tag.GetTitle(); }
    break;
  case MUSICPLAYER_LYRICS:
    if (tag.GetLyrics().size()) { return tag.GetLyrics(); }
    break;
  case MUSICPLAYER_ALBUM:
    if (tag.GetAlbum().size()) { return tag.GetAlbum(); }
    break;
  case MUSICPLAYER_ARTIST:
    if (tag.GetArtist().size()) { return StringUtils::Join(tag.GetArtist(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_ALBUM_ARTIST:
    if (tag.GetAlbumArtist().size()) { return StringUtils::Join(tag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_YEAR:
    if (tag.GetYear()) { return tag.GetYearString(); }
    break;
  case MUSICPLAYER_GENRE:
    if (tag.GetGenre().size()) { return StringUtils::Join(tag.GetGenre(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      CStdString strTrack;
      if (tag.Loaded() && tag.GetTrackNumber() > 0)
      {
        strTrack.Format("%02i", tag.GetTrackNumber());
        return strTrack;
      }
    }
    break;
  case MUSICPLAYER_DISC_NUMBER:
    return GetItemLabel(item, LISTITEM_DISC_NUMBER);
  case MUSICPLAYER_RATING:
    return GetItemLabel(item, LISTITEM_RATING);
  case MUSICPLAYER_COMMENT:
    return GetItemLabel(item, LISTITEM_COMMENT);
  case MUSICPLAYER_DURATION:
    return GetItemLabel(item, LISTITEM_DURATION);
  case MUSICPLAYER_PLAYCOUNT:
    return GetItemLabel(item, LISTITEM_PLAYCOUNT);
  case MUSICPLAYER_LASTPLAYED:
    return GetItemLabel(item, LISTITEM_LASTPLAYED);
  }
  return "";
}

CStdString CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.IsPlayingVideo()) 
    return "";
  
  if (item == VIDEOPLAYER_TITLE)
  {
    if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_strTitle.IsEmpty())
      return m_currentFile->GetVideoInfoTag()->m_strTitle;
    // don't have the title, so use label, or drop down to title from path
    if (!m_currentFile->GetLabel().IsEmpty())
      return m_currentFile->GetLabel();
    return CUtil::GetTitleFromPath(m_currentFile->GetPath());
  }
  else if (item == VIDEOPLAYER_PLAYLISTLEN)
  {
    if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      return GetPlaylistLabel(PLAYLIST_LENGTH);
  }
  else if (item == VIDEOPLAYER_PLAYLISTPOS)
  {
    if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      return GetPlaylistLabel(PLAYLIST_POSITION);
  }
  else if (m_currentFile->HasVideoInfoTag())
  {
    switch (item)
    {
    case VIDEOPLAYER_ORIGINALTITLE:
      return m_currentFile->GetVideoInfoTag()->m_strOriginalTitle;
      break;
    case VIDEOPLAYER_GENRE:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_DIRECTOR:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_RATING:
      {
        CStdString strRating;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
          strRating.Format("%.1f", m_currentFile->GetVideoInfoTag()->m_fRating);
        return strRating;
      }
      break;
    case VIDEOPLAYER_RATING_AND_VOTES:
      {
        CStdString strRatingAndVotes;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
        {
          if (m_currentFile->GetVideoInfoTag()->m_strVotes.IsEmpty())
            strRatingAndVotes.Format("%.1f", m_currentFile->GetVideoInfoTag()->m_fRating);
          else
            strRatingAndVotes.Format("%.1f (%s %s)", m_currentFile->GetVideoInfoTag()->m_fRating, m_currentFile->GetVideoInfoTag()->m_strVotes, g_localizeStrings.Get(20350));
        }
        return strRatingAndVotes;
      }
      break;
    case VIDEOPLAYER_YEAR:
      {
        CStdString strYear;
        if (m_currentFile->GetVideoInfoTag()->m_iYear > 0)
          strYear.Format("%i", m_currentFile->GetVideoInfoTag()->m_iYear);
        return strYear;
      }
      break;
    case VIDEOPLAYER_PREMIERED:
      {
        if (m_currentFile->GetVideoInfoTag()->m_firstAired.IsValid())
          return m_currentFile->GetVideoInfoTag()->m_firstAired.GetAsLocalizedDate();
        if (m_currentFile->GetVideoInfoTag()->m_premiered.IsValid())
          return m_currentFile->GetVideoInfoTag()->m_premiered.GetAsLocalizedDate();
      }
      break;
    case VIDEOPLAYER_PLOT:
      return m_currentFile->GetVideoInfoTag()->m_strPlot;
    case VIDEOPLAYER_TRAILER:
      return m_currentFile->GetVideoInfoTag()->m_strTrailer;
    case VIDEOPLAYER_PLOT_OUTLINE:
      return m_currentFile->GetVideoInfoTag()->m_strPlotOutline;
    case VIDEOPLAYER_EPISODE:
      {
        CStdString strEpisode;
        if (m_currentFile->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
          strEpisode.Format("S%i", m_currentFile->GetVideoInfoTag()->m_iSpecialSortEpisode);
        else if(m_currentFile->GetVideoInfoTag()->m_iEpisode > 0) 
          strEpisode.Format("%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        return strEpisode;
      }
      break;
    case VIDEOPLAYER_SEASON:
      {
        CStdString strSeason;
        if (m_currentFile->GetVideoInfoTag()->m_iSpecialSortSeason > 0)
          strSeason.Format("%i", m_currentFile->GetVideoInfoTag()->m_iSpecialSortSeason);
        else if(m_currentFile->GetVideoInfoTag()->m_iSeason > 0)
          strSeason.Format("%i", m_currentFile->GetVideoInfoTag()->m_iSeason);
        return strSeason;
      }
      break;
    case VIDEOPLAYER_TVSHOW:
      return m_currentFile->GetVideoInfoTag()->m_strShowTitle;

    case VIDEOPLAYER_STUDIO:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_COUNTRY:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_MPAA:
      return m_currentFile->GetVideoInfoTag()->m_strMPAARating;
    case VIDEOPLAYER_TOP250:
      {
        CStdString strTop250;
        if (m_currentFile->GetVideoInfoTag()->m_iTop250 > 0)
          strTop250.Format("%i", m_currentFile->GetVideoInfoTag()->m_iTop250);
        return strTop250;
      }
      break;
    case VIDEOPLAYER_CAST:
      return m_currentFile->GetVideoInfoTag()->GetCast();
    case VIDEOPLAYER_CAST_AND_ROLE:
      return m_currentFile->GetVideoInfoTag()->GetCast(true);
    case VIDEOPLAYER_ARTIST:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_ALBUM:
      return m_currentFile->GetVideoInfoTag()->m_strAlbum;
    case VIDEOPLAYER_WRITER:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_TAGLINE:
      return m_currentFile->GetVideoInfoTag()->m_strTagLine;
    case VIDEOPLAYER_LASTPLAYED:
      return m_currentFile->GetVideoInfoTag()->m_lastPlayed.GetAsLocalizedDateTime();
    case VIDEOPLAYER_PLAYCOUNT:
      {
        CStdString strPlayCount;
        if (m_currentFile->GetVideoInfoTag()->m_playCount > 0)
          strPlayCount.Format("%i", m_currentFile->GetVideoInfoTag()->m_playCount);
        return strPlayCount;
      }
    }
  }
  return "";
}

__int64 CGUIInfoManager::GetPlayTime() const
{
  if (g_application.IsPlaying())
  {
    __int64 lPTS = (__int64)(g_application.GetTime() * 1000);
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  if (g_application.IsPlayingAudio() || g_application.IsPlayingVideo())
    return StringUtils::SecondsToTimeString((int)(GetPlayTime()/1000), format);
  return "";
}

CStdString CGUIInfoManager::GetCurrentSeekTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  float time = GetTotalPlayTime() * g_application.GetSeekHandler()->GetPercent() * 0.01f;
  return StringUtils::SecondsToTimeString((int)time, format);
}

int CGUIInfoManager::GetTotalPlayTime() const
{
  int iTotalTime = (int)g_application.GetTotalTime();
  return iTotalTime > 0 ? iTotalTime : 0;
}

int CGUIInfoManager::GetPlayTimeRemaining() const
{
  int iReverse = GetTotalPlayTime() - (int)g_application.GetTime();
  return iReverse > 0 ? iReverse : 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTimeRemaining(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  int timeRemaining = GetPlayTimeRemaining();
  if (timeRemaining && (g_application.IsPlayingAudio() || g_application.IsPlayingVideo()))
    return StringUtils::SecondsToTimeString(timeRemaining, format);
  return "";
}

void CGUIInfoManager::ResetCurrentItem()
{ 
  m_currentFile->Reset();
  m_currentMovieThumb = "";
  m_currentMovieDuration = "";
}

void CGUIInfoManager::SetCurrentItem(CFileItem &item)
{
  ResetCurrentItem();

  if (item.IsAudio())
    SetCurrentSong(item);
  else
    SetCurrentMovie(item);
}

void CGUIInfoManager::SetCurrentAlbumThumb(const CStdString thumbFileName)
{
  if (CFile::Exists(thumbFileName))
    m_currentFile->SetArt("thumb", thumbFileName);
  else
  {
    m_currentFile->SetArt("thumb", "");
    m_currentFile->FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.GetPath().c_str());
  *m_currentFile = item;

  m_currentFile->LoadMusicTag();
  if (m_currentFile->GetMusicInfoTag()->GetTitle().IsEmpty())
  {
    // No title in tag, show filename only
    m_currentFile->GetMusicInfoTag()->SetTitle(CUtil::GetTitleFromPath(m_currentFile->GetPath()));
  }
  m_currentFile->GetMusicInfoTag()->SetLoaded(true);

  // find a thumb for this file.
  if (m_currentFile->IsInternetStream())
  {
    if (!g_application.m_strPlayListFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem streamingItem(g_application.m_strPlayListFile,false);

      CMusicThumbLoader loader;
      loader.FillThumb(streamingItem);
      if (streamingItem.HasArt("thumb"))
        m_currentFile->SetArt("thumb", streamingItem.GetArt("thumb"));
    }
  }
  else
  {
    CMusicThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }
  m_currentFile->FillInDefaultIcon();

  CMusicInfoLoader::LoadAdditionalTagInfo(m_currentFile);
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)",item.GetPath().c_str());
  *m_currentFile = item;
  
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    dbs.LoadVideoInfo(item.GetPath(), *m_currentFile->GetVideoInfoTag());
    dbs.Close();
  }

  // Find a thumb for this file.
  if (!item.HasArt("thumb"))
  {
    CVideoThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }

  // find a thumb for this stream
  if (item.IsInternetStream())
  {
    // case where .strm is used to start an audio stream
    if (g_application.IsPlayingAudio())
    {
      SetCurrentSong(item);
      return;
    }

    // else its a video
    if (!g_application.m_strPlayListFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem thumbItem(g_application.m_strPlayListFile,false);

      CVideoThumbLoader loader;
      if (loader.FillThumb(thumbItem))
        item.SetArt("thumb", thumbItem.GetArt("thumb"));
    }
  }

  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetArt("thumb");
}

string CGUIInfoManager::GetSystemHeatInfo(int info)
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= 1000)
  { // update our variables
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
#ifdef HAS_XBOX_HARDWARE
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp = CFanController::Instance()->GetCPUTemp();
#endif
  }

  CStdString text;
  switch(info)
  {
    case LCD_CPU_TEMPERATURE:
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.ToString();
      break;
    case LCD_GPU_TEMPERATURE:
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.ToString();
      break;
    case LCD_FAN_SPEED:
    case SYSTEM_FAN_SPEED:
      text.Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
      text.Format("%2.0f%%", (1.0f - g_application.m_idleThread.GetRelativeUsage())*100);
      break;
  }
  return text;
}

CStdString CGUIInfoManager::GetVersion()
{
  CStdString tmp;
  tmp.Format("%s", VERSION_STRING);
  return tmp;
}

CStdString CGUIInfoManager::GetBuild()
{
  CStdString tmp;
  tmp.Format("%s", __DATE__);
  return tmp;
}

void CGUIInfoManager::SetDisplayAfterSeek(unsigned int timeOut, int seekOffset)
{
  g_infoManager.m_performingSeek = false;
  if (timeOut>0)
  {
    m_AfterSeekTimeout = CTimeUtils::GetFrameTime() +  timeOut;
    if (seekOffset)
      m_seekOffset = seekOffset;
  }
  else
    m_AfterSeekTimeout = 0;
}

bool CGUIInfoManager::GetDisplayAfterSeek()
{
  if (CTimeUtils::GetFrameTime() < m_AfterSeekTimeout)
    return true;
  m_seekOffset = 0;
  return false;
}

void CGUIInfoManager::Clear()
{
  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.clear();

  /*
    Erase any info bools that are unused. We do this repeatedly as each run
    will remove those bools that are no longer dependencies of other bools
    in the vector.
   */
  vector<InfoPtr>::iterator i = remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  while (i != m_bools.end())
  {
    m_bools.erase(i, m_bools.end());
    i = remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  }
  // log which ones are used - they should all be gone by now
  for (vector<InfoPtr>::const_iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    CLog::Log(LOGDEBUG, "Infobool '%s' still used by %u instances", (*i)->GetExpression().c_str(), (unsigned int) i->use_count());
}

void CGUIInfoManager::UpdateFPS()
{
  m_frameCounter++;
  unsigned int curTime = CTimeUtils::GetFrameTime();

  float fTimeSpan = (float)(curTime - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = curTime;
    m_frameCounter = 0;
  }
}

int CGUIInfoManager::AddListItemProp(const CStdString &str, int offset)
{
  for (int i=0; i < (int)m_listitemProperties.size(); i++)
    if (m_listitemProperties[i] == str)
      return (LISTITEM_PROPERTY_START+offset + i);
      
  if (m_listitemProperties.size() < LISTITEM_PROPERTY_END - LISTITEM_PROPERTY_START)
  {
    m_listitemProperties.push_back(str);
    return LISTITEM_PROPERTY_START + offset + m_listitemProperties.size() - 1;
  }

  CLog::Log(LOGERROR,"%s - not enough listitem property space!", __FUNCTION__);
  return 0;
}

int CGUIInfoManager::AddMultiInfo(const GUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); i++)
    if (m_multiInfo[i] == info)
      return (int)i + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.push_back(info);
  return (int)m_multiInfo.size() + MULTI_INFO_START - 1;
}

int CGUIInfoManager::ConditionalStringParameter(const CStdString &parameter, bool caseSensitive /*= false*/)
{
  // check to see if we have this parameter already
  for (unsigned int i = 0; i < m_stringParameters.size(); i++)
    if (parameter.Equals(m_stringParameters[i], caseSensitive))
      return (int)i;
  // return the new offset
  m_stringParameters.push_back(parameter);
  return (int)m_stringParameters.size() - 1;
}

bool CGUIInfoManager::GetItemInt(int &value, const CGUIListItem *item, int info) const
{
  if (!item) return 0;

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    CStdString property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    CStdString val = item->GetProperty(property).asString();
    value = atoi(val);
    return true;
  }

  switch (info)
  {
    case LISTITEM_PERCENT_PLAYED:
      if (item->IsFileItem() && ((const CFileItem *)item)->HasVideoInfoTag() && ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.IsPartWay())
        value = (int)(100 * ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds / ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds);
      return true;
  }

  return false;
}

CStdString CGUIInfoManager::GetItemLabel(const CFileItem *item, int info, CStdString *fallback)
{
  if (!item) return "";

  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false, item);

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET) < (int)m_listitemProperties.size())
  { // grab the art
    std::string art = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET)];
    return item->GetArt(art);
  }

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property  
    CStdString property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    return item->GetProperty(property).asString();
  }

  if (info >= LISTITEM_PICTURE_START && info <= LISTITEM_PICTURE_END && item->HasPictureInfoTag())
    return item->GetPictureInfoTag()->GetInfo(picture_slide_map[info - LISTITEM_PICTURE_START]);

  switch (info)
  {
  case LISTITEM_LABEL:
    return item->GetLabel();
  case LISTITEM_LABEL2:
    return item->GetLabel2();
  case LISTITEM_TITLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTitle;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetTitle();
    break;
  case LISTITEM_ORIGINALTITLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strOriginalTitle;
    break;
  case LISTITEM_PLAYCOUNT:
    {
      CStdString strPlayCount;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount > 0)
        strPlayCount.Format("%i", item->GetVideoInfoTag()->m_playCount);
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetPlayCount() > 0)
        strPlayCount.Format("%i", item->GetMusicInfoTag()->GetPlayCount());
      return strPlayCount;
    }
  case LISTITEM_LASTPLAYED:
    {
      CStdString strLastPlayed;
      if (item->HasVideoInfoTag())
        return item->GetVideoInfoTag()->m_lastPlayed.GetAsLocalizedDate();
      if (item->HasMusicInfoTag())
        return item->GetMusicInfoTag()->GetLastPlayed().GetAsLocalizedDate();
      break;
    }
  case LISTITEM_TRACKNUMBER:
    {
      CStdString track;
      if (item->HasMusicInfoTag())
        track.Format("%i", item->GetMusicInfoTag()->GetTrackNumber());

      return track;
    }
  case LISTITEM_DISC_NUMBER:
    {
      CStdString disc;
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDiscNumber() > 0)
        disc.Format("%i", item->GetMusicInfoTag()->GetDiscNumber());
      return disc;
    }
  case LISTITEM_ARTIST:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_ALBUM_ARTIST:
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_DIRECTOR:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_ALBUM:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strAlbum;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbum();
    break;
  case LISTITEM_YEAR:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iYear > 0)
        strResult.Format("%i",item->GetVideoInfoTag()->m_iYear);
      return strResult;
    }
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetYearString();
    break;
  case LISTITEM_PREMIERED:
    if (item->HasVideoInfoTag())
    {
      if (item->GetVideoInfoTag()->m_firstAired.IsValid())
        return item->GetVideoInfoTag()->m_firstAired.GetAsLocalizedDate();
      if (item->GetVideoInfoTag()->m_premiered.IsValid())
        return item->GetVideoInfoTag()->m_premiered.GetAsLocalizedDate();
    }
    break;
  case LISTITEM_GENRE:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetGenre(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_FILENAME:
  case LISTITEM_FILE_EXTENSION:
    {
      CStdString strFile;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        strFile = URIUtils::GetFileName(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        strFile = URIUtils::GetFileName(item->GetVideoInfoTag()->m_strFileNameAndPath);
      else
        strFile = URIUtils::GetFileName(item->GetPath());

      if (info==LISTITEM_FILE_EXTENSION)
      {
        CStdString strExtension = URIUtils::GetExtension(strFile);
        return strExtension.TrimLeft(".");
      }
      return strFile;
    }
    break;
  case LISTITEM_DATE:
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate();
    break;
  case LISTITEM_SIZE:
    if (!item->m_bIsFolder || item->m_dwSize)
      return StringUtils::SizeToString(item->m_dwSize);
    break;
  case LISTITEM_RATING:
    {
      CStdString rating;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
        rating.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > '0')
      { // song rating.  Images will probably be better than numbers for this in the long run
        rating = item->GetMusicInfoTag()->GetRating();
      }
      return rating;
    }
  case LISTITEM_RATING_AND_VOTES:
    {
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
      {
        CStdString strRatingAndVotes;
        if (item->GetVideoInfoTag()->m_strVotes.IsEmpty())
          strRatingAndVotes.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
        else
          strRatingAndVotes.Format("%.1f (%s %s)", item->GetVideoInfoTag()->m_fRating, item->GetVideoInfoTag()->m_strVotes, g_localizeStrings.Get(20350));
        return strRatingAndVotes;
      }
    }
    break;
  case LISTITEM_PROGRAM_COUNT:
    {
      CStdString count;
      count.Format("%i", item->m_iprogramCount);
      return count;
    }
  case LISTITEM_DURATION:
    {
      CStdString duration;
      if (item->HasVideoInfoTag())
      {
        duration.Format("%d", item->GetVideoInfoTag()->GetDuration() / 60);
      }
      if (item->HasMusicInfoTag())
      {
        if (item->GetMusicInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetMusicInfoTag()->GetDuration());
      }
      return duration;
    }
  case LISTITEM_PLOT:
    if (item->HasVideoInfoTag())
    {
      if (!(!item->GetVideoInfoTag()->m_strShowTitle.IsEmpty() && item->GetVideoInfoTag()->m_iSeason == -1)) // dont apply to tvshows
        if (item->GetVideoInfoTag()->m_playCount == 0 && !CSettings::Get().GetBool("videolibrary.showunwatchedplots"))
          return g_localizeStrings.Get(20370);

      return item->GetVideoInfoTag()->m_strPlot;
    }
    break;
  case LISTITEM_PLOT_OUTLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strPlotOutline;
    break;
  case LISTITEM_EPISODE:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
        strResult.Format("S%d",item->GetVideoInfoTag()->m_iEpisode);
      else if (item->GetVideoInfoTag()->m_iEpisode > 0) // if m_iEpisode = -1 there's no episode detail
        strResult.Format("%d",item->GetVideoInfoTag()->m_iEpisode);
      return strResult;
    }
    break;
  case LISTITEM_SEASON:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iSpecialSortSeason > 0)
        strResult.Format("%d",item->GetVideoInfoTag()->m_iSpecialSortSeason);
      else if (item->GetVideoInfoTag()->m_iSeason > 0) // if m_iSeason = -1 there's no season detail
        strResult.Format("%d",item->GetVideoInfoTag()->m_iSeason);
      return strResult;
    }
    break;
  case LISTITEM_TVSHOW:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strShowTitle;
    break;
  case LISTITEM_COMMENT:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetComment();
    break;
  case LISTITEM_ACTUAL_ICON:
    return item->GetIconImage();
  case LISTITEM_ICON:
    {
      CStdString strThumb = item->GetArt("thumb");
      if (strThumb.IsEmpty())
        strThumb = item->GetIconImage();
      if (fallback)
        *fallback = item->GetIconImage();
      return strThumb;
    }
  case LISTITEM_OVERLAY:
    return item->GetOverlayImage();
  case LISTITEM_THUMB:
    return item->GetArt("thumb");
  case LISTITEM_FOLDERPATH:
    return CURL(item->GetPath()).GetWithoutUserDetails();
  case LISTITEM_FOLDERNAME:
  case LISTITEM_PATH:
    {
      CStdString path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = URIUtils::GetDirectory(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
      {
        if( item->m_bIsFolder )
          path = item->GetVideoInfoTag()->m_strPath;
        else
          URIUtils::GetParentPath(item->GetVideoInfoTag()->m_strFileNameAndPath, path);
      }
      else
        URIUtils::GetParentPath(item->GetPath(), path);
      path = CURL(path).GetWithoutUserDetails();
      if (info==LISTITEM_FOLDERNAME)
      {
        URIUtils::RemoveSlashAtEnd(path);
        path=URIUtils::GetFileName(path);
      }
      CURL::Decode(path);
      return path;
     }
   case LISTITEM_FILENAME_AND_PATH:
    {
      CStdString path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = item->GetMusicInfoTag()->GetURL();
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;
      else
        path = item->GetPath();
      path = CURL(path).GetWithoutUserDetails();
      CURL::Decode(path);
      return path;
    }
  case LISTITEM_PICTURE_PATH:
    if (item->IsPicture() && (!item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR()))
      return item->GetPath();
    break;
  case LISTITEM_STUDIO:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_COUNTRY:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_MPAA:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strMPAARating;
    break;
  case LISTITEM_CAST:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast();
    break;
  case LISTITEM_CAST_AND_ROLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast(true);
    break;
  case LISTITEM_WRITER:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_TAGLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTagLine;
    break;
  case LISTITEM_TRAILER:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTrailer;
    break;
  case LISTITEM_TOP250:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iTop250 > 0)
        strResult.Format("%i",item->GetVideoInfoTag()->m_iTop250);
      return strResult;
    }
    break;
  case LISTITEM_SORT_LETTER:
    {
      CStdString letter;
      std::wstring character(1, item->GetSortLabel()[0]);
      StringUtils2::ToUpper(character);
      g_charsetConverter.wToUTF8(character, letter);
      return letter;
    }
    break;
  case LISTITEM_TAG:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_tags, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_SET:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strSet;
    break;
  case LISTITEM_VIDEO_CODEC:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetVideoCodec();
    break;
  case LISTITEM_VIDEO_RESOLUTION:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoDimsToResolutionDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoWidth(), item->GetVideoInfoTag()->m_streamDetails.GetVideoHeight());
    break;
  case LISTITEM_VIDEO_ASPECT:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoAspectToAspectDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoAspect());
    break;
  case LISTITEM_AUDIO_CODEC:
    if (item->HasVideoInfoTag())
    {
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioCodec();
    }
    break;
  case LISTITEM_AUDIO_CHANNELS:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      int iChannels = item->GetVideoInfoTag()->m_streamDetails.GetAudioChannels();
      if (iChannels > -1)
        strResult.Format("%i", iChannels);
      return strResult;
    }
    break;
  case LISTITEM_AUDIO_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioLanguage();
    break;
  case LISTITEM_SUBTITLE_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetSubtitleLanguage();
    break;
  case LISTITEM_DATE_ADDED:
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_dateAdded.IsValid())
      return item->GetVideoInfoTag()->m_dateAdded.GetAsLocalizedDate();
    break;
  case LISTITEM_DBTYPE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_type;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetType();
    break;
  case LISTITEM_DBID:
    if (item->HasVideoInfoTag())
      {
        CStdString dbid;
        dbid.Format("%i", item->GetVideoInfoTag()->m_iDbId);
        return dbid;
      }
    if (item->HasMusicInfoTag())
      {
        CStdString dbid;
        dbid.Format("%i", item->GetMusicInfoTag()->GetDatabaseId());
        return dbid;
      }
    break;
  }
  return "";
}

CStdString CGUIInfoManager::GetItemImage(const CFileItem *item, int info, CStdString *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true, item);

  switch (info)
  {
  case LISTITEM_RATING:  // old song rating format
    {
      CStdString rating;
      if (item->HasMusicInfoTag())
      {
        rating.Format("songrating%c.png", item->GetMusicInfoTag()->GetRating());
        return rating;
      }
    }
    break;
  case LISTITEM_STAR_RATING:
    {
      CStdString rating;
      if (item->HasVideoInfoTag())
      { // rating for videos is assumed 0..10, so convert to 0..5
        rating.Format("rating%d.png", (long)((item->GetVideoInfoTag()->m_fRating * 0.5f) + 0.5f));
      }
      else if (item->HasMusicInfoTag())
      { // song rating.
        rating.Format("rating%c.png", item->GetMusicInfoTag()->GetRating());
      }
      return rating;
    }
    break;
  }  /* switch (info) */

  return GetItemLabel(item, info, fallback);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int condition) const
{
  if (!item) return false;
  if (condition >= LISTITEM_PROPERTY_START && condition - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    CStdString property = m_listitemProperties[condition - LISTITEM_PROPERTY_START];
    return item->GetProperty(property).asBoolean();
  }
  else if (condition == LISTITEM_ISPLAYING)
  {
    if (item->IsFileItem() && !m_currentFile->GetPath().IsEmpty())
    {
      if (!g_application.m_strPlayListFile.IsEmpty())
      {
        //playlist file that is currently playing or the playlistitem that is currently playing.
        return g_application.m_strPlayListFile.Equals(((const CFileItem *)item)->GetPath()) || m_currentFile->IsSamePath((const CFileItem *)item);
      }
      return m_currentFile->IsSamePath((const CFileItem *)item);
    }
  }
  else if (condition == LISTITEM_ISSELECTED)
    return item->IsSelected();
  else if (condition == LISTITEM_IS_FOLDER)
    return item->m_bIsFolder;
  else if (condition == LISTITEM_IS_RESUMABLE)
    return (item->IsFileItem() && ((const CFileItem *)item)->HasVideoInfoTag() && ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds > 0);
  return false;
}

void CGUIInfoManager::ResetCache()
{
  // reset any animation triggers as well
  m_containerMoves.clear();
  // mark our infobools as dirty
  CSingleLock lock(m_critInfo);
  for (vector<InfoPtr>::iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    (*i)->SetDirty();
}

// Called from tuxbox service thread to update current status
void CGUIInfoManager::UpdateFromTuxBox()
{ 
  if(g_tuxbox.vVideoSubChannel.mode)
    m_currentFile->GetVideoInfoTag()->m_strTitle = g_tuxbox.vVideoSubChannel.current_name;

  // Set m_currentMovieDuration
  if(!g_tuxbox.sCurSrvData.current_event_duration.IsEmpty() && 
    !g_tuxbox.sCurSrvData.next_event_description.IsEmpty() &&      
    !g_tuxbox.sCurSrvData.current_event_duration.Equals("-") && 
    !g_tuxbox.sCurSrvData.next_event_description.Equals("-"))
  {
    g_tuxbox.sCurSrvData.current_event_duration.Replace("(","");
    g_tuxbox.sCurSrvData.current_event_duration.Replace(")","");
  
    m_currentMovieDuration.Format("%s: %s %s (%s - %s)",
      g_localizeStrings.Get(180),
      g_tuxbox.sCurSrvData.current_event_duration,
      g_localizeStrings.Get(12391),
      g_tuxbox.sCurSrvData.current_event_time, 
      g_tuxbox.sCurSrvData.next_event_time);
  }

  //Set strVideoGenre
  if (!g_tuxbox.sCurSrvData.current_event_description.IsEmpty() && 
    !g_tuxbox.sCurSrvData.next_event_description.IsEmpty() && 
    !g_tuxbox.sCurSrvData.current_event_description.Equals("-") && 
    !g_tuxbox.sCurSrvData.next_event_description.Equals("-"))
  {
    CStdString genre;
    genre.Format("%s %s  -  (%s: %s)",
      g_localizeStrings.Get(143),
      g_tuxbox.sCurSrvData.current_event_description,
      g_localizeStrings.Get(209),
      g_tuxbox.sCurSrvData.next_event_description);
   m_currentFile->GetVideoInfoTag()->m_genre = StringUtils::Split(genre, g_advancedSettings.m_videoItemSeparator);
  }

  //Set m_currentMovie.m_director
  if (!g_tuxbox.sCurSrvData.current_event_details.Equals("-") &&
    !g_tuxbox.sCurSrvData.current_event_details.IsEmpty())
  {
    m_currentFile->GetVideoInfoTag()->m_director = StringUtils::Split(g_tuxbox.sCurSrvData.current_event_details, g_advancedSettings.m_videoItemSeparator);
  }
}

CStdString CGUIInfoManager::GetPictureLabel(int info)
{
  if (info == SLIDE_FILE_NAME)
    return GetItemLabel(m_currentSlide, LISTITEM_FILENAME);
  else if (info == SLIDE_FILE_PATH)
  {
    CStdString path = URIUtils::GetDirectory(m_currentSlide->GetPath());
    return CURL(path).GetWithoutUserDetails();
  }
  else if (info == SLIDE_FILE_SIZE)
    return GetItemLabel(m_currentSlide, LISTITEM_SIZE);
  else if (info == SLIDE_FILE_DATE)
    return GetItemLabel(m_currentSlide, LISTITEM_DATE);
  else if (info == SLIDE_INDEX)
  {
    CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (slideshow && slideshow->NumSlides())
    {
      CStdString index;
      index.Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
      return index;
    }
  }
  if (m_currentSlide->HasPictureInfoTag())
    return m_currentSlide->GetPictureInfoTag()->GetInfo(info);
  return "";
}

void CGUIInfoManager::SetCurrentSlide(CFileItem &item)
{
  if (m_currentSlide->GetPath() != item.GetPath())
  {
    if (!item.HasPictureInfoTag() && !item.GetPictureInfoTag()->Loaded())
      item.GetPictureInfoTag()->Load(item.GetPath());
    *m_currentSlide = item;
  }
}

void CGUIInfoManager::ResetCurrentSlide()
{
  m_currentSlide->Reset();
}

bool CGUIInfoManager::CheckWindowCondition(CGUIWindow *window, int condition) const
{
  // check if it satisfies our condition
  if (!window) return false;
  if ((condition & WINDOW_CONDITION_HAS_LIST_ITEMS) && !window->HasListItems())
    return false;
  if ((condition & WINDOW_CONDITION_IS_MEDIA_WINDOW) && !window->IsMediaWindow())
    return false;
  return true;
}

CGUIWindow *CGUIInfoManager::GetWindowWithCondition(int contextWindow, int condition) const
{
  CGUIWindow *window = g_windowManager.GetWindow(contextWindow);
  if (CheckWindowCondition(window, condition))
    return window;

  // try topmost dialog
  window = g_windowManager.GetWindow(g_windowManager.GetTopMostModalDialogID());
  if (CheckWindowCondition(window, condition))
    return window;

  // try active window
  window = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (CheckWindowCondition(window, condition))
    return window;

  return NULL;
}

void CGUIInfoManager::SetCurrentVideoTag(const CVideoInfoTag &tag)
{     
  *m_currentFile->GetVideoInfoTag() = tag; 
  m_currentFile->m_lStartOffset = 0;
}

void CGUIInfoManager::SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag) 
{
  //CLog::Log(LOGDEBUG, "Asked to SetCurrentTag");
  *m_currentFile->GetMusicInfoTag() = tag; 
  m_currentFile->m_lStartOffset = 0;
}

CStdString CGUIInfoManager::VideoWidthToResolutionDescription(int iWidth) const
{
  if (iWidth == 0)
    return "";

  else if (iWidth < 721)
    return "480";
  // 960x540
  else if (iWidth < 961)
    return "540";
  // 1280x720
  else if (iWidth < 1281)
    return "720";
  // 1920x1080
  else 
    return "1080";
}

CStdString CGUIInfoManager::VideoAspectToAspectDescription(float fAspect) const
{
  const float VIDEOASPECT_EPSILON = 0.025f;
  if (fAspect == 0.0f)
    return "";

  // With the epsilon method some of the ranges slightly overlap
  // so go in increasing size order to minimize the impact
  // of a growing tolerance value
  float fTolerance = (fAspect * VIDEOASPECT_EPSILON);

  // 4:3 video standard
  if (fabs(fAspect - 1.33f) < fTolerance)
    return "1.33";
  // 1.66:1 35mm European flat
  if (fabs(fAspect - 1.66f) < fTolerance)
    return "1.66";
  // 16:9 video widescreen 
  if (fabs(fAspect - 1.77f) < fTolerance)
    return "1.78";
  // 1.85:1 35mm US flat (theatrical widescreen)
  if (fabs(fAspect - 1.85f) < fTolerance)
    return "1.85";
  // 2.20:1 70m standard
  if (fabs(fAspect - 2.20f) < fTolerance)
    return "2.20";
  // 2.35:1 anamorphic wide - included are both true 2.35 (pre 1970s) and new
  // 2.39 as the industry convetion is to call the new standard 2.35 anyway
  if (fabs(fAspect - 2.35f) < fTolerance)
    return "2.35";
  if (fabs(fAspect - 2.39f) < fTolerance)
    return "2.35";

  return "";
}

const CFileItem& CGUIInfoManager::GetCurrentSlide() const
{ 
  return *m_currentSlide; 
}

const MUSIC_INFO::CMusicInfoTag* CGUIInfoManager::GetCurrentSongTag() const
{
  if (m_currentFile->HasMusicInfoTag())
    return m_currentFile->GetMusicInfoTag(); 

  return NULL;
}

const CVideoInfoTag* CGUIInfoManager::GetCurrentMovieTag() const
{
  if (m_currentFile->HasVideoInfoTag())
    return m_currentFile->GetVideoInfoTag(); 

  return NULL;
}

void GUIInfo::SetInfoFlag(uint32_t flag)
{
  assert(flag >= (1 << 24));
  m_data1 |= flag;
}

uint32_t GUIInfo::GetInfoFlag() const
{
  // we strip out the bottom 24 bits, where we keep data
  // and return the flag only
  return m_data1 & 0xff000000;
}

uint32_t GUIInfo::GetData1() const
{
  // we strip out the top 8 bits, where we keep flags
  // and return the unflagged data
  return m_data1 & ((1 << 24) -1);
}

int GUIInfo::GetData2() const
{
  return m_data2;
}

void CGUIInfoManager::SetLibraryBool(int condition, bool value)
{
  switch (condition)
  {
    case LIBRARY_HAS_MUSIC:
      m_libraryHasMusic = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIES:
      m_libraryHasMovies = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIE_SETS:
      m_libraryHasMovieSets = value ? 1 : 0;
      break;
    case LIBRARY_HAS_TVSHOWS:
      m_libraryHasTVShows = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MUSICVIDEOS:
      m_libraryHasMusicVideos = value ? 1 : 0;
      break;
    default:
      break;
  }
}

void CGUIInfoManager::ResetLibraryBools()
{
  m_libraryHasMusic = -1;
  m_libraryHasMovies = -1;
  m_libraryHasTVShows = -1;
  m_libraryHasMusicVideos = -1;
  m_libraryHasMovieSets = -1;
}

bool CGUIInfoManager::GetLibraryBool(int condition)
{
  if (condition == LIBRARY_HAS_MUSIC)
  {
    if (m_libraryHasMusic < 0)
    { // query
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasMusic = (db.GetSongsCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusic > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIES)
  {
    if (m_libraryHasMovies < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovies = db.HasContent(VIDEODB_CONTENT_MOVIES) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovies > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIE_SETS)
  {
    if (m_libraryHasMovieSets < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovieSets = db.HasSets() ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovieSets > 0;
  }
  else if (condition == LIBRARY_HAS_TVSHOWS)
  {
    if (m_libraryHasTVShows < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasTVShows = db.HasContent(VIDEODB_CONTENT_TVSHOWS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasTVShows > 0;
  }
  else if (condition == LIBRARY_HAS_MUSICVIDEOS)
  {
    if (m_libraryHasMusicVideos < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMusicVideos = db.HasContent(VIDEODB_CONTENT_MUSICVIDEOS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusicVideos > 0;
  }
  else if (condition == LIBRARY_HAS_VIDEO)
  {
    return (GetLibraryBool(LIBRARY_HAS_MOVIES) ||
            GetLibraryBool(LIBRARY_HAS_TVSHOWS) ||
            GetLibraryBool(LIBRARY_HAS_MUSICVIDEOS));
  }
  return false;
}

int CGUIInfoManager::RegisterSkinVariableString(const CSkinVariableString* info)
{
  if (!info)
    return 0;

  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.push_back(*info);
  delete info;
  return CONDITIONAL_LABEL_START + m_skinVariableStrings.size() - 1;
}

int CGUIInfoManager::TranslateSkinVariableString(const CStdString& name, int context)
{
  for (vector<CSkinVariableString>::const_iterator it = m_skinVariableStrings.begin();
       it != m_skinVariableStrings.end(); ++it)
  {
    if (it->GetName().Equals(name) && it->GetContext() == context)
      return it - m_skinVariableStrings.begin() + CONDITIONAL_LABEL_START;
  }
  return 0;
}

CStdString CGUIInfoManager::GetSkinVariableString(int info,
                                                  bool preferImage /*= false*/,
                                                  const CGUIListItem *item /*= NULL*/)
{
  info -= CONDITIONAL_LABEL_START;
  if (info >= 0 && info < (int)m_skinVariableStrings.size())
    return m_skinVariableStrings[info].GetValue(preferImage, item);

  return "";
}

bool CGUIInfoManager::ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map)
{
  for (std::map<INFO::InfoPtr, bool>::const_iterator it = map.begin() ; it != map.end() ; it++)
  {
    if (it->first->Get() != it->second)
      return true;
  }
  return false;
}
