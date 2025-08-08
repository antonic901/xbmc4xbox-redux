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

#include "ProgramDatabase.h"

#include "dbwrappers/dataset.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "settings/MediaSourceSettings.h"
#include "utils/Crc32.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/Trainer.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "windows/GUIWindowFileManager.h"

using namespace XFILE;

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{ }

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{ }

//********************************************************************************************************************************
bool CProgramDatabase::Open()
{
  return CDatabase::Open();
}

void CProgramDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create files table");
  m_pDS->exec("CREATE TABLE files ( idFile integer primary key, strFilename text, titleId integer, xbedescription text, iTimesPlayed integer, lastAccessed integer, iRegion integer, iSize integer)\n");

  CLog::Log(LOGINFO, "create trainers table");
  m_pDS->exec("CREATE TABLE trainers (idTrainer integer primary key, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");
}

void CProgramDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
  m_pDS->exec("CREATE INDEX idxTitleIdFiles ON files(titleId)");
}

void CProgramDatabase::UpdateTables(int version)
{
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

bool CProgramDatabase::GetXBEPathByTitleId(const int idTitle, std::string& strPathAndFilename)
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
      StringUtils::Replace(strPathAndFilename, '/', '\\');
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

bool CProgramDatabase::AddTrainer(int idTitle, CTrainer &trainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    char* temp = new char[trainer.GetNumberOfOptions() + 1];
    int i;
    for(i = 0; i < trainer.GetNumberOfOptions(); ++i)
      temp[i] = '0';
    temp[i] = '\0';

    strSQL = PrepareSQL("insert into trainers (idTrainer, idTitle, strTrainerPath, strSettings, Active) values (NULL, %u, '%s', '%s', %i)", idTitle, trainer.GetPath().c_str(), temp, 0);
    m_pDS->exec(strSQL.c_str());

    delete[] temp;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::RemoveTrainer(int idTrainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("delete from trainers where idTrainer=%i", idTrainer);
    return m_pDS->exec(strSQL.c_str()) == 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::SetTrainer(int idTitle, CTrainer *trainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // deactivate all trainers
    strSQL = PrepareSQL("update trainers set Active=%u where idTitle=%u", 0, idTitle);
    m_pDS->exec(strSQL.c_str());

    if (trainer == nullptr)
      return true;

    // set current trainer as active
    char* temp = new char[trainer->GetNumberOfOptions() + 1];
    int i;
    for (i = 0; i < trainer->GetNumberOfOptions(); ++i)
    {
      if (trainer->GetOptions()[i] == 1)
        temp[i] = '1';
      else
        temp[i] = '0';
    }
    temp[i] = '\0';

    strSQL = PrepareSQL("update trainers set Active=%u, strSettings='%s' where idTrainer=%i and idTitle=%u", 1, temp, trainer->GetTrainerId(), idTitle);
    m_pDS->exec(strSQL.c_str());

    delete[] temp;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainers(CFileItemList& items, unsigned int idTitle /* = 0 */)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("select * from trainers");
    if (idTitle)
      strSQL += PrepareSQL(" where idTitle = %u", idTitle);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      std::string strPath = m_pDS->fv("strTrainerPath").get_asString();
      CFileItemPtr pItem(new CFileItem(strPath, false));
      items.Add(pItem);
      pItem->SetProperty("idtrainer", m_pDS->fv("idTrainer").get_asInt());
      pItem->SetProperty("isactive", m_pDS->fv("Active").get_asBool());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainerOptions(int idTrainer, unsigned int idTitle, unsigned char* data, int numOptions)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("select * from trainers where idTrainer=%i and idTitle=%u", idTrainer, idTitle);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    std::string strSettings = m_pDS->fv("strSettings").get_asString();
    for (int i = 0; i < numOptions && i < 100; i++)
      data[i] = strSettings[i] == '1' ? 1 : 0;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::HasTrainer(const std::string& strTrainerPath)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("SELECT strTrainerPath FROM trainers WHERE strTrainerPath='%s'", strTrainerPath.c_str());
    return !GetSingleValue(strSQL).empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
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
        CStdString strPath = URIUtils::GetDirectory(item->GetPath());
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
    if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
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

    CStdString strPath = URIUtils::GetDirectory(item->GetPath());
    // special case - programs in root of sources
    bool bIsShare=false;
    CUtil::GetMatchingSource(strPath,*CMediaSourceSettings::Get().GetSources("programs"),bIsShare);
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
