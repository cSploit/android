/*
 *	PROGRAM:	JRD Rebuild scrambled database
 *	MODULE:		rebuild.cpp
 *	DESCRIPTION:	Main routine for analyzing and rebuilding database
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
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/jrd_time.h"
#include "../jrd/pag.h"
#include "../jrd/tra.h"
#include "../utilities/rebuild/rebuild.h"
#include "../utilities/rebuild/rebui_proto.h"
#include "../utilities/rebuild/rmet_proto.h"
#include "../utilities/rebuild/rstor_proto.h"
#include "../jrd/dmp_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h

#ifndef O_RDWR
#include <fcntl.h>
#endif

Database dbb_struct;
thread_db tdbb_struct, *gdbb;
PageControl dim;
jrd_tra dull;

const ULONG* tips;

FILE* dbg_file;

static void checksum(rbdb*, ULONG, ULONG, bool);
static USHORT compute_checksum(const rbdb*, PAG);
static void db_error(int);
static void dump(FILE*, rbdb*, ULONG, ULONG, UCHAR);
static void dump_tips(FILE*, rbdb*);
static void format_header(const rbdb*, header_page*, int, TraNumber, TraNumber, TraNumber, ULONG);
static void format_index_root(index_root_page*, int, SSHORT, SSHORT);
static void format_pointer(pointer_page*, int, SSHORT, SSHORT, bool, SSHORT, const SLONG*);
static void format_pip(page_inv_page*, int, int);
static void format_tip(tx_inv_page*, int, SLONG);
static void get_next_file(rbdb*, header_page*);
static void get_range(TEXT***, const TEXT* const* const, ULONG*, ULONG*);
static void get_switch(TEXT**, swc*);
static header_page* open_database(rbdb*, ULONG);
static void print_db_header(FILE*, const header_page*);
static void rebuild(rbdb*);
static void write_headers(FILE*, rbdb*, ULONG, ULONG);

static bool sw_rebuild;
static bool sw_print;
static bool sw_store;
static bool sw_dump_pages;
static bool sw_checksum;
//static bool sw_fudge;
static bool sw_fix;
static bool sw_dump_tips;

static const ULONG PPG_NUMBERS[] =
{
	5897, 6058, 5409, 6199, 6200, 6220, 6221,
	4739, 4868, 6332, 6333, 6329, 6359, 6751,
	6331, 6392, 6806, 6819, 6820, 6866, 6875,
	6876, 7019, 7284, 7430, 7431, 7757, 6893,
	6894, 7408, 8308, 8309, 1036, 9120, 4528,
	4563, 4572, 0, 0
};


int main( int argc, char *argv[])
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Parse and interpret command line, then do a variety
 *	of things.
 *
 **************************************/
	TEXT out_file[128];

	dbg_file = stdout;
	sw_rebuild = sw_print = sw_store = sw_dump_pages = sw_checksum = false;
	sw_dump_tips = /*sw_fudge = */ sw_fix = false;

	ULONG c_lower_bound, c_upper_bound, d_lower_bound, d_upper_bound,
		p_lower_bound, p_upper_bound, pg_size;
	pg_size = p_lower_bound = c_lower_bound = d_lower_bound = 0;
	p_upper_bound = c_upper_bound = d_upper_bound = BIG_NUMBER;
	USHORT pg_type = 0;

	const TEXT* const* const end = argv + argc;
	++argv;
	swc switch_space;
	swc* token = &switch_space;

	rbdb* rbdb = NULL;
	header_page* header = NULL;
	TEXT* ascii_out = NULL;
	TEXT* db_in = NULL;

	while (argv < end)
	{
		get_switch(argv, token);
		argv++;
		if (!token->swc_switch)
			db_in = token->swc_string;
		else
		{
			switch (*token->swc_string)
			{
			case 'b':
				pg_size = atoi(*argv++);
				break;

			case 'c':
				sw_checksum = true;
				get_range(&argv, end, &c_lower_bound, &c_upper_bound);
				break;

			case 'd':
				if ((argv < end) && (!strcmp(*argv, "tips")))
				{
					argv++;
					sw_dump_tips = true;
				}
				else
					sw_dump_pages = true;

				get_range(&argv, end, &d_lower_bound, &d_upper_bound);
				break;

			case 'f':
				sw_fix = true;
				break;

			case 'o':
				if (argv < end)
				{
					get_switch(argv, token);
					if (token->swc_switch)
						break;
					fb_utils::copy_terminate(out_file, token->swc_string, sizeof(out_file));
					ascii_out = out_file;
					argv++;
				}
			case 'p':
				sw_print = true;
				get_range(&argv, end, &p_lower_bound, &p_upper_bound);
				break;

			case 'r':
				sw_rebuild = true;
				break;

			case 's':
				sw_store = true;
				break;

			case 't':
				pg_type = atoi(*argv++);
				break;

			}
		}
	}

	if (db_in)
	{
		rbdb = (rbdb*) RBDB_alloc((SLONG) (sizeof(struct rbdb) + strlen(db_in)));
		strcpy(rbdb->rbdb_file.fil_name, db_in);
		rbdb->rbdb_file.fil_length = strlen(db_in);
		if (header = open_database(rbdb, pg_size))
			get_next_file(rbdb, header);

		// some systems don't care for this write sharing stuff...
		if (rbdb && (sw_dump_tips || sw_dump_pages))
		{
			RBDB_close(rbdb);
			if (rbdb->rbdb_valid)
				tips = RMET_tips(db_in);
			RBDB_open(rbdb);
		}
	}


	gdbb = &tdbb_struct;
	gdbb->tdbb_database = &dbb_struct;
	gdbb->tdbb_transaction = &dull;
	dull.tra_number = header->hdr_next_transaction;
	gdbb->tdbb_database->dbb_max_records = (rbdb->rbdb_page_size - sizeof(struct data_page)) /
		(sizeof(data_page::dpg_repeat) + OFFSETA(RHD, rhd_data));
	gdbb->tdbb_database->dbb_pcontrol = &dim;
	gdbb->tdbb_database->dbb_dp_per_pp =
		(rbdb->rbdb_page_size - OFFSETA(pointer_page*, ppg_page)) * 8 / 34;
	gdbb->tdbb_database->dbb_pcontrol->pgc_bytes =
		rbdb->rbdb_page_size - OFFSETA(page_inv_page*, pip_bits);
	gdbb->tdbb_database->dbb_pcontrol->pgc_ppp = gdbb->tdbb_database->dbb_pcontrol->pgc_bytes * 8;
	gdbb->tdbb_database->dbb_pcontrol->pgc_tpt =
		(rbdb->rbdb_page_size - OFFSETA(tx_inv_page*, tip_transactions)) * 4;
	gdbb->tdbb_database->dbb_pcontrol->pgc_pip = 1;

	if (ascii_out)
		dbg_file = fopen(ascii_out, "w");

	if (sw_print && rbdb && header)
		write_headers(dbg_file, rbdb, p_lower_bound, p_upper_bound);

	if (sw_store && rbdb && header)
		RSTORE(rbdb);

	if (sw_checksum && rbdb && header)
		checksum(rbdb, c_lower_bound, c_upper_bound, sw_fix);

	if (sw_dump_tips && rbdb && header)
		dump_tips(dbg_file, rbdb);

	if (sw_dump_pages && rbdb && header)
		dump(dbg_file, rbdb, d_lower_bound, d_upper_bound, pg_type);

	if (sw_rebuild && rbdb && header)
		rebuild(rbdb);

	if (ascii_out)
		fclose(dbg_file);

	if (rbdb)
		RBDB_close(rbdb);

	while (rbdb)
	{
		rbdb* const next_db = rbdb->rbdb_next;
		if (rbdb->rbdb_buffer1)
			gds__free(rbdb->rbdb_buffer1);
		if (rbdb->rbdb_buffer2)
			gds__free(rbdb->rbdb_buffer2);
		gds__free(rbdb);
		rbdb = next_db;
	}

    return 0;
}


