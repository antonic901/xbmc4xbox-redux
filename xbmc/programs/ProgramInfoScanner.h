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

#include "utils/Thread.h"
#include "ProgramDatabase.h"
#include "addons/Scraper.h"
#include "NfoFile.h"
#include "DateTime.h"

namespace PROGRAM
{
  typedef struct SScanSettings
  {
    SScanSettings() { parent_name = parent_name_root = noupdate = exclude = false; recurse = 1;}
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
    bool exclude;           /* exclude this path from scraping */
  } SScanSettings;

  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, FETCHING_GAME_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

  class IProgramInfoScannerObserver
  {
  public:
    virtual ~IProgramInfoScannerObserver() { }
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnSetCurrentProgress(int currentItem, int itemCount)=0;
    virtual void OnSetTitle(const CStdString& strTitle) = 0;
    virtual void OnFinished() = 0;
  };

  /*! \brief return values from the information lookup functions
   */
  enum INFO_RET { INFO_CANCELLED,
                  INFO_ERROR,
                  INFO_NOT_NEEDED,
                  INFO_HAVE_ALREADY,
                  INFO_NOT_FOUND,
                  INFO_ADDED };

  class CProgramInfoScanner : CThread
  {
  public:
    CProgramInfoScanner();
    virtual ~CProgramInfoScanner();

    /*! \brief Scan a folder using the background scanner
     \param strDirectory path to scan
     \param scanAll whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned.) Defaults to false.
     */
    void Start(const CStdString& strDirectory, bool scanAll = false);
    bool IsScanning();
    void Stop();
    void SetObserver(IProgramInfoScannerObserver* pObserver);

    /*! \brief Retrieve information for a list of items and add them to the database.
     \param items list of items to retrieve info for.
     \param bDirNames whether we should use folder or file names for lookups.
     \param content type of content to retrieve.
     \param useLocal should local data (.nfo and art) be used. Defaults to true.
     \param pURL an optional URL to use to retrieve online info.  Defaults to NULL.
     \param pDlgProgress progress dialog to update and check for cancellation during processing.  Defaults to NULL.
     \return true if we successfully found information for some items, false otherwise
     */
    bool RetrieveProgramInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal = true, CScraperUrl *pURL = NULL, CGUIDialogProgress* pDlgProgress = NULL);

    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);
  protected:
    INFO_RET RetrieveInfoForGame(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);

    IProgramInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bCanInterrupt;
    bool m_bClean;
    bool m_scanAll;
    CStdString m_strStartDir;
    CProgramDatabase m_database;
    std::set<CStdString> m_pathsToScan;
    std::set<CStdString> m_pathsToCount;
    std::vector<int> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}
