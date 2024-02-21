/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include "DisplaySettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GraphicContext.h"
#include "guilib/gui3d.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "XBVideoConfig.h"

#include "defs_from_settings.h"

// 0.1 second increments
#define MAX_REFRESH_CHANGE_DELAY 200

using namespace std;

static RESOLUTION_INFO EmptyResolution;
static RESOLUTION_INFO EmptyModifiableResolution;

CDisplaySettings::CDisplaySettings()
{
  m_resolutions.insert(m_resolutions.begin(), RES_AUTORES, RESOLUTION_INFO());

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
}

CDisplaySettings::~CDisplaySettings()
{ }

CDisplaySettings& CDisplaySettings::Get()
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
      if (it->strMode.Equals(cal.strMode))
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

  m_zoomAmount = 1.0f;
  m_pixelRatio = 1.0f;
}

bool CDisplaySettings::OnSettingChanging(const CSetting *setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == "videoscreen.resolution")
  {
    // check if this is the revert call for a failed OnSettingChanging
    // in which case we don't want to ask the user again
    if (m_ignoreSettingChanging.find(make_pair(settingId, true)) == m_ignoreSettingChanging.end())
    {
      RESOLUTION newRes = RES_AUTORES;
      if (settingId == "videoscreen.resolution")
        newRes = (RESOLUTION)((CSettingInt*)setting)->GetValue();

      // We need to change and save videoscreen.resolution which will
      // trigger another call to this OnSettingChanging() which should not
      // trigger a user-input dialog which is already triggered by the callback
      // of the changed setting
      bool save = settingId != "videoscreen.resolution";
      if (save)
        m_ignoreSettingChanging.insert(make_pair("videoscreen.resolution", true));
      SetCurrentResolution(newRes, save);
      g_graphicsContext.SetVideoResolution(newRes);

      // check if this setting is temporarily blocked from showing the dialog
      if (m_ignoreSettingChanging.find(make_pair(settingId, false)) == m_ignoreSettingChanging.end())
      {
        bool cancelled = false;
        if (!CGUIDialogYesNo::ShowAndGetInput(13110, 13111, 20022, 20022, -1, -1, cancelled, 10000))
        {
          // we need to ignore the next OnSettingChanging() call for
          // the same setting which is executed to broadcast that
          // changing the setting has failed
          m_ignoreSettingChanging.insert(make_pair(settingId, false));
          return false;
        }
      }
      else
        m_ignoreSettingChanging.erase(make_pair(settingId, false));
    }
    else
      m_ignoreSettingChanging.erase(make_pair(settingId, true));
  }
  else if (settingId == "videoscreen.flickerfilter" || settingId == "videoscreen.soften")
    g_graphicsContext.SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution(), TRUE);
  else if (StringUtils2::StartsWith(settingId, "videooutput."))
  {
    if (settingId == "videooutput.aspect")
    {
      switch(((CSettingInt*)setting)->GetValue())
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
      g_videoConfig.Set480p(((CSettingBool*)setting)->GetValue());
    else if (settingId == "videooutput.hd720p")
      g_videoConfig.Set720p(((CSettingBool*)setting)->GetValue());
    else if (settingId == "videooutput.hd1080i")
      g_videoConfig.Set1080i(((CSettingBool*)setting)->GetValue());

    if (g_videoConfig.NeedsSave())
      g_videoConfig.Save();
  }

  return true;
}

bool CDisplaySettings::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
  if (settingId == "videoscreen.resolution")
  {
    CSettingString *screenmodeSetting = (CSettingString*)setting;
    std::string screenmode = screenmodeSetting->GetValue();
    // in Eden there was no character ("i" or "p") indicating interlaced/progressive
    // at the end so we just add a "p" and assume progressive
    if (screenmode.size() == 20)
      return screenmodeSetting->SetValue(screenmode + "p");
  }

  return false;
}

void CDisplaySettings::SetCurrentResolution(RESOLUTION resolution, bool save /* = false */)
{
  if (save)
    CSettings::Get().SetInt("videoscreen.resolution", (int)resolution);

  m_currentResolution = resolution;

  // SetChanged() is added in PVR pull request
  CSettings::Get().Save()/*g_guiSettings.SetChanged()*/;
}

RESOLUTION CDisplaySettings::GetDisplayResolution() const
{
  return (RESOLUTION)CSettings::Get().GetInt("videoscreen.resolution");
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
      if (itCal->strMode.Equals(m_resolutions[res].strMode))
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
        if (m_resolutions[res].iSubtitles < m_resolutions[res].iHeight / 2)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight / 2;
        if (m_resolutions[res].iSubtitles > m_resolutions[res].iHeight* 5/4)
          m_resolutions[res].iSubtitles = m_resolutions[res].iHeight* 5/4;

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

void CDisplaySettings::UpdateCalibrations()
{
  CSingleLock lock(m_critical);
  for (size_t res = RES_HDTV_1080i; res < m_resolutions.size(); ++res)
  {
    // find calibration
    bool found = false;
    for (ResolutionInfos::iterator itCal = m_calibrations.begin(); itCal != m_calibrations.end(); ++itCal)
    {
      if (itCal->strMode.Equals(m_resolutions[res].strMode))
      {
        // TODO: erase calibrations with default values
        *itCal = m_resolutions[res];
        found = true;
        break;
      }
    }

    if (!found)
      m_calibrations.push_back(m_resolutions[res]);
  }
}

void CDisplaySettings::SettingOptionsResolutionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  RESOLUTION res = RES_INVALID;

  vector<RESOLUTION> resolutions;
  g_graphicsContext.GetAllowedResolutions(resolutions, false);
  for (vector<RESOLUTION>::const_iterator it = resolutions.begin(); it != resolutions.end(); ++it)
  {
    RESOLUTION resolution = *it;
    RESOLUTION_INFO res1 = CDisplaySettings::Get().GetCurrentResolutionInfo();
    RESOLUTION_INFO res2 = CDisplaySettings::Get().GetResolutionInfo(resolution);

    list.push_back(make_pair(res2.strMode, resolution));

    if (
        res1.iWidth  == res2.iWidth &&
        res1.iHeight == res2.iHeight &&
        (res1.dwFlags & D3DPRESENTFLAG_INTERLACED) == (res2.dwFlags & D3DPRESENTFLAG_INTERLACED))
      res = resolution;
  }

  if (res != RES_INVALID)
    current = res;
}

void CDisplaySettings::SettingOptionsFramerateconversionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  list.push_back(make_pair(g_localizeStrings.Get(13340), FRAME_RATE_LEAVE_AS_IS));
  list.push_back(make_pair(g_videoConfig.HasPAL() ? g_localizeStrings.Get(38716) : g_localizeStrings.Get(38717), FRAME_RATE_CONVERT));
  if (g_videoConfig.HasPAL() && g_videoConfig.HasPAL60())
    list.push_back(make_pair(g_localizeStrings.Get(38718), FRAME_RATE_USE_PAL60));
}
