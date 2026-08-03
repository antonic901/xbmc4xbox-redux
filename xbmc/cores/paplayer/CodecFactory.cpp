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
#include "XBAudioConfig.h"
#include "CodecFactory.h"
#include "MP3codec.h"
#include "OGGcodec.h"
#include "FLACcodec.h"
#include "WAVcodec.h"
#include "ModuleCodec.h"
#include "NSFCodec.h"
#include "SPCCodec.h"
#include "GYMCodec.h"
#include "SIDCodec.h"
#include "AdplugCodec.h"
#include "VGMCodec.h"
#include "YMCodec.h"
#include "ADPCMCodec.h"
#include "TimidityCodec.h"
#include "ASAPCodec.h"
#include "URL.h"
#include "DVDPlayerCodec.h"

ICodec* CodecFactory::CreateCodec(const std::string& strFileType)
{
  if (strFileType == "mp3" || strFileType == "mp2")
    return new MP3Codec();
  else if (strFileType == "ape" || strFileType == "mac")
    return new DVDPlayerCodec();
  else if (strFileType == "cdda")
    return new DVDPlayerCodec();
  else if (strFileType == "mpc" || strFileType == "mp+" || strFileType == "mpp")
    return new DVDPlayerCodec();
  else if (strFileType == "shn")
    return new DVDPlayerCodec();
  else if (strFileType == "mka")
    return new DVDPlayerCodec();
  else if (strFileType == "flac")
    return new FLACCodec();
  else if (strFileType == "wav")
    return new DVDPlayerCodec();
  else if (strFileType == "dts" || strFileType == "ac3" ||
           strFileType == "m4a" || strFileType == "aac")
    return new DVDPlayerCodec();
  else if (strFileType == "wv")
    return new DVDPlayerCodec();
  else if (ModuleCodec::IsSupportedFormat(strFileType))
    return new ModuleCodec();
  else if (strFileType == "nsf" || strFileType == "nsfstream")
    return new NSFCodec();
  else if (strFileType == "spc")
    return new SPCCodec();
  else if (strFileType == "gym")
    return new GYMCodec();
  else if (strFileType == "sid" || strFileType == "sidstream")
    return new SIDCodec();
  else if (AdplugCodec::IsSupportedFormat(strFileType))
    return new AdplugCodec();
  else if (VGMCodec::IsSupportedFormat(strFileType))
    return new VGMCodec();
  else if (strFileType == "ym")
    return new YMCodec();
  else if (strFileType == "wma")
    return new DVDPlayerCodec();
  else if (strFileType == "aiff" || strFileType == "aif")
    return new DVDPlayerCodec();
  else if (strFileType == "xwav")
    return new ADPCMCodec();
  else if (TimidityCodec::IsSupportedFormat(strFileType))
    return new TimidityCodec();
  else if (ASAPCodec::IsSupportedFormat(strFileType) || strFileType == "asapstream")
    return new ASAPCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const std::string& strFile, const std::string& strContent, unsigned int filecache)
{
  CURL urlFile(strFile);
  if( strContent == "audio/mpeg"
  ||  strContent == "audio/mp3" )
    return new MP3Codec();
  else if( strContent == "audio/aac"
    || strContent == "audio/aacp" )
  {
    DVDPlayerCodec *pCodec = new DVDPlayerCodec;
    if (urlFile.GetProtocol() == "shout" )
      pCodec->SetContentType(strContent);
    return pCodec;
  }
  else if( strContent == "audio/x-ms-wma" )
    return new DVDPlayerCodec();
  else if( strContent == "application/ogg" || strContent == "audio/ogg")
    return CreateOGGCodec(strFile,filecache);
   else if (strContent == "audio/flac" || strContent == "audio/x-flac" || strContent == "application/x-flac")
     return new FLACCodec();

  if (urlFile.IsProtocol("shout"))
  {
    return new MP3Codec(); // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }

  if (urlFile.IsFileType("wav"))
  {
    ICodec* codec;
    //lets see what it contains...
    //this kinda sucks 'cause if it's a plain wav file the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for wav
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType("audio/x-spdif-compressed");
    if (dvdcodec->Init(strFile, filecache))
    {
      return dvdcodec;
    }
    delete dvdcodec;
    codec = new ADPCMCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;

    codec = new WAVCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  else if (urlFile.IsFileType("ogg") || urlFile.IsFileType("oggstream") || urlFile.IsFileType("oga"))
    return CreateOGGCodec(strFile,filecache);

  //default
  return CreateCodec(urlFile.GetFileType());
}

ICodec* CodecFactory::CreateOGGCodec(const std::string& strFile,
                                     unsigned int filecache)
{
  // oldnemesis: we want to use OGGCodec() for OGG music since unlike DVDCodec
  // it provides better timings for Karaoke. However OGGCodec() cannot handle
  // ogg-flac and ogg videos, that's why this block.
  ICodec* codec = new OGGCodec();

  // hack - force DVDPlayer for now - there is a memory leak with our ogg player -
  // http://redmine.exotica.org.uk/issues/228
  return new DVDPlayerCodec();

  try
  {
    if (codec->Init(strFile, filecache))
      return codec;
  }
  catch( ... )
  {
  }
  delete codec;
  return new DVDPlayerCodec();
}