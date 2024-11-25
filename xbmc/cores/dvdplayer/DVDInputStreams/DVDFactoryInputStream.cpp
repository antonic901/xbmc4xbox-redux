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
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "DVDInputStreamHttp.h"
#include "DVDInputStreamFFmpeg.h"
#include "DVDInputStreamRTMP.h"
#ifdef ENABLE_DVDINPUTSTREAM_STACK
#include "DVDInputStreamStack.h"
#endif
#include "FileItem.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, CFileItem fileitem)
{
  std::string file = fileitem.GetPath();

  if (fileitem.IsDiscImage())
    return new CDVDInputStreamNavigator(pPlayer, fileitem);

#ifdef _XBOX
  if (file.compare("\\Device\\Cdrom0") == 0)
#else
  if(file.compare(g_mediaManager.TranslateDevicePath("")) == 0)
#endif
    return new CDVDInputStreamNavigator(pPlayer, fileitem);
  if (fileitem.IsDVDFile(false, true))
    return (new CDVDInputStreamNavigator(pPlayer, fileitem));
  else if(file.substr(0, 6) == "rtp://"
       || file.substr(0, 7) == "rtsp://"
       || file.substr(0, 6) == "sdp://"
       || file.substr(0, 6) == "udp://"
       || file.substr(0, 6) == "tcp://"
       || file.substr(0, 6) == "mms://"
       || file.substr(0, 7) == "mmst://"
       || file.substr(0, 7) == "mmsh://")
    return new CDVDInputStreamFFmpeg(fileitem);
#ifdef ENABLE_DVDINPUTSTREAM_STACK
  else if(file.substr(0, 8) == "stack://")
    return new CDVDInputStreamStack(fileitem);
#endif
  else if(file.substr(0, 7) == "rtmp://"
       || file.substr(0, 8) == "rtmpt://"
       || file.substr(0, 8) == "rtmpe://"
       || file.substr(0, 9) == "rtmpte://"
       || file.substr(0, 8) == "rtmps://")
    return new CDVDInputStreamRTMP(fileitem);
  else if (fileitem.IsInternetStream())
  {
    if (fileitem.IsType(".m3u8"))
      return new CDVDInputStreamFFmpeg(fileitem);

    if (fileitem.ContentLookup())
    {
      // request header
      fileitem.SetMimeType("");
      fileitem.FillInMimeType();
    }

    if (fileitem.GetMimeType() == "application/vnd.apple.mpegurl")
      return new CDVDInputStreamFFmpeg(fileitem);
  }
  
  // our file interface handles all these types of streams
  return (new CDVDInputStreamFile(fileitem));
}
