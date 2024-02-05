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

#include "system.h"
#include "GUISettings.h"
#include "Settings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/FanController.h"
#endif
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "XBTimeZone.h"
#ifdef HAS_XFONT
#include <xfont.h>
#endif
#include "storage/MediaManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/VideoSettings.h"
#include "cores/paplayer/AudioDecoder.h"
#include "LocalizeStrings.h"
#include "filesystem/CurlFile.h"
#include "guilib/GUIFont.h" // for FONT_STYLE_* definitions
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"

using namespace std;
using namespace ADDON;

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

#define DEFAULT_VISUALISATION "visualization.milkdrop"

struct sortsettings
{
  bool operator()(const CSetting* pSetting1, const CSetting* pSetting2)
  {
    return pSetting1->GetOrder() < pSetting2->GetOrder();
  }
};

void CSettingBool::FromString(const CStdString &strValue)
{
  m_bData = (strValue == "true");
}

CStdString CSettingBool::ToString()
{
  return m_bData ? "true" : "false";
}

CSettingSeparator::CSettingSeparator(int iOrder, const char *strSetting)
    : CSetting(iOrder, strSetting, 0, SEPARATOR_CONTROL)
{
}

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_fData = fData;
  m_fMin = fMin;
  m_fStep = fStep;
  m_fMax = fMax;
}

void CSettingFloat::FromString(const CStdString &strValue)
{
  SetData((float)atof(strValue.c_str()));
}

CStdString CSettingFloat::ToString()
{
  CStdString strValue;
  strValue.Format("%f", m_fData);
  return strValue;
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iFormat = -1;
  m_iLabelMin = -1;
  if (strFormat)
    m_strFormat = strFormat;
  else
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iLabelMin = iLabelMin;
  m_iFormat = iFormat;
  if (m_iFormat < 0)
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel,
                         int iData, const map<int,int>& entries, int iControlType)
  : CSetting(iOrder, strSetting, iLabel, iControlType),
    m_entries(entries)
{
  m_iData = iData;
  m_iMin = -1;
  m_iMax = -1;
  m_iStep = 1;
  m_iLabelMin = -1;
}

void CSettingInt::FromString(const CStdString &strValue)
{
  int id = atoi(strValue.c_str());
  SetData(id);
}

CStdString CSettingInt::ToString()
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", (unsigned int *)&iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
  CStdString strValue;
  strValue.Format("%x", m_iData);
  return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}

void CSettingString::FromString(const CStdString &strValue)
{
  m_strData = strValue;
}

CStdString CSettingString::ToString()
{
  return m_strData;
}

CSettingPath::CSettingPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString)
{
}

CSettingAddon::CSettingAddon(int iOrder, const char *strSetting, int iLabel, const char *strData, const TYPE type)
  : CSettingString(iOrder, strSetting, iLabel, strData, BUTTON_CONTROL_STANDARD, false, -1)
  , m_type(type)
{
}

void CSettingsGroup::GetCategories(vecSettingsCategory &vecCategories)
{
  vecCategories.clear();
  for (unsigned int i = 0; i < m_vecCategories.size(); i++)
  {
    vecSettings settings;
    // check whether we actually have these settings available.
    g_guiSettings.GetSettingsGroup(m_vecCategories[i]->m_strCategory, settings);
    if (settings.size())
      vecCategories.push_back(m_vecCategories[i]);
  }
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
}

