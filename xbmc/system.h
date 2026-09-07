#pragma once

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

#define DEBUG_KEYBOARD
#include <xtl.h>
#include <xvoice.h>
#include <xonline.h>

#define HAS_XBOX_D3D
#define HAS_RAM_CONTROL
#define HAS_XFONT
#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_RAR
#define HAS_GAMEPAD
#define HAS_IR_REMOTE
#define HAS_OPTICAL_DRIVE
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
#define HAVE_LIBMP3LAME
#define HAVE_LIBVORBISENC

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#if defined(_DEBUG) && defined(_MEMTRACKING)
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifdef QueryPerformanceFrequency
#undef QueryPerformanceFrequency
#endif
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)

#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

#include "platform/xbox/PlatformDefs.h"
#include "svn_rev.h"
