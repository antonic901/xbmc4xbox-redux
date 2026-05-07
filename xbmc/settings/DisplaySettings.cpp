/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplaySettings.h"

#include "ServiceBroker.h"
#include "XBVideoConfig.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <boost/boost/bind.hpp>
#include <cstdlib>
#include <float.h>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::MESSAGING;

using namespace KODI::MESSAGING::HELPERS;

static RESOLUTION_INFO EmptyResolution;
static RESOLUTION_INFO EmptyModifiableResolution;

CDisplaySettings::CDisplaySettings()
{
  m_resolutions.insert(m_resolutions.begin(), RES_AUTORES, RESOLUTION_INFO());

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
  m_resolutionChangeAborted = false;
}

CDisplaySettings::~CDisplaySettings() {}

CDisplaySettings& CDisplaySettings::GetInstance()
{
  static CDisplaySettings sDisplaySettings;
  return sDisplaySettings;
}

bool CDisplaySettings::Load(const TiXmlNode *settings)
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();

  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = settings->FirstChildElement("resolutions");
  if (!pElement)
  {
    CLog::Log(LOGERROR, "CDisplaySettings: settings file doesn't contain <resolutions>");
    return false;
  }

  const TiXmlElement *pResolution = pElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this calibration
    RESOLUTION_INFO cal;

    XMLUtils::GetString(pResolution, "description", cal.strMode);
    XMLUtils::GetInt(pResolution, "subtitles", cal.iSubtitles);
    XMLUtils::GetFloat(pResolution, "pixelratio", cal.fPixelRatio);
#ifdef HAVE_X11
    XMLUtils::GetFloat(pResolution, "refreshrate", cal.fRefreshRate);
    XMLUtils::GetString(pResolution, "output", cal.strOutput);
    XMLUtils::GetString(pResolution, "xrandrid", cal.strId);
#endif

    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      XMLUtils::GetInt(pOverscan, "left", cal.Overscan.left);
      XMLUtils::GetInt(pOverscan, "top", cal.Overscan.top);
      XMLUtils::GetInt(pOverscan, "right", cal.Overscan.right);
      XMLUtils::GetInt(pOverscan, "bottom", cal.Overscan.bottom);
    }

    // mark calibration as not updated
    // we must not delete those, resolution just might not be available
    cal.iWidth = cal.iHeight = 0;

    // store calibration, avoid adding duplicates
    bool found = false;
    for (ResolutionInfos::const_iterator  it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
    {
      if (StringUtils::EqualsNoCase(it->strMode, cal.strMode))
      {
        found = true;
        break;
      }
    }
    if (!found)
      m_calibrations.push_back(cal);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }

  ApplyCalibrations();
  return true;
}

bool CDisplaySettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  TiXmlElement xmlRootElement("resolutions");
  TiXmlNode *pRoot = settings->InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  // save calibrations
  for (ResolutionInfos::const_iterator it = m_calibrations.begin(); it != m_calibrations.end(); ++it)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    if (pNode == NULL)
      return false;

    // Now write each of the pieces of information we need...
    XMLUtils::SetString(pNode, "description", it->strMode);
    XMLUtils::SetInt(pNode, "subtitles", it->iSubtitles);
    XMLUtils::SetFloat(pNode, "pixelratio", it->fPixelRatio);
#ifdef HAVE_X11
    XMLUtils::SetFloat(pNode, "refreshrate", it->fRefreshRate);
    XMLUtils::SetString(pNode, "output", it->strOutput);
    XMLUtils::SetString(pNode, "xrandrid", it->strId);
#endif

    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    if (pOverscanNode == NULL)
      return false;

    XMLUtils::SetInt(pOverscanNode, "left", it->Overscan.left);
    XMLUtils::SetInt(pOverscanNode, "top", it->Overscan.top);
    XMLUtils::SetInt(pOverscanNode, "right", it->Overscan.right);
    XMLUtils::SetInt(pOverscanNode, "bottom", it->Overscan.bottom);
  }

  return true;
}

void CDisplaySettings::Clear()
{
  CSingleLock lock(m_critical);
  m_calibrations.clear();
  m_resolutions.clear();
  m_resolutions.insert(m_resolutions.begin(), RES_AUTORES, RESOLUTION_INFO());

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
}

