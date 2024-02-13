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

#include "interfaces/Builtins.h"
#include "Autorun.h"
#include "Application.h"
#include "storage/DetectDVDType.h"
#include "Util.h"
#include "GUIPassword.h"
#include "PlayListPlayer.h"
#include "xbox/xbeheader.h"
#include "filesystem/StackDirectory.h"
#include "ProgramDatabase.h"
#include "utils/Trainer.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "playlists/PlayList.h"
#include "guilib/LocalizeStrings.h"

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

void CAutorun::ExecuteAutorun( bool bypassSettings, bool ignoreplaying )
{
  if ((!ignoreplaying && (g_application.IsPlayingAudio() || g_application.IsPlayingVideo() || g_windowManager.HasModalDialog())) || g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN)
    return ;

  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaverWindow();  // turn off the screensaver if it's active

  if ( pInfo->IsAudio( 1 ) )
  {
    if( !bypassSettings && !CSettings::Get().GetBool("autorun.cdda") )
      return;

    if (!g_passwordManager.IsMasterLockUnlocked(false))
      if (CProfilesManager::Get().GetCurrentProfile().musicLocked())
        return ;

    RunCdda();
  }
  else if (pInfo->IsUDFX( 1 ) || pInfo->IsUDF(1) || (pInfo->IsISOUDF(1) && g_advancedSettings.m_detectAsUdf))
  {
    RunXboxCd(bypassSettings);
  }
  else if (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1))
  {
    RunISOMedia(bypassSettings);
  }
  else
  {
    RunXboxCd(bypassSettings);
  }
}

void CAutorun::ExecuteXBE(const CStdString &xbeFile)
{
  int iRegion;
  if (CSettings::Get().GetBool("myprograms.gameautoregion"))
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
  DWORD dwTitleId = CUtil::GetXbeID(xbeFile);
  CStdString strTrainer = database.GetActiveTrainer(dwTitleId);
  if (strTrainer != "")
  {
    CTrainer trainer;
    if (trainer.Load(strTrainer))
    {
      database.GetTrainerOptions(strTrainer,dwTitleId,trainer.GetOptions(),trainer.GetNumberOfOptions());
      CUtil::InstallTrainer(trainer);
    }
  }

  database.Close();
  CUtil::RunXBE(xbeFile.c_str(), NULL,F_VIDEO(iRegion));
}

void CAutorun::RunXboxCd(bool bypassSettings)
{
  if ( CFile::Exists("D:\\default.xbe") )
  {
    if (!CSettings::Get().GetBool("autorun.xbox") && !bypassSettings)
      return;

    if (!g_passwordManager.IsMasterLockUnlocked(false))
      if (CProfilesManager::Get().GetCurrentProfile().programsLocked())
        return;

    ExecuteXBE("D:\\default.xbe");
    return;
  }

  if ( !CSettings::Get().GetBool("autorun.dvd") && !CSettings::Get().GetBool("autorun.vcd") && !CSettings::Get().GetBool("autorun.video") && !CSettings::Get().GetBool("autorun.music") && !CSettings::Get().GetBool("autorun.pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "D:\\" ) );
  bool bPlaying = RunDisc(pDir.get(), "D:\\", nAddedToPlaylist, true, bypassSettings);
  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
    g_windowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    g_playlistPlayer.Play( nSize );
  }
}

void CAutorun::RunCdda()
{
  CFileItemList vecItems;

  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "cdda://local/" ) );
  if ( !pDir->GetDirectory( "cdda://local/", vecItems ) )
    return ;

  if ( vecItems.Size() <= 0 )
    return ;

  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Add(PLAYLIST_MUSIC, vecItems);
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Play();
}

