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

#include <set>

#include "settings/ISettingsHandler.h"
#include "settings/ISubSettings.h"
#include "utils/CriticalSection.h"
#include "utils/StdString.h"

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

class CGUISettings;
class TiXmlElement;
class TiXmlNode;

class CSettings : private ISettingsHandler, ISubSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  void RegisterSettingsHandler(ISettingsHandler *settingsHandler);
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);
  void RegisterSubSettings(ISubSettings *subSettings);
  void UnregisterSubSettings(ISubSettings *subSettings);

  void Initialize();

  bool Load();
  void Save() const;
  bool Reset();

  void Clear();

  int m_HttpApiBroadcastPort;
  int m_HttpApiBroadcastLevel;
  int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
  int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.

  int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
  bool m_bMute;

  VOICE_MASK m_karaokeVoiceMask[4];

  CStdString GetFFmpegDllFolder() const;
  CStdString GetPlayerName(const int& player) const;
  CStdString GetDefaultVideoPlayerName() const;
  CStdString GetDefaultAudioPlayerName() const;

  CStdString GetAvpackSettingsFile() const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  static bool GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue);
  static bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
protected:
  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool SaveAvpackXML() const;
  bool SaveNewAvpackXML() const;
  bool SaveAvpackSettings(TiXmlNode *io_pRoot) const;
  bool LoadAvpackXML();

private:
  // implementation of ISettingsHandler
  virtual bool OnSettingsLoading();
  virtual void OnSettingsLoaded();
  virtual bool OnSettingsSaving() const;
  virtual void OnSettingsSaved() const;
  virtual void OnSettingsCleared();

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  CCriticalSection m_critical;
  typedef std::set<ISettingsHandler*> SettingsHandlers;
  SettingsHandlers m_settingsHandlers;
  typedef std::set<ISubSettings*> SubSettings;
  SubSettings m_subSettings;
};

extern class CSettings g_settings;