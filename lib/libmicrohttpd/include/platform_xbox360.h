/*
 * platform_xbox360.h - Platform abstraction for Xbox 360
 * Replaces the POSIX-centric platform.h from libmicrohttpd source tree.
 *
 * This header is included by libmicrohttpd internals before microhttpd.h
 * and provides Xbox 360 / XDK specific definitions.
 */

#ifndef MHD_PLATFORM_XBOX360_H
#define MHD_PLATFORM_XBOX360_H

#include "MHD_config_xbox360.h"

/* Xbox 360 XDK headers */
#include <xtl.h>
#include <winsockx.h>

/* Standard C headers available on Xbox 360 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

/* ---------------------------------------------------------------- */
/*                    POSIX compatibility shims                     */
/* ---------------------------------------------------------------- */

/* Map POSIX socket close to Winsock */
#define MHD_socket_close_(fd)  closesocket((fd))

/* No SIGPIPE on Xbox 360 */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

/* Xbox 360 uses SOCKET type, not int */
typedef SOCKET MHD_socket;
#define MHD_INVALID_SOCKET INVALID_SOCKET
#define MHD_SOCKET_DEFINED 1

/* ssize_t for Xbox 360 */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

/* socklen_t if not defined */
#ifndef _SOCKLEN_T_DEFINED
#define _SOCKLEN_T_DEFINED
typedef int socklen_t;
#endif

/* snprintf may be _snprintf on older MSVC */
#ifndef snprintf
#define snprintf _snprintf
#endif

/* ---------------------------------------------------------------- */
/*                    Threading (Win32 model)                       */
/* ---------------------------------------------------------------- */

/*
 * libmicrohttpd 1.x on Win32 uses native threads.
 * On Xbox 360 we use the same CreateThread / CRITICAL_SECTION model.
 */

typedef HANDLE MHD_thread_handle_;
typedef DWORD  MHD_thread_ID_;

#define MHD_THRD_RTRN_TYPE_ DWORD WINAPI
#define MHD_THRD_CALL_SPEC_ /* nothing - already WINAPI in return type */

/* Mutex = CRITICAL_SECTION */
typedef CRITICAL_SECTION MHD_mutex_;

#define MHD_mutex_init_(m)      (InitializeCriticalSection(m), !0)
#define MHD_mutex_destroy_(m)   (DeleteCriticalSection(m), !0)
#define MHD_mutex_lock_(m)      (EnterCriticalSection(m), !0)
#define MHD_mutex_unlock_(m)    (LeaveCriticalSection(m), !0)

/* ---------------------------------------------------------------- */
/*                    ITC (Inter-Thread Communication)              */
/* ---------------------------------------------------------------- */

/*
 * On Xbox 360, we use Win32 Events for ITC signalling between threads.
 * This is a simplified approach - the daemon select() loop can use
 * WaitForMultipleObjects or we can add a loopback socket pair.
 *
 * For MHD_USE_SELECT_INTERNALLY with thread pool size 1,
 * ITC is not strictly required (we set connection timeout to wake up).
 */

/* Placeholder - ITC can be a manual-reset event */
typedef HANDLE MHD_itc_;
#define MHD_ITC_IS_VALID_(itc)    ((itc) != NULL)
#define MHD_itc_init_(itc)        (*(itc) = CreateEvent(NULL, FALSE, FALSE, NULL), MHD_ITC_IS_VALID_(*(itc)))
#define MHD_itc_activate_(itc)    SetEvent(itc)
#define MHD_itc_clear_(itc)       ResetEvent(itc)
#define MHD_itc_destroy_(itc)     (CloseHandle(itc), (itc) = NULL)

#endif /* MHD_PLATFORM_XBOX360_H */
