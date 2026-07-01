/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIVideoControl.h"

#include "GUIComponent.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "input/actions/ActionIDs.h"
#include "utils/ColorUtils.h"

using namespace KODI;

CGUIVideoControl::CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_VIDEO;
}

CGUIVideoControl::~CGUIVideoControl(void) {}

void CGUIVideoControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed. Just mark always for now.
  MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIVideoControl::Render()
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
#ifdef HAS_VIDEO_PLAYBACK
  // don't render if we aren't playing video, or if the renderer isn't started
  // (otherwise the lock we have from CApplication::Render() may clash with the startup
  // locks in the RenderManager.)
  if (appPlayer->IsPlayingVideo() && g_renderManager.IsStarted())
  {
#else
  if (appPlayer->IsPlayingVideo())
  {
#endif
    if (!appPlayer->IsPaused())
    {
      CApplicationComponents &appComponents = CServiceBroker::GetAppComponents();
      const boost::shared_ptr<CApplicationPowerHandling> appPower = appComponents.GetComponent<CApplicationPowerHandling>();
      appPower->ResetScreenSaver();
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewWindow(m_posX, m_posY, m_posX + m_width, m_posY + m_height);
    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewPort(m_posX, m_posY, m_width, m_height);

#ifdef HAS_VIDEO_PLAYBACK
    color_t alpha = CServiceBroker::GetWinSystem()->GetGfxContext().MergeAlpha(0xFF000000) >> 24;
    g_renderManager.RenderUpdate(false, 0, alpha);
#endif
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreViewPort();
  }
  CGUIControl::Render();
}

bool CGUIVideoControl::CanFocus() const
{ // unfocusable
  return false;
}

bool CGUIVideoControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}
