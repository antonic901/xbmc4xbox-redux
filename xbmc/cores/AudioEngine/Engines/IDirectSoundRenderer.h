/*
 *  Copyright (C) 2005-2018 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <xtl.h>

#include "cores/AudioEngine/Interfaces/IAudioCallback.h"

class IDirectSoundRenderer
{
public:
  IDirectSoundRenderer() {};
  virtual ~IDirectSoundRenderer() {};

  virtual void UnRegisterAudioCallback() = 0;
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;
  virtual FLOAT GetDelay() = 0;
  virtual FLOAT GetCacheTime() = 0;
  virtual FLOAT GetCacheTotal() = 0;

  virtual DWORD AddPackets(unsigned char* data, DWORD len) = 0;
  virtual bool IsResampling() { return false;};
  virtual DWORD GetSpace() = 0;
  virtual HRESULT Deinitialize() = 0;
  virtual HRESULT Pause() = 0;
  virtual HRESULT Stop() = 0;
  virtual HRESULT Resume() = 0;
  virtual DWORD GetChunkLen() = 0;

  virtual LONG GetMinimumVolume() const = 0;
  virtual LONG GetMaximumVolume() const = 0;
  virtual LONG GetCurrentVolume() const = 0;
  virtual void Mute(bool bMute) = 0;
  virtual HRESULT SetCurrentVolume(float volume) = 0;
  virtual void SetDynamicRangeCompression(long drc) {};
  virtual int SetPlaySpeed(int iSpeed) = 0;
  virtual void WaitCompletion() = 0;
  virtual void DoWork() {}
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers) = 0;

  static long ConvertVolumeToDSVolume(const float volume) { return static_cast<long>((volume - 1.0f) * 6000.0f); }
};
