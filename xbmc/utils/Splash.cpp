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
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName) : CThread("CSplash")
{
  m_ImageName = imageName;
  m_messageLayout = NULL;
  m_image = NULL;
  m_layoutWasLoading = false;
}


CSplash::~CSplash()
{
  Stop();
  delete m_image;
  delete m_messageLayout;
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Show()
{
  Show("");
}

void CSplash::Show(const CStdString& message)
{
  g_graphicsContext.Lock();
#ifdef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
#else
  g_graphicsContext.Clear();
#endif

  RESOLUTION_INFO res(1280,720,0);
  g_graphicsContext.SetRenderingResolution(res, true);
  if (!m_image)
  {
    m_image = new CGUIImage(0, 0, 0, 0, 1280, 720, m_ImageName);
    m_image->SetAspectRatio(CAspectRatio::AR_CENTER);
  }

  //render splash image
#ifndef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BeginScene();
#endif

  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();

  // render message
  if (!message.IsEmpty())
  {
    if (!m_layoutWasLoading)
    {
      // load arial font, white body, no shadow, size: 20, no additional styling
      CGUIFont *messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 20, FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_messageLayout = new CGUITextLayout(messageFont, true, 0);
      m_layoutWasLoading = true;
    }
    if (m_messageLayout)
    {
      m_messageLayout->Update(message, 1150, false, true);

      float textWidth, textHeight;
      m_messageLayout->GetTextExtent(textWidth, textHeight);
      // ideally place text in center of empty area below splash image
      float y = 540 + m_image->GetTextureHeight() / 4 - textHeight / 2;
      if (y + textHeight > 720) // make sure entire text is visible
        y = 720 - textHeight;

      m_messageLayout->RenderOutline(640, y, 0, 0xFF000000, XBFONT_CENTER_X, 1280);
    }
  }

  //show it on screen
#ifdef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#else
  g_graphicsContext.Get3DDevice()->EndScene();
  g_graphicsContext.Flip();
#endif
  g_graphicsContext.Unlock();
}

void CSplash::Process()
{
  Show();
}

bool CSplash::Start()
{
  if (m_ImageName.IsEmpty() || !CFile::Exists(m_ImageName))
  {
    CLog::Log(LOGDEBUG, "Splash image %s not found", m_ImageName.c_str());
    return false;
  }
  Create();
  return true;
}

void CSplash::Stop()
{
  StopThread();
}
