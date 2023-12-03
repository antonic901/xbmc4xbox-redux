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

#include "ProgramInfoScanner.h"
#include "addons/AddonManager.h"
#include "settings/AdvancedSettings.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

namespace PROGRAM
{
  CProgramInfoScanner::CProgramInfoScanner()
  {
    m_bRunning = false;
    m_pObserver = NULL;
    m_bCanInterrupt = false;
    m_currentItem = 0;
    m_itemCount = 0;
    m_bClean = false;
    m_scanAll = false;
  }

  CProgramInfoScanner::~CProgramInfoScanner()
  {
  }

  void CProgramInfoScanner::Start(const CStdString& strDirectory, bool scanAll)
  {
    m_strStartDir = strDirectory;
    m_scanAll = scanAll;
    m_pathsToScan.clear();
    m_pathsToClean.clear();

    if (strDirectory.IsEmpty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
      // we go.
      m_database.Open();
      m_database.GetPaths(m_pathsToScan);
      m_database.Close();
    }
    else
    {
      m_pathsToScan.insert(strDirectory);
    }
    m_bClean = g_advancedSettings.m_bProgramLibraryCleanOnUpdate;

    StopThread();
    Create();
    m_bRunning = true;
  }

  bool CProgramInfoScanner::IsScanning()
  {
    return m_bRunning;
  }

  void CProgramInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interupt();

    StopThread();
  }

  void CProgramInfoScanner::SetObserver(IProgramInfoScannerObserver* pObserver)
  {
    m_pObserver = pObserver;
  }
}
