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

#include "settings/ISettingsHandler.h"

#define XC_AUDIO_FLAGS 9

class XBAudioConfig : public ISettingsHandler
{
public:
  XBAudioConfig();
  ~XBAudioConfig();

  virtual void OnSettingsLoaded();

  bool HasDigitalOutput();

  bool GetAC3Enabled();
  void SetAC3Enabled(bool bEnable);
  bool GetDTSEnabled();
  void SetDTSEnabled(bool bEnable);
  bool GetAACEnabled();
  void SetAACEnabled(bool bEnable);
  bool GetMP1Enabled();
  void SetMP1Enabled(bool bEnable);
  bool GetMP2Enabled();
  void SetMP2Enabled(bool bEnable);
  bool GetMP3Enabled();
  void SetMP3Enabled(bool bEnable);
  bool NeedsSave();
  void Save();

private:
  DWORD m_dwAudioFlags;
};

extern XBAudioConfig g_audioConfig;
