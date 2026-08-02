#pragma once

#include "network/UdpClient.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/Key.h"
#include "boost/shared_ptr.hpp"

/******************************** Description *********************************/

/*
 *  Header file that provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.h
 */

/********************************* Includes ***********************************/

typedef char char_t;
typedef struct websRec *webs_t;

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;

class CXbmcHttpShim
{
public:
  CXbmcHttpShim();
  ~CXbmcHttpShim();

  void xbmcForm(webs_t wp, char_t *path, char_t *query);
  int    xbmcCommand( int eid, webs_t wp, int argc, char_t **argv);
  std::string xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);
  std::string xbmcExternalCall(char *command);
  bool checkForFunctionTypeParas(std::string &cmd, std::string &paras);
private:
  std::string flushResult(int eid, webs_t wp, const std::string &output);
};

class CUdpBroadcast : public CUdpClient
{
public:
  CUdpBroadcast();
  ~CUdpBroadcast();
  bool broadcast(std::string message, int port);
};

class CXbmcHttp
{
public:
  std::string userHeader, userFooter;
  bool incWebFooter, incWebHeader, shuttingDown, tempSkipWebFooterHeader;

  CXbmcHttp();
  ~CXbmcHttp();

  int xbmcCommand(const std::string &parameter);
  int xbmcAddToPlayList(int numParas, std::string paras[]);
  int xbmcAddToPlayListFromDB(int numParas, std::string paras[]);
  int xbmcPlayerPlayFile(int numParas, std::string paras[]);
  int xbmcClearPlayList(int numParas, std::string paras[]);
  int xbmcGetCurrentlyPlaying(int numParas, std::string paras[]);
  int xbmcGetXBEID(int numParas, std::string paras[]);
  int xbmcGetXBETitle(int numParas, std::string paras[]);
  int xbmcGetSources(int numParas, std::string paras[]);
  int xbmcGetMediaLocation(int numParas, std::string paras[]);
  int xbmcGetDirectory(int numParas, std::string paras[]);
  int xbmcGetTagFromFilename(int numParas, std::string paras[]);
  int xbmcGetCurrentPlayList();
  int xbmcSetCurrentPlayList(int numParas, std::string paras[]);
  int xbmcGetPlayListContents(int numParas, std::string paras[]);
  int xbmcGetPlayListLength(int numParas, std::string paras[]);
  int xbmcRemoveFromPlayList(int numParas, std::string paras[]);
  int xbmcSetPlayListSong(int numParas, std::string paras[]);
  int xbmcGetPlayListSong(int numParas, std::string paras[]);
  int xbmcSwapPlayListItems(int numParas, std::string paras[]);
  int xbmcSetPlaySpeed(int numParas, std::string paras[]);
  int xbmcGetPlaySpeed();
  int xbmcPlayListNext();
  int xbmcPlayListPrev();
  int xbmcSetVolume(int numParas, std::string paras[]);
  int xbmcGetVolume();
  int xbmcMute();
  int xbmcGetPercentage();
  int xbmcSeekPercentage(int numParas, std::string paras[], bool relative);
  int xbmcAction(int numParas, std::string paras[], int theAction);
  int xbmcExit(int theAction);
  int xbmcGetThumb(int numParas, std::string paras[], bool bGetThumb);
  int xbmcGetThumbFilename(int numParas, std::string paras[]);
  int xbmcLookupAlbum(int numParas, std::string paras[]);
  int xbmcChooseAlbum(int numParas, std::string paras[]);
  int xbmcQueryMusicDataBase(int numParas, std::string paras[]);
  int xbmcQueryVideoDataBase(int numParas, std::string paras[]);
  int xbmcQueryProgramDataBase(int numParas, std::string paras[]);
  int xbmcExecMusicDataBase(int numParas, std::string paras[]);
  int xbmcExecVideoDataBase(int numParas, std::string paras[]);
  int xbmcDownloadInternetFile(int numParas, std::string paras[]);
  int xbmcSetKey(int numParas, std::string paras[]);
  int xbmcSetKeyRepeat(int numParas, std::string paras[]);
  int xbmcGetMovieDetails(int numParas, std::string paras[]);
  int xbmcDeleteFile(int numParas, std::string paras[]);
  int xbmcCopyFile(int numParas, std::string paras[]);
  int xbmcSetFile(int numParas, std::string paras[]);
  int xbmcFileExists(int numParas, std::string paras[]);
  int xbmcFileSize(int numParas, std::string paras[]);
  int xbmcShowPicture(int numParas, std::string paras[]);
  int xbmcGetGUIStatus();
  int xbmcExecBuiltIn(int numParas, std::string paras[]);
  int xbmcSTSetting(int numParas, std::string paras[]);
  int xbmcConfig(int numParas, std::string paras[]);
  int xbmcHelp();
  int xbmcGetSystemInfo(int numParas, std::string paras[]);
  int xbmcGetSystemInfoByName(int numParas, std::string paras[]);
  int xbmcAddToSlideshow(int numParas, std::string paras[]);
  int xbmcClearSlideshow();
  int xbmcPlaySlideshow(int numParas, std::string paras[]);
  int xbmcSlideshowSelect(int numParas, std::string paras[]);
  int xbmcGetSlideshowContents();
  int xbmcGetCurrentSlide();
  int xbmcGUISetting(int numParas, std::string paras[]);
  int xbmcTakeScreenshot(int numParas, std::string paras[]);
  int xbmcGetGUIDescription();
  int xbmcAutoGetPictureThumbs(int numParas, std::string paras[]);
  int xbmcSetResponseFormat(int numParas, std::string paras[]);
  int xbmcSpinDownHardDisk(int numParas, std::string paras[]);
  int xbmcBroadcast(int numParas, std::string paras[]);
  bool xbmcBroadcast(std::string message, int level=0);
  int xbmcSetBroadcast(int numParas, std::string paras[]);
  int xbmcGetBroadcast();
  int xbmcOnAction(int numParas, std::string paras[]);
  int xbmcRecordStatus(int numParas, std::string paras[]);
  int xbmcGetMusicLabel(int numParas, std::string paras[]);
  int xbmcGetVideoLabel(int numParas, std::string paras[]);
  int xbmcGetSkinSetting(int numParas, std::string paras[]);
  int xbmcWebServerStatus(int numParas, std::string paras[]);
  int xbmcGetLogLevel();
  int xbmcSetLogLevel(int numParas, std::string paras[]);
  CKey GetKey();
  void ResetKey();
  std::string GetOpenTag();
  std::string GetCloseTag();

private:
  CKey key;
  CUdpBroadcast* pUdpBroadcast;
  CUdpClient UdpClient;
  CKey lastKey;
  int repeatKeyRate; //ms
  unsigned int MarkTime;
  bool autoGetPictureThumbs;
  std::string lastThumbFn, lastPlayingInfo;
  std::string openTag, closeTag,  openRecordSet, closeRecordSet, openRecord, closeRecord, openField, closeField, openBroadcast, closeBroadcast;
  bool  closeFinalTag;

