/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <boost/move/unique_ptr.hpp>
#include <string>

class CGUIWindowManager;
// class CGUITextureManager;
// class CGUILargeTextureManager;
// class CGUIInfoManager;
// class CGUIColorManager;
class CGUIAudioManager;

class CGUIComponent
{
public:
  CGUIComponent();
  virtual ~CGUIComponent();
  void Init();
  void Deinit();

  CGUIWindowManager& GetWindowManager();
  // CGUITextureManager& GetTextureManager();
  // CGUILargeTextureManager& GetLargeTextureManager();
  // CGUIInfoManager &GetInfoManager();
  // CGUIColorManager &GetColorManager();
  CGUIAudioManager &GetAudioManager();

  bool ConfirmDelete(const std::string& path);

protected:
  // members are pointers in order to avoid includes
  boost::movelib::unique_ptr<CGUIWindowManager> m_pWindowManager;
  // boost::movelib::unique_ptr<CGUITextureManager> m_pTextureManager;
  // boost::movelib::unique_ptr<CGUILargeTextureManager> m_pLargeTextureManager;
  // boost::movelib::unique_ptr<CGUIInfoManager> m_guiInfoManager;
  // boost::movelib::unique_ptr<CGUIColorManager> m_guiColorManager;
  boost::movelib::unique_ptr<CGUIAudioManager> m_guiAudioManager;
};
