/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureManager.h"

#include "ServiceBroker.h"
#include "Texture.h"
#include "URL.h"
#include "commons/ilog.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#if 0
#include "guilib/TextureBundle.h"
#endif
#include "guilib/TextureFormats.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <mutex>

#ifdef _DEBUG_TEXTURES
#include "utils/TimeUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#define WIN_SYSTEM_CLASS CWinSystemIOS
#include "windowing/ios/WinSystemIOS.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#elif defined(TARGET_DARWIN_TVOS)
#define WIN_SYSTEM_CLASS CWinSystemTVOS
#include "windowing/tvos/WinSystemTVOS.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#endif

#if defined(HAS_GL) || defined(HAS_GLES)
#include "system_gl.h"
#endif

#ifndef _XBOX
#include "FFmpegImage.h"
#endif

#include <algorithm>
#include <cassert>
#include <exception>

/************************************************************************/
/*                                                                      */
/************************************************************************/
CTextureArray::CTextureArray(int width, int height, int loops,  bool texCoordsArePixels)
{
  m_width = width;
  m_height = height;
  m_loops = loops;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
}

CTextureArray::CTextureArray()
{
  Reset();
}

CTextureArray::~CTextureArray() {}

unsigned int CTextureArray::size() const
{
  return m_textures.size();
}


void CTextureArray::Reset()
{
  m_textures.clear();
  m_delays.clear();
  m_width = 0;
  m_height = 0;
  m_loops = 0;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
}

void CTextureArray::Add(boost::shared_ptr<CTexture> texture, int delay)
{
  if (!texture)
    return;

  m_texWidth = texture->GetTextureWidth();
  m_texHeight = texture->GetTextureHeight();
  m_texCoordsArePixels = false;

  m_textures.emplace_back(std::move(texture));
  m_delays.push_back(delay);
}

void CTextureArray::Set(boost::shared_ptr<CTexture> texture, int width, int height)
{
  assert(!m_textures.size()); // don't try and set a texture if we already have one!
  m_width = width;
  m_height = height;
  m_orientation = texture ? texture->GetOrientation() : 0;
  Add(std::move(texture), 2);
}

void CTextureArray::Free()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  Reset();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

