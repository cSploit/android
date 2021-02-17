/*
 * MTEST.C -- A test program to compare performance of memcpy()/memset()
 * 	   -- vs. MOV_fast*()/MOV_fill() functions.
 *
 *  This program does speed tests on various implementions of "copy buffer"
 *  and "initialize buffer" operations.  It's purpose is as a development
 *  aid to determine which implementations are more efficient for different
 *  platforms.
 *
 *  Considerations when doing this evaluation:
 *	Alignment of source & destination buffers.
 *	Size of buffers.
 *	Compiler optimization.
 *	Automatic compiler in-lining optimization.
 *
 * Because of the short nature of the operation being timed (eg: copy 20
 * bytes) - the operation is repeated several times for each 'timing'.
 *
 * Each timing run is then also repeated several times to take an average.
 *
 * Program originally by Zouhair Janbay - modified
 * 1996-Feb-09 David Schnepper
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
 */

#include "firebird.h"
#include <stdio.h>
#include "../jrd/mov.cpp"
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
#ifdef HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif


const int TEST_REPEATS	= 6;		/* Number of times to repeat timing */
const int MCOUNT		= 10000;	/* Number of operations per timing */
const int MAXSIZE		= 8196;		/* Max size of buffer to copy */

ULONG force_alignment_dummy1;
UCHAR source[MAXSIZE + sizeof(ULONG) * 2];

ULONG force_alignment_dummy2;
UCHAR destination[MAXSIZE + sizeof(ULONG) + 2];

/* Test copies of these sizes of buffers */
USHORT block_sizes[] = { 0, 1, 4, 15, 16, 17, 32, 150, 512, 1024 * 3 - 35 };

#define	BSIZES FB_NELEM(block_sizes)

struct timeb tp;
time_t first_time, last_time, curr_time;
int curr_repeat = 0;
ULONG intervals[TEST_REPEATS];

void report_time(const char* s);
void init_time();
void start_time();
void stop_time();
void MOV_fill(), MOV_fast(), MOV_faster();
void memcpy(), memset();

void init_time()
{
	int i;
	for (i = 0; i < FB_NELEM(intervals); i++)
		intervals[i] = 0;
	curr_repeat = 0;
}

void start_time()
{
	if (ftime(&tp) != -1)
		last_time = (tp.time * 1000) + tp.millitm;
	else
		printf("ftime() failed\n");
}

void stop_time()
{
	if (ftime(&tp) != -1)
	{
		curr_time = (tp.time * 1000) + tp.millitm;
		intervals[curr_repeat++] = curr_time - last_time;
	}
	else
		printf("ftime() failed\n");
}

void report_time(const char* s)
{
	int i;
	ULONG max_interval = 0;
	ULONG min_interval = (ULONG) - 1;
	ULONG sum_interval = 0;
	ULONG avg_interval;

/* Doesn't handle single repeat case */
/* Ignores the first timing */

	for (i = 1; i < curr_repeat; i++)
	{
		sum_interval += intervals[i];
		if (intervals[i] > max_interval)
			max_interval = intervals[i];
		if (intervals[i] < min_interval)
			min_interval = intervals[i];
	}

	avg_interval = (sum_interval - (max_interval + min_interval)) / (curr_repeat - 3);

	printf("%s: %4ld avg ms %4ld min ms %4ld max ms\n",
			  s, avg_interval, min_interval, max_interval);
}

main(int argc, char** argv)
{
	int i, j, k, r;

	init_time();
	for (r = 0; r < TEST_REPEATS; ++r)
	{
		start_time();
		{
			for (j = 0; j < MCOUNT; ++j)
				/* Do nothing */ ;
		}
		stop_time();
	}
	report_time("Empty Loop");

	init_time();
	for (r = 0; r < TEST_REPEATS; ++r)
	{
		start_time();
		{
			for (j = 0; j < MCOUNT; ++j)
				empty_procedure();
		}
		stop_time();
	}
	report_time("Empty procedure call");

	printf("\n\nTiming memset (destination aligments & blocksizes)\n\n");

	for (k = 0; k < BSIZES; ++k)
	{
		/* Aligned */
		do_test_memset((UCHAR *) destination, block_sizes[k]);
	}

	for (k = 0; k < BSIZES; ++k)
	{
		/* Unaligned */
		do_test_memset((UCHAR *) destination + 1, block_sizes[k]);
	}
	fflush(stdout);


	printf
		("\n\nTiming memcpy (source aligments, destination alignments & blocksizes)\n\n");
	for (k = 0; k < BSIZES; ++k)
	{
		/* Both aligned */
		do_test_memcpy((UCHAR *) source, (UCHAR *) destination, block_sizes[k]);
	}

	for (k = 0; k < BSIZES; ++k)
	{
		/* Both on SAME alignment */
		do_test_memcpy((UCHAR *) source + 1, (UCHAR *) destination + 1, block_sizes[k]);
	}

	for (k = 0; k < BSIZES; ++k)
	{
		/* Different alignments */
		do_test_memcpy((UCHAR *) source + 1, (UCHAR *) destination + 3, block_sizes[k]);
	}

	fflush(stdout);

	exit(0);
}


empty_procedure()
/* Just an empty procedure for testing call overhead */
{
}

