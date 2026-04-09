/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFont.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

class CGraphicContext;

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;
struct FT_BitmapGlyphRec_;
struct FT_StrokerRec_;

typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_GlyphSlotRec_* FT_GlyphSlot;
typedef struct FT_BitmapGlyphRec_* FT_BitmapGlyph;
typedef struct FT_StrokerRec_* FT_Stroker;

typedef uint32_t character_t;
typedef std::vector<character_t> vecText;

/*!
 \ingroup textures
 \brief
 */
class CGUIFontTTF
{
  // use lookup table for the first 4096 glyphs (almost any letter or symbol) to
  // speed up GUI rendering and decrease CPU usage and less memory reallocations
  static const int MAX_GLYPH_IDX = 4096;
  static const size_t LOOKUPTABLE_SIZE = MAX_GLYPH_IDX * FONT_STYLES_COUNT;

  friend class CGUIFont;

public:
  virtual ~CGUIFontTTF();

  static CGUIFontTTF* CreateGUIFontTTF(const std::string& fontIdent);

  void Clear();

  bool Load(const std::string& strFilename,
            float height = 20.0f,
            float aspect = 1.0f,
            float lineSpacing = 1.0f,
            bool border = false);

  void Begin();
  void End();

  const std::string& GetFontIdent() const { return m_fontIdent; }

protected:
  explicit CGUIFontTTF(const std::string& fontIdent);

  struct Character
  {
    short m_offsetX;
    short m_offsetY;
    float m_left;
    float m_top;
    float m_right;
    float m_bottom;
    float m_advance;
    FT_UInt m_glyphIndex;
    character_t m_glyphAndStyle;
  };

  void AddReference();
  void RemoveReference();

  float GetTextWidthInternal(const vecText& text);
  float GetCharWidthInternal(character_t ch);
  float GetTextHeight(float lineSpacing, int numLines) const;
  float GetTextBaseLine() const { return static_cast<float>(m_cellBaseLine); }
  float GetLineHeight(float lineSpacing) const;
  float GetFontHeight() const { return m_height; }

  void DrawTextInternal(CGraphicContext& context,
                        float x,
                        float y,
                        const std::vector<UTILS::COLOR::Color>& colors,
                        const vecText& text,
                        uint32_t alignment,
                        float maxPixelWidth,
                        bool scrolling);

  float m_height;

  // Stuff for pre-rendering for speed
  Character* GetCharacter(character_t letter, FT_UInt glyphIndex);
  bool CacheCharacter(FT_UInt glyphIndex, uint32_t style, Character* ch);
  void RenderCharacter(CGraphicContext& context,
                       float posX,
                       float posY,
                       const Character* ch,
                       UTILS::COLOR::Color color,
                       bool roundX);
  void ClearCharacterCache();

  virtual bool CopyCharToTexture(FT_BitmapGlyph bitGlyph,
                                 unsigned int x1,
                                 unsigned int y1,
                                 unsigned int x2,
                                 unsigned int y2);

  // modifying glyphs
  void SetGlyphStrength(FT_GlyphSlot slot, int glyphStrength);
  static void ObliqueGlyph(FT_GlyphSlot slot);

  LPDIRECT3DTEXTURE8 m_texture; // texture that holds our rendered characters (8bit alpha only)

  unsigned int m_textureWidth; // width of our texture
  unsigned int m_textureHeight; // height of our texture
  int m_posX; // current position in the texture
  int m_posY;

  /*! \brief the height of each line in the texture.
   Accounts for spacing between lines to avoid characters overlapping.
   */
  unsigned int GetTextureLineHeight() const;
  unsigned int GetMaxFontHeight() const;

  std::vector<Character> m_char; // our characters

  // room for the first MAX_GLYPH_IDX glyphs in 7 styles
  Character* m_charquick[LOOKUPTABLE_SIZE];

  bool m_ellipseCached;
  float m_ellipsesWidth; // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;
  unsigned int m_maxFontHeight;

  unsigned int m_nestedBeginCount; // speedups

  // freetype stuff
  FT_Face m_face;
  FT_Stroker m_stroker;

  float m_originX;
  float m_originY;

  float m_textureScaleX;
  float m_textureScaleY;

  const std::string m_fontIdent;

  int m_numCharactersRendered;

private:
  float GetTabSpaceLength();

  virtual bool FirstBegin();
  virtual void LastEnd();
  CGUIFontTTF(const CGUIFontTTF&);
  CGUIFontTTF& operator=(const CGUIFontTTF&);
  int m_referenceCount;
};
