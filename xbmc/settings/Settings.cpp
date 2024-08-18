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

#include "system.h"

#include "Settings.h"
#include "Application.h"
#include "Autorun.h"
#include "LangInfo.h"
#include "Util.h"
#include "addons/AddonSystemSettings.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Skin.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#ifdef HAS_XBOX_D3D
#include "cores/VideoRenderers/XBoxRenderer.h"
#endif
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIFontManager.h"
#include "guilib/common/Mouse.h"
#include "input/KeyboardLayoutManager.h"
#if defined(TARGET_POSIX)
#include "linux/LinuxTimezone.h"
#endif // defined(TARGET_POSIX)
#include "network/NetworkServices.h"
#include "network/upnp/UPnPSettings.h"
#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/XBMCHelper.h"
#endif // defined(TARGET_DARWIN_OSX)
#if defined(TARGET_DARWIN)
#include "platform/darwin/DarwinUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "SettingAddon.h"
#endif
#if defined(TARGET_RASPBERRY_PI)
#include "linux/RBP.h"
#endif
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#endif // defined(HAS_LIBAMCODEC)
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingConditions.h"
#include "settings/SettingUtils.h"
#include "settings/SkinSettings.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/RssManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Weather.h"
#include "utils/XBMCTinyXML.h"
#include "utils/SeekHandler.h"
#include "utils/Variant.h"
#include "view/ViewStateSettings.h"
#ifdef _XBOX
#include "utils/FanController.h"
#include "CdgParser.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "XBTimeZone.h"
#endif

#include "defs_from_settings.h"

#define SETTINGS_XML_FOLDER "special://xbmc/system/settings/"
#define SETTINGS_XML_ROOT   "settings"

using namespace XFILE;

CSettings::CSettings()
  : m_initialized(false)
{
  m_settingsManager = new CSettingsManager();
}

CSettings::~CSettings()
{
  Uninitialize();

  delete m_settingsManager;
}

CSettings& CSettings::GetInstance()
{
  static CSettings sSettings;
  return sSettings;
}

bool CSettings::Initialize()
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // option fillers and conditions need to be
  // initialized before the setting definitions
  InitializeOptionFillers();
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions())
    return false;

  m_settingsManager->SetInitialized();

  InitializeISettingsHandlers();
  InitializeISubSettings();
  InitializeISettingCallbacks();

  m_initialized = true;

  return true;
}

