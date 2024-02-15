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
#include "XBAudioConfig.h"
#include "settings/Settings.h"
#include "utils/log.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/Undocumented.h"
#endif

#include "defs_from_settings.h"

XBAudioConfig g_audioConfig;

XBAudioConfig::XBAudioConfig()
{
#ifdef HAS_XBOX_AUDIO
  m_dwAudioFlags = XGetAudioFlags();
#else
  m_dwAudioFlags = 0;
#endif
}

XBAudioConfig::~XBAudioConfig()
{
}

void XBAudioConfig::OnSettingsLoaded()
{
  if (CSettings::Get().GetInt("audiooutput.mode") == AUDIO_DIGITAL && !HasDigitalOutput())
    CSettings::Get().SetInt("audiooutput.mode", AUDIO_ANALOG);
  CSettings::Get().SetBool("audiooutput.ac3passthrough", GetAC3Enabled());
  CSettings::Get().SetBool("audiooutput.dtspassthrough", GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", CSettings::Get().GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", CSettings::Get().GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", CSettings::Get().GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "AAC pass through is %s", CSettings::Get().GetBool("audiooutput.aacpassthrough") ? "enabled" : "disabled");
}

bool XBAudioConfig::HasDigitalOutput()
{
#ifdef HAS_XBOX_AUDIO
  DWORD dwAVPack = XGetAVPack();
  if (dwAVPack == XC_AV_PACK_SCART ||
      dwAVPack == XC_AV_PACK_HDTV ||
      dwAVPack == XC_AV_PACK_VGA ||
      dwAVPack == XC_AV_PACK_SVIDEO)
    return true;
#endif
  return false;
}

void XBAudioConfig::SetAC3Enabled(bool bEnable)
{
#ifdef HAS_XBOX_AUDIO
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_AC3;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_AC3;
#endif
}

bool XBAudioConfig::GetAC3Enabled()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_AC3) != 0;
#else
  return false;
#endif
}

void XBAudioConfig::SetDTSEnabled(bool bEnable)
{
#ifdef HAS_XBOX_AUDIO
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_DTS;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_DTS;
#endif
}

bool XBAudioConfig::GetDTSEnabled()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_DTS) != 0;
#else
  return false;
#endif
}

void XBAudioConfig::SetAACEnabled(bool bEnable)
{
}

bool XBAudioConfig::GetAACEnabled()
{
  return HasDigitalOutput();
}

void XBAudioConfig::SetMP1Enabled(bool bEnable)
{
}

bool XBAudioConfig::GetMP1Enabled()
{
  return HasDigitalOutput();
}

void XBAudioConfig::SetMP2Enabled(bool bEnable)
{
}

bool XBAudioConfig::GetMP2Enabled()
{
  return HasDigitalOutput();
}

void XBAudioConfig::SetMP3Enabled(bool bEnable)
{
}

bool XBAudioConfig::GetMP3Enabled()
{
  return HasDigitalOutput();
}

bool XBAudioConfig::NeedsSave()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return m_dwAudioFlags != XGetAudioFlags();
#else
  return false;
#endif
}

// USE VERY CAREFULLY!!
void XBAudioConfig::Save()
{
  if (!NeedsSave()) return ;
#ifdef HAS_XBOX_AUDIO
  // update the EEPROM settings
  DWORD type = REG_BINARY;
  ExSaveNonVolatileSetting(XC_AUDIO_FLAGS, &type, (PULONG)&m_dwAudioFlags, 4);
  // check that we updated correctly
  if (m_dwAudioFlags != XGetAudioFlags())
  {
    CLog::Log(LOGNOTICE, "Failed to save audio config!");
  }
#endif
}