CTextureMap::CTextureMap()
{
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::CTextureMap(const std::string& textureName, int width, int height, int loops)
: m_texture(width, height, loops)
, m_textureName(textureName)
{
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::~CTextureMap()
{
  FreeTexture();
}

bool CTextureMap::Release()
{
  if (!m_texture.m_textures.size())
    return true;
  if (!m_referenceCount)
    return true;

  m_referenceCount--;
  if (!m_referenceCount)
  {
    return true;
  }
  return false;
}

const std::string& CTextureMap::GetName() const
{
  return m_textureName;
}

const CTextureArray& CTextureMap::GetTexture()
{
  m_referenceCount++;
  return m_texture;
}

void CTextureMap::Dump() const
{
  if (!m_referenceCount)
    return;   // nothing to see here

  CLog::Log(LOGDEBUG, "{0}: texture:{1} has {2} frames {3} refcount", __FUNCTION__, m_textureName,
            m_texture.m_textures.size(), m_referenceCount);
}

unsigned int CTextureMap::GetMemoryUsage() const
{
  return m_memUsage;
}

void CTextureMap::Flush()
{
  if (!m_referenceCount)
    FreeTexture();
}


void CTextureMap::FreeTexture()
{
  m_texture.Free();
}

void CTextureMap::SetHeight(int height)
{
  m_texture.m_height = height;
}

void CTextureMap::SetWidth(int height)
{
  m_texture.m_width = height;
}

bool CTextureMap::IsEmpty() const
{
  return m_texture.m_textures.empty();
}

void CTextureMap::Add(boost::movelib::unique_ptr<CTexture> texture, int delay)
{
  if (texture)
#ifdef HAS_XBOX_D3D
    m_memUsage += sizeof(CTexture) + (texture->GetPitch() * texture->GetRows()); // This is equal to D3DSURFACE_DESC.Size
#else
    m_memUsage += sizeof(CTexture) + (texture->GetTextureWidth() * texture->GetTextureHeight() * 4);
#endif

  m_texture.Add(std::move(texture), delay);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
CGUITextureManager::CGUITextureManager(void)
{
#if 0
#ifdef HAS_XBOX_D3D
  D3DXSetDXT3DXT5(TRUE);
  for (int bundle = 0; bundle < 2; bundle++)
    m_iNextPreload[bundle] = m_PreLoadNames[bundle].end();
#endif
  // we set the theme bundle to be the first bundle (thus prioritizing it)
  m_TexBundle[0].SetThemeBundle(true);
#endif
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}

#if 0
#ifdef HAS_XBOX_D3D
void CGUITextureManager::StartPreLoad()
{
  for (int bundle = 0; bundle < 2; bundle++)
    m_PreLoadNames[bundle].clear();
}

void CGUITextureManager::PreLoad(const std::string& strTextureName)
{
  if (strTextureName.c_str()[1] == ':' || strTextureName == "-")
    return ;

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
      return ;
  }

  for (int bundle = 0; bundle < 2; bundle++)
  {
    for (std::list<std::string>::iterator i = m_PreLoadNames[bundle].begin(); i != m_PreLoadNames[bundle].end(); ++i)
    {
      if (*i == strTextureName)
        return ;
    }

    if (m_TexBundle[bundle].HasFile(strTextureName))
    {
      m_PreLoadNames[bundle].push_back(strTextureName);
      return;
    }
  }
}

void CGUITextureManager::EndPreLoad()
{
  for (int i = 0; i < 2; i++)
  {
    m_iNextPreload[i] = m_PreLoadNames[i].begin();
    // preload next file
    if (m_iNextPreload[i] != m_PreLoadNames[i].end())
      m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
  }
}

void CGUITextureManager::FlushPreLoad()
{
  for (int i = 0; i < 2; i++)
  {
    m_PreLoadNames[i].clear();
    m_iNextPreload[i] = m_PreLoadNames[i].end();
  }
}
#endif
#endif

/************************************************************************/
/*                                                                      */
/************************************************************************/
bool CGUITextureManager::CanLoad(const std::string &texturePath)
{
  if (texturePath.empty())
    return false;

  if (!CURL::IsFullPath(texturePath))
    return true;  // assume we have it

  // we can't (or shouldn't) be loading from remote paths, so check these
  return URIUtils::IsHD(texturePath);
}

bool CGUITextureManager::HasTexture(const std::string &textureName, std::string *path, int *bundle, int *size)
{
  CSingleLock lock(m_section);

  // default values
  if (bundle) *bundle = -1;
  if (size) *size = 0;
  if (path) *path = textureName;

  if (textureName.empty())
    return false;

  if (!CanLoad(textureName))
    return false;

  // Check our loaded and bundled textures - we store in bundles using \\.
#if 0
  std::string bundledName = CTextureBundle::Normalize(textureName);
#endif
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == textureName)
    {
#if 0
#ifdef HAS_XBOX_D3D
      for (int i = 0; i < 2; i++)
      {
        if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == bundledName))
        {
          ++m_iNextPreload[i];
          // preload next file
          if (m_iNextPreload[i] != m_PreLoadNames[i].end())
            m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
        }
      }
#endif
#endif
      if (size) *size = 1;
      return true;
    }
  }

#if 0
  for (int i = 0; i < 2; i++)
  {
#ifdef HAS_XBOX_D3D
    if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == bundledName))
    {
      if (bundle) *bundle = i;
      ++m_iNextPreload[i];
      // preload next file
      if (m_iNextPreload[i] != m_PreLoadNames[i].end())
        m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
      return true;
    }
    else
#endif
    if (m_TexBundle[i].HasFile(bundledName))
    {
      if (bundle) *bundle = i;
      return true;
    }
  }
