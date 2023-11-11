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

#include "utils/log.h"
#include "music/GUIViewStateMusic.h"
#include "PlayListPlayer.h"
#include "GUIBaseContainer.h" // for VIEW_TYPE_*
#include "video/VideoDatabase.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Util.h"
#include "LocalizeStrings.h"

#include "FileSystem/Directory.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/VideoDatabaseDirectory.h"

using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;

int CGUIViewStateWindowMusic::GetPlaylist()
{
  //return PLAYLIST_MUSIC_TEMP;
  return PLAYLIST_MUSIC;
}

bool CGUIViewStateWindowMusic::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

bool CGUIViewStateWindowMusic::AutoPlayNextItem()
{
  return g_guiSettings.GetBool("musicplayer.autoplaynextitem") &&
         !g_guiSettings.GetBool("musicplayer.queuebydefault");
}

CStdString CGUIViewStateWindowMusic::GetLockType()
{
  return "music";
}

CStdString CGUIViewStateWindowMusic::GetExtensions()
{
  return g_settings.m_musicExtensions;
}

VECSOURCES& CGUIViewStateWindowMusic::GetSources()
{
  AddAddonsSource("audio", g_localizeStrings.Get(1038), "DefaultAddonMusic.png");
  return CGUIViewState::GetSources();
}

CGUIViewStateMusicSearch::CGUIViewStateMusicSearch(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.librarytrackformat");
  if (strTrackLeft.IsEmpty())
    strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.librarytrackformatright");
  if (strTrackRight.IsEmpty())
    strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");

  CStdString strAlbumLeft = g_advancedSettings.m_strMusicLibraryAlbumFormat;
  if (strAlbumLeft.IsEmpty())
    strAlbumLeft = "%B"; // album
  CStdString strAlbumRight = g_advancedSettings.m_strMusicLibraryAlbumFormatRight;
  if (strAlbumRight.IsEmpty())
    strAlbumRight = "%A"; // artist

  SortAttribute sortAttribute = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttribute = SortAttributeIgnoreArticle;

  AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D", "%L", "%A"));  // Title, Artist, Duration| empty, empty
  SetSortMethod(SortByTitle, sortAttribute);

  SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);

  SetSortOrder(g_settings.m_viewStateMusicNavSongs.m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSearch::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, &g_settings.m_viewStateMusicNavSongs);
}

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(items.GetPath());

  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.librarytrackformat");
  if (strTrackLeft.IsEmpty())
    strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.librarytrackformatright");
  if (strTrackRight.IsEmpty())
    strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");

  CStdString strAlbumLeft = g_advancedSettings.m_strMusicLibraryAlbumFormat;
  if (strAlbumLeft.IsEmpty())
    strAlbumLeft = "%B"; // album
  CStdString strAlbumRight = g_advancedSettings.m_strMusicLibraryAlbumFormatRight;
  if (strAlbumRight.IsEmpty())
    strAlbumRight = "%A"; // artist

  CLog::Log(LOGDEBUG,"Album format left  = [%s]", strAlbumLeft.c_str());
  CLog::Log(LOGDEBUG,"Album format right = [%s]", strAlbumRight.c_str());

  SortAttribute sortAttribute = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttribute = SortAttributeIgnoreArticle;

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_TOP100:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_GENRE:
    {
      AddSortMethod(SortByGenre, 515, LABEL_MASKS("%F", "", "%G", ""));  // Filename, empty | Genre, empty
      SetSortMethod(SortByGenre);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderAscending);
    }
    break;
  case NODE_TYPE_YEAR:
    {
      AddSortMethod(SortByLabel, 551, LABEL_MASKS("%F", "", "%Y", ""));  // Filename, empty | Year, empty
      SetSortMethod(SortByLabel);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderAscending);
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
      SetSortMethod(SortByArtist, sortAttribute);

      SetViewAsControl(g_settings.m_viewStateMusicNavArtists.m_viewMode);

      SetSortOrder(g_settings.m_viewStateMusicNavArtists.m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM:
  case NODE_TYPE_YEAR_ALBUM:
    {
      // album
      AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
      // artist
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
      // year
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));

      SetSortMethod(g_settings.m_viewStateMusicNavAlbums.m_sortDescription);

      SetViewAsControl(g_settings.m_viewStateMusicNavAlbums.m_viewMode);

      SetSortOrder(g_settings.m_viewStateMusicNavAlbums.m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(SortByNone, 552, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
      SetSortMethod(SortByNone);

      SetViewAsControl(g_settings.m_viewStateMusicNavAlbums.m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SortByNone, 552, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
      SetSortMethod(SortByNone);

      SetViewAsControl(g_settings.m_viewStateMusicNavAlbums.m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined | empty, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(g_settings.m_viewStateMusicNavAlbums.m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_SINGLES:
    {
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
      AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrackLeft, strTrackRight));
      AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
      AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title - Artist, Rating
      
      SetSortMethod(g_settings.m_viewStateMusicNavSongs.m_sortDescription);
      
      SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);
      
      SetSortOrder(g_settings.m_viewStateMusicNavSongs.m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
  case NODE_TYPE_ALBUM_TOP100_SONGS:
  case NODE_TYPE_YEAR_SONG:
  case NODE_TYPE_SONG:
    {
      AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
      AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Title, Artist, Duration| empty, empty
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
      AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrackLeft, strTrackRight));
      AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
      AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title - Artist, Rating
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year

      // the "All Albums" entries always default to SortByAlbum as this is most logical - user can always
      // change it and the change will be saved for this particular path
      if (dir.IsAllItem(items.GetPath()))
        SetSortMethod(SortByAlbum, sortAttribute);
      else
        SetSortMethod(g_settings.m_viewStateMusicNavSongs.m_sortDescription);

      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T - %A", "%V"));  // Titel - Artist, PlayCount

      SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);

      SetSortOrder(g_settings.m_viewStateMusicNavSongs.m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      AddSortMethod(SortByNone, 576, LABEL_MASKS("%T - %A", "%V"));
      SetSortMethod(SortByPlaycount);

      SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  default:
    break;
  }

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicDatabase::SaveViewState()
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(m_items.GetPath());

  switch (NodeType)
  {
    case NODE_TYPE_ARTIST:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, &g_settings.m_viewStateMusicNavArtists);
      break;
    case NODE_TYPE_ALBUM_COMPILATIONS:
    case NODE_TYPE_ALBUM:
    case NODE_TYPE_YEAR_ALBUM:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, &g_settings.m_viewStateMusicNavAlbums);
      break;
    case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    case NODE_TYPE_ALBUM_TOP100:
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
      break;
    case NODE_TYPE_SINGLES:
    case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
    case NODE_TYPE_SONG:
    case NODE_TYPE_YEAR_SONG:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, &g_settings.m_viewStateMusicNavSongs);
      break;
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    case NODE_TYPE_SONG_TOP100:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
      break;
    default:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
      break;
	}
}

