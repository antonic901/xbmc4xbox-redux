/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"

#include <stdint.h>
#include <string>

class CHTTPImageTransformationHandler : public IHTTPRequestHandler
{
public:
  CHTTPImageTransformationHandler();
  virtual ~CHTTPImageTransformationHandler();

  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) const { return new CHTTPImageTransformationHandler(request); }
  virtual bool CanHandleRequest(const HTTPRequest &request)const;

  virtual MHD_RESULT HandleRequest();

  virtual bool CanHandleRanges() const { return true; }
  virtual bool CanBeCached() const { return true; }
  virtual bool GetLastModifiedDate(CDateTime &lastModified) const;

  virtual HttpResponseRanges GetResponseData() const { return m_responseData; }

  // priority must be higher than the one of CHTTPImageHandler
  virtual int GetPriority() const { return 6; }

protected:
  explicit CHTTPImageTransformationHandler(const HTTPRequest &request);

private:
  std::string m_url;
  CDateTime m_lastModified;

  uint8_t* m_buffer;
  HttpResponseRanges m_responseData;
};
