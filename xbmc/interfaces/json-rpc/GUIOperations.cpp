/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIOperations.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "application/Application.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace ADDON;

JSONRPC_STATUS CGUIOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CGUIOperations::ActivateWindow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int iWindow = CButtonTranslator::TranslateWindow(parameterObject["window"].asString());
  if (iWindow != WINDOW_INVALID)
  {
    std::vector<std::string> params;
    for (CVariant::const_iterator_array param = parameterObject["parameters"].begin_array();
         param != parameterObject["parameters"].end_array(); ++param)
    {
      if (param->isString() && !param->empty())
        params.push_back(param->asString());
    }
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTIVATE_WINDOW, iWindow, 0, NULL, "",
                                               params);
    return ACK;
  }

  return InvalidParams;
}

JSONRPC_STATUS CGUIOperations::ShowNotification(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string image = parameterObject["image"].asString();
  std::string title = parameterObject["title"].asString();
  std::string message = parameterObject["message"].asString();
  unsigned int displaytime = (unsigned int)parameterObject["displaytime"].asUnsignedInteger();

  if (image.compare("info") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, title, message, displaytime);
  else if (image.compare("warning") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, title, message, displaytime);
  else if (image.compare("error") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, title, message, displaytime);
  else
    CGUIDialogKaiToast::QueueNotification(image, title, message, displaytime);

  return ACK;
}

JSONRPC_STATUS CGUIOperations::SetFullscreen(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ((parameterObject["fullscreen"].isString() &&
       parameterObject["fullscreen"].asString().compare("toggle") == 0) ||
      (parameterObject["fullscreen"].isBoolean() &&
       parameterObject["fullscreen"].asBoolean() != g_application.IsFullScreen()))
  {
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_SHOW_GUI)));
  }
  else if (!parameterObject["fullscreen"].isBoolean() && !parameterObject["fullscreen"].isString())
    return InvalidParams;

  return GetPropertyValue("fullscreen", result);
}

JSONRPC_STATUS CGUIOperations::ActivateScreenSaver(const std::string& method,
                                                   ITransportLayer* transport,
                                                   IClient* client,
                                                   const CVariant& parameterObject,
                                                   CVariant& result)
{
  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_ACTIVATESCREENSAVER);
  return ACK;
}

JSONRPC_STATUS CGUIOperations::GetPropertyValue(const std::string &property, CVariant &result)
{
  if (property == "currentwindow")
  {
    result["label"] = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
        CServiceBroker::GetGUI()->GetInfoManager().TranslateString("System.CurrentWindow"),
        INFO::DEFAULT_CONTEXT);
    result["id"] = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();
  }
  else if (property == "currentcontrol")
    result["label"] = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(
        CServiceBroker::GetGUI()->GetInfoManager().TranslateString("System.CurrentControl"),
        INFO::DEFAULT_CONTEXT);
  else if (property == "skin")
  {
    std::string skinId = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
    AddonPtr addon;
    if (!CServiceBroker::GetAddonMgr().GetAddon(skinId, addon, AddonType::SKIN,
                                                OnlyEnabled::CHOICE_YES))
      return InternalError;

    result["id"] = skinId;
    if (addon.get())
      result["name"] = addon->Name();
  }
  else if (property == "fullscreen")
    result = g_application.IsFullScreen();
  else
    return InvalidParams;

  return OK;
}
