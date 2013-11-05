/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dpm.epp
 *	DESCRIPTION:	Data page manager
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

/*
 * Modified by: Patrick J. P. Griffin
 * Date: 11/29/2000
 * Problem:   Bug 116733 Too many generators corrupt database.
 *            DPM_gen_id was not calculating page and offset correctly.
 * Change:    Corrected routine to use new variables from PAG_init.
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/req.h"
#include "../jrd/ibase.h"
#include "../jrd/sqz.h"
#include "../jrd/irq.h"
#include "../jrd/blb.h"
#include "../jrd/tra.h"
#include "../jrd/lls.h"
#include "../jrd/lck.h"
#include "../jrd/cch.h"
#include "../jrd/pag.h"
#include "../jrd/rse.h"
#include "../jrd/val.h"
#ifdef VIO_DEBUG
#include "../jrd/vio_debug.h"
#endif
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/sqz_proto.h"
#include "../common/StatusArg.h"

#ifdef DEV_BUILD
#include "../jrd/dbg_proto.h"
#endif

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [94] =
   {	// blr string 

4,2,4,0,5,0,8,0,8,0,7,0,7,0,7,0,2,7,'C',1,'K',0,0,0,255,14,0,
2,1,24,0,0,0,25,0,0,0,1,24,0,2,0,25,0,1,0,1,21,8,0,1,0,0,0,25,
0,2,0,1,24,0,3,0,25,0,3,0,1,24,0,1,0,25,0,4,0,255,14,0,1,21,8,
0,0,0,0,0,25,0,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_7 [61] =
   {	// blr string 

4,2,4,0,4,0,8,0,8,0,7,0,7,0,12,0,15,'K',0,0,0,2,1,25,0,0,0,24,
0,0,0,1,25,0,1,0,24,0,2,0,1,25,0,2,0,24,0,3,0,1,25,0,3,0,24,0,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_13 [95] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',0,0,0,'G',47,24,0,1,0,25,0,0,0,255,2,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 


#define DECOMPOSE(n, divisor, q, r) {r = n % divisor; q = n / divisor;}
//#define DECOMPOSE_QUOTIENT(n, divisor, q) {q = n / divisor;}
#define HIGH_WATER(x)	((SSHORT) sizeof (data_page) + (SSHORT) sizeof (data_page::dpg_repeat) * (x - 1))
#define SPACE_FUDGE	RHDF_SIZE

using namespace Jrd;
using namespace Ods;
using namespace Firebird;

static void delete_tail(thread_db*, rhdf*, const USHORT, USHORT);
static void fragment(thread_db*, record_param*, SSHORT, DataComprControl*, SSHORT, const jrd_tra*);
static void extend_relation(thread_db*, jrd_rel*, WIN*);
static UCHAR* find_space(thread_db*, record_param*, SSHORT, PageStack&, Record*, USHORT);
static bool get_header(WIN*, SSHORT, record_param*);
static pointer_page* get_pointer_page(thread_db*, jrd_rel*, RelationPages*, WIN*, USHORT, USHORT);
static rhd* locate_space(thread_db*, record_param*, SSHORT, PageStack&, Record*, USHORT);
static void mark_full(thread_db*, record_param*);
static void store_big_record(thread_db*, record_param*, PageStack&, DataComprControl*, USHORT);


PAG DPM_allocate(thread_db* tdbb, WIN* window)
{
/**************************************
 *
 *	D P M _ a l l o c a t e
 *
 **************************************
 *
 * Functional description
 *	Allocate a data page.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("DPM_allocate (window page %"SLONGFORMAT")\n",
				  window ? window->win_page.getPageNum() : 0);
	}
#endif

	pag* page = PAG_allocate(tdbb, window);

	return page;
}


void DPM_backout( thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	D P M _ b a c k o u t
 *
 **************************************
 *
 * Functional description
 *	Backout a record where the record and previous version are on
 *	the same page.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
		printf("DPM_backout (record_param %"QUADFORMAT"d)\n", rpb->rpb_number.getValue());
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT" back %"
			SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif

	CCH_MARK(tdbb, &rpb->getWindow(tdbb));
	data_page* page = (data_page*) rpb->getWindow(tdbb).win_buffer;
	data_page::dpg_repeat* index1 = page->dpg_rpt + rpb->rpb_line;
	data_page::dpg_repeat* index2 = page->dpg_rpt + rpb->rpb_b_line;
	*index1 = *index2;
	index2->dpg_offset = index2->dpg_length = 0;

	rhd* header = (rhd*) ((SCHAR *) page + index1->dpg_offset);
	header->rhd_flags &= ~(rhd_chain | rhd_gc_active);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    old record %"SLONGFORMAT":%d, new record %"SLONGFORMAT
			":%d, old dpg_count %d, ",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_b_page,
			 rpb->rpb_b_line, page->dpg_count);
	}
#endif

	// Check to see if the index got shorter
	USHORT n;
	for (n = page->dpg_count; --n;)
	{
		if (page->dpg_rpt[n].dpg_length)
			break;
	}

	page->dpg_count = n + 1;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
		printf("    new dpg_count %d\n", page->dpg_count);
#endif

	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
}


void DPM_backout_mark(thread_db* tdbb, record_param* rpb, const jrd_tra* transaction)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	WIN *window = &rpb->getWindow(tdbb);
	CCH_MARK(tdbb, window);

	data_page* page = (data_page*) window->win_buffer;
	data_page::dpg_repeat* index = page->dpg_rpt + rpb->rpb_line;
	rhd* header = (rhd*) ((SCHAR *) page + index->dpg_offset);

	header->rhd_flags |= rhd_gc_active;
	header->rhd_transaction = transaction->tra_number;

	CCH_RELEASE(tdbb, window);
}


double DPM_cardinality(thread_db* tdbb, jrd_rel* relation, const Format* format)
{
/**************************************
 *
 *	D P M _ c a r d i n a l i t y
 *
 **************************************
 *
 * Functional description
 *	Estimate cardinality for the given relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// Get the number of data-pages for this relation

	const SLONG dataPages = DPM_data_pages(tdbb, relation);

	// Calculate record count and total compressed record length
	// on the first data page

	USHORT recordCount = 0, recordLength = 0;
	RelationPages* relPages = relation->getPages(tdbb);
	vcl* vector = relPages->rel_pages;
	if (vector)
	{
		WIN window(relPages->rel_pg_space_id, (*vector)[0]);
		Ods::pointer_page* ppage =
		(Ods::pointer_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_pointer);
		if (!ppage)
		{
			 BUGCHECK(243);
			 // msg 243 missing pointer page in DPM_data_pages
		}

		const SLONG* page = ppage->ppg_page;
		if (*page)
		{
			Ods::data_page* dpage =
				(Ods::data_page*) CCH_HANDOFF(tdbb, &window, *page, LCK_read, pag_data);

			const data_page::dpg_repeat* index = dpage->dpg_rpt;
			const data_page::dpg_repeat* const end = index + dpage->dpg_count;
			for (; index < end; index++)
			{
				if (index->dpg_offset)
				{
					recordCount++;
					recordLength += index->dpg_length - RHD_SIZE;
				}
			}
		}
		CCH_RELEASE(tdbb, &window);
	}

	// AB: If we have only 1 data-page then the cardinality calculation
	// is to worse to be useful, therefore rely on the record count
	// from the data-page.

	if (dataPages == 1)
	{
		return (double) recordCount;
	}

	// Estimate total number of records for this relation

	if (!format)
	{
		format = relation->rel_current_format;
	}

	static const double DEFAULT_COMPRESSION_RATIO = 0.5;

	const USHORT compressedSize =
		recordCount ? recordLength / recordCount :
		format->fmt_length * DEFAULT_COMPRESSION_RATIO;

	const USHORT recordSize = sizeof(Ods::data_page::dpg_repeat) +
		ROUNDUP(compressedSize + RHD_SIZE, ODS_ALIGNMENT) +
		((dbb->dbb_flags & DBB_no_reserve) ? 0 : SPACE_FUDGE);

	return (double) dataPages * (dbb->dbb_page_size - DPG_SIZE) / recordSize;
}


bool DPM_chain( thread_db* tdbb, record_param* org_rpb, record_param* new_rpb)
{
/**************************************
 *
 *	D P M _ c h a i n
 *
 **************************************
 *
 * Functional description
 *	Start here with a plausible, but non-active record_param.
 *
 *	We need to create a new version of a record.  If the new version
 *	fits on the same page as the old record, things are simple and
 *	quick.  If not, return false and let somebody else suffer.
 *
 *	Note that we also return false if the record fetched doesn't
 *	match the state of the input rpb, or if there is no record for
 *	that record number.  The caller has to check the results to
 *	see what failed if false is returned.  At the moment, there is
 *	only one caller, VIO_erase.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("DPM_chain (org_rpb %"QUADFORMAT"d, new_rpb %"
			  QUADFORMAT"d)\n", org_rpb->rpb_number.getValue(),
			  new_rpb ? new_rpb->rpb_number.getValue() : 0);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    org record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT
			" back %"SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 org_rpb->rpb_page, org_rpb->rpb_line, org_rpb->rpb_transaction_nr,
			 org_rpb->rpb_b_page, org_rpb->rpb_b_line, org_rpb->rpb_f_page,
			 org_rpb->rpb_f_line, org_rpb->rpb_flags);

		if (new_rpb)
		{
			printf("    new record length %d transaction %"SLONGFORMAT
					  " flags %d\n",
					  new_rpb->rpb_length, new_rpb->rpb_transaction_nr,
					  new_rpb->rpb_flags);
		}
	}
#endif

	record_param temp = *org_rpb;
	DataComprControl dcc(*tdbb->getDefaultPool());
	const USHORT size = SQZ_length((SCHAR*) new_rpb->rpb_address, (int) new_rpb->rpb_length, &dcc);

	if (!DPM_get(tdbb, org_rpb, LCK_write))
	{
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    record not found in DPM_chain\n");
#endif
		return false;
	}

	// if somebody has modified the record since we looked last, stop now!

	if (temp.rpb_transaction_nr != org_rpb->rpb_transaction_nr ||
		temp.rpb_b_page != org_rpb->rpb_b_page ||
		temp.rpb_b_line != org_rpb->rpb_b_line)
	{
		CCH_RELEASE(tdbb, &org_rpb->getWindow(tdbb));
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    record changed in DPM_chain\n");
#endif
		return false;
	}


	if ((org_rpb->rpb_flags & rpb_delta) && temp.rpb_prior) {
		org_rpb->rpb_prior = temp.rpb_prior;
	}
	else if (org_rpb->rpb_flags & rpb_delta)
	{
		CCH_RELEASE(tdbb, &org_rpb->getWindow(tdbb));
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    record delta state changed\n");
#endif
		return false;
	}

	data_page* page = (data_page*) org_rpb->getWindow(tdbb).win_buffer;

	// If the record obviously isn't going to fit, don't even try

	if (size > dbb->dbb_page_size - (sizeof(data_page) + RHD_SIZE))
	{
		CCH_RELEASE(tdbb, &org_rpb->getWindow(tdbb));
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    insufficient room found in DPM_chain\n");
#endif
		return false;
	}

	// The record must be long enough to permit fragmentation later.  If it's
	// too small, compute the number of pad bytes required

	SLONG fill = (RHDF_SIZE - RHD_SIZE) - size;
	if (fill < 0 || (new_rpb->rpb_flags & rpb_deleted)) {
		fill = 0;
	}

	// Accomodate max record size i.e. 64K
	const SLONG length = ROUNDUP(RHD_SIZE + size + fill, ODS_ALIGNMENT);

	// Find space on page and open slot

	SSHORT slot = page->dpg_count;
	SSHORT space = dbb->dbb_page_size;
	SSHORT top = HIGH_WATER(page->dpg_count);
	SSHORT available = dbb->dbb_page_size - top;

	SSHORT n = 0;
	const data_page::dpg_repeat* index = page->dpg_rpt;
	for (const data_page::dpg_repeat* const end = index + page->dpg_count;
		 index < end; index++, n++)
	{
		if (!index->dpg_length && slot == page->dpg_count) {
			slot = n;
		}
		SSHORT offset;
		if (index->dpg_length && (offset = index->dpg_offset))
		{
			available -= ROUNDUP(index->dpg_length, ODS_ALIGNMENT);
			space = MIN(space, offset);
		}
	}

	if (slot == page->dpg_count)
	{
		top += sizeof(data_page::dpg_repeat);
		available -= sizeof(data_page::dpg_repeat);
	}

	// If the record doesn't fit, punt

	if (length > available)
	{
		CCH_RELEASE(tdbb, &org_rpb->getWindow(tdbb));
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    compressed page doesn't have room in DPM_chain\n");
#endif
		return false;
	}

	CCH_precedence(tdbb, &org_rpb->getWindow(tdbb), -org_rpb->rpb_transaction_nr);
	CCH_MARK(tdbb, &org_rpb->getWindow(tdbb));

	// Record fits, in theory.  Check to see if the page needs compression

	space -= length;
	if (space < top) {
		space = DPM_compress(tdbb, page) - length;
	}

	if (slot == page->dpg_count) {
		++page->dpg_count;
	}

	// Swap the old record into the new slot and the new record into the old slot

	new_rpb->rpb_b_page = new_rpb->rpb_page = org_rpb->rpb_page;
	new_rpb->rpb_b_line = slot;
	new_rpb->rpb_line = org_rpb->rpb_line;

	data_page::dpg_repeat* index2 = page->dpg_rpt + org_rpb->rpb_line;
	rhd* header = (rhd*) ((SCHAR *) page + index2->dpg_offset);
	header->rhd_flags |= rhd_chain;
	page->dpg_rpt[slot] = *index2;

	index2->dpg_offset = space;
	index2->dpg_length = RHD_SIZE + size + fill;

	header = (rhd*) ((SCHAR *) page + space);
	header->rhd_flags = new_rpb->rpb_flags;
	header->rhd_transaction = new_rpb->rpb_transaction_nr;
	header->rhd_format = new_rpb->rpb_format_number;
	header->rhd_b_page = new_rpb->rpb_b_page;
	header->rhd_b_line = new_rpb->rpb_b_line;

	SQZ_fast(&dcc, (SCHAR*) new_rpb->rpb_address, (SCHAR*) header->rhd_data);

	if (fill) {
		memset(header->rhd_data + size, 0, fill);
	}

	CCH_RELEASE(tdbb, &org_rpb->getWindow(tdbb));

	return true;
}


int DPM_compress( thread_db* tdbb, data_page* page)
{
/**************************************
 *
 *	D P M _ c o m p r e s s
 *
 **************************************
 *
 * Functional description
 *	Compress a data page.  Return the high water mark.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("compress (page)\n");
	if (debug_flag > DEBUG_TRACE_ALL_INFO)
		printf("    sequence %"SLONGFORMAT"\n", page->dpg_sequence);
#endif

	UCHAR temp_page[MAX_PAGE_SIZE];
	if (dbb->dbb_page_size > sizeof(temp_page)) {
		BUGCHECK(250);			// msg 250 temporary page buffer too small
	}
	SSHORT space = dbb->dbb_page_size;
	const data_page::dpg_repeat* const end = page->dpg_rpt + page->dpg_count;

	for (data_page::dpg_repeat* index = page->dpg_rpt; index < end; index++)
	{
		if (index->dpg_offset)
		{
			// 11-Aug-2004. Nickolay Samofatov.
			// Copy block of pre-aligned length to avoid putting rubbish from stack into database
			// This should also work just a little bit faster too.
			const SSHORT l = ROUNDUP(index->dpg_length, ODS_ALIGNMENT);
			space -= l;
			memcpy(temp_page + space, (UCHAR *) page + index->dpg_offset, l);
			index->dpg_offset = space;
		}
	}

	memcpy((UCHAR *) page + space, temp_page + space, dbb->dbb_page_size - space);

	if (page->dpg_header.pag_type != pag_data) {
		BUGCHECK(251);			// msg 251 damaged data page
	}

	return space;
}


void DPM_create_relation( thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *	D P M _ c r e a t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Create a new relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("DPM_create_relation (relation %d)\n", relation->rel_id);
#endif

	RelationPages* relPages = relation->getBasePages();
	DPM_create_relation_pages(tdbb, relation, relPages);

	// Store page numbers in RDB$PAGES
	DPM_pages(tdbb, relation->rel_id, pag_pointer, (ULONG) 0,
			  (*relPages->rel_pages)[0] /*window.win_page*/);
	DPM_pages(tdbb, relation->rel_id, pag_root, (ULONG) 0,
			  relPages->rel_index_root /*root_window.win_page*/);
}


