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

#include "system.h"
#include "ProgramDatabase.h"
#include "addons/AddonManager.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "windows/GUIWindowFileManager.h"
#include "filesystem/MultiPathDirectory.h"
#include "programs/ProgramInfoScanner.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "utils/Crc32.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "DateTime.h"
#include "programs/ProgramDbUrl.h"
#include "SmartPlaylist.h"

using namespace std;
using namespace dbiplus;
using namespace XFILE;
using namespace PROGRAM;
using namespace ADDON;

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{

}

//********************************************************************************************************************************
bool CProgramDatabase::Open()
{
  return CDatabase::Open();
}

bool CProgramDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create developer table");
    m_pDS->exec("CREATE TABLE developer ( idDeveloper integer primary key, strDeveloper text)\n");

    CLog::Log(LOGINFO, "create publisher table");
    m_pDS->exec("CREATE TABLE publisher ( idPublisher integer primary key, strPublisher text)\n");

    CLog::Log(LOGINFO, "create descriptor table");
    m_pDS->exec("CREATE TABLE descriptor ( idDescriptor integer primary key, strDescriptor text)\n");

    CLog::Log(LOGINFO, "create generalfeature table");
    m_pDS->exec("CREATE TABLE generalfeature ( idGeneralFeature integer primary key, strGeneralFeature text)\n");

    CLog::Log(LOGINFO, "create onlinefeature table");
    m_pDS->exec("CREATE TABLE onlinefeature ( idOnlineFeature integer primary key, strOnlineFeature text)\n");

    CLog::Log(LOGINFO, "create platform table");
    m_pDS->exec("CREATE TABLE platform ( idPlatform integer primary key, strPlatform text)\n");

    CLog::Log(LOGINFO, "create game table");
    CStdString columns = "CREATE TABLE game ( idGame integer primary key, idFile integer";
    for (int i = 0; i < PROGRAMDB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ")";
    m_pDS->exec(columns.c_str());
    m_pDS->exec("CREATE UNIQUE INDEX ix_game_file_1 ON game (idFile, idGame)");
    m_pDS->exec("CREATE UNIQUE INDEX ix_game_file_2 ON game (idGame, idFile)");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool, exclude bool, dateAdded text)");
    m_pDS->exec("CREATE UNIQUE INDEX ix_path ON path ( strPath(255) )");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId integer, xbedescription text, playCount integer, iTimesPlayed integer, lastPlayed text, lastAccessed integer, iRegion integer, iSize integer, dateAdded text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_files ON files ( idPath, strFilename(255) )");
    m_pDS->exec("CREATE INDEX idx_title_id_files ON files(titleId)");

    CLog::Log(LOGINFO, "create genrelinkgame table");
    m_pDS->exec("CREATE TABLE genrelinkgame ( idGenre integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkgame_1 ON genrelinkgame ( idGenre, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkgame_2 ON genrelinkgame ( idGame, idGenre)\n");

    CLog::Log(LOGINFO, "create developerlinkgame table");
    m_pDS->exec("CREATE TABLE developerlinkgame ( idDeveloper integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_developerlinkgame_1 ON developerlinkgame ( idDeveloper, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_developerlinkgame_2 ON developerlinkgame ( idGame, idDeveloper)\n");

    CLog::Log(LOGINFO, "create publisherlinkgame table");
    m_pDS->exec("CREATE TABLE publisherlinkgame ( idPublisher integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_publisherlinkgame_1 ON publisherlinkgame ( idPublisher, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_publisherlinkgame_2 ON publisherlinkgame ( idGame, idPublisher)\n");

    CLog::Log(LOGINFO, "create descriptorlinkgame table");
    m_pDS->exec("CREATE TABLE descriptorlinkgame ( idDescriptor integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_descriptorlinkgame_1 ON descriptorlinkgame ( idDescriptor, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_descriptorlinkgame_2 ON descriptorlinkgame ( idGame, idDescriptor)\n");

    CLog::Log(LOGINFO, "create generalfeaturelinkgame table");
    m_pDS->exec("CREATE TABLE generalfeaturelinkgame ( idGeneralFeature integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_generalfeaturelinkgame_1 ON generalfeaturelinkgame ( idGeneralFeature, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_generalfeaturelinkgame_2 ON generalfeaturelinkgame ( idGame, idGeneralFeature)\n");

    CLog::Log(LOGINFO, "create onlinefeaturelinkgame table");
    m_pDS->exec("CREATE TABLE onlinefeaturelinkgame ( idOnlineFeature integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_onlinefeaturelinkgame_1 ON onlinefeaturelinkgame ( idOnlineFeature, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_onlinefeaturelinkgame_2 ON onlinefeaturelinkgame ( idGame, idOnlineFeature)\n");

    CLog::Log(LOGINFO, "create platformlinkgame table");
    m_pDS->exec("CREATE TABLE platformlinkgame ( idPlatform integer, idGame integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_platformlinkgame_1 ON platformlinkgame ( idPlatform, idGame)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_platformlinkgame_2 ON platformlinkgame ( idGame, idPlatform)\n");

    CLog::Log(LOGINFO, "create trainers table");
    m_pDS->exec("CREATE TABLE trainers (idKey integer auto_increment primary key, idCRC integer, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");

    // we create views last to ensure all indexes are rolled in
    CreateViews();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase::unable to create tables:%lu", GetLastError());
    return false;
  }

  return true;
}

void CProgramDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create gameview");
  try
  {
    m_pDS->exec("DROP VIEW gameview");
  }
  catch (...) {}
  m_pDS->exec("CREATE VIEW gameview AS SELECT"
              "  game.*,"
              "  files.strFileName AS strFileName,"
              "  path.strPath AS strPath, "
              "  files.playCount AS playCount,"
              "  files.lastPlayed AS lastPlayed,"
              "  files.dateAdded AS dateAdded "
              "FROM game"
              "  JOIN files ON"
              "    files.idFile=game.idFile"
              "  JOIN path ON"
              "    path.idPath=files.idPath");
}

void CProgramDatabase::RemoveContentForPath(const CStdString& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  // TODO: implement this
}

void CProgramDatabase::SetScraperForPath(const CStdString& filePath, const ScraperPtr& scraper, const PROGRAM::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths too
  if(URIUtils::IsMultiPath(filePath))
  {
    vector<CStdString> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],scraper,settings);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    int idPath = AddPath(filePath);
    if (idPath < 0)
      return;

    // Update
    CStdString strSQL;
    if (settings.exclude)
    { //NB See note in ::GetScraperForPath about strContent=='none'
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=1, exclude=0 where idPath=%i", idPath);
    }
    else if(!scraper)
    { // catch clearing content, but not excluding
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0, exclude=1 where idPath=%i", idPath);
    }
    else
    {
      CStdString content = TranslateContent(scraper->Content());
      strSQL=PrepareSQL("update path set strContent='%s', strScraper='%s', scanRecursive=%i, useFolderNames=%i, strSettings='%s', noUpdate=%i, exclude=0 where idPath=%i", content.c_str(), scraper->ID().c_str(),settings.recurse,settings.parent_name,scraper->GetPathSettings().c_str(),settings.noupdate, idPath);
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

bool CProgramDatabase::ScraperInUse(const CStdString &scraperID) const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("select count(1) from path where strScraper='%s'", scraperID.c_str());
    if (!m_pDS->query(sql.c_str()) || m_pDS->num_rows() == 0)
      return false;
    bool found = m_pDS->fv(0).get_asInt() > 0;
    m_pDS->close();
    return found;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, scraperID.c_str());
  }
  return false;
}

bool CProgramDatabase::UpdateOldVersion(int version)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

