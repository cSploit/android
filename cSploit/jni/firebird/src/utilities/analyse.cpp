/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Analyse.cpp
 *	DESCRIPTION:	I/O trace analysis
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
 */

#include "firebird.h"

#ifdef HAVE_TIMES
#include <sys/types.h>
#include <sys/times.h>
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

#ifdef HAVE_IO_H
#include <io.h> // open, close
#endif

#include <stdio.h>
#include <errno.h>
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/perf.h"

#ifndef HAVE_TIMES
static time_t times(struct tms*);
#endif


using namespace Ods;

static void analyse(int, const SCHAR*, const pag*, int);
static SLONG get_long();
static void db_error(int);
static void db_open(const char*, USHORT);
static PAG db_read(SLONG);

static FILE *trace;
static int file;

// Physical IO trace events

//const SSHORT trace_create	= 1;
const SSHORT trace_open		= 2;
const SSHORT trace_page_size	= 3;
const SSHORT trace_read		= 4;
const SSHORT trace_write	= 5;
const SSHORT trace_close	= 6;

static USHORT page_size;
static pag* global_buffer;

const int MAX_PAGES	= 50000;

static USHORT read_counts[MAX_PAGES], write_counts[MAX_PAGES];


void main( int argc, char **argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Replay all I/O to compute overhead of I/O system.
 *
 **************************************/

	bool detail = true;

	char** end;
	for (end = argv + argc, ++argv; argv < end; argv++)
	{
		const char* s = *argv;
		if (*s++ == '-')
		{
			if (UPPER(*s) == 'S')
				detail = false;
		}
	}

	SLONG reads = 0, writes = 0;
	trace = fopen("trace.log", "r");
	page_size = 1024;
	SLONG sequence = 0;

	struct tms before;
	time_t elapsed = times(&before);

	SCHAR string[128] = "";

	const pag* page;
	SSHORT event;
	while ((event = getc(trace)) != trace_close && event != EOF)
	{
		switch (event)
		{
		case trace_open:
			{
				const SLONG length = getc(trace);
				SLONG n = length;
				SCHAR* p = string;
				while (--n >= 0)
					*p++ = getc(trace);
				*p = 0;
				db_open(string, length);
			}
			break;

		case trace_page_size:
			page_size = get_long();
			if (global_buffer)
				free(global_buffer);
			break;

		case trace_read:
			{
				const SLONG n = get_long();
				if (n < MAX_PAGES)
					++read_counts[n];

				if (detail && (page = db_read(n)))
					analyse(n, "Read", page, ++sequence);
				reads++;
			}
			break;

		case trace_write:
			{
				const SLONG n = get_long();
				if (n < MAX_PAGES)
					++write_counts[n];

				if (detail && (page = db_read(n)))
					analyse(n, "Write", page, ++sequence);
				writes++;
			}
			break;

		default:
			printf("don't understand event %d\n", event);
			abort();
		}
	}

	struct tms after;
	elapsed = times(&after) - elapsed;
	const SLONG cpu = after.tms_utime - before.tms_utime;
	const SLONG system = after.tms_stime - before.tms_stime;

	printf
		("File: %s:\n elapsed = %d.%.2d, cpu = %d.%.2d, system = %d.%.2d, reads = %d, writes = %d\n",
		 string, elapsed / 60, (elapsed % 60) * 100 / 60, cpu / 60,
		 (cpu % 60) * 100 / 60, system / 60, (system % 60) * 100 / 60, reads,
		 writes);

	printf("High activity pages:\n");

	const USHORT *r, *w;
	SLONG n;

	for (r = read_counts, w = write_counts, n = 0; n < MAX_PAGES; n++, r++, w++)
	{
		if (*r > 1 || *w > 1)
		{
			sprintf(string, "  Read: %d, write: %d", *r, *w);
			if (page = db_read(n))
				analyse(n, string, page, 0);
		}
	}
}


static void analyse( int number, const SCHAR* string, const pag* page, int sequence)
{
/**************************************
 *
 *	a n a l y s e
 *
 **************************************
 *
 * Functional description
 *	Analyse a page event.
 *
 **************************************/

	if (sequence)
		printf("%d.  %s\t%d\t\t", sequence, string, number);
	else
		printf("%s\t%d\t\t", string, number);

	switch (page->pag_type)
	{
	case pag_header:
		printf("Header page\n");
		break;

	case pag_pages:
		printf("Page inventory page\n");
		break;

	case pag_transactions:
		printf("Transaction inventory page\n");
		break;

	case pag_pointer:
		printf("Pointer page, relation %d, sequence %d\n",
				  ((pointer_page*) page)->ppg_relation, ((pointer_page*) page)->ppg_sequence);
		break;

	case pag_data:
		printf("Data page, relation %d, sequence %d\n",
				  ((data_page*) page)->dpg_relation, ((data_page*) page)->dpg_sequence);
		break;

	case pag_root:
		printf("Index root page, relation %d\n",
				  ((index_root_page*) page)->irt_relation);
		break;

	case pag_index:
		printf("B-Tree page, relation %d, index %d, level %d\n",
				  ((btree_page*) page)->btr_relation, ((btree_page*) page)->btr_id,
				  ((btree_page*) page)->btr_level);
		break;

	case pag_blob:
		printf("Blob page\n\tFlags: %x, lead page: %d, sequence: %d, length: %d\n\t",
			page->pag_flags, ((blob_page*) page)->blp_lead_page,
			((blob_page*) page)->blp_sequence, ((blob_page*) page)->blp_length);
		break;

	default:
		printf("Unknown type %d\n", page->pag_type);
		break;
	}
}


static SLONG get_long()
{
/**************************************
 *
 *	g e t _ l o n g
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	union {
		SLONG l;
		SSHORT i;
		SCHAR c;
	} value;

	SCHAR* p = (SCHAR *) & value.l;
	SLONG i = getc(trace);
	const SLONG x = i;

	while (--i >= 0)
		*p++ = getc(trace);

	if (x == 1)
		return value.c;

	if (x == 2)
		return value.i;

	return value.l;
}


static void db_error( int status)
{
/**************************************
 *
 *	d b _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	printf(strerror(status));
	abort();
}


static void db_open( const char* file_name, USHORT file_length)
{
/**************************************
 *
 *	d b _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *
 **************************************/

	if ((file = open(file_name, 2)) == -1)
		db_error(errno);
}


static PAG db_read( SLONG page_number)
{
/**************************************
 *
 *	d b _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/

	const FB_UINT64 offset = ((FB_UINT64) page_number) * ((FB_UINT64) page_size);

	if (!global_buffer)
		global_buffer = (pag*) malloc(page_size);

	if (lseek (file, offset, 0) == -1)
		db_error(errno);

	if (read(file, global_buffer, page_size) == -1)
		db_error(errno);

	return global_buffer;
}


#ifndef HAVE_TIMES
static time_t times(struct tms* buffer)
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
	return buffer->tms_utime;
}
#endif
