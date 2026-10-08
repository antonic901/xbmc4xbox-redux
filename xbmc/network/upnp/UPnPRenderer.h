/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"

#include <Platinum/Source/Devices/MediaRenderer/PltMediaRenderer.h>

class CVariant;

namespace UPNP
{

class CRendererReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
};

class CUPnPRenderer : public PLT_MediaRenderer
                    , public ANNOUNCEMENT::IAnnouncer
{
public:
    CUPnPRenderer(const char*  friendly_name,
                  bool         show_ip = false,
                  const char*  uuid = NULL,
                  unsigned int port = 0);

    virtual ~CUPnPRenderer();

    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data);
    void UpdateState();

    // Http server handler
    virtual NPT_Result ProcessHttpGetRequest(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);

    // AVTransport methods
    virtual NPT_Result OnNext(PLT_ActionReference& action);
    virtual NPT_Result OnPause(PLT_ActionReference& action);
    virtual NPT_Result OnPlay(PLT_ActionReference& action);
    virtual NPT_Result OnPrevious(PLT_ActionReference& action);
    virtual NPT_Result OnStop(PLT_ActionReference& action);
    virtual NPT_Result OnSeek(PLT_ActionReference& action);
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action);
    virtual NPT_Result OnSetNextAVTransportURI(PLT_ActionReference& action);

    // RenderingControl methods
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetMute(PLT_ActionReference& action);

private:
    virtual NPT_Result SetupServices();
    virtual NPT_Result SetupIcons();
    NPT_Result GetMetadata(NPT_String& meta);
    NPT_Result PlayMedia(const NPT_String& uri,
                         const NPT_String& meta,
                         PLT_Action* action = NULL);
    NPT_Result Reset(PLT_Service* avt);
    NPT_String GetTransportState();
    NPT_Mutex m_state;
};

} /* namespace UPNP */

