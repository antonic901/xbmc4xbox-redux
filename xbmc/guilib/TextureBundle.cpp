

#include "include.h"
#include "TextureBundle.h"
#include "Texture.h"
#include "GraphicContext.h"
#ifdef HAS_XBOX_D3D
#include <XGraphics.h>
#include "utils/CharsetConverter.h"
#else
#include "DirectXGraphics.h"
#endif
#include "liblzo/LZO1X.H"
#include "addons/Skin.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "filesystem/SpecialProtocol.h"

#ifdef _XBOX
#pragma comment(lib,"lib/liblzo/lzo.lib")
#elif !defined(__GNUC__)
#pragma comment(lib,"../../lib/liblzo/lzo.lib")
#endif

// alignment of file blocks - should be a multiple of the sector size of the disk and a power of 2
// HDD sector = 512 bytes, DVD/CD sector = 2048 bytes
#define ALIGN (512)

enum XPR_FLAGS
{
  XPRFLAG_PALETTE = 0x00000001,
  XPRFLAG_ANIM = 0x00000002
};

class CAutoBuffer
{
  BYTE* p;
public:
  CAutoBuffer() { p = 0; }
  explicit CAutoBuffer(size_t s) { p = (BYTE*)malloc(s); }
  ~CAutoBuffer() { free(p); }
operator BYTE*() { return p; }
  void Set(BYTE* buf) { free(p); p = buf; }
  bool Resize(size_t s);
void Release() { p = 0; }
};

bool CAutoBuffer::Resize(size_t s)
{
  if (s == 0)
  {
    if (!p)
      return false;
    free(p);
    p = 0;
    return true;
  }
  void* q = realloc(p, s);
  if (q)
  {
    p = (BYTE*)q;
    return true;
  }
  return false;
}

// as above but for texture allocation (do not change from XPhysicalAlloc!)
class CAutoTexBuffer
{
  BYTE* p;
public:
  CAutoTexBuffer() { p = 0; }
  explicit CAutoTexBuffer(size_t s) { p = (BYTE*)XPhysicalAlloc(s, MAXULONG_PTR, 128, PAGE_READWRITE); }
  ~CAutoTexBuffer() { if (p) XPhysicalFree(p); }
operator BYTE*() { return p; }
  BYTE* Set(BYTE* buf) { if (p) XPhysicalFree(p); return p = buf; }
void Release() { p = 0; }
};

CTextureBundle::CTextureBundle(void)
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_Ovl[0].hEvent = CreateEvent(0, TRUE, TRUE, 0);
  m_Ovl[1].hEvent = CreateEvent(0, TRUE, TRUE, 0);
  m_CurFileHeader[0] = m_FileHeaders.end();
  m_CurFileHeader[1] = m_FileHeaders.end();
  m_PreLoadBuffer[0] = 0;
  m_PreLoadBuffer[1] = 0;
  m_themeBundle = false;
  m_LoadIdx = 0;
  m_PreloadIdx = 0;
}

CTextureBundle::~CTextureBundle(void)
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
  if (m_PreLoadBuffer[0])
    free(m_PreLoadBuffer[0]);
  if (m_PreLoadBuffer[1])
    free(m_PreLoadBuffer[1]);
  CloseHandle(m_Ovl[0].hEvent);
  CloseHandle(m_Ovl[1].hEvent);
}

bool CTextureBundle::OpenBundle()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    Cleanup();

  CStdString strPath;

  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    CStdString themeXPR = CSettings::Get().GetString("lookandfeel.skintheme");
    if (!themeXPR.IsEmpty() && themeXPR.CompareNoCase("SKINDEFAULT"))
    {
      strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
      strPath = URIUtils::AddFileToFolder(strPath, themeXPR);
    }
    else
      return false;
  }
  else
    strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media/Textures.xpr");

  if (GetFileAttributes(strPath.c_str()) == -1)
    return false;

  m_TimeStamp.dwLowDateTime = m_TimeStamp.dwHighDateTime = 0;

