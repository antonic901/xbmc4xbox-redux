/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "addons/AddonVersion.h"
#include "utils/Digest.h"

#include <memory>
#include <string>
#include <vector>

namespace ADDON
{
class CAddonExtensions;

struct RepositoryDirInfo
{
  RepositoryDirInfo() : minversion(""), maxversion(""), checksumType(KODI::UTILITY::CDigest::Type::INVALID), hashType(KODI::UTILITY::CDigest::Type::INVALID) {}
  CAddonVersion minversion;
  CAddonVersion maxversion;
  std::string info;
  std::string checksum;
  KODI::UTILITY::CDigest::Type checksumType;
  std::string datadir;
  std::string artdir;
  KODI::UTILITY::CDigest::Type hashType;
};

typedef std::vector<RepositoryDirInfo> RepositoryDirList;

class CRepository : public CAddon
{
public:
  explicit CRepository(const AddonInfoPtr& addonInfo);

  enum FetchStatus
  {
    STATUS_OK,
    STATUS_NOT_MODIFIED,
    STATUS_ERROR
  };

  FetchStatus FetchIfChanged(const std::string& oldChecksum,
                             std::string& checksum,
                             std::vector<AddonInfoPtr>& addons,
                             int& recheckAfter) const;

  struct ResolveResult
  {
    std::string location;
    KODI::UTILITY::TypedDigest digest;
  };
  ResolveResult ResolvePathAndHash(AddonPtr const& addon) const;

  // Implementation of CAddon
  virtual void OnPostInstall(bool update, bool modal);

private:
  static bool FetchChecksum(const std::string& url,
                            std::string& checksum,
                            int& recheckAfter);
  static bool FetchIndex(const RepositoryDirInfo& repo,
                         std::string const& digest,
                         std::vector<AddonInfoPtr>& addons);

  static RepositoryDirInfo ParseDirConfiguration(const CAddonExtensions& configuration);

  RepositoryDirList m_dirs;
};

typedef boost::shared_ptr<CRepository> RepositoryPtr;
}

