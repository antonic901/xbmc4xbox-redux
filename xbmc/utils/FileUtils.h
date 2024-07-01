#pragma once
#include "FileItem.h"
class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item, bool force=false);
  static bool DeleteItem(const CStdString &strPath, bool force=false);
  static bool RenameFile(const CStdString &strFile);
  static bool RemoteAccessAllowed(const CStdString &strPath);
  /*! \brief Get the modified date of a file if its invalid it returns the creation date - this behavior changes when you set bUseLatestDate
  \param strFileNameAndPath path to the file
  \param bUseLatestDate use the newer datetime of the files mtime and ctime
  \return Returns the file date, can return a invalid date if problems occure
  */
  static CDateTime GetModificationDate(const std::string& strFileNameAndPath, const bool& bUseLatestDate);
};