void DPM_create_relation_pages(thread_db* tdbb, jrd_rel* relation, RelationPages* relPages)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Allocate first pointer page
	WIN window(relPages->rel_pg_space_id, -1);
	pointer_page* page = (pointer_page*) DPM_allocate(tdbb, &window);
	page->ppg_header.pag_type = pag_pointer;
	page->ppg_relation = relation->rel_id;
	page->ppg_header.pag_flags = ppg_eof;
	CCH_RELEASE(tdbb, &window);

	// If this is relation 0 (RDB$PAGES), update the header

	if (relation->rel_id == 0)
	{
		WIN root_window(HEADER_PAGE_NUMBER);
		header_page* header = (header_page*) CCH_FETCH(tdbb, &root_window, LCK_write, pag_header);
		CCH_MARK(tdbb, &root_window);
		header->hdr_PAGES = window.win_page.getPageNum();

		CCH_RELEASE(tdbb, &root_window);
	}

	// Keep track in memory of the first pointer page

	if (!relPages->rel_pages)
	{
		vcl* vector = vcl::newVector(*dbb->dbb_permanent, 1);
		relPages->rel_pages = vector;
	}
	(*relPages->rel_pages)[0] = window.win_page.getPageNum();

	// CVC: AFAIK, DPM_allocate calls PAG_allocate and neither of them cares about win_page.
	// Therefore, I decided that the root_window in the if() above and this one aren't related.
	// Create an index root page
	WIN root_window(relPages->rel_pg_space_id, -1);
	index_root_page* root = (index_root_page*) DPM_allocate(tdbb, &root_window);
	root->irt_header.pag_type = pag_root;
	root->irt_relation = relation->rel_id;
	//root->irt_count = 0;
	CCH_RELEASE(tdbb, &root_window);
	relPages->rel_index_root = root_window.win_page.getPageNum();
}


SLONG DPM_data_pages(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *	D P M _ d a t a _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Compute and return the number of data pages in a relation.
 *	Some poor sucker is going to use this information to make
 *	an educated guess at the number of records in the relation.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("DPM_data_pages (relation %d)\n", relation->rel_id);
#endif

	RelationPages* relPages = relation->getPages(tdbb);
	SLONG pages = relPages->rel_data_pages;
	if (!pages)
	{
		WIN window(relPages->rel_pg_space_id, -1);
		for (USHORT sequence = 0; true; sequence++)
		{
			const pointer_page* ppage =
				get_pointer_page(tdbb, relation, relPages, &window, sequence, LCK_read);
			if (!ppage)
			{
				 BUGCHECK(243);
				 // msg 243 missing pointer page in DPM_data_pages
			}
			const SLONG* page = ppage->ppg_page;
			const SLONG* const end_page = page + ppage->ppg_count;
			while (page < end_page)
			{
				if (*page++) {
					pages++;
				}
			}
			if (ppage->ppg_header.pag_flags & ppg_eof) {
				break;
			}
			CCH_RELEASE(tdbb, &window);
		}

		CCH_RELEASE(tdbb, &window);
#ifdef SUPERSERVER
		relPages->rel_data_pages = pages;
#endif
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("    returned pages: %"SLONGFORMAT"\n", pages);
#endif

	return pages;
}


void DPM_delete( thread_db* tdbb, record_param* rpb, SLONG prior_page)
{
/**************************************
 *
 *	D P M _ d e l e t e
 *
 **************************************
 *
 * Functional description
 *	Delete a fragment from data page.  Assume the page has
 *	already been fetched (but not marked) for write.  Copy the
 *	record header into the record parameter block before deleting
 *	it.  If the record goes empty, release the page.  Release the
 *	page when we're done.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("DPM_delete (record_param %"QUADFORMAT", prior_page %"SLONGFORMAT")\n",
		           rpb->rpb_number.getValue(), prior_page);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT" back %"
			SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif

	WIN* const window = &rpb->getWindow(tdbb);
	data_page* page = (data_page*) window->win_buffer;
	SLONG sequence = page->dpg_sequence;
	data_page::dpg_repeat* index = &page->dpg_rpt[rpb->rpb_line];
	const RecordNumber number = rpb->rpb_number;

	if (!get_header(window, rpb->rpb_line, rpb))
	{
		CCH_RELEASE(tdbb, window);
		BUGCHECK(244);			// msg 244 Fragment does not exist
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO) {
		printf("    record length as stored in record header: %d\n", rpb->rpb_length);
	}
#endif

	rpb->rpb_number = number;

	CCH_precedence(tdbb, window, prior_page);
	CCH_MARK(tdbb, window);
	index->dpg_offset = 0;
	index->dpg_length = 0;

	// Compute the highest line number level on page

	for (index = &page->dpg_rpt[page->dpg_count]; index > page->dpg_rpt; --index)
	{
		if (index[-1].dpg_offset)
			break;
	}

	USHORT count;
	page->dpg_count = count = index - page->dpg_rpt;

	// If the page is not empty and used to be marked as full, change the
	// state of both the page and the appropriate pointer page.

	if (count && (page->dpg_header.pag_flags & dpg_full))
	{
		DEBUG
		page->dpg_header.pag_flags &= ~dpg_full;
		mark_full(tdbb, rpb);

#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("    page is no longer full\n");
#endif
		return;
	}

	const UCHAR flags = page->dpg_header.pag_flags;
	CCH_RELEASE(tdbb, window);

	// If the page is non-empty, we're done.

	if (count)
		return;

	if (flags & dpg_orphan)
	{
		// The page inventory page will be written after the page being
		// released, which will be written after the pages from which earlier
		// fragments were deleted, which will be written after the page from
		// which the first fragment is deleted.
		// The resulting 'must-be-written-after' graph is:
		// pip --> deallocated page --> prior_page
		PAG_release_page(tdbb, window->win_page, window->win_page);
		return;
	}

	// Data page has gone empty.  Refetch page thru pointer page.  If it's
	// still empty, remove page from relation.

	pointer_page* ppage;
	SSHORT slot;
	USHORT pp_sequence;

	DECOMPOSE(sequence, dbb->dbb_dp_per_pp, pp_sequence, slot);

	RelationPages* relPages	= NULL;
	WIN pwindow(DB_PAGE_SPACE, -1);

	for (;;)
	{
		relPages = rpb->rpb_relation->getPages(tdbb, rpb->rpb_transaction_nr);
		pwindow = WIN(relPages->rel_pg_space_id, -1);

		if (!(ppage = get_pointer_page(tdbb, rpb->rpb_relation, relPages, &pwindow,
				pp_sequence, LCK_write)))
		{
			BUGCHECK(245);	// msg 245 pointer page disappeared in DPM_delete
		}

		if (slot >= ppage->ppg_count || !(window->win_page = ppage->ppg_page[slot]))
		{
			CCH_RELEASE(tdbb, &pwindow);
			return;
		}

		// Since this fetch for exclusive access follows a (pointer page) fetch for
		// exclusive access, put a timeout on this fetch to be able to recover from
		// possible deadlocks.
		page = (data_page*) CCH_FETCH_TIMEOUT(tdbb, window, LCK_write, pag_data, -1);
		if (!page) {
			CCH_RELEASE(tdbb, &pwindow);
		}
		else
			break;
	}

	if (page->dpg_count)
	{
		CCH_RELEASE(tdbb, &pwindow);
		CCH_RELEASE(tdbb, window);
		return;
	}

	// Data page is still empty and still in the relation.  Eliminate the
	// pointer to the data page then release the page.

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("\tDPM_delete:  page %"SLONGFORMAT
			 " is empty and about to be released from relation %d\n",
			 window->win_page.getPageNum(), rpb->rpb_relation->rel_id);
	}
#endif

	// Make sure that the pointer page is written after the data page.
	// The resulting 'must-be-written-after' graph is:
	// pip --> pp --> deallocated page --> prior_page
	CCH_precedence(tdbb, &pwindow, window->win_page);

	CCH_MARK(tdbb, &pwindow);
	ppage->ppg_page[slot] = 0;

	const SLONG* ptr;
	for (ptr = &ppage->ppg_page[ppage->ppg_count]; ptr > ppage->ppg_page; --ptr)
	{
		if (ptr[-1])
			break;
	}

	ppage->ppg_count = count = ptr - ppage->ppg_page;
	if (count) {
		count--;
	}
	ppage->ppg_min_space = MIN(ppage->ppg_min_space, count);
	//jrd_rel* relation = rpb->rpb_relation;

	relPages->rel_slot_space = MIN(relPages->rel_slot_space, pp_sequence);
	if (relPages->rel_data_pages) {
		--relPages->rel_data_pages;
	}

	CCH_RELEASE(tdbb, &pwindow);
	CCH_RELEASE(tdbb, window);

	// Make sure that the page inventory page is written after the pointer page.
	// Earlier, we make sure that the pointer page is written after the data
	// page being released.
	PAG_release_page(tdbb, window->win_page, pwindow.win_page);
}


void DPM_delete_relation( thread_db* tdbb, jrd_rel* relation)
{
   struct {
          SSHORT jrd_21;	// gds__utility 
   } jrd_20;
   struct {
          SSHORT jrd_19;	// gds__utility 
   } jrd_18;
   struct {
          SSHORT jrd_17;	// gds__utility 
   } jrd_16;
   struct {
          SSHORT jrd_15;	// RDB$RELATION_ID 
   } jrd_14;
/**************************************
 *
 *	D P M _ d e l e t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Get rid of an unloved, unwanted relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	RelationPages* relPages = relation->getBasePages();
	DPM_delete_relation_pages(tdbb, relation, relPages);

	// Next, cancel out stuff from RDB$PAGES

	jrd_req* handle = NULL;

	/*FOR(REQUEST_HANDLE handle) X IN RDB$PAGES WITH
		X.RDB$RELATION_ID EQ relation->rel_id*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_13, sizeof(jrd_13), true);
	jrd_14.jrd_15 = relation->rel_id;
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_14);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_16);
	   if (!jrd_16.jrd_17) break;
			/*ERASE X;*/
			EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_18);
	/*END_FOR;*/
	   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_20);
	   }
	}

	CMP_release(tdbb, handle);
	CCH_flush(tdbb, FLUSH_ALL, 0);
}


