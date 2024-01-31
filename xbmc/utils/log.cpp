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

#include "log.h"
#include <share.h>
#include "CriticalSection.h"
#include "SingleLock.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "Thread.h"

XFILE::CFile* CLog::m_file = NULL;

static CCriticalSection critSec;

static char levelNames[][8] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

#define LINE_ENDING "\n"

CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  CSingleLock waitLock(critSec);
  if (m_file)
  {
    m_file->Close();
    delete m_file;
    m_file = NULL;
  }
}


void CLog::Log(int loglevel, const char *format, ... )
{
  if (g_advancedSettings.m_logLevel > LOG_LEVEL_NORMAL ||
     (g_advancedSettings.m_logLevel > LOG_LEVEL_NONE && loglevel >= LOGNOTICE))
  {
    CSingleLock waitLock(critSec);
    if (!m_file)
    {
      m_file = new XFILE::CFile;
      if (!m_file)
        return;

      // g_advancedSettings.m_logFolder is initialized in the CAdvancedSettings constructor
      // and changed in CApplication::Create()
      CStdString strLogFile, strLogFileOld;

      strLogFile.Format("%sxbmc.log", g_advancedSettings.m_logFolder.c_str());
      strLogFileOld.Format("%sxbmc.old.log", g_advancedSettings.m_logFolder.c_str());

      if(m_file->Exists(strLogFileOld))
        m_file->Delete(strLogFileOld);
      if(m_file->Exists(strLogFile))
        m_file->Rename(strLogFile, strLogFileOld);

      if(!m_file->OpenForWrite(strLogFile))
        return;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    CStdString strPrefix, strData;

    strPrefix.Format("%02.2d:%02.2d:%02.2d T:%"PRIu64" M:%9"PRIu64" %7s: ", time.wHour, time.wMinute, time.wSecond, (uint64_t)CThread::GetCurrentThreadId(), (uint64_t)stat.dwAvailPhys, levelNames[loglevel]);

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);
    va_end(va);


    unsigned int length = 0;
    while ( length != strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');
      strData.TrimRight("\r");
    }

    if (!length)
      return;

#if !defined(_LINUX) && (defined(_DEBUG) || defined(PROFILE))
    OutputDebugString(strData.c_str());
    OutputDebugString("\n");
#endif

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", LINE_ENDING"                                            ");
    strData += LINE_ENDING;

    m_file->Write(strPrefix.c_str(), strPrefix.size());
    m_file->Write(strData.c_str(), strData.size());
  }
#if defined(_DEBUG) || defined(PROFILE)
  else
  {
    // In debug mode dump everything to devstudio regardless of level
    CSingleLock waitLock(critSec);
    CStdString strData;
    strData.reserve(16384);

    va_list va;
    va_start(va, format);
    strData.FormatV(format, va);
    va_end(va);

    OutputDebugString(strData.c_str());
    if( strData.Right(1) != "\n" )
      OutputDebugString("\n");

  }
#endif
}

void CLog::DebugLog(const char *format, ... )
{
#ifdef _DEBUG
  CSingleLock waitLock(critSec);

  CStdString strData;
  strData.reserve(16384);

  va_list va;
  va_start(va, format);
  strData.FormatV(format, va);
  va_end(va);

  OutputDebugString(strData.c_str());
  if( strData.Right(1) != "\n" )
    OutputDebugString("\n");
#endif
}

void CLog::DebugLogMemory()
{
  CSingleLock waitLock(critSec);
  MEMORYSTATUS stat;
  CStdString strData;

  GlobalMemoryStatus(&stat);
#ifdef __APPLE__
  strData.Format("%ju bytes free\n", stat.dwAvailPhys);
#else
  strData.Format("%lu bytes free\n", stat.dwAvailPhys);
#endif
  OutputDebugString(strData.c_str());
}

void CLog::MemDump(char *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      CStdString strFormat;
      if (*alpha > 31)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, "%s", strLine.c_str());
  }
}
