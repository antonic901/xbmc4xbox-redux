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

#include "GUILargeTextureManager.h"
#include "pictures/Picture.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "guilib/GraphicContext.h"
#include "TextureCache.h"

using namespace std;

CGUILargeTextureManager g_largeTextureManager;

CImageLoader::CImageLoader(const CStdString &path)
{
  m_path = path;
  m_texture = NULL;
  m_width = m_height = m_orientation = 0;
}

CImageLoader::~CImageLoader()
{
#ifdef HAS_XBOX_D3D
  SAFE_RELEASE(m_texture);
#else
  delete(m_texture);
#endif
}

bool CImageLoader::DoWork()
{
  CStdString texturePath = g_TextureManager.GetTexturePath(m_path);
  CStdString loadPath = CTextureCache::Get().CheckCachedImage(texturePath); 
  
  // If empty, then go on to validate and cache image as appropriate
  // If hit, continue down and load image
  if (loadPath.IsEmpty())
  {
    CFileItem file(m_path, false);

    // Validate file URL to see if it is an image
    if ((file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() )) 
       || file.GetMimeType().Left(6).Equals("image/")) // ignore non-pictures
    { 
      // Cache the image if necessary
      loadPath = CTextureCache::Get().CacheImageFile(texturePath);
      if (loadPath.IsEmpty())
        return false;
    }
    else
      return true;
  }
 
#ifdef HAS_XBOX_D3D
  int width = min(g_graphicsContext.GetWidth(), 1024);
  int height = min(g_graphicsContext.GetHeight(), 720);
  // if loading a .tbn that is not a fanart, try and load at requested thumbnail size as actual on disk
  // tbn might be larger due to libjpeg 1/8 - 8/8 scaling.
  CStdString directoryPath;
  URIUtils::GetDirectory(loadPath, directoryPath);
  URIUtils::RemoveSlashAtEnd(directoryPath);
  if (directoryPath != CProfilesManager::Get().GetVideoFanartFolder() &&
      directoryPath != CProfilesManager::Get().GetMusicFanartFolder() &&
      URIUtils::GetExtension(loadPath).Equals(".tbn"))
  {
    width = g_advancedSettings.m_thumbSize;
    height = g_advancedSettings.m_thumbSize;
  }

  CPicture pic;
  m_texture = pic.Load(loadPath, width, height);
  if (m_texture)
  {
    m_width = pic.GetWidth();
    m_height = pic.GetHeight();
    m_orientation = (CSettings::Get().GetBool("pictures.useexifrotation") && pic.GetExifInfo()->Orientation) ? pic.GetExifInfo()->Orientation - 1: 0;
  }
  else
    SAFE_RELEASE(m_texture);
#else
  m_texture = new CTexture();
  unsigned int start = XbmcThreads::SystemClockMillis();
  if (!m_texture->LoadFromFile(loadPath, min(g_graphicsContext.GetWidth(), 2048), min(g_graphicsContext.GetHeight(), 1080), g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    delete m_texture;
    m_texture = NULL;
  }
  else if (XbmcThreads::SystemClockMillis() - start > 100)
    CLog::Log(LOGDEBUG, "%s - took %u ms to load %s", __FUNCTION__, XbmcThreads::SystemClockMillis() - start, loadPath.c_str());
#endif

  return true;
}

CGUILargeTextureManager::CLargeTexture::CLargeTexture(const CStdString &path)
{
  m_path = path;
  m_orientation = 0;
  m_refCount = 1;
  m_timeToDelete = 0;
};

CGUILargeTextureManager::CLargeTexture::~CLargeTexture()
{
  assert(m_refCount == 0);
  m_texture.Free();
};

void CGUILargeTextureManager::CLargeTexture::AddRef()
{
  m_refCount++;
}

bool CGUILargeTextureManager::CLargeTexture::DecrRef(bool deleteImmediately)
{
  assert(m_refCount);
  m_refCount--;
  if (m_refCount == 0)
  {
    if (deleteImmediately)
      delete this;
    else
      m_timeToDelete = XbmcThreads::SystemClockMillis() + TIME_TO_DELETE;
    return true;
  }
  return false;
};

bool CGUILargeTextureManager::CLargeTexture::DeleteIfRequired()
{
  if (m_refCount == 0 && m_timeToDelete < XbmcThreads::SystemClockMillis())
  {
    delete this;
    return true;
  }
  return false;
};

void CGUILargeTextureManager::CLargeTexture::SetTexture(LPDIRECT3DTEXTURE8 texture, int width, int height, int orientation)
{
  assert(!m_texture.size());
  if (texture)
    m_texture.Set(texture, width, height);
  m_orientation = orientation;
};

CGUILargeTextureManager::CGUILargeTextureManager()
{
}

CGUILargeTextureManager::~CGUILargeTextureManager()
{
}

void CGUILargeTextureManager::CleanupUnusedImages()
{
  CSingleLock lock(m_listSection);
  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    if (image->DeleteIfRequired())
      it = m_allocated.erase(it);
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
bool CGUILargeTextureManager::GetImage(const CStdString &path, CTexture &texture, int &orientation, bool firstRequest)
{
  // note: max size to load images: 2048x1024? (8MB)
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (firstRequest)
        image->AddRef();
      orientation = image->GetOrientation();
      texture = image->GetTexture();
      return texture.size() > 0;
    }
  }

  if (firstRequest)
    QueueImage(path);

  return true;
}

void CGUILargeTextureManager::ReleaseImage(const CStdString &path, bool immediately)
{
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (image->DecrRef(immediately) && immediately)
        m_allocated.erase(it);
      return;
    }
  }
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    unsigned int id = it->first;
    CLargeTexture *image = it->second;
    if (image->GetPath() == path && image->DecrRef(true))
    {
      // cancel this job
      CJobManager::GetInstance().CancelJob(id);
      m_queued.erase(it);
      return;
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(const CStdString &path)
{
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = it->second;
    if (image->GetPath() == path)
    {
      image->AddRef();
      return; // already queued
    }
  }

  // queue the item
  CLargeTexture *image = new CLargeTexture(path);
  unsigned int jobID = CJobManager::GetInstance().AddJob(new CImageLoader(path), this, CJob::PRIORITY_NORMAL);
  m_queued.push_back(make_pair(jobID, image));
}
  
void CGUILargeTextureManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // see if we still have this job id
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    if (it->first == jobID)
    { // found our job
      CImageLoader *loader = (CImageLoader *)job;
      CLargeTexture *image = it->second;
      image->SetTexture(loader->m_texture, loader->m_width, loader->m_height, loader->m_orientation);
      loader->m_texture = NULL; // we want to keep the texture, and jobs are auto-deleted.
      m_queued.erase(it);
      m_allocated.push_back(image);
      return;
    }
  }
}

