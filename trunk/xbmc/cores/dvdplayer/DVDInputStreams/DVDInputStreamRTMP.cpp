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

#include "stdafx.h"
#include "FileItem.h"
#include "DVDInputStreamRTMP.h"
#include "VideoInfoTag.h"
#include "FileSystem/IFile.h"
#include "AdvancedSettings.h"
#include "utils/log.h"
#include "lib/libRTMP/log.h"

#ifdef _LINUX
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <netinet/in.h>
#endif

#include "Application.h"

using namespace XFILE;

extern "C" {
static void CDVDInputStreamRTMP_Log(int level, const char *fmt, va_list args)
{
  char buf[2048];

  if (level > RTMP_LogGetLevel())
    return;

    switch(level) {
    default:
    case RTMP_LOGCRIT:    level = LOGFATAL;   break;
    case RTMP_LOGERROR:   level = LOGERROR;   break;
    case RTMP_LOGWARNING: level = LOGWARNING; break;
    case RTMP_LOGINFO:    level = LOGNOTICE;  break;
    case RTMP_LOGDEBUG:   level = LOGINFO;    break;
    case RTMP_LOGDEBUG2:  level = LOGDEBUG;   break;
    }

  _vsnprintf(buf, sizeof(buf), fmt, args);
  CLog::Log(level, "%s", buf);
}
};

CDVDInputStreamRTMP::CDVDInputStreamRTMP() : CDVDInputStream(DVDSTREAM_TYPE_RTMP)
{
  RTMP_LogLevel level;

  RTMP_LogSetCallback(CDVDInputStreamRTMP_Log);
  switch (g_advancedSettings.m_logLevel) {
  case LOG_LEVEL_DEBUG_SAMBA: level = RTMP_LOGDEBUG2; break;
  case LOG_LEVEL_DEBUG_FREEMEM:
  case LOG_LEVEL_DEBUG: level = RTMP_LOGDEBUG; break;
  case LOG_LEVEL_NORMAL: level = RTMP_LOGINFO; break;
  default: level = RTMP_LOGCRIT; break;
  }
  RTMP_LogSetLevel(level);
  RTMP_Init(&m_rtmp);
  m_eof = true;
  m_bPaused = false;
  m_sStreamPlaying = NULL;
}

CDVDInputStreamRTMP::~CDVDInputStreamRTMP()
{
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  Close();
}

bool CDVDInputStreamRTMP::IsEOF()
{
  return m_eof;
}

#define SetAVal(av, cstr) av.av_val = (char *)cstr.c_str(); av.av_len = cstr.length()
#undef AVC
#define AVC(str) {(char *)str,sizeof(str)-1}

/* librtmp option names are slightly different */
static const struct {
  const char *name;
  AVal key;
} options[] = {
  { "SWFPlayer", AVC("swfUrl") },
  { "PageURL",   AVC("pageUrl") },
  { "PlayPath",  AVC("playpath") },
  { "TcUrl",     AVC("tcUrl") },
  { "IsLive",    AVC("live") },
  { NULL }
};

bool CDVDInputStreamRTMP::Open(const char* strFile, const std::string& content)
{
  int i;
  if (m_sStreamPlaying)
  {
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;
  }

  if (!CDVDInputStream::Open(strFile, "video/x-flv")) return false;

  i = strlen(strFile);
  m_sStreamPlaying = (char*)malloc(i+1);
  memcpy(m_sStreamPlaying,strFile,i);
  m_sStreamPlaying[i] = '\0';
  if (!RTMP_SetupURL(&m_rtmp, m_sStreamPlaying)) return false;

  for (i=0; options[i].name; i++)
  {
    std::string tmp = m_item.GetProperty(options[i].name);
    if (tmp.length())
    {
      AVal av_tmp;
      SetAVal(av_tmp, tmp);
      RTMP_SetOpt(&m_rtmp, &options[i].key, &av_tmp);
    }
  }

  CSingleLock lock(m_RTMPSection);

  if (!RTMP_Connect(&m_rtmp, NULL) || !RTMP_ConnectStream(&m_rtmp, 0))
    return false;

  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamRTMP::Close()
{
  CSingleLock lock(m_RTMPSection);
  CDVDInputStream::Close();
  RTMP_Close(&m_rtmp);
  m_eof = true;
  m_bPaused = false;
}

int CDVDInputStreamRTMP::Read(BYTE* buf, int buf_size)
{
  int i;
  CSingleLock lock(m_RTMPSection);
  i = RTMP_Read(&m_rtmp, (char *)buf, buf_size);
  if (i < 0)
    m_eof = true;
  return i;
}

__int64 CDVDInputStreamRTMP::Seek(__int64 offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1;
}

bool CDVDInputStreamRTMP::SeekTime(int iTimeInMsec)
{
  CLog::Log(LOGNOTICE, "RTMP Seek to %i requested", iTimeInMsec);
  CSingleLock lock(m_RTMPSection);
  if (RTMP_SendSeek(&m_rtmp, iTimeInMsec))
  {
    return true;
  }
  else
    return false;
}

__int64 CDVDInputStreamRTMP::GetLength()
{
  return -1;
}

bool CDVDInputStreamRTMP::NextStream()
{
  return false;
}

bool CDVDInputStreamRTMP::Pause(double dTime)
{
  CSingleLock lock(m_RTMPSection);
  if (!m_bPaused)
    m_rtmp.m_pauseStamp = m_rtmp.m_channelTimestamp[m_rtmp.m_mediaChannel];
  m_bPaused = !m_bPaused;
  RTMP_SendPause(&m_rtmp, m_bPaused, m_rtmp.m_pauseStamp);
  return true;
}

