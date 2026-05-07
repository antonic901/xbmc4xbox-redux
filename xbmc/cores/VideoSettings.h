/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE = 0,
  VS_INTERLACEMETHOD_AUTO = 1,
  VS_INTERLACEMETHOD_RENDER_BLEND = 2,
  VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED = 3,
  VS_INTERLACEMETHOD_RENDER_WEAVE = 4,
  VS_INTERLACEMETHOD_RENDER_BOB_INVERTED = 5,
  VS_INTERLACEMETHOD_RENDER_BOB = 6,
  VS_INTERLACEMETHOD_DEINTERLACE = 7,
  VS_INTERLACEMETHOD_MAX // do not use and keep as last enum value.
};

enum ViewMode
{
  ViewModeNormal = 0,
  ViewModeZoom,
  ViewModeStretch4x3,
  ViewModeStretch14x9,
  ViewModeStretch16x9,
  ViewModeOriginal,
  ViewModeCustom
};

class CVideoSettings
{
public:
  CVideoSettings();
  ~CVideoSettings() {}

  bool operator!=(const CVideoSettings &right) const;

  EINTERLACEMETHOD m_InterlaceMethod;
  int m_ViewMode; // current view mode
  float m_CustomZoomAmount; // custom setting zoom amount
  float m_CustomPixelRatio; // custom setting pixel ratio
  int m_AudioStream;
  float m_VolumeAmplification;
  int m_SubtitleStream;
  float m_SubtitleDelay;
  bool m_SubtitleOn;
  bool m_SubtitleCached;
  float m_Brightness;
  float m_Contrast;
  float m_Gamma;
  bool m_PostProcess;
  float m_AudioDelay;
  // Xbox specific
  bool m_OutputToAllSpeakers;
  bool m_NoCache;
  bool m_NonInterleaved;
  float m_FilmGrain;
  bool m_Crop;
  int m_CropTop;
  int m_CropBottom;
  int m_CropLeft;
  int m_CropRight;
};
