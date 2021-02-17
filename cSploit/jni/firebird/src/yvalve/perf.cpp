/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		perf.cpp
 *	DESCRIPTION:	Performance monitoring routines
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <limits.h>
#include "../jrd/ibase.h"
#include "../yvalve/perf.h"
#include "../yvalve/gds_proto.h"
#include "../yvalve/perf_proto.h"
#include "../yvalve/utl_proto.h"
#include "../common/gdsassert.h"
#include "../common/classes/fb_string.h"

#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

template <typename T>
static SINT64 get_parameter(const T** ptr)
{
/**************************************
 *
 *	g e t _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Get a parameter (length is encoded), convert to internal form,
 *	and return.
 *
 **************************************/
	SSHORT l = *(*ptr)++;
	l += (*(*ptr)++) << 8;
	const SINT64 parameter = isc_portable_integer(reinterpret_cast<const ISC_UCHAR*>(*ptr), l);
	*ptr += l;

	return parameter;
}


#ifndef HAVE_TIMES
static void times(struct tms*);
#endif

static const SCHAR items[] =
{
	isc_info_reads,
	isc_info_writes,
	isc_info_fetches,
	isc_info_marks,
	isc_info_page_size, isc_info_num_buffers,
	isc_info_current_memory, isc_info_max_memory
};

static const SCHAR* report = "elapsed = !e cpu = !u reads = !r writes = !w fetches = !f marks = !m$";

#if defined(WIN_NT) && !defined(CLOCKS_PER_SEC)
#define TICK	100
#endif

// EKU: TICK (sys/param.h) and CLOCKS_PER_SEC (time.h) may both be defined
#if !defined(TICK) && defined(CLOCKS_PER_SEC)
#define TICK ((SLONG)CLOCKS_PER_SEC)
#endif

#ifndef TICK
#define TICK	((SLONG)CLK_TCK)
#endif

template<typename P>
static int perf_format(const P* before, const P* after,
				const SCHAR* string, SCHAR* buffer, SSHORT* buf_len)
{
/**************************************
 *
 *	P E R F _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Format a buffer with statistical stuff under control of formatting
 *	string.  Substitute in appropriate stuff.  Return the length of the
 *	formatting output.
 *
 **************************************/
	SCHAR c;
	SLONG buffer_length = buf_len ? *buf_len : 0;
	SCHAR* p = buffer;

	if (buffer_length < 0)
	{
		buffer_length = 0;
	}

	while ((c = *string++) && c != '$')
	{
		if (c != '!')
			*p++ = c;
		else
		{
			SINT64 delta;
			switch (c = *string++)
			{
			case 'r':
				delta = after->perf_reads - before->perf_reads;
				break;
			case 'w':
				delta = after->perf_writes - before->perf_writes;
				break;
			case 'f':
				delta = after->perf_fetches - before->perf_fetches;
				break;
			case 'm':
				delta = after->perf_marks - before->perf_marks;
				break;
			case 'd':
				delta = after->perf_current_memory - before->perf_current_memory;
				break;
			case 'p':
				delta = after->perf_page_size;
				break;
			case 'b':
				delta = after->perf_buffers;
				break;
			case 'c':
				delta = after->perf_current_memory;
				break;
			case 'x':
				delta = after->perf_max_memory;
				break;
			case 'e':
				delta = after->perf_elapsed - before->perf_elapsed;
				break;
			case 'u':
				delta = after->perf_times.tms_utime - before->perf_times.tms_utime;
				break;
			case 's':
				delta = after->perf_times.tms_stime - before->perf_times.tms_stime;
				break;
			default:
				sprintf(p, "?%c?", c);
				while (*p)
					p++;
			}

			switch (c)
			{
			case 'r':
			case 'w':
			case 'f':
			case 'm':
			case 'd':
			case 'p':
			case 'b':
			case 'c':
			case 'x':
				sprintf(p, "%"SQUADFORMAT, delta);
				while (*p)
					p++;
				break;

			case 'u':
			case 's':
				sprintf(p, "%"SQUADFORMAT".%.2"SQUADFORMAT, delta / TICK, (delta % TICK) * 100 / TICK);
				while (*p)
					p++;
				break;

			case 'e':
				sprintf(p, "%"SQUADFORMAT".%.2"SQUADFORMAT, delta / 100, delta % 100);
				while (*p)
					p++;
				break;
			}
		}
	}

	*p = 0;
	const int length = static_cast<int>(p - buffer);
	if (buffer_length && (buffer_length -= length) >= 0) {
		memset(p, ' ', static_cast<size_t>(buffer_length));
	}

	return length;
}


