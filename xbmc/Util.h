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
#include <vector>
#include <stdint.h>
#include "utils/StringUtils.h"
#include "MediaSource.h"
#include "utils/Digest.h"

#include "platform/xbox/custom_launch_params.h"

#define ARRAY_SIZE(X)         (sizeof(X)/sizeof((X)[0]))

// A list of filesystem types for LegalPath/FileName
#define LEGAL_NONE            0
#define LEGAL_WIN32_COMPAT    1
#define LEGAL_FATX            2

namespace XFILE
{
  class IFileCallback;
}

class CFileItem;
class CFileItemList;
class CURL;

// for 'cherry' patching
typedef enum
{
  COUNTRY_NULL = 0,
  COUNTRY_USA,
  COUNTRY_JAP,
  COUNTRY_EUR
} F_COUNTRY;

typedef enum
{
  VIDEO_NULL = 0,
  VIDEO_NTSCM,
  VIDEO_NTSCJ,
  VIDEO_PAL50,
  VIDEO_PAL60
} F_VIDEO;

struct XBOXDETECTION
{
  std::vector<std::string> client_ip;
  std::vector<std::string> client_info;
  std::vector<unsigned int> client_lookup_count;
  std::vector<bool> client_informed;
};

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static bool GetVolumeFromFileName(const std::string& strFileName, std::string& strFileTitle, std::string& strVolumeNumber);
  static void CleanString(const std::string& strFileName, std::string& strTitle, std::string& strTitleAndYear, std::string& strYear, bool bRemoveExtension = false, bool bCleanChars = true);
  static bool GetFilenameIdentifier(const std::string& fileName,
                                    std::string& identifierType,
                                    std::string& identifier);
  static bool GetFilenameIdentifier(const std::string& fileName,
                                    std::string& identifierType,
                                    std::string& identifier,
                                    std::string& match);
  static bool HasFilenameIdentifier(const std::string& fileName);
  static std::string GetTitleFromPath(const CURL& url, bool bIsFolder = false);
  static std::string GetTitleFromPath(const std::string& strFileNameAndPath, bool bIsFolder = false);

  /*! \brief Return the disc number in case the last segment of given path ends with 'Disc n'.
   Will look for 'Disc', 'Disk' and the locale specific spelling.
   \return the disc number as string if found, empty string otherwise.
   */
  static std::string GetDiscNumberFromPath(const std::string& path);

  /*! \brief Remove last segment of the given path if it matches 'Disc n'.
   Will look for 'Disc', 'Disk' and the locale specific spelling.
   \return the given path with last segment removed if it matches 'Disc n', unchanged path otherwise.
   */
  static std::string RemoveTrailingDiscNumberSegmentFromPath(std::string path);

  static void GetQualifiedFilename(const std::string &strBasePath, std::string &strFilename);
  static bool PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video);
  static void RunShortcut(const char* szPath);
  static void RunXBE(const char* szPath, char* szParameters = NULL, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL, CUSTOM_LAUNCH_DATA* pData=NULL);
  static void LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL, CUSTOM_LAUNCH_DATA* pData=NULL);
  static void GetHomePath(std::string& strPath);
  static bool ExcludeFileOrFolder(const std::string& strFileOrFolder, const std::vector<std::string>& regexps);
  static void GetFileAndProtocol(const std::string& strURL, std::string& strDir);
  static int GetDVDIfoTitle(const std::string& strPathFile);
  static bool CacheXBEIcon(const std::string& strFilePath, const std::string& strIcon);
  static bool GetXBEDescription(const std::string& strFileName, std::string& strDescription);
  static bool SetXBEDescription(const std::string& strFileName, const std::string& strDescription);
  static DWORD GetXbeID( const std::string& strFilePath);

  /*! \brief retrieve MD5sum of a file
   \param strPath - path to the file to MD5sum
   \return md5 sum of the file
   */
  static std::string GetFileDigest(const std::string& strPath, KODI::UTILITY::CDigest::Type type);
  static bool GetDirectoryName(const std::string& strFileName, std::string& strDescription);
  static void CreateShortcuts(CFileItemList &items);
  static void CreateShortcut(CFileItem* pItem);
  static std::string GetFatXQualifiedPath(const std::string& strPath);
  static bool ShortenFileName(std::string& strFileNameAndPath);
  static bool IsWritable(const std::string& strFile);
  static bool IsPicture(const std::string& strFile);
  static void GetDVDDriveIcon( const std::string& strPath, std::string& strIcon );
  static void RemoveTempFiles();
  static void ClearTempFonts();
  static void DeleteGUISettings();

  static void RemoveIllegalChars(std::string& strText);
  static void CacheSubtitles(const std::string& strMovie, std::string& strExtensionCached, XFILE::IFileCallback *pCallback = NULL);
  static bool CacheRarSubtitles(const std::string& strRarPath, const std::string& strCompare);
  static void ClearSubtitles();
  static void PrepareSubtitleFonts();
  static __int64 ToInt64(DWORD dwHigh, DWORD dwLow);
  static void PlayDVD(const std::string& strProtocol = "dvd", bool restart = false);
  static std::string GetNextFilename(const std::string &fn_template, int max);
  static std::string GetNextPathname(const std::string &path_template, int max);
  static void TakeScreenshot();
  static void TakeScreenshot(const std::string& strFileName, bool flash);
  static void SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate);
  static void SetBrightnessContrastGammaPercent(float brightness, float contrast, float gamma, bool immediate);
  static void FlashScreen(bool bImmediate, bool bOn);
  static void RestoreBrightnessContrastGamma();
  static void InitGamma();
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct _stat *result, struct __stat64 *stat);
  static bool CreateDirectoryEx(const std::string& strPath);

