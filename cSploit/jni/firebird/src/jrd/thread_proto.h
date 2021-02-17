#ifndef JRD_THREAD_PROTO_H
#define JRD_THREAD_PROTO_H

#include "../common/thd.h"

inline void THREAD_SLEEP(ULONG msecs)
{
	THD_sleep(msecs);
}

inline void THREAD_YIELD()
{
	THD_yield();
}

#endif // JRD_THREAD_PROTO_H
