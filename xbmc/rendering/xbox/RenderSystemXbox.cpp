/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemXbox.h"

CRenderSystemXbox::CRenderSystemXbox() : CRenderSystemBase()
{
}

CRenderSystemXbox::~CRenderSystemXbox() {}

bool CRenderSystemXbox::InitRenderSystem()
{
  return false;
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

  return false;
}

bool CRenderSystemXbox::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return false;
}

void CRenderSystemXbox::PresentRender(bool rendered, bool videoLayer)
{
  if (m_bRenderCreated)
    PresentRenderImpl(rendered);
}
