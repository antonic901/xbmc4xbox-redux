/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TCPServer.h"

#include "ServiceBroker.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "websocket/WebSocketManager.h"

#include <stdio.h>
#include <stdlib.h>

#include <memory.h>

using namespace JSONRPC;

#define RECEIVEBUFFER 4096

namespace
{
const size_t maxBufferLength = 64 * 1024;
}

CTCPServer *CTCPServer::ServerInstance = NULL;

bool CTCPServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  ServerInstance = new CTCPServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create(false);
    return true;
  }
  else
    return false;
}

void CTCPServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

bool CTCPServer::IsRunning()
{
  if (ServerInstance == NULL)
    return false;

  return ((CThread*)ServerInstance)->IsRunning();
}

CTCPServer::CTCPServer(int port, bool nonlocal) : CThread("TCPServer")
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_sdpd = NULL;
}

void CTCPServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    for (std::vector<SOCKET>::iterator it = m_servers.begin(); it != m_servers.end(); ++it)
    {
      FD_SET(*it, &rfds);
    }

    for (unsigned int i = 0; i < m_connections.size(); i++)
    {
      FD_SET(m_connections[i]->m_socket, &rfds);
    }

    int res = select(0, &rfds, NULL, NULL, &to);
    if (res == SOCKET_ERROR)
    {
      CLog::Log(LOGERROR, "JSONRPC Server: Select failed: %i", WSAGetLastError());
      CThread::Sleep(1000);
      Initialize();
    }
    else if (res > 0)
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        SOCKET socket = m_connections[i]->m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {0};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          bool close = false;
          if (nread > 0)
          {
            std::string response;
            if (m_connections[i]->IsNew())
            {
              CWebSocket *websocket = CWebSocketManager::Handle(buffer, nread, response);

              if (!response.empty())
                m_connections[i]->Send(response.c_str(), response.size());

              if (websocket != NULL)
              {
                // Replace the CTCPClient with a CWebSocketClient
                CWebSocketClient *websocketClient = new CWebSocketClient(websocket, *(m_connections[i]));
                delete m_connections[i];
                m_connections.erase(m_connections.begin() + i);
                m_connections.insert(m_connections.begin() + i, websocketClient);
              }
            }

            if (response.size() <= 0)
              m_connections[i]->PushBuffer(this, buffer, nread);

            close = m_connections[i]->Closing();
          }
          else
            close = true;

          if (close)
          {
            CLog::Log(LOGINFO, "JSONRPC Server: Disconnection detected");
            m_connections[i]->Disconnect();
            delete m_connections[i];
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      for (std::vector<SOCKET>::iterator it = m_servers.begin(); it != m_servers.end(); ++it)
      {
        if (FD_ISSET(*it, &rfds))
        {
          CLog::Log(LOGDEBUG, "JSONRPC Server: New connection detected");
          CTCPClient *newconnection = new CTCPClient();
          newconnection->m_socket =
              accept(*it, (sockaddr*)&newconnection->m_cliaddr, &newconnection->m_addrlen);

          if (newconnection->m_socket == INVALID_SOCKET)
          {
            int error = WSAGetLastError();
            delete newconnection;
            CLog::Log(LOGERROR, "JSONRPC Server: Accept of new connection failed: %i", error);
            if (WSAENOTSOCK == error)
            {
              CThread::Sleep(1000);
              Initialize();
              break;
            }
          }
          else
          {
            CLog::Log(LOGINFO, "JSONRPC Server: New connection added");
            m_connections.push_back(newconnection);
          }
        }
      }
    }
  }

  Deinitialize();
}

bool CTCPServer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  return false;
}

bool CTCPServer::Download(const char *path, CVariant &result)
{
  return false;
}

int CTCPServer::GetCapabilities()
{
  return Response | Announcing;
}

void CTCPServer::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                          const std::string& sender,
                          const std::string& message,
                          const CVariant& data)
{
  if (m_connections.empty())
    return;

  std::string str = IJSONRPCAnnouncer::AnnouncementToJSONRPC(flag, sender, message, data, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_jsonOutputCompact);

  for (unsigned int i = 0; i < m_connections.size(); i++)
  {
    {
      CSingleLock lock(m_connections[i]->m_critSection);
      if ((m_connections[i]->GetAnnouncementFlags() & flag) == 0)
        continue;
    }

    m_connections[i]->Send(str.c_str(), str.size());
  }
}

