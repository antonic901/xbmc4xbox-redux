#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

#include "programs/ProgramInfoScanner.h"
#include "programs/jobs/ProgramLibraryJob.h"

/*!
 \brief Program library job implementation for scanning items.

 Uses CProgramInfoScanner for the whole filesystem scanning and can be run with
 or without a visible progress bar.
 */
class CProgramLibraryScanningJob : public CProgramLibraryJob
{
public:
  /*!
   \brief Creates a new program library scanning job.

   \param[in] directory Directory to be scanned for new items
   \param[in] scanAll Whether to scan all items or not
   \param[in] showProgress Whether to show a progress bar or not
   */
  CProgramLibraryScanningJob(const std::string& directory, bool showProgress = true);
  virtual ~CProgramLibraryScanningJob();

  // specialization of CProgramLibraryJob
  virtual bool CanBeCancelled() const { return true; }
  virtual bool Cancel();

  // specialization of CJob
  virtual const char *GetType() const { return "ProgramLibraryScanningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CProgramLibraryJob
  virtual bool Work(CProgramDatabase &db);

private:
  PROGRAM::CProgramInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
};