void CGUISettings::Initialize()
{
  // Pictures settings
  AddGroup(0, 1);
  CSettingsCategory* pic = AddCategory(0, "pictures", 14081);
  AddBool(pic, "pictures.usetags", 14082, true);
  AddBool(pic,"pictures.generatethumbs",13360,true);
  AddBool(pic, "pictures.useexifrotation", 20184, true);
  AddBool(pic, "pictures.showvideos", 22022, false);
  AddInt(pic, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_HDTV_1080i, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);

  CSettingsCategory* cat = AddCategory(0, "slideshow", 108);
  AddInt(cat, "slideshow.staytime", 12378, 5, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(cat, "slideshow.displayeffects", 12379, true);
  AddBool(NULL, "slideshow.shuffle", 13319, false);

  // Programs settings
  AddGroup(1, 0);
  CSettingsCategory* pro = AddCategory(1, "myprograms", 16000);
  AddBool(pro, "myprograms.autoffpatch", 29999, false);
  AddSeparator(pro,"myprograms.sep1");
  AddBool(pro, "myprograms.gameautoregion",511,false);
  AddInt(pro, "myprograms.ntscmode", 16110, 0, 0, 1, 3, SPIN_CONTROL_TEXT);
  AddSeparator(pro,"myprograms.sep2");
  AddString(pro, "myprograms.trainerpath", 20003, "select folder", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddSeparator(pro,"myprograms.sep3");
  AddBool(pro, "myprograms.usedashpath", 13007, true);
  AddString(pro, "myprograms.dashboard", 13006, "C:\\xboxdash.xbe", BUTTON_CONTROL_PATH_INPUT, false, 655);

  // My Weather settings
  AddGroup(2, 8);
  CSettingsCategory* wea = AddCategory(2, "weather", 16000);
  AddInt(NULL, "weather.currentlocation", 0, 1, 1, 1, 3, SPIN_CONTROL_INT_PLUS, NULL, -1);
  AddString(wea, "weather.areacode1", 14019, "USNY0996 - New York, NY", BUTTON_CONTROL_STANDARD);
  AddString(wea, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(wea, "weather.areacode3", 14021, "JAXX0085 - Tokyo, Japan", BUTTON_CONTROL_STANDARD);
  AddSeparator(wea, "weather.sep1");
  AddDefaultAddon(wea, "weather.addon", 24027, DEFAULT_WEATHER_ADDON, ADDON_SCRIPT_WEATHER);
  AddString(wea, "weather.addonsettings", 21417, "", BUTTON_CONTROL_STANDARD, true);

  // My Music Settings
  AddGroup(3, 2);
  CSettingsCategory* ml = AddCategory(3,"musiclibrary",14022);
  AddBool(ml, "musiclibrary.enabled", 421, true);
  AddBool(ml, "musiclibrary.showcompilationartists", 13414, true);
  AddSeparator(ml,"musiclibrary.sep1");
  AddBool(ml,"musiclibrary.downloadinfo", 20192, false);
  AddDefaultAddon(ml, "musiclibrary.albumscraper", 20193, "metadata.albums.tadb.com", ADDON_SCRAPER_ALBUMS);
  AddDefaultAddon(ml, "musiclibrary.artistscraper", 20194, "metadata.artists.tadb.com", ADDON_SCRAPER_ARTISTS);
  AddBool(ml, "musiclibrary.updateonstartup", 22000, false);
  AddBool(NULL, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(ml,"musiclibrary.sep2");
  AddString(ml, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(ml, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* mp = AddCategory(3, "musicplayer", 14086);
  AddBool(mp, "musicplayer.autoplaynextitem", 489, true);
  AddBool(mp, "musicplayer.queuebydefault", 14084, false);
  AddSeparator(mp, "musicplayer.sep1");
  map<int,int> gain;
  gain.insert(make_pair(351,REPLAY_GAIN_NONE));
  gain.insert(make_pair(639,REPLAY_GAIN_TRACK));
  gain.insert(make_pair(640,REPLAY_GAIN_ALBUM));

  AddInt(mp, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, gain, SPIN_CONTROL_TEXT);
  AddInt(NULL, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(NULL, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(NULL, "musicplayer.replaygainavoidclipping", 643, false);
  AddInt(mp, "musicplayer.crossfade", 13314, 0, 0, 1, 15, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(mp, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(mp, "musicplayer.sep2");
  AddDefaultAddon(mp, "musicplayer.visualisation", 250, DEFAULT_VISUALISATION, ADDON_VIZ);
  AddSeparator(mp, "musicplayer.sep3");

  map<int,int> defaultMusicPlayers;
  defaultMusicPlayers.insert(make_pair(22027, PLAYER_PAPLAYER));
  defaultMusicPlayers.insert(make_pair(22029, PLAYER_MPLAYER));
  defaultMusicPlayers.insert(make_pair(22028, PLAYER_DVDPLAYER));
  AddInt(mp, "musicplayer.defaultplayer", 22003, PLAYER_PAPLAYER, defaultMusicPlayers, SPIN_CONTROL_TEXT);
#ifdef _XBOX
  AddBool(mp, "musicplayer.outputtoallspeakers", 252, false);
#endif

  CSettingsCategory* mf = AddCategory(3, "musicfiles", 14081);
  AddBool(mf, "musicfiles.usetags", 258, false);
  AddString(mf, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(mf, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(NULL, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(NULL, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddBool(mf, "musicfiles.findremotethumbs", 14059, true);

  CSettingsCategory* acd = AddCategory(3, "audiocds", 620);
  AddBool(acd, "audiocds.usecddb", 227, true);
  AddSeparator(acd, "audiocds.sep1");
  AddPath(acd,"audiocds.recordingpath",20000,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);
  AddString(acd, "audiocds.trackpathformat", 13307, "%A - %B/[%N. ][%A - ]%T", EDIT_CONTROL_INPUT, false, 16016);
  map<int,int> encoders;
  encoders.insert(make_pair(34005, CDDARIP_ENCODER_FLAC));
  encoders.insert(make_pair(34000,CDDARIP_ENCODER_LAME));
  encoders.insert(make_pair(34001,CDDARIP_ENCODER_VORBIS));
  encoders.insert(make_pair(34002,CDDARIP_ENCODER_WAV));
  AddInt(acd, "audiocds.encoder", 621, CDDARIP_ENCODER_LAME, encoders, SPIN_CONTROL_TEXT);

  map<int,int> qualities;
  qualities.insert(make_pair(604,CDDARIP_QUALITY_CBR));
  qualities.insert(make_pair(601,CDDARIP_QUALITY_MEDIUM));
  qualities.insert(make_pair(602,CDDARIP_QUALITY_STANDARD));
  qualities.insert(make_pair(603,CDDARIP_QUALITY_EXTREME));
  AddInt(acd, "audiocds.quality", 622, CDDARIP_QUALITY_CBR, qualities, SPIN_CONTROL_TEXT);
  AddInt(acd, "audiocds.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);
  AddInt(acd, "audiocds.compressionlevel", 665, 5, 0, 1, 8, SPIN_CONTROL_INT_PLUS);

  CSettingsCategory* kar = AddCategory(3, "karaoke", 13327);
  AddBool(kar, "karaoke.enabled", 13323, false);
  AddBool(kar, "karaoke.voiceenabled", 13361, false);
  AddInt(kar, "karaoke.volume", 13376, 100, 0, 1, 100, SPIN_CONTROL_INT, MASK_PERCENT);
  AddString(kar, "karaoke.port0voicemask", 13382, "None", SPIN_CONTROL_TEXT);
  AddString(kar, "karaoke.port1voicemask", 13383, "None", SPIN_CONTROL_TEXT);
  AddString(kar, "karaoke.port2voicemask", 13384, "None", SPIN_CONTROL_TEXT);
  AddString(kar, "karaoke.port3voicemask", 13385, "None", SPIN_CONTROL_TEXT);

  // System settings
  AddGroup(4, 13000);

  map<int,int> ledPlaybacks;
  ledPlaybacks.insert(make_pair(106, LED_PLAYBACK_OFF));
  ledPlaybacks.insert(make_pair(13002, LED_PLAYBACK_VIDEO));
  ledPlaybacks.insert(make_pair(475, LED_PLAYBACK_MUSIC));
  ledPlaybacks.insert(make_pair(476, LED_PLAYBACK_VIDEO_MUSIC));

  map<int,int> ledColours;
  ledColours.insert(make_pair(13340, LED_COLOUR_NO_CHANGE));
  ledColours.insert(make_pair(13341, LED_COLOUR_GREEN));
  ledColours.insert(make_pair(13342, LED_COLOUR_ORANGE));
  ledColours.insert(make_pair(13343, LED_COLOUR_RED));
  ledColours.insert(make_pair(13344, LED_COLOUR_CYCLE));
  ledColours.insert(make_pair(351, LED_COLOUR_OFF));

#ifdef HAS_XBOX_HARDWARE
  CSettingsCategory* sys = AddCategory(4, "system", 128);
  AddBool(sys, "system.mceremote", 13601, false);
  AddInt(sys, "system.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddInt(sys, "system.ledcolour", 13339, LED_COLOUR_NO_CHANGE, ledColours, SPIN_CONTROL_TEXT);
  AddInt(sys, "system.leddisableonplayback", 13345, LED_PLAYBACK_OFF, ledPlaybacks, SPIN_CONTROL_TEXT);
  AddBool(sys, "system.ledenableonpaused", 20313, true);
  AddSeparator(sys, "system.sep1");
  AddBool(sys, "system.fanspeedcontrol", 13302, false);
  AddInt(sys, "system.fanspeed", 13300, CFanController::Instance()->GetFanSpeed(), 5, 5, 50, SPIN_CONTROL_TEXT);
  AddSeparator(sys, "system.sep2");
  AddBool(sys, "system.autotemperature", 13301, false);
  AddInt(sys, "system.targettemperature", 13299, 55, 40, 1, 68, SPIN_CONTROL_TEXT);
  AddInt(sys, "system.minfanspeed", 13411, 1, 1, 1, 50, SPIN_CONTROL_TEXT);
#endif
  
  CSettingsCategory* vo = AddCategory(4, "videooutput", 21373);
  map<int,int> videoAspects;
  videoAspects.insert(make_pair(21375, VIDEO_NORMAL));
  videoAspects.insert(make_pair(21376, VIDEO_LETTERBOX));
  videoAspects.insert(make_pair(21377, VIDEO_WIDESCREEN));

  AddInt(vo, "videooutput.aspect", 21374, VIDEO_NORMAL, videoAspects, SPIN_CONTROL_TEXT);
  AddBool(vo,  "videooutput.hd480p", 21378, true);
  AddBool(vo,  "videooutput.hd720p", 21379, true);
  AddBool(vo,  "videooutput.hd1080i", 21380, false);

  CSettingsCategory* ao = AddCategory(4, "audiooutput", 772);

  map<int,int> audiomode;
  audiomode.insert(make_pair(338, AUDIO_ANALOG));
  audiomode.insert(make_pair(339, AUDIO_DIGITAL));
  AddInt(ao, "audiooutput.mode", 337, AUDIO_ANALOG, audiomode, SPIN_CONTROL_TEXT);
  AddBool(ao, "audiooutput.ac3passthrough", 364, true);
  AddBool(ao, "audiooutput.dtspassthrough", 254, true);
  AddBool(ao, "audiooutput.aacpassthrough", 299, false);
#ifdef _XBOX
  AddBool(ao, "audiooutput.downmixmultichannel", 548, true);
#endif

  CSettingsCategory* lcd = AddCategory(4, "lcd", 448);

  map<int,int> lcdTypes;
  lcdTypes.insert(make_pair(351, LCD_TYPE_NONE));
  lcdTypes.insert(make_pair(34006, LCD_TYPE_LCD_HD44780));
  lcdTypes.insert(make_pair(34007, LCD_TYPE_LCD_KS0073));
  lcdTypes.insert(make_pair(34008, LCD_TYPE_VFD));
  AddInt(lcd, "lcd.type", 4501, LCD_TYPE_NONE, lcdTypes, SPIN_CONTROL_TEXT);

  map<int,int> lcdModcips;
  lcdModcips.insert(make_pair(34009, MODCHIP_SMARTXX));
  lcdModcips.insert(make_pair(34010, MODCHIP_XENIUM));
  lcdModcips.insert(make_pair(34011, MODCHIP_XECUTER3));

  AddInt(lcd, "lcd.modchip", 471, MODCHIP_SMARTXX, lcdModcips, SPIN_CONTROL_TEXT);
  AddInt(lcd, "lcd.backlight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(lcd, "lcd.contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddSeparator(lcd, "lcd.sep1");
  AddInt(lcd, "lcd.disableonplayback", 20310, LED_PLAYBACK_OFF, ledPlaybacks, SPIN_CONTROL_TEXT);
  AddBool(lcd, "lcd.enableonpaused", 20312, true);

  CSettingsCategory* dbg = AddCategory(4, "debug", 14092);
  AddBool(dbg, "debug.showloginfo", 20191, false);
  AddPath(dbg, "debug.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  CSettingsCategory* ar = AddCategory(4, "autorun", 447);
  AddBool(ar, "autorun.dvd", 240, true);
  AddBool(ar, "autorun.vcd", 241, true);
  AddBool(ar, "autorun.cdda", 242, true);
  AddBool(ar, "autorun.xbox", 243, true);
  AddBool(ar, "autorun.video", 244, true);
  AddBool(ar, "autorun.music", 245, true);
  AddBool(ar, "autorun.pictures", 246, true);

  CSettingsCategory* hdd = AddCategory(4, "harddisk", 440);
  AddInt(hdd, "harddisk.spindowntime", 229, 0, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF); // Minutes
  map<int,int> remotePlaySpinDowns;
  remotePlaySpinDowns.insert(make_pair(474, SPIN_DOWN_NONE));
  remotePlaySpinDowns.insert(make_pair(475, SPIN_DOWN_MUSIC));
  remotePlaySpinDowns.insert(make_pair(13002, SPIN_DOWN_VIDEO));
  remotePlaySpinDowns.insert(make_pair(476, SPIN_DOWN_BOTH));

  AddInt(hdd, "harddisk.remoteplayspindown", 13001, 0, remotePlaySpinDowns, SPIN_CONTROL_TEXT); // off, music, video, both
  AddInt(NULL, "harddisk.remoteplayspindownminduration", 13004, 20, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_MINS); // Minutes
  AddInt(NULL, "harddisk.remoteplayspindowndelay", 13003, 20, 5, 5, 300, SPIN_CONTROL_INT_PLUS, MASK_SECS); // seconds

  map<int,int> aamLevels;
  aamLevels.insert(make_pair(21388, AAM_QUIET));
  aamLevels.insert(make_pair(21387, AAM_FAST));
  AddInt(hdd, "harddisk.aamlevel", 21386, AAM_FAST, aamLevels, SPIN_CONTROL_TEXT);

  map<int,int> apmLevels;
  apmLevels.insert(make_pair(21391, APM_HIPOWER));
  apmLevels.insert(make_pair(21392, APM_LOPOWER));
  apmLevels.insert(make_pair(21393, APM_HIPOWER_STANDBY));
  apmLevels.insert(make_pair(21394, APM_LOPOWER_STANDBY));
  AddInt(hdd, "harddisk.apmlevel", 21390, APM_HIPOWER, apmLevels, SPIN_CONTROL_TEXT);

  CSettingsCategory* dpc = AddCategory(4, "dvdplayercache", 483);
  AddInt(dpc, "dvdplayercache.video", 14096, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(dpc, "dvdplayercache.videotime", 14097, 8, 0, 1, 30, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddSeparator(dpc, "dvdplayercache.sep1");
  AddInt(dpc, "dvdplayercache.audio", 14098, 384, 0, 128, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(dpc, "dvdplayercache.audiotime", 14099, 8, 0, 1, 30, SPIN_CONTROL_INT_PLUS, MASK_SECS);

  CSettingsCategory* cac = AddCategory(4, "cache", 439);
  AddInt(cac, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(cac, "cache.sep1");
  AddInt(cac, "cachevideo.dvdrom", 14026, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(cac, "cachevideo.lan", 14027, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(cac, "cachevideo.internet", 14028, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(cac, "cache.sep2");
  AddInt(cac, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(cac, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(cac, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(cac, "cache.sep3");
  AddInt(cac, "cachedvd.dvdrom", 14034, 512, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(cac, "cachedvd.lan", 14035, 512, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(cac, "cache.sep4");
  AddInt(cac, "cacheunknown.internet", 14060, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  // !! Should be the last category, else disabling it will cause problems!
  CSettingsCategory* mst = AddCategory(4, "masterlock", 12360);
  AddString(mst, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddBool(mst, "masterlock.startuplock"      , 20076,false);
  AddBool(mst, "masterlock.enableshutdown"   , 12362,false);  
  // hidden masterlock settings
  AddInt(NULL,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  // video settings
  AddGroup(5, 3);
  CSettingsCategory* vdl = AddCategory(5, "videolibrary", 14022);
  AddBool(vdl, "videolibrary.enabled", 421, true);
  AddBool(vdl, "videolibrary.showunwatchedplots", 20369, true);
  AddBool(vdl, "videolibrary.seasonthumbs", 20382, true);
  AddBool(vdl, "videolibrary.actorthumbs", 20402, false);

  map<int,int> flattenTVShowOptions;
  flattenTVShowOptions.insert(make_pair(20420, 0));
  flattenTVShowOptions.insert(make_pair(20421, 1));
  flattenTVShowOptions.insert(make_pair(20422, 2));
  AddInt(vdl, "videolibrary.flattentvshows", 20412, 1, flattenTVShowOptions, SPIN_CONTROL_TEXT);

  AddBool(vdl, "videolibrary.groupmoviesets", 20458, false);
  AddBool(vdl, "videolibrary.updateonstartup", 22000, false);
  AddBool(NULL, "videolibrary.backgroundupdate", 22001, false);
  AddSeparator(vdl, "videolibrary.sep3");
  AddString(vdl, "videolibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(vdl, "videolibrary.export", 647, "", BUTTON_CONTROL_STANDARD);
  AddString(vdl, "videolibrary.import", 648, "", BUTTON_CONTROL_STANDARD);

  CSettingsCategory* vp = AddCategory(5, "videoplayer", 14086);

  AddString(vp, "videoplayer.calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(vp, "videoplayer.sep1");

  map<int,int> renderMethods;
  renderMethods.insert(make_pair(13355, RENDER_LQ_RGB_SHADER));
  renderMethods.insert(make_pair(13356, RENDER_OVERLAYS));
  renderMethods.insert(make_pair(13357, RENDER_HQ_RGB_SHADER));
  renderMethods.insert(make_pair(21397, RENDER_HQ_RGB_SHADERV2));

  AddInt(vp, "videoplayer.rendermethod", 13354, RENDER_HQ_RGB_SHADER, renderMethods, SPIN_CONTROL_TEXT);
  AddInt(vp, "videoplayer.displayresolution", 169, (int)RES_AUTORES, (int)RES_HDTV_1080i, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);

  map<int,int> framerateConversions;
  framerateConversions.insert(make_pair(231, FRAME_RATE_LEAVE_AS_IS));
  framerateConversions.insert(make_pair(g_videoConfig.HasPAL() ? 12380 : 12381, FRAME_RATE_CONVERT));
  if (g_videoConfig.HasPAL() && g_videoConfig.HasPAL60())
    framerateConversions.insert(make_pair(12382, FRAME_RATE_USE_PAL60));

  AddInt(vp, "videoplayer.framerateconversions", 336, FRAME_RATE_LEAVE_AS_IS, framerateConversions, SPIN_CONTROL_TEXT);
  AddInt(vp, "videoplayer.flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(vp, "videoplayer.soften", 215, false);
  AddFloat(vp, "videoplayer.errorinaspect", 22021, 3.0f, 0.0f, 1.0f, 20.0f);
  AddSeparator(vp, "videoplayer.sep2");

  map<int,int> defaultVideoPlayers;
  defaultVideoPlayers.insert(make_pair(22028, PLAYER_DVDPLAYER));
  defaultVideoPlayers.insert(make_pair(22029, PLAYER_MPLAYER));

  AddInt(vp, "videoplayer.defaultplayer", 22003, PLAYER_DVDPLAYER, defaultVideoPlayers, SPIN_CONTROL_TEXT);
  AddBool(vp, "videoplayer.allcodecs", 22025, false);
  AddBool(vp, "videoplayer.fast", 22026, false);

  map<int,int> skipLoopFilters;
  skipLoopFilters.insert(make_pair(14101, VS_SKIPLOOP_DEFAULT));
  skipLoopFilters.insert(make_pair(14102, VS_SKIPLOOP_NONREF));
  skipLoopFilters.insert(make_pair(14103, VS_SKIPLOOP_BIDIR));
  skipLoopFilters.insert(make_pair(14104, VS_SKIPLOOP_NONKEY));
  skipLoopFilters.insert(make_pair(14105, VS_SKIPLOOP_ALL));
  AddInt(vp, "videoplayer.skiploopfilter", 14100, VS_SKIPLOOP_NONREF, skipLoopFilters, SPIN_CONTROL_TEXT);

  CSettingsCategory* vid = AddCategory(5, "myvideos", 14081);

  map<int, int> myVideosSelectActions;
  myVideosSelectActions.insert(make_pair(22080, SELECT_ACTION_CHOOSE));
  myVideosSelectActions.insert(make_pair(208,   SELECT_ACTION_PLAY_OR_RESUME));
  myVideosSelectActions.insert(make_pair(13404, SELECT_ACTION_RESUME));
  myVideosSelectActions.insert(make_pair(22081, SELECT_ACTION_INFO));

  AddInt(vid, "myvideos.selectaction", 22079, SELECT_ACTION_PLAY_OR_RESUME, myVideosSelectActions, SPIN_CONTROL_TEXT);
  AddBool(NULL, "myvideos.extractflags",20433, false);
  AddBool(NULL, "myvideos.replacelabels", 20419, true);
  AddBool(NULL, "myvideos.extractthumb",20433, false);

  AddSeparator(NULL, "myvideos.sep1");
  AddInt(NULL, "myvideos.startwindow", 0, WINDOW_VIDEO_FILES, WINDOW_VIDEO_FILES, 1, WINDOW_VIDEO_NAV, SPIN_CONTROL_INT);
  AddBool(NULL, "myvideos.stackvideos", 0, false);
  AddBool(NULL, "myvideos.flatten", 0, false);

  CSettingsCategory* sub = AddCategory(5, "subtitles", 287);
  AddString(sub, "subtitles.font", 288, "Arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(sub, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed

  map<int,int> fontStyles;
  fontStyles.insert(make_pair(738, FONT_STYLE_NORMAL));
  fontStyles.insert(make_pair(739, FONT_STYLE_BOLD));
  fontStyles.insert(make_pair(740, FONT_STYLE_ITALICS));
  fontStyles.insert(make_pair(741, FONT_STYLE_BOLD_ITALICS));

  AddInt(sub, "subtitles.style", 736, FONT_STYLE_BOLD, fontStyles, SPIN_CONTROL_TEXT);
  AddInt(sub, "subtitles.color", 737, SUBTITLE_COLOR_START + 1, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(sub, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(sub, "subtitles.sep1");
  AddPath(sub, "subtitles.custompath", 21366, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddSeparator(sub,"subtitles.sep2");
  AddBool(sub, "subtitles.searchrars", 13249, false);

  CSettingsCategory* dvd = AddCategory(5, "dvds", 14087);
  AddInt(dvd, "dvds.playerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(dvd, "dvds.automenu", 21882, false);
  AddBool(dvd, "dvds.useexternaldvdplayer", 20001, false);
  AddString(dvd, "dvds.externaldvdplayer", 20002, "",  BUTTON_CONTROL_PATH_INPUT, true, 655);

  // Don't add the category - makes them hidden in the GUI
  AddCategory(5, "postprocessing", 14041);
  AddBool(NULL, "postprocessing.enable", 286, false);
  AddBool(NULL, "postprocessing.auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(NULL, "postprocessing.verticaldeblocking", 308, false);
  AddInt(NULL, "postprocessing.verticaldeblocklevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(NULL, "postprocessing.horizontaldeblocking", 309, false);
  AddInt(NULL, "postprocessing.horizontaldeblocklevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(NULL, "postprocessing.autobrightnesscontrastlevels", 310, false);
  AddBool(NULL, "postprocessing.dering", 311, false);

  AddDefaultAddon(NULL, "scrapers.moviesdefault", 21413, "metadata.movies.themoviedb.org", ADDON_SCRAPER_MOVIES);
  AddDefaultAddon(NULL, "scrapers.tvshowsdefault", 21414, "metadata.tvshows.themoviedb.org", ADDON_SCRAPER_TVSHOWS);
  AddDefaultAddon(NULL, "scrapers.musicvideosdefault", 21415, "metadata.musicvideos.nfo", ADDON_SCRAPER_MUSICVIDEOS);

  // network settings
  AddGroup(6, 705);

  CSettingsCategory* srv = AddCategory(6, "services", 14036);
  AddString(srv,"services.devicename", 1271, "XBMC", EDIT_CONTROL_INPUT);
  AddSeparator(srv,"services.sep4");
  AddBool(srv, "services.upnpserver", 21360, false);
  AddBool(srv, "services.upnprenderer", 21881, false);
  AddSeparator(srv,"services.sep3");

  AddBool(srv,  "services.webserver",        263, false);
  AddString(srv,"services.webserverport",    730, "80", EDIT_CONTROL_NUMBER_INPUT, false, 730);
  AddString(srv,"services.webserverusername",1048, "xbmc", EDIT_CONTROL_INPUT);
  AddString(srv,"services.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);
  AddDefaultAddon(srv, "services.webskin",199, DEFAULT_WEB_INTERFACE, ADDON_WEB_INTERFACE);
  AddInt(NULL, "services.httpapibroadcastlevel", 0, 0, 0, 1, 5, SPIN_CONTROL_INT);
  AddInt(NULL, "services.httpapibroadcastport", 0, 8278, 1, 1, 65535, SPIN_CONTROL_INT);
#ifdef HAS_EVENT_SERVER
  AddSeparator(srv,"services.sep1");
  AddBool(srv,  "services.esenabled",         794, true);
  AddString(NULL,"services.esport",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(NULL,   "services.esportrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.esmaxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.esinitialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(NULL,   "services.escontinuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
#endif

  AddSeparator(srv, "services.sep2");
  AddBool(srv,  "services.ftpserver",        167, true);
  AddString(srv,"services.ftpserveruser",    1245, "xbox", SPIN_CONTROL_TEXT);
  AddString(srv,"services.ftpserverpassword",1246, "xbox", EDIT_CONTROL_HIDDEN_INPUT, true, 1246);
  AddBool(srv,  "services.ftpautofatx",      771, true);

  CSettingsCategory* atd = AddCategory(6,"autodetect",           1250  );
  AddBool(atd,    "autodetect.onoff",     1251, false);
  AddBool(atd,    "autodetect.popupinfo", 1254, true);
  AddString(atd,  "autodetect.nickname",  1252, "XBMC-NickName",EDIT_CONTROL_INPUT, false, 1252);
  AddSeparator(atd, "autodetect.sep1");
  AddBool(atd,    "autodetect.senduserpw",1255, true); // can be in advanced.xml! default:true

  CSettingsCategory* smb = AddCategory(6, "smb", 1200);
  AddString(smb, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(smb, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);

  CSettingsCategory* net = AddCategory(6, "network", 705);

  map<int,int> networkAssignments;
  networkAssignments.insert(make_pair(716, NETWORK_DHCP));
  networkAssignments.insert(make_pair(717, NETWORK_STATIC));
  networkAssignments.insert(make_pair(718, NETWORK_DASH));

  AddInt(net, "network.assignment", 715, NETWORK_DHCP, networkAssignments, SPIN_CONTROL_TEXT);
  AddString(net, "network.ipaddress", 719, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(net, "network.subnet", 720, "255.255.255.0", EDIT_CONTROL_IP_INPUT);
  AddString(net, "network.gateway", 721, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(net, "network.dns", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(net, "network.dns2", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(net, "network.dnssuffix", 22002, "", EDIT_CONTROL_INPUT, true);
  AddInt(net, "network.bandwidth", 14042, 0, 0, 512, 100*1024, SPIN_CONTROL_INT_PLUS, MASK_KBPS, TEXT_OFF);

  AddSeparator(net, "network.sep1");
  AddBool(net, "network.usehttpproxy", 708, false);
  map<int,int> proxyTypes;
  proxyTypes.insert(make_pair(1181, XFILE::CCurlFile::PROXY_HTTP));
  proxyTypes.insert(make_pair(1182, XFILE::CCurlFile::PROXY_SOCKS4));
  proxyTypes.insert(make_pair(1183, XFILE::CCurlFile::PROXY_SOCKS4A));
  proxyTypes.insert(make_pair(1184, XFILE::CCurlFile::PROXY_SOCKS5));
  proxyTypes.insert(make_pair(1185, XFILE::CCurlFile::PROXY_SOCKS5_REMOTE));
  AddInt(net, "network.httpproxytype", 1180, XFILE::CCurlFile::PROXY_HTTP, proxyTypes, SPIN_CONTROL_TEXT);
  AddString(net, "network.httpproxyserver", 706, "", EDIT_CONTROL_INPUT);
  AddString(net, "network.httpproxyport", 730, "8080", EDIT_CONTROL_NUMBER_INPUT, false, 707);
  AddString(net, "network.httpproxyusername", 1048, "", EDIT_CONTROL_INPUT);
  AddString(net, "network.httpproxypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT,true,733);

  // appearance settings
  AddGroup(7, 480);
  CSettingsCategory* laf = AddCategory(7,"lookandfeel", 166);
  AddDefaultAddon(laf, "lookandfeel.skin",166,DEFAULT_SKIN, ADDON_SKIN);
  AddString(laf, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(laf, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(laf, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddString(laf, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(laf, "lookandfeel.sep2");
  AddBool(laf, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(laf, "lookandfeel.rssedit", 21450, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(laf, "lookandfeel.sep3");
  AddBool(laf, "lookandfeel.enablemouse", 21369, false);

  CSettingsCategory* loc = AddCategory(7, "locale", 14090);
  AddString(loc, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.country", 20026, "UK (24h)", SPIN_CONTROL_TEXT);
  AddString(loc, "locale.charset",14091,"DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
  AddSeparator(loc, "locale.sep1");
  AddString(loc, "locale.time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(loc, "locale.date", 14064, "", BUTTON_CONTROL_MISC_INPUT);
  AddInt(loc, "locale.timezone", 14074, 0, 0, 1, g_timezone.GetNumberOfTimeZones(), SPIN_CONTROL_TEXT);
  AddBool(loc, "locale.usedst", 14075, false);
  AddSeparator(loc, "locale.sep2");
  AddBool(loc, "locale.timeserver", 168, false);
  AddString(loc, "locale.timeserveraddress", 731, "pool.ntp.org", EDIT_CONTROL_INPUT);

  CSettingsCategory* vs = AddCategory(7, "videoscreen", 131);
  AddInt(vs, "videoscreen.resolution",169,(int)RES_AUTORES, (int)RES_HDTV_1080i, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);
  AddString(vs, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
  AddInt(vs, "videoscreen.flickerfilter", 13100, 5, 0, 1, 5, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(vs, "videoscreen.soften", 215, false);

  CSettingsCategory* fl = AddCategory(7, "filelists", 14081);
  AddBool(fl, "filelists.showparentdiritems", 13306, true);
  AddBool(fl, "filelists.showextensions", 497, true);
  AddBool(fl, "filelists.ignorethewhensorting", 13399, true);
  AddBool(fl, "filelists.allowfiledeletion", 14071, false);
  AddBool(fl, "filelists.showaddsourcebuttons", 21382,  true);
  AddBool(fl, "filelists.showhidden", 21330, false);
  AddSeparator(fl, "filelists.sep1");
  AddBool(fl, "filelists.unrollarchives",516, false);

  CSettingsCategory* ss = AddCategory(7, "screensaver", 360);
  AddInt(ss, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddDefaultAddon(ss, "screensaver.mode", 356, "screensaver.xbmc.builtin.dim", ADDON_SCREENSAVER);
  AddString(ss, "screensaver.settings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddString(ss, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(ss, "screensaver.sep1");
  AddBool(ss, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(ss, "screensaver.usedimonpause", 22014, true);

  AddPath(NULL,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);

  AddInt(NULL, "mymusic.startwindow", 0, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, 1, WINDOW_MUSIC_NAV, SPIN_CONTROL_INT);
  AddBool(NULL, "mymusic.songthumbinvis", 0, false);
  AddString(NULL, "mymusic.defaultlibview", 0, "", BUTTON_CONTROL_STANDARD);

  AddBool(NULL, "general.addonautoupdate", 0, true);
  AddBool(NULL, "general.addonnotifications", 0, true);
  AddBool(NULL, "general.addonforeignfilter", 0, false);
}

CGUISettings::~CGUISettings(void)
{
  Clear();
}

void CGUISettings::AddGroup(int groupID, int labelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(groupID, labelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

CSettingsCategory* CGUISettings::AddCategory(int groupID, const char *strSetting, int labelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), labelID);
  }
  return NULL;
}

CSettingsGroup *CGUISettings::GetGroup(int groupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group (%i) was not found.  "
                      "It must be case-sensitive",
            groupID);
  return NULL;
}

void CGUISettings::AddSeparator(CSettingsCategory* cat, const char *strSetting)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddBool(CSettingsCategory* cat, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  CStdString lower(strSetting);
  lower.ToLower();
  constMapIter it = settingsMap.find(lower);
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Forward compatibility for new skins (skins use this setting)
  if (lower == "input.enablemouse")
    return GetBool("lookandfeel.enablemouse");
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(CSettingsCategory* cat, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  //ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.enableshutdown");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
    it = settingsMap.find("autodetect.nickname");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}


void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(CSettingsCategory* cat, const char *strSetting,
                          int iLabel, int iData, const map<int,int>& entries,
                          int iControlType)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, entries, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddHex(CSettingsCategory* cat, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
}

void CGUISettings::AddString(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddPath(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddDefaultAddon(CSettingsCategory* cat, const char *strSetting, int iLabel, const char *strData, const TYPE type)
{
  int iOrder = cat?++cat->m_entries:0;
  CSettingAddon* pSetting = new CSettingAddon(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, type);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    CSettingString* result = ((CSettingString *)(*it).second);
    if (result->GetData() == "select folder" || result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,result->GetData() == "select writable folder"))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    return result->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  // hardcoded return value so that compiler is happy
  return StringUtils::EmptyString;
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  vecSettings unorderedSettings;
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0 && !(*it).second->IsAdvanced())
      unorderedSettings.push_back((*it).second);
  }
  // now order them...
  sort(unorderedSettings.begin(), unorderedSettings.end(), sortsettings());

  // remove any instances of 2 separators in a row
  bool lastWasSeparator(false);
  for (vecSettings::iterator it = unorderedSettings.begin(); it != unorderedSettings.end(); it++)
  {
    CSetting *setting = *it;
    // only add separators if we don't have 2 in a row
    if (setting->GetType() == SETTINGS_TYPE_SEPARATOR)
    {
      if (!lastWasSeparator)
        settings.push_back(setting);
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      settings.push_back(setting);
    }
  }
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("audiooutput.mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("audiooutput.mode", AUDIO_ANALOG);
  SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "AAC pass through is %s", GetBool("audiooutput.aacpassthrough") ? "enabled" : "disabled");

  if (g_videoConfig.HasLetterbox())
    SetInt("videooutput.aspect", VIDEO_LETTERBOX);
  else if (g_videoConfig.HasWidescreen())
    SetInt("videooutput.aspect", VIDEO_WIDESCREEN);
  else
    SetInt("videooutput.aspect", VIDEO_NORMAL);
  SetBool("videooutput.hd480p", g_videoConfig.Has480p());
  SetBool("videooutput.hd720p", g_videoConfig.Has720p());

  SetInt("locale.timezone", g_timezone.GetTimeZoneIndex());
  SetBool("locale.usedst", g_timezone.GetDST());

  // Move replaygain settings into our struct
  ReplayGainSettings &replayGainSettings = CAudioDecoder::GetReplayGainSettings();
  replayGainSettings.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  replayGainSettings.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  replayGainSettings.iType = GetInt("musicplayer.replaygaintype");
  replayGainSettings.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlElement *pGrandChild = pChild->FirstChildElement(strSplit[1].c_str());
      if (pGrandChild)
      {
        CStdString strValue = pGrandChild->FirstChild() ? pGrandChild->FirstChild()->Value() : "";
        if (strValue != "-")
        { // update our item
          (*it).second->FromString(strValue);
          if (advanced)
            (*it).second->SetAdvanced();
        }
      }
    }
  }
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    // don't save advanced settings
    CStdString first = (*it).first;
    if ((*it).second->IsAdvanced())
      continue;

    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
      if (!pChild)
      { // add our group tag
        TiXmlElement newElement(strSplit[0].c_str());
        pChild = pRootNode->InsertEndChild(newElement);
      }

      if (pChild)
      { // successfully added (or found) our group
        TiXmlElement newElement(strSplit[1]);
        if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
          newElement.SetAttribute("pathversion", XMLUtils::path_version);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();
}
