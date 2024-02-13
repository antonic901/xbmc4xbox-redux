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

#include "InfoLoader.h"
#include "StdString.h"
#include "settings/ISettingCallback.h"
#include "utils/GlobalsHandling.h"

#include <map>

class TiXmlElement;

#define WEATHER_LABEL_LOCATION   10
#define WEATHER_IMAGE_CURRENT_ICON 21
#define WEATHER_LABEL_CURRENT_COND 22
#define WEATHER_LABEL_CURRENT_TEMP 23
#define WEATHER_LABEL_CURRENT_FEEL 24
#define WEATHER_LABEL_CURRENT_UVID 25
#define WEATHER_LABEL_CURRENT_WIND 26
#define WEATHER_LABEL_CURRENT_DEWP 27
#define WEATHER_LABEL_CURRENT_HUMI 28

#define MAX_LOCATION 3

struct day_forecast
{
  CStdString m_icon;
  CStdString m_overview;
  CStdString m_day;
  CStdString m_high;
  CStdString m_low;
};

#define NUM_DAYS 7

class CWeatherInfo
{
public:
  day_forecast forecast[NUM_DAYS];

  void Reset()
  {
    lastUpdateTime = "";
    currentIcon = "";
    currentConditions = "";
    currentTemperature = "";
    currentFeelsLike = "";
    currentWind = "";
    currentHumidity = "";
    currentUVIndex = "";
    currentDewPoint = "";

    for (int i = 0; i < NUM_DAYS; i++)
    {
      forecast[i].m_icon = "";
      forecast[i].m_overview = "";
      forecast[i].m_day = "";
      forecast[i].m_high = "";
      forecast[i].m_low = "";
    }
  };

  CStdString lastUpdateTime;
  CStdString location;
  CStdString currentIcon;
  CStdString currentConditions;
  CStdString currentTemperature;
  CStdString currentFeelsLike;
  CStdString currentUVIndex;
  CStdString currentWind;
  CStdString currentDewPoint;
  CStdString currentHumidity;
  CStdString busyString;
  CStdString naIcon;
};

class CWeatherJob : public CJob
{
public:
  CWeatherJob(const CStdString &areaCode);

  virtual bool DoWork();

  const CWeatherInfo &GetInfo() const;
private:
  bool LoadWeather(const CStdString& strWeatherFile); //parse strWeatherFile
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &value, const CStdString& strDefaultValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
  void LocalizeOverview(CStdString &str);
  void LocalizeOverviewToken(CStdString &str);
  void LocalizeDay(CStdString &day);
  void LoadLocalizedToken();
  int ConvertSpeed(int speed);
  std::map<CStdString, int> m_localizedTokens;
  typedef std::map<CStdString, int>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  CStdString m_areaCode;

  static bool m_imagesOkay;
};

class CWeather : public CInfoLoader,
                 public ISettingCallback
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);

  CStdString GetLocation(int iLocation);
  const CStdString &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  const day_forecast &GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  void SetArea(int iLocation);
  int GetArea() const;

  static CStdString GetAreaCode(const CStdString &codeAndCity);
  static CStdString GetAreaCity(const CStdString &codeAndCity);

protected:
  virtual CJob *GetJob() const;
  virtual CStdString TranslateInfo(int info) const;
  virtual CStdString BusyInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

private:
  CStdString m_location[MAX_LOCATION];

  CWeatherInfo m_info;
};

XBMC_GLOBAL_REF(CWeather, g_weatherManager);
#define g_weatherManager XBMC_GLOBAL_USE(CWeather)
