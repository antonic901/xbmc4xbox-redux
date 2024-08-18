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
#include "XBVideoConfig.h"
#include "settings/Settings.h"
#include "utils/log.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/Undocumented.h"
#endif

#include "defs_from_settings.h"

XBVideoConfig g_videoConfig;

XBVideoConfig::XBVideoConfig()
{
  bHasPAL = false;
  bHasNTSC = false;
#ifdef HAS_XBOX_D3D
  m_dwVideoFlags = XGetVideoFlags();
#else
  m_dwVideoFlags = 0;
#endif
}

XBVideoConfig::~XBVideoConfig()
{}

void XBVideoConfig::OnSettingsLoaded()
{
  if (HasLetterbox())
    CSettings::GetInstance().SetInt("videooutput.aspect", VIDEO_LETTERBOX);
  else if (HasWidescreen())
    CSettings::GetInstance().SetInt("videooutput.aspect", VIDEO_WIDESCREEN);
  else
    CSettings::GetInstance().SetInt("videooutput.aspect", VIDEO_NORMAL);
  CSettings::GetInstance().SetBool("videooutput.hd480p", Has480p());
  CSettings::GetInstance().SetBool("videooutput.hd720p", Has720p());
  CSettings::GetInstance().SetBool("videooutput.hd1080i", Has1080i());
}

bool XBVideoConfig::HasPAL() const
{
#ifdef HAS_XBOX_D3D
  if (bHasPAL) return true;
  if (bHasNTSC) return false; // has NTSC (or PAL60) but not PAL
  return (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::HasPAL60() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_PAL_60Hz) != 0;
#else
  return false; // no need for pal60 mode IMO
#endif
}

bool XBVideoConfig::HasNTSC() const
{
#ifdef HAS_XBOX_D3D
  return !HasPAL();
#else
  return true;
#endif
}

bool XBVideoConfig::HasWidescreen() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_WIDESCREEN) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::HasLetterbox() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_LETTERBOX) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has480p() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_480p) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has720p() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_720p) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has1080i() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_1080i) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::HasHDPack() const
{
#ifdef HAS_XBOX_D3D
  return XGetAVPack() == XC_AV_PACK_HDTV;
#else
  return true;
#endif
}

void XBVideoConfig::GetModes(LPDIRECT3D8 pD3D)
{
  bHasPAL = false;
  bHasNTSC = false;
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  CLog::Log(LOGINFO, "Available videomodes:");
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // Skip modes we don't care about
    if ( mode.Format != D3DFMT_LIN_A8R8G8B8 )
      continue;
#ifdef HAS_XBOX_D3D
    if ( mode.Flags & D3DPRESENTFLAG_FIELD )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
      continue;
#endif
    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    // If we get here, we found an acceptable mode
#ifdef HAS_XBOX_D3D
    CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz, %s", mode.Width, mode.Height, mode.RefreshRate, mode.Flags & D3DPRESENTFLAG_WIDESCREEN ? "Widescreen" : "");
#else
    CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz", mode.Width, mode.Height, mode.RefreshRate);
#endif
    if (mode.Width == 720 && mode.Height == 576 && mode.RefreshRate == 50)
      bHasPAL = true;
    if (mode.Width == 720 && mode.Height == 480 && mode.RefreshRate == 60)
      bHasNTSC = true;
  }
}

RESOLUTION XBVideoConfig::GetSafeMode() const
{
  if (HasPAL()) return RES_PAL_4x3;
  return RES_NTSC_4x3;
}

RESOLUTION XBVideoConfig::GetBestMode() const
{
  RESOLUTION bestRes = RES_INVALID;
  RESOLUTION resolutions[] = {RES_HDTV_1080i, RES_HDTV_720p, RES_HDTV_480p_16x9, RES_HDTV_480p_4x3, RES_NTSC_16x9, RES_NTSC_4x3, RES_PAL_16x9, RES_PAL_4x3, RES_PAL60_16x9, RES_PAL60_4x3, RES_INVALID};
  UCHAR i = 0;
  while (resolutions[i] != RES_INVALID)
  {
    if (IsValidResolution(resolutions[i]))
    {
      bestRes = resolutions[i];
      break;
    }
    i++;
  }
  return bestRes;
}

bool XBVideoConfig::IsValidResolution(RESOLUTION res) const
{
  bool bCanDoWidescreen = HasWidescreen();
  if (HasPAL())
  {
    bool bCanDoPAL60 = HasPAL60();
    if (res == RES_PAL_4x3) return true;
    if (res == RES_PAL_16x9 && bCanDoWidescreen) return true;
    if (res == RES_PAL60_4x3 && bCanDoPAL60) return true;
    if (res == RES_PAL60_16x9 && bCanDoPAL60 && bCanDoWidescreen) return true;
  }
  if (HasNTSC())
  {
    if (res == RES_NTSC_4x3) return true;
    if (res == RES_NTSC_16x9 && bCanDoWidescreen) return true;
    if (res == RES_HDTV_480p_4x3 && Has480p()) return true;
    if (res == RES_HDTV_480p_16x9 && Has480p() && bCanDoWidescreen) return true;
    if (res == RES_HDTV_720p && Has720p()) return true;
    if (res == RES_HDTV_1080i && Has1080i()) return true;
  }
  return false;
}

//pre: XBVideoConfig::GetModes has been called before this function
RESOLUTION XBVideoConfig::GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams)
{
  bool bHasPal = HasPAL();
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

#ifdef HAS_XBOX_D3D
    // Skip modes we don't care about
    if ( mode.Format != D3DFMT_LIN_A8R8G8B8 )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_FIELD )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
      continue;
