/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderFactory.h"

#include "FileItem.h"
#include "MusicInfoTagLoaderFactory.h"
#include "MusicInfoTagLoaderMP3.h"
#include "MusicInfoTagLoaderOgg.h"
#include "MusicInfoTagLoaderWMA.h"
#include "MusicInfoTagLoaderFlac.h"
#include "MusicInfoTagLoaderMP4.h"
#include "MusicInfoTagLoaderCDDA.h"
#include "MusicInfoTagLoaderApe.h"
#include "MusicInfoTagLoaderMPC.h"
#include "MusicInfoTagLoaderShn.h"
#include "MusicInfoTagLoaderSid.h"
#include "MusicInfoTagLoaderWav.h"
#include "MusicInfoTagLoaderAAC.h"
#include "MusicInfoTagLoaderWavPack.h"
#include "MusicInfoTagLoaderNSF.h"
#include "MusicInfoTagLoaderSPC.h"
#include "MusicInfoTagLoaderGYM.h"
#include "MusicInfoTagLoaderAdplug.h"
#include "MusicInfoTagLoaderYM.h"
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicInfoTagLoaderASAP.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory() {}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory() {}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CFileItem& item)
{
  // dont try to read the tags for streams & shoutcast
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  std::string strExtension = URIUtils::GetExtension(item.GetPath());
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
    return NULL;

  if (strExtension == "mp3")
  {
    CMusicInfoTagLoaderMP3 *pTagLoader = new CMusicInfoTagLoaderMP3();
    return pTagLoader;
  }
  else if (strExtension == "ogg" || strExtension == "oggstream")
  {
    CMusicInfoTagLoaderOgg *pTagLoader = new CMusicInfoTagLoaderOgg();
    return pTagLoader;
  }
  else if (strExtension == "wma")
  {
    CMusicInfoTagLoaderWMA *pTagLoader = new CMusicInfoTagLoaderWMA();
    return pTagLoader;
  }
  else if (strExtension == "flac")
  {
    CMusicInfoTagLoaderFlac *pTagLoader = new CMusicInfoTagLoaderFlac();
    return pTagLoader;
  }
  else if (strExtension == "m4a" || strExtension == "mp4")
  {
    CMusicInfoTagLoaderMP4 *pTagLoader = new CMusicInfoTagLoaderMP4();
    return pTagLoader;
  }
#ifdef HAS_OPTICAL_DRIVE
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return pTagLoader;
  }
#endif
  else if (strExtension == "ape" || strExtension == "mac")
  {
    CMusicInfoTagLoaderApe *pTagLoader = new CMusicInfoTagLoaderApe();
    return pTagLoader;
  }
  else if (strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+")
  {
    CMusicInfoTagLoaderMPC *pTagLoader = new CMusicInfoTagLoaderMPC();
    return pTagLoader;
  }
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return pTagLoader;
  }
  else if (strExtension == "sid" || strExtension == "sidstream")
  {
    CMusicInfoTagLoaderSid *pTagLoader = new CMusicInfoTagLoaderSid();
    return pTagLoader;
  }
  else if (strExtension == "wav")
  {
    CMusicInfoTagLoaderWAV *pTagLoader = new CMusicInfoTagLoaderWAV();
    return pTagLoader;
  }
  else if (strExtension == "aac")
  {
    CMusicInfoTagLoaderAAC *pTagLoader = new CMusicInfoTagLoaderAAC();
    return pTagLoader;
  }
  else if (strExtension == "wv")
  {
    CMusicInfoTagLoaderWAVPack *pTagLoader = new CMusicInfoTagLoaderWAVPack();
    return pTagLoader;
  }
  else if (strExtension == "nsf" || strExtension == "nsfstream")
  {
    CMusicInfoTagLoaderNSF *pTagLoader = new CMusicInfoTagLoaderNSF();
    return pTagLoader;
  }
  else if (strExtension == "spc")
  {
    CMusicInfoTagLoaderSPC *pTagLoader = new CMusicInfoTagLoaderSPC();
    return pTagLoader;
  }
  else if (strExtension == "gym")
  {
    CMusicInfoTagLoaderGYM *pTagLoader = new CMusicInfoTagLoaderGYM();
    return pTagLoader;
  }
  else if (strExtension == "ym")
  {
    CMusicInfoTagLoaderYM *pTagLoader = new CMusicInfoTagLoaderYM();
    return pTagLoader;
  }
  else if (AdplugCodec::IsSupportedFormat(strExtension))
  {
    CMusicInfoTagLoaderAdplug *pTagLoader = new CMusicInfoTagLoaderAdplug();
    return pTagLoader;
  }
  else if (ASAPCodec::IsSupportedFormat(strExtension) || strExtension == "asapstream")
  {
    CMusicInfoTagLoaderASAP *pTagLoader = new CMusicInfoTagLoaderASAP();
    return pTagLoader;
  }

  return NULL;
}