void CAutorun::RunISOMedia(bool bypassSettings)
{
  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "iso9660://" ));
  bool bPlaying = RunDisc(pDir.get(), "iso9660://", nAddedToPlaylist, true, bypassSettings);
  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
    g_windowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    g_playlistPlayer.Play(nSize);
  }
}
bool CAutorun::RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */)
{
  bool bPlaying(false);
  CFileItemList vecItems;
  CFileItemList itemlist(vecItems);
  char szSlash = '\\';
  if (strDrive.Find("iso9660") != -1) szSlash = '/';

  if ( !pDir->GetDirectory( strDrive, vecItems ) )
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

  if( bRoot )
  {

    // check root folders first, for normal structured dvd's
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];

      if (pItem->m_bIsFolder && pItem->GetPath() != "." && pItem->GetPath() != "..")
      {
        if (pItem->GetPath().Find( "VIDEO_TS" ) != -1 && bAllowVideo
        && (bypassSettings || CSettings::Get().GetBool("autorun.dvd")))
        {
          CUtil::PlayDVD();
          bPlaying = true;
          return true;
        }
        else if (pItem->GetPath().Find("MPEGAV") != -1 && bAllowVideo
             && (bypassSettings || CSettings::Get().GetBool("autorun.vcd")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, ".dat");
          if (items.Size())
          {
            items.Sort(SortByLabel, SortOrderAscending);
            g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Play(0);
            bPlaying = true;
            return true;
          }
        }
        else if (pItem->GetPath().Find("MPEG2") != -1 && bAllowVideo
              && (bypassSettings || CSettings::Get().GetBool("autorun.vcd")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->GetPath(), items, ".mpg");
          if (items.Size())
          {
            items.Sort(SortByLabel, SortOrderDescending);
            g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Add(PLAYLIST_VIDEO, items);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
            g_playlistPlayer.Play(0);
            bPlaying = true;
            return true;
          }
        }
        else if (pItem->GetPath().Find("PICTURES") != -1 && bAllowPictures
              && (bypassSettings || CSettings::Get().GetBool("autorun.pictures")))
        {
          bPlaying = true;
          CStdString strExec;
          strExec.Format("XBMC.RecursiveSlideShow(%s)", pItem->GetPath().c_str());
          CBuiltins::Execute(strExec);
          return true;
        }
      }
    }
  }

  // check video first
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CSettings::Get().GetBool("autorun.video")))
  {
    // stack video files
    CFileItemList tempItems;
    tempItems.Append(vecItems);
    if (CSettings::Get().GetBool("myvideos.stackvideos"))
      tempItems.Stack();
    itemlist.Clear();

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
          dir.GetDirectory(pItem->GetPath(), items);
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
      g_playlistPlayer.Play(0);
    }
  }
  // then music
  if (!bPlaying && (bypassSettings || CSettings::Get().GetBool("autorun.music")) && bAllowMusic)
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
  if (!nAddedToPlaylist && !bPlaying && (bypassSettings || CSettings::Get().GetBool("autorun.pictures")) && bAllowPictures)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItemPtr pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsPicture())
      {
        bPlaying = true;
        CStdString strExec;
        strExec.Format("XBMC.RecursiveSlideShow(%s)", strDrive.c_str());
        CBuiltins::Execute(strExec);
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
          if (RunDisc(pDir, pItem->GetPath(), nAddedToPlaylist, false, bypassSettings))
          {
            bPlaying = true;
            break;
          }
        }
      }
    }
  }

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
    ExecuteAutorun();
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

bool CAutorun::PlayDisc()
{
  ExecuteAutorun(true,true);
  return true;
}

void CAutorun::SettingOptionAudioCdActionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  list.push_back(make_pair(g_localizeStrings.Get(16018), AUTOCD_NONE));
  list.push_back(make_pair(g_localizeStrings.Get(14098), AUTOCD_PLAY));
#ifdef HAS_CDDA_RIPPER
  list.push_back(make_pair(g_localizeStrings.Get(14096), AUTOCD_RIP));
#endif
}

void CAutorun::SettingOptionAudioCdEncodersFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
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

