/*
 * Socket classes
 *  Copyright (c) 2008 d4rk
 *  Copyright (C) 2008-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Socket.h"

#include "utils/log.h"

#include <boost/make_shared.hpp>
#include <vector>

using namespace SOCKETS;

#ifdef WINSOCK_VERSION
#define close closesocket
#endif

/**********************************************************************/
/* CPosixUDPSocket                                                    */
/**********************************************************************/

bool CPosixUDPSocket::Bind(bool localOnly, int port, int range)
{
  // close any existing sockets
  Close();

  m_iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (m_iSock == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "UDP: Could not create socket");
    CLog::Log(LOGERROR, "UDP: %s", strerror(errno));
    return false;
  }

  // make sure we can reuse the address
#ifdef WINSOCK_VERSION
  const char yes=1;
#else
  int yes = 1;
#endif
  if (setsockopt(m_iSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
  {
    CLog::Log(LOGWARNING, "UDP: Could not enable the address reuse options");
    CLog::Log(LOGWARNING, "UDP: %s", strerror(errno));
  }

  // bind to any address or localhost
  if (localOnly)
    m_addr = CAddress("127.0.0.1");
  else
    m_addr = CAddress("0.0.0.0");

  // bind the socket ( try from port to port+range )
  for (m_iPort = port; m_iPort <= port + range; ++m_iPort)
  {
    m_addr.saddr.saddr4.sin_port = htons(m_iPort);

    if (bind(m_iSock, (struct sockaddr*)&m_addr.saddr, m_addr.size) != 0)
    {
      CLog::Log(LOGWARNING, "UDP: Error binding socket on port %i", m_iPort);
      CLog::Log(LOGWARNING, "UDP: %s", strerror(errno));
    }
    else
    {
      CLog::Log(LOGINFO, "UDP: Listening on port %i", m_iPort);
      SetBound();
      SetReady();
      break;
    }
  }

  // check for errors
  if (!Bound())
  {
    CLog::Log(LOGERROR, "UDP: No suitable port found");
    Close();
    return false;
  }

  return true;
}

void CPosixUDPSocket::Close()
{
  if (m_iSock>=0)
  {
    close(m_iSock);
    m_iSock = INVALID_SOCKET;
  }
  SetBound(false);
  SetReady(false);
}

int CPosixUDPSocket::Read(CAddress& addr, const int buffersize, void *buffer)
{
  return (int)recvfrom(m_iSock, (char*)buffer, (size_t)buffersize, 0,
                       (struct sockaddr*)&addr.saddr, &addr.size);
}

int CPosixUDPSocket::SendTo(const CAddress& addr, const int buffersize,
                          const void *buffer)
{
  return (int)sendto(m_iSock, (const char *)buffer, (size_t)buffersize, 0,
                     (const struct sockaddr*)&addr.saddr, addr.size);
}

/**********************************************************************/
/* CSocketFactory                                                     */
/**********************************************************************/

boost::shared_ptr<CUDPSocket> CSocketFactory::CreateUDPSocket()
{
  return boost::make_shared<CPosixUDPSocket>();
}

/**********************************************************************/
/* CSocketListener                                                    */
/**********************************************************************/

CSocketListener::CSocketListener()
{
  Clear();
}

void CSocketListener::AddSocket(CBaseSocket *sock)
{
  // WARNING: not threadsafe (which is ok for now)

  if (sock && sock->Ready())
  {
    m_sockets.push_back(sock);
    FD_SET(sock->Socket(), &m_fdset);
#ifndef WINSOCK_VERSION
    if (sock->Socket() > m_iMaxSockets)
      m_iMaxSockets = sock->Socket();
#endif
  }
}

bool CSocketListener::Listen(int timeout)
{
  if (m_sockets.size()==0)
  {
    CLog::Log(LOGERROR, "SOCK: No sockets to listen for");
    throw LISTENEMPTY;
  }

  m_iReadyCount = 0;
  m_iCurrentSocket = 0;

  FD_ZERO(&m_fdset);
  for (unsigned int i = 0 ; i<m_sockets.size() ; i++)
  {
    FD_SET(m_sockets[i]->Socket(), &m_fdset);
  }

  // set our timeout
  struct timeval tv;
  int rem = timeout % 1000;
  tv.tv_usec = rem * 1000;
  tv.tv_sec = timeout / 1000;

  m_iReadyCount = select(m_iMaxSockets+1, &m_fdset, NULL, NULL, (timeout < 0 ? NULL : &tv));

  if (m_iReadyCount<0)
  {
    CLog::Log(LOGERROR, "SOCK: Error selecting socket(s)");
    Clear();
    throw LISTENERROR;
  }
  else
  {
    m_iCurrentSocket = 0;
    return (m_iReadyCount>0);
  }
}

void CSocketListener::Clear()
{
  m_sockets.clear();
  FD_ZERO(&m_fdset);
  m_iReadyCount = 0;
  m_iMaxSockets = 0;
  m_iCurrentSocket = 0;
}

CBaseSocket* CSocketListener::GetFirstReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;

  for (int i = 0 ; i < (int)m_sockets.size() ; i++)
  {
    if (FD_ISSET((m_sockets[i])->Socket(), &m_fdset))
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}

CBaseSocket* CSocketListener::GetNextReadySocket()
{
  if (m_iReadyCount<=0)
    return NULL;

  for (int i = m_iCurrentSocket+1 ; i<(int)m_sockets.size() ; i++)
  {
    if (FD_ISSET(m_sockets[i]->Socket(), &m_fdset))
    {
      m_iCurrentSocket = i;
      return m_sockets[i];
    }
  }
  return NULL;
}
