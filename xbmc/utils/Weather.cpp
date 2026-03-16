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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "Weather.h"

#include <utility>

#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonSettings.h"
#include "Application.h"
#include "filesystem/Directory.h"
#include "filesystem/ZipManager.h"
#ifdef _XBOX
#include "filesystem/CurlFile.h"
#include "utils/JSONVariantParser.h"
#include "utils/MathUtils.h"
#include "utils/XBMCTinyXML.h"
#endif
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/Key.h"
#include "GUIUserMessages.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "LangInfo.h"
#include "log.h"
#include "xbox/Network.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "utils/POUtils.h"
#include "utils/Temperature.h"
#include "XBDateTime.h"
#include "XMLUtils.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

using namespace ADDON;
using namespace XFILE;

#define LOCALIZED_TOKEN_FIRSTID    370
#define LOCALIZED_TOKEN_LASTID     395
#define LOCALIZED_TOKEN_FIRSTID2  1350
#define LOCALIZED_TOKEN_LASTID2   1449
#define LOCALIZED_TOKEN_FIRSTID3    11
#define LOCALIZED_TOKEN_LASTID3     17
#define LOCALIZED_TOKEN_FIRSTID4    71
#define LOCALIZED_TOKEN_LASTID4     97

static const std::string IconAddonPath = "resource://resource.images.weathericons.default";

/*
FIXME'S
>strings are not centered
*/

