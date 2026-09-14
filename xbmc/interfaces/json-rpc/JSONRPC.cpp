/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "JSONRPC.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/JSONVariantParser.h"
#include "threads/SingleLock.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "guilib/GUIAudioManager.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIComponent.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "input/keyboard/Key.h"
#include "input/keyboard/KeyIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "video/VideoInfoTag.h"
#include "utils/JSONVariantParser.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "utils/JSONVariantWriter.h"

using namespace JSONRPC;
using namespace PLAYLIST;

static CCriticalSection s_inputCritSection;
static uint32_t s_pendingKey = KEY_INVALID;

uint32_t CJSONRPC::GetInputKey()
{
    CSingleLock lock(s_inputCritSection);
    uint32_t currentKey = s_pendingKey;
    s_pendingKey = KEY_INVALID;
    return currentKey;
}

//
// ===== JSON-RPC Status Codes (matches Eden) =====
//
enum JSON_STATUS
{
    OK = 0,
    ACK = 1,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ParseError = -32700,
    BadPermission = -32099,
    FailedToExecute = -32100
};

//
// ===== Response Builders (matches Eden CJSONRPC::BuildResponse) =====
//
static void BuildResponse(const CVariant &request, JSON_STATUS code, const CVariant &result, CVariant &response)
{
    response["jsonrpc"] = "2.0";
    response["id"] = request.isMember("id") ? request["id"] : CVariant();

    switch (code)
    {
        case OK:
            response["result"] = result;
            break;
        case ACK:
            response["result"] = CVariant("OK");
            break;
        case InvalidRequest:
            response["error"]["code"] = InvalidRequest;
            response["error"]["message"] = "Invalid request.";
            break;
        case InvalidParams:
            response["error"]["code"] = InvalidParams;
            response["error"]["message"] = "Invalid params.";
            break;
        case MethodNotFound:
            response["error"]["code"] = MethodNotFound;
            response["error"]["message"] = "Method not found.";
            break;
        case ParseError:
            response["error"]["code"] = ParseError;
            response["error"]["message"] = "Parse error.";
            break;
        case BadPermission:
            response["error"]["code"] = BadPermission;
            response["error"]["message"] = "Bad client permission.";
            break;
        case FailedToExecute:
            response["error"]["code"] = FailedToExecute;
            response["error"]["message"] = "Failed to execute method.";
            break;
        default:
            response["error"]["code"] = InternalError;
            response["error"]["message"] = "Internal error.";
            break;
    }
}

//
// ===== Application Operations (matches Eden CApplicationOperations) =====
//
static JSON_STATUS ApplicationGetProperties(const CVariant &params, CVariant &result)
{
    result = CVariant(CVariant::VariantTypeObject);

    for (unsigned int i = 0; i < params["properties"].size(); i++)
    {
        std::string prop = params["properties"][i].asString();

    if (prop == "volume" || prop == "muted")
    {
      const CApplicationComponents &components = CServiceBroker::GetAppComponents();
      const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
      if (prop == "volume")
        result = static_cast<int>(appVolume->GetVolumePercent());
      else if (prop == "muted")
        result = appVolume->IsMuted();
    }
        else if (prop == "name")
            result["name"] = "Xodi";
        else if (prop == "version")
        {
            CVariant ver(CVariant::VariantTypeObject);
            ver["major"] = 22;
            ver["minor"] = 1;
            ver["revision"] = "dev";
            ver["tag"] = "beta";
            result["version"] = ver;
        }
    }

    return OK;
}

static JSON_STATUS ApplicationSetVolume(const CVariant &params, CVariant &result)
{
  bool up = false;
  if (params["volume"].isInteger())
  {
    CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
    int oldVolume = static_cast<int>(appVolume->GetVolumePercent());
    float volume = static_cast<float>(params["volume"].asFloat());

    appVolume->SetVolume(volume, true);

    up = oldVolume < volume;
  }

  return ApplicationGetProperties("volume", result);
}

static JSON_STATUS ApplicationSetMute(const CVariant &params, CVariant &result)
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  if ((params["mute"].isString() &&
       params["mute"].asString().compare("toggle") == 0) ||
      (params["mute"].isBoolean() &&
       params["mute"].asBoolean() != appVolume->IsMuted()))
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_MUTE)));
  else if (!params["mute"].isBoolean() && !params["mute"].isString())
    return InvalidParams;

    return ApplicationGetProperties("muted", result);
}

//
// ===== System Operations (matches Eden CSystemOperations) =====
//
static JSON_STATUS SystemShutdown(const CVariant &params, CVariant &result)
{
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_POWERDOWN);
    return ACK;
}