#ifdef _WIN32
  static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_WIN32_COMPAT);
  static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
  static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_NONE);
  static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_NONE);
#endif
  static std::string ValidatePath(const std::string &path, bool bFixDoubleSlashes = false); ///< return a validated path, with correct directory separators.

  static bool IsUsingTTFSubtitles();
  static void SplitParams(const std::string &paramString, std::vector<std::string> &parameters);
  static void SplitExecFunction(const std::string &execString, std::string &function, std::vector<std::string> &parameters);
  static int GetMatchingSource(const std::string& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static std::string TranslateSpecialSource(const std::string &strSpecial);
  static void DeleteDirectoryCache(const std::string &prefix = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static void DeleteProgramDatabaseDirectoryCache();
  static std::string MusicPlaylistsLocation();
  static std::string VideoPlaylistsLocation();

  static bool SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute);
  static int GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew);
  static bool SetXBOXNickName(std::string strXboxNickNameIn, std::string &strXboxNickNameOut);
  static bool GetXBOXNickName(std::string &strXboxNickNameOut);
  static bool AutoDetectionPing(std::string strFTPUserName, std::string strFTPPass, std::string strNickName, int iFTPPort);
  static bool AutoDetection();
  static void AutoDetectionGetSource(VECSOURCES &share);
  static void GetSkinThemes(std::vector<std::string>& vecTheme);
  static void GetRecursiveListing(const std::string& strPath, CFileItemList& items, const std::string& strMask, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void GetRecursiveDirsListing(const std::string& strPath, CFileItemList& items, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void WipeDir(const std::string& strPath);
  static void ForceForwardSlashes(std::string& strPath);
  static bool PWMControl(const std::string &strRGBa, const std::string &strRGBb, const std::string &strWhiteA, const std::string &strWhiteB, const std::string &strTransition, int iTrTime);
  static bool RunFFPatchedXBE(std::string szPath1, std::string& szNewPath);
  static void RemoveKernelPatch();
  static bool LookForKernelPatch();

  static double AlbumRelevance(const std::string& strAlbumTemp1, const std::string& strAlbum1, const std::string& strArtistTemp1, const std::string& strArtist1);
  static bool MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength);
  /*! \brief Checks wether the supplied path supports Write file operations (e.g. Rename, Delete, ...)

   \param strPath the path to be checked

   \return true if Write file operations are supported, false otherwise
   */
  static bool SupportsWriteFileOperations(const std::string& strPath);
  /*! \brief Checks wether the supplied path supports Read file operations (e.g. Copy, ...)

   \param strPath the path to be checked

   \return true if Read file operations are supported, false otherwise
   */
  static bool SupportsReadFileOperations(const std::string& strPath);
  static std::string GetDefaultFolderThumb(const std::string &folderThumb);

  static void BootToDash();

  static void InitRandomSeed();

  // Get decimal integer representation of roman digit, ivxlcdm are valid
  // return 0 for other chars;
  static int LookupRomanDigit(char roman_digit);
  // Translate a string of roman numerals to decimal a decimal integer
  // return -1 on error, valid range is 1-3999
  static int TranslateRomanNumeral(const char* roman_numeral);

  static bool CanBindPrivileged();
  static bool ValidatePort(int port);

  /*!
   * \brief Thread-safe random number generation
   */
  static int GetRandomNumber();

  static int64_t ConvertSecsToMilliSecs(double secs) { return static_cast<int64_t>(secs * 1000); }
  static double ConvertMilliSecsToSecs(int64_t offset) { return offset / 1000.0; }
  static int64_t ConvertMilliSecsToSecsInt(int64_t offset) { return offset / 1000; }
  static int64_t ConvertMilliSecsToSecsIntRounded(int64_t offset) { return ConvertMilliSecsToSecsInt(offset + 499); }

  /** \brief Copy files from the application bundle over to the user data directory in Application Support/Kodi.
  */
  static void CopyUserDataIfNeeded(const std::string& strPath,
                                   const std::string& file,
                                   const std::string& destname = "");
};


