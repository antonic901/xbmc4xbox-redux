/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/dialogs/GUIDialogVideoManager.h"

#include <memory>
#include <string>

class CFileItem;

class CGUIDialogVideoManagerExtras : public CGUIDialogVideoManager
{
public:
  CGUIDialogVideoManagerExtras();
  virtual ~CGUIDialogVideoManagerExtras() {}

  virtual void SetVideoAsset(const boost::shared_ptr<CFileItem>& item);
  /*!
   * \brief Open the Manage Extras dialog for a video
   * \param item video to manage
   * \return true: the video or another item was modified, a containing list should be refreshed.
   * false: no changes
   */
  static bool ManageVideoExtras(const boost::shared_ptr<CFileItem>& item);
  static std::string GenerateVideoExtra(const std::string& extrasRoot,
                                        const std::string& extrasPath);

protected:
  virtual bool OnMessage(CGUIMessage& message);

  virtual VideoAssetType::Type GetVideoAssetType();
  virtual int GetHeadingId() { return 40025; } // Extras:

  virtual void UpdateButtons();

private:
  /*!
   * \brief Add an extra to the video, using GUI user-provided information.
   * \return true if an extra was added, false otherwise.
   */
  bool AddVideoExtra();
  static std::string GenerateVideoExtra(const std::string& extrasPath);
};