#ifdef HPUX
PAG CCH_fetch(WIN* x, USHORT y, int z)
{
/**************************************
 *
 *	C C H _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	This routine lets me link in
 *	DMP.C from JRD without having the
 *	linker piss all over it.
 *
 **************************************/
}


PAG CCH_release(WIN* x)
{
/**************************************
 *
 *	C C H _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	This routine lets me link in
 *	DMP.C from JRD without having the
 *	linker piss all over it.
 *
 **************************************/
}
#endif


void* RBDB_alloc(SLONG size)
{
/**************************************
 *
 *	R B D B _ a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate and zero a piece of memory.
 *
 **************************************/
	return memset(gds__alloc(size), 0, size);
}


void RBDB_close( rbdb* rbdb)
{
/**************************************
 *
 *	R B D B _ c l o s e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	for (; rbdb; rbdb = rbdb->rbdb_next)
		close(rbdb->rbdb_file.fil_file);
}


void RBDB_open( rbdb* rbdb)
{
/**************************************
 *
 *	R B D B _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *
 **************************************/
	if ((rbdb->rbdb_file.fil_file = open(rbdb->rbdb_file.fil_name, O_RDWR, 0)) == -1)
	{
		db_error(errno);
	}
}


PAG RBDB_read(rbdb* rbdb, SLONG page_number)
{
/**************************************
 *
 *	R B D B _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/
	int file = rbdb->rbdb_file.fil_file;

	const FB_UINT64 offset = ((FB_UINT64) page_number) * ((FB_UINT64) rbdb->rbdb_page_size);
	if (lseek (file, offset, 0) == -1)
		db_error(errno);

	SSHORT length = rbdb->rbdb_page_size;
	for (char* p = (SCHAR *) rbdb->rbdb_buffer1; length > 0;)
	{
		const SSHORT l = read(file, p, length);
		if (l < 0)
			db_error(errno);
		else if (l == 0)
			return NULL;
		p += l;
		length -= l;
	}
	rbdb->rbdb_file.fil_file = file;
	return rbdb->rbdb_buffer1;
}


void RBDB_write( rbdb* rbdb, PAG page, SLONG page_number)
{
/**************************************
 *
 *	R B D B _ w r i t e   ( u n i x )
 *
 **************************************
 *
 * Functional description
 *	Write a database page.
 *
 **************************************/
	page->pag_checksum = compute_checksum(rbdb, page);
	const ULONG page_size = rbdb->rbdb_page_size;
	int fd = rbdb->rbdb_file.fil_file;

	const FB_UINT64 offset = ((FB_UINT64) page_number) * ((FB_UINT64) page_size);
	if (lseek (fd, offset, 0) == -1)
		db_error(errno);
	if (write(fd, page, page_size) == -1)
		db_error(errno);
}


