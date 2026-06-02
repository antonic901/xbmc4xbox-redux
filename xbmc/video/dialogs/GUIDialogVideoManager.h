/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h" // VideoAssetType

#include <memory>

class CFileItem;
class CFileItemList;
class CMediaSource;

class CGUIDialogVideoManager : public CGUIDialog
{
public:
  explicit CGUIDialogVideoManager(int windowId);
  virtual ~CGUIDialogVideoManager() {}

  virtual void SetVideoAsset(const boost::shared_ptr<CFileItem>& item);
  virtual void SetSelectedVideoAsset(const boost::shared_ptr<CFileItem>& asset);
  virtual bool HasUpdatedItems() const { return m_hasUpdatedItems; }

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  virtual VideoAssetType::Type GetVideoAssetType() = 0;
  virtual int GetHeadingId() = 0;

  virtual void Clear();
  virtual void Refresh();
  virtual void UpdateButtons();
  virtual void UpdateAssetsList();

  virtual void Play();
  virtual void Remove();
  virtual void Rename();
  virtual void ChooseArt();

  void DisableRemove();
  void EnableRemove();

  void UpdateControls();

  static int ChooseVideoAsset(const boost::shared_ptr<CFileItem>& item,
                              VideoAssetType::Type assetType,
                              const std::string& defaultName);
  void AppendItemFolderToFileBrowserSources(std::vector<CMediaSource>& sources);
  void RefreshSelectedVideoAsset();

  CVideoDatabase m_database;
  boost::shared_ptr<CFileItem> m_videoAsset;
  boost::movelib::unique_ptr<CFileItemList> m_videoAssetsList;
  boost::shared_ptr<CFileItem> m_selectedVideoAsset;
  bool m_hasUpdatedItems;

private:
  CGUIDialogVideoManager();

  void CloseAll();
  bool UpdateSelectedAsset();
};
