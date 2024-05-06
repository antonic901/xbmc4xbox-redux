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

#include "system.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "Util.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "filesystem/StackDirectory.h"
#include "addons/Addon.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <sys\stat.h>

using namespace std;
using namespace ADDON;

CStdString URLEncodeInline(const CStdString& strData)
{
  CStdString buffer = strData;
  CURL::Encode(buffer);
  return buffer;
}

CURL::CURL(const CStdString& strURL1)
{
  Parse(strURL1);
}

CURL::CURL()
{
  m_iPort = 0;
}

CURL::~CURL()
{
}

void CURL::Reset()
{
  m_strHostName.clear();
  m_strDomain.clear();
  m_strUserName.clear();
  m_strPassword.clear();
  m_strShareName.clear();
  m_strFileName.clear();
  m_strProtocol.clear();
  m_strFileType.clear();
  m_iPort = 0;
}

void CURL::Parse(const CStdString& strURL1)
{
  Reset();
  // start by validating the path
  CStdString strURL = CUtil::ValidatePath(strURL1);

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return ;
  if (strURL.Equals("?", true)) return;

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  int iPos = strURL.Find("://");
  if (iPos < 0)
  {
    // This is an ugly hack that needs some work.
    // example: filename /foo/bar.zip/alice.rar/bob.avi
    // This should turn into zip://rar:///foo/bar.zip/alice.rar/bob.avi
    iPos = 0;
    while (1)
    {
      iPos = strURL.Find(".zip/", iPos);
      int extLen = 3;
      if (iPos < 0)
      {
        /* set filename and update extension*/
        SetFileName(strURL);
        return ;
      }
      iPos += extLen + 1;
      CStdString archiveName = strURL.Left(iPos);
      struct __stat64 s;
      if (XFILE::CFile::Stat(archiveName, &s) == 0)
      {
#ifdef _LINUX
        if (!S_ISDIR(s.st_mode))
#else
        if (!(s.st_mode & S_IFDIR))
#endif
        {
          CURL::Encode(archiveName);
          CURL c((CStdString)"zip" + "://" + archiveName + '/' + strURL.Right(strURL.size() - iPos - 1));
          *this = c;
          return;
        }
      }
    }
  }
  else
  {
    SetProtocol(strURL.Left(iPos));
    iPos += 3;
  }

  // virtual protocols
  // why not handle all format 2 (protocol://file) style urls here?
  // ones that come to mind are iso9660, cdda, musicdb, etc.
  // they are all local protocols and have no server part, port number, special options, etc.
  // this removes the need for special handling below.
  if (
    IsProtocol("stack") ||
    IsProtocol("virtualpath") ||
    IsProtocol("multipath") ||
    IsProtocol("filereader") ||
    IsProtocol("special")
    )
  {
    SetFileName(strURL.Mid(iPos));
    return;
  }

  // check for username/password - should occur before first /
  if (iPos == -1) iPos = 0;

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  int iEnd = strURL.length();
  const char* sep = NULL;

  std::string strProtocol2 = GetTranslatedProtocol();
  if(IsProtocol("rss") ||
     IsProtocol("rar") ||
     IsProtocol("addons") ||
     IsProtocol("image") ||
     IsProtocol("videodb") ||
     IsProtocol("musicdb"))
    sep = "?";
  else
  if(  IsProtocolEqual(strProtocol2, "http")
    || IsProtocolEqual(strProtocol2, "https")
    || IsProtocolEqual(strProtocol2, "plugin")
    || IsProtocolEqual(strProtocol2, "hdhomerun")
    || IsProtocolEqual(strProtocol2, "rtsp")
    || IsProtocolEqual(strProtocol2, "zip"))
    sep = "?;#|";
  else if(IsProtocolEqual(strProtocol2, "ftp")
       || IsProtocolEqual(strProtocol2, "ftps"))
    sep = "?;";

  if(sep)
  {
    int iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions >= 0 )
    {
      // we keep the initial char as it can be any of the above
      int iProto = strURL.find_first_of("|",iOptions);
      if (iProto >= 0)
      {
        m_strProtocolOptions = strURL.substr(iProto+1);
        m_strOptions = strURL.substr(iOptions,iProto-iOptions);
      }
      else
        m_strOptions = strURL.substr(iOptions);
      iEnd = iOptions;
      m_options.AddOptions(m_strOptions);
    }
  }

  int iSlash = strURL.Find("/", iPos);
  if(iSlash >= iEnd)
    iSlash = -1; // was an invalid slash as it was contained in options

  if( !IsProtocol("iso9660") )
  {
    int iAlphaSign = strURL.Find("@", iPos);
    if (iAlphaSign >= 0 && iAlphaSign < iEnd && (iAlphaSign < iSlash || iSlash < 0))
    {
      // username/password found
      CStdString strUserNamePassword = strURL.Mid(iPos, iAlphaSign - iPos);

      // first extract domain, if protocol is smb
      if (IsProtocol("smb"))
      {
        int iSemiColon = strUserNamePassword.Find(";");

        if (iSemiColon >= 0)
        {
          m_strDomain = strUserNamePassword.Left(iSemiColon);
          strUserNamePassword.Delete(0, iSemiColon + 1);
        }
      }

      // username:password
      int iColon = strUserNamePassword.Find(":");
      if (iColon >= 0)
      {
        m_strUserName = strUserNamePassword.Left(iColon);
        iColon++;
        m_strPassword = strUserNamePassword.Right(strUserNamePassword.size() - iColon);
      }
      // username
      else
      {
        m_strUserName = strUserNamePassword;
      }

      iPos = iAlphaSign + 1;
      iSlash = strURL.Find("/", iAlphaSign);

      if(iSlash >= iEnd)
        iSlash = -1;
    }
  }

  // detect hostname:port/
  if (iSlash < 0)
  {
    CStdString strHostNameAndPort = strURL.Mid(iPos, iEnd - iPos);
    int iColon = strHostNameAndPort.Find(":");
    if (iColon >= 0)
    {
      m_strHostName = strHostNameAndPort.Left(iColon);
      iColon++;
      CStdString strPort = strHostNameAndPort.Right(strHostNameAndPort.size() - iColon);
      m_iPort = atoi(strPort.c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }

  }
  else
  {
    CStdString strHostNameAndPort = strURL.Mid(iPos, iSlash - iPos);
    int iColon = strHostNameAndPort.Find(":");
    if (iColon >= 0)
    {
      m_strHostName = strHostNameAndPort.Left(iColon);
      iColon++;
      CStdString strPort = strHostNameAndPort.Right(strHostNameAndPort.size() - iColon);
      m_iPort = atoi(strPort.c_str());
    }
    else
    {
      m_strHostName = strHostNameAndPort;
    }
    iPos = iSlash + 1;
    if (iEnd > iPos)
    {
      m_strFileName = strURL.Mid(iPos, iEnd - iPos);

      iSlash = m_strFileName.Find("/");
      if(iSlash < 0)
        m_strShareName = m_strFileName;
      else
        m_strShareName = m_strFileName.Left(iSlash);
    }
  }

  // iso9960 doesnt have an hostname;-)
  if (IsProtocol("iso9660")
   || IsProtocol("musicdb")
   || IsProtocol("videodb")
   || IsProtocol("sources")
   || IsProtocol("mem"))
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      CStdString strFileName = m_strFileName;
      m_strFileName.Format("%s/%s", m_strHostName.c_str(), strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      if (!m_strHostName.IsEmpty() && strURL[iEnd-1]=='/')
        m_strFileName = m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  m_strFileName.Replace("\\", "/");

  /* update extension */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  if(URIUtils::HasEncodedHostname(*this))
  {
    CURL::Decode(m_strHostName);
    // Validate it as it is likely to contain a filename
    SetHostName(CUtil::ValidatePath(m_strHostName));
  }

  CURL::Decode(m_strUserName);
  CURL::Decode(m_strPassword);
}

void CURL::SetFileName(const CStdString& strFileName)
{
  m_strFileName = strFileName;

  int slash = m_strFileName.find_last_of(GetDirectorySeparator());
  int period = m_strFileName.find_last_of('.');
  if(period != -1 && (slash == -1 || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  m_strFileType.Normalize();
}

void CURL::SetHostName(const CStdString& strHostName)
{
  m_strHostName = strHostName;
}

void CURL::SetUserName(const CStdString& strUserName)
{
  m_strUserName = strUserName;
}

void CURL::SetPassword(const CStdString& strPassword)
{
  m_strPassword = strPassword;
}

void CURL::SetProtocol(const CStdString& strProtocol)
{
  m_strProtocol = strProtocol;
  m_strProtocol.ToLower();
}

void CURL::SetOptions(const CStdString& strOptions)
{
  m_strOptions.Empty();
  m_options.Clear();
  if( strOptions.length() > 0)
  {
    if( strOptions[0] == '?' || strOptions[0] == '#' || strOptions[0] == ';' || strOptions.Find("xml") >=0 )
    {
      m_strOptions = strOptions;
      m_options.AddOptions(m_strOptions);
    }
    else
      CLog::Log(LOGWARNING, "%s - Invalid options specified for url %s", __FUNCTION__, strOptions.c_str());
  }
}

void CURL::SetProtocolOptions(const CStdString& strOptions)
{
  m_strProtocolOptions = strOptions;
}

void CURL::SetPort(int port)
{
  m_iPort = port;
}

bool CURL::HasPort() const
{
  return (m_iPort != 0);
}

int CURL::GetPort() const
{
  return m_iPort;
}


const CStdString& CURL::GetHostName() const
{
  return m_strHostName;
}

const CStdString&  CURL::GetShareName() const
{
  return m_strShareName;
}

const CStdString& CURL::GetDomain() const
{
  return m_strDomain;
}

const CStdString& CURL::GetUserName() const
{
  return m_strUserName;
}

const CStdString& CURL::GetPassWord() const
{
  return m_strPassword;
}

const CStdString& CURL::GetFileName() const
{
  return m_strFileName;
}

const CStdString& CURL::GetProtocol() const
{
  return m_strProtocol;
}

const CStdString CURL::GetTranslatedProtocol() const
{
  if (IsProtocol("ftpx"))
    return "ftp";

  if (IsProtocol("shout")
   || IsProtocol("daap")
   || IsProtocol("dav")
   || IsProtocol("tuxbox")
   || IsProtocol("mms")
   || IsProtocol("rss"))
    return "http";

  if (IsProtocol("davs"))
    return "https";

  return GetProtocol();
}

const CStdString& CURL::GetFileType() const
{
  return m_strFileType;
}

const CStdString& CURL::GetOptions() const
{
  return m_strOptions;
}

const CStdString& CURL::GetProtocolOptions() const
{
  return m_strProtocolOptions;
}

const CStdString CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  if ((IsProtocol("rar")  ||
       IsProtocol("zip")  ||
       IsProtocol("apk")) &&
       m_strFileName.empty())
    return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  CStdString file(m_strFileName);
  URIUtils::RemoveSlashAtEnd(file);
  return URIUtils::GetFileName(file);
}

char CURL::GetDirectorySeparator() const
{
  if ( IsLocal() )
    return '\\';
  else
    return '/';
}

CStdString CURL::Get() const
{
  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (m_strProtocol == "")
    return m_strFileName;

  CStdString strURL;
  strURL.reserve(sizeneed);

  strURL = GetWithoutFilename();
  strURL += m_strFileName;

  if( !m_strOptions.empty() )
    strURL += m_strOptions;
  if (!m_strProtocolOptions.empty())
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  std::string strURL;

  if (IsProtocol("stack"))
  {
    CFileItemList items;
    XFILE::CStackDirectory dir;
    dir.GetDirectory(*this,items);
    vector<std::string> newItems;
    for (int i=0;i<items.Size();++i)
    {
      CURL url(items[i]->GetPath());
      items[i]->SetPath(url.GetWithoutUserDetails(redact));
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems, strURL);
    return strURL;
  }

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (redact)
    sizeneed += sizeof("USERNAME:PASSWORD@");

  strURL.reserve(sizeneed);

  if (m_strProtocol == "")
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (redact && !m_strUserName.empty())
  {
    strURL += "USERNAME";
    if (!m_strPassword.empty())
    {
      strURL += ":PASSWORD";
    }
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string strHostName;

    if (URIUtils::HasParentInHostname(*this))
      strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    else
      strHostName = m_strHostName;

    if (URIUtils::HasEncodedHostname(*this))
      strURL += URLEncodeInline(strHostName);
    else
      strURL += strHostName;

    if ( HasPort() )
    {
      strURL += StringUtils2::Format(":%i", m_iPort);
    }
    strURL += "/";
  }
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
  if( m_strProtocolOptions.length() > 0 )
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

CStdString CURL::GetWithoutFilename() const
{
  if (m_strProtocol == "")
    return "";

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  CStdString strURL;
  strURL.reserve(sizeneed);

  strURL = m_strProtocol;
  strURL += "://";

  if (m_strDomain != "")
  {
    strURL += m_strDomain;
    strURL += ";";
  }
  else if (m_strUserName != "")
  {
    strURL += URLEncodeInline(m_strUserName);
    if (m_strPassword != "")
    {
      strURL += ":";
      strURL += URLEncodeInline(m_strPassword);
    }
    strURL += "@";
  }
  else if (m_strDomain != "")
    strURL += "@";

  if (m_strHostName != "")
  {
    if( URIUtils::HasEncodedHostname(*this) )
      strURL += URLEncodeInline(m_strHostName);
    else
      strURL += m_strHostName;
    if (HasPort())
    {
      CStdString strPort;
      strPort.Format("%i", m_iPort);
      strURL += ":";
      strURL += strPort;
    }
    strURL += "/";
  }

  return strURL;
}

std::string CURL::GetRedacted() const
{
  return GetWithoutUserDetails(true);
}

std::string CURL::GetRedacted(const std::string& path)
{
  return CURL(path).GetRedacted();
}

bool CURL::IsLocal() const
{
  return m_strProtocol.IsEmpty();
}

bool CURL::IsFileOnly(const CStdString &url)
{
  return url.find_first_of("/\\") == CStdString::npos;
}

bool CURL::IsFullPath(const CStdString &url)
{
  if (url.size() && url[0] == '/') return true;     //   /foo/bar.ext
  if (url.Find("://") >= 0) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (URIUtils::PathStarts(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

void CURL::Decode(CStdString& strURLData)
//modified to be more accomodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  CStdString strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp,"%x",(unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  strURLData = strResult;
}

void CURL::Encode(CStdString& strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (size_t i = 0; i < strURLData.size(); ++i)
  {
    const char kar = strURLData[i];

    // Don't URL encode "-_.!()" according to RFC1738
    // TODO: Update it to "-_.~" after Gotham according to RFC3986
    if (StringUtils2::isasciialphanum(kar) || kar == '-' || kar == '.' || kar == '_' || kar == '!' || kar == '(' || kar == ')')
      strResult.push_back(kar);
    else
      strResult += StringUtils2::Format("%%%02.2x", (unsigned int)((unsigned char)kar)); // TODO: Change to "%%%02.2X" after Gotham
  }
  strURLData = strResult;
}

std::string CURL::Decode(const std::string& strURLData)
{
  CStdString url = strURLData;
  Decode(url);
  return url;
}

std::string CURL::Encode(const std::string& strURLData)
{
  CStdString url = strURLData;
  Encode(url);
  return url;
}

void CURL::GetOptions(std::map<CStdString, CStdString> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
    options[option->first] = option->second.asString();
}

bool CURL::HasOption(const CStdString &key) const
{
  return m_options.HasOption(key);
}

bool CURL::GetOption(const CStdString &key, CStdString &value) const
{
  CVariant valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

CStdString CURL::GetOption(const CStdString &key) const
{
  CStdString value;
  if (!GetOption(key, value))
    return "";

  return value;
}

void CURL::SetOption(const CStdString &key, const CStdString &value)
{
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const CStdString &key)
{
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

bool CURL::IsProtocolEqual(const std::string &protocol, const char *type)
{
  /*
   NOTE: We're currently using == here as m_strProtocol is assigned as lower-case in SetProtocol(),
   and we've assumed all other callers are calling with protocol lower-case otherwise.
   We possibly shouldn't do this (as CURL(foo).Get() != foo, though there are other reasons for this as well)
   but it handles the requirements of RFC-1738 which allows the scheme to be case-insensitive.
   */
  if (type)
    return protocol == type;
  return false;
}
