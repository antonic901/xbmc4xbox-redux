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
#include "utils/log.h"
#include "Application.h"
#include "interfaces/Builtins.h"
#include "Splash.h"
#include "input/KeyboardLayoutConfiguration.h"
#include "LangInfo.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/XKEEPROM.h"
#include "utils/LCD.h"
#include "xbox/IoSupport.h"
#include "xbox/XKHDD.h"
#endif
#include "xbox/xbeheader.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "TextureManager.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "PlayListPlayer.h"
#include "Autorun.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "utils/MemoryUnitManager.h"
#include "utils/FanController.h"
#include "utils/LED.h"
#endif
#include "XBVideoConfig.h"
#include "XBAudioConfig.h"
#include "utils/LangCodeExpander.h"
#include "GUIInfoManager.h"
#include "playlists/PlayListFactory.h"
#include "GUIFontManager.h"
#include "GUIColorManager.h"
#include "GUITextLayout.h"
#include "addons/Skin.h"
#include "libPython/XBPython.h"
#include "input/ButtonTranslator.h"
#include "GUIAudioManager.h"
#include "GUIPassword.h"
#include "ApplicationMessenger.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIUserMessages.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/MythSession.h"
#include "filesystem/PluginDirectory.h"
#ifdef HAS_FILESYSTEM_SAP
#include "filesystem/SAPDirectory.h"
#endif
#include "filesystem/HTSPDirectory.h"
#include "utils/TuxBoxUtil.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "GUILargeTextureManager.h"
#include "SmartPlaylist.h"
#include "filesystem/RarManager.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "utils/DownloadQueueManager.h"
#include "settings/SettingAddon.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SkinSettings.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"
#include "DatabaseManager.h"
#include "utils/RssManager.h"
#include "utils/Weather.h"
#include "view/ViewStateSettings.h"
#ifdef HAS_FILESYSTEM
#include "filesystem/DAAPFile.h"
#endif
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#include "filesystem/UPnPDirectory.h"
#endif
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_KARAOKE
#include "CdgParser.h"
#endif
#include "AudioContext.h"
#include "GUIFontTTF.h"
#include "threads/platform/win/Win32Exception.h"
#include "libGoAhead/XBMChttp.h"
#ifdef HAS_XFONT
#include <xfont.h>  // for textout functions
#endif
#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif
#include "network/NetworkServices.h"
#include "interfaces/AnnouncementManager.h"
#include "music/infoscanner/MusicInfoScanner.h"

// Windows includes
#include "GUIWindowManager.h"
#include "windows/GUIWindowHome.h"
#include "settings/windows/GUIWindowSettings.h"
#include "windows/GUIWindowFileManager.h"
#include "settings/windows/GUIWindowSettingsCategory.h"
#include "music/windows/GUIWindowMusicPlaylist.h"
#include "music/windows/GUIWindowMusicSongs.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "music/windows/GUIWindowMusicPlaylistEditor.h"
#include "video/windows/GUIWindowVideoPlaylist.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "profiles/windows/GUIWindowSettingsProfile.h"
#include "settings/windows/GUIWindowSettingsScreenCalibration.h"
#include "programs/GUIWindowPrograms.h"
#include "pictures/GUIWindowPictures.h"
#include "windows/GUIWindowWeather.h"
#include "GUIWindowGameSaves.h"
#include "windows/GUIWindowLoginScreen.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "music/windows/GUIWindowVisualisation.h"
#include "windows/GUIWindowSystemInfo.h"
#include "windows/GUIWindowScreensaver.h"
#include "pictures/GUIWindowSlideShow.h"
#include "windows/GUIWindowStartup.h"
#include "video/windows/GUIWindowFullScreen.h"
#include "video/dialogs/GUIDialogVideoOSD.h"

// Dialog includes
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "music/dialogs/GUIDialogVisualisationPresetList.h"
#include "dialogs/GUIDialogTrainerSettings.h"
#include "network/GUIDialogNetworkSetup.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "video/dialogs/GUIDialogVideoSettings.h"
#include "video/dialogs/GUIDialogAudioSubtitleSettings.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogTextViewer.h"

#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSeekBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "dialogs/GUIDialogMuteBug.h"
#include "video/dialogs/GUIDialogFileStacking.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "dialogs/GUIDialogFavourites.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogPlayerControls.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogSmartPlaylistRule.h"
#include "pictures/GUIDialogPictureInfo.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIDialogAddonInfo.h"
#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "dialogs/GUIDialogSlider.h"
#include "cores/dlgcache.h"
#include "guilib/GUIControlFactory.h"
#include "dialogs/GUIDialogMediaFilter.h"
#include "utils/XMLUtils.h"
#include "addons/AddonInstaller.h"
#include "utils/JobManager.h"
#include "utils/SaveFileStateJob.h"

#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/SkinSettings.h"
#include "view/ViewStateSettings.h"

#ifdef _LINUX
#include "XHandle.h"
#endif

using namespace std;
using namespace ADDON;
using namespace XFILE;
using namespace MEDIA_DETECT;
using namespace PLAYLIST;
using namespace VIDEO;
using namespace MUSIC_INFO;
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif
using namespace ANNOUNCEMENT;

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#ifdef HAS_LCD
#pragma comment (lib,"lib/libXenium/XeniumSPIg.lib")
#endif

#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
 #ifdef HAS_FILESYSTEM
  #pragma comment (lib,"lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
  #pragma comment (lib,"lib/libxdaap/libxdaapd.lib") // SECTIONNAME=LIBXDAAP
  #pragma comment (lib,"lib/libRTV/libRTVd.lib")    // SECTIONNAME=LIBRTV
 #endif
 #ifdef _XBOX
  #pragma comment (lib,"lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
  #pragma comment (lib,"lib/sqLite/libSQLite3d.lib")
  #pragma comment (lib,"lib/libshout/libshoutd.lib" )
  #pragma comment (lib,"lib/libcdio/libcdiod.lib" )
  #pragma comment (lib,"lib/libiconv/libiconvd.lib")
  #pragma comment (lib,"lib/libfribidi/libfribidid.lib")
  #pragma comment (lib,"lib/libpcre/libpcred.lib")
 #else
  #pragma comment (lib,"../../lib/libGoAhead/goahead_win32d.lib") // SECTIONNAME=LIBHTTP
  #pragma comment (lib,"../../lib/sqLite/libSQLite3_win32d.lib")
  #pragma comment (lib,"../../lib/libcdio/libcdio_win32d.lib" )
  #pragma comment (lib,"../../lib/libiconv/libiconvd.lib")
  #pragma comment (lib,"../../lib/libfribidi/libfribidid.lib")
  #pragma comment (lib,"../../lib/libpcre/libpcred.lib")
 #endif
 #ifdef HAS_MIKMOD
  #pragma comment (lib,"lib/mikxbox/mikxboxd.lib")  // SECTIONNAME=MOD_RW,MOD_RX
 #endif
#else
 #ifdef HAS_FILESYSTEM
  #pragma comment (lib,"lib/libsmb/libsmb.lib")
  #pragma comment (lib,"lib/libxdaap/libxdaap.lib") // SECTIONNAME=LIBXDAAP
  #pragma comment (lib,"lib/libRTV/libRTV.lib")
 #endif
 #ifdef _XBOX
  #pragma comment (lib,"lib/libGoAhead/goahead.lib")
  #pragma comment (lib,"lib/sqLite/libSQLite3.lib")
  #pragma comment (lib,"lib/libcdio/libcdio.lib")
  #pragma comment (lib,"lib/libiconv/libiconv.lib")
  #pragma comment (lib,"lib/libfribidi/libfribidi.lib")
  #pragma comment (lib,"lib/libpcre/libpcre.lib")
 #else
  #pragma comment (lib,"../../lib/libGoAhead/goahead_win32.lib")
  #pragma comment (lib,"../../lib/sqLite/libSQLite3_win32.lib")
  #pragma comment (lib,"../../lib/libshout/libshout_win32.lib" )
  #pragma comment (lib,"../../lib/libcdio/libcdio_win32.lib" )
  #pragma comment (lib,"../../lib/libiconv/libiconv.lib")
  #pragma comment (lib,"../../lib/libfribidi/libfribidi.lib")
  #pragma comment (lib,"../../lib/libpcre/libpcre.lib")
 #endif
 #ifdef HAS_MIKMOD
  #pragma comment (lib,"lib/mikxbox/mikxbox.lib")
 #endif
#endif

#define MAX_FFWD_SPEED 5

CStdString g_LoadErrorStr;

#ifdef HAS_XBOX_D3D
static void WaitCallback(DWORD flags)
{
#ifndef PROFILE
  /* if cpu is far ahead of gpu, sleep instead of yield */
  if( flags & D3DWAIT_PRESENT )
    while(D3DDevice::GetPushDistance(D3DDISTANCE_FENCES_TOWAIT) > 0)
      Sleep(1);
  else if( flags & (D3DWAIT_OBJECTLOCK | D3DWAIT_BLOCKONFENCE | D3DWAIT_BLOCKUNTILIDLE) )
    while(D3DDevice::GetPushDistance(D3DDISTANCE_FENCES_TOWAIT) > 1)
      Sleep(1);
#endif
}
#endif

//extern IDirectSoundRenderer* m_pAudioDecoder;
CApplication::CApplication(void)
  : m_ctrDpad(220, 220)
  , m_itemCurrentFile(new CFileItem)
  , m_stackFileItemToUpdate(new CFileItem)
  , m_progressTrackingItem(new CFileItem)
  , m_videoInfoScanner(new CVideoInfoScanner)
  , m_musicInfoScanner(new CMusicInfoScanner)
{
  m_network = NULL;
  m_iPlaySpeed = 1;
  m_bSpinDown = false;
  m_bNetworkSpinDown = false;
  m_dwSpinDownTime = timeGetTime();
  m_pXbmcHttp = NULL;
  m_prevMedia="";
  m_pPlayer = NULL;
#ifdef HAS_XBOX_HARDWARE
  XSetProcessQuantumLength(5); //default=20msec
  XSetFileCacheSize (256*1024); //default=64kb
#endif
  m_bScreenSave = false;   // CB: SCREENSAVER PATCH
  m_iScreenSaveLock = 0;
  m_bInitializing = true;
  m_eForcedNextPlayer = EPC_NONE;
  m_strPlayListFile = "";
  m_nextPlaylistItem = -1;
  m_bPlaybackStarting = false;
  m_skinReverting = false;

  //true while we in IsPaused mode! Workaround for OnPaused, which must be add. after v2.0
  m_bIsPaused = false;

  /* for now always keep this around */
#ifdef HAS_KARAOKE
  m_pCdgParser = new CCdgParser();
#endif
  m_currentStack = new CFileItemList;
  m_debugLayout = NULL;

  m_volumeLevel = 0;
  m_dynamicRangeCompressionLevel = 0;
  m_muted = false;
}

CApplication::~CApplication(void)
{
  delete m_currentStack;

  if (m_network != NULL)
     delete m_network;
}

// text out routine for below
#ifdef HAS_XFONT
static void __cdecl FEH_TextOut(XFONT* pFont, int iLine, const wchar_t* fmt, ...)
{
  wchar_t buf[100];
  va_list args;
  va_start(args, fmt);
  _vsnwprintf(buf, 100, fmt, args);
  va_end(args);

  if (!(iLine & 0x8000))
    CLog::Log(LOGFATAL, "%S", buf);

  bool Center = (iLine & 0x10000) > 0;
  pFont->SetTextAlignment(Center ? XFONT_TOP | XFONT_CENTER : XFONT_TOP | XFONT_LEFT);

  iLine &= 0x7fff;

  for (int i = 0; i < 2; i++)
  {
    D3DRECT rc = { 0, 50 + 25 * iLine, 720, 50 + 25 * (iLine + 1) };
    D3DDevice::Clear(1, &rc, D3DCLEAR_TARGET, 0, 0, 0);
    pFont->TextOut(g_application.m_pBackBuffer, buf, -1, Center ? 360 : 80, 50 + 25*iLine);
    D3DDevice::Present(0, 0, 0, 0);
  }
}
#else
static void __cdecl FEH_TextOut(void* pFont, int iLine, const wchar_t* fmt, ...) {}
#endif

HWND g_hWnd = NULL;

void CApplication::InitBasicD3D()
{
  bool bPal = g_videoConfig.HasPAL();
  CLog::Log(LOGINFO, "Init display in default mode: %s", bPal ? "PAL" : "NTSC");
  // init D3D with defaults (NTSC or PAL standard res)
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = bPal ? 576 : 480;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_X8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
#ifdef HAS_XBOX_D3D
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
#else
  m_d3dpp.FullScreen_PresentationInterval = 0;
  m_d3dpp.Windowed = TRUE;
  m_d3dpp.hDeviceWindow = g_hWnd;
#endif

  if (!(m_pD3D = Direct3DCreate8(D3D_SDK_VERSION)))
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create Direct3D!");
    Sleep(INFINITE); // die
  }

  // Check if we have the required modes available
  g_videoConfig.GetModes(m_pD3D);
  if (!g_graphicsContext.IsValidResolution(CDisplaySettings::Get().GetCurrentResolution()))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    CDisplaySettings::Get().SetCurrentResolution(g_videoConfig.GetSafeMode(), true);
    CLog::Log(LOGERROR, "Resetting to mode %s", CDisplaySettings::Get().GetCurrentResolutionInfo().strMode.c_str());
    CLog::Log(LOGERROR, "Done reset");
  }

  // Transfer the resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution(), TRUE);

  // Create the device
#ifdef HAS_XBOX_D3D
  // Xbox MUST use HAL / Hardware Vertex Processing!
  if (m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create D3D Device!");
    Sleep(INFINITE); // die
  }
  m_pd3dDevice->GetBackBuffer(0, 0, &m_pBackBuffer);
#else
  if (m_pD3D->CreateDevice(0, D3DDEVTYPE_REF, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create D3D Device!");
    Sleep(INFINITE); // die
  }
#endif

  if (m_splash)
  {
    m_splash->Stop();
  }

  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

// This function does not return!
void CApplication::FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork)
{
  // XBMC couldn't start for some reason...
  // g_LoadErrorStr should contain the reason
  CLog::Log(LOGWARNING, "Emergency recovery console starting...");

  bool HaveGamepad = true; // should always have the gamepad when we get here
  if (InitD3D)
    InitBasicD3D();

  if (m_splash)
  {
    m_splash->Stop();
  }

  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

  // D3D is up, load default font
#ifdef HAS_XFONT
  XFONT* pFont;
  if (XFONT_OpenDefaultFont(&pFont) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to open default font!");
    Sleep(INFINITE); // die
  }

  // defaults for text
  pFont->SetBkMode(XFONT_OPAQUE);
  pFont->SetBkColor(D3DCOLOR_XRGB(0, 0, 0));
  pFont->SetTextColor(D3DCOLOR_XRGB(0xff, 0x20, 0x20));
#else
  void *pFont = NULL;
#endif
  int iLine = 0;
  FEH_TextOut(pFont, iLine++, L"XBMC Fatal Error:");
  char buf[500];
  strncpy(buf, g_LoadErrorStr.c_str(), 500);
  buf[499] = 0;
  char* pNewline = strtok(buf, "\n");
  while (pNewline)
  {
    FEH_TextOut(pFont, iLine++, L"%S", pNewline);
    pNewline = strtok(NULL, "\n");
  }
  ++iLine;

#ifdef HAS_XBOX_HARDWARE
  if (MapDrives)
  {
    // map in default drives
    CIoSupport::RemapDriveLetter('C',"Harddisk0\\Partition2");
    CIoSupport::RemapDriveLetter('D',"Cdrom0");
    CIoSupport::RemapDriveLetter('E',"Harddisk0\\Partition1");

    //Add. also Drive F/G
    if (CIoSupport::PartitionExists(6)) 
      CIoSupport::RemapDriveLetter('F',"Harddisk0\\Partition6");
    if (CIoSupport::PartitionExists(7))
      CIoSupport::RemapDriveLetter('G',"Harddisk0\\Partition7");
  }
#endif
  bool Pal = g_graphicsContext.GetVideoResolution() == RES_PAL_4x3;

  if (HaveGamepad)
    FEH_TextOut(pFont, (Pal ? 16 : 12) | 0x18000, L"Press any button to reboot");

  bool NetworkUp = false;

  // Boot up the network for FTP
  if (InitNetwork)
  {
    std::vector<int> netorder;
    netorder.push_back(NETWORK_DASH);
    netorder.push_back(NETWORK_DHCP);
    netorder.push_back(NETWORK_STATIC);

    while(1)
    {
      std::vector<int>::iterator it;
      for( it = netorder.begin();it != netorder.end(); it++)
      {
        m_network->Deinitialize();

#ifdef HAS_XBOX_NETWORK
        if (!m_network->IsEthernetConnected())
        {
          FEH_TextOut(pFont, iLine, L"Network cable unplugged");
          break;
        }
#endif
        switch( (*it) )
        {
          case NETWORK_DASH:
            FEH_TextOut(pFont, iLine, L"Init network using dash settings...");
            m_network->Initialize(NETWORK_DASH, "","","","","");
            break;
          case NETWORK_DHCP:
            FEH_TextOut(pFont, iLine, L"Init network using DHCP...");
            m_network->Initialize(NETWORK_DHCP, "","","","","");
            break;
          default:
            FEH_TextOut(pFont, iLine, L"Init network using static ip...");
            m_network->Initialize(NETWORK_STATIC,
                  "192.168.0.42",
                  "255.255.255.0",
                  "192.168.0.1",
                  "192.168.0.1",
                  "0.0.0.0" );
            break;
        }

#ifdef HAS_XBOX_NETWORK
        DWORD dwState = XNET_GET_XNADDR_PENDING;

        while (dwState == XNET_GET_XNADDR_PENDING)
        {
          dwState = m_network->UpdateState();

          if (HaveGamepad && AnyButtonDown())
            CApplicationMessenger::Get().Restart();

          Sleep(50);
        }

        if ((dwState & XNET_GET_XNADDR_DHCP || dwState & XNET_GET_XNADDR_STATIC) && !(dwState & XNET_GET_XNADDR_NONE || dwState & XNET_GET_XNADDR_TROUBLESHOOT))
        {
          /* yay, we got network */
          NetworkUp = true;
          break;
        }
#endif
        /* increment line before next attempt */
        ++iLine;
      }

      /* break out of the continous loop if we have network*/
      if( NetworkUp )
        break;
      else
      {
        int n = 10;
        while (n)
        {
          FEH_TextOut(pFont, (iLine + 1) | 0x8000, L"Unable to init network, retrying in %d seconds", n--);
          for (int i = 0; i < 20; ++i)
          {
            Sleep(50);

            if (HaveGamepad && AnyButtonDown())
              CApplicationMessenger::Get().Restart();
          }
        }
      }
    }
  }

  if( NetworkUp )
  {
    FEH_TextOut(pFont, iLine++, L"IP Address: %S", m_network->m_networkinfo.ip);
    ++iLine;
  }

  if (NetworkUp)
  {
#ifdef HAS_FTP_SERVER
    // Start FTP with default settings
    FEH_TextOut(pFont, iLine++, L"Starting FTP server...");
    CNetworkServices::Get().StartFtpEmergencyRecoveryMode();
    FEH_TextOut(pFont, iLine++, L"FTP server running on port %d, login: xbox/xbox", CNetworkServices::Get().GetFtpServerPort());
#endif
    ++iLine;
  }

  if (HaveGamepad)
  {
    for (;;)
    {
      Sleep(50);
      if (AnyButtonDown())
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKUtils::XBOXPowerCycle();
#endif
#else
        SendMessage(g_hWnd, WM_CLOSE, 0, 0);
#endif
      }
    }
  }
  else
  {
#ifdef _XBOX
    Sleep(INFINITE);
#else
    SendMessage(g_hWnd, WM_CLOSE, 0, 0);
#endif
  }
}

LONG WINAPI CApplication::UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
  PCSTR pExceptionString = "Unknown exception code";

#define STRINGIFY_EXCEPTION(code) case code: pExceptionString = #code; break

  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    STRINGIFY_EXCEPTION(EXCEPTION_ACCESS_VIOLATION);
    STRINGIFY_EXCEPTION(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    STRINGIFY_EXCEPTION(EXCEPTION_BREAKPOINT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DENORMAL_OPERAND);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INEXACT_RESULT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INVALID_OPERATION);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_STACK_CHECK);
    STRINGIFY_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_INVALID_DISPOSITION);
    STRINGIFY_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    STRINGIFY_EXCEPTION(EXCEPTION_SINGLE_STEP);
  }
#undef STRINGIFY_EXCEPTION

  g_LoadErrorStr.Format("%s (0x%08x)\n at 0x%08x",
                        pExceptionString, ExceptionInfo->ExceptionRecord->ExceptionCode,
                        ExceptionInfo->ExceptionRecord->ExceptionAddress);

  CLog::Log(LOGFATAL, "%s", g_LoadErrorStr.c_str());

  return ExceptionInfo->ExceptionRecord->ExceptionCode;
}

#ifdef _XBOX
#include "xbox/Undocumented.h"
extern "C" HANDLE __stdcall KeGetCurrentThread(VOID);
#endif
extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();

