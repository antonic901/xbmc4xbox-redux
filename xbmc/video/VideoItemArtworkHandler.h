/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include <boost/shared_ptr.hpp>
#include <boost/move/unique_ptr.hpp>
#include <string>
#include <vector>

class CFileItem;
class CMediaSource;

namespace VIDEO
{
class IVideoItemArtworkHandler
{
public:
  virtual ~IVideoItemArtworkHandler() {}

  virtual std::string GetCurrentArt() const = 0;

  virtual std::string GetEmbeddedArt() const { return std::string(); }
  virtual std::vector<std::string> GetRemoteArt() const { return std::vector<std::string>(); }
  virtual std::string GetLocalArt() const { return std::string(); }

  virtual std::string GetDefaultIcon() const = 0;
  virtual bool SupportsFlippedArt() const { return false; }

  virtual void AddItemPathToFileBrowserSources(std::vector<CMediaSource>& sources) {}

  virtual std::string UpdateEmbeddedArt(const std::string& art) { return art; }
  virtual std::string UpdateRemoteArt(const std::vector<std::string>& art, int index)
  {
    return art[index];
  }

  virtual void PersistArt(const std::string& art) = 0;
};

class IVideoItemArtworkHandlerFactory
{
public:
  static boost::movelib::unique_ptr<IVideoItemArtworkHandler> Create(const boost::shared_ptr<CFileItem>& item,
                                                          const std::string& mediaType,
                                                          const std::string& artType);
};

} // namespace VIDEO