bool CProgramDatabase::LookupByFolders(const CStdString &path, bool shows)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(path, settings, foundDirectly);
  return settings.parent_name_root; // games
}

//********************************************************************************************************************************
int CProgramDatabase::GetPathId(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    int idPath=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (URIUtils::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      idPath = m_pDS->fv("path.idPath").get_asInt();

    m_pDS->close();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetSubPaths(const CStdString &basepath, vector<int>& subpaths)
{
  CStdString sql;
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    sql = PrepareSQL("SELECT idPath FROM path WHERE strPath LIKE '%s%%'", basepath.c_str());
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      subpaths.push_back(m_pDS->fv(0).get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

int CProgramDatabase::RunQuery(const CStdString &sql)
{
  unsigned int time = CTimeUtils::GetTimeMS();
  int rows = -1;
  if (m_pDS->query(sql.c_str()))
  {
    rows = m_pDS->num_rows();
    if (rows == 0)
      m_pDS->close();
  }
  CLog::Log(LOGDEBUG, "%s took %d ms for %d items query: %s", __FUNCTION__, CTimeUtils::GetTimeMS() - time, rows, sql.c_str());
  return rows;
}

int CProgramDatabase::AddPath(const CStdString& strPath, const CStdString &strDateAdded /*= "" */)
{
  CStdString strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (URIUtils::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

        // only set dateadded if we got one
    if (!strDateAdded.empty())
      strSQL=PrepareSQL("insert into path (idPath, strPath, strContent, strScraper, dateAdded) values (NULL,'%s','','', '%s')", strPath1.c_str(), strDateAdded.c_str());
    else
      strSQL=PrepareSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
    m_pDS->exec(strSQL.c_str());
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetPathHash(const CStdString &path, CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select strHash from path where strPath like '%s'", path.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
      return false;
    hash = m_pDS->fv("strHash").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }

  return false;
}

//********************************************************************************************************************************
int CProgramDatabase::AddFile(const CStdString& strFileNameAndPath)
{
  CStdString strSQL = "";
  try
  {
    int idFile;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strFileName, strPath;
    SplitPath(strFileNameAndPath,strPath,strFileName);

    int idPath = AddPath(strPath);
    if (idPath < 0)
      return -1;

    CStdString strSQL=PrepareSQL("select idFile from files where strFileName like '%s' and idPath=%i", strFileName.c_str(),idPath);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      idFile = m_pDS->fv("idFile").get_asInt() ;
      m_pDS->close();
      return idFile;
    }
    m_pDS->close();
    strSQL=PrepareSQL("insert into files (idFile, idPath, strFileName) values(NULL, %i, '%s')", idPath, strFileName.c_str());
    m_pDS->exec(strSQL.c_str());
    idFile = (int)m_pDS->lastinsertid();
    return idFile;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addfile (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

int CProgramDatabase::AddFile(const CFileItem& item)
{
  if (item.IsProgramDb() && item.HasProgramInfoTag())
    return AddFile(item.GetProgramInfoTag()->m_strFileNameAndPath);
  return AddFile(item.GetPath());
}

void CProgramDatabase::UpdateFileDateAdded(int idFile, const CStdString& strFileNameAndPath)
{
  if (idFile < 0 || strFileNameAndPath.empty())
    return;

  CStdString strSQL = "";
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CDateTime dateAdded;
    // Let's try to get the modification datetime
    struct __stat64 buffer;
    if (CFile::Stat(strFileNameAndPath, &buffer) == 0)
      dateAdded = *localtime((const time_t*)&buffer.st_mtime);

    if (!dateAdded.IsValid())
      dateAdded = CDateTime::GetCurrentDateTime();

    strSQL = PrepareSQL("update files set dateAdded='%s' where idFile=%d", dateAdded.GetAsDBDateTime().c_str(), idFile);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to update dateadded for file (%s)", __FUNCTION__, strSQL.c_str());
  }
}

bool CProgramDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (hash.IsEmpty())
    { // this is an empty folder - we need only add it to the path table
      // if the path actually exists
      if (!CDirectory::Exists(path))
        return false;
    }
    int idPath = AddPath(path);
    if (idPath < 0) return false;

    CStdString strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

//********************************************************************************************************************************
int CProgramDatabase::GetFileId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);

    int idPath = GetPathId(strPath);
    if (idPath >= 0)
    {
      CStdString strSQL;
      strSQL=PrepareSQL("select idFile from files where strFileName like '%s' and idPath=%i", strFileName.c_str(),idPath);
      m_pDS->query(strSQL.c_str());
      if (m_pDS->num_rows() > 0)
      {
        int idFile = m_pDS->fv("files.idFile").get_asInt();
        m_pDS->close();
        return idFile;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetPaths(set<CStdString> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();

    // grab all paths with game content set
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where (strContent = 'games')"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

//********************************************************************************************************************************
int CProgramDatabase::GetGameId(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idGame = -1;

    // needed for query parameters
    int idFile = GetFileId(strFilenameAndPath);
    int idPath = -1;
    CStdString strPath;
    if (idFile < 0)
    {
      CStdString strFile;
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join gameinfo table for correct results
      idPath = GetPathId(strPath);
      if (idPath < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (idFile == -1 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (idFile == -1)
      strSQL=PrepareSQL("select idGame from game join files on files.idFile=game.idFile where files.idpath = %i",idPath);
    else
      strSQL=PrepareSQL("select idGame from game where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      idGame = m_pDS->fv("idGame").get_asInt();
    m_pDS->close();

    return idGame;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CProgramDatabase::AddGame(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idGame = GetGameId(strFilenameAndPath);
    if (idGame < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      CStdString strSQL=PrepareSQL("insert into game (idGame, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL.c_str());
      idGame = (int)m_pDS->lastinsertid();
    }

    return idGame;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CProgramDatabase::AddToTable(const CStdString& table, const CStdString& firstField, const CStdString& secondField, const CStdString& value)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = PrepareSQL("select %s from %s where %s like '%s'", firstField.c_str(), table.c_str(), secondField.c_str(), value.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL = PrepareSQL("insert into %s (%s, %s) values( NULL, '%s')", table.c_str(), firstField.c_str(), secondField.c_str(), value.c_str());
      m_pDS->exec(strSQL.c_str());
      int id = (int)m_pDS->lastinsertid();
      return id;
    }
    else
    {
      int id = m_pDS->fv(firstField).get_asInt();
      m_pDS->close();
      return id;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, value.c_str() );
  }

  return -1;
}

int CProgramDatabase::AddDeveloper(const CStdString& strDeveloper)
{
  return AddToTable("developer", "idDeveloper", "strDeveloper", strDeveloper);
}

int CProgramDatabase::AddPublisher(const CStdString& strPublisher)
{
  return AddToTable("publisher", "idPublisher", "strPublisher", strPublisher);
}

int CProgramDatabase::AddGenre(const CStdString& strGenre)
{
  return AddToTable("genre", "idGenre", "strGenre", strGenre);
}

int CProgramDatabase::AddDescriptor(const CStdString& strDescriptor)
{
  return AddToTable("descriptor", "idDescriptor", "strDescriptor", strDescriptor);
}

int CProgramDatabase::AddGeneralFeature(const CStdString& strGeneralFeature)
{
  return AddToTable("generalfeature", "idGeneralFeature", "strGeneralFeature", strGeneralFeature);
}

int CProgramDatabase::AddOnlineFeature(const CStdString& strOnlineFeature)
{
  return AddToTable("onlinefeature", "idOnlineFeature", "strOnlineFeature", strOnlineFeature);
}

int CProgramDatabase::AddPlatform(const CStdString& strPlatform)
{
  return AddToTable("platform", "idPlatform", "strPlatform", strPlatform);
}

void CProgramDatabase::AddToLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField /* = NULL */, const char *type /* = NULL */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL = PrepareSQL("select * from %s where %s=%i and %s=%i", table, firstField, firstID, secondField, secondID);
    if (typeField != NULL && type != NULL)
      strSQL += PrepareSQL(" and %s='%s'", typeField, type);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      if (typeField == NULL || type == NULL)
        strSQL = PrepareSQL("insert into %s (%s,%s) values(%i,%i)", table, firstField, secondField, firstID, secondID);
      else
        strSQL = PrepareSQL("insert into %s (%s,%s,%s) values(%i,%i,'%s')", table, firstField, secondField, typeField, firstID, secondID, type);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CProgramDatabase::RemoveFromLinkTable(const char *table, const char *firstField, int firstID, const char *secondField, int secondID, const char *typeField /* = NULL */, const char *type /* = NULL */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL = PrepareSQL("DELETE FROM %s WHERE %s = %i AND %s = %i", table, firstField, firstID, secondField, secondID);
    if (typeField != NULL && type != NULL)
      strSQL += PrepareSQL(" AND %s='%s'", typeField, type);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

//****Developers****
void CProgramDatabase::AddDeveloperToGame(int idGame, int idDeveloper)
{
  AddToLinkTable("developerlinkgame", "idDeveloper", idDeveloper, "idGame", idGame);
}

//****Publishers****
void CProgramDatabase::AddPublisherToGame(int idGame, int idPublisher)
{
  AddToLinkTable("publisherlinkgame", "idPublisher", idPublisher, "idGame", idGame);
}

//****Genres****
void CProgramDatabase::AddGenreToGame(int idGame, int idGenre)
{
  AddToLinkTable("genrelinkgame", "idGenre", idGenre, "idGame", idGame);
}

//****Descriptors****
void CProgramDatabase::AddDescriptorToGame(int idGame, int idDescriptor)
{
  AddToLinkTable("descriptorlinkgame", "idDescriptor", idDescriptor, "idGame", idGame);
}

//****General features****
void CProgramDatabase::AddGeneralFeatureToGame(int idGame, int idGeneralFeature)
{
  AddToLinkTable("generalfeaturelinkgame", "idGeneralFeature", idGeneralFeature, "idGame", idGame);
}

//****Online features****
void CProgramDatabase::AddOnlineFeatureToGame(int idGame, int idOnlineFeature)
{
  AddToLinkTable("onlinefeaturelinkgame", "idOnlineFeature", idOnlineFeature, "idGame", idGame);
}

//****Platforms****
void CProgramDatabase::AddPlatformToGame(int idGame, int idPlatform)
{
  AddToLinkTable("platformlinkgame", "idPlatform", idPlatform, "idGame", idGame);
}

void CProgramDatabase::ConstructPath(CStdString& strDest, const CStdString& strPath, const CStdString& strFileName)
{
  if (URIUtils::IsStack(strFileName) || URIUtils::IsInArchive(strFileName))
    strDest = strFileName;
  else
    URIUtils::AddFileToFolder(strPath, strFileName, strDest);
}

void CProgramDatabase::SplitPath(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  if (URIUtils::IsStack(strFileNameAndPath) || strFileNameAndPath.Mid(0,6).Equals("rar://") || strFileNameAndPath.Mid(0,6).Equals("zip://"))
  {
    URIUtils::GetParentPath(strFileNameAndPath,strPath);
    strFileName = strFileNameAndPath;
  }
  else
    URIUtils::Split(strFileNameAndPath,strPath, strFileName);
}

void CProgramDatabase::InvalidatePathHash(const CStdString& strPath)
{
  // TODO: implement this
}

bool CProgramDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so recalculate
    g_infoManager.SetLibraryBool(LIBRARY_HAS_GAMES, HasContent(PROGRAMDB_CONTENT_GAMES));
    return true;
  }
  return false;
}

void CProgramDatabase::DeleteThumbForItem(const CStdString& strPath, bool bFolder)
{
  // TODO: implement this
}

void CProgramDatabase::SetDetail(const CStdString& strDetail, int id, int field,
                               PROGRAMDB_CONTENT_TYPE type)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strTable, strField;
    if (type == PROGRAMDB_CONTENT_GAMES)
    {
      strTable = "game";
      strField = "idGame";
    }

    if (strTable.IsEmpty())
      return;

    CStdString strSQL = PrepareSQL("update %s set c%02u='%s' where %s=%u",
                                  strTable.c_str(), field, strDetail.c_str(), strField.c_str(), id);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CProgramDatabase::UpdateFanart(const CFileItem &item, PROGRAMDB_CONTENT_TYPE type)
{
  if (NULL == m_pDB.get()) return;
  if (NULL == m_pDS.get()) return;
  if (!item.HasProgramInfoTag() || item.GetProgramInfoTag()->m_iDbId < 0) return;

  CStdString exec;
  if (type == PROGRAMDB_CONTENT_GAMES)
    exec = PrepareSQL("UPDATE game set c%02d='%s' WHERE idgame=%i", PROGRAMDB_ID_FANART, item.GetProgramInfoTag()->m_fanart.m_xml.c_str(), item.GetProgramInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - error updating fanart for %s", __FUNCTION__, item.GetPath().c_str());
  }
}

void CProgramDatabase::UpdateGameTitle(int idGame, const CStdString& strNewGameTitle, PROGRAMDB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString content;
    CStdString strSQL;
    if (iType == PROGRAMDB_CONTENT_GAMES)
    {
      CLog::Log(LOGINFO, "Changing Game:id:%i New Title:%s", idGame, strNewGameTitle.c_str());
      strSQL = PrepareSQL("UPDATE game SET c%02d='%s' WHERE idGame=%i", PROGRAMDB_ID_TITLE, strNewGameTitle.c_str(), idGame );
      content = "game";
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (int idGame, const CStdString& strNewGameTitle) failed on GameID:%i and Title:%s", __FUNCTION__, idGame, strNewGameTitle.c_str());
  }
}

bool CProgramDatabase::HasGameInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idGame = GetGameId(strFilenameAndPath);
    return (idGame > 0); // index of zero is also invalid

    // work in progress
    if (idGame > 0)
    {
      // get title.  if no title, the id was "deleted" for in-place update
      CProgramInfoTag details;
      GetGameInfo(strFilenameAndPath, details, idGame);
      if (!details.m_strTitle.IsEmpty()) return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
void CProgramDatabase::GetFilePathById(int idGame, CStdString &filePath, PROGRAMDB_CONTENT_TYPE iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (idGame < 0) return ;

    CStdString strSQL;
    if (iType == PROGRAMDB_CONTENT_GAMES)
      strSQL=PrepareSQL("select path.strPath,files.strFileName from path, files, game where path.idPath=files.idPath and files.idFile=game.idFile and game.idGame=%i order by strFilename", idGame );

    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
      filePath = m_pDS->fv("path.strPath").get_asString();
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

CStdString CProgramDatabase::GetDeveloperById(int id)
{
  return GetSingleValue("developer", "strDeveloper", PrepareSQL("idDeloper=%i", id));
}

CStdString CProgramDatabase::GetPublisherById(int id)
{
  return GetSingleValue("publisher", "strPublisher", PrepareSQL("idPublisher=%i", id));
}

CStdString CProgramDatabase::GetGenreById(int id)
{
  return GetSingleValue("genre", "strGenre", PrepareSQL("idGenre=%i", id));
}

CStdString CProgramDatabase::GetDescriptorById(int id)
{
  return GetSingleValue("descriptor", "strDescriptor", PrepareSQL("idDescriptor=%i", id));
}

CStdString CProgramDatabase::GetGeneralFeatureById(int id)
{
  return GetSingleValue("generalfeature", "strGeneralFeature", PrepareSQL("idGeneralFeature=%i", id));
}

CStdString CProgramDatabase::GetOnlineFeatureById(int id)
{
  return GetSingleValue("onlinefeature", "strOnlineFeature", PrepareSQL("idOnlineFeature=%i", id));
}

CStdString CProgramDatabase::GetPlatformById(int id)
{
  return GetSingleValue("platform", "strPlatform", PrepareSQL("idPlatform=%i", id));
}

//********************************************************************************************************************************
bool CProgramDatabase::LoadProgramInfo(const CStdString& strFilenameAndPath, CProgramInfoTag& details)
{
  if (HasGameInfo(strFilenameAndPath))
  {
    GetGameInfo(strFilenameAndPath, details);
    CLog::Log(LOGDEBUG,"%s, got game info!", __FUNCTION__);
    CLog::Log(LOGDEBUG,"  Title = %s", details.m_strTitle.c_str());
  }

  return !details.IsEmpty();
}

//********************************************************************************************************************************
void CProgramDatabase::GetGameInfo(const CStdString& strFilenameAndPath, CProgramInfoTag& details, int idGame /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (idGame < 0)
      idGame = GetGameId(strFilenameAndPath);
    if (idGame < 0) return ;

    CStdString sql = PrepareSQL("select * from gameview where idGame=%i", idGame);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForGame(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CProgramDatabase::AddGenreAndDevelopersAndPublishers(const CProgramInfoTag& details, vector<int>& vecDevelopers, vector<int>& vecPublishers, vector<int>& vecGenres,
                                                          vector<int>& vecDescriptors, vector<int>& vecGeneralFeatures, vector<int>& vecOnlineFeatures, vector<int>& vecPlatforms)
{
  // add all developers
  for (unsigned int i = 0; i < details.m_developer.size(); i++)
    vecDevelopers.push_back(AddDeveloper(details.m_developer[i]));

  // add all publishers
  for (unsigned int i = 0; i < details.m_publisher.size(); i++)
    vecPublishers.push_back(AddPublisher(details.m_publisher[i]));

  // add all genres
  for (unsigned int i = 0; i < details.m_genre.size(); i++)
    vecGenres.push_back(AddGenre(details.m_genre[i]));

  // add all descriptors
  for (unsigned int i = 0; i < details.m_descriptor.size(); i++)
    vecDescriptors.push_back(AddDescriptor(details.m_descriptor[i]));

  // add all general features
  for (unsigned int i = 0; i < details.m_generalFeature.size(); i++)
    vecGeneralFeatures.push_back(AddGeneralFeature(details.m_generalFeature[i]));

  // add all online features
  for (unsigned int i = 0; i < details.m_onlineFeature.size(); i++)
    vecOnlineFeatures.push_back(AddOnlineFeature(details.m_onlineFeature[i]));

  // add all platforms
  for (unsigned int i = 0; i < details.m_platform.size(); i++)
    vecPlatforms.push_back(AddPlatform(details.m_platform[i]));
}

CStdString CProgramDatabase::GetValueString(const CProgramInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const
{
  CStdString sql;
  for (int i = min + 1; i < max; ++i)
  {
    switch (offsets[i].type)
    {
    case PROGRAMDB_TYPE_STRING:
      sql += PrepareSQL("c%02d='%s',", i, ((CStdString*)(((char*)&details)+offsets[i].offset))->c_str());
      break;
    case PROGRAMDB_TYPE_INT:
      sql += PrepareSQL("c%02d='%i',", i, *(int*)(((char*)&details)+offsets[i].offset));
      break;
    case PROGRAMDB_TYPE_COUNT:
      {
        int value = *(int*)(((char*)&details)+offsets[i].offset);
        if (value)
          sql += PrepareSQL("c%02d=%i,", i, value);
        else
          sql += PrepareSQL("c%02d=NULL,", i);
      }
      break;
    case PROGRAMDB_TYPE_BOOL:
      sql += PrepareSQL("c%02d='%s',", i, *(bool*)(((char*)&details)+offsets[i].offset)?"true":"false");
      break;
    case PROGRAMDB_TYPE_FLOAT:
      sql += PrepareSQL("c%02d='%f',", i, *(float*)(((char*)&details)+offsets[i].offset));
      break;
    case PROGRAMDB_TYPE_STRINGARRAY:
      sql += PrepareSQL("c%02d='%s',", i, StringUtils::Join(*((std::vector<std::string>*)(((char*)&details)+offsets[i].offset)), g_advancedSettings.m_programItemSeparator).c_str());
      break;
    case PROGRAMDB_TYPE_DATE:
      sql += PrepareSQL("c%02d='%s',", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDate().c_str());
      break;
    case PROGRAMDB_TYPE_DATETIME:
      sql += PrepareSQL("c%02d='%s',", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDateTime().c_str());
      break;
    }
  }
  sql.TrimRight(',');
  return sql;
}

//********************************************************************************************************************************
int CProgramDatabase::SetDetailsForGame(const CStdString& strFilenameAndPath, const CProgramInfoTag& details, int idGame /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idGame < 0)
    {
      idGame = GetGameId(strFilenameAndPath);
      if (idGame > -1)
        DeleteGame(strFilenameAndPath, true, true, idGame); // true to keep the table entry and the thumb
      else
      {
        // only add a new game if we don't already have a valid idGame
        // (DeleteGame is called with bKeepId == true so the game won't
        // be removed from the game table)
        idGame = AddGame(strFilenameAndPath);
        if (idGame < 0)
        {
          CommitTransaction();
          return idGame;
        }
      }
    }

    vector<int> vecDevelopers;
    vector<int> vecPublishers;
    vector<int> vecGenres;
    vector<int> vecDescriptors;
    vector<int> vecGeneralFeatures;
    vector<int> vecOnlineFeatures;
    vector<int> vecPlatforms;
    AddGenreAndDevelopersAndPublishers(details,vecDevelopers,vecPublishers,vecGenres,vecDescriptors,vecGeneralFeatures,vecOnlineFeatures,vecPlatforms);

    for (unsigned int i = 0; i < vecDevelopers.size(); ++i)
      AddDeveloperToGame(idGame, vecDevelopers[i]);

    for (unsigned int i = 0; i < vecPublishers.size(); ++i)
      AddPublisherToGame(idGame, vecPublishers[i]);

    for (unsigned int i = 0; i < vecGenres.size(); ++i)
      AddGenreToGame(idGame, vecGenres[i]);

    for (unsigned int i = 0; i < vecDescriptors.size(); ++i)
      AddDescriptorToGame(idGame, vecDescriptors[i]);

    for (unsigned int i = 0; i < vecGeneralFeatures.size(); ++i)
      AddGeneralFeatureToGame(idGame, vecGeneralFeatures[i]);

    for (unsigned int i = 0; i < vecOnlineFeatures.size(); ++i)
      AddOnlineFeatureToGame(idGame, vecOnlineFeatures[i]);

    for (unsigned int i = 0; i < vecPlatforms.size(); ++i)
      AddPlatformToGame(idGame, vecPlatforms[i]);

    // update our game table (we know it was added already above)
    // and insert the new row
    CStdString sql = "update game set " + GetValueString(details, PROGRAMDB_ID_MIN, PROGRAMDB_ID_MAX, DbGameOffsets);
    sql += PrepareSQL(" where idGame=%i", idGame);
    m_pDS->exec(sql.c_str());
    CommitTransaction();
    return idGame;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
void CProgramDatabase::DeleteGame(int idGame, bool bKeepId /* = false */, bool bKeepThumb /* = false */)
{
  if (idGame < 0)
    return;

  CStdString path;
  GetFilePathById(idGame, path, PROGRAMDB_CONTENT_GAMES);
  if (!path.empty())
    DeleteGame(path, bKeepId, bKeepThumb, idGame);
}

void CProgramDatabase::DeleteGame(const CStdString& strFilenameAndPath, bool bKeepId /* = false */, bool bKeepThumb /* = false */, int idGame /* = -1 */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (idGame < 0)
    {
      idGame = GetGameId(strFilenameAndPath);
      if (idGame < 0)
        return;
    }

    BeginTransaction();

    CStdString strSQL;
    strSQL=PrepareSQL("delete from developerlinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from publisherlinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from genrelinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from descriptorlinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from generalfeaturelinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from onlinefeaturelinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    strSQL=PrepareSQL("delete from platformlinkgame where idgame=%i", idGame);
    m_pDS->exec(strSQL.c_str());

    if (!bKeepThumb)
      DeleteThumbForItem(strFilenameAndPath,false);

    // keep the game table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from game where idgame=%i", idGame);
      m_pDS->exec(strSQL.c_str());
    }

    CStdString strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);
    InvalidatePathHash(strPath);
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CProgramDatabase::GetDetailsFromDB(auto_ptr<Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset)
{
  GetDetailsFromDB(pDS->get_sql_record(), min, max, offsets, details, idxOffset);
}

void CProgramDatabase::GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset)
{
  for (int i = min + 1; i < max; i++)
  {
    switch (offsets[i].type)
    {
    case PROGRAMDB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asString();
      break;
    case PROGRAMDB_TYPE_INT:
    case PROGRAMDB_TYPE_COUNT:
      *(int*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asInt();
      break;
    case PROGRAMDB_TYPE_BOOL:
      *(bool*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asBool();
      break;
    case PROGRAMDB_TYPE_FLOAT:
      *(float*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asFloat();
      break;
    case PROGRAMDB_TYPE_STRINGARRAY:
      *(std::vector<std::string>*)(((char*)&details)+offsets[i].offset) = StringUtils::Split(record->at(i+idxOffset).get_asString(), g_advancedSettings.m_programItemSeparator);
      break;
    case PROGRAMDB_TYPE_DATE:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDate(record->at(i+idxOffset).get_asString());
      break;
    case PROGRAMDB_TYPE_DATETIME:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDateTime(record->at(i+idxOffset).get_asString());
      break;
    }
  }
}

DWORD gameTime = 0;

CProgramInfoTag CProgramDatabase::GetDetailsForGame(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  return GetDetailsForGame(pDS->get_sql_record(), needsCast);
}

CProgramInfoTag CProgramDatabase::GetDetailsForGame(const dbiplus::sql_record* const record, bool needsCast /* = false */)
{
  CProgramInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = CTimeUtils::GetTimeMS();
  int idGame = record->at(0).get_asInt();

  GetDetailsFromDB(record, PROGRAMDB_ID_MIN, PROGRAMDB_ID_MAX, DbGameOffsets, details);

  details.m_iDbId = idGame;
  details.m_type = "game";

  details.m_iFileId = record->at(PROGRAMDB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(PROGRAMDB_DETAILS_GAME_PATH).get_asString();
  CStdString strFileName = record->at(PROGRAMDB_DETAILS_GAME_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = record->at(PROGRAMDB_DETAILS_GAME_PLAYCOUNT).get_asInt();
  details.m_lastPlayed.SetFromDBDateTime(record->at(PROGRAMDB_DETAILS_GAME_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(PROGRAMDB_DETAILS_GAME_DATEADDED).get_asString());

  gameTime += CTimeUtils::GetTimeMS() - time; time = CTimeUtils::GetTimeMS();

  return details;
}

bool CProgramDatabase::GetDevelopersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "developer", idContent, filter, countOnly);
}

bool CProgramDatabase::GetPublishersNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "publisher", idContent, filter, countOnly);
}

bool CProgramDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "genre", idContent, filter, countOnly);
}

bool CProgramDatabase::GetDescriptorsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "descriptor", idContent, filter, countOnly);
}

bool CProgramDatabase::GetGeneralFeaturesNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "generalfeature", idContent, filter, countOnly);
}

bool CProgramDatabase::GetOnlineFeaturesNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "onlinefeature", idContent, filter, countOnly);
}

bool CProgramDatabase::GetPlatformsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetNavCommon(strBaseDir, items, "platform", idContent, filter, countOnly);
}

bool CProgramDatabase::GetNavCommon(const CStdString& strBaseDir, CFileItemList& items, const CStdString &type, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    Filter extFilter = filter;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == PROGRAMDB_CONTENT_GAMES)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, path.strPath, files.playCount", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkgame on %s.id%s = %slinkgame.id%s join gameview on %slinkgame.idGame = gameview.idGame join files on files.idFile = gameview.idFile join path on path.idPath = files.idPath",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
      }
      else
        return false;
    }
    else
    {
      if (idContent == PROGRAMDB_CONTENT_GAMES)
      {
        strSQL = "select %s " + PrepareSQL("from %s ", type.c_str());
        extFilter.fields = PrepareSQL("%s.id%s, %s.str%s, count(1), count(files.playCount)", type.c_str(), type.c_str(), type.c_str(), type.c_str());
        extFilter.AppendJoin(PrepareSQL("join %slinkgame on %s.id%s = %slinkgame.id%s join gameview on %slinkgame.idGame = gameview.idGame join files on files.idFile = gameview.idFile",
                                        type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str(), type.c_str()));
        extFilter.AppendGroup(PrepareSQL("%s.id%s", type.c_str(), type.c_str()));
      }
      else
        return false;
    }

    if (countOnly)
    {
      extFilter.fields = PrepareSQL("COUNT(DISTINCT %s.id%s)", type.c_str(), type.c_str());
      extFilter.group.clear();
      extFilter.order.clear();
    }
    strSQL.Format(strSQL.c_str(), !extFilter.fields.empty() ? extFilter.fields.c_str() : "*");

    CProgramDbUrl programUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, programUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);

      m_pDS->close();
      return true;
    }

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapItems;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int id = m_pDS->fv(0).get_asInt();
        CStdString str = m_pDS->fv(1).get_asString();

        // was this already found?
        it = mapItems.find(id);
        if (it == mapItems.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv(2).get_asString()),g_settings.m_programSources))
          {
            if (idContent == PROGRAMDB_CONTENT_GAMES)
              mapItems.insert(pair<int, pair<CStdString,int> >(id, pair<CStdString,int>(str,m_pDS->fv(3).get_asInt()))); //fv(3) is file.playCount
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it = mapItems.begin(); it != mapItems.end(); ++it)
      {
        CFileItemPtr pItem(new CFileItem(it->second.first));
        pItem->GetProgramInfoTag()->m_iDbId = it->first;
        pItem->GetProgramInfoTag()->m_type = type;

        CProgramDbUrl itemUrl = programUrl;
        CStdString path; path.Format("%ld/", it->first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        if (idContent == PROGRAMDB_CONTENT_GAMES)
          pItem->GetProgramInfoTag()->m_playCount = it->second.second;
        if (!items.Contains(pItem->GetPath()))
        {
          pItem->SetLabelPreformated(true);
          items.Add(pItem);
        }
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
        pItem->GetProgramInfoTag()->m_iDbId = m_pDS->fv(0).get_asInt();
        pItem->GetProgramInfoTag()->m_type = type;

        CProgramDbUrl itemUrl = programUrl;
        CStdString path; path.Format("%ld/", m_pDS->fv(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->SetLabelPreformated(true);
        if (idContent == PROGRAMDB_CONTENT_GAMES)
        { // fv(3) is the number of program watched, fv(2) is the total number.  We set the playcount
          // only if the number of programs watched is equal to the total number (i.e. every program watched)
          pItem->GetProgramInfoTag()->m_playCount = (m_pDS->fv(3).get_asInt() == m_pDS->fv(2).get_asInt()) ? 1 : 0;
        }
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CProgramDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    Filter extFilter = filter;
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == PROGRAMDB_CONTENT_GAMES)
      {
        strSQL = PrepareSQL("select gameview.c%02d, path.strPath, files.playCount from gameview ", PROGRAMDB_ID_YEAR);
        extFilter.AppendJoin("join files on files.idFile = gameview.idFile join path on files.idPath = path.idPath");
      }
      else
        return false;
    }
    else
    {
      CStdString group;
      if (idContent == PROGRAMDB_CONTENT_GAMES)
      {
        strSQL = PrepareSQL("select gameview.c%02d, count(1), count(files.playCount) from gameview ", PROGRAMDB_ID_YEAR);
        extFilter.AppendJoin("join files on files.idFile = gameview.idFile");
        extFilter.AppendGroup(PrepareSQL("gameview.c%02d", PROGRAMDB_ID_YEAR));
      }
      else
        return false;
    }

    CProgramDbUrl programUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, programUrl))
      return false;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<int, pair<CStdString,int> > mapYears;
      map<int, pair<CStdString,int> >::iterator it;
      while (!m_pDS->eof())
      {
        int lYear = 0;
        if (idContent == PROGRAMDB_CONTENT_GAMES)
          lYear = m_pDS->fv(0).get_asInt();
        it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_programSources))
          {
            CStdString year;
            year.Format("%d", lYear);
            if (idContent == PROGRAMDB_CONTENT_GAMES)
              mapYears.insert(pair<int, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,m_pDS->fv(2).get_asInt())));
            else
              mapYears.insert(pair<int, pair<CStdString,int> >(lYear, pair<CStdString,int>(year,0)));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        if (it->first == 0)
          continue;
        CFileItemPtr pItem(new CFileItem(it->second.first));

        CProgramDbUrl itemUrl = programUrl;
        CStdString path; path.Format("%ld/", it->first);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        if (idContent == PROGRAMDB_CONTENT_GAMES)
          pItem->GetProgramInfoTag()->m_playCount = it->second.second;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        int lYear = 0;
        CStdString strLabel;
        if (idContent == PROGRAMDB_CONTENT_GAMES)
        {
          lYear = m_pDS->fv(0).get_asInt();
          strLabel = m_pDS->fv(0).get_asString();
        }
        if (lYear == 0)
        {
          m_pDS->next();
          continue;
        }
        CFileItemPtr pItem(new CFileItem(strLabel));

        CProgramDbUrl itemUrl = programUrl;
        CStdString path; path.Format("%ld/", lYear);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder=true;
        if (idContent == PROGRAMDB_CONTENT_GAMES)
        {
          // fv(2) is the number of programs watched, fv(1) is the total number.  We set the playcount
          // only if the number of programs watched is equal to the total number (i.e. every program watched)
          pItem->GetProgramInfoTag()->m_playCount = (m_pDS->fv(2).get_asInt() == m_pDS->fv(1).get_asInt()) ? 1 : 0;
        }

        // take care of dupes ..
        if (!items.Contains(pItem->GetPath()))
          items.Add(pItem);

        m_pDS->next();
      }
      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CProgramDatabase::GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(strBaseDir))
    return false;

  return GetItems(strBaseDir, programUrl.GetType(), programUrl.GetItemType(), items, filter, sortDescription);
}

