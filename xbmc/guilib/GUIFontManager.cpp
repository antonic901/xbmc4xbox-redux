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

#include "include.h"
#include "GUIFontManager.h"
#include "GraphicContext.h"
#include "GUIWindowManager.h"
#include "addons/Skin.h"
#include "GUIFontTTF.h"
#include "GUIFont.h"
#include "utils/XMLUtils.h"
#include "GUIControlFactory.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "URL.h"
#include "Util.h"

using namespace std;

GUIFontManager g_fontManager;

GUIFontManager::GUIFontManager(void)
{
}

GUIFontManager::~GUIFontManager(void)
{
  Clear();
}

void GUIFontManager::RescaleFontSizeAndAspect(float *size, float *aspect, const RESOLUTION_INFO &sourceRes, bool preserveAspect) const
{
  // set scaling resolution so that we can scale our font sizes correctly
  // as fonts aren't scaled at render time (due to aliasing) we must scale
  // the size of the fonts before they are drawn to bitmaps
  g_graphicsContext.SetScalingResolution(sourceRes, true);

  if (preserveAspect)
  {
    // font always displayed in the aspect specified by the aspect parameter
    *aspect /= g_graphicsContext.GetPixelRatio(g_graphicsContext.GetVideoResolution());
  }
  else
  {
    // font streched like the rest of the UI, aspect parameter being the original aspect

    // adjust aspect ratio
    *aspect *= sourceRes.fPixelRatio;

    *aspect *= g_graphicsContext.GetGUIScaleY() / g_graphicsContext.GetGUIScaleX();
  }

  *size /= g_graphicsContext.GetGUIScaleY();
}

CGUIFont* GUIFontManager::LoadTTF(const CStdString& strFontName, const CStdString& strFilename, color_t textColor, color_t shadowColor, const int iSize, const int iStyle, bool border, float lineSpacing, float aspect, const RESOLUTION_INFO *sourceRes, bool preserveAspect)
{
  float originalAspect = aspect;

  //check if font already exists
  CGUIFont* pFont = GetFont(strFontName, false);
  if (pFont)
    return pFont;

  if (!sourceRes) // no source res specified, so assume the skin res
    sourceRes = &m_skinResolution;

  float newSize = (float)iSize;
  RescaleFontSizeAndAspect(&newSize, &aspect, *sourceRes, preserveAspect);

  CStdString strPath;
  if (!CURL::IsFullPath(strFilename))
  {
    strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "fonts");
    strPath = URIUtils::AddFileToFolder(strPath, strFilename);
  }
  else
    strPath = strFilename;

  // Check if the file exists, otherwise try loading it from the global media dir
  if (!XFILE::CFile::Exists(strPath))
  {
    strPath = URIUtils::AddFileToFolder("special://xbmc/media/Fonts", URIUtils::GetFileName(strFilename));
  }

  // check if we already have this font file loaded (font object could differ only by color or style)
  CStdString TTFfontName;
  TTFfontName.Format("%s_%f_%f%s", strFilename, newSize, aspect, border ? "_border" : "");
  CGUIFontTTF* pFontFile = GetFontFile(TTFfontName);
  if (!pFontFile)
  {
    pFontFile = new CGUIFontTTF(TTFfontName);
    bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect, 1.0f, border);
    if (!bFontLoaded)
    {
      // Now try to load it from media\fonts
      if (strFilename[1] != ':')
      {
        strPath = "Q:\\media\\Fonts\\";
        strPath += strFilename;
      }

      bFontLoaded = pFontFile->Load(strPath, newSize, aspect);
    }

    if (!bFontLoaded)
    {
      delete pFontFile;

      // font could not be loaded - try Arial.ttf, which we distribute
      if (strFilename != "arial.ttf")
      {
        CLog::Log(LOGERROR, "Couldn't load font name: %s(%s), trying to substitute arial.ttf", strFontName.c_str(), strFilename.c_str());
        return LoadTTF(strFontName, "arial.ttf", textColor, shadowColor, iSize, iStyle, border, lineSpacing, originalAspect);
      }
      CLog::Log(LOGERROR, "Couldn't load font name:%s file:%s", strFontName.c_str(), strPath.c_str());

      return NULL;
    }
    m_vecFontFiles.push_back(pFontFile);
  }

  // font file is loaded, create our CGUIFont
  CGUIFont *pNewFont = new CGUIFont(strFontName, iStyle, textColor, shadowColor, lineSpacing, (float)iSize, pFontFile);
  m_vecFonts.push_back(pNewFont);

  // Store the original TTF font info in case we need to reload it in a different resolution
  OrigFontInfo fontInfo;
  fontInfo.size = iSize;
  fontInfo.aspect = originalAspect;
  fontInfo.fontFilePath = strPath;
  fontInfo.fileName = strFilename;
  fontInfo.sourceRes = *sourceRes;
  fontInfo.preserveAspect = preserveAspect;
  fontInfo.border = border;
  m_vecFontInfo.push_back(fontInfo);

  return pNewFont;
}

