/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <boost/move/unique_ptr.hpp>

class CGraphicContext;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();

  static boost::movelib::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Access render system interface
  virtual CGraphicContext& GetGfxContext() const;

protected:
  boost::movelib::unique_ptr<CGraphicContext> m_gfxContext;
};
