/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MusicLibraryProgressJob.h"
#include "settings/LibExportSettings.h"

class CGUIDialogProgress;

/*!
 \brief Music library job implementation for exporting the music library.
*/
class CMusicLibraryExportJob : public CMusicLibraryProgressJob
{
public:
  /*!
  \brief Creates a new music library export job for the given paths.

  \param[in] settings       Library export settings
  \param[in] progressDialog Progress dialog to be used to display the export progress
  */
  CMusicLibraryExportJob(const CLibExportSettings& settings, CGUIDialogProgress* progressDialog);

  virtual ~CMusicLibraryExportJob();

  // specialization of CJob
  virtual const char *GetType() const { return "MusicLibraryExportJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CMusicLibraryJob
  virtual bool Work(CMusicDatabase &db);

private:
  CLibExportSettings m_settings;
};