bool CDisplaySettings::OnSettingChanging(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_VIDEOSCREEN_RESOLUTION)
  {
    RESOLUTION oldRes = GetCurrentResolution();
    RESOLUTION newRes = (RESOLUTION)boost::static_pointer_cast<const CSettingInt>(setting)->GetValue();

    SetCurrentResolution(newRes, false);
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(newRes, false);

    // check if the old or the new resolution was/is windowed
    // in which case we don't show any prompt to the user
    if (oldRes != newRes)
    {
      if (!m_resolutionChangeAborted)
      {
        if (HELPERS::ShowYesNoDialogText(13110, 13111, "",
                                         "", 15000) != YES)
        {
          m_resolutionChangeAborted = true;
          return false;
        }
      }
      else
        m_resolutionChangeAborted = false;
    }
  }
  else if (settingId == "videoscreen.flickerfilter" || settingId == "videoscreen.soften")
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(CDisplaySettings::GetInstance().GetCurrentResolution(), TRUE);
  else if (StringUtils::StartsWith(settingId, "videooutput."))
  {
    if (settingId == "videooutput.aspect")
    {
      switch(boost::static_pointer_cast<const CSettingInt>(setting)->GetValue())
      {
      case VIDEO_NORMAL:
        g_videoConfig.SetNormal();
        break;
      case VIDEO_LETTERBOX:
        g_videoConfig.SetLetterbox(true);
        break;
      case VIDEO_WIDESCREEN:
        g_videoConfig.SetWidescreen(true);
        break;
      }
    }
    else if (settingId == "videooutput.hd480p")
      g_videoConfig.Set480p(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == "videooutput.hd720p")
      g_videoConfig.Set720p(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == "videooutput.hd1080i")
      g_videoConfig.Set1080i(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());

    if (g_videoConfig.NeedsSave())
      g_videoConfig.Save();
  }

  return true;
}

void CDisplaySettings::SetCurrentResolution(RESOLUTION resolution, bool save /* = false */)
{
  if (save)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(CSettings::SETTING_VIDEOSCREEN_RESOLUTION, static_cast<int>(resolution));
  }

  if (resolution == RES_AUTORES)
    m_currentResolution = g_videoConfig.GetBestMode();
  else
    m_currentResolution = resolution;
}

RESOLUTION CDisplaySettings::GetDisplayResolution() const
{
  return static_cast<RESOLUTION>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOSCREEN_RESOLUTION));
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index) const
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
    return EmptyResolution;

  return m_resolutions[index];
}

const RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution) const
{
  if (resolution <= RES_INVALID)
    return EmptyResolution;

  return GetResolutionInfo((size_t)resolution);
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(size_t index)
{
  CSingleLock lock(m_critical);
  if (index >= m_resolutions.size())
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return m_resolutions[index];
}

RESOLUTION_INFO& CDisplaySettings::GetResolutionInfo(RESOLUTION resolution)
{
  if (resolution <= RES_INVALID)
  {
    EmptyModifiableResolution = RESOLUTION_INFO();
    return EmptyModifiableResolution;
  }

  return GetResolutionInfo((size_t)resolution);
}

void CDisplaySettings::AddResolutionInfo(const RESOLUTION_INFO &resolution)
{
  CSingleLock lock(m_critical);
  m_resolutions.push_back(resolution);
}

void CDisplaySettings::ApplyCalibrations()
{
  CSingleLock lock(m_critical);
  // apply all calibrations to the resolutions
  for (ResolutionInfos::const_iterator itCal = m_calibrations.begin(); itCal != m_calibrations.end(); ++itCal)
  {
    // find resolutions
    for (size_t res = 0; res < m_resolutions.size(); ++res)
    {
      if (StringUtils::EqualsNoCase(itCal->strMode, m_resolutions[res].strMode))
      {
        // overscan
        m_resolutions[res].Overscan.left = itCal->Overscan.left;
        if (m_resolutions[res].Overscan.left < -m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = -m_resolutions[res].iWidth/4;
        if (m_resolutions[res].Overscan.left > m_resolutions[res].iWidth/4)
          m_resolutions[res].Overscan.left = m_resolutions[res].iWidth/4;

        m_resolutions[res].Overscan.top = itCal->Overscan.top;
        if (m_resolutions[res].Overscan.top < -m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = -m_resolutions[res].iHeight/4;
        if (m_resolutions[res].Overscan.top > m_resolutions[res].iHeight/4)
          m_resolutions[res].Overscan.top = m_resolutions[res].iHeight/4;

        m_resolutions[res].Overscan.right = itCal->Overscan.right;
        if (m_resolutions[res].Overscan.right < m_resolutions[res].iWidth / 2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth / 2;
        if (m_resolutions[res].Overscan.right > m_resolutions[res].iWidth * 3/2)
          m_resolutions[res].Overscan.right = m_resolutions[res].iWidth *3/2;

        m_resolutions[res].Overscan.bottom = itCal->Overscan.bottom;
        if (m_resolutions[res].Overscan.bottom < m_resolutions[res].iHeight / 2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].Overscan.bottom > m_resolutions[res].iHeight * 3/2)
          m_resolutions[res].Overscan.bottom = m_resolutions[res].iHeight * 3/2;

        m_resolutions[res].iSubtitles = itCal->iSubtitles;
        if (m_resolutions[res].iSubtitles < 0)
          m_resolutions[res].iSubtitles = 0;
        if (m_resolutions[res].iSubtitles > m_resolutions[res].iHeight * 3 / 2)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight * 3 / 2;

        m_resolutions[res].fPixelRatio = itCal->fPixelRatio;
        if (m_resolutions[res].fPixelRatio < 0.5f)
          m_resolutions[res].fPixelRatio = 0.5f;
        if (m_resolutions[res].fPixelRatio > 2.0f)
          m_resolutions[res].fPixelRatio = 2.0f;
        break;
      }
    }
  }
}

static bool ModeEquals(const RESOLUTION_INFO& lhs, const RESOLUTION_INFO& rhs) { return StringUtils::EqualsNoCase(lhs.strMode, rhs.strMode); }

void CDisplaySettings::UpdateCalibrations()
{
  CSingleLock lock(m_critical);

  // Add new (unique) resolutions
  for (ResolutionInfos::const_iterator res = m_resolutions.begin(); res != m_resolutions.end(); ++res)
    if (std::find_if(m_calibrations.begin(), m_calibrations.end(),
      boost::bind(&ModeEquals, *res, _1)) == m_calibrations.end())
        m_calibrations.push_back(*res);

  for (ResolutionInfos::iterator cal = m_calibrations.begin(); cal != m_calibrations.end(); ++cal)
  {
    ResolutionInfos::const_iterator res(std::find_if(m_resolutions.begin(), m_resolutions.end(),
    boost::bind(&ModeEquals, *res, _1)));

    if (res != m_resolutions.end())
    {
      //! @todo erase calibrations with default values
      *cal = *res;
    }
  }
}

void CDisplaySettings::SettingOptionsResolutionsFiller(const SettingConstPtr& setting,
                                                       std::vector<IntegerSettingOption>& list,
                                                       int& current,
                                                       void* data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(16316), RES_AUTORES));

  std::vector<RESOLUTION> resolutions;
  CServiceBroker::GetWinSystem()->GetGfxContext().GetAllowedResolutions(resolutions, false);
  for (std::vector<RESOLUTION>::const_iterator resolution = resolutions.begin(); resolution != resolutions.end(); ++resolution)
  {
    RESOLUTION_INFO res2 = CDisplaySettings::GetInstance().GetResolutionInfo(*resolution);
    list.push_back(IntegerSettingOption(res2.strMode, *resolution));
  }
}

void CDisplaySettings::SettingOptionsFramerateconversionsFiller(const SettingConstPtr& setting,
                                                                std::vector<IntegerSettingOption>& list,
                                                                int& current,
                                                                void* data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(13340), FRAME_RATE_LEAVE_AS_IS));
  list.push_back(IntegerSettingOption(g_videoConfig.HasPAL() ? g_localizeStrings.Get(38716) : g_localizeStrings.Get(38717), FRAME_RATE_CONVERT));
  if (g_videoConfig.HasPAL() && g_videoConfig.HasPAL60())
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(38718), FRAME_RATE_USE_PAL60));
}

