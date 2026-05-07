/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "windowing/GraphicContext.h" // RESOLUTION

#include <map>
#include <set>
#include <utility>
#include <vector>

class TiXmlNode;
struct IntegerSettingOption;
struct StringSettingOption;

class CDisplaySettings : public ISettingCallback, public ISubSettings,
                         public Observable
{
public:
  static CDisplaySettings& GetInstance();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  virtual bool OnSettingChanging(const boost::shared_ptr<const CSetting>& setting);

  /*!
   \brief Returns the currently active resolution

   This resolution might differ from the display resolution which is based on
   the user's settings.

   \sa SetCurrentResolution
   \sa GetResolutionInfo
   \sa GetDisplayResolution
   */
  RESOLUTION GetCurrentResolution() const { return m_currentResolution; }
  void SetCurrentResolution(RESOLUTION resolution, bool save = false);
  /*!
   \brief Returns the best-matching resolution of the videoscreen.screenmode setting value

   This resolution might differ from the current resolution which is based on
   the properties of the operating system and the attached displays.

   \sa GetCurrentResolution
   */
  RESOLUTION GetDisplayResolution() const;

  const RESOLUTION_INFO& GetResolutionInfo(size_t index) const;
  const RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution) const;
  RESOLUTION_INFO& GetResolutionInfo(size_t index);
  RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution);
  size_t ResolutionInfoSize() const { return m_resolutions.size(); }
  void AddResolutionInfo(const RESOLUTION_INFO &resolution);

  const RESOLUTION_INFO& GetCurrentResolutionInfo() const { return GetResolutionInfo(m_currentResolution); }
  RESOLUTION_INFO& GetCurrentResolutionInfo() { return GetResolutionInfo(m_currentResolution); }

  void ApplyCalibrations();
  void UpdateCalibrations();

  float GetZoomAmount() const { return m_zoomAmount; }
  void SetZoomAmount(float zoomAmount) { m_zoomAmount = zoomAmount; }
  float GetPixelRatio() const { return m_pixelRatio; }
  void SetPixelRatio(float pixelRatio) { m_pixelRatio = pixelRatio; }
  static void SettingOptionsResolutionsFiller(const boost::shared_ptr<const CSetting>& setting,
                                              std::vector<IntegerSettingOption>& list,
                                              int& current,
                                              void* data);
  static void SettingOptionsFramerateconversionsFiller(const boost::shared_ptr<const CSetting>& setting,
                                              std::vector<IntegerSettingOption>& list,
                                              int& current,
                                              void* data);


protected:
  CDisplaySettings();
  CDisplaySettings(const CDisplaySettings&);
  CDisplaySettings& operator=(CDisplaySettings const&);
  virtual ~CDisplaySettings();

private:
  // holds the real gui resolution
  RESOLUTION m_currentResolution;

  typedef std::vector<RESOLUTION_INFO> ResolutionInfos;
  ResolutionInfos m_resolutions;
  ResolutionInfos m_calibrations;

  float m_zoomAmount;         // current zoom amount
  float m_pixelRatio;         // current pixel ratio

  bool m_resolutionChangeAborted;
  mutable CCriticalSection m_critical;
};
