/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/HTTPFileHandler.h"

#include <string>

class CHTTPVfsHandler : public CHTTPFileHandler
{
public:
  CHTTPVfsHandler() {}
  ~CHTTPVfsHandler() {}

  IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPVfsHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const;

  int GetPriority() const { return 5; }

protected:
  explicit CHTTPVfsHandler(const HTTPRequest &request);
};
