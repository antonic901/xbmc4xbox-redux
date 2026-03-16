/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIComponent.h"

#include "GUIAudioManager.h"
// #include "GUIColorManager.h"
// #include "GUIInfoManager.h"
// #include "GUILargeTextureManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "ServiceBroker.h"
// #include "TextureManager.h"
#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"

#include <boost/move/make_unique.hpp>

CGUIComponent::CGUIComponent()
  : m_pWindowManager(boost::movelib::make_unique<CGUIWindowManager>()),
    // m_pTextureManager(std::make_unique<CGUITextureManager>()),
    // m_pLargeTextureManager(std::make_unique<CGUILargeTextureManager>()),
    // m_guiInfoManager(std::make_unique<CGUIInfoManager>()),
    // m_guiColorManager(std::make_unique<CGUIColorManager>()),
    m_guiAudioManager(boost::movelib::make_unique<CGUIAudioManager>())
{
}

CGUIComponent::~CGUIComponent()
{
  Deinit();
}

void CGUIComponent::Init()
{
  m_pWindowManager->Initialize();
  // m_guiInfoManager->Initialize();

  CServiceBroker::RegisterGUI(this);
}

void CGUIComponent::Deinit()
{
  CServiceBroker::UnregisterGUI();

  m_pWindowManager->DeInitialize();
}

CGUIWindowManager& CGUIComponent::GetWindowManager()
{
  return *m_pWindowManager;
}

// CGUITextureManager& CGUIComponent::GetTextureManager()
// {
//   return *m_pTextureManager;
// }

// CGUILargeTextureManager& CGUIComponent::GetLargeTextureManager()
// {
//   return *m_pLargeTextureManager;
// }

// CGUIInfoManager &CGUIComponent::GetInfoManager()
// {
//   return *m_guiInfoManager;
// }

// CGUIColorManager &CGUIComponent::GetColorManager()
// {
//   return *m_guiColorManager;
// }

CGUIAudioManager &CGUIComponent::GetAudioManager()
{
  return *m_guiAudioManager;
}

bool CGUIComponent::ConfirmDelete(const std::string& path)
{
  CGUIDialogYesNo* pDialog = dynamic_cast<CGUIDialogYesNo*>(GetWindowManager().GetWindow(WINDOW_DIALOG_YES_NO));
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CURL(path).GetWithoutUserDetails());
    pDialog->SetLine(2, "");
    pDialog->Open();
    if (pDialog->IsConfirmed())
      return true;
  }
  return false;
}