bool CSettings::Load()
{
  return Load(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Load(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  bool updated = false;
  if (!XFILE::CFile::Exists(file) || !xmlDoc.LoadFile(file) ||
      !m_settingsManager->Load(xmlDoc.RootElement(), updated))
  {
    CLog::Log(LOGERROR, "CSettings: unable to load settings from %s, creating new default settings", file.c_str());
    if (!Reset())
      return false;

    if (!Load(file))
      return false;
  }
  // if the settings had to be updated, we need to save the changes
  else if (updated)
    return Save(file);

  return true;
}

bool CSettings::Load(const TiXmlElement *root, bool hide /* = false */)
{
  if (root == NULL)
    return false;

  std::map<std::string, CSetting*> *loadedSettings = NULL;
  if (hide)
    loadedSettings = new std::map<std::string, CSetting*>();

  bool updated;
  // only trigger settings events if hiding is disabled
  bool success = m_settingsManager->Load(root, updated, !hide, loadedSettings);
  // if necessary hide all the loaded settings
  if (success && hide && loadedSettings != NULL)
  {
    for(std::map<std::string, CSetting*>::const_iterator setting = loadedSettings->begin(); setting != loadedSettings->end(); ++setting)
      setting->second->SetVisible(false);
  }
  delete loadedSettings;

  return success;
}

void CSettings::SetLoaded()
{
  m_settingsManager->SetLoaded();
}

bool CSettings::Save()
{
  return Save(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Save(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement rootElement(SETTINGS_XML_ROOT);
  TiXmlNode *root = xmlDoc.InsertEndChild(rootElement);
  if (root == NULL)
    return false;

  if (!m_settingsManager->Save(root))
    return false;

  return xmlDoc.SaveFile(file);
}

void CSettings::Unload()
{
  CSingleLock lock(m_critical);
  m_settingsManager->Unload();
}

void CSettings::Uninitialize()
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return;

  // unregister setting option fillers
  m_settingsManager->UnregisterSettingOptionsFiller("audiocdactions");
  m_settingsManager->UnregisterSettingOptionsFiller("audiocdencoders");
  m_settingsManager->UnregisterSettingOptionsFiller("charsets");
  m_settingsManager->UnregisterSettingOptionsFiller("fanspeeds");
  m_settingsManager->UnregisterSettingOptionsFiller("fontheights");
  m_settingsManager->UnregisterSettingOptionsFiller("fonts");
  m_settingsManager->UnregisterSettingOptionsFiller("languagenames");
  m_settingsManager->UnregisterSettingOptionsFiller("framerateconversions");
  m_settingsManager->UnregisterSettingOptionsFiller("regions");
  m_settingsManager->UnregisterSettingOptionsFiller("shortdateformats");
  m_settingsManager->UnregisterSettingOptionsFiller("longdateformats");
  m_settingsManager->UnregisterSettingOptionsFiller("timeformats");
  m_settingsManager->UnregisterSettingOptionsFiller("24hourclockformats");
  m_settingsManager->UnregisterSettingOptionsFiller("speedunits");
  m_settingsManager->UnregisterSettingOptionsFiller("temperatureunits");
  m_settingsManager->UnregisterSettingOptionsFiller("rendermethods");
  m_settingsManager->UnregisterSettingOptionsFiller("resolutions");
  m_settingsManager->UnregisterSettingOptionsFiller("videoseeksteps");
  m_settingsManager->UnregisterSettingOptionsFiller("startupwindows");
  m_settingsManager->UnregisterSettingOptionsFiller("audiostreamlanguages");
  m_settingsManager->UnregisterSettingOptionsFiller("subtitlestreamlanguages");
  m_settingsManager->UnregisterSettingOptionsFiller("subtitledownloadlanguages");
  m_settingsManager->UnregisterSettingOptionsFiller("iso6391languages");
  m_settingsManager->UnregisterSettingOptionsFiller("skincolors");
  m_settingsManager->UnregisterSettingOptionsFiller("skinfonts");
  m_settingsManager->UnregisterSettingOptionsFiller("skinthemes");
  m_settingsManager->UnregisterSettingOptionsFiller("targettemperatures");
  m_settingsManager->UnregisterSettingOptionsFiller("timezones");
  m_settingsManager->UnregisterSettingOptionsFiller("keyboardlayouts");
  m_settingsManager->UnregisterSettingOptionsFiller("voicemasks");

  // unregister ISettingCallback implementations
  m_settingsManager->UnregisterCallback(&g_advancedSettings);
  m_settingsManager->UnregisterCallback(&CMediaSettings::Get());
  m_settingsManager->UnregisterCallback(&CDisplaySettings::Get());
  m_settingsManager->UnregisterCallback(&CSeekHandler::Get());
  m_settingsManager->UnregisterCallback(&g_application);
  m_settingsManager->UnregisterCallback(&g_audioManager);
  m_settingsManager->UnregisterCallback(&g_charsetConverter);
#ifdef _XBOX
  m_settingsManager->UnregisterCallback(CFanController::Instance());
#endif
  m_settingsManager->UnregisterCallback(&g_langInfo);
#if defined(TARGET_WINDOWS) || defined(HAS_SDL_JOYSTICK)
  m_settingsManager->UnregisterCallback(&g_Joystick);
#endif
  m_settingsManager->UnregisterCallback(&g_Mouse);
  m_settingsManager->UnregisterCallback(&CNetworkServices::Get());
  m_settingsManager->UnregisterCallback(&g_passwordManager);
  m_settingsManager->UnregisterCallback(&CRssManager::Get());
  m_settingsManager->UnregisterCallback(&ADDON::CRepositoryUpdater::GetInstance());
#if defined(TARGET_LINUX) || defined(_XBOX)
  m_settingsManager->UnregisterCallback(&g_timezone);
#endif // defined(TARGET_LINUX)
  m_settingsManager->UnregisterCallback(&g_weatherManager);

  // cleanup the settings manager
  m_settingsManager->Clear();

  // unregister ISubSettings implementations
  m_settingsManager->UnregisterSubSettings(&g_application);
  m_settingsManager->UnregisterSubSettings(&CDisplaySettings::Get());
  m_settingsManager->UnregisterSubSettings(&CMediaSettings::Get());
  m_settingsManager->UnregisterSubSettings(&CSkinSettings::Get());
  m_settingsManager->UnregisterSubSettings(&g_sysinfo);
  m_settingsManager->UnregisterSubSettings(&CViewStateSettings::Get());

  // unregister ISettingsHandler implementations
  m_settingsManager->UnregisterSettingsHandler(&g_advancedSettings);
  m_settingsManager->UnregisterSettingsHandler(&CMediaSourceSettings::Get());
  m_settingsManager->UnregisterSettingsHandler(&CPlayerCoreFactory::Get());
  m_settingsManager->UnregisterSettingsHandler(&CRssManager::Get());
#ifdef HAS_UPNP
  m_settingsManager->UnregisterSettingsHandler(&CUPnPSettings::Get());
#endif
  m_settingsManager->UnregisterSettingsHandler(&CProfilesManager::Get());
  m_settingsManager->UnregisterSettingsHandler(&g_application);
#ifdef _XBOX
  m_settingsManager->UnregisterSettingsHandler(&g_audioConfig);
  m_settingsManager->UnregisterSettingsHandler(&g_videoConfig);
  m_settingsManager->UnregisterSettingsHandler(&g_timezone);
#endif

  m_initialized = false;
}

void CSettings::RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList)
{
  m_settingsManager->RegisterCallback(callback, settingList);
}

void CSettings::UnregisterCallback(ISettingCallback *callback)
{
  m_settingsManager->UnregisterCallback(callback);
}

CSetting* CSettings::GetSetting(const std::string &id) const
{
  CSingleLock lock(m_critical);
  if (id.empty())
    return NULL;

  return m_settingsManager->GetSetting(id);
}

std::vector<CSettingSection*> CSettings::GetSections() const
{
  CSingleLock lock(m_critical);
  return m_settingsManager->GetSections();
}

CSettingSection* CSettings::GetSection(const std::string &section) const
{
  CSingleLock lock(m_critical);
  if (section.empty())
    return NULL;

  return m_settingsManager->GetSection(section);
}

bool CSettings::GetBool(const std::string &id) const
{
  return m_settingsManager->GetBool(id);
}

bool CSettings::SetBool(const std::string &id, bool value)
{
  return m_settingsManager->SetBool(id, value);
}

bool CSettings::ToggleBool(const std::string &id)
{
  return m_settingsManager->ToggleBool(id);
}

int CSettings::GetInt(const std::string &id) const
{
  return m_settingsManager->GetInt(id);
}

bool CSettings::SetInt(const std::string &id, int value)
{
  return m_settingsManager->SetInt(id, value);
}

double CSettings::GetNumber(const std::string &id) const
{
  return m_settingsManager->GetNumber(id);
}

bool CSettings::SetNumber(const std::string &id, double value)
{
  return m_settingsManager->SetNumber(id, value);
}

std::string CSettings::GetString(const std::string &id) const
{
  return m_settingsManager->GetString(id);
}

bool CSettings::SetString(const std::string &id, const std::string &value)
{
  return m_settingsManager->SetString(id, value);
}

std::vector<CVariant> CSettings::GetList(const std::string &id) const
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeList)
    return std::vector<CVariant>();

  return CSettingUtils::GetList(static_cast<CSettingList*>(setting));
}