do_test_memset(void* dest, int size)
{
	int j;
	int r;
	char message_buffer[150];


	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		start_time();
		for (j = 0; j < MCOUNT; ++j)
			MOV_fill((SLONG *) dest, (ULONG) size);
		stop_time();
	}
	sprintf(message_buffer, "Dest align %1d Size %4d %20s",
			(((U_IPTR) dest) % sizeof(ULONG)), size, "MOV_fill");
	report_time(message_buffer);

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		start_time();
		for (j = 0; j < MCOUNT; ++j)
			memset(dest, 0, size);
		stop_time();
	}
	sprintf(message_buffer, "Dest align %1d Size %4d %20s",
			(((U_IPTR) dest) % sizeof(ULONG)), size, "memset");
	report_time(message_buffer);

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		char *p, *p_end;

		start_time();
		for (j = 0; j < MCOUNT; ++j)
		{
			for (p = dest, p_end = p + size; p < p_end;)
				*p++ = 0;
		}
		stop_time();
	}
	sprintf(message_buffer, "Dest align %1d Size %4d %20s",
			(((U_IPTR) dest) % sizeof(ULONG)), size, "inline - bytes");
	report_time(message_buffer);

/* Guesses for optimal length for each algorithm */
const int SIZE_OPTIMAL_MEMSET_INLINE	= 16;
const int SIZE_OPTIMAL_MEMSET_MEMSET	= 160;
const int SIZE_OPTIMAL_MEMCPY_INLINE	= 16;
const int SIZE_OPTIMAL_MEMCPY_MEMCPY	= 160;

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		char *p, *p_end;

		start_time();
		for (j = 0; j < MCOUNT; ++j)
		{
			if (size < SIZE_OPTIMAL_MEMSET_INLINE)
				for (p = dest, p_end = p + size; p < p_end;)
					*p++ = 0;
			else if (size < SIZE_OPTIMAL_MEMSET_MEMSET)
				memset(dest, 0, size);
			else
				MOV_fill((SLONG *) dest, (ULONG) size);
		}
		stop_time();
	}
	sprintf(message_buffer, "Dest align %1d Size %4d %20s",
			(((U_IPTR) dest) % sizeof(ULONG)), size, "inline - testing");
	report_time(message_buffer);
}


do_test_memcpy(void* src, void* dest, int size)
{
	int j;
	int r;
	char message_buffer[150];

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		start_time();
		{
			for (j = 0; j < MCOUNT; ++j)
				MOV_fast(src, dest, size);
		}
		stop_time();
	}
	sprintf(message_buffer, "Src align %1d Dest align %1d Size %4d %20s",
			(((U_IPTR) src) % sizeof(ULONG)),
			(((U_IPTR) dest) % sizeof(ULONG)), size, "MOV_fast");
	report_time(message_buffer);

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		start_time();
		{
			for (j = 0; j < MCOUNT; ++j)
				memcpy(dest, src, size);
		}
		stop_time();
	}
	sprintf(message_buffer, "Src align %1d Dest align %1d Size %4d %20s",
			(((U_IPTR) src) % sizeof(ULONG)),
			(((U_IPTR) dest) % sizeof(ULONG)), size, "memcpy");
	report_time(message_buffer);

/* MOV_faster can only do aligned moves, so don't test it otherwise */
	if (((((U_IPTR) dest) % sizeof(ULONG)) == 0) &&
		((((U_IPTR) src) % sizeof(ULONG)) == 0))
	{
		init_time();
		for (r = 0; r < TEST_REPEATS; r++)
		{
			start_time();
			{
				for (j = 0; j < MCOUNT; ++j)
					MOV_faster((SLONG *) src, (SLONG *) dest, (ULONG) size);
			}
			stop_time();
		}
		sprintf(message_buffer, "Src align %1d Dest align %1d Size %4d %20s",
				(((U_IPTR) src) % sizeof(ULONG)),
				(((U_IPTR) dest) % sizeof(ULONG)), size, "MOV_faster");
		report_time(message_buffer);
	}

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		char *p, *q, *p_end;

		start_time();
		for (j = 0; j < MCOUNT; ++j)
		{
			for (p = dest, q = src, p_end = p + size; p < p_end;)
				*p++ = *q++;
		}
		stop_time();
	}
	sprintf(message_buffer, "Src align %1d Dest align %1d Size %4d %20s",
			(((U_IPTR) src) % sizeof(ULONG)),
			(((U_IPTR) dest) % sizeof(ULONG)), size, "inline move - bytes");
	report_time(message_buffer);

	init_time();
	for (r = 0; r < TEST_REPEATS; r++)
	{
		char *p, *q, *p_end;

		start_time();
		for (j = 0; j < MCOUNT; ++j)
		{
			if (size < SIZE_OPTIMAL_MEMCPY_INLINE)
				for (p = dest, q = src, p_end = p + size; p < p_end;)
					*p++ = *q++;
			else if ((size >= SIZE_OPTIMAL_MEMCPY_MEMCPY) &&
					 (((((U_IPTR) dest) % sizeof(ULONG)) == 0) &&
						 ((((U_IPTR) src) % sizeof(ULONG)) == 0)))
				MOV_faster((SLONG *) src, (SLONG *) dest, (ULONG) size);
			else
				memcpy(dest, src, size);
		}
		stop_time();
	}
	sprintf(message_buffer, "Src align %1d Dest align %1d Size %4d %20s",
			(((U_IPTR) src) % sizeof(ULONG)),
			(((U_IPTR) dest) % sizeof(ULONG)),
			size, "inline testing - bytes");
}


/* stubs */
void gds__encode_date()
{
}

USHORT CVT2_make_string2()
{
}

USHORT CVT_make_string()
{
}

int CVT2_compare()
{
}

USHORT CVT_get_string_ptr()
{
}

SLONG CVT_get_long()
{
}

void CVT2_get_name()
{
}

void CVT_double_to_date()
{
}

double CVT_date_to_double()
{
}

SQUAD CVT_get_quad()
{
}

void CVT_move()
{
}

void ERR_post()
{
}

double CVT_get_double()
{
}

