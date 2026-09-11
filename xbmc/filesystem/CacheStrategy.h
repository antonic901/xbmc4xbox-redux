/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"

#include <stdint.h>
#include <string>

namespace XFILE {

#define CACHE_RC_OK  0
#define CACHE_RC_ERROR -1
#define CACHE_RC_WOULD_BLOCK -2
#define CACHE_RC_TIMEOUT -3

class IFile; // forward declaration

class CCacheStrategy{
public:
  CCacheStrategy() : m_bEndOfInput(false) {}
  virtual ~CCacheStrategy();

  virtual int Open() = 0;
  virtual void Close() = 0;

  virtual size_t GetMaxWriteSize(const size_t& iRequestSize) = 0;
  virtual int WriteToCache(const char *pBuffer, size_t iSize) = 0;
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize) = 0;
  virtual int64_t WaitForData(uint32_t iMinAvail, unsigned int timeout) = 0;

  virtual int64_t Seek(int64_t iFilePosition) = 0;

  /*!
   \brief Reset cache position
   \param iSourcePosition position to reset to
   \return Whether a full reset was performed, or not (e.g. only cache swap)
   \sa CCacheStrategy
   */
  virtual bool Reset(int64_t iSourcePosition) = 0;

  virtual void EndOfInput(); // mark the end of the input stream so that Read will know when to return EOF
  virtual bool IsEndOfInput();
  virtual void ClearEndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition) = 0;
  virtual int64_t CachedDataStartPos() = 0;
  virtual int64_t CachedDataEndPos() = 0;
  virtual bool IsCachedPosition(int64_t iFilePosition) = 0;

  virtual CCacheStrategy *CreateNew() = 0;

  CEvent m_space;
protected:
  bool  m_bEndOfInput;
};

/**
*/
class CSimpleFileCache : public CCacheStrategy {
public:
  CSimpleFileCache();
  virtual ~CSimpleFileCache();

  virtual int Open();
  virtual void Close();

  virtual size_t GetMaxWriteSize(const size_t& iRequestSize);
  virtual int WriteToCache(const char *pBuffer, size_t iSize);
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize);
  virtual int64_t WaitForData(uint32_t iMinAvail, unsigned int timeout);

  virtual int64_t Seek(int64_t iFilePosition);
  virtual bool Reset(int64_t iSourcePosition);
  virtual void EndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition);
  virtual int64_t CachedDataStartPos();
  virtual int64_t CachedDataEndPos();
  virtual bool IsCachedPosition(int64_t iFilePosition);

  virtual CCacheStrategy *CreateNew();

  int64_t  GetAvailableRead();

protected:
  std::string m_filename;
  IFile*   m_cacheFileRead;
  IFile*   m_cacheFileWrite;
  CEvent*  m_hDataAvailEvent;
  volatile int64_t m_nStartPosition;
  volatile int64_t m_nWritePosition;
  volatile int64_t m_nReadPosition;
};

class CDoubleCache : public CCacheStrategy{
public:
  explicit CDoubleCache(CCacheStrategy *impl);
  virtual ~CDoubleCache();

  virtual int Open();
  virtual void Close();

  virtual size_t GetMaxWriteSize(const size_t& iRequestSize);
  virtual int WriteToCache(const char *pBuffer, size_t iSize);
  virtual int ReadFromCache(char *pBuffer, size_t iMaxSize);
  virtual int64_t WaitForData(uint32_t iMinAvail, unsigned int timeout);

  virtual int64_t Seek(int64_t iFilePosition);
  virtual bool Reset(int64_t iSourcePosition);
  virtual void EndOfInput();
  virtual bool IsEndOfInput();
  virtual void ClearEndOfInput();

  virtual int64_t CachedDataEndPosIfSeekTo(int64_t iFilePosition);
  virtual int64_t CachedDataStartPos();
  virtual int64_t CachedDataEndPos();
  virtual bool IsCachedPosition(int64_t iFilePosition);

  virtual CCacheStrategy *CreateNew();

protected:
  CCacheStrategy *m_pCache;
  CCacheStrategy *m_pCacheOld;
};

}

