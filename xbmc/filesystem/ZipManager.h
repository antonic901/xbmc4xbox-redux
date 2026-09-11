/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// See http://www.pkware.com/documents/casestudies/APPNOTE.TXT
#define ZIP_LOCAL_HEADER 0x04034b50
#define ZIP_DATA_RECORD_HEADER 0x08074b50
#define ZIP_CENTRAL_HEADER 0x02014b50
#define ZIP_END_CENTRAL_HEADER 0x06054b50
#define ZIP_SPLIT_ARCHIVE_HEADER 0x30304b50
#define LHDR_SIZE 30
#define DREC_SIZE 16
#define CHDR_SIZE 46
#define ECDREC_SIZE 22

#include <cstring>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

class CURL;

static const std::string PATH_TRAVERSAL("(^|\\/|\\\\)\\.\\.($|\\/|\\\\)");

struct SZipEntry {
  unsigned int header;
  unsigned short version;
  unsigned short flags;
  unsigned short method;
  unsigned short mod_time;
  unsigned short mod_date;
  unsigned int crc32;
  unsigned int csize; // compressed size
  unsigned int usize; // uncompressed size
  unsigned short flength; // filename length
  unsigned short elength; // extra field length (local file header)
  unsigned short eclength; // extra field length (central file header)
  unsigned short clength; // file comment length (central file header)
  unsigned int lhdrOffset; // Relative offset of local header
  int64_t offset;         // offset in file to compressed data
  char name[255];

  SZipEntry()
  {
    header = 0;
    version = 0;
    flags = 0;
    method = 0;
    mod_time = 0;
    mod_date = 0;
    crc32 = 0;
    csize = 0;
    usize = 0;
    flength = 0;
    elength = 0;
    eclength = 0;
    clength = 0;
    lhdrOffset = 0;
    offset = 0;
    name[0] = '\0';
  }
};

class CZipManager
{
public:
  CZipManager();
  ~CZipManager();

  bool GetZipList(const CURL& url, std::vector<SZipEntry>& items);
  bool GetZipEntry(const CURL& url, SZipEntry& item);
  bool ExtractArchive(const std::string& strArchive, const std::string& strPath);
  bool ExtractArchive(const CURL& archive, const std::string& strPath);
  void release(const std::string& strPath); // release resources used by list zip
  static void readHeader(const char* buffer, SZipEntry& info);
  static void readCHeader(const char* buffer, SZipEntry& info);
private:
  std::map<std::string,std::vector<SZipEntry> > mZipMap;
  std::map<std::string,int64_t> mZipDate;

  template<typename T>
  static T ReadUnaligned(const void* mem)
  {
    T var;
    std::memcpy(&var, mem, sizeof(T));
    return var;
  }
};

extern CZipManager g_ZipManager;

