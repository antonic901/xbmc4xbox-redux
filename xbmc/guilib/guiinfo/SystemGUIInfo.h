/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
#include "utils/Temperature.h"

#include <memory>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CSystemGUIInfo : public CGUIInfoProvider
{
public:
  CSystemGUIInfo();
  virtual ~CSystemGUIInfo() {}

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  virtual bool InitCurrentItem(CFileItem *item);
  virtual bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const;
  virtual bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;
  virtual bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

  float GetFPS() const { return m_fps; }
  void UpdateFPS();

private:
  std::string GetSystemHeatInfo(int info) const;

  static const int SYSTEM_HEAT_UPDATE_INTERVAL = 60000;

  mutable unsigned int m_lastSysHeatInfoTime;
  mutable CTemperature m_gpuTemp;
  mutable CTemperature m_cpuTemp;
  mutable int m_fanSpeed;
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
