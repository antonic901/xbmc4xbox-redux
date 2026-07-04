/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/xbox/RenderSystemXbox.h"
#include "windowing/WinSystem.h"

class CWinSystemXbox : public CWinSystemBase, public CRenderSystemXbox
{
public:
  CWinSystemXbox() {};
  virtual ~CWinSystemXbox() {};

  static boost::movelib::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystemBase
  virtual CRenderSystemBase *GetRenderSystem() { return this; }
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res);

protected:
  // Implementation of CRenderSystemXbox
  virtual void PresentRenderImpl(bool rendered);
};