#endif
    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    p3dParams->BackBufferWidth = mode.Width;
    p3dParams->BackBufferHeight = mode.Height;
    p3dParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
#ifdef HAS_XBOX_D3D
    p3dParams->Flags = mode.Flags;
#endif
    if ((bHasPal) && ((mode.Height != 576) || (mode.RefreshRate != 50)))
    {
      continue;
    }
    //take the first available mode
#ifdef HAS_XBOX_D3D
    if (!HasWidescreen() && !(mode.Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      break;
    }
    if (HasWidescreen() && (mode.Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      break;
    }
#endif
  }

  if (HasPAL())
  {
    if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      return RES_PAL_16x9;
    }
    else
    {
      return RES_PAL_4x3;
    }
  }
  if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
  {
    return RES_NTSC_16x9;
  }
  else
  {
    return RES_NTSC_4x3;
  }
}

CStdString XBVideoConfig::GetAVPack() const
{
#ifdef HAS_XBOX_HARDWARE
  switch (XGetAVPack())
  {
    case XC_AV_PACK_STANDARD :
      return "Standard";
    case XC_AV_PACK_SVIDEO :
      return "S-Video";
    case XC_AV_PACK_SCART :
      return "Scart";
    case XC_AV_PACK_HDTV :
      return "HDTV";
    case XC_AV_PACK_VGA :
      return "VGA";
    case XC_AV_PACK_RFU :
      return "RF";
  }
#endif
  return "Unknown";
}

void XBVideoConfig::PrintInfo() const
{
#ifdef HAS_XBOX_D3D
  CLog::Log(LOGINFO, "AV Pack: %s", GetAVPack().c_str());
  CStdString strAVFlags;
  if (HasWidescreen()) strAVFlags += "Widescreen,";
  if (HasPAL60()) strAVFlags += "Pal60,";
  if (Has480p()) strAVFlags += "480p,";
  if (Has720p()) strAVFlags += "720p,";
  if (Has1080i()) strAVFlags += "1080i,";
  if (strAVFlags.size() > 1) strAVFlags = strAVFlags.Left(strAVFlags.size() - 1);
  CLog::Log(LOGINFO, "AV Flags: %s", strAVFlags.c_str());
#endif
}

bool XBVideoConfig::NeedsSave()
{
#ifdef HAS_XBOX_D3D
  return m_dwVideoFlags != XGetVideoFlags();
#else
  return false;
#endif
}

void XBVideoConfig::Set480p(bool bEnable)
{
#ifdef HAS_XBOX_D3D
  if (bEnable)
    m_dwVideoFlags |= XC_VIDEO_FLAGS_HDTV_480p;
  else
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_HDTV_480p;
#endif
}

void XBVideoConfig::Set720p(bool bEnable)
{
#ifdef HAS_XBOX_D3D
  if (bEnable)
    m_dwVideoFlags |= XC_VIDEO_FLAGS_HDTV_720p;
  else
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_HDTV_720p;
#endif
}

void XBVideoConfig::Set1080i(bool bEnable)
{
#ifdef HAS_XBOX_D3D
  if (bEnable)
    m_dwVideoFlags |= XC_VIDEO_FLAGS_HDTV_1080i;
  else
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_HDTV_1080i;
#endif
}

void XBVideoConfig::SetNormal()
{
#ifdef HAS_XBOX_D3D
  m_dwVideoFlags &= ~XC_VIDEO_FLAGS_LETTERBOX;
  m_dwVideoFlags &= ~XC_VIDEO_FLAGS_WIDESCREEN;
#endif
}

void XBVideoConfig::SetLetterbox(bool bEnable)
{
#ifdef HAS_XBOX_D3D
  if (bEnable)
  {
    m_dwVideoFlags |= XC_VIDEO_FLAGS_LETTERBOX;
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_WIDESCREEN;
  }
  else
  {
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_LETTERBOX;
  }
#endif
}

void XBVideoConfig::SetWidescreen(bool bEnable)
{
#ifdef HAS_XBOX_D3D
  if (bEnable)
  {
    m_dwVideoFlags |= XC_VIDEO_FLAGS_WIDESCREEN;
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_LETTERBOX;
  }
  else
  {
    m_dwVideoFlags &= ~XC_VIDEO_FLAGS_WIDESCREEN;
  }
#endif
}

void XBVideoConfig::Save()
{
  if (!NeedsSave()) return;
#ifdef HAS_XBOX_D3D
  // update the EEPROM settings
  DWORD type = REG_DWORD;
  DWORD eepVideoFlags, size;

  // Video flags do not exactly match the flags variable in the EEPROM
  // To account for this, we get the actual value straight from the EEPROM (or shadow),
  // and shift the flags to their proper location in the EEPROM variable.

  ExQueryNonVolatileSetting(XC_VIDEO_FLAGS, &type, (PULONG)&eepVideoFlags, 4, &size);

  eepVideoFlags &= ~(0x5F << 16);
  eepVideoFlags |= ((m_dwVideoFlags & 0x5F) << 16);

  ExSaveNonVolatileSetting(XC_VIDEO_FLAGS, &type, (PULONG)&eepVideoFlags, 4);

  // check that we updated correctly
  if (m_dwVideoFlags != XGetVideoFlags())
  {
    CLog::Log(LOGNOTICE, "Failed to save video config!");
  }
#endif
}
