/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonBase.h"

#include "GUIUserMessages.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/kodi-dev-kit/include/kodi/versions.h"
#include "addons/settings/AddonSettings.h"
#include "Application.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

// "C" interface addon callback handle classes
#if 0
#include "AudioEngine.h"
#include "Filesystem.h"
#include "General.h"
#include "Network.h"
#include "gui/General.h"
#endif

namespace ADDON
{

std::vector<ADDON_GET_INTERFACE_FN> Interface_Base::s_registeredInterfaces;

bool Interface_Base::InitInterface(CAddonDll* addon,
                                   AddonGlobalInterface& addonInterface,
                                   KODI_ADDON_INSTANCE_STRUCT* firstKodiInstance)
{
  addonInterface = AddonGlobalInterface();

  addonInterface.addonBase = NULL;
  addonInterface.globalSingleInstance = NULL;
  addonInterface.firstKodiInstance = firstKodiInstance;

  // Create function list from kodi to addon, generated with malloc to have
  // compatible with other versions
  addonInterface.toKodi = new AddonToKodiFuncTable_Addon();
  addonInterface.toKodi->kodiBase = addon;
  addonInterface.toKodi->addon_log_msg = addon_log_msg;
  addonInterface.toKodi->free_string = free_string;
  addonInterface.toKodi->free_string_array = free_string_array;

  addonInterface.toKodi->kodi_addon = new AddonToKodiFuncTable_kodi_addon();
  addonInterface.toKodi->kodi_addon->get_addon_path = get_addon_path;
  addonInterface.toKodi->kodi_addon->get_lib_path = get_lib_path;
  addonInterface.toKodi->kodi_addon->get_user_path = get_user_path;
  addonInterface.toKodi->kodi_addon->get_temp_path = get_temp_path;
  addonInterface.toKodi->kodi_addon->get_localized_string = get_localized_string;
  addonInterface.toKodi->kodi_addon->open_settings_dialog = open_settings_dialog;
  addonInterface.toKodi->kodi_addon->is_setting_using_default = is_setting_using_default;
  addonInterface.toKodi->kodi_addon->get_setting_bool = get_setting_bool;
  addonInterface.toKodi->kodi_addon->get_setting_int = get_setting_int;
  addonInterface.toKodi->kodi_addon->get_setting_float = get_setting_float;
  addonInterface.toKodi->kodi_addon->get_setting_string = get_setting_string;
  addonInterface.toKodi->kodi_addon->set_setting_bool = set_setting_bool;
  addonInterface.toKodi->kodi_addon->set_setting_int = set_setting_int;
  addonInterface.toKodi->kodi_addon->set_setting_float = set_setting_float;
  addonInterface.toKodi->kodi_addon->set_setting_string = set_setting_string;
  addonInterface.toKodi->kodi_addon->get_addon_info = get_addon_info;
  addonInterface.toKodi->kodi_addon->get_type_version = get_type_version;
  addonInterface.toKodi->kodi_addon->get_interface = get_interface;

  // Related parts becomes set from addon headers, make here to NULL to allow
  // checks for right set of them
  addonInterface.toAddon = new KodiToAddonFuncTable_Addon();

  // Init the other interfaces
#if 0
  Interface_General::Init(&addonInterface);
  Interface_AudioEngine::Init(&addonInterface);
  Interface_Filesystem::Init(&addonInterface);
  Interface_Network::Init(&addonInterface);
  Interface_GUIGeneral::Init(&addonInterface);
#endif

  return true;
}

void Interface_Base::DeInitInterface(AddonGlobalInterface& addonInterface)
{
#if 0
  Interface_GUIGeneral::DeInit(&addonInterface);
  Interface_Network::DeInit(&addonInterface);
  Interface_Filesystem::DeInit(&addonInterface);
  Interface_AudioEngine::DeInit(&addonInterface);
  Interface_General::DeInit(&addonInterface);
#endif

  if (addonInterface.toKodi)
    delete addonInterface.toKodi->kodi_addon;
  delete addonInterface.toKodi;
  delete addonInterface.toAddon;
  addonInterface = AddonGlobalInterface();
}

void Interface_Base::RegisterInterface(ADDON_GET_INTERFACE_FN fn)
{
  s_registeredInterfaces.push_back(fn);
}

bool Interface_Base::UpdateSettingInActiveDialog(CAddonDll* addon,
                                                 AddonInstanceId instanceId,
                                                 const char* id,
                                                 const std::string& value)
{
  if (!CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
    return false;

  CGUIDialogAddonSettings* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(
          WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog->GetCurrentAddonID() != addon->ID())
    return false;

  CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
  std::vector<std::string> params;
  params.push_back(id);
  params.push_back(value);
  message.SetStringParams(params);
  message.SetParam1(instanceId);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message,
                                                                 WINDOW_DIALOG_ADDON_SETTINGS);

  return true;
}

/*!
 * @brief Addon to Kodi basic callbacks below
 *
 * The amount of functions here are hold so minimal as possible. Only parts
 * where needed on nearly every add-on (e.g. addon_log_msg) are to add there.
 *
 * More specific parts like e.g. to open files should be added to a separate
 * part.
 */
//@{

void Interface_Base::addon_log_msg(const KODI_ADDON_BACKEND_HDL hdl,
                                   const int addonLogLevel,
                                   const char* strMessage)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "addon_log_msg(...) called with empty kodi instance pointer");
    return;
  }

  int logLevel = LOGNONE;
  switch (addonLogLevel)
  {
    case ADDON_LOG_DEBUG:
      logLevel = LOGDEBUG;
      break;
    case ADDON_LOG_INFO:
      logLevel = LOGINFO;
      break;
    case ADDON_LOG_WARNING:
      logLevel = LOGWARNING;
      break;
    case ADDON_LOG_ERROR:
      logLevel = LOGERROR;
      break;
    case ADDON_LOG_FATAL:
      logLevel = LOGFATAL;
      break;
    default:
      logLevel = LOGDEBUG;
      break;
  }

  CLog::Log(logLevel, "AddOnLog: %s: %s", addon->ID().c_str(), strMessage);
}