bool CTCPServer::Initialize()
{
  Deinitialize();

  bool started = false;

  started |= InitializeTCP();

  if (started)
  {
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
    CLog::Log(LOGINFO, "JSONRPC Server: Successfully initialized");
    return true;
  }
  return false;
}

bool CTCPServer::InitializeTCP()
{
  Deinitialize();

  SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Unable to create TCP socket: %i", WSAGetLastError());
    return false;
  }

  sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_port = htons((unsigned short)m_port);
  address.sin_addr.s_addr = htonl(m_nonlocal ? INADDR_ANY : INADDR_LOOPBACK);

  if (bind(socket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Unable to bind TCP socket: %i", WSAGetLastError());
    closesocket(socket);
    return false;
  }

  if (listen(socket, 10) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "JSONRPC Server: Unable to listen on TCP socket: %i", WSAGetLastError());
    closesocket(socket);
    return false;
  }

  m_servers.push_back(socket);
  return true;
}

void CTCPServer::Deinitialize()
{
  for (unsigned int i = 0; i < m_connections.size(); i++)
  {
    m_connections[i]->Disconnect();
    delete m_connections[i];
  }

  m_connections.clear();

  for (unsigned int i = 0; i < m_servers.size(); i++)
    closesocket(m_servers[i]);

  m_servers.clear();

  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

CTCPServer::CTCPClient::CTCPClient()
{
  m_new = true;
  m_announcementflags = ANNOUNCEMENT::ANNOUNCE_ALL;
  m_socket = INVALID_SOCKET;
  m_beginBrackets = 0;
  m_endBrackets = 0;
  m_beginChar = 0;
  m_endChar = 0;

  m_addrlen = sizeof(m_cliaddr);
}

CTCPServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
}

CTCPServer::CTCPClient& CTCPServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  return *this;
}

int CTCPServer::CTCPClient::GetPermissionFlags()
{
  return OPERATION_PERMISSION_ALL;
}

int CTCPServer::CTCPClient::GetAnnouncementFlags()
{
  return m_announcementflags;
}

bool CTCPServer::CTCPClient::SetAnnouncementFlags(int flags)
{
  m_announcementflags = flags;
  return true;
}

void CTCPServer::CTCPClient::Send(const char *data, unsigned int size)
{
  unsigned int sent = 0;
  do
  {
    CSingleLock lock(m_critSection);
    int result = send(m_socket, data + sent, size - sent, 0);
    if (result == SOCKET_ERROR)
    {
      CLog::Log(LOGERROR, "JSONRPC Server: Send failed: %i", WSAGetLastError());
      return;
    }
    if (result == 0)
      return;
    sent += result;
  } while (sent < size);
}

void CTCPServer::CTCPClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
  m_new = false;
  bool inObject = false;
  bool inString = false;
  bool escapeNext = false;

  for (int i = 0; i < length; i++)
  {
    char c = buffer[i];

    if (m_beginChar == 0 && c == '{')
    {
      m_beginChar = '{';
      m_endChar = '}';
    }
    else if (m_beginChar == 0 && c == '[')
    {
      m_beginChar = '[';
      m_endChar = ']';
    }

    if (m_beginChar != 0)
    {
      m_buffer.push_back(c);
      if (inObject)
      {
        if (!inString)
        {
          if (c == '"')
            inString = true;
        }
        else
        {
          if (escapeNext)
          {
            escapeNext = false;
          }
          else
          {
            if (c == '\\')
              escapeNext = true;
            else if (c == '"')
              inString = false;
          }
        }
      }
      if (!inString)
      {
        if (c == m_beginChar)
        {
          m_beginBrackets++;
          inObject = true;
        }
        else if (c == m_endChar)
        {
          m_endBrackets++;
          if (m_beginBrackets == m_endBrackets)
            inObject = false;
        }
      }
      if (m_beginBrackets > 0 && m_endBrackets > 0 && m_beginBrackets == m_endBrackets)
      {
        std::string line = CJSONRPC::MethodCall(m_buffer, host, this);
        Send(line.c_str(), line.size());
        m_beginChar = m_beginBrackets = m_endBrackets = 0;
        m_buffer.clear();
      }
    }
  }
}

