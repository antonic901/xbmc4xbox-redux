/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowScreensaver.h"

#include "application/Application.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/ScreenSaver.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUITexture.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <boost/move/make_unique.hpp>

using namespace KODI;

CGUIWindowScreensaver::CGUIWindowScreensaver()
  : CGUIDialog(WINDOW_SCREENSAVER, "", MODELESS)
{
  m_renderOrder = RENDER_ORDER_WINDOW_SCREENSAVER;
  m_visible = false;
}

void CGUIWindowScreensaver::Process(unsigned int currentTime, CDirtyRegionList& regions)
{
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, regions);
  const CGraphicContext &context = CServiceBroker::GetWinSystem()->GetGfxContext();
  m_renderRegion.SetRect(0, 0, static_cast<float>(context.GetWidth()),
                         static_cast<float>(context.GetHeight()));
}

void CGUIWindowScreensaver::Render()
{
  // FIXME/TODO: Screensaver addons should make the screen black instead
  // keeping this just for compatibility reasons since it's now a dialog.
  CGUITexture::DrawQuad(m_renderRegion, UTILS::COLOR::BLACK);

  if (m_addon)
  {
    CGraphicContext &context = CServiceBroker::GetWinSystem()->GetGfxContext();

#ifdef HAS_XBOX_D3D
    if (m_addon->ID() == "screensaver.cpblobs" || m_addon->ID() == "screensaver.pmblobs" || m_addon->ID() == "screensaver.drempels")
      context.ApplyStateBlock();
    else
#endif
      context.CaptureStateBlock();
    m_addon->Render();
    context.ApplyStateBlock();
    return;
  }

  CGUIDialog::Render();
}

void CGUIWindowScreensaver::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  m_visible = true;
}

void CGUIWindowScreensaver::UpdateVisibility()
{
  if (!g_application.IsInScreenSaver() && m_visible)
  {
    m_visible = false;
    Close();
  }
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_addon)
      {
        m_addon->Stop();
        m_addon.reset();
      }

      CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);

#ifdef HAS_XBOX_D3D
      if (m_addon->ID() == "screensaver.cpblobs" || m_addon->ID() == "screensaver.pmblobs" || m_addon->ID() == "screensaver.drempels")
        CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
      else
#endif
        CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();

      const std::string addon = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_SCREENSAVER_MODE);
      const ADDON::AddonInfoPtr addonBase =
          CServiceBroker::GetAddonMgr().GetAddonInfo(addon, ADDON::AddonType::SCREENSAVER);
      if (!addonBase)
        return false;
      m_addon = boost::movelib::make_unique<KODI::ADDONS::CScreenSaver>(addonBase);
      return m_addon->Start();
    }

    case GUI_MSG_CHECK_LOCK:
    {
      if (!g_passwordManager.IsProfileLockUnlocked())
      {
        g_application.m_iScreenSaveLock = -1;
        return false;
      }
      g_application.m_iScreenSaveLock = 1;
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}
