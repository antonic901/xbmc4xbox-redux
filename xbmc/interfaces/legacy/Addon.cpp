/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Addon.h"

#include "GUIUserMessages.h"
#include "LanguageHook.h"
#include "Exception.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonSettings.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace ADDON;

namespace XBMCAddon
{
  namespace xbmcaddon
  {
    String Addon::getDefaultId() { return languageHook == NULL ? emptyString : languageHook->GetAddonId(); }

    String Addon::getAddonVersion() { return languageHook == NULL ? emptyString : languageHook->GetAddonVersion(); }

    bool Addon::UpdateSettingInActiveDialog(const char* id, const String& value)
    {
      ADDON::AddonPtr addon(pAddon);
      if (!g_windowManager.IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
        return false;

      CGUIDialogAddonSettings* dialog = dynamic_cast<CGUIDialogAddonSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS));
      if (dialog->GetCurrentID() != addon->ID())
        return false;

      CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
      std::vector<std::string> params;
      params.push_back(id);
      params.push_back(value);
      message.SetStringParams(params);
      g_windowManager.SendThreadMessage(message, WINDOW_DIALOG_ADDON_SETTINGS);

      return true;
    }

    Addon::Addon(const char* cid)
    {
      String id(cid ? cid : emptyString);

      // if the id wasn't passed then get the id from
      //   the global dictionary
      if (id.empty())
        id = getDefaultId();

      // if we still don't have an id then bail
      if (id.empty())
        throw AddonException("No valid addon id could be obtained. None was passed and the script wasn't executed in a normal xbmc manner.");

      if (!CServiceBroker::GetAddonMgr().GetAddon(id.c_str(), pAddon))
        throw AddonException("Unknown addon id '%s'.", id.c_str());

      CServiceBroker::GetAddonMgr().AddToUpdateableAddons(pAddon);
    }

    Addon::~Addon()
    {
      CServiceBroker::GetAddonMgr().RemoveFromUpdateableAddons(pAddon);
    }

    String Addon::getLocalizedString(int id)
    {
      return g_localizeStrings.GetAddonString(pAddon->ID(), id);
    }

    String Addon::getSetting(const char* id)
    {
      return pAddon->GetSetting(id);
    }

    bool Addon::getSettingBool(const char* id)
    {
      THROW_UNIMP("getSettingBool");
      return false;
    }

    int Addon::getSettingInt(const char* id)
    {
      THROW_UNIMP("getSettingInt");
      return 0;
    }

    double Addon::getSettingNumber(const char* id)
    {
      THROW_UNIMP("getSettingNumber");
      return 0;
    }

    String Addon::getSettingString(const char* id)
    {
      THROW_UNIMP("getSettingString");
      return "";
    }

    void Addon::setSetting(const char* id, const String& value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (!UpdateSettingInActiveDialog(id, value))
      {
        addon->UpdateSetting(id, value);
        addon->SaveSettings();
      }
    }

    bool Addon::setSettingBool(const char* id, bool value)
    {
      THROW_UNIMP("setSettingBool");
      return false;
    }

    bool Addon::setSettingInt(const char* id, int value)
    {
      THROW_UNIMP("setSettingInt");
      return false;
    }

    bool Addon::setSettingNumber(const char* id, double value)
    {
      THROW_UNIMP("setSettingNumber");
      return false;
    }

    bool Addon::setSettingString(const char* id, const String& value)
    {
      THROW_UNIMP("setSettingString");
      return false;
    }

    void Addon::openSettings()
    {
      DelayedCallGuard dcguard(languageHook);
      // show settings dialog
      ADDON::AddonPtr addon(pAddon);
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
    }

    String Addon::getAddonInfo(const char* id)
    {
      if (strcmpi(id, "author") == 0)
        return pAddon->Author();
      else if (strcmpi(id, "changelog") == 0)
        return pAddon->ChangeLog();
      else if (strcmpi(id, "description") == 0)
        return pAddon->Description();
      else if (strcmpi(id, "disclaimer") == 0)
        return pAddon->Disclaimer();
      else if (strcmpi(id, "fanart") == 0)
        return pAddon->FanArt();
      else if (strcmpi(id, "icon") == 0)
        return pAddon->Icon();
      else if (strcmpi(id, "id") == 0)
        return pAddon->ID();
      else if (strcmpi(id, "name") == 0)
        return pAddon->Name();
      else if (strcmpi(id, "path") == 0)
        return pAddon->Path();
      else if (strcmpi(id, "profile") == 0)
        return pAddon->Profile();
      else if (strcmpi(id, "stars") == 0)
        return StringUtils::Format("-1");
      else if (strcmpi(id, "summary") == 0)
        return pAddon->Summary();
      else if (strcmpi(id, "type") == 0)
        return ADDON::TranslateType(pAddon->Type());
      else if (strcmpi(id, "version") == 0)
        return pAddon->Version().asString();
      else
        throw AddonException("'%s' is an invalid Id", id);
    }
  }
}
