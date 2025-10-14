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

#include "ProgramLibraryQueue.h"

#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "threads/SingleLock.h"
#include "Util.h"
#include "programs/jobs/ProgramLibraryCleaningJob.h"
#include "programs/jobs/ProgramLibraryJob.h"
#include "programs/jobs/ProgramLibraryScanningJob.h"

CProgramLibraryQueue::CProgramLibraryQueue()
  : CJobQueue(false, 1, CJob::PRIORITY_LOW),
    m_jobs(),
    m_modal(false),
    m_cleaning(false)
{ }

CProgramLibraryQueue::~CProgramLibraryQueue()
{
  CSingleLock lock(m_critical);
  m_jobs.clear();
}

CProgramLibraryQueue& CProgramLibraryQueue::GetInstance()
{
  static CProgramLibraryQueue s_instance;
  return s_instance;
}

void CProgramLibraryQueue::ScanLibrary(const std::string& directory, bool showProgress /* = true */)
{
  AddJob(new CProgramLibraryScanningJob(directory, showProgress));
}

void CProgramLibraryQueue::CleanLibrary(const std::string& directory, bool showProgress /* = true */)
{
  AddJob(new CProgramLibraryCleaningJob(directory, showProgress));
}

void CProgramLibraryQueue::AddJob(CProgramLibraryJob *job)
{
  if (job == NULL)
    return;

  CSingleLock lock(m_critical);
  if (!CJobQueue::AddJob(job))
    return;

  // add the job to our list of queued/running jobs
  std::string jobType = job->GetType();
  ProgramLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt == m_jobs.end())
  {
    ProgramLibraryJobs jobs;
    jobs.insert(job);
    m_jobs.insert(std::make_pair(jobType, jobs));
  }
  else
    jobsIt->second.insert(job);
}

void CProgramLibraryQueue::Refresh()
{
  CUtil::DeleteProgramDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CProgramLibraryQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    if (QueueEmpty())
      Refresh();
  }

  {
    CSingleLock lock(m_critical);
    // remove the job from our list of queued/running jobs
    ProgramLibraryJobMap::iterator jobsIt = m_jobs.find(job->GetType());
    if (jobsIt != m_jobs.end())
      jobsIt->second.erase(static_cast<CProgramLibraryJob*>(job));
  }

  return CJobQueue::OnJobComplete(jobID, success, job);
}
