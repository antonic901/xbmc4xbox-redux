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

#include "music/tags/ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderWMA: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderWMA(void);
  virtual ~CMusicInfoTagLoaderWMA();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);

protected:
  void SetTagValueString(const CStdString& strFrameName, const CStdString& strValue, CMusicInfoTag& tag);
  void SetTagValueDWORD(const CStdString& strFrameName, DWORD dwValue, CMusicInfoTag& tag);
  void SetTagValueBinary(const CStdString& strFrameName, const LPBYTE pValue, CMusicInfoTag& tag);
  void SetTagValueBool(const CStdString& strFrameName, BOOL bValue, CMusicInfoTag& tag);
};
}