HRESULT CApplication::Create(HWND hWnd)
{
#if defined(HAS_LINUX_NETWORK)
  m_network = new CNetworkLinux();
#elif defined(HAS_WIN32_NETWORK)
  m_network = new CNetworkWin32();
#else
  m_network = new CNetwork();
#endif

#ifdef HAS_XBOX_HARDWARE
  // better 128mb ram support
  // set MTRRDefType memory type to write-back as done in other XBox apps - seems a bit of a hack as really the def type
  // should be uncachable and the mtrr/mask for ram instead set up for 128MB with writeback as is done in cromwell.
  m_128MBHack = false;
  MEMORYSTATUS status;
  GlobalMemoryStatus( &status );
  // if we have more than 64MB free
  if( status.dwTotalPhys > 67108864 )
  {
    __asm
    {
      mov ecx, 0x2ff
      rdmsr
      mov al, 0x06
      wrmsr
    }
    m_128MBHack = true;
  }
  g_advancedSettings.m_guiKeepInMemory = m_128MBHack;
#endif

  for (int i = RES_HDTV_1080i; i <= RES_PAL60_16x9; i++)
  {
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan((RESOLUTION)i, CDisplaySettings::Get().GetResolutionInfo(i).Overscan);
  }

  g_hWnd = hWnd;

  HRESULT hr;
  //grab a handle to our thread to be used later in identifying the render thread
  m_threadID = GetCurrentThreadId();

  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  CStdString strExecutablePath;
  char szDevicePath[MAX_PATH];

  // map Q to home drive of xbe to load the config file
  CUtil::GetHomePath(strExecutablePath);
  CIoSupport::GetPartition(strExecutablePath.c_str()[0], szDevicePath);
  strcat(szDevicePath, &strExecutablePath.c_str()[2]);
  CIoSupport::RemapDriveLetter('Q', szDevicePath);

  // Do all the special:// & driveletter mapping
  InitDirectoriesXbox();

  if (!CLog::Init(CSpecialProtocol::TranslatePath(g_advancedSettings.m_logFolder).c_str()))
  {
    fprintf(stderr,"Could not init logging classes. Permission errors on ~/.xbmc (%s)\n",
      CSpecialProtocol::TranslatePath(g_advancedSettings.m_logFolder).c_str());
    return false;
  }

  init_emu_environ();

  CProfilesManager::Get().Load();

  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();

  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
  CLog::Log(LOGNOTICE, "Starting XBMC4Xbox %s (SVN:%s, compiler %i). Built on %s ", VERSION_STRING, SVN_REV, _MSC_VER, __DATE__);
  CSpecialProtocol::LogPaths();

  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  CLog::Log(LOGNOTICE, "The executable running is: %s", szXBEFileName);
  CLog::Log(LOGNOTICE, "Log File is located: %sxbmc.log", g_advancedSettings.m_logFolder.c_str());
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

  // if we are running from DVD our UserData location will be TDATA
  if (URIUtils::IsDVD(strExecutablePath))
  {
    // TODO: Should we copy over any UserData folder from the DVD?
    if (!CFile::Exists("special://masterprofile/guisettings.xml")) // first run - cache userdata folder
    {
      CFileItemList items;
      CUtil::GetRecursiveListing("special://xbmc/userdata",items,"");
      for (int i=0;i<items.Size();++i)
          CFile::Copy(items[i]->GetPath(),"special://masterprofile/"+URIUtils::GetFileName(items[i]->GetPath()));
    }
    g_advancedSettings.m_logFolder = "special://masterprofile/";
  }
  else
  {
    CStdString strMnt = CSpecialProtocol::TranslatePath(CProfilesManager::Get().GetUserDataFolder());
    if (strMnt.Left(2).Equals("Q:"))
    {
      CUtil::GetHomePath(strMnt);
      strMnt += CSpecialProtocol::TranslatePath(CProfilesManager::Get().GetUserDataFolder()).substr(2);
    }

    CIoSupport::GetPartition(strMnt.c_str()[0], szDevicePath);
    strcat(szDevicePath, &strMnt.c_str()[2]);
    CIoSupport::RemapDriveLetter('T', szDevicePath);
  }

  if (m_128MBHack)
    CLog::Log(LOGNOTICE, "128MB hack enabled");

  CLog::Log(LOGNOTICE, "Setup DirectX");
  // Create the Direct3D object
  if ( NULL == ( m_pD3D = Direct3DCreate8(D3D_SDK_VERSION) ) )
  {
    CLog::Log(LOGFATAL, "XBAppEx: Unable to create Direct3D!" );
    return E_FAIL;
  }

  //list available videomodes
  g_videoConfig.GetModes(m_pD3D);
  //init the present parameters with values that are supported
  RESOLUTION initialResolution = g_videoConfig.GetInitialMode(m_pD3D, &m_d3dpp);
  // Transfer the resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetVideoResolution(initialResolution, TRUE);

  // Initialize core peripheral port support. Note: If these parameters
  // are 0 and NULL, respectively, then the default number and types of
  // controllers will be initialized.
#ifdef HAS_XBOX_HARDWARE
  XInitDevices( m_dwNumInputDeviceTypes, m_InputDeviceTypes );

  // Create the gamepad devices
  if ( FAILED(hr = XBInput_CreateGamepads(&m_Gamepad)) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to CreateGamepads() failed!" );
    return hr;
  }

  if ( FAILED(hr = XBInput_CreateIR_Remotes()) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to CreateIRRemotes() failed!" );
    return hr;
  }
#endif

  // Create the Mouse and Keyboard devices
  g_Mouse.Initialize(&hWnd);
  g_Keyboard.Initialize(hWnd);

#ifdef HAS_XBOX_HARDWARE
  // Wait for controller polling to finish. in an elegant way, instead of a Sleep(1000)
  while (XGetDeviceEnumerationStatus() == XDEVICE_ENUMERATION_BUSY)
  {
    ReadInput();
  }
  Sleep(10); // needed or the readinput doesnt fetch anything
  ReadInput();
#endif
#ifdef HAS_GAMEPAD
  //Check for LTHUMBCLICK+RTHUMBCLICK and BLACK+WHITE, no LTRIGGER+RTRIGGER
  if (((m_DefaultGamepad.wButtons & (XINPUT_GAMEPAD_LEFT_THUMB + XINPUT_GAMEPAD_RIGHT_THUMB)) && !(m_DefaultGamepad.wButtons & (KEY_BUTTON_LEFT_TRIGGER+KEY_BUTTON_RIGHT_TRIGGER))) ||
      ((m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE]) && !(m_DefaultGamepad.wButtons & KEY_BUTTON_LEFT_TRIGGER+KEY_BUTTON_RIGHT_TRIGGER)))
  {
    CLog::Log(LOGINFO, "Key combination detected for userdata deletion (LTHUMB+RTHUMB or BLACK+WHITE)");
    InitBasicD3D();
    // D3D is up, load default font
    XFONT* pFont;
    if (XFONT_OpenDefaultFont(&pFont) != S_OK)
    {
      CLog::Log(LOGFATAL, "FATAL ERROR: Unable to open default font!");
      Sleep(INFINITE); // die
    }
    // defaults for text
    pFont->SetBkMode(XFONT_OPAQUE);
    pFont->SetBkColor(D3DCOLOR_XRGB(0, 0, 0));
    pFont->SetTextColor(D3DCOLOR_XRGB(0xff, 0x20, 0x20));
    int iLine = 0;
    FEH_TextOut(pFont, iLine++, L"Key combination for userdata deletion detected!");
    FEH_TextOut(pFont, iLine++, L"Are you sure you want to proceed?");
    iLine++;
    FEH_TextOut(pFont, iLine++, L"A for yes, any other key for no");
    bool bAnyAnalogKey = false;
    while (m_DefaultGamepad.wPressedButtons != XBGAMEPAD_NONE) // wait for user to let go of lclick + rclick
    {
      ReadInput();
    }
    while (m_DefaultGamepad.wPressedButtons == XBGAMEPAD_NONE && !bAnyAnalogKey)
    {
      ReadInput();
      bAnyAnalogKey = m_DefaultGamepad.bPressedAnalogButtons[0] || m_DefaultGamepad.bPressedAnalogButtons[1] || m_DefaultGamepad.bPressedAnalogButtons[2] || m_DefaultGamepad.bPressedAnalogButtons[3] || m_DefaultGamepad.bPressedAnalogButtons[4] || m_DefaultGamepad.bPressedAnalogButtons[5] || m_DefaultGamepad.bPressedAnalogButtons[6] || m_DefaultGamepad.bPressedAnalogButtons[7];
    }
    if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
    {
      CUtil::DeleteGUISettings();
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"database\\"));
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"thumbnails\\"));
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"playlists\\"));
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"cache\\"));
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"profiles\\"));
      CUtil::WipeDir(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"visualisations\\"));
      CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"avpacksettings.xml"));
      // delete all profiles
      for (size_t i = 0; i < CProfilesManager::Get().GetNumberOfProfiles(); ++i)
        CProfilesManager::Get().DeleteProfile(i);

      CProfilesManager::Get().Save();

      char szXBEFileName[1024];

      CIoSupport::GetXbePath(szXBEFileName);
      CUtil::RunXBE(szXBEFileName);
    }
    m_pd3dDevice->Release();
  }
#endif

  CIoSupport::RemapDriveLetter('C', "Harddisk0\\Partition2");
  CIoSupport::RemapDriveLetter('E', "Harddisk0\\Partition1");

  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");

  // Attempt to read the LBA48 v3 patch partition table, if kernel supports the command and it exists.
  CIoSupport::ReadPartitionTable();
  if (CIoSupport::HasPartitionTable())
  {
    // Mount up to Partition15 if they are available.
    for (int i=EXTEND_PARTITION_BEGIN; i <= (EXTEND_PARTITION_BEGIN+EXTEND_PARTITIONS_LIMIT-1); i++)
    {
      char szDevice[32];
      if (CIoSupport::PartitionExists(i))
      {
        char cDriveLetter = 'A' + i - 1;
        
        char extendDriveLetter = CIoSupport::GetExtendedPartitionDriveLetter(cDriveLetter-EXTEND_DRIVE_BEGIN);
        CLog::Log(LOGNOTICE, "  map extended drive %c:", extendDriveLetter);
		
        sprintf(szDevice, "Harddisk0\\Partition%u", i);

        CIoSupport::RemapDriveLetter(extendDriveLetter, szDevice);
      }
    }
  }
  else
  {
    if (CIoSupport::DriveExists('F'))
      CIoSupport::RemapDriveLetter('F', "Harddisk0\\Partition6");
    if (CIoSupport::DriveExists('G'))
      CIoSupport::RemapDriveLetter('G', "Harddisk0\\Partition7");
  }

  CIoSupport::RemapDriveLetter('X',"Harddisk0\\Partition3");
  CIoSupport::RemapDriveLetter('Y',"Harddisk0\\Partition4");
#ifdef HAS_XBOX_HARDWARE
  CIoSupport::RemapDriveLetter('Z',"Harddisk0\\Partition5");
#endif

  CLog::Log(LOGINFO, "Drives are mapped");

  // Initialize default Settings - don't move
  CLog::Log(LOGNOTICE, "load settings...");
  if (!CSettings::Get().Initialize())
    return false;

  g_LoadErrorStr = "Unable to load settings";
  
  // load the actual values
  if (!CSettings::Get().Load())
    FatalErrorHandler(true, true, true);
  CSettings::Get().SetLoaded();

  update_emu_environ();//apply the GUI settings

  // initialize the addon database (must be before the addon manager is init'd)
  CDatabaseManager::Get().Initialize(true);

  // start-up Addons Framework
  // currently bails out if either cpluff Dll is unavailable or system dir can not be scanned
  if (!CAddonMgr::Get().Init())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to start CAddonMgr");
    FatalErrorHandler(true, true, true);
  }

  // set logging from debug add-on
  AddonPtr addon;
  CAddonMgr::Get().GetAddon("xbmc.debug", addon);
  if (addon)
    g_advancedSettings.SetExtraLogsFromAddon(addon.get());

  // Check for WHITE + Y for forced Error Handler (to recover if something screwy happens)
#ifdef HAS_GAMEPAD
  if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    g_LoadErrorStr = "Key code detected for Error Recovery mode";
    FatalErrorHandler(true, true, true);
  }
#endif

  //Check for X+Y - if pressed, set debug log mode and mplayer debuging on
  CheckForDebugButtonCombo();

#ifdef HAS_XBOX_HARDWARE
  bool bNeedReboot = false;
  char temp[1024];
  CIoSupport::GetXbePath(temp);
  char temp2[1024];
  char temp3;
  temp3 = temp[0];
  CIoSupport::GetPartition(temp3,temp2);
  CStdString strTemp(temp+2);
  int iLastSlash = strTemp.rfind('\\');
  strcat(temp2,strTemp.substr(0,iLastSlash).c_str());
  F_VIDEO ForceVideo = VIDEO_NULL;
  F_COUNTRY ForceCountry = COUNTRY_NULL;

  if (CUtil::RemoveTrainer())
    bNeedReboot = true;

// now check if we are switching video modes. if, are we in the wrong mode according to eeprom?
  if (CSettings::Get().GetBool("myprograms.gameautoregion"))
  {
    bool fDoPatchTest = false;

    // should use xkeeprom.h :/
    EEPROMDATA EEPROM;
    ZeroMemory(&EEPROM, sizeof(EEPROMDATA));

    if( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&EEPROM))
    {
      DWORD DWVideo = *(LPDWORD)(&EEPROM.VideoStandard[0]);
      char temp[1024];
      CIoSupport::GetXbePath(temp);
      char temp2[1024];
      char temp3;
      temp3 = temp[0];
      CIoSupport::GetPartition(temp3,temp2);
      CStdString strTemp(temp+2);
      int iLastSlash = strTemp.rfind('\\');
      strcat(temp2,strTemp.substr(0,iLastSlash).c_str());

      if ((DWVideo == XKEEPROM::VIDEO_STANDARD::NTSC_M) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) || (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_J) || initialResolution > 5))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to NTSC_M", (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) ? "PAL" : "NTSC_J");
        ForceVideo = VIDEO_NTSCM;
        ForceCountry = COUNTRY_USA;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else if ((DWVideo == XKEEPROM::VIDEO_STANDARD::PAL_I) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) || (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_J) || initialResolution < 6))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to PAL_I", (XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) ? "NTSC_M" : "NTSC_J");
        ForceVideo = VIDEO_PAL50;
        ForceCountry = COUNTRY_EUR;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else if ((DWVideo == XKEEPROM::VIDEO_STANDARD::NTSC_J) && ((XGetVideoStandard() == XC_VIDEO_STANDARD_NTSC_M) || (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) || initialResolution > 5))
      {
        CLog::Log(LOGINFO, "Rebooting to change resolution from %s back to NTSC_J", (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) ? "PAL" : "NTSC_M");
        ForceVideo = VIDEO_NTSCJ;
        ForceCountry = COUNTRY_JAP;
        bNeedReboot = true;
        fDoPatchTest = true;
      }
      else
        CUtil::RemoveKernelPatch(); // This removes the Resolution patch from the kernel if it is not needed (if actual resolution matches eeprom setting)

      if (fDoPatchTest) // Is set if we have to test whether our patch is in the kernel & therefore responsible for the mismatch of resolution & eeprom setting
      {
        if (!CUtil::LookForKernelPatch()) // If our patch is not present we are not responsible for the mismatch of current resolution & eeprom setting
        {
          // We do a hard reset to come back to default resolution and avoid infinite reboots
          CLog::Log(LOGINFO, "No infinite reboot loop...");
          CApplicationMessenger::Get().Reset();
        }
      }
    }
  }

  if (bNeedReboot)
  {
    Destroy();
    CUtil::LaunchXbe(temp2,("D:\\"+strTemp.substr(iLastSlash+1)).c_str(),NULL,ForceVideo,ForceCountry);
  }
#endif

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLanguage = CSettings::Get().GetString("locale.language");
  strLanguage[0] = toupper(strLanguage[0]);

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language info file:%s", strLangInfoPath.c_str());
  g_langInfo.Load(strLangInfoPath);

  CStdString strKeyboardLayoutConfigurationPath;
  strKeyboardLayoutConfigurationPath.Format("Q:\\language\\%s\\keyboardmap.xml", CSettings::Get().GetString("locale.language"));
  CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
  g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);

  CStdString strLanguagePath = "special://xbmc/language/";

  CLog::Log(LOGINFO, "load %s language file, from path: %s", strLanguage.c_str(), strLanguagePath.c_str());
  if (!g_localizeStrings.Load(strLanguagePath, strLanguage))
    FatalErrorHandler(true, false, true);

  CLog::Log(LOGINFO, "load keymapping");
  if (!CButtonTranslator::GetInstance().Load())
    FatalErrorHandler(true, false, true);

  // Retrieve the matching resolution based on GUI settings
  CDisplaySettings::Get().SetCurrentResolution(CDisplaySettings::Get().GetDisplayResolution());
  CLog::Log(LOGNOTICE, "Checking resolution %i", CDisplaySettings::Get().GetCurrentResolution());
  if (!g_graphicsContext.IsValidResolution(CDisplaySettings::Get().GetCurrentResolution()))
  {
    #ifdef _XBOX
        RESOLUTION newRes = g_videoConfig.GetBestMode();
    #else
        RESOLUTION newRes = g_videoConfig.GetSafeMode();
    #endif
    CLog::Log(LOGNOTICE, "Setting safe mode %i", newRes);
    CDisplaySettings::Get().SetCurrentResolution(newRes, true);
  }

  // Transfer the new resolution information to our graphics context
#ifndef HAS_XBOX_D3D
  m_d3dpp.Windowed = TRUE;
  m_d3dpp.hDeviceWindow = g_hWnd;
#else
#define D3DCREATE_MULTITHREADED 0
#endif

  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution(), TRUE);
  
  if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                                         D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                         &m_d3dpp, &m_pd3dDevice ) ) )
  {
    // try software vertex processing
    if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                                          D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                          &m_d3dpp, &m_pd3dDevice ) ) )
    {
      // and slow as arse reference processing
      if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_REF, NULL,
                                            D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                            &m_d3dpp, &m_pd3dDevice ) ) )
      {

        CLog::Log(LOGFATAL, "XBAppEx: Could not create D3D device!" );
        CLog::Log(LOGFATAL, " width/height:(%ix%i)" , m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
        CLog::Log(LOGFATAL, " refreshrate:%i" , m_d3dpp.FullScreen_RefreshRateInHz);
        if (m_d3dpp.Flags & D3DPRESENTFLAG_WIDESCREEN)
          CLog::Log(LOGFATAL, " 16:9 widescreen");
        else
          CLog::Log(LOGFATAL, " 4:3");

        if (m_d3dpp.Flags & D3DPRESENTFLAG_INTERLACED)
          CLog::Log(LOGFATAL, " interlaced");
        if (m_d3dpp.Flags & D3DPRESENTFLAG_PROGRESSIVE)
          CLog::Log(LOGFATAL, " progressive");
        return hr;
      }
    }
  }

  g_graphicsContext.SetD3DDevice(m_pd3dDevice);
  g_graphicsContext.CaptureStateBlock();
  // set filters
  g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_settings.m_minFilter*/ );
  g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_settings.m_maxFilter*/ );
  CUtil::InitGamma();
  
  // set GUI res and force the clear of the screen
  g_graphicsContext.SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution(), TRUE, true);

  if (g_advancedSettings.m_splashImage)
  {
    CStdString strUserSplash = "special://home/media/Splash.png";
    if (CFile::Exists(strUserSplash))
    {
      CLog::Log(LOGINFO, "load user splash image: %s", CSpecialProtocol::TranslatePath(strUserSplash).c_str());
      m_splash = new CSplash(strUserSplash);
    }
    else
    {
      CLog::Log(LOGINFO, "load default splash image: %s", CSpecialProtocol::TranslatePath("special://xbmc/media/Splash.png").c_str());
      m_splash = new CSplash("special://xbmc/media/Splash.png");
    }
    m_splash->Start();
  }

  int iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGINFO, "GUI format %ix%i %s",
            CDisplaySettings::Get().GetResolutionInfo(iResolution).iWidth,
            CDisplaySettings::Get().GetResolutionInfo(iResolution).iHeight,
            CDisplaySettings::Get().GetResolutionInfo(iResolution).strMode.c_str());
  g_windowManager.Initialize();

  // show recovery console on fatal error instead of freezing
  CLog::Log(LOGINFO, "install unhandled exception filter");
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);

#ifdef HAS_XBOX_D3D
  D3DDevice::SetWaitCallback(WaitCallback);
#endif

  g_Mouse.SetEnabled(CSettings::Get().GetBool("input.enablemouse"));

  CUtil::InitRandomSeed();

  return CXBApplicationEx::Create(hWnd);
}


