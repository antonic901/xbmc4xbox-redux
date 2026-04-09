/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "include.h"
#include "GUIFontTTF.h"

#include "GUIFontManager.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SystemClock.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <math.h>
#include <memory>
#include <queue>
#include <utility>

// stuff for freetype
#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_STROKER_H

#define USE_RELEASE_LIBS

// our free type library (debug)
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"lib/freetype/libs/freetype2410_D.lib")
#else
  #pragma comment (lib,"lib/freetype/libs/freetype2410.lib")
#endif

namespace
{
const int VERTEX_PER_GLYPH = 4; // number of vertex for each glyph
const int CHARS_PER_TEXTURE_LINE = 20; // number characters to cache per texture line
const int MAX_TRANSLATED_VERTEX = 32; // max number of structs CTranslatedVertices expect to use
const int MAX_GLYPHS_PER_TEXT_LINE = 1024; // max number of glyphs per text line expect to use
const unsigned int SPACING_BETWEEN_CHARACTERS_IN_TEXTURE = 1;
const int CHAR_CHUNK = 64; // 64 chars allocated at a time (2048 bytes)
const int GLYPH_STRENGTH_BOLD = 24;
const int GLYPH_STRENGTH_LIGHT = -48;
const int TAB_SPACE_LENGTH = 4;

// \brief Check for conflicting alignments
void ValidateAlignments(uint32_t& aligns)
{
  // Validate the horizontal alignment (XBFONT_LEFT is implicit unless otherwise specified)
  {
    const uint32_t hAligns = XBFONT_RIGHT | XBFONT_CENTER_X | XBFONT_JUSTIFIED;
    const uint32_t commonFlags = hAligns & aligns;
    // Check if at least 2 bits are set, it means multiple aligns
    if ((commonFlags & (commonFlags - 1)) != 0)
    {
      CLog::Log(LOGERROR, "Text with invalid multiple horizontal alignments");
      aligns &= ~commonFlags;
    }
  }

  // Validate truncate alignment
  {
    const uint32_t truncateAligns = XBFONT_TRUNCATED | XBFONT_TRUNCATED_LEFT;
    const uint32_t commonFlags = truncateAligns & aligns;
    // Check if at least 2 bits are set, it means multiple aligns
    if ((commonFlags & (commonFlags - 1)) != 0)
    {
      CLog::Log(LOGERROR, "Text with invalid multiple truncate alignments");
      aligns &= ~commonFlags;
      aligns |= XBFONT_TRUNCATED;
    }
  }
}

} /* namespace */

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
  }

  virtual ~CFreeTypeLibrary()
  {
    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const std::string& filename,
                  float size,
                  float aspect)
  {
    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::Log(LOGERROR, "Unable to initialize freetype library");
      return NULL;
    }

    FT_Face face;

    // ok, now load the font face
    CURL realFile(CSpecialProtocol::TranslatePath(filename));
    if (realFile.GetFileName().empty())
      return NULL;

    if (FT_New_Face(m_library, realFile.GetFileName().c_str(), 0, &face))
      return NULL;

    unsigned int ydpi = 72; // 72 points to the inch is the freetype default
    unsigned int xdpi =
        static_cast<unsigned int>(MathUtils::round_int(static_cast<double>(ydpi * aspect)));

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size(face, 0, static_cast<int>(size * 64 + 0.5f), xdpi, ydpi))
    {
      FT_Done_Face(face);
      return NULL;
    }

    return face;
  };

  FT_Stroker GetStroker()
  {
    if (!m_library)
      return NULL;

    FT_Stroker stroker;
    if (FT_Stroker_New(m_library, &stroker))
      return NULL;

    return stroker;
  };

  static void ReleaseFont(FT_Face face)
  {
    assert(face);
    FT_Done_Face(face);
  };

  static void ReleaseStroker(FT_Stroker stroker)
  {
    assert(stroker);
    FT_Stroker_Done(stroker);
  }

private:
  FT_Library m_library;
};

