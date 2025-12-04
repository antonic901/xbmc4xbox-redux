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
#include "DVDFactoryDemuxer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamHttp.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"
#include "DVDDemuxCDDA.h"
#include "utils/log.h"

using namespace std;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  // Try to open CDDA demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("application/octet-stream") == 0)
  {
    std::string filename = pInputStream->GetFileName();
    if (filename.substr(0, 7) == "cdda://")
    {
      CLog::Log(LOGDEBUG, "DVDFactoryDemuxer: Stream is probably CD audio. Creating CDDA demuxer.");
      auto_ptr<CDVDDemuxCDDA> demuxer(new CDVDDemuxCDDA());
      if (demuxer->Open(pInputStream))
      {
        return demuxer.release();
      }
    }
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_HTTP))
  {
    CDVDInputStreamHttp* pHttpStream = (CDVDInputStreamHttp*)pInputStream;
    CHttpHeader *header = pHttpStream->GetHttpHeader();

    /* check so we got the meta information as requested in our http header */
    if( header->GetValue("icy-metaint").length() > 0 )
    {
      auto_ptr<CDVDDemuxShoutcast> demuxer(new CDVDDemuxShoutcast());
      if(demuxer->Open(pInputStream))
        return demuxer.release();
      else
        return NULL;
    }
  }

  auto_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream))
    return demuxer.release();
  else
    return NULL;
}