HRESULT CApplication::Initialize()
{
  CLog::Log(LOGINFO, "creating subdirectories");

  CLog::Log(LOGINFO, "userdata folder: %s", CProfilesManager::Get().GetProfileUserDataFolder().c_str());
  CLog::Log(LOGINFO, "recording folder: %s", CSettings::Get().GetString("audiocds.recordingpath").c_str());
  CLog::Log(LOGINFO, "screenshots folder: %s", CSettings::Get().GetString("debug.screenshotpath").c_str());

  // UserData folder layout:
  // UserData/
  //   Database/
  //     CDDb/
  //   Thumbnails/
  //     Music/
  //       temp/
  //     0 .. F/

  CDirectory::Create(CProfilesManager::Get().GetUserDataFolder());
  CDirectory::Create(CProfilesManager::Get().GetProfileUserDataFolder());
  CProfilesManager::Get().CreateProfileFolders();

  CDirectory::Create(CProfilesManager::Get().GetProfilesThumbFolder());

  CDirectory::Create("special://home/addons");
  CDirectory::Create("special://home/addons/packages");
  CUtil::WipeDir("special://temp/");
  CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA

  CreateDirectory("Q:\\language", NULL);

  /* setup network based on our settings */
  /* network will start it's init procedure */
  if(m_network->SetupNetwork())
    m_network->WaitForSetup();

  // initialize (and update as needed) our databases
  CDatabaseManager::Get().Initialize();

  StartServices();

  g_windowManager.Add(new CGUIWindowHome);                     // window id = 0

  if (!LoadSkin(CSettings::Get().GetString("lookandfeel.skin")))
    LoadSkin(DEFAULT_SKIN);

  g_windowManager.Add(new CGUIWindowPrograms);                 // window id = 1
  g_windowManager.Add(new CGUIWindowPictures);                 // window id = 2
  g_windowManager.Add(new CGUIWindowFileManager);      // window id = 3
  g_windowManager.Add(new CGUIWindowSettings);                 // window id = 4
  g_windowManager.Add(new CGUIWindowSystemInfo);               // window id = 7
  g_windowManager.Add(new CGUIWindowSettingsScreenCalibration); // window id = 11
  g_windowManager.Add(new CGUIWindowSettingsCategory);         // window id = 12 slideshow:window id 2007
  g_windowManager.Add(new CGUIWindowVideoNav);                 // window id = 36
  g_windowManager.Add(new CGUIWindowVideoPlaylist);            // window id = 28
  g_windowManager.Add(new CGUIWindowLoginScreen);            // window id = 29
  g_windowManager.Add(new CGUIWindowSettingsProfile);          // window id = 34
  g_windowManager.Add(new CGUIWindow(WINDOW_SKIN_SETTINGS, "SkinSettings.xml"));
  g_windowManager.Add(new CGUIWindowAddonBrowser);          // window id = 40
  g_windowManager.Add(new CGUIWindowGameSaves);               // window id = 35
  g_windowManager.Add(new CGUIDialogYesNo);              // window id = 100
  g_windowManager.Add(new CGUIDialogProgress);           // window id = 101
  g_windowManager.Add(new CGUIDialogExtendedProgressBar);     // window id = 148
  g_windowManager.Add(new CGUIDialogKeyboardGeneric);           // window id = 103
  g_windowManager.Add(new CGUIDialogVolumeBar);          // window id = 104
  g_windowManager.Add(new CGUIDialogSeekBar);            // window id = 115
  g_windowManager.Add(new CGUIDialogSubMenu);            // window id = 105
  g_windowManager.Add(new CGUIDialogContextMenu);        // window id = 106
  g_windowManager.Add(new CGUIDialogKaiToast);           // window id = 107
  g_windowManager.Add(new CGUIDialogNumeric);            // window id = 109
  g_windowManager.Add(new CGUIDialogGamepad);            // window id = 110
  g_windowManager.Add(new CGUIDialogButtonMenu);         // window id = 111
  g_windowManager.Add(new CGUIDialogMuteBug);            // window id = 113
  g_windowManager.Add(new CGUIDialogPlayerControls);     // window id = 114
  g_windowManager.Add(new CGUIDialogSlider);             // window id = 145
  g_windowManager.Add(new CGUIDialogMusicOSD);           // window id = 120
  g_windowManager.Add(new CGUIDialogVisualisationPresetList);   // window id = 122
  g_windowManager.Add(new CGUIDialogVideoSettings);             // window id = 123
  g_windowManager.Add(new CGUIDialogAudioSubtitleSettings);     // window id = 124
  g_windowManager.Add(new CGUIDialogVideoBookmarks);      // window id = 125
  // Don't add the filebrowser dialog - it's created and added when it's needed
  g_windowManager.Add(new CGUIDialogTrainerSettings);  // window id = 127
  g_windowManager.Add(new CGUIDialogNetworkSetup);  // window id = 128
  g_windowManager.Add(new CGUIDialogMediaSource);   // window id = 129
  g_windowManager.Add(new CGUIDialogProfileSettings); // window id = 130
  g_windowManager.Add(new CGUIDialogFavourites);     // window id = 134
  g_windowManager.Add(new CGUIDialogSongInfo);       // window id = 135
  g_windowManager.Add(new CGUIDialogSmartPlaylistEditor);       // window id = 136
  g_windowManager.Add(new CGUIDialogSmartPlaylistRule);       // window id = 137
  g_windowManager.Add(new CGUIDialogBusy);      // window id = 138
  g_windowManager.Add(new CGUIDialogPictureInfo);      // window id = 139
  g_windowManager.Add(new CGUIDialogAddonInfo);
  g_windowManager.Add(new CGUIDialogAddonSettings);      // window id = 140
  g_windowManager.Add(new CGUIDialogTextViewer);              // window id = 147

  g_windowManager.Add(new CGUIDialogLockSettings); // window id = 131

  g_windowManager.Add(new CGUIDialogContentSettings);        // window id = 132

  g_windowManager.Add(new CGUIDialogMediaFilter);   // window id = 151

  g_windowManager.Add(new CGUIWindowMusicPlayList);          // window id = 500
  g_windowManager.Add(new CGUIWindowMusicSongs);             // window id = 501
  g_windowManager.Add(new CGUIWindowMusicNav);               // window id = 502
  g_windowManager.Add(new CGUIWindowMusicPlaylistEditor);    // window id = 503

  g_windowManager.Add(new CGUIDialogSelect);             // window id = 2000
  g_windowManager.Add(new CGUIDialogMusicInfo);                // window id = 2001
  g_windowManager.Add(new CGUIDialogOK);                 // window id = 2002
  g_windowManager.Add(new CGUIDialogVideoInfo);                // window id = 2003
  g_windowManager.Add(new CGUIWindowFullScreen);         // window id = 2005
  g_windowManager.Add(new CGUIWindowVisualisation);      // window id = 2006
  g_windowManager.Add(new CGUIWindowSlideShow);          // window id = 2007
  g_windowManager.Add(new CGUIDialogFileStacking);       // window id = 2008

  g_windowManager.Add(new CGUIDialogVideoOSD);                // window id = 2901
  g_windowManager.Add(new CGUIWindowScreensaver);        // window id = 2900 Screensaver
  g_windowManager.Add(new CGUIWindowWeather);            // window id = 2600 WEATHER
  g_windowManager.Add(new CGUIWindowStartup);            // startup window (id 2999)

  /* window id's 3000 - 3100 are reserved for python */
  g_DownloadManager.Initialize();

  m_ctrDpad.SetDelays(100, 500); //g_settings.m_iMoveDelayController, g_settings.m_iRepeatDelayController);

  SAFE_DELETE(m_splash);

  if (CSettings::Get().GetBool("masterlock.startuplock") && 
      CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
     !CProfilesManager::Get().GetMasterProfile().getLockCode().IsEmpty())
  {
     g_passwordManager.CheckStartUpLock();
  }

  // check if we should use the login screen
  if (CProfilesManager::Get().UsingLoginScreen())
  {
    g_windowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
  }
  else
  {
    g_windowManager.ActivateWindow(g_SkinInfo->GetFirstWindow());
  }

  //g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  //  Show mute symbol
  if (m_muted)
    Mute();
  SetVolume(m_volumeLevel, false);

  if (!CProfilesManager::Get().UsingLoginScreen())
  {
    UpdateLibraries();
#ifdef HAS_PYTHON
  g_pythonParser.m_bLogin = true;
#endif
  }

  m_slowTimer.StartZero();

#ifdef __APPLE__
  g_xbmcHelper.CaptureAllInput();
#endif
  CAddonMgr::Get().StartServices(false);

  CLog::Log(LOGNOTICE, "initialize done");

  m_bInitializing = false;

  // final check for debugging combo
  CheckForDebugButtonCombo();

  // reset our screensaver (starts timers etc.)
  ResetScreenSaver();
  return S_OK;
}

void CApplication::PrintXBEToLCD(const char* xbePath)
{
#ifdef HAS_LCD
  int pLine = 0;
  CStdString strXBEName;
  if (!CUtil::GetXBEDescription(xbePath, strXBEName))
  {
    CUtil::GetDirectoryName(xbePath, strXBEName);
    CUtil::ShortenFileName(strXBEName);
    CUtil::RemoveIllegalChars(strXBEName);
  }
  // crop to LCD screen size
  if ((int)strXBEName.size() > g_advancedSettings.m_lcdColumns)
    strXBEName = strXBEName.Left(g_advancedSettings.m_lcdColumns);
  if (g_lcd)
  {
    g_infoManager.SetLaunchingXBEName(strXBEName);
    g_lcd->Render(ILCD::LCD_MODE_XBE_LAUNCH);
  }
#endif
}

void CApplication::StartIdleThread()
{
  m_idleThread.Create(false, 0x100);
}

void CApplication::StopIdleThread()
{
  m_idleThread.StopThread();
}

void CApplication::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (CSettings::Get().GetBool("services.esenabled"))
  {
    CEventServer::GetInstance()->RefreshSettings();
  }
#endif
}

void CApplication::StartLEDControl(bool switchoff)
{
#ifdef HAS_XBOX_HARDWARE
  if (switchoff && CSettings::Get().GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE)
  {
    if ( IsPlayingVideo() && (CSettings::Get().GetInt("system.leddisableonplayback") == LED_PLAYBACK_VIDEO))
      ILED::CLEDControl(LED_COLOUR_OFF);
    if ( IsPlayingAudio() && (CSettings::Get().GetInt("system.leddisableonplayback") == LED_PLAYBACK_MUSIC))
      ILED::CLEDControl(LED_COLOUR_OFF);
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && (CSettings::Get().GetInt("system.leddisableonplayback") == LED_PLAYBACK_VIDEO_MUSIC))
      ILED::CLEDControl(LED_COLOUR_OFF);
  }
  else if (!switchoff)
    ILED::CLEDControl(CSettings::Get().GetInt("system.ledcolour"));
#endif
}

void CApplication::DimLCDOnPlayback(bool dim)
{
#ifdef HAS_LCD
  if(g_lcd && dim && (CSettings::Get().GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF) && (CSettings::Get().GetInt("lcd.type") != LCD_TYPE_NONE))
  {
    if ( (IsPlayingVideo()) && CSettings::Get().GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO)
      g_lcd->SetBackLight(0);
    if ( (IsPlayingAudio()) && CSettings::Get().GetInt("lcd.disableonplayback") == LED_PLAYBACK_MUSIC)
      g_lcd->SetBackLight(0);
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && CSettings::Get().GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO_MUSIC)
      g_lcd->SetBackLight(0);
  }
  else if(!dim)
    g_lcd->SetBackLight(CSettings::Get().GetInt("lcd.backlight"));
#endif
}

void CApplication::StartServices()
{
#ifdef HAS_XBOX_HARDWARE
  if (g_advancedSettings.m_bPowerSave)
  {
    CLog::Log(LOGNOTICE, "Using idle thread with HLT (power saving)");
    StartIdleThread();
  }
  else
    CLog::Log(LOGNOTICE, "Not using idle thread with HLT (no power saving)");
#endif

  CheckDate();
  StartLEDControl(false);

  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create(false, THREAD_MINSTACKSIZE);

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, CMediaSettings::Get().DoesMusicPlaylistRepeat() ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, CMediaSettings::Get().IsMusicPlaylistShuffled());
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, CMediaSettings::Get().DoesVideoPlaylistRepeat() ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, CMediaSettings::Get().IsVideoPlaylistShuffled());
  CLog::Log(LOGNOTICE, "DONE initializing playlistplayer");

#ifdef HAS_LCD
  CLCDFactory factory;
  g_lcd = factory.Create();
  if (g_lcd)
  {
    g_lcd->Initialize();
  }
#endif

#ifdef HAS_XBOX_HARDWARE
  if (CSettings::Get().GetBool("system.autotemperature"))
  {
    CLog::Log(LOGNOTICE, "start fancontroller");
    CFanController::Instance()->Start(CSettings::Get().GetInt("system.targettemperature"), CSettings::Get().GetInt("system.minfanspeed"));
  }
  else if (CSettings::Get().GetBool("system.fanspeedcontrol"))
  {
    CLog::Log(LOGNOTICE, "setting fanspeed");
    CFanController::Instance()->SetFanSpeed(CSettings::Get().GetInt("system.fanspeed"));
  }
  int setting_level = CSettings::Get().GetInt("harddisk.aamlevel");
  if (setting_level == AAM_QUIET)
    XKHDD::SetAAMLevel(0x80);
  else if (setting_level == AAM_FAST)
    XKHDD::SetAAMLevel(0xFE);
  setting_level = CSettings::Get().GetInt("harddisk.apmlevel");
  switch(setting_level)
  {
  case APM_LOPOWER:
    XKHDD::SetAPMLevel(0x80);
    break;
  case APM_HIPOWER:
    XKHDD::SetAPMLevel(0xFE);
    break;
  case APM_LOPOWER_STANDBY:
    XKHDD::SetAPMLevel(0x01);
    break;
  case APM_HIPOWER_STANDBY:
    XKHDD::SetAPMLevel(0x7F);
    break;
  }
#endif
}

void CApplication::CheckDate()
{
  CLog::Log(LOGNOTICE, "Checking the Date!");
  // Check the Date: Year, if it is  above 2099 set to 2004!
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  CLog::Log(LOGINFO, "Current Date is: %i-%i-%i", CurTime.wDay, CurTime.wMonth, CurTime.wYear);
  if ((CurTime.wYear > 2099) || (CurTime.wYear < 2001) )        // XBOX MS Dashboard also uses min/max DateYear 2001/2099 !!
  {
    CLog::Log(LOGNOTICE, "- The Date is Wrong: Setting New Date!");
    NewTime.wYear       = 2004; // 2004
    NewTime.wMonth      = 1;  // January
    NewTime.wDayOfWeek  = 1;  // Monday
    NewTime.wDay        = 5;  // Monday 05.01.2004!!
    NewTime.wHour       = 12;
    NewTime.wMinute     = 0;

    FILETIME stNewTime, stCurTime;
    SystemTimeToFileTime(&NewTime, &stNewTime);
    SystemTimeToFileTime(&CurTime, &stCurTime);
#ifdef HAS_XBOX_HARDWARE
    NtSetSystemTime(&stNewTime, &stCurTime);    // Set a Default Year 2004!
#endif
    CLog::Log(LOGNOTICE, "- New Date is now: %i-%i-%i",NewTime.wDay, NewTime.wMonth, NewTime.wYear);
  }
  return ;
}

void CApplication::StopServices()
{
  m_network->NetworkMessage(CNetwork::SERVICES_DOWN, 0);

  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();

#ifdef HAS_XBOX_HARDWARE
  CLog::Log(LOGNOTICE, "stop fancontroller");
  CFanController::Instance()->Stop();
  CFanController::RemoveInstance();
  if (g_advancedSettings.m_bPowerSave)
    StopIdleThread();
#endif  
}

void CApplication::ReloadSkin(bool confirm/*=false*/)
{
  std::string oldSkin = g_SkinInfo ? g_SkinInfo->ID() : "";

  CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1, g_windowManager.GetActiveWindow());
  g_windowManager.SendMessage(msg);

  string newSkin = CSettings::Get().GetString("lookandfeel.skin");
  if (LoadSkin(newSkin))
  {
    /* The Reset() or SetString() below will cause recursion, so the m_skinReverting boolean is set so as to not prompt the
       user as to whether they want to keep the current skin. */
    if (confirm && !m_skinReverting)
    {
      bool cancelled;
      if (!CGUIDialogYesNo::ShowAndGetInput(13123, 13111, -1, -1, -1, -1, cancelled, 10000))
      {
        m_skinReverting = true;
        if (oldSkin.empty())
          CSettings::Get().GetSetting("lookandfeel.skin")->Reset();
        else
          CSettings::Get().SetString("lookandfeel.skin", oldSkin);
      }
    }
  }
  else
  {
    // skin failed to load - we revert to the default only if we didn't fail loading the default
    string defaultSkin = ((CSettingString*)CSettings::Get().GetSetting("lookandfeel.skin"))->GetDefault();
    if (newSkin != defaultSkin)
    {
      m_skinReverting = true;
      CSettings::Get().GetSetting("lookandfeel.skin")->Reset();
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24102), g_localizeStrings.Get(24103));
    }
  }
  m_skinReverting = false;
}

bool CApplication::OnSettingsSaving() const
{
  // don't save settings when we're busy stopping the application
  // a lot of screens try to save settings on deinit and deinit is
  // called for every screen when the application is stopping
  if (m_bStop)
    return false;

  return true;
}

bool CApplication::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  const TiXmlElement *audioElement = settings->FirstChildElement("audio");
  if (audioElement != NULL)
  {
    XMLUtils::GetBoolean(audioElement, "mute", m_muted);
    if (!XMLUtils::GetInt(audioElement, "volumelevel", m_volumeLevel, VOLUME_MINIMUM, VOLUME_MAXIMUM))
      m_volumeLevel = VOLUME_MAXIMUM;
    if (!XMLUtils::GetInt(audioElement, "dynamicrangecompression", m_dynamicRangeCompressionLevel, 0/*VOLUME_DRC_MINIMUM*/, 3000/*VOLUME_DRC_MAXIMUM*/))
      m_dynamicRangeCompressionLevel = 0;
    for (int i = 0; i < 4; i++)
    {
      CStdString setting;
      setting.Format("karaoke%i", i);
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
      if(!XMLUtils::GetFloat(audioElement, setting + "energy", m_karaokeVoiceMask[i].energy, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].energy = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, setting + "pitch", m_karaokeVoiceMask[i].pitch, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].pitch = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, setting + "whisper", m_karaokeVoiceMask[i].whisper, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].whisper = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, setting + "robotic", m_karaokeVoiceMask[i].robotic, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].robotic = XVOICE_MASK_PARAM_DISABLED;
    }
  }

  return true;
}

bool CApplication::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  TiXmlElement volumeNode("audio");
  TiXmlNode *audioNode = settings->InsertEndChild(volumeNode);
  if (audioNode == NULL)
    return false;

  XMLUtils::SetBoolean(audioNode, "mute", m_muted);
  XMLUtils::SetInt(audioNode, "volumelevel", m_volumeLevel);
  XMLUtils::SetInt(audioNode, "dynamicrangecompression", m_dynamicRangeCompressionLevel);
  for (int i = 0; i < 4; i++)
  {
    CStdString setting;
    setting.Format("karaoke%i", i);
    XMLUtils::SetFloat(audioNode, setting + "energy", m_karaokeVoiceMask[i].energy);
    XMLUtils::SetFloat(audioNode, setting + "pitch", m_karaokeVoiceMask[i].pitch);
    XMLUtils::SetFloat(audioNode, setting + "whisper", m_karaokeVoiceMask[i].whisper);
    XMLUtils::SetFloat(audioNode, setting + "robotic", m_karaokeVoiceMask[i].robotic);
  }
  return true;
}

bool CApplication::LoadSkin(const CStdString& skinID)
{
  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(skinID, addon))
  {
    if (LoadSkin(boost::dynamic_pointer_cast<ADDON::CSkinInfo>(addon)))
      return true;
  }
  CLog::Log(LOGERROR, "failed to load requested skin '%s'", skinID.c_str());
  return false;
}

bool CApplication::LoadSkin(const SkinPtr& skin)
{
  if (!skin)
    return false;

  skin->Start();
  if (!skin->HasSkinFile("Home.xml"))
    return false;

  bool bPreviousPlayingState=false;
  bool bPreviousRenderingState=false;
  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    bPreviousPlayingState = !g_application.m_pPlayer->IsPaused();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
#ifdef HAS_VIDEO_PLAYBACK
    if (!g_renderManager.Paused())
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
     {
        g_windowManager.ActivateWindow(WINDOW_HOME);
        bPreviousRenderingState = true;
      }
    }
#endif
  }
  // close the music and video overlays (they're re-opened automatically later)
  CSingleLock lock(g_graphicsContext);

  // save the current window details and focused control
  int currentWindow = g_windowManager.GetActiveWindow();
  int iCtrlID = -1;
  CGUIWindow* pWindow = g_windowManager.GetWindow(currentWindow);
  if (pWindow)
    iCtrlID = pWindow->GetFocusedControlID();
  vector<int> currentModelessWindows;
  g_windowManager.GetActiveModelessWindows(currentModelessWindows);

  UnloadSkin();

  CLog::Log(LOGINFO, "  load skin from:%s", skin->Path().c_str());

  g_SkinInfo = skin;
  g_SkinInfo->Start();

  CLog::Log(LOGINFO, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(skin->Path());
  g_directoryCache.ClearSubPaths(skin->Path());

  g_colorManager.Load(CSettings::Get().GetString("lookandfeel.skincolors"));

  g_fontManager.LoadFonts(CSettings::Get().GetString("lookandfeel.font"));

  // load in the skin strings
  CStdString langPath;
  URIUtils::AddFileToFolder(skin->Path(), "language", langPath);
  URIUtils::AddSlashAtEnd(langPath);

  g_localizeStrings.LoadSkinStrings(langPath, CSettings::Get().GetString("locale.language"));

  g_SkinInfo->LoadIncludes();

  int64_t start;
  start = CurrentHostCounter();

  CLog::Log(LOGINFO, "  load new skin...");

  // Load the user windows
  LoadUserWindows();

  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  CLog::Log(LOGDEBUG,"Load Skin XML: %.2fms", 1000.f * (end - start) / freq);

  CLog::Log(LOGINFO, "  initialize new skin...");
  m_guiPointer.AllocResources(true);
  g_windowManager.AddMsgTarget(this);
  g_windowManager.AddMsgTarget(&g_playlistPlayer);
  g_windowManager.AddMsgTarget(&g_infoManager);
  g_windowManager.SetCallback(*this);
  g_windowManager.Initialize();
  g_audioManager.Enable(true);
  g_audioManager.Load();

  if (g_SkinInfo->HasSkinFile("DialogFullScreenInfo.xml"))
    g_windowManager.Add(new CGUIDialogFullScreenInfo);

  CLog::Log(LOGINFO, "  skin loaded...");

  // leave the graphics lock
  lock.Leave();

  // restore windows
  if (currentWindow != WINDOW_INVALID)
  {
    g_windowManager.ActivateWindow(currentWindow);
    for (unsigned int i = 0; i < currentModelessWindows.size(); i++)
    {
      CGUIDialog *dialog = (CGUIDialog *)g_windowManager.GetWindow(currentModelessWindows[i]);
      if (dialog) dialog->Show();
    }
    if (iCtrlID != -1)
    {
      pWindow = g_windowManager.GetWindow(currentWindow);
      if (pWindow && pWindow->HasSaveLastControl())
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, currentWindow, iCtrlID, 0);
        pWindow->OnMessage(msg);
      }
    }
  }

  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (bPreviousRenderingState)
      g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
  }
  return true;
}

void CApplication::UnloadSkin(bool forReload /* = false */)
{
  CLog::Log(LOGINFO, "Unloading old skin %s...", forReload ? "for reload " : "");

  g_audioManager.Enable(false);

  g_windowManager.DeInitialize();

  //These windows are not handled by the windowmanager (why not?) so we should unload them manually
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  m_guiPointer.OnMessage(msg);
  m_guiPointer.ResetControlStates();
  m_guiPointer.FreeResources(true);

  delete m_debugLayout;
  m_debugLayout = NULL;

  // remove the skin-dependent window
  g_windowManager.Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

  g_TextureManager.Cleanup();

  g_fontManager.Clear();

  g_colorManager.Clear();

  g_infoManager.Clear();

  g_SkinInfo.reset();
}

