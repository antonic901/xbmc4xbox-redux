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
#include "filesystem/File.h"
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName) : CThread("CSplash")
{
  m_ImageName = imageName;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Show()
{
  g_graphicsContext.Lock();
#ifdef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
#else
  g_graphicsContext.Clear();
#endif

  RESOLUTION_INFO res(1280,720,0);
  g_graphicsContext.SetRenderingResolution(res, true);
  CGUIImage* image = new CGUIImage(0, 0, 0, 0, 1280, 720, m_ImageName);
  image->SetAspectRatio(CAspectRatio::AR_CENTER);
  image->AllocResources();

  //render splash image
#ifndef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BeginScene();
#endif

  image->Render();
  image->FreeResources();
  delete image;

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
