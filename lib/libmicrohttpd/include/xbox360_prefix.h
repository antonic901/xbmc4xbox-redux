/*
 * xbox360_prefix.h - Forced-include prefix for Xbox 360 libmicrohttpd build
 *
 * This is specified as ForcedIncludeFiles in the .vcxproj.
 * It is processed BEFORE any source file content, ensuring
 * Xbox 360 XDK headers are loaded and Win32-compatible guards
 * are set so the upstream source's #include <windows.h> etc. are safe.
 */

#ifndef XBOX360_PREFIX_H
#define XBOX360_PREFIX_H 1

/* FD_SETSIZE must be defined BEFORE any Winsock headers are pulled in */
#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

/* Include Xbox 360 XDK master headers FIRST.
 * xtl.h provides the Windows-like API (CreateThread, CRITICAL_SECTION, etc.)
 * winsockx.h provides the Winsock API (SOCKET, select, closesocket, etc.) */
#include <xtl.h>
#include <winsockx.h>

/* Set include guards so that when upstream source does
 * #include <windows.h> or #include <winsock2.h>, they become no-ops.
 * The Xbox 360 XDK xtl.h/winsockx.h already provided everything. */
#ifndef _WINDOWS_
#define _WINDOWS_
#endif
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef _WINSOCK2API_
#define _WINSOCK2API_
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#ifndef _WS2TCPIP_H_
#define _WS2TCPIP_H_
#endif
#ifndef _WS2DEF_
#define _WS2DEF_
#endif

/* Provide ws2tcpip.h types that the source references */
#ifndef IPPROTO_IPV6
/* Xbox 360 may or may not support IPv6 - define the constant
   so code compiles, actual IPv6 features won't be used */
#define IPPROTO_IPV6 41
#endif

/* sockaddr_storage - Xbox 360 XDK may not define this (it's in ws2tcpip.h).
 * Needed by daemon.c for address-agnostic socket handling. */
#ifndef _SS_MAXSIZE
#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE (sizeof(__int64))
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof(short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof(short) + _SS_PAD1SIZE + _SS_ALIGNSIZE))
struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[_SS_PAD1SIZE];
    __int64 __ss_align;
    char __ss_pad2[_SS_PAD2SIZE];
};
#endif

/* SHUT_WR / SHUT_RD / SHUT_RDWR for Winsock */
#ifndef SD_RECEIVE
#define SD_RECEIVE 0
#endif
#ifndef SD_SEND
#define SD_SEND 1
#endif
#ifndef SD_BOTH
#define SD_BOTH 2
#endif

/* ssize_t for Xbox 360 (MSVC doesn't define it) */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

/* socklen_t for Xbox 360 */
#ifndef _SOCKLEN_T_DEFINED
#define _SOCKLEN_T_DEFINED
typedef int socklen_t;
#endif

/* MSG_NOSIGNAL doesn't exist on Windows/Xbox */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* bool type for C compilation (no stdbool.h on Xbox 360 XDK) */
#ifndef __cplusplus
#ifndef _STDBOOL_H
#define _STDBOOL_H
#ifndef bool
#define bool  int
#define true  1
#define false 0
#endif
#endif
#endif

/* snprintf - older MSVC uses _snprintf */
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif

/* off_t for file offset (not defined in Xbox 360 XDK) */
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long off_t;
#endif

/* HANDLE_FLAG_INHERIT - not available on Xbox 360 (no child processes) */
#ifndef HANDLE_FLAG_INHERIT
#define HANDLE_FLAG_INHERIT 0x00000001
#endif

/* SetHandleInformation stub - no handle inheritance on Xbox 360 */
#ifndef SetHandleInformation
#define SetHandleInformation(h, mask, flags) (1)
#endif

/* SYSTEM_INFO / GetSystemInfo - may not be in Xbox 360 XDK */
#ifndef _SYSTEM_INFO_XBOX360_DEFINED
#define _SYSTEM_INFO_XBOX360_DEFINED
#ifndef SYSTEM_INFO
typedef struct _SYSTEM_INFO_XBOX {
    DWORD dwPageSize;
    DWORD dwNumberOfProcessors;
    DWORD dwAllocationGranularity;
} SYSTEM_INFO;
static __inline void GetSystemInfo(SYSTEM_INFO* si) {
    si->dwPageSize = 4096;             /* Xbox 360 uses 4KB pages */
    si->dwNumberOfProcessors = 6;      /* 3 cores x 2 threads */
    si->dwAllocationGranularity = 65536; /* 64KB granularity */
}
#endif
#endif

/* On Xbox 360 XDK, InitializeCriticalSectionAndSpinCount returns void
 * (unlike standard Win32 where it returns BOOL). Wrap it to return TRUE. */
#ifdef InitializeCriticalSectionAndSpinCount
#undef InitializeCriticalSectionAndSpinCount
#endif
static __inline BOOL Xbox360_InitCS(CRITICAL_SECTION* cs, DWORD spin) {
    InitializeCriticalSection(cs);
    (void)spin;
    return TRUE;
}
#define InitializeCriticalSectionAndSpinCount(cs, spin) Xbox360_InitCS((cs), (spin))

/* Provide timespec if needed (Xbox 360 XDK doesn't have it) */
#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
    long tv_sec;
    long tv_nsec;
};
#endif

#endif /* XBOX360_PREFIX_H */
