/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/xbmc.h"

void main()
{
  int status = XBMC_Run();
  if (status == -1)
  {
    // TODO: Initialize basic D3D and start FTP server
    // FatalErrorHandler and InitBasicD3D
  }
}
