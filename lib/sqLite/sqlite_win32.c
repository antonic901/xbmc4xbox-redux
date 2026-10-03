/*
 *  Copyright (C) Nikola Antonic, Ryzee119
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// This implementation is based on LithiumX version.
// Locking and other missing implementation are taken from XBMC4Xbox's SQLite3 (os_win.c)
// https://github.com/Ryzee119/LithiumX/blob/master/src/platform/win32/sqlite_win32.c
// https://github.com/antonic901/xbmc4xbox-redux/blob/master/docs/libSQLite3.rar

#include "sqlite3.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <xtl.h>

#include "lockfile.h"

#define UNUSED_PARAMETER(x) (void)(x)

#define PENDING_BYTE 0x40000000 /* First byte past the 1GB boundary */

#define RESERVED_BYTE (PENDING_BYTE + 1)
#define SHARED_FIRST (PENDING_BYTE + 2)
#define SHARED_SIZE 510

/*
** Some microsoft compilers lack this definition.
*/
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) - 1)
#endif

typedef struct _winFile
{
  sqlite3_file base;
  HANDLE h;
  unsigned char locktype; /* Type of lock currently held on this file */
  unsigned char isOpen; /* True if needs to be closed */
  short sharedLockByte; /* Randomly chosen byte used as a shared lock */
  DWORD lastErrno; /* The Windows errno from the last I/O error */
} winFile;

#define sql_DbgPrint (void)

/*
** Undo a readlock
*/
static int unlockReadLock(sqlite3_file* id)
{
  winFile* f = (winFile*)id;
  return UnlockFile(f->h, SHARED_FIRST + f->sharedLockByte, 0, 1, 0);
}

/*
** Acquire a reader lock.
*/
static int getReadLock(sqlite3_file* id)
{
  winFile* f = (winFile*)id;

  int lk;
  sqlite3_randomness(sizeof(lk), &lk);
  f->sharedLockByte = (lk & 0x7fffffff) % (SHARED_SIZE - 1);
  return LockFile(f->h, SHARED_FIRST + f->sharedLockByte, 0, 1, 0);
}

static int winClose(sqlite3_file* id)
{
  winFile* f = (winFile*)id;
  if (f->isOpen)
  {
    RemoveLockMap(f->h);
    CloseHandle(f->h);
    f->isOpen = 0;
  }

  return SQLITE_OK;
}

static int winRead(sqlite3_file* id, void* pBuf, int amt, sqlite3_int64 offset)
{
  DWORD read;
  LARGE_INTEGER loffset;
  winFile* f = (winFile*)id;
  assert(f->isOpen);

  loffset.QuadPart = offset;

  if (SetFilePointerEx(f->h, loffset, NULL, FILE_BEGIN) == 0)
  {
    return SQLITE_IOERR_SEEK;
  }

  if (ReadFile(f->h, pBuf, (DWORD)amt, &read, NULL))
  {
    return (read != amt) ? SQLITE_IOERR_SHORT_READ : SQLITE_OK;
  }
  else
  {
    return SQLITE_IOERR_READ;
  }
}

static int winWrite(sqlite3_file* id, const void* pBuf, int amt, sqlite3_int64 offset)
{
  DWORD written;
  LARGE_INTEGER loffset;

  winFile* f = (winFile*)id;
  assert(f->isOpen);
  assert(amt > 0);

  loffset.QuadPart = offset;

  if (SetFilePointerEx(f->h, loffset, NULL, FILE_BEGIN) == 0)
  {
    return SQLITE_IOERR_SEEK;
  }

  if (WriteFile(f->h, pBuf, (DWORD)amt, &written, NULL))
  {
    return SQLITE_OK;
  }
  else
  {
    return SQLITE_IOERR_WRITE;
  }
}

static int winTruncate(sqlite3_file* id, sqlite3_int64 nByte)
{
  LONG upperBits = nByte >> 32;

  winFile* f = (winFile*)id;
  assert(f->isOpen);

  SetFilePointer(f->h, nByte, &upperBits, FILE_BEGIN);
  SetEndOfFile(f->h);

  return SQLITE_OK;
}

