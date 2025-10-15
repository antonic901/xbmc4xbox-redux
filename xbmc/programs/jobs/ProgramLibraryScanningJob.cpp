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

#include "ProgramLibraryScanningJob.h"
#include "programs/ProgramDatabase.h"
#include "Util.h"

CProgramLibraryScanningJob::CProgramLibraryScanningJob(const std::string& directory, bool showProgress /* = true */)
  : m_scanner(),
    m_directory(directory),
    m_showProgress(showProgress)
{ }

CProgramLibraryScanningJob::~CProgramLibraryScanningJob()
{ }

bool CProgramLibraryScanningJob::Cancel()
{
  if (!m_scanner.IsScanning())
    return true;

  m_scanner.Stop();
  return true;
}

bool CProgramLibraryScanningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CProgramLibraryScanningJob* scanningJob = dynamic_cast<const CProgramLibraryScanningJob*>(job);
  if (scanningJob == NULL)
    return false;

  return m_directory == scanningJob->m_directory;
}

bool CProgramLibraryScanningJob::Work(CProgramDatabase &db)
{
  m_scanner.ShowDialog(m_showProgress);
  m_scanner.Start(m_directory);

  CUtil::DeleteProgramDatabaseDirectoryCache();
  return true;
}
