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
#include "Database.h"
#include "programs/ProgramInfoTag.h"
#include "video/VideoDatabase.h" // for SDbTableOffsets

typedef std::vector<CStdString> VECPROGRAMPATHS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

class CFileItem;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

#ifndef my_offsetof
#ifndef _LINUX
#define my_offsetof(TYPE, MEMBER) offsetof(TYPE, MEMBER)
#else
/*
   Custom version of standard offsetof() macro which can be used to get
   offsets of members in class for non-POD types (according to the current
   version of C++ standard offsetof() macro can't be used in such cases and
   attempt to do so causes warnings to be emitted, OTOH in many cases it is
   still OK to assume that all instances of the class has the same offsets
   for the same members).
 */
#define my_offsetof(TYPE, MEMBER) \
               ((size_t)((char *)&(((TYPE *)0x10)->MEMBER) - (char*)0x10))
#endif
#endif

typedef std::vector<CProgramInfoTag> VECPROGRAMS;

namespace PROGRAM
{
  class IProgramInfoScannerObserver;
  struct SScanSettings;
}

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define PROGRAMDB_MAX_COLUMNS 24
#define PROGRAMDB_DETAILS_FILEID      1

#define PROGRAMDB_DETAILS_MOVIE_FILE			    PROGRAMDB_MAX_COLUMNS + 2
#define PROGRAMDB_DETAILS_MOVIE_PATH			    PROGRAMDB_MAX_COLUMNS + 3
#define PROGRAMDB_DETAILS_MOVIE_PLAYCOUNT		  PROGRAMDB_MAX_COLUMNS + 4
#define PROGRAMDB_DETAILS_MOVIE_LASTPLAYED		PROGRAMDB_MAX_COLUMNS + 5
#define PROGRAMDB_DETAILS_MOVIE_DATEADDED		  PROGRAMDB_MAX_COLUMNS + 6

#define PROGRAMDB_TYPE_STRING 1
#define PROGRAMDB_TYPE_INT 2
#define PROGRAMDB_TYPE_FLOAT 3
#define PROGRAMDB_TYPE_BOOL 4
#define PROGRAMDB_TYPE_COUNT 5
#define PROGRAMDB_TYPE_STRINGARRAY 6
#define PROGRAMDB_TYPE_DATE 7
#define PROGRAMDB_TYPE_DATETIME 8

typedef enum
{
  PROGRAMDB_CONTENT_GAMES = 1,
  PROGRAMDB_CONTENT_EMULATORS = 2,
  PROGRAMDB_CONTENT_HOMEBREWS = 3
} PROGRAMDB_CONTENT_TYPE;

typedef enum
{
  PROGRAMDB_ID_MIN = -1,
  PROGRAMDB_ID_TITLE = 0,
  PROGRAMDB_ID_PLOT = 1,
  PROGRAMDB_ID_YEAR = 2,
  PROGRAMDB_ID_RATING = 3,
  PROGRAMDB_ID_THUMBURL = 4,
  PROGRAMDB_ID_IDENT = 5,
  PROGRAMDB_ID_ESRB = 6,
  PROGRAMDB_ID_ESRB_DES = 7,
  PROGRAMDB_ID_GENRE = 8,
  PROGRAMDB_ID_DEVELOPER = 9,
  PROGRAMDB_ID_PUBLISHER = 10,
  PROGRAMDB_ID_ORIGINALTITLE = 11,
  PROGRAMDB_ID_TRAILER = 12,
  PROGRAMDB_ID_FANART = 13,
  PROGRAMDB_ID_BASEPATH = 14,
  PROGRAMDB_ID_PARENTPATHID = 15,
  PROGRAMDB_ID_MAX
} PROGRAMDB_IDS;

const struct SDbTableOffsets DbGameOffsets[] =
{
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPlot) },
  { PROGRAMDB_TYPE_FLOAT, my_offsetof(CProgramInfoTag,m_fRating) },
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_iYear) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_xml) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strXBENumber) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRB) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRBDescription) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_developer) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_publisher) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_genre) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strOriginalTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_spoof) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTrailer) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_fanart.m_xml) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_basePath) },
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_parentPathID) }
};

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);
  virtual bool Open();

  bool AddTrainer(int iTitleId, const CStdString& strText);
  bool RemoveTrainer(const CStdString& strText);
  bool GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers);
  bool GetAllTrainers(std::vector<CStdString>& vecTrainers);
  bool SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  bool GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  void SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive);
  CStdString GetActiveTrainer(unsigned int iTitleId);
  bool HasTrainer(const CStdString& strTrainerPath);
  bool ItemHasTrainer(unsigned int iTitleId);

  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=-1);

  int GetTitleId(const CStdString& strFilenameAndPath);
  bool SetTitleId(const CStdString& strFilenameAndPath, int idTitle);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  bool GetXBEPathByTitleId(const int idTitle, CStdString& strPathAndFilename);

  int GetProgramInfo(CFileItem *item);
  bool AddProgramInfo(CFileItem *item, unsigned int titleID);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
  virtual int GetMinVersion() const { return 3; };
  const char *GetDefaultDBName() const { return "MyPrograms6"; };

  FILETIME TimeStampToLocalTime( unsigned __int64 timeStamp );
};
