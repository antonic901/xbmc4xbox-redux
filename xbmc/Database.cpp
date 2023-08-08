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

#include "utils/log.h"
#include "AutoPtrHandle.h"
#include "Database.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/Crc32.h"
#include "FileSystem/SpecialProtocol.h"
#include "AutoPtrHandle.h"
#include "utils/SingleLock.h"

using namespace AUTOPTR;
using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

std::map<std::string, bool> CDatabase::m_updated;

void CDatabase::Filter::AppendField(const std::string &strField)
{
  if (strField.empty())
    return;

  if (fields.empty() || fields == "*")
    fields = strField;
  else
    fields += ", " + strField;
}

void CDatabase::Filter::AppendJoin(const std::string &strJoin)
{
  if (strJoin.empty())
    return;

  if (join.empty())
    join = strJoin;
  else
    join += " " + strJoin;
}

void CDatabase::Filter::AppendWhere(const std::string &strWhere, bool combineWithAnd /* = true */)
{
  if (strWhere.empty())
    return;

  if (where.empty())
    where = strWhere;
  else
  {
    where = "(" + where + ") ";
    where += combineWithAnd ? "AND" : "OR";
    where += " (" + strWhere + ")";
  }
}

void CDatabase::Filter::AppendOrder(const std::string &strOrder)
{
  if (strOrder.empty())
    return;

  if (order.empty())
    order = strOrder;
  else
    order += ", " + strOrder;
}

void CDatabase::Filter::AppendGroup(const std::string &strGroup)
{
  if (strGroup.empty())
    return;

  if (group.empty())
    group = strGroup;
  else
    group += ", " + strGroup;
}

CDatabase::CDatabase(void)
{
  m_bOpen = false;
  m_iRefCount = 0;
  m_sqlite = true;
  m_bMultiWrite = false;
}

CDatabase::~CDatabase(void)
{
  Close();
}

void CDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

DWORD CDatabase::ComputeCRC(const CStdString &text)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(text);
  return (DWORD)crc;
}

CStdString CDatabase::FormatSQL(CStdString strStmt, ...)
{
  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  strStmt.Replace("%s", "%q");
  //  the %I64 enhancement is not supported by sqlite3_vmprintf
  //  must be %ll instead
  strStmt.Replace("%I64", "%ll");

  va_list args;
  va_start(args, strStmt);
  auto_aptr<char> szStmt(sqlite3_vmprintf(strStmt.c_str(), args));
  va_end(args);

  return szStmt.get();
}

CStdString CDatabase::PrepareSQL(CStdString strStmt, ...) const
{
  CStdString strResult = "";

  if (NULL != m_pDB.get())
  {
    va_list args;
    va_start(args, strStmt);
    strResult = m_pDB->vprepare(strStmt.c_str(), args);
    va_end(args);
  }

  return strResult;
}

std::string CDatabase::GetSingleValue(const std::string &query, std::auto_ptr<Dataset> &ds)
{
  std::string ret;
  try
  {
    if (!m_pDB.get() || !ds.get())
      return ret;

    if (ds->query(query.c_str()) && ds->num_rows() > 0)
      ret = ds->fv(0).get_asString();

    ds->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - failed on query '%s'", __FUNCTION__, query.c_str());
  }
  return ret;
}

CStdString CDatabase::GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause /* = CStdString() */, const CStdString &strOrderBy /* = CStdString() */)
{
  CStdString query = PrepareSQL("SELECT %s FROM %s", strColumn.c_str(), strTable.c_str());
  if (!strWhereClause.empty())
    query += " WHERE " + strWhereClause;
  if (!strOrderBy.empty())
    query += " ORDER BY " + strOrderBy;
  query += " LIMIT 1";
  return GetSingleValue(query, m_pDS);
}