void DPM_delete_relation_pages(Jrd::thread_db* tdbb, Jrd::jrd_rel* relation, Jrd::RelationPages* relPages)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	WIN window(relPages->rel_pg_space_id, -1), data_window(relPages->rel_pg_space_id, -1);
	window.win_flags = data_window.win_flags = WIN_large_scan;
	window.win_scans = data_window.win_scans = 1;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("DPM_delete_relation (relation %d)\n", relation->rel_id);
#endif

	// Delete all data and pointer pages

	for (USHORT sequence = 0; true; sequence++)
	{
		const pointer_page* ppage =
			get_pointer_page(tdbb, relation, relPages, &window, sequence, LCK_read);
		if (!ppage)
		{
			 BUGCHECK(246);	// msg 246 pointer page lost from DPM_delete_relation
		}
		const SLONG* page = ppage->ppg_page;
		const UCHAR* flags = (UCHAR *) (ppage->ppg_page + dbb->dbb_dp_per_pp);
		for (USHORT i = 0; i < ppage->ppg_count; i++, page++)
		{
			if (!*page) {
				continue;
			}
			if (flags[i >> 2] & (2 << ((i & 3) << 1)))
			{
				data_window.win_page = *page;
				data_page* dpage = (data_page*) CCH_FETCH(tdbb, &data_window, LCK_write, pag_data);
				const data_page::dpg_repeat* line = dpage->dpg_rpt;
				const data_page::dpg_repeat* const end_line = line + dpage->dpg_count;
				for (; line < end_line; line++)
				{
					if (line->dpg_length)
					{
						rhd* header = (rhd*) ((UCHAR *) dpage + line->dpg_offset);
						if (header->rhd_flags & rhd_large)
						{
							delete_tail(tdbb, (rhdf*)header, relPages->rel_pg_space_id,
										line->dpg_length);
						}
					}
				}
				CCH_RELEASE_TAIL(tdbb, &data_window);
			}
			PAG_release_page(tdbb, PageNumber(relPages->rel_pg_space_id, *page), ZERO_PAGE_NUMBER);
		}
		const UCHAR pag_flags = ppage->ppg_header.pag_flags;
		CCH_RELEASE_TAIL(tdbb, &window);
		PAG_release_page(tdbb, window.win_page, ZERO_PAGE_NUMBER);
		if (pag_flags & ppg_eof)
		{
			break;
		}
	}

	delete relPages->rel_pages;
	relPages->rel_pages = NULL;
	relPages->rel_data_pages = 0;

	// Now get rid of the index root page

	PAG_release_page(tdbb,
		PageNumber(relPages->rel_pg_space_id, relPages->rel_index_root), ZERO_PAGE_NUMBER);
	relPages->rel_index_root = 0;
}

bool DPM_fetch(thread_db* tdbb, record_param* rpb, USHORT lock)
{
/**************************************
 *
 *	D P M _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	Fetch a particular record fragment from page and line numbers.
 *	Get various header stuff, but don't change the record number.
 *
 *	return: true if the fragment is returned.
 *		false if the fragment is not found.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
	{
		printf("DPM_fetch (record_param %"QUADFORMAT"d, lock %d)\n",
			rpb->rpb_number.getValue(), lock);
	}
	if (debug_flag > DEBUG_READS_INFO)
		printf("    record %"SLONGFORMAT":%d\n", rpb->rpb_page, rpb->rpb_line);
#endif

	const RecordNumber number = rpb->rpb_number;
	RelationPages* relPages = rpb->rpb_relation->getPages(tdbb);
	rpb->getWindow(tdbb).win_page = PageNumber(relPages->rel_pg_space_id, rpb->rpb_page);
	CCH_FETCH(tdbb, &rpb->getWindow(tdbb), lock, pag_data);

	if (!get_header(&rpb->getWindow(tdbb), rpb->rpb_line, rpb))
	{
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		return false;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf
			("    record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT" back %"
			 SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif
	rpb->rpb_number = number;

	return true;
}


SSHORT DPM_fetch_back(thread_db* tdbb,
					  record_param* rpb, USHORT lock, SSHORT latch_wait)
{
/**************************************
 *
 *	D P M _ f e t c h _ b a c k
 *
 **************************************
 *
 * Functional description
 *	Chase a backpointer with a handoff.
 *
 * input:
 *      latch_wait:     1 => Wait as long as necessary to get the latch.
 *                              This can cause deadlocks of course.
 *                      0 => If the latch can't be acquired immediately,
 *                              give up and return 0.
 *                      <negative number> => Latch timeout interval in seconds.
 *
 * return:
 *	1:	fetch back was successful.
 *	0:	unsuccessful (only possible is latch_wait <> 1),
 *			The latch timed out.
 *			The latch on the fetched page is downgraded to shared.
 *			The fetched page is unmarked.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
	{
		printf("DPM_fetch_back (record_param %"QUADFORMAT"d, lock %d)\n",
			rpb->rpb_number.getValue(), lock);
	}
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf("    record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT
				  " back %"SLONGFORMAT":%d\n",
				  rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
				  rpb->rpb_b_page, rpb->rpb_b_line);
	}
#endif

	// Possibly allow a latch timeout to occur.  Return error if that is the case.

	if (!(CCH_HANDOFF_TIMEOUT(tdbb,
							  &rpb->getWindow(tdbb),
							  rpb->rpb_b_page,
							  lock,
							  pag_data,
							  latch_wait)))
	{
		return 0;
	}

	const RecordNumber number = rpb->rpb_number;
	rpb->rpb_page = rpb->rpb_b_page;
	rpb->rpb_line = rpb->rpb_b_line;
	if (!get_header(&rpb->getWindow(tdbb), rpb->rpb_line, rpb))
	{
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		BUGCHECK(291);			// msg 291 cannot find record back version
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf("    record fetched  %"SLONGFORMAT":%d transaction %"
			 SLONGFORMAT" back %"SLONGFORMAT":%d fragment %"SLONGFORMAT
			 ":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif


	rpb->rpb_number = number;

	return 1;
}


void DPM_fetch_fragment( thread_db* tdbb, record_param* rpb, USHORT lock)
{
/**************************************
 *
 *	D P M _ f e t c h _f r a g m e n t
 *
 **************************************
 *
 * Functional description
 *	Chase a fragment pointer with a handoff.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
		printf("DPM_fetch_fragment (record_param %"QUADFORMAT"d, lock %d)\n",
			rpb->rpb_number.getValue(), lock);
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf("    record  %"SLONGFORMAT":%d transaction %"SLONGFORMAT
				  " back %"SLONGFORMAT":%d\n",
				  rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
				  rpb->rpb_b_page, rpb->rpb_b_line);
	}
#endif

	const RecordNumber number = rpb->rpb_number;
	rpb->rpb_page = rpb->rpb_f_page;
	rpb->rpb_line = rpb->rpb_f_line;
	CCH_HANDOFF(tdbb, &rpb->getWindow(tdbb), rpb->rpb_page, lock, pag_data);

	if (!get_header(&rpb->getWindow(tdbb), rpb->rpb_line, rpb))
	{
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		BUGCHECK(248);			// msg 248 cannot find record fragment
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
	{
		printf
			("    record fetched  %"SLONGFORMAT":%d transaction %"SLONGFORMAT
			 " back %"SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif
	rpb->rpb_number = number;
}


SINT64 DPM_gen_id(thread_db* tdbb, SLONG generator, bool initialize, SINT64 val)
{
/**************************************
 *
 *	D P M _ g e n _ i d
 *
 **************************************
 *
 * Functional description
 *	Generate relation specific value.
 *      If initialize is set then value of generator is made
 *      equal to val else generator is incremented by val.
 *      The resulting value is the result of the function.
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf("DPM_gen_id (generator %"SLONGFORMAT", val %" QUADFORMAT "d)\n", generator, val);
	}
#endif

	const USHORT sequence = generator / dbb->dbb_page_manager.gensPerPage;
	const USHORT offset = generator % dbb->dbb_page_manager.gensPerPage;

	WIN window(DB_PAGE_SPACE, -1);
	vcl* vector = dbb->dbb_gen_id_pages;
	if (!vector || (sequence >= vector->count()) || !((*vector)[sequence]))
	{
		DPM_scan_pages(tdbb);
		if (!(vector = dbb->dbb_gen_id_pages) ||
			(sequence >= vector->count()) || !((*vector)[sequence]))
		{
			generator_page* page = (generator_page*) DPM_allocate(tdbb, &window);
			page->gpg_header.pag_type = pag_ids;
			page->gpg_sequence = sequence;
			CCH_must_write(&window);
			CCH_RELEASE(tdbb, &window);
			DPM_pages(tdbb, 0, pag_ids, (ULONG) sequence, window.win_page.getPageNum());
			vector = dbb->dbb_gen_id_pages =
				vcl::newVector(*dbb->dbb_permanent, dbb->dbb_gen_id_pages, sequence + 1);
			(*vector)[sequence] = window.win_page.getPageNum();
		}
	}

	window.win_page = (*vector)[sequence];
	window.win_flags = 0;
	generator_page* page;
	if (dbb->dbb_flags & DBB_read_only) {
		page = (generator_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_ids);
	}
	else {
		page = (generator_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_ids);
	}

	/*  If we are in ODS >= 10, then we have a pointer to an int64 value in the
	 *  generator page: if earlier than 10, it's a pointer to a long value.
	 *  Pick up the right kind of pointer, based on the ODS version.
	 *  The conditions were commented out 1999-05-14 by ChrisJ, because we
	 *  decided that the V6 engine would only access an ODS-10 database.
	 *  (and uncommented 2000-05-05, also by ChrisJ, when minds changed.)
	 */
	SINT64* ptr = 0;
	SLONG* lptr = 0;
	if (dbb->dbb_ods_version >= ODS_VERSION10) {
		ptr = ((SINT64 *) (page->gpg_values)) + offset;
	}
	else {
		lptr = ((SLONG *) (((pointer_page*) page)->ppg_page)) + offset;
	}

	if (val || initialize)
	{
		if (dbb->dbb_flags & DBB_read_only)
		{
			CCH_RELEASE(tdbb, &window);
			ERR_post(Arg::Gds(isc_read_only_database));
		}
		CCH_MARK_SYSTEM(tdbb, &window);

		/* Initialize or increment the quad value in an ODS 10 or later
		 * generator page, or the long value in ODS <= 9.
		 * The conditions were commented out 1999-05-14 by ChrisJ, because we
		 * decided that the V6 engine would only access an ODS-10 database.
		 * (and uncommented 2000-05-05, also by ChrisJ, when minds changed.)
		 */
		if (dbb->dbb_ods_version >= ODS_VERSION10)
		{
			if (initialize) {
				*ptr = val;
			}
			else {
				*ptr += val;
			}
		}
		else if (initialize) {
			*lptr = (SLONG) val;
		}
		else {
			*lptr += (SLONG) val;
		}

		if (tdbb->getTransaction()) {
			tdbb->getTransaction()->tra_flags |= TRA_write;
		}
	}

	/*  If we are in ODS >= 10, then we have a pointer to an int64 value in the
	 *  generator page: if earlier than 10, it's a pointer to a long value.
	 *  We always want to return an int64, so convert the long value from the
	 *  old ODS into an int64.
	 * The conditions were commented out 1999-05-14 by ChrisJ, because we
	 * decided that the V6 engine would only access an ODS-10 database.
	 * (and uncommented 2000-05-05, also by ChrisJ, when minds changed.)
	 */
	SINT64 value;
	if (dbb->dbb_ods_version >= ODS_VERSION10) {
		value = *ptr;
	}
	else {
		value = (SINT64) *lptr;
	}

	CCH_RELEASE(tdbb, &window);

	return value;
}


