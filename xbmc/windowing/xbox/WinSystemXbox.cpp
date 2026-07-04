/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemXbox.h"

boost::movelib::unique_ptr<CWinSystemBase> CWinSystemXbox::CreateWinSystem()
{
  return boost::movelib::unique_ptr<CWinSystemBase>(new CWinSystemXbox());
}

void CWinSystemXbox::PresentRenderImpl(bool rendered)
{

}

bool CWinSystemXbox::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  return false;
}
