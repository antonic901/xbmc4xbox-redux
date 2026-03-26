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

#include <string>

class CGUISound
{
public:
  CGUISound();
  virtual ~CGUISound();

  bool        Load(const std::string& strFile);
  void        Play();
  void        Stop();
  bool        IsPlaying();
  void        SetVolume(int level);

private:
#if 0
  bool        LoadWav(const std::string& strFile, WAVEFORMATEX* wfx, LPBYTE* ppWavData, int* pDataSize);
  bool        CreateBuffer(LPWAVEFORMATEX wfx, int iLength);
  bool        FillBuffer(LPBYTE pbData, int iLength);
#endif
  void        FreeBuffer();

#if 0
  LPDIRECTSOUNDBUFFER m_soundBuffer;
#endif
};
