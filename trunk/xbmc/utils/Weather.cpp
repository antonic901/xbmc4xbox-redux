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
#include "GUIDialogPluginSettings.h"

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
  if (info == WEATHER_LABEL_CURRENT_COND) return m_szCurrentConditions;
  else if (info == WEATHER_IMAGE_CURRENT_ICON) return m_szCurrentIcon;
  else if (info == WEATHER_LABEL_CURRENT_TEMP) return m_szCurrentTemperature;
  else if (info == WEATHER_LABEL_LOCATION) return m_szCurrentLocation;
  return "";
}

DWORD CWeather::TimeToNextRefreshInMs()
{ // 10 minutes
  return 10 * 60 * 1000;
}

char *CWeather::GetLocation(int iLocation)
{
  if (strlen(m_szLocation[iLocation]) == 0)
  {
    CStdString cScriptPath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
    CScriptSettings* settings = new CScriptSettings();
    settings->Clear();
    settings->Load(cScriptPath);
    CStdString setting;
    setting.Format("town%i", iLocation + 1);
    strcpy(m_szLocation[iLocation], settings->Get(setting).c_str());
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
  strcpy(m_szLastUpdateTime, "false");
  strcpy(m_szCurrentIcon,"");
  strcpy(m_szCurrentConditions, "");
  strcpy(m_szCurrentTemperature, "");
  strcpy(m_szCurrentLocation, "");

  for (int i = 0; i < MAX_LOCATION; i++)
  {
    strcpy(m_szLocation[i], "");
  }
  m_MaxLocations = -1;
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  SetInfo();
  return (strcmp(m_szLastUpdateTime, "true") == 0);
}

void CWeather::SetInfo()
{
  if (strcmp(m_szLastUpdateTime, "false") == 0 || strcmp(m_szLastUpdateTime, "") == 0)
  {
    CGUIWindow *window = g_windowManager.GetWindow(WINDOW_WEATHER);
    if (window)
    {
      // set these so existing weather infolabels don't break
      strcpy(m_szLastUpdateTime, ((CGUIMediaWindow*)window)->GetProperty("Weather.IsFetched").c_str());
      strcpy(m_szCurrentIcon, ((CGUIMediaWindow*)window)->GetProperty("Current.ConditionIcon").c_str());
      strcpy(m_szCurrentConditions, ((CGUIMediaWindow*)window)->GetProperty("Current.Condition").c_str());
      strcpy(m_szCurrentTemperature, ((CGUIMediaWindow*)window)->GetProperty("Current.Condition").c_str());
      strcpy(m_szCurrentLocation, ((CGUIMediaWindow*)window)->GetProperty("Location").c_str());
    }
  }
}
