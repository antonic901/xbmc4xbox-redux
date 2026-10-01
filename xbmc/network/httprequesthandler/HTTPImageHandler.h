/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/httprequesthandler/HTTPFileHandler.h"

#include <string>

class CHTTPImageHandler : public CHTTPFileHandler
{
public:
  CHTTPImageHandler() {}
  ~CHTTPImageHandler() {}

  IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPImageHandler(request); }
  bool CanHandleRequest(const HTTPRequest &request) const;

  int GetPriority() const { return 5; }
  int GetMaximumAgeForCaching() const { return 60 * 60 * 24 * 7; }

protected:
  explicit CHTTPImageHandler(const HTTPRequest &request);
};