void GUIFontManager::ReloadTTFFonts(void)
{
  if (!m_vecFonts.size())
    return;   // we haven't even loaded fonts in yet

  for (unsigned int i = 0; i < m_vecFonts.size(); i++)
  {
    CGUIFont* font = m_vecFonts[i];
    OrigFontInfo fontInfo = m_vecFontInfo[i];
    CGUIFontTTF* currentFontTTF = font->GetFont();

    float aspect = fontInfo.aspect;
    float newSize = (float)fontInfo.size;
    CStdString& strPath = fontInfo.fontFilePath;
    CStdString& strFilename = fontInfo.fileName;

    RescaleFontSizeAndAspect(&newSize, &aspect, fontInfo.sourceRes, fontInfo.preserveAspect);

    // check if we already have this font file loaded (font object could differ only by color or style)
    CStdString TTFfontName;
    TTFfontName.Format("%s_%f_%f%s", strFilename, newSize, aspect, fontInfo.border ? "_border" : "");

    CGUIFontTTF* pFontFile = GetFontFile(TTFfontName);
    if (!pFontFile)
    {
      pFontFile = new CGUIFontTTF(TTFfontName);
      bool bFontLoaded = pFontFile->Load(strPath, newSize, aspect);
      pFontFile->CopyReferenceCountFrom(*currentFontTTF);

      if (!bFontLoaded)
      {
        delete pFontFile;
        // font could not b loaded
        CLog::Log(LOGERROR, "Couldn't re-load font file:%s", strPath.c_str());
        return;
      }

      m_vecFontFiles.push_back(pFontFile);
    }

    font->SetFont(pFontFile);
  }
}

void GUIFontManager::Unload(const CStdString& strFontName)
{
  for (vector<CGUIFont*>::iterator iFont = m_vecFonts.begin(); iFont != m_vecFonts.end(); ++iFont)
  {
    if ((*iFont)->GetFontName() == strFontName)
    {
      delete (*iFont);
      m_vecFonts.erase(iFont);
      return;
    }
  }
}

void GUIFontManager::FreeFontFile(CGUIFontTTF *pFont)
{
  for (vector<CGUIFontTTF*>::iterator it = m_vecFontFiles.begin(); it != m_vecFontFiles.end(); ++it)
  {
    if (pFont == *it)
    {
      m_vecFontFiles.erase(it);
      delete pFont;
      return;
    }
  }
}

CGUIFontTTF* GUIFontManager::GetFontFile(const CStdString& strFileName)
{
  for (int i = 0; i < (int)m_vecFontFiles.size(); ++i)
  {
    CGUIFontTTF* pFont = m_vecFontFiles[i];
    if (pFont->GetFileName() == strFileName)
      return pFont;
  }
  return NULL;
}

