#pragma once

#include <string>

class CFileItemList;

class IFileSystem
{
public:
  IFileSystem(unsigned char unit) { m_unit = unit; };

  virtual bool Open(const std::string &file)=0;
  virtual bool OpenForWrite(const std::string &file, bool overWrite)=0;
  virtual unsigned int Read(void *buffer, __int64 size)=0;
  virtual unsigned int Write(const void *buffer, __int64 size)=0;
  virtual __int64 Seek(__int64 iFilePosition)=0;
  virtual void Close()=0;
  virtual __int64 GetLength()=0;
  virtual __int64 GetPosition()=0;
  virtual bool GetDirectory(const std::string &directory, CFileItemList &items)=0;
  virtual bool Delete(const std::string &file)=0;
  virtual bool Rename(const std::string &oldFile, const std::string &newFile)=0;
  virtual bool MakeDir(const std::string &path)=0;
  virtual bool RemoveDir(const std::string &path)=0;
protected:
  unsigned char m_unit;
};