bool CProgramDatabase::GetItems(const CStdString &strBaseDir, const CStdString &mediaType, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  PROGRAMDB_CONTENT_TYPE contentType;
  if (mediaType.Equals("games"))
    contentType = PROGRAMDB_CONTENT_GAMES;
  else
    return false;

  return GetItems(strBaseDir, contentType, itemType, items, filter, sortDescription);
}

bool CProgramDatabase::GetItems(const CStdString &strBaseDir, PROGRAMDB_CONTENT_TYPE mediaType, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (itemType.Equals("games") && mediaType == PROGRAMDB_CONTENT_GAMES)
    return GetGamesByWhere(strBaseDir, filter, items, sortDescription);
  else if (itemType.Equals("developers"))
    return GetDevelopersNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("publishers"))
    return GetPublishersNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("genres"))
    return GetGenresNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("descriptors"))
    return GetDescriptorsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("generalfeatures"))
    return GetGeneralFeaturesNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("onlinefeatures"))
    return GetOnlineFeaturesNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("platforms"))
    return GetPlatformsNav(strBaseDir, items, mediaType, filter);
  else if (itemType.Equals("years"))
    return GetYearsNav(strBaseDir, items, mediaType, filter);

  return false;
}

CStdString CProgramDatabase::GetItemById(const CStdString &itemType, int id)
{
  if (itemType.Equals("genres"))
    return GetGenreById(id);

  return "";
}

