/*
 *  Copyright (C) 2005-2018 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectSoundRenderer.h"

class CAc97DirectSound : public IDirectSoundRenderer
{
public:
  CAc97DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bAC3DTS = true);
  virtual ~CAc97DirectSound();

  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  virtual FLOAT GetCacheTime();
  virtual FLOAT GetCacheTotal();
  virtual DWORD AddPackets(unsigned char* data, DWORD len);
  virtual DWORD GetSpace();
  virtual HRESULT Deinitialize();
  virtual HRESULT Pause();
  virtual HRESULT Stop();
  virtual HRESULT Resume();
  virtual LONG GetMinimumVolume() const;
  virtual LONG GetMaximumVolume() const;
  virtual LONG GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual HRESULT SetCurrentVolume(float volume);
  static void CALLBACK StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus);
  void StreamCallback(LPVOID pPacketContext, DWORD dwStatus);
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers){};

private:
  IAudioCallback* m_pCallback;
  LONG m_lFadeVolume;
  bool FindFreePacket( DWORD& pdwIndex );

  LPAC97MEDIAOBJECT m_pDigitalOutput;
  DWORD m_dwPacketSize;
  DWORD m_dwNumPackets;
  PBYTE m_pbSampleData[64];
  DWORD* m_adwStatus;
  DWORD m_dwTotalBytesAdded;
  bool m_bPause;
  bool m_bMute;
  bool m_bIsAllocated;
  LPDIRECTSOUND8 m_pDSound;

  //add for 44.1KHz 2 Channel audio Passthrough after software resample
  bool m_bAc3DTS; //input stream property
};