static JSON_STATUS SystemReboot(const CVariant &params, CVariant &result)
{
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTART);
    return ACK;
}

//
// ===== Input Operations (matches Eden CInputOperations) =====
//
static JSON_STATUS InputSendKey(uint32_t keyCode)
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetSystemIdleTimer();
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetAudioManager().PlayActionSound(keyCode);

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                              static_cast<void*>(new CAction(keyCode)));
  return ACK;
}

static JSON_STATUS InputLeft(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_MOVE_LEFT);
}

static JSON_STATUS InputRight(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_MOVE_RIGHT);
}

static JSON_STATUS InputUp(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_MOVE_UP);
}

static JSON_STATUS InputDown(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_MOVE_DOWN);
}

static JSON_STATUS InputSelect(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_SELECT_ITEM);
}

static JSON_STATUS InputBack(const CVariant &params, CVariant &result)
{
    return InputSendKey(ACTION_NAV_BACK);
}

static JSON_STATUS InputHome(const CVariant &params, CVariant &result)
{
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTIVATE_WINDOW, WINDOW_HOME, 0);
    return ACK;
}

//
// ===== JSONRPC Intrinsics =====
//
static JSON_STATUS JSONRPCPing(const CVariant &params, CVariant &result)
{
    CVariant pong("pong");
    result.swap(pong);
    return OK;
}

//
// ===== Method Dispatch (matches Eden CJSONRPC::HandleMethodCall) =====
//
typedef JSON_STATUS (*MethodHandler)(const CVariant &params, CVariant &result);

struct MethodEntry
{
    const char *name;
    MethodHandler handler;
};

static const MethodEntry s_methods[] =
{
    // JSONRPC
    { "jsonrpc.ping",               JSONRPCPing },

    // Application
    { "application.getproperties",  ApplicationGetProperties },
    { "application.setvolume",      ApplicationSetVolume },
    { "application.setmute",        ApplicationSetMute },

    // System
    { "system.shutdown",            SystemShutdown },
    { "system.reboot",              SystemReboot },

    // Input
    { "input.up",                   InputUp },
    { "input.down",                 InputDown },
    { "input.left",                 InputLeft },
    { "input.right",                InputRight },
    { "input.select",               InputSelect },
    { "input.back",                 InputBack },
    { "input.home",                 InputHome },

    { NULL, NULL }
};

static MethodHandler FindMethod(const std::string &method)
{
    std::string lower = method;
    for (size_t i = 0; i < lower.size(); i++)
        lower[i] = (char)tolower((unsigned char)lower[i]);

    for (const MethodEntry *entry = s_methods; entry->name != NULL; entry++)
    {
        if (lower == entry->name)
            return entry->handler;
    }
    return NULL;
}

//
// ===== Public Interface =====
//
void CJSONRPC::Initialize()
{
    CLog::Log(LOGINFO, "JSONRPC: Initialized");
}

std::string CJSONRPC::MethodCall(const std::string &inputString, ITransportLayer *transport, IClient *client)
{
    CVariant inputroot, outputroot, result;

    CLog::Log(LOGDEBUG, "JSONRPC: Incoming request: %s", inputString.substr(0, 200).c_str());

    inputroot = CJSONVariantParser::Parse(inputString);

    if (inputroot.isNull())
    {
        CLog::Log(LOGERROR, "JSONRPC: Failed to parse request");
        BuildResponse(inputroot, ParseError, CVariant(), outputroot);
        return CJSONVariantWriter::Write(outputroot, true).c_str();
    }

    // Validate JSON-RPC 2.0 structure
    if (!inputroot.isObject() || !inputroot.isMember("method") || !inputroot["method"].isString())
    {
        BuildResponse(inputroot, InvalidRequest, CVariant(), outputroot);
        return CJSONVariantWriter::Write(outputroot, true).c_str();
    }

    std::string methodName = inputroot["method"].asString();
    CVariant params = inputroot.isMember("params") ? inputroot["params"] : CVariant(CVariant::VariantTypeObject);

    CLog::Log(LOGDEBUG, "JSONRPC: Calling %s", methodName.c_str());

    MethodHandler handler = FindMethod(methodName);
    if (handler)
    {
        JSON_STATUS status = handler(params, result);
        BuildResponse(inputroot, status, result, outputroot);
    }
    else
    {
        CLog::Log(LOGWARNING, "JSONRPC: Method not found: %s", methodName.c_str());
        BuildResponse(inputroot, MethodNotFound, CVariant(), outputroot);
    }

    return CJSONVariantWriter::Write(outputroot, true).c_str();
}
