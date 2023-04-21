#pragma once

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

#include "GraphicContext.h" // needed for the RESOLUTION members
#include "GUIIncludes.h"    // needed for the GUIInclude member

#define CREDIT_LINE_LENGTH 50

class TiXmlNode;

class CSkinInfo
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const CStdString &name)
    {
      m_id = id; m_name = name;
    };
    int m_id;
    CStdString m_name;
  };

  CSkinInfo();
  ~CSkinInfo();

  /*! \brief Load information regarding the skin from the given skin directory
   \param skinDir folder of the skin to load
   \param loadIncludes whether the includes from the skin should also be loaded (defaults to true)
   */
  void Load(const CStdString& skinDir, bool loadIncludes = true);

  bool HasSkinFile(const CStdString &strFile) const;

  /*! \brief Get the full path to the specified file in the skin
   We search for XML files in the skin folder that best matches the current resolution.
   \param file XML file to look for
   \param res [out] If non-NULL, the resolution that the returned XML file is in is returned.  Defaults to NULL.
   \param baseDir [in] If non-empty, the given directory is searched instead of the skin's directory.  Defaults to empty.
   \return path to the XML file
   */
  CStdString GetSkinPath(const CStdString& file, RESOLUTION *res = NULL, const CStdString& baseDir = "") const;

  wchar_t* GetCreditsLine(int i);
  CStdString GetBaseDir() const;
  double GetVersion() const { return m_Version; };

  /*! \brief Return whether skin debugging is enabled
   \return true if skin debugging (set via <debugging>true</debugging> in skin.xml) is enabled.
   */
  bool IsDebugging() const { return m_debugging; };

  /*! \brief Get the id of the first window to load
   The first window is generally Startup.xml unless it doesn't exist or if the skinner
   has specified which start windows they support and the user is going to somewhere other
   than the home screen.
   \return id of the first window to load
   */
  int GetFirstWindow() const;

  /*! \brief Get the id of the window the user wants to start in after any skin animation
   \return id of the start window
   */
  int GetStartWindow() const;

  /*! \brief Translate a resolution string
   \param res the string to translate
   \param def the default to use if res is invalid
   \return the translated resolution
   */
  static RESOLUTION TranslateResolution(const CStdString &res, RESOLUTION def);

  void ResolveIncludes(TiXmlElement *node, std::map<int, bool>* xmlIncludeConditions = NULL);

  float GetEffectsSlowdown() const { return m_effectsSlowDown; };

  const std::vector<CStartupWindow> &GetStartupWindows() const { return m_startupWindows; };

  inline float GetSkinZoom() const { return m_skinzoom; };

  inline float GetLegacy() { return m_bLegacy; };

  /*! \brief Retrieve the skin paths to search for skin XML files
   \param paths [out] vector of paths to search, in order.
   */
  void GetSkinPaths(std::vector<CStdString> &paths) const;;

  static bool Check(const CStdString& strSkinDir); // checks if everything is present and accounted for without loading the skin
  static double GetMinVersion();
  const INFO::CSkinVariableString* CreateSkinVariable(const CStdString& name, int context);
protected:
  /*! \brief grab a resolution tag from an XML node
   \param node XML node to look for the given tag
   \param tag name of the tag to look for
   \param res resolution to return
   \return true if we find a valid XML node containing a valid resolution, false otherwise
   */
  bool GetResolution(const TiXmlNode *node, const char *tag, RESOLUTION &res) const;

  /*! \brief Given a resolution, retrieve the corresponding directory name
   \param res RESOLUTION to translate
   \return directory name for res
   */
  CStdString GetDirFromRes(RESOLUTION res) const;

  void SetDefaults();
  void LoadIncludes();
  bool LoadStartupWindows(const TiXmlElement *startup);
  bool IsWide(RESOLUTION res) const;

  wchar_t credits[6][CREDIT_LINE_LENGTH];  // credits info
  int m_iNumCreditLines;  // number of credit lines
  RESOLUTION m_DefaultResolution; // default resolution for the skin in 4:3 modes
  RESOLUTION m_DefaultResolutionWide; // default resolution for the skin in 16:9 modes
  CStdString m_strBaseDir;
  double m_Version;

  float m_effectsSlowDown;
  CGUIIncludes m_includes;

  std::vector<CStartupWindow> m_startupWindows;
  bool m_onlyAnimateToHome;
  bool m_debugging;

  float m_skinzoom;

  bool m_bLegacy;
};

extern CSkinInfo g_SkinInfo;
