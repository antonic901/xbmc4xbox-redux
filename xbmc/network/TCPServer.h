/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/IJSONRPCAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "websocket/WebSocket.h"

#include <vector>

#include <xtl.h>
#include <winsockx.h>

#include "PlatformDefs.h"

class CVariant;

namespace JSONRPC
{
  class CTCPServer : public ITransportLayer, public JSONRPC::IJSONRPCAnnouncer, public CThread
  {
  public:
    static bool StartServer(int port, bool nonlocal);
    static void StopServer(bool bWait);
    static bool IsRunning();

    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    virtual bool Download(const char *path, CVariant &result);
    virtual int GetCapabilities();

    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data);
  protected:
    virtual void Process();
  private:
    CTCPServer(int port, bool nonlocal);
    bool Initialize();
    bool InitializeTCP();
    void Deinitialize();

    class CTCPClient : public IClient
    {
    public:
      CTCPClient();
      //Copying a CCriticalSection is not allowed, so copy everything but that
      //when adding a member variable, make sure to copy it in CTCPClient::Copy
      CTCPClient(const CTCPClient& client);
      CTCPClient& operator=(const CTCPClient& client);
      virtual ~CTCPClient() {}

      virtual int GetPermissionFlags();
      virtual int GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);

      virtual void Send(const char *data, unsigned int size);
      virtual void PushBuffer(CTCPServer *host, const char *buffer, int length);
      virtual void Disconnect();

      virtual bool IsNew() const { return m_new; }
      virtual bool Closing() const { return false; }

      SOCKET m_socket;
      sockaddr_in m_cliaddr;
      int m_addrlen;
      CCriticalSection m_critSection;

    protected:
      void Copy(const CTCPClient& client);
    private:
      bool m_new;
      int m_announcementflags;
      int m_beginBrackets, m_endBrackets;
      char m_beginChar, m_endChar;
      std::string m_buffer;
    };

    class CWebSocketClient : public CTCPClient
    {
    public:
      explicit CWebSocketClient(CWebSocket *websocket);
      CWebSocketClient(const CWebSocketClient& client);
      CWebSocketClient(CWebSocket *websocket, const CTCPClient& client);
      CWebSocketClient& operator=(const CWebSocketClient& client);
      virtual ~CWebSocketClient();

      virtual void Send(const char *data, unsigned int size);
      virtual void PushBuffer(CTCPServer *host, const char *buffer, int length);
      virtual void Disconnect();

      virtual bool IsNew() const { return m_websocket == NULL; }
      virtual bool Closing() const { return m_websocket != NULL && m_websocket->GetState() == WebSocketStateClosed; }

    private:
      CWebSocket *m_websocket;
      std::string m_buffer;
    };

    std::vector<CTCPClient*> m_connections;
    std::vector<SOCKET> m_servers;
    int m_port;
    bool m_nonlocal;
    void* m_sdpd;

    static CTCPServer *ServerInstance;
  };
}
