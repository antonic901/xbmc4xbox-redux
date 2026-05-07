/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSettings.h"

CVideoSettings::CVideoSettings()
{
  m_InterlaceMethod = VS_INTERLACEMETHOD_AUTO;
  m_ViewMode = ViewModeNormal;
  m_CustomZoomAmount = 1.0f;
  m_CustomPixelRatio = 1.0f;
  m_AudioStream = -1;
  m_SubtitleStream = -1;
  m_SubtitleDelay = 0.0f;
  m_SubtitleOn = true;
  m_SubtitleCached = false;
  m_Brightness = 50.0f;
  m_Contrast = 50.0f;
  m_Gamma = 20.0f;
  m_PostProcess = false;
  m_VolumeAmplification = 0;
  m_AudioDelay = 0.0f;
  // Xbox specific
  m_OutputToAllSpeakers = false;
  m_NoCache = false;
  m_NonInterleaved = false;
  m_FilmGrain = 0;
  m_Crop = false;
  m_CropTop = 0;
  m_CropBottom = 0;
  m_CropLeft = 0;
  m_CropRight = 0;
}

bool CVideoSettings::operator!=(const CVideoSettings &right) const
{
  if (m_InterlaceMethod != right.m_InterlaceMethod) return true;
  if (m_ViewMode != right.m_ViewMode) return true;
  if (m_CustomZoomAmount != right.m_CustomZoomAmount) return true;
  if (m_CustomPixelRatio != right.m_CustomPixelRatio) return true;
  if (m_AudioStream != right.m_AudioStream) return true;
  if (m_SubtitleStream != right.m_SubtitleStream) return true;
  if (m_SubtitleDelay != right.m_SubtitleDelay) return true;
  if (m_SubtitleOn != right.m_SubtitleOn) return true;
  if (m_SubtitleCached != right.m_SubtitleCached) return true;
  if (m_Brightness != right.m_Brightness) return true;
  if (m_Contrast != right.m_Contrast) return true;
  if (m_Gamma != right.m_Gamma) return true;
  if (m_PostProcess != right.m_PostProcess) return true;
  if (m_VolumeAmplification != right.m_VolumeAmplification) return true;
  if (m_AudioDelay != right.m_AudioDelay) return true;
  // Xbox specific
  if (m_OutputToAllSpeakers != right.m_OutputToAllSpeakers) return true;
  if (m_NoCache != right.m_NoCache) return true;
  if (m_NonInterleaved != right.m_NonInterleaved) return true;
  if (m_FilmGrain != right.m_FilmGrain) return true;
  if (m_Crop != right.m_Crop) return true;
  if (m_CropTop != right.m_CropTop) return true;
  if (m_CropBottom != right.m_CropBottom) return true;
  if (m_CropLeft != right.m_CropLeft) return true;
  if (m_CropRight != right.m_CropRight) return true;
  return false;
}
