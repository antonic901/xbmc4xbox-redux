/* fcntl.h - Stub for Xbox 360 (no file control operations) */
#ifndef _FCNTL_H_XBOX360_STUB
#define _FCNTL_H_XBOX360_STUB

/* Xbox 360 doesn't have POSIX fcntl.
 * Winsock ioctlsocket() is used for non-blocking sockets instead. */

#ifndef O_BINARY
#define O_BINARY 0x8000
#endif

#ifndef O_RDWR
#define O_RDWR 0x0002
#endif

#ifndef O_WRONLY
#define O_WRONLY 0x0001
#endif

#ifndef _O_TRUNC
#define _O_TRUNC 0x0200
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef F_SETFL
#define F_SETFL 0
#endif

#ifndef F_GETFL
#define F_GETFL 0
#endif

static __inline int fcntl(int fd, int cmd, ...) { (void)fd; (void)cmd; return -1; }

#endif /* _FCNTL_H_XBOX360_STUB */
