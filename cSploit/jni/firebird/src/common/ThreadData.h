/*
 *	PROGRAM:	JRD access method
 *	MODULE:		ThreadData.h
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

#ifndef JRD_THREADDATA_H
#define JRD_THREADDATA_H

#include "firebird.h"

#ifdef USE_POSIX_THREADS
#include "fb_pthread.h"
#endif

// Thread specific data

namespace Firebird {

class ThreadData
{
public:
	enum ThreadDataType {
		tddGBL = 1,		// used by backup/restore
		tddDBB = 2,		// used in engine
		tddDBA = 3,		// used in gstat utility
		tddALICE = 4,	// used by gfix
		tddSEC = 5		// used by gsec
	};

private:
	ThreadData*		threadDataPriorContext;
	ThreadDataType	threadDataType;	// what kind of thread context this is

public:
	explicit ThreadData(ThreadDataType t)
		: threadDataPriorContext(0), threadDataType(t)
	{}

	ThreadDataType getType() const
	{
		return threadDataType;
	}

	static ThreadData*	getSpecific();
	void			putSpecific();
	static void		restoreSpecific();
};

} // namespace Firebird

// Thread entry point definitions might much better look in ThreadStart.h,
// but due to need to have them in utilities declarations, and no need in
// gds__thread_start() there, placed here. AP 2007.

#if defined(WIN_NT)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN unsigned int
#define THREAD_ENTRY_CALL __stdcall
#elif defined(USE_POSIX_THREADS)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN void*
#define THREAD_ENTRY_CALL
#else
// Define correct types for other platforms
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN int
#define THREAD_ENTRY_CALL
#endif

#define THREAD_ENTRY_DECLARE THREAD_ENTRY_RETURN THREAD_ENTRY_CALL

#endif // JRD_THREADDATA_H
