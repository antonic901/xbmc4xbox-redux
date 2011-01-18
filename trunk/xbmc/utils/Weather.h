#pragma once

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

#include "InfoLoader.h"
#include "StdString.h"

#define WEATHER_LABEL_LOCATION   10
#define WEATHER_IMAGE_CURRENT_ICON 21
#define WEATHER_LABEL_CURRENT_COND 22
#define WEATHER_LABEL_CURRENT_TEMP 23
#define WEATHER_ISFETCHED 24
#define WEATHER_LABEL_FANART_CODE 25


class CBackgroundWeatherLoader : public CBackgroundLoader
{
public:
  CBackgroundWeatherLoader(CInfoLoader *pCallback) : CBackgroundLoader(pCallback) {};
};

class CWeather : public CInfoLoader
{
public:
  CWeather(void);
  virtual ~CWeather(void);

  CStdString GetLocation(int iLocation);
  bool IsFetched();
  void Reset();
  void RefreshInfo();

  void SetArea(int iArea) { m_iCurWeather = iArea; };
  unsigned int GetMaxLocations();

protected:
  virtual const char *TranslateInfo(int info);
  virtual DWORD TimeToNextRefreshInMs();
  void SetInfo();

  CStdString m_szLocation[10];

  unsigned int m_iCurWeather;
  int m_MaxLocations;

  char m_CurrentCondition[256];
  char m_CurrentConditionIcon[256];
  char m_CurrentTemperature[32];
  char m_CurrentLocation[256];
  char m_CurrentFanartCode[32];
  char m_IsFetched[32];
};

extern CWeather g_weatherManager;
