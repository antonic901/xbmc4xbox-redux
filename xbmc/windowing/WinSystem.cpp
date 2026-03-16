/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystem.h"

#include "windowing/GraphicContext.h"

#include <boost/move/make_unique.hpp>

CWinSystemBase::CWinSystemBase() : m_gfxContext(boost::movelib::make_unique<CGraphicContext>())
{
}

CWinSystemBase::~CWinSystemBase() {};

CGraphicContext& CWinSystemBase::GetGfxContext() const
{
  return *m_gfxContext;
}
