/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// FileShoutcast.h: interface for the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "CurlFile.h"
#include "IFile.h"
#include "threads/Thread.h"

#include <memory>
#include <queue>
#include <utility>

namespace MUSIC_INFO
{
class CMusicInfoTag;
}

namespace XFILE
{

class CFileCache;

class CShoutcastFile : public IFile, public CThread
{
public:
  CShoutcastFile();
  virtual ~CShoutcastFile();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true; }
  virtual int Stat(const CURL& url, struct __stat64* buffer)
  {
    errno = ENOENT;
    return -1;
  }
  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual int IoControl(EIoControl request, void* param);

  virtual void Process();
protected:
  bool ExtractTagInfo(const char* buf);
  void ReadTruncated(char* buf2, int size);

private:
  std::string DecodeToUTF8(const std::string& str);

  CCurlFile m_file;
  std::string m_fileCharset;
  int m_metaint;
  int m_discarded; // data used for tags
  int m_currint;
  char* m_buffer; // buffer used for tags
  std::string m_title;

  CFileCache* m_cacheReader;
  CEvent m_tagChange;
  CCriticalSection m_tagSection;
  typedef std::pair<int64_t, boost::shared_ptr<MUSIC_INFO::CMusicInfoTag> > TagInfo;
  std::queue<TagInfo> m_tags; // tagpos, tag
  boost::shared_ptr<MUSIC_INFO::CMusicInfoTag> m_masterTag;
};
}