bool DPM_get( thread_db* tdbb, record_param* rpb, SSHORT lock_type)
{
/**************************************
 *
 *	D P M _ g e t
 *
 **************************************
 *
 * Functional description
 *	Get a specific record in a relation.  If it doesn't exit,
 *	just return false.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
	{
		printf("DPM_get (record_param %"QUADFORMAT"d, lock type %d)\n",
			rpb->rpb_number.getValue(), lock_type);
	}
#endif

	WIN* window = &rpb->getWindow(tdbb);
	rpb->rpb_prior = NULL;

	// Find starting point
	USHORT pp_sequence;
	SSHORT slot, line;
	rpb->rpb_number.decompose(dbb->dbb_max_records, dbb->dbb_dp_per_pp, line, slot, pp_sequence);
	// Check if the record number is OK

	if (rpb->rpb_number.getValue() < 0) {
		return false;
	}

	// Find the next pointer page, data page, and record
	pointer_page* page = get_pointer_page(tdbb, rpb->rpb_relation,
		rpb->rpb_relation->getPages(tdbb), window, pp_sequence, LCK_read);
	if (!page) {
		return false;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO)
		printf("    record  %"SLONGFORMAT":%d\n", page->ppg_page[slot], line);
#endif

	const SLONG page_number = page->ppg_page[slot];
	if (page_number)
	{
		CCH_HANDOFF(tdbb, window, page_number, lock_type, pag_data);
		if (get_header(window, line, rpb) &&
			!(rpb->rpb_flags & (rpb_blob | rpb_chained | rpb_fragment)))
		{
			return true;
		}
	}

	CCH_RELEASE(tdbb, window);

	return false;
}


ULONG DPM_get_blob(thread_db* tdbb,
				   blb* blob,
				   RecordNumber record_number, bool delete_flag, SLONG prior_page)
{
/**************************************
 *
 *	D P M _ g e t _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Given a blob block, find the associated blob.  If blob is level 0,
 *	get the data clump, otherwise get the vector of pointers.
 *
 *	If the delete flag is set, delete the blob header after access
 *	and return the page number.  This is a kludge, but less code
 *	a completely separate delete blob call.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	record_param rpb;
	rpb.rpb_relation = blob->blb_relation;
	rpb.getWindow(tdbb).win_flags = WIN_secondary;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
	{
		printf
			("DPM_get_blob (blob, record_number %"QUADFORMAT
			"d, delete_flag %d, prior_page %"SLONGFORMAT")\n",
			 record_number.getValue(), (int) delete_flag, prior_page);
	}
#endif

	// Find starting point
	USHORT pp_sequence;
	SSHORT slot, line;

	record_number.decompose(dbb->dbb_max_records, dbb->dbb_dp_per_pp, line, slot, pp_sequence);

	// Find the next pointer page, data page, and record.  If the page or
	// record doesn't exist, or the record isn't a blob, give up and
	// let somebody else complain.

	pointer_page* ppage = get_pointer_page(tdbb, blob->blb_relation,
		blob->blb_relation->getPages(tdbb), &rpb.getWindow(tdbb), pp_sequence, LCK_read);
	if (!ppage)
	{
		blob->blb_flags |= BLB_damaged;
		return 0UL;
	}

	// Do not delete this scope. It makes goto compatible with variables
	// defined in their minimal scope.
	{
		const SLONG page_number = ppage->ppg_page[slot];
		if (!page_number)
		{
			goto punt;
		}

		data_page* page = (data_page*) CCH_HANDOFF(tdbb,
							&rpb.getWindow(tdbb),
							page_number,
							(SSHORT) (delete_flag ? LCK_write : LCK_read),
							pag_data);
		if (line >= page->dpg_count) {
			goto punt;
		}

		data_page::dpg_repeat* index = &page->dpg_rpt[line];
		if (index->dpg_offset == 0) {
			goto punt;
		}

		blh* header = (blh*) ((SCHAR *) page + index->dpg_offset);
		if (!(header->blh_flags & rhd_blob)) {
			goto punt;
		}

		// We've got the blob header and everything looks ducky.  Get the header
		// fields.

		blob->blb_lead_page = header->blh_lead_page;
		blob->blb_max_sequence = header->blh_max_sequence;
		blob->blb_count = header->blh_count;
		blob->blb_length = header->blh_length;
		blob->blb_max_segment = header->blh_max_segment;
		blob->blb_level = header->blh_level;
		blob->blb_sub_type = header->blh_sub_type;
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
			blob->blb_charset = header->blh_charset;

		// Unless this is the only attachment, don't allow the sequential scan
		// of very large blobs to flush pages used by other attachments.

		Attachment* attachment = tdbb->getAttachment();
		if (attachment && (attachment != dbb->dbb_attachments || attachment->att_next))
		{
			// If the blob has more pages than the page buffer cache then mark
			// it as large. If this is a database backup then mark any blob as
			// large as the cumulative effect of scanning many small blobs is
			// equivalent to scanning single large blobs.

			if (blob->blb_max_sequence > dbb->dbb_bcb->bcb_count ||
				attachment->att_flags & ATT_gbak_attachment)
			{
				blob->blb_flags |= BLB_large_scan;
			}
		}

		if (header->blh_flags & rhd_stream_blob) {
			blob->blb_flags |= BLB_stream;
		}

		if (header->blh_flags & rhd_damaged) {
			goto punt;
		}

		// Retrieve the data either into page clump (level 0) or page vector (levels
		// 1 and 2).

		const USHORT length = index->dpg_length - BLH_SIZE;
		const UCHAR* q = (UCHAR *) header->blh_page;

		if (blob->blb_level == 0)
		{
			blob->blb_space_remaining = length;
			if (length)
				memcpy(blob->getBuffer(), q, length);
		}
		else
		{
			vcl* vector = blob->blb_pages;
			if (!vector) {
				vector = blob->blb_pages = vcl::newVector(*blob->blb_transaction->tra_pool, 0);
			}
			vector->resize(length / sizeof(SLONG));
			memcpy(vector->memPtr(), q, length);
		}

		if (!delete_flag)
		{
			CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
			return 0UL;
		}

		// We've been asked (nicely) to delete the blob.  So do so.

		rpb.rpb_relation = blob->blb_relation;
		rpb.rpb_page = rpb.getWindow(tdbb).win_page.getPageNum();
		rpb.rpb_line = line;
		DPM_delete(tdbb, &rpb, prior_page);
		return rpb.rpb_page;
	} // scope

punt:
	CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
	blob->blb_flags |= BLB_damaged;
	return 0UL;
}


bool DPM_next(thread_db* tdbb,
				 record_param* rpb,
				 USHORT lock_type,
#ifdef SCROLLABLE_CURSORS
				 bool backwards,
#endif
				 bool onepage)
{
/**************************************
 *
 *	D P M _ n e x t
 *
 **************************************
 *
 * Functional description
 *	Get the next record in a stream.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS)
		printf("DPM_next (record_param %"QUADFORMAT"d)\n", rpb->rpb_number.getValue());
#endif

	WIN* window = &rpb->getWindow(tdbb);
	RelationPages* relPages = rpb->rpb_relation->getPages(tdbb);

	if (window->win_flags & WIN_large_scan)
	{
		// Try to account for staggered execution of large sequential scans.

		window->win_scans = rpb->rpb_relation->rel_scan_count - rpb->rpb_org_scans;
		if (window->win_scans < 1) {
			window->win_scans = rpb->rpb_relation->rel_scan_count;
		}
	}
	rpb->rpb_prior = NULL;

	// Find starting point

#ifdef SCROLLABLE_CURSORS
	if (backwards)
	{
		if (rpb->rpb_number.isEmpty())
			return false;

		if (!rpb->rpb_number.isBof()) {
			rpb->rpb_number.decrement();
		}
		else
		{
			/*  if the stream was just opened, assume we want to start
			at the end of the stream, so compute the last theoretically
			possible rpb_number and go down from there.
			For now, we must force a scan to make sure that we get
			the last pointer page: this should be changed to use
			a coordination mechanism (probably using a shared lock)
			to keep apprised of when a pointer page gets added */

			DPM_scan_pages(tdbb);
			const vcl* vector = relPages->rel_pages;
			if (!vector) {
				return false;
			}
			const size_t pp_sequence = vector->count();
			rpb->rpb_number.setValue(
				((SINT64) pp_sequence) * dbb->dbb_dp_per_pp * dbb->dbb_max_records - 1);
		}
	}
	else
#endif
	{
		rpb->rpb_number.increment();
	}

	SSHORT slot, line;
	USHORT pp_sequence;
	rpb->rpb_number.decompose(dbb->dbb_max_records, dbb->dbb_dp_per_pp, line, slot, pp_sequence);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_READS_INFO) {
		printf("    pointer, slot, and line  %d:%d%d\n", pp_sequence, slot, line);
	}
#endif

	// Find the next pointer page, data page, and record

	while (true)
	{
		const pointer_page* ppage = get_pointer_page(tdbb, rpb->rpb_relation,
							relPages, window, pp_sequence, LCK_read);
		if (!ppage) {
				BUGCHECK(249);	// msg 249 pointer page vanished from DPM_next
		}

#ifdef SCROLLABLE_CURSORS
		if (backwards && slot >= ppage->ppg_count) {
			slot = ppage->ppg_count - 1;
		}
#endif

		for (; slot >= 0 && slot < ppage->ppg_count;)
		{
			const SLONG page_number = ppage->ppg_page[slot];
			if (page_number)
			{
#ifdef SUPERSERVER_V2
				// Perform sequential prefetch of relation's data pages.
				// This may need more work for scrollable cursors.

#ifdef SCROLLABLE_CURSORS
				if (!onepage && !line && !backwards)
#else
				if (!onepage && !line)
#endif
				{
					if (!(slot % dbb->dbb_prefetch_sequence))
					{
                        SLONG pages[PREFETCH_MAX_PAGES + 1];
						USHORT slot2 = slot;
						USHORT i;
						for (i = 0; i < dbb->dbb_prefetch_pages && slot2 < ppage->ppg_count;)
						{
							pages[i++] = ppage->ppg_page[slot2++];
						}

						// If no more data pages, piggyback next pointer page.

						if (slot2 >= ppage->ppg_count) {
							pages[i++] = ppage->ppg_next;
						}

						CCH_PREFETCH(tdbb, pages, i);
					}
				}
#endif
				const data_page* dpage = (data_page*) CCH_HANDOFF(tdbb, window,
									page_number, lock_type, pag_data);

#ifdef SCROLLABLE_CURSORS
				if (backwards && line >= dpage->dpg_count) {
					line = dpage->dpg_count - 1;
				}
#endif

				for (; line >= 0 && line < dpage->dpg_count;
#ifdef SCROLLABLE_CURSORS
					backwards ? line-- : line++
#else
					++line
#endif
					)
				{
					if (get_header(window, line, rpb) &&
						!(rpb->rpb_flags & (rpb_blob | rpb_chained | rpb_fragment)))
					{
						rpb->rpb_number.compose(dbb->dbb_max_records, dbb->dbb_dp_per_pp,
												line, slot, pp_sequence);
						return true;
					}
				}

				// Prevent large relations from emptying cache. When scrollable
				// cursors are surfaced, this logic may need to be revisited.

				if (window->win_flags & WIN_large_scan) {
					CCH_RELEASE_TAIL(tdbb, window);
				}
				else if (window->win_flags & WIN_garbage_collector &&
						 window->win_flags & WIN_garbage_collect)
				{
					CCH_RELEASE_TAIL(tdbb, window);
					window->win_flags &= ~WIN_garbage_collect;
				}
				else {
					CCH_RELEASE(tdbb, window);
				}

				if (onepage) {
					return false;
				}

				if (!(ppage = get_pointer_page(tdbb, rpb->rpb_relation, relPages, window,
												pp_sequence, LCK_read)))
				{
					BUGCHECK(249);	// msg 249 pointer page vanished from DPM_next
				}
			}

			if (onepage)
			{
				CCH_RELEASE(tdbb, window);
				return false;
			}

#ifdef SCROLLABLE_CURSORS
			if (backwards)
			{
				slot--;
				line = dbb->dbb_max_records - 1;
			}
			else
#endif
			{
				slot++;
				line = 0;
			}
		}

		const UCHAR flags = ppage->ppg_header.pag_flags;
#ifdef SCROLLABLE_CURSORS
		if (backwards)
		{
			pp_sequence--;
			slot = ppage->ppg_count - 1;
			line = dbb->dbb_max_records - 1;
		}
		else
#endif
		{
			pp_sequence++;
			slot = 0;
			line = 0;
		}

		if (window->win_flags & WIN_large_scan) {
			CCH_RELEASE_TAIL(tdbb, window);
		}
		else {
			CCH_RELEASE(tdbb, window);
		}
		if (flags & ppg_eof || onepage) {
			return false;
		}
	}
}


