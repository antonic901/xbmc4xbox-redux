/*
 *  Copyright (C) 2023-2026 Team Xodi
 *  This file is part of Kodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DynamicDll.h"

#include "taglib/XBOXbuild/xbox/taglib_xbmc.h"

class DllTagLib : public DllDynamic
{
  DECLARE_DLL_WRAPPER(DllTagLib, DLL_PATH_LIBTAG)
  DEFINE_METHOD_LINKAGE0(unsigned int, XBMC_TAGLIB_CALL, xbmc_taglib_version)
  DEFINE_METHOD_LINKAGE3(int, XBMC_TAGLIB_CALL, xbmc_taglib_read, (const XbmcTagLibRequest* p1, char* p2, unsigned int p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(xbmc_taglib_version)
    RESOLVE_METHOD(xbmc_taglib_read)
  END_METHOD_RESOLVE()
};
