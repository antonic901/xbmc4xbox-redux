/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemXbox.h"

#include "settings/DisplaySettings.h"
#include "windowing/GraphicContext.h"

boost::movelib::unique_ptr<CWinSystemBase> CWinSystemXbox::CreateWinSystem()
{
  return boost::movelib::unique_ptr<CWinSystemBase>(new CWinSystemXbox());
}

void CWinSystemXbox::PresentRenderImpl(bool rendered)
{
  m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

bool CWinSystemXbox::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  for (int i = 0; i <= RES_PAL60_16x9; ++i)
  {
    m_gfxContext->ResetScreenParameters(static_cast<RESOLUTION>(i));
    m_gfxContext->ResetOverscan(static_cast<RESOLUTION>(i), CDisplaySettings::GetInstance().GetResolutionInfo(static_cast<RESOLUTION>(i)).Overscan);
  }

  return true;
}