char* Interface_Base::get_type_version(const KODI_ADDON_BACKEND_HDL hdl, int type)
{
  return strdup(kodi::addon::GetTypeVersion(type));
}

char* Interface_Base::get_addon_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "get_addon_path(...) called with empty kodi instance pointer");
    return NULL;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Path()).c_str());
}

char* Interface_Base::get_lib_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "get_lib_path(...) called with empty kodi instance pointer");
    return NULL;
  }

  return strdup(CSpecialProtocol::TranslatePath("special://xbmcbinaddons/" + addon->ID()).c_str());
}

char* Interface_Base::get_user_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "get_user_path(...) called with empty kodi instance pointer");
    return NULL;
  }

  return strdup(CSpecialProtocol::TranslatePath(addon->Profile()).c_str());
}

char* Interface_Base::get_temp_path(const KODI_ADDON_BACKEND_HDL hdl)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "get_temp_path(...) called with empty kodi instance pointer");
    return NULL;
  }

  std::string tempPath =
      URIUtils::AddFileToFolder(CServiceBroker::GetAddonMgr().GetTempAddonBasePath(), addon->ID());
  tempPath += "-temp";
  XFILE::CDirectory::Create(tempPath);

  return strdup(CSpecialProtocol::TranslatePath(tempPath).c_str());
}

char* Interface_Base::get_localized_string(const KODI_ADDON_BACKEND_HDL hdl, long label_id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (!addon)
  {
    CLog::Log(LOGERROR, "get_localized_string(...) called with empty kodi instance pointer");
    return NULL;
  }

  if (g_application.m_bStop)
    return NULL;

  std::string label = g_localizeStrings.GetAddonString(addon->ID(), label_id);
  if (label.empty())
    label = g_localizeStrings.Get(label_id);
  char* buffer = strdup(label.c_str());
  return buffer;
}

