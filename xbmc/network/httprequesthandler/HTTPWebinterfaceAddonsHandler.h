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
  virtual ~CHTTPWebinterfaceAddonsHandler() {}

  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPWebinterfaceAddonsHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request) const;

  virtual MHD_RESULT HandleRequest();

  virtual HttpResponseRanges GetResponseData() const;

  virtual int GetPriority() const { return 4; }

protected:
  explicit CHTTPWebinterfaceAddonsHandler(const HTTPRequest &request)
    : IHTTPRequestHandler(request)
  { }

private:
  std::string m_responseData;
  CHttpResponseRange m_responseRange;
};
