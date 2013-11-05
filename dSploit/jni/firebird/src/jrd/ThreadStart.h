/*
 *	PROGRAM:	JRD access method
 *	MODULE:		thd.h
 *	DESCRIPTION:	Thread support definitions
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
 * Alex Peshkov
 */

#ifndef JRD_THREADSTART_H
#define JRD_THREADSTART_H

#include "firebird.h"
#include "../jrd/ThreadData.h"

/* Thread priorities (may be ignored) */

const int THREAD_high			= 1;
const int THREAD_medium_high	= 2;
const int THREAD_medium			= 3;
const int THREAD_medium_low		= 4;
const int THREAD_low			= 5;
const int THREAD_critical		= 6;


/* Thread startup */

// BRS 01/07/2004
// Hack due to a bug in mingw.
// The definition inside the thdd class should be replaced with the following one.
typedef THREAD_ENTRY_DECLARE ThreadEntryPoint(THREAD_ENTRY_PARAM);

class ThreadStart : public ThreadData
{
public:
	explicit ThreadStart(ThreadData::ThreadDataType t) : ThreadData(t) { }

	static void	start(ThreadEntryPoint* routine, void* arg, int priority_arg, void* thd_id);
};

extern "C" {
	int	API_ROUTINE gds__thread_start(ThreadEntryPoint*, void*, int, int, void*);
}

#ifdef WIN_NT
typedef HANDLE ThreadHandle;
#endif
#ifdef USE_POSIX_THREADS
typedef pthread_t ThreadHandle;
#endif

void THD_wait_for_completion(ThreadHandle& handle);

#endif // JRD_THREADSTART_H
