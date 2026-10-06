/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef XBMC_TAGLIB_METADATA_H
#define XBMC_TAGLIB_METADATA_H

#include "taglib_xbmc.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

namespace XbmcTagLib
{
class ReplayGain
{
public:
  enum Type { ALBUM = 0, TRACK = 1 };
  ReplayGain() : values(4), present(4, false) {}
  void ParseGain(Type type, const std::string& value)
  { values[type * 2] = value; present[type * 2] = true; }
  void ParsePeak(Type type, const std::string& value)
  { values[type * 2 + 1] = value; present[type * 2 + 1] = true; }
  std::vector<std::string> values;
  std::vector<bool> present;
};

class Metadata
{
public:
  explicit Metadata(const XbmcTagLibRequest& request) : m_request(request) {}
  void Send(const XbmcTagLibEvent& event)
  {
    if (!m_request.callbacks.event(m_request.callbacks.context, &event))
      throw std::runtime_error("XBMC metadata callback failed");
  }
  int Query(unsigned int field)
  {
    int result = m_request.callbacks.query(m_request.callbacks.context, field);
    if (result < 0)
      throw std::runtime_error("XBMC metadata query failed");
    return result;
  }
  void Strings(unsigned int field, const std::vector<std::string>& values,
               const char* text = NULL)
  {
    std::vector<const char*> pointers;
    for (size_t i = 0; i < values.size(); ++i)
      pointers.push_back(values[i].c_str());
    XbmcTagLibEvent event = {};
    event.field = field;
    event.values = pointers.empty() ? NULL : &pointers[0];
    event.count = static_cast<unsigned int>(pointers.size());
    event.text = text;
    Send(event);
  }
  void Text(unsigned int field, const std::string& value)
  { Strings(field, std::vector<std::string>(1, value)); }
  void Number(unsigned int field, int value)
  {
    XbmcTagLibEvent event = {};
    event.field = field;
    event.number = value;
    Send(event);
  }
  void SetTitle(const std::string& value) { Text(XBMC_TAGLIB_SetTitle, value); }
  void SetAlbum(const std::string& value) { Text(XBMC_TAGLIB_SetAlbum, value); }
  void SetComment(const std::string& value) { Text(XBMC_TAGLIB_SetComment, value); }
  void SetMood(const std::string& value) { Text(XBMC_TAGLIB_SetMood, value); }
  void SetRecordLabel(const std::string& value) { Text(XBMC_TAGLIB_SetRecordLabel, value); }
  void SetReleaseDate(const std::string& value) { Text(XBMC_TAGLIB_SetReleaseDate, value); }
  void SetOriginalDate(const std::string& value) { Text(XBMC_TAGLIB_SetOriginalDate, value); }
  void AddReleaseDate(const std::string& value) { Text(XBMC_TAGLIB_AddReleaseDate, value); }
  void AddOriginalDate(const std::string& value) { Text(XBMC_TAGLIB_AddOriginalDate, value); }
  void SetDiscSubtitle(const std::string& value) { Text(XBMC_TAGLIB_SetDiscSubtitle, value); }
  void SetMusicBrainzAlbumID(const std::string& value) { Text(XBMC_TAGLIB_SetMusicBrainzAlbumID, value); }
  void SetMusicBrainzReleaseGroupID(const std::string& value) { Text(XBMC_TAGLIB_SetMusicBrainzReleaseGroupID, value); }
  void SetMusicBrainzTrackID(const std::string& value) { Text(XBMC_TAGLIB_SetMusicBrainzTrackID, value); }
  void SetAlbumReleaseStatus(const std::string& value) { Text(XBMC_TAGLIB_SetAlbumReleaseStatus, value); }
  void SetLyrics(const std::string& value) { Text(XBMC_TAGLIB_SetLyrics, value); }
  void SetCueSheet(const std::string& value) { Text(XBMC_TAGLIB_SetCueSheet, value); }
  void SetTrackNumber(int value) { Number(XBMC_TAGLIB_SetTrackNumber, value); }
  void SetDiscNumber(int value) { Number(XBMC_TAGLIB_SetDiscNumber, value); }
  void SetYear(int value) { Number(XBMC_TAGLIB_SetYear, value); }
  void SetCompilation(int value) { Number(XBMC_TAGLIB_SetCompilation, value); }
  void SetBPM(int value) { Number(XBMC_TAGLIB_SetBPM, value); }
  void SetUserrating(int value) { Number(XBMC_TAGLIB_SetUserrating, value); }
  void SetLoaded(int value) { Number(XBMC_TAGLIB_SetLoaded, value); }
  void SetDuration(int value) { Number(XBMC_TAGLIB_SetDuration, value); }
  void SetBitRate(int value) { Number(XBMC_TAGLIB_SetBitRate, value); }
  void SetNoOfChannels(int value) { Number(XBMC_TAGLIB_SetNoOfChannels, value); }
  void SetSampleRate(int value) { Number(XBMC_TAGLIB_SetSampleRate, value); }
  void SetArtist(const std::vector<std::string>& values) { Strings(XBMC_TAGLIB_SetArtist, values); }
  void SetMusicBrainzArtistID(const std::vector<std::string>& values) { Strings(XBMC_TAGLIB_SetMusicBrainzArtistID, values); }
  void SetMusicBrainzAlbumArtistID(const std::vector<std::string>& values) { Strings(XBMC_TAGLIB_SetMusicBrainzAlbumArtistID, values); }
  void SetArtist(const std::string& value) { Text(XBMC_TAGLIB_SetArtist, value); }
  void SetGenre(const std::string& value, bool) { Text(XBMC_TAGLIB_SetGenre, value); }
  void SetReplayGain(const ReplayGain& gain)
  {
    const char* values[4];
    for (unsigned int i = 0; i < 4; ++i)
      values[i] = gain.present[i] ? gain.values[i].c_str() : NULL;
    XbmcTagLibEvent event = {};
    event.field = XBMC_TAGLIB_SetReplayGain;
    event.values = values;
    event.count = 4;
    Send(event);
  }
  void SetCoverArtInfo(unsigned int size, const std::string& mime)
  {
    XbmcTagLibEvent event = {};
    event.field = XBMC_TAGLIB_SetCoverArtInfo;
    event.text = mime.c_str();
    event.size = size;
    Send(event);
  }
  bool Debug() const { return m_request.debug != 0; }
  void Log(const char* format, ...)
  {
    char message[1024];
    va_list args;
    va_start(args, format);
#if defined(_MSC_VER) && _MSC_VER < 1900
    _vsnprintf(message, sizeof(message), format, args);
#else
    vsnprintf(message, sizeof(message), format, args);
#endif
    va_end(args);
    message[sizeof(message) - 1] = '\0';
    XbmcTagLibEvent event = {};
    event.field = XBMC_TAGLIB_Log;
    event.text = message;
    Send(event);
  }
private:
  const XbmcTagLibRequest& m_request;
};

class Artwork
{
public:
  explicit Artwork(Metadata& tag) : m_tag(tag) {}
  void Set(const unsigned char* data, unsigned int size, const std::string& mime)
  {
    XbmcTagLibEvent event = {};
    event.field = XBMC_TAGLIB_SetCoverArt;
    event.data = data;
    event.size = size;
    event.text = mime.c_str();
    m_tag.Send(event);
  }
private:
  Metadata& m_tag;
};

// These helpers preserve the host's splitting, joining and role rules.
inline void SetArtist(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetArtist, values); }
inline void SetArtistSort(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetArtistSort, values); }
inline void SetArtistHints(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetArtistHints, values); }
inline void SetAlbumArtist(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetAlbumArtist, values); }
inline void SetAlbumArtistSort(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetAlbumArtistSort, values); }
inline void SetAlbumArtistHints(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetAlbumArtistHints, values); }
inline void SetComposerSort(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetComposerSort, values); }
inline void SetReleaseType(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_SetReleaseType, values); }
inline void AddArtistInstrument(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_AddArtistInstrument, values); }
inline void AddArtistRole(Metadata& tag, const std::string& role,
                          const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_AddArtistRole, values, role.c_str()); }
inline void AddArtistRole(Metadata& tag, const std::vector<std::string>& values)
{ tag.Strings(XBMC_TAGLIB_AddArtistRolePairs, values); }
}
#endif
