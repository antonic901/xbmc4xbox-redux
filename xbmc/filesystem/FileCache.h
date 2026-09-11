/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CacheStrategy.h"
#include "File.h"
#include "IFile.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <atomic.h>
#include <memory>

namespace XFILE
{

  class CFileCache : public IFile, public CThread
  {
  public:
    explicit CFileCache(const unsigned int flags);
    virtual ~CFileCache();

    // CThread methods
    virtual void Process();
    virtual void OnExit();
    virtual void StopThread(bool bWait = true);

    // IFIle methods
    virtual bool Open(const CURL& url);
    virtual void Close();
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);

    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);

    virtual int64_t Seek(int64_t iFilePosition, int iWhence);
    virtual int64_t GetPosition();
    virtual int64_t GetLength();

    virtual int IoControl(EIoControl request, void* param);

    IFile *GetFileImp();

    virtual const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const;

    virtual const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string& name = "") const
    {
      return std::vector<std::string>();
    }

  private:
    boost::movelib::unique_ptr<CCacheStrategy> m_pCache;
    int m_seekPossible;
    CFile m_source;
    std::string m_sourcePath;
    CEvent m_seekEvent;
    CEvent m_seekEnded;
    int64_t m_nSeekResult;
    int64_t m_seekPos;
    int64_t m_readPos;
    int64_t m_writePos;
    unsigned m_chunkSize;
    uint32_t m_writeRate;
    uint32_t m_writeRateActual;
    uint32_t m_writeRateLowSpeed;
    int64_t m_forwardCacheSize;
    int64_t m_maxForward;
    bool m_bFilling;
    atomic<int64_t> m_fileSize;
    unsigned int m_flags;
    CCriticalSection m_sync;
    unsigned int m_processWait;
  };

}