static int winSync(sqlite3_file* id, int flags)
{
  winFile* pFile = (winFile*)id;
  UNUSED_PARAMETER(flags);
  if (FlushFileBuffers(pFile->h))
  {
    return SQLITE_OK;
  }
  else
  {
    pFile->lastErrno = GetLastError();
    return SQLITE_IOERR;
  }
}

static int winFileSize(sqlite3_file* id, sqlite3_int64* pSize)
{
  DWORD upperBits;
  DWORD lowerBits;
  winFile* f;

  f = (winFile*)id;
  assert(f->isOpen);

  lowerBits = GetFileSize(f->h, &upperBits);
  *pSize = (((sqlite_int64)upperBits) << 32) + lowerBits;

  return SQLITE_OK;
}

static int winLock(sqlite3_file* id, int locktype)
{
  int rc = SQLITE_OK; /* Return code from subroutines */
  int res = 1; /* Result of a windows lock call */
  int newLocktype; /* Set id->locktype to this value before exiting */
  int gotPendingLock = 0; /* True if we acquired a PENDING lock this time */

  winFile* f = (winFile*)id;
  assert(f->isOpen);

  /* If there is already a lock of this type or more restrictive on the
    ** OsFile, do nothing. Don't use the end_lock: exit path, as
    ** sqlite3OsEnterMutex() hasn't been called yet.
    */
  if (f->locktype >= locktype)
  {
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
    */
  assert(f->locktype != SQLITE_LOCK_NONE || locktype == SQLITE_LOCK_SHARED);
  assert(locktype != SQLITE_LOCK_PENDING);
  assert(locktype != SQLITE_LOCK_RESERVED || f->locktype == SQLITE_LOCK_SHARED);

  /* Lock the SQLITE_LOCK_PENDING byte if we need to acquire a PENDING lock or
    ** a SHARED lock.  If we are acquiring a SHARED lock, the acquisition of
    ** the SQLITE_LOCK_PENDING byte is temporary.
    */
  newLocktype = f->locktype;
  if (f->locktype == SQLITE_LOCK_NONE ||
      (locktype == SQLITE_LOCK_EXCLUSIVE && f->locktype == SQLITE_LOCK_RESERVED))
  {
    int cnt = 3;
    while (cnt-- > 0 && (res = LockFile(f->h, PENDING_BYTE, 0, 1, 0)) == 0)
    {
      /* Try 3 times to get the pending lock.  The pending lock might be
            ** held by another reader process who will release it momentarily.
            */
      Sleep(1);
    }
    gotPendingLock = res;
  }

  /* Acquire a shared lock
    */
  if (locktype == SQLITE_LOCK_SHARED && res)
  {
    assert(f->locktype == SQLITE_LOCK_NONE);
    res = getReadLock(id);
    if (res)
    {
      newLocktype = SQLITE_LOCK_SHARED;
    }
  }

  /* Acquire a RESERVED lock
    */
  if (locktype == SQLITE_LOCK_RESERVED && res)
  {
    assert(f->locktype == SQLITE_LOCK_SHARED);
    res = LockFile(f->h, RESERVED_BYTE, 0, 1, 0);
    if (res)
    {
      newLocktype = SQLITE_LOCK_RESERVED;
    }
  }

  /* Acquire a PENDING lock
    */
  if (locktype == SQLITE_LOCK_EXCLUSIVE && res)
  {
    newLocktype = SQLITE_LOCK_PENDING;
    gotPendingLock = 0;
  }

  /* Acquire an EXCLUSIVE lock
    */
  if (locktype == SQLITE_LOCK_EXCLUSIVE && res)
  {
    assert(f->locktype >= SQLITE_LOCK_SHARED);
    res = unlockReadLock(id);
    res = LockFile(f->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if (res)
    {
      newLocktype = SQLITE_LOCK_EXCLUSIVE;
    }
    else
    {
      // ERROR
    }
  }

  /* If we are holding a PENDING lock that ought to be released, then
    ** release it now.
    */
  if (gotPendingLock && locktype == SQLITE_LOCK_SHARED)
  {
    UnlockFile(f->h, PENDING_BYTE, 0, 1, 0);
  }

  /* Update the state of the lock has held in the file descriptor then
    ** return the appropriate result code.
    */
  if (res)
  {
    rc = SQLITE_OK;
  }
  else
  {
    rc = SQLITE_BUSY;
  }
  f->locktype = newLocktype;
  return rc;
}

static int winUnlock(sqlite3_file* id, int locktype)
{
  int type;
  int rc = SQLITE_OK;

  winFile* f = (winFile*)id;
  assert(f->isOpen);
  assert(f->isOpen);
  assert(locktype <= SQLITE_LOCK_SHARED);

  type = f->locktype;
  if (type >= SQLITE_LOCK_EXCLUSIVE)
  {
    UnlockFile(f->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if (locktype == SQLITE_LOCK_SHARED && !getReadLock(id))
    {
      /* This should never happen.  We should always be able to
            ** reacquire the read lock */
      rc = SQLITE_IOERR;
    }
  }
  if (type >= SQLITE_LOCK_RESERVED)
  {
    UnlockFile(f->h, RESERVED_BYTE, 0, 1, 0);
  }
  if (locktype == SQLITE_LOCK_NONE && type >= SQLITE_LOCK_SHARED)
  {
    unlockReadLock(id);
  }
  if (type >= SQLITE_LOCK_PENDING)
  {
    UnlockFile(f->h, PENDING_BYTE, 0, 1, 0);
  }
  f->locktype = locktype;

  return rc;
}

static int winCheckReservedLock(sqlite3_file* id, int* pResOut)
{
  int rc;

  winFile* f = (winFile*)id;
  assert(f->isOpen);

  if (f->locktype >= SQLITE_LOCK_RESERVED)
  {
    rc = 1;
  }
  else
  {
    rc = LockFile(f->h, RESERVED_BYTE, 0, 1, 0);
    if (rc)
    {
      UnlockFile(f->h, RESERVED_BYTE, 0, 1, 0);
    }
    rc = !rc;
  }
  return rc;
}

static int winFileControl(sqlite3_file* id, int op, void* pArg)
{
  UNUSED_PARAMETER(id);
  UNUSED_PARAMETER(op);
  UNUSED_PARAMETER(pArg);
  return SQLITE_NOTFOUND;
}

static int winSectorSize(sqlite3_file* id)
{
  UNUSED_PARAMETER(id);
  return 4096;
}

static int winDeviceCharacteristics(sqlite3_file* id)
{
  UNUSED_PARAMETER(id);
  return SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN;
}

static const sqlite3_io_methods nxdk_io = {
    1, /* iVersion */
    winClose, /* xClose */
    winRead, /* xRead */
    winWrite, /* xWrite */
    winTruncate, /* xTruncate */
    winSync, /* xSync */
    winFileSize, /* xFileSize */
    winLock, /* xLock */
    winUnlock, /* xUnlock */
    winCheckReservedLock, /* xCheckReservedLock */
    winFileControl, /* xFileControl */
    winSectorSize, /* xSectorSize */
    winDeviceCharacteristics, /* xDeviceCharacteristics */
};

static DWORD sqlite_to_win_access(int sql_flags)
{
  DWORD attrib = 0;
  if (sql_flags & SQLITE_OPEN_READONLY)
  {
    attrib |= GENERIC_READ;
  }
  if (sql_flags & SQLITE_OPEN_READWRITE)
  {
    attrib |= GENERIC_READ | GENERIC_WRITE;
  }
  return attrib;
}

static DWORD sqlite_to_win_attr(int sql_flags)
{
  DWORD attrib = 0;
  if (sql_flags & SQLITE_OPEN_CREATE)
  {
    attrib |= OPEN_ALWAYS;
  }
  else
  {
    attrib |= OPEN_EXISTING;
  }
  return attrib;
}

static int winOpen(
    sqlite3_vfs* pVfs, const char* zFilename, sqlite3_file* id, int flags, int* pOutFlags)
{
  winFile* f;
  UNUSED_PARAMETER(pVfs);

  f = (winFile*)id;
  assert(!f->isOpen);

  f->base.pMethods = &nxdk_io;
  f->h = CreateFileA(zFilename, sqlite_to_win_access(flags), FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, sqlite_to_win_attr(flags), FILE_ATTRIBUTE_NORMAL, NULL);
  if (f->h == INVALID_HANDLE_VALUE)
  {
    return SQLITE_CANTOPEN;
  }

  if (pOutFlags)
  {
    *pOutFlags = flags;
  }

  AddLockMap(zFilename, f->h);
  f->locktype = SQLITE_LOCK_NONE;
  f->sharedLockByte = 0;
  f->isOpen = 1;

  return SQLITE_OK;
}

static int winDelete(sqlite3_vfs* pVfs, const char* zFilename, int syncDir)
{
  UNUSED_PARAMETER(pVfs);
  UNUSED_PARAMETER(syncDir);
  if (DeleteFileA(zFilename))
  {
    return SQLITE_OK;
  }

  return SQLITE_IOERR_DELETE_NOENT;
}

static int winAccess(sqlite3_vfs* pVfs, const char* zFilename, int flags, int* pResOut)
{
  DWORD attr;
  int rc = 0;
  UNUSED_PARAMETER(pVfs);
  attr = GetFileAttributesA(zFilename);
  switch (flags)
  {
    case SQLITE_ACCESS_READ:
    case SQLITE_ACCESS_EXISTS:
      rc = attr != INVALID_FILE_ATTRIBUTES;
      break;
    case SQLITE_ACCESS_READWRITE:
      rc = (attr & FILE_ATTRIBUTE_READONLY) == 0;
      break;
    default:
      assert(!"Invalid flags argument");
  }
  *pResOut = rc;
  return SQLITE_OK;
}

static int winFullPathname(sqlite3_vfs* pVfs, const char* zRelative, int nFull, char* zFull)
{
  int i;
  for (i = 0; i < nFull; i++)
  {
    zFull[i] = zRelative[i];
    if (zRelative[i] == '\0')
    {
      break;
    }
  }
  zFull[nFull - 1] = '\0';
  return SQLITE_OK;
}

static void* winDlOpen(sqlite3_vfs* pVfs, const char* zFilename)
{
  UNUSED_PARAMETER(pVfs);
  UNUSED_PARAMETER(zFilename);
  return NULL;
}

static void winDlError(sqlite3_vfs* pVfs, int nBuf, char* zBufOut)
{
  UNUSED_PARAMETER(pVfs);
  sqlite3_snprintf(nBuf, zBufOut, "Loadable extensions are not supported");
  zBufOut[nBuf - 1] = '\0';
}

static void (*winDlSym(sqlite3_vfs* pVfs, void* pH, const char* zSym))(void)
{
  UNUSED_PARAMETER(pVfs);
  UNUSED_PARAMETER(pH);
  UNUSED_PARAMETER(zSym);
  return 0;
}

static void winDlClose(sqlite3_vfs* pVfs, void* pHandle)
{
  UNUSED_PARAMETER(pVfs);
  UNUSED_PARAMETER(pHandle);
  return;
}

static int winRandomness(sqlite3_vfs* pVfs, int nBuf, char* zBuf)
{
  int i;
  UNUSED_PARAMETER(pVfs);
  for (i = 0; i < nBuf; i++)
  {
    zBuf[i] = rand() % 0xFF;
  }
  return nBuf;
}

static int winSleep(sqlite3_vfs* pVfs, int microsec)
{
  UNUSED_PARAMETER(pVfs);
  Sleep((microsec + 999) / 1000);
  return microsec;
}

/* Return number of milliseconds since the Julian epoch of noon in Greenwich on
 * November 24, 4714 B.C according to the proleptic Gregorian calendar.
 */
static int winCurrentTimeInt64(sqlite3_vfs* pVfs, sqlite3_int64* piNow)
{
  /* FILETIME structure is a 64-bit value representing the number of
     100-nanosecond intervals since January 1, 1601 (= JD 2305813.5).
  */
  FILETIME ft;
  static const sqlite3_int64 winFiletimeEpoch = 23058135 * (sqlite3_int64)8640000;
  /* 2^32 - to avoid use of LL and warnings in gcc */
  static const sqlite3_int64 max32BitValue =
      (sqlite3_int64)2000000000 + (sqlite3_int64)2000000000 + (sqlite3_int64)294967296;

  GetSystemTimeAsFileTime(&ft);

  *piNow = winFiletimeEpoch + ((((sqlite3_int64)ft.dwHighDateTime) * max32BitValue) +
                               (sqlite3_int64)ft.dwLowDateTime) /
                                  (sqlite3_int64)10000;

  UNUSED_PARAMETER(pVfs);
  return SQLITE_OK;
}

static int winCurrentTime(sqlite3_vfs* pVfs, double* prNow)
{
  sqlite3_int64 piNow;

  UNUSED_PARAMETER(pVfs);

  if (winCurrentTimeInt64(pVfs, &piNow) == SQLITE_OK)
  {
    piNow /= 86400000.0;
    *prNow = (double)piNow;
    return SQLITE_OK;
  }
  return SQLITE_ERROR;
}

static int winGetLastError(sqlite3_vfs* pVfs, int nBuf, char* zBuf)
{
  UNUSED_PARAMETER(pVfs);

  if (nBuf > 0)
  {
    zBuf[0] = '\0';
  }
  return GetLastError();
}

static sqlite3_vfs win32_vfs = {
    1, /* iVersion */
    sizeof(winFile), /* szOsFile */
    MAX_PATH, /* mxPathname */
    NULL, /* pNext */
    "xodi", /* zName */
    NULL, /* pAppData */
    winOpen, /* xOpen */
    winDelete, /* xDelete */
    winAccess, /* xAccess */
    winFullPathname, /* xFullPathname */
    winDlOpen, /* xDlOpen */
    winDlError, /* xDlError */
    winDlSym, /* xDlSym */
    winDlClose, /* xDlClose */
    winRandomness, /* xRandomness */
    winSleep, /* xSleep */
    winCurrentTime, /* xCurrentTime */
    winGetLastError, /* xGetLastError */
    winCurrentTimeInt64, /* xCurrentTimeInt64 */
    NULL, /* xSetSystemCall */
    NULL, /* xGetSystemCall */
    NULL, /* xNextSystemCall */
};

#if (0)
static int winMutexInit(void)
{
  return SQLITE_OK;
}

static int winMutexEnd(void)
{
  return SQLITE_OK;
}

static sqlite3_mutex* winMutexAlloc(int id)
{
  HANDLE mutex = CreateMutex(NULL, FALSE, NULL);
  return (sqlite3_mutex*)mutex;
}

static void(winMutexFree)(sqlite3_mutex* mutex)
{
  CloseHandle((HANDLE)mutex);
}

static void winMutexEnter(sqlite3_mutex* mutex)
{
  WaitForSingleObject((HANDLE)mutex, INFINITE);
}

static int winMutexTry(sqlite3_mutex* mutex)
{
  DWORD res = WaitForSingleObject((HANDLE)mutex, 0);
  if (res == WAIT_OBJECT_0 || res == WAIT_ABANDONED)
  {
    return SQLITE_OK;
  }
  return SQLITE_BUSY;
}

static void winMutexLeave(sqlite3_mutex* mutex)
{
  ReleaseMutex((HANDLE)mutex);
}

static sqlite3_mutex_methods win32_mutex_methods = {.xMutexInit = winMutexInit,
                                                    .xMutexEnd = winMutexEnd,
                                                    .xMutexAlloc = winMutexAlloc,
                                                    .xMutexFree = winMutexFree,
                                                    .xMutexEnter = winMutexEnter,
                                                    .xMutexTry = winMutexTry,
                                                    .xMutexLeave = winMutexLeave,
                                                    .xMutexHeld = NULL,
                                                    .xMutexNotheld = NULL};

// Needs to be called before sqlite3_initialize()
void sqlite3_register_win32_mutex()
{
  sqlite3_config(SQLITE_CONFIG_MUTEX, &win32_mutex_methods);
}

#endif

sqlite3_vfs* sqlite_xbox_fs(void)
{
  return &win32_vfs;
}

int sqlite3_os_init(void)
{
  sqlite3_vfs_register(sqlite_xbox_fs(), 1);
  return SQLITE_OK;
}

int sqlite3_os_end(void)
{
  return SQLITE_OK;
}