char* Interface_Base::get_addon_info(const KODI_ADDON_BACKEND_HDL hdl, const char* id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL)
  {
    CLog::Log(LOGERROR, "get_addon_info(...) called with empty pointer");
    return NULL;
  }

  std::string str;
  if (StringUtils::CompareNoCase(id, "author") == 0)
    str = addon->Author();
  else if (StringUtils::CompareNoCase(id, "changelog") == 0)
    str = addon->ChangeLog();
  else if (StringUtils::CompareNoCase(id, "description") == 0)
    str = addon->Description();
  else if (StringUtils::CompareNoCase(id, "disclaimer") == 0)
    str = addon->Disclaimer();
  else if (StringUtils::CompareNoCase(id, "fanart") == 0)
    str = addon->FanArt();
  else if (StringUtils::CompareNoCase(id, "icon") == 0)
    str = addon->Icon();
  else if (StringUtils::CompareNoCase(id, "id") == 0)
    str = addon->ID();
  else if (StringUtils::CompareNoCase(id, "name") == 0)
    str = addon->Name();
  else if (StringUtils::CompareNoCase(id, "path") == 0)
    str = addon->Path();
  else if (StringUtils::CompareNoCase(id, "profile") == 0)
    str = addon->Profile();
  else if (StringUtils::CompareNoCase(id, "summary") == 0)
    str = addon->Summary();
  else if (StringUtils::CompareNoCase(id, "type") == 0)
    str = ADDON::CAddonInfo::TranslateType(addon->Type());
  else if (StringUtils::CompareNoCase(id, "version") == 0)
    str = addon->Version().asString();
  else
  {
    CLog::Log(LOGERROR, "Interface_Base::%s -  add-on '%s' requests invalid id '%s'", __FUNCTION__,
              addon->Name().c_str(), id);
    return NULL;
  }

  char* buffer = strdup(str.c_str());
  return buffer;
}

bool Interface_Base::open_settings_dialog(const KODI_ADDON_BACKEND_HDL hdl)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "open_settings_dialog(...) called with empty kodi instance pointer");
    return false;
  }

  // show settings dialog
  AddonPtr addonInfo;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addon->ID(), addonInfo, OnlyEnabled::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - Could not get addon information for '%s'", __FUNCTION__,
              addon->ID().c_str());
    return false;
  }

  return CGUIDialogAddonSettings::ShowForAddon(addonInfo);
}

bool Interface_Base::is_setting_using_default(const KODI_ADDON_BACKEND_HDL hdl, const char* id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p')", __FUNCTION__, hdl,
              static_cast<const void*>(id));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - couldn't get settings for add-on '%s'", __FUNCTION__,
              addon->Name().c_str());
    return false;
  }

  SettingPtr setting = addon->GetSettings()->GetSetting(id);
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - can't find setting '%s' in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  return setting->IsDefault();
}

bool Interface_Base::get_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL || value == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p', value='%p')",
              __FUNCTION__, hdl, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - couldn't get settings for add-on '%s'", __FUNCTION__,
              addon->Name().c_str());
    return false;
  }

  SettingPtr setting = addon->GetSettings()->GetSetting(id);
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - can't find setting '%s' in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Boolean)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - setting '%s' is not a boolean in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  *value = boost::static_pointer_cast<CSettingBool>(setting)->GetValue();
  return true;
}

