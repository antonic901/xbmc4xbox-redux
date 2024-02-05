#pragma once
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
#include "utils/Thread.h"
#include "music/MusicDatabase.h"
#include "music/infoscanner/MusicAlbumInfo.h"

class CAlbum;
class CArtist;
class CGUIDialogProgressBarHandle;

namespace MUSIC_INFO
{
class CMusicInfoScanner : CThread, public IRunnable
{
public:
  CMusicInfoScanner();
  virtual ~CMusicInfoScanner();

  void Start(const CStdString& strDirectory);
  void FetchAlbumInfo(const CStdString& strDirectory);
  void FetchArtistInfo(const CStdString& strDirectory);
  bool IsScanning();
  void Stop();

  //! \brief Set whether or not to show a progress dialog
  void ShowDialog(bool show) { m_showDialog = show; }

  static void CheckForVariousArtists(VECSONGS &songs);
  static bool HasSingleAlbum(const VECSONGS &songs, CStdString &album, CStdString &artist);

  bool DownloadAlbumInfo(const CStdString& strPath, const CStdString& strArtist, const CStdString& strAlbum, bool& bCanceled, MUSIC_GRABBER::CMusicAlbumInfo& album, CGUIDialogProgress* pDialog=NULL);
  bool DownloadArtistInfo(const CStdString& strPath, const CStdString& strArtist, bool& bCanceled, CGUIDialogProgress* pDialog=NULL);
protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  void UpdateFolderThumb(const VECSONGS &songs, const CStdString &folderPath);
  int GetPathHash(const CFileItemList &items, CStdString &hash);
  void GetAlbumArtwork(long id, const CAlbum &artist);
  void GetArtistArtwork(long id, const CStdString &artistName, const CArtist *artist = NULL);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const CStdString& strPath);

protected:
  bool m_showDialog;
  CGUIDialogProgressBarHandle* m_handle;
  int m_currentItem;
  int m_itemCount;
  bool m_bRunning;
  bool m_bCanInterrupt;
  bool m_needsCleanup;
  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
  CMusicDatabase m_musicDatabase;

  std::set<CStdString> m_pathsToScan;
  std::set<CAlbum> m_albumsToScan;
  std::set<CArtist> m_artistsToScan;
  std::set<CStdString> m_pathsToCount;
  std::vector<long> m_artistsScanned;
  std::vector<long> m_albumsScanned;
};
}
