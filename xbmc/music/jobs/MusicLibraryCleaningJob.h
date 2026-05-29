/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/jobs/MusicLibraryProgressJob.h"

#include <set>

/*!
 \brief Music library job implementation for cleaning the video library.
*/
class CMusicLibraryCleaningJob : public CMusicLibraryProgressJob
{
public:
  /*!
   \brief Creates a new music library cleaning job.
   \param[in] progressDialog Progress dialog to be used to display the cleaning progress
  */
  CMusicLibraryCleaningJob(CGUIDialogProgress* progressDialog);
  virtual ~CMusicLibraryCleaningJob();

  // specialization of CJob
  virtual const char *GetType() const { return "MusicLibraryCleaningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CMusicLibraryJob
  virtual bool Work(CMusicDatabase &db);

private:

};
