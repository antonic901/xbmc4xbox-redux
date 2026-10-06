/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Shared C ABI. No TagLib, STL, CRT allocations or exceptions cross this boundary. */
#ifndef TAGLIB_XBMC_H
#define TAGLIB_XBMC_H

#if defined(_WIN32) || defined(_XBOX)
#define XBMC_TAGLIB_CALL __cdecl
#define XBMC_TAGLIB_EXPORT __declspec(dllexport)
#else
#define XBMC_TAGLIB_CALL
#define XBMC_TAGLIB_EXPORT __attribute__((visibility("default")))
#endif

#if defined(_MSC_VER)
typedef __int64 XbmcTagLibOffset;
#else
typedef long long XbmcTagLibOffset;
#endif

#define XBMC_TAGLIB_ABI_VERSION 1

/* Append fields only; incompatible changes require a new ABI version. */
enum XbmcTagLibField
{
  XBMC_TAGLIB_SetTitle = 1,
  XBMC_TAGLIB_SetAlbum = 2,
  XBMC_TAGLIB_SetComment = 3,
  XBMC_TAGLIB_SetMood = 4,
  XBMC_TAGLIB_SetRecordLabel = 5,
  XBMC_TAGLIB_SetReleaseDate = 6,
  XBMC_TAGLIB_SetOriginalDate = 7,
  XBMC_TAGLIB_AddReleaseDate = 8,
  XBMC_TAGLIB_AddOriginalDate = 9,
  XBMC_TAGLIB_SetDiscSubtitle = 10,
  XBMC_TAGLIB_SetMusicBrainzAlbumID = 11,
  XBMC_TAGLIB_SetMusicBrainzReleaseGroupID = 12,
  XBMC_TAGLIB_SetMusicBrainzTrackID = 13,
  XBMC_TAGLIB_SetAlbumReleaseStatus = 14,
  XBMC_TAGLIB_SetLyrics = 15,
  XBMC_TAGLIB_SetCueSheet = 16,
  XBMC_TAGLIB_SetTrackNumber = 17,
  XBMC_TAGLIB_SetDiscNumber = 18,
  XBMC_TAGLIB_SetYear = 19,
  XBMC_TAGLIB_SetCompilation = 20,
  XBMC_TAGLIB_SetBPM = 21,
  XBMC_TAGLIB_SetUserrating = 22,
  XBMC_TAGLIB_SetLoaded = 23,
  XBMC_TAGLIB_SetDuration = 24,
  XBMC_TAGLIB_SetBitRate = 25,
  XBMC_TAGLIB_SetNoOfChannels = 26,
  XBMC_TAGLIB_SetSampleRate = 27,
  XBMC_TAGLIB_SetArtist = 28,
  XBMC_TAGLIB_SetArtistSort = 29,
  XBMC_TAGLIB_SetArtistHints = 30,
  XBMC_TAGLIB_SetAlbumArtist = 31,
  XBMC_TAGLIB_SetAlbumArtistSort = 32,
  XBMC_TAGLIB_SetAlbumArtistHints = 33,
  XBMC_TAGLIB_SetComposerSort = 34,
  XBMC_TAGLIB_SetGenre = 35,
  XBMC_TAGLIB_SetReleaseType = 36,
  XBMC_TAGLIB_SetMusicBrainzArtistID = 37,
  XBMC_TAGLIB_SetMusicBrainzAlbumArtistID = 38,
  XBMC_TAGLIB_AddArtistRole = 39,
  XBMC_TAGLIB_AddArtistRolePairs = 40,
  XBMC_TAGLIB_AddArtistInstrument = 41,
  XBMC_TAGLIB_SetReplayGain = 42,
  XBMC_TAGLIB_SetCoverArtInfo = 43,
  XBMC_TAGLIB_SetCoverArt = 44,
  XBMC_TAGLIB_Log = 45,
};

enum XbmcTagLibQuery
{
  XBMC_TAGLIB_ARTIST_EMPTY = 1,
  XBMC_TAGLIB_USER_RATING = 2
};

#pragma pack(push, 8)
/* All strings are UTF-8 and all pointers are borrowed for the duration of the
 * callback only. The receiver must copy anything it retains. SetReplayGain
 * values are album gain, album peak, track gain, track peak; NULL means absent.
 * SetCoverArtInfo uses text (MIME) and size; SetCoverArt also supplies data.
 * AddArtistRole uses text (role) and values (artists). Log uses text.
 */
typedef struct XbmcTagLibEvent
{
  unsigned int field;
  const char* const* values;
  unsigned int count;
  const char* text;
  int number;
  const unsigned char* data;
  unsigned int size;
} XbmcTagLibEvent;

typedef struct XbmcTagLibIO
{
  void* context;
  /* read: bytes read, 0 at EOF, -1 on error. Short reads are allowed. */
  int (XBMC_TAGLIB_CALL *read)(void*, void*, unsigned int);
  /* seek: absolute resulting position, -1 on error; origin 0/1/2 = set/cur/end. */
  XbmcTagLibOffset (XBMC_TAGLIB_CALL *seek)(void*, XbmcTagLibOffset, int);
  XbmcTagLibOffset (XBMC_TAGLIB_CALL *length)(void*);
} XbmcTagLibIO;

typedef struct XbmcTagLibCallbacks
{
  void* context;
  /* Return 1 on success, 0 to abort. No callback may throw across the ABI. */
  int (XBMC_TAGLIB_CALL *event)(void*, const XbmcTagLibEvent*);
  /* Return the requested integer, or -1 to abort. */
  int (XBMC_TAGLIB_CALL *query)(void*, unsigned int);
} XbmcTagLibCallbacks;

typedef struct XbmcTagLibRequest
{
  unsigned int size;
  unsigned int abi_version;
  const char* filename;
  const char* extension; /* lower case, without the leading dot */
  XbmcTagLibIO io;
  XbmcTagLibCallbacks callbacks;
  int prefer_ape;
  int read_art;
  int debug;
} XbmcTagLibRequest;
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif
XBMC_TAGLIB_EXPORT unsigned int XBMC_TAGLIB_CALL xbmc_taglib_version(void);
/* Synchronous and read-only. Returns 1 on success, 0 on error. Every TagLib
 * object is destroyed before return. error may be NULL when error_size is 0.
 * The DLL never closes the caller's stream or retains callbacks. */
XBMC_TAGLIB_EXPORT int XBMC_TAGLIB_CALL xbmc_taglib_read(
    const XbmcTagLibRequest* request, char* error, unsigned int error_size);
#ifdef __cplusplus
}
#endif
#endif