CWeatherJob::CWeatherJob(int location)
{
  m_location = location;
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!g_application.getNetwork().IsAvailable())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(CSettings::GetInstance().GetString("weather.addon"), addon, ADDON_SCRIPT_WEATHER))
    return false;

  // initialize our sys.argv variables
  std::vector<std::string> argv;
  argv.push_back(addon->LibPath());

  std::string strSetting = StringUtils::Format("%i", m_location);
  argv.push_back(strSetting);

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");
  // call our script, passing the areacode
  int scriptId = -1;
  if (addon->ID() == "weather.xbmc.builtin")
  {
    int maxLocations = 0;
    for (int i = 1; i <= 5; ++i)
    {
      if (!addon->GetSetting(StringUtils::Format("Location%i", i)).empty())
        maxLocations++;
    }
    // locations are still not set
    if (!maxLocations)
      return true;

    std::string strLocation = addon->GetSetting(StringUtils::Format("Location%i", m_location));
    std::string strLocationID = addon->GetSetting(StringUtils::Format("Location%iID", m_location));
    FetchInternalWeather(strLocationID, strLocation, maxLocations);

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
  if ((scriptId = CScriptInvocationManager::GetInstance().ExecuteAsync(argv[0], addon, argv)) >= 0)
  {
    while (true)
    {
      if (!CScriptInvocationManager::GetInstance().IsRunning(scriptId))
        break;
      Sleep(100);
    }

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

const CWeatherInfo &CWeatherJob::GetInfo() const
{
  return m_info;
}

#ifdef _XBOX
const struct WeatherCode {
    const char* key;
    const char* value;
} WEATHER_ICONS[] =
{
  {"c4",          "26"},
  {"c4.st",       "26"},
  {"c4.r1",       "11"},
  {"c4.r1.st",    "4"},
  {"c4.r2",       "11"},
  {"c4.r2.st",    "4"},
  {"c4.r3",       "12"},
  {"c4.r3.st",    "4"},
  {"c4.s1",       "16"},
  {"c4.s1.st",    "16"},
  {"c4.s2",       "16"},
  {"c4.s2.st",    "16"},
  {"c4.s3",       "16"},
  {"c4.s3.st",    "16"},
  {"c4.rs1",      "5"},
  {"c4.rs1.st",   "5"},
  {"c4.rs2",      "5"},
  {"c4.rs2.st",   "5"},
  {"c4.rs3",      "5"},
  {"c4.rs3.st",   "5"},
  {"d",           "32"},
  {"d.st",        "32"},
  {"d.c2",        "30"},
  {"d.c2.r1",     "39"},
  {"d.c2.r1.st",  "37"},
  {"d.c2.r2",     "39"},
  {"d.c2.r2.st",  "37"},
  {"d.c2.r3",     "39"},
  {"d.c2.r3.st",  "37"},
  {"d.c2.rs1",    "42"},
  {"d.c2.rs1.st", "42"},
  {"d.c2.rs2",    "42"},
  {"d.c2.rs2.st", "42"},
  {"d.c2.rs3",    "42"},
  {"d.c2.rs3.st", "42"},
  {"d.c2.s1",     "41"},
  {"d.c2.s1.st",  "41"},
  {"d.c2.s2",     "41"},
  {"d.c2.s2.st",  "41"},
  {"d.c2.s3",     "41"},
  {"d.c2.s3.st",  "41"},
  {"d.c3",        "28"},
  {"d.c3.r1",     "11"},
  {"d.c3.r1.st",  "38"},
  {"d.c3.r2",     "11"},
  {"d.c3.r2.st",  "38"},
  {"d.c3.r3",     "11"},
  {"d.c3.r3.st",  "38"},
  {"d.c3.s1",     "14"},
  {"d.c3.s1.st",  "14"},
  {"d.c3.s2",     "14"},
  {"d.c3.s2.st",  "14"},
  {"d.c3.s3",     "14"},
  {"d.c3.s3.st",  "14"},
  {"d.c3.rs1",    "42"},
  {"d.c3.rs1.st", "42"},
  {"d.c3.rs2",    "42"},
  {"d.c3.rs2.st", "42"},
  {"d.c3.rs3",    "42"},
  {"d.c3.rs3.st", "42"},
  {"n",           "31"},
  {"n.st",        "31"},
  {"n.c2",        "29"},
  {"n.c2.r1",     "45"},
  {"n.c2.r1.st",  "47"},
  {"n.c2.r2",     "45"},
  {"n.c2.r2.st",  "47"},
  {"n.c2.r3",     "45"},
  {"n.c2.r3.st",  "47"},
  {"n.c2.rs1",    "42"},
  {"n.c2.rs1.st", "42"},
  {"n.c2.rs2",    "42"},
  {"n.c2.rs2.st", "42"},
  {"n.c2.rs3",    "42"},
  {"n.c2.rs3.st", "42"},
  {"n.c2.s1",     "46"},
  {"n.c2.s1.st",  "46"},
  {"n.c2.s2",     "46"},
  {"n.c2.s2.st",  "46"},
  {"n.c2.s3",     "46"},
  {"n.c2.s3.st",  "46"},
  {"n.c3",        "27"},
  {"n.c3.r1",     "11"},
  {"n.c3.r1.st",  "4"},
  {"n.c3.r2",     "11"},
  {"n.c3.r2.st",  "4"},
  {"n.c3.r3",     "11"},
  {"n.c3.r3.st",  "4"},
  {"n.c3.rs1",    "42"},
  {"n.c3.rs1.st", "42"},
  {"n.c3.rs2",    "42"},
  {"n.c3.rs2.st", "42"},
  {"n.c3.rs3",    "42"},
  {"n.c3.rs3.st", "42"},
  {"n.c3.s1",     "14"},
  {"n.c3.s1.st",  "14"},
  {"n.c3.s2",     "14"},
  {"n.c3.s2.st",  "14"},
  {"n.c3.s3",     "14"},
  {"n.c3.s3.st",  "14"},
  {"mist",        "32"},
  {"r1.mist",     "11"},
  {"r1.st.mist",  "38"},
  {"r2.mist",     "11"},
  {"r2.st.mist",  "38"},
  {"r3.mist",     "11"},
  {"r3.st.mist",  "38"},
  {"s1.mist",     "14"},
  {"s1.st.mist",  "14"},
  {"s2.mist",     "14"},
  {"s2.st.mist",  "14"},
  {"s3.mist",     "14"},
  {"s3.st.mist",  "14"},
  {"rs1.mist",    "42"},
  {"rs1.st.mist", "42"},
  {"rs2.mist",    "42"},
  {"rs2.st.mist", "42"},
  {"rs3.mist",    "42"},
  {"rs3.st.mist", "42"}
};

static const size_t WEATHER_ICONS_SIZE = sizeof(WEATHER_ICONS) / sizeof(WEATHER_ICONS[0]);

std::string weather_code_lookup(const std::string& key)
{
    for (size_t i = 0; i < WEATHER_ICONS_SIZE; ++i)
    {
      if (key == WEATHER_ICONS[i].key)
        return std::string(WEATHER_ICONS[i].value) + ".png";
    }
    return "na.png";
}

int CalculateDewPoint(double Tc = 0.0, double RH = 93.0, bool ext = true, double minRH = 0.0)
{
  // thanks to FrostBox @ http://forum.kodi.tv/showthread.php?tid=114637&pid=937168#pid937168
  double Es = 6.11 * std::pow(10.0, (7.5 * Tc) / (237.7 + Tc));
  if (RH == 0.0)
    RH = minRH;
  double E = (RH * Es) / 100.0;
  if (E > 0.0)
  {
    double lnE = std::log(E);
    double dewPoint = (-430.22 + 237.7 * lnE) / (-lnE + 19.08);
    return MathUtils::round_int(dewPoint);
  }
  return 0;
}

bool CWeatherJob::FetchInternalWeather(const std::string& strLocationID, const std::string& strLocation, const int maxLocations) const
{
  std::string strLocale = g_langInfo.GetLocale().GetLanguageCode();
  std::string strURL = "https://services.gismeteo.net/inform-service/inf_chrome/forecast/?lang=en&city=" + strLocationID;

  XFILE::CCurlFile httpUtil;
  std::string bodyResponse;
  if (!httpUtil.Get(strURL, bodyResponse))
  {
    CLog::Log(LOGWARNING, "Internal weather fetching failed");
    return false;
  }

  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  window->SetProperty("Current.Location", strLocation);
  window->SetProperty("Locations", maxLocations);

  CXBMCTinyXML doc;
  if (doc.Parse(bodyResponse))
  {
    TiXmlElement* element = doc.RootElement();
    if (!element)
      return false;

    element = element->FirstChildElement("location");
    if (!element)
      return false;

    // Parse current forecast
    element = element->FirstChildElement("fact");
    std::string strValue, strTemp;

    strValue = XMLUtils::GetAttribute(element, "sunrise");
    if (!strValue.empty())
    {
      time_t t = static_cast<time_t>(strtol(strValue.c_str(), 0, 10));
      CDateTime dateTime = CDateTime::FromUTCDateTime(t);
      window->SetProperty("Today.Sunrise", dateTime.GetAsLocalizedTime("HH:mm"));
    }

    strValue = XMLUtils::GetAttribute(element, "sunset");
    if (!strValue.empty())
    {
      time_t t = static_cast<time_t>(strtol(strValue.c_str(), 0, 10));
      CDateTime dateTime = CDateTime::FromUTCDateTime(t);
      window->SetProperty("Today.Sunset", dateTime.GetAsLocalizedTime("HH:mm"));
    }

    element = element->FirstChildElement("values");
    strValue = XMLUtils::GetAttribute(element, "hum");
    if (!strValue.empty())
    {
      window->SetProperty("Current.Humidity", strValue);
    }

    strValue = XMLUtils::GetAttribute(element, "ws");
    if (!strValue.empty())
    {
      int speed = MathUtils::round_int(atoi(strValue.c_str()) * 3.6);
      window->SetProperty("Current.Wind", speed);
      window->SetProperty("Current.WindSpeed", speed);
    }

    strValue = XMLUtils::GetAttribute(element, "wd");
    if (!strValue.empty())
    {
      if (strValue == "0")
        window->SetProperty("Current.WindDirection", "CALM");
      else if (strValue == "1")
        window->SetProperty("Current.WindDirection", "N");
      else if (strValue == "2")
        window->SetProperty("Current.WindDirection", "NE");
      else if (strValue == "3")
        window->SetProperty("Current.WindDirection", "E");
      else if (strValue == "4")
        window->SetProperty("Current.WindDirection", "SE");
      else if (strValue == "5")
        window->SetProperty("Current.WindDirection", "S");
      else if (strValue == "6")
        window->SetProperty("Current.WindDirection", "SW");
      else if (strValue == "7")
        window->SetProperty("Current.WindDirection", "W");
      else if (strValue == "8")
        window->SetProperty("Current.WindDirection", "NW");
    }

    strValue = XMLUtils::GetAttribute(element, "t");
    if (!strValue.empty())
    {
      window->SetProperty("Current.Temperature", strValue);
    }

    strValue = XMLUtils::GetAttribute(element, "hi");
    if (!strValue.empty())
    {
      window->SetProperty("Current.FeelsLike", strValue);
    }

    strValue = XMLUtils::GetAttribute(element, "descr");
    if (!strValue.empty())
    {
      window->SetProperty("Current.Condition", strValue);
    }

    strValue = XMLUtils::GetAttribute(element, "icon");
    if (!strValue.empty())
    {
      window->SetProperty("Current.OutlookIcon", "10.png");
    }

    strValue = XMLUtils::GetAttribute(element, "pt");
    if (!strValue.empty())
    {
      strValue += ".0 mm";
      window->SetProperty("Current.Precipitation", strValue);
    }

    int temperature = window->GetProperty("Current.Temperature").asInteger32();
    int humidity = window->GetProperty("Current.Humidity").asInteger32();
    window->SetProperty("Current.DewPoint", CalculateDewPoint(temperature, humidity));

    element = doc.RootElement()->FirstChildElement("location")->FirstChildElement("day");
    if (!element)
      return false;

    // Parse hourly forecast
    element = element->FirstChildElement("forecast");
    int i = 1;
    while(element)
    {
      const std::string strKey = StringUtils::Format("Hourly.%i.", i);

      strValue = XMLUtils::GetAttribute(element, "valid");
      if (!strValue.empty())
      {
        CDateTime dateTime = CDateTime::FromDBDateTime(strValue);
        window->SetProperty(strKey + "Time", dateTime.GetAsLocalizedTime("HH:mm"));

        strValue = StringUtils::Format("%i %s", dateTime.GetDay(), g_localizeStrings.Get(dateTime.GetMonth() + 50).c_str());
        window->SetProperty(strKey + "ShortDate", strValue);
      }

      TiXmlElement* elValues = element->FirstChildElement("values");
      strTemp = XMLUtils::GetAttribute(elValues, "t");
      if (!strTemp.empty())
      {
        FormatTemperature(strValue, strtod(strTemp.c_str(), nullptr));
        strValue += g_langInfo.GetTemperatureUnitString();
        window->SetProperty(strKey + "Temperature", strValue);
      }

      strValue = XMLUtils::GetAttribute(elValues, "pr");
      if (!strValue.empty())
      {
        strValue += ".0 mm";
        window->SetProperty(strKey + "Precipitation", strValue);
      }

      strValue = XMLUtils::GetAttribute(elValues, "descr");
      if (!strValue.empty())
      {
        window->SetProperty(strKey + "Outlook", strValue);
      }

      strValue = XMLUtils::GetAttribute(elValues, "icon");
      if (!strValue.empty())
      {
        window->SetProperty(strKey + "OutlookIcon", weather_code_lookup(strValue));
      }

      ++i;
      element = element->NextSiblingElement("forecast");
    }

    // Parse daily forecast
    element = doc.RootElement()->FirstChildElement("location")->FirstChildElement("day");
    i = 1;
    while(element)
    {
      const std::string strKey = StringUtils::Format("Daily.%i.", i);
      const std::string strKey2 = StringUtils::Format("Day%i.", i);

      strValue = XMLUtils::GetAttribute(element, "date");
      if (!strValue.empty())
      {
        CDateTime dateTime = CDateTime::FromDBDate(strValue);
        int dayOfWeek = dateTime.GetDayOfWeek();
        if (dayOfWeek == 0)
          dayOfWeek = 7;
        std::string strValue = g_localizeStrings.Get(dayOfWeek + 40);
        window->SetProperty(strKey + "ShortDay", strValue);
        window->SetProperty(strKey2 + "Title", strValue);
        strValue = StringUtils::Format("%i %s", dateTime.GetDay(), g_localizeStrings.Get(dateTime.GetMonth() + 50).c_str());
        window->SetProperty(strKey + "ShortDate", strValue);
      }

      strTemp = XMLUtils::GetAttribute(element, "tmin");
      if (!strTemp.empty())
      {
        FormatTemperature(strValue, strtod(strTemp.c_str(), nullptr));
        strValue += g_langInfo.GetTemperatureUnitString();
        window->SetProperty(strKey + "LowTemperature", strValue);
        window->SetProperty(strKey2 + "LowTemp", strValue);
      }

      strTemp = XMLUtils::GetAttribute(element, "tmax");
      if (!strTemp.empty())
      {
        FormatTemperature(strValue, strtod(strTemp.c_str(), nullptr));
        strValue += g_langInfo.GetTemperatureUnitString();
        window->SetProperty(strKey + "HighTemperature", strValue);
        window->SetProperty(strKey2 + "HighTemp", strValue);
      }

      strValue = XMLUtils::GetAttribute(element, "descr");
      if (!strValue.empty())
      {
        window->SetProperty(strKey + "Outlook", strValue);
        window->SetProperty(strKey2 + "Outlook", strValue);
      }

      strValue = XMLUtils::GetAttribute(element, "icon");
      if (!strValue.empty())
      {
        std::string strIcon = weather_code_lookup(strValue);
        window->SetProperty(strKey + "OutlookIcon", strIcon);
        window->SetProperty(strKey2 + "OutlookIcon", strIcon);
      }

      ++i;
      element = element->NextSiblingElement("day");
    }
  }

  window->SetProperty("Hourly.IsFetched", "true");
  window->SetProperty("Daily.IsFetched", "true");
  return true;
}
#endif

void CWeatherJob::LocalizeOverviewToken(std::string &token)
{
  // This routine is case-insensitive.
  std::string strLocStr;
  if (!token.empty())
  {
    ilocalizedTokens i;
    i = m_localizedTokens.find(token);
    if (i != m_localizedTokens.end())
    {
      strLocStr = g_localizeStrings.Get(i->second);
    }
  }
  if (strLocStr == "")
    strLocStr = token; //if not found, let fallback
  token = strLocStr;
}

void CWeatherJob::LocalizeOverview(std::string &str)
{
  std::vector<std::string> words = StringUtils::Split(str, " ");
  for (std::vector<std::string>::iterator i = words.begin(); i != words.end(); ++i)
    LocalizeOverviewToken(*i);
  str = StringUtils::Join(words, " ");
}

void CWeatherJob::FormatTemperature(std::string &text, double temp)
{
  CTemperature temperature = CTemperature::CreateFromCelsius(temp);
  text = StringUtils::Format("%.0f", temperature.To(g_langInfo.GetTemperatureUnit()));
}

void CWeatherJob::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  std::string language = LANGUAGE_DEFAULT;
  CSettingString* languageSetting = static_cast<CSettingString*>(CSettings::GetInstance().GetSetting("locale.language"));
  if (languageSetting != NULL)
    language = languageSetting->GetDefault();

  // Try the strings PO file first
  CPODocument PODoc;
  if (PODoc.LoadFile(URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.po")))
  {
    int counter = 0;

    while (PODoc.GetNextEntry())
    {
      if (PODoc.GetEntryType() != ID_FOUND)
        continue;

      uint32_t id = PODoc.GetEntryID();
      PODoc.ParseEntry(ISSOURCELANG);

      if (id > LOCALIZED_TOKEN_LASTID2) break;
      if ((LOCALIZED_TOKEN_FIRSTID  <= id && id <= LOCALIZED_TOKEN_LASTID)  ||
          (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
          (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
          (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
      {
        if (!PODoc.GetMsgid().empty())
        {
          m_localizedTokens.insert(make_pair(PODoc.GetMsgid(), id));
          counter++;
        }
      }
    }

    CLog::Log(LOGDEBUG, "POParser: loaded %i weather tokens", counter);
    return;
  }

  CLog::Log(LOGDEBUG,
            "Weather: no PO string file available, to load English tokens, "
            "fallback to strings.xml file");

  // We load the tokens from the strings.xml file
  std::string strLanguagePath = URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.xml");

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strLanguagePath) || !xmlDoc.RootElement())
  {
    CLog::Log(LOGERROR, "Weather: unable to load %s: %s at line %d", strLanguagePath.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->ValueStr() != "strings")
    return;

  const TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    std::string strValue = pChild->ValueStr();
    if (strValue == "string")
    { // Load new style language file with id as attribute
      const char* attrId = pChild->Attribute("id");
      if (attrId && !pChild->NoChildren())
      {
        int id = atoi(attrId);
        if ((LOCALIZED_TOKEN_FIRSTID  <= id && id <= LOCALIZED_TOKEN_LASTID)  ||
            (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
            (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
            (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
        {
          std::string utf8Label(pChild->FirstChild()->ValueStr());
          if (!utf8Label.empty())
            m_localizedTokens.insert(make_pair(utf8Label, id));
        }
      }
    }
    pChild = pChild->NextSiblingElement();
  }
}

static std::string ConstructPath(std::string in) // copy intended
{
  if (in.find("/") != std::string::npos || in.find("\\") != std::string::npos)
    return in;
  if (in.empty() || in == "N/A")
    in = "na.png";

  return URIUtils::AddFileToFolder(IconAddonPath, in);
}

void CWeatherJob::SetFromProperties()
{
  // Load in our tokens if necessary
  if (m_localizedTokens.empty())
    LoadLocalizedToken();

  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);
    m_info.currentConditions = window->GetProperty("Current.Condition").asString();
    m_info.currentIcon = ConstructPath(window->GetProperty("Current.OutlookIcon").asString());
    LocalizeOverview(m_info.currentConditions);
    FormatTemperature(m_info.currentTemperature,
        strtod(window->GetProperty("Current.Temperature").asString().c_str(), nullptr));
    FormatTemperature(m_info.currentFeelsLike,
        strtod(window->GetProperty("Current.FeelsLike").asString().c_str(), nullptr));
    m_info.currentUVIndex = window->GetProperty("Current.UVIndex").asString();
    LocalizeOverview(m_info.currentUVIndex);
    CSpeed speed = CSpeed::CreateFromKilometresPerHour(strtol(window->GetProperty("Current.Wind").asString().c_str(),0,10));
    std::string direction = window->GetProperty("Current.WindDirection").asString();
    if (direction == "CALM")
      m_info.currentWind = g_localizeStrings.Get(1410);
    else
    {
      LocalizeOverviewToken(direction);
      m_info.currentWind = StringUtils::Format(g_localizeStrings.Get(434).c_str(),
          direction.c_str(), (int)speed.To(g_langInfo.GetSpeedUnit()), g_langInfo.GetSpeedUnitString().c_str());
    }
    std::string windspeed = StringUtils::Format("%i %s", (int)speed.To(g_langInfo.GetSpeedUnit()), g_langInfo.GetSpeedUnitString().c_str());
    window->SetProperty("Current.WindSpeed",windspeed);
    FormatTemperature(m_info.currentDewPoint,
        strtod(window->GetProperty("Current.DewPoint").asString().c_str(), nullptr));
    if (window->GetProperty("Current.Humidity").asString().empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity = StringUtils::Format("%s%%", window->GetProperty("Current.Humidity").asString().c_str());
    m_info.location = window->GetProperty("Current.Location").asString();
    for (int i=0;i<NUM_DAYS;++i)
    {
      std::string strDay = StringUtils::Format("Day%i.Title",i);
      m_info.forecast[i].m_day = window->GetProperty(strDay).asString();
      LocalizeOverviewToken(m_info.forecast[i].m_day);
      strDay = StringUtils::Format("Day%i.HighTemp",i);
      FormatTemperature(m_info.forecast[i].m_high,
                    strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day%i.LowTemp",i);
      FormatTemperature(m_info.forecast[i].m_low,
                    strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day%i.OutlookIcon",i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      strDay = StringUtils::Format("Day%i.Outlook",i);
      m_info.forecast[i].m_overview = window->GetProperty(strDay).asString();
      LocalizeOverview(m_info.forecast[i].m_overview);
    }
  }
}

CWeather::CWeather(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  Reset();
}

CWeather::~CWeather(void)
{
}

std::string CWeather::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(IconAddonPath, "na.png");

  return CInfoLoader::BusyInfo(info);
}

std::string CWeather::TranslateInfo(int info) const
{
  if (info == WEATHER_LABEL_CURRENT_COND) return m_info.currentConditions;
  else if (info == WEATHER_IMAGE_CURRENT_ICON) return m_info.currentIcon;
  else if (info == WEATHER_LABEL_CURRENT_TEMP) return m_info.currentTemperature;
  else if (info == WEATHER_LABEL_CURRENT_FEEL) return m_info.currentFeelsLike;
  else if (info == WEATHER_LABEL_CURRENT_UVID) return m_info.currentUVIndex;
  else if (info == WEATHER_LABEL_CURRENT_WIND) return m_info.currentWind;
  else if (info == WEATHER_LABEL_CURRENT_DEWP) return m_info.currentDewPoint;
  else if (info == WEATHER_LABEL_CURRENT_HUMI) return m_info.currentHumidity;
  else if (info == WEATHER_LABEL_LOCATION) return m_info.location;
  return "";
}

/*!
 \brief Retrieve the city name for the specified location from the settings
 \param iLocation the location index (can be in the range [1..MAXLOCATION])
 \return the city name (without the accompanying region area code)
 */
std::string CWeather::GetLocation(int iLocation)
{
  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    std::string setting = StringUtils::Format("Location%i", iLocation);
    return window->GetProperty(setting).asString();
  }
  return "";
}

void CWeather::Reset()
{
  m_info.Reset();
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_info.lastUpdateTime.empty();
}

const day_forecast &CWeather::GetForecast(int day) const
{
  return m_info.forecast[day];
}

/*!
 \brief Saves the specified location index to the settings. Call Refresh()
        afterwards to update weather info for the new location.
 \param iLocation the new location index (can be in the range [1..MAXLOCATION])
 */
void CWeather::SetArea(int iLocation)
{
  CSettings::GetInstance().SetInt("weather.currentlocation", iLocation);
  CSettings::GetInstance().Save();
}

/*!
 \brief Retrieves the current location index from the settings
 \return the active location index (will be in the range [1..MAXLOCATION])
 */
int CWeather::GetArea() const
{
  return CSettings::GetInstance().GetInt("weather.currentlocation");
}

CJob *CWeather::GetJob() const
{
  return new CWeatherJob(GetArea());
}

void CWeather::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = ((CWeatherJob *)job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}

void CWeather::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == "weather.addon")
  {
    // clear "WeatherProviderLogo" property that some weather addons set
    CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
    window->SetProperty("WeatherProviderLogo", "");
    Refresh();
  }
}

void CWeather::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == "weather.addonsettings")
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(CSettings::GetInstance().GetString("weather.addon"), addon, ADDON_SCRIPT_WEATHER) && addon != NULL)
    { //! @todo maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
      Refresh();
    }
  }
}