#ifdef _XBOX
  if (ALIGN % XGetDiskSectorSize(strPath.Left(3).c_str()))
  {
    CLog::Log(LOGWARNING, "Disk sector size is not supported, caching textures.xpr");

    WIN32_FIND_DATA FindData[2];
    FindClose(FindFirstFile(strPath.c_str(), &FindData[0]));
    HANDLE hFind = FindFirstFile("Z:\\Textures.xpr", &FindData[1]);
    FindClose(hFind);

    if (hFind == INVALID_HANDLE_VALUE || FindData[0].nFileSizeLow != FindData[1].nFileSizeLow ||
        CompareFileTime(&FindData[0].ftLastWriteTime, &FindData[1].ftLastWriteTime))
    {
      SetFileAttributes("Z:\\Textures.xpr", FILE_ATTRIBUTE_NORMAL); //must set readable before overwriting
      if (!CopyFile(strPath, "Z:\\Textures.xpr", FALSE))
      {
        CLog::Log(LOGERROR, "Unable to open file: %s: %x", strPath.c_str(), GetLastError());
        return false;
      }
      m_hFile = CreateFile(strPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
      if (m_hFile != INVALID_HANDLE_VALUE)
      {
        GetFileTime(m_hFile, NULL, NULL, &m_TimeStamp);
        CloseHandle(m_hFile);
      }
    }
    strPath = "Z:\\Textures.xpr";
  }
#endif

  CAutoBuffer HeaderBuf(ALIGN);
  DWORD n;

  m_hFile = CreateFile(strPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, 0);
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGERROR, "Unable to open file: %s: %x", strPath.c_str(), GetLastError());
    return false;
  }

  if (m_TimeStamp.dwLowDateTime || m_TimeStamp.dwHighDateTime)
    SetFileTime(m_hFile, NULL, NULL, &m_TimeStamp);

  m_Ovl[0].Offset = 0;
  m_Ovl[0].OffsetHigh = 0;
  if (!ReadFile(m_hFile, HeaderBuf, ALIGN, &n, &m_Ovl[0]) && GetLastError() != ERROR_IO_PENDING)
    goto LoadError;
  if (!GetOverlappedResult(m_hFile, &m_Ovl[0], &n, TRUE) || n < ALIGN)
    goto LoadError;

  XPR_HEADER* pXPRHeader = (XPR_HEADER*)(BYTE*)HeaderBuf;
  int Version = (pXPRHeader->dwMagic >> 24) - '0';
  pXPRHeader->dwMagic -= Version << 24;
  Version &= 0x0f;

  if (pXPRHeader->dwMagic != XPR_MAGIC_VALUE || Version < 2)
    goto LoadError;

  DWORD HeaderSize = pXPRHeader->dwHeaderSize;
  DWORD AlignedSize = (HeaderSize - 1) & ~(ALIGN - 1); // align to sector, but remove the first sector
  HeaderBuf.Resize(AlignedSize + ALIGN);

  m_Ovl[0].Offset = ALIGN;
  if (!ReadFile(m_hFile, HeaderBuf + ALIGN, AlignedSize, &n, &m_Ovl[0]) && GetLastError() != ERROR_IO_PENDING)
    goto LoadError;
  if (!GetOverlappedResult(m_hFile, &m_Ovl[0], &n, TRUE) || n < AlignedSize)
    goto LoadError;

  struct DiskFileHeader_t
  {
    char Name[116];
    DWORD Offset;
    DWORD UnpackedSize;
    DWORD PackedSize;
  }
  *FileHeader;
  FileHeader = (DiskFileHeader_t*)(HeaderBuf + sizeof(XPR_HEADER));

  n = (HeaderSize - sizeof(XPR_HEADER)) / sizeof(DiskFileHeader_t);
  for (unsigned i = 0; i < n; ++i)
  {
    std::pair<CStdString, FileHeader_t> entry;
    entry.first = Normalize(FileHeader[i].Name);
    entry.second.Offset = FileHeader[i].Offset;
    entry.second.UnpackedSize = FileHeader[i].UnpackedSize;
    entry.second.PackedSize = FileHeader[i].PackedSize;
    m_FileHeaders.insert(entry);
  }
  m_CurFileHeader[0] = m_FileHeaders.end();
  m_CurFileHeader[1] = m_FileHeaders.end();
  m_PreloadIdx = m_LoadIdx = 0;

  GetFileTime(m_hFile, NULL, NULL, &m_TimeStamp);

  if (lzo_init() != LZO_E_OK)
    goto LoadError;

  return true;

LoadError:
  CLog::Log(LOGERROR, "Unable to load file: %s: %x", strPath.c_str(), GetLastError());
  CloseHandle(m_hFile); m_hFile = INVALID_HANDLE_VALUE;
  return false;
}

void CTextureBundle::Cleanup()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
  m_hFile = INVALID_HANDLE_VALUE;

  m_FileHeaders.clear();

  m_CurFileHeader[0] = m_FileHeaders.end();
  m_CurFileHeader[1] = m_FileHeaders.end();

  if (m_PreLoadBuffer[0])
    free(m_PreLoadBuffer[0]);
  m_PreLoadBuffer[0] = 0;
  if (m_PreLoadBuffer[1])
    free(m_PreLoadBuffer[1]);
  m_PreLoadBuffer[1] = 0;
}