XBMC_GLOBAL_REF(CFreeTypeLibrary, g_freeTypeLibrary); // our freetype library
#define g_freeTypeLibrary XBMC_GLOBAL_USE(CFreeTypeLibrary)

CGUIFontTTF::CGUIFontTTF(const std::string& fontIdent)
  : m_fontIdent(fontIdent),
    m_height(0.0f),
    m_textureWidth(0),
    m_textureHeight(0),
    m_posX(0),
    m_posY(0),
    m_ellipseCached(false),
    m_ellipsesWidth(0.0f),
    m_cellBaseLine(0),
    m_cellHeight(0),
    m_maxFontHeight(0),
    m_nestedBeginCount(0),
    m_originX(0.0f),
    m_originY(0.0f),
    m_textureScaleX(0.0f),
    m_textureScaleY(0.0f),
    m_numCharactersRendered(0),
    m_referenceCount(0)
{
  memset(m_charquick, 0, sizeof(m_charquick));
  m_texture = NULL;
  m_face = NULL;
  m_stroker = NULL;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTF::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}


void CGUIFontTTF::ClearCharacterCache()
{
  SAFE_RELEASE(m_texture);
  m_char.clear();
  m_char.reserve(CHAR_CHUNK);
  memset(m_charquick, 0, sizeof(m_charquick));
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());
  m_textureHeight = 0;
}

void CGUIFontTTF::Clear()
{
  SAFE_RELEASE(m_texture);
  memset(m_charquick, 0, sizeof(m_charquick));
  m_posX = 0;
  m_posY = 0;
  m_nestedBeginCount = 0;

  if (m_face)
    g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = NULL;
  if (m_stroker)
    g_freeTypeLibrary.ReleaseStroker(m_stroker);
  m_stroker = NULL;
}

bool CGUIFontTTF::Load(
    const std::string& strFilename, float height, float aspect, float lineSpacing, bool border)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts
  m_face = g_freeTypeLibrary.GetFont(strFilename, height, aspect);
  if (!m_face)
    return false;

  /*
   the values used are described below

      XBMC coords                                     Freetype coords

                0  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  bbox.yMax, ascender
                        A                 \
                       A A                |
                      A   A               |
                      AAAAA  pppp   cellAscender
                      A   A  p   p        |
                      A   A  p   p        |
   m_cellBaseLine  _ _A_ _A_ pppp_ _ _ _ _/_ _ _ _ _  0, base line.
                             p            \
                             p      cellDescender
     m_cellHeight  _ _ _ _ _ p _ _ _ _ _ _/_ _ _ _ _  bbox.yMin, descender

   */
  int cellDescender = std::min<int>(m_face->bbox.yMin, m_face->descender);
  int cellAscender = std::max<int>(m_face->bbox.yMax, m_face->ascender);

  if (border)
  {
    /*
     add on the strength of any border - the non-bordered font needs
     aligning with the bordered font by utilising GetTextBaseLine()
     */
    FT_Pos strength = FT_MulFix(m_face->units_per_EM, m_face->size->metrics.y_scale) / 12;
    if (strength < 128)
      strength = 128;

    cellDescender -= strength;
    cellAscender += strength;

    m_stroker = g_freeTypeLibrary.GetStroker();
    if (m_stroker)
      FT_Stroker_Set(m_stroker, strength, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
  }

  // scale to pixel sizing, rounding so that maximal extent is obtained
  float scaler = height / m_face->units_per_EM;
  cellDescender =
      MathUtils::round_int(cellDescender * static_cast<double>(scaler) - 0.5); // round down
  cellAscender = MathUtils::round_int(cellAscender * static_cast<double>(scaler) + 0.5); // round up

  m_cellBaseLine = cellAscender;
  m_cellHeight = cellAscender - cellDescender;

  m_height = height;

  SAFE_RELEASE(m_texture);

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  if (m_textureWidth > CServiceBroker::GetWinSystem()->GetGfxContext().GetMaxTextureSize())
    m_textureWidth = CServiceBroker::GetWinSystem()->GetGfxContext().GetMaxTextureSize();
  m_textureScaleX = 1.0f / m_textureWidth;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());

  return true;
}

