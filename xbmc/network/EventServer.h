/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "EventClient.h"
#include "Socket.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <atomic.h>
#include <map>
#include <queue>
#include <vector>

namespace EVENTSERVER
{

  /**********************************************************************/
  /* UDP Event Server Class                                             */
  /**********************************************************************/
  class CEventServer : private CThread
  {
  public:
    static void RemoveInstance();
    static CEventServer* GetInstance();

    CEventServer();
    virtual ~CEventServer() {}

    // IRunnable entry point for thread
    virtual void  Process();

    bool Running()
    {
      return m_bRunning.value();
    }

    void RefreshSettings()
    {
      CSingleLock lock(m_critSection);
      m_bRefreshSettings = true;
    }

    // start / stop server
    void StartServer();
    void StopServer(bool bWait);

    // get events
    unsigned int GetButtonCode(std::string& strMapName, bool& isAxis, float& amount, bool &isJoystick);
    bool ExecuteNextAction();
    int GetNumberOfClients();

  protected:
    void Cleanup();
    void Run();
    void ProcessPacket(SOCKETS::CAddress& addr, int packetSize);
    void ProcessEvents();
    void RefreshClients();

    std::map<unsigned long, boost::shared_ptr<EVENTCLIENT::CEventClient> > m_clients;
    static boost::shared_ptr<CEventServer> m_pInstance;
    boost::shared_ptr<SOCKETS::CUDPSocket> m_pSocket;
    int              m_iPort;
    int              m_iListenTimeout;
    int              m_iMaxClients;
    std::vector<uint8_t> m_pPacketBuffer;
    atomic<bool> m_bRunning;
    CCriticalSection m_critSection;
    bool             m_bRefreshSettings;
  };

}

