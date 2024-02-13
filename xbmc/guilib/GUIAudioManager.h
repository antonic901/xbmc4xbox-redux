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

#include <map>

#include "settings/ISettingCallback.h"
#include "IAudioDeviceChangedCallback.h"
#include "threads/CriticalSection.h"

// forward definitions
class CAction;
class TiXmlNode;
class CGUISound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager : public ISettingCallback
{
  class CWindowSounds
  {
  public:
    CStdString strInitFile;
    CStdString strDeInitFile;
  };

public:
  CGUIAudioManager();
  virtual ~CGUIAudioManager();

  virtual void OnSettingChanged(const CSetting *setting);

  virtual void Initialize(int iDevice);
  virtual void DeInitialize(int iDevice);

  bool Load();

  void PlayActionSound(const CAction& action);
  void PlayWindowSound(int id, WINDOW_SOUND event);
  void PlayPythonSound(const CStdString& strFileName);

  void FreeUnused();

  void Enable(bool bEnable);
  void SetVolume(int iLevel);
  void Stop();
private:
  bool LoadWindowSound(TiXmlNode* pWindowNode, const CStdString& strIdentifier, CStdString& strFile);

  typedef std::map<int, CStdString> actionSoundMap;
  typedef std::map<int, CWindowSounds> windowSoundMap;

  typedef std::map<CStdString, CGUISound*> pythonSoundsMap;
  typedef std::map<int, CGUISound*> windowSoundsMap;

  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;

  CGUISound*          m_actionSound;
  windowSoundsMap     m_windowSounds;
  pythonSoundsMap     m_pythonSounds;

  CStdString          m_strMediaDir;
  bool                m_bEnabled;

  CCriticalSection    m_cs;
};

extern CGUIAudioManager g_audioManager;
