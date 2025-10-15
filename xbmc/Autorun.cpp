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

#include "Autorun.h"
#include "Application.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "video/VideoDatabase.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "Util.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif
#ifdef _XBOX
#include "interfaces/builtins/Builtins.h"
#include "utils/Trainer.h"
#include "xbox/xbeheader.h"
#include "programs/ProgramDatabase.h"
#endif

#include "defs_from_settings.h"

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun()
{}

void CAutorun::ExecuteAutorun( bool bypassSettings, bool ignoreplaying, bool restart )
{
  if ((!ignoreplaying && (g_application.m_pPlayer->IsPlayingAudio() || g_application.m_pPlayer->IsPlayingVideo() || g_windowManager.HasModalDialog())) || g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return ;

  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaverWindow();  // turn off the screensaver if it's active

  if ( pInfo->IsAudio( 1 ) )
  {
    if( !bypassSettings && !CSettings::GetInstance().GetBool("autorun.cdda") )
      return;

    if (!g_passwordManager.IsMasterLockUnlocked(false))
      if (CProfilesManager::Get().GetCurrentProfile().musicLocked())
        return ;

    RunCdda();
  }
  else
  {
    RunMedia(bypassSettings, restart);
  }
}

void CAutorun::ExecuteXBE(const CStdString &xbeFile)
{
  int iRegion;
  if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(xbeFile);
    if (iRegion < 1 || iRegion > 7)
      iRegion = 0;
    iRegion = xbe.FilterRegion(iRegion);
  }
  else
    iRegion = 0;

  CProgramDatabase database;
  database.Open();

  // Load active trainer
  DWORD dwTitleId = CUtil::GetXbeID(xbeFile);
  CFileItemList items;
  if (database.GetTrainers(items, dwTitleId))
  {
   for (int i = 0; i < items.Size(); ++i)
   {
     if (items[i]->GetProperty("isactive").asBoolean())
     {
       CTrainer* trainer = new CTrainer(items[i]->GetProperty("idtrainer").asInteger32());
       if (trainer->Load(items[i]->GetPath()))
       {
         database.GetTrainerOptions(trainer->GetTrainerId(), dwTitleId, trainer->GetOptions(), trainer->GetNumberOfOptions());
         CTrainer::InstallTrainer(*trainer);
       }
       else
       {
         delete trainer;
       }
     }
   }
  }

  database.Close();
  CUtil::RunXBE(xbeFile.c_str(), NULL,F_VIDEO(iRegion));
}

void CAutorun::RunCdda()
{
  CFileItemList vecItems;

  const CURL pathToUrl("cdda://local/");
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( pathToUrl ) );
  if ( !pDir->GetDirectory( pathToUrl, vecItems ) )
    return ;

  if ( vecItems.Size() <= 0 )
    return ;

  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Add(PLAYLIST_MUSIC, vecItems);
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Play();
}

void CAutorun::RunMedia(bool bypassSettings, bool restart)
{
#ifdef _XBOX
  if ( CFile::Exists("D:\\default.xbe") )
  {
    if (!CSettings::GetInstance().GetBool("autorun.xbox") && !bypassSettings)
      return;

    if (!g_passwordManager.IsMasterLockUnlocked(false))
      if (CProfilesManager::Get().GetCurrentProfile().programsLocked())
        return;

    ExecuteXBE("D:\\default.xbe");
    return;
  }
#endif

  if ( !CSettings::GetInstance().GetBool("autorun.dvd") && !CSettings::GetInstance().GetBool("autorun.vcd") && !CSettings::GetInstance().GetBool("autorun.video") && !CSettings::GetInstance().GetBool("autorun.music") && !CSettings::GetInstance().GetBool("autorun.pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
#ifndef _XBOX
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( g_mediaManager.TranslateDevicePath("") ));
  bool bPlaying = RunDisc(pDir.get(), g_mediaManager.TranslateDevicePath(""), nAddedToPlaylist, true, bypassSettings, restart);
#else
  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

  if ( pInfo == NULL )
    return ;

  bool bPlaying;
  if (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1))
  {
    const CURL pathToUrl("iso9660://");
    auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( pathToUrl ));
    bPlaying = RunDisc(pDir.get(), "iso9660://", nAddedToPlaylist, true, bypassSettings, restart);
  }
  else
  {
    const CURL pathToUrl("D:\\");
    auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( pathToUrl ));
    bPlaying = RunDisc(pDir.get(), "D:\\", nAddedToPlaylist, true, bypassSettings, restart);
  }
