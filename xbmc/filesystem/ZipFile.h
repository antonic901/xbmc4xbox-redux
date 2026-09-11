/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "File.h"
#include "IFile.h"
#include "ZipManager.h"

#include <zlib/zlib.h>

namespace XFILE
{
  class CZipFile : public IFile
  {
  public:
    CZipFile();
    virtual ~CZipFile();

    virtual int64_t GetPosition();
    virtual int64_t GetLength();
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(struct __stat64* buffer);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
    //virtual bool ReadString(char *szLine, int iLineLength);
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();

    //NOTE: gzip doesn't work. use DecompressGzip() instead
    int UnpackFromMemory(std::string& strDest, const std::string& strInput, bool isGZ=false);

    /*! Decompress gzip encoded buffer in-memory */
    static bool DecompressGzip(const std::string& in, std::string& out);

  private:
    bool InitDecompress();
    bool FillBuffer();
    void DestroyBuffer(void* lpBuffer, int iBufSize);
    CFile mFile;
    SZipEntry mZipItem;
    int64_t m_iFilePos; // position in _uncompressed_ data read
    int64_t m_iZipFilePos; // position in _compressed_ data
    int m_iAvailBuffer;
    z_stream m_ZStream;
    char m_szBuffer[65535];     // 64k buffer for compressed data
    char* m_szStringBuffer;
    char* m_szStartOfStringBuffer; // never allocated!
    size_t m_iDataInStringBuffer;
    int m_iRead;
    bool m_bFlush;
    bool m_bCached;
  };
}

