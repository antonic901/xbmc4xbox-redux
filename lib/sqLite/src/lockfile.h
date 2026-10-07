#include <winbase.h>

#define LOCKFILE_FAIL_IMMEDIATELY 0

#ifdef __cplusplus
extern "C" {
#endif

void    AddLockMap(const char* zFileName, HANDLE hFile);

void    RemoveLockMap(HANDLE hFile);

BOOL LockFileEx(
  HANDLE hFile,
  DWORD dwFlags,
  DWORD dwReserved,
  DWORD nNumberOfBytesToLockLow,
  DWORD nNumberOfBytesToLockHigh,
  LPOVERLAPPED lpOverlapped);

BOOL LockFile(
  HANDLE hFile,
  DWORD dwFileOffsetLow,
  DWORD dwFileOffsetHigh,
  DWORD nNumberOfBytesToLockLow,
  DWORD nNumberOfBytesToLockHigh);

BOOL UnlockFile(
  HANDLE hFile,
  DWORD dwFileOffsetLow,
  DWORD dwFileOffsetHigh,
  DWORD nNumberOfBytesToUnlockLow,
  DWORD nNumberOfBytesToUnlockHigh);

#ifdef __cplusplus
}
#endif
