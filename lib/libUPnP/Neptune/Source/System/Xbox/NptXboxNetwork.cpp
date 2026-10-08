/*****************************************************************
|
|  Neptune - Network :: Xbox Winsock Implementation
|
|  (c) 2001-2005 Gilles Boccon-Gibod
|  Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <xtl.h>
#include <winsockx.h>

#include "NptNetwork.h"
#include "NptXboxNetwork.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   NPT_IpAddress::Any and NPT_IpAddress::Loopback
+---------------------------------------------------------------------*/
const NPT_IpAddress NPT_IpAddress::Any;
const NPT_IpAddress NPT_IpAddress::Loopback(127,0,0,1);

/*----------------------------------------------------------------------
|   NPT_IpAddress::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_IpAddress::ToString() const
{
    NPT_String address;
    address.Reserve(16);
    address += NPT_String::FromInteger(m_Address[0]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[1]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[2]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[3]);

    return address;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_IpAddress::Parse(const char* name)
{
    // check the name
    if (name == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    // clear the address
    NPT_SetMemory(&m_Address[0], 0, sizeof(m_Address));

    // parse
    unsigned int  fragment;
    bool          fragment_empty = true;
    unsigned char address[4];
    unsigned int  accumulator;
    for (fragment = 0, accumulator = 0; fragment < 4; ++name) {
        if (*name == '\0' || *name == '.') {
            // fragment terminator
            if (fragment_empty) return NPT_ERROR_INVALID_SYNTAX;
            address[fragment++] = accumulator;
            if (*name == '\0') break;
            accumulator = 0;
            fragment_empty = true;
        } else if (*name >= '0' && *name <= '9') {
            // numerical character
            accumulator = accumulator*10 + (*name - '0');
            if (accumulator > 255) return NPT_ERROR_INVALID_SYNTAX;
            fragment_empty = false;
        } else {
            // invalid character
            return NPT_ERROR_INVALID_SYNTAX;
        }
    }

    if (fragment == 4 && *name == '\0' && !fragment_empty) {
        return Set(address);
    } else {
        return NPT_ERROR_INVALID_SYNTAX;
    }
}

/*----------------------------------------------------------------------
|   static initializer
+---------------------------------------------------------------------*/
NPT_WinsockSystem::NPT_WinsockSystem() {
    XNetStartupParams xnsp;
    memset(&xnsp, 0, sizeof(xnsp));
    xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
    xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

    // create more memory for networking
    xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
    xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
    xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
    xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
    xnsp.cfgSockMaxSockets = 64; // default = 64
    xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
    xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

    INT err = XNetStartup(&xnsp);

    WORD    wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup( wVersionRequested, &wsaData );
}
NPT_WinsockSystem::~NPT_WinsockSystem() {
    WSACleanup();
    XNetCleanup();
}
NPT_WinsockSystem NPT_WinsockSystem::Initializer;

/*----------------------------------------------------------------------
|       NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    XNADDR xna;
    DWORD  state;
    do {
        state = XNetGetTitleXnAddr(&xna);
        Sleep(100);
    } while (state == XNET_GET_XNADDR_PENDING);

    if (state & XNET_GET_XNADDR_STATIC || state & XNET_GET_XNADDR_DHCP) {
        NPT_IpAddress primary_address(ntohl(xna.ina.s_addr));
        NPT_IpAddress netmask; /* no support for netmask */
        NPT_IpAddress broadcast_address(ntohl(xna.ina.s_addr));
        NPT_Flags     flags = NPT_NETWORK_INTERFACE_FLAG_BROADCAST | NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

        NPT_MacAddress mac;
        if (state & XNET_GET_XNADDR_ETHERNET) {
            mac.SetAddress(NPT_MacAddress::TYPE_ETHERNET, xna.abEnet, 6);
        }

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
    }

    return NPT_SUCCESS;
}

