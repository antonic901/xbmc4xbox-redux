/* Xbox 360 doesn't have GetTickCount64 (Vista+). Provide a shim using GetTickCount(). */
#include <xtl.h>

#ifdef __cplusplus
extern "C" {
#endif

ULONGLONG GetTickCount64(void)
{
    return (ULONGLONG)GetTickCount();
}

#ifdef __cplusplus
}
#endif