bool CDatabase::DeleteValues(const CStdString &strTable, const CStdString &strWhereClause /* = CStdString() */)
{
  bool bReturn = true;

  CStdString strQueryBase = "DELETE FROM %s";
  if (!strWhereClause.IsEmpty())
    strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());

  CStdString strQuery = PrepareSQL(strQueryBase, strTable.c_str());

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CDatabase::ExecuteQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;
    m_pDS->exec(strQuery.c_str());
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::ResultQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;

    CStdString strPreparedQuery = PrepareSQL(strQuery.c_str());

    bReturn = m_pDS->query(strPreparedQuery.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::QueueInsertQuery(const CStdString &strQuery)
{
  if (strQuery.IsEmpty())
    return false;

  if (!m_bMultiWrite)
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    m_bMultiWrite = true;
    m_pDS2->insert();
  }

  m_pDS2->add_insert_sql(strQuery);

  return true;
}

bool CDatabase::CommitInsertQueries()
{
  bool bReturn = false;

  if (m_bMultiWrite)
  {
    try
    {
      m_bMultiWrite = false;
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
      bReturn = true;
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - failed to execute queries",
          __FUNCTION__);
    }
  }

  return bReturn;
}

bool CDatabase::Open()
{
  DatabaseSettings db_fallback;
  return Open(db_fallback);
}

bool CDatabase::Open(const DatabaseSettings &settings)
{
  // take a copy - we're gonna be messing with it and we don't want to touch the original
  DatabaseSettings dbSettings = settings;
  if (IsOpen())
  {
    m_iRefCount++;
    return true;
  }

  m_sqlite = true;

  if ( dbSettings.type.Equals("mysql") )
  {
    // check we have all information before we cancel the fallback
    if ( ! (dbSettings.host.IsEmpty() || dbSettings.user.IsEmpty() || dbSettings.pass.IsEmpty()) )
      m_sqlite = false;
    else
      CLog::Log(LOGINFO, "essential mysql database information is missing (eg. host, user, pass)");
  }

  // set default database name if appropriate
  if ( dbSettings.name.IsEmpty() )
    dbSettings.name = GetDefaultDBName();

  // always safely fallback to sqlite3
  if (m_sqlite)
  {
    dbSettings.type = "sqlite3";
    dbSettings.host = _P(g_settings.GetDatabaseFolder());
  }

  if (Connect(dbSettings, true) && UpdateVersion(dbSettings.name))
    return true;
  // failed to update or open the database
  Close();
  CLog::Log(LOGERROR, "Unable to open database %s", dbSettings.name.c_str());
  return false;
}

bool CDatabase::Connect(const DatabaseSettings &dbSettings, bool create)
{
  // create the appropriate database structure
  if (dbSettings.type.Equals("sqlite3"))
  {
    m_pDB.reset( new SqliteDatabase() ) ;
  }
  else if (dbSettings.type.Equals("mysql"))
  {
    // TODO: Add MySQL library as well as mysqldataset.cpp and mysqldataset.h
    // m_pDB.reset( new MysqlDatabase() ) ;
    CLog::Log(LOGWARNING, "MySQL database is still not supported, falling back to SQLite...");
    m_pDB.reset( new SqliteDatabase() ) ;
  }
  else
  {
    CLog::Log(LOGERROR, "Unable to determine database type: %s", dbSettings.type.c_str());
    return false;
  }

  // host name is always required
  m_pDB->setHostName(dbSettings.host.c_str());

  if (!dbSettings.port.IsEmpty())
    m_pDB->setPort(dbSettings.port.c_str());

  if (!dbSettings.user.IsEmpty())
    m_pDB->setLogin(dbSettings.user.c_str());

  if (!dbSettings.pass.IsEmpty())
    m_pDB->setPasswd(dbSettings.pass.c_str());

  // database name is always required
  m_pDB->setDatabase(dbSettings.name.c_str());

  // create the datasets
  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  CLog::Log(LOGDEBUG, "CDatabase: Connecting to database %s at %s:%s",
            dbSettings.name.c_str(), dbSettings.host.c_str(), dbSettings.port.c_str());

  if (m_pDB->connect(create) != DB_CONNECTION_OK)
    return false;
  
  // test if db already exists, if not we need to create the tables
  if (!m_pDB->exists() && create)
  {
    if (dbSettings.type.Equals("sqlite3"))
    {
      //  all fatx formatted partitions, except the utility drive,
      //  have a cluster size of 16k. To gain better performance
      //  when performing write operations to the database, set
      //  the page size of the database file to 16k.
      //  This needs to be done before any table is created.
      m_pDS->exec("PRAGMA page_size=16384\n");

      //  Also set the memory cache size to 16k
      m_pDS->exec("PRAGMA default_cache_size=16384\n");
    }
    CreateTables();
  }

  // Mark our db as open here to make our destructor to properly close the file handle
  m_bOpen = true;

  // sqlite3 post connection operations
  if (dbSettings.type.Equals("sqlite3"))
  {
    m_pDS->exec("PRAGMA cache_size=16384\n");
    m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
    m_pDS->exec("PRAGMA journal_mode='TRUNCATE'\n");
  //  m_pDS->exec("PRAGMA journal_mode='OFF'\n");
    m_pDS->exec("PRAGMA count_changes='OFF'\n");
  }

  m_iRefCount++;
  return true;
}