bool CProgramDatabase::GetGamesNav(const CStdString& strBaseDir, CFileItemList& items,
                                  int idDeveloper /* = -1 */, int idPublisher /* = -1 */, int idGenre /* = -1 */,
                                  int idDescriptor /* = -1 */, int idGeneralFeature /* = -1 */, int idOnlineFeature /* = -1 */,
                                  int idPlatform /* = -1 */, int idYear /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */)
{
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(strBaseDir))
    return false;

  if (idDeveloper > 0)  
    programUrl.AddOption("developerid", idDeveloper);
  else if (idPublisher > 0)  
    programUrl.AddOption("publisherid", idPublisher);
  else if (idGenre > 0)
    programUrl.AddOption("genreid", idGenre);
  else if (idDescriptor > 0)
    programUrl.AddOption("descriptorid", idDescriptor);
  else if (idGeneralFeature > 0)
    programUrl.AddOption("generalfeatureid", idGeneralFeature);
  else if (idOnlineFeature > 0)
    programUrl.AddOption("onlinefeatureid", idOnlineFeature);
  else if (idPlatform > 0)
    programUrl.AddOption("platformid", idPlatform);
  else if (idYear > 0)
    programUrl.AddOption("year", idYear);
  else if (idTag > 0)
    programUrl.AddOption("tagid", idTag);

  Filter filter;
  return GetGamesByWhere(programUrl.ToString(), filter, items, sortDescription);
}

