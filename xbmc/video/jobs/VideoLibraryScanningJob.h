/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/VideoInfoScanner.h"
#include "video/jobs/VideoLibraryJob.h"

#include <string>

/*!
 \brief Video library job implementation for scanning items.

 Uses CVideoInfoScanner for the whole filesystem scanning and can be run with
 or without a visible progress bar.
 */
class CVideoLibraryScanningJob : public CVideoLibraryJob
{
public:
  /*!
   \brief Creates a new video library scanning job.

   \param[in] directory Directory to be scanned for new items
   \param[in] scanAll Whether to scan all items or not
   \param[in] showProgress Whether to show a progress bar or not
   */
  CVideoLibraryScanningJob(const std::string& directory, bool scanAll = false, bool showProgress = true);
  virtual ~CVideoLibraryScanningJob();

  // specialization of CVideoLibraryJob
  virtual bool CanBeCancelled() const { return true; }
  virtual bool Cancel();

  // specialization of CJob
  virtual const char *GetType() const { return "VideoLibraryScanningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CVideoLibraryJob
  virtual bool Work(CVideoDatabase &db);

private:
  VIDEO::CVideoInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
  bool m_scanAll;
};
