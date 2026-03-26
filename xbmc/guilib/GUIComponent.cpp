/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIComponent.h"

#include "GUIAudioManager.h"
#include "GUIColorManager.h"
#include "GUIInfoManager.h"
#include "GUILargeTextureManager.h"
#include "GUIWindowManager.h"
#include "ServiceBroker.h"
#include "TextureManager.h"
#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"

#include <memory>

CGUIComponent::CGUIComponent()
  : m_pWindowManager(boost::movelib::make_unique<CGUIWindowManager>()),
    m_pTextureManager(boost::movelib::make_unique<CGUITextureManager>()),
    m_pLargeTextureManager(boost::movelib::make_unique<CGUILargeTextureManager>()),
    m_guiInfoManager(boost::movelib::make_unique<CGUIInfoManager>()),
    m_guiColorManager(boost::movelib::make_unique<CGUIColorManager>()),
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
  m_guiInfoManager->Initialize();

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

CGUITextureManager& CGUIComponent::GetTextureManager()
{
  return *m_pTextureManager;
}

CGUILargeTextureManager& CGUIComponent::GetLargeTextureManager()
{
  return *m_pLargeTextureManager;
}

CGUIInfoManager &CGUIComponent::GetInfoManager()
{
  return *m_guiInfoManager;
}

CGUIColorManager &CGUIComponent::GetColorManager()
{
  return *m_guiColorManager;
}

CGUIAudioManager &CGUIComponent::GetAudioManager()
{
  return *m_guiAudioManager;
}

bool CGUIComponent::ConfirmDelete(const std::string& path)
{
  CGUIDialogYesNo* pDialog = GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(CVariant{122});
    pDialog->SetLine(0, CVariant{125});
    pDialog->SetLine(1, CVariant{CURL(path).GetWithoutUserDetails()});
    pDialog->SetLine(2, CVariant{""});
    pDialog->Open();
    if (pDialog->IsConfirmed())
      return true;
  }
  return false;
}