static void checksum( rbdb* rbdb, ULONG lower, ULONG upper, bool sw_fix)
{
/**************************************
 *
 *	c h e c k s u m
 *
 **************************************
 *
 * Functional description
 *	read, compute, check, and correct
 *	checksums in this database.
 *
 **************************************/
	TEXT s[128];

	for (ULONG page_number = lower; page_number <= upper; page_number++)
	{
		pag* page = RBDB_read(rbdb, page_number);
		if (!page)
			return;
		const USHORT old_checksum = page->pag_checksum;
		const USHORT new_checksum = compute_checksum(rbdb, page);
		if (sw_fix)
			page->pag_checksum = new_checksum;
		if (new_checksum == old_checksum)
			sprintf(s, "checksum %5d is OK", old_checksum);
		else
			sprintf(s, "stored checksum %5d\tcomputed checksum %5d\t%s",
					old_checksum, new_checksum, sw_fix ? "fixed" : "");
		printf("page %9d\t%s\n", page_number, s);
	}
}


static USHORT compute_checksum( const rbdb* rbdb, PAG page)
{
/**************************************
 *
 *	c o m p u t e _ c h e c k s u m
 *
 **************************************
 *
 * Functional description
 *	compute checksum for a V3 page.
 *
 **************************************/
	const ULONG* const end = (ULONG *) ((SCHAR *) page + rbdb->rbdb_page_size);
	const USHORT old_checksum = page->pag_checksum;
	page->pag_checksum = 0;
	const ULONG* p = (ULONG *) page;
	ULONG checksum = 0;

	do {
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
	} while (p < end);

	page->pag_checksum = old_checksum;

	if (checksum)
		return checksum;

	// If the page is all zeros, return an artificial checksum

	for (p = (ULONG *) page; p < end;)
	{
		if (*p++)
			return checksum;
	}

	// Page is all zeros -- invent a checksum

	return 12345;
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
	exit(FINI_ERROR);
}