template<typename P>
static void perf_get_info(FB_API_HANDLE* handle, P* perf)
{
/**************************************
 *
 *	P E R F _ g e t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Acquire timing and performance information.  Some info comes
 *	from the system and some from the database.
 *
 **************************************/
	SSHORT buffer_length, item_length;
	ISC_STATUS_ARRAY jrd_status;
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tp;
#else
	struct timeb time_buffer;
#define LARGE_NUMBER 696600000	// to avoid overflow, get rid of decades)
#endif

	// If there isn't a database, zero everything out

	if (!*handle) {
		memset(perf, 0, sizeof(PERF));
	}

	// Get system time

	times(&perf->perf_times);

#ifdef HAVE_GETTIMEOFDAY
	GETTIMEOFDAY(&tp);
	perf->perf_elapsed = tp.tv_sec * 100 + tp.tv_usec / 10000;
#else
	ftime(&time_buffer);
	perf->perf_elapsed = (time_buffer.time - LARGE_NUMBER) * 100 + (time_buffer.millitm / 10);
#endif

	if (!*handle)
		return;

	SCHAR buffer[256];
	buffer_length = sizeof(buffer);
	item_length = sizeof(items);
	isc_database_info(jrd_status, handle, item_length, items, buffer_length, buffer);

	const char* p = buffer;

	while (true)
		switch (*p++)
		{
		case isc_info_reads:
			perf->perf_reads = get_parameter(&p);
			break;

		case isc_info_writes:
			perf->perf_writes = get_parameter(&p);
			break;

		case isc_info_marks:
			perf->perf_marks = get_parameter(&p);
			break;

		case isc_info_fetches:
			perf->perf_fetches = get_parameter(&p);
			break;

		case isc_info_num_buffers:
			perf->perf_buffers = get_parameter(&p);
			break;

		case isc_info_page_size:
			perf->perf_page_size = get_parameter(&p);
			break;

		case isc_info_current_memory:
			perf->perf_current_memory = get_parameter(&p);
			break;

		case isc_info_max_memory:
			perf->perf_max_memory = get_parameter(&p);
			break;

		case isc_info_end:
			return;

		case isc_info_error:
			switch (p[2])
			{
			 case isc_info_marks:
				perf->perf_marks = 0;
				break;
			case isc_info_current_memory:
				perf->perf_current_memory = 0;
				break;
			case isc_info_max_memory:
				perf->perf_max_memory = 0;
				break;
			}

			{
				const SLONG temp = isc_vax_integer(p, 2);
				fb_assert(temp <= MAX_SSHORT);
				p += temp + 2;
			}

			perf->perf_marks = 0;
			break;

		default:
			return;
		}
}


template<typename P>
static void perf_report(const P* before, const P* after, SCHAR* buffer, SSHORT* buf_len)
{
/**************************************
 *
 *	p e r f _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Report a common set of parameters.
 *
 **************************************/

	perf_format<P>(before, after, report, buffer, buf_len);
}


#ifndef HAVE_TIMES
static void times(struct tms* buffer)
{
/**************************************
 *
 *	t i m e s
 *
 **************************************
 *
 * Functional description
 *	Emulate the good old unix call "times".  Only both with user time.
 *
 **************************************/

	buffer->tms_utime = clock();
}
#endif


int API_ROUTINE perf_format(const PERF* before, const PERF* after,
				const SCHAR* string, SCHAR* buffer, SSHORT* buf_len)
{
	return perf_format<PERF>(before, after, string, buffer, buf_len);
}

int API_ROUTINE perf64_format(const PERF64* before, const PERF64* after,
				const SCHAR* string, SCHAR* buffer, SSHORT* buf_len)
{
	return perf_format<PERF64>(before, after, string, buffer, buf_len);
}


void API_ROUTINE perf_get_info(FB_API_HANDLE* handle, PERF* perf)
{
	perf_get_info<PERF>(handle, perf);
}

void API_ROUTINE perf64_get_info(FB_API_HANDLE* handle, PERF64* perf)
{
	perf_get_info<PERF64>(handle, perf);
}


void API_ROUTINE perf_report(const PERF* before, const PERF* after, SCHAR* buffer, SSHORT* buf_len)
{
	perf_report<PERF>(before, after, buffer, buf_len);
}

void API_ROUTINE perf64_report(const PERF64* before, const PERF64* after, SCHAR* buffer, SSHORT* buf_len)
{
	perf_report<PERF64>(before, after, buffer, buf_len);
}

