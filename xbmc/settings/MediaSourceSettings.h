/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "settings/lib/ISettingsHandler.h"

#include <string>

class CProfilesManager;
class TiXmlNode;

class CMediaSourceSettings : public ISettingsHandler
{
public:
  static CMediaSourceSettings& GetInstance();

  static std::string GetSourcesFile();

  virtual void OnSettingsLoaded();
  virtual void OnSettingsUnloaded();

  bool Load();
  bool Load(const std::string &file);
  bool Save();
  bool Save(const std::string &file) const;
  void Clear();

  VECSOURCES* GetSources(const std::string &type);
  const std::string& GetDefaultSource(const std::string &type) const;
  void SetDefaultSource(const std::string &type, const std::string &source);

  bool UpdateSource(const std::string &strType, const std::string &strOldName, const std::string &strUpdateChild, const std::string &strUpdateValue);
  bool DeleteSource(const std::string &strType, const std::string &strName, const std::string &strPath, bool virtualSource = false);
  bool AddShare(const std::string &type, const CMediaSource &share);
  bool UpdateShare(const std::string &type, const std::string &oldName, const CMediaSource &share);

protected:
  CMediaSourceSettings();
  CMediaSourceSettings(const CMediaSourceSettings&);
  CMediaSourceSettings& operator=(CMediaSourceSettings const&);
  virtual ~CMediaSourceSettings();

private:
  bool GetSource(const std::string& category, const TiXmlNode* source, CMediaSource& share);
  void GetSources(const TiXmlNode* rootElement,
                  const std::string& tagName,
                  VECSOURCES& items,
                  std::string& defaultString);
  bool SetSources(TiXmlNode* rootNode,
                  const char* section,
                  const VECSOURCES& shares,
                  const std::string& defaultPath) const;

  VECSOURCES m_programSources;
  VECSOURCES m_pictureSources;
  VECSOURCES m_fileSources;
  VECSOURCES m_musicSources;
  VECSOURCES m_videoSources;
  VECSOURCES m_gameSources;

  std::string m_defaultProgramSource;
  std::string m_defaultMusicSource;
  std::string m_defaultPictureSource;
  std::string m_defaultFileSource;
};
