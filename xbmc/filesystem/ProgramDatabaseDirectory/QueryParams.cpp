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

#include "QueryParams.h"
#include "programs/ProgramDatabase.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;

CQueryParams::CQueryParams()
{
  m_idGame = -1;
  m_idApplication = -1;
  m_idDeveloper = -1;
  m_idPublisher = -1;
  m_idGenre = -1;
  m_idDescriptor = -1;
  m_idGeneralFeature = -1;
  m_idOnlineFeature = -1;
  m_idPlatform = -1;
  m_idYear = -1;
  m_idContent = -1;
  m_idTag = -1;
}

void CQueryParams::SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName)
{
  long idDb=atol(strNodeName.c_str());

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    m_idContent = strNodeName.Equals("games") ? PROGRAMDB_CONTENT_GAMES : PROGRAMDB_CONTENT_APPLICATIONS;
    break;
  case NODE_TYPE_DEVELOPER:
    m_idDeveloper = idDb;
    break;
  case NODE_TYPE_PUBLISHER:
    m_idPublisher = idDb;
    break;
  case NODE_TYPE_GENRE:
    m_idGenre = idDb;
    break;
  case NODE_TYPE_DESCRIPTOR:
    m_idDescriptor = idDb;
    break;
  case NODE_TYPE_GENERALFEATURE:
    m_idGeneralFeature = idDb;
    break;
  case NODE_TYPE_ONLINEFEATURE:
    m_idOnlineFeature = idDb;
    break;
  case NODE_TYPE_PLATFORM:
    m_idPlatform = idDb;
    break;
  case NODE_TYPE_YEAR:
    m_idYear = idDb;
    break;
  case NODE_TYPE_TITLE_GAMES:
  case NODE_TYPE_RECENTLY_ADDED_GAMES:
  case NODE_TYPE_RECENTLY_PLAYED_GAMES:
    m_idGame = idDb;
    break;
  case NODE_TYPE_TITLE_APPLICATIONS:
    m_idApplication = idDb;
    break;
  default:
    break;
  }
}
