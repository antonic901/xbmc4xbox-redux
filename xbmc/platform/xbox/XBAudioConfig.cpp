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

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include "platform/xbox/Undocumented.h"

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
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH) && !HasDigitalOutput())
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, false);
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH, GetAC3Enabled());
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH, GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH) ? "digital" : "analog" );
  CLog::Log(LOGINFO, "AC3 pass through is %s", CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH) ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH) ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "AAC pass through is %s", CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_AACPASSTHROUGH) ? "enabled" : "disabled");
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

void XBAudioConfig::SettingAudioOutputFiller(const SettingConstPtr& setting,
                                             std::vector<IntegerSettingOption>& list,
                                             int& current,
                                             void* data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(38630), 0)); // ANALOG
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(38631), 1)); // DIGITAL
}
