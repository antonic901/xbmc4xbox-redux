/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemXbox.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "XBVideoConfig.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

static void WaitCallback(DWORD flags)
{
#ifndef PROFILE
  // if cpu is far ahead of gpu, sleep instead of yield
  if (flags & D3DWAIT_PRESENT)
    while (D3DDevice::GetPushDistance(D3DDISTANCE_FENCES_TOWAIT) > 0)
      Sleep(1);
  else if (flags & (D3DWAIT_OBJECTLOCK | D3DWAIT_BLOCKONFENCE | D3DWAIT_BLOCKUNTILIDLE))
    while (D3DDevice::GetPushDistance(D3DDISTANCE_FENCES_TOWAIT) > 1)
      Sleep(1);
#endif
}

CRenderSystemXbox::CRenderSystemXbox() : CRenderSystemBase()
{
  ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = 576;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_A8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.AutoDepthStencilFormat = D3DFMT_LIN_D16;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  m_pD3D = NULL;
  m_pd3dDevice = NULL;
  m_pBackBuffer = NULL;
}

CRenderSystemXbox::~CRenderSystemXbox() {}

bool CRenderSystemXbox::InitRenderSystem()
{
  // Create the Direct3D object
  if ((m_pD3D = Direct3DCreate8(D3D_SDK_VERSION)) == NULL)
  {
    CLog::Log(LOGFATAL, "%s: Unable to create Direct3D!", __FUNCTION__);
    return false;
  }

  // List available video modes
  g_videoConfig.GetModes(m_pD3D);

  // Init the present parameters with values that are supported
  RESOLUTION initialResolution = g_videoConfig.GetInitialMode(m_pD3D, &m_d3dpp);

  // TODO: CGraphicsContext refactor (next two lines)
  CServiceBroker::GetWinSystem()->GetGfxContext().SetD3DParameters(&m_d3dpp);
  CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(initialResolution, TRUE);

  #define D3DCREATE_MULTITHREADED 0

  if (FAILED(m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice)))
  {
    // try software vertex processing
    if (FAILED(m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice)))
    {
      // and slow as arse reference processing
      if (FAILED(m_pD3D->CreateDevice(0, D3DDEVTYPE_REF, NULL, D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice)))
      {
        CLog::Log(LOGFATAL, "%s: Could not create D3D device!", __FUNCTION__);
        CLog::Log(LOGFATAL, " width/height:(%ix%i)" , m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
        CLog::Log(LOGFATAL, " refreshrate:%i" , m_d3dpp.FullScreen_RefreshRateInHz);

        if (m_d3dpp.Flags & D3DPRESENTFLAG_WIDESCREEN)
          CLog::Log(LOGFATAL, " 16:9 widescreen");
        else
          CLog::Log(LOGFATAL, " 4:3");

        if (m_d3dpp.Flags & D3DPRESENTFLAG_INTERLACED)
          CLog::Log(LOGFATAL, " interlaced");
        if (m_d3dpp.Flags & D3DPRESENTFLAG_PROGRESSIVE)
          CLog::Log(LOGFATAL, " progressive");

        return false;
      }
    }
  }

  // TODO: CGraphicsContext refactor (next two lines)
  CServiceBroker::GetWinSystem()->GetGfxContext().SetD3DDevice(m_pd3dDevice);
  CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();

  m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
  m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
  CUtil::InitGamma();

  D3DDevice::SetWaitCallback(WaitCallback);

  m_bRenderCreated = true;
  return true;
}

bool CRenderSystemXbox::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return false;
}

bool CRenderSystemXbox::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  m_pd3dDevice->BeginScene();

  return true;
}

bool CRenderSystemXbox::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  m_pd3dDevice->EndScene();

  return true;
}

void CRenderSystemXbox::PresentRender(bool rendered, bool videoLayer)
{
  if (m_bRenderCreated)
    PresentRenderImpl(rendered);
}