bool CApplication::LoadUserWindows()
{
  // Start from wherever home.xml is
  std::vector<CStdString> vecSkinPath;
  g_SkinInfo->GetSkinPaths(vecSkinPath);
  for (unsigned int i = 0;i < vecSkinPath.size();++i)
  {
    CLog::Log(LOGINFO, "Loading user windows, path %s", vecSkinPath[i].c_str());
    CFileItemList items;
    if (CDirectory::GetDirectory(vecSkinPath[i], items, ".xml", DIR_FLAG_NO_FILE_DIRS))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        if (items[i]->m_bIsFolder)
          continue;
        CStdString skinFile = URIUtils::GetFileName(items[i]->GetPath());
        if (skinFile.Left(6).CompareNoCase("custom") == 0)
        {
          CXBMCTinyXML xmlDoc;
          if (!xmlDoc.LoadFile(items[i]->GetPath()))
          {
            CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", items[i]->GetPath().c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
            continue;
          }

          // Root element should be <window>
          TiXmlElement* pRootElement = xmlDoc.RootElement();
          CStdString strValue = pRootElement->Value();
          if (!strValue.Equals("window"))
          {
            CLog::Log(LOGERROR, "file:%s doesnt contain <window>", skinFile.c_str());
            continue;
          }

          // Read the <type> element to get the window type to create
          // If no type is specified, create a CGUIWindow as default
          CGUIWindow* pWindow = NULL;
          CStdString strType;
          if (pRootElement->Attribute("type"))
            strType = pRootElement->Attribute("type");
          else
          {
            const TiXmlNode *pType = pRootElement->FirstChild("type");
            if (pType && pType->FirstChild())
              strType = pType->FirstChild()->Value();
          }
          int id = WINDOW_INVALID;
          if (!pRootElement->Attribute("id", &id))
          {
            const TiXmlNode *pType = pRootElement->FirstChild("id");
            if (pType && pType->FirstChild())
              id = atol(pType->FirstChild()->Value());
          }
          CStdString visibleCondition;
          CGUIControlFactory::GetConditionalVisibility(pRootElement, visibleCondition);

          if (strType.Equals("dialog"))
            pWindow = new CGUIDialog(id + WINDOW_HOME, skinFile);
          else if (strType.Equals("submenu"))
            pWindow = new CGUIDialogSubMenu(id + WINDOW_HOME, skinFile);
          else if (strType.Equals("buttonmenu"))
            pWindow = new CGUIDialogButtonMenu(id + WINDOW_HOME, skinFile);
          else
            pWindow = new CGUIWindow(id + WINDOW_HOME, skinFile);

          // Check to make sure the pointer isn't still null
          if (pWindow == NULL)
          {
            CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
            return false;
          }
          if (id == WINDOW_INVALID || g_windowManager.GetWindow(WINDOW_HOME + id))
          {
            delete pWindow;
            continue;
          }
          pWindow->SetVisibleCondition(visibleCondition);
          pWindow->SetLoadType(CGUIWindow::KEEP_IN_MEMORY);
          g_windowManager.AddCustomWindow(pWindow);
        }
      }
    }
  }
  return true;
}

#ifdef HAS_XBOX_D3D  // needed for screenshot
void CApplication::Render()
{
#else
void CApplication::RenderNoPresent()
{
#endif
  // don't do anything that would require graphiccontext to be locked before here in fullscreen.
  // that stuff should go into renderfullscreen instead as that is called from the renderin thread
#ifdef HAS_XBOX_HARDWARE  // Win32 renders from the main thread, not from the player thread
  // dont show GUI when playing full screen video
  if (g_graphicsContext.IsFullScreenVideo() && IsPlaying() && !IsPaused())
  {
    Sleep(50);
    ResetScreenSaver();
    g_infoManager.ResetCache();
    return;
  }
#endif
  if(!m_pd3dDevice)
    return;

  g_graphicsContext.Lock();

  m_pd3dDevice->BeginScene();

  g_windowManager.UpdateModelessVisibility();

  //SWATHWIDTH of 4 improves fillrates (performance investigator)
#ifdef HAS_XBOX_D3D
  m_pd3dDevice->SetRenderState(D3DRS_SWATHWIDTH, 4);
#endif
  g_windowManager.Render();

  // if we're recording an audio stream then show blinking REC
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    if (m_pPlayer && m_pPlayer->IsRecording() )
    {
      static int iBlinkRecord = 0;
      iBlinkRecord++;
      if (iBlinkRecord > 25)
      {
        CGUIFont* pFont = g_fontManager.GetFont("font13");
        CGUITextLayout::DrawText(pFont, 60, 50, 0xffff0000, 0, "REC", 0);
      }

      if (iBlinkRecord > 50)
        iBlinkRecord = 0;
    }
  }

  // Now render any dialogs
  g_windowManager.RenderDialogs();

  // Render the mouse pointer
  if (g_Mouse.IsActive())
  {
    m_guiPointer.Render();
  }

  {
    // free memory if we got les then 10megs free ram
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    DWORD dwMegFree = (DWORD)(stat.dwAvailPhys / (1024 * 1024));
    if (dwMegFree <= 10)
    {
      g_TextureManager.Flush();
    }

    // reset image scaling and effect states
    g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);

    // If we have the remote codes enabled, then show them
    if (g_advancedSettings.m_displayRemoteCodes)
    {
#ifdef HAS_IR_REMOTE
      XBIR_REMOTE* pRemote = &m_DefaultIR_Remote;
      static iRemoteCode = 0;
      static iShowRemoteCode = 0;
      if (pRemote->wButtons)
      {
        iRemoteCode = 255 - pRemote->wButtons; // remote OBC code is 255-wButtons
        iShowRemoteCode = 50;
      }
      if (iShowRemoteCode > 0)
      {
        CStdStringW wszText;
        wszText.Format(L"Remote Code: %i", iRemoteCode);
        float x = 0.08f * g_graphicsContext.GetWidth();
        float y = 0.12f * g_graphicsContext.GetHeight();
#ifndef _DEBUG
        if (LOG_LEVEL_DEBUG_FREEMEM > g_advancedSettings.m_logLevel)
          y = 0.08f * g_graphicsContext.GetHeight();
#endif
        CGUITextLayout::DrawOutlineText(g_fontManager.GetFont("font13"), x, y, 0xffffffff, 0xff000000, 2, wszText);
        iShowRemoteCode--;
      }
#endif
    }

    RenderMemoryStatus();
  }

  m_pd3dDevice->EndScene();
#ifdef HAS_XBOX_D3D
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
#endif
  g_graphicsContext.Unlock();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  g_infoManager.ResetCache();
}

#ifndef HAS_XBOX_D3D
void CApplication::Render()
{
  g_graphicsContext.Lock();
  { // frame rate limiter (really bad, but it does the trick :p)
    const static unsigned int singleFrameTime = 10;
    static unsigned int lastFrameTime = 0;
    unsigned int currentTime = timeGetTime();
    if (lastFrameTime + singleFrameTime > currentTime)
      Sleep(lastFrameTime + singleFrameTime - currentTime);
    lastFrameTime = timeGetTime();
  }
  RenderNoPresent();
  // Present the backbuffer contents to the display
  if (m_pd3dDevice) m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
  CTimeUtils::UpdateFrameTime();
  g_graphicsContext.Unlock();
}
#endif

void CApplication::RenderMemoryStatus()
{
  g_infoManager.UpdateFPS();

  if (!m_debugLayout)
  {
    CGUIFont *font13 = g_fontManager.GetDefaultFont();
    CGUIFont *font13border = g_fontManager.GetDefaultFont(true);
    if (font13)
      m_debugLayout = new CGUITextLayout(font13, true, 0, font13border);
  }
  if (!m_debugLayout)
    return;

#if !defined(_DEBUG) && !defined(PROFILE)
  if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel)
#endif
  {
    // reset the window scaling and fade status
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);

    CStdString info;
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    info.Format("FreeMem %d/%d KB, FPS %2.1f, CPU %2.0f%%", stat.dwAvailPhys/1024, stat.dwTotalPhys/1024, g_infoManager.GetFPS(), (1.0f - m_idleThread.GetRelativeUsage())*100);
    
    if(g_SkinInfo->IsDebugging())
    {
      if (!info.IsEmpty())
        info += "\n";
      CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
      CGUIWindow *pointer = g_windowManager.GetWindow(105);
      CPoint point;
      if (pointer)
        point = CPoint(pointer->GetXPosition(), pointer->GetYPosition());
      if (window)
      {
        CStdString windowName = CButtonTranslator::TranslateWindow(window->GetID());
        if (!windowName.IsEmpty())
          windowName += " (" + window->GetProperty("xmlfile").asString() + ")";
        else
          windowName = window->GetProperty("xmlfile").asString();
        info += "Window: " + windowName + "  ";
        // transform the mouse coordinates to this window's coordinates
        g_graphicsContext.SetScalingResolution(window->GetCoordsRes(), true);
        point.x *= g_graphicsContext.GetGUIScaleX();
        point.y *= g_graphicsContext.GetGUIScaleY();
        g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
      }
      info.AppendFormat("Mouse: (%d,%d)  ", (int)point.x, (int)point.y);
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
          info.AppendFormat("Focused: %i (%s)", control->GetID(), CGUIControlFactory::TranslateControlType(control->GetControlType()).c_str());
      }
    }
    float x = 0.04f * g_graphicsContext.GetWidth() + CDisplaySettings::Get().GetResolutionInfo(res).Overscan.left;
    float y = 0.04f * g_graphicsContext.GetHeight() + CDisplaySettings::Get().GetResolutionInfo(res).Overscan.top;

    m_debugLayout->Update(info);
    m_debugLayout->RenderOutline(x, y, 0xffffffff, 0xff000000, 0, 0);
  }
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CApplication::OnKey(CKey& key)
{
  // Turn the mouse off, as we've just got a keypress from controller or remote
  g_Mouse.SetInactive();
  
  // get the current active window
  int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CAction action = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // a key has been pressed.
  // Reset the screensaver timer
  // but not for the analog thumbsticks/triggers
  if (!key.IsAnalogButton())
  {
    // reset harddisk spindown timer
    m_bSpinDown = false;
    m_bNetworkSpinDown = false;

    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();

    // allow some keys to be processed while the screensaver is active
    if (ResetScreenSaverWindow())
    {
      return true;
    }  
  }

  // change this if we have a dialog up
  if (g_windowManager.HasModalDialog())
  {
    iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }
  if (iWin == WINDOW_DIALOG_FULLSCREEN_INFO)
  { // fullscreen info dialog - special case
    action = CButtonTranslator::GetInstance().GetAction(iWin, key);

#ifdef HAS_SDL
    g_Keyboard.Reset();
#endif

    if (!key.IsAnalogButton())
      CLog::Log(LOGDEBUG, "%s: %i pressed, trying fullscreen info action %s", __FUNCTION__, (int) key.GetButtonCode(), action.GetName().c_str());

    if (OnAction(action))
      return true;

    // fallthrough to the main window
    iWin = WINDOW_FULLSCREEN_VIDEO;
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      action = CButtonTranslator::GetInstance().GetAction(WINDOW_VIDEO_MENU, key);
    }
    else
    {
      // no then use the fullscreen window section of keymap.xml to map key->action
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
  }
  else
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard = key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow *window = g_windowManager.GetWindow(iWin);
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
      {
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT ||
            (control->IsContainer() && g_Keyboard.GetShift()))
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      action = CAction(0); // reset our action
      if (key.GetFromHttpApi())
        action = CAction(key.GetButtonCode() != KEY_INVALID ? key.GetButtonCode() : 0, key.GetUnicode());
      else
      { // see if we've got an ascii key
        if (g_Keyboard.GetUnicode())
          action = CAction(g_Keyboard.GetAscii() | KEY_ASCII, g_Keyboard.GetUnicode());
        else
          action = CAction(g_Keyboard.GetKey() | KEY_VKEY);
      }
#ifdef HAS_SDL
      g_Keyboard.Reset();
#endif

      CLog::Log(LOGDEBUG, "%s: %i pressed, trying keyboard action %i", __FUNCTION__, (int) key.GetButtonCode(), action.GetID());

      if (OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromHttpApi())
    {
      if (key.GetButtonCode() != KEY_INVALID)
        action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
    else
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
  }
  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, "%s: %i pressed, action is %s", __FUNCTION__, (int) key.GetButtonCode(), action.GetName().c_str());

  bool bResult = false;

  // play sound before the action unless the button is held, 
  // where we execute after the action as held actions aren't fired every time.
  if(action.GetHoldTime())
  {
    bResult = OnAction(action);
    if(bResult)
      g_audioManager.PlayActionSound(action);
  }
  else
  {
    g_audioManager.PlayActionSound(action);
    bResult = OnAction(action);
  }

  return bResult;
}

