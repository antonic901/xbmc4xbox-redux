/*
 *  Copyright (C) 2005-2026 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GlobalsHandling.h"

#define DSMIXBINTYPE_STANDARD 1
#define DSMIXBINTYPE_DMO 2
#define DSMIXBINTYPE_AAC 3
#define DSMIXBINTYPE_OGG 4
#define DSMIXBINTYPE_CUSTOM 5
#define DSMIXBINTYPE_STEREOALL 6
#define DSMIXBINTYPE_STEREOLEFT 7
#define DSMIXBINTYPE_STEREORIGHT 8

class CAudioContext
{
public:
  enum AUDIO_DEVICE
  {
    NONE = 0,
    DEFAULT_DEVICE,
    DIRECTSOUND_DEVICE,
    AC97_DEVICE
  };

  CAudioContext();
  virtual ~CAudioContext() {}

  int GetActiveDevice() { return m_iDevice; }
  bool SetActiveDevice(const AUDIO_DEVICE& iDevice);

  LPDIRECTSOUND8 GetDirectSoundDevice() { return m_pDirectSoundDevice; }
  LPAC97MEDIAOBJECT GetAc97Device() { return m_pAC97Device; }

  void SetupSpeakerConfig(int iChannels, bool& bAudioOnAllSpeakers, bool bIsMusic = true);

  bool IsAC3EncoderActive() const;
  bool IsPassthroughActive() const { return m_iDevice == AC97_DEVICE; }

  bool GetMixBin(DSMIXBINVOLUMEPAIR* dsmbvp, int* MixBinCount, DWORD* dwChannelMask, int Type, int Channels);

protected:
  void RemoveActiveDevice();

  LPDIRECTSOUND8 m_pDirectSoundDevice;
  LPAC97MEDIAOBJECT m_pAC97Device;

  AUDIO_DEVICE m_iDevice;
  bool m_bAC3EncoderActive;
};

XBMC_GLOBAL_REF(CAudioContext,g_audioContext);
#define g_audioContext XBMC_GLOBAL_USE(CAudioContext)
