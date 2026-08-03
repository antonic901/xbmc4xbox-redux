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

#include "FatXFileSystem.h"
#include "FatXDevice.h"
#include "platform/xbox/filesystem/MemoryUnitManager.h"
#include "filesystem/HDDirectory.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/StringUtils.h"

using namespace XFILE;

CFatXFileSystem::CFatXFileSystem(unsigned char unit) : IFileSystem(unit)
{
}

bool CFatXFileSystem::Open(const std::string &file)
{
  CURL url(GetLocal(file));
  return m_file.Open(url);
}

bool CFatXFileSystem::OpenForWrite(const std::string &file, bool overWrite)
{
  CURL url(GetLocal(file));
  return m_file.OpenForWrite(url, overWrite);
}

void CFatXFileSystem::Close()
{
  m_file.Close();
}

unsigned int CFatXFileSystem::Read(void *buffer, __int64 length)
{
  return m_file.Read(buffer, length);
}

unsigned int CFatXFileSystem::Write(const void *buffer, __int64 length)
{
  return m_file.Write(buffer, length);
}

__int64 CFatXFileSystem::Seek(__int64 position)
{
  return m_file.Seek(position, SEEK_SET);
}

__int64 CFatXFileSystem::GetLength()
{
  return m_file.GetLength();
}

__int64 CFatXFileSystem::GetPosition()
{
  return m_file.GetPosition();
}

bool CFatXFileSystem::Delete(const std::string &file)
{
  CURL url(GetLocal(file));
  CHDFile hdFile;
  return hdFile.Delete(url);
}

bool CFatXFileSystem::Rename(const std::string &oldFile, const std::string &newFile)
{
  CURL urlOld(GetLocal(oldFile));
  CURL urlNew(GetLocal(newFile));
  CHDFile hdFile;
  return hdFile.Rename(urlOld, urlNew);
}

bool CFatXFileSystem::MakeDir(const std::string &path)
{
  CHDDirectory hdDir;
  const CURL pathToUrl(GetLocal(path));
  return hdDir.Create(pathToUrl);
}

bool CFatXFileSystem::RemoveDir(const std::string &path)
{
  CHDDirectory hdDir;
  const CURL pathToUrl(GetLocal(path));
  return hdDir.Remove(pathToUrl);
}

bool CFatXFileSystem::GetDirectory(const std::string &directory, CFileItemList &items)
{
  CHDDirectory hd;
  const CURL pathToUrl(GetLocal(directory));
  if (hd.GetDirectory(pathToUrl, items))
  { // replace our items with our nicer URL
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      std::string path = StringUtils::Format("mem%d://%s", m_unit, item->GetPath().substr(3).c_str());
      path.Replace("\\","/");
      item->SetPath(path);
    }
    return true;
  }
  return false;
}

std::string CFatXFileSystem::GetLocal(const std::string &file)
{
  std::string path;
  CFatXDevice *device = (CFatXDevice *)g_memoryUnitManager.GetDevice(m_unit);
  if (device)
  {
    path = StringUtils::Format("%c:\\%s", device->GetDrive(), file.c_str());
    path.Replace("/", "\\");
  }
  return path;
}