static void dump(FILE* file, rbdb* rbdb, ULONG lower, ULONG upper, UCHAR pg_type)
{
/**************************************
 *
 *	d u m p
 *
 **************************************
 *
 * Functional description
 *	dump the contents of some page
 *	or pages in the database.
 *
 **************************************/
	ULONG sequence = 0;

	if (rbdb->rbdb_last_page && upper == BIG_NUMBER)
		upper = rbdb->rbdb_last_page;

	PAG page;
	while (page = RBDB_read(rbdb, lower))
	{
		if (page->pag_type == pag_transactions && tips)
		{
			for (const ULONG* tip = tips; tip[sequence]; sequence++)
			{
				if (tip[sequence] == lower)
					break;
				else if (!tip[sequence])
				{
					sequence = 0;
					break;
				}
			}
		}
		else
			sequence = 0;
		if (pg_type && (page->pag_type != pg_type))
		{
			fprintf(file, "\nChanging page %d type from %d to %d\n", lower, page->pag_type, pg_type);
			page->pag_type = pg_type;
		}
		DMP_fetched_page(page, lower, sequence, rbdb->rbdb_page_size);
		const ULONG* p = (ULONG *) page;
		const ULONG* const end = p + (rbdb->rbdb_page_size / sizeof(ULONG)) - 1;
		while (!*p && p < end)
			++p;
		if (!*p)
			fprintf(file, "    Page is all zeroes.\n");
		// This cannot be true because sw_fudge is never activated
		//if (sw_fudge)
		//	RBDB_write(rbdb, page, lower);
		if (++lower > upper)
			break;
	}
}


static void dump_tips( FILE* file, rbdb* rbdb)
{
/**************************************
 *
 *	d u m p _ t i p s
 *
 **************************************
 *
 * Functional description
 *	dump the contents of tip pages
 *	in the database.
 *
 **************************************/
	if (!tips)
		fprintf(file, "not enough database.  Store headers and look there\n");

	PAG page;
	ULONG sequence = 1;
	for (const ULONG* tip = tips; *tip && (page = RBDB_read(rbdb, *tip)); sequence++)
	{
		DMP_fetched_page(page, *tip++, sequence, rbdb->rbdb_page_size);
	}
}


static void format_header(const rbdb* rbdb, header_page* page, int page_size,
	TraNumber oldest, TraNumber active, TraNumber next, ULONG imp)
{
/**************************************
 *
 *	f o r m a t _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Format a header page from inputs
 *
 **************************************/
	memset(page, 0, page_size);
	page->hdr_page_size = page_size;
	page->pag_type = pag_header;
	page->hdr_ods_version = ODS_VERSION | ODS_TYPE_CURRENT;
	page->hdr_PAGES = 2;
	page->hdr_oldest_transaction = oldest;
	page->hdr_oldest_active = active;
	page->hdr_next_transaction = next;
	page->hdr_implementation = imp;
	page->pag_checksum = compute_checksum(rbdb, page);
}


static void format_index_root(index_root_page* page,
							  int page_size, SSHORT relation_id, SSHORT count)
{
/**************************************
 *
 *	f o r m a t _ i n d e x _ r o o t
 *
 **************************************
 *
 * Functional description
 *	Format an index root page, without any indexes for a start
 *
 **************************************/
	memset(page, 0, page_size);
	page->pag_type = pag_root;
	page->irt_relation = relation_id;
	page->irt_count = count;
}


// Unused function.
static void format_pointer(pointer_page* page,
						   int page_size,
						   SSHORT relation_id,
						   SSHORT sequence,
						   bool eof, SSHORT count,
						   const SLONG* page_vector)
{
/**************************************
 *
 *	f o r m a t _ p o i n t e r
 *
 **************************************
 *
 * Functional description
 *	Format a pointer page.  In addition of the buffer and page size,
 *	we need a relation id, a pointer page sequence (within relation),
 *	a flag indicated whether this is the last pointer page for the
 *	relation, and a count and vector of pages to point to.
 *
 **************************************/
	memset(page, 0, page_size);
	page->pag_type = pag_pointer;
	page->ppg_sequence = sequence;
	page->ppg_relation = relation_id;
	page->ppg_count = count;
	page->ppg_min_space = 0;
	page->ppg_max_space = count;
	if (eof)
		page->pag_flags |= ppg_eof;
	memcpy(page->ppg_page, page_vector, count * sizeof(SLONG));
}


