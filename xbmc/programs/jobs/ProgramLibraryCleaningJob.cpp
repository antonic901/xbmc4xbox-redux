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

#include "ProgramLibraryCleaningJob.h"

#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "programs/ProgramDatabase.h"
#include "Util.h"

CProgramLibraryCleaningJob::CProgramLibraryCleaningJob(const std::string& directory, bool showProgress /* = true */)
  : m_directory(directory),
    m_showProgress(showProgress)
{ }

CProgramLibraryCleaningJob::~CProgramLibraryCleaningJob()
{ }

bool CProgramLibraryCleaningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CProgramLibraryCleaningJob* scanningJob = dynamic_cast<const CProgramLibraryCleaningJob*>(job);
  if (scanningJob == NULL)
    return false;

  return m_directory == scanningJob->m_directory;
}

bool CProgramLibraryCleaningJob::Work(CProgramDatabase &db)
{
  CGUIDialogProgressBarHandle* handle = NULL;
  if (m_showProgress)
  {
    CGUIDialogExtendedProgressBar* dialog = static_cast<CGUIDialogExtendedProgressBar*>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
    if (dialog)
      handle = dialog->GetHandle(g_localizeStrings.Get(314));
  }

  if (handle)
  {
    handle->SetTitle(g_localizeStrings.Get(700));
    handle->SetText("");
  }

  db.RemoveContentForPath(m_directory);

  if (handle)
    handle->MarkFinished();
  handle = NULL;

  CUtil::DeleteProgramDatabaseDirectoryCache();
  return true;
}