bool Interface_Base::get_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL || value == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p', value='%p')",
              __FUNCTION__, hdl, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - couldn't get settings for add-on '%s'", __FUNCTION__,
              addon->Name().c_str());
    return false;
  }

  SettingPtr setting = addon->GetSettings()->GetSetting(id);
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - can't find setting '%s' in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Integer && setting->GetType() != SettingType::Number)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - setting '%s' is not a integer in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  if (setting->GetType() == SettingType::Integer)
    *value = boost::static_pointer_cast<CSettingInt>(setting)->GetValue();
  else
    *value = static_cast<int>(boost::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_float(const KODI_ADDON_BACKEND_HDL hdl,
                                       const char* id,
                                       float* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL || value == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p', value='%p')",
              __FUNCTION__, hdl, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - couldn't get settings for add-on '%s'", __FUNCTION__,
              addon->Name().c_str());
    return false;
  }

  SettingPtr setting = addon->GetSettings()->GetSetting(id);
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - can't find setting '%s' in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::Number)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - setting '%s' is not a number in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  *value = static_cast<float>(boost::static_pointer_cast<CSettingNumber>(setting)->GetValue());
  return true;
}

bool Interface_Base::get_setting_string(const KODI_ADDON_BACKEND_HDL hdl,
                                        const char* id,
                                        char** value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL || value == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p', value='%p')",
              __FUNCTION__, hdl, static_cast<const void*>(id), static_cast<void*>(value));

    return false;
  }

  if (!addon->HasSettings())
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - couldn't get settings for add-on '%s'", __FUNCTION__,
              addon->Name().c_str());
    return false;
  }

  SettingPtr setting = addon->GetSettings()->GetSetting(id);
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - can't find setting '%s' in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  if (setting->GetType() != SettingType::String)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - setting '%s' is not a string in '%s'", __FUNCTION__, id,
              addon->Name().c_str());
    return false;
  }

  *value = strdup(boost::static_pointer_cast<CSettingString>(setting)->GetValue().c_str());
  return true;
}

bool Interface_Base::set_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p')", __FUNCTION__, hdl,
              static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  value ? "true" : "false"))
    return true;

  if (!addon->UpdateSettingBool(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p')", __FUNCTION__, hdl,
              static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  std::to_string(value)))
    return true;

  if (!addon->UpdateSettingInt(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_float(const KODI_ADDON_BACKEND_HDL hdl,
                                       const char* id,
                                       float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p')", __FUNCTION__, hdl,
              static_cast<const void*>(id));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id,
                                                  StringUtils::Format("%f", value)))
    return true;

  if (!addon->UpdateSettingNumber(id, static_cast<double>(value)))
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

bool Interface_Base::set_setting_string(const KODI_ADDON_BACKEND_HDL hdl,
                                        const char* id,
                                        const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(hdl);
  if (addon == NULL || id == NULL || value == NULL)
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid data (addon='%p', id='%p', value='%p')",
              __FUNCTION__, hdl, static_cast<const void*>(id), static_cast<const void*>(value));

    return false;
  }

  if (Interface_Base::UpdateSettingInActiveDialog(addon, ADDON_SETTINGS_ID, id, value))
    return true;

  if (!addon->UpdateSettingString(id, value))
  {
    CLog::Log(LOGERROR, "Interface_Base::%s - invalid setting type", __FUNCTION__);
    return false;
  }

  addon->SaveSettings();

  return true;
}

void Interface_Base::free_string(const KODI_ADDON_BACKEND_HDL hdl, char* str)
{
  if (str)
    free(str);
}

void Interface_Base::free_string_array(const KODI_ADDON_BACKEND_HDL hdl,
                                       char** arr,
                                       int numElements)
{
  if (arr)
  {
    for (int i = 0; i < numElements; ++i)
    {
      free(arr[i]);
    }
    free(arr);
  }
}

void* Interface_Base::get_interface(const KODI_ADDON_BACKEND_HDL hdl,
                                    const char* name,
                                    const char* version)
{
  if (!name || !version)
    return NULL;

  void* retval(NULL);

  for (std::vector<ADDON_GET_INTERFACE_FN>::iterator fn = s_registeredInterfaces.begin(); fn != s_registeredInterfaces.end(); ++fn)
    if ((retval = (*fn)(name, version)))
      break;

  return retval;
}

//@}

} // namespace ADDON