bool CApplication::OnAction(CAction &action)
{
  // Let's tell the outside world about this action
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=2)
  {
    CStdString tmp;
    tmp.Format("%i",action.GetID());
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnAction:"+tmp+";2");
  }

  // special case for switching between GUI & fullscreen mode.
  if (action.GetID() == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  // in normal case
  // just pass the action to the current window and let it handle it
  if (g_windowManager.OnAction(action))
  {
    m_navigationTimer.StartZero();
    return true;
  }

  // handle extra global presses

  // screenshot : take a screenshot :)
  if (action.GetID() == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    CBuiltins::Execute(action.GetName());
    m_navigationTimer.StartZero();
    return true;
  }

  // power down : turn off after 3 seconds of button down
  static bool PowerButtonDown = false;
  static DWORD PowerButtonCode;
  static DWORD MarkTime;
  if (action.GetID() == ACTION_POWERDOWN)
  {
    // Hold button for 3 secs to power down
    if (!PowerButtonDown)
    {
      MarkTime = GetTickCount();
      PowerButtonDown = true;
      PowerButtonCode = action.GetButtonCode();
    }
  }
  if (PowerButtonDown)
  {
    if (g_application.IsButtonDown(PowerButtonCode))
    {
      if (GetTickCount() >= MarkTime + 3000)
      {
        CApplicationMessenger::Get().Shutdown();
        return true;
      }
    }
    else
      PowerButtonDown = false;
  }
  // reload keymaps
  if (action.GetID() == ACTION_RELOAD_KEYMAPS)
  {
    CButtonTranslator::GetInstance().Clear();
    CButtonTranslator::GetInstance().Load();
  }

  // show info : Shows the current video or song information
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    g_infoManager.ToggleShowInfo();
    return true;
  }

  // codec info : Shows the current song, video or picture codec information
  if (action.GetID() == ACTION_SHOW_CODEC)
  {
    g_infoManager.ToggleShowCodec();
    return true;
  }

  if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) && IsPlayingAudio())
  {
    const CMusicInfoTag *tag = g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      *m_itemCurrentFile->GetMusicInfoTag() = *tag;
      char rating = tag->GetRating();
      bool needsUpdate(false);
      if (rating > '0' && action.GetID() == ACTION_DECREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating - 1);
        needsUpdate = true;
      }
      else if (rating < '5' && action.GetID() == ACTION_INCREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating + 1);
        needsUpdate = true;
      }
      if (needsUpdate)
      {
        CMusicDatabase db;
        if (db.Open())      // OpenForWrite() ?
        {
          db.SetSongRating(m_itemCurrentFile->GetPath(), m_itemCurrentFile->GetMusicInfoTag()->GetRating());
          db.Close();
        }
        // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
        g_windowManager.SendMessage(msg);
      }
    }
    return true;
  }

  // stop : stops playing current audio song
  if (action.GetID() == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // previous : play previous song from playlist
  if (action.GetID() == ACTION_PREV_ITEM)
  {
    // first check whether we're within 3 seconds of the start of the track
    // if not, we just revert to the start of the track
    if (m_pPlayer && m_pPlayer->CanSeek() && GetTime() > 3)
    {
      SeekTime(0);
      SetPlaySpeed(1);
    }
    else
    {
      g_playlistPlayer.PlayPrevious();
    }
    return true;
  }

  // next : play next song from playlist
  if (action.GetID() == ACTION_NEXT_ITEM)
  {
    if (IsPlaying() && m_pPlayer->SkipNext())
      return true;

    g_playlistPlayer.PlayNext();

    return true;
  }

  if ( IsPlaying())
  {
    // pause : pauses current audio song
    if (action.GetID() == ACTION_PAUSE)
    {
      m_pPlayer->Pause();
      if (!m_pPlayer->IsPaused())
      { // unpaused - set the playspeed back to normal
        SetPlaySpeed(1);
      }
      g_audioManager.Enable(m_pPlayer->IsPaused());
      return true;
    }
    if (!m_pPlayer->IsPaused())
    {
      // if we do a FF/RW in my music then map PLAY action togo back to normal speed
      // if we are playing at normal speed, then allow play to pause
      if (action.GetID() == ACTION_PLAYER_PLAY)
      {
        if (m_iPlaySpeed != 1)
        {
          SetPlaySpeed(1);
        }
        else
        {
          m_pPlayer->Pause();
        }
        return true;
      }
      if (action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND)
      {
        int iPlaySpeed = m_iPlaySpeed;
        if (action.GetID() == ACTION_PLAYER_REWIND && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (action.GetID() == ACTION_PLAYER_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (action.GetID() == ACTION_PLAYER_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
          iPlaySpeed /= 2;
        else
          iPlaySpeed *= 2;

        if (action.GetID() == ACTION_PLAYER_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          iPlaySpeed = 1;
        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        SetPlaySpeed(iPlaySpeed);
        return true;
      }
      else if ((action.GetAmount() || GetPlaySpeed() != 1) && (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.GetID() == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        m_pPlayer->Pause();
        g_audioManager.Enable(m_pPlayer->IsPaused());

        g_application.SetPlaySpeed(1);
        return true;
      }
    }
  }
  if (action.GetID() == ACTION_MUTE)
  {
    ToggleMute();
    return true;
  }
 
  if (action.GetID() == ACTION_TOGGLE_DIGITAL_ANALOG)
  { 
    if(CSettings::Get().GetInt("audiooutput.mode")==AUDIO_DIGITAL)
      CSettings::Get().SetInt("audiooutput.mode", AUDIO_ANALOG);
    else
      CSettings::Get().SetInt("audiooutput.mode", AUDIO_DIGITAL);
    g_application.Restart();
    if (g_windowManager.GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if (action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN))
  {
    if (m_muted)
      UnMute();
    int volume = m_volumeLevel + m_dynamicRangeCompressionLevel;

    // calculate speed so that a full press will equal 1 second from min to max
    float speed = float(VOLUME_MAXIMUM - VOLUME_MINIMUM);
    if( action.GetRepeat() )
      speed *= action.GetRepeat();
    else
      speed /= 50; //50 fps

    if (action.GetID() == ACTION_VOLUME_UP)
      volume += (int)((float)fabs(action.GetAmount()) * action.GetAmount() * speed);
    else
      volume -= (int)((float)fabs(action.GetAmount()) * action.GetAmount() * speed);

    SetVolume(volume, false);

    // show visual feedback of volume change...
    ShowVolumeBar(&action);
    return true;
  }
  // Check for global seek control
  if (IsPlaying() && action.GetAmount() && (action.GetID() == ACTION_ANALOG_SEEK_FORWARD || action.GetID() == ACTION_ANALOG_SEEK_BACK))
  {
    if (!m_pPlayer->CanSeek()) return false;
    CGUIWindow *seekBar = g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
    if (seekBar)
      seekBar->OnAction(action);
    return true;
  }
  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
    if (iPlaylist == PLAYLIST_VIDEO && g_windowManager.GetActiveWindow() != WINDOW_VIDEO_PLAYLIST)
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    else if (iPlaylist == PLAYLIST_MUSIC && g_windowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;
  }
  return false;
}

void CApplication::UpdateLCD()
{
#ifdef HAS_LCD
  static lTickCount = 0;

  if (!g_lcd || CSettings::Get().GetInt("lcd.type") == LCD_TYPE_NONE)
    return ;
  long lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( ((long)GetTickCount() - lTickCount) >= lTimeOut)
  {
    if (g_application.NavigationIdleTime() < 5)
      g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
    else if (IsPlayingVideo())
      g_lcd->Render(ILCD::LCD_MODE_VIDEO);
    else if (IsPlayingAudio())
      g_lcd->Render(ILCD::LCD_MODE_MUSIC);
    else if (IsInScreenSaver())
      g_lcd->Render(ILCD::LCD_MODE_SCREENSAVER);
    else
      g_lcd->Render(ILCD::LCD_MODE_GENERAL);

    // reset tick count
    lTickCount = GetTickCount();
  }
#endif
}

void CApplication::FrameMove()
{
  // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
  float frameTime = m_frameTime.GetElapsedSeconds();
  m_frameTime.StartZero();
  // never set a frametime less than 2 fps to avoid problems when debuggin and on breaks
  if( frameTime > 0.5 ) frameTime = 0.5;

  g_graphicsContext.Lock();
  // check if there are notifications to display
  CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
  if (toast && toast->DoWork())
  {
    if (!toast->IsDialogRunning())
    {
      toast->Show();
    }
  }
  g_graphicsContext.Unlock();

  UpdateLCD();

  // read raw input from controller, remote control, mouse and keyboard
  ReadInput();
  // process input actions
  ProcessMouse();
  ProcessHTTPApiButtons();
  ProcessKeyboard();
  ProcessRemote(frameTime);
  ProcessGamepad(frameTime);
  ProcessEventServer(frameTime);

  g_windowManager.FrameMove();
}

bool CApplication::ProcessGamepad(float frameTime)
{
#ifdef HAS_GAMEPAD
  // Handle the gamepad button presses.  We check for button down,
  // then call OnKey() which handles the translation to actions, and sends the
  // action to our window manager's OnAction() function, which filters the messages
  // to where they're supposed to end up, returning true if the message is successfully
  // processed.  If OnKey() returns false, then the key press wasn't processed at all,
  // and we can safely process the next key (or next check on the same key in the
  // case of the analog sticks which can produce more than 1 key event.)

  WORD wButtons = m_DefaultGamepad.wButtons;
  WORD wDpad = wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);

  BYTE bLeftTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
  BYTE bRightTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
  BYTE bButtonA = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A];
  BYTE bButtonB = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B];
  BYTE bButtonX = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X];
  BYTE bButtonY = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y];

  // pass them through the delay
  WORD wDir = m_ctrDpad.DpadInput(wDpad, 0 != bLeftTrigger, 0 != bRightTrigger);

  // map all controller & remote actions to their keys
  if (m_DefaultGamepad.fX1 || m_DefaultGamepad.fY1)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.fX2 || m_DefaultGamepad.fY2)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  // direction specific keys (for defining different actions for each direction)
  // We need to be able to know when it last had a direction, so that we can
  // post the reset direction code the next time around (to reset scrolling,
  // fastforwarding and other analog actions)

  // For the sticks, once it is pushed in one direction (eg up) it will only
  // detect movement in that direction of movement (eg up or down) - the other
  // direction (eg left and right) will not be registered until the stick has
  // been recentered for at least 2 frames.

  // first the right stick
  static lastRightStickKey = 0;
  int newRightStickKey = 0;
  if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_UP || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 != 0)
    {
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
      //m_DefaultGamepad.fY2 = 0.00001f; // small amount of movement
    }
  }
  else if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
    else if (m_DefaultGamepad.fY2 != 0)
    {
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
      //m_DefaultGamepad.fX2 = 0.00001f; // small amount of movement
    }
  }
  else
  {
    if (m_DefaultGamepad.fY2 > 0 && m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0 && m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 > 0 && m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0 && m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  }
  if (lastRightStickKey && newRightStickKey != lastRightStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastRightStickKey, 0, 0, 0, 0, 0, 0);
    lastRightStickKey = newRightStickKey;
    if (OnKey(key)) return true;
  }
  lastRightStickKey = newRightStickKey;
  // post the new key's message
  if (newRightStickKey)
  {
    CKey key(newRightStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // now the left stick
  static lastLeftStickKey = 0;
  int newLeftStickKey = 0;
  if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_UP || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  }
  else if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_LEFT || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }
  else
  { // check for a new control movement
    if (m_DefaultGamepad.fY1 > 0 && m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0 && m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX1 > 0 && m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0 && m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }

  if (lastLeftStickKey && newLeftStickKey != lastLeftStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastLeftStickKey, 0, 0, 0, 0, 0, 0);
    lastLeftStickKey = newLeftStickKey;
    if (OnKey(key)) return true;
  }
  lastLeftStickKey = newLeftStickKey;
  // post the new key's message
  if (newLeftStickKey)
  {
    CKey key(newLeftStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // Trigger detection
  static lastTriggerKey = 0;
  int newTriggerKey = 0;
  if (bLeftTrigger)
    newTriggerKey = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (bRightTrigger)
    newTriggerKey = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  if (lastTriggerKey && newTriggerKey != lastTriggerKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastTriggerKey, 0, 0, 0, 0, 0, 0);
    lastTriggerKey = newTriggerKey;
    if (OnKey(key)) return true;
  }
  lastTriggerKey = newTriggerKey;
  // post the new key's message
  if (newTriggerKey)
  {
    CKey key(newTriggerKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  // Now the digital buttons...
  if ( wDir & DC_LEFTTRIGGER)
  {
    CKey key(KEY_BUTTON_LEFT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
    CKey key(KEY_BUTTON_RIGHT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_LEFT )
  {
    CKey key(KEY_BUTTON_DPAD_LEFT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_RIGHT)
  {
    CKey key(KEY_BUTTON_DPAD_RIGHT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_UP )
  {
    CKey key(KEY_BUTTON_DPAD_UP, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if ( wDir & DC_DOWN )
  {
    CKey key(KEY_BUTTON_DPAD_DOWN, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    CKey key(KEY_BUTTON_BACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START)
  {
    CKey key(KEY_BUTTON_START, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    CKey key(KEY_BUTTON_A, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    CKey key(KEY_BUTTON_B, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }

  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    CKey key(KEY_BUTTON_X, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    CKey key(KEY_BUTTON_Y, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    CKey key(KEY_BUTTON_BLACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CKey key(KEY_BUTTON_WHITE, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key)) return true;
  }
#endif
  return false;
}

bool CApplication::ProcessRemote(float frameTime)
{
#ifdef HAS_IR_REMOTE
  if (m_DefaultIR_Remote.wButtons)
  {
    // time depends on whether the movement is repeated (held down) or not.
    // If it is, we use the FPS timer to get a repeatable speed.
    // If it isn't, we use 20 to get repeatable jumps.
    float time = (m_DefaultIR_Remote.bHeldDown) ? frameTime : 0.020f;
    CKey key(m_DefaultIR_Remote.wButtons, 0, 0, 0, 0, 0, 0, time);
    return OnKey(key);
  }
#endif
  return false;
}

bool CApplication::ProcessMouse()
{
  if (!g_Mouse.IsActive())
    return false;
  // Reset the screensaver and idle timers
  m_idleTimer.StartZero();
  ResetScreenSaver();
  if (ResetScreenSaverWindow())
    return true;

  int actionID = ACTION_MOUSE_MOVE;
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
    actionID = ACTION_MOUSE_LEFT_CLICK;
  else if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
    actionID = ACTION_MOUSE_RIGHT_CLICK;
  else if (g_Mouse.bClick[MOUSE_MIDDLE_BUTTON])
    actionID = ACTION_MOUSE_MIDDLE_CLICK;
  else if (g_Mouse.bDoubleClick[MOUSE_LEFT_BUTTON])
    actionID = ACTION_MOUSE_DOUBLE_CLICK;
  else if (g_Mouse.bHold[MOUSE_LEFT_BUTTON])
    actionID = ACTION_MOUSE_DRAG;
  else if (g_Mouse.GetWheel() > 0)
    actionID = ACTION_MOUSE_WHEEL_UP;
  else if (g_Mouse.GetWheel() < 0)
    actionID = ACTION_MOUSE_WHEEL_DOWN;

  CAction action(actionID, (unsigned int)g_Mouse.bHold[MOUSE_LEFT_BUTTON], g_Mouse.GetLocation().x, g_Mouse.GetLocation().y, g_Mouse.GetLastMove().x, g_Mouse.GetLastMove().y);
  return g_windowManager.OnAction(action);
}

void  CApplication::CheckForTitleChange()
{ 
  if (CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
  {
    if (IsPlayingVideo())
    {
      const CVideoInfoTag* tagVal = g_infoManager.GetCurrentMovieTag();
      if (m_pXbmcHttp && tagVal && !(tagVal->m_strTitle.IsEmpty()))
      {
        CStdString msg=m_pXbmcHttp->GetOpenTag()+"MovieTitle:"+tagVal->m_strTitle+m_pXbmcHttp->GetCloseTag();
        if (m_prevMedia!=msg && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
        {
          CApplicationMessenger::Get().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
          m_prevMedia=msg;
        }
      }
    }
    else if (IsPlayingAudio())
    {
      const CMusicInfoTag* tagVal=g_infoManager.GetCurrentSongTag();
      if (m_pXbmcHttp && tagVal)
      {
        CStdString msg="";
        if (!tagVal->GetTitle().IsEmpty())
          msg=m_pXbmcHttp->GetOpenTag()+"AudioTitle:"+tagVal->GetTitle()+m_pXbmcHttp->GetCloseTag();
        if (!tagVal->GetArtist().empty())
          msg+=m_pXbmcHttp->GetOpenTag()+"AudioArtist:"+StringUtils::Join(tagVal->GetArtist(), g_advancedSettings.m_musicItemSeparator)+m_pXbmcHttp->GetCloseTag();
        if (m_prevMedia!=msg)
        {
          CApplicationMessenger::Get().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
          m_prevMedia=msg;
        }
      }
    }
  }
}

bool CApplication::ProcessHTTPApiButtons()
{
  if (m_pXbmcHttp)
  {
    // copy key from webserver, and reset it in case we're called again before
    // whatever happens in OnKey()
    CKey keyHttp(m_pXbmcHttp->GetKey());
    m_pXbmcHttp->ResetKey();
    if (keyHttp.GetButtonCode() != KEY_INVALID)
    {
      if (keyHttp.GetButtonCode() == KEY_VMOUSE) //virtual mouse
      {
        g_Mouse.SetLocation(CPoint(keyHttp.GetLeftThumbX(), keyHttp.GetLeftThumbY()));
        int actionID = ACTION_MOUSE_MOVE;
        if (keyHttp.GetLeftTrigger() == 1)
          actionID = ACTION_MOUSE_LEFT_CLICK;
        else if (keyHttp.GetLeftTrigger() == 2)
          actionID = ACTION_MOUSE_RIGHT_CLICK;
        else if (keyHttp.GetLeftTrigger() == 3)
          actionID = ACTION_MOUSE_MIDDLE_CLICK;
        else if (keyHttp.GetRightTrigger() == 1)
          actionID = ACTION_MOUSE_DOUBLE_CLICK;
        CAction action(actionID, keyHttp.GetLeftThumbX(), keyHttp.GetLeftThumbY());
        g_windowManager.OnAction(action);
      }
      else
        OnKey(keyHttp);
      return true;
    }
  }
  return false;
}

bool CApplication::ProcessEventServer(float frameTime)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients()==0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    m_idleTimer.StartZero();
    ResetScreenSaver();
    ResetScreenSaverWindow();
  }

  // now handle any buttons or axis
  std::string joystickName;
  bool isAxis = false;
  float fAmount = 0.0;

  WORD wKeyID = es->GetButtonCode(joystickName, isAxis, fAmount);

  if (wKeyID)
  {
    if (joystickName.length() > 0)
    {
      if (isAxis == true)
      {
        if (fabs(fAmount) >= 0.08)
          m_lastAxisMap[joystickName][wKeyID] = fAmount;
        else
          m_lastAxisMap[joystickName].erase(wKeyID);
      }

      return ProcessJoystickEvent(joystickName, wKeyID, isAxis, fAmount);
    }
    else
    {
      CKey key;
      if(wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, (BYTE)(255*fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, (BYTE)(255*fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0,  fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0,  fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0,  fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0,  fAmount, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      return OnKey(key);
    }
  }

  if (m_lastAxisMap.size() > 0)
  {
    // Process all the stored axis.
    for (map<std::string, map<int, float> >::iterator iter = m_lastAxisMap.begin(); iter != m_lastAxisMap.end(); ++iter)
    {
      for (map<int, float>::iterator iterAxis = (*iter).second.begin(); iterAxis != (*iter).second.end(); ++iterAxis)
        ProcessJoystickEvent((*iter).first, (*iterAxis).first, true, (*iterAxis).second);
    }
  }

  {
    CPoint pos;
    if (es->GetMousePos(pos.x, pos.y) && g_Mouse.IsEnabled())
    {
      g_Mouse.SetLocation(pos, true);
      return g_windowManager.OnAction(CAction(ACTION_MOUSE_MOVE, pos.x, pos.y));
    }
  }
#endif
  return false;
}

bool CApplication::ProcessJoystickEvent(const std::string& joystickName, int wKeyID, bool isAxis, float fAmount, unsigned int holdTime /*=0*/)
{
#ifdef HAS_EVENT_SERVER
  m_idleTimer.StartZero();

   // Make sure to reset screen saver, mouse.
   ResetScreenSaver();
   if (ResetScreenSaverWindow())
     return true;

#ifdef HAS_SDL_JOYSTICK
   g_Joystick.Reset();
#endif
   g_Mouse.SetInactive();

   // Figure out what window we're taking the event for.
   int iWin = g_windowManager.GetActiveWindow() & WINDOW_ID_MASK;
   if (g_windowManager.HasModalDialog())
       iWin = g_windowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;

   // This code is copied from the OnKey handler, it should be factored out.
   if (iWin == WINDOW_FULLSCREEN_VIDEO &&
       g_application.m_pPlayer &&
       g_application.m_pPlayer->IsInMenu())
   {
     // If player is in some sort of menu, (ie DVDMENU) map buttons differently.
     iWin = WINDOW_VIDEO_MENU;
   }

   int actionID;
   CStdString actionName;
   bool fullRange = false;

   // Translate using regular joystick translator.
   if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, joystickName.c_str(), wKeyID, isAxis, actionID, actionName, fullRange))
   {
     CAction action(actionID, fAmount, 0.0f, actionName, holdTime);
     bool bResult = false;

     // play sound before the action unless the button is held, 
     // where we execute after the action as held actions aren't fired every time.
     if(action.GetHoldTime())
     {
       bResult = OnAction(action);
       if(bResult)
         g_audioManager.PlayActionSound(action);
     }
     else
     {
       g_audioManager.PlayActionSound(action);
       bResult = OnAction(action);
     }

     return bResult;
   }
   else
   {
     CLog::Log(LOGDEBUG, "ERROR mapping joystick action");
   }
#endif

   return false;
}

bool CApplication::ProcessKeyboard()
{
  // process the keyboard buttons etc.
  BYTE vkey = g_Keyboard.GetKey();
  WCHAR unicode = g_Keyboard.GetUnicode();
  if (vkey || unicode)
  {
    // got a valid keypress - convert to a key code
    int keyID;
    if (vkey) // FIXME, every ascii has a vkey so vkey would always and ascii would never be processed, but fortunately OnKey uses wkeyID only to detect keyboard use and the real key is recalculated correctly.
      keyID = vkey | KEY_VKEY;
    else
      keyID = KEY_UNICODE;
    //  CLog::Log(LOGDEBUG,"Keyboard: time=%i key=%i", CTimeUtils::GetFrameTime(), vkey);
    CKey key(keyID);
    key.SetHeld(g_Keyboard.KeyHeld());
    return OnKey(key);
  }
  return false;
}

bool CApplication::IsButtonDown(DWORD code)
{
#ifdef HAS_GAMEPAD
  if (code >= KEY_BUTTON_A && code <= KEY_BUTTON_RIGHT_TRIGGER)
  {
    // analogue
    return (m_DefaultGamepad.bAnalogButtons[code - KEY_BUTTON_A + XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK);
  }
  else if (code >= KEY_BUTTON_DPAD_UP && code <= KEY_BUTTON_RIGHT_THUMB_BUTTON)
  {
    // digital
    return (m_DefaultGamepad.wButtons & (1 << (code - KEY_BUTTON_DPAD_UP))) != 0;
  }
  else
  {
    // remote
    return m_DefaultIR_Remote.wButtons == code;
  }
#endif
  return false;
}

bool CApplication::AnyButtonDown()
{
  ReadInput();
#ifdef HAS_GAMEPAD
  if (m_DefaultGamepad.wPressedButtons || m_DefaultIR_Remote.wButtons)
    return true;

  for (int i = 0; i < 6; ++i)
  {
    if (m_DefaultGamepad.bPressedAnalogButtons[i])
      return true;
  }
#endif
  return false;
}

HRESULT CApplication::Cleanup()
{
  try
  {
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST);
    g_windowManager.Delete(WINDOW_MUSIC_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_MUSIC_FILES);
    g_windowManager.Delete(WINDOW_MUSIC_NAV);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_INFO);
    g_windowManager.Delete(WINDOW_VIDEO_FILES);
    g_windowManager.Delete(WINDOW_VIDEO_PLAYLIST);
    g_windowManager.Delete(WINDOW_VIDEO_NAV);
    g_windowManager.Delete(WINDOW_FILES);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_YES_NO);
    g_windowManager.Delete(WINDOW_DIALOG_PROGRESS);
    g_windowManager.Delete(WINDOW_DIALOG_NUMERIC);
    g_windowManager.Delete(WINDOW_DIALOG_GAMEPAD);
    g_windowManager.Delete(WINDOW_DIALOG_SUB_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_CONTEXT_MENU);
    g_windowManager.Delete(WINDOW_DIALOG_PLAYER_CONTROLS);
    g_windowManager.Delete(WINDOW_DIALOG_MUSIC_OSD);
    g_windowManager.Delete(WINDOW_DIALOG_VIS_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_VIS_PRESET_LIST);
    g_windowManager.Delete(WINDOW_DIALOG_SELECT);
    g_windowManager.Delete(WINDOW_DIALOG_OK);
    g_windowManager.Delete(WINDOW_DIALOG_FILESTACKING);
    g_windowManager.Delete(WINDOW_DIALOG_KEYBOARD);
    g_windowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
    g_windowManager.Delete(WINDOW_DIALOG_TRAINER_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_PROFILE_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_LOCK_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_NETWORK_SETUP);
    g_windowManager.Delete(WINDOW_DIALOG_MEDIA_SOURCE);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    g_windowManager.Delete(WINDOW_DIALOG_CONTENT_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_FAVOURITES);
    g_windowManager.Delete(WINDOW_DIALOG_SONG_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    g_windowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    g_windowManager.Delete(WINDOW_DIALOG_BUSY);
    g_windowManager.Delete(WINDOW_DIALOG_PICTURE_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_ADDON_INFO);
    g_windowManager.Delete(WINDOW_DIALOG_ADDON_SETTINGS);
    g_windowManager.Delete(WINDOW_DIALOG_SLIDER);
    g_windowManager.Delete(WINDOW_DIALOG_MEDIA_FILTER);
    g_windowManager.Delete(WINDOW_DIALOG_TEXT_VIEWER);

    g_windowManager.Delete(WINDOW_STARTUP_ANIM);
    g_windowManager.Delete(WINDOW_LOGIN_SCREEN);
    g_windowManager.Delete(WINDOW_VISUALISATION);
    g_windowManager.Delete(WINDOW_SETTINGS_MENU);
    g_windowManager.Delete(WINDOW_SETTINGS_PROFILES);
    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);  // all the settings categories
    g_windowManager.Delete(WINDOW_SCREEN_CALIBRATION);
    g_windowManager.Delete(WINDOW_SYSTEM_INFORMATION);
    g_windowManager.Delete(WINDOW_SCREENSAVER);
    g_windowManager.Delete(WINDOW_DIALOG_VIDEO_OSD);
    g_windowManager.Delete(WINDOW_SLIDESHOW);
    g_windowManager.Delete(WINDOW_SKIN_SETTINGS);

    g_windowManager.Delete(WINDOW_HOME);
    g_windowManager.Delete(WINDOW_PROGRAMS);
    g_windowManager.Delete(WINDOW_PICTURES);
    g_windowManager.Delete(WINDOW_SCRIPTS);
    g_windowManager.Delete(WINDOW_GAMESAVES);
    g_windowManager.Delete(WINDOW_WEATHER);

    g_windowManager.Delete(WINDOW_SETTINGS_MYPICTURES);
    g_windowManager.Remove(WINDOW_SETTINGS_MYPROGRAMS);
    g_windowManager.Remove(WINDOW_SETTINGS_MYWEATHER);
    g_windowManager.Remove(WINDOW_SETTINGS_MYMUSIC);
    g_windowManager.Remove(WINDOW_SETTINGS_SYSTEM);
    g_windowManager.Remove(WINDOW_SETTINGS_MYVIDEOS);
    g_windowManager.Remove(WINDOW_SETTINGS_SERVICE);
    g_windowManager.Remove(WINDOW_SETTINGS_APPEARANCE);
    g_windowManager.Remove(WINDOW_DIALOG_KAI_TOAST);

    g_windowManager.Remove(WINDOW_DIALOG_SEEK_BAR);
    g_windowManager.Remove(WINDOW_DIALOG_VOLUME_BAR);

    CAddonMgr::Get().DeInit();

    CLog::Log(LOGNOTICE, "unload sections");
    CSectionLoader::UnloadAll();
    // reset our d3d params before we destroy
    g_graphicsContext.SetD3DDevice(NULL);
    g_graphicsContext.SetD3DParameters(NULL);

#ifdef _DEBUG
    //  Shutdown as much as possible of the
    //  application, to reduce the leaks dumped
    //  to the vc output window before calling
    //  _CrtDumpMemoryLeaks(). Most of the leaks
    //  shown are no real leaks, as parts of the app
    //  are still allocated.

    g_localizeStrings.Clear();
    g_LangCodeExpander.Clear();
    g_charsetConverter.clear();
    g_directoryCache.Clear();
    CButtonTranslator::GetInstance().Clear();
#ifdef HAS_EVENT_SERVER
    CEventServer::RemoveInstance();
#endif
    g_infoManager.Clear();
    DllLoaderContainer::Clear();
    g_playlistPlayer.Clear();
    CSettings::Get().Uninitialize();
    g_advancedSettings.Clear();

#ifdef HAS_DVD_DRIVE
    CLibcdio::ReleaseInstance();
#endif
#endif
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif
    return S_OK;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return E_FAIL;
  }
}

void CApplication::Stop(bool bLCDStop)
{
  try
  {
    SaveFileState(true);

    if (m_pXbmcHttp)
    {
      if(CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
        CApplicationMessenger::Get().HttpApi("broadcastlevel; ShutDown;1");

      m_pXbmcHttp->shuttingDown=true;
      //Sleep(100);
    }

    CLog::Log(LOGNOTICE, "Storing total System Uptime");
    g_sysinfo.SetTotalUptime(g_sysinfo.GetTotalUptime() + (int)(CTimeUtils::GetFrameTime() / 60000));

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(CProfilesManager::Get().GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      CSettings::Get().Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    m_bStop = true;
    CLog::Log(LOGNOTICE, "stop all");

    // stop scanning before we kill the network and so on
    if (m_musicInfoScanner->IsScanning())
      m_musicInfoScanner->Stop();

    if (m_videoInfoScanner->IsScanning())
      m_videoInfoScanner->Stop();

    StopServices();
    //Sleep(5000);

#ifdef __APPLE__
    g_xbmcHelper.ReleaseAllInput();
#endif

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stop player");
      delete m_pPlayer;
      m_pPlayer = NULL;
    }

#ifdef HAS_FILESYSTEM
    CLog::Log(LOGNOTICE, "stop daap clients");
    g_DaapClient.Release();
#endif
    //g_lcd->StopThread();
    CApplicationMessenger::Get().Cleanup();

    CLog::Log(LOGNOTICE, "clean cached files!");
    g_RarManager.ClearCache(true);

    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();

    // Stop services before unloading Python
    CAddonMgr::Get().StopServices(false);

    CLog::Log(LOGNOTICE, "stop python");
    g_pythonParser.FreeResources();

#ifdef HAS_LCD
    if (g_lcd && bLCDStop)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd=NULL;
    }
#endif
    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
  }

  Destroy();
}

bool CApplication::PlayMedia(const CFileItem& item, int iPlaylist)
{
  //If item is a plugin, expand out now and run ourselves again
  if (item.IsPlugin())
  {
    CFileItem item_new(item);
    if (XFILE::CPluginDirectory::GetPluginResult(item.GetPath(), item_new))
      return PlayMedia(item_new, iPlaylist);
    return false;
  }
  if (item.IsSmartPlayList())
  {
    CDirectory dir;
    CFileItemList items;
    if (dir.GetDirectory(item.GetPath(), items) && items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.GetURL());
      CPlayList playlist;
      playlist.Add(items);
      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, (smartpl.GetType() == "songs" || smartpl.GetType() == "albums") ? PLAYLIST_MUSIC:PLAYLIST_VIDEO);
    }
  }
  else if (item.IsPlayList() || item.IsInternetStream())
  {
    //is or could be a playlist
    auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
    if (pPlayList.get() && pPlayList->Load(item.GetPath()))
    {
      if (iPlaylist != PLAYLIST_NONE)
      {
        int track=0;
        if (item.HasProperty("playlist_starting_track"))
          track = item.GetProperty("playlist_starting_track").asInteger();
        return ProcessAndStartPlaylist(item.GetPath(), *pPlayList, iPlaylist, track);
      }
      else
      {
        CLog::Log(LOGWARNING, "CApplication::PlayMedia called to play a playlist %s but no idea which playlist to use, playing first item", item.GetPath().c_str());
        if(pPlayList->size())
          return PlayFile(*(*pPlayList)[0], false);
      }
    }
  }

  //nothing special just play
  return PlayFile(item, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
bool CApplication::PlayStack(const CFileItem& item, bool bRestart)
{
  if (!item.IsStack())
    return false;

  CVideoDatabase dbs;

  // case 1: stacked ISOs
  if (CFileItem(CStackDirectory::GetFirstStackedFile(item.GetPath()),false).IsDVDImage())
  {
    CStackDirectory dir;
    CFileItemList movieList;
    dir.GetDirectory(item.GetURL(), movieList);

    // first assume values passed to the stack
    int selectedFile = item.m_lStartPartNumber;
    int startoffset = item.m_lStartOffset;

    // check if we instructed the stack to resume from default
    if (startoffset == STARTOFFSET_RESUME) // selected file is not specified, pick the 'last' resume point
    {
      if (dbs.Open())
      {
        CBookmark bookmark;
        if (dbs.GetResumeBookMark(item.GetPath(), bookmark))
        {
          startoffset = (int)(bookmark.timeInSeconds*75);
          selectedFile = bookmark.partNumber;
        }
        dbs.Close();
      }
      else
        CLog::Log(LOGERROR, "%s - Cannot open VideoDatabase", __FUNCTION__);
    }

    // make sure that the selected part is within the boundaries
    if (selectedFile <= 0)
    {
      CLog::Log(LOGWARNING, "%s - Selected part %d out of range, playing part 1", __FUNCTION__, selectedFile);
      selectedFile = 1;
    }
    else if (selectedFile > movieList.Size())
    {
      CLog::Log(LOGWARNING, "%s - Selected part %d out of range, playing part %d", __FUNCTION__, selectedFile, movieList.Size());
      selectedFile = movieList.Size();
    }

    // set startoffset in movieitem, track stack item for updating purposes, and finally play disc part
    movieList[selectedFile - 1]->m_lStartOffset = startoffset > 0 ? STARTOFFSET_RESUME : 0;
    movieList[selectedFile - 1]->SetProperty("stackFileItemToUpdate", true);
    *m_stackFileItemToUpdate = item;
    return PlayFile(*(movieList[selectedFile - 1]));
  }
  // case 2: all other stacks
  else
  {
    // see if we have the info in the database
    // TODO: If user changes the time speed (FPS via framerate conversion stuff)
    //       then these times will be wrong.
    //       Also, this is really just a hack for the slow load up times we have
    //       A much better solution is a fast reader of FPS and fileLength
    //       that we can use on a file to get it's time.
    vector<int> times;
    bool haveTimes(false);
    CVideoDatabase dbs;
    if (dbs.Open())
    {
      dbs.GetVideoSettings(item.GetPath(), CMediaSettings::Get().GetCurrentVideoSettings());
      haveTimes = dbs.GetStackTimes(item.GetPath(), times);
      dbs.Close();
    }

    // calculate the total time of the stack
    CStackDirectory dir;
    dir.GetDirectory(item.GetURL(), *m_currentStack);
    long totalTime = 0;
    for (int i = 0; i < m_currentStack->Size(); i++)
    {
      if (haveTimes)
        (*m_currentStack)[i]->m_lEndOffset = times[i];
      else
      {
        int duration;
        if (!CDVDFileInfo::GetFileDuration((*m_currentStack)[i]->GetPath(), duration))
        {
          m_currentStack->Clear();
          return false;
        }
        totalTime += duration / 1000;
        (*m_currentStack)[i]->m_lEndOffset = totalTime;
        times.push_back(totalTime);
      }
    }

    double seconds = item.m_lStartOffset / 75.0;

    if (!haveTimes || item.m_lStartOffset == STARTOFFSET_RESUME )
    {  // have our times now, so update the dB
      if (dbs.Open())
      {
        if( !haveTimes )
          dbs.SetStackTimes(item.GetPath(), times);

        if( item.m_lStartOffset == STARTOFFSET_RESUME )
        {
          // can only resume seek here, not dvdstate
          CBookmark bookmark;
          if( dbs.GetResumeBookMark(item.GetPath(), bookmark) )
            seconds = bookmark.timeInSeconds;
          else
            seconds = 0.0f;
        }
        dbs.Close();
      }
    }      

    *m_itemCurrentFile = item;
    m_currentStackPosition = 0;
    m_eCurrentPlayer = EPC_NONE; // must be reset on initial play otherwise last player will be used

    if (seconds > 0)
    {
      // work out where to seek to
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if (seconds < (*m_currentStack)[i]->m_lEndOffset)
        {
          CFileItem item(*(*m_currentStack)[i]);
          long start = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          item.m_lStartOffset = (long)(seconds - start) * 75;
          m_currentStackPosition = i;
          return PlayFile(item, true);
        }
      }
    }

    return PlayFile(*(*m_currentStack)[0], true);
  }
  return false;
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (!bRestart)
  {
    SaveCurrentFileSettings();

    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    CMediaSettings::Get().GetCurrentVideoSettings() = CMediaSettings::Get().GetDefaultVideoSettings();
    // see if we have saved options in the database

    m_iPlaySpeed = 1;
    *m_itemCurrentFile = item;
    m_nextPlaylistItem = -1;
    m_currentStackPosition = 0;
    m_currentStack->Clear();
   
    if (item.IsVideo())
      CUtil::ClearSubtitles();
  }

  if (item.IsPlayList())
    return false;

  if (item.IsPlugin())
  { // we modify the item so that it becomes a real URL
    CFileItem item_new(item);
    if (XFILE::CPluginDirectory::GetPluginResult(item.GetPath(), item_new))
      return PlayFile(item_new, false);
    return false;
  }

  if (URIUtils::IsUPnP(item.GetPath()))
  {
    CFileItem item_new(item);
    if (XFILE::CUPnPDirectory::GetResource(item.GetURL(), item_new))
      return PlayFile(item_new, false);
    return false;
  }

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  //Is TuxBox, this should probably be moved to CFileTuxBox
  if(item.IsTuxBox())
  {
    CLog::Log(LOGDEBUG, "%s - TuxBox URL Detected %s",__FUNCTION__, item.GetPath().c_str());

    if(g_tuxboxService.IsRunning())
      g_tuxboxService.Stop();

    CFileItem item_new;
    if(g_tuxbox.CreateNewItem(item, item_new))
    {

      // Make sure it doesn't have a player
      // so we actually select one normally
      m_eCurrentPlayer = EPC_NONE;

      // keep the tuxbox:// url as playing url
      // and give the new url to the player
      if(PlayFile(item_new, true))
      {
        if(!g_tuxboxService.IsRunning())
          g_tuxboxService.Start();
        return true;
      }
    }
    return false;
  }

  CPlayerOptions options;
  
  if( item.HasProperty("StartPercent") )
  {
    options.startpercent = item.GetProperty("StartPercent").asDouble();
  }
  
  PLAYERCOREID eNewCore = EPC_NONE;
  if( bRestart )
  {
    // have to be set here due to playstack using this for starting the file
    options.starttime = item.m_lStartOffset / 75.0;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_itemCurrentFile->m_lStartOffset != 0)
      m_itemCurrentFile->m_lStartOffset = STARTOFFSET_RESUME; // to force fullscreen switching

    if( m_eForcedNextPlayer != EPC_NONE )
      eNewCore = m_eForcedNextPlayer;
    else if( m_eCurrentPlayer == EPC_NONE )
      eNewCore = CPlayerCoreFactory::Get().GetDefaultPlayer(item);
    else
      eNewCore = m_eCurrentPlayer;
  }
  else
  {
    options.starttime = item.m_lStartOffset / 75.0;

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();
      dbs.GetVideoSettings(item.GetPath(), CMediaSettings::Get().GetCurrentVideoSettings());

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        options.starttime = 0.0f;
        CBookmark bookmark;
        CStdString path = item.GetPath();
        if (item.IsDVD()) 
          path = item.GetVideoInfoTag()->m_strFileNameAndPath;
        if(dbs.GetResumeBookMark(path, bookmark))
        {
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId != -1 && tag->m_iBookmarkId != 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }

    if (m_eForcedNextPlayer != EPC_NONE)
      eNewCore = m_eForcedNextPlayer;
    else
      eNewCore = CPlayerCoreFactory::Get().GetDefaultPlayer(item);
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  int playlist = g_playlistPlayer.GetCurrentPlaylist();
  if (playlist == PLAYLIST_VIDEO && g_playlistPlayer.GetPlaylist(playlist).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !g_playlistPlayer.HasPlayedFirstFile() && g_advancedSettings.m_fullScreenOnMovieStart && !CMediaSettings::Get().DoesVideoStartWindowed();
  }
  else if(m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
  {
    // TODO - this will fail if user seeks back to first file in stack
    if(m_currentStackPosition == 0 || m_itemCurrentFile->m_lStartOffset == STARTOFFSET_RESUME)
      options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !CMediaSettings::Get().DoesVideoStartWindowed();
    else
      options.fullscreen = false;
    // reset this so we don't think we are resuming on seek
    m_itemCurrentFile->m_lStartOffset = 0;
  }
  else
    options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !CMediaSettings::Get().DoesVideoStartWindowed();

  // reset VideoStartWindowed as it's a temp setting
  CMediaSettings::Get().SetVideoStartWindowed(false);
  // reset any forced player
  m_eForcedNextPlayer = EPC_NONE;

#ifdef HAS_KARAOKE
  //We have to stop parsing a cdg before mplayer is deallocated
  // WHY do we have to do this????
  if(m_pCdgParser)
    m_pCdgParser->Stop();
#endif

  // tell system we are starting a file
  m_bPlaybackStarting = true;

  // We should restart the player, unless the previous and next tracks are using
  // one of the players that allows gapless playback (paplayer, dvdplayer)
  if (m_pPlayer)
  {
    if ( !(m_eCurrentPlayer == eNewCore && (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer  == EPC_PAPLAYER)) )
    {
      delete m_pPlayer;
      m_pPlayer = NULL;
    }
  }

  if (!m_pPlayer)
  {
    m_eCurrentPlayer = eNewCore;
    m_pPlayer = CPlayerCoreFactory::Get().CreatePlayer(eNewCore, *this);
  }

  bool bResult;
  if (m_pPlayer)
  {
    // don't hold graphicscontext here since player
    // may wait on another thread, that requires gfx
    CSingleExit ex(g_graphicsContext);
    bResult = m_pPlayer->OpenFile(item, options);
  }
  else
  {
    CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", item.GetPath().c_str());
    bResult = false;
  }

  if(bResult)
  {
    if (m_iPlaySpeed != 1)
    {
      int iSpeed = m_iPlaySpeed;
      m_iPlaySpeed = 1;
      SetPlaySpeed(iSpeed);
    }

    if( IsPlayingAudio() )
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
        g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
    }

#ifdef HAS_VIDEO_PLAYBACK
    if( IsPlayingVideo() )
    {
      if (g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
        g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);

      // if player didn't manange to switch to fullscreen by itself do it here
      if( options.fullscreen && g_renderManager.IsStarted()
       && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
       SwitchToFullScreen();

      if (!item.IsDVDImage() && !item.IsDVDFile())
      {
        CVideoInfoTag *details = m_itemCurrentFile->GetVideoInfoTag();
        // Save information about the stream if we currently have no data
        if (!details->HasStreamDetails() ||
             details->m_streamDetails.GetVideoDuration() <= 0)
        {
          if (m_pPlayer->GetStreamDetails(details->m_streamDetails) && details->HasStreamDetails())
          {
            CVideoDatabase dbs;
            dbs.Open();
            dbs.SetStreamDetailsForFileId(details->m_streamDetails, details->m_iFileId);
            dbs.Close();
            CUtil::DeleteVideoDatabaseDirectoryCache();
          }
        }
      }
    }
#endif

    g_audioManager.Enable(false);
  }
  m_bPlaybackStarting = false;
  if(bResult)
  {
    // we must have started, otherwise player might send this later
    if(IsPlaying())
      OnPlayBackStarted();
    else
      OnPlayBackEnded();
  }
  else
  {
    // we send this if it isn't playlistplayer that is doing this
    int next = g_playlistPlayer.GetNextSong();
    int size = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).size();
    if(next < 0
    || next >= size)
      OnPlayBackStopped();
  }

  return bResult;
}

void CApplication::OnPlayBackEnded()
{
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackEnded();
  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnPlayBackEnded;1");

  CLog::Log(LOGDEBUG, "%s - Playback has finished", __FUNCTION__);

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStarted()
{
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnPlayBackStarted;1");

  CLog::Log(LOGDEBUG, "%s - Playback has started", __FUNCTION__);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnQueueNextItem()
{
  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
  g_pythonParser.OnQueueNextItem(); // currently unimplemented

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
  CApplicationMessenger::Get().HttpApi("broadcastlevel; OnQueueNextItem;1");

  CLog::Log(LOGDEBUG, "Player has asked for the next item");

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStopped();

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnPlayBackStopped;1");

  CLog::Log(LOGDEBUG, "%s - Playback was stopped", __FUNCTION__);

  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
  g_windowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackPaused()
{
  g_pythonParser.OnPlayBackPaused();

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnPlayBackPaused;1");

  CLog::Log(LOGDEBUG, "%s - Playback was paused", __FUNCTION__);
}

void CApplication::OnPlayBackResumed()
{
  g_pythonParser.OnPlayBackResumed();

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    CApplicationMessenger::Get().HttpApi("broadcastlevel; OnPlayBackResumed;1");

  CLog::Log(LOGDEBUG, "%s - Playback was resumed", __FUNCTION__);
}

void CApplication::OnPlayBackSpeedChanged(int iSpeed)
{
  g_pythonParser.OnPlayBackSpeedChanged(iSpeed);

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSpeedChanged:%i;1",iSpeed);
    CApplicationMessenger::Get().HttpApi(tmp);
  }

  CLog::Log(LOGDEBUG, "%s - Playback speed changed", __FUNCTION__);
}

void CApplication::OnPlayBackSeek(int iTime, int seekOffset)
{
  g_pythonParser.OnPlayBackSeek(iTime, seekOffset);

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSeek:%i;1",iTime);
    CApplicationMessenger::Get().HttpApi(tmp);
  }

  CLog::Log(LOGDEBUG, "%s - Playback skip", __FUNCTION__);
//  g_infoManager.SetDisplayAfterSeek(2500, seekOffset/1000);
}

void CApplication::OnPlayBackSeekChapter(int iChapter)
{
  g_pythonParser.OnPlayBackSeekChapter(iChapter);

  // Let's tell the outside world as well
  if (m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
  {
    CStdString tmp;
    tmp.Format("broadcastlevel; OnPlayBackSkeekChapter:%i;1",iChapter);
    CApplicationMessenger::Get().HttpApi(tmp);
  }

  CLog::Log(LOGDEBUG, "%s - Playback skip", __FUNCTION__);
}

bool CApplication::IsPlaying() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return true;
}

bool CApplication::IsPaused() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return m_pPlayer->IsPaused();
}

bool CApplication::IsPlayingAudio() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return false;
  if (m_pPlayer->HasAudio())
    return true;
  return false;
}

bool CApplication::IsPlayingVideo() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return true;

  return false;
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  return IsPlayingVideo() && g_graphicsContext.IsFullScreenVideo();
}

bool CApplication::IsFullScreen()
{
  return IsPlayingFullScreenVideo() ||
        (g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION) ||
         g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW;
}

void CApplication::SaveFileState(bool bForeground /* = false */)
{
  if (!CProfilesManager::Get().GetCurrentProfile().canWriteDatabases())
    return;

  if (bForeground)
  {
    CSaveFileStateJob job(*m_progressTrackingItem,
    *m_stackFileItemToUpdate,
    m_progressTrackingVideoResumeBookmark,
    m_progressTrackingPlayCountUpdate);

    // Run job in the foreground to make sure it finishes
    job.DoWork();
  }
  else
  {
    CJob* job = new CSaveFileStateJob(*m_progressTrackingItem,
        *m_stackFileItemToUpdate,
        m_progressTrackingVideoResumeBookmark,
        m_progressTrackingPlayCountUpdate);
    CJobManager::GetInstance().AddJob(job, NULL);
  }
}

void CApplication::UpdateFileState()
{
  // Did the file change?
  if (m_progressTrackingItem->GetPath() != "" && m_progressTrackingItem->GetPath() != CurrentFile())
  {
    SaveFileState();

    // Reset tracking item
    m_progressTrackingItem->Reset();
  }
  else
  {
    if (IsPlayingVideo() || IsPlayingAudio())
    {
      if (m_progressTrackingItem->GetPath() == "")
      {
        // Init some stuff
        *m_progressTrackingItem = CurrentFileItem();
        m_progressTrackingPlayCountUpdate = false;
      }

      if ((m_progressTrackingItem->IsAudio() && g_advancedSettings.m_audioPlayCountMinimumPercent > 0 &&
          GetPercentage() >= g_advancedSettings.m_audioPlayCountMinimumPercent) ||
          (m_progressTrackingItem->IsVideo() && g_advancedSettings.m_videoPlayCountMinimumPercent > 0 &&
          GetPercentage() >= g_advancedSettings.m_videoPlayCountMinimumPercent))
      {
        m_progressTrackingPlayCountUpdate = true;
      }

      if (m_progressTrackingItem->IsVideo())
      {
        if ((m_progressTrackingItem->IsDVDImage() ||
             m_progressTrackingItem->IsDVDFile()    ) &&
            m_pPlayer->GetTotalTime() > 15*60)

        {
          m_progressTrackingItem->GetVideoInfoTag()->m_streamDetails.Reset();
          m_pPlayer->GetStreamDetails(m_progressTrackingItem->GetVideoInfoTag()->m_streamDetails);
        }
        // Update bookmark for save
        m_progressTrackingVideoResumeBookmark.player = CPlayerCoreFactory::Get().GetPlayerName(m_eCurrentPlayer);
        m_progressTrackingVideoResumeBookmark.playerState = m_pPlayer->GetPlayerState();
        m_progressTrackingVideoResumeBookmark.thumbNailImage.Empty();

        if (g_advancedSettings.m_videoIgnorePercentAtEnd > 0 &&
            GetTotalTime() - GetTime() < 0.01f * g_advancedSettings.m_videoIgnorePercentAtEnd * GetTotalTime())
        {
          // Delete the bookmark
          m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
        }
        else
        if (GetTime() > g_advancedSettings.m_videoIgnoreSecondsAtStart)
        {
          // Update the bookmark
          m_progressTrackingVideoResumeBookmark.timeInSeconds = GetTime();
          m_progressTrackingVideoResumeBookmark.totalTimeInSeconds = GetTotalTime();
        }
        else
        {
          // Do nothing
          m_progressTrackingVideoResumeBookmark.timeInSeconds = 0.0f;
        }
      }
    }
  }
}

void CApplication::StopPlaying()
{
  int iWin = g_windowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
#ifdef HAS_KARAOKE
    if( m_pCdgParser )
      m_pCdgParser->Stop();
#endif

    if (m_pPlayer)
      m_pPlayer->CloseFile();

    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION
    ||  iWin == WINDOW_FULLSCREEN_VIDEO)
      g_windowManager.PreviousWindow();

    g_partyModeManager.Disable();
  }
}

bool CApplication::NeedRenderFullScreen()
{
  if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    g_windowManager.UpdateModelessVisibility();

    if (g_windowManager.HasDialogOnScreen()) return true;
    if (g_Mouse.IsActive()) return true;

    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return false;
    return pFSWin->NeedRenderFullScreen();
  }
  return false;
}

void CApplication::RenderFullScreen()
{
  if (g_graphicsContext.IsFullScreenVideo())
  {
    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)g_windowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return ;
    pFSWin->RenderFullScreen();

    if (g_windowManager.HasDialogOnScreen())
      g_windowManager.RenderDialogs();
    // Render the mouse pointer, if visible...
    if (g_Mouse.IsActive())
      g_application.m_guiPointer.Render();
  }
}

void CApplication::ResetScreenSaver()
{
  // reset our timers
  m_shutdownTimer.StartZero();

  // screen saver timer is reset only if we're not already in screensaver mode
  if (!m_bScreenSave && m_iScreenSaveLock == 0)
    m_screenSaverTimer.StartZero();
}

bool CApplication::ResetScreenSaverWindow()
{
  if (m_iScreenSaveLock == 2)
    return false;

  // if Screen saver is active
  if (m_bScreenSave && m_screenSaver)
  {
    if (m_iScreenSaveLock == 0)
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          (CProfilesManager::Get().UsingLoginScreen() || CSettings::Get().GetBool("masterlock.startuplock")) &&
          CProfilesManager::Get().GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          m_screenSaver->ID() != "screensaver.xbmc.builtin.dim" && m_screenSaver->ID() != "screensaver.xbmc.builtin.black" && m_screenSaver->ID() != "visualization")
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);

        CGUIWindow* pWindow = g_windowManager.GetWindow(WINDOW_SCREENSAVER);
        if (pWindow)
          pWindow->OnMessage(msg);
      }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_bScreenSave = false;
    m_iScreenSaveLock = 0;
    m_screenSaverTimer.StartZero();

    float fFadeLevel = 1.0f;
    if (m_screenSaver->ID() == "visualization" || m_screenSaver->ID() == "screensaver.xbmc.builtin.slideshow")
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.dim")
    {
      if (!m_screenSaver->GetSetting("level").IsEmpty())
        fFadeLevel = 1.0f - 0.01f * (float)atof(m_screenSaver->GetSetting("level"));
    }
    else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.black")
    {
      fFadeLevel = 0;
    }
    else if (!m_screenSaver->ID().IsEmpty())
    { // we're in screensaver window
      if (g_windowManager.GetActiveWindow() == WINDOW_SCREENSAVER)
        g_windowManager.PreviousWindow();  // show the previous window
      return true;
    }

    // Fade to dim or black screensaver is active --> fade in
    D3DGAMMARAMP Ramp;
    for (float fade = fFadeLevel; fade <= 1; fade += 0.01f)
    {
      for (int i = 0;i < 256;i++)
      {
        Ramp.red[i] = (int)((float)m_OldRamp.red[i] * fade);
        Ramp.green[i] = (int)((float)m_OldRamp.green[i] * fade);
        Ramp.blue[i] = (int)((float)m_OldRamp.blue[i] * fade);
      }
      Sleep(5);
      m_pd3dDevice->SetGammaRamp(GAMMA_RAMP_FLAG, &Ramp); // use immediate to get a smooth fade
    }
    m_pd3dDevice->SetGammaRamp(0, &m_OldRamp); // put the old gamma ramp back in place
    return true;
  }
  else
    return false;
}

void CApplication::CheckScreenSaver()
{
  // if the screen saver window is active, then clearly we are already active
  if (g_windowManager.IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_bScreenSave = true;
    return;
  }

  bool resetTimer = false;
  if (IsPlayingVideo() && !m_pPlayer->IsPaused()) // are we playing video and it is not paused?
    resetTimer = true;

  if (IsPlayingAudio() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION) // are we playing some music in fullscreen vis?
    resetTimer = true;

  if (resetTimer)
  {
    m_screenSaverTimer.StartZero();
    return;
  }

  if (m_bScreenSave) // already running the screensaver
    return;

  if ( m_screenSaverTimer.GetElapsedSeconds() > CSettings::Get().GetInt("screensaver.time") * 60 )
    ActivateScreenSaver();
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  D3DGAMMARAMP Ramp;
  FLOAT fFadeLevel = 0;

  m_bScreenSave = true;

  // Get Screensaver Mode
  m_screenSaver.reset();
  if (!CAddonMgr::Get().GetAddon(CSettings::Get().GetString("screensaver.mode"), m_screenSaver))
    m_screenSaver.reset(new CScreenSaver(""));

  // disable screensaver lock from the login screen
  m_iScreenSaveLock = g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN ? 1 : 0;
  if (!forceType)
  {
    // set to Dim in the case of a dialog on screen or playing video
    if (g_windowManager.HasModalDialog() || (IsPlayingVideo() && CSettings::Get().GetBool("screensaver.usedimonpause")))
    {
      if (!CAddonMgr::Get().GetAddon("screensaver.xbmc.builtin.dim", m_screenSaver))
        m_screenSaver.reset(new CScreenSaver(""));
    }
    // Check if we are Playing Audio and Vis instead Screensaver!
    else if (IsPlayingAudio() && CSettings::Get().GetBool("screensaver.usemusicvisinstead") && !CSettings::Get().GetString("musicplayer.visualisation").empty())
    { // activate the visualisation
      m_screenSaver.reset(new CScreenSaver("visualization"));
      g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }
  // Picture slideshow
  if (m_screenSaver->ID() == "screensaver.xbmc.builtin.slideshow")
  {
    // reset our codec info - don't want that on screen
    g_infoManager.SetShowCodec(false);
    CStdString type = m_screenSaver->GetSetting("type");
    CStdString path = m_screenSaver->GetSetting("path");
    if (type == "2" && path.IsEmpty())
      type = "0";
    if (type == "0")
      path = "special://profile/thumbnails/Video/Fanart";
    if (type == "1")
      path = "special://profile/thumbnails/Music/Fanart";
    CApplicationMessenger::Get().PictureSlideShow(path, true, type != "2");
    return;
  }
  else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.dim")
  {
    if (!m_screenSaver->GetSetting("level").IsEmpty())
      fFadeLevel = 1.0f - 0.01f * (float)atof(m_screenSaver->GetSetting("level"));
  }
  else if (m_screenSaver->ID() == "screensaver.xbmc.builtin.black")
  {
    fFadeLevel = 0;
  }
  else if (!m_screenSaver->ID().IsEmpty())
  {
    g_windowManager.ActivateWindow(WINDOW_SCREENSAVER);
    return ;
  }
  
  // Fade to fFadeLevel
  m_pd3dDevice->GetGammaRamp(&m_OldRamp); // Store the old gamma ramp
  for (float fade = 1.f; fade >= fFadeLevel; fade -= 0.01f)
  {
    for (int i = 0;i < 256;i++)
    {
      Ramp.red[i] = (int)((float)m_OldRamp.red[i] * fade);
      Ramp.green[i] = (int)((float)m_OldRamp.green[i] * fade);
      Ramp.blue[i] = (int)((float)m_OldRamp.blue[i] * fade);
    }
    Sleep(5);
    m_pd3dDevice->SetGammaRamp(GAMMA_RAMP_FLAG, &Ramp); // use immediate to get a smooth fade
  }
}

void CApplication::CheckShutdown()
{
#ifdef HAS_XBOX_HARDWARE
  // first check if we should reset the timer
  bool resetTimer = false;
  if (IsPlaying()) // is something playing?
    resetTimer = true;

#ifdef HAS_FTP_SERVER
  if (CNetworkServices::Get().IsFtpServerRunning() && CNetworkServices::Get().FtpHasActiveConnections()) // is FTP active ?
    resetTimer = true;
#endif

  if (m_musicInfoScanner->IsScanning())
    resetTimer = true;

  if (m_videoInfoScanner->IsScanning())
    resetTimer = true;

  if (g_windowManager.IsWindowActive(WINDOW_DIALOG_PROGRESS)) // progress dialog is onscreen
    resetTimer = true;

  if (resetTimer)
  {
    m_shutdownTimer.StartZero();
    return;
  }

  if ( m_shutdownTimer.GetElapsedSeconds() > CSettings::Get().GetInt("powermanagement.shutdowntime") * 60 )
  {
    CApplicationMessenger::Get().Shutdown(); // Turn off the box
  }
#endif
}

//Check if hd spindown must be blocked
bool CApplication::MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown)
{
#ifdef HAS_XBOX_HARDWARE
  if (IsPlayingVideo())
  {
    //block immediate spindown when playing a video non-fullscreen (videocontrol is playing)
    if ((!bCheckThisForNormalSpinDown) && (!g_graphicsContext.IsFullScreenVideo()))
    {
      return true;
    }
    //allow normal hd spindown always if the movie is paused
    if ((bCheckThisForNormalSpinDown) && (m_pPlayer->IsPaused()))
    {
      return false;
    }
    //don't allow hd spindown when playing files with vobsub subtitles.
    CStdString strSubTitelExtension;
    if (m_pPlayer->GetSubtitleExtension(strSubTitelExtension))
    {
      return (strSubTitelExtension == ".idx");
    }
  }
#endif
  return false;
}

void CApplication::CheckNetworkHDSpinDown(bool playbackStarted)
{
  int iSpinDown = CSettings::Get().GetInt("harddisk.remoteplayspindown");
  if (iSpinDown == SPIN_DOWN_NONE)
    return ;
  if (g_windowManager.HasModalDialog())
    return ;
  if (MustBlockHDSpinDown(false))
    return ;

  if ((!m_bNetworkSpinDown) || playbackStarted)
  {
    int iDuration = 0;
    if (IsPlayingAudio())
    {
      //try to get duration from current tag because mplayer doesn't calculate vbr mp3 correctly
      if (m_itemCurrentFile->HasMusicInfoTag())
        iDuration = m_itemCurrentFile->GetMusicInfoTag()->GetDuration();
    }
    if (IsPlaying() && iDuration <= 0)
    {
      iDuration = (int)GetTotalTime();
    }
    //spin down harddisk when the current file being played is not on local harddrive and
    //duration is more then spindown timeoutsetting or duration is unknown (streams)
    if (
      !m_itemCurrentFile->IsHD() &&
      (
        (iSpinDown == SPIN_DOWN_VIDEO && IsPlayingVideo()) ||
        (iSpinDown == SPIN_DOWN_MUSIC && IsPlayingAudio()) ||
        (iSpinDown == SPIN_DOWN_BOTH && (IsPlayingVideo() || IsPlayingAudio()))
      ) &&
      (
        (iDuration <= 0) ||
        (iDuration > CSettings::Get().GetInt("harddisk.remoteplayspindownminduration")*60)
      )
    )
    {
      m_bNetworkSpinDown = true;
      if (!playbackStarted)
      { //if we got here not because of a playback start check what screen we are in
        // get the current active window
        int iWin = g_windowManager.GetActiveWindow();
        if (iWin == WINDOW_FULLSCREEN_VIDEO)
        {
          // check if OSD is visible, if so don't do immediate spindown
          CGUIDialogVideoOSD *pOSD = (CGUIDialogVideoOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
          if (pOSD)
            m_bNetworkSpinDown = !pOSD->IsDialogRunning();
        }
      }
      if (m_bNetworkSpinDown)
      {
        //do the spindown right now + delayseconds
        m_dwSpinDownTime = timeGetTime();
      }
    }
  }
  if (m_bNetworkSpinDown)
  {
    // check the elapsed time
    DWORD dwTimeSpan = timeGetTime() - m_dwSpinDownTime;
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)CSettings::Get().GetInt("harddisk.remoteplayspindowndelay")*1000UL)) )
    {
      // time has elapsed, spin it down
#ifdef HAS_XBOX_HARDWARE
      XKHDD::SpindownHarddisk();
#endif
      //stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
      m_bNetworkSpinDown = true;
    }
    else if (m_dwSpinDownTime == 0 && IsPlaying())
    {
      // we are currently spun down - let's spin back up again if we are playing media
      // and we're within 10 seconds (or 0.5*spindown time) of the end.  This should
      // make returning to the GUI a bit snappier + speed up stacked item changes.
      int iMinSpinUp = 10;
      if (iMinSpinUp > CSettings::Get().GetInt("harddisk.remoteplayspindowndelay")*0.5f)
        iMinSpinUp = (int)(CSettings::Get().GetInt("harddisk.remoteplayspindowndelay")*0.5f);
      if (g_infoManager.GetPlayTimeRemaining() == iMinSpinUp)
      { // spin back up
#ifdef HAS_XBOX_HARDWARE
        XKHDD::SpindownHarddisk(false);
#endif
      }
    }
  }
}

