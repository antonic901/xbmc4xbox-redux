/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <string>

class CHTTPWebinterfaceAddonsHandler : public IHTTPRequestHandler
{
public:
  CHTTPWebinterfaceAddonsHandler() {}
  ~CHTTPWebinterfaceAddonsHandler() {}

  IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPWebinterfaceAddonsHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const;

  MHD_RESULT HandleRequest();

  HttpResponseRanges GetResponseData() const;

  int GetPriority() const { return 4; }

protected:
  explicit CHTTPWebinterfaceAddonsHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

private:
  std::string m_responseData;
  CHttpResponseRange m_responseRange;
};
