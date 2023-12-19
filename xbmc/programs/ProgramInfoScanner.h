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

    /*! \brief Add an item to the database.
     \param pItem item to add to the database.
     \param content content type of the item.
     \param programFolder whether the program is represented by a folder (single game per folder). Defaults to false.
     \param idShow database id of the tvshow if we're adding an episode.  Defaults to -1.
     \return database id of the added item, or -1 on failure.
     */
    long AddProgram(CFileItem *pItem, const CONTENT_TYPE &content, bool programFolder = false, int idShow = -1);

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

    static void ApplyThumbToFolder(const CStdString &folder, const CStdString &igdbThumb);
    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ADDON::ScraperPtr& scraper, CScraperUrl& scrUrl);

  protected:
    INFO_RET RetrieveInfoForGame(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);

    /*! \brief Update the progress bar with the heading and line and check for cancellation
     \param progress CGUIDialogProgress bar
     \param heading string id of heading
     \param line1   string to set for the first line
     \return true if the user has cancelled the scanner, false otherwise
     */
    bool ProgressCancelled(CGUIDialogProgress* progress, int heading, const CStdString &line1);

    /*! \brief Find a url for the given program using the given scraper
     \param programName name of the program to lookup
     \param scraper scraper to use for the lookup
     \param url [out] returned url from the scraper
     \param progress CGUIDialogProgress bar
     \return >0 on success, <0 on failure (cancellation), and 0 on no info found
     */
    int FindProgram(const CStdString &programName, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    /*! \brief Retrieve detailed information for an item from an online source, optionally supplemented with local data
     TODO: sort out some better return codes.
     \param pItem item to retrieve online details for.
     \param url URL to use to retrieve online details.
     \param scraper Scraper that handles parsing the online data.
     \param nfoFile if set, we override the online data with the locally supplied data. Defaults to NULL.
     \param pDialog progress dialog to update and check for cancellation during processing. Defaults to NULL.
     \return true if information is found, false if an error occurred, the lookup was cancelled, or no information was found.
     */
    bool GetDetails(CFileItem *pItem, CScraperUrl &url, const ADDON::ScraperPtr &scraper, CNfoFile *nfoFile=NULL, CGUIDialogProgress* pDialog=NULL);

    /*! \brief Retrieve any artwork associated with an item
     \param pItem item to add to the database.
     \param content content type of the item.
     \param bApplyToDir whether we should apply any thumbs to a folder.  Defaults to false.
     \param useLocal whether we should use local thumbs. Defaults to true.
     \param pDialog progress dialog to update during processing. Defaults to NULL.
     */
    void GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir=false, bool useLocal=true, CGUIDialogProgress* pDialog = NULL);

    /*! \brief Download an image file and apply the image to a folder if necessary
     \param url URL of the image.
     \param destination File to save the image as
     \param asThumb whether we need to download as a thumbnail or as a full image. Defaults to true
     \param progress progressbar to update - defaults to NULL
     \param directory directory that this thumbnail should be applied to. Defaults to empty
     */
    void DownloadImage(const CStdString &url, const CStdString &destination, bool asThumb = true, CGUIDialogProgress *dialog = NULL);
  
    CStdString GetnfoFile(CFileItem *item, bool bGrabAny=false) const;

    /*! \brief Retrieve the parent folder of an item, accounting for stacks and files in rars.
     \param item a media item.
     \return the folder that contains the item.
     */
    CStdString GetParentDir(const CFileItem &item) const;
  
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
