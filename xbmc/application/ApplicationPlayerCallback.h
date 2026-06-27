/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayerCallback.h"
#include "threads/Event.h"

#include <memory>

class CApplicationStackHelper;
class CFileItem;

class CApplicationPlayerCallback : public IPlayerCallback
{
public:
  CApplicationPlayerCallback();

  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted(const CFileItem& file);
  virtual void OnPlayerCloseFile(const CFileItem& file, const CBookmark& bookmark);
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnPlayBackError();
  virtual void OnQueueNextItem();
  virtual void OnPlayBackSeek(int64_t iTime, int64_t seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnPlayBackSpeedChanged(int iSpeed);
  virtual void RequestVideoSettings(const CFileItem& fileItem);
  virtual void StoreVideoSettings(const CFileItem& fileItem, const CVideoSettings& vs);
};
