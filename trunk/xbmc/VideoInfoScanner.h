#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "utils/Thread.h"
#include "VideoDatabase.h"
#include "ScraperSettings.h"
#include "NfoFile.h"
#include "utils/IMDB.h"
#include "DateTime.h"
#include "FileItem.h"

class CIMDB;
class CRegExp;

namespace VIDEO
{
  typedef struct SScanSettings
  {
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
  } SScanSettings;

  typedef struct SEpisode
  {
    CStdString strPath;
    int iSeason;
    int iEpisode;
    CDateTime cDate;
  } SEpisode;

  typedef std::vector<SEpisode> EPISODES;

  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, FETCHING_MOVIE_INFO, FETCHING_MUSICVIDEO_INFO, FETCHING_TVSHOW_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

  class IVideoInfoScannerObserver
  {
  public:
    virtual ~IVideoInfoScannerObserver() { }
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnSetCurrentProgress(int currentItem, int itemCount)=0;
    virtual void OnSetTitle(const CStdString& strTitle) = 0;
    virtual void OnFinished() = 0;
  };

  class CVideoInfoScanner : CThread
  {
  public:
    CVideoInfoScanner();
    virtual ~CVideoInfoScanner();
    void Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll);
    bool IsScanning();
    void Stop();
    void SetObserver(IVideoInfoScannerObserver* pObserver);

    void EnumerateSeriesFolder(CFileItem* item, EPISODES& episodeList);
    bool ProcessItemNormal(CFileItemPtr item, EPISODES& episodeList, CStdString regexp);
    bool ProcessItemByDate(CFileItemPtr item, EPISODES& eipsodeList, CStdString regexp);
    long AddMovie(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, int idShow = -1);
    long AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, int idShow, bool bApplyToDir=false, bool bRefresh=false, CGUIDialogProgress* pDialog = NULL);
    bool OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, EPISODES& files, int idShow, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress = NULL);
    static CStdString GetnfoFile(CFileItem *item, bool bGrabAny=false);
    long GetIMDBDetails(CFileItem *pItem, CScraperUrl &url, const SScraperInfo& info, bool bUseDirNames=false, CGUIDialogProgress* pDialog=NULL, bool bCombined=false, bool bRefresh=false);
    bool RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh=false, CScraperUrl *pURL=NULL, CGUIDialogProgress* pDlgProgress  = NULL, bool ignoreNfo=false);
    static void ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);
    static int GetPathHash(const CFileItemList &items, CStdString &hash);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, SScraperInfo& info, CScraperUrl& scrUrl);
    CIMDB m_IMDB;
    /*! \brief Fetch thumbs for seasons for a given show
     Fetches and caches local season thumbs of the form season##.tbn and season-all.tbn for the current show,
     and downloads online thumbs if they don't exist.
     \param idTvShow database id of the tvshow.
     \param folderToCheck folder to check for local thumbs, if other than the show folder.  Defaults to empty.
     \param download whether we should download thumbs that don't exist.  Defaults to true.
     \param overwrite whether to overwrite currently cached thumbs.  Defaults to false.
     */
    void FetchSeasonThumbs(int idTvShow, const CStdString &folderToCheck = "", bool download = true, bool overwrite = false);
  protected:
    virtual void Process();
    bool DoScan(const CStdString& strDirectory, SScanSettings settings);

    /*! \brief Extract episode and season numbers from a processed regexp
     \param reg Regular expression object with at least 2 matches
     \param episodeInfo Episode information to fill in.
     \return true on success (2 matches), false on failure (fewer than 2 matches)
     */
    bool GetEpisodeAndSeasonFromRegExp(CRegExp &reg, SEpisode &episodeInfo);

    void FetchActorThumbs(const std::vector<SActorInfo>& actors, const CStdString& strPath);

    /*! \brief Retrieve a "fast" hash of the given directory (if available)
     Performs a stat() on the directory, and uses modified time to create a "fast"
     hash of the folder. If no modified time is available, the create time is used,
     and if neither are available, an empty hash is returned.
     \param directory folder to hash
     \return the hash of the folder of the form "fast<datetime>"
     */
    CStdString GetFastHash(const CStdString &directory) const;

    /*! \brief Decide whether a folder listing could use the "fast" hash
     Fast hashing can be done whenever the folder contains no scannable subfolders, as the
     fast hash technique uses modified time to determine when folder content changes, which
     is generally not propogated up the directory tree.
     \param items the directory listing
     \return true if this directory listing can be fast hashed, false otherwise
     */
    bool CanFastHash(const CFileItemList &items) const;

  protected:
    IVideoInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bUpdateAll;
    bool m_bCanInterrupt;
    bool m_bClean;
    CStdString m_strStartDir;
    CVideoDatabase m_database;
    SScraperInfo m_info;
    std::map<CStdString,SScanSettings> m_pathsToScan;
    std::set<CStdString> m_pathsToCount;
    std::vector<int> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}