static void format_pip( page_inv_page* page, int page_size, int last_flag)
{
/**************************************
 *
 *	f o r m a t _ p i p
 *
 **************************************
 *
 * Functional description
 *	Fake a fully RBDB_allocated (all pages RBDB_allocated) page inventory
 *	page.
 *
 **************************************/
	page->pag_type = pag_pages;
	page->pag_flags = 0;

	// Set all page bits to zero, indicating RBDB_allocated

	const SSHORT bytes = page_size - OFFSETA(page_inv_page*, pip_bits);
	memset(page->pip_bits, 0, bytes);

	// If this is the last pip, make sure the last page (which
	// will become the next pip) is marked free.  When the
	// time comes to RBDB_allocate the next page, that page will
	// be formatted as the next pip.

	if (last_flag)
		page->pip_bits[bytes - 1] |= 1 << 7;
}


static void format_tip( tx_inv_page* page, int page_size, SLONG next_page)
{
/**************************************
 *
 *	f o r m a t _ t i p
 *
 **************************************
 *
 * Functional description
 *	Fake a fully commit transaction inventory page.
 *
 **************************************/
	page->pag_type = pag_transactions;
	page->pag_flags = 0;

	// The "next" tip page number is included for redundancy, but is not actually
	// read by the engine, so can be safely left zero.  If known, it would nice
	// to supply it.

	page->tip_next = next_page;

	// Code for committed transaction is 3, so just fill all bytes with -1

	const SSHORT bytes = page_size - OFFSETA(tx_inv_page*, tip_transactions);
	memset(page->tip_transactions, -1, bytes);
}


static void get_next_file( rbdb* rbdb, header_page* header)
{
/**************************************
 *
 *	g e t _ n e x t _ f i l e
 *
 **************************************
 *
 * Functional description
 *	If there's another file as part of
 *	this database, get it now.
 *
 **************************************/
	rbdb** next = &rbdb->rbdb_next;
	const UCHAR* p = header->hdr_data;
	for (const UCHAR* const end = p + header->hdr_page_size; p < end && *p != HDR_end; p += 2 + p[1])
	{
		if (*p == HDR_file)
		{
			rbdb* next_rbdb = (rbdb*) RBDB_alloc(sizeof(struct rbdb) + (SSHORT) p[1]);
			next_rbdb->rbdb_file.fil_length = (SSHORT) p[1];
			strncpy(next_rbdb->rbdb_file.fil_name, p + 2, (SSHORT) p[1]);
			*next = next_rbdb;
			next = &next_rbdb->rbdb_next;
			break;
		}
	}
}


static void get_range(TEXT*** argv, const TEXT* const* const end, ULONG* lower, ULONG* upper)
{
/**************************************
 *
 *	g e t _ r a n g e
 *
 **************************************
 *
 * Functional description
 *	get a range out of the argument
 *	vector;
 *
 **************************************/
	struct swc token_space;
	swc* token = &token_space;

	if (*argv < end)
	{
		get_switch(*argv, token);
		if (token->swc_switch)
			return;
		++*argv;
		TEXT c = 0;
		const TEXT* p;
		for (p = token->swc_string; *p; p++)
			if (*p < '0' || *p > '9')
			{
				c = *p;
				*p++ = 0;
				break;
			}
		*upper = *lower = (ULONG) atoi(token->swc_string);
		if (*p && (c == ':' || c == ','))
		{
			if (*p == '*')
				*upper = BIG_NUMBER;
			else
				*upper = (ULONG) atoi(p);
			return;
		}
	}
	if (*argv < end)
	{
		get_switch(*argv, token);
		if (token->swc_switch)
			return;
		if ((*token->swc_string == ':') || (*token->swc_string == ','))
		{
			const TEXT* p = token->swc_string;
			if (*++p)
			{
				if (*p == '*')
					*upper = BIG_NUMBER;
				else
					*upper = (ULONG) atoi(p);
			}
			else if (*argv++ < end)
			{
				get_switch(*argv, token);
				if (token->swc_switch)
					return;
			}
		}
		if (*token->swc_string == '*')
			*upper = BIG_NUMBER;
		else
			*upper = (ULONG) atoi(token->swc_string);
		*argv++;
	}
}


