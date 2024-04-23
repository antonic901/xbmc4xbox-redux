/*
* UPnP Support for XBMC
* Copyright (c) 2006 c0diq (Sylvain Rebaud)
* Portions Copyright (c) by the authors of libPlatinum
*
* http://www.plutinosoft.com/blog/category/platinum/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "xbox/Network.h"
#include "system.h"
#include "Util.h"
#include "Application.h"
#include "ApplicationMessenger.h"

#include "UPnP.h"
#include "UPnPInternal.h"
#include "UPnPServer.h"
#include "Platinum.h"
#include "URL.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIInfoManager.h"
#include "UPnPSettings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace std;
using namespace UPNP;

extern CGUIInfoManager g_infoManager;

NPT_SET_LOCAL_LOGGER("xbmc.upnp")

#define UPNP_DEFAULT_MAX_RETURNED_ITEMS 200
#define UPNP_DEFAULT_MIN_RETURNED_ITEMS 30

/*
# Play speed
#    1 normal
#    0 invalid
DLNA_ORG_PS = 'DLNA.ORG_PS'
DLNA_ORG_PS_VAL = '1'

# Convertion Indicator
#    1 transcoded
#    0 not transcoded
DLNA_ORG_CI = 'DLNA.ORG_CI'
DLNA_ORG_CI_VAL = '0'

# Operations
#    00 not time seek range, not range
#    01 range supported
#    10 time seek range supported
#    11 both supported
DLNA_ORG_OP = 'DLNA.ORG_OP'
DLNA_ORG_OP_VAL = '01'

# Flags
#    senderPaced                      80000000  31
#    lsopTimeBasedSeekSupported       40000000  30
#    lsopByteBasedSeekSupported       20000000  29
#    playcontainerSupported           10000000  28
#    s0IncreasingSupported            08000000  27
#    sNIncreasingSupported            04000000  26
#    rtspPauseSupported               02000000  25
#    streamingTransferModeSupported   01000000  24
#    interactiveTransferModeSupported 00800000  23
#    backgroundTransferModeSupported  00400000  22
#    connectionStallingSupported      00200000  21
#    dlnaVersion15Supported           00100000  20
DLNA_ORG_FLAGS = 'DLNA.ORG_FLAGS'
DLNA_ORG_FLAGS_VAL = '01500000000000000000000000000000'
*/

#ifdef HAS_XBOX_NETWORK
#include <xtl.h>
#include <winsockx.h>
#include "NptXboxNetwork.h"

/*----------------------------------------------------------------------
|   static initializer
+---------------------------------------------------------------------*/
NPT_WinsockSystem::NPT_WinsockSystem() 
{
}

NPT_WinsockSystem::~NPT_WinsockSystem() 
{
}

NPT_WinsockSystem NPT_WinsockSystem::Initializer;

