/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <string>

class CHTTPJsonRpcHandler : public IHTTPRequestHandler
{
public:
  CHTTPJsonRpcHandler() {}
  ~CHTTPJsonRpcHandler() {}

  // implementations of IHTTPRequestHandler
  IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPJsonRpcHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const;

  MHD_RESULT HandleRequest();

  HttpResponseRanges GetResponseData() const;

  int GetPriority() const { return 5; }

protected:
  explicit CHTTPJsonRpcHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

  bool appendPostData(const char *data, size_t size);

private:
  std::string m_requestData;
  std::string m_responseData;
  CHttpResponseRange m_responseRange;

  class CHTTPTransportLayer : public JSONRPC::ITransportLayer
  {
  public:
    CHTTPTransportLayer() {}
    ~CHTTPTransportLayer() {}

    // implementations of JSONRPC::ITransportLayer
    bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    bool Download(const char *path, CVariant &result);
    int GetCapabilities();
  };
  CHTTPTransportLayer m_transportLayer;

  class CHTTPClient : public JSONRPC::IClient
  {
  public:
    explicit CHTTPClient(HTTPMethod method);
    ~CHTTPClient() {}

    int GetPermissionFlags() { return m_permissionFlags; }
    int GetAnnouncementFlags();
    bool SetAnnouncementFlags(int flags);

  private:
    int m_permissionFlags;
  };
};
