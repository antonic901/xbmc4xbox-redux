#ifdef SPYCE_SUPPORT
#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

	namespace WEBS_SPYCE
	{
		int spyceOpen();
		void spyceClose();
		int spyceRequest(webs_t wp, char_t *lpath);
	}

#if defined(__cplusplus)
}
#endif
 
#endif