/*----------------------------------------------------------------------
|       NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    if (!g_application.getNetwork().IsAvailable(true))
        return NPT_ERROR_NETWORK_DOWN;

    NPT_IpAddress primary_address;
    primary_address.ResolveName(g_application.getNetwork().m_networkinfo.ip);

    NPT_IpAddress netmask;
    netmask.ResolveName(g_application.getNetwork().m_networkinfo.subnet);

    NPT_IpAddress broadcast_address;        
    broadcast_address.ResolveName("255.255.255.255");

    NPT_Flags flags = NPT_NETWORK_INTERFACE_FLAG_BROADCAST | NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

    NPT_MacAddress mac;
    //mac.SetAddress(NPT_MacAddress::TYPE_ETHERNET, g_application.getNetwork().m_networkinfo.mac, 6);

    // create an interface object
    char iface_name[5];
    iface_name[0] = 'i';
    iface_name[1] = 'f';
    iface_name[2] = '0';
    iface_name[3] = '0';
    iface_name[4] = '\0';
    NPT_NetworkInterface* iface = new NPT_NetworkInterface(iface_name, mac, flags);

    // set the interface address
    NPT_NetworkInterfaceAddress iface_address(
        primary_address,
        broadcast_address,
        NPT_IpAddress::Any,
        netmask);
    iface->AddAddress(iface_address);  

    // add the interface to the list
    interfaces.Add(iface);  

    return NPT_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|   NPT_GetEnvironment
+---------------------------------------------------------------------*/
NPT_Result 
NPT_GetEnvironment(const char* name, NPT_String& value)
{
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   NPT_Console::Output
+---------------------------------------------------------------------*/
void
NPT_Console::Output(const char* message)
{
    CLog::Log(LOGDEBUG, "%s", message);
}

namespace UPNP
{

/*----------------------------------------------------------------------
|   static
+---------------------------------------------------------------------*/
CUPnP* CUPnP::upnp = NULL;
// change to false for XBMC_PC if you want real UPnP functionality
// otherwise keep to true for xbox as it doesn't support multicast
// don't change unless you know what you're doing!
bool CUPnP::broadcast = true; 

/*----------------------------------------------------------------------
|   CDeviceHostReferenceHolder class
+---------------------------------------------------------------------*/
class CDeviceHostReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
};

/*----------------------------------------------------------------------
|   CCtrlPointReferenceHolder class
+---------------------------------------------------------------------*/
class CCtrlPointReferenceHolder
{
public:
    PLT_CtrlPointReference m_CtrlPoint;
};

/*----------------------------------------------------------------------
|   CUPnPCleaner class
+---------------------------------------------------------------------*/
class CUPnPCleaner : public NPT_Thread
{
public:
    CUPnPCleaner(CUPnP* upnp) : NPT_Thread(true), m_UPnP(upnp) {}
    void Run() {
        delete m_UPnP;
    }

    CUPnP* m_UPnP;
};

/*----------------------------------------------------------------------
|   CUPnPRenderer
+---------------------------------------------------------------------*/
class CUPnPRenderer : public PLT_MediaRenderer
{
public:
    CUPnPRenderer(const char*  friendly_name,
                  bool         show_ip = false,
                  const char*  uuid = NULL,
                  unsigned int port = 0);

    void UpdateState();

    // Http server handler
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request,
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

    // RenderingControl methods
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetMute(PLT_ActionReference& action);

private:
    NPT_Result SetupServices(PLT_DeviceData& data);
    NPT_Result GetMetadata(NPT_String& meta);
    NPT_Result PlayMedia(const char* uri,
                         const char* metadata = NULL,
                         PLT_Action* action = NULL);
};

/*----------------------------------------------------------------------
|   CUPnPRenderer::CUPnPRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer::CUPnPRenderer(const char*  friendly_name,
                             bool         show_ip /* = false */,
                             const char*  uuid /* = NULL */,
                             unsigned int port /* = 0 */) :
    PLT_MediaRenderer(friendly_name,
                      show_ip,
                      uuid,
                      port)
{
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::SetupServices(PLT_DeviceData& data)
{
    NPT_CHECK(PLT_MediaRenderer::SetupServices(data));

    // update what we can play
    PLT_Service* service = NULL;
    NPT_CHECK_FATAL(FindServiceByType("urn:schemas-upnp-org:service:ConnectionManager:1", service));
    service->SetStateVariable("SinkProtocolInfo"
        ,"http-get:*:*:*"
        ",xbmc-get:*:*:*"
        ",http-get:*:audio/mpegurl:*"
        ",http-get:*:audio/mpeg:*"
        ",http-get:*:audio/mpeg3:*"
        ",http-get:*:audio/mp3:*"
        ",http-get:*:audio/basic:*"
        ",http-get:*:audio/midi:*"
        ",http-get:*:audio/ulaw:*"
        ",http-get:*:audio/ogg:*"
        ",http-get:*:audio/DVI4:*"
        ",http-get:*:audio/G722:*"
        ",http-get:*:audio/G723:*"
        ",http-get:*:audio/G726-16:*"
        ",http-get:*:audio/G726-24:*"
        ",http-get:*:audio/G726-32:*"
        ",http-get:*:audio/G726-40:*"
        ",http-get:*:audio/G728:*"
        ",http-get:*:audio/G729:*"
        ",http-get:*:audio/G729D:*"
        ",http-get:*:audio/G729E:*"
        ",http-get:*:audio/GSM:*"
        ",http-get:*:audio/GSM-EFR:*"
        ",http-get:*:audio/L8:*"
        ",http-get:*:audio/L16:*"
        ",http-get:*:audio/LPC:*"
        ",http-get:*:audio/MPA:*"
        ",http-get:*:audio/PCMA:*"
        ",http-get:*:audio/PCMU:*"
        ",http-get:*:audio/QCELP:*"
        ",http-get:*:audio/RED:*"
        ",http-get:*:audio/VDVI:*"
        ",http-get:*:audio/ac3:*"
        ",http-get:*:audio/vorbis:*"
        ",http-get:*:audio/speex:*"
        ",http-get:*:audio/x-aiff:*"
        ",http-get:*:audio/x-pn-realaudio:*"
        ",http-get:*:audio/x-realaudio:*"
        ",http-get:*:audio/x-wav:*"
        ",http-get:*:audio/x-ms-wma:*"
        ",http-get:*:audio/x-mpegurl:*"
        ",http-get:*:application/x-shockwave-flash:*"
        ",http-get:*:application/ogg:*"
        ",http-get:*:application/sdp:*"
        ",http-get:*:image/gif:*"
        ",http-get:*:image/jpeg:*"
        ",http-get:*:image/ief:*"
        ",http-get:*:image/png:*"
        ",http-get:*:image/tiff:*"
        ",http-get:*:video/avi:*"
        ",http-get:*:video/mpeg:*"
        ",http-get:*:video/fli:*"
        ",http-get:*:video/flv:*"
        ",http-get:*:video/quicktime:*"
        ",http-get:*:video/vnd.vivo:*"
        ",http-get:*:video/vc1:*"
        ",http-get:*:video/ogg:*"
        ",http-get:*:video/mp4:*"
        ",http-get:*:video/BT656:*"
        ",http-get:*:video/CelB:*"
        ",http-get:*:video/JPEG:*"
        ",http-get:*:video/H261:*"
        ",http-get:*:video/H263:*"
        ",http-get:*:video/H263-1998:*"
        ",http-get:*:video/H263-2000:*"
        ",http-get:*:video/MPV:*"
        ",http-get:*:video/MP2T:*"
        ",http-get:*:video/MP1S:*"
        ",http-get:*:video/MP2P:*"
        ",http-get:*:video/BMPEG:*"
        ",http-get:*:video/x-ms-wmv:*"
        ",http-get:*:video/x-ms-avi:*"
        ",http-get:*:video/x-flv:*"
        ",http-get:*:video/x-fli:*"
        ",http-get:*:video/x-ms-asf:*"
        ",http-get:*:video/x-ms-asx:*"
        ",http-get:*:video/x-ms-wmx:*"
        ",http-get:*:video/x-ms-wvx:*"
        ",http-get:*:video/x-msvideo:*"
        );
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::ProcessHttpRequest
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::ProcessHttpRequest(NPT_HttpRequest&              request,
                                  const NPT_HttpRequestContext& context,
                                  NPT_HttpResponse&             response)
{
    // get the address of who sent us some data back
    NPT_String  ip_address = context.GetRemoteAddress().GetIpAddress().ToString();
    NPT_String  method     = request.GetMethod();
    NPT_String  protocol   = request.GetProtocol();
    NPT_HttpUrl url        = request.GetUrl();

    if (url.GetPath() == "/thumb.jpg") {
        NPT_HttpUrlQuery query(url.GetQuery());
        NPT_String filepath = query.GetField("path");
        if (!filepath.IsEmpty()) {
            NPT_HttpEntity* entity = response.GetEntity();
            if (entity == NULL) return NPT_ERROR_INVALID_STATE;

            // check the method
            if (request.GetMethod() != NPT_HTTP_METHOD_GET &&
                request.GetMethod() != NPT_HTTP_METHOD_HEAD) {
                response.SetStatus(405, "Method Not Allowed");
                return NPT_SUCCESS;
            }

            // ensure that the request's path is a valid thumb path
            if (URIUtils::IsRemote(filepath.GetChars()) ||
                !filepath.StartsWith(CProfilesManager::Get().GetUserDataFolder().c_str())) {
                response.SetStatus(404, "Not Found");
                return NPT_SUCCESS;
            }

            // prevent hackers from accessing files outside of our root
            if ((filepath.Find("/..") >= 0) || (filepath.Find("\\..") >=0)) {
                return NPT_FAILURE;
            }

            // open the file
            NPT_File file(filepath);
            NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
            if (NPT_FAILED(result)) {
                response.SetStatus(404, "Not Found");
                return NPT_SUCCESS;
            }
            NPT_InputStreamReference stream;
            file.GetInputStream(stream);
            entity->SetContentType(GetMimeType(filepath));
            entity->SetInputStream(stream, true);

            return NPT_SUCCESS;
        }
    }

    return PLT_MediaRenderer::ProcessHttpRequest(request, context, response);
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::UpdateState
+---------------------------------------------------------------------*/
void
CUPnPRenderer::UpdateState()
{
    PLT_Service *avt, *rct;
    if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", avt)))
        return;
    if (NPT_FAILED(FindServiceByType("urn:schemas-upnp-org:service:RenderingControl:1", rct)))
        return;

    CStdString buffer;
    int volume;
    if (g_application.IsMuted()) {
        rct->SetStateVariable("Mute", "1");
    } else {
        rct->SetStateVariable("Mute", "0");
    }
    volume = g_application.GetVolume();

    buffer.Format("%d", volume);
    rct->SetStateVariable("Volume", buffer.c_str());

    buffer.Format("%d", 256 * (volume * 60 - 60) / 100);
    rct->SetStateVariable("VolumeDb", buffer.c_str());

    if (g_application.IsPlaying() || g_application.IsPaused()) {
        if (g_application.IsPaused()) {
            avt->SetStateVariable("TransportState", "PAUSED_PLAYBACK");
        } else {
            avt->SetStateVariable("TransportState", "PLAYING");
        }

        avt->SetStateVariable("TransportStatus", "OK");
        avt->SetStateVariable("TransportPlaySpeed", (const char*)NPT_String::FromInteger(g_application.GetPlaySpeed()));
        avt->SetStateVariable("NumberOfTracks", "1");
        avt->SetStateVariable("CurrentTrack", "1");

        buffer = g_infoManager.GetCurrentPlayTime(TIME_FORMAT_HH_MM_SS);
        avt->SetStateVariable("RelativeTimePosition", buffer.c_str());
        buffer = StringUtils::SecondsToTimeString((long)g_infoManager.GetTotalPlayTime(), TIME_FORMAT_HH_MM_SS);
        avt->SetStateVariable("AbsoluteTimePosition", buffer.c_str());

        buffer = g_infoManager.GetDuration(TIME_FORMAT_HH_MM_SS);
        if (buffer.length() > 0) {
          avt->SetStateVariable("CurrentTrackDuration", buffer.c_str());
          avt->SetStateVariable("CurrentMediaDuration", buffer.c_str());
        } else {
          avt->SetStateVariable("CurrentTrackDuration", "00:00:00");
          avt->SetStateVariable("CurrentMediaDuration", "00:00:00");
        }

        avt->SetStateVariable("AVTransportURI", g_application.CurrentFile().c_str());
        avt->SetStateVariable("CurrentTrackURI", g_application.CurrentFile().c_str());

        NPT_String metadata;
        avt->GetStateVariableValue("AVTransportURIMetaData", metadata);
        // try to recreate the didl dynamically if not set
        if (metadata.IsEmpty()) {
            GetMetadata(metadata);
        }
        avt->SetStateVariable("CurrentTrackMetadata", metadata);
        avt->SetStateVariable("AVTransportURIMetaData", metadata);
    } else {
        avt->SetStateVariable("TransportState", "STOPPED");
        avt->SetStateVariable("TransportPlaySpeed", "1");
        avt->SetStateVariable("NumberOfTracks", "0");
        avt->SetStateVariable("CurrentTrack", "0");
        avt->SetStateVariable("RelativeTimePosition", "00:00:00");
        avt->SetStateVariable("AbsoluteTimePosition", "00:00:00");
        avt->SetStateVariable("CurrentTrackDuration", "00:00:00");
        avt->SetStateVariable("CurrentMediaDuration", "00:00:00");
    }
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::GetMetadata
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::GetMetadata(NPT_String& meta)
{
    NPT_Result res = NPT_FAILURE;
    const CFileItem &item = g_application.CurrentFileItem();
    NPT_String file_path;
    PLT_MediaObject* object = BuildObject(item, file_path, false);
    if (object) {
        // fetch the path to the thumbnail
        CStdString thumb = g_infoManager.GetImage(MUSICPLAYER_COVER, -1); //TODO: Only audio for now
            
        NPT_String ip = g_application.getNetwork().m_networkinfo.ip;

        // build url, use the internal device http server to serv the image
        NPT_HttpUrlQuery query;
        query.AddField("path", thumb.c_str());
        object->m_ExtraInfo.album_art_uri = NPT_HttpUrl(
            ip,
            m_URLDescription.GetPort(),
            "/thumb.jpg",
            query.ToString()).ToString();

        res = PLT_Didl::ToDidl(*object, "*", meta);
        delete object;
    }
    return res;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnNext
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnNext(PLT_ActionReference& action)
{
    CApplicationMessenger::Get().PlayListPlayerNext();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPause
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPause(PLT_ActionReference& action)
{
    if (!g_application.IsPaused())
        CApplicationMessenger::Get().MediaPause();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPlay
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPlay(PLT_ActionReference& action)
{
    if (g_application.IsPaused()) {
        CApplicationMessenger::Get().MediaPause();
    } else if (!g_application.IsPlaying()) {
        NPT_String uri, meta;
        PLT_Service* service;
        // look for value set previously by SetAVTransportURI
        NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));
        NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURI", uri));
        NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURIMetaData", meta));
        
        // if not set, use the current file being played
        PlayMedia(uri, meta);
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnPrevious
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnPrevious(PLT_ActionReference& action)
{
    CApplicationMessenger::Get().PlayListPlayerPrevious();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnStop
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnStop(PLT_ActionReference& action)
{
    CApplicationMessenger::Get().MediaStop();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetAVTransportURI
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetAVTransportURI(PLT_ActionReference& action)
{
    NPT_String uri, meta;
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURI", uri));
    NPT_CHECK_SEVERE(action->GetArgumentValue("CurrentURIMetaData", meta));

    // if not playing already, just keep around uri & metadata
    // and wait for play command
    if (!g_application.IsPlaying()) {
        service->SetStateVariable("TransportState", "STOPPED");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("TransportPlaySpeed", "1");
        service->SetStateVariable("AVTransportURI", uri);
        service->SetStateVariable("AVTransportURIMetaData", meta);

        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
        return NPT_SUCCESS;
    }

    return PlayMedia(uri, meta, action.AsPointer());
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::PlayMedia
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::PlayMedia(const char* uri, const char* meta, PLT_Action* action)
{
    bool bImageFile = false;
    PLT_Service* service;
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));

    service->SetStateVariable("TransportState", "TRANSITIONING");
    service->SetStateVariable("TransportStatus", "OK");
    service->SetStateVariable("TransportPlaySpeed", "1");

    PLT_MediaObjectListReference list;
    PLT_MediaObject*             object = NULL;

    if (meta && NPT_SUCCEEDED(PLT_Didl::FromDidl(meta, list))) {
      list->Get(0, object);
    }

    if(object) {
        CFileItem item(uri, false);

        PLT_MediaItemResource* res = object->m_Resources.GetFirstItem();
        for(NPT_Cardinal i = 0; i < object->m_Resources.GetItemCount(); i++) {
          if(object->m_Resources[i].m_Uri == uri) { 
            res = &object->m_Resources[i];
            break;
          }
        }
        for(NPT_Cardinal i = 0; i < object->m_Resources.GetItemCount(); i++) {
            if(object->m_Resources[i].m_ProtocolInfo.ToString().StartsWith("xbmc-get:")) {
            res = &object->m_Resources[i];
            item.SetPath(CStdString(res->m_Uri));
            break;
          }
        }

        if (res && res->m_ProtocolInfo.IsValid()) {
            item.SetMimeType((const char*)res->m_ProtocolInfo.GetContentType());
        }

        item.m_dateTime.SetFromDateString((const char*)object->m_Date);
        item.m_strTitle = (const char*)object->m_Title;
        item.SetLabel((const char*)object->m_Title);
        item.SetLabelPreformated(true);
        item.SetArt("thumb", (const char*)object->m_ExtraInfo.album_art_uri);
        if       (object->m_ObjectClass.type.StartsWith("object.item.audioItem")) {            
            if(NPT_SUCCEEDED(PopulateTagFromObject(*item.GetMusicInfoTag(), *object, res)))
                item.SetLabelPreformated(false);
        } else if(object->m_ObjectClass.type.StartsWith("object.item.videoItem")) {
            if(NPT_SUCCEEDED(PopulateTagFromObject(*item.GetVideoInfoTag(), *object, res)))
                item.SetLabelPreformated(false);
        } else if(object->m_ObjectClass.type.StartsWith("object.item.imageItem")) {
            bImageFile = true;
        }
        bImageFile?CApplicationMessenger::Get().PictureShow(item.GetPath())
                  :CApplicationMessenger::Get().MediaPlay(item);
    } else {
        bImageFile = NPT_String(PLT_MediaObject::GetUPnPClass(uri)).StartsWith("object.item.imageItem", true);

        bImageFile?CApplicationMessenger::Get().PictureShow((const char*)uri)
                  :CApplicationMessenger::Get().MediaPlay((const char*)uri);
    }

    if (!g_application.IsPlaying()) {
        service->SetStateVariable("TransportState", "STOPPED");
        service->SetStateVariable("TransportStatus", "ERROR_OCCURRED");
    } else {
        service->SetStateVariable("AVTransportURI", uri);
        service->SetStateVariable("AVTransportURIMetaData", meta);
    }

    if (action) {
        NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetVolume
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetVolume(PLT_ActionReference& action)
{
    NPT_String volume;
    NPT_CHECK_SEVERE(action->GetArgumentValue("DesiredVolume", volume));
    g_application.SetVolume(atoi((const char*)volume));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSetMute
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSetMute(PLT_ActionReference& action)
{
    NPT_String mute;
    NPT_CHECK_SEVERE(action->GetArgumentValue("DesiredMute",mute));
    if((mute == "1") ^ g_application.IsMuted())
        g_application.ToggleMute();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPRenderer::OnSeek
+---------------------------------------------------------------------*/
NPT_Result
CUPnPRenderer::OnSeek(PLT_ActionReference& action)
{
    if (!g_application.IsPlaying()) return NPT_ERROR_INVALID_STATE;

    NPT_String unit, target;
    NPT_CHECK_SEVERE(action->GetArgumentValue("Unit", unit));
    NPT_CHECK_SEVERE(action->GetArgumentValue("Target", target));

    if (!unit.Compare("REL_TIME")) {
        // converts target to seconds
        NPT_UInt32 seconds;
        NPT_CHECK_SEVERE(PLT_Didl::ParseTimeStamp(target, seconds));
        g_application.SeekTime(seconds);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CRendererReferenceHolder class
+---------------------------------------------------------------------*/
class CRendererReferenceHolder
{
public:
    PLT_DeviceHostReference m_Device;
};

/*----------------------------------------------------------------------
|   CMediaBrowser class
+---------------------------------------------------------------------*/
class CMediaBrowser : public PLT_SyncMediaBrowser,
                      public PLT_MediaContainerChangesListener
{
public:
    CMediaBrowser(PLT_CtrlPointReference& ctrlPoint)
        : PLT_SyncMediaBrowser(ctrlPoint, true)
    {
        SetContainerListener(this);
    }

    // PLT_MediaBrowser methods
    virtual bool OnMSAdded(PLT_DeviceDataReference& device)
    {
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam("upnp://");
        g_windowManager.SendThreadMessage(message);

        return PLT_SyncMediaBrowser::OnMSAdded(device);
    }
    virtual void OnMSRemoved(PLT_DeviceDataReference& device)
    {
        PLT_SyncMediaBrowser::OnMSRemoved(device);

        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam("upnp://");
        g_windowManager.SendThreadMessage(message);

        PLT_SyncMediaBrowser::OnMSRemoved(device);
    }

    // PLT_MediaContainerChangesListener methods
    virtual void OnContainerChanged(PLT_DeviceDataReference& device,
                                    const char*              item_id,
                                    const char*              update_id)
    {
        NPT_String path = "upnp://"+device->GetUUID()+"/";
        if (!NPT_StringsEqual(item_id, "0")) {
            CStdString id = item_id;
            CURL::Encode(id);
            path += id.c_str();
            path += "/";
        }

        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
        message.SetStringParam(path.GetChars());
        g_windowManager.SendThreadMessage(message);
    }
};


/*----------------------------------------------------------------------
|   CUPnP::CUPnP
+---------------------------------------------------------------------*/
CUPnP::CUPnP() :
    m_MediaBrowser(NULL),
    m_ServerHolder(new CDeviceHostReferenceHolder()),
    m_RendererHolder(new CRendererReferenceHolder()),
    m_CtrlPointHolder(new CCtrlPointReferenceHolder())
{
    broadcast = false;

    // initialize upnp in broadcast listening mode for xbmc
    m_UPnP = new PLT_UPnP(1900, !broadcast);

    // keep main IP around
    m_IP = g_application.getNetwork().m_networkinfo.ip;
    NPT_List<NPT_IpAddress> list;
    if (NPT_SUCCEEDED(PLT_UPnPMessageHelper::GetIPAddresses(list))) {
        m_IP = (*(list.GetFirstItem())).ToString();
    }

    // start upnp monitoring
    m_UPnP->Start();
}

/*----------------------------------------------------------------------
|   CUPnP::~CUPnP
+---------------------------------------------------------------------*/
CUPnP::~CUPnP()
{
    m_UPnP->Stop();
    StopClient();
    StopServer();

    delete m_UPnP;
    delete m_ServerHolder;
    delete m_RendererHolder;
    delete m_CtrlPointHolder;
}

/*----------------------------------------------------------------------
|   CUPnP::GetInstance
+---------------------------------------------------------------------*/
CUPnP*
CUPnP::GetInstance()
{
    if (!upnp) {
        upnp = new CUPnP();
    }

    return upnp;
}

/*----------------------------------------------------------------------
|   CUPnP::ReleaseInstance
+---------------------------------------------------------------------*/
void
CUPnP::ReleaseInstance(bool bWait)
{
    if (upnp) {
        CUPnP* _upnp = upnp;
        upnp = NULL;

        if (bWait) {
            delete _upnp;
        } else {
            // since it takes a while to clean up
            // starts a detached thread to do this
            CUPnPCleaner* cleaner = new CUPnPCleaner(_upnp);
            cleaner->Start();
        }
    }
}

/*----------------------------------------------------------------------
|   CUPnP::StartClient
+---------------------------------------------------------------------*/
void
CUPnP::StartClient()
{
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) return;

    // create controlpoint, pass NULL to avoid sending a multicast search
    m_CtrlPointHolder->m_CtrlPoint = new PLT_CtrlPoint(broadcast?NULL:"upnp:rootdevice");

    // ignore our own server
    if (!m_ServerHolder->m_Device.IsNull()) {
        m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_ServerHolder->m_Device->GetUUID());
    }

    // start it
    m_UPnP->AddCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);

    // start browser
    m_MediaBrowser = new CMediaBrowser(m_CtrlPointHolder->m_CtrlPoint);

#ifdef _XBOX
    // Issue a search request on the every 6 seconds, both on broadcast and multicast
    // xbox can't receive multicast, but it can send it so upnp clients know we are here
    m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, 6000);
    m_CtrlPointHolder->m_CtrlPoint->Discover(NPT_HttpUrl("239.255.255.250", 1900, "*"), "upnp:rootdevice", 1, 6000);
#endif
}

/*----------------------------------------------------------------------
|   CUPnP::StopClient
+---------------------------------------------------------------------*/
void
CUPnP::StopClient()
{
    if (m_CtrlPointHolder->m_CtrlPoint.IsNull()) return;

    m_UPnP->RemoveCtrlPoint(m_CtrlPointHolder->m_CtrlPoint);
    m_CtrlPointHolder->m_CtrlPoint = NULL;

    delete m_MediaBrowser;
    m_MediaBrowser = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateServer
+---------------------------------------------------------------------*/
CUPnPServer*
CUPnP::CreateServer(int port /* = 0 */)
{
    CUPnPServer* device =
        new CUPnPServer(g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME),
                        CUPnPSettings::Get().GetServerUUID().length()?CUPnPSettings::Get().GetServerUUID().c_str():NULL,
                        port);

    // trying to set optional upnp values for XP UPnP UI Icons to detect us
    // but it doesn't work anyways as it requires multicast for XP to detect us
    device->m_PresentationURL =
        NPT_HttpUrl(m_IP,
                    CSettings::Get().GetInt("services.webserverport"),
                    "/").ToString();

    device->m_ModelName        = "XBMC Media Center";
    device->m_ModelNumber      = "1.0";
    device->m_ModelDescription = "XBMC Media Center - Media Server";
    device->m_ModelURL         = "http://www.xbmc.org/";
    device->m_Manufacturer     = "Team XBMC";
    device->m_ManufacturerURL  = "http://www.xbmc.org/";

    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartServer
+---------------------------------------------------------------------*/
bool
CUPnP::StartServer()
{
    if (!m_ServerHolder->m_Device.IsNull()) return false;

    // load upnpserver.xml
    CStdString filename;
    URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), "upnpserver.xml", filename);
    CUPnPSettings::Get().Load(filename);

    // create the server with a XBox compatible friendlyname and UUID from upnpserver.xml if found
    m_ServerHolder->m_Device = CreateServer(CUPnPSettings::Get().GetServerPort());

#ifdef _XBOX
    // since the xbox doesn't support multicast
    // we use broadcast but we advertise more often
    m_ServerHolder->m_Device->SetBroadcast(broadcast);
#endif

    // tell controller to ignore ourselves from list of upnp servers
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) {
        m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_ServerHolder->m_Device->GetUUID());
    }

    // start server
    NPT_Result res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    if (NPT_FAILED(res)) {
        // if the upnp device port was not 0, it could have failed because
        // of port being in used, so restart with a random port
        if (CUPnPSettings::Get().GetServerPort() > 0) m_ServerHolder->m_Device = CreateServer(0);

        // tell controller to ignore ourselves from list of upnp servers
        if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) {
            m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_ServerHolder->m_Device->GetUUID());
        }

        res = m_UPnP->AddDevice(m_ServerHolder->m_Device);
    }

    // save port but don't overwrite saved settings if port was random
    if (NPT_SUCCEEDED(res)) {
        if (CUPnPSettings::Get().GetServerPort() == 0) {
            CUPnPSettings::Get().SetServerPort(m_ServerHolder->m_Device->GetPort());
        }
        CUPnPServer::m_MaxReturnedItems = UPNP_DEFAULT_MAX_RETURNED_ITEMS;
        if (CUPnPSettings::Get().GetMaximumReturnedItems() > 0) {
            // must be > UPNP_DEFAULT_MIN_RETURNED_ITEMS
            CUPnPServer::m_MaxReturnedItems = max(UPNP_DEFAULT_MIN_RETURNED_ITEMS, CUPnPSettings::Get().GetMaximumReturnedItems());
        }
        CUPnPSettings::Get().SetMaximumReturnedItems(CUPnPServer::m_MaxReturnedItems);
    }

    // save UUID
    CUPnPSettings::Get().SetServerUUID(m_ServerHolder->m_Device->GetUUID().GetChars());
    return CUPnPSettings::Get().Save(filename);
}

/*----------------------------------------------------------------------
|   CUPnP::StopServer
+---------------------------------------------------------------------*/
void
CUPnP::StopServer()
{
    if (m_ServerHolder->m_Device.IsNull()) return;

    m_UPnP->RemoveDevice(m_ServerHolder->m_Device);
    m_ServerHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::CreateRenderer
+---------------------------------------------------------------------*/
CUPnPRenderer*
CUPnP::CreateRenderer(int port /* = 0 */)
{
    CUPnPRenderer* device =
        new CUPnPRenderer(g_infoManager.GetLabel(SYSTEM_FRIENDLY_NAME).c_str(),
                          false,
                          (CUPnPSettings::Get().GetRendererUUID().length() ? CUPnPSettings::Get().GetRendererUUID().c_str() : NULL),
                          port);

    device->m_PresentationURL =
        NPT_HttpUrl(m_IP,
                    CSettings::Get().GetInt("services.webserverport"),
                    "/").ToString();
    device->m_ModelName = "XBMC";
    device->m_ModelNumber = "2.0";
    device->m_ModelDescription = "XBMC Media Center - Media Renderer";
    device->m_ModelURL = "http://www.xbmc.org/";
    device->m_Manufacturer = "Team XBMC";
    device->m_ManufacturerURL = "http://www.xbmc.org/";

    return device;
}

/*----------------------------------------------------------------------
|   CUPnP::StartRenderer
+---------------------------------------------------------------------*/
bool CUPnP::StartRenderer()
{
    if (!m_RendererHolder->m_Device.IsNull()) return false;

    CStdString filename;
    URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), "upnpserver.xml", filename);
    CUPnPSettings::Get().Load(filename);

    m_RendererHolder->m_Device = CreateRenderer(CUPnPSettings::Get().GetRendererPort());

    // tell controller to ignore ourselves from list of upnp servers
    if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) {
        m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_RendererHolder->m_Device->GetUUID());
    }

#ifdef _XBOX
    m_RendererHolder->m_Device->SetBroadcast(broadcast);
#endif

    NPT_Result res = m_UPnP->AddDevice(m_RendererHolder->m_Device);

    // failed most likely because port is in use, try again with random port now
    if (NPT_FAILED(res) && CUPnPSettings::Get().GetRendererPort() != 0) {
        m_RendererHolder->m_Device = CreateRenderer(0);

        // tell controller to ignore ourselves from list of upnp servers
        if (!m_CtrlPointHolder->m_CtrlPoint.IsNull()) {
            m_CtrlPointHolder->m_CtrlPoint->IgnoreUUID(m_RendererHolder->m_Device->GetUUID());
        }

        res = m_UPnP->AddDevice(m_RendererHolder->m_Device);
    }

    if (NPT_SUCCEEDED(res) && CUPnPSettings::Get().GetRendererPort() == 0) {
        CUPnPSettings::Get().SetRendererPort(m_RendererHolder->m_Device->GetPort());
    }

    // save UUID
    CUPnPSettings::Get().SetRendererUUID(m_RendererHolder->m_Device->GetUUID().GetChars());
    return CUPnPSettings::Get().Save(filename);
}

/*----------------------------------------------------------------------
|   CUPnP::StopRenderer
+---------------------------------------------------------------------*/
void CUPnP::StopRenderer()
{
    if (m_RendererHolder->m_Device.IsNull()) return;

    m_UPnP->RemoveDevice(m_RendererHolder->m_Device);
    m_RendererHolder->m_Device = NULL;
}

/*----------------------------------------------------------------------
|   CUPnP::UpdateState
+---------------------------------------------------------------------*/
void CUPnP::UpdateState()
{
  if (!m_RendererHolder->m_Device.IsNull())
      ((CUPnPRenderer*)m_RendererHolder->m_Device.AsPointer())->UpdateState();
}

} /* namespace UPNP */
