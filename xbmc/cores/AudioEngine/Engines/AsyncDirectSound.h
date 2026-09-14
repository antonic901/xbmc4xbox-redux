/*
 *  Copyright (C) 2005-2018 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectSoundRenderer.h"

class CASyncDirectSound : public IDirectSoundRenderer
{
public:
  CASyncDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, const char* strAudioCodec = "", bool bIsMusic = false);
  virtual ~CASyncDirectSound();

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
  virtual void DoWork();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);
  virtual void SetDynamicRangeCompression(long drc);

private:
  void ApplyDynamicRangeCompression(void *dest, const void *source, const int bytes);

  IAudioCallback* m_pCallback;
  LONG m_lFadeVolume;

  bool FindFreePacket( DWORD& pdwIndex );

  LPDIRECTSOUNDSTREAM m_pStream;
  LPDIRECTSOUND8 m_pDSound;

  WAVEFORMATEX m_wfx;

  LONG m_nCurrentVolume;
  DWORD m_dwPacketSize;
  bool m_AudioEOF;
  DWORD m_dwNumPackets;
  PBYTE* m_pbSampleData;
  DWORD* m_adwStatus;
  LARGE_INTEGER m_LastPacketCompletedAt;
  bool m_bPause;
  bool m_bIsPlaying;
  bool m_bIsAllocated;
  bool m_bFirstPackets;
  PBYTE m_VisBuffer;
  DWORD m_VisBytes;
  DWORD m_VisMaxBytes;

  LONGLONG m_startTime;
  LONGLONG m_delay;
  FLOAT m_fCurDelay;
  int m_iCalcDelay;
  WAVEFORMATEXTENSIBLE m_wfxex;
  LONGLONG m_TicksPerSec;
  int m_iAudioSkip;
  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;

  int m_iCurrentAudioStream;   //The Variable tracking LEFT/RIGHT/STEREO

  // Dynamic range compensation stuff
  short *m_drcTable;      // lookup table for fast pow() function
  int m_drcAmount;        // amount of dynamic range compression in milliBels.
};