#endif
  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
    g_windowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    g_playlistPlayer.Play(nSize, "");
  }
}

/**
 * This method tries to determine what type of disc is located in the given drive and starts to play the content appropriately.
 */
bool CAutorun::RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */, bool restart /* = false */)
{
  bool bPlaying(false);
  CFileItemList vecItems;
  char szSlash = '\\';
  if (strDrive.Find("iso9660") != -1) szSlash = '/';

  const CURL pathToUrl(strDrive);
  if ( !pDir->GetDirectory( pathToUrl, vecItems ) )
  {
    return false;
  }

  bool bAllowVideo = true;
  bool bAllowPictures = true;
  bool bAllowMusic = true;
  if (!g_passwordManager.IsMasterLockUnlocked(false))
  {
    bAllowVideo = !CProfilesManager::Get().GetCurrentProfile().videoLocked();
    bAllowPictures = !CProfilesManager::Get().GetCurrentProfile().picturesLocked();
    bAllowMusic = !CProfilesManager::Get().GetCurrentProfile().musicLocked();
  }

  // is this a root folder we have to check the content to determine a disc type
  if( bRoot )
  {

    // check root folders first, for normal structured dvd's
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];

      // is the current item a (non system) folder?
      if (pItem->m_bIsFolder && pItem->GetPath() != "." && pItem->GetPath() != "..")
      {
        // Check if the current foldername indicates a DVD structure (name is "VIDEO_TS")
        if (pItem->GetPath().find("VIDEO_TS") != std::string::npos && bAllowVideo
        && (bypassSettings || CSettings::GetInstance().GetBool("autorun.dvd")))
        {
          CUtil::PlayDVD("dvd", restart);
          bPlaying = true;
          return true;
        }

        // Video CDs can have multiple file formats. First we need to determine which one is used on the CD
        CStdString strExt;
        if (pItem->GetPath().find("MPEGAV") != std::string::npos)
          strExt = ".dat";
        if (pItem->GetPath().find("MPEG2") != std::string::npos)
          strExt = ".mpg";

        // If a file format was extracted we are sure this is a VCD. Autoplay if settings indicate we should.
        if (!strExt.IsEmpty() && bAllowVideo
              && (bypassSettings || CSettings::GetInstance().GetBool("autorun.vcd")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, strExt, DIR_FLAG_DEFAULTS);
          if (items.Size())
          {
            items.Sort(SortByLabel, SortOrderDescending);
            g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Play(0, "");
            bPlaying = true;
            return true;
          }
        }
        else if (pItem->GetPath().find("PICTURES") != std::string::npos && bAllowPictures
              && (bypassSettings || CSettings::GetInstance().GetBool("autorun.pictures")))
        {
          bPlaying = true;
          CStdString strExec;
          strExec.Format("XBMC.RecursiveSlideShow(%s)", pItem->GetPath().c_str());
          CBuiltins::GetInstance().Execute(strExec);
          return true;
        }
      }
    }
  }

  // check video first
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CSettings::GetInstance().GetBool("autorun.video")))
  {
    // stack video files
    CFileItemList tempItems;
    tempItems.Append(vecItems);
    if (CSettings::GetInstance().GetBool("myvideos.stackvideos"))
      tempItems.Stack();
    CFileItemList itemlist;

    for (int i = 0; i < tempItems.Size(); i++)
    {
      CFileItemPtr pItem = tempItems[i];
      if (!pItem->m_bIsFolder && pItem->IsVideo())
      {
        bPlaying = true;
        if (pItem->IsStack())
        {
          // TODO: remove this once the app/player is capable of handling stacks immediately
          CStackDirectory dir;
          CFileItemList items;
          dir.GetDirectory(pItem->GetURL(), items);
          itemlist.Append(items);
        }
        else
          itemlist.Add(pItem);
      }
    }
    if (itemlist.Size())
    {
      if (!bAllowVideo)
      {
        if (!bypassSettings)
          return false;

        if (g_windowManager.GetActiveWindow() != WINDOW_VIDEO_FILES)
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;
      }
      g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
      g_playlistPlayer.Add(PLAYLIST_VIDEO, itemlist);
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
      g_playlistPlayer.Play(0, "");
    }
  }
  // then music
  if (!bPlaying && (bypassSettings || CSettings::GetInstance().GetBool("autorun.music")) && bAllowMusic)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsAudio())
      {
        nAddedToPlaylist++;
        g_playlistPlayer.Add(PLAYLIST_MUSIC, pItem);
      }
    }
  }
  // and finally pictures
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CSettings::GetInstance().GetBool("autorun.pictures")) && bAllowPictures)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsPicture())
      {
        bPlaying = true;
        CStdString strExec;
        strExec.Format("XBMC.RecursiveSlideShow(%s)", strDrive.c_str());
        CBuiltins::GetInstance().Execute(strExec);
        break;
      }
    }
  }

  // check subdirs if we are not playing yet
  if (!bPlaying)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr  pItem = vecItems[i];
      if (pItem->m_bIsFolder)
      {
        if (pItem->GetPath() != "." && pItem->GetPath() != ".." )
        {
          if (RunDisc(pDir, pItem->GetPath(), nAddedToPlaylist, false, bypassSettings, restart))
          {
            bPlaying = true;
            break;
          }
        }
      } // if (non system) folder
    } // for all items in directory
  } // if root folder

  return bPlaying;
}