namespace {

static const unsigned CNT_DB_INFO = 1;
static const unsigned CNT_TIMER = 2;
enum CntTimer {CNT_TIME_REAL, CNT_TIME_USER, CNT_TIME_SYSTEM};

struct KnownCounters
{
	const char* name;
	unsigned type;
	unsigned code;
};

#define TOTAL_COUNTERS 11

// we use case-insensitive names, here they are written with capital letters for human readability
KnownCounters knownCounters[TOTAL_COUNTERS] = {
	{"RealTime", CNT_TIMER, CNT_TIME_REAL},
	{"UserTime", CNT_TIMER, CNT_TIME_USER},
	{"SystemTime", CNT_TIMER, CNT_TIME_SYSTEM},
	{"Fetches", CNT_DB_INFO, isc_info_fetches},
	{"Marks", CNT_DB_INFO, isc_info_marks},
	{"Reads", CNT_DB_INFO, isc_info_reads},
	{"Writes", CNT_DB_INFO, isc_info_writes},
	{"CurrentMemory", CNT_DB_INFO, isc_info_current_memory},
	{"MaxMemory", CNT_DB_INFO, isc_info_max_memory},
	{"Buffers", CNT_DB_INFO, isc_info_num_buffers},
	{"PageSize", CNT_DB_INFO, isc_info_page_size}
};

} // anonymous namespace

void FB_CARG Why::UtlInterface::getPerfCounters(Firebird::IStatus* status, Firebird::IAttachment* att,
		const char* countersSet, ISC_INT64* counters)
{
	try
	{
		// Parse countersSet
		unsigned cntLink[TOTAL_COUNTERS];
		memset(cntLink, 0xFF, sizeof cntLink);
		Firebird::string dupSet(countersSet);
		char* set = dupSet.begin();
		char* save = NULL;
		const char* delim = " \t,;";
		unsigned typeMask = 0;
		unsigned n = 0;
		UCHAR info[TOTAL_COUNTERS];		// will never use all, but do not care about few bytes
		UCHAR* pinfo = info;

#ifdef WIN_NT
#define strtok_r strtok_s
#endif

		for (char* nm = strtok_r(set, delim, &save); nm; nm = strtok_r(NULL, delim, &save))
		{
			Firebird::NoCaseString name(nm);

			for (unsigned i = 0; i < TOTAL_COUNTERS; ++i)
			{
				if (name == knownCounters[i].name)
				{
					if (cntLink[i] != ~0u)
						(Firebird::Arg::Gds(isc_random) << "Duplicated name").raise();	//report name & position

					cntLink[i] = n++;
					typeMask |= knownCounters[i].type;

					if (knownCounters[i].type == CNT_DB_INFO)
						*pinfo++ = knownCounters[i].code;

					goto found;
				}
			}
			(Firebird::Arg::Gds(isc_random) << "Unknown name").raise();	//report name & position
found:		;
		}

#ifdef WIN_NT
#undef strtok_r
#endif

		// Force reset counters
		memset(counters, 0, n * sizeof(ISC_INT64));

		// Fill time counters
		if (typeMask & CNT_TIMER)
		{
			SINT64 tr = fb_utils::query_performance_counter() * 1000 / fb_utils::query_performance_frequency();
			SINT64 uTime, sTime;
			fb_utils::get_process_times(uTime, sTime);

			for (unsigned i = 0; i < TOTAL_COUNTERS; ++i)
			{
				if (cntLink[i] == ~0u)
					continue;

				if (knownCounters[i].type == CNT_TIMER)
				{
					clock_t v = 0;

					switch(knownCounters[i].code)
					{
					case CNT_TIME_REAL:
						v = tr;
						break;
					case CNT_TIME_USER:
						v = uTime;
						break;
					case CNT_TIME_SYSTEM:
						v = sTime;
						break;
					default:
						fb_assert(false);
						break;
					}

					counters[cntLink[i]] = v;
				}
			}
		}

		// Fill DB counters
		if (typeMask & CNT_DB_INFO)
		{
			UCHAR buffer[BUFFER_LARGE];
			att->getInfo(status, pinfo - info, info, sizeof(buffer), buffer);
			if (!status->isSuccess())
				return;

			const UCHAR* p = buffer;

			while (true)
			{
				SINT64 v = 0;
				UCHAR ipb = *p++;

				switch (ipb)
				{
				case isc_info_reads:
				case isc_info_writes:
				case isc_info_marks:
				case isc_info_fetches:
				case isc_info_num_buffers:
				case isc_info_page_size:
				case isc_info_current_memory:
				case isc_info_max_memory:
					v = get_parameter(&p);
					break;

				case isc_info_end:
					goto parsed;

				case isc_info_error:
				{
					const SINT64 temp = isc_portable_integer(p, 2);
					fb_assert(temp <= MAX_SSHORT);
					p += temp + 2;
					continue;
				}

				default:
					(Firebird::Arg::Gds(isc_random) << "Unknown info code").raise();   //report char code
				}

				for (unsigned i = 0; i < TOTAL_COUNTERS; ++i)
				{
					if (knownCounters[i].type == CNT_DB_INFO && knownCounters[i].code == ipb)
					{
						if (cntLink[i] != ~0u)
							counters[cntLink[i]] = v;

						break;
					}
				}
			}
parsed:		;
		}
	}
	catch (const Firebird::Exception& ex)
	{
		ex.stuffException(status);
	}
}
