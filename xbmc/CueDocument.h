/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/Song.h"

#include <string>
#include <vector>

#define MAX_PATH_SIZE 1024

class CueReader;

class CCueDocument
{
  class CCueTrack
  {
  public:
    CCueTrack() : iTrackNumber(0), iStartTime(0), iEndTime(0) {}
    std::string strArtist;
    std::string strTitle;
    std::string strFile;
    int iTrackNumber;
    int iStartTime;
    int iEndTime;
    ReplayGain::Info replayGain;
  };
public:
  CCueDocument() : m_iYear(0), m_iTrack(0), m_iDiscNumber(0), m_bOneFilePerTrack(false) {}
  ~CCueDocument(void);
  // USED
  bool ParseFile(const std::string &strFilePath);
  bool ParseTag(const std::string &strContent);
  void GetSongs(VECSONGS &songs);
  std::string GetMediaPath();
  std::string GetMediaTitle();
  void GetMediaFiles(std::vector<std::string>& mediaFiles);
  void UpdateMediaFile(const std::string& oldMediaFile, const std::string& mediaFile);
  bool IsOneFilePerTrack() const;
  bool IsLoaded() const;
private:
  void Clear();
  bool Parse(CueReader& reader, const std::string& strFile = std::string());

  // Member variables
  std::string m_strArtist;  // album artist
  std::string m_strAlbum;  // album title
  std::string m_strGenre;  // album genre
  int m_iYear;            //album year
  int m_iTrack;   // current track
  int m_iDiscNumber;  // Disc number
  ReplayGain::Info m_albumReplayGain;

  bool m_bOneFilePerTrack;

  // cuetrack array
  typedef std::vector<CCueTrack> Tracks;
  Tracks m_tracks;

  std::string ExtractInfo(const std::string &line);
  int ExtractTimeFromIndex(const std::string &index);
  int ExtractNumericInfo(const std::string &info);
  bool ResolvePath(std::string &strPath, const std::string &strBase);
};
