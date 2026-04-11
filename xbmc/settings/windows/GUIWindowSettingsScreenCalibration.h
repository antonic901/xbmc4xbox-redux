/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <map>
#include <utility>
#include <vector>

class CGUIWindowSettingsScreenCalibration : public CGUIWindow
{
public:
  CGUIWindowSettingsScreenCalibration(void);
  virtual ~CGUIWindowSettingsScreenCalibration(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions);
  virtual void FrameMove();
  virtual void DoRender();
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);

protected:
  unsigned int FindCurrentResolution();
  void NextControl();
  void ResetControls();
  void EnableControl(int iControl);
  bool UpdateFromControl(int iControl);
  void ResetCalibration();
  unsigned int m_iCurRes;
  std::vector<RESOLUTION> m_Res;
  int m_iControl;
  float m_fPixelRatioBoxHeight;

private:
  std::map<int, std::pair<float, float>> m_controlsSize;
  int m_subtitlesHalfSpace{0};
  int m_subtitleVerticalMargin{0};
  bool m_isSubtitleBarEnabled{false};
};
