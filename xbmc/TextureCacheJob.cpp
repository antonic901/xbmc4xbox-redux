/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCacheJob.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "commons/ilog.h"
#include "filesystem/File.h"
#include "guilib/Texture.h"
#include "imagefiles/SpecialImageLoaderFactory.h"
#include "pictures/Picture.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <utility>

#include "PlatformDefs.h"

CTextureCacheJob::CTextureCacheJob(const std::string &url, const std::string &oldHash):
  m_url(url),
  m_oldHash(oldHash),
  m_cachePath(CTextureCache::GetCacheFile(m_url))
{
}

CTextureCacheJob::~CTextureCacheJob() {}

bool CTextureCacheJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureCacheJob* cacheJob = dynamic_cast<const CTextureCacheJob*>(job);
    if (cacheJob && cacheJob->m_cachePath == m_cachePath)
      return true;
  }
  return false;
}

bool CTextureCacheJob::DoWork()
{
  if (ShouldCancel(0, 0))
    return false;

  // check whether we need cache the job anyway
  bool needsRecaching = false;
  std::string path(CServiceBroker::GetTextureCache()->CheckCachedImage(m_url, needsRecaching));
  if (!path.empty() && !needsRecaching)
    return false;
  if (CServiceBroker::GetTextureCache()->StartCacheImage(m_url))
    return CacheTexture();

  return false;
}

namespace
{
// Most PVR images use "additional_info" to signify 'ownership' of basic images for easy
// cache cleaning, rather than special generated images
bool IsPVROwnedImage(const std::string& additional_info)
{
  return additional_info == "pvrchannel_radio" || additional_info == "pvrchannel_tv" ||
         additional_info == "pvrprovider" || additional_info == "pvrrecording" ||
         StringUtils::StartsWith(additional_info, "epgtag_");
}

// DecodeImageURL can also set "additional_info" to 'flipped' for mirror images selected in
// the GUI, so is not a special generated image
bool IsControl(const std::string& additional_info)
{
  return additional_info == "flipped";
}

// special generated images and images served via HTTP should not be regularly checked for changes
bool ShouldCheckForChanges(const std::string& additional_info, const std::string& url)
{
  const bool isSpecialImage =
      !additional_info.empty() && !IsControl(additional_info) && !IsPVROwnedImage(additional_info);
  if (isSpecialImage)
    return false;

  const bool isHTTP =
      StringUtils::StartsWith(url, "http://") || StringUtils::StartsWith(url, "https://");
  return !isHTTP;
}
} // namespace

bool CTextureCacheJob::CacheTexture(boost::movelib::unique_ptr<CTexture>* out_texture)
{
  // unwrap the URL as required
  std::string additional_info;
  unsigned int width, height;
  std::string image = DecodeImageURL(m_url, width, height, additional_info);

  m_details.updateable = ShouldCheckForChanges(additional_info, image);

  if (m_details.updateable)
  {
    // generate the hash
    m_details.hash = GetImageHash(image);
    if (m_details.hash.empty())
      return false;

    if (m_details.hash == m_oldHash)
    {
      m_details.hashRevalidated = true;
      return true;
    }
  }

  boost::movelib::unique_ptr<CTexture> texture = LoadImage(image, width, height, additional_info, true);
  if (texture)
  {
    if (texture->HasAlpha())
      m_details.file = m_cachePath + ".png";
    else
      m_details.file = m_cachePath + ".jpg";

    CLog::Log(LOGDEBUG, "{} image '{}' to '{}':", m_oldHash.empty() ? "Caching" : "Recaching",
              CURL::GetRedacted(image), m_details.file);

    if (CPicture::CacheTexture(texture.get(), width, height,
                               CTextureCache::GetCachedPath(m_details.file)))
    {
      m_details.width = width;
      m_details.height = height;
      if (out_texture) // caller wants the texture
#ifdef _XBOX
      { // load cached image
        texture.reset();
        *out_texture = CTexture::LoadFromFile(CTextureCache::GetCachedPath(m_details.file), width, height, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("pictures.useexifrotation"));
      }
#else
        *out_texture = boost::move(texture);
#endif
      return true;
    }
  }
  return false;
}