#endif

  std::string fullPath = GetTexturePath(textureName);
  if (path)
    *path = fullPath;

  return !fullPath.empty();
}

const CTextureArray& CGUITextureManager::Load(const std::string& strTextureName, bool checkBundleOnly /*= false */)
{
  std::string strPath;
  static CTextureArray emptyTexture;
  int bundle = -1;
  int size = 0;

  if (strTextureName.empty())
    return emptyTexture;

  if (!HasTexture(strTextureName, &strPath, &bundle, &size))
    return emptyTexture;

  if (size) // we found the texture
  {
    for (int i = 0; i < (int)m_vecTextures.size(); ++i)
    {
      CTextureMap *pMap = m_vecTextures[i];
      if (pMap->GetName() == strTextureName)
      {
        //CLog::Log(LOGDEBUG, "Total memusage {}", GetMemoryUsage());
        return pMap->GetTexture();
      }
    }
    // Whoops, not there.
    return emptyTexture;
  }

  for (auto i = m_unusedTextures.begin(); i != m_unusedTextures.end(); ++i)
  {
    CTextureMap* pMap = i->first;

    auto timestamp = i->second.time_since_epoch();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp);

    if (pMap->GetName() == strTextureName && duration.count() > 0)
    {
      m_vecTextures.push_back(pMap);
      m_unusedTextures.erase(i);
      return pMap->GetTexture();
    }
  }

  if (checkBundleOnly && bundle == -1)
    return emptyTexture;

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

#ifdef _DEBUG_TEXTURES
  const auto start = std::chrono::steady_clock::now();
#endif

  if (bundle >= 0 && StringUtils::EndsWithNoCase(strPath, ".gif"))
  {
#if 0
    CTextureMap* pMap = nullptr;
    std::optional<CTextureBundleXBT::Animation> animation =
        m_TexBundle[bundle].LoadAnim(strTextureName);
    if (!animation)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: {}", strTextureName);
      return emptyTexture;
    }

    int nLoops = animation.value().loops;
    int width = animation.value().width;
    int height = animation.value().height;

    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    pMap = new CTextureMap(strTextureName, width, height, nLoops);
    for (auto& texture : animation.value().textures)
    {
      maxWidth = std::max(maxWidth, texture.first->GetWidth());
      maxHeight = std::max(maxHeight, texture.first->GetHeight());
      pMap->Add(std::move(texture.first), texture.second);
    }

    pMap->SetWidth((int)maxWidth);
    pMap->SetHeight((int)maxHeight);

    m_vecTextures.push_back(pMap);
    return pMap->GetTexture();
#else
    CLog::Log(LOGDEBUG, "{} - GIFs from bundle are not supported: {}", __FUNCTION__, strPath);
    return emptyTexture;
