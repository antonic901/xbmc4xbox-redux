/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Resolution.h"

#include <boost/move/unique_ptr.hpp>

class CGraphicContext;
class CRenderSystemBase;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();

  static boost::movelib::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Access render system interface
  virtual CRenderSystemBase *GetRenderSystem() { return NULL; }

  // windowing interfaces
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) = 0;
  virtual bool DestroyWindow() { return false; }

  // render loop
  void DriveRenderLoop();

  // winsystem events
  virtual bool MessagePump() { return false; }

  // Access render system interface
  virtual CGraphicContext& GetGfxContext() const;

protected:
  boost::movelib::unique_ptr<CGraphicContext> m_gfxContext;
};