std::string CTextureCacheJob::DecodeImageURL(const std::string &url, unsigned int &width, unsigned int &height, std::string &additional_info)
{
  // unwrap the URL as required
  std::string image(url);
  additional_info.clear();
  width = height = 0;
  if (StringUtils::StartsWith(url, "image://"))
  {
    // format is image://[type@]<url_encoded_path>?options
    CURL thumbURL(url);

    if (!CTextureCache::CanCacheImageURL(thumbURL))
      return "";
    if (thumbURL.GetUserName() == "music" || thumbURL.GetUserName() == "video" ||
        thumbURL.GetUserName() == "picturefolder")
      additional_info = thumbURL.GetUserName();
    if (StringUtils::StartsWith(thumbURL.GetUserName(), "video_"))
      additional_info = thumbURL.GetUserName();

    image = thumbURL.GetHostName();

    if (thumbURL.HasOption("flipped"))
      additional_info = "flipped";

    if (thumbURL.GetOption("size") == "thumb")
      width = height = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes;
    else
    {
      if (thumbURL.HasOption("width") && StringUtils::IsInteger(thumbURL.GetOption("width")))
        width = strtol(thumbURL.GetOption("width").c_str(), NULL, 0);
      if (thumbURL.HasOption("height") && StringUtils::IsInteger(thumbURL.GetOption("height")))
        height = strtol(thumbURL.GetOption("height").c_str(), NULL, 0);
    }
  }

  if (StringUtils::StartsWith(url, "chapter://"))
  {
    // workaround for chapter thumbnail paths, which don't yet conform to the image:// path.
    additional_info = "videochapter";
  }

  return image;
}

boost::movelib::unique_ptr<CTexture> CTextureCacheJob::LoadImage(const std::string& image,
                                                      unsigned int width,
                                                      unsigned int height,
                                                      const std::string& additional_info,
                                                      bool requirePixels)
{
  if (!additional_info.empty())
  {
    IMAGE_FILES::CSpecialImageLoaderFactory specialImageLoader;
    boost::movelib::unique_ptr<CTexture> texture = specialImageLoader.Load(additional_info, image, width, height);
    if (texture)
      return boost::move(texture);
  }

  // Validate file URL to see if it is an image
  CFileItem file(image, false);
  file.FillInMimeType();
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() ))
      && !StringUtils::StartsWithNoCase(file.GetMimeType(), "image/") && !StringUtils::EqualsNoCase(file.GetMimeType(), "application/octet-stream")) // ignore non-pictures
    return NULL;

  boost::movelib::unique_ptr<CTexture> texture =
      CTexture::LoadFromFile(image, width, height, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("pictures.useexifrotation"));
  if (!texture)
    return NULL;

  // EXIF bits are interpreted as: <flipXY><flipY*flipX><flipX>
  // where to undo the operation we apply them in reverse order <flipX>*<flipY*flipX>*<flipXY>
  // When flipped we have an additional <flipX> on the left, which is equivalent to toggling the last bit
  if (additional_info == "flipped")
    texture->SetOrientation(texture->GetOrientation() ^ 1);

  return boost::move(texture);
}

std::string CTextureCacheJob::GetImageHash(const std::string &url)
{
  // silently ignore - we cannot stat these
  // in the case of upnp thumbs are/should be provided when filling the directory list, there's no reason to stat all object ids
  if (URIUtils::IsProtocol(url, "addons") || URIUtils::IsProtocol(url, "plugin") ||
      URIUtils::IsProtocol(url, "upnp"))
    return "";

  struct __stat64 st;
  if (XFILE::CFile::Stat(url, &st) == 0)
  {
    int64_t time = st.st_mtime;
    if (!time)
      time = st.st_ctime;
    if (time || st.st_size)
      return StringUtils::Format("d{}s{}", time, st.st_size);

    // the image exists but we couldn't determine the mtime/ctime and/or size
    // so set an obviously bad hash
    return "BADHASH";
  }

  CLog::Log(LOGDEBUG, "{} - unable to stat url {}", __FUNCTION__, CURL::GetRedacted(url));
  return "";
}

CTextureUseCountJob::CTextureUseCountJob(const std::vector<CTextureDetails> &textures) : m_textures(textures)
{
}

bool CTextureUseCountJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureUseCountJob* useJob = dynamic_cast<const CTextureUseCountJob*>(job);
    if (useJob && useJob->m_textures == m_textures)
      return true;
  }
  return false;
}

bool CTextureUseCountJob::DoWork()
{
  CTextureDatabase db;
  if (db.Open())
  {
    db.BeginTransaction();
    for (std::vector<CTextureDetails>::const_iterator i = m_textures.begin(); i != m_textures.end(); ++i)
      db.IncrementUseCount(*i);
    db.CommitTransaction();
  }
  return true;
}
