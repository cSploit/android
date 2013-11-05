/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		thd.cpp
 *	DESCRIPTION:	Thread support routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include "../jrd/common.h"
#include "../common/thd.h"
#include "../jrd/gdsassert.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/alloc.h"
#include "../common/classes/init.h"


#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef USE_POSIX_THREADS
#include <pthread.h>
#endif

FB_THREAD_ID getThreadId() throw()
{
/**************************************
 *
 *	T H D _ g e t _ t h r e a d _ i d
 *
 **************************************
 *
 * Functional description
 *	Get platform's notion of a thread ID.
 *
 **************************************/
	FB_THREAD_ID id = 1;
#ifdef WIN_NT
	id = GetCurrentThreadId();
#endif

#ifdef USE_POSIX_THREADS
	id = (FB_THREAD_ID) pthread_self();
#endif

	return id;
}


void THD_sleep(ULONG milliseconds)
{
/**************************************
 *
 *	T H D _ s l e e p
 *
 **************************************
 *
 * Functional description
 *	Thread sleeps for requested number
 *	of milliseconds.
 *
 **************************************/
#if defined(WIN_NT)
	SleepEx(milliseconds, FALSE);
#elif defined(HAVE_NANOSLEEP)
	timespec timer, rem;
	timer.tv_sec = milliseconds / 1000;
	timer.tv_nsec = (milliseconds % 1000) * 1000000;

	while (nanosleep(&timer, &rem) != 0)
	{
		if (errno != EINTR)
		{
			Firebird::system_call_failed::raise("nanosleep");
		}
		timer = rem;
	}
#else
	Firebird::Semaphore timer;
	timer.tryEnter(0, milliseconds);
#endif
}


void THD_yield()
{
/**************************************
 *
 *	T H D _ y i e l d
 *
 **************************************
 *
 * Functional description
 *	Thread relinquishes the processor.
 *
 **************************************/
#ifdef USE_POSIX_THREADS
/* use sched_yield() instead of pthread_yield(). Because pthread_yield()
   is not part of the (final) POSIX 1003.1c standard. Several drafts of
   the standard contained pthread_yield(), but then the POSIX guys
   discovered it was redundant with sched_yield() and dropped it.
   So, just use sched_yield() instead. POSIX systems on which
   sched_yield() is available define _POSIX_PRIORITY_SCHEDULING
   in <unistd.h>.  Darwin defined _POSIX_THREAD_PRIORITY_SCHEDULING
   instead of _POSIX_PRIORITY_SCHEDULING.
*/
#if (defined _POSIX_PRIORITY_SCHEDULING || defined _POSIX_THREAD_PRIORITY_SCHEDULING)
	sched_yield();
#else
	pthread_yield();
#endif // _POSIX_PRIORITY_SCHEDULING
#endif

#ifdef WIN_NT
	SleepEx(0, FALSE);
#endif
}
