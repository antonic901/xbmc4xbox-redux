/*
 * MHD_config_xbox360.h - Hand-crafted configuration for Xbox 360 XDK
 * Based on libmicrohttpd 1.0.x configuration system
 *
 * This replaces the autoconf-generated MHD_config.h for Xbox 360 builds.
 */

#ifndef MHD_CONFIG_XBOX360_H
#define MHD_CONFIG_XBOX360_H

/* ---------------------------------------------------------------- */
/*                    Xbox 360 specific settings                    */
/* ---------------------------------------------------------------- */

/* We are building on a Windows-like platform */
#define WINDOWS 1
#define MHD_WINSOCK_SOCKETS 1

/* Xbox 360 is big-endian PowerPC */
#define WORDS_BIGENDIAN 1

/* Disable HTTPS/TLS - not needed */
/* #undef HTTPS_SUPPORT */

/* Disable poll() and epoll() - use select() only */
/* #undef HAVE_POLL_H */
/* #undef HAVE_POLL */
/* #undef HAVE_EPOLL_CREATE1 */

/* Threading: Use Win32 threads (CreateThread/CRITICAL_SECTION) */
#define MHD_USE_W32_THREADS 1
/* #undef HAVE_PTHREAD_H */

/* ---------------------------------------------------------------- */
/*                          HEADER FILES                            */
/* ---------------------------------------------------------------- */

#define HAVE_ERRNO_H     1
#define HAVE_LIMITS_H    1
#define HAVE_STDLIB_H    1
#define HAVE_STRING_H    1
#define HAVE_STDINT_H    1
#define HAVE_TIME_H      1

/* Xbox 360 XDK has these via xtl.h / winsockx.h */
#define HAVE_WINSOCK2_H  1

/* Xbox 360 XDK does NOT have these */
/* #undef HAVE_ARPA_INET_H */
/* #undef HAVE_NETDB_H */
/* #undef HAVE_NETINET_IN_H */
/* #undef HAVE_SYS_SOCKET_H */
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_TIME_H */
/* #undef HAVE_SYS_STAT_H */
/* #undef HAVE_SYS_TYPES_H */
/* #undef HAVE_UNISTD_H */
/* #undef HAVE_FCNTL_H */
/* #undef HAVE_SIGNAL_H */
/* #undef HAVE_SYS_MMAN_H */
/* #undef HAVE_SEARCH_H */

/* ---------------------------------------------------------------- */
/*                             FUNCTIONS                            */
/* ---------------------------------------------------------------- */

#define HAVE_CLOSESOCKET 1
#define HAVE_SELECT      1
#define HAVE_SOCKET      1
#define HAVE_RECV        1
#define HAVE_SEND        1

/* Xbox 360 does NOT have these */
/* #undef HAVE_ACCEPT4 */
/* #undef HAVE_GMTIME_R */
/* #undef HAVE_MEMMEM */
/* #undef HAVE_PIPE */
/* #undef HAVE_PIPE2 */
/* #undef HAVE_SOCKETPAIR */
/* #undef HAVE_SENDFILE */

/* ---------------------------------------------------------------- */
/*                         SIZE DEFINES                             */
/* ---------------------------------------------------------------- */

#define SIZEOF_INT       4
#define SIZEOF_SIZE_T    4

/* ---------------------------------------------------------------- */
/*                       FEATURE TOGGLES                            */
/* ---------------------------------------------------------------- */

/* Enable error log messages */
#define HAVE_MESSAGES    1

/* Enable Basic Auth */
#define BAUTH_SUPPORT    1

/* Disable Digest Auth to keep things simple for now */
/* #undef DAUTH_SUPPORT */

/* Disable POST processor for now */
/* #undef HAVE_POSTPROCESSOR */

/* No sendfile on Xbox 360 */
/* #undef HAVE_SENDFILE */
/* #undef HAVE_LINUX_SENDFILE */
/* #undef HAVE_FREEBSD_SENDFILE */
/* #undef HAVE_DARWIN_SENDFILE */

/* ITC (Inter-Thread Communication) via W32 events */
#define MHD_ITC_EVENTFD  0
#define MHD_ITC_PIPE     0
#define MHD_ITC_SOCKETPAIR 0

/* FD_SETSIZE - Xbox 360 Winsock default */
#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#endif /* MHD_CONFIG_XBOX360_H */
