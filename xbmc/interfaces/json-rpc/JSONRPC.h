#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IClient.h"
#include "ITransportLayer.h"

#include "PlatformDefs.h"

#include <string>

namespace JSONRPC
{
  /*!
   \brief Permission categories for json rpc methods
   */
  enum OperationPermission
  {
    ReadData        =   0x1,
    ControlPlayback =   0x2,
    ControlNotify   =   0x4,
    ControlPower    =   0x8,
    UpdateData      =  0x10,
    RemoveData      =  0x20,
    Navigate        =  0x40,
    WriteFile       =  0x80
  };

  static const int OPERATION_PERMISSION_ALL = (ReadData | ControlPlayback | ControlNotify | ControlPower | UpdateData | RemoveData | Navigate | WriteFile);

  /*!
   \ingroup jsonrpc
   \brief JSON RPC handler

   Sets up and manages all needed information to process
   JSON-RPC requests and answering with the appropriate
   JSON-RPC response (actual response or error message).
   */
  class CJSONRPC
  {
  public:
    static void Initialize();
    static std::string MethodCall(const std::string &inputString, ITransportLayer *transport, IClient *client);
    static uint32_t GetInputKey();
  };
}
