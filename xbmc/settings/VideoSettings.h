/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
// VideoSettings.h: interface for the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE=0,
  VS_INTERLACEMETHOD_AUTO=1,
  VS_INTERLACEMETHOD_RENDER_BLEND=2,

  VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED=3,
  VS_INTERLACEMETHOD_RENDER_WEAVE=4,
  
  VS_INTERLACEMETHOD_RENDER_BOB_INVERTED=5,
  VS_INTERLACEMETHOD_RENDER_BOB=6,

  VS_INTERLACEMETHOD_DEINTERLACE=7
};

enum ESCALINGMETHOD
{
  VS_SCALINGMETHOD_MAX // do not use and keep as last enum value.
};

enum ESKIPLOOPFILTER
{
  VS_SKIPLOOP_DEFAULT,
  VS_SKIPLOOP_NONREF,
  VS_SKIPLOOP_BIDIR,
  VS_SKIPLOOP_NONKEY,
  VS_SKIPLOOP_ALL
};

typedef enum {
  ViewModeNormal      = 0,
  ViewModeZoom,
  ViewModeStretch4x3,
  ViewModeStretch14x9,
  ViewModeStretch16x9,
  ViewModeOriginal,
  ViewModeCustom
} ViewMode;

class CVideoSettings
{
public:
  CVideoSettings();
  ~CVideoSettings() {};

  bool operator!=(const CVideoSettings &right) const;

  bool m_NoCache;
  bool m_NonInterleaved;
  bool m_bForceIndex;
  EINTERLACEMETHOD m_InterlaceMethod;
  ESCALINGMETHOD   m_ScalingMethod;
  float m_FilmGrain;
  int m_ViewMode;   // current view mode
  float m_CustomZoomAmount; // custom setting zoom amount
  float m_CustomPixelRatio; // custom setting pixel ratio
  float m_CustomVerticalShift; // custom setting vertical shift
  bool  m_CustomNonLinStretch;
  int m_AudioStream;
  float m_VolumeAmplification;
  bool m_OutputToAllSpeakers;
  int m_SubtitleStream;
  float m_SubtitleDelay;
  bool m_SubtitleOn;
  bool m_SubtitleCached;
  float m_Brightness;
  float m_Contrast;
  float m_Gamma;
  float m_NoiseReduction;
  bool m_PostProcess;
  float m_Sharpness;
  float m_AudioDelay;
  int m_ResumeTime;
  int m_StereoMode;
  bool m_StereoInvert;
  int m_VideoStream;
  bool m_Crop;
  int m_CropTop;
  int m_CropBottom;
  int m_CropLeft;
  int m_CropRight;

private:
};

#endif // !defined(AFX_VIDEOSETTINGS_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
