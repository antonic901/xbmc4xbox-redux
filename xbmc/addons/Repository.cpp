/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Repository.h"

#include <iterator>
#include <utility>
#include <boost/bind.hpp>

#include "ServiceBroker.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileItem.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/ZipFile.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "utils/Base64.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

bool dirHasParent(const ADDON::CRepository::DirInfo &dir, const std::string &path) { return URIUtils::PathHasParent(path, dir.datadir, true); }

CRepository::ResolveResult CRepository::ResolvePathAndHash(const AddonPtr& addon) const
{
  std::string const& path = addon->Path();

  ADDON::CRepository::DirList::const_iterator dirIt = std::find_if(m_dirs.begin(), m_dirs.end(), boost::bind(dirHasParent, _1, boost::cref(path)));
  if (dirIt == m_dirs.end())
  {
    CLog::Log(LOGERROR, "Requested path {} not found in known repository directories", path);
    ResolveResult resolveResult = {};
    return resolveResult;
  }

  if (!dirIt->hashes)
  {
    // We have a path, but need no hash
    ResolveResult resolveResult = {path, ""};
    return resolveResult;
  }

  // Do not follow mirror redirect, we want the headers of the redirect response
  CURL url(path);
  url.SetProtocolOption("redirect-limit", "0");
  CCurlFile file;
  if (!file.Open(url))
  {
    CLog::Log(LOGERROR, "Could not fetch addon location and hash from {}", path);
    ResolveResult resolveResult = {};
    return resolveResult;
  }

  // Return the location from the header so we don't have to look it up again
  // (saves one request per addon install)
  std::string location = file.GetRedirectURL();
  // content-* headers are base64, convert to base16
  std::string hash = StringUtils::ToHexadecimal(Base64::Decode(file.GetHttpHeader().GetValue("content-md5")));

  if (hash.empty())
  {
    // Expected hash, but none found -> fall back to old method
    if (!FetchChecksum(path + ".md5", hash))
    {
      CLog::Log(LOGERROR, "Failed to find hash for {} from HTTP header and in separate file", path);
      ResolveResult resolveResult = {};
      return resolveResult;
    }
  }
  if (location.empty())
  {
    // Fall back to original URL if we do not get a redirect
    location = path;
  }

  CLog::Log(LOGDEBUG, "Resolved addon path {} to {} hash {}", path, location, hash);

  ResolveResult resolveResult = {location, hash};
  return resolveResult;
}

CRepository::DirInfo CRepository::ParseDirConfiguration(cp_cfg_element_t* configuration)
{
  const ADDON::CAddonMgr &mgr = CServiceBroker::GetAddonMgr();
  DirInfo dir;
  dir.checksum = mgr.GetExtValue(configuration, "checksum");
  dir.info = mgr.GetExtValue(configuration, "info");
  dir.datadir = mgr.GetExtValue(configuration, "datadir");
  dir.artdir = mgr.GetExtValue(configuration, "artdir");
  if (dir.artdir.empty())
  {
    dir.artdir = dir.datadir;
  }
  dir.hashes = mgr.GetExtValue(configuration, "hashes") == "true";
  dir.version = AddonVersion(mgr.GetExtValue(configuration, "@minversion"));
  return dir;
}

boost::movelib::unique_ptr<CRepository> CRepository::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  DirList dirs;
  AddonVersion version("0.0.0");
  AddonPtr addonver;
  if (CServiceBroker::GetAddonMgr().GetAddon("xbmc.addon", addonver))
    version = addonver->Version();
  for (size_t i = 0; i < ext->configuration->num_children; ++i)
  {
    cp_cfg_element_t* element = &ext->configuration->children[i];
    if(element->name && strcmp(element->name, "dir") == 0)
    {
      DirInfo dir = ParseDirConfiguration(element);
      if (dir.version <= version)
      {
        dirs.push_back(boost::move(dir));
      }
    }
  }
  if (!CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, "info").empty())
  {
    dirs.push_back(ParseDirConfiguration(ext->configuration));
  }
  return boost::movelib::unique_ptr<CRepository>(new CRepository(boost::move(props), boost::move(dirs)));
}