bool CTextureBundle::HasFile(const CStdString& Filename)
{
  if (m_hFile == INVALID_HANDLE_VALUE && !OpenBundle())
    return false;

  FILETIME ts;
  GetFileTime(m_hFile, NULL, NULL, &ts);
  if (CompareFileTime(&m_TimeStamp, &ts))
  {
    CLog::Log(LOGINFO, "Texture bundle has changed, reloading");
    Cleanup();
    if (!OpenBundle())
      return false;
  }

  CStdString name = Normalize(Filename);
  return m_FileHeaders.find(name) != m_FileHeaders.end();
}

void CTextureBundle::GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures)
{
  if (path.GetLength() > 1 && path[1] == ':')
    return;

  if (m_hFile == INVALID_HANDLE_VALUE && !OpenBundle())
    return;

  CStdString testPath = Normalize(path);
  if (!URIUtils::HasSlashAtEnd(testPath))
    testPath += "\\";
  int testLength = testPath.GetLength();
  std::map<CStdString, FileHeader_t>::iterator it;
  for (it = m_FileHeaders.begin(); it != m_FileHeaders.end(); it++)
  {
    if (it->first.Left(testLength).Equals(testPath))
      textures.push_back(it->first);
  }
}

bool CTextureBundle::PreloadFile(const CStdString& Filename)
{
  CStdString name = Normalize(Filename);

  if (m_PreLoadBuffer[m_PreloadIdx])
    free(m_PreLoadBuffer[m_PreloadIdx]);
  m_PreLoadBuffer[m_PreloadIdx] = 0;

  m_CurFileHeader[m_PreloadIdx] = m_FileHeaders.find(name);
  if (m_CurFileHeader[m_PreloadIdx] != m_FileHeaders.end())
  {
    if (!HasOverlappedIoCompleted(&m_Ovl[m_PreloadIdx]))
    {
      bool FlushBuf = !HasOverlappedIoCompleted(&m_Ovl[1 - m_PreloadIdx]);
      CancelIo(m_hFile);
      if (FlushBuf)
      {
        free(m_PreLoadBuffer[1 - m_PreloadIdx]);
        m_PreLoadBuffer[1 - m_PreloadIdx] = 0;
        m_CurFileHeader[1 - m_PreloadIdx] = m_FileHeaders.end();
      }
    }

    // preload texture
    DWORD ReadSize = (m_CurFileHeader[m_PreloadIdx]->second.PackedSize + (ALIGN - 1)) & ~(ALIGN - 1);
    m_PreLoadBuffer[m_PreloadIdx] = (BYTE*)malloc(ReadSize);

    if (m_PreLoadBuffer[m_PreloadIdx])
    {
      m_Ovl[m_PreloadIdx].Offset = m_CurFileHeader[m_PreloadIdx]->second.Offset;
      m_Ovl[m_PreloadIdx].OffsetHigh = 0;

      DWORD n;
      if (!ReadFile(m_hFile, m_PreLoadBuffer[m_PreloadIdx], ReadSize, &n, &m_Ovl[m_PreloadIdx]) && GetLastError() != ERROR_IO_PENDING)
      {
        CLog::Log(LOGERROR, "Error loading texture: %s: %x", Filename.c_str(), GetLastError());
        free(m_PreLoadBuffer[m_PreloadIdx]);
        m_PreLoadBuffer[m_PreloadIdx] = 0;
        m_CurFileHeader[m_PreloadIdx] = m_FileHeaders.end();
        return false;
      }

      m_PreloadIdx = 1 - m_PreloadIdx;
      return true;
    }
    else
    {
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %lu bytes, have %lu bytes)", name.c_str(), ReadSize, stat.dwAvailPhys);
    }
  }
  return false;
}

