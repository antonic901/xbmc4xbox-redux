/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace VideoAssetTypeOwner
{
  enum Type {
    UNKNOWN = -1,
    SYSTEM = 0,
    AUTO = 1,
    USER = 2
  };
}

namespace VideoAssetType
{
  enum Type {
    UNKNOWN = -1,
    VERSION = 0,
    EXTRA = 1
  };
}

namespace MediaRole
{
  enum Type {
    NewVersion,
    Parent
  };
}

static const int VIDEO_VERSION_ID_BEGIN = 40400;
static const int VIDEO_VERSION_ID_END = 40800;
static const int VIDEO_VERSION_ID_DEFAULT = VIDEO_VERSION_ID_BEGIN;
static const int VIDEO_VERSION_ID_ALL = 0;
static const std::string VIDEODB_PATH_VERSION_ID_ALL = "videodb://movies/videoversions/0";

struct VideoAssetInfo
{
  VideoAssetInfo()
  {
    m_idFile = -1;
    m_assetTypeId = -1;
    m_idMedia = -1;
    m_mediaType = MediaTypeNone;
    m_assetType = VideoAssetType::UNKNOWN;
  }

  int m_idFile;
  int m_assetTypeId;
  std::string m_assetTypeName;
  int m_idMedia;
  MediaType m_mediaType;
  VideoAssetType::Type m_assetType;
};