void CApplication::CheckHDSpindown()
{
  if (!CSettings::Get().GetInt("harddisk.spindowntime"))
    return ;
  if (g_windowManager.HasModalDialog())
    return ;
  if (MustBlockHDSpinDown())
    return ;

  if (!m_bSpinDown &&
      (
        !IsPlaying() ||
        (IsPlaying() && !m_itemCurrentFile->IsHD())
      )
     )
  {
    m_bSpinDown = true;
    m_bNetworkSpinDown = false; // let networkspindown override normal spindown
    m_dwSpinDownTime = timeGetTime();
  }

  //Can we do a spindown right now?
  if (m_bSpinDown)
  {
    // yes, then check the elapsed time
    DWORD dwTimeSpan = timeGetTime() - m_dwSpinDownTime;
    if ( (m_dwSpinDownTime != 0) && (dwTimeSpan >= ((DWORD)CSettings::Get().GetInt("harddisk.spindowntime")*60UL*1000UL)) )
    {
      // time has elapsed, spin it down
#ifdef HAS_XBOX_HARDWARE
      XKHDD::SpindownHarddisk();
#endif
      //stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
      m_bSpinDown = true;
    }
  }
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        int nRemoved = g_playlistPlayer.RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          g_windowManager.SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      { // we've started a previously queued item
        CFileItemPtr item = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist())[m_nextPlaylistItem];
        // update the playlist manager
        int currentSong = g_playlistPlayer.GetCurrentSong();
        int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, g_playlistPlayer.GetCurrentPlaylist(), param, item);
        g_windowManager.SendThreadMessage(msg);
        g_playlistPlayer.SetCurrentSong(m_nextPlaylistItem);
        *m_itemCurrentFile = *item;
      }
      g_infoManager.SetCurrentItem(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

      CheckNetworkHDSpinDown(true);
      StartLEDControl(true);
      DimLCDOnPlayback(true);

      if (IsPlayingAudio())
      {
        // Start our cdg parser as appropriate
#ifdef HAS_KARAOKE
        if (m_pCdgParser && CSettings::Get().GetBool("karaoke.enabled") && !m_itemCurrentFile->IsInternetStream())
        {
          if (m_pCdgParser->IsRunning())
            m_pCdgParser->Stop();
          if (m_itemCurrentFile->IsMusicDb())
          {
            if (!m_itemCurrentFile->HasMusicInfoTag() || !m_itemCurrentFile->GetMusicInfoTag()->Loaded())
            {
              IMusicInfoTagLoader* tagloader = CMusicInfoTagLoaderFactory::CreateLoader(m_itemCurrentFile->GetPath());
              tagloader->Load(m_itemCurrentFile->GetPath(),*m_itemCurrentFile->GetMusicInfoTag());
              delete tagloader;
            }
            m_pCdgParser->Start(m_itemCurrentFile->GetMusicInfoTag()->GetURL());
          }
          else
            m_pCdgParser->Start(m_itemCurrentFile->GetPath());
        }
#endif
      }
      
      return true;
    }
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = g_playlistPlayer.GetNextSong();
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        if (m_pPlayer) m_pPlayer->OnNothingToQueueNotify();
        return true; // nothing to do
      }
      // ok, grab the next song
      CFileItemPtr item = playlist[iNext];
      // ok - send the file to the player if it wants it
      if (m_pPlayer && m_pPlayer->QueueNextFile(*item))
      { // player wants the next file
        m_nextPlaylistItem = iNext;
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      // first check if we still have items in the stack to play
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_currentStackPosition < m_currentStack->Size() - 1)
        { // just play the next item in the stack
          PlayFile(*(*m_currentStack)[++m_currentStackPosition], true);
          return true;
        }
      }
      
      // In case playback ended due to user eg. skipping over the end, clear
      // our resume bookmark here
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED && m_progressTrackingPlayCountUpdate && g_advancedSettings.m_videoIgnorePercentAtEnd > 0)
      {
        // Delete the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
      }

      // reset our spindown
      m_bNetworkSpinDown = false;
      m_bSpinDown = false;

      // reset the current playing file
      m_itemCurrentFile->Reset();
      g_infoManager.ResetCurrentItem();
      m_currentStack->Clear();

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        // sending true to PlayNext() effectively passes bRestart to PlayFile()
        // which is not generally what we want (except for stacks, which are
        // handled above)
        g_playlistPlayer.PlayNext();
      }
      else
      {
        delete m_pPlayer;
        m_pPlayer = 0;
      }

      if (!IsPlaying())
      {
        g_audioManager.Enable(true);
        StartLEDControl(false);
        DimLCDOnPlayback(false);

#ifdef HAS_KARAOKE
        if(m_pCdgParser)
          m_pCdgParser->Free();
#endif
      }

      if (!IsPlayingVideo() && g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      {
        g_windowManager.PreviousWindow();
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        CSettings::Get().Save();  // save vis settings
        ResetScreenSaverWindow();
        g_windowManager.PreviousWindow();
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) && !CDetectDVDMedia::IsDiscInDrive() && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        CSettings::Get().Save();    // save vis settings
        ResetScreenSaverWindow();
        g_windowManager.PreviousWindow();
      }
      
      return true;
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_FULLSCREEN:
    { // Switch to fullscreen, if we can
      SwitchToFullScreen();
      return true;
    }
    break;
  case GUI_MSG_EXECUTE:
    if (message.GetStringParam().length() > 0)
      return ExecuteXBMCAction(message.GetStringParam());

    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr)
    {
      // see if it is a user set string
      CLog::Log(LOGDEBUG,"%s : Translating %s", __FUNCTION__, actionStr.c_str());
      CGUIInfoLabel info(actionStr, "");
      actionStr = info.GetLabel(0);
      CLog::Log(LOGDEBUG,"%s : To %s", __FUNCTION__, actionStr.c_str());

      // user has asked for something to be executed
      if (CBuiltins::HasCommand(actionStr))
        CBuiltins::Execute(actionStr);
      else
      {
        // try translating the action from our ButtonTranslator
        int actionID;
        if (CButtonTranslator::TranslateActionString(actionStr.c_str(), actionID))
        {
          OnAction(CAction(actionID));
          return true;
        }
        CFileItem item(actionStr, false);
        if (item.IsPythonScript())
        { // a python script
          g_pythonParser.evalFile(item.GetPath().c_str(),ADDON::AddonPtr());
        }
        else if (item.IsXBE())
        { // an XBE
          int iRegion;
          if (CSettings::Get().GetBool("myprograms.gameautoregion"))
          {
            CXBE xbe;
            iRegion = xbe.ExtractGameRegion(item.GetPath());
            if (iRegion < 1 || iRegion > 7)
              iRegion = 0;
            iRegion = xbe.FilterRegion(iRegion);
          }
          else
            iRegion = 0;
          CUtil::RunXBE(item.GetPath().c_str(),NULL,F_VIDEO(iRegion));
        }
        else if (item.IsAudio() || item.IsVideo())
        { // an audio or video file
          PlayFile(item);
        }
        else
          return false;
      }
      return true;
    }