CGUIViewStateMusicSmartPlaylist::CGUIViewStateMusicSmartPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttribute = SortAttributeIgnoreArticle;

  if (items.GetContent() == "songs" || items.GetContent() == "mixed") 
  {
    CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
    CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

    AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
    AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
    AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
    AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
    AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrackLeft, strTrackRight));
    AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Titel, Artist, Rating| empty, empty
    AddPlaylistOrder(items, LABEL_MASKS(strTrackLeft, strTrackRight));

    SetViewAsControl(g_settings.m_viewStateMusicNavSongs.m_viewMode);
  } 
  else if (items.GetContent() == "albums") 
  {
    CStdString strAlbumLeft = g_advancedSettings.m_strMusicLibraryAlbumFormat;
    if (strAlbumLeft.IsEmpty())
      strAlbumLeft = "%B"; // album
    CStdString strAlbumRight = g_advancedSettings.m_strMusicLibraryAlbumFormatRight;
    if (strAlbumRight.IsEmpty())
      strAlbumRight = "%A"; // artist

    // album
    AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined    
    // artist
    AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));  // Filename, empty | Userdefined, Userdefined
    // year
    AddSortMethod(SortByYear, 562, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));

    AddPlaylistOrder(items, LABEL_MASKS("%F", "", strAlbumLeft, strAlbumRight));

    SetViewAsControl(g_settings.m_viewStateMusicNavAlbums.m_viewMode);
  } 
  else 
  {
    CLog::Log(LOGERROR,"Music Smart Playlist must be one of songs, mixed or albums");
  }
  
  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSmartPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, &g_settings.m_viewStateMusicNavSongs);
}

