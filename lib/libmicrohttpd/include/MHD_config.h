/*
 * MHD_config.h - Hand-crafted configuration for Xbox 360 XDK
 *
 * This replaces the autoconf-generated MHD_config.h.
 * Included by mhd_options.h (the real upstream header).
 */

#ifndef MHD_CONFIG_H
#define MHD_CONFIG_H 1

/* ================================================================ */
/*  Compiler Features                                                */
/* ================================================================ */

/* This is a Windows system */
#define WINDOWS 1

/* Define if MS VC compiler is used */
#define MSVC 1

/* MS VC does not support VLAs */
#ifndef __STDC_NO_VLA__
#define __STDC_NO_VLA__ 1
#endif

/* Define to 1 if your C compiler supports inline functions. */
#define INLINE_FUNC 1

/* Define to prefix which will be used with MHD inline functions. */
#define _MHD_static_inline static __forceinline

/* Define to supported 'noreturn' function declaration */
#define _MHD_NORETURN __declspec(noreturn)

/* Define to 1 if your compiler supports __func__ */
#define HAVE___FUNC__ 1

/* bool type - Xbox 360 XDK compiler (VS 2005 era) doesn't have stdbool.h in C mode */
#ifndef __cplusplus
#ifndef bool
#define bool int
#define true (!0)
#define false 0
#endif
#endif

/* Define to 1 if you have usable assert() */
#define HAVE_ASSERT 1

/* ================================================================ */
/*  Xbox 360 Platform Identity                                      */
/* ================================================================ */

/* The Xbox 360 XDK compiler defines _WIN32 natively.
 * This ensures the source takes Win32 code paths.
 * Xbox 360 provides the same Win32 API surface
 * (SOCKET, closesocket, CreateThread, CRITICAL_SECTION, etc.) */
#ifndef _WIN32
#define _WIN32 1
#endif

/* Xbox 360 is big-endian PowerPC */
#define WORDS_BIGENDIAN 1

/* We are building a static library */
#define BUILDING_MHD_LIB 1

/* ================================================================ */
/*  Threading                                                        */
/* ================================================================ */

/* Use Win32 threads (CreateThread / CRITICAL_SECTION) */
#define MHD_USE_W32_THREADS 1
/* #undef MHD_USE_POSIX_THREADS */
/* #undef HAVE_PTHREAD_H */

/* ================================================================ */
/*  Networking                                                       */
/* ================================================================ */

/* Use Winsock sockets */
#define MHD_WINSOCK_SOCKETS 1
/* #undef MHD_POSIX_SOCKETS */

/* ITC: socket pair type is defined for struct layout compatibility,
   but Xbox 360 cannot create socket pairs (no loopback, and XNet
   does not support connecting to your own IP reliably).
   HAVE_LISTEN_SHUTDOWN is defined so MHD does NOT attempt to create
   an ITC socket pair.  Xbox 360's shutdown() on a listening socket
   returns WSAENOTCONN (error 10057) and fails to wake select(), so
   we cap the polling thread's select timeout in daemon.c instead. */
#define _MHD_ITC_SOCKETPAIR 1
/* #undef _MHD_ITC_EVENTFD */
/* #undef _MHD_ITC_PIPE */
#define HAVE_LISTEN_SHUTDOWN 1

/* No poll/epoll on Xbox 360 - select() only */
/* #undef HAVE_POLL */
/* #undef HAVE_POLL_H */
/* #undef EPOLL_SUPPORT */
/* #undef HAVE_EPOLL_CREATE1 */

/* No sendfile */
/* #undef HAVE_LINUX_SENDFILE */
/* #undef HAVE_FREEBSD_SENDFILE */
/* #undef HAVE_DARWIN_SENDFILE */
/* #undef HAVE_SOLARIS_SENDFILE */

/* Socket features */
/* #undef HAVE_ACCEPT4 */
/* #undef HAVE_SOCK_NONBLOCK */
/* #undef SOCK_CLOEXEC */
/* #undef SOCK_NOSIGPIPE */

/* ================================================================ */
/*  TLS / HTTPS                                                      */
/* ================================================================ */