bool CProgramDatabase::GetGamesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    gameTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // parse the base path to get additional filters
    CProgramDbUrl programUrl;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!programUrl.FromString(strBaseDir) || !GetFilter(programUrl, extFilter, sorting))
      return false;

    int total = -1;

    CStdString strSQL = "select %s from gameview ";
    CStdString strSQLExtra;

    if (!CDatabase::BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sorting.sortBy == SortByNone &&
       (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);

    if (!SortUtils::SortFromDataset(sorting, MediaTypeGame, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->find(FieldRow)->second.asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CProgramInfoTag game = GetDetailsForGame(record);
      if (g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(game.m_strPath, g_settings.m_programSources))
      {
        CFileItemPtr pItem(new CFileItem(game));

        CProgramDbUrl itemUrl = programUrl;
        CStdString path; path.Format("%ld", game.m_iDbId);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,game.m_playCount > 0);
        items.Add(pItem);
      }
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CProgramDatabase::GetRecentlyAddedGamesNav(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  Filter filter;
  filter.order = "dateAdded desc, idGame desc";
  filter.limit = PrepareSQL("%u", limit ? limit : g_advancedSettings.m_iProgramLibraryRecentlyAddedItems);
  return GetGamesByWhere(strBaseDir, filter, items);
}

bool CProgramDatabase::HasContent()
{
  return (HasContent(PROGRAMDB_CONTENT_GAMES));
}

bool CProgramDatabase::HasContent(PROGRAMDB_CONTENT_TYPE type)
{
  bool result = false;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql;
    if (type == PROGRAMDB_CONTENT_GAMES)
      sql = "select count(1) from game";
    m_pDS->query( sql.c_str() );

    if (!m_pDS->eof())
      result = (m_pDS->fv(0).get_asInt() > 0);

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return result;
}

ScraperPtr CProgramDatabase::GetScraperForPath( const CStdString& strPath )
{
  SScanSettings settings;
  return GetScraperForPath(strPath, settings);
}

ScraperPtr CProgramDatabase::GetScraperForPath(const CStdString& strPath, SScanSettings& settings)
{
  bool dummy;
  return GetScraperForPath(strPath, settings, dummy);
}

ScraperPtr CProgramDatabase::GetScraperForPath(const CStdString& strPath, SScanSettings& settings, bool& foundDirectly)
{
  foundDirectly = false;
  try
  {
    if (strPath.IsEmpty() || !m_pDB.get() || !m_pDS.get()) return ScraperPtr();

    ScraperPtr scraper;
    CStdString strPath1;
    CStdString strPath2(strPath);

    if (URIUtils::IsMultiPath(strPath))
      strPath2 = CMultiPathDirectory::GetFirstPath(strPath);

    URIUtils::GetDirectory(strPath2,strPath1);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      CStdString strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate,path.exclude from path where path.idPath=%i",idPath);
      m_pDS->query( strSQL.c_str() );
    }

    int iFound = 1;
    CONTENT_TYPE content = CONTENT_NONE;
    if (!m_pDS->eof())
    { // path is stored in db

      if (m_pDS->fv("path.exclude").get_asBool())
      {
        settings.exclude = true;
        m_pDS->close();
        return ScraperPtr();
      }
      settings.exclude = false;

      // try and ascertain scraper for this path
      CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
      strcontent.ToLower();
      content = TranslateContent(strcontent);

      //FIXME paths stored should not have empty strContent
      //assert(content != CONTENT_NONE);
      CStdString scraperID = m_pDS->fv("path.strScraper").get_asString();

      AddonPtr addon;
      if (!scraperID.empty() &&
        CAddonMgr::Get().GetAddon(scraperID, addon))
      {
        scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone(addon));
        if (!scraper)
          return ScraperPtr();

        // store this path's content & settings
        scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
        settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
        settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
        settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
      }
    }

    if (content == CONTENT_NONE)
    { // this path is not saved in db
      // we must drill up until a scraper is configured
      CStdString strParent;
      while (URIUtils::GetParentPath(strPath1, strParent))
      {
        iFound++;
 
        CStdString strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate, path.exclude from path where strPath='%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());

        CONTENT_TYPE content = CONTENT_NONE;
        if (!m_pDS->eof())
        {

          CStdString strcontent = m_pDS->fv("path.strContent").get_asString();
          strcontent.ToLower();
          if (m_pDS->fv("path.exclude").get_asBool())
          {
            settings.exclude = true;
            scraper.reset();
            m_pDS->close();
            break;
          }

          content = TranslateContent(strcontent);

          AddonPtr addon;
          if (content != CONTENT_NONE &&
              CAddonMgr::Get().GetAddon(m_pDS->fv("path.strScraper").get_asString(), addon))
          {
            scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone(addon));
            scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
            settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
            settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
            settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
            settings.exclude = false;

            break;
          }
        }
        strPath1 = strParent;
      }
    }
    m_pDS->close();

    if (!scraper || scraper->Content() == CONTENT_NONE)
      return ScraperPtr();

    if (scraper->Content() == CONTENT_GAMES)
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
    }
    else
    {
      iFound = 0;
      return ScraperPtr();
    }
    foundDirectly = (iFound == 1);
    return scraper;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return ScraperPtr();
}

