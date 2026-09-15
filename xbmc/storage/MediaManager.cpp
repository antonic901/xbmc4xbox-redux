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

#include "MediaManager.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include "platform/xbox/storage/IoSupport.h"

using namespace std;

const char MEDIA_SOURCES_XML[] = { "Q:\\system\\mediasources.xml" };

CMediaManager::CMediaManager()
{
}

bool CMediaManager::LoadSources()
{
  // clear our location list
  m_locations.clear();

  // load xml file...
  CXBMCTinyXML xmlDoc;
  if ( !xmlDoc.LoadFile( MEDIA_SOURCES_XML ) )
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if ( !pRootElement || strcmpi(pRootElement->Value(), "mediasources") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", MEDIA_SOURCES_XML, xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  // load the <network> block
  TiXmlNode *pNetwork = pRootElement->FirstChild("network");
  if (pNetwork)
  {
    TiXmlElement *pLocation = pNetwork->FirstChildElement("location");
    while (pLocation)
    {
      CNetworkLocation location;
      pLocation->Attribute("id", &location.id);
      if (pLocation->FirstChild())
      {
        location.path = pLocation->FirstChild()->Value();
        m_locations.push_back(location);
      }
      pLocation = pLocation->NextSiblingElement("location");
    }
  }
  LoadAddonSources();
  return true;
}

bool CMediaManager::SaveSources()
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("mediasources");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  TiXmlElement networkNode("network");
  TiXmlNode *pNetworkNode = pRoot->InsertEndChild(networkNode);
  if (pNetworkNode)
  {
    for (vector<CNetworkLocation>::iterator it = m_locations.begin(); it != m_locations.end(); it++)
    {
      TiXmlElement locationNode("location");
      locationNode.SetAttribute("id", (*it).id);
      TiXmlText value((*it).path);
      locationNode.InsertEndChild(value);
      pNetworkNode->InsertEndChild(locationNode);
    }
  }
  return xmlDoc.SaveFile(MEDIA_SOURCES_XML);
}

void CMediaManager::GetLocalDrives(VECSOURCES &localDrives, bool includeQ)
{
  // Local shares
  CMediaSource share;
  share.strPath = "C:\\";
  share.strName = StringUtils::Format(g_localizeStrings.Get(21438).c_str(),'C');
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);
  share.strPath = "D:\\";
  share.strName = g_localizeStrings.Get(218);
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  localDrives.push_back(share);
  share.strPath = "E:\\";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  share.strName = StringUtils::Format(g_localizeStrings.Get(21438).c_str(),'E');
  localDrives.push_back(share);
  for (int driveCount=EXTEND_PARTITION_BEGIN;driveCount<=(EXTEND_PARTITION_BEGIN+EXTEND_PARTITIONS_LIMIT-1);driveCount++)
  {
    if (CIoSupport::DriveExists(CIoSupport::GetExtendedPartitionDriveLetter(driveCount-EXTEND_PARTITION_BEGIN)))
    {
      CMediaSource share;
      share.strPath = StringUtils::Format("%c:\\", CIoSupport::GetExtendedPartitionDriveLetter(driveCount-EXTEND_PARTITION_BEGIN));
      CLog::Log(LOGINFO, "  Local Source Drive %c:", CIoSupport::GetExtendedPartitionDriveLetter(driveCount-EXTEND_PARTITION_BEGIN));
      share.strName = StringUtils::Format(g_localizeStrings.Get(21438).c_str(),CIoSupport::GetExtendedPartitionDriveLetter(driveCount-EXTEND_PARTITION_BEGIN));
      share.m_ignore = true;
      localDrives.push_back(share);
    }
  }
  if (includeQ)
  {
    CMediaSource share;
    share.strPath = "Q:\\";
    share.strName = StringUtils::Format(g_localizeStrings.Get(21438).c_str(),'Q');
    share.m_ignore = true;
    localDrives.push_back(share);
  }
}

void CMediaManager::GetNetworkLocations(VECSOURCES &locations)
{
  for (unsigned int i = 0; i < m_locations.size(); i++)
  {
    CMediaSource share;
    share.strPath = m_locations[i].path;
    CURL url(share.strPath);
    share.strName = url.GetWithoutUserDetails();
    locations.push_back(share);
  }
}

bool CMediaManager::AddNetworkLocation(const std::string &path)
{
  CNetworkLocation location;
  location.path = path;
  location.id = (int)m_locations.size();
  m_locations.push_back(location);
  return SaveSources();
}

bool CMediaManager::HasLocation(const std::string& path) const
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
      return true;
  }

  return false;
}


bool CMediaManager::RemoveLocation(const std::string& path)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
    {
      // prompt for sources, remove, cancel,
      m_locations.erase(m_locations.begin()+i);
      return SaveSources();
    }
  }

  return false;
}

bool CMediaManager::SetLocationPath(const std::string& oldPath, const std::string& newPath)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == oldPath)
    {
      m_locations[i].path = newPath;
      return SaveSources();
    }
  }

  return false;
}

void CMediaManager::LoadAddonSources() const
{
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVirtualShares)
  {
    CMediaSourceSettings::GetInstance().AddShare("video", GetRootAddonTypeSource("video"));
    CMediaSourceSettings::GetInstance().AddShare("programs", GetRootAddonTypeSource("programs"));
    CMediaSourceSettings::GetInstance().AddShare("pictures", GetRootAddonTypeSource("pictures"));
    CMediaSourceSettings::GetInstance().AddShare("music", GetRootAddonTypeSource("music"));
  }
}

CMediaSource CMediaManager::GetRootAddonTypeSource(const std::string& type) const
{
  if (type == "programs" || type == "myprograms")
  {
    return ComputeRootAddonTypeSource("executable", g_localizeStrings.Get(1043),
                                      "DefaultAddonProgram.png");
  }
  else if (type == "video" || type == "videos")
  {
    return ComputeRootAddonTypeSource("video", g_localizeStrings.Get(1037),
                                      "DefaultAddonVideo.png");
  }
  else if (type == "music")
  {
    return ComputeRootAddonTypeSource("music", g_localizeStrings.Get(1038),
                                      "DefaultAddonMusic.png");
  }
  else if (type == "pictures")
  {
    return ComputeRootAddonTypeSource("image", g_localizeStrings.Get(1039),
                                      "DefaultAddonPicture.png");
  }
  else
  {
    CLog::Log(LOGERROR, "Invalid type %s provided", type.c_str());
    return CMediaSource();
  }
}

CMediaSource CMediaManager::ComputeRootAddonTypeSource(const std::string& type,
                                                       const std::string& label,
                                                       const std::string& thumb) const
{
  CMediaSource source;
  source.strPath = "addons://sources/" + type + "/";
  source.strName = label;
  source.m_strThumbnailImage = thumb;
  source.m_iDriveType = CMediaSource::SOURCE_TYPE_VPATH;
  source.m_ignore = true;
  return source;
}