void CAutorun::HandleAutorun()
{
  if (!m_bEnable)
  {
    CDetectDVDMedia::m_evAutorun.Reset();
    return ;
  }

  if (CDetectDVDMedia::m_evAutorun.WaitMSec(0))
  {
    ExecuteAutorun();
    CDetectDVDMedia::m_evAutorun.Reset();
  }
}

void CAutorun::Enable()
{
  m_bEnable = true;
}

void CAutorun::Disable()
{
  m_bEnable = false;
}

bool CAutorun::IsEnabled() const
{
  return m_bEnable;
}

bool CAutorun::PlayDisc(bool restart)
{
  ExecuteAutorun(true,true, restart);
  return true;
}

void CAutorun::SettingOptionAudioCdActionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(16018), AUTOCD_NONE));
  list.push_back(make_pair(g_localizeStrings.Get(14098), AUTOCD_PLAY));
#ifdef HAS_CDDA_RIPPER
  list.push_back(make_pair(g_localizeStrings.Get(14096), AUTOCD_RIP));
#endif
}

void CAutorun::SettingOptionAudioCdEncodersFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
#ifdef HAVE_LIBMP3LAME
  list.push_back(make_pair(g_localizeStrings.Get(34000), CDDARIP_ENCODER_LAME));
#endif
#ifdef HAVE_LIBVORBISENC
  list.push_back(make_pair(g_localizeStrings.Get(34001), CDDARIP_ENCODER_VORBIS));
#endif
  list.push_back(make_pair(g_localizeStrings.Get(34002), CDDARIP_ENCODER_WAV));
  list.push_back(make_pair(g_localizeStrings.Get(34005), CDDARIP_ENCODER_FLAC));
}

