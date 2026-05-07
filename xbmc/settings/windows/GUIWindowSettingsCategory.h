/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManagerBase.h"

class CSettings;

class CGUIWindowSettingsCategory : public CGUIDialogSettingsManagerBase
{
public:
  CGUIWindowSettingsCategory();
  virtual ~CGUIWindowSettingsCategory();

  // specialization of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual int GetID() const { return CGUIDialogSettingsManagerBase::GetID() + m_iSection; }

  // specialization of CGUIWindow
  virtual bool IsDialog() const { return false; }

protected:
  // specialization of CGUIWindow
  virtual void OnWindowLoaded();

  // implementation of CGUIDialogSettingsBase
  virtual int GetSettingLevel() const;
  virtual boost::shared_ptr<CSettingSection> GetSection();
  virtual bool Save();

  // implementation of CGUIDialogSettingsManagerBase
  virtual CSettingsManager* GetSettingsManager() const;

  /*!
   * Set focus to a category or setting in this window. The setting/category must be active in the
   * current level.
   */
  void FocusElement(const std::string& elementId);

  boost::shared_ptr<CSettings> m_settings;
  int m_iSection;
  bool m_returningFromSkinLoad; // true if we are returning from loading the skin
};