  void encodeblock( unsigned char in[3], unsigned char out[4], int len );
  std::string encodeFileToBase64(const std::string &inFilename, int linesize );
  void decodeblock( unsigned char in[4], unsigned char out[3] );
  bool decodeBase64ToFile( const std::string &inString, const std::string &outfilename, bool append = false );
  __int64 fileSize(const std::string &filename);
  void resetTags();
  std::string procMask(std::string mask);
  int splitParameter(const std::string &parameter, std::string& command, std::string paras[], const std::string &sep);
  bool playableFile(const std::string &filename);
  int SetResponse(const std::string &response);
  std::string flushResult(int eid, webs_t wp, const std::string &output);
  int displayDir(int numParas, std::string paras[]);
  void SetCurrentMediaItem(CFileItem& newItem);
  void AddItemToPlayList(const CFileItemPtr &pItem, int playList, int sortMethod, std::string mask, bool recursive);
  void LoadPlayListOld(const std::string& strPlayList, int playList);
  bool LoadPlayList(std::string strPath, int iPlaylist, bool clearList, bool autoStart);
  void copyThumb(std::string srcFn, std::string destFn);
  int FindPathInPlayList(int playList, std::string path);
};

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")

extern CXbmcHttp* m_pXbmcHttp; //make it global so Application.cpp can access it for key/button messages
extern CXbmcHttpShim* pXbmcHttpShim;
