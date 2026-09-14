/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIComponent.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <string>

// forward definitions
class CAction;
class CSettings;
class TiXmlNode;
class CGUISound;

enum WINDOW_SOUND { SOUND_INIT = 0, SOUND_DEINIT };

class CGUIAudioManager : public ISettingCallback
{
  class CWindowSounds
  {
  public:
    boost::shared_ptr<CGUISound> initSound;
    boost::shared_ptr<CGUISound> deInitSound;
  };

public:
  CGUIAudioManager();
  virtual ~CGUIAudioManager();

  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual bool OnSettingUpdate(const boost::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode);

  void Initialize();
  void DeInitialize();

  bool Load();
  void UnLoad();


  void PlayActionSound(const CAction& action);
  void PlayWindowSound(int id, WINDOW_SOUND event);
  void PlayPythonSound(const std::string& strFileName, bool useCached = true);

  void Enable(bool bEnable);
  void SetVolume(float level);
  void Stop();

  /*!
  \brief Used on Xbox to clear buffer of inactive sounds.
  */
  void FreeUnused();

private:
  // Construction parameters
  boost::shared_ptr<CSettings> m_settings;

  typedef std::map<const std::string, boost::weak_ptr<CGUISound> > soundCache;
  typedef std::map<int, boost::shared_ptr<CGUISound> > actionSoundMap;
  typedef std::map<int, CWindowSounds> windowSoundMap;
  typedef std::map<const std::string, boost::shared_ptr<CGUISound> > pythonSoundsMap;

  soundCache          m_soundCache;
  actionSoundMap      m_actionSoundMap;
  windowSoundMap      m_windowSoundMap;
  pythonSoundsMap     m_pythonSounds;

  std::string          m_strMediaDir;
  bool                m_bEnabled;

  CCriticalSection    m_cs;

  boost::shared_ptr<CGUISound> LoadSound(const std::string& filename);
  boost::shared_ptr<CGUISound> LoadWindowSound(TiXmlNode* pWindowNode,
                                            const std::string& strIdentifier);
};