bool CSettings::SetList(const std::string &id, const std::vector<CVariant> &value)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeList)
    return false;

  return CSettingUtils::SetList(static_cast<CSettingList*>(setting), value);
}

bool CSettings::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  return m_settingsManager->LoadSetting(node, settingId);
}

bool CSettings::Initialize(const std::string &file)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file.c_str()))
  {
    CLog::Log(LOGERROR, "CSettings: error loading settings definition from %s, Line %d\n%s", file.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  CLog::Log(LOGDEBUG, "CSettings: loaded settings definition from %s", file.c_str());

  TiXmlElement *root = xmlDoc.RootElement();
  if (root == NULL)
    return false;

  return m_settingsManager->Initialize(root);
}

bool CSettings::InitializeDefinitions()
{
  if (!Initialize(SETTINGS_XML_FOLDER "settings.xml"))
  {
    CLog::Log(LOGFATAL, "Unable to load settings definitions");
    return false;
  }
#if defined(TARGET_WINDOWS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "win32.xml") && !Initialize(SETTINGS_XML_FOLDER "win32.xml"))
    CLog::Log(LOGFATAL, "Unable to load win32-specific settings definitions");
#elif defined(TARGET_ANDROID)
  if (CFile::Exists(SETTINGS_XML_FOLDER "android.xml") && !Initialize(SETTINGS_XML_FOLDER "android.xml"))
    CLog::Log(LOGFATAL, "Unable to load android-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-android.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-android.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-android-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_RASPBERRY_PI)
  if (CFile::Exists(SETTINGS_XML_FOLDER "rbp.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp-specific settings definitions");
  if (g_RBP.RasberryPiVersion() > 1 && CFile::Exists(SETTINGS_XML_FOLDER "rbp2.xml") && !Initialize(SETTINGS_XML_FOLDER "rbp2.xml"))
    CLog::Log(LOGFATAL, "Unable to load rbp2-specific settings definitions");
#elif defined(TARGET_FREEBSD)
  if (CFile::Exists(SETTINGS_XML_FOLDER "freebsd.xml") && !Initialize(SETTINGS_XML_FOLDER "freebsd.xml"))
    CLog::Log(LOGFATAL, "Unable to load freebsd-specific settings definitions");
#elif defined(HAS_IMXVPU)
  if (CFile::Exists(SETTINGS_XML_FOLDER "imx6.xml") && !Initialize(SETTINGS_XML_FOLDER "imx6.xml"))
    CLog::Log(LOGFATAL, "Unable to load imx6-specific settings definitions");
#elif defined(TARGET_LINUX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "linux.xml") && !Initialize(SETTINGS_XML_FOLDER "linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load linux-specific settings definitions");
#if defined(HAS_LIBAMCODEC)
  if (aml_present() && CFile::Exists(SETTINGS_XML_FOLDER "aml-linux.xml") && !Initialize(SETTINGS_XML_FOLDER "aml-linux.xml"))
    CLog::Log(LOGFATAL, "Unable to load aml-linux-specific settings definitions");
#endif // defined(HAS_LIBAMCODEC)
#elif defined(TARGET_DARWIN)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin.xml"))
    CLog::Log(LOGFATAL, "Unable to load darwin-specific settings definitions");
#if defined(TARGET_DARWIN_OSX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_osx.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_osx.xml"))
    CLog::Log(LOGFATAL, "Unable to load osx-specific settings definitions");
#elif defined(TARGET_DARWIN_IOS)
  if (CFile::Exists(SETTINGS_XML_FOLDER "darwin_ios.xml") && !Initialize(SETTINGS_XML_FOLDER "darwin_ios.xml"))
    CLog::Log(LOGFATAL, "Unable to load ios-specific settings definitions");
#endif
#elif defined(_XBOX)
  if (CFile::Exists(SETTINGS_XML_FOLDER "xbox.xml") && !Initialize(SETTINGS_XML_FOLDER "xbox.xml"))
    CLog::Log(LOGFATAL, "Unable to load xbox-specific settings definitions");
#endif

  // load any custom visibility and default values before loading the special
  // appliance.xml so that appliances are able to overwrite even those values
  InitializeVisibility();
  InitializeDefaults();

  if (CFile::Exists(SETTINGS_XML_FOLDER "appliance.xml") && !Initialize(SETTINGS_XML_FOLDER "appliance.xml"))
    CLog::Log(LOGFATAL, "Unable to load appliance-specific settings definitions");

  return true;
}

void CSettings::InitializeSettingTypes()
{
  // register "addon" and "path" setting types implemented by CSettingAddon
  m_settingsManager->RegisterSettingType("addon", this);
  m_settingsManager->RegisterSettingType("path", this);
}

void CSettings::InitializeControls()
{
  m_settingsManager->RegisterSettingControl("toggle", this);
  m_settingsManager->RegisterSettingControl("spinner", this);
  m_settingsManager->RegisterSettingControl("edit", this);
  m_settingsManager->RegisterSettingControl("button", this);
  m_settingsManager->RegisterSettingControl("list", this);
  m_settingsManager->RegisterSettingControl("slider", this);
  m_settingsManager->RegisterSettingControl("range", this);
  m_settingsManager->RegisterSettingControl("title", this);
}

void CSettings::InitializeVisibility()
{
  // hide some settings if necessary
#if defined(TARGET_DARWIN)
  CSettingString* timezonecountry = (CSettingString*)m_settingsManager->GetSetting(CSettings::SETTING_LOCALE_TIMEZONECOUNTRY);
  CSettingString* timezone = (CSettingString*)m_settingsManager->GetSetting(CSettings::SETTING_LOCALE_TIMEZONE);

  if (CDarwinUtils::GetIOSVersion() >= 4.3)
  {
    timezonecountry->SetRequirementsMet(false);
    timezone->SetRequirementsMet(false);
  }
#endif
}

void CSettings::InitializeDefaults()
{
  // set some default values if necessary
#if defined(HAS_SKIN_TOUCHED) && defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
  ((CSettingAddon*)m_settingsManager->GetSetting("lookandfeel.skin"))->SetDefault("skin.touched");
#endif

#if defined(_LINUX)
  CSettingString* timezonecountry = (CSettingString*)m_settingsManager->GetSetting("locale.timezonecountry");
  CSettingString* timezone = (CSettingString*)m_settingsManager->GetSetting("locale.timezone");

  if (timezonecountry->IsVisible())
    timezonecountry->SetDefault(g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()));
  if (timezone->IsVisible())
    timezone->SetDefault(g_timezone.GetOSConfiguredTimezone());
#endif // defined(_LINUX)

#if defined(TARGET_WINDOWS) || defined(_XBOX)
  #if defined(HAS_DX) || defined(HAS_XBOX_D3D)
  ((CSettingString*)m_settingsManager->GetSetting("musicplayer.visualisation"))->SetDefault("visualization.milkdrop");
  #endif

  #if !defined(HAS_GL) && !defined(HAS_XBOX_D3D)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  if (g_sysinfo.IsAeroDisabled())
    ((CSettingBool*)m_settingsManager->GetSetting("videoscreen.fakefullscreen"))->SetDefault(false);
  #endif
#endif

#if defined(HAS_WEB_SERVER)
  if (CUtil::CanBindPrivileged())
    ((CSettingInt*)m_settingsManager->GetSetting("services.webserverport"))->SetDefault(80);
#endif

#if defined(_XBOX)
  // actual values are set inside OnSettingsLoaded() callback
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (((CSettingInt*)m_settingsManager->GetSetting("audiooutput.mode"))->GetValue() == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    ((CSettingInt*)m_settingsManager->GetSetting("audiooutput.mode"))->SetDefault(AUDIO_ANALOG);
  ((CSettingBool*)m_settingsManager->GetSetting("audiooutput.ac3passthrough"))->SetDefault(g_audioConfig.GetAC3Enabled());
  ((CSettingBool*)m_settingsManager->GetSetting("audiooutput.dtspassthrough"))->SetDefault(g_audioConfig.GetDTSEnabled());

  if (g_videoConfig.HasLetterbox())
    ((CSettingInt*)m_settingsManager->GetSetting("videooutput.aspect"))->SetDefault(VIDEO_LETTERBOX);
  else if (g_videoConfig.HasWidescreen())
    ((CSettingInt*)m_settingsManager->GetSetting("videooutput.aspect"))->SetDefault(VIDEO_WIDESCREEN);
  else
    ((CSettingInt*)m_settingsManager->GetSetting("videooutput.aspect"))->SetDefault(VIDEO_NORMAL);
  ((CSettingBool*)m_settingsManager->GetSetting("videooutput.hd480p"))->SetDefault(g_videoConfig.Has480p());
  ((CSettingBool*)m_settingsManager->GetSetting("videooutput.hd720p"))->SetDefault(g_videoConfig.Has720p());
  ((CSettingBool*)m_settingsManager->GetSetting("videooutput.hd1080i"))->SetDefault(g_videoConfig.Has1080i());

  ((CSettingInt*)m_settingsManager->GetSetting("locale.timezone"))->SetDefault(g_timezone.GetTimeZoneIndex());
  ((CSettingBool*)m_settingsManager->GetSetting("locale.usedst"))->SetDefault(g_timezone.GetDST());
#endif
}

void CSettings::InitializeOptionFillers()
{
  // register setting option fillers
#ifdef HAS_DVD_DRIVE
  m_settingsManager->RegisterSettingOptionsFiller("audiocdactions", MEDIA_DETECT::CAutorun::SettingOptionAudioCdActionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiocdencoders", MEDIA_DETECT::CAutorun::SettingOptionAudioCdEncodersFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("charsets", CCharsetConverter::SettingOptionsCharsetsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("fanspeeds", CFanController::SettingOptionsSpeedsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("fonts", GUIFontManager::SettingOptionsFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("languagenames", CLangInfo::SettingOptionsLanguageNamesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("fontheights", GUIFontManager::SettingOptionsSubtitleHeightsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("framerateconversions", CDisplaySettings::SettingOptionsFramerateconversionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("regions", CLangInfo::SettingOptionsRegionsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("shortdateformats", CLangInfo::SettingOptionsShortDateFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("longdateformats", CLangInfo::SettingOptionsLongDateFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("timeformats", CLangInfo::SettingOptionsTimeFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("24hourclockformats", CLangInfo::SettingOptions24HourClockFormatsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("speedunits", CLangInfo::SettingOptionsSpeedUnitsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("temperatureunits", CLangInfo::SettingOptionsTemperatureUnitsFiller);
#ifdef HAS_XBOX_D3D
  m_settingsManager->RegisterSettingOptionsFiller("rendermethods", CXBoxRenderer::SettingOptionsRenderMethodsFiller);
#else
  m_settingsManager->RegisterSettingOptionsFiller("rendermethods", CBaseRenderer::SettingOptionsRenderMethodsFiller);
#endif
  m_settingsManager->RegisterSettingOptionsFiller("resolutions", CDisplaySettings::SettingOptionsResolutionsFiller);
//   m_settingsManager->RegisterSettingOptionsFiller("shutdownstates", CPowerManager::SettingOptionsShutdownStatesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("videoseeksteps", CSeekHandler::SettingOptionsSeekStepsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("startupwindows", ADDON::CSkinInfo::SettingOptionsStartupWindowsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("audiostreamlanguages", CLangInfo::SettingOptionsAudioStreamLanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("subtitlestreamlanguages", CLangInfo::SettingOptionsSubtitleStreamLanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("subtitledownloadlanguages", CLangInfo::SettingOptionsSubtitleDownloadlanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("iso6391languages", CLangInfo::SettingOptionsISO6391LanguagesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skincolors", ADDON::CSkinInfo::SettingOptionsSkinColorsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinfonts", ADDON::CSkinInfo::SettingOptionsSkinFontsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("skinthemes", ADDON::CSkinInfo::SettingOptionsSkinThemesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("targettemperatures", CFanController::SettingOptionsTemperaturesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("timezones", XBTimeZone::SettingOptionsTimezonesFiller);
  m_settingsManager->RegisterSettingOptionsFiller("keyboardlayouts", CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller);
  m_settingsManager->RegisterSettingOptionsFiller("voicemasks", CCdgParser::SettingOptionsVoiceMasksFiller);
}

void CSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string> &simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator itCondition = simpleConditions.begin(); itCondition != simpleConditions.end(); ++itCondition)
    m_settingsManager->AddCondition(*itCondition);

  // add more complex conditions
  const std::map<std::string, SettingConditionCheck> &complexConditions = CSettingConditions::GetComplexConditions();
  for (std::map<std::string, SettingConditionCheck>::const_iterator itCondition = complexConditions.begin(); itCondition != complexConditions.end(); ++itCondition)
    m_settingsManager->AddCondition(itCondition->first, itCondition->second);
}

void CSettings::InitializeISettingsHandlers()
{
  // register ISettingsHandler implementations
  // The order of these matters! Handlers are processed in the order they were registered.
  m_settingsManager->RegisterSettingsHandler(&g_advancedSettings);
  m_settingsManager->RegisterSettingsHandler(&CMediaSourceSettings::Get());
  m_settingsManager->RegisterSettingsHandler(&CPlayerCoreFactory::Get());
  m_settingsManager->RegisterSettingsHandler(&CProfilesManager::Get());
#ifdef HAS_UPNP
  m_settingsManager->RegisterSettingsHandler(&CUPnPSettings::Get());
#endif
  m_settingsManager->RegisterSettingsHandler(&CRssManager::Get());
  m_settingsManager->RegisterSettingsHandler(&g_langInfo);
  m_settingsManager->RegisterSettingsHandler(&g_application);
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
  m_settingsManager->RegisterSettingsHandler(&g_timezone);
#endif
#ifdef _XBOX
  m_settingsManager->RegisterSettingsHandler(&g_audioConfig);
  m_settingsManager->RegisterSettingsHandler(&g_videoConfig);
  m_settingsManager->RegisterSettingsHandler(&g_timezone);
#endif
  m_settingsManager->RegisterSettingsHandler(&CMediaSettings::Get());
}

void CSettings::InitializeISubSettings()
{
  // register ISubSettings implementations
  m_settingsManager->RegisterSubSettings(&g_application);
  m_settingsManager->RegisterSubSettings(&CDisplaySettings::Get());
  m_settingsManager->RegisterSubSettings(&CMediaSettings::Get());
  m_settingsManager->RegisterSubSettings(&CSkinSettings::Get());
  m_settingsManager->RegisterSubSettings(&g_sysinfo);
  m_settingsManager->RegisterSubSettings(&CViewStateSettings::Get());
}

void CSettings::InitializeISettingCallbacks()
{
  // register any ISettingCallback implementations
  std::set<std::string> settingSet;
  settingSet.insert("debug.showloginfo");
  settingSet.insert("debug.setextraloglevel");
  m_settingsManager->RegisterCallback(&g_advancedSettings, settingSet);

  settingSet.clear();
  settingSet.insert("karaoke.export");
  settingSet.insert("karaoke.importcsv");
  settingSet.insert("musiclibrary.cleanup");
  settingSet.insert("musiclibrary.export");
  settingSet.insert("musiclibrary.import");
  settingSet.insert("musicfiles.trackformat");
  settingSet.insert("musicfiles.trackformatright");
  settingSet.insert("videolibrary.flattentvshows");
  settingSet.insert("videolibrary.removeduplicates");
  settingSet.insert("videolibrary.groupmoviesets");
  settingSet.insert("videolibrary.cleanup");
  settingSet.insert("videolibrary.import");
  settingSet.insert("videolibrary.export");
  m_settingsManager->RegisterCallback(&CMediaSettings::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("videoscreen.resolution");
  settingSet.insert("videoscreen.flickerfilter");
  settingSet.insert("videoscreen.soften");
  settingSet.insert("videooutput.aspect");
  settingSet.insert("videooutput.hd480p");
  settingSet.insert("videooutput.hd720p");
  settingSet.insert("videooutput.hd1080i");
  m_settingsManager->RegisterCallback(&CDisplaySettings::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("videoplayer.seekdelay");
  settingSet.insert("videoplayer.seeksteps");
  settingSet.insert("musicplayer.seekdelay");
  settingSet.insert("musicplayer.seeksteps");
  m_settingsManager->RegisterCallback(&CSeekHandler::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("audiooutput.channels");
  settingSet.insert("audiooutput.guisoundmode");
  settingSet.insert("audiooutput.ac3passthrough");
  settingSet.insert("audiooutput.dtspassthrough");
  settingSet.insert("audiooutput.aacpassthrough");
  settingSet.insert("audiooutput.mp1passthrough");
  settingSet.insert("audiooutput.mp2passthrough");
  settingSet.insert("audiooutput.mp3passthrough");
  settingSet.insert("harddisk.aamlevel");
  settingSet.insert("harddisk.apmlevel");
  settingSet.insert("karaoke.port0voicemask");
  settingSet.insert("karaoke.port1voicemask");
  settingSet.insert("karaoke.port2voicemask");
  settingSet.insert("karaoke.port3voicemask");
  settingSet.insert("lcd.backlight");
  settingSet.insert("lcd.contrast");
  settingSet.insert("lcd.modchip");
  settingSet.insert("lcd.type");
  settingSet.insert("lookandfeel.skin");
  settingSet.insert("lookandfeel.skinsettings");
  settingSet.insert("lookandfeel.font");
  settingSet.insert("lookandfeel.skintheme");
  settingSet.insert("lookandfeel.skincolors");
  settingSet.insert("lookandfeel.skinzoom");
  settingSet.insert("musicplayer.replaygainpreamp");
  settingSet.insert("musicplayer.replaygainnogainpreamp");
  settingSet.insert("musicplayer.replaygaintype");
  settingSet.insert("musicplayer.replaygainavoidclipping");
  settingSet.insert("network.assignment");
  settingSet.insert("network.ipaddress");
  settingSet.insert("network.subnet");
  settingSet.insert("network.gateway");
  settingSet.insert("network.dns");
  settingSet.insert("network.dns2");
  settingSet.insert("scrapers.musicvideosdefault");
  settingSet.insert("screensaver.mode");
  settingSet.insert("screensaver.preview");
  settingSet.insert("screensaver.settings");
  settingSet.insert("system.ledcolour");
  settingSet.insert("videoscreen.guicalibration");
  settingSet.insert("source.videos");
  settingSet.insert("source.music");
  settingSet.insert("source.pictures");
  m_settingsManager->RegisterCallback(&g_application, settingSet);

  settingSet.clear();
  settingSet.insert("lookandfeel.soundskin");
  m_settingsManager->RegisterCallback(&g_audioManager, settingSet);

  settingSet.clear();
  settingSet.insert("subtitles.charset");
  settingSet.insert("karaoke.charset");
  settingSet.insert("locale.charset");
  m_settingsManager->RegisterCallback(&g_charsetConverter, settingSet);

#ifdef _XBOX
  settingSet.clear();
  settingSet.insert("system.autotemperature");
  settingSet.insert("system.fanspeedcontrol");
  settingSet.insert("system.fanspeed");
  settingSet.insert("system.minfanspeed");
  settingSet.insert("system.targettemperature");
  m_settingsManager->RegisterCallback(CFanController::Instance(), settingSet);
#endif

  settingSet.clear();
  settingSet.insert("locale.audiolanguage");
  settingSet.insert("locale.subtitlelanguage");
  settingSet.insert("locale.language");
  settingSet.insert("locale.country");
  settingSet.insert("locale.shortdateformat");
  settingSet.insert("locale.longdateformat");
  settingSet.insert("locale.timeformat");
  settingSet.insert("locale.use24hourclock");
  settingSet.insert("locale.temperatureunit");
  settingSet.insert("locale.speedunit");
  m_settingsManager->RegisterCallback(&g_langInfo, settingSet);

#if defined(HAS_SDL_JOYSTICK)
  settingSet.clear();
  settingSet.insert("input.enablejoystick");
  m_settingsManager->RegisterCallback(&g_Joystick, settingSet);
#endif

  settingSet.clear();
  settingSet.insert("input.enablemouse");
  m_settingsManager->RegisterCallback(&g_Mouse, settingSet);

  settingSet.clear();
  settingSet.insert("services.webserver");
  settingSet.insert("services.webserverport");
  settingSet.insert("services.webserverusername");
  settingSet.insert("services.webserverpassword");
  settingSet.insert("services.zeroconf");
  settingSet.insert("services.airplay");
  settingSet.insert("services.useairplaypassword");
  settingSet.insert("services.airplaypassword");
  settingSet.insert("services.upnpserver");
  settingSet.insert("services.upnprenderer");
  settingSet.insert("services.upnpcontroller");
  settingSet.insert("services.esenabled");
  settingSet.insert("services.esport");
  settingSet.insert("services.esallinterfaces");
  settingSet.insert("services.esinitialdelay");
  settingSet.insert("services.escontinuousdelay");
  settingSet.insert("services.ftpserver");
  settingSet.insert("services.ftpserveruser");
  settingSet.insert("services.ftpserverpassword");
  settingSet.insert("services.timeserver");
  settingSet.insert("services.timeserveraddress");
  settingSet.insert("smb.winsserver");
  settingSet.insert("smb.workgroup");
  m_settingsManager->RegisterCallback(&CNetworkServices::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("masterlock.lockcode");
  m_settingsManager->RegisterCallback(&g_passwordManager, settingSet);

  settingSet.clear();
  settingSet.insert("lookandfeel.rssedit");
  m_settingsManager->RegisterCallback(&CRssManager::Get(), settingSet);

  settingSet.clear();
  settingSet.insert("locale.timezone");
  settingSet.insert("locale.usedst");
  m_settingsManager->RegisterCallback(&g_timezone, settingSet);

  settingSet.clear();
  settingSet.insert("weather.addon");
  settingSet.insert("weather.addonsettings");
  m_settingsManager->RegisterCallback(&g_weatherManager, settingSet);

  settingSet.clear();
  settingSet.insert("general.addonupdates");
  m_settingsManager->RegisterCallback(&ADDON::CRepositoryUpdater::GetInstance(), settingSet);

  settingSet.clear();
  settingSet.insert("addons.showrunning");
  settingSet.insert("addons.managedependencies");
  settingSet.insert("addons.unknownsources");
  m_settingsManager->RegisterCallback(&ADDON::CAddonSystemSettings::GetInstance(), settingSet);
}

bool CSettings::Reset()
{
  std::string settingsFile = CProfilesManager::Get().GetSettingsFile();
  // try to delete the settings file
  if (XFILE::CFile::Exists(settingsFile, false) && !XFILE::CFile::Delete(settingsFile))
    CLog::Log(LOGWARNING, "Unable to delete old settings file at %s", settingsFile.c_str());

  // unload any loaded settings
  Unload();

  // try to save the default settings
  if (!Save())
  {
    CLog::Log(LOGWARNING, "Failed to save the default settings to %s", settingsFile.c_str());
    return false;
  }

  return true;
}

bool CSettings::LoadAvpackXML()
{
  // TODO: move this to separate setting class and load it at the end
  // CStdString avpackSettingsXML;
  // avpackSettingsXML  = GetAvpackSettingsFile();
  // CXBMCTinyXML avpackXML;
  // if (!CFile::Exists(avpackSettingsXML))
  // {
  //   CLog::Log(LOGERROR, "Error loading AV pack settings : %s not found !", avpackSettingsXML.c_str());
  //   return false;
  // }

  // CLog::Log(LOGNOTICE, "%s found : loading %s",
  //   g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  // if (!avpackXML.LoadFile(avpackSettingsXML.c_str()))
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s",
  //     avpackSettingsXML.c_str(), avpackXML.ErrorRow(), avpackXML.ErrorDesc());
  //   return false;
  // }

  // TiXmlElement *pMainElement = avpackXML.RootElement();
  // if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, no <settings> node", avpackSettingsXML.c_str());
  //   return false;
  // }

  // TiXmlElement *pRoot = pMainElement->FirstChildElement(g_videoConfig.GetAVPack());
  // if (!pRoot)
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, no <%s> node",
  //     avpackSettingsXML.c_str(), g_videoConfig.GetAVPack().c_str());
  //   return false;
  // }

  // // Load guisettings
  // g_guiSettings.LoadXML(pRoot);

  // // Load calibration
  // return LoadCalibration(pRoot, avpackSettingsXML);
  return false;
}

// Save the avpack settings in the current 'avpacksettings.xml' file
bool CSettings::SaveAvpackXML() const
{
  // CStdString avpackSettingsXML;
  // avpackSettingsXML  = GetAvpackSettingsFile();

  // CLog::Log(LOGNOTICE, "Saving %s settings in %s",
  //   g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  // // The file does not exist : Save defaults
  // if (!CFile::Exists(avpackSettingsXML))
  //   return SaveNewAvpackXML();

  // // The file already exists :
  // // We need to preserve other avpack settings

  // // First load the previous settings
  // CXBMCTinyXML xmlDoc;

  // if (!xmlDoc.LoadFile(avpackSettingsXML))
  // {
  //   CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, Line %d\n%s\nCreating new file.",
  //     avpackSettingsXML.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
  //   return SaveNewAvpackXML();
  // }

  // // Get the main element
  // TiXmlElement *pMainElement = xmlDoc.RootElement();
  // if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  // {
  //   CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, no <settings> node.\nCreating new file.",
  //     avpackSettingsXML.c_str());
  //   return SaveNewAvpackXML();
  // }

  // // Delete the plugged avpack root if it exists, then recreate it
  // // TODO : to support custom avpack settings, the two XMLs should
  // // be synchronized, not just overwrite the old one
  // TiXmlNode *pRoot = pMainElement->FirstChild(g_videoConfig.GetAVPack());
  // if (pRoot)
  //   pMainElement->RemoveChild(pRoot);

  // TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  // pRoot = pMainElement->InsertEndChild(pluggedNode);
  // if (!pRoot) return false;

  // if (!SaveAvpackSettings(pRoot))
  //   return false;

  // return xmlDoc.SaveFile(avpackSettingsXML);
  return false;
}

// Create an 'avpacksettings.xml' file with in the current profile directory
bool CSettings::SaveNewAvpackXML() const
{
  // CXBMCTinyXML xmlDoc;
  // TiXmlElement xmlMainElement("settings");
  // TiXmlNode *pMain = xmlDoc.InsertEndChild(xmlMainElement);
  // if (!pMain) return false;

  // TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  // TiXmlNode *pRoot = pMain->InsertEndChild(pluggedNode);
  // if (!pRoot) return false;

  // if (!SaveAvpackSettings(pRoot))
  //   return false;

  // return xmlDoc.SaveFile(GetAvpackSettingsFile());
  return false;
}

// Save avpack settings in the provided xml node
bool CSettings::SaveAvpackSettings(TiXmlNode *io_pRoot) const
{
  // TODO: move this to separate setting class and save it at the end
  // TiXmlElement programsNode("myprograms");
  // TiXmlNode *pNode = io_pRoot->InsertEndChild(programsNode);
  // if (!pNode) return false;
  // XMLUtils::SetBoolean(pNode, "gameautoregion", g_guiSettings.GetBool("myprograms.gameautoregion"));
  // XMLUtils::SetInt(pNode, "ntscmode", g_guiSettings.GetInt("myprograms.ntscmode"));

  // // default video settings
  // TiXmlElement videoSettingsNode("defaultvideosettings");
  // pNode = io_pRoot->InsertEndChild(videoSettingsNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "interlacemethod", CMediaSettings::Get().GetDefaultVideoSettings().m_InterlaceMethod);
  // XMLUtils::SetFloat(pNode, "filmgrain", CMediaSettings::Get().GetCurrentVideoSettings().m_FilmGrain);
  // XMLUtils::SetInt(pNode, "viewmode", CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  // XMLUtils::SetFloat(pNode, "zoomamount", CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount);
  // XMLUtils::SetFloat(pNode, "pixelratio", CMediaSettings::Get().GetCurrentVideoSettings().m_CustomPixelRatio);
  // XMLUtils::SetFloat(pNode, "volumeamplification", CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification);
  // XMLUtils::SetBoolean(pNode, "outputtoallspeakers", CMediaSettings::Get().GetCurrentVideoSettings().m_OutputToAllSpeakers);
  // XMLUtils::SetBoolean(pNode, "showsubtitles", CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn);
  // XMLUtils::SetFloat(pNode, "brightness", CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness);
  // XMLUtils::SetFloat(pNode, "contrast", CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast);
  // XMLUtils::SetFloat(pNode, "gamma", CMediaSettings::Get().GetCurrentVideoSettings().m_Gamma);

  // TiXmlElement audiooutputNode("audiooutput");
  // pNode = io_pRoot->InsertEndChild(audiooutputNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "mode", g_guiSettings.GetInt("audiooutput.mode"));
  // XMLUtils::SetBoolean(pNode, "ac3passthrough", g_guiSettings.GetBool("audiooutput.ac3passthrough"));
  // XMLUtils::SetBoolean(pNode, "dtspassthrough", g_guiSettings.GetBool("audiooutput.dtspassthrough"));

  // TiXmlElement videooutputNode("videooutput");
  // pNode = io_pRoot->InsertEndChild(videooutputNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "aspect", g_guiSettings.GetInt("videooutput.aspect"));
  // XMLUtils::SetBoolean(pNode, "hd480p", g_guiSettings.GetBool("videooutput.hd480p"));
  // XMLUtils::SetBoolean(pNode, "hd720p", g_guiSettings.GetBool("videooutput.hd720p"));
  // XMLUtils::SetBoolean(pNode, "hd1080i", g_guiSettings.GetBool("videooutput.hd1080i"));

  // TiXmlElement videoscreenNode("videoscreen");
  // pNode = io_pRoot->InsertEndChild(videoscreenNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "flickerfilter", g_guiSettings.GetInt("videoscreen.flickerfilter"));
  // XMLUtils::SetInt(pNode, "resolution", g_guiSettings.GetInt("videoscreen.resolution"));
  // XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoscreen.soften"));

  // TiXmlElement videoplayerNode("videoplayer");
  // pNode = io_pRoot->InsertEndChild(videoplayerNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "displayresolution", g_guiSettings.GetInt("videoplayer.displayresolution"));
  // XMLUtils::SetInt(pNode, "flicker", g_guiSettings.GetInt("videoplayer.flicker"));
  // XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoplayer.soften"));

  // return SaveCalibration(io_pRoot);
  return false;
}

std::string CSettings::GetFFmpegDllFolder() const
{
  std::string folder = "Q:\\system\\players\\dvdplayer\\";
  if (CSettings::GetInstance().GetBool("videoplayer.allcodecs"))
    folder += "full\\";
  return folder;
}

std::string CSettings::GetPlayerName(const int& player) const
{
  if (player == PLAYER_PAPLAYER)
    return "paplayer";
  if (player == PLAYER_MPLAYER)
    return "mplayer";
  if (player == PLAYER_DVDPLAYER)
    return "dvdplayer";

  return "";
}

std::string CSettings::GetDefaultVideoPlayerName() const
{
  return GetPlayerName(CSettings::GetInstance().GetInt("videoplayer.defaultplayer"));
}

std::string CSettings::GetDefaultAudioPlayerName() const
{
  return GetPlayerName(CSettings::GetInstance().GetInt("musicplayer.defaultplayer"));
}

std::string CSettings::GetAvpackSettingsFile() const
{
  if (CProfilesManager::Get().GetCurrentProfileIndex() == 0)
    return "T:\\avpacksettings.xml";
  else
    return "P:\\avpacksettings.xml";

  return "";
}
