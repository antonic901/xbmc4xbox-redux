/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace ADDONS
{
class CScreenSaver;
} // namespace ADDONS
} // namespace KODI

class CGUIWindowScreensaver : public CGUIDialog
{
public:
  CGUIWindowScreensaver();

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action)
  {
    // We're just a screen saver, nothing to do here
    return false;
  }
  virtual void Render();
  virtual void Process(unsigned int currentTime, CDirtyRegionList& regions);

protected:
  virtual void UpdateVisibility();
  virtual void OnInitWindow();

private:
  boost::movelib::unique_ptr<KODI::ADDONS::CScreenSaver> m_addon;
  bool m_visible;
};
