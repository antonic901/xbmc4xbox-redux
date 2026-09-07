/* signal.h - Stub for Xbox 360 (no POSIX signals) */
#ifndef _SIGNAL_H_XBOX360_STUB
#define _SIGNAL_H_XBOX360_STUB

/* Xbox 360 has no POSIX signal support.
 * Define minimal constants so code compiles. */

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
#endif

typedef int sigset_t;
typedef void (*sighandler_t)(int);

#ifndef SIG_IGN
#define SIG_IGN ((sighandler_t)1)
#endif

static __inline sighandler_t signal(int sig, sighandler_t handler) { (void)sig; (void)handler; return SIG_IGN; }
static __inline int sigemptyset(sigset_t *set) { (void)set; return 0; }
static __inline int sigaddset(sigset_t *set, int sig) { (void)set; (void)sig; return 0; }

#endif /* _SIGNAL_H_XBOX360_STUB */