void DPM_pages(thread_db* tdbb, SSHORT rel_id, int type, ULONG sequence, SLONG page)
{
   struct {
          SLONG  jrd_9;	// RDB$PAGE_NUMBER 
          SLONG  jrd_10;	// RDB$PAGE_SEQUENCE 
          SSHORT jrd_11;	// RDB$PAGE_TYPE 
          SSHORT jrd_12;	// RDB$RELATION_ID 
   } jrd_8;
/**************************************
 *
 *	D P M _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Write a record in RDB$PAGES.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
	{
		printf("DPM_pages (rel_id %d, type %d, sequence %"ULONGFORMAT", page %"SLONGFORMAT")\n",
				rel_id, type, sequence, page);
	}
#endif

	jrd_req* request = CMP_find_request(tdbb, irq_s_pages, IRQ_REQUESTS);

	/*STORE(REQUEST_HANDLE request)
		X IN RDB$PAGES*/
	{
	
		/*X.RDB$RELATION_ID*/
		jrd_8.jrd_12 = rel_id;
		/*X.RDB$PAGE_TYPE*/
		jrd_8.jrd_11 = type;
		/*X.RDB$PAGE_SEQUENCE*/
		jrd_8.jrd_10 = sequence;
		/*X.RDB$PAGE_NUMBER*/
		jrd_8.jrd_9 = page;
	/*END_STORE*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_7, sizeof(jrd_7), true);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 12, (UCHAR*) &jrd_8);
	}

	if (!REQUEST(irq_s_pages))
		REQUEST(irq_s_pages) = request;
}


#ifdef SUPERSERVER_V2
SLONG DPM_prefetch_bitmap(thread_db* tdbb, jrd_rel* relation, PageBitmap* bitmap, RecordNumber number)
{
/**************************************
 *
 *	D P M _ p r e f e t c h _ b i t m a p
 *
 **************************************
 *
 * Functional description
 *	Generate a vector of corresponding data page
 *	numbers from a bitmap of relation record numbers.
 *	Return the bitmap record number of where we left
 *	off.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Empty and singular bitmaps aren't worth prefetch effort.

	if (!bitmap || bitmap->sbm_state != SBM_PLURAL) {
		return number;
	}

	Database* dbb = tdbb->getDatabase();
	WIN window(-1);
	SLONG pages[PREFETCH_MAX_PAGES];

	// CVC: If this routine is activated some day, what's the default value
	// of this variable? I set it to -1.
	SLONG prefetch_number = -1;

	USHORT i;
	for (i = 0; i < dbb->dbb_prefetch_pages;)
	{
        SLONG dp_sequence;
        SSHORT line, slot;
        USHORT pp_sequence;
		DECOMPOSE(number, dbb->dbb_max_records, dp_sequence, line);
		DECOMPOSE(dp_sequence, dbb->dbb_dp_per_pp, pp_sequence, slot);

		const pointer_page* ppage = get_pointer_page(tdbb, relation, &window, pp_sequence, LCK_read);
		if (!ppage)
		{
			BUGCHECK(249);
			// msg 249 pointer page vanished from DPM_prefetch_bitmap
		}
		pages[i] = (slot >= 0 && slot < ppage->ppg_count) ? ppage->ppg_page[slot] : 0;
		CCH_RELEASE(tdbb, &window);

		if (i++ < dbb->dbb_prefetch_sequence) {
			prefetch_number = number;
		}
		number = ((dp_sequence + 1) * dbb->dbb_max_records) - 1;
		if (!SBM_next(bitmap, &number, RSE_get_forward)) {
			break;
		}
	}

	CCH_PREFETCH(tdbb, pages, i);
	return prefetch_number;
}
#endif


void DPM_scan_pages( thread_db* tdbb)
{
   struct {
          SLONG  jrd_2;	// RDB$PAGE_NUMBER 
          SLONG  jrd_3;	// RDB$PAGE_SEQUENCE 
          SSHORT jrd_4;	// gds__utility 
          SSHORT jrd_5;	// RDB$PAGE_TYPE 
          SSHORT jrd_6;	// RDB$RELATION_ID 
   } jrd_1;
/**************************************
 *
 *	D P M _ s c a n _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Scan RDB$PAGES.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("DPM_scan_pages ()\n");
#endif

	// Special case update of RDB$PAGES pointer page vector to avoid
	// infinite recursion from this internal request when RDB$PAGES
	// has been extended with another pointer page.

	jrd_rel* relation = MET_relation(tdbb, 0);
	RelationPages* relPages = relation->getBasePages();

	vcl** address = &relPages->rel_pages;
	vcl* vector = *address;
	size_t sequence = vector->count() - 1;
	WIN window(relPages->rel_pg_space_id, (*vector)[sequence]);
	pointer_page* ppage = (pointer_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_pointer);

	while (ppage->ppg_next)
	{
		++sequence;
		vector->resize(sequence + 1);
		(*vector)[sequence] = ppage->ppg_next;
		ppage = (pointer_page*) CCH_HANDOFF(tdbb, &window, ppage->ppg_next, LCK_read, pag_pointer);
	}

	CCH_RELEASE(tdbb, &window);

	jrd_req* request = CMP_find_request(tdbb, irq_r_pages, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) X IN RDB$PAGES*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 14, (UCHAR*) &jrd_1);
	   if (!jrd_1.jrd_4) break;

		if (!REQUEST(irq_r_pages))
			REQUEST(irq_r_pages) = request;
		relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
					      jrd_1.jrd_6);
		relPages = relation->getBasePages();
		sequence = /*X.RDB$PAGE_SEQUENCE*/
			   jrd_1.jrd_3;
		switch (/*X.RDB$PAGE_TYPE*/
			jrd_1.jrd_5)
		{
		case pag_root:
			relPages->rel_index_root = /*X.RDB$PAGE_NUMBER*/
						   jrd_1.jrd_2;
			continue;

		case pag_pointer:
			address = &relPages->rel_pages;
			break;

		case pag_transactions:
			address = &dbb->dbb_t_pages;
			break;

		case pag_ids:
			address = &dbb->dbb_gen_id_pages;
			break;

		default:
			CORRUPT(257);		// msg 257 bad record in RDB$PAGES
		}
		vector = *address = vcl::newVector(*dbb->dbb_permanent, *address, sequence + 1);
		(*vector)[sequence] = /*X.RDB$PAGE_NUMBER*/
				      jrd_1.jrd_2;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_pages))
		REQUEST(irq_r_pages) = request;
}


