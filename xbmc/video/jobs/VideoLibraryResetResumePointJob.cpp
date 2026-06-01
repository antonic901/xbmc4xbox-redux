/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryResetResumePointJob.h"

#include <vector>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/IDirectory.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

CVideoLibraryResetResumePointJob::CVideoLibraryResetResumePointJob(
    const boost::shared_ptr<CFileItem>& item)
  : m_item(item)
{
}

bool CVideoLibraryResetResumePointJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryResetResumePointJob* resetJob = dynamic_cast<const CVideoLibraryResetResumePointJob*>(job);
  if (!resetJob)
    return false;

  return m_item->IsSamePath(resetJob->m_item.get());
}

bool CVideoLibraryResetResumePointJob::Work(CVideoDatabase &db)
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (!profileManager->GetCurrentProfile().canWriteDatabases())
    return false;

  CFileItemList items;
  items.Add(boost::make_shared<CFileItem>(*m_item));

  if (m_item->m_bIsFolder)
    CUtil::GetRecursiveListing(m_item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);

  std::vector<CFileItemPtr> resetItems;
  for (int i = 0; i < items.Size(); ++i)
  {
#ifdef HAS_UPNP
    if (URIUtils::IsUPnP(items[i]->GetPath()) && UPNP::CUPnP::SaveFileState(*items[i], CBookmark(), false /* updatePlayCount */))
      continue;
#endif

    resetItems.push_back(items[i]);
  }

  if (resetItems.empty())
    return true;

  db.BeginTransaction();

  for (std::vector<CFileItemPtr>::const_iterator resetItem = resetItems.begin(); resetItem != resetItems.end(); ++resetItem)
  {
    db.DeleteResumeBookMark(**resetItem);
  }

  db.CommitTransaction();
  db.Close();

  return true;
}
