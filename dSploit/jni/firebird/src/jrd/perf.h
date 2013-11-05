/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		perf.h
 *	DESCRIPTION:	Peformance tool block
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "OS/2" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_PERF_H
#define JRD_PERF_H

#if defined LINUX && !defined ARM
#include <libio.h>
#endif

#ifdef HAVE_TIMES
#include <sys/types.h>
#include <sys/times.h>
#else
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structure returned by times()
 */
struct tms
{
	time_t tms_utime;			/* user time */
	time_t tms_stime;			/* system time */
	time_t tms_cutime;			/* user time, children */
	time_t tms_cstime;			/* system time, children */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !HAVE_TIMES */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct perf
{
	long perf_fetches;
	long perf_marks;
	long perf_reads;
	long perf_writes;
	long perf_current_memory;
	long perf_max_memory;
	long perf_buffers;
	long perf_page_size;
	long perf_elapsed;
	struct tms perf_times;
} PERF;

typedef struct perf64
{
	ISC_INT64 perf_fetches;
	ISC_INT64 perf_marks;
	ISC_INT64 perf_reads;
	ISC_INT64 perf_writes;
	ISC_INT64 perf_current_memory;
	ISC_INT64 perf_max_memory;
	long perf_buffers;
	long perf_page_size;
	long perf_elapsed;
	struct tms perf_times;
} PERF64;

/* Letter codes controlling printing of statistics:

	!f - fetches
	!m - marks
	!r - reads
	!w - writes
	!e - elapsed time (in seconds)
	!u - user times
	!s - system time
	!p - page size
	!b - number buffers
	!d - delta memory
	!c - current memory
	!x - max memory

*/

#ifdef __cplusplus
} /* extern "C" */
#endif


#include "../jrd/perf_proto.h"

#endif /* JRD_PERF_H */

