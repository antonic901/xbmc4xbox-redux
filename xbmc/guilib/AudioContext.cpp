/*
 *  Copyright (C) 2005-2026 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioContext.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIAudioManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include "platform/xbox/XBAudioConfig.h"

CAudioContext::CAudioContext()
{
  m_pDirectSoundDevice = NULL;
  m_pAC97Device = NULL;
  m_iDevice = NONE;
}

bool CAudioContext::SetActiveDevice(const AUDIO_DEVICE& iDevice)
{
  if (m_iDevice == iDevice)
    return true;

  if (iDevice == DEFAULT_DEVICE)
  {
    bool audioOnAllSpeakers(false);
    SetupSpeakerConfig(2, audioOnAllSpeakers);
    return SetActiveDevice(DIRECTSOUND_DEVICE);
  }

  // remove current active device
  RemoveActiveDevice();

  m_iDevice = iDevice;
  if (m_iDevice == DIRECTSOUND_DEVICE)
  {
    if (DirectSoundCreate(NULL, &m_pDirectSoundDevice, NULL) < 0)
    {
      CLog::Log(LOGERROR, "%s - Failed to create DirectSound device!", __FUNCTION__);
      return false;
    }
  }
  else if (m_iDevice == AC97_DEVICE)
  {
    if (Ac97CreateMediaObject(DSAC97_CHANNEL_DIGITAL, NULL, NULL, &m_pAC97Device) < 0)
    {
      CLog::Log(LOGERROR, "%s - Failed to create AC97 device!", __FUNCTION__);
      return false;
    }
  }
  return true;
}

void CAudioContext::RemoveActiveDevice()
{
  CServiceBroker::GetGUI()->GetAudioManager().DeInitialize(m_iDevice);
  m_iDevice = NONE;

  if (m_pAC97Device)
  {
    m_pAC97Device->Release();
    m_pAC97Device = NULL;
  }
  if (m_pDirectSoundDevice)
  {
    m_pDirectSoundDevice->Release();
    m_pDirectSoundDevice = NULL;
  }
}

void CAudioContext::SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers, bool bIsMusic)
{
  bAudioOnAllSpeakers = false;

  DWORD spconfig = DSSPEAKER_USE_DEFAULT;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH))
  {
    bool outputToAllSpeakers =
        (bIsMusic && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICPLAYER_OUTPUT_TO_ALL_SPEAKERS)) ||
        (!bIsMusic && CMediaSettings::GetInstance().GetCurrentVideoSettings().m_OutputToAllSpeakers);
    if (outputToAllSpeakers && g_audioConfig.GetAC3Enabled())
    {
      bAudioOnAllSpeakers = true;
    }
    else if (iChannels == 1)
    {
      spconfig = DSSPEAKER_MONO;
    }
    else if (outputToAllSpeakers)
    {
      // check if surround mode is allowed, if not then use normal stereo
      spconfig = DSSPEAKER_STEREO;
      if (XC_AUDIO_FLAGS_BASIC(XGetAudioFlags()) == XC_AUDIO_FLAGS_SURROUND)
        spconfig = DSSPEAKER_SURROUND;
    }
    else if (iChannels == 2)
    {
      spconfig = DSSPEAKER_STEREO;
    }
  }
  else
  {
    // check if surround mode is allowed, if not then use normal stereo
    spconfig = DSSPEAKER_STEREO;
    if (iChannels == 1)
      spconfig = DSSPEAKER_MONO;
    else if (XC_AUDIO_FLAGS_BASIC(XGetAudioFlags()) == XC_AUDIO_FLAGS_SURROUND)
      spconfig = DSSPEAKER_SURROUND;
  }

  DWORD spconfig_old = DSSPEAKER_USE_DEFAULT;
  if (m_pDirectSoundDevice)
  {
    m_pDirectSoundDevice->GetSpeakerConfig(&spconfig_old);
    DWORD spconfig_default = XGetAudioFlags();
    if (spconfig_old == spconfig_default)
      spconfig_old = DSSPEAKER_USE_DEFAULT;
  }

  if (spconfig != spconfig_old)
  {
    // speaker config has changed, caller need to recreate it
    RemoveActiveDevice();
    DirectSoundOverrideSpeakerConfig(spconfig);
  }
}

bool CAudioContext::IsAC3EncoderActive() const
{
  return g_audioConfig.GetAC3Enabled();
}

bool CAudioContext::GetMixBin(DSMIXBINVOLUMEPAIR* dsmbvp, int* MixBinCount, DWORD* dwChannelMask, int Type, int Channels)
{
  //3, 5, >6 channel are invalid XBOX wav formats thus can not be processed at this stage

  if (Type == 0 || Type == DSMIXBINTYPE_DMO)
  { // FL, FR, C, LFE, BL, BR, (FLC, FRC, BC, SL, SR, TC, TFL, TFC, TFR, TBL, TBC, TBR)
    // This is the standard windows format, any channel can be left out, the channel mask indicate
    // wich ones are present. Let's use the standard features for this.

    *MixBinCount = 0;
    if (*dwChannelMask == 0)
    { // no channel mask specified, generate one
      switch (Channels)
      {
        case 6:
          *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
          break;
        case 5:
          *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
          break;
        case 4:
          *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
          break;
        case 3:
          *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER;
          break;
        case 2:
          *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
          break;
        case 1:
          *dwChannelMask = SPEAKER_FRONT_CENTER;
          break;
      }
    }
    return true;
  }

  *MixBinCount = Channels;

  if (Channels == 6) //Handle 6 channels.
  {
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;

    switch (Type)
    {
    case DSMIXBINTYPE_AAC:  //C, FL, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_OGG:  //FL, C, FR, SL, SR, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    case DSMIXBINTYPE_STANDARD:  //FL, FR, SL, SR, C, LFE
      {
        DSMIXBINVOLUMEPAIR dsm[6] =
          {
            {DSMIXBIN_FRONT_LEFT , 0},
            {DSMIXBIN_FRONT_RIGHT, 0},
            {DSMIXBIN_BACK_LEFT, 0},
            {DSMIXBIN_BACK_RIGHT, 0},
            {DSMIXBIN_FRONT_CENTER, 0},
            {DSMIXBIN_LOW_FREQUENCY, 0}
          };
        memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
        return true;
      }
    }
    //Didn't manage to get anything
    CLog::Log(LOGERROR, "Invalid Mixbin type specified, reverting to standard");
    GetMixBin(dsmbvp, MixBinCount, dwChannelMask, DSMIXBINTYPE_STANDARD, Channels);
    return true;
  }
  else if (Channels == 4)
  {
    DSMIXBINVOLUMEPAIR dsm[4] = { DSMIXBINVOLUMEPAIRS_DEFAULT_4CHANNEL };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    return true;
  }
  else if (Channels == 2)
  {
    if ( Type == DSMIXBINTYPE_STEREOALL )
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_FRONT_RIGHT, 0},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_BACK_RIGHT, 0},
          // left and right both to center and LFE, but attenuate each 3dB first
          // so they're the same level.
          // attenuate the center another 3dB so that it is a total 6dB lower
          // so that stereo effect is not lost.
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_LOW_FREQUENCY, -301},
          {DSMIXBIN_FRONT_CENTER, -602},
          {DSMIXBIN_FRONT_CENTER, -602}
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREOLEFT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // left route to 4 channels
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else if (Type == DSMIXBINTYPE_STEREORIGHT)
    {
      *MixBinCount = 8;
      DSMIXBINVOLUMEPAIR dsm[8] =
        {
          // right route to 4 channels
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_LEFT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_FRONT_RIGHT , 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_LEFT, 0},
          {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MIN},
          {DSMIXBIN_BACK_RIGHT, 0},
        };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }
    else
    {
      DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_STEREO };
      memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
      *dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    }
    return true;
  }
  else if (Channels == 1)
  {
    *MixBinCount = 2;
    DSMIXBINVOLUMEPAIR dsm[2] = { DSMIXBINVOLUMEPAIRS_DEFAULT_MONO };
    memcpy(dsmbvp, &dsm, sizeof(DSMIXBINVOLUMEPAIR)*(*MixBinCount));
    *dwChannelMask = SPEAKER_FRONT_LEFT;
    return true;
  }
  CLog::Log(LOGERROR, "Invalid Mixbin channels specified, get MixBins failed");
  return false;
}