void DPM_store( thread_db* tdbb, record_param* rpb, PageStack& stack, USHORT type)
{
/**************************************
 *
 *	D P M _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Store a new record in a relation.  If we can put it on a
 *	specific page, so much the better.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("DPM_store (record_param %"QUADFORMAT"d, stack, type %d)\n",
				  rpb->rpb_number.getValue(), type);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record to store %"SLONGFORMAT":%d transaction %"SLONGFORMAT
			 " back %"SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif

	DataComprControl dcc(*tdbb->getDefaultPool());
	const USHORT size = SQZ_length((SCHAR*) rpb->rpb_address, (int) rpb->rpb_length, &dcc);

	// If the record isn't going to fit on a page, even if fragmented,
	// handle it a little differently.

	if (size > dbb->dbb_page_size - (sizeof(data_page) + RHD_SIZE))
	{
		store_big_record(tdbb, rpb, stack, &dcc, size);
		return;
	}

	SLONG fill = (RHDF_SIZE - RHD_SIZE) - size;
	if (fill < 0) {
		fill = 0;
	}

	// Accomodate max record size i.e. 64K
	const SLONG length = RHD_SIZE + size + fill;
	rhd* header = locate_space(tdbb, rpb, (SSHORT)length, stack, NULL, type);

	header->rhd_flags = rpb->rpb_flags;
	header->rhd_transaction = rpb->rpb_transaction_nr;
	header->rhd_format = rpb->rpb_format_number;
	header->rhd_b_page = rpb->rpb_b_page;
	header->rhd_b_line = rpb->rpb_b_line;

	SQZ_fast(&dcc, (SCHAR*) rpb->rpb_address, (SCHAR*) header->rhd_data);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record %"SLONGFORMAT":%d, length %"SLONGFORMAT
			 ", rpb_flags %d, f_page %"SLONGFORMAT":%d, b_page %"SLONGFORMAT
			 ":%d\n",
			 rpb->rpb_page, rpb->rpb_line, length, rpb->rpb_flags,
			 rpb->rpb_f_page, rpb->rpb_f_line, rpb->rpb_b_page,
			 rpb->rpb_b_line);
	}
#endif

	if (fill) {
		 memset(header->rhd_data + size, 0, fill);
	}

	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
}


RecordNumber DPM_store_blob(thread_db* tdbb, blb* blob, Record* record)
{
/**************************************
 *
 *	D P M _ s t o r e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Store a blob on a data page.  Not so hard, all in all.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
		printf("DPM_store_blob (blob, record)\n");
#endif

	// Figure out length of blob on page.  Remember that blob can either
	// be a clump of data or a vector of page pointers.
	vcl* vector = 0;
	USHORT length;
	const UCHAR* q;
	PageStack stack;
	Firebird::Array<UCHAR> buffer;

	if (blob->blb_level == 0)
	{
		length = blob->blb_clump_size - blob->blb_space_remaining;

		if (!blob->hasBuffer())
		{
			if (blob->blb_temp_size > 0)
			{
				blob->blb_transaction->getBlobSpace()->read(
					blob->blb_temp_offset, buffer.getBuffer(blob->blb_temp_size), blob->blb_temp_size);
				q = buffer.begin();
			}
			else
			{
				fb_assert(length == 0);
				q = NULL;
			}
		}
		else
			q = blob->getBuffer();

		if (q)
			q = (UCHAR*) ((blob_page*) q)->blp_page;
	}
	else
	{
		vector = blob->blb_pages;
		length = vector->count() * sizeof(SLONG);
		q = (UCHAR *) (vector->begin());
		// Figure out precedence pages, if any
		vcl::iterator ptr, end;
		for (ptr = vector->begin(), end = vector->end(); ptr < end; ++ptr) {
            stack.push(*ptr);
        }
	}


	// Locate space to store blob

	record_param rpb;
	//rpb.getWindow(tdbb).win_flags = 0; redundant.

	rpb.rpb_relation = blob->blb_relation;
	rpb.rpb_transaction_nr = blob->blb_transaction->tra_number;

	blh* header = (blh*) locate_space(tdbb, &rpb, (SSHORT)(BLH_SIZE + length),
										stack, record, DPM_other);
	header->blh_flags = rhd_blob;

	if (blob->blb_flags & BLB_stream) {
		header->blh_flags |= rhd_stream_blob;
	}

	if (blob->blb_level) {
		header->blh_flags |= rhd_large;
	}

	header->blh_lead_page = blob->blb_lead_page;
	header->blh_max_sequence = blob->blb_max_sequence;
	header->blh_count = blob->blb_count;
	header->blh_max_segment = blob->blb_max_segment;
	header->blh_length = blob->blb_length;
	header->blh_level = blob->blb_level;
	header->blh_sub_type = blob->blb_sub_type;
	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		header->blh_charset = blob->blb_charset;

	if (length)
		memcpy(header->blh_page, q, length);

	data_page* page = (data_page*) rpb.getWindow(tdbb).win_buffer;
	if (blob->blb_level && !(page->dpg_header.pag_flags & dpg_large))
	{
		page->dpg_header.pag_flags |= dpg_large;
		mark_full(tdbb, &rpb);
	}
	else {
		CCH_RELEASE(tdbb, &rpb.getWindow(tdbb));
	}

	return rpb.rpb_number;
}


void DPM_rewrite_header( thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	D P M _ r e w r i t e _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Re-write benign fields in record header.  This is mostly used
 *	to purge out old versions.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
		printf("DPM_rewrite_header (record_param %"QUADFORMAT"d)\n", rpb->rpb_number.getValue());
	if (debug_flag > DEBUG_WRITES_INFO)
		printf("    record  %"SLONGFORMAT":%d\n", rpb->rpb_page, rpb->rpb_line);
#endif

	WIN* window = &rpb->getWindow(tdbb);
	data_page* page = (data_page*) window->win_buffer;
	rhd* header = (rhd*) ((SCHAR *) page + page->dpg_rpt[rpb->rpb_line].dpg_offset);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    old flags %d, old transaction %"SLONGFORMAT
			 ", old format %d, old back record %"SLONGFORMAT":%d\n",
			 header->rhd_flags, header->rhd_transaction, (int) header->rhd_format,
			 header->rhd_b_page, header->rhd_b_line);
		printf
			("    new flags %d, new transaction %"SLONGFORMAT
			 ", new format %d, new back record %"SLONGFORMAT":%d\n",
			 rpb->rpb_flags, rpb->rpb_transaction_nr, rpb->rpb_format_number,
			 rpb->rpb_b_page, rpb->rpb_b_line);
	}
#endif

	header->rhd_flags = rpb->rpb_flags;
	header->rhd_transaction = rpb->rpb_transaction_nr;
	header->rhd_format = rpb->rpb_format_number;
	header->rhd_b_page = rpb->rpb_b_page;
	header->rhd_b_line = rpb->rpb_b_line;
}


void DPM_update( thread_db* tdbb, record_param* rpb, PageStack* stack,
	const jrd_tra* transaction)
{
/**************************************
 *
 *	D P M _ u p d a t e
 *
 **************************************
 *
 * Functional description
 *	Replace an existing record.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf("DPM_update (record_param %"QUADFORMAT"d, stack, transaction %"SLONGFORMAT")\n",
				  rpb->rpb_number.getValue(), transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record %"SLONGFORMAT":%d transaction %"SLONGFORMAT" back %"
			 SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif

	// Mark the page as modified, then figure out the compressed length of the
	// replacement record.

	DEBUG
	if (stack)
	{
		while (stack->hasData())
		{
			CCH_precedence(tdbb, &rpb->getWindow(tdbb), stack->pop());
		}
	}

	CCH_precedence(tdbb, &rpb->getWindow(tdbb), -rpb->rpb_transaction_nr);
	CCH_MARK(tdbb, &rpb->getWindow(tdbb));
	data_page* page = (data_page*) rpb->getWindow(tdbb).win_buffer;
	DataComprControl dcc(*tdbb->getDefaultPool());
	const USHORT size = SQZ_length((SCHAR*) rpb->rpb_address, (int) rpb->rpb_length, &dcc);

	// It is critical that the record be padded, if necessary, to the length of
	// a fragmented record header.  Compute the amount of fill required.

	SLONG fill = (RHDF_SIZE - RHD_SIZE) - size;
	if (fill < 0) {
		fill = 0;
	}

	// Accomodate max record size i.e. 64K
	const SLONG length = ROUNDUP(RHD_SIZE + size + fill, ODS_ALIGNMENT);
	const SSHORT slot = rpb->rpb_line;

	// Find space on page
	SSHORT space = dbb->dbb_page_size;
	const SSHORT top = HIGH_WATER(page->dpg_count);
	SSHORT available = dbb->dbb_page_size - top;
	const SSHORT old_length = page->dpg_rpt[slot].dpg_length;
	page->dpg_rpt[slot].dpg_length = 0;

    const data_page::dpg_repeat* index = page->dpg_rpt;
	for (const data_page::dpg_repeat* const end = index + page->dpg_count; index < end; index++)
	{
		const SSHORT offset = index->dpg_offset;
		if (offset)
		{
			available -= ROUNDUP(index->dpg_length, ODS_ALIGNMENT);
			space = MIN(space, offset);
		}
	}

	if (length > available)
	{
		fragment(tdbb, rpb, available, &dcc, old_length, transaction);
		return;
	}

	space -= length;
	if (space < top) {
		space = DPM_compress(tdbb, page) - length;
	}

	page->dpg_rpt[slot].dpg_offset = space;
	page->dpg_rpt[slot].dpg_length = RHD_SIZE + size + fill;

	rhd* header = (rhd*) ((SCHAR *) page + space);
	header->rhd_flags = rpb->rpb_flags;
	header->rhd_transaction = rpb->rpb_transaction_nr;
	header->rhd_format = rpb->rpb_format_number;
	header->rhd_b_page = rpb->rpb_b_page;
	header->rhd_b_line = rpb->rpb_b_line;

	SQZ_fast(&dcc, (SCHAR*) rpb->rpb_address, (SCHAR*) header->rhd_data);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record %"SLONGFORMAT
			 ":%d, dpg_length %d, rpb_flags %d, rpb_f record %"SLONGFORMAT
			 ":%d, rpb_b record %"SLONGFORMAT":%d\n",
			 rpb->rpb_page, rpb->rpb_line, page->dpg_rpt[slot].dpg_length,
			 rpb->rpb_flags, rpb->rpb_f_page, rpb->rpb_f_line,
			 rpb->rpb_b_page, rpb->rpb_b_line);
	}
#endif

	if (fill) {
		memset(header->rhd_data + size, 0, fill);
	}

	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
}


static void delete_tail(thread_db* tdbb, rhdf* header, const USHORT page_space, USHORT length)
{
/**************************************
 *
 *	d e l e t e _ t a i l
 *
 **************************************
 *
 * Functional description
 *	Delete the tail of a large object.  This is called only from
 *	DPM_delete_relation.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
		printf("delete_tail (header, length)\n");
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    transaction %"SLONGFORMAT" flags %d fragment %"
				  SLONGFORMAT":%d back %"SLONGFORMAT":%d\n",
				  header->rhdf_transaction, header->rhdf_flags,
				  header->rhdf_f_page, header->rhdf_f_line,
				  header->rhdf_b_page, header->rhdf_b_line);
	}
#endif

	WIN window(page_space, -1);
	window.win_flags = WIN_large_scan;
	window.win_scans = 1;

	// If the object isn't a blob, things are a little simpler.

	if (!(header->rhdf_flags & rhd_blob))
	{
		SLONG page_number = header->rhdf_f_page;
		while (true)
		{
			window.win_page = page_number;
			data_page* dpage = (data_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_data);
			header = (rhdf*) ((UCHAR *) dpage + dpage->dpg_rpt[0].dpg_offset);
			const USHORT flags = header->rhdf_flags;
			page_number = header->rhdf_f_page;
			CCH_RELEASE_TAIL(tdbb, &window);
			PAG_release_page(tdbb, window.win_page, ZERO_PAGE_NUMBER);
			if (!(flags & rhd_incomplete)) {
				break;
			}
		}
		return;
	}

	// Object is a blob, and a big one at that

	blh* blob = (blh*) header;
	const SLONG* page1 = blob->blh_page;
	const SLONG* const end1 = page1 + (length - BLH_SIZE) / sizeof(SLONG);

	for (; page1 < end1; page1++)
	{
		if (blob->blh_level == 2)
		{
			window.win_page = *page1;
			blob_page* bpage = (blob_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_blob);
			SLONG* page2 = bpage->blp_page;
			const SLONG* const end2 = page2 + ((bpage->blp_length - BLP_SIZE) / sizeof(SLONG));
			while (page2 < end2) {
				PAG_release_page(tdbb, PageNumber(page_space, *page2++), ZERO_PAGE_NUMBER);
			}
			CCH_RELEASE_TAIL(tdbb, &window);
		}
		PAG_release_page(tdbb, PageNumber(page_space, *page1), ZERO_PAGE_NUMBER);
	}
}


static void fragment(thread_db* tdbb,
					 record_param* rpb,
					 SSHORT available_space,
					 DataComprControl* dcc, SSHORT length, const jrd_tra* transaction)
{
/**************************************
 *
 *	f r a g m e n t
 *
 **************************************
 *
 * Functional description
 *	DPM_update tried to replace a record on a page, but it doesn't
 *	fit.  The record, obviously, needs to be fragmented.  Stick as
 *	much as fits on the page and put the rest elsewhere (though not
 *	in that order.
 *
 *	Doesn't sound so bad does it?
 *
 *	Little do you know.  The challenge is that we have to put the
 *	head of the record in the designated page:line space and put
 *	the tail on a different page.  To maintain on-disk consistency
 *	we have to write the tail first.  But we don't want anybody
 *	messing with the head until we get back, and, if possible, we'd
 *	like to keep as much space on the original page as we can get.
 *	Making matters worse, we may be storing a new version of the
 *	record or we may be backing out an old one and replacing it
 *	with one older still (replacing a dead rolled back record with
 *	the preceding version).
 *
 *	Here is the order of battle.   If we're creating a new version,
 *	and the previous version is not a delta, we compress the page
 *	and create a new record which claims all the available space
 *	(but which contains garbage).  That record will have our transaction
 *	id (so it won't be committed for anybody) and it will have legitimate
 *	back pointers to the previous version.
 *
 *	If we're creating a new version and the previous version is a delta,
 *	we leave the current version in place, putting our transaction id
 *	on it.  We then point the back pointers to the delta version of the
 *	same generation of the record, which will have the correct transaction
 *	id.  Applying deltas to the expanded form of the same version of the
 *	record is a no-op -- but it doesn't cost much and the case is rare.
 *
 *	If we're backing out a rolled back version, we've got another
 *	problem.  The rpb we've got is for record version n - 1, not version
 *	n + 1.  (e.g. I'm transaction 32 removing the rolled back record
 *	created by transaction 28 and reinstating the committed version
 *	created by transaction 26.  The rpb I've got is for the record
 *	written by transaction 26 -- the delta flag indicates whether the
 *	version written by transaction 24 was a delta, the transaction id
 *	is for a committed transaction, and the back pointers go back to
 *	transaction 24's version.  So in that case, we slap our transaction
 *	id on transaction 28's version (the one we'll be eliminating) and
 *	don't change anything else.
 *
 *	In all cases, once the tail is written and safe, we come back to
 *	the head, store the remaining data, fix up the record header and
 *	everything is done.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES)
	{
		printf
			("fragment (record_param %"QUADFORMAT
			 "d, available_space %d, dcc, length %d, transaction %"
			 SLONGFORMAT")\n",
			 rpb->rpb_number.getValue(), available_space, length,
			 transaction ? transaction->tra_number : 0);
	}
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    record %"SLONGFORMAT":%d transaction %"SLONGFORMAT" back %"
			 SLONGFORMAT":%d fragment %"SLONGFORMAT":%d flags %d\n",
			 rpb->rpb_page, rpb->rpb_line, rpb->rpb_transaction_nr,
			 rpb->rpb_b_page, rpb->rpb_b_line, rpb->rpb_f_page,
			 rpb->rpb_f_line, rpb->rpb_flags);
	}
#endif

	/* Start by claiming what space exists on the page.  Note that
	   DPM_update has already set the length for this record to zero,
	   so we're all set for a compress. However, if the current
	   version was re-stored as a delta, or if we're restoring an
	   older version, we must preserve it here so
	   that others may reconstruct it during fragmentation of the
	   new version. */

	WIN* window = &rpb->getWindow(tdbb);
	data_page* page = (data_page*) window->win_buffer;
	const SSHORT line = rpb->rpb_line;

	rhdf* header;
	if (transaction->tra_number != rpb->rpb_transaction_nr)
	{
		header = (rhdf*) ((SCHAR *) page + page->dpg_rpt[line].dpg_offset);
		header->rhdf_transaction = transaction->tra_number;
		header->rhdf_flags |= rhd_gc_active;
		page->dpg_rpt[line].dpg_length = available_space = length;
	}
	else
	{
		if (rpb->rpb_flags & rpb_delta)
		{
			header = (rhdf*) ((SCHAR *) page + page->dpg_rpt[line].dpg_offset);
			header->rhdf_flags |= rpb_delta;
			page->dpg_rpt[line].dpg_length = available_space = length;
		}
		else
		{
			const SSHORT space = DPM_compress(tdbb, page) - available_space;
			header = (rhdf*) ((SCHAR *) page + space);
			header->rhdf_flags = rhd_deleted;
			header->rhdf_f_page = header->rhdf_f_line = 0;
			page->dpg_rpt[line].dpg_offset = space;
			page->dpg_rpt[line].dpg_length = available_space;
		}
		header->rhdf_transaction = rpb->rpb_transaction_nr;
		header->rhdf_b_page = rpb->rpb_b_page;
		header->rhdf_b_line = rpb->rpb_b_line;
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf
			("    rhdf_transaction %"SLONGFORMAT", window record %"SLONGFORMAT
			 ":%d, available_space %d, rhdf_flags %d, rhdf_f record %"
			 SLONGFORMAT":%d, rhdf_b record %"SLONGFORMAT":%d\n",
			 header->rhdf_transaction, window->win_page.getPageNum(), line,
			 available_space, header->rhdf_flags, header->rhdf_f_page,
			 header->rhdf_f_line, header->rhdf_b_page, header->rhdf_b_line);
	}
#endif

	CCH_RELEASE(tdbb, window);

	// The next task is to store the tail where it fits.  To do this, we
	// next to compute the size (compressed) of the tail.  This requires
	// first figuring out how much of the original record fits on the
	// original page.

	const USHORT pre_header_length =
		SQZ_compress_length(dcc, (SCHAR*) rpb->rpb_address, (int) (available_space - RHDF_SIZE));

	record_param tail_rpb = *rpb;
	tail_rpb.rpb_flags = rpb_fragment;
	tail_rpb.rpb_b_page = 0;
	tail_rpb.rpb_b_line = 0;
	tail_rpb.rpb_address = rpb->rpb_address + pre_header_length;
	tail_rpb.rpb_length = rpb->rpb_length - pre_header_length;
	tail_rpb.getWindow(tdbb).win_flags = 0;

	PageStack stack;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
		printf("    about to store tail\n");
#endif

	DPM_store(tdbb, &tail_rpb, stack, DPM_other);

	// That was unreasonablly easy.  Now re-fetch the original page and
	// fill in the fragment pointer

	page = (data_page*) CCH_FETCH(tdbb, window, LCK_write, pag_data);
	CCH_precedence(tdbb, window, tail_rpb.rpb_page);
	CCH_MARK(tdbb, window);

	header = (rhdf*) ((SCHAR *) page + page->dpg_rpt[line].dpg_offset);
	header->rhdf_flags = rhd_incomplete | rpb->rpb_flags;
	header->rhdf_transaction = rpb->rpb_transaction_nr;
	header->rhdf_format = rpb->rpb_format_number;
	header->rhdf_f_page = tail_rpb.rpb_page;
	header->rhdf_f_line = tail_rpb.rpb_line;

	if (transaction->tra_number != rpb->rpb_transaction_nr)
	{
		header->rhdf_b_page = rpb->rpb_b_page;
		header->rhdf_b_line = rpb->rpb_b_line;
	}


	const USHORT post_header_length =
		SQZ_compress(dcc, (SCHAR*) rpb->rpb_address, (SCHAR*) header->rhdf_data,
					 (int) (available_space - RHDF_SIZE));

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    fragment head \n");
		printf
			("    rhdf_trans %"SLONGFORMAT", window record %"SLONGFORMAT
			 ":%d, dpg_length %d\n\trhdf_flags %d, rhdf_f record %"SLONGFORMAT
			 ":%d, rhdf_b record %"SLONGFORMAT":%d\n",
			 header->rhdf_transaction, window->win_page.getPageNum(), line,
			 page->dpg_rpt[line].dpg_length, header->rhdf_flags,
			 header->rhdf_f_page, header->rhdf_f_line, header->rhdf_b_page,
			 header->rhdf_b_line);
	}
