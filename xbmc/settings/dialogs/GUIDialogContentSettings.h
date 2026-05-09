/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Scraper.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <map>
#include <utility>

namespace VIDEO
{
  struct SScanSettings;
}
class CFileItemList;

class CGUIDialogContentSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogContentSettings();

  // specialization of CGUIWindow
  virtual bool HasListItems() const { return true; }

  CONTENT_TYPE GetContent() const { return m_content; }
  void SetContent(CONTENT_TYPE content);
  void ResetContent();

  const ADDON::ScraperPtr& GetScraper() const { return m_scraper; }
  void SetScraper(ADDON::ScraperPtr scraper) { m_scraper = boost::move(scraper); }

  void SetScanSettings(const VIDEO::SScanSettings &scanSettings);
  bool GetScanRecursive() const { return m_scanRecursive; }
  bool GetUseDirectoryNames() const { return m_useDirectoryNames; }
  bool GetContainsSingleItem() const { return m_containsSingleItem; }
  bool GetExclude() const { return m_exclude; }
  bool GetNoUpdating() const { return m_noUpdating; }

  static bool Show(ADDON::ScraperPtr& scraper, CONTENT_TYPE content = CONTENT_NONE);
  static bool Show(ADDON::ScraperPtr& scraper, VIDEO::SScanSettings& settings, CONTENT_TYPE content = CONTENT_NONE);

protected:
  // specializations of CGUIWindow
  virtual void OnInitWindow();

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

private:
  void SetLabel2(const std::string &settingid, const std::string &label);
  void ToggleState(const std::string &settingid, bool enabled);
  using CGUIDialogSettingsManualBase::SetFocus;
  void SetFocusToSetting(const std::string& settingid);

  /*!
  * @brief The currently selected content type
  */
  CONTENT_TYPE m_content;
  /*!
  * @brief The selected content type at dialog creation
  */
  CONTENT_TYPE m_originalContent;
  /*!
  * @brief The currently selected scraper
  */
  ADDON::ScraperPtr m_scraper;

  bool m_showScanSettings;
  bool m_scanRecursive;
  bool m_useDirectoryNames;
  bool m_containsSingleItem;
  bool m_exclude;
  bool m_noUpdating;
};
