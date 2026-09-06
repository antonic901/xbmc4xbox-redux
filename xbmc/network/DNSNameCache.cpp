/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DNSNameCache.h"

#include "ServiceBroker.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

CDNSNameCache g_DNSCache;

CCriticalSection CDNSNameCache::m_critical;

CDNSNameCache::CDNSNameCache(void) {}

CDNSNameCache::~CDNSNameCache(void) {}

bool CDNSNameCache::Lookup(const std::string& strHostName, std::string& strIpAddress)
{
  if (strHostName.empty() && strIpAddress.empty())
    return false;

  // first see if this is already an ip address
  unsigned long ulHostIp = inet_addr( strHostName.c_str() );
  if (ulHostIp != 0xFFFFFFFF)
  {
    // yes it is, just return it
    strIpAddress = StringUtils::Format("%d.%d.%d.%d", (ulHostIp & 0xFF), (ulHostIp & 0xFF00) >> 8, (ulHostIp & 0xFF0000) >> 16, (ulHostIp & 0xFF000000) >> 24);
    return true;
  }

  // check if there's a custom entry or if it's already cached
  if (g_DNSCache.GetCached(strHostName, strIpAddress))
    return true;

  // perform dns lookup
  std::string fqdn = strHostName;

  std::string suffix = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("network.dnssuffix");
  if (!suffix.empty() && strHostName.find(".") < 0)
    fqdn = strHostName + "." + suffix;

  WSAEVENT hEvent = WSACreateEvent();
  XNDNS* pDns = NULL;
  INT err = XNetDnsLookup(fqdn.c_str(), hEvent, &pDns);
  WaitForSingleObject((HANDLE)hEvent, INFINITE);
  if (pDns && pDns->iStatus == 0)
  {
    unsigned long ulHostIp;
    memcpy(&ulHostIp, &(pDns->aina[0].s_addr), 4);

    strIpAddress = StringUtils::Format("%d.%d.%d.%d", (ulHostIp & 0xFF), (ulHostIp & 0xFF00) >> 8, (ulHostIp & 0xFF0000) >> 16, (ulHostIp & 0xFF000000) >> 24);

    g_DNSCache.Add(fqdn, strIpAddress);

    XNetDnsRelease(pDns);
    WSACloseEvent(hEvent);
    return true;
  }

  if (pDns)
  {
    CLog::Log(LOGERROR, "DNS lookup for %s failed: %u", strHostName.c_str(), pDns->iStatus);
    XNetDnsRelease(pDns);
  }
  else
    CLog::Log(LOGERROR, "DNS lookup for %s failed: %u", strHostName.c_str(), err);

  WSACloseEvent(hEvent);

  CLog::Log(LOGERROR, "Unable to lookup host: '%s'", strHostName.c_str());
  return false;
}

bool CDNSNameCache::GetCached(const std::string& strHostName, std::string& strIpAddress)
{
  {
    CSingleLock lock(m_critical);

    // loop through all DNSname entries and see if strHostName is cached
    for (std::vector<CDNSName>::const_iterator DNSname = g_DNSCache.m_vecDNSNames.begin(); DNSname != g_DNSCache.m_vecDNSNames.end(); ++DNSname)
    {
      if (DNSname->m_strHostName == strHostName)
      {
        strIpAddress = DNSname->m_strIpAddress;
        return true;
      }
    }
  }

  // not cached
  return false;
}

void CDNSNameCache::Add(const std::string& strHostName, const std::string& strIpAddress)
{
  CDNSName dnsName;

  dnsName.m_strHostName = strHostName;
  dnsName.m_strIpAddress  = strIpAddress;

  CSingleLock lock(m_critical);
  g_DNSCache.m_vecDNSNames.push_back(dnsName);
}

