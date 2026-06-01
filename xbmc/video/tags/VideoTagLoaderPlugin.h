/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVideoInfoTagLoader.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

//! \brief Video tag loader from plugin source.
class CVideoTagLoaderPlugin : public VIDEO::IVideoInfoTagLoader
{
public:
  CVideoTagLoaderPlugin(const CFileItem& item, bool forceRefresh);

  virtual ~CVideoTagLoaderPlugin() {}

  //! \brief Returns whether or not read has info.
  virtual bool HasInfo();

  //! \brief Load "tag" from plugin.
  //! \param tag Tag to load info into
  virtual CInfoScanner::INFO_TYPE Load(CVideoInfoTag& tag, bool prioritise,
                               std::vector<EmbeddedArt>* = NULL);

  inline boost::movelib::unique_ptr<std::map<std::string, std::string> >& GetArt()
  {
    return m_art;
  }
protected:
  boost::movelib::unique_ptr<CVideoInfoTag> m_tag;
  boost::movelib::unique_ptr<std::map<std::string, std::string> > m_art;
  bool m_force_refresh;
};
