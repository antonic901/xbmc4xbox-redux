/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/infoscanner/MusicInfoScanner.h"
#include "music/jobs/MusicLibraryJob.h"

#include <string>

/*!
 \brief Music library job implementation for scanning items.
 Uses CMusicInfoScanner for scanning and can be run with  or
 without a visible progress bar.
 */
class CMusicLibraryScanningJob : public CMusicLibraryJob
{
public:
  /*!
   \brief Creates a new music library tag scanning and data scraping job.
   \param[in] directory Directory to be scanned for new items
   \param[in] flags What kind of scan to do
   \param[in] showProgress Whether to show a progress bar or not
   */
  CMusicLibraryScanningJob(const std::string& directory, int flags, bool showProgress = true);
  virtual ~CMusicLibraryScanningJob();

  // specialization of CMusicLibraryJob
  virtual bool CanBeCancelled() const { return true; }
  virtual bool Cancel();

  // specialization of CJob
  virtual const char *GetType() const { return "MusicLibraryScanningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CMusicLibraryJob
  virtual bool Work(CMusicDatabase &db);

private:
  MUSIC_INFO::CMusicInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
  int m_flags;
};
