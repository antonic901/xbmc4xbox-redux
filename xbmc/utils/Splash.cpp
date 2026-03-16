/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "Splash.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIFontManager.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"

using namespace XFILE;

CSplash::CSplash()
{
}

CSplash& CSplash::GetInstance()
{
  static CSplash instance;
  return instance;
}

void CSplash::Show(const std::string& message /* = "" */)
{
  if (!g_advancedSettings.m_splashImage && !(m_image || !message.empty()))
    return;

  if (!m_image)
  {
    std::string splashImage = "special://home/media/Splash.png";
    if (!XFILE::CFile::Exists(splashImage))
      splashImage = "special://xbmc/media/Splash.png";

    m_image = boost::movelib::unique_ptr<CGUIImage>(new CGUIImage(0, 0, 0, 0, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
        CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(), CTextureInfo(splashImage)));
    m_image->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().Lock();
#ifdef HAS_XBOX_D3D
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
#else
  CServiceBroker::GetWinSystem()->GetGfxContext().Clear();
#endif

  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(res, true);

  //render splash image
#ifdef HAS_XBOX_D3D
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->BeginScene();
#else
  g_Windowing.BeginRender();
#endif

  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();

  if (!message.empty())
  {
    if (!m_messageLayout)
    {
      CGUIFont *messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 20, FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_messageLayout = boost::movelib::unique_ptr<CGUITextLayout>(new CGUITextLayout(messageFont, true, 0));
    }

    if (m_messageLayout)
    {
      m_messageLayout->Update(message, 1150, false, true);
      float textWidth, textHeight;
      m_messageLayout->GetTextExtent(textWidth, textHeight);

      int width = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
      int height = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
      float y = height - textHeight - 100;
      m_messageLayout->RenderOutline(width/2, y, 0, 0xFF000000, XBFONT_CENTER_X, width);
    }
  }

  //show it on screen
#ifdef HAS_XBOX_D3D
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->BlockUntilVerticalBlank();
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#else
  g_Windowing.EndRender();
  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(true, false);
#endif
  CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
}
