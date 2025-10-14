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

#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

class CProgramLibraryJob;

class CProgramLibraryQueue : protected CJobQueue
{
public:
  ~CProgramLibraryQueue();

  /*!
   \brief Gets the singleton instance of the program library queue.
  */
  static CProgramLibraryQueue& GetInstance();

  /*!
   \brief Enqueue a library scan job.

   \param[in] directory Directory to scan
   \param[in] scanAll Ignore exclude setting for items. Defaults to false
   \param[in] showProgress Whether or not to show a progress dialog. Defaults to true
   */
  void ScanLibrary(const std::string& directory, bool showProgress = true);

  void CleanLibrary(const std::string& directory, bool showProgress = true);

  /*!
   \brief Adds the given job to the queue.

   \param[in] job Program library job to be queued.
   */
  void AddJob(CProgramLibraryJob *job);

protected:
  // implementation of IJobCallback
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  /*!
   \brief Notifies all to refresh the current listings.
   */
  void Refresh();

private:
  CProgramLibraryQueue();
  CProgramLibraryQueue(const CProgramLibraryQueue&);
  CProgramLibraryQueue const& operator=(CProgramLibraryQueue const&);

  typedef std::set<CProgramLibraryJob*> ProgramLibraryJobs;
  typedef std::map<std::string, ProgramLibraryJobs> ProgramLibraryJobMap;
  ProgramLibraryJobMap m_jobs;
  CCriticalSection m_critical;

  bool m_modal;
  bool m_cleaning;
};
