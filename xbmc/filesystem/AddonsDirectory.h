/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "IDirectory.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class CFileItem;
class CFileItemList;
class CURL;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

namespace ADDON
{
class IAddon;
typedef std::vector<boost::shared_ptr<IAddon> > VECADDONS;
} // namespace ADDON

namespace XFILE
{

  /*!
  \ingroup windows
  \brief Get access to shares and it's directories.
  */
  class CAddonsDirectory : public IDirectory
  {
  public:
    CAddonsDirectory(void);
    virtual ~CAddonsDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Create(const CURL& url) { return true; }
    virtual bool Exists(const CURL& url) { return true; }
    virtual bool AllowAll() const { return true; }

    /*! \brief Fetch script and plugin addons of a given content type
     \param content the content type to fetch
     \param addons the list of addons to fill with scripts and plugin content
     \return true if content is valid, false if it's invalid.
     */
    static bool GetScriptsAndPlugins(const std::string &content, ADDON::VECADDONS &addons);

    /*! \brief Fetch scripts and plugins of a given content type
     \param content the content type to fetch
     \param items the list to fill with scripts and content
     \return true if more than one item is found, false otherwise.
     */
    static bool GetScriptsAndPlugins(const std::string &content, CFileItemList &items);

    static void GenerateAddonListing(const CURL& path,
                                     const ADDON::VECADDONS& addons,
                                     CFileItemList& items,
                                     const std::string& label);
    static CFileItemPtr FileItemFromAddon(const boost::shared_ptr<ADDON::IAddon>& addon,
                                          const std::string& path,
                                          bool folder = false);

    /*! \brief Returns true if `path` is a path or subpath of the repository directory, otherwise false */
    static bool IsRepoDirectory(const CURL& path);

  private:
    bool GetSearchResults(const CURL& path, CFileItemList &items);
  };
}
