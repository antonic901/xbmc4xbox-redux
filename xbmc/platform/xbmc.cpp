/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "application/Application.h"

extern "C" int XBMC_Run()
{
  int status = -1;

  if (!g_application.Create())
  {
    return status;
  }

  if (!g_application.CreateGUI())
  {
    if (g_application.Stop(0))
      g_application.Cleanup();
    return status;
  }
  if (!g_application.Initialize())
  {
    return status;
  }

  status = g_application.Run();

  return status;
}
