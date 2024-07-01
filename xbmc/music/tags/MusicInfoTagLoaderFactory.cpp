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
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/tags/MusicInfoTagLoaderMP3.h"
#include "music/tags/MusicInfoTagLoaderOgg.h"
#include "music/tags/MusicInfoTagLoaderWMA.h"
#include "music/tags/MusicInfoTagLoaderFlac.h"
#include "music/tags/MusicInfoTagLoaderMP4.h"
#include "music/tags/MusicInfoTagLoaderCDDA.h"
#include "music/tags/MusicInfoTagLoaderApe.h"
#include "music/tags/MusicInfoTagLoaderMPC.h"
#include "music/tags/MusicInfoTagLoaderShn.h"
#include "music/tags/MusicInfoTagLoaderSid.h"
#include "music/tags/MusicInfoTagLoaderMod.h"
#include "music/tags/MusicInfoTagLoaderWav.h"
#include "music/tags/MusicInfoTagLoaderAAC.h"
#include "music/tags/MusicInfoTagLoaderWavPack.h"
#ifdef HAS_MOD_PLAYER
#include "cores/ModPlayer.h"
#endif
#include "music/tags/MusicInfoTagLoaderNSF.h"
#include "music/tags/MusicInfoTagLoaderSPC.h"
#include "music/tags/MusicInfoTagLoaderGYM.h"
#include "music/tags/MusicInfoTagLoaderAdplug.h"
#include "music/tags/MusicInfoTagLoaderYM.h"
#include "music/tags/MusicInfoTagLoaderDatabase.h"
#include "music/tags/MusicInfoTagLoaderASAP.h"

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{}

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
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ogg" || strExtension == "oggstream")
  {
    CMusicInfoTagLoaderOgg *pTagLoader = new CMusicInfoTagLoaderOgg();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "wma")
  {
    CMusicInfoTagLoaderWMA *pTagLoader = new CMusicInfoTagLoaderWMA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "flac")
  {
    CMusicInfoTagLoaderFlac *pTagLoader = new CMusicInfoTagLoaderFlac();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "m4a" || strExtension == "mp4")
  {
    CMusicInfoTagLoaderMP4 *pTagLoader = new CMusicInfoTagLoaderMP4();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ape" || strExtension == "mac")
  {
    CMusicInfoTagLoaderApe *pTagLoader = new CMusicInfoTagLoaderApe();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+")
  {
    CMusicInfoTagLoaderMPC *pTagLoader = new CMusicInfoTagLoaderMPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "sid" || strExtension == "sidstream")
  {
    CMusicInfoTagLoaderSid *pTagLoader = new CMusicInfoTagLoaderSid();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_MOD_PLAYER
  else if (ModPlayer::IsSupportedFormat(strExtension) || strExtension == "mod" || strExtension == "it" || strExtension == "s3m")
  {
    CMusicInfoTagLoaderMod *pTagLoader = new CMusicInfoTagLoaderMod();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (strExtension == "wav")
  {
    CMusicInfoTagLoaderWAV *pTagLoader = new CMusicInfoTagLoaderWAV();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "aac")
  {
    CMusicInfoTagLoaderAAC *pTagLoader = new CMusicInfoTagLoaderAAC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "wv")
  {
    CMusicInfoTagLoaderWAVPack *pTagLoader = new CMusicInfoTagLoaderWAVPack();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "nsf" || strExtension == "nsfstream")
  {
    CMusicInfoTagLoaderNSF *pTagLoader = new CMusicInfoTagLoaderNSF();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "spc")
  {
    CMusicInfoTagLoaderSPC *pTagLoader = new CMusicInfoTagLoaderSPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "gym")
  {
    CMusicInfoTagLoaderGYM *pTagLoader = new CMusicInfoTagLoaderGYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ym")
  {
    CMusicInfoTagLoaderYM *pTagLoader = new CMusicInfoTagLoaderYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (AdplugCodec::IsSupportedFormat(strExtension))
  {
    CMusicInfoTagLoaderAdplug *pTagLoader = new CMusicInfoTagLoaderAdplug();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (ASAPCodec::IsSupportedFormat(strExtension) || strExtension == "asapstream")
  {
    CMusicInfoTagLoaderASAP *pTagLoader = new CMusicInfoTagLoaderASAP();
    return (IMusicInfoTagLoader*)pTagLoader;
  }

  return NULL;
}