#endif
  }
  else if (StringUtils::EndsWithNoCase(strPath, ".gif") ||
           StringUtils::EndsWithNoCase(strPath, ".apng"))
  {
#if 0
    std::string mimeType;
    if (StringUtils::EndsWithNoCase(strPath, ".gif"))
      mimeType = "image/gif";
    else if (StringUtils::EndsWithNoCase(strPath, ".apng"))
      mimeType = "image/apng";

    XFILE::CFile file;
    std::vector<uint8_t> buf;
    CFFmpegImage anim(mimeType);

    if (file.LoadFile(strPath, buf) <= 0 || !anim.Initialize(buf.data(), buf.size()))
    {
      CLog::Log(LOGERROR, "Texture manager unable to load file: {}", CURL::GetRedacted(strPath));
      file.Close();
      return emptyTexture;
    }

    CTextureMap* pMap = new CTextureMap(strTextureName, 0, 0, 0);
    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    uint64_t maxMemoryUsage = 91238400;// 1920*1080*4*11 bytes, i.e, a total of approx. 12 full hd frames

    auto frame = anim.ReadFrame();
    while (frame)
    {
      boost::movelib::unique_ptr<CTexture> glTexture = CTexture::CreateTexture();
      if (glTexture)
      {
        glTexture->LoadFromMemory(anim.Width(), anim.Height(), frame->GetPitch(), XB_FMT_A8R8G8B8, true, frame->m_pImage);
        maxWidth = std::max(maxWidth, glTexture->GetWidth());
        maxHeight = std::max(maxHeight, glTexture->GetHeight());
        pMap->Add(std::move(glTexture), frame->m_delay);
      }

      if (pMap->GetMemoryUsage() <= maxMemoryUsage)
      {
        frame = anim.ReadFrame();
      }
      else
      {
        CLog::Log(LOGDEBUG, "Memory limit ({} bytes) exceeded, {} frames extracted from file : {}",
                  (maxMemoryUsage / 11) * 12, pMap->GetTexture().size(),
                  CURL::GetRedacted(strPath));
        break;
      }
    }

    pMap->SetWidth((int)maxWidth);
    pMap->SetHeight((int)maxHeight);

    file.Close();

    m_vecTextures.push_back(pMap);
    return pMap->GetTexture();
#else
    CLog::Log(LOGDEBUG, "{} - GIFs/APNGs are not supported: {}", __FUNCTION__, strPath);
    return emptyTexture;
#endif
  }

  boost::movelib::unique_ptr<CTexture> pTexture;
  int width = 0, height = 0;
  if (bundle >= 0)
  {
#if 0
    std::optional<CTextureBundleXBT::Texture> texture =
        m_TexBundle[bundle].LoadTexture(strTextureName);
    if (!texture)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: {}", strTextureName);
      return emptyTexture;
    }

    pTexture = std::move(texture.value().texture);
    width = texture.value().width;
    height = texture.value().height;
#endif
  }
  else
  {
#ifdef HAS_XBOX_D3D
    bool isThumbnail = URIUtils::GetExtension(strPath).Equals(".tbn");
    pTexture = CTexture::LoadFromFile(strPath, isThumbnail ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->GetThumbSize() : 0, isThumbnail ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->GetThumbSize() : 0);
#else
    pTexture = CTexture::LoadFromFile(strPath);
#endif
    if (!pTexture)
      return emptyTexture;
    width = pTexture->GetWidth();
    height = pTexture->GetHeight();
  }

  if (!pTexture) return emptyTexture;

  CTextureMap* pMap = new CTextureMap(strTextureName, width, height, 0);
  pMap->Add(std::move(pTexture), 100);
  m_vecTextures.push_back(pMap);

#ifdef _DEBUG_TEXTURES
  const auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double, std::milli> duration = end - start;
  CLog::Log(LOGDEBUG, "Load {}: {:.3f} ms {}", strPath, duration.count(),
            (bundle >= 0) ? "(bundled)" : "");
#endif

  return pMap->GetTexture();
}


void CGUITextureManager::ReleaseTexture(const std::string& strTextureName, bool immediately /*= false */)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    if (pMap->GetName() == strTextureName)
    {
      if (pMap->Release())
      {
        //CLog::Log(LOGINFO, "  cleanup:{}", strTextureName);
        // add to our textures to free
        std::chrono::time_point<std::chrono::steady_clock> timestamp;

        if (!immediately)
          timestamp = std::chrono::steady_clock::now();

        m_unusedTextures.emplace_back(pMap, timestamp);
        i = m_vecTextures.erase(i);
      }
      return;
    }
    ++i;
  }
  CLog::Log(LOGWARNING, "{}: Unable to release texture {}", __FUNCTION__, strTextureName);
}

void CGUITextureManager::FreeUnusedTextures(unsigned int timeDelay)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  for (auto i = m_unusedTextures.begin(); i != m_unusedTextures.end();)
  {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - i->second);

    if (duration.count() >= timeDelay)
    {
      delete i->first;
      i = m_unusedTextures.erase(i);
    }
    else
      ++i;
  }

