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

#include "programs/jobs/ProgramLibraryJob.h"

/*!
 \brief Program library job implementation for cleaning the program library.
*/
class CProgramLibraryCleaningJob : public CProgramLibraryJob
{
public:
  /*!
   \brief Creates a new program library cleaning job for the given path.

   \param[in] path Directory to be cleared
   \param[in] showDialog Whether to show a modal dialog or not
  */
  CProgramLibraryCleaningJob(const std::string& directory, bool showProgress = true);
  virtual ~CProgramLibraryCleaningJob();

  // specialization of CJob
  virtual const char *GetType() const { return "ProgramLibraryCleaningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CProgramLibraryJob
  virtual bool Work(CProgramDatabase &db);

private:
  std::string m_directory;
  bool m_showProgress;
};