/* No HTTPS support */
/* #undef HTTPS_SUPPORT */

/* ================================================================ */
/*  Authentication                                                   */
/* ================================================================ */

/* Enable Basic Auth */
#define BAUTH_SUPPORT 1

/* Disable Digest Auth (requires SHA/MD5 we don't need) */
/* #undef DAUTH_SUPPORT */

/* ================================================================ */
/*  Features                                                         */
/* ================================================================ */

/* Enable error/log messages */
#define HAVE_MESSAGES 1

/* Enable POST processor */
#define HAVE_POSTPROCESSOR 1

/* Use our bundled tsearch (no system search.h) */
/* #undef MHD_USE_SYS_TSEARCH */
/* #undef HAVE_SEARCH_H */

/* ================================================================ */
/*  Available Headers                                                */
/* ================================================================ */

#define HAVE_STDLIB_H     1
#define HAVE_STRING_H     1
#define HAVE_ERRNO_H      1
#define HAVE_LIMITS_H     1
#define HAVE_STDINT_H     1
#define HAVE_STDDEF_H     1
#define HAVE_TIME_H       1
#define HAVE_MEMORY_H     1

/* Xbox 360 XDK has Winsock via winsockx.h (included via xtl.h path) */
#define HAVE_WINSOCK2_H   1

/* NOT available on Xbox 360 */
/* #undef HAVE_STDBOOL_H */
/* #undef HAVE_INTTYPES_H */
/* #undef HAVE_UNISTD_H */
/* #undef HAVE_FCNTL_H */
/* #undef HAVE_SIGNAL_H */
/* #undef HAVE_ARPA_INET_H */
/* #undef HAVE_NETDB_H */
/* #undef HAVE_NETINET_IN_H */
/* #undef HAVE_NETINET_TCP_H */
/* #undef HAVE_SYS_SOCKET_H */
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_TIME_H */
/* #undef HAVE_SYS_STAT_H */
/* #undef HAVE_SYS_TYPES_H */
/* #undef HAVE_SYS_MMAN_H */
/* #undef HAVE_SYS_MSG_H */
/* #undef HAVE_NET_IF_H */
/* #undef HAVE_SOCKLIB_H */
/* #undef HAVE_INETLIB_H */

/* ================================================================ */
/*  Available Functions                                              */
/* ================================================================ */

#define HAVE_CALLOC       1
#define HAVE_SNPRINTF     1

/* #undef HAVE_GMTIME_R */
/* #undef HAVE_MEMMEM */
/* #undef HAVE_PIPE */
/* #undef HAVE_PIPE2 */
/* #undef HAVE_PIPE2_FUNC */
/* #undef HAVE_SOCKETPAIR */
/* #undef HAVE_WRITEV */
/* #undef HAVE_SENDMSG */

/* ================================================================ */
/*  Size Defines                                                     */
/* ================================================================ */

#define SIZEOF_INT                4
#define SIZEOF_UNSIGNED_INT       4
#define SIZEOF_UNSIGNED_LONG      4
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_SIZE_T             4
#define SIZEOF_STRUCT_TIMEVAL_TV_SEC 4
#define SIZEOF_INT64_T            8
#define SIZEOF_UINT64_T           8

/* ================================================================ */
/*  FD_SETSIZE                                                       */
/* ================================================================ */

/* Override before winsock headers are included */
#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

/* Tell sysfdsetsize that system FD_SETSIZE equals our defined value */
#define MHD_SYS_FD_SETSIZE_ 64

/* ================================================================ */
/*  Miscellaneous                                                    */
/* ================================================================ */

/* No IPv6 on Xbox 360 (optional, may work if XDK supports it) */
/* #undef HAVE_INET6 */

/* Disable auto-init (we'll call MHD init explicitly or not at all) */
/* #undef _AUTOINIT_FUNCS_ARE_SUPPORTED */

/* No TCP_FASTOPEN */
/* #undef TCP_FASTOPEN */
/* #undef HAVE_FREEBSD_SENDFILE */

/* No itoa but have _itoa */
/* #undef HAVE_ITOA */

#endif /* MHD_CONFIG_H */
