/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "ISerializable.h"
#include "utils/IArchivable.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class CStreamDetails;
class CVariant;

class CStreamDetail : public IArchivable, public ISerializable
{
public:
  enum StreamType {
    VIDEO,
    AUDIO,
    SUBTITLE
  };

  explicit CStreamDetail(StreamType type) : m_eType(type), m_pParent(NULL) {}
  virtual ~CStreamDetail() {}
  virtual bool IsWorseThan(const CStreamDetail &that) const = 0;

  const StreamType m_eType;

protected:
  CStreamDetails *m_pParent;
  friend class CStreamDetails;
};

class CStreamDetailVideo : public CStreamDetail
{
public:
  CStreamDetailVideo();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual bool IsWorseThan(const CStreamDetail &that) const;

  int m_iWidth;
  int m_iHeight;
  float m_fAspect;
  int m_iDuration;
  std::string m_strCodec;
  std::string m_strStereoMode;
  std::string m_strLanguage;
};

class CStreamDetailAudio : public CStreamDetail
{
public:
  CStreamDetailAudio();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual bool IsWorseThan(const CStreamDetail &that) const;

  int m_iChannels;
  std::string m_strCodec;
  std::string m_strLanguage;
};

class CStreamDetailSubtitle : public CStreamDetail
{
public:
  CStreamDetailSubtitle();
  CStreamDetailSubtitle& operator=(const CStreamDetailSubtitle &that);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual bool IsWorseThan(const CStreamDetail &that) const;

  std::string m_strLanguage;
};

class CStreamDetails : public IArchivable, public ISerializable
{
public:
  CStreamDetails() { Reset(); }
  CStreamDetails(const CStreamDetails &that);
  CStreamDetails& operator=(const CStreamDetails &that);
  bool operator ==(const CStreamDetails &that) const;
  bool operator !=(const CStreamDetails &that) const;

  static std::string VideoDimsToResolutionDescription(int iWidth, int iHeight);
  static std::string VideoAspectToAspectDescription(float fAspect);

  bool HasItems(void) const { return m_vecItems.size() > 0; }
  int GetStreamCount(CStreamDetail::StreamType type) const;
  int GetVideoStreamCount(void) const;
  int GetAudioStreamCount(void) const;
  int GetSubtitleStreamCount(void) const;
  const CStreamDetail* GetNthStream(CStreamDetail::StreamType type, int idx) const;

  std::string GetVideoCodec(int idx = 0) const;
  float GetVideoAspect(int idx = 0) const;
  int GetVideoWidth(int idx = 0) const;
  int GetVideoHeight(int idx = 0) const;
  int GetVideoDuration(int idx = 0) const;
  void SetVideoDuration(int idx, const int duration);
  std::string GetStereoMode(int idx = 0) const;
  std::string GetVideoLanguage(int idx = 0) const;

  std::string GetAudioCodec(int idx = 0) const;
  std::string GetAudioLanguage(int idx = 0) const;
  int GetAudioChannels(int idx = 0) const;

  std::string GetSubtitleLanguage(int idx = 0) const;

  void AddStream(CStreamDetail *item);
  void Reset(void);
  void DetermineBestStreams(void);

  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;

private:
  CStreamDetail *NewStream(CStreamDetail::StreamType type);
  std::vector<boost::shared_ptr<CStreamDetail> > m_vecItems;
  const CStreamDetailVideo *m_pBestVideo;
  const CStreamDetailAudio *m_pBestAudio;
  const CStreamDetailSubtitle *m_pBestSubtitle;
};