bool CDatabase::UpdateVersion(const CStdString &dbName)
{
  CSingleLock lock(m_critSect);
  if (m_updated[dbName])
    return true;

  int version = 0;
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    version = m_pDS->fv("idVersion").get_asInt();

  if (version < GetMinVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
    if (UpdateOldVersion(version) && UpdateVersionNumber())
      CLog::Log(LOGINFO, "Update to version %i successfull", GetMinVersion());
    else
    {
      CLog::Log(LOGERROR, "Can't update the database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
      return false;
    }
  }
  else if (version > GetMinVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
    return false;
  }

  m_updated[dbName] = true;
  return true;
}

bool CDatabase::IsOpen()
{
  return m_bOpen;
}

void CDatabase::Close()
{
  if (!m_bOpen)
    return ;

  if (m_iRefCount > 1)
  {
    m_iRefCount--;
    return ;
  }

  m_iRefCount--;
  m_bOpen = false;

  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
  m_pDS2.reset();
}

bool CDatabase::Compress(bool bForce /* =true */)
{
  if (!m_sqlite)
    return true;

  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (!bForce)
    {
      m_pDS->query("select iCompressCount from version");
      if (!m_pDS->eof())
      {
        int iCount = m_pDS->fv(0).get_asInt();
        if (iCount > MAX_COMPRESS_COUNT)
          iCount = -1;
        m_pDS->close();
        CStdString strSQL=PrepareSQL("update version set iCompressCount=%i\n",++iCount);
        m_pDS->exec(strSQL.c_str());
        if (iCount != 0)
          return true;
      }
    }

    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Compressing the database failed", __FUNCTION__);
    return false;
  }
  return true;
}

void CDatabase::Interupt()
{
  m_pDS->interrupt();
}

void CDatabase::BeginTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->start_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:begintransaction failed");
  }
}

bool CDatabase::CommitTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->commit_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:committransaction failed");
    return false;
  }
  return true;
}

void CDatabase::RollbackTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->rollback_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:rollbacktransaction failed");
  }
}

bool CDatabase::InTransaction()
{
  if (NULL != m_pDB.get()) return false;
  return m_pDB->in_transaction();
}

bool CDatabase::CreateTables()
{
    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    CStdString strSQL=PrepareSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());

    return true;
}

bool CDatabase::UpdateVersionNumber()
{
  try
  {
    CStdString strSQL=PrepareSQL("UPDATE version SET idVersion=%i\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    return false;
  }

  return true;
}

bool CDatabase::BuildSQL(const CStdString &strQuery, const Filter &filter, CStdString &strSQL)
{
  strSQL = strQuery;

  if (!filter.join.empty())
    strSQL += filter.join;
  if (!filter.where.empty())
    strSQL += " WHERE " + filter.where;
  if (!filter.group.empty())
    strSQL += " GROUP BY " + filter.group;
  if (!filter.order.empty())
    strSQL += " ORDER BY " + filter.order;
  if (!filter.limit.empty())
    strSQL += " LIMIT " + filter.limit;

  return true;
}
