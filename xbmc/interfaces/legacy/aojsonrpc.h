/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef HAS_JSONRPC
#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPC.h"

class CVariant;

class CAddOnTransport : public JSONRPC::ITransportLayer
{
public:
  virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) { return false; }
  virtual bool Download(const char *path, CVariant& result) { return false; }
  virtual int GetCapabilities() { return JSONRPC::Response; }

  class CAddOnClient : public JSONRPC::IClient
  {
  public:
    virtual int  GetPermissionFlags() { return JSONRPC::OPERATION_PERMISSION_ALL; }
    virtual int  GetAnnouncementFlags() { return 0; }
    virtual bool SetAnnouncementFlags(int flags) { return true; }
  };
};
#endif