bool CTextureBundle::LoadFile(const CStdString& Filename, CAutoTexBuffer& UnpackedBuf)
{
  CStdString name = Normalize(Filename);
  if (m_CurFileHeader[0] != m_FileHeaders.end() && m_CurFileHeader[0]->first == name)
    m_LoadIdx = 0;
  else if (m_CurFileHeader[1] != m_FileHeaders.end() && m_CurFileHeader[1]->first == name)
    m_LoadIdx = 1;
  else
  {
    m_LoadIdx = m_PreloadIdx;
    if (!PreloadFile(Filename))
      return false;
  }

  if (!m_PreLoadBuffer[m_LoadIdx])
    return false;
  if (!UnpackedBuf.Set((BYTE*)XPhysicalAlloc(m_CurFileHeader[m_LoadIdx]->second.UnpackedSize, MAXULONG_PTR, 128, PAGE_READWRITE)))
  {
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %lu bytes, have %lu bytes)", name.c_str(),
              m_CurFileHeader[m_LoadIdx]->second.UnpackedSize, stat.dwAvailPhys);
    return false;
  }

  DWORD n;
  if (!GetOverlappedResult(m_hFile, &m_Ovl[m_LoadIdx], &n, TRUE) || n < m_CurFileHeader[m_LoadIdx]->second.PackedSize)
  {
    CLog::Log(LOGERROR, "Error loading texture: %s: %x", Filename.c_str(), GetLastError());
    return false;
  }

  lzo_uint s = m_CurFileHeader[m_LoadIdx]->second.UnpackedSize;
  bool success = true;
  if (lzo1x_decompress(m_PreLoadBuffer[m_LoadIdx], m_CurFileHeader[m_LoadIdx]->second.PackedSize, UnpackedBuf, &s, NULL) != LZO_E_OK ||
      s != m_CurFileHeader[m_LoadIdx]->second.UnpackedSize)
  {
    CLog::Log(LOGERROR, "Error loading texture: %s: Decompression error", Filename.c_str());
    success = false;
  }

  try
  {
    free(m_PreLoadBuffer[m_LoadIdx]);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error freeing preload buffer.");
  }

  m_PreLoadBuffer[m_LoadIdx] = 0;
  m_CurFileHeader[m_LoadIdx] = m_FileHeaders.end();

  // switch on writecombine on memory and flush the cache for the gpu
  // it's about 3 times faster to load in cached ram then do this than to load in wc ram. :)
  if (success)
  {
#ifdef _XBOX
    // this causes xbmc to crash when swtiching back to gui from pal60, not really needed anyway as nothing should be writing to texture ram.
    //XPhysicalProtect(UnpackedBuf, m_CurFileHeader[m_LoadIdx]->second.UnpackedSize, PAGE_READWRITE | PAGE_WRITECOMBINE);

    __asm {
      wbinvd
    }
#endif
  }

  return success;
}

