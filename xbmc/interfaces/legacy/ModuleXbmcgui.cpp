/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ModuleXbmcgui.h"

#include "LanguageHook.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GraphicContext.h"

#define NOTIFICATION_INFO     "info"
#define NOTIFICATION_WARNING  "warning"
#define NOTIFICATION_ERROR    "error"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    long getCurrentWindowId()
    {
      DelayedCallGuard dg;
      CSingleLock gl(g_graphicsContext);
      return g_windowManager.GetActiveWindow();
    }

    long getCurrentWindowDialogId()
    {
      DelayedCallGuard dg;
      CSingleLock gl(g_graphicsContext);
      return g_windowManager.GetTopMostModalDialogID();
    }

    long getScreenHeight()
    {
      XBMC_TRACE;
      return g_graphicsContext.GetHeight();
    }

    long getScreenWidth()
    {
      XBMC_TRACE;
      return g_graphicsContext.GetWidth();
    }

    const char* getNOTIFICATION_INFO()    { return NOTIFICATION_INFO; }
    const char* getNOTIFICATION_WARNING() { return NOTIFICATION_WARNING; }
    const char* getNOTIFICATION_ERROR()   { return NOTIFICATION_ERROR; }

  }
}