CStdString CProgramDatabase::GetContentForPath(const CStdString& strPath)
{
  SScanSettings settings;
  bool foundDirectly = false;
  ScraperPtr scraper = GetScraperForPath(strPath, settings, foundDirectly);
  if (scraper)
    return TranslateContent(scraper->Content());
  return "";
}

bool CProgramDatabase::GetItemForPath(const CStdString &content, const CStdString &path, CFileItem &item)
{
  CFileItemList items;
  if (content == "games")
  {
    CStdString where = PrepareSQL("where c%02d='%s' limit 1", PROGRAMDB_ID_BASEPATH, path.c_str());
    GetGamesByWhere("", where, items);
  }
  if (items.Size())
  {
    item = *items[0];
    if (item.m_bIsFolder)
      item.SetPath(item.GetProgramInfoTag()->m_strPath);
    else
      item.SetPath(item.GetProgramInfoTag()->m_strFileNameAndPath);
    return true;
  }
  return false;
}

bool CProgramDatabase::GetItemsForPath(const CStdString &content, const CStdString &strPath, CFileItemList &items)
{
  CStdString path(strPath);

  if(URIUtils::IsMultiPath(path))
    path = CMultiPathDirectory::GetFirstPath(path);

  int pathID = GetPathId(path);
  if (pathID < 0)
    return false;

  if (content == "games")
  {
    Filter filter(PrepareSQL("c%02d=%d", PROGRAMDB_ID_BASEPATH, pathID));
    GetGamesByWhere("programdb://games/titles/", filter, items);
  }
  for (int i = 0; i < items.Size(); i++)
    items[i]->SetPath(items[i]->GetProgramInfoTag()->m_basePath);
  return items.Size() > 0;
}

