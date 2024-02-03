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

#include "Settings.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/GUISettings.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#ifdef HAS_XBOX_HARDWARE
#include "settings/MediaSettings.h" // for AVPack methods
#include "filesystem/SpecialProtocol.h"
#include "utils/MemoryUnitManager.h"
#include "Util.h"
#include "xbox/IoSupport.h" // for CIoSupport
#include "XBVideoConfig.h" // for AVPack methods
#endif

using namespace std;
using namespace XFILE;

class CSettings g_settings;

extern CStdString g_LoadErrorStr;

CSettings::CSettings(void)
{
}

void CSettings::RegisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.insert(settingsHandler);
}

void CSettings::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.erase(settingsHandler);
}

void CSettings::RegisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.erase(subSettings);
}

CSettings::~CSettings(void)
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_subSettings.clear();

  Clear();
}

void CSettings::Save() const
{
  if (!SaveSettings(CProfilesManager::Get().GetSettingsFile()))
    CLog::Log(LOGERROR, "Unable to save settings to %s", CProfilesManager::Get().GetSettingsFile().c_str());
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(CProfilesManager::Get().GetSettingsFile());
  Save();
  return LoadSettings(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Load()
{
  if (!OnSettingsLoading())
    return false;

#ifdef _XBOX
  char szDevicePath[1024];
  CStdString strMnt = CSpecialProtocol::TranslatePath(CProfilesManager::Get().GetProfileUserDataFolder());
  if (strMnt.Left(2).Equals("Q:"))
  {
    CUtil::GetHomePath(strMnt);
    strMnt += CSpecialProtocol::TranslatePath(CProfilesManager::Get().GetProfileUserDataFolder()).substr(2);
  }
  CIoSupport::GetPartition(strMnt.c_str()[0], szDevicePath);
  strcat(szDevicePath,strMnt.c_str()+2);
  CIoSupport::RemapDriveLetter('P', szDevicePath);
#endif
  CLog::Log(LOGNOTICE, "loading %s", CProfilesManager::Get().GetSettingsFile().c_str());
  if (!LoadSettings(CProfilesManager::Get().GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", CProfilesManager::Get().GetSettingsFile().c_str(), CProfilesManager::Get().GetSettingsFile().c_str());
    if (!Reset())
      return false;
  }

  OnSettingsLoaded();

  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{
  // load the xml file
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    g_LoadErrorStr.Format("%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    g_LoadErrorStr.Format("%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }

  g_guiSettings.LoadXML(pRootElement);
  
  // Override settings with avpack settings
  if (CProfilesManager::Get().GetCurrentProfile().useAvpackSettings())
  {
    CLog::Log(LOGNOTICE, "Per AV pack settings are on");
    LoadAvpackXML();
  }
  else
    CLog::Log(LOGNOTICE, "Per AV pack settings are off");

  // load any ISubSettings implementations
  return Load(pRootElement);
}

bool CSettings::LoadAvpackXML()
{
  return false;
  // TODO: move this to separate setting class and load it at the end
  // CStdString avpackSettingsXML;
  // avpackSettingsXML  = GetAvpackSettingsFile();
  // CXBMCTinyXML avpackXML;
  // if (!CFile::Exists(avpackSettingsXML))
  // {
  //   CLog::Log(LOGERROR, "Error loading AV pack settings : %s not found !", avpackSettingsXML.c_str());
  //   return false;
  // }

  // CLog::Log(LOGNOTICE, "%s found : loading %s",
  //   g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  // if (!avpackXML.LoadFile(avpackSettingsXML.c_str()))
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s",
  //     avpackSettingsXML.c_str(), avpackXML.ErrorRow(), avpackXML.ErrorDesc());
  //   return false;
  // }

  // TiXmlElement *pMainElement = avpackXML.RootElement();
  // if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, no <settings> node", avpackSettingsXML.c_str());
  //   return false;
  // }

  // TiXmlElement *pRoot = pMainElement->FirstChildElement(g_videoConfig.GetAVPack());
  // if (!pRoot)
  // {
  //   CLog::Log(LOGERROR, "Error loading %s, no <%s> node",
  //     avpackSettingsXML.c_str(), g_videoConfig.GetAVPack().c_str());
  //   return false;
  // }

  // // Load guisettings
  // g_guiSettings.LoadXML(pRoot);

  // // Load calibration
  // return LoadCalibration(pRoot, avpackSettingsXML);
}

// Save the avpack settings in the current 'avpacksettings.xml' file
bool CSettings::SaveAvpackXML() const
{
  CStdString avpackSettingsXML;
  avpackSettingsXML  = GetAvpackSettingsFile();

  CLog::Log(LOGNOTICE, "Saving %s settings in %s",
    g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  // The file does not exist : Save defaults
  if (!CFile::Exists(avpackSettingsXML))
    return SaveNewAvpackXML();

  // The file already exists :
  // We need to preserve other avpack settings

  // First load the previous settings
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile(avpackSettingsXML))
  {
    CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, Line %d\n%s\nCreating new file.",
      avpackSettingsXML.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return SaveNewAvpackXML();
  }

  // Get the main element
  TiXmlElement *pMainElement = xmlDoc.RootElement();
  if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  {
    CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, no <settings> node.\nCreating new file.",
      avpackSettingsXML.c_str());
    return SaveNewAvpackXML();
  }

  // Delete the plugged avpack root if it exists, then recreate it
  // TODO : to support custom avpack settings, the two XMLs should
  // be synchronized, not just overwrite the old one
  TiXmlNode *pRoot = pMainElement->FirstChild(g_videoConfig.GetAVPack());
  if (pRoot)
    pMainElement->RemoveChild(pRoot);

  TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  pRoot = pMainElement->InsertEndChild(pluggedNode);
  if (!pRoot) return false;

  if (!SaveAvpackSettings(pRoot))
    return false;

  return xmlDoc.SaveFile(avpackSettingsXML);
}

// Create an 'avpacksettings.xml' file with in the current profile directory
bool CSettings::SaveNewAvpackXML() const
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlMainElement("settings");
  TiXmlNode *pMain = xmlDoc.InsertEndChild(xmlMainElement);
  if (!pMain) return false;

  TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  TiXmlNode *pRoot = pMain->InsertEndChild(pluggedNode);
  if (!pRoot) return false;

  if (!SaveAvpackSettings(pRoot))
    return false;

  return xmlDoc.SaveFile(GetAvpackSettingsFile());
}

// Save avpack settings in the provided xml node
bool CSettings::SaveAvpackSettings(TiXmlNode *io_pRoot) const
{
  return false;
  // TODO: move this to separate setting class and save it at the end
  // TiXmlElement programsNode("myprograms");
  // TiXmlNode *pNode = io_pRoot->InsertEndChild(programsNode);
  // if (!pNode) return false;
  // XMLUtils::SetBoolean(pNode, "gameautoregion", g_guiSettings.GetBool("myprograms.gameautoregion"));
  // XMLUtils::SetInt(pNode, "ntscmode", g_guiSettings.GetInt("myprograms.ntscmode"));

  // // default video settings
  // TiXmlElement videoSettingsNode("defaultvideosettings");
  // pNode = io_pRoot->InsertEndChild(videoSettingsNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "interlacemethod", CMediaSettings::Get().GetDefaultVideoSettings().m_InterlaceMethod);
  // XMLUtils::SetFloat(pNode, "filmgrain", CMediaSettings::Get().GetCurrentVideoSettings().m_FilmGrain);
  // XMLUtils::SetInt(pNode, "viewmode", CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  // XMLUtils::SetFloat(pNode, "zoomamount", CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount);
  // XMLUtils::SetFloat(pNode, "pixelratio", CMediaSettings::Get().GetCurrentVideoSettings().m_CustomPixelRatio);
  // XMLUtils::SetFloat(pNode, "volumeamplification", CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification);
  // XMLUtils::SetBoolean(pNode, "outputtoallspeakers", CMediaSettings::Get().GetCurrentVideoSettings().m_OutputToAllSpeakers);
  // XMLUtils::SetBoolean(pNode, "showsubtitles", CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn);
  // XMLUtils::SetFloat(pNode, "brightness", CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness);
  // XMLUtils::SetFloat(pNode, "contrast", CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast);
  // XMLUtils::SetFloat(pNode, "gamma", CMediaSettings::Get().GetCurrentVideoSettings().m_Gamma);

  // TiXmlElement audiooutputNode("audiooutput");
  // pNode = io_pRoot->InsertEndChild(audiooutputNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "mode", g_guiSettings.GetInt("audiooutput.mode"));
  // XMLUtils::SetBoolean(pNode, "ac3passthrough", g_guiSettings.GetBool("audiooutput.ac3passthrough"));
  // XMLUtils::SetBoolean(pNode, "dtspassthrough", g_guiSettings.GetBool("audiooutput.dtspassthrough"));

  // TiXmlElement videooutputNode("videooutput");
  // pNode = io_pRoot->InsertEndChild(videooutputNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "aspect", g_guiSettings.GetInt("videooutput.aspect"));
  // XMLUtils::SetBoolean(pNode, "hd480p", g_guiSettings.GetBool("videooutput.hd480p"));
  // XMLUtils::SetBoolean(pNode, "hd720p", g_guiSettings.GetBool("videooutput.hd720p"));
  // XMLUtils::SetBoolean(pNode, "hd1080i", g_guiSettings.GetBool("videooutput.hd1080i"));

  // TiXmlElement videoscreenNode("videoscreen");
  // pNode = io_pRoot->InsertEndChild(videoscreenNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "flickerfilter", g_guiSettings.GetInt("videoscreen.flickerfilter"));
  // XMLUtils::SetInt(pNode, "resolution", g_guiSettings.GetInt("videoscreen.resolution"));
  // XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoscreen.soften"));

  // TiXmlElement videoplayerNode("videoplayer");
  // pNode = io_pRoot->InsertEndChild(videoplayerNode);
  // if (!pNode) return false;
  // XMLUtils::SetInt(pNode, "displayresolution", g_guiSettings.GetInt("videoplayer.displayresolution"));
  // XMLUtils::SetInt(pNode, "flicker", g_guiSettings.GetInt("videoplayer.flicker"));
  // XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoplayer.soften"));

  // return SaveCalibration(io_pRoot);
}
bool CSettings::SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings /* = NULL */) const
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our tags one by one - just a big list for now (can be flashed up later)

  if (!OnSettingsSaving())
    return false;

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  if (CProfilesManager::Get().GetCurrentProfile().useAvpackSettings())
    SaveAvpackXML();

  OnSettingsSaved();

  if (!Save(pRoot))
    return false;

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

void CSettings::Clear()
{
  OnSettingsCleared();

  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->Clear();
}

CStdString CSettings::GetFFmpegDllFolder() const
{
  CStdString folder = "Q:\\system\\players\\dvdplayer\\";
  if (g_guiSettings.GetBool("videoplayer.allcodecs"))
    folder += "full\\";
  return folder;
}

CStdString CSettings::GetPlayerName(const int& player) const
{
  CStdString strPlayer;
  
  if (player == PLAYER_PAPLAYER)
    strPlayer = "paplayer";
  else
  if (player == PLAYER_MPLAYER)
    strPlayer = "mplayer";
  else
  if (player == PLAYER_DVDPLAYER)
    strPlayer = "dvdplayer";

  return strPlayer;
}

CStdString CSettings::GetDefaultVideoPlayerName() const
{
  return GetPlayerName(g_guiSettings.GetInt("videoplayer.defaultplayer"));
}

CStdString CSettings::GetDefaultAudioPlayerName() const
{
  return GetPlayerName(g_guiSettings.GetInt("musicplayer.defaultplayer"));
}

CStdString CSettings::GetAvpackSettingsFile() const
{
  CStdString  strAvpackSettingsFile;
  if (CProfilesManager::Get().GetCurrentProfileIndex() == 0)
    strAvpackSettingsFile = "T:\\avpacksettings.xml";
  else
    strAvpackSettingsFile = "P:\\avpacksettings.xml";
  return strAvpackSettingsFile;
}

bool CSettings::OnSettingsLoading()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettings::OnSettingsLoaded()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsLoaded();
}

bool CSettings::OnSettingsSaving() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettings::OnSettingsSaved() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsSaved();
}

void CSettings::OnSettingsCleared()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsCleared();
}

bool CSettings::Load(const TiXmlNode *settings)
{
  bool ok = true;
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    ok &= (*it)->Load(settings);

  return ok;
}

bool CSettings::Save(TiXmlNode *settings) const
{
  CSingleLock lock(m_critical);
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->Save(settings))
      return false;
  }

  return true;
}
