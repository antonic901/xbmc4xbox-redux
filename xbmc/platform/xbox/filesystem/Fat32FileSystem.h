#pragma once
/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "IFileSystem.h"
#include "dosfs.h"

namespace XFILE
{
  class CFat32FileSystem : public IFileSystem
  {
    enum OPEN_STATE { CLOSED = 0, OPEN_FOR_READ, OPEN_FOR_WRITE };

  public:
    CFat32FileSystem(unsigned char unit);

    virtual bool Open(const std::string &file);
    virtual bool OpenForWrite(const std::string &file, bool overWrite);
    unsigned int Read(void *buffer, __int64 size);
    unsigned int Write(const void *buffer, __int64 size);
    virtual __int64 Seek(__int64 iFilePosition);
    virtual void Close();
    virtual __int64 GetLength();
    virtual __int64 GetPosition();
    virtual bool GetDirectory(const std::string &directory, CFileItemList &items);
    virtual bool Delete(const std::string &file);
    virtual bool Rename(const std::string &oldFile, const std::string &newFile);
    virtual bool MakeDir(const std::string &path);
    virtual bool RemoveDir(const std::string &path);
  protected:
    bool GetDirectoryWithShortPaths(const std::string &directory, CFileItemList &items);
    bool GetShortFilePath(const std::string &longPath, std::string &shortPath);
    FILEINFO m_file;
    OPEN_STATE m_opened;
  };
};
