/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystem.h"

#include "Util.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <boost/move/make_unique.hpp>

CRenderSystemBase::CRenderSystemBase()
{
  m_bRenderCreated = false;
  m_maxTextureSize = 2048;
}

CRenderSystemBase::~CRenderSystemBase() {}

void CRenderSystemBase::ShowSplash(const std::string& message)
{
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_splashImage && !(m_splashImage || !message.empty()))
    return;

  if (!m_splashImage)
  {
    m_splashImage = boost::movelib::make_unique<CGUIImage>(
        0, 0, .0f, .0f,
        static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()),
        static_cast<float>(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()),
        CTextureInfo("special://xbmc/media/Splash.png"));
    m_splashImage->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Clear();

  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(res, true);

  //render splash image
  BeginRender();

  m_splashImage->AllocResources();
  m_splashImage->Render();
  m_splashImage->FreeResources();

  if (!message.empty())
  {
    if (!m_splashMessageLayout)
    {
      CGUIFont *messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 20, FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_splashMessageLayout = boost::movelib::make_unique<CGUITextLayout>(messageFont, true, .0f);
    }

    if (m_splashMessageLayout)
    {
      m_splashMessageLayout->Update(message, 1150, false, true);
      float textWidth, textHeight;
      m_splashMessageLayout->GetTextExtent(textWidth, textHeight);

      int width = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
      int height = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
      float y = height - textHeight - 100;
      m_splashMessageLayout->RenderOutline(width/2, y, 0, 0xFF000000, XBFONT_CENTER_X, width);
    }
  }

  //show it on screen
  EndRender();
  CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(true, false);
}

