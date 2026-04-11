/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFontManager.h
\brief
*/

#include "IMsgTargetCallback.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/ColorUtils.h"
#include "utils/GlobalsHandling.h"
#include "windowing/GraphicContext.h"

#include <set>
#include <utility>
#include <vector>

// Forward
class CGUIFont;
class CGUIFontTTF;
class CXBMCTinyXML;
class TiXmlNode;
class CSetting;
struct StringSettingOption;

struct OrigFontInfo
{
  int size;
  float aspect;
  std::string fontFilePath;
  std::string fileName;
  RESOLUTION_INFO sourceRes;
  bool preserveAspect;
  bool border;
};

struct FontMetadata
{
  FontMetadata(const std::string& filename, const std::set<std::string>& familyNames)
    : m_filename(filename), m_familyNames(familyNames)
  {
  }

  std::string m_filename;
  std::set<std::string> m_familyNames;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager : public IMsgTargetCallback
{
public:
  GUIFontManager();
  virtual ~GUIFontManager();

  /*!
   *  \brief Initialize the font manager.
   *  Checks that fonts cache are up to date, otherwise update it.
   */
  void Initialize();

  bool IsUpdating() const { return m_critSection.IsLocked(); }

  virtual bool OnMessage(CGUIMessage& message);

  void Unload(const std::string& strFontName);
  void LoadFonts(const std::string& fontSet);
  CGUIFont* LoadTTF(const std::string& strFontName,
                    const std::string& strFilename,
                    UTILS::COLOR::Color textColor,
                    UTILS::COLOR::Color shadowColor,
                    const int iSize,
                    const int iStyle,
                    bool border = false,
                    float lineSpacing = 1.0f,
                    float aspect = 1.0f,
                    const RESOLUTION_INFO* res = NULL,
                    bool preserveAspect = false);
  CGUIFont* GetFont(const std::string& strFontName, bool fallback = true);

  /*! \brief return a default font
   \param border whether the font should be a font with an outline
   \return the font. `NULL` if no default font can be found.
   */
  CGUIFont* GetDefaultFont(bool border = false);

  void Clear();
  void FreeFontFile(CGUIFontTTF* pFont);

  static void SettingOptionsFontsFiller(const CSetting *setting,
                                        std::vector< std::pair<std::string, std::string> >& list,
                                        std::string& current,
                                        void* data);
#ifdef _XBOX
  static void SettingOptionsSubtitleHeightsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
#endif

  /*!
   * \brief Get the list of user fonts as family names from cache
   * \return The list of available fonts family names
   */
  std::vector<std::string> GetUserFontsFamilyNames();

protected:
  void ReloadTTFFonts();
  static void RescaleFontSizeAndAspect(CGraphicContext& context,
                                       float* size,
                                       float* aspect,
                                       const RESOLUTION_INFO& sourceRes,
                                       bool preserveAspect);
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const std::string& fontIdent);
  static void GetStyle(const TiXmlNode* fontNode, int& iStyle);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTF*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  RESOLUTION_INFO m_skinResolution;
  bool m_canReload;

private:
  void LoadUserFonts();
  bool LoadFontsFromFile(const std::string& fontsetFilePath,
                         const std::string& fontSet,
                         std::string& firstFontset);

  mutable CCriticalSection m_critSection;
  std::vector<FontMetadata> m_userFontsCache;
};

/*!
 \ingroup textures
 \brief
 */
XBMC_GLOBAL_REF(GUIFontManager, g_fontManager);
#define g_fontManager XBMC_GLOBAL_USE(GUIFontManager)
