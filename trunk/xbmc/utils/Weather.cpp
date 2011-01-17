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

CStdString CWeather::TranslateInfo(int info) const
{
  CGUIWindow *window = g_windowManager.GetWindow(WINDOW_WEATHER);
  if (window)
  {
    if (info == WEATHER_LABEL_CURRENT_COND)
      return window->GetProperty("Current.Condition");
    else if (info == WEATHER_IMAGE_CURRENT_ICON)
      return window->GetProperty("Current.ConditionIcon");
    else if (info == WEATHER_LABEL_CURRENT_TEMP)
      return window->GetProperty("Current.Temperature");
    else if (info == WEATHER_LABEL_LOCATION)
      return window->GetProperty("Location");
    else if (info == WEATHER_ISFETCHED)
      return window->GetProperty("Weather.IsFetched");
    else if (info == WEATHER_LABEL_FANART_CODE)
      return window->GetProperty("Current.FanartCode");
  }
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
  for (int i = 0; i < MAX_LOCATION; i++)
  {
    m_szLocation[i] = "";
  }
  m_MaxLocations = -1;
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return TranslateInfo(WEATHER_ISFETCHED).Equals("true");
}
