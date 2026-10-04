/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/python/PythonInvoker.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

#include <string>

class CHTTPPythonInvoker : public CPythonInvoker
{
public:
  virtual ~CHTTPPythonInvoker();

  virtual HTTPPythonRequest* GetRequest() = 0;

protected:
  CHTTPPythonInvoker(ILanguageInvocationHandler* invocationHandler, HTTPPythonRequest* request);

  // overrides of CPythonInvoker
  virtual void onAbort();
  virtual void onError(const std::string& exceptionType = "", const std::string& exceptionValue = "", const std::string& exceptionTraceback = "");

  HTTPPythonRequest* m_request;
  bool m_internalError;
};
