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
#include "UPnPRenderer.h"
#include "UPnPServer.h"
#include "Platinum.h"
#include "PltSyncMediaBrowser.h"
#include "URL.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIInfoManager.h"
#include "video/VideoInfoTag.h"
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
|   CUPnP::MarkWatched
+---------------------------------------------------------------------*/
bool
CUPnP::MarkWatched(const CFileItem& item, const bool watched)
{
    // TODO: implement this
    return false;
}

/*----------------------------------------------------------------------
|   CUPnP::SaveFileState
+---------------------------------------------------------------------*/
bool
CUPnP::SaveFileState(const CFileItem& item, const CBookmark& bookmark, const bool updatePlayCount)
{
    // TODO: implement this
    return false;
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