CGUIFont* GUIFontManager::GetFont(const CStdString& strFontName, bool fallback /*= true*/)
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    if (pFont->GetFontName() == strFontName)
      return pFont;
  }
  // fall back to "font13" if we have none
  if (fallback && !strFontName.IsEmpty() && !strFontName.Equals("-") && !strFontName.Equals("font13"))
    return GetFont("font13");
  return NULL;
}

CGUIFont* GUIFontManager::GetDefaultFont(bool border)
{
  // first find "font13" or "__defaultborder__"
  unsigned int font13index = m_vecFonts.size();
  CGUIFont *font13border = NULL;
  for (unsigned int i = 0; i < m_vecFonts.size(); i++)
  {
    CGUIFont* font = m_vecFonts[i];
    if (font->GetFontName() == "font13")
      font13index = i;
    else if (font->GetFontName() == "__defaultborder__")
      font13border = font;
  }
  // no "font13" means no default font is found - use the first font found.
  if (font13index == m_vecFonts.size())
  {
    if (m_vecFonts.empty())
      return NULL;
    font13index = 0;
  }

  if (border)
  {
    if (!font13border)
    { // create it
      CGUIFont *font13 = m_vecFonts[font13index];
      OrigFontInfo fontInfo = m_vecFontInfo[font13index];
      font13border = LoadTTF("__defaultborder__", fontInfo.fileName, 0xFF000000, 0, fontInfo.size, font13->GetStyle(), true, 1.0f, fontInfo.aspect, &fontInfo.sourceRes, fontInfo.preserveAspect);
    }
    return font13border;
  }
  return m_vecFonts[font13index];
}

void GUIFontManager::Clear()
{
  for (int i = 0; i < (int)m_vecFonts.size(); ++i)
  {
    CGUIFont* pFont = m_vecFonts[i];
    delete pFont;
  }

  m_vecFonts.clear();
  m_vecFontFiles.clear();
  m_vecFontInfo.clear();
}

void GUIFontManager::LoadFonts(const std::string& fontSet)
{
  // Get the file to load fonts from:
  const std::string strPath = g_SkinInfo->GetSkinPath("Font.xml", &m_skinResolution);
  CLog::Log(LOGINFO, "Loading fonts from %s", strPath.c_str());

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->ValueStr() != "fonts")
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return;
  }

  // take note of the first font available in case we can't load the one specified
  std::string firstFont;

  const TiXmlElement *pChild = pRootElement->FirstChildElement("fontset");
  while (pChild)
  {
    const char* idAttr = pChild->Attribute("id");
    if (idAttr)
    {
      if (firstFont.empty())
        firstFont = idAttr;

      if (StringUtils::EqualsNoCase(fontSet, idAttr))
      {
        LoadFonts(pChild->FirstChild("font"));
        return;
      }
    }
    pChild = pChild->NextSiblingElement("fontset");
  }

  // no fontset was loaded, try the first
  if (!firstFont.empty())
  {
    CLog::Log(LOGWARNING, "file doesnt have <fontset> with name '%s', defaulting to first fontset", fontSet.c_str());
    LoadFonts(firstFont);
  }
  else
    CLog::Log(LOGERROR, "file '%s' doesnt have a valid <fontset>", strPath.c_str());
}

void GUIFontManager::LoadFonts(const TiXmlNode* fontNode)
{
  while (fontNode)
  {
    std::string fontName;
    std::string fileName;
    int iSize = 20;
    float aspect = 1.0f;
    float lineSpacing = 1.0f;
    color_t shadowColor = 0;
    color_t textColor = 0;
    int iStyle = FONT_STYLE_NORMAL;

    XMLUtils::GetString(fontNode, "name", fontName);
    XMLUtils::GetInt(fontNode, "size", iSize);
    XMLUtils::GetFloat(fontNode, "linespacing", lineSpacing);
    XMLUtils::GetFloat(fontNode, "aspect", aspect);
    CGUIControlFactory::GetColor(fontNode, "shadow", shadowColor);
    CGUIControlFactory::GetColor(fontNode, "color", textColor);
    XMLUtils::GetString(fontNode, "filename", fileName);
    GetStyle(fontNode, iStyle);

    if (!fontName.empty() && URIUtils::HasExtension(fileName, ".ttf"))
    {
      // TODO: Why do we tolower() this shit?
      CStdString strFontFileName = fileName;
      StringUtils::ToLower(strFontFileName);
      LoadTTF(fontName, strFontFileName, textColor, shadowColor, iSize, iStyle, false, lineSpacing, aspect);
    }
    fontNode = fontNode->NextSibling("font");
  }
}