int CProgramDatabase::GetRegion(const CStdString& strFilenameAndPath)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  try
  {
    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFilenameAndPath.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return 0;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return 0;
    }
    int iRegion = m_pDS->fv("files.iRegion").get_asInt();
    m_pDS->close();

    return iRegion;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:GetRegion(%s) failed", strFilenameAndPath.c_str());
  }
  return 0;
}

int CProgramDatabase::GetTitleId(const CStdString& strFilenameAndPath)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  try
  {
    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFilenameAndPath.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return 0;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return 0;
    }
    int idTitle = m_pDS->fv("files.TitleId").get_asInt();
    m_pDS->close();
    return idTitle;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:GetTitleId(%s) failed", strFilenameAndPath.c_str());
  }
  return 0;
}

bool CProgramDatabase::SetRegion(const CStdString& strFileName, int iRegion)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetRegion(%s), idFile=%i, region=%i",
              strFileName.c_str(), idFile,iRegion);

    strSQL=PrepareSQL("update files set iRegion=%i where idFile=%i",
                  iRegion, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::SetTitleId(const CStdString& strFileName, int idTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetTitle(%s), idFile=%i, region=%i",
              strFileName.c_str(), idFile, idTitle);

    strSQL=PrepareSQL("update files set titleId=%i where idFile=%i",
                  idTitle, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::GetXBEPathByTitleId(const int idTitle, CStdString& strPathAndFilename)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select files.strFilename from files where files.titleId=%i", idTitle);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      strPathAndFilename = m_pDS->fv("files.strFilename").get_asString();
      strPathAndFilename.Replace('/', '\\');
      m_pDS->close();
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetXBEPathByTitleId(%i) failed", idTitle);
  }
  return false;
}

bool CProgramDatabase::ItemHasTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for title's trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::HasTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("select * from trainers where idCRC=%u", (unsigned __int32) crc);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for trainer existance (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::AddTrainer(int iTitleId, const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    for( int i=0;i<100;++i)
      temp[i] = '0';
    temp[100] = '\0';
    strSQL=PrepareSQL("insert into trainers (idKey,idCRC,idTitle,strTrainerPath,strSettings,Active) values(NULL,%u,%u,'%s','%s',%i)",(unsigned __int32)crc,iTitleId,strTrainerPath.c_str(),temp,0);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to add trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::RemoveTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL=PrepareSQL("delete from trainers where idCRC=%u", (unsigned __int32)crc);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to remove trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers for %i (%s)",iTitleId,strSQL.c_str());
  }
  return false;

}