CRepository::CRepository(AddonProps props, DirList dirs)
    : CAddon(boost::move(props)), m_dirs(boost::move(dirs))
{
  for (DirList::const_iterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
  {
    const ADDON::CRepository::DirInfo &dir = *it;
    if (CURL(dir.datadir).IsProtocol("http"))
    {
      CLog::Log(LOGWARNING, "Repository {} uses plain HTTP for add-on downloads - this is insecure and will make your Kodi installation vulnerable to attacks if enabled!", Name());
    }
  }
}

bool CRepository::FetchChecksum(const std::string& url, std::string& checksum)
{
  CFile file;
  if (!file.Open(url))
    return false;

  // we intentionally avoid using file.GetLength() for
  // Transfer-Encoding: chunked servers.
  std::stringstream ss;
  char temp[1024];
  int read;
  while ((read = file.Read(temp, sizeof(temp))) > 0)
    ss.write(temp, read);
  if (read <= -1)
    return false;
  checksum = ss.str();
  std::size_t pos = checksum.find_first_of(" \n");
  if (pos != std::string::npos)
  {
    checksum = checksum.substr(0, pos);
  }
  return true;
}

bool CRepository::FetchIndex(const DirInfo& repo, VECADDONS& addons)
{
  XFILE::CCurlFile http;
  http.SetAcceptEncoding("gzip");

  std::string response;
  if (!http.Get(repo.info, response))
  {
    CLog::Log(LOGERROR, "CRepository: failed to read %s", repo.info.c_str());
    return false;
  }

  if (URIUtils::HasExtension(repo.info, ".gz")
      || CMime::GetFileTypeFromMime(http.GetMimeType()) == CMime::EFileType::FileTypeGZip)
  {
    CLog::Log(LOGDEBUG, "CRepository '%s' is gzip. decompressing", repo.info.c_str());
    std::string buffer;
    if (!CZipFile::DecompressGzip(response, buffer))
    {
      CLog::Log(LOGERROR, "CRepository: failed to decompress gzip from '%s'", repo.info.c_str());
      return false;
    }
    response = boost::move(buffer);
  }

  return CServiceBroker::GetAddonMgr().AddonsFromRepoXML(repo, response, addons);
}

CRepository::FetchStatus CRepository::FetchIfChanged(const std::string& oldChecksum,
    std::string& checksum, VECADDONS& addons) const
{
  checksum = "";
  for (DirList::const_iterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
  {
    const ADDON::CRepository::DirInfo &dir = *it;
    if (!dir.checksum.empty())
    {
      std::string part;
      if (!FetchChecksum(dir.checksum, part))
      {
        CLog::Log(LOGERROR, "CRepository: failed read '%s'", dir.checksum.c_str());
        return STATUS_ERROR;
      }
      checksum += part;
    }
  }

  if (oldChecksum == checksum && !oldChecksum.empty())
    return STATUS_NOT_MODIFIED;

  for (DirList::const_iterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
  {
    const ADDON::CRepository::DirInfo &dir = *it;
    VECADDONS tmp;
    if (!FetchIndex(dir, tmp))
      return STATUS_ERROR;
    addons.insert(addons.end(), tmp.begin(), tmp.end());
  }
  return STATUS_OK;
}

CRepositoryUpdateJob::CRepositoryUpdateJob(const RepositoryPtr& repo) : m_repo(repo) {}

bool CRepositoryUpdateJob::DoWork()
{
  CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] checking for updates.", m_repo->ID().c_str());
  CAddonDatabase database;
  database.Open();

  std::string oldChecksum;
  if (database.GetRepoChecksum(m_repo->ID(), oldChecksum) == -1)
    oldChecksum = "";

  std::string newChecksum;
  VECADDONS addons;
  ADDON::CRepository::FetchStatus status = m_repo->FetchIfChanged(oldChecksum, newChecksum, addons);

  database.SetLastChecked(m_repo->ID(), m_repo->Version(),
      CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  MarkFinished();

  if (status == CRepository::STATUS_ERROR)
    return false;

  if (status == CRepository::STATUS_NOT_MODIFIED)
  {
    CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] checksum not changed.", m_repo->ID().c_str());
    return true;
  }

  //Invalidate art.
  {
    CTextureDatabase textureDB;
    textureDB.Open();
    textureDB.BeginMultipleExecute();

    for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
    {
      const ADDON::AddonPtr &addon = *it;
      AddonPtr oldAddon;
      if (database.GetAddon(addon->ID(), oldAddon) && addon->Version() > oldAddon->Version())
      {
        if (!oldAddon->Icon().empty() || !oldAddon->Art().empty() || !oldAddon->Screenshots().empty())
          CLog::Log(LOGDEBUG, "CRepository: invalidating cached art for '%s'", addon->ID().c_str());

        if (!oldAddon->Icon().empty())
          textureDB.InvalidateCachedTexture(oldAddon->Icon());

        std::vector<std::string> vecScreenshots = oldAddon->Screenshots();
        for (std::vector<std::string>::const_iterator it = vecScreenshots.begin(); it != vecScreenshots.end(); ++it)
          textureDB.InvalidateCachedTexture(*it);

        ArtMap mapArt = oldAddon->Art();
        for (ArtMap::const_iterator it = mapArt.begin(); it != mapArt.end(); ++it)
          textureDB.InvalidateCachedTexture(it->second);
      }
    }
    textureDB.CommitMultipleExecute();
  }

  database.UpdateRepositoryContent(m_repo->ID(), m_repo->Version(), newChecksum, addons);
  return true;
}