static void get_switch( TEXT** argv, swc* token)
{
/**************************************
 *
 *	g e t _ s w i t c h
 *
 **************************************
 *
 * Functional description
 *	get the next argument in the argument
 * 	vector;
 *
 **************************************/
	token->swc_string = *argv;

	if (*token->swc_string == '-')
	{
		token->swc_switch = true;
		token->swc_string++;
	}
	else
		token->swc_switch = false;

	const int temp = strlen(token->swc_string) - 1;

	if (token->swc_string[temp] == ',')
	{
		token->swc_string[temp] = '\0';
		//token->swc_comma = true;
	}
	//else
	//	token->swc_comma = false;
}


static header_page* open_database( rbdb* rbdb, ULONG pg_size)
{
/**************************************
 *
 *	o p e n _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Open a database and setup the
 *	rbdb.  Return a pointer to the
 *	header page page header;
 *
 **************************************/
	UCHAR temp[1024];

	RBDB_open(rbdb);

	rbdb->rbdb_page_size = sizeof(temp);
	rbdb->rbdb_buffer1 = (PAG) temp;
	rbdb->rbdb_valid = true;

	header_page* header = (header_page*) RBDB_read(rbdb, (SLONG) 0);

	if (header->pag_type != pag_header)
	{
		printf("header page has wrong type, expected %d found %d!\n", pag_header, header->pag_type);
		rbdb->rbdb_valid = false;
	}

	if (header->hdr_ods_version != ODS_VERSION | ODS_TYPE_CURRENT)
	{
		printf("Wrong ODS version, expected %d type %04x, encountered %d type %04x.\n",
				  ODS_VERSION, ODS_TYPE_CURRENT,
				  header->hdr_ods_version & ~ODS_TYPE_MASK,
				  header->hdr_ods_version & ODS_TYPE_MASK
			  );
		rbdb->rbdb_valid = false;
	}

	if (pg_size && (pg_size != header->hdr_page_size))
	{
		printf("Using page size %d\n", pg_size);
		header->hdr_page_size = pg_size;
		rbdb->rbdb_valid = false;
	}
	else if (!header->hdr_page_size)
	{
		printf("Using page size 1024\n");
		header->hdr_page_size = 1024;
		rbdb->rbdb_valid = false;
	}

	printf("\nDatabase \"%s\"\n\n", rbdb->rbdb_file.fil_name);


	rbdb->rbdb_page_size = header->hdr_page_size;
	rbdb->rbdb_map_count = rbdb->rbdb_map_length / rbdb->rbdb_page_size;
	rbdb->rbdb_buffer1 = (PAG) RBDB_alloc(rbdb->rbdb_page_size);
	rbdb->rbdb_buffer2 = (PAG) RBDB_alloc(rbdb->rbdb_page_size);

	return header;
}


