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

#include "DirectoryNode.h"

namespace XFILE
{
  namespace PROGRAMDATABASEDIRECTORY
  {
    class CQueryParams
    {
    public:
      CQueryParams();
      long GetContentType() const { return m_idContent; }
      long GetGameId() const { return m_idGame; }
      long GetApplicationId() const { return m_idApplication; }
      long GetYear() const { return m_idYear; }
      long GetDeveloperId() const { return m_idDeveloper; }
      long GetPublisherId() const { return m_idPublisher; }
      long GetGenreId() const { return m_idGenre; }
      long GetDescriptorId() const { return m_idDescriptor; }
      long GetGeneralFeatureId() const { return m_idGeneralFeature; }
      long GetOnlineFeatureId() const { return m_idOnlineFeature; }
      long GetPlatformId() const { return m_idPlatform; }
      long GetTagId() const { return m_idTag; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend class CDirectoryNode;
    private:
      long m_idContent;
      long m_idGame;
      long m_idApplication;
      long m_idDeveloper;
      long m_idPublisher;
      long m_idGenre;
      long m_idDescriptor;
      long m_idGeneralFeature;
      long m_idOnlineFeature;
      long m_idPlatform;
      long m_idYear;
      long m_idTag;
    };
  }
}
