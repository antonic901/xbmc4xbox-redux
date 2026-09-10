/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SectionLoader.h"

#include "cores/DllLoader/DllLoaderContainer.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/GlobalsHandling.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#define g_sectionLoader XBMC_GLOBAL_USE(CSectionLoader)

//  delay for unloading dll's
#define UNLOAD_DELAY 10*1000 // 10 sec.

//Define this to get logging on all calls to load/unload sections/dlls
//#define LOGALL

CSectionLoader::CSectionLoader(void) {}

CSectionLoader::~CSectionLoader(void)
{
  UnloadAll();
}

LibraryLoader *CSectionLoader::LoadDLL(const std::string &dllname, bool bDelayUnload /*=true*/, bool bLoadSymbols /*=false*/)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (dllname.empty()) return NULL;
  // check if it's already loaded, and increase the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (StringUtils::EqualsNoCase(dll.m_strDllName, dllname))
    {
      dll.m_lReferenceCount++;
      return dll.m_pDll;
    }
  }

  // ok, now load the dll
  CLog::Log(LOGDEBUG, "SECTION:LoadDLL(%s)", dllname.c_str());
  LibraryLoader* pDll = DllLoaderContainer::LoadModule(dllname.c_str(), NULL, bLoadSymbols);
  if (!pDll)
    return NULL;

  CDll newDLL;
  newDLL.m_strDllName = dllname;
  newDLL.m_lReferenceCount = 1;
  newDLL.m_bDelayUnload=bDelayUnload;
  newDLL.m_pDll=pDll;
  g_sectionLoader.m_vecLoadedDLLs.push_back(newDLL);

  return newDLL.m_pDll;
}

void CSectionLoader::UnloadDLL(const std::string &dllname)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (dllname.empty()) return;
  // check if it's already loaded, and decrease the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (StringUtils::EqualsNoCase(dll.m_strDllName, dllname))
    {
      dll.m_lReferenceCount--;
      if (0 == dll.m_lReferenceCount)
      {
        if (dll.m_bDelayUnload)
          dll.m_unloadDelayStartTick = XbmcThreads::SystemClockMillis();
        else
        {
          CLog::Log(LOGDEBUG, "SECTION:UnloadDll(%s)", dllname.c_str());
          if (dll.m_pDll)
            DllLoaderContainer::ReleaseModule(dll.m_pDll);
          g_sectionLoader.m_vecLoadedDLLs.erase(g_sectionLoader.m_vecLoadedDLLs.begin() + i);
        }

        return;
      }
    }
  }
}

void CSectionLoader::UnloadDelayed()
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  std::vector<CSection>::iterator section = g_sectionLoader.m_vecLoadedSections.begin();
  while (section != g_sectionLoader.m_vecLoadedSections.end())
  {
    if (section->m_lReferenceCount == 0 && XbmcThreads::SystemClockMillis() - section->m_unloadDelayStartTick > UNLOAD_DELAY)
    {
      CLog::Log(LOGDEBUG, "SECTION:UnloadDelayed(SECTION: %s)", section->m_strSectionName.c_str());
      XFreeSection(section->m_strSectionName.c_str());
      section = g_sectionLoader.m_vecLoadedSections.erase(section);
    }
    else
    {
      ++section;
    }
  }

  // check if we can unload any unreferenced dlls
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    unsigned int now = XbmcThreads::SystemClockMillis();
    if (dll.m_lReferenceCount == 0 && now - dll.m_unloadDelayStartTick > UNLOAD_DELAY)
    {
      CLog::Log(LOGDEBUG, "SECTION:UnloadDelayed(DLL: %s)", dll.m_strDllName.c_str());

      if (dll.m_pDll)
        DllLoaderContainer::ReleaseModule(dll.m_pDll);
      g_sectionLoader.m_vecLoadedDLLs.erase(g_sectionLoader.m_vecLoadedDLLs.begin() + i);
      return;
    }
  }
}

void CSectionLoader::UnloadAll()
{
  std::vector<CSection>::iterator section = g_sectionLoader.m_vecLoadedSections.begin();
  while (section != g_sectionLoader.m_vecLoadedSections.end())
  {
    XFreeSection(section->m_strSectionName.c_str());
    section = g_sectionLoader.m_vecLoadedSections.erase(section);
  }

  // delete the dll's
  CSingleLock lock(g_sectionLoader.m_critSection);
  std::vector<CDll>::iterator it = g_sectionLoader.m_vecLoadedDLLs.begin();
  while (it != g_sectionLoader.m_vecLoadedDLLs.end())
  {
    CDll& dll = *it;
    if (dll.m_pDll)
      DllLoaderContainer::ReleaseModule(dll.m_pDll);
    it = g_sectionLoader.m_vecLoadedDLLs.erase(it);
  }
}

bool CSectionLoader::IsLoaded(const std::string& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  for (std::vector<CSection>::iterator section = g_sectionLoader.m_vecLoadedSections.begin(); section != g_sectionLoader.m_vecLoadedSections.end(); ++section)
  {
    if (section->m_strSectionName == strSection && section->m_lReferenceCount > 0)
      return true;
  }

  return false;
}

bool CSectionLoader::Load(const std::string& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  for (std::vector<CSection>::iterator section = g_sectionLoader.m_vecLoadedSections.begin(); section != g_sectionLoader.m_vecLoadedSections.end(); ++section)
  {
    if (section->m_strSectionName == strSection)
    {
      section->m_lReferenceCount++;
      return true;
    }
  }

  if (NULL == XLoadSection(strSection.c_str()))
  {
    CLog::Log(LOGDEBUG, "SECTION:LoadSection(%s) load failed!", strSection.c_str());
    return false;
  }

  HANDLE hHandle = XGetSectionHandle(strSection.c_str());
  CLog::Log(LOGDEBUG, "SECTION:Section %s loaded count: 1 size: %i", strSection.c_str(), XGetSectionSize(hHandle));

  CSection newSection;
  newSection.m_strSectionName = strSection;
  newSection.m_lReferenceCount = 1;
  g_sectionLoader.m_vecLoadedSections.push_back(newSection);

  return true;
}

void CSectionLoader::Unload(const std::string& strSection)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (!CSectionLoader::IsLoaded(strSection))
    return;

  for (std::vector<CSection>::iterator section = g_sectionLoader.m_vecLoadedSections.begin(); section != g_sectionLoader.m_vecLoadedSections.end(); ++section)
  {
    if (section->m_strSectionName == strSection)
    {
      section->m_lReferenceCount--;
      if (0 == section->m_lReferenceCount)
      {
        section->m_unloadDelayStartTick = XbmcThreads::SystemClockMillis();
        return;
      }
    }
  }
}