#if defined(HAS_GL) || defined(HAS_GLES)
  for (unsigned int i = 0; i < m_unusedHwTextures.size(); ++i)
  {
    // on ios/tvos the hw textures might be deleted from the os
    // when XBMC is backgrounded (e.x. for backgrounded music playback)
    // sanity check before delete in that case.
#if defined(TARGET_DARWIN_EMBEDDED)
    auto winSystem = dynamic_cast<WIN_SYSTEM_CLASS*>(CServiceBroker::GetWinSystem());
    if (!winSystem->IsBackgrounded() || glIsTexture(m_unusedHwTextures[i]))
#endif
      glDeleteTextures(1, (GLuint*) &m_unusedHwTextures[i]);
  }
#endif
  m_unusedHwTextures.clear();
}

void CGUITextureManager::ReleaseHwTexture(unsigned int texture)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_unusedHwTextures.push_back(texture);
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    CLog::Log(LOGWARNING, "{}: Having to cleanup texture {}", __FUNCTION__, pMap->GetName());
    delete pMap;
    i = m_vecTextures.erase(i);
  }
#if 0
  // Kodi XBT bundle
  m_TexBundle[0].Close();
  m_TexBundle[1].Close();
  m_TexBundle[0] = CTextureBundle(true);
  m_TexBundle[1] = CTextureBundle();
#else
#if 0
  // XBMC4Xbox XPR bundle
  m_TexBundle[0].Cleanup();
  m_TexBundle[1].Cleanup();
#endif
#endif
  FreeUnusedTextures();
}

void CGUITextureManager::Dump() const
{
  CLog::Log(LOGDEBUG, "{0}: total texturemaps size: {1}", __FUNCTION__, m_vecTextures.size());

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    const CTextureMap* pMap = m_vecTextures[i];
    if (!pMap->IsEmpty())
      pMap->Dump();
  }
}

void CGUITextureManager::Flush()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    pMap->Flush();
    if (pMap->IsEmpty() )
    {
      delete pMap;
      i = m_vecTextures.erase(i);
    }
    else
    {
      ++i;
    }
  }
}

unsigned int CGUITextureManager::GetMemoryUsage() const
{
  unsigned int memUsage = 0;
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    memUsage += m_vecTextures[i]->GetMemoryUsage();
  }
  return memUsage;
}

void CGUITextureManager::SetTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  m_texturePaths.clear();
  AddTexturePath(texturePath);
}

void CGUITextureManager::AddTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  if (!texturePath.empty())
    m_texturePaths.push_back(texturePath);
}

void CGUITextureManager::RemoveTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  for (std::vector<std::string>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
  {
    if (*it == texturePath)
    {
      m_texturePaths.erase(it);
      return;
    }
  }
}

std::string CGUITextureManager::GetTexturePath(const std::string &textureName, bool directory /* = false */)
{
  if (CURL::IsFullPath(textureName))
    return textureName;
  else
  { // texture doesn't include the full path, so check all fallbacks
    CSingleLock lock(m_section);
    for (const std::string& it : m_texturePaths)
    {
      std::string path = URIUtils::AddFileToFolder(it, "media", textureName);
      if (directory)
      {
        if (XFILE::CDirectory::Exists(path))
          return path;
      }
      else
      {
        if (XFILE::CFile::Exists(path))
          return path;
      }
    }
  }

  CLog::Log(LOGDEBUG, "[Warning] CGUITextureManager::GetTexturePath: could not find texture '{}'",
            textureName);
  return "";
}

std::vector<std::string> CGUITextureManager::GetBundledTexturesFromPath(
    const std::string& texturePath)
{
#if 0
  std::vector<std::string> items = m_TexBundle[0].GetTexturesFromPath(texturePath);
  if (items.empty())
    items = m_TexBundle[1].GetTexturesFromPath(texturePath);
  return items;
#else
  return std::vector<std::string>();
#endif
}
