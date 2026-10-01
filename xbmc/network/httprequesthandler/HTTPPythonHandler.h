/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/IAddon.h"
#include "addons/Webinterface.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

class CHTTPPythonHandler : public IHTTPRequestHandler
{
public:
  CHTTPPythonHandler();
  ~CHTTPPythonHandler() {}

  IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPPythonHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const;
  bool CanHandleRanges() const { return false; }
  bool CanBeCached() const { return false; }
  bool GetLastModifiedDate(CDateTime &lastModified) const;

  MHD_RESULT HandleRequest();

  HttpResponseRanges GetResponseData() const { return m_responseRanges; }

  std::string GetRedirectUrl() const { return m_redirectUrl; }

  int GetPriority() const { return 3; }

protected:
  explicit CHTTPPythonHandler(const HTTPRequest &request);

  bool appendPostData(const char *data, size_t size);

private:
  std::string m_scriptPath;
  ADDON::AddonPtr m_addon;
  CDateTime m_lastModified;

  std::string m_requestData;
  std::string m_responseData;
  HttpResponseRanges m_responseRanges;

  std::string m_redirectUrl;
};
