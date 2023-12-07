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

#include "FileItem.h"
#include "ProgramInfoScanner.h"
#include "addons/AddonManager.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

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

  bool CProgramInfoScanner::RetrieveProgramInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    if (pDlgProgress)
    {
      if (items.Size() > 1)
      {
        pDlgProgress->ShowProgressBar(true);
        pDlgProgress->SetPercentage(0);
      }
      else
        pDlgProgress->ShowProgressBar(false);

      pDlgProgress->Progress();
    }

    m_database.Open();

    bool FoundSomeInfo = false;
    vector<int> seenPaths;
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      m_nfoReader.Close();
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(pItem->m_bIsFolder ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), g_advancedSettings.m_gamesExcludeFromScanRegExps))
        continue;

      if (info2->Content() == CONTENT_GAMES)
      {
        if (m_pObserver)
        {
          m_pObserver->OnSetCurrentProgress(i, items.Size());
          if (!pItem->m_bIsFolder && m_itemCount)
            m_pObserver->OnSetProgress(m_currentItem++, m_itemCount);
        }
      }

      // clear our scraper cache
      info2->ClearCache();

      INFO_RET ret = INFO_CANCELLED;
      if (info2->Content() == CONTENT_GAMES)
        ret = RetrieveInfoForGame(pItem, bDirNames, info2, useLocal, pURL, pDlgProgress);
      else
      {
        CLog::Log(LOGERROR, "ProgramInfoScanner: Unknown content type %d (%s)", info2->Content(), pItem->GetPath().c_str());
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_CANCELLED || ret == INFO_ERROR)
      {
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_ADDED || ret == INFO_HAVE_ALREADY)
        FoundSomeInfo = true;

      pURL = NULL;

      // Keep track of directories we've seen
      if (pItem->m_bIsFolder)
        seenPaths.push_back(m_database.GetPathId(pItem->GetPath()));
    }

    if(pDlgProgress)
      pDlgProgress->ShowProgressBar(false);

    g_infoManager.ResetLibraryBools();
    m_database.Close();
    return FoundSomeInfo;
  }

  INFO_RET CProgramInfoScanner::RetrieveInfoForGame(CFileItemPtr pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    // TODO: This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  bool CProgramInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    if (g_advancedSettings.m_bProgramScannerIgnoreErrors)
      return true;

    if (pDialog)
    {
      CGUIDialogOK::ShowAndGetInput(20448,20449,20022,20022);
      return false;
    }
    return CGUIDialogYesNo::ShowAndGetInput(20448,20449,20450,20022);
  }
}
