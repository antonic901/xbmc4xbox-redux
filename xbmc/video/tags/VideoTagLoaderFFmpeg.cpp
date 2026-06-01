/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoTagLoaderFFmpeg.h"

#include "FileItem.h"
#include "NfoFile.h"
#include "addons/Scraper.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

using namespace XFILE;

static int vfs_file_read(void *h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t vfs_file_seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if (whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

CVideoTagLoaderFFmpeg::CVideoTagLoaderFFmpeg(const CFileItem& item,
                                             const ADDON::ScraperPtr& info,
                                             bool lookInFolder)
  : IVideoInfoTagLoader(item, info, lookInFolder)
  , m_info(info)
  , m_ioctx(NULL)
  , m_fctx(NULL)
  , m_file(NULL)
  , m_metadata_stream(-1)
  , m_override_data(false)
{
  if (!m_dllAvUtil.Load() || !m_dllAvFormat.Load())
  {
    return;
  }

  std::string filename =
      item.IsStack() ? CStackDirectory::GetFirstStackedFile(item.GetPath()) : item.GetPath();

  m_file = new CFile;

  if (!m_file->Open(filename))
  {
    delete m_file;
    m_file = NULL;
    return;
  }

  int blockSize = m_file->GetChunkSize();
  int bufferSize = blockSize > 1 ? blockSize : 4096;
  uint8_t* buffer = (uint8_t*)m_dllAvUtil.av_malloc(bufferSize);
  m_ioctx = m_dllAvFormat.avio_alloc_context(buffer, bufferSize, 0,
                               m_file, vfs_file_read, NULL,
                               vfs_file_seek);

  m_fctx = m_dllAvFormat.avformat_alloc_context();
  m_fctx->pb = m_ioctx;

  if (m_file->IoControl(IOCTRL_SEEK_POSSIBLE, NULL) != 1)
    m_ioctx->seekable = 0;

  AVInputFormat* iformat = NULL;
  m_dllAvFormat.av_probe_input_buffer(m_ioctx, &iformat, m_item.GetPath().c_str(), NULL, 0, 0);
  if (m_dllAvFormat.avformat_open_input(&m_fctx, m_item.GetPath().c_str(), iformat, NULL) < 0)
  {
    delete m_file;
    m_file = NULL;
  }
}

CVideoTagLoaderFFmpeg::~CVideoTagLoaderFFmpeg()
{
  if (m_fctx)
    m_dllAvFormat.avformat_close_input(&m_fctx);
  if (m_ioctx)
  {
    m_dllAvUtil.av_free(m_ioctx->buffer);
    m_dllAvUtil.av_free(m_ioctx);
  }
  delete m_file;

  m_dllAvFormat.Unload();
  m_dllAvUtil.Unload();
}

bool CVideoTagLoaderFFmpeg::HasInfo()
{
  if (!m_file)
    return false;

  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    AVDictionaryEntry* avtag;
    avtag = m_dllAvUtil.av_dict_get(m_fctx->streams[i]->metadata, "filename", NULL, AV_DICT_IGNORE_SUFFIX);
    if (avtag && strcmp(avtag->value,"kodi-metadata") == 0)
    {
      m_metadata_stream = i;
      return true;
    }
    else if (avtag && strcmp(avtag->value,"kodi-override-metadata") == 0)
    {
      m_metadata_stream = i;
      m_override_data = true;
      return true;
    }
  }

  AVDictionaryEntry* avtag = NULL;
  if (m_item.IsType(".mkv"))
  {
    avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "IMDBURL", NULL, AV_DICT_IGNORE_SUFFIX);
    if (!avtag)
      avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "TMDBURL", NULL, AV_DICT_IGNORE_SUFFIX);
    if (!avtag)
      avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "TITLE", NULL, AV_DICT_IGNORE_SUFFIX);
  } else if (m_item.IsType(".mp4") || m_item.IsType(".avi"))
    avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);

  return avtag != NULL;
}

CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::Load(CVideoInfoTag& tag,
                                                    bool, std::vector<EmbeddedArt>* art)
{
  if (m_item.IsType(".mkv"))
    return LoadMKV(tag, art);
  else if (m_item.IsType(".mp4"))
    return LoadMP4(tag, art);
  else if (m_item.IsType(".avi"))
    return LoadAVI(tag, art);
  else
    return CInfoScanner::NO_NFO;

}

CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadMKV(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  // embedded art
  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    if ((m_fctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;
    AVDictionaryEntry* avtag;
    avtag = m_dllAvUtil.av_dict_get(m_fctx->streams[i]->metadata, "filename", NULL, AV_DICT_IGNORE_SUFFIX);
    std::string value;
    if (avtag)
      value =  avtag->value;
    avtag = m_dllAvUtil.av_dict_get(m_fctx->streams[i]->metadata, "mimetype", NULL, AV_DICT_IGNORE_SUFFIX);
    if (!value.empty() && avtag)
    {
      std::string type;
      if (value == "fanart.png" || value == "fanart.jpg")
        type = "fanart";
      else if (value == "cover.png" || value == "cover.jpg")
        type = "poster";
      else if (value == "small_cover.png" || value == "small_cover.jpg")
        type = "thumb";
      if (type.empty())
        continue;
      size_t size = m_fctx->streams[i]->attached_pic.size;
      if (art)
        art->push_back(EmbeddedArt(m_fctx->streams[i]->attached_pic.data, size, avtag->value, type));
      else
        tag.m_coverArt.push_back(EmbeddedArtInfo(size, avtag->value, type));
    }
  }

  if (m_metadata_stream != -1)
  {
    CNfoFile nfo;
    uint8_t *data = m_fctx->streams[m_metadata_stream]->codec->extradata;
    const char* content = reinterpret_cast<const char*>(data);
    if (!m_override_data)
    {
      nfo.GetDetails(tag, content);
      return CInfoScanner::FULL_NFO;
    }
    else
    {
      nfo.Create(content, m_info);
      m_url = nfo.ScraperUrl();
      return CInfoScanner::URL_NFO;
    }
  }

  AVDictionaryEntry* avtag = NULL;
  bool hastag = false;
  while ((avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (StringUtils::CompareNoCase(avtag->key, "imdburl") == 0 ||
        StringUtils::CompareNoCase(avtag->key, "tmdburl") == 0)
    {
      CNfoFile nfo;
      nfo.Create(avtag->value, m_info);
      m_url = nfo.ScraperUrl();
      return CInfoScanner::URL_NFO;
    }
    else if (StringUtils::CompareNoCase(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (StringUtils::CompareNoCase(avtag->key, "director") == 0)
    {
      std::vector<std::string> dirs = StringUtils::Split(avtag->value, " / ");
      tag.SetDirector(dirs);
    }
    else if (StringUtils::CompareNoCase(avtag->key, "date_released") == 0)
      tag.SetYear(atoi(avtag->value));
    hastag = true;
  }

  return hastag ? CInfoScanner::TITLE_NFO : CInfoScanner::NO_NFO;
}

// https://wiki.multimedia.cx/index.php/FFmpeg_Metadata
CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadMP4(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  bool hasfull = false;
  AVDictionaryEntry* avtag = NULL;
  // If either description or synopsis is found, assume user wants to use the tag info only
  while ((avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (strcmp(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (strcmp(avtag->key, "composer") == 0)
      tag.SetWritingCredits(StringUtils::Split(avtag->value, " / "));
    else if (strcmp(avtag->key, "genre") == 0)
      tag.SetGenre(StringUtils::Split(avtag->value, " / "));
    else if (strcmp(avtag->key,"date") == 0)
      tag.SetYear(atoi(avtag->value));
    else if (strcmp(avtag->key, "description") == 0)
    {
      tag.SetPlotOutline(avtag->value);
      hasfull = true;
    }
    else if (strcmp(avtag->key, "synopsis") == 0)
    {
      tag.SetPlot(avtag->value);
      hasfull = true;
    }
    else if (strcmp(avtag->key, "track") == 0)
      tag.m_iTrack = atoi(avtag->value);
    else if (strcmp(avtag->key, "album") == 0)
      tag.SetAlbum(avtag->value);
    else if (strcmp(avtag->key, "artist") == 0)
      tag.SetArtist(StringUtils::Split(avtag->value, " / "));
  }

  for (size_t i = 0; i < m_fctx->nb_streams; ++i)
  {
    if ((m_fctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;

    size_t size = m_fctx->streams[i]->attached_pic.size;
    const std::string type = "poster";
    if (art)
      art->push_back(EmbeddedArt(m_fctx->streams[i]->attached_pic.data, size, "image/png", type));
    else
      tag.m_coverArt.push_back(EmbeddedArtInfo(size, "image/png", type));
  }

  return hasfull ? CInfoScanner::FULL_NFO : CInfoScanner::TITLE_NFO;
}

// https://wiki.multimedia.cx/index.php/FFmpeg_Metadata#AVI
CInfoScanner::INFO_TYPE CVideoTagLoaderFFmpeg::LoadAVI(CVideoInfoTag& tag,
                                                       std::vector<EmbeddedArt>* art)
{
  AVDictionaryEntry* avtag = NULL;
  while ((avtag = m_dllAvUtil.av_dict_get(m_fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (strcmp(avtag->key, "title") == 0)
      tag.SetTitle(avtag->value);
    else if (strcmp(avtag->key,"date") == 0)
      tag.SetYear(atoi(avtag->value));
  }

  return CInfoScanner::TITLE_NFO;
}