bool CTextureBundle::LoadTexture(const CStdString& Filename, CBaseTexture** ppTexture, int &width, int &height)
{
  DWORD ResDataOffset;
  *ppTexture = NULL;

  CAutoTexBuffer UnpackedBuf;
  if (!LoadFile(Filename, UnpackedBuf))
    return false;

  D3DTexture* pTex = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]); // Why we are appending sizeof(DWORD) here? It's not like that on Kodi
  D3DPalette* pPal = 0;
  void* ResData = 0;

  WORD RealSize[2];

  enum XPR_FLAGS
  {
    XPRFLAG_PALETTE = 0x00000001,
    XPRFLAG_ANIM = 0x00000002
  };

  BYTE* Next = UnpackedBuf;

  DWORD flags = *(DWORD*)Next;
  Next += sizeof(DWORD);
  if ((flags & XPRFLAG_ANIM) || (flags >> 16) > 1)
    goto PackedLoadError;

  if (flags & XPRFLAG_PALETTE)
  {
    pPal = new D3DPalette;
    memcpy(pPal, Next, sizeof(D3DPalette));
    Next += sizeof(D3DPalette);
  }

  memcpy(pTex, Next, sizeof(D3DTexture));
  Next += sizeof(D3DTexture);

  memcpy(RealSize, Next, 4);
  Next += 4;

  ResDataOffset = ((Next - UnpackedBuf) + 127) & ~127;
  ResData = UnpackedBuf + ResDataOffset;

  if ((pTex->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
    goto PackedLoadError;

#ifdef HAS_XBOX_D3D
  *ppTexture = new CTexture(RealSize[0], RealSize[1], XB_FMT_A8R8G8B8, (LPDIRECT3DTEXTURE8)pTex, NULL, true);
  (*ppTexture)->GetTextureObject()->Register(ResData);
#else
  GetTextureFromData(pTex, ResData, ppTexture);
  delete[] pTex;
#endif
#ifdef HAS_XBOX_D3D
  *(DWORD*)(pTex + 1) = (DWORD)(BYTE*)UnpackedBuf;
  if (pPal)
  {
    (*ppTexture)->SetPaletteObject((LPDIRECT3DPALETTE8)pPal);
    (*ppTexture)->GetPaletteObject()->Register(ResData);
  }
  UnpackedBuf.Release();
#endif

  width = RealSize[0];
  height = RealSize[1];
/* DXMERGE - this was previously used to specify the format of the image - probably only affects directx?
#ifndef HAS_SDL
  D3DSURFACE_DESC desc;
  (*ppTexture)->GetLevelDesc(0, &desc);
  pInfo->Format = desc.Format;
#endif
*/
  return true;

PackedLoadError:
  CLog::Log(LOGERROR, "Error loading texture: %s: Invalid data", Filename.c_str());
  delete [] pTex;
  if (pPal) delete pPal;
  return false;
}
int CTextureBundle::LoadAnim(const CStdString& Filename, CBaseTexture*** ppTextures,
                              int &width, int &height, int& nLoops, int** ppDelays)
{
  DWORD ResDataOffset;
  int nTextures = 0;

  *ppTextures = NULL; *ppDelays = NULL;

  CAutoTexBuffer UnpackedBuf;
  if (!LoadFile(Filename, UnpackedBuf))
    return 0;

  struct AnimInfo_t
  {
    DWORD nLoops;
    WORD RealSize[2];
  }
  *pAnimInfo;

  D3DTexture** ppTex = 0;
  D3DPalette* pPal = 0;
  void* ResData = 0;

  BYTE* Next = UnpackedBuf;

  DWORD flags = *(DWORD*)Next;
  Next += sizeof(DWORD);
  if (!(flags & XPRFLAG_ANIM))
    goto PackedAnimError;

  pAnimInfo = (AnimInfo_t*)Next;
  Next += sizeof(AnimInfo_t);
  nLoops = pAnimInfo->nLoops;

  if (flags & XPRFLAG_PALETTE)
  {
    pPal = new D3DPalette;
    memcpy(pPal, Next, sizeof(D3DPalette));
    Next += sizeof(D3DPalette);
  }

  nTextures = flags >> 16;
  ppTex = new D3DTexture * [nTextures];
  *ppDelays = new int[nTextures];
  for (int i = 0; i < nTextures; ++i)
  {
    ppTex[i] = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]);
    memcpy(ppTex[i], Next, sizeof(D3DTexture));
    Next += sizeof(D3DTexture);

    (*ppDelays)[i] = *(int*)Next;
    Next += sizeof(int);
  }

  ResDataOffset = ((DWORD)(Next - UnpackedBuf) + 127) & ~127;
  ResData = UnpackedBuf + ResDataOffset;

  *ppTextures = new CBaseTexture*[nTextures];
  for (int i = 0; i < nTextures; ++i)
  {
    if ((ppTex[i]->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
      goto PackedAnimError;

#ifdef HAS_XBOX_D3D
    (*ppTextures)[i] = new CTexture(pAnimInfo->RealSize[0], pAnimInfo->RealSize[1], XB_FMT_A8R8G8B8, (LPDIRECT3DTEXTURE8)ppTex[i], NULL, true);
    (*ppTextures)[i]->GetTextureObject()->Register(ResData);
#else
    GetTextureFromData(ppTex[i], ResData, &(*ppTextures)[i]);
#endif
    *(DWORD*)(ppTex[i] + 1) = 0;
  }
  *(DWORD*)(ppTex[0] + 1) = (DWORD)(BYTE*)UnpackedBuf;

  delete [] ppTex;
  ppTex = 0;

#ifdef HAS_XBOX_D3D
  if (pPal)
  {
    (*ppTextures)[0]->SetPaletteObject((LPDIRECT3DPALETTE8)pPal);
    (*ppTextures)[0]->GetPaletteObject()->Register(ResData);
  }
  UnpackedBuf.Release();
#endif

  width = pAnimInfo->RealSize[0];
  height = pAnimInfo->RealSize[1];

  return nTextures;

PackedAnimError:
  CLog::Log(LOGERROR, "Error loading texture: %s: Invalid data", Filename.c_str());
  if (ppTex)
  {
    for (int i = 0; i < nTextures; ++i)
      delete [] ppTex[i];
    delete [] ppTex;
  }
  if (pPal) delete pPal;
  if (*ppDelays) delete [] *ppDelays;
  return 0;
}

void CTextureBundle::SetThemeBundle(bool themeBundle)
{
  m_themeBundle = themeBundle;
}

// normalize to how it's stored within the bundle
// lower case + using \\ rather than /
CStdString CTextureBundle::Normalize(const CStdString &name)
{
  CStdString newName(name);
  newName.Normalize();
  newName.Replace('/','\\');
  return newName;
}
