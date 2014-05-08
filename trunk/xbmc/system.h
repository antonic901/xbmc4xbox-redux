#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _XBOX
#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xvoice.h>
#include <xonline.h>
#define HAS_XBOX_D3D
#define HAS_RAM_CONTROL
#define HAS_XFONT
#define HAS_FILESYSTEM
#define HAS_GAMEPAD
#define HAS_IR_REMOTE
#define HAS_DVD_DRIVE
#define HAS_XBOX_HARDWARE
#define HAS_XBOX_NETWORK
#define HAS_VIDEO_PLAYBACK
#define HAS_XBOX_AUDIO
#define HAS_AUDIO_PASS_THROUGH
#define HAS_FTP_SERVER
#define HAS_WEB_SERVER
#define HAS_TIME_SERVER
#define HAS_VISUALISATION
#define HAS_KARAOKE
#undef HAS_CREDITS
#define HAS_MODPLAYER
#define HAS_SYSINFO
#define HAS_SCREENSAVER
#define HAS_MIKMOD
#define HAS_SECTIONS
#define HAS_UPNP
#define HAS_LCD
#define HAS_UNDOCUMENTED
#define HAS_SECTIONS
#define HAS_CDDA_RIPPER
#define HAS_PYTHON
#define HAS_AUDIO
#define HAS_EVENT_SERVER
#define SPYCE_SUPPORT
#undef HAS_NEW_KARAOKE
#else
#undef HAS_XBOX_D3D
#undef HAS_RAM_CONTROL
#undef HAS_XFONT
#undef HAS_FILESYSTEM
#undef HAS_GAMEPAD
#undef HAS_IR_REMOTE
#undef HAS_DVD_DRIVE
#undef HAS_XBOX_HARDWARE
#undef HAS_XBOX_NETWORK
#define HAS_VIDEO_PLAYBACK
#undef HAS_XBOX_AUDIO
#undef HAS_AUDIO_PASS_THROUGH
#undef HAS_FTP_SERVER
#undef HAS_WEB_SERVER
#undef SPYCE_SUPPORT
#undef HAS_TIME_SERVER
#undef HAS_VISUALISATION
#undef HAS_KARAOKE
#undef HAS_CREDITS
#undef HAS_MODPLAYER
#undef HAS_SYSINFO
#undef HAS_SCREENSAVER
#undef HAS_MIKMOD
#undef HAS_SECTIONS
#define HAS_UPNP
#undef HAS_LCD
#undef HAS_UNDOCUMENTED
#undef HAS_SECTIONS
#undef HAS_CDDA_RIPPER
#define HAS_PYTHON
#define HAS_AUDIO
#undef HAS_NEW_KARAOKE

// additional includes and defines
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace

#endif

#undef USE_LIBMAD

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD

#ifdef _XBOX
#if defined(_DEBUG) && defined(_MEMTRACKING)
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

#ifdef _XBOX
#ifdef QueryPerformanceFrequency
#undef QueryPerformanceFrequency
#endif
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)
#else
#undef GetFreeSpace
#endif

#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

#include "../xbmc/xbox/PlatformInclude.h"

#ifndef SVN_REV
#define SVN_REV "Unknown"
#endif
