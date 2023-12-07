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
#include "Database.h"
#include "programs/ProgramInfoTag.h"
#include "video/VideoDatabase.h" // for SDbTableOffsets
#include "addons/Scraper.h"

typedef std::vector<CStdString> VECPROGRAMPATHS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

class CFileItem;
class CFileItemList;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

#ifndef my_offsetof
#ifndef _LINUX
#define my_offsetof(TYPE, MEMBER) offsetof(TYPE, MEMBER)
#else
/*
   Custom version of standard offsetof() macro which can be used to get
   offsets of members in class for non-POD types (according to the current
   version of C++ standard offsetof() macro can't be used in such cases and
   attempt to do so causes warnings to be emitted, OTOH in many cases it is
   still OK to assume that all instances of the class has the same offsets
   for the same members).
 */
#define my_offsetof(TYPE, MEMBER) \
               ((size_t)((char *)&(((TYPE *)0x10)->MEMBER) - (char*)0x10))
#endif
#endif

typedef std::vector<CProgramInfoTag> VECPROGRAMS;

namespace PROGRAM
{
  class IProgramInfoScannerObserver;
  struct SScanSettings;
}

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define PROGRAMDB_MAX_COLUMNS 24
#define PROGRAMDB_DETAILS_FILEID      1

#define PROGRAMDB_DETAILS_GAME_FILE			    PROGRAMDB_MAX_COLUMNS + 2
#define PROGRAMDB_DETAILS_GAME_PATH			    PROGRAMDB_MAX_COLUMNS + 3
#define PROGRAMDB_DETAILS_GAME_PLAYCOUNT		PROGRAMDB_MAX_COLUMNS + 4
#define PROGRAMDB_DETAILS_GAME_LASTPLAYED   PROGRAMDB_MAX_COLUMNS + 5
#define PROGRAMDB_DETAILS_GAME_DATEADDED		PROGRAMDB_MAX_COLUMNS + 6

#define PROGRAMDB_TYPE_STRING 1
#define PROGRAMDB_TYPE_INT 2
#define PROGRAMDB_TYPE_FLOAT 3
#define PROGRAMDB_TYPE_BOOL 4
#define PROGRAMDB_TYPE_COUNT 5
#define PROGRAMDB_TYPE_STRINGARRAY 6
#define PROGRAMDB_TYPE_DATE 7
#define PROGRAMDB_TYPE_DATETIME 8

typedef enum
{
  PROGRAMDB_CONTENT_GAMES = 1,
  PROGRAMDB_CONTENT_EMULATORS = 2,
  PROGRAMDB_CONTENT_HOMEBREWS = 3
} PROGRAMDB_CONTENT_TYPE;

typedef enum
{
  PROGRAMDB_ID_MIN = -1,
  PROGRAMDB_ID_TITLE = 0,
  PROGRAMDB_ID_PLOT = 1,
  PROGRAMDB_ID_YEAR = 2,
  PROGRAMDB_ID_RATING = 3,
  PROGRAMDB_ID_THUMBURL = 4,
  PROGRAMDB_ID_IDENT = 5,
  PROGRAMDB_ID_ESRB = 6,
  PROGRAMDB_ID_ESRB_DES = 7,
  PROGRAMDB_ID_GENRE = 8,
  PROGRAMDB_ID_DEVELOPER = 9,
  PROGRAMDB_ID_PUBLISHER = 10,
  PROGRAMDB_ID_ORIGINALTITLE = 11,
  PROGRAMDB_ID_TRAILER = 12,
  PROGRAMDB_ID_FANART = 13,
  PROGRAMDB_ID_BASEPATH = 14,
  PROGRAMDB_ID_PARENTPATHID = 15,
  PROGRAMDB_ID_MAX
} PROGRAMDB_IDS;