static void print_db_header( FILE* file, const header_page* header)
{
/**************************************
 *
 *	p r i n t _ d b _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Print database header page.
 *
 **************************************/
	fprintf(file, "Database header page information:\n");
	fprintf(file, "    Page size\t\t\t%d\n", header->hdr_page_size);
	fprintf(file, "    ODS version\t\t\t%d type %04x\n",
		header->hdr_ods_version & ~ODS_TYPE_MASK,
		header->hdr_ods_version & ODS_TYPE_MASK);
	fprintf(file, "    PAGES\t\t\t%d\n", header->hdr_PAGES);
	fprintf(file, "    next page\t\t\t%d\n", header->hdr_next_page);
	fprintf(file, "    Oldest transaction\t\t%lu\n", header->hdr_oldest_transaction);
	fprintf(file, "    Oldest active\t\t%lu\n", header->hdr_oldest_active);
	fprintf(file, "    Oldest snapshot\t\t%lu\n", header->hdr_oldest_snapshot);
	fprintf(file, "    Next transaction\t\t%lu\n", header->hdr_next_transaction);

	fprintf(file, "    Data pages per pointer page\t%ld\n", gdbb->tdbb_database->dbb_dp_per_pp);
	fprintf(file, "    Max records per page\t%ld\n", gdbb->tdbb_database->dbb_max_records);

	//fprintf ("    Sequence number    %d\n", header->hdr_sequence);
	//fprintf ("    Creation date    \n", header->hdr_creation_date);

	fprintf(file, "    Next attachment ID\t\t%ld\n", header->hdr_attachment_id);
	fprintf(file, "    Implementation ID\t\t%ld\n", header->hdr_implementation);
	fprintf(file, "    Shadow count\t\t%ld\n", header->hdr_shadow_count);

	tm time;
	isc_decode_timestamp(header->hdr_creation_date, &time);

	fprintf(file, "    Creation date:\t\t%s %d, %d %d:%02d:%02d\n",
			   FB_SHORT_MONTHS[time.tm_mon], time.tm_mday, time.tm_year + 1900,
			   time.tm_hour, time.tm_min, time.tm_sec);
	fprintf(file, "    Cache buffers\t\t%ld\n", header->hdr_cache_buffers);

	fprintf(file, "\n    Variable header data:\n");

	SLONG number;

	const UCHAR* p = header->hdr_data;
	for (const UCHAR* const end = p + header->hdr_page_size;
		 p < end && *p != HDR_end;
		 p += 2 + p[1])
	{
		switch (*p)
		{
		case HDR_root_file_name:
			fprintf(file, "\tRoot file name: %*s\n", p[1], p + 2);
			break;
/*
		case HDR_journal_server:
			fprintf(file, "\tJournal server: %*s\n", p[1], p + 2);
			break;
*/
		case HDR_file:
			fprintf(file, "\tContinuation file: %*s\n", p[1], p + 2);
			break;

		case HDR_last_page:
			memcpy(&number, p + 2, sizeof(number));
			fprintf(file, "\tLast logical page: %ld\n", number);
			break;
/*
		case HDR_unlicensed:
			memcpy(&number, p + 2, sizeof(number));
			fprintf(file, "\tUnlicensed accesses: %ld\n", number);
			break;
*/
		case HDR_sweep_interval:
			memcpy(&number, p + 2, sizeof(number));
			fprintf(file, "\tSweep interval: %ld\n", number);
			break;
/*
		case HDR_log_name:
			fprintf(file, "\tReplay logging file: %*s\n", p[1], p + 2);
			break;

		case HDR_journal_file:
			fprintf(file, "\tJournal file: %*s\n", p[1], p + 2);
			break;
*/
		case HDR_password_file_key:
			fprintf(file, "\tPassword file key: (can't print)\n");
			break;
/*
		case HDR_backup_info:
			fprintf(file, "\tBackup info: (can't print)\n");
			break;

		case HDR_cache_file:
			fprintf(file, "\tShared cache file: %*s\n", p[1], p + 2);
			break;
*/
		default:
			fprintf(file, "\tUnrecognized option %d, length %d\n", p[0], p[1]);
		}
	}

	fprintf(file, "\n\n");
}


static void rebuild( rbdb* rbdb)
{
/**************************************
 *
 *	r e b u i l d
 *
 **************************************
 *
 * Functional description
 *	Write out an improved database.
 *
 **************************************/
	const ULONG page_size = rbdb->rbdb_page_size;
	pag* page = rbdb->rbdb_buffer1;
	const ULONG* page_numbers = PPG_NUMBERS;
	for (ULONG number = 5898; (number < 5899) && (page = RBDB_read(rbdb, number)); number++)
	{
		pointer_page* pointer = (pointer_page*) page;
		// format_pointer (page, page_size, 25, 3, true, 37, page_numbers);

		RBDB_write(rbdb, page, number);
	}
}


