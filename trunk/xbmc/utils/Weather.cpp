/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Weather.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIMediaWindow.h"
#include "ScriptSettings.h"

using namespace std;

#define MAX_LOCATION   10

CWeather g_weatherManager;

CWeather::CWeather(void) : CInfoLoader("weather")
{
  Reset();
}

CWeather::~CWeather(void)
{
}

const char *CWeather::TranslateInfo(int info)
{
  SetInfo();
  if (info == WEATHER_LABEL_CURRENT_COND)
    return m_CurrentCondition;
  else if (info == WEATHER_IMAGE_CURRENT_ICON)
    return m_CurrentConditionIcon;
  else if (info == WEATHER_LABEL_CURRENT_TEMP)
    return m_CurrentTemperature;
  else if (info == WEATHER_LABEL_LOCATION)
    return m_CurrentLocation;
  else if (info == WEATHER_LABEL_FANART_CODE)
    return m_CurrentFanartCode;
  else if (info == WEATHER_ISFETCHED)
    return m_IsFetched;
  return "";
}

DWORD CWeather::TimeToNextRefreshInMs()
{ // 15 minutes
  return 15 * 60 * 1000;
}

CStdString CWeather::GetLocation(int iLocation)
{
  if (m_szLocation[iLocation].IsEmpty())
  {
    CStdString cScriptPath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
    CScriptSettings* settings = new CScriptSettings();
    settings->Clear();
    settings->Load(cScriptPath);
    CStdString setting;
    setting.Format("town%i", iLocation + 1);
    m_szLocation[iLocation] = settings->Get(setting);
  }
  return m_szLocation[iLocation];
}

unsigned int CWeather::GetMaxLocations()
{
  if (m_MaxLocations == -1)
  {
    CStdString cScriptPath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
    CScriptSettings* settings = new CScriptSettings();
    settings->Clear();
    settings->Load(cScriptPath);
    m_MaxLocations = atoi(settings->Get("maxlocations")) + 1;
  }
  return m_MaxLocations;
}

void CWeather::Reset()
{
  RefreshInfo();
  for (unsigned int i = 0; i < MAX_LOCATION; i++)
  {
    m_szLocation[i] = "";
  }
  m_MaxLocations = -1;
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  return (strcmp(GetInfo(WEATHER_ISFETCHED), "true") == 0);
}

void CWeather::RefreshInfo()
{
  strcpy(m_CurrentCondition, "");
  strcpy(m_CurrentConditionIcon, "");
  strcpy(m_CurrentTemperature, "");
  strcpy(m_CurrentLocation, m_szLocation[m_iCurWeather]);
  strcpy(m_CurrentFanartCode, "");
  strcpy(m_IsFetched, "false");
}

void CWeather::SetInfo()
{
  if ((strcmp(m_CurrentCondition, "") == 0 || strcmp(m_CurrentConditionIcon, "") == 0 ||
      strcmp(m_CurrentTemperature, "") == 0 || strcmp(m_CurrentFanartCode, "") == 0) && 
      strcmp(m_IsFetched, "true") == 0)
  {
    CGUIWindow *window = g_windowManager.GetWindow(WINDOW_WEATHER);
    if (window)
    {
      // set these so existing weather infolabels don't break
      strcpy(m_CurrentCondition, ((CGUIMediaWindow*)window)->GetProperty("Current.Condition").c_str());
      strcpy(m_CurrentConditionIcon, ((CGUIMediaWindow*)window)->GetProperty("Current.ConditionIcon").c_str());
      strcpy(m_CurrentTemperature, ((CGUIMediaWindow*)window)->GetProperty("Current.Temperature").c_str());
      strcpy(m_CurrentLocation, ((CGUIMediaWindow*)window)->GetProperty("Location").c_str());
      strcpy(m_CurrentFanartCode, ((CGUIMediaWindow*)window)->GetProperty("Current.FanartCode").c_str());
    }
  }
  else if (strcmp(m_IsFetched, "false") == 0)
  {
    CGUIWindow *window = g_windowManager.GetWindow(WINDOW_WEATHER);
    if (window)
    {
      strcpy(m_IsFetched, ((CGUIMediaWindow*)window)->GetProperty("Weather.IsFetched").c_str());
    }
  }
}