void CGUIFontTTF::Begin()
{
  if (m_nestedBeginCount == 0 && m_texture && FirstBegin())
  {
  }
  // Keep track of the nested begin/end calls.
  m_nestedBeginCount++;
}

void CGUIFontTTF::End()
{
  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  LastEnd();
}

void CGUIFontTTF::DrawTextInternal(CGraphicContext& context,
                                   float x,
                                   float y,
                                   const std::vector<UTILS::COLOR::Color>& colors,
                                   const vecText& text,
                                   uint32_t alignment,
                                   float maxPixelWidth,
                                   bool scrolling)
{
  if (text.empty())
  {
    return;
  }

  Begin();

  {
    // Try to validate any conflicting alignments
    //! @todo: This validate is the last resort and can result in a bad rendered text
    //! because the alignment it is used also by caller components for other operations
    //! this inform the problem on the log, potentially can be improved
    //! by add validating alignments from each parent caller component
    ValidateAlignments(alignment);

    // save the origin, which is scaled separately
    m_originX = x;
    m_originY = y;

    // cache the ellipses width
    if (!m_ellipseCached)
    {
      m_ellipseCached = true;
      Character* ellipse = GetCharacter(L'.', 0);
      if (ellipse)
        m_ellipsesWidth = ellipse->m_advance;
    }

    // Define the width of ellipses of three chars "..."
    const float ellipsesWidth = 3 * m_ellipsesWidth;

    // Check if we will really need to truncate or justify the text
    if (alignment & XBFONT_TRUNCATED)
    {
      if (maxPixelWidth <= 0.0f || GetTextWidthInternal(text) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED;
    }
    else if (alignment & XBFONT_TRUNCATED_LEFT)
    {
      if (maxPixelWidth <= 0.0f || GetTextWidthInternal(text) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED_LEFT;
    }
    else if (alignment & XBFONT_JUSTIFIED)
    {
      if (maxPixelWidth <= 0.0f)
        alignment &= ~XBFONT_JUSTIFIED;
    }

    // calculate sizing information
    float startX = 0;
    float startY = (alignment & XBFONT_CENTER_Y) ? -0.5f * m_cellHeight : 0; // vertical centering

    size_t startPosGlyph = 0; // Defines the index position where start rendering glyphs
    float textWidth = 0; // The text width, by taking in account truncate (and ellipses)

    if (alignment & XBFONT_TRUNCATED_LEFT)
    {
      // To truncate to the left, we skip all characters that exceed the maximum width,
      // so the rendering starts from the first character that falls within the maximum width,
      // taking into account also the ellipses
      textWidth = ellipsesWidth;

      // We need to iterate from the end to the beginning
      for (vecText::const_reverse_iterator itRGlyph = text.rbegin(); itRGlyph != text.rend(); ++itRGlyph)
      {
        const character_t ch = *itRGlyph;
        Character* c = GetCharacter(ch, 0);
        if (!c)
          continue;

        float nextWidth = textWidth;
        if ((ch & 0xffff) == static_cast<character_t>('\t'))
          nextWidth += GetTabSpaceLength();
        else
          nextWidth += c->m_advance;

        if (maxPixelWidth > 0 && nextWidth > maxPixelWidth)
        {
          // Start rendering from the glyph that does not exceed the maximum width
          startPosGlyph = std::distance(itRGlyph, text.rend());
          break;
        }
        textWidth = nextWidth;
      }
    }
    else
    {
      // Calculates the text width based on the characters that can be contained within the maximum width
      if (alignment & XBFONT_TRUNCATED)
        textWidth = ellipsesWidth;

      for (vecText::const_iterator glyph = text.begin(); glyph != text.end(); ++glyph)
      {
        const character_t ch = *glyph;
        Character* c = GetCharacter(ch, 0);
        if (!c)
          continue;

        float nextWidth = textWidth;
        if ((ch & 0xffff) == static_cast<character_t>('\t'))
          nextWidth += GetTabSpaceLength();
        else
          nextWidth += c->m_advance;

        if (maxPixelWidth > 0 && nextWidth > maxPixelWidth)
          break;

        textWidth = nextWidth;
      }
    }

    if (alignment & XBFONT_RIGHT)
    {
      // Moves the x pos with the purpose of having the text effect aligned to the right
      startX += maxPixelWidth - textWidth;
    }
    else if (alignment & XBFONT_CENTER_X)
    {
      textWidth *= 0.5f;
      startX -= textWidth;
    }

    float spacePerSpaceCharacter = 0; // for justification effects
    if (alignment & XBFONT_JUSTIFIED)
    {
      // first compute the size of the text to render in both characters and pixels
      unsigned int numSpaces = 0;
      float linePixels = 0;
      for (vecText::const_iterator glyph = text.begin(); glyph != text.end(); ++glyph)
      {
        Character* ch = GetCharacter(*glyph, 0);
        if (ch)
        {
          if ((*glyph & 0xffff) == L' ')
            numSpaces += 1;
          linePixels += ch->m_advance;
        }
      }
      if (numSpaces > 0)
        spacePerSpaceCharacter = (maxPixelWidth - linePixels) / numSpaces;
    }

    float cursorX = 0; // current position along the line

    // Collect all the Character info in a first pass, in case any of them
    // are not currently cached and cause the texture to be enlarged, which
    // would invalidate the texture coordinates.
    std::queue<Character> characters;

    if (alignment & XBFONT_TRUNCATED_LEFT)
      cursorX += ellipsesWidth;

    vecText::const_iterator glyphBegin = text.begin() + startPosGlyph;

    for (vecText::const_iterator itGlyph = glyphBegin; itGlyph != text.end(); ++itGlyph)
    {
      Character* ch =
          GetCharacter(*itGlyph, 0);
      if (!ch)
      {
        Character null = {};
        characters.push(null);
        continue;
      }
      characters.push(*ch);

      if (maxPixelWidth > 0)
      {
        float nextCursorX = cursorX;

        if (alignment & XBFONT_TRUNCATED)
          nextCursorX += ch->m_advance + ellipsesWidth;

        if (nextCursorX > maxPixelWidth)
          break;
      }

      cursorX += ch->m_advance;
    }

    cursorX = 0;

    for (vecText::const_iterator itGlyph = glyphBegin; itGlyph != text.end(); ++itGlyph)
    {
      // If starting text on a new line, determine justification effects
      // Get the current letter in the CStdString
      UTILS::COLOR::Color color = (*itGlyph & 0xff0000) >> 16;
      if (color >= colors.size())
        color = 0;
      color = colors[color];

      // grab the next character
      Character* ch = &characters.front();

      if ((*itGlyph & 0xffff) == static_cast<character_t>('\t'))
      {
        const float tabwidth = GetTabSpaceLength();
        const float a = cursorX / tabwidth;
        cursorX += tabwidth - ((a - floorf(a)) * tabwidth);
        characters.pop();
        continue;
      }

      if (alignment & XBFONT_TRUNCATED)
      {
        // Check if we will be exceeded the max allowed width
        if (cursorX + ch->m_advance + ellipsesWidth > maxPixelWidth)
        {
          // Yup. Let's draw the ellipses, then bail
          // Perhaps we should really bail to the next line in this case??
          Character* period = GetCharacter(L'.', 0);
          if (!period)
            break;

          for (int i = 0; i < 3; i++)
          {
            RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling);
            cursorX += period->m_advance;
          }
          break;
        }
      }
      else if (alignment & XBFONT_TRUNCATED_LEFT && itGlyph == glyphBegin)
      {
        // Add ellipsis only at the beginning of the text
        Character* period = GetCharacter(L'.', 0);
        if (!period)
          break;

        for (int i = 0; i < 3; i++)
        {
          RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling);
          cursorX += period->m_advance;
        }
      }
      else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
        break; // exceeded max allowed width - stop rendering

      RenderCharacter(context, startX + cursorX, startY, ch, color, !scrolling);
      if (alignment & XBFONT_JUSTIFIED)
      {
        if ((*itGlyph & 0xffff) == L' ')
          cursorX += ch->m_advance + spacePerSpaceCharacter;
        else
          cursorX += ch->m_advance;
      }
      else
        cursorX += ch->m_advance;
      characters.pop();
    }
  }

  End();
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTF::GetTextWidthInternal(const vecText& text)
{
  float width = 0;
  for (vecText::const_iterator it = text.begin(); it != text.end(); it++)
  {
    const character_t ch = *it;
    Character* c = GetCharacter(ch, 0);
    if (c)
    {
      // If last character in line, we want to add render width
      // and not advance distance - this makes sure that italic text isn't
      // choped on the end (as render width is larger than advance then).
      vecText::const_iterator next = it;
      if (++next == text.end())
        width += std::max(c->m_right - c->m_left + c->m_offsetX, c->m_advance);
      else if ((ch & 0xffff) == static_cast<character_t>('\t'))
        width += GetTabSpaceLength();
      else
        width += c->m_advance;
    }
  }

  return width;
}

float CGUIFontTTF::GetCharWidthInternal(character_t ch)
{
  Character* c = GetCharacter(ch, 0);
  if (c)
  {
    if ((ch & 0xffff) == static_cast<character_t>('\t'))
      return GetTabSpaceLength();
    else
      return c->m_advance;
  }

  return 0;
}

float CGUIFontTTF::GetTextHeight(float lineSpacing, int numLines) const
{
  return static_cast<float>(numLines - 1) * GetLineHeight(lineSpacing) + m_cellHeight;
}

float CGUIFontTTF::GetLineHeight(float lineSpacing) const
{
  if (!m_face)
    return 0.0f;

  return lineSpacing * m_face->size->metrics.height / 64.0f;
}

unsigned int CGUIFontTTF::GetTextureLineHeight() const
{
  return m_cellHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

unsigned int CGUIFontTTF::GetMaxFontHeight() const
{
  return m_maxFontHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

CGUIFontTTF::Character* CGUIFontTTF::GetCharacter(character_t chr, FT_UInt glyphIndex)
{
  const wchar_t letter = static_cast<wchar_t>(chr & 0xffff);

  // ignore linebreaks
  if (letter == L'\r')
    return NULL;

  const character_t style = (chr & 0x7000000) >> 24; // style = 0 - 6

  if (!glyphIndex)
    glyphIndex = FT_Get_Char_Index(m_face, letter);

  // quick access to the most frequently used glyphs
  if (glyphIndex < MAX_GLYPH_IDX)
  {
    character_t ch = (style << 12) | glyphIndex; // 2^12 = 4096

    if (ch < LOOKUPTABLE_SIZE && m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and glyph
  character_t ch = (style << 16) | glyphIndex;

  // perform binary search on sorted array by m_glyphAndStyle and
  // if not found obtains position to insert the new m_char to keep sorted
  int low = 0;
  int high = m_char.size() - 1;
  while (low <= high)
  {
    int mid = (low + high) >> 1;
    if (ch > m_char[mid].m_glyphAndStyle)
      low = mid + 1;
    else if (ch < m_char[mid].m_glyphAndStyle)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  int startIndex = low;

  // increase the size of the buffer if we need it
  if (m_char.size() == m_char.capacity())
  {
    m_char.reserve(m_char.capacity() + CHAR_CHUNK);
    startIndex = 0;
  }

  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  unsigned int nestedBeginCount = m_nestedBeginCount;
  m_nestedBeginCount = 1;
  if (nestedBeginCount)
    End();

  m_char.insert(m_char.begin() + low, Character());
  if (!CacheCharacter(glyphIndex, style, &m_char[0] + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::Log(LOGDEBUG, "Unable to cache character. Clearing character cache of %i characters",
               m_char.size());
    ClearCharacterCache();
    low = 0;
    startIndex = 0;
    m_char.insert(m_char.begin(), Character());
    if (!CacheCharacter(glyphIndex, style, &m_char[0]))
    {
      CLog::Log(LOGERROR, "Unable to cache character (out of memory?)");
      if (nestedBeginCount)
        Begin();
      m_nestedBeginCount = nestedBeginCount;
      return NULL;
    }
  }

  if (nestedBeginCount)
    Begin();
  m_nestedBeginCount = nestedBeginCount;

  // update the lookup table with only the m_char addresses that have changed
  for (size_t i = startIndex; i < m_char.size(); ++i)
  {
    if (m_char[i].m_glyphIndex < MAX_GLYPH_IDX)
    {
      // >> 16 is style (0-6), then 16 - 12 (>> 4) is equivalent to style * 4096
      character_t ch = ((m_char[i].m_glyphAndStyle & 0xffff0000) >> 4) | m_char[i].m_glyphIndex;

      if (ch < LOOKUPTABLE_SIZE)
        m_charquick[ch] = &m_char[0] + i;
    }
  }

  return &m_char[0] + low;
}

bool CGUIFontTTF::CacheCharacter(FT_UInt glyphIndex, uint32_t style, Character* ch)
{
  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_TARGET_LIGHT))
  {
    CLog::Log(LOGDEBUG, "Failed to load glyph %x", glyphIndex);
    return false;
  }

  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    SetGlyphStrength(m_face->glyph, GLYPH_STRENGTH_BOLD);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(m_face->glyph);
  // and light if applicable
  if (style & FONT_STYLE_LIGHT)
    SetGlyphStrength(m_face->glyph, GLYPH_STRENGTH_LIGHT);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::Log(LOGDEBUG, "Failed to get glyph %x", glyphIndex);
    return false;
  }
  if (m_stroker)
    FT_Glyph_StrokeBorder(&glyph, m_stroker, 0, 1);
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
  {
    CLog::Log(LOGDEBUG, "Failed to render glyph %x to a bitmap", glyphIndex);
    return false;
  }

  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  bool isEmptyGlyph = (bitmap.width == 0 || bitmap.rows == 0);

  if (!isEmptyGlyph)
  {
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    // check we have enough room for the character.
    // cast-fest is here to avoid warnings due to freeetype version differences (signedness of width).
    if (static_cast<int>(m_posX + bitGlyph->left + bitmap.width +
                         SPACING_BETWEEN_CHARACTERS_IN_TEXTURE) > static_cast<int>(m_textureWidth))
    { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
      m_posX = 1;
      m_posY += GetTextureLineHeight();
      if (bitGlyph->left < 0)
        m_posX += -bitGlyph->left;

      if (m_posY + GetTextureLineHeight() >= m_textureHeight)
      {
        // create the new larger texture
        unsigned int newHeight = m_posY + GetTextureLineHeight();
        // check for max height
        if (newHeight > CServiceBroker::GetWinSystem()->GetGfxContext().GetMaxTextureSize())
        {
          CLog::Log(LOGDEBUG, "New cache texture is too large (%u > %u pixels long)", newHeight,
                     CServiceBroker::GetWinSystem()->GetGfxContext().GetMaxTextureSize());
          FT_Done_Glyph(glyph);
          return false;
        }

        LPDIRECT3DDEVICE8 m_pD3DDevice = CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice();
        LPDIRECT3DTEXTURE8 newTexture;
        if (D3D_OK != D3DXCreateTexture(m_pD3DDevice, m_textureWidth, newHeight, 1, 0, D3DFMT_LIN_A8, D3DPOOL_MANAGED, &newTexture))
        {
          FT_Done_Glyph(glyph);
          CLog::Log(LOGDEBUG, "Failed to allocate new texture of height %u", newHeight);
          return false;
        }
        // correct texture sizes
        D3DSURFACE_DESC desc;
        newTexture->GetLevelDesc(0, &desc);
        m_textureHeight = desc.Height;
        m_textureWidth = desc.Width;

        // clear texture, doesn't cost much
        D3DLOCKED_RECT rect;
        newTexture->LockRect(0, &rect, NULL, 0);
        memset(rect.pBits, 0, rect.Pitch * m_textureHeight);
        newTexture->UnlockRect(0);

        if (m_texture)
        { // copy across from our current one using gpu
          LPDIRECT3DSURFACE8 pTarget, pSource;
          newTexture->GetSurfaceLevel(0, &pTarget);
          m_texture->GetSurfaceLevel(0, &pSource);

          m_pD3DDevice->CopyRects(pSource, NULL, 0, pTarget, NULL);

          SAFE_RELEASE(pTarget);
          SAFE_RELEASE(pSource);
          SAFE_RELEASE(m_texture);
        }
        m_texture = newTexture;
      }
      m_posY = GetMaxFontHeight();
    }

    if (!m_texture)
    {
      FT_Done_Glyph(glyph);
      CLog::Log(LOGDEBUG, "no texture to cache character to");
      return false;
    }
  }

  // set the character in our table
  ch->m_glyphAndStyle = (style << 16) | glyphIndex;
  ch->m_glyphIndex = glyphIndex;
  ch->m_offsetX = static_cast<short>(bitGlyph->left);
  ch->m_offsetY = static_cast<short>(m_cellBaseLine - bitGlyph->top);
  ch->m_left = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posX));
  ch->m_top = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posY));
  ch->m_right = ch->m_left + bitmap.width;
  ch->m_bottom = ch->m_top + bitmap.rows;
  ch->m_advance =
      static_cast<float>(MathUtils::round_int(static_cast<double>(m_face->glyph->advance.x) / 64));

  // we need only render if we actually have some pixels
  if (!isEmptyGlyph)
  {
    // ensure our rect will stay inside the texture (it *should* but we need to be certain)
    unsigned int x1 = std::max(m_posX, 0);
    unsigned int y1 = std::max(m_posY, 0);
    unsigned int x2 = std::min(x1 + bitmap.width, m_textureWidth);
    unsigned int y2 = std::min(y1 + bitmap.rows, m_textureHeight);
    m_maxFontHeight = std::max(m_maxFontHeight, y2);
    CopyCharToTexture(bitGlyph, x1, y1, x2, y2);

    m_posX += SPACING_BETWEEN_CHARACTERS_IN_TEXTURE +
              static_cast<unsigned short>(ch->m_right - ch->m_left);
  }

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTF::RenderCharacter(CGraphicContext& context,
                                  float posX,
                                  float posY,
                                  const Character* ch,
                                  UTILS::COLOR::Color color,
                                  bool roundX)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->m_right - ch->m_left;
  const float height = ch->m_bottom - ch->m_top;

  // return early if nothing to render
  if (width == 0 || height == 0)
    return;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->m_offsetX) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY) * context.GetGUIScaleY(),
               (posX + ch->m_offsetX + width) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY + height) * context.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
  CRect texture(ch->m_left, ch->m_top, ch->m_right, ch->m_bottom);
  context.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[VERTEX_PER_GLYPH] = {context.ScaleFinalXCoord(vertex.x1, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y2),
                               context.ScaleFinalXCoord(vertex.x1, vertex.y2)};

  if (roundX)
  {
    // We only round the "left" side of the character, and then use the direction of rounding to
    // move the "right" side of the character.  This ensures that a constant width is kept when rendering
    // the same letter at the same size at different places of the screen, avoiding the problem
    // of the "left" side rounding one way while the "right" side rounds the other way, thus getting
    // altering the width of thin characters substantially.  This only really works for positive
    // coordinates (due to the direction of truncation for negatives) but this is the only case that
    // really interests us anyway.
    float rx0 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[0])));
    float rx3 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[3])));
    x[1] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[1])));
    x[2] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[2])));
    if (x[0] > 0.0f && rx0 > x[0])
      x[1] += 1;
    else if (x[0] < 0.0f && rx0 < x[0])
      x[1] -= 1;
    if (x[3] > 0.0f && rx3 > x[3])
      x[2] += 1;
    else if (x[3] < 0.0f && rx3 < x[3])
      x[2] -= 1;
    x[0] = rx0;
    x[3] = rx3;
  }

  const float y[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y2))))};

  const float z[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y2))))};

  m_numCharactersRendered++;

  LPDIRECT3DDEVICE8 m_pD3DDevice = CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice();
  if (m_numCharactersRendered >= MAX_GLYPHS_PER_TEXT_LINE)
  { // we're pushing the (undocumented) limits of xbox here
    m_pD3DDevice->End();
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
    m_numCharactersRendered = 1;
  }
  m_pD3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, color);

  m_pD3DDevice->SetVertexData2f(D3DVSDE_TEXCOORD0, texture.x1, texture.y1);
  m_pD3DDevice->SetVertexData4f(D3DVSDE_VERTEX, x[0], y[0], z[0], 1);
  m_pD3DDevice->SetVertexData2f(D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  m_pD3DDevice->SetVertexData4f(D3DVSDE_VERTEX, x[1], y[1], z[1], 1);
  m_pD3DDevice->SetVertexData2f(D3DVSDE_TEXCOORD0, texture.x2, texture.y2);
  m_pD3DDevice->SetVertexData4f(D3DVSDE_VERTEX, x[2], y[2], z[2], 1);
  m_pD3DDevice->SetVertexData2f(D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  m_pD3DDevice->SetVertexData4f(D3DVSDE_VERTEX, x[3], y[3], z[3], 1);
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform(&slot->outline, &transform);
}

// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::SetGlyphStrength(FT_GlyphSlot slot, int glyphStrength)
{
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix(m_face->units_per_EM, m_face->size->metrics.y_scale) / glyphStrength;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox(&slot->outline, &bbox_before);
  FT_Outline_Embolden(&slot->outline, strength); // ignore error
  FT_Outline_Get_CBox(&slot->outline, &bbox_after);

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if (slot->advance.x)
    slot->advance.x += dx;

  if (slot->advance.y)
    slot->advance.y += dy;

  slot->metrics.width += dx;
  slot->metrics.height += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance += dy;
}

float CGUIFontTTF::GetTabSpaceLength()
{
  const Character* c = GetCharacter(static_cast<character_t>('X'), 0);
  return c ? c->m_advance * TAB_SPACE_LENGTH : 28.0f * TAB_SPACE_LENGTH;
}

CGUIFontTTF* CGUIFontTTF::CreateGUIFontTTF(const std::string& fontIdent)
{
  return new CGUIFontTTF(fontIdent);
}

bool CGUIFontTTF::CopyCharToTexture(
    FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  // render this onto our normal texture using gpu
  LPDIRECT3DSURFACE8 target;
  bool result = m_texture->GetSurfaceLevel(0, &target) != D3D_OK;

  RECT sourcerect = {0, 0, bitmap.width, bitmap.rows};
  RECT targetrect = {x1, y1, x2, y2};

  result &= D3DXLoadSurfaceFromMemory(target, NULL, &targetrect,
                                      bitmap.buffer, D3DFMT_LIN_A8, bitmap.pitch, NULL, &sourcerect,
                                      D3DX_FILTER_NONE, 0x00000000) != D3D_OK;

  SAFE_RELEASE(target);
  return result;
}

bool CGUIFontTTF::FirstBegin()
{
  LPDIRECT3DDEVICE8 m_pD3DDevice = CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice();

  // just have to blit from our texture.
  m_pD3DDevice->SetTexture(0, m_texture);

  m_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1); // only use diffuse
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  // no other texture stages needed
  m_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

  m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
  m_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
  m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

  m_pD3DDevice->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

  // Render the image
  m_pD3DDevice->SetScreenSpaceOffset(-0.5f, -0.5f);
  m_pD3DDevice->Begin(D3DPT_QUADLIST);

  return true;
}

void CGUIFontTTF::LastEnd()
{
  LPDIRECT3DDEVICE8 m_pD3DDevice = CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice();

  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  m_numCharactersRendered = 0;
}