CGUIViewStateMusicPlaylist::CGUIViewStateMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttribute = SortAttributeIgnoreArticle;

  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

  AddSortMethod(SortByPlaylistOrder, 559, LABEL_MASKS(strTrackLeft, strTrackRight));
  AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
  AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Titel, Artist, Duration| empty, empty
  AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Titel, Duration| empty, empty
  AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrackLeft, strTrackRight));
  AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Titel, Artist, Duration| empty, empty
  AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Titel, Artist, Rating| empty, empty

  SetSortMethod(SortByPlaylistOrder);
  SetViewAsControl(g_settings.m_viewStateMusicFiles.m_viewMode);
  SetSortOrder(g_settings.m_viewStateMusicFiles.m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_FILES);
}

void CGUIViewStateMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_FILES);
}

CGUIViewStateWindowMusicNav::CGUIViewStateWindowMusicNav(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttribute = SortAttributeIgnoreArticle;

  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SortByNone);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else
  {
    if (items.IsVideoDb() && items.Size() > (!g_guiSettings.GetBool("filelists.showparentdiritems")?0:1))
    {
      XFILE::VIDEODATABASEDIRECTORY::CQueryParams params;
      XFILE::CVideoDatabaseDirectory::GetQueryParams(items[!g_guiSettings.GetBool("filelists.showparentdiritems")?0:1]->GetPath(),params);
      if (params.GetMVideoId() != -1)
      {
        AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS("%T", "%Y"));  // Filename, Duration | Foldername, empty
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));
        AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%Y"));
        AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T", "%Y"));

        CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
        CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");
        AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrackLeft, strTrackRight));  // Userdefined, Userdefined| empty, empty
      }
      else
      {
        AddSortMethod(SortByLabel, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SortByLabel);
      }
    }
    else
    {
      AddSortMethod(SortByLabel, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
      SetSortMethod(SortByLabel);
    }
    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderAscending);
  }
  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::AddOnlineShares()
{
  if (!g_advancedSettings.m_bVirtualShares) return;
  for (int i = 0; i < (int)g_settings.m_musicSources.size(); ++i)
  {
    CMediaSource share = g_settings.m_musicSources.at(i);
    if (share.strPath.Find("lastfm://") == 0)//lastfm share
      m_sources.push_back(share);
  }
}

VECSOURCES& CGUIViewStateWindowMusicNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  //  Musicdb shares
  CFileItemList items;
  CDirectory::GetDirectory("musicdb://", items, "");
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName=item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage = item->GetIconImage();
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }

  //  Playlists share
  CMediaSource share;
  share.strName=g_localizeStrings.Get(136); // Playlists
  share.strPath = "special://musicplaylists/";
  share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultMusicPlaylists.png");
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  AddOnlineShares();

  // Search share
  share.strName=g_localizeStrings.Get(137); // Search
  share.strPath = "musicsearch://";
  share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultMusicSearch.png");
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  // music video share
  CVideoDatabase database;
  database.Open();
  if (database.HasContent(VIDEODB_CONTENT_MUSICVIDEOS))
  {
    share.strName = g_localizeStrings.Get(20389);
    share.strPath = "videodb://musicvideos/";
    share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultMusicVideos.png");
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }

  return CGUIViewStateWindowMusic::GetSources();
}

CGUIViewStateWindowMusicSongs::CGUIViewStateWindowMusicSongs(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS()); // Preformated
    AddSortMethod(SortByDriveType, 564, LABEL_MASKS()); // Preformated
    SetSortMethod(SortByLabel);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderAscending);
  }
  else if (items.GetPath() == "special://musicplaylists/")
  { // playlists list sorts by label only, ignoring folders
    AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(SortByLabel, SortAttributeIgnoreFolders);
  }
  else
  {
    CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
    CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

    AddSortMethod(SortByLabel, 551, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""),  // Userdefined, Userdefined | FolderName, empty
      g_guiSettings.GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);
    AddSortMethod(SortBySize, 553, LABEL_MASKS(strTrackLeft, "%I", "%L", "%I"));  // Userdefined, Size | FolderName, Size
    AddSortMethod(SortByBitrate, 623, LABEL_MASKS(strTrackLeft, "%X", "%L", "%X"));  // Userdefined, Bitrate | FolderName, Bitrate  
    AddSortMethod(SortByDate, 552, LABEL_MASKS(strTrackLeft, "%J", "%L", "%J"));  // Userdefined, Date | FolderName, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
    AddSortMethod(SortByListeners, 20455,LABEL_MASKS(strTrackLeft, "%W", "%L", "%W"));

    SetSortMethod(g_settings.m_viewStateMusicFiles.m_sortDescription);
    SetViewAsControl(g_settings.m_viewStateMusicFiles.m_viewMode);
    SetSortOrder(g_settings.m_viewStateMusicFiles.m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_MUSIC_FILES);
}

