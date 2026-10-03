#include <xtl.h>

#include <map>
#include <set>
#include <string>
#include <vector>

struct lockregion
{
  DWORD m_LockingThread;
  DWORD dwFileOffsetLow;
  DWORD dwFileOffsetHigh;
  DWORD nNumberOfBytesToLockLow;
  DWORD nNumberOfBytesToLockHigh;
};

struct stFileLock
{
  std::set<HANDLE> m_HandleSet;
  std::vector<lockregion> m_lockregions;
};

std::map<std::string, stFileLock*> lockmap;

CRITICAL_SECTION g_cs;
bool g_bCsInited = false;

#define LOCKFILE_FAIL_IMMEDIATELY 0

#ifdef __cplusplus
extern "C"
{
#endif

  void AddLockMap(const char* zFileName, HANDLE hFile)
  {
    if (!g_bCsInited)
    {
      InitializeCriticalSection(&g_cs);
      g_bCsInited = true;
    }

    EnterCriticalSection(&g_cs);

    std::string strFileName(zFileName);
    std::map<std::string, stFileLock*>::iterator it = lockmap.find(strFileName);
    if (it == lockmap.end())
    {
      stFileLock* filelock = new stFileLock;
      filelock->m_HandleSet.insert(hFile);
      lockmap.insert(std::pair<std::string, stFileLock*>(strFileName, filelock));
    }
    else
    {
      stFileLock* filelock = it->second;
      filelock->m_HandleSet.insert(hFile);
    }

    LeaveCriticalSection(&g_cs);
  }

  void RemoveLockMap(HANDLE hFile)
  {
    if (!g_bCsInited)
    {
      InitializeCriticalSection(&g_cs);
      g_bCsInited = true;
    }

    EnterCriticalSection(&g_cs);

    for (std::map<std::string, stFileLock*>::iterator it = lockmap.begin(); it != lockmap.end(); it++)
    {
      stFileLock* filelock = it->second;
      if (filelock->m_HandleSet.find(hFile) != filelock->m_HandleSet.end())
      {
        filelock->m_HandleSet.erase(hFile);
        if (filelock->m_HandleSet.size() == 0)
        {
          delete filelock;
          lockmap.erase(it);
          break;
        }
      }
    }

    LeaveCriticalSection(&g_cs);
  }

  BOOL LockFileEx(HANDLE hFile,
                  DWORD dwFlags,
                  DWORD dwReserved,
                  DWORD nNumberOfBytesToLockLow,
                  DWORD nNumberOfBytesToLockHigh,
                  LPOVERLAPPED lpOverlapped)
  {
    // Function is unused if isNT() returns false
    return FALSE;
  }

  BOOL LockFile(HANDLE hFile,
                DWORD dwFileOffsetLow,
                DWORD dwFileOffsetHigh,
                DWORD nNumberOfBytesToLockLow,
                DWORD nNumberOfBytesToLockHigh)
  {
    if (!g_bCsInited)
    {
      InitializeCriticalSection(&g_cs);
      g_bCsInited = true;
    }

    EnterCriticalSection(&g_cs);

    for (std::map<std::string, stFileLock*>::iterator it = lockmap.begin(); it != lockmap.end(); it++)
    {
      stFileLock* filelock = it->second;
      if (filelock->m_HandleSet.find(hFile) != filelock->m_HandleSet.end())
      {
        DWORD dwThread = GetCurrentThreadId();
        for (int i = 0; i < (int)filelock->m_lockregions.size(); i++)
        {
          lockregion region = filelock->m_lockregions[i];
          if (region->dwFileOffsetLow == dwFileOffsetLow &&
              region->dwFileOffsetHigh == dwFileOffsetHigh &&
              region->nNumberOfBytesToLockLow == nNumberOfBytesToLockLow &&
              region->nNumberOfBytesToLockHigh == nNumberOfBytesToLockHigh)
          {
            if (region->m_LockingThread == dwThread)
            {
              LeaveCriticalSection(&g_cs);
              return TRUE;
            }
            else
            {
              LeaveCriticalSection(&g_cs);
              return FALSE;
            }
          }
        }

        lockregion region;
        region->dwFileOffsetLow = dwFileOffsetLow;
        region->dwFileOffsetHigh = dwFileOffsetHigh;
        region->nNumberOfBytesToLockLow = nNumberOfBytesToLockLow;
        region->nNumberOfBytesToLockHigh = nNumberOfBytesToLockHigh;
        region->m_LockingThread = dwThread;
        filelock->m_lockregions.push_back(region);
        LeaveCriticalSection(&g_cs);
        return TRUE;
      }
    }

    assert(false);
    LeaveCriticalSection(&g_cs);
    return FALSE;
  }

  BOOL UnlockFile(HANDLE hFile,
                  DWORD dwFileOffsetLow,
                  DWORD dwFileOffsetHigh,
                  DWORD nNumberOfBytesToUnlockLow,
                  DWORD nNumberOfBytesToUnlockHigh)
  {
    if (!g_bCsInited)
    {
      InitializeCriticalSection(&g_cs);
      g_bCsInited = true;
    }
    EnterCriticalSection(&g_cs);

    for (std::map<std::string, stFileLock*>::iterator it = lockmap.begin(); it != lockmap.end(); it++)
    {
      stFileLock* filelock = it->second;
      if (filelock->m_HandleSet.find(hFile) != filelock->m_HandleSet.end())
      {
        DWORD dwThread = GetCurrentThreadId();
        for (std::vector<lockregion>::iterator region = filelock->m_lockregions.begin(); region != filelock->m_lockregions.end(); region++)
        {
          if (region->dwFileOffsetLow == dwFileOffsetLow &&
              region->dwFileOffsetHigh == dwFileOffsetHigh &&
              region->nNumberOfBytesToLockLow == nNumberOfBytesToUnlockLow &&
              region->nNumberOfBytesToLockHigh == nNumberOfBytesToUnlockHigh)
          {
            if (region->m_LockingThread == dwThread)
            {
              filelock->m_lockregions.erase(region);
              LeaveCriticalSection(&g_cs);
              return TRUE;
            }
            else
            {
              LeaveCriticalSection(&g_cs);
              return FALSE;
            }
          }
        }
      }
    }

    assert(false);
    LeaveCriticalSection(&g_cs);
    return FALSE;
  }

#ifdef __cplusplus
}
#endif