/*
 *  Copyright (C) 2003-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFont.h
\brief
*/

#include "ServiceBroker.h" // for workaround for PAL vs NTSC speeds on xbox
#include "utils/ColorUtils.h"
#include "windowing/GraphicContext.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string>
#include <vector>

typedef uint32_t character_t;
typedef std::vector<character_t> vecText;

class CGUIFontTTF;
class CGraphicContext;

///
/// \defgroup kodi_gui_font_alignment Font alignment flags
/// \ingroup python_xbmcgui_control_radiobutton
/// @{
/// @brief Flags for alignment
///
/// Flags are used as bits to have several together, e.g. `XBFONT_LEFT | XBFONT_CENTER_Y`
///
// clang-format off
const static int XBFONT_LEFT = 0; ///< Align X left
const static int XBFONT_RIGHT = (1 << 0); ///< Align X right
const static int XBFONT_CENTER_X = (1 << 1); ///< Align X center
const static int XBFONT_CENTER_Y = (1 << 2); ///< Align Y center
const static int XBFONT_TRUNCATED = (1 << 3); ///< Truncated text from right (text end with ellipses)
const static int XBFONT_JUSTIFIED = (1 << 4); ///< Justify text
const static int XBFONT_TRUNCATED_LEFT = (1 << 5); ///< Truncated text from left (text start with ellipses)
// clang-format on
/// @}

// flags for font style. lower 16 bits are the unicode code
// points, 16-24 are color bits and 24-32 are style bits
const static int FONT_STYLE_NORMAL = 0;
const static int FONT_STYLE_BOLD = (1 << 0);
const static int FONT_STYLE_ITALICS = (1 << 1);
const static int FONT_STYLE_LIGHT = (1 << 2);
const static int FONT_STYLE_UPPERCASE = (1 << 3);
const static int FONT_STYLE_LOWERCASE = (1 << 4);
const static int FONT_STYLE_CAPITALIZE = (1 << 5);
const static int FONT_STYLE_MASK = 0xFF;
const static int FONT_STYLES_COUNT = 7;

class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50,
              float pos = 0,
              int speed = defaultSpeed,
              const std::string& scrollSuffix = " | ");

  void SetSpeed(int speed)
  {
#ifdef _XBOX
    if (speed == defaultSpeed)
    {
      // HACK: workaround for PAL vs NTSC speeds on xbox
      if (CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution() == RES_PAL_4x3 ||
          CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution() == RES_PAL_16x9)
        speed = 50;
    }
#endif
    m_pixelSpeed = speed * 0.001f;
  }
  void Reset()
  {
    m_waitTime = m_initialWait;
    // pixelPos is where we start the current letter, so is measured
    // to the left of the text rendering's left edge.  Thus, a negative
    // value will mean the text starts to the right
    m_pixelPos = -m_initialPos;
    // privates:
    m_averageFrameTime = 1000.f / fabs((float)defaultSpeed);
    m_lastFrameTime = 0;
    m_textWidth = 0;
    m_totalWidth = 0;
    m_widthValid = false;
    m_loopCount = 0;
  }
  float GetPixelsPerFrame();

  float m_pixelPos;
  float m_pixelSpeed;
  unsigned int m_waitTime;
  unsigned int m_initialWait;
  float m_initialPos;
  vecText m_suffix;
  mutable float m_textWidth;
  mutable float m_totalWidth;
  mutable bool m_widthValid;

  unsigned int m_loopCount;

  static const int defaultSpeed = 60;

private:
  float m_averageFrameTime;
  uint32_t m_lastFrameTime;
};

/*!
 \ingroup textures
 \brief
 */
class CGUIFont
{
public:
  CGUIFont(const std::string& strFontName,
           uint32_t style,
           UTILS::COLOR::Color textColor,
           UTILS::COLOR::Color shadowColor,
           float lineSpacing,
           float origHeight,
           CGUIFontTTF* font);
  virtual ~CGUIFont();

  std::string& GetFontName();

  void DrawText(float x,
                float y,
                UTILS::COLOR::Color color,
                UTILS::COLOR::Color shadowColor,
                const vecText& text,
                uint32_t alignment,
                float maxPixelWidth)
  {
    std::vector<UTILS::COLOR::Color> colors;
    colors.push_back(color);
    DrawText(x, y, colors, shadowColor, text, alignment, maxPixelWidth);
  };

  void DrawText(float x,
                float y,
                const std::vector<UTILS::COLOR::Color>& colors,
                UTILS::COLOR::Color shadowColor,
                const vecText& text,
                uint32_t alignment,
                float maxPixelWidth);

  void DrawScrollingText(float x,
                         float y,
                         const std::vector<UTILS::COLOR::Color>& colors,
                         UTILS::COLOR::Color shadowColor,
                         const vecText& text,
                         uint32_t alignment,
                         float maxPixelWidth,
                         const CScrollInfo& scrollInfo);

  bool UpdateScrollInfo(const vecText& text, CScrollInfo& scrollInfo);

  float GetTextWidth(const vecText& text);
  float GetCharWidth(character_t ch);
  float GetTextHeight(int numLines) const;
  float GetTextBaseLine() const;
  float GetLineHeight() const;

  //! get font scale factor (rendered height / original height)
  float GetScaleFactor() const;

  void Begin();
  void End();

  uint32_t GetStyle() const { return m_style; }

  CGUIFontTTF* GetFont() const { return m_font; }

  void SetFont(CGUIFontTTF* font);

protected:
  std::string m_strFontName;
  uint32_t m_style;
  UTILS::COLOR::Color m_shadowColor;
  UTILS::COLOR::Color m_textColor;
  float m_lineSpacing;
  float m_origHeight;
  CGUIFontTTF* m_font; // the font object has the size information

private:
  bool ClippedRegionIsEmpty(
      CGraphicContext& context, float x, float y, float width, uint32_t alignment) const;
};
