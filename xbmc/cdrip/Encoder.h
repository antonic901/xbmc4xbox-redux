/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IEncoder.h"

#include <boost/move/unique_ptr.hpp>
#include <stdint.h>
#include <stdio.h>
#include <string>

enum CDDARipEncoder
{
  CDDARIP_ENCODER_LAME    = 0,
  CDDARIP_ENCODER_VORBIS,
  CDDARIP_ENCODER_WAV,
  CDDARIP_ENCODER_FLAC
};

enum CDDARipQuality
{
  CDDARIP_QUALITY_CBR      = 0,
  CDDARIP_QUALITY_MEDIUM,
  CDDARIP_QUALITY_STANDARD,
  CDDARIP_QUALITY_EXTREME
};

namespace XFILE
{
class CFile;
}

namespace KODI
{
namespace CDRIP
{

const size_t WRITEBUFFER_SIZE = 131072; // 128k buffer

class CEncoder : public IEncoder
{
public:
  CEncoder();
  virtual ~CEncoder();

  bool EncoderInit(const std::string& strFile, int iInChannels, int iInRate, int iInBits);
  ssize_t EncoderEncode(uint8_t* pbtStream, size_t nNumBytesRead);
  bool EncoderClose();

  void SetComment(const std::string& str) { m_strComment = str; }
  void SetArtist(const std::string& str) { m_strArtist = str; }
  void SetTitle(const std::string& str) { m_strTitle = str; }
  void SetAlbum(const std::string& str) { m_strAlbum = str; }
  void SetAlbumArtist(const std::string& str) { m_strAlbumArtist = str; }
  void SetGenre(const std::string& str) { m_strGenre = str; }
  void SetTrack(const std::string& str) { m_strTrack = str; }
  void SetTrackLength(int length) { m_iTrackLength = length; }
  void SetYear(const std::string& str) { m_strYear = str; }

protected:
  virtual ssize_t Write(const uint8_t* pBuffer, size_t iBytes);
  virtual ssize_t Seek(ssize_t iFilePosition, int iWhence);
  virtual int64_t GetLength(); // CEncoderFlac
  virtual bool CloseFile(); // CEncoderLame, CEncoderVorbis

private:
  bool FileCreate(const std::string& filename);
  bool FileClose();
  ssize_t FileWrite(const uint8_t* pBuffer, size_t iBytes);
  ssize_t FlushStream();

  boost::movelib::unique_ptr<XFILE::CFile> m_file;

  uint8_t m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  size_t m_dwWriteBufferPointer;
};

} /* namespace CDRIP */
} /* namespace KODI */