void CTCPServer::CTCPClient::Disconnect()
{
  if (m_socket != INVALID_SOCKET)
  {
    CSingleLock lock(m_critSection);
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
}

void CTCPServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_new               = client.m_new;
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_announcementflags = client.m_announcementflags;
  m_beginBrackets     = client.m_beginBrackets;
  m_endBrackets       = client.m_endBrackets;
  m_beginChar         = client.m_beginChar;
  m_endChar           = client.m_endChar;
  m_buffer            = client.m_buffer;
}

CTCPServer::CWebSocketClient::CWebSocketClient(CWebSocket *websocket)
{
  m_websocket = websocket;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::CWebSocketClient(const CWebSocketClient& client)
  : CTCPServer::CTCPClient(client)
{
  *this = client;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::CWebSocketClient(CWebSocket *websocket, const CTCPClient& client)
{
  Copy(client);

  m_websocket = websocket;
  m_buffer.reserve(maxBufferLength);
}

CTCPServer::CWebSocketClient::~CWebSocketClient()
{
  delete m_websocket;
}

CTCPServer::CWebSocketClient& CTCPServer::CWebSocketClient::operator=(const CWebSocketClient& client)
{
  Copy(client);

  m_websocket = client.m_websocket;
  m_buffer = client.m_buffer;

  return *this;
}

void CTCPServer::CWebSocketClient::Send(const char *data, unsigned int size)
{
  const CWebSocketMessage *msg = m_websocket->Send(WebSocketTextFrame, data, size);
  if (msg == NULL || !msg->IsComplete())
    return;

  std::vector<const CWebSocketFrame *> frames = msg->GetFrames();
  for (unsigned int index = 0; index < frames.size(); index++)
    CTCPClient::Send(frames.at(index)->GetFrameData(), (unsigned int)frames.at(index)->GetFrameLength());
}

void CTCPServer::CWebSocketClient::PushBuffer(CTCPServer *host, const char *buffer, int length)
{
  bool send;
  const CWebSocketMessage *msg = NULL;

  if (m_buffer.size() + length > maxBufferLength)
  {
    CLog::Log(LOGINFO, "WebSocket: client buffer size %" PRIuS " exceeded", maxBufferLength);
    return Disconnect();
  }

  m_buffer.append(buffer, length);

  const char* buf = m_buffer.data();
  size_t len = m_buffer.size();

  do
  {
    if ((msg = m_websocket->Handle(buf, len, send)) != NULL && msg->IsComplete())
    {
      std::vector<const CWebSocketFrame *> frames = msg->GetFrames();
      if (send)
      {
        for (unsigned int index = 0; index < frames.size(); index++)
          CTCPClient::Send(frames.at(index)->GetFrameData(),
                           static_cast<unsigned int>(frames.at(index)->GetFrameLength()));
      }
      else
      {
        for (unsigned int index = 0; index < frames.size(); index++)
          CTCPClient::PushBuffer(host, frames.at(index)->GetApplicationData(), (int)frames.at(index)->GetLength());
      }

      delete msg;
    }
  }
  while (len > 0 && msg != NULL);

  if (len < m_buffer.size())
    m_buffer = m_buffer.substr(m_buffer.size() - len);

  if (m_websocket->GetState() == WebSocketStateClosed)
    Disconnect();
}

void CTCPServer::CWebSocketClient::Disconnect()
{
  if (m_socket != INVALID_SOCKET)
  {
    if (m_websocket->GetState() != WebSocketStateClosed && m_websocket->GetState() != WebSocketStateNotConnected)
    {
      const CWebSocketFrame *closeFrame = m_websocket->Close();
      if (closeFrame)
        Send(closeFrame->GetFrameData(), (unsigned int)closeFrame->GetFrameLength());
    }

    if (m_websocket->GetState() == WebSocketStateClosed)
      CTCPClient::Disconnect();
  }
}