#endif

	if (pre_header_length != post_header_length)
	{
		CCH_RELEASE(tdbb, window);
		BUGCHECK(252);			// msg 252 header fragment length changed
	}

	CCH_RELEASE(tdbb, window);
}


static void extend_relation( thread_db* tdbb, jrd_rel* relation, WIN* window)
{
/**************************************
 *
 *	e x t e n d _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Extend a relation with a given page.  The window points to an
 *	already allocated, fetched, and marked data page to be inserted
 *	into the pointer pages for a given relation.
 *	This routine returns a window on the datapage locked for write
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);
	RelationPages* relPages = relation->getPages(tdbb);
	WIN pp_window(relPages->rel_pg_space_id, -1),
		new_pp_window(relPages->rel_pg_space_id, -1);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO) {
		printf("     extend_relation (relation %d, window)\n", relation->rel_id);
	}
#endif

	// Release faked page before fetching pointer page to prevent deadlocks. This is only
	// a problem for multi-threaded servers using internal latches. The faked page may be
	// dirty from its previous incarnation and involved in a precedence relationship. This
	// special case may need a more general solution.

	CCH_RELEASE(tdbb, window);

	/* Search pointer pages for an empty slot.
	   If we run out of pointer pages, allocate another one.  Note that the code below
	   is not careful in preventing deadlocks when it allocates a new pointer page:
	    - the last already-existing pointer page is fetched with an exclusive latch,
	    - allocation of a new pointer page requires the fetching of the PIP page with
		an exlusive latch.
	   This might cause a deadlock.  Fortunately, pointer pages don't need to be
	   allocated often. */

	pointer_page* ppage = NULL;
	USHORT pp_sequence, slot = 0;
	data_page* dpage = NULL;

	for (;;)
	{
		for (pp_sequence = relPages->rel_slot_space;; pp_sequence++)
		{
			if (!(ppage = get_pointer_page(tdbb, relation, relPages, &pp_window,
										   pp_sequence, LCK_write)))
			{
				BUGCHECK(253);	// msg 253 pointer page vanished from extend_relation
			}
			SLONG* slots = ppage->ppg_page;
			for (slot = 0; slot < ppage->ppg_count; slot++, slots++)
			{
				if (*slots == 0) {
					break;
				}
			}
			if (slot < ppage->ppg_count) {
				break;
			}
			if ((pp_sequence && ppage->ppg_count < dbb->dbb_dp_per_pp) ||
				(ppage->ppg_count < dbb->dbb_dp_per_pp - 1))
			{
				slot = ppage->ppg_count;
				break;
			}
			if (ppage->ppg_header.pag_flags & ppg_eof)
			{
				ppage = (pointer_page*) DPM_allocate(tdbb, &new_pp_window);
				ppage->ppg_header.pag_type = pag_pointer;
				ppage->ppg_header.pag_flags |= ppg_eof;
				ppage->ppg_relation = relation->rel_id;
				ppage->ppg_sequence = ++pp_sequence;
				slot = 0;
				CCH_must_write(&new_pp_window);
				CCH_RELEASE(tdbb, &new_pp_window);

				vcl* vector = relPages->rel_pages =
					vcl::newVector(*dbb->dbb_permanent, relPages->rel_pages, pp_sequence + 1);
				(*vector)[pp_sequence] = new_pp_window.win_page.getPageNum();

				// hvlad: temporary tables don't save their pointer pages in RDB$PAGES
				if (relation->rel_id && (relPages->rel_instance_id == 0))
				{
					DPM_pages(tdbb, relation->rel_id, pag_pointer,
							  (SLONG) pp_sequence, new_pp_window.win_page.getPageNum());
				}
				relPages->rel_slot_space = pp_sequence;

				ppage = (pointer_page*) pp_window.win_buffer;
				CCH_MARK(tdbb, &pp_window);
				ppage->ppg_header.pag_flags &= ~ppg_eof;
				ppage->ppg_next = new_pp_window.win_page.getPageNum();
				--pp_sequence;
			}
			CCH_RELEASE(tdbb, &pp_window);
		}

		// We've found a slot.  Stick in the pointer to the data page

		if (ppage->ppg_page[slot])
		{
			CCH_RELEASE(tdbb, &pp_window);
			CORRUPT(258);			// msg 258 page slot not empty
		}

		// Refetch newly allocated page that was released above.
		// To prevent possible deadlocks (since we own already an exlusive latch and we
		// are asking for another exclusive latch), time out on the latch after 1 second.

		dpage = (data_page*) CCH_FETCH_TIMEOUT(tdbb, window, LCK_write, pag_undefined, -1);

		// In the case of a timeout, retry the whole thing.

		if (!dpage) {
			CCH_RELEASE(tdbb, &pp_window);
		}
		else
			break;
	}

	CCH_MARK(tdbb, window);
	dpage->dpg_sequence = (SLONG) pp_sequence * dbb->dbb_dp_per_pp + slot;
	dpage->dpg_relation = relation->rel_id;
	dpage->dpg_header.pag_type = pag_data;
	relPages->rel_data_space = pp_sequence;

	CCH_RELEASE(tdbb, window);

	CCH_precedence(tdbb, &pp_window, window->win_page);
	CCH_MARK_SYSTEM(tdbb, &pp_window);
	ppage->ppg_page[slot] = window->win_page.getPageNum();
	ppage->ppg_min_space = MIN(ppage->ppg_min_space, slot);
	ppage->ppg_count = MAX(ppage->ppg_count, slot + 1);
	UCHAR* bits = (UCHAR *) (ppage->ppg_page + dbb->dbb_dp_per_pp);
	bits[slot >> 2] &= ~(1 << ((slot & 3) << 1));
	if (relPages->rel_data_pages) {
		++relPages->rel_data_pages;
	}

	*window = pp_window;
	CCH_HANDOFF(tdbb, window, ppage->ppg_page[slot], LCK_write, pag_data);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("   extended_relation (relation %d, window_page %"SLONGFORMAT")\n",
				  relation->rel_id, window->win_page.getPageNum());
	}
#endif
}


static UCHAR* find_space(thread_db*	tdbb,
						 record_param*	rpb,
						 SSHORT	size,
						 PageStack&	stack,
						 Record*	record,
						 USHORT	type)
{
/**************************************
 *
 *	f i n d _ s p a c e
 *
 **************************************
 *
 * Functional description
 *	Find space of a given size on a data page.  If no space, return
 *	null.  If space is found, mark the page, set up the line field
 *	in the record parameter block, set up the offset/length on the
 *	data page, and return a pointer to the space.
 *
 *	To maintain page precedence when objects point to objects, a stack
 *	of pages of high precedence may be passed in.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const SSHORT aligned_size = ROUNDUP(size, ODS_ALIGNMENT);
	data_page* page = (data_page*) rpb->getWindow(tdbb).win_buffer;

	// Scan allocated lines looking for an empty slot, the high water mark,
	// and the amount of space potentially available on the page

	SSHORT space = dbb->dbb_page_size;
	SSHORT slot = 0;
	SSHORT used = HIGH_WATER(page->dpg_count);

	{ // scope
		const bool reserving = !(dbb->dbb_flags & DBB_no_reserve);
		const data_page::dpg_repeat* index = page->dpg_rpt;
		for (SSHORT i = 0; i < page->dpg_count; i++, index++)
		{
			if (index->dpg_offset)
			{
				space = MIN(space, index->dpg_offset);
				used += ROUNDUP(index->dpg_length, ODS_ALIGNMENT);
				if (type == DPM_primary && reserving)
				{
					const rhd* header = (rhd*) ((SCHAR *) page + index->dpg_offset);
					if (!header->rhd_b_page &&
						!(header->rhd_flags & (rhd_chain | rhd_blob | rhd_deleted | rhd_fragment)))
					{
						used += SPACE_FUDGE;
					}
				}
			}
			else if (!slot) {
				slot = i;
			}
		}
	} // scope

	if (!slot) {
		used += sizeof(data_page::dpg_repeat);
	}

	// If there isn't space, give up

	if (aligned_size > (int) dbb->dbb_page_size - used)
	{
		CCH_MARK(tdbb, &rpb->getWindow(tdbb));
		page->dpg_header.pag_flags |= dpg_full;
		mark_full(tdbb, rpb);
		return NULL;
	}

	// There's space on page.  If the line index needs expansion, do so.
	// If the page need to be compressed, compress it.

	while (stack.hasData()) {
		CCH_precedence(tdbb, &rpb->getWindow(tdbb), stack.pop());
	}
	CCH_MARK(tdbb, &rpb->getWindow(tdbb));

	{ // scope
		const USHORT rec_segments = page->dpg_count + (slot ? 0 : 1);
		fb_assert(rec_segments > 0); // zero is a disaster in macro HIGH_WATER
		if (aligned_size > space - HIGH_WATER(rec_segments))
			space = DPM_compress(tdbb, page);
	} // scope

	if (!slot) {
		slot = page->dpg_count++;
	}

	space -= aligned_size;
	data_page::dpg_repeat* index = &page->dpg_rpt[slot];
	index->dpg_length = size;
	index->dpg_offset = space;
	rpb->rpb_page = rpb->getWindow(tdbb).win_page.getPageNum();
	rpb->rpb_line = slot;
	rpb->rpb_number.setValue(((SINT64) page->dpg_sequence) * dbb->dbb_max_records + slot);

	if (record) {
		record->rec_precedence.push(rpb->rpb_page);
	}

	return (UCHAR*) page + space;
}


static bool get_header( WIN* window, SSHORT line, record_param* rpb)
{
/**************************************
 *
 *	g e t _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Copy record header fields into a record parameter block.  If
 *	the line is empty, return false;
 *
 **************************************/
	const data_page* page = (data_page*) window->win_buffer;
	if (line >= page->dpg_count) {
		return false;
	}

	const data_page::dpg_repeat* index = &page->dpg_rpt[line];
	if (index->dpg_offset == 0) {
		return false;
	}

	rhdf* header = (rhdf*) ((SCHAR *) page + index->dpg_offset);
	rpb->rpb_page = window->win_page.getPageNum();
	rpb->rpb_line = line;
	rpb->rpb_flags = header->rhdf_flags;

	if (!(rpb->rpb_flags & rpb_fragment))
	{
		rpb->rpb_b_page = header->rhdf_b_page;
		rpb->rpb_b_line = header->rhdf_b_line;
		rpb->rpb_transaction_nr = header->rhdf_transaction;
		rpb->rpb_format_number = header->rhdf_format;
	}

	if (rpb->rpb_flags & rpb_incomplete)
	{
		rpb->rpb_f_page = header->rhdf_f_page;
		rpb->rpb_f_line = header->rhdf_f_line;
		rpb->rpb_address = header->rhdf_data;
		rpb->rpb_length = index->dpg_length - RHDF_SIZE;
	}
	else
	{
		rpb->rpb_address = ((rhd*) header)->rhd_data;
		rpb->rpb_length = index->dpg_length - RHD_SIZE;
	}

	return true;
}


static pointer_page* get_pointer_page(thread_db* tdbb,
	   	 							  jrd_rel* relation, RelationPages* relPages,
									  WIN* window, USHORT sequence, USHORT lock)
{
/**************************************
 *
 *	g e t _ p o i n t e r _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Fetch a specific pointer page.  If we don't know about it,
 *	do a re-scan of RDB$PAGES to find it.  If that doesn't work,
 *	try the sibling pointer.  If that doesn't work, just stop,
 *	return NULL, and let our caller think about it.
 *
 **************************************/
	SET_TDBB(tdbb);

	vcl* vector = relPages->rel_pages;
	if (!vector || sequence >= vector->count())
	{
		for (;;)
		{
			DPM_scan_pages(tdbb);
			// If the relation is gone, then we can't do anything anymore.
			if (!relation || !(vector = relPages->rel_pages)) {
				return NULL;
			}
			if (sequence < vector->count()) {
				break;			// we are in business again
			}
			window->win_page = (*vector)[vector->count() - 1];
			const pointer_page* page = (pointer_page*) CCH_FETCH(tdbb, window, lock, pag_pointer);
			const SLONG next_ppg = page->ppg_next;
			CCH_RELEASE(tdbb, window);
			if (!next_ppg)
				return NULL;

			// hvlad: temporary tables don't save their pointer pages in RDB$PAGES
			if (relPages->rel_instance_id == 0)
				DPM_pages(tdbb, relation->rel_id, pag_pointer, vector->count(), next_ppg);
		}
	}

	window->win_page = (*vector)[sequence];
	pointer_page* page = (pointer_page*) CCH_FETCH(tdbb, window, lock, pag_pointer);

	if (page->ppg_relation != relation->rel_id || page->ppg_sequence != sequence)
	{
		CORRUPT(259);	// msg 259 bad pointer page
	}

	return page;
}