void CApplication::Process()
{
  // dispatch the messages generated by python or other threads to the current window
  g_windowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after g_windowManager.Render())
  CApplicationMessenger::Get().ProcessWindowMessages();

  // process any Python scripts
  g_pythonParser.Process();

  // process messages, even if a movie is playing
  CApplicationMessenger::Get().ProcessMessages();

  // check for memory unit changes
#ifdef HAS_XBOX_HARDWARE
  if (g_memoryUnitManager.Update())
  { // changes have occured - update our shares
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_REMOVED_MEDIA);
    g_windowManager.SendThreadMessage(msg);
  }
#endif

  // check if we can free unused memory
  g_audioManager.FreeUnused();

  // check how far we are through playing the current item
  // and do anything that needs doing (playcount updates etc)
  CheckPlayingProgress();

  // update sound
  if (m_pPlayer)
    m_pPlayer->DoAudioWork();

  // process karaoke
#ifdef HAS_KARAOKE
  if (m_pCdgParser)
    m_pCdgParser->ProcessVoice();
#endif

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  // check our network state every 15 seconds or when net status changes
  m_network->CheckNetwork(30);
  
  // check if we need 2 spin down the harddisk
  CheckNetworkHDSpinDown();
  if (!m_bNetworkSpinDown)
    CheckHDSpindown();

  // Store our file state for use on close()
  UpdateFileState();

  // Check if we need to activate the screensaver (if enabled).
  if (CSettings::Get().GetString("screensaver.mode") != "None")
    CheckScreenSaver();

  // check if we need to shutdown (if enabled)
  if (CSettings::Get().GetInt("powermanagement.shutdowntime"))
    CheckShutdown();

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  if (!IsPlayingVideo())
    CSectionLoader::UnloadDelayed();

  // Xbox Autodetection - Send in X sec PingTime Interval
  if (g_windowManager.GetActiveWindow() != WINDOW_LOGIN_SCREEN) // sorry jm ;D
    CUtil::AutoDetection();

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  // check for any idle myth sessions
  CMythSession::CheckIdle();
#ifdef HAS_FILESYSTEM
  // check for any idle htsp sessions
  HTSP::CHTSPDirectorySession::CheckIdle();
#endif
#ifdef HAS_TIME_SERVER
  // check for any needed sntp update
  if(CNetworkServices::Get().IsTimeServerRunning() && CNetworkServices::Get().IsTimeServerUpdateNeeded())
    CNetworkServices::Get().UpdateTimeServer();
#endif

  // LED - LCD SwitchOn On Paused! m_bIsPaused=TRUE -> LED/LCD is ON!
  if(IsPaused() != m_bIsPaused)
  {
    if(CSettings::Get().GetBool("system.ledenableonpaused"))
      StartLEDControl(m_bIsPaused);
    if(CSettings::Get().GetBool("lcd.enableonpaused"))
      DimLCDOnPlayback(m_bIsPaused);
    m_bIsPaused = IsPaused();
  }

  if (!IsPlayingVideo())
    g_largeTextureManager.CleanupUnusedImages();

  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  if (!IsPlayingVideo())
    m_Autorun.HandleAutorun();

  // update upnp server/renderer states
  if(CUPnP::IsInstantiated())
    CUPnP::GetInstance()->UpdateState();

  //Check to see if current playing Title has changed and whether we should broadcast the fact
  CheckForTitleChange();

  if (!IsPlayingVideo())
    CAddonInstaller::Get().UpdateRepos();
}