bool CProgramDatabase::GetAllTrainers(std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select distinct strTrainerPath from trainers");//PrepareSQL("select * from trainers");
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    int i;
    for (i=0;i<numOptions && i<100;++i)
    {
      if (data[i] == 1)
        temp[i] = '1';
      else
        temp[i] = '0';
    }
    temp[i] = '\0';

    strSQL = PrepareSQL("update trainers set strSettings='%s' where idCRC=%u and idTitle=%u", temp, (unsigned __int32)crc,iTitleId);
    if (m_pDS->exec(strSQL.c_str()))
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL.c_str());
  }

  return false;
}

void CProgramDatabase::SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("update trainers set Active=%u where idCRC=%u and idTitle=%u", bActive?1:0, (unsigned __int32)crc, iTitleId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL.c_str());
  }
}

CStdString CProgramDatabase::GetActiveTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u and Active=1", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return "";

    if (!m_pDS->eof())
      return m_pDS->fv("strTrainerPath").get_asString();
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error finding active trainer for %i (%s)",iTitleId,strSQL.c_str());
  }

  return "";
}

bool CProgramDatabase::GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("select * from trainers where idCRC=%u and idTitle=%u", (unsigned __int32)crc, iTitleId);
    if (m_pDS->query(strSQL.c_str()))
    {
      CStdString strSettings = m_pDS->fv("strSettings").get_asString();
      for (int i=0;i<numOptions && i < 100;++i)
        data[i] = strSettings[i]=='1'?1:0;

      return true;
    }

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::GetTrainerOptions failed (%s)",strSQL.c_str());
  }

  return false;
}

int CProgramDatabase::GetProgramInfo(CFileItem *item)
{
  int idTitle = 0;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select xbedescription,iTimesPlayed,lastAccessed,titleId,iSize from files where strFileName like '%s'", item->GetPath().c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    { // get info - only set the label if not preformatted
      if (!item->IsLabelPreformated())
        item->SetLabel(m_pDS->fv("xbedescription").get_asString());
      item->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asInt();
      item->m_strTitle = item->GetLabel();  // is this needed?
      item->m_dateTime = TimeStampToLocalTime(_atoi64(m_pDS->fv("lastAccessed").get_asString().c_str()));
      item->m_dwSize = _atoi64(m_pDS->fv("iSize").get_asString().c_str());
      idTitle = m_pDS->fv("titleId").get_asInt();
      if (item->m_dwSize == -1)
      {
        CStdString strPath;
        URIUtils::GetDirectory(item->GetPath(),strPath);
        __int64 iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
        CStdString strSQL=PrepareSQL("update files set iSize=%I64u where strFileName like '%s'",iSize,item->GetPath().c_str());
        m_pDS->exec(strSQL.c_str());
      }
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgramInfo(%s) failed", item->GetPath().c_str());
  }
  return idTitle;
}

bool CProgramDatabase::AddProgramInfo(CFileItem *item, unsigned int titleID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int iRegion = -1;
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(item->GetPath());
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
    }
    FILETIME time;
    item->m_dateTime=CDateTime::GetCurrentDateTime();
    item->m_dateTime.GetAsTimeStamp(time);

    ULARGE_INTEGER lastAccessed;
    lastAccessed.u.LowPart = time.dwLowDateTime; 
    lastAccessed.u.HighPart = time.dwHighDateTime;

    CStdString strPath, strParent;
    URIUtils::GetDirectory(item->GetPath(),strPath);
    // special case - programs in root of sources
    bool bIsShare=false;
    CUtil::GetMatchingSource(strPath,g_settings.m_programSources,bIsShare);
    __int64 iSize=0;
    if (bIsShare || !item->IsDefaultXBE())
    {
      __stat64 stat;
      if (CFile::Stat(item->GetPath(),&stat) == 0)
        iSize = stat.st_size;
    }
    else
      iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
    if (titleID == 0)
      titleID = (unsigned int) -1;
    CStdString strSQL=PrepareSQL("insert into files (idFile, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed, iRegion, iSize) values(NULL, '%s', %u, '%s', %i, %I64u, %i, %I64u)", item->GetPath().c_str(), titleID, item->GetLabel().c_str(), 0, lastAccessed.QuadPart, iRegion, iSize);
    m_pDS->exec(strSQL.c_str());
    item->m_dwSize = iSize;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddProgramInfo(%s) failed", item->GetPath().c_str());
  }
  return true;
}

FILETIME CProgramDatabase::TimeStampToLocalTime( unsigned __int64 timeStamp )
{
  FILETIME fileTime;
  ::FileTimeToLocalFileTime( (const FILETIME *)&timeStamp, &fileTime);
  return fileTime;
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    int iTimesPlayed = m_pDS->fv("files.iTimesPlayed").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::IncTimesPlayed(%s), idFile=%i, iTimesPlayed=%i",
              strFileName.c_str(), idFile, iTimesPlayed);

    strSQL=PrepareSQL("update files set iTimesPlayed=%i where idFile=%i",
                  ++iTimesPlayed, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:IncTimesPlayed(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::SetDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetDescription(%s), idFile=%i, description=%s",
              strFileName.c_str(), idFile,strDescription.c_str());

    strSQL=PrepareSQL("update files set xbedescription='%s' where idFile=%i",
                  strDescription.c_str(), idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::GetArbitraryQuery(const CStdString& strQuery,      const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                                         const CStdString& strOpenRecord, const CStdString& strCloseRecord,   const CStdString& strOpenField, 
										 const CStdString& strCloseField,       CStdString& strResult)
{
  try
  {
    strResult = "";
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL=strQuery;
    if (!m_pDS->query(strSQL.c_str()))
    {
      strResult = m_pDB->getErrorMsg();
      return false;
    }
    strResult=strOpenRecordSet;
    while (!m_pDS->eof())
    {
      strResult += strOpenRecord;
      for (int i=0; i<m_pDS->fieldCount(); i++)
      {
        strResult += strOpenField + CStdString(m_pDS->fv(i).get_asString()) + strCloseField;
      }
      strResult += strCloseRecord;
      m_pDS->next();
    }
    strResult += strCloseRecordSet;
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  try
  {
    if (NULL == m_pDB.get()) return false;
    strResult = m_pDB->getErrorMsg();
  }
  catch (...)
  {

  }

  return false;
}

bool CProgramDatabase::GetFilter(CDbUrl &programUrl, Filter &filter, SortDescription &sorting)
{
  if (!programUrl.IsValid())
    return false;

  std::string type = programUrl.GetType();
  std::string itemType = ((const CProgramDbUrl &)programUrl).GetItemType();
  const CUrlOptions::UrlOptions& options = programUrl.GetOptions();
  CUrlOptions::UrlOptions::const_iterator option;

  if (type == "games")
  {
    // TODO: implement this
  }
  else
    return false;

  option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return false;

    // check if the filter playlist matches the item type
    if (xsp.GetType() == itemType ||
       (xsp.GetGroup() == itemType && !xsp.IsGroupMixed()))
    {
      std::set<CStdString> playlists;
      filter.AppendWhere(xsp.GetWhereClause(*this, playlists));

      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      if (xsp.GetOrderDirection() != SortOrderNone)
        sorting.sortOrder = xsp.GetOrderDirection();
      if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }

  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return false;

    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == itemType)
    {
      std::set<CStdString> playlists;
      filter.AppendWhere(xspFilter.GetWhereClause(*this, playlists));
    }
    // remove the filter if it doesn't match the item type
    else
      programUrl.RemoveOption("filter");
  }

  return true;
}