static rhd* locate_space(thread_db* tdbb,
						 record_param* rpb,
						 SSHORT size, PageStack& stack, Record* record, USHORT type)
{
/**************************************
 *
 *	l o c a t e _ s p a c e
 *
 **************************************
 *
 * Functional description
 *	Find space in a relation for a record.  Find a likely data page
 *	and call find_space to see if there really is space there.  If
 *	we can't find any space, extend the relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	jrd_rel* relation = rpb->rpb_relation;
	RelationPages* relPages = relation->getPages(tdbb, rpb->rpb_transaction_nr);
	WIN* window = &rpb->getWindow(tdbb);

	// If there is a preferred page, try there first

	PagesArray lowPages;
	SLONG dp_primary = 0;
	if (type == DPM_secondary)
	{
		USHORT pp_sequence;
		SSHORT slot, line;
		rpb->rpb_number.decompose(dbb->dbb_max_records, dbb->dbb_dp_per_pp, line, slot, pp_sequence);
		const pointer_page* ppage =
			get_pointer_page(tdbb, relation, relPages, window, pp_sequence, LCK_read);
		if (ppage)
		{
			if (slot < ppage->ppg_count && ((dp_primary = ppage->ppg_page[slot])) )
			{
				CCH_HANDOFF(tdbb, window, dp_primary, LCK_write, pag_data);
				UCHAR* space = find_space(tdbb, rpb, size, stack, record, type);
				if (space)
					return (rhd*) space;

				if (!window->win_page.isTemporary()) {
					CCH_get_related(tdbb, window->win_page, lowPages);
				}
			}
			else {
				CCH_RELEASE(tdbb, window);
			}
		}
	}

	// Look for space anywhere

	for (USHORT pp_sequence = relPages->rel_data_space;; pp_sequence++)
	{
		relPages->rel_data_space = pp_sequence;
		const pointer_page* ppage =
			 get_pointer_page(tdbb, relation, relPages, window, pp_sequence, LCK_read);
		if (!ppage)
		{
				BUGCHECK(254);
				// msg 254 pointer page vanished from relation list in locate_space
		}
		const SLONG pp_number = window->win_page.getPageNum();
		const UCHAR* bits = (UCHAR *) (ppage->ppg_page + dbb->dbb_dp_per_pp);
		for (USHORT slot = ppage->ppg_min_space; slot < ppage->ppg_count; slot++)
		{
			const SLONG dp_number = ppage->ppg_page[slot];

			// hvlad: avoid creating circle in precedence graph, if possible
			if (type == DPM_secondary && lowPages.exist(dp_number))
				continue;

			if (dp_number && ~bits[slot >> 2] & (1 << ((slot & 3) << 1)))
			{
				CCH_HANDOFF(tdbb, window, dp_number, LCK_write, pag_data);
				UCHAR* space = find_space(tdbb, rpb, size, stack, record, type);
				if (space)
					return (rhd*) space;
				window->win_page = pp_number;
				ppage = (pointer_page*) CCH_FETCH(tdbb, window, LCK_read, pag_pointer);
			}
		}
		const UCHAR flags = ppage->ppg_header.pag_flags;
		CCH_RELEASE(tdbb, window);
		if (flags & ppg_eof) {
			break;
		}
	}

	// Sigh.  No space.  Extend relation. Try for a while in case someone grabs the page
	// before we can get it locked, then give up on the assumption that things
	// are really screwed up.
	UCHAR* space = 0;
	int i;
	for (i = 0; i < 20; ++i)
	{
		DPM_allocate(tdbb, window);
		extend_relation(tdbb, relation, window);
		space = find_space(tdbb, rpb, size, stack, record, type);
		if (space) {
			break;
		}
	}
	if (i == 20) {
		BUGCHECK(255);			// msg 255 cannot find free space
	}

	if (record) {
		record->rec_precedence.push(window->win_page.getPageNum());
	}

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    extended relation %d with page %"SLONGFORMAT" to get %d bytes\n",
				  relation->rel_id, window->win_page.getPageNum(), size);
	}
#endif

	return (rhd*) space;
}


static void mark_full( thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	m a r k _ f u l l
 *
 **************************************
 *
 * Functional description
 *	Mark a fetched page and it's pointer page to indicate the page
 *	is full.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("mark_full ()\n");
#endif

	// We need to access the pointer page for write.  To avoid deadlocks,
	// we need to release the data page, fetch the pointer page for write,
	// and re-fetch the data page.  If the data page is still empty, set
	// it's "full" bit on the pointer page.

	data_page* dpage = (data_page*) rpb->getWindow(tdbb).win_buffer;
	const SLONG sequence = dpage->dpg_sequence;
	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));

	jrd_rel* relation = rpb->rpb_relation;
	RelationPages* relPages = relation->getPages(tdbb);
	WIN pp_window(relPages->rel_pg_space_id, -1);

	USHORT slot, pp_sequence;
	DECOMPOSE(sequence, dbb->dbb_dp_per_pp, pp_sequence, slot);

	// Fetch the pointer page, then the data page.  Since this is a case of
	// fetching a second page after having fetched the first page with an
	// exclusive latch, care has to be taken to prevent a deadlock.  This
	// is accomplished by timing out the second latch request and retrying
	// the whole thing.

	pointer_page* ppage = 0;
	do {
		ppage = get_pointer_page(tdbb, relation, relPages, &pp_window, pp_sequence, LCK_write);
		if (!ppage)
			BUGCHECK(256);	// msg 256 pointer page vanished from mark_full

		// If data page has been deleted from relation then there's nothing left to do.
		if (slot >= ppage->ppg_count ||
			rpb->getWindow(tdbb).win_page.getPageNum() != ppage->ppg_page[slot])
		{
			CCH_RELEASE(tdbb, &pp_window);
			return;
		}

		// Fetch the data page, but timeout after 1 second to break a possible deadlock.
		dpage = (data_page*) CCH_FETCH_TIMEOUT(tdbb, &rpb->getWindow(tdbb), LCK_read, pag_data, -1);

		// In case of a latch timeout, release the latch on the pointer page and retry.
		if (!dpage) {
			CCH_RELEASE(tdbb, &pp_window);
		}
	} while (!dpage);


	const UCHAR flags = dpage->dpg_header.pag_flags;
	CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));

	CCH_precedence(tdbb, &pp_window, rpb->getWindow(tdbb).win_page);
	CCH_MARK(tdbb, &pp_window);

	UCHAR bit = 1 << ((slot & 3) << 1);
	UCHAR* byte = (UCHAR *) (&ppage->ppg_page[dbb->dbb_dp_per_pp]) + (slot >> 2);

	if (flags & dpg_full)
	{
		*byte |= bit;
		ppage->ppg_min_space = MAX(slot + 1, ppage->ppg_min_space);
	}
	else
	{
		*byte &= ~bit;
		ppage->ppg_min_space = MIN(slot, ppage->ppg_min_space);
		relPages->rel_data_space = MIN(pp_sequence, relPages->rel_data_space);
	}

	// Next, handle the "large object" bit

	bit <<= 1;

	if (flags & dpg_large) {
		*byte |= bit;
	}
	else {
		*byte &= ~bit;
	}

	CCH_RELEASE(tdbb, &pp_window);
}


static void store_big_record(thread_db* tdbb, record_param* rpb,
							 PageStack& stack,
							 DataComprControl* dcc, USHORT size)
{
/**************************************
 *
 *	s t o r e _ b i g _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Store a new record in a relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_TRACE_ALL)
		printf("store_big_record ()\n");
#endif

	// Start compression from the end.

	const SCHAR* control = dcc->end();
	const SCHAR* in = (SCHAR *) rpb->rpb_address + rpb->rpb_length;
	RelationPages* relPages = rpb->rpb_relation->getPages(tdbb);
	PageNumber prior(relPages->rel_pg_space_id, 0);
	SCHAR count = 0;
	const USHORT max_data = dbb->dbb_page_size - (sizeof(data_page) + RHDF_SIZE);

	// Fill up data pages tail first until what's left fits on a single page.

	while (size > max_data)
	{
		// Allocate and format data page and fragment header

		data_page* page = (data_page*) DPM_allocate(tdbb, &rpb->getWindow(tdbb));
		page->dpg_header.pag_type = pag_data;
		page->dpg_header.pag_flags = dpg_orphan | dpg_full;
		page->dpg_relation = rpb->rpb_relation->rel_id;
		page->dpg_count = 1;
		rhdf* header = (rhdf*) & page->dpg_rpt[1];
		page->dpg_rpt[0].dpg_offset = (UCHAR *) header - (UCHAR *) page;
		page->dpg_rpt[0].dpg_length = max_data + RHDF_SIZE;
		header->rhdf_flags = (prior.getPageNum()) ? rhd_fragment | rhd_incomplete : rhd_fragment;
		header->rhdf_f_page = prior.getPageNum();
		USHORT length = max_data;
		size -= length;
		SCHAR* out = (SCHAR *) header->rhdf_data + length;

		// Move compressed data onto page

		while (length > 1)
		{
			// Handle residual count, if any
			if (count > 0)
			{
                const USHORT l = MIN((USHORT) count, length - 1);
				USHORT n = l;
				do {
					*--out = *--in;
				} while (--n);
				*--out = l;
				length -= (SSHORT) (l + 1);	// bytes remaining on page
				count -= (SSHORT) l;	// bytes remaining in run
				continue;
			}

			if ((count = *--control) < 0)
			{
				*--out = in[-1];
				*--out = count;
				in += count;
				length -= 2;
			}
		}

		// Page is full.  If there is an odd byte left, fudge it.

		if (length)
		{
			*--out = 0;
			++size;
		}
		else if (count > 0) {
			++size;
		}
		if (prior.getPageNum()) {
			CCH_precedence(tdbb, &rpb->getWindow(tdbb), prior);
		}

#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
		{
			printf("    back portion\n");
			printf
				("    getWindow(tdbb) page %"SLONGFORMAT
				 ", max_data %d, \n\trhdf_flags %d, prior %"SLONGFORMAT"\n",
				 rpb->getWindow(tdbb).win_page.getPageNum(), max_data, header->rhdf_flags,
				 prior.getPageNum());
		}
#endif

		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
		prior = rpb->getWindow(tdbb).win_page;
	}

	// What's left fits on a page.  Luckily, we don't have to store it ourselves.

	size = SQZ_length((SCHAR*) rpb->rpb_address, in - (SCHAR*) rpb->rpb_address, dcc);
	rhdf* header = (rhdf*)locate_space(tdbb, rpb, (SSHORT)(RHDF_SIZE + size), stack, NULL, DPM_other);

	header->rhdf_flags = rhd_incomplete | rhd_large | rpb->rpb_flags;
	header->rhdf_transaction = rpb->rpb_transaction_nr;
	header->rhdf_format = rpb->rpb_format_number;
	header->rhdf_b_page = rpb->rpb_b_page;
	header->rhdf_b_line = rpb->rpb_b_line;
	header->rhdf_f_page = prior.getPageNum();
	header->rhdf_f_line = 0;
	SQZ_fast(dcc, (SCHAR*) rpb->rpb_address, (SCHAR*) header->rhdf_data);

	data_page* page = (data_page*) rpb->getWindow(tdbb).win_buffer;

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
	{
		printf("    front part\n");
		printf
			("    rhdf_trans %"SLONGFORMAT", rpb_window record %"SLONGFORMAT
			 ":%d, dpg_length %d \n\trhdf_flags %d, rhdf_f record %"SLONGFORMAT
			 ":%d, rhdf_b record %"SLONGFORMAT":%d\n",
			 header->rhdf_transaction, rpb->getWindow(tdbb).win_page.getPageNum(),
			 rpb->rpb_line, page->dpg_rpt[rpb->rpb_line].dpg_length,
			 header->rhdf_flags, header->rhdf_f_page, header->rhdf_f_line,
			 header->rhdf_b_page, header->rhdf_b_line);
	}
#endif

	if (!(page->dpg_header.pag_flags & dpg_large))
	{
		page->dpg_header.pag_flags |= dpg_large;
		mark_full(tdbb, rpb);
	}
	else {
		CCH_RELEASE(tdbb, &rpb->getWindow(tdbb));
	}
}
