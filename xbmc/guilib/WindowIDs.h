/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Window ID defines to make the code a bit more readable
#define WINDOW_INVALID                     9999 // do not change. value is used to avoid include in headers.
#define WINDOW_HOME                       10000
#define WINDOW_PROGRAMS                   10001
#define WINDOW_PICTURES                   10002
#define WINDOW_FILES                      10003
#define WINDOW_SETTINGS_MENU              10004
#define WINDOW_SYSTEM_INFORMATION         10007
#define WINDOW_SCREEN_CALIBRATION         10011

#define WINDOW_SETTINGS_START             10016
#define WINDOW_SETTINGS_SYSTEM            10016
#define WINDOW_SETTINGS_SERVICE           10018

#define WINDOW_SETTINGS_MYGAMES           10022

#define WINDOW_VIDEO_NAV                  10025
#define WINDOW_VIDEO_PLAYLIST             10028

#define WINDOW_LOGIN_SCREEN               10029

#define WINDOW_SETTINGS_PLAYER            10030
#define WINDOW_SETTINGS_MEDIA             10031
#define WINDOW_SETTINGS_INTERFACE         10032

#define WINDOW_SETTINGS_PROFILES          10034
#define WINDOW_SKIN_SETTINGS              10035

#define WINDOW_ADDON_BROWSER              10040

#define WINDOW_DIALOG_PROGRAM_INFO        10062
#define WINDOW_DIALOG_PROGRAM_SETTINGS    10063

#define WINDOW_DIALOG_YES_NO              10100
#define WINDOW_DIALOG_PROGRESS            10101
#define WINDOW_DIALOG_KEYBOARD            10103
#define WINDOW_DIALOG_VOLUME_BAR          10104
#define WINDOW_DIALOG_SUB_MENU            10105
#define WINDOW_DIALOG_CONTEXT_MENU        10106
#define WINDOW_DIALOG_KAI_TOAST           10107
#define WINDOW_DIALOG_NUMERIC             10109
#define WINDOW_DIALOG_GAMEPAD             10110
#define WINDOW_DIALOG_BUTTON_MENU         10111
#define WINDOW_DIALOG_PLAYER_CONTROLS     10114
#define WINDOW_DIALOG_SEEK_BAR            10115
#define WINDOW_DIALOG_MUSIC_OSD           10120
#define WINDOW_DIALOG_VIS_SETTINGS        10121
#define WINDOW_DIALOG_VIS_PRESET_LIST     10122
#define WINDOW_DIALOG_VIDEO_OSD_SETTINGS  10123
#define WINDOW_DIALOG_AUDIO_OSD_SETTINGS  10124
#define WINDOW_DIALOG_VIDEO_BOOKMARKS     10125
#define WINDOW_DIALOG_FILE_BROWSER        10126
#define WINDOW_DIALOG_NETWORK_SETUP       10128
#define WINDOW_DIALOG_MEDIA_SOURCE        10129
#define WINDOW_DIALOG_PROFILE_SETTINGS    10130
#define WINDOW_DIALOG_LOCK_SETTINGS       10131
#define WINDOW_DIALOG_CONTENT_SETTINGS    10132
#define WINDOW_DIALOG_LIBEXPORT_SETTINGS  10133
#define WINDOW_DIALOG_FAVOURITES          10134
#define WINDOW_DIALOG_SONG_INFO           10135
#define WINDOW_DIALOG_SMART_PLAYLIST_EDITOR 10136
#define WINDOW_DIALOG_SMART_PLAYLIST_RULE   10137
#define WINDOW_DIALOG_BUSY                10138
#define WINDOW_DIALOG_PICTURE_INFO        10139
#define WINDOW_DIALOG_ADDON_SETTINGS      10140
#define WINDOW_DIALOG_FULLSCREEN_INFO     10142
#define WINDOW_DIALOG_SLIDER              10145
#define WINDOW_DIALOG_ADDON_INFO          10146
#define WINDOW_DIALOG_TEXT_VIEWER         10147
#define WINDOW_DIALOG_EXT_PROGRESS        10151
#define WINDOW_DIALOG_MEDIA_FILTER        10152
#define WINDOW_DIALOG_SUBTITLES           10153
#define WINDOW_DIALOG_INFOPROVIDER_SETTINGS 10158

#define WINDOW_MUSIC_PLAYLIST             10500
#define WINDOW_MUSIC_NAV                  10502
#define WINDOW_MUSIC_PLAYLIST_EDITOR      10503

//#define WINDOW_VIRTUAL_KEYBOARD           11000
// WINDOW_ID's from 11100 to 11199 reserved for Skins

#define WINDOW_DIALOG_SELECT              12000
#define WINDOW_DIALOG_MUSIC_INFO          12001
#define WINDOW_DIALOG_OK                  12002
#define WINDOW_DIALOG_VIDEO_INFO          12003
#define WINDOW_FULLSCREEN_VIDEO           12005
#define WINDOW_VISUALISATION              12006
#define WINDOW_SLIDESHOW                  12007

//! @todo Numbers given here must match the ids given in strings.po for a translatable string for
//! the window. 12009 to 12014 are already taken for something else in strings.po (accidentally).
//! So, do not define windows with ids 12009 to 12014, unless strings.po got fixed.

#define WINDOW_WEATHER                    12600
#define WINDOW_INSIGNIA                   12700
#define WINDOW_SCREENSAVER                12900
#define WINDOW_DIALOG_VIDEO_OSD           12901

#define WINDOW_VIDEO_MENU                 12902
#define WINDOW_VIDEO_TIME_SEEK            12905 // virtual window for time seeking during fullscreen video

#define WINDOW_FULLSCREEN_GAME            12906

#define WINDOW_SPLASH                     12997 // splash window
#define WINDOW_START                      12998 // first window to load
#define WINDOW_STARTUP_ANIM               12999 // for startup animations

// WINDOW_ID's from 13000 to 13099 reserved for Python

#define WINDOW_PYTHON_START               13000
#define WINDOW_PYTHON_END                 13099

// WINDOW_ID's from 14000 to 14099 reserved for Addons

#define WINDOW_ADDON_START                14000
#define WINDOW_ADDON_END                  14099