void CGUIViewStateWindowMusicSongs::SaveViewState()
{
    SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_FILES, &g_settings.m_viewStateMusicFiles);  
}

VECSOURCES& CGUIViewStateWindowMusicSongs::GetSources()
{
  AddOrReplace(g_settings.m_musicSources, CGUIViewStateWindowMusic::GetSources());
  return g_settings.m_musicSources; 
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.nowplayingtrackformat");
  if (strTrackLeft.IsEmpty())
    strTrackLeft = g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.nowplayingtrackformatright");
  if (strTrackRight.IsEmpty())
    strTrackRight = g_guiSettings.GetString("musicfiles.trackformatright");

  AddSortMethod(SortByNone, 551, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
  SetSortMethod(SortByNone);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderNone);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

int CGUIViewStateWindowMusicPlaylist::GetPlaylist()
{
  return PLAYLIST_MUSIC;
}

bool CGUIViewStateWindowMusicPlaylist::AutoPlayNextItem()
{
  return false;
}

bool CGUIViewStateWindowMusicPlaylist::HideParentDirItems()
{
  return true;
}

VECSOURCES& CGUIViewStateWindowMusicPlaylist::GetSources()
{
  m_sources.clear();
  //  Playlist share
  CMediaSource share;
  share.strPath = "playlistmusic://";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  // CGUIViewState::GetSources would add music plugins
  return m_sources;
}

CGUIViewStateMusicShoutcast::CGUIViewStateMusicShoutcast(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  /* sadly m_idepth isn't remembered when a directory is retrieved from cache */
  /* and thus this check hardly ever works, so let's just disable it for now */
  if( true || m_items.m_idepth > 1 )
  { /* station list */
    AddSortMethod(SortByLabel, 551, LABEL_MASKS("%K", "%B kbps", "%K", ""));  // Title, Bitrate | Title, nothing
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%K", "%A listeners", "%K", ""));  // Titel, Listeners | Titel, nothing
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%K", "%B kbps", "%K", ""));  // Title, Bitrate | Title, nothing

    SetSortMethod(g_settings.m_viewStateMusicShoutcast.m_sortDescription);
    SetSortOrder(g_settings.m_viewStateMusicShoutcast.m_sortDescription.sortOrder);
  }
  else
  { /* genre list */
    AddSortMethod(SortByLabel, 551, LABEL_MASKS("%K", "", "%K", ""));  // Title, nothing | Title, nothing
    SetSortMethod(SortByLabel);
    SetSortOrder(SortOrderAscending); /* maybe we should have this stored somewhere */
  }

  SetViewAsControl(DEFAULT_VIEW_LIST);
  LoadViewState(items.GetPath(), WINDOW_MUSIC_FILES);
}

bool CGUIViewStateMusicShoutcast::AutoPlayNextItem()
{
  return false;
}

void CGUIViewStateMusicShoutcast::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_FILES, &g_settings.m_viewStateMusicShoutcast);
}

CGUIViewStateMusicLastFM::CGUIViewStateMusicLastFM(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
  CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

  AddSortMethod(SortByNone, 571, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
  AddSortMethod(SortByLabel, 551, LABEL_MASKS(strTrackLeft, strTrackRight, "%L", ""));  // Userdefined, Userdefined | FolderName, empty
  AddSortMethod(SortBySize, 553, LABEL_MASKS(strTrackLeft, "%I", "%L", "%I"));  // Userdefined, Size | FolderName, Size

  SetSortMethod(g_settings.m_viewStateMusicLastFM.m_sortDescription);
  SetSortOrder(g_settings.m_viewStateMusicLastFM.m_sortDescription.sortOrder);

  SetViewAsControl(DEFAULT_VIEW_LIST);
  LoadViewState(items.GetPath(), WINDOW_MUSIC_FILES);
}

bool CGUIViewStateMusicLastFM::AutoPlayNextItem()
{
  return false;
}

void CGUIViewStateMusicLastFM::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_FILES, &g_settings.m_viewStateMusicLastFM);
}