// Global Idle Time in Seconds
// idle time will be resetet if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplication::GlobalIdleTime()
{
  if(!m_idleTimer.IsRunning())
  {
    m_idleTimer.Stop();
    m_idleTimer.StartZero();
  }
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplication::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
  {
    m_navigationTimer.Stop();
    m_navigationTimer.StartZero();
  }
  return m_navigationTimer.GetElapsedSeconds();
}

void CApplication::DelayedPlayerRestart()
{
  m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
  if (m_restartPlayerTimer.GetElapsedSeconds() > 3)
  {
    m_restartPlayerTimer.Stop();
    m_restartPlayerTimer.Reset();
    Restart(true);
  }
}

void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  if ( !IsPlayingVideo() && !IsPlayingAudio())
    return ;

  if( !m_pPlayer )
    return ;

  SaveFileState();

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  CStdString state = m_pPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->m_lStartOffset = (long)(time * 75.0);

  // reopen the file
  if ( PlayFile(*m_itemCurrentFile, true) && m_pPlayer )
    m_pPlayer->SetPlayerState(state);
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile->GetPath();
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

void CApplication::ShowVolumeBar(const CAction *action)
{
  CGUIDialog *volumeBar = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VOLUME_BAR);
  if (volumeBar)
  {
    volumeBar->Show();
    if (action)
      volumeBar->OnAction(*action);
  }
}

bool CApplication::IsMuted() const
{
  return m_muted;
}

void CApplication::ToggleMute(void)
{
  if (m_muted)
    UnMute();
  else
    Mute();
}

void CApplication::SetMute(bool mute)
{
  if (m_muted != mute)
  {
    ToggleMute();
    m_muted = mute;
  }
}

void CApplication::Mute()
{
  m_muted = true;
}

void CApplication::UnMute()
{
  m_muted = false;
}

void CApplication::SetVolume(long iValue, bool isPercentage /* = true */)
{
  if (isPercentage)
    iValue = (long)((float)iValue * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);

  SetHardwareVolume(iValue);
#ifndef HAS_SDL_AUDIO
  g_audioManager.SetVolume(m_volumeLevel);
#else
  g_audioManager.SetVolume((int)(128.f * (m_volumeLevel - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)));
#endif

  CVariant data(CVariant::VariantTypeObject);
  data["volume"] = (int)(((float)(m_volumeLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 100.0f + 0.5f);
  /* TODO: add once DRC is available
  data["drc"] = (int)(((float)(m_dynamicRangeCompressionLevel - VOLUME_DRC_MINIMUM)) / (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) * 100.0f + 0.5f);*/
  data["muted"] = m_muted;
  CAnnouncementManager::Announce(Application, "xbmc", "OnVolumeChanged", data);
}

void CApplication::SetHardwareVolume(long hardwareVolume)
{
  // TODO DRC
  if (hardwareVolume >= VOLUME_MAXIMUM) // + VOLUME_DRC_MAXIMUM
    hardwareVolume = VOLUME_MAXIMUM;// + VOLUME_DRC_MAXIMUM;
  if (hardwareVolume <= VOLUME_MINIMUM)
    hardwareVolume = VOLUME_MINIMUM;

  // update our settings
  if (hardwareVolume > VOLUME_MAXIMUM)
  {
    m_dynamicRangeCompressionLevel = hardwareVolume - VOLUME_MAXIMUM;
    m_volumeLevel = VOLUME_MAXIMUM;
  }
  else
  {
    m_dynamicRangeCompressionLevel = 0;
    m_volumeLevel = hardwareVolume;
  }

  // and tell our player to update the volume
  if (m_pPlayer)
  {
    m_pPlayer->SetVolume(m_volumeLevel);
    // TODO DRC
//    m_pPlayer->SetDynamicRangeCompression(m_dynamicRangeCompressionLevel);
  }
}

int CApplication::GetVolume(bool percentage /* = true */) const
{
  if (percentage)
  { // converts the hardware volume (in mB) to a percentage
    return int(((float)(m_volumeLevel + m_dynamicRangeCompressionLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f);
  }

  return m_volumeLevel;
}

int CApplication::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay + g_advancedSettings.m_videoSubsDelayRange)) / (2 * g_advancedSettings.m_videoSubsDelayRange)*100.0f + 0.5f);
}

int CApplication::GetAudioDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay + g_advancedSettings.m_videoAudioDelayRange)) / (2 * g_advancedSettings.m_videoAudioDelayRange)*100.0f + 0.5f);
}

void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
  if (!m_pPlayer->CanSeek())
    return;
  if (m_pPlayer->IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (iSpeed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (iSpeed < m_iPlaySpeed))
    )
    {
      iSpeed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    m_pPlayer->Pause();
  }
  m_iPlaySpeed = iSpeed;

  m_pPlayer->ToFFRW(m_iPlaySpeed);
  if (m_iPlaySpeed == 1)
  { // restore volume
    m_pPlayer->SetVolume(m_volumeLevel);
  }
  else
  { // mute volume
    m_pPlayer->SetVolume(VOLUME_MINIMUM);
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      rc = (*m_currentStack)[m_currentStack->Size() - 1]->m_lEndOffset;
    else
      rc = m_pPlayer->GetTotalTime();
  }

  return rc;
}

void CApplication::ResetPlayTime()
{
  if (IsPlaying() && m_pPlayer)
    m_pPlayer->ResetTime();
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      long startOfCurrentFile = (m_currentStackPosition > 0) ? (*m_currentStack)[m_currentStackPosition-1]->m_lEndOffset : 0;
      rc = (double)startOfCurrentFile + m_pPlayer->GetTime() * 0.001;
    }
    else
      rc = static_cast<double>(m_pPlayer->GetTime() * 0.001f);
  }

  return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
  if (IsPlaying() && m_pPlayer && (dTime >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if ((*m_currentStack)[i]->m_lEndOffset > dTime)
        {
          long startOfNewFile = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          if (m_currentStackPosition == i)
            m_pPlayer->SeekTime((__int64)((dTime - startOfNewFile) * 1000.0));
          else
          { // seeking to a new file
            m_currentStackPosition = i;
            CFileItem item(*(*m_currentStack)[i]);
            item.m_lStartOffset = (long)((dTime - startOfNewFile) * 75.0);
            // don't just call "PlayFile" here, as we are quite likely called from the
            // player thread, so we won't be able to delete ourselves.
            CApplicationMessenger::Get().PlayFile(item, true);
          }
          return;
        }
      }
    }
    // convert to milliseconds and perform seek
    m_pPlayer->SeekTime( static_cast<__int64>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (IsPlayingAudio() && m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    } 
    
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      return (float)(GetTime() / GetTotalTime() * 100);
    else
      return m_pPlayer->GetPercentage();
  }
  return 0.0f;
}

float CApplication::GetCachePercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    // Note that the player returns a relative cache percentage and we want an absolute percentage
    // We also need to take into account the stack's total time vs. currently playing file's total time
    float stackedTotalTime = (float) GetTotalTime();
    if (stackedTotalTime > 0.0f)
      return min( 100.0f, GetPercentage() + (m_pPlayer->GetCachePercentage() * m_pPlayer->GetTotalTime() / stackedTotalTime ) );
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  if (IsPlaying() && m_pPlayer && (percent >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_pPlayer->SeekPercentage(percent);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
  // if playing from the video info window, close it first!
  if (g_windowManager.HasModalDialog() && g_windowManager.GetTopMostModalDialogID() == WINDOW_DIALOG_VIDEO_INFO)
  {
    CGUIDialogVideoInfo* pDialog = (CGUIDialogVideoInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }

  // don't switch if there is a dialog on screen or the slideshow is active
  if (g_windowManager.HasModalDialog() || g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return false;

  // See if we're playing a video, and are in GUI mode
  if ( IsPlayingVideo() && g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
  {
    // then switch to fullscreen mode
    g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
    g_TextureManager.Flush();
    return true;
  }
  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (IsPlayingAudio() && g_windowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  { // then switch to visualisation
    g_windowManager.ActivateWindow(WINDOW_VISUALISATION);
    g_TextureManager.Flush();
    return true;
  }
  return false;
}

PLAYERCOREID CApplication::GetCurrentPlayer()
{
  return m_eCurrentPlayer;
}

void CApplication::UpdateLibraries()
{
  if (CSettings::Get().GetBool("videolibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting video library startup scan", __FUNCTION__);
    StartVideoScan("");
  }
 
  if (CSettings::Get().GetBool("musiclibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "%s - Starting music library startup scan", __FUNCTION__);
    StartMusicScan("");
  }
}

bool CApplication::IsVideoScanning() const
{
  return m_videoInfoScanner->IsScanning();
}

bool CApplication::IsMusicScanning() const
{
  return m_musicInfoScanner->IsScanning();
}

void CApplication::StopVideoScan()
{
  if (m_videoInfoScanner->IsScanning())
    m_videoInfoScanner->Stop();
}

void CApplication::StopMusicScan()
{
  if (m_musicInfoScanner->IsScanning())
    m_musicInfoScanner->Stop();
}

void CApplication::StartVideoCleanup()
{
  if (m_videoInfoScanner->IsScanning())
    return;

  m_videoInfoScanner->CleanDatabase();
}

void CApplication::StartVideoScan(const CStdString &strDirectory, bool scanAll)
{
  if (m_videoInfoScanner->IsScanning())
    return;

  m_videoInfoScanner->ShowDialog(true);

  m_videoInfoScanner->Start(strDirectory,scanAll);
}

void CApplication::StartMusicScan(const CStdString &strDirectory)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->Start(strDirectory);
}

void CApplication::StartMusicAlbumScan(const CStdString& strDirectory)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->FetchAlbumInfo(strDirectory);
}

void CApplication::StartMusicArtistScan(const CStdString& strDirectory)
{
  if (m_musicInfoScanner->IsScanning())
    return;

  m_musicInfoScanner->ShowDialog(true);

  m_musicInfoScanner->FetchArtistInfo(strDirectory);
}

void CApplication::CheckPlayingProgress()
{
  // check if we haven't rewound past the start of the file
  if (IsPlaying())
  {
    int iSpeed = g_application.GetPlaySpeed();
    if (iSpeed < 1)
    {
      iSpeed *= -1;
      int iPower = 0;
      while (iSpeed != 1)
      {
        iSpeed >>= 1;
        iPower++;
      }
      if (g_infoManager.GetPlayTime() / 1000 < iPower)
      {
        g_application.SetPlaySpeed(1);
        g_application.SeekTime(0);
      }
    }
  }
}

bool CApplication::ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist, int track)
{
  CLog::Log(LOGDEBUG,"CApplication::ProcessAndStartPlaylist(%s, %i)",strPlayList.c_str(), iPlaylist);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return false;

  // setup correct playlist
  g_playlistPlayer.ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  g_application.m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  g_playlistPlayer.Add(iPlaylist, playlist);

  // if we have a playlist
  if (g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    // start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play(track);
    return true;
  }
  return false;
}

void CApplication::CheckForDebugButtonCombo()
{
#ifdef HAS_GAMEPAD
  ReadInput();
  if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    g_advancedSettings.m_logLevel = LOG_LEVEL_DEBUG_FREEMEM;
    CLog::Log(LOGINFO, "Key combination detected for full debug logging (X+Y)");
  }
#ifdef _DEBUG
  g_advancedSettings.m_logLevel = LOG_LEVEL_DEBUG_FREEMEM;
#endif
#endif
}

void CApplication::SaveCurrentFileSettings()
{
  if (m_itemCurrentFile->IsVideo())
  {
    // save video settings
    if (CMediaSettings::Get().GetCurrentVideoSettings() != CMediaSettings::Get().GetDefaultVideoSettings())
    {
      CVideoDatabase dbs;
      dbs.Open();
      dbs.SetVideoSettings(m_itemCurrentFile->GetPath(), CMediaSettings::Get().GetCurrentVideoSettings());
      dbs.Close();
    }
  }
}

CNetwork& CApplication::getNetwork()
{
   return *m_network;
}

bool CApplication::IsCurrentThread() const
{
  return CThread::IsCurrentThread(m_threadID);
}

void CApplication::InitDirectoriesXbox()
{
  // map our special drives to the correct drive letter
  CSpecialProtocol::SetXBMCPath("Q:\\"); // Use Q as ie. F doesn't exist yet!!!
  CSpecialProtocol::SetHomePath("Q:\\home");
  CSpecialProtocol::SetTempPath("Z:\\");

  // First profile is always the Master Profile
  CSpecialProtocol::SetMasterProfilePath("Q:\\home\\userdata");

  g_advancedSettings.m_logFolder = "special://home/";
}

bool CApplication::SetLanguage(const CStdString &strLanguage)
{
  CStdString strPreviousLanguage = CSettings::Get().GetString("locale.language");
  if (strLanguage != strPreviousLanguage)
  {
    CStdString strLangInfoPath = StringUtils2::Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());
    if (!g_langInfo.Load(strLangInfoPath))
      return false;

    CSettings::Get().SetString("locale.language", strLanguage);

    CStdString strKeyboardLayoutConfigurationPath;
    strKeyboardLayoutConfigurationPath.Format("special://xbmc/language/%s/keyboardmap.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
    g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);

    if (!g_localizeStrings.Load("special://xbmc/language/", strLanguage))
      return false;

    // also tell our weather and skin to reload as these are localized
    g_weatherManager.Refresh();
    ReloadSkin();
  }

  return true;
}

void CApplication::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "lookandfeel.skin" ||
      settingId == "lookandfeel.font" ||
      settingId == "lookandfeel.skincolors")
  {
    // if the skin changes and the current theme is not the default one, reset
    // the theme to the default value (which will also change lookandfeel.skincolors
    // which in turn will reload the skin.  Similarly, if the current skin font is not
    // the default, reset it as well.
    if (settingId == "lookandfeel.skin" && CSettings::Get().GetString("lookandfeel.skintheme") != "SKINDEFAULT")
    {
      CSettings::Get().SetString("lookandfeel.skintheme", "SKINDEFAULT");
      return;
    }
    if (settingId == "lookandfeel.skin" && CSettings::Get().GetString("lookandfeel.font") != "Default")
    {
      CSettings::Get().SetString("lookandfeel.font", "Default");
      return;
    }

    std::string builtin("ReloadSkin");
    if (settingId == "lookandfeel.skin" && !m_skinReverting)
      builtin += "(confirm)";
    CApplicationMessenger::Get().ExecBuiltIn(builtin);
  }
  else if (settingId == "lookandfeel.skintheme")
  {
    // also set the default color theme
    string colorTheme = URIUtils::ReplaceExtension(((CSettingString*)setting)->GetValue(), ".xml");
    if (StringUtils2::EqualsNoCase(colorTheme, "Textures.xml"))
      colorTheme = "defaults.xml";

    // check if we have to change the skin color
    // if yes, it will trigger a call to ReloadSkin() in
    // it's OnSettingChanged() callback
    // if no we have to call ReloadSkin() ourselves
    if (!StringUtils2::EqualsNoCase(colorTheme, CSettings::Get().GetString("lookandfeel.skincolors")))
      CSettings::Get().SetString("lookandfeel.skincolors", colorTheme);
    else
      CApplicationMessenger::Get().ExecBuiltIn("ReloadSkin");
  }
  else if (settingId == "lookandfeel.skinzoom")
#ifdef _XBOX
    g_fontManager.ReloadTTFFonts();
#else
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
#endif
  else if (StringUtils2::StartsWith(settingId, "audiooutput."))
  {
    if (settingId == "audiooutput.ac3passthrough")
      g_audioConfig.SetAC3Enabled(((CSettingBool*)setting)->GetValue());
    else if (settingId == "audiooutput.dtspassthrough")
      g_audioConfig.SetDTSEnabled(((CSettingBool*)setting)->GetValue());
    else if (settingId == "audiooutput.aacpassthrough")
      g_audioConfig.SetAACEnabled(((CSettingBool*)setting)->GetValue());
    else if (settingId == "audiooutput.mp1passthrough")
      g_audioConfig.SetMP1Enabled(((CSettingBool*)setting)->GetValue());
    else if (settingId == "audiooutput.mp2passthrough")
      g_audioConfig.SetMP2Enabled(((CSettingBool*)setting)->GetValue());
    else if (settingId == "audiooutput.mp3passthrough")
      g_audioConfig.SetMP3Enabled(((CSettingBool*)setting)->GetValue());

    if (g_audioConfig.NeedsSave())
      g_audioConfig.Save();
  //   if (settingId == "audiooutput.guisoundmode")
  //     CAEFactory::SetSoundMode(((CSettingInt*)setting)->GetValue());
  //   CAEFactory::OnSettingsChange(settingId);
  }
  else if (settingId == "harddisk.aamlevel")
  {
    if (((CSettingInt*)setting)->GetValue() == AAM_QUIET)
      XKHDD::SetAAMLevel(0x80);
    else if (((CSettingInt*)setting)->GetValue() == AAM_FAST)
      XKHDD::SetAAMLevel(0xFE);
  }
  else if (settingId == "harddisk.apmlevel")
  {
    switch(((CSettingInt*)setting)->GetValue())
    {
    case APM_LOPOWER:
      XKHDD::SetAPMLevel(0x80);
      break;
    case APM_HIPOWER:
      XKHDD::SetAPMLevel(0xFE);
      break;
    case APM_LOPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x01);
      break;
    case APM_HIPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x7F);
      break;
    }
  }
  else if (settingId == "karaoke.port0voicemask")
    CCdgParser::FillInVoiceMaskValues(0, ((CSettingString*)setting)->GetValue());
  else if (settingId == "karaoke.port1voicemask")
    CCdgParser::FillInVoiceMaskValues(1, ((CSettingString*)setting)->GetValue());
  else if (settingId == "karaoke.port2voicemask")
    CCdgParser::FillInVoiceMaskValues(2, ((CSettingString*)setting)->GetValue());
  else if (settingId == "karaoke.port3voicemask")
    CCdgParser::FillInVoiceMaskValues(3, ((CSettingString*)setting)->GetValue());
  else if (settingId == "lcd.type")
    g_lcd->Initialize();
  else if (settingId == "lcd.backlight")
    g_lcd->SetBackLight(((CSettingInt*)setting)->GetValue());
  else if (settingId == "lcd.modchip")
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
  else if (settingId == "lcd.contrast")
    g_lcd->SetContrast(((CSettingInt*)setting)->GetValue());
  else if (StringUtils2::EqualsNoCase(settingId, "musicplayer.replaygaintype"))
    m_replayGainSettings.iType = ((CSettingInt*)setting)->GetValue();
  else if (StringUtils2::EqualsNoCase(settingId, "musicplayer.replaygainpreamp"))
    m_replayGainSettings.iPreAmp = ((CSettingInt*)setting)->GetValue();
  else if (StringUtils2::EqualsNoCase(settingId, "musicplayer.replaygainnogainpreamp"))
    m_replayGainSettings.iNoGainPreAmp = ((CSettingInt*)setting)->GetValue();
  else if (StringUtils2::EqualsNoCase(settingId, "musicplayer.replaygainavoidclipping"))
    m_replayGainSettings.bAvoidClipping = ((CSettingBool*)setting)->GetValue();
  else if (settingId == "network.assignment" || settingId == "network.ipaddress" ||
           settingId == "network.subnet" || settingId == "network.gateway" ||
           settingId == "network.dns" || settingId == "network.dns2")
  {
    m_network->NetworkMessage(CNetwork::SERVICES_DOWN,1);
    m_network->SetupNetwork();
  }
  else if (settingId == "system.ledcolour")
  {
    // Alter LED Colour immediately
    int iData =  ((CSettingInt*)setting)->GetValue();
    if (iData == LED_COLOUR_NO_CHANGE)
      // LED_COLOUR_NO_CHANGE: to prevent "led off" on colour immediately change, set to default green! 
      //                       (we have no previos reference LED COLOUR, to set the LED colour back)
      //                       on next boot the colour will not changed and the default BIOS led colour will used
      ILED::CLEDControl(LED_COLOUR_GREEN); 
    else
      ILED::CLEDControl(iData);
  }
}

void CApplication::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "lookandfeel.skinsettings")
    g_windowManager.ActivateWindow(WINDOW_SKIN_SETTINGS);
  else if (settingId == "screensaver.preview")
    ActivateScreenSaver(true);
  else if (settingId == "screensaver.settings")
  {
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(CSettings::Get().GetString("screensaver.mode"), addon, ADDON_SCREENSAVER))
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
  }
  else if (settingId == "videoscreen.guicalibration")
    g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
}

bool CApplication::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  // if (settingId == "audiooutput.channels")
  // {
  //   // check if this is an update from Eden
  //   if (oldSettingId != NULL && oldSettingNode != NULL &&
  //       StringUtils2::EqualsNoCase(oldSettingId, "audiooutput.channellayout"))
  //   {
  //     bool ret = false;
  //     CSettingInt* channels = (CSettingInt*)setting;
  //     if (channels->FromString(oldSettingNode->FirstChild()->ValueStr()) && channels->GetValue() < AE_CH_LAYOUT_MAX - 1)
  //       ret = channels->SetValue(channels->GetValue() + 1);

  //     // let's just reset the audiodevice settings as well
  //     std::string audiodevice = CSettings::Get().GetString("audiooutput.audiodevice");
  //     CAEFactory::VerifyOutputDevice(audiodevice, false);
  //     ret |= CSettings::Get().SetString("audiooutput.audiodevice", audiodevice.c_str());

  //     return ret;
  //   }
  // }
  // else if (settingId == "screensaver.mode")
  // {
  //   CSettingString *screensaverMode = (CSettingString*)setting;
  //   // we no longer ship the built-in slideshow screensaver, replace it if it's still in use
  //   if (StringUtils2::EqualsNoCase(screensaverMode->GetValue(), "screensaver.xbmc.builtin.slideshow"))
  //     return screensaverMode->SetValue("screensaver.xbmc.builtin.dim");
  // }
  // else if (settingId == "scrapers.musicvideosdefault")
  // {
  //   CSettingAddon *musicvideoScraper = (CSettingAddon*)setting;
  //   if (StringUtils2::EqualsNoCase(musicvideoScraper->GetValue(), "metadata.musicvideos.last.fm"))
  //   {
  //     musicvideoScraper->Reset();
  //     return true;
  //   }
  // }

  return false;
}