void GUIFontManager::GetStyle(const TiXmlNode *fontNode, int &iStyle)
{
  std::string style;
  iStyle = FONT_STYLE_NORMAL;
  if (XMLUtils::GetString(fontNode, "style", style))
  {
    vector<string> styles = StringUtils::Tokenize(style, " ");
    for (vector<string>::const_iterator i = styles.begin(); i != styles.end(); ++i)
    {
      if (*i == "bold")
        iStyle |= FONT_STYLE_BOLD;
      else if (*i == "italics")
        iStyle |= FONT_STYLE_ITALICS;
      else if (*i == "bolditalics") // backward compatibility
        iStyle |= (FONT_STYLE_BOLD | FONT_STYLE_ITALICS);
      else if (*i == "uppercase")
        iStyle |= FONT_STYLE_UPPERCASE;
      else if (*i == "lowercase")
        iStyle |= FONT_STYLE_LOWERCASE;
      else if (*i == "capitalize")
        iStyle |= FONT_STYLE_CAPITALIZE;
      else if (*i == "lighten")
        iStyle |= FONT_STYLE_LIGHT;
    }
  }
}

void GUIFontManager::SettingOptionsFontsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current)
{
  CFileItemList items;
  CFileItemList items2;

  // find TTF fonts
  XFILE::CDirectory::GetDirectory("special://home/media/Fonts/", items2);

  if (XFILE::CDirectory::GetDirectory("special://xbmc/media/Fonts/", items))
  {
    items.Append(items2);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (!pItem->m_bIsFolder
          && URIUtils::HasExtension(pItem->GetLabel(), ".ttf"))
      {
        list.push_back(make_pair(pItem->GetLabel(), pItem->GetLabel()));
      }
    }
  }

#ifdef _XBOX
  // Find mplayer fonts...
  XFILE::CDirectory::GetDirectory("special://xbmc/system/players/mplayer/font/", items);
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      list.push_back(make_pair(pItem->GetLabel(), pItem->GetLabel()));
    }
  }
#endif
}

void GUIFontManager::SettingOptionsSubtitleHeightsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  if (CUtil::IsUsingTTFSubtitles())
  { // easy - just fill as per usual
    CSettingInt* pSettingInt = ((CSettingInt*)setting);
    for (int i = pSettingInt->GetMinimum(); i <= pSettingInt->GetMaximum(); i += pSettingInt->GetStep())
      list.push_back(std::make_pair(StringUtils::Format("%i", i), i));
  }
#ifdef _XBOX
  else
  {
    if (CSettings::Get().GetString("subtitles.font").size())
    {
      //find font sizes...
      CFileItemList items;
      CStdString strPath = "special://xbmc/system/players/mplayer/font/";
      strPath += CSettings::Get().GetString("subtitles.font");
      strPath += "/";
      XFILE::CDirectory::GetDirectory(strPath, items);
      int iCurrentSize = 0;
      
      bool found = false;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];
        if (pItem->m_bIsFolder)
        {
          if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
          iCurrentSize = atoi(pItem->GetLabel().c_str());
          if (iCurrentSize == current)
            found = true;
          list.push_back(std::make_pair(StringUtils::Format("%i", iCurrentSize), iCurrentSize));
        }
      }
      
      if (!found)
        current = iCurrentSize;
    }
  }
#endif
}