const struct SDbTableOffsets DbGameOffsets[] =
{
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPlot) },
  { PROGRAMDB_TYPE_FLOAT, my_offsetof(CProgramInfoTag,m_fRating) },
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_iYear) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_xml) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strXBENumber) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRB) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRBDescription) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_developer) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_publisher) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_genre) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strOriginalTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_spoof) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTrailer) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_fanart.m_xml) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_basePath) },
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_parentPathID) }
};

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  int AddGame(const CStdString& strFilenameAndPath);

  void UpdateGameTitle(int idGame, const CStdString& strNewGameTitle, PROGRAMDB_CONTENT_TYPE iType=PROGRAMDB_CONTENT_GAMES);

  bool HasGameInfo(const CStdString& strFilenameAndPath);

  void GetFilePathById(int idGame, CStdString &filePath, PROGRAMDB_CONTENT_TYPE iType);
  CStdString GetDeveloperById(int id);
  CStdString GetPublisherById(int id);
  CStdString GetGenreById(int id);

  bool LoadProgramInfo(const CStdString& strFilenameAndPath, CProgramInfoTag& details);
  void GetGameInfo(const CStdString& strFilenameAndPath, CProgramInfoTag& details, int idGame = -1);

  int GetPathId(const CStdString& strPath);

  int SetDetailsForGame(const CStdString& strFilenameAndPath, const CProgramInfoTag& details, int idGame = -1);
  void SetDetail(const CStdString& strDetail, int id, int field, PROGRAMDB_CONTENT_TYPE type);

  void DeleteGame(int idGame, bool bKeepId = false, bool bKeepThumb = false);
  void DeleteGame(const CStdString& strFilenameAndPath, bool bKeepId = false, bool bKeepThumb = false, int idGame = -1);
  void RemoveContentForPath(const CStdString& strPath,CGUIDialogProgress *progress = NULL);
  void UpdateFanart(const CFileItem &item, PROGRAMDB_CONTENT_TYPE type);

  // scraper settings
  void SetScraperForPath(const CStdString& filePath, const ADDON::ScraperPtr& info, const PROGRAM::SScanSettings& settings);
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath);
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath, PROGRAM::SScanSettings& settings);

  /*! \brief Retrieve the scraper and settings we should use for the specified path
   If the scraper is not set on this particular path, we'll recursively check parent folders.
   \param strPath path to start searching in.
   \param settings [out] scan settings for this folder.
   \param foundDirectly [out] true if a scraper was found directly for strPath, false if it was in a parent path.
   \return A ScraperPtr containing the scraper information. Returns NULL if a trivial (Content == CONTENT_NONE)
           scraper or no scraper is found.
   */
  ADDON::ScraperPtr GetScraperForPath(const CStdString& strPath, PROGRAM::SScanSettings& settings, bool& foundDirectly);
  /*! \brief Retrieve the content type of programs in the given path
   If content is set on the folder, we return the given content type
   Note that any subfolders in games will be treated as games.
   \param strPath path to start searching in.
   \return A content type string for the current path.
   */
  CStdString GetContentForPath(const CStdString& strPath);

  /*! \brief Get a program of the given content type from the given path, if it exists
   \param content the content type to fetch.
   \param path the path to fetch a program from.
   \param item the returned item.
   \return true if an item is found, false otherwise.
   */
  bool GetItemForPath(const CStdString &content, const CStdString &path, CFileItem &item);

  /*! \brief Get programs of the given content type from the given path
   \param content the content type to fetch.
   \param path the path to fetch programs from.
   \param items the returned items
   \return true if items are found, false otherwise.
   */
  bool GetItemsForPath(const CStdString &content, const CStdString &path, CFileItemList &items);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const CStdString &scraperID) const;

  // scanning hashes and paths scanned
  bool SetPathHash(const CStdString &path, const CStdString &hash);
  bool GetPathHash(const CStdString &path, CStdString &hash);
  bool GetPaths(std::set<CStdString> &paths);

  /*! \brief retrieve subpaths of a given path.  Assumes a heirarchical folder structure
   \param basepath the root path to retrieve subpaths for
   \param subpaths the returned subpaths
   \return true if we successfully retrieve subpaths (may be zero), false on error
   */
  bool GetSubPaths(const CStdString& basepath, std::vector<int>& subpaths);

  // general browsing
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetDevelopersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetPublishersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent=-1, const Filter &filter = Filter());

  bool GetGamesNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idCountry=-1, int idSet=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription());

  bool GetRecentlyAddedGamesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit=0);

  bool HasContent();
  bool HasContent(PROGRAMDB_CONTENT_TYPE type);

  /*! \brief Add a file to the database, if necessary
   If the file is already in the database, we simply return its id.
   \param url - full path of the file to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CStdString& url);

  /*! \brief Add a file to the database, if necessary
   Works for both programdb:// items and normal fileitems
   \param item CFileItem to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CFileItem& item);

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const CStdString& strPath, const CStdString &strDateAdded = "");

  // smart playlists and main retrieval work in these functions
  bool GetGamesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription());

  static void ProgramContentTypeToString(PROGRAMDB_CONTENT_TYPE type, CStdString& out)
  {
    switch (type)
    {
      case PROGRAMDB_CONTENT_GAMES:
        out = "game";
        break;
      default:
        break;
    }
  }

  bool AddTrainer(int iTitleId, const CStdString& strText);
  bool RemoveTrainer(const CStdString& strText);
  bool GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers);
  bool GetAllTrainers(std::vector<CStdString>& vecTrainers);
  bool SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  bool GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  void SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive);
  CStdString GetActiveTrainer(unsigned int iTitleId);
  bool HasTrainer(const CStdString& strTrainerPath);
  bool ItemHasTrainer(unsigned int iTitleId);

  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=-1);

  int GetTitleId(const CStdString& strFilenameAndPath);
  bool SetTitleId(const CStdString& strFilenameAndPath, int idTitle);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  bool GetXBEPathByTitleId(const int idTitle, CStdString& strPathAndFilename);

  int GetProgramInfo(CFileItem *item);
  bool AddProgramInfo(CFileItem *item, unsigned int titleID);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);

protected:
  int GetGameId(const CStdString& strFilenameAndPath);

  /*! \brief Get the id of a file from path
   \param url full path to the file
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const CStdString& url);

  /*! \brief Updates the dateAdded field in the files table for the file
   with the given idFile and the given path based on the files modification date
   \param idFile id of the file in the files table
   \param strFileNameAndPath path to the file
   */
  void UpdateFileDateAdded(int idFile, const CStdString& strFileNameAndPath);

  int AddToTable(const CStdString& table, const CStdString& firstField, const CStdString& secondField, const CStdString& value);
  int AddDeveloper(const CStdString& strDeveloper);
  int AddPublisher(const CStdString& strPublisher);
  int AddGenre(const CStdString& strGenre);

  // link functions - these two do all the work
  void AddToLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField = NULL, const char *type = NULL);
  void RemoveFromLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField = NULL, const char *type = NULL);

  void AddDeveloperToGame(int idGame, int idDeveloper);
  void AddPublisherToGame(int idGame, int idPublisher);
  void AddGenreToGame(int idGame, int idGenre);

  void AddGenreAndDevelopersAndPublishers(const CProgramInfoTag& details, std::vector<int>& vecDevelopers, std::vector<int>& vecGenres, std::vector<int>& vecPublishers);

  CProgramInfoTag GetDetailsByTypeAndId(PROGRAMDB_CONTENT_TYPE type, int id);
  CProgramInfoTag GetDetailsForGame(std::auto_ptr<dbiplus::Dataset> &pDS, bool needsCast = false);
  CProgramInfoTag GetDetailsForGame(const dbiplus::sql_record* const record, bool needsCast = false);
  bool GetNavCommon(const CStdString& strBaseDir, CFileItemList& items, const CStdString& type, int idContent=-1, const Filter &filter = Filter(), bool countOnly = false);

  void GetDetailsFromDB(std::auto_ptr<dbiplus::Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset = 2);
  void GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset = 2);
  CStdString GetValueString(const CProgramInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;
private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

  /*! \brief Run a query on the main dataset and return the number of rows
   If no rows are found we close the dataset and return 0.
   \param sql the sql query to run
   \return the number of rows, -1 for an error.
   */
  int RunQuery(const CStdString &sql);

  /*! \brief (Re)Create the generic database views for games, emulators,
     and homebrew
   */
  void CreateViews();

  /*! \brief Determine whether the path is using lookup using folders
   \param path the path to check
   \param shows whether this path is from a tvshow (defaults to false)
   */
  bool LookupByFolders(const CStdString &path, bool shows = false);

  virtual int GetMinVersion() const { return 3; };
  const char *GetDefaultDBName() const { return "MyPrograms6"; };

  void ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName);
  void SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  void InvalidatePathHash(const CStdString& strPath);
  void DeleteThumbForItem(const CStdString& strPath, bool bFolder);

  FILETIME TimeStampToLocalTime( unsigned __int64 timeStamp );
};
