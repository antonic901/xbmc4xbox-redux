/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CurlFile.h"

namespace XFILE
{
  class CDAVFile : public CCurlFile
  {
  public:
    CDAVFile(void);
    virtual ~CDAVFile(void);

    virtual bool Execute(const CURL& url);

    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& url, const CURL& urlnew);

    virtual int GetLastResponseCode() { return m_lastResponseCode; }

  private:
    int m_lastResponseCode;
  };
}
