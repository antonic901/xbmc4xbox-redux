/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "video/jobs/VideoLibraryProgressJob.h"

#include <boost/shared_ptr.hpp>
#include <string>

class CFileItem;

/*!
 \brief Video library job implementation for refreshing a single item.
*/
class CVideoLibraryRefreshingJob : public CVideoLibraryProgressJob
{
public:
  /*!
   \brief Creates a new video library cleaning job for the given paths.

   \param[inout] item Video item to be refreshed
   \param[in] forceRefresh Whether to force a complete refresh (including NFO or internet lookup)
   \param[in] refreshAll Whether to refresh all sub-items (in case of a tvshow)
   \param[in] ignoreNfo Whether or not to ignore local NFO files
   \param[in] searchTitle Title to use for the search (instead of determining it from the item's filename/path)
  */
  CVideoLibraryRefreshingJob(boost::shared_ptr<CFileItem> item,
                             bool forceRefresh,
                             bool refreshAll,
                             bool ignoreNfo = false,
                             const std::string& searchTitle = "");

  virtual ~CVideoLibraryRefreshingJob();

  // specialization of CJob
  virtual const char *GetType() const { return "VideoLibraryRefreshingJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CVideoLibraryJob
  virtual bool Work(CVideoDatabase &db);

private:
  boost::shared_ptr<CFileItem> m_item;
  bool m_forceRefresh;
  bool m_refreshAll;
  bool m_ignoreNfo;
  std::string m_searchTitle;
};
