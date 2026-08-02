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

#include <xtl.h>
#include <string>

class CGFFPatch
{
  public:
    bool FFPatch(const std::string& m_FFPatchFilePath, std::string &strNEW_FFPatchFilePath); //
  private:
    BOOL applyPatches(BYTE* pbuffer, int patchCount);
    BOOL Patch1(BYTE* pbuffer, UINT location);
    BOOL Patch2(BYTE* pbuffer, UINT location);
    BOOL Patch3(BYTE* pbuffer, UINT location);
    BOOL Patch4(BYTE* pbuffer, UINT location);
    void replaceConditionalJump(BYTE* pbuffer, UINT &location, UINT range);
    BOOL findConditionalJump(BYTE* pbuffer, UINT &location, UINT range);
    int examinePatch(BYTE* pbuffer, UINT location);
    UINT searchData(BYTE* pBuffer, UINT startPos, UINT size);
};
