/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TextureCache.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include "DDSImage.h"

using namespace XFILE;

CTextureCache::CDDSJob::CDDSJob(const CStdString &url, const CStdString &original)
{
  m_url = url;
  m_original = original;
  m_dds = URIUtils::AddFileToFolder("dds", URIUtils::ReplaceExtension(CTextureCache::GetCacheFile(url), ".dds"));
}

bool CTextureCache::CDDSJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CDDSJob* ddsJob = dynamic_cast<const CDDSJob*>(job);
    if (ddsJob && ddsJob->m_original == m_original)
      return true;
  }
  return false;
}

bool CTextureCache::CDDSJob::DoWork()
{
  // TODO: implement DDS compressing
  return false;
}

CTextureCache &CTextureCache::Get()
{
  static CTextureCache s_cache;
  return s_cache;
}

CTextureCache::CTextureCache()
{
}

CTextureCache::~CTextureCache()
{
}

void CTextureCache::Initialize()
{
  CSingleLock lock(m_databaseSection);
  if (!m_database.IsOpen())
    m_database.Open();
}

void CTextureCache::Deinitialize()
{
  CancelJobs();
  CSingleLock lock(m_databaseSection);
  m_database.Close();
}

CStdString CTextureCache::CheckAndCacheImage(const CStdString &url)
{
  if (URIUtils::GetExtension(url).Equals(".dds") || 0 == strncmp(url.c_str(), "special://skin/", 15))
    return url; // already cached

  // lookup the item in the database
  CStdString cacheFile, ddsFile;
  if (GetCachedTexture(url, cacheFile, ddsFile))
  {
    CStdString path = GetCachedPath(ddsFile);
    if (!ddsFile.IsEmpty() && ddsFile != "failed" && CFile::Exists(path))
      return path;
    path = GetCachedPath(cacheFile);
    if (CFile::Exists(path))
    {
      if (ddsFile != "failed")
        AddJob(new CDDSJob(url, cacheFile));
      return path;
    }
  }

  // Cache the texture
  CStdString originalFile = URIUtils::AddFileToFolder("original", GetCacheFile(url));
  CStdString originalURL = GetCachedPath(originalFile);

  CStdString hash = GetImageHash(url);
  if (!hash.IsEmpty() && CFile::Copy(url, originalURL))
  {
    AddCachedTexture(url, originalFile, "", hash);
    AddJob(new CDDSJob(url, originalFile));
    return originalURL;
  }

  return "";
}

void CTextureCache::ClearCachedImage(const CStdString &url)
{
  CStdString cachedFile, ddsFile;
  if (ClearCachedTexture(url, cachedFile, ddsFile))
  {
    CStdString path = GetCachedPath(ddsFile);
    if (!ddsFile.IsEmpty() && ddsFile != "failed" && CFile::Exists(path))
      CFile::Delete(path);
    path = GetCachedPath(cachedFile);
    if (CFile::Exists(path))
      CFile::Delete(path);
  }
}

bool CTextureCache::GetCachedTexture(const CStdString &url, CStdString &cachedURL, CStdString &ddsURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.GetCachedTexture(url, cachedURL, ddsURL);
}

bool CTextureCache::AddCachedTexture(const CStdString &url, const CStdString &cachedURL, const CStdString &ddsURL, const CStdString &hash)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, cachedURL, ddsURL, hash);
}

bool CTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL, CStdString &ddsURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL, ddsURL);
}

CStdString CTextureCache::GetImageHash(const CStdString &url) const
{
  // TODO: stat the image and grab ctime/mtime and size
  return "nohash";
}

CStdString CTextureCache::GetCacheFile(const CStdString &url)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("%c/%s%s", hex[0], hex.c_str(), URIUtils::GetExtension(url).c_str());
  return hash;
}

CStdString CTextureCache::GetCachedPath(const CStdString &file)
{
  return URIUtils::AddFileToFolder(CProfilesManager::Get().GetThumbnailsFolder(), file);
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CDDSJob *ddsJob = (CDDSJob *)job;
  if (success)
    AddCachedTexture(ddsJob->m_url, ddsJob->m_original, ddsJob->m_dds, "");
  else
    AddCachedTexture(ddsJob->m_url, ddsJob->m_original, "failed", "");
  return CJobQueue::OnJobComplete(jobID, success, job);
}
