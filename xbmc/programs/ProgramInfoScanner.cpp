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
#include "pictures/Picture.h"
#include "ProgramInfoDownloader.h"
#include "GUIInfoManager.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/File.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/md5.h"

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

  void CProgramInfoScanner::Process()
  {
    try
    {
      unsigned int tick = CTimeUtils::GetTimeMS();

      m_database.Open();

      if (m_pObserver)
        m_pObserver->OnStateChanged(PREPARING);

      m_bCanInterrupt = true;

      CLog::Log(LOGNOTICE, "ProgramInfoScanner: Starting scan ..");

      // Reset progress vars
      m_currentItem = 0;
      m_itemCount = -1;

      SetPriority(GetMinPriority());

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCancelled = false;
      while (!bCancelled && m_pathsToScan.size())
      {
        /*
         * A copy of the directory path is used because the path supplied is
         * immediately removed from the m_pathsToScan set in DoScan(). If the
         * reference points to the entry in the set a null reference error
         * occurs.
         */
        CStdString directory = *m_pathsToScan.begin();
        if (!DoScan(directory))
          bCancelled = true;
      }

      if (!bCancelled)
      {
        if (m_bClean)
          m_database.CleanDatabase(m_pObserver,&m_pathsToClean);
        else
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);
          m_database.Compress(false);
        }
      }

      m_database.Close();

      tick = CTimeUtils::GetTimeMS() - tick;
      CLog::Log(LOGNOTICE, "ProgramInfoScanner: Finished scan. Scanning for program info took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());

      m_bRunning = false;
      if (m_pObserver)
        m_pObserver->OnFinished();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "ProgramInfoScanner: Exception while scanning.");
    }
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

  bool CProgramInfoScanner::DoScan(const CStdString& strDirectory)
  {
    if (m_pObserver)
    {
      m_pObserver->OnDirectoryChanged(strDirectory);
      m_pObserver->OnSetTitle(g_localizeStrings.Get(20415));
    }

    /*
     * Remove this path from the list we're processing. This must be done prior to
     * the check for file or folder exclusion to prevent an infinite while loop
     * in Process().
     */
    set<CStdString>::iterator it = m_pathsToScan.find(strDirectory);
    if (it != m_pathsToScan.end())
      m_pathsToScan.erase(it);

    // load subfolder
    CFileItemList items;
    bool foundDirectly = false;
    bool bSkip = false;

    SScanSettings settings;
    ScraperPtr info = m_database.GetScraperForPath(strDirectory, settings, foundDirectly);
    CONTENT_TYPE content = info ? info->Content() : CONTENT_NONE;

    // exclude folders that match our exclude regexps
    CStdStringArray regexps = g_advancedSettings.m_gamesExcludeFromScanRegExps;

    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
      return true;

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == CONTENT_NONE || ignoreFolder)
      return true;

    CStdString hash, dbHash;
    if (content == CONTENT_GAMES || content == CONTENT_APPLICATIONS)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(content == CONTENT_GAMES ? FETCHING_GAME_INFO : FETCHING_APPLICATION_INFO);

      CStdString fastHash = GetFastHash(strDirectory);
      if (m_database.GetPathHash(strDirectory, dbHash) && !fastHash.IsEmpty() && fastHash == dbHash)
      { // fast hashes match - no need to process anything
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Skipping dir '%s' due to no change (fasthash)", strDirectory.c_str());
        hash = fastHash;
        bSkip = true;
      }
      if (!bSkip)
      { // need to fetch the folder
        CDirectory::GetDirectory(strDirectory, items, g_settings.m_programExtensions);
        if (content == CONTENT_GAMES )
          items.Stack();
        // compute hash
        GetPathHash(items, hash);
        if (hash != dbHash && !hash.IsEmpty())
        {
          if (dbHash.IsEmpty())
            CLog::Log(LOGDEBUG, "ProgramInfoScanner: Scanning dir '%s' as not in the database", strDirectory.c_str());
          else
            CLog::Log(LOGDEBUG, "ProgramInfoScanner: Rescanning dir '%s' due to change (%s != %s)", strDirectory.c_str(), dbHash.c_str(), hash.c_str());
        }
        else
        { // they're the same or the hash is empty (dir empty/dir not retrievable)
          if (hash.IsEmpty() && !dbHash.IsEmpty())
          {
            CLog::Log(LOGDEBUG, "ProgramInfoScanner: Skipping dir '%s' as it's empty or doesn't exist - adding to clean list", strDirectory.c_str());
            m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
          }
          else
            CLog::Log(LOGDEBUG, "ProgramInfoScanner: Skipping dir '%s' due to no change", strDirectory.c_str());
          bSkip = true;
          if (m_pObserver)
            m_pObserver->OnDirectoryScanned(strDirectory);
        }
        // update the hash to a fast hash if needed
        if (CanFastHash(items) && !fastHash.IsEmpty())
          hash = fastHash;
      }
    }

    if (!bSkip)
    {
      if (RetrieveProgramInfo(items, settings.parent_name_root, content))
      {
        if (!m_bStop && (content == CONTENT_GAMES || content == CONTENT_APPLICATIONS))
        {
          m_database.SetPathHash(strDirectory, hash);
          m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
          CLog::Log(LOGDEBUG, "ProgramInfoScanner: Finished adding information from dir %s", strDirectory.c_str());
        }
      }
      else
      {
        m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: No (new) information was found in dir %s", strDirectory.c_str());
      }
    }
    else if (hash != dbHash && (content == CONTENT_GAMES || content == CONTENT_APPLICATIONS))
    { // update the hash either way - we may have changed the hash to a fast version
      m_database.SetPathHash(strDirectory, hash);
    }

    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (m_bStop)
        break;

      // if we have a directory item (non-playlist) we then recurse into that folder
      // do not recurse for tv shows - we have already looked recursively for episodes
      if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse > 0)
      {
        if (!DoScan(pItem->GetPath()))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
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

      if (info2->Content() == CONTENT_GAMES || info2->Content() == CONTENT_APPLICATIONS)
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
      if (info2->Content() == CONTENT_GAMES || info2->Content() == CONTENT_APPLICATIONS)
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
    if (pItem->m_bIsFolder || !pItem->IsProgram() || pItem->IsNFO() || pItem->IsPlayList())
      return INFO_NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 35006, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (info2->Content() == CONTENT_GAMES ? m_database.HasGameInfo(pItem->GetPath()) : m_database.HasApplicationInfo(pItem->GetPath()))
      return INFO_HAVE_ALREADY;

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    if (useLocal)
      result = CheckForNFOFile(pItem.get(), bDirNames, info2, scrUrl);
    if (result == CNfoFile::FULL_NFO)
    {
      pItem->GetProgramInfoTag()->Reset();
      m_nfoReader.GetDetails(*pItem->GetProgramInfoTag());
      if (m_pObserver)
        m_pObserver->OnSetTitle(pItem->GetProgramInfoTag()->m_strTitle);

      if (AddProgram(pItem.get(), info2->Content(), bDirNames) < 0)
        return INFO_ERROR;
      GetArtwork(pItem.get(), info2->Content(), bDirNames, true, pDlgProgress);
      return INFO_ADDED;
    }
    if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
      pURL = &scrUrl;

    CScraperUrl url;
    int retVal = 0;
    if (pURL)
      url = *pURL;
    else if ((retVal = FindProgram(pItem->GetGameName(bDirNames), info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    if (m_pObserver && !url.strTitle.IsEmpty())
      m_pObserver->OnSetTitle(url.strTitle);

    if (GetDetails(pItem.get(), url, info2, result == CNfoFile::COMBINED_NFO ? &m_nfoReader : NULL, pDlgProgress))
    {
      if (AddProgram(pItem.get(), info2->Content(), bDirNames) < 0)
        return INFO_ERROR;
      GetArtwork(pItem.get(), info2->Content(), bDirNames, useLocal);
      return INFO_ADDED;
    }

    return INFO_NOT_FOUND;
  }

  long CProgramInfoScanner::AddProgram(CFileItem *pItem, const CONTENT_TYPE &content, bool programFolder, int idShow)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    CLog::Log(LOGDEBUG, "ProgramInfoScanner: Adding new item to %s:%s", TranslateContent(content).c_str(), pItem->GetPath().c_str());
    long lResult = -1;

    CProgramInfoTag &gameDetails = *pItem->GetProgramInfoTag();
    if (gameDetails.m_basePath.IsEmpty())
      gameDetails.m_basePath = pItem->GetBaseGamePath(programFolder);
    gameDetails.m_parentPathID = m_database.AddPath(URIUtils::GetParentPath(gameDetails.m_basePath));

    if (content == CONTENT_GAMES)
    {
      // find local trailer first
      CStdString strTrailer = pItem->FindTrailer();
      if (!strTrailer.IsEmpty())
        gameDetails.m_strTrailer = strTrailer;

      lResult = m_database.SetDetailsForGame(pItem->GetPath(), gameDetails);
      gameDetails.m_iDbId = lResult;
    }
    else if (content == CONTENT_APPLICATIONS)
    {
      lResult = m_database.SetDetailsForApplication(pItem->GetPath(), gameDetails);
      gameDetails.m_iDbId = lResult;
    }

    m_database.Close();
    return lResult;
  }

  void CProgramInfoScanner::GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir, bool useLocal, CGUIDialogProgress* pDialog /* == NULL */)
  {
    CProgramInfoTag &programDetails = *pItem->GetProgramInfoTag();
    // get & save fanart image
    if (!useLocal || !pItem->CacheLocalFanart())
    {
      if (programDetails.m_fanart.GetNumFanarts())
        DownloadImage(programDetails.m_fanart.GetImageURL(), pItem->GetCachedFanart(), false, pDialog);
    }

    // get & save thumb image
    CStdString cachedThumb = pItem->GetCachedProgramThumb();
    if (CFile::Exists(cachedThumb))
    {
      programDetails.m_strFileNameAndPath = pItem->GetPath();
      CFileItem item(programDetails);
      cachedThumb = item.GetCachedEpisodeThumb();
    }

    CStdString localThumb;
    if (useLocal)
    {
      localThumb = pItem->GetUserProgramThumb();
      if (bApplyToDir && localThumb.IsEmpty())
      {
        CStdString strParent;
        URIUtils::GetParentPath(pItem->GetPath(), strParent);
        CFileItem item(*pItem);
        item.SetPath(strParent);
        item.m_bIsFolder = true;
        localThumb = item.GetUserProgramThumb();
      }
    }

    // parent folder to apply the thumb to and to search for local actor thumbs
    CStdString parentDir = GetParentDir(*pItem);

    if (!localThumb.IsEmpty())
      CPicture::CacheThumb(localThumb, cachedThumb);
    else
    { // see if we have an online image to use
      CStdString onlineThumb = CScraperUrl::GetThumbURL(programDetails.m_strPictureURL.GetFirstThumb());
      if (!onlineThumb.IsEmpty())
      {
        if (onlineThumb.Find("http://") < 0 &&
            onlineThumb.Find("/") < 0 &&
            onlineThumb.Find("\\") < 0)
        {
          CStdString strPath;
          URIUtils::GetDirectory(pItem->GetPath(), strPath);
          onlineThumb = URIUtils::AddFileToFolder(strPath, onlineThumb);
        }
        DownloadImage(onlineThumb, cachedThumb, true, pDialog);
      }
    }
    if (bApplyToDir)
      ApplyThumbToFolder(parentDir, cachedThumb);
  }

  void CProgramInfoScanner::DownloadImage(const CStdString &url, const CStdString &destination, bool asThumb /*= true */, CGUIDialogProgress *progress /*= NULL */)
  {
    if (progress)
    {
      progress->SetLine(2, 415);
      progress->Progress();
    }
    bool result = false;
    if (asThumb)
      result = CPicture::CreateThumbnail(url, destination);
    else
      result = CPicture::CacheFanart(url, destination);
    if (!result)
    {
      CFile::Delete(destination);
      return;
    }
  }

  CStdString CProgramInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny) const
  {
    CStdString nfoFile;
    // Find a matching .nfo file
    if (!item->m_bIsFolder)
    {
      // grab the folder path
      CStdString strPath;
      URIUtils::GetDirectory(item->GetPath(), strPath);

      if (bGrabAny)
      { // looking up by folder name - game.nfo takes priority
        nfoFile = URIUtils::AddFileToFolder(strPath, "game.nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;
      }

      if (item->IsXBE())
      { // use resources file from XBMC4Gamers
        nfoFile = URIUtils::AddFileToFolder(strPath, "_resources\\default.xml");
      }
      else if(item->IsROM())
      { // romname.nfo
        nfoFile = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
      }
    }

    return nfoFile;
  }

  bool CProgramInfoScanner::GetDetails(CFileItem *pItem, CScraperUrl &url, const ScraperPtr& scraper, CNfoFile *nfoFile, CGUIDialogProgress* pDialog /* = NULL */)
  {
    CProgramInfoTag gameDetails;
    gameDetails.m_strFileNameAndPath = pItem->GetPath();

    CProgramInfoDownloader igdb(scraper);
    if ( igdb.GetDetails(url, gameDetails, pDialog) )
    {
      if (nfoFile)
        nfoFile->GetDetails(gameDetails);

      if (m_pObserver && url.strTitle.IsEmpty())
        m_pObserver->OnSetTitle(gameDetails.m_strTitle);

      if (pDialog)
      {
        pDialog->SetLine(1, gameDetails.m_strTitle);
        pDialog->Progress();
      }

      *pItem->GetProgramInfoTag() = gameDetails;
      return true;
    }
    return false; // no info found, or cancelled
  }

  void CProgramInfoScanner::ApplyThumbToFolder(const CStdString &folder, const CStdString &igdbThumb)
  {
    // copy icon to folder also;
    if (CFile::Exists(igdbThumb))
    {
      CFileItem folderItem(folder, true);
      CStdString strThumb(folderItem.GetCachedProgramThumb());
      CFile::Cache(igdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
    }
  }

  int CProgramInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
  {
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    XBMC::XBMC_MD5 md5state;
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr pItem = items[i];
      md5state.append(pItem->GetPath());
      md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
      FILETIME time = pItem->m_dateTime;
      md5state.append((unsigned char *)&time, sizeof(FILETIME));
      if (pItem->IsProgram() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    md5state.getDigest(hash);
    return count;
  }

  bool CProgramInfoScanner::CanFastHash(const CFileItemList &items) const
  {
    // TODO: Probably should account for excluded folders here (eg samples), though that then
    //       introduces possible problems if the user then changes the exclude regexps and
    //       expects excluded folders that are inside a fast-hashed folder to then be picked
    //       up. The chances that the user has a folder which contains only excluded folders
    //       where some of those folders should be scanned recursively is pretty small.
    return items.GetFolderCount() == 0;
  }

  CStdString CProgramInfoScanner::GetFastHash(const CStdString &directory) const
  {
    struct __stat64 buffer;
    if (XFILE::CFile::Stat(directory, &buffer) == 0)
    {
      int64_t time = buffer.st_mtime;
      if (!time)
        time = buffer.st_ctime;
      if (time)
      {
        CStdString hash;
        hash.Format("fast%"PRId64, time);
        return hash;
      }
    }
    return "";
  }

  CNfoFile::NFOResult CProgramInfoScanner::CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ScraperPtr& info, CScraperUrl& scrUrl)
  {
    CStdString strNfoFile;
    if (info->Content() == CONTENT_GAMES || info->Content() == CONTENT_APPLICATIONS)
      strNfoFile = GetnfoFile(pItem, bGrabAny);

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    if (!strNfoFile.IsEmpty() && CFile::Exists(strNfoFile))
    {
      result = m_nfoReader.Create(strNfoFile,info,-1);

      CStdString type;
      switch(result)
      {
        case CNfoFile::COMBINED_NFO:
          type = "Mixed";
          break;
        case CNfoFile::FULL_NFO:
          type = "Full";
          break;
        case CNfoFile::URL_NFO:
          type = "URL";
          break;
        case CNfoFile::NO_NFO:
          type = "";
          break;
        default:
          type = "malformed";
      }
      if (result != CNfoFile::NO_NFO)
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Found matching %s NFO file: %s", type.c_str(), strNfoFile.c_str());
      if (result == CNfoFile::FULL_NFO)
      {
        // TODO: we will (probably) need this block for emulators roms integration (if not remove it)
      }
      else if (result != CNfoFile::NO_NFO && result != CNfoFile::ERROR_NFO)
      {
        scrUrl = m_nfoReader.ScraperUrl();
        info = m_nfoReader.GetScraperInfo();

        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Fetching url '%s' using %s scraper (content: '%s')",
          scrUrl.m_url[0].m_url.c_str(), info->Name().c_str(), TranslateContent(info->Content()).c_str());

        if (result == CNfoFile::COMBINED_NFO)
          m_nfoReader.GetDetails(*pItem->GetProgramInfoTag());
      }
    }
    else
      CLog::Log(LOGDEBUG, "ProgramInfoScanner: No NFO file found. Using title search for '%s'", pItem->GetPath().c_str());

    return result;
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

  bool CProgramInfoScanner::ProgressCancelled(CGUIDialogProgress* progress, int heading, const CStdString &line1)
  {
    if (progress)
    {
      progress->SetHeading(heading);
      progress->SetLine(0, line1);
      progress->SetLine(2, "");
      progress->Progress();
      return progress->IsCanceled();
    }
    return m_bStop;
  }

  int CProgramInfoScanner::FindProgram(const CStdString &programName, const ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress)
  {
    GAMELIST gamelist;
    CProgramInfoDownloader igdb(scraper);
    int returncode = igdb.FindGame(programName, gamelist, progress);
    if (returncode < 0 || (returncode == 0 && !DownloadFailed(progress)))
    { // scraper reported an error, or we had an error and user wants to cancel the scan
      m_bStop = true;
      return -1; // cancelled
    }
    if (returncode > 0 && gamelist.size())
    {
      url = gamelist[0];
      return 1;  // found a game
    }
    return 0; // didn't find anything
  }

  CStdString CProgramInfoScanner::GetParentDir(const CFileItem &item) const
  {
    CStdString strCheck = item.GetPath();
    if (item.IsStack())
      strCheck = CStackDirectory::GetFirstStackedFile(item.GetPath());

    CStdString strDirectory;
    URIUtils::GetDirectory(strCheck, strDirectory);
    if (URIUtils::IsInRAR(strCheck))
    {
      CStdString strPath=strDirectory;
      URIUtils::GetParentPath(strPath, strDirectory);
    }
    if (item.IsStack())
    {
      strCheck = strDirectory;
      URIUtils::RemoveSlashAtEnd(strCheck);
      if (URIUtils::GetFileName(strCheck).size() == 3 && URIUtils::GetFileName(strCheck).Left(2).Equals("cd"))
        URIUtils::GetDirectory(strCheck, strDirectory);
    }
    return strDirectory;
  }
}
