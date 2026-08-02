#pragma once
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

#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"

#include <string>

//  forward
class LibraryLoader;

class CSectionLoader
{
public:
  class CSection
  {
  public:
    std::string m_strSectionName;
    long m_lReferenceCount;
    unsigned int m_unloadDelayStartTick;
  };
  class CDll
  {
  public:
    std::string m_strDllName;
    long m_lReferenceCount;
    LibraryLoader *m_pDll;
    unsigned int m_unloadDelayStartTick;
    bool m_bDelayUnload;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const std::string& strSection);
  static bool Load(const std::string& strSection);
  static void Unload(const std::string& strSection);
  static LibraryLoader* LoadDLL(const std::string& strSection, bool bDelayUnload=true, bool bLoadSymbols=false);
  static void UnloadDLL(const std::string& strSection);
  static void UnloadDelayed();
protected:
  std::vector<CSection> m_vecLoadedSections;
  typedef std::vector<CSection>::iterator ivecLoadedSections;
  std::vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;

private:
  void UnloadAll();
};

extern  CSectionLoader g_sectionLoader;
