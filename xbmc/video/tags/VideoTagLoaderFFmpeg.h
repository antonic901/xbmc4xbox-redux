/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVideoInfoTagLoader.h"
#include "cores/dvdplayer/Codecs/DllAvFormat.h"
#include "cores/dvdplayer/Codecs/DllAvUtil.h"

#include <string>
#include <vector>

namespace XFILE
{
  class CFile;
}

//! \brief Video tag loader using FFMPEG.
class CVideoTagLoaderFFmpeg : public VIDEO::IVideoInfoTagLoader
{
public:
  //! \brief Constructor.
  CVideoTagLoaderFFmpeg(const CFileItem& item,
                        const ADDON::ScraperPtr& info,
                        bool lookInFolder);

  //! \brief Destructor.
  virtual ~CVideoTagLoaderFFmpeg();

  //! \brief Returns whether or not reader has info.
  virtual bool HasInfo();

  //! \brief Load "tag" from nfo file.
  //! \brief tag Tag to load info into
  virtual CInfoScanner::INFO_TYPE Load(CVideoInfoTag& tag, bool,
                               std::vector<EmbeddedArt>* art = nullptr);

  ADDON::ScraperPtr GetScraperInfo() const { return m_info; }

protected:
  ADDON::ScraperPtr m_info; //!< Passed scraper info
  AVIOContext* m_ioctx; //!< IO context for file
  AVFormatContext* m_fctx; //!< Format context for file
  XFILE::CFile* m_file; //!< VFS file handle for file
  mutable int m_metadata_stream; //!< Stream holding kodi metadata (mkv)
  mutable bool m_override_data; //!< Data is for overriding

  DllAvFormat m_dllAvFormat;
  DllAvUtil m_dllAvUtil;

  //! \brief Load tags from MKV file.
  CInfoScanner::INFO_TYPE LoadMKV(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);

  //! \brief Load tags from MP4 file.
  CInfoScanner::INFO_TYPE LoadMP4(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);

  //! \brief Load tags from AVI file.
  CInfoScanner::INFO_TYPE LoadAVI(CVideoInfoTag& tag, std::vector<EmbeddedArt>* art);
};