static void write_headers(FILE* file, rbdb* rbdb, ULONG lower, ULONG upper)
{
/**************************************
 *
 *	w r i t e _ h e a d e r s
 *
 **************************************
 *
 * Functional description
 *	Print out the page headers.
 *
 **************************************/
	const pag* page;
	for (ULONG page_number = lower;
		(page_number <= upper) && (page = RBDB_read(rbdb, page_number));
		page_number++)
	{
		fprintf(file, "page %d, ", page_number);

		switch (page->pag_type)
		{
		case pag_header:
			fprintf(file, "header page, checksum %d\n", page->pag_checksum);
			print_db_header(file, (header_page*) page);
			break;

		case pag_pages:
			{
				fprintf(file, "page inventory page, checksum %d\n", page->pag_checksum);
				const page_inv_page* pip = (page_inv_page*) page;
				fprintf(file, "\tlowest free page %d\n\n", pip->pip_min);
			}
			break;

		case pag_transactions:
			fprintf(file, "TIP page, checksum %d\n", page->pag_checksum);
			fprintf(file, "\tnext tip for database %ld\n\n", ((tx_inv_page*) page)->tip_next);
			break;

		case pag_pointer:
			{
				fprintf(file, "pointer page, checksum %d\n", page->pag_checksum);
				const pointer_page* pointer = (pointer_page*) page;
				fprintf(file, "\trelation %d, sequence %ld, next pip %ld, active slots %d\n",
						   pointer->ppg_relation, pointer->ppg_sequence,
						   pointer->ppg_next, pointer->ppg_count);
				fprintf(file, "\tfirst slot with space %d, last slot with space %d\n",
						   pointer->ppg_min_space, pointer->ppg_max_space);
				fprintf(file, "\t%s\n",
						   (pointer->pag_flags & ppg_eof) ? "last pointer for relation\n" : "");
			}
			break;

		case pag_data:
			{
				fprintf(file, "data page, checksum %d\n", page->pag_checksum);
				const data_page* data = (data_page*) page;
				fprintf(file, "\trelation %d, sequence %ld, records on page %d\n",
						   data->dpg_relation, data->dpg_sequence, data->dpg_count);
				fprintf(file, "\t%s%s%s%s\n",
						   (data->pag_flags & dpg_orphan) ? "orphan " : "",
						   (data->pag_flags & dpg_full) ? "full " : "",
						   (data->pag_flags & dpg_large) ? "contains a large object" : "",
						   (data->pag_flags) ? "\n" : "");
			}
			break;

		case pag_root:
			{
				fprintf(file, "index root page, checksum %d\n", page->pag_checksum);
				const index_root_page* index_root = (index_root_page*) page;
				fprintf(file, "\trelation %d, number of indexes %d\n\n",
						   index_root->irt_relation, index_root->irt_count);
			}
			break;

		case pag_index:
			{
				fprintf(file, "btree page (bucket), checksum %d\n", page->pag_checksum);
				const btree_page* bucket = (btree_page*) page;
				fprintf(file, "\trelation %d, right sibling bucket: %ld,\n",
						   bucket->btr_relation, bucket->btr_sibling);
				fprintf(file, "\tdata length %d, index id %d, level %d\n",
						   bucket->btr_length, bucket->btr_id, bucket->btr_level);
				fprintf(file, "\t%s%s%s\n",
						   (bucket->pag_flags & btr_leftmost) ? "leftmost " : "",
						   (bucket->pag_flags & btr_not_prop) ? "all duplicates " : "",
						   (bucket->Pag_flags & btr_marked) ? "marked for delete" : "");
			}
			break;

		case pag_blob:
			{
				fprintf(file, "blob page, checksum %d\n", page->pag_checksum);
				const blob_page* blob = (blob_page*) page;
				fprintf(file, "\tlead page: %ld, sequence: %ld, length: %d\n",
						   blob->blp_lead_page, blob->blp_sequence, blob->blp_length);
				fprintf(file, "\tcontains %s\n",
						   (blob->pag_flags & blp_pointers) ? "pointers" : "data");
			}
			break;

		case pag_ids:
			fprintf(file, "generator page, checksum %d\n\n", page->pag_checksum);
			break;

		case pag_log:
			fprintf(dbg_file, "write-ahead log info page, checksum %d\n\n", page->pag_checksum);
			break;

		default:
			fprintf(file, "unknown page type\n\n");
			break;
		}
	}
}

