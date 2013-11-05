/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blb.cpp
 *	DESCRIPTION:	Blob handler
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
 * 2001.6.23 Claudio Valderrama: move_from_string to accept assignments
 * from string to blob field. First use was to allow inserting a literal string
 * in a blob field without requiring an UDF.
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 * Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"
#include "memory_routines.h"
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"

#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/req.h"
#include "../jrd/blb.h"
#include "../jrd/ods.h"
#include "../jrd/lls.h"
#include "gen/iberror.h"
#include "../jrd/blob_filter.h"
#include "../jrd/sdl.h"
#include "../jrd/intl.h"
#include "../jrd/cch.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/blf_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/filte_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/sdl_proto.h"
#include "../jrd/dsc_proto.h"
#include "../common/classes/array.h"
#include "../common/classes/VaryStr.h"

using namespace Jrd;
using namespace Firebird;
using Firebird::UCharBuffer;

typedef Ods::blob_page blob_page;

inline bool SEGMENTED(const blb* blob)
{
	return !(blob->blb_flags & BLB_stream);
}

static ArrayField* alloc_array(jrd_tra*, Ods::InternalArrayDesc*);
static blb* allocate_blob(thread_db*, jrd_tra*);
static ISC_STATUS blob_filter(USHORT, BlobControl*);
static blb* copy_blob(thread_db*, const bid*, bid*, USHORT, const UCHAR*, USHORT);
static void delete_blob(thread_db*, blb*, ULONG);
static void delete_blob_id(thread_db*, const bid*, SLONG, jrd_rel*);
static ArrayField* find_array(jrd_tra*, const bid*);
static BlobFilter* find_filter(thread_db*, SSHORT, SSHORT);
static blob_page* get_next_page(thread_db*, blb*, WIN *);
static void insert_page(thread_db*, blb*);
static void move_from_string(Jrd::thread_db*, const dsc*, dsc*, Jrd::jrd_nod*);
static void move_to_string(Jrd::thread_db*, dsc*, dsc*);
static void release_blob(blb*, const bool);
static void slice_callback(array_slice*, ULONG, dsc*);
static blb* store_array(thread_db*, jrd_tra*, bid*);


void BLB_cancel(thread_db* tdbb, blb* blob)
{
/**************************************
 *
 *      B L B _ c a n c e l
 *
 **************************************
 *
 * Functional description
 *      Abort a blob operation.  If the blob is a partially created
 *      temporary blob, free up any allocated pages.  In any case,
 *      get rid of the blob block.
 *
 **************************************/

	SET_TDBB(tdbb);

	// Release filter control resources

	if (blob->blb_flags & BLB_temporary)
		delete_blob(tdbb, blob, 0);

	release_blob(blob, true);
}


void BLB_check_well_formed(Jrd::thread_db* tdbb, const dsc* desc, Jrd::blb* blob)
{
/**************************************
 *
 *      B L B _ c h e c k _ w e l l _ f o r m e d
 *
 **************************************
 *
 * Functional description
 *      Check if a text BLOB is well formed.
 *
 **************************************/

	SET_TDBB(tdbb);

	// Need to verify this to work in database creation, as the charsets didn't exist yet.
	if (desc->getCharSet() == CS_NONE || desc->getCharSet() == CS_BINARY)
		return;

	CharSet* charSet = INTL_charset_lookup(tdbb, desc->getCharSet());

	if (!charSet->shouldCheckWellFormedness())
		return;

	HalfStaticArray<UCHAR, BUFFER_MEDIUM> buffer;
	ULONG pos = 0;

	while (!(blob->blb_flags & BLB_eof))
	{
		const ULONG len = BLB_get_data(tdbb, blob,
			buffer.getBuffer(buffer.getCapacity()) + pos, buffer.getCapacity() - pos, false);
		buffer.resize(pos + len);

		if (charSet->wellFormed(pos + len, buffer.begin(), &pos))
			pos = 0;
		else
		{
			if (pos == 0)
				status_exception::raise(Arg::Gds(isc_malformed_string));
			else
			{
				buffer.removeCount(0, pos);
				pos = buffer.getCount();
			}
		}
	}

	if (pos != 0)
		status_exception::raise(Arg::Gds(isc_malformed_string));
}


void BLB_close(thread_db* tdbb, Jrd::blb* blob)
{
/**************************************
 *
 *      B L B _ c l o s e
 *
 **************************************
 *
 * Functional description
 *      Close a blob.  If the blob is open for retrieval, release the
 *      blob block.  If it's a temporary blob, flush out the last page
 *      (if necessary) in preparation for materialization.
 *
 **************************************/

	SET_TDBB(tdbb);

	// Release filter control resources

	if (blob->blb_filter)
		BLF_close_blob(tdbb, &blob->blb_filter);

	blob->blb_flags |= BLB_closed;

	if (!(blob->blb_flags & BLB_temporary))
	{
		release_blob(blob, true);
		return;
	}

	if (blob->blb_level == 0)
	{
		//Database* dbb = tdbb->getDatabase();

		blob->blb_temp_size = blob->blb_clump_size - blob->blb_space_remaining;

		if (blob->blb_temp_size > 0)
		{
			blob->blb_temp_size += BLP_SIZE;
			jrd_tra* transaction = blob->blb_transaction;
			TempSpace* const tempSpace = transaction->getBlobSpace();

			blob->blb_temp_offset = tempSpace->allocateSpace(blob->blb_temp_size);
			tempSpace->write(blob->blb_temp_offset, blob->getBuffer(), blob->blb_temp_size);
		}
	}
	else if (blob->blb_level >= 1 &&
			 blob->blb_space_remaining < blob->blb_clump_size)
	{
		insert_page(tdbb, blob);
	}

	blob->freeBuffer();
}


blb* BLB_create(thread_db* tdbb, jrd_tra* transaction, bid* blob_id)
{
/**************************************
 *
 *      B L B _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *      Create a shiney, new, empty blob.
 *
 **************************************/

	SET_TDBB(tdbb);
	return BLB_create2(tdbb, transaction, blob_id, 0, NULL);
}


blb* BLB_create2(thread_db* tdbb,
				jrd_tra* transaction, bid* blob_id,
				USHORT bpb_length, const UCHAR* bpb)
{
/**************************************
 *
 *      B L B _ c r e a t e 2
 *
 **************************************
 *
 * Functional description
 *      Create a shiney, new, empty blob.
 *      Basically BLB_create() with BPB structure.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Create a blob large enough to hold a single data page
	SSHORT from, to;
	SSHORT from_charset, to_charset;
	const SSHORT type = gds__parse_bpb2(bpb_length,
						   bpb,
						   &from,
						   &to,
						   reinterpret_cast<USHORT*>(&from_charset),	// safe - alignment not changed
						   reinterpret_cast<USHORT*>(&to_charset),		// safe - alignment not changed
						   NULL, NULL, NULL, NULL);
	blb* blob = allocate_blob(tdbb, transaction);

	if (type & isc_bpb_type_stream)
		blob->blb_flags |= BLB_stream;

	if ((type & isc_bpb_storage_temp) || (dbb->dbb_flags & DBB_read_only)) {
		blob->blb_pg_space_id = dbb->dbb_page_manager.getTempPageSpaceID(tdbb);
	}
	else {
		blob->blb_pg_space_id = DB_PAGE_SPACE;
	}

	//blob->blb_source_interp = from_charset;
	//blob->blb_target_interp = to_charset;
	blob->blb_sub_type = to;

	bool filter_required = false;
	BlobFilter* filter = NULL;
	if (to && from != to)
	{
		// ASF: filter_text is not supported for write operations
		if (!(from == 0 && to == 1))
		{
			filter = find_filter(tdbb, from, to);
			filter_required = true;
		}
	}
	else if (to == isc_blob_text && (from_charset != to_charset))
	{
		if (from_charset == CS_dynamic)
			from_charset = tdbb->getAttachment()->att_charset;
		if (to_charset == CS_dynamic)
			to_charset = tdbb->getAttachment()->att_charset;

		if ((to_charset != CS_NONE) && (from_charset != CS_NONE) &&
			(to_charset != CS_BINARY) && (from_charset != CS_BINARY) &&
			(from_charset != to_charset))
		{
			filter = find_filter(tdbb, from, to);
			filter_required = true;
		}
	}

	blob->blb_space_remaining = blob->blb_clump_size;

	if (filter_required)
	{
		BLF_create_blob(tdbb,
						transaction,
						&blob->blb_filter,
						blob_id,
						bpb_length,
						bpb,
						blob_filter,
						filter);
		blob->blb_flags |= BLB_temporary;
		return blob;
	}

	blob->blb_flags |= BLB_temporary;

	// Set up for a "small" blob -- a blob that fits on an ordinary blob page

	blob_page* page = (blob_page*) blob->getBuffer();
	memset(page, 0, BLP_SIZE);	// initialize page header with NULLs
	page->blp_header.pag_type = pag_blob;
	blob->blb_segment = (UCHAR *) page->blp_page;

	// Format blob id and return blob handle

	blob_id->set_temporary(blob->blb_temp_id);

	return blob;
}


//
// This function makes linear stacks lookup. Therefore
// in case of big stacks garbage collection speed may become
// real problem. Stacks should be sorted before run?
// 2006.03.17 hvlad: it is sorted now
void BLB_garbage_collect(thread_db* tdbb,
						 RecordStack& going,
						 RecordStack& staying,
						 SLONG prior_page, jrd_rel* relation)
{
/**************************************
 *
 *      B L B _ g a r b a g e _ c o l l e c t
 *
 **************************************
 *
 * Functional description
 *      Garbage collect indices and blobs.  Garbage_collect is passed a
 *      stack of record blocks of records that are being deleted and a stack
 *      of records blocks of records that are staying.  Garbage_collect
 *      can be called from four operations:
 *
 *              VIO_backout     -- removing top record
 *              expunge         -- removing all versions of a record
 *              purge           -- removing all but top version of a record
 *              update_in_place -- replace the top level record.
 *
 *	hvlad: note that same blob_id can be reused by the staying record version
 *		in the different field than it was in going record. This is happening
 *		with 3 blob fields and update_in_place
 *
 **************************************/
	SET_TDBB(tdbb);

	RecordBitmap bmGoing;
	ULONG cntGoing = 0;

	// Loop thru records on the way out looking for blobs to garbage collect
	for (RecordStack::iterator stack1(going); stack1.hasData(); ++stack1)
	{
		Record* rec = stack1.object();
		if (!rec)
			continue;

		// Look for active blob records
		const Format* format = rec->rec_format;
		for (USHORT id = 0; id < format->fmt_count; id++)
		{
			DSC desc;
			if (DTYPE_IS_BLOB(format->fmt_desc[id].dsc_dtype) && EVL_field(0, rec, id, &desc))
			{
				const bid* blob = (bid*) desc.dsc_address;
				if (!blob->isEmpty())
				{
					if (blob->bid_internal.bid_relation_id == relation->rel_id)
					{
						const RecordNumber number = blob->get_permanent_number();
						bmGoing.set(number.getValue());
						cntGoing++;
					}
					else
					{
						// hvlad: blob_id in descriptor is not from our relation. Yes, it is
						// garbage in user data but we can handle it without bugcheck - just
						// ignore it. To be reconsider latter based on real user reports.
						// The same about staying blob few lines below
						gds__log("going blob (%ld:%ld) is not owned by relation (id = %d), ignored",
							blob->bid_quad.bid_quad_high, blob->bid_quad.bid_quad_low, relation->rel_id);
					}
				}
			}
		}
	}

	if (!cntGoing)
		return;

	// Make sure the blob doesn't stay in any record remaining
	for (RecordStack::iterator stack2(staying); stack2.hasData(); ++stack2)
	{
		Record* rec = stack2.object();
		if (!rec)
			continue;

		const Format* format = rec->rec_format;
		for (USHORT id = 0; id < format->fmt_count; id++)
		{
			DSC desc;
			if (DTYPE_IS_BLOB(format->fmt_desc[id].dsc_dtype) && EVL_field(0, rec, id, &desc))
			{
				const bid* blob = (bid*) desc.dsc_address;
				if (!blob->isEmpty())
				{
					if (blob->bid_internal.bid_relation_id == relation->rel_id)
					{
						const RecordNumber number = blob->get_permanent_number();
						if (bmGoing.test(number.getValue()))
						{
							bmGoing.clear(number.getValue());
							if (!--cntGoing)
								return;
						}
					}
					else
					{
						gds__log("staying blob (%ld:%ld) is not owned by relation (id = %d), ignored",
							blob->bid_quad.bid_quad_high, blob->bid_quad.bid_quad_low, relation->rel_id);
					}
				}
			}
		}
	}

	// Get rid of blob
	if (bmGoing.getFirst())
	{
		do {
			const FB_UINT64 id = bmGoing.current();

			bid blob;
			blob.set_permanent(relation->rel_id, RecordNumber(id));

			delete_blob_id(tdbb, &blob, prior_page, relation);
		} while (bmGoing.getNext());
	}
}


void BLB_gen_bpb(SSHORT source, SSHORT target, UCHAR sourceCharset, UCHAR targetCharset, UCharBuffer& bpb)
{
	bpb.resize(15);

	UCHAR* p = bpb.begin();
	*p++ = isc_bpb_version1;

	*p++ = isc_bpb_source_type;
	*p++ = 2;
	put_vax_short(p, source);
	p += 2;
	if (source == isc_blob_text)
	{
		*p++ = isc_bpb_source_interp;
		*p++ = 1;
		*p++ = sourceCharset;
	}

	*p++ = isc_bpb_target_type;
	*p++ = 2;
	put_vax_short(p, target);
	p += 2;
	if (target == isc_blob_text)
	{
		*p++ = isc_bpb_target_interp;
		*p++ = 1;
		*p++ = targetCharset;
	}
	fb_assert(static_cast<size_t>(p - bpb.begin()) <= bpb.getCount());

	// set the array count to the number of bytes we used
	bpb.shrink(p - bpb.begin());
}


void BLB_gen_bpb_from_descs(const dsc* fromDesc, const dsc* toDesc, UCharBuffer& bpb)
{
	BLB_gen_bpb(fromDesc->getBlobSubType(), toDesc->getBlobSubType(),
		fromDesc->getCharSet(), toDesc->getCharSet(), bpb);
}


blb* BLB_get_array(thread_db* tdbb, jrd_tra* transaction, const bid* blob_id,
				   Ods::InternalArrayDesc* desc)
{
/**************************************
 *
 *      B L B _ g e t _ a r r a y
 *
 **************************************
 *
 * Functional description
 *      Get array blob and array descriptor.
 *
 **************************************/
	SET_TDBB(tdbb);

	blb* blob = BLB_open2(tdbb, transaction, blob_id, 0, 0);

	if (blob->blb_length < sizeof(Ods::InternalArrayDesc))
	{
		BLB_close(tdbb, blob);
		IBERROR(193);			// msg 193 null or invalid array
	}

	BLB_get_segment(tdbb, blob, reinterpret_cast<UCHAR*>(desc), sizeof(Ods::InternalArrayDesc));

	const USHORT n = desc->iad_length - sizeof(Ods::InternalArrayDesc);
	if (n) {
		BLB_get_segment(tdbb, blob, reinterpret_cast<UCHAR*>(desc) + sizeof(Ods::InternalArrayDesc), n);
	}

	return blob;
}


ULONG BLB_get_data(thread_db* tdbb, blb* blob, UCHAR* buffer, SLONG length, bool close)
{
/**************************************
 *
 *      B L B _ g e t _ d a t a
 *
 **************************************
 *
 * Functional description
 *      Get a large hunk of data from a blob, which can then be
 *      closed (if close == true).  Return total number of bytes read.
 *      Don't worry about segment boundaries.
 *
 **************************************/
	SET_TDBB(tdbb);
	BLOB_PTR* p = buffer;

	while (length > 0)
	{
		// I have no idea why this limit is 32768 instead of 32767
		// 1994-August-12 David Schnepper
		USHORT n = (USHORT) MIN(length, (SLONG) 32768);
		n = BLB_get_segment(tdbb, blob, p, n);
		p += n;
		length -= n;
		if (blob->blb_flags & BLB_eof)
			break;
	}

	if (close)
		BLB_close(tdbb, blob);

	return (ULONG)(p - buffer);
}


USHORT BLB_get_segment(thread_db* tdbb, blb* blob, UCHAR* segment, USHORT buffer_length)
{
/**************************************
 *
 *      B L B _ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *      Get next segment or fragment from a blob.  Return the number
 *      of bytes returned.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	// If we reached end of file, we're still there

	if (blob->blb_flags & BLB_eof)
		return 0;

	if (blob->blb_filter)
	{
		blob->blb_fragment_size = 0;
		USHORT tmp_len = 0;
		const ISC_STATUS status =
			BLF_get_segment(tdbb, &blob->blb_filter, &tmp_len, buffer_length, segment);

		switch (status)
		{
			case isc_segstr_eof:
				blob->blb_flags |= BLB_eof;
				break;
			case isc_segment:
				blob->blb_fragment_size = 1;
				break;
			default:
				fb_assert(status == 0);
		}

		return tmp_len;
	}

	// If there is a seek pending, handle it here

	USHORT seek = 0;

	if (blob->blb_flags & BLB_seek)
	{
		if (blob->blb_seek >= blob->blb_length)
		{
			blob->blb_flags |= BLB_eof;
			return 0;
		}
		const USHORT l = dbb->dbb_page_size - BLP_SIZE;
		blob->blb_sequence = blob->blb_seek / l;
		seek = (USHORT)(blob->blb_seek % l);	// safe cast
		blob->blb_flags &= ~BLB_seek;
		blob->blb_fragment_size = 0;
		if (blob->blb_level)
		{
			blob->blb_space_remaining = 0;
			blob->blb_segment = NULL;
		}
		else
		{
			blob->blb_space_remaining = blob->blb_length - seek;
			blob->blb_segment = blob->getBuffer() + seek;
		}
	}

	if (!blob->blb_space_remaining && blob->blb_segment)
	{
		blob->blb_flags |= BLB_eof;
		return 0;
	}

	// Figure out how much data is to be moved, move it, and if necessary,
	// advance to the next page.  The length is a function of segment
	// size (or fragment size), buffer size, and amount of data left in the blob.

	BLOB_PTR* to = segment;
	const BLOB_PTR* from = blob->blb_segment;
	USHORT length = blob->blb_space_remaining;
	bool active_page = false;
	fb_assert(blob->blb_pg_space_id);
	WIN window(blob->blb_pg_space_id, -1); // there was no initialization of win_page here.
	if (blob->blb_flags & BLB_large_scan)
	{
		window.win_flags = WIN_large_scan;
		window.win_scans = 1;
	}

	while (true)
	{
		// If the blob is segmented, and this isn't a fragment, pick up
		// the length of the next segment.

		if (SEGMENTED(blob) && !blob->blb_fragment_size)
		{
			while (length < 2)
			{
				if (active_page)
				{
					if (window.win_flags & WIN_large_scan)
						CCH_RELEASE_TAIL(tdbb, &window);
					else
						CCH_RELEASE(tdbb, &window);
				}
				const blob_page* page = get_next_page(tdbb, blob, &window);
				if (!page)
				{
					blob->blb_flags |= BLB_eof;
					return 0;
				}
				from = (const UCHAR*) page->blp_page;
				length = page->blp_length;
				active_page = true;
			}

			UCHAR* p = (UCHAR *) & blob->blb_fragment_size;
			*p++ = *from++;
			*p++ = *from++;
			length -= 2;
		}

		// Figure out how much data can be moved.  Then account for the space, and move the data

		USHORT l = MIN(buffer_length, length);

		if (SEGMENTED(blob))
		{
			l = MIN(l, blob->blb_fragment_size);
			blob->blb_fragment_size -= l;
		}

		length -= l;
		buffer_length -= l;

		memcpy(to, from, l);

		to += l;
		from += l;

		// If we ran out of space in the data clump, and there is a next clump, get it.

		if (!length)
		{
			if (active_page)
			{
				if (window.win_flags & WIN_large_scan)
					CCH_RELEASE_TAIL(tdbb, &window);
				else
					CCH_RELEASE(tdbb, &window);
			}
			const blob_page* page = get_next_page(tdbb, blob, &window);
			if (!page)
			{
				active_page = false;
				break;
			}
			from = reinterpret_cast<const UCHAR*>(page->blp_page) + seek;
			length = page->blp_length - seek;
			seek = 0;
			active_page = true;
		}

		// If either the buffer or the fragment is exhausted, we're done.

		if (!buffer_length || (SEGMENTED(blob) && !blob->blb_fragment_size))
			break;
	}

	if (active_page)
	{
		UCHAR* buffer = blob->getBuffer();
		memcpy(buffer, from, length);
		from = buffer;

		if (window.win_flags & WIN_large_scan)
			CCH_RELEASE_TAIL(tdbb, &window);
		else
			CCH_RELEASE(tdbb, &window);
	}

	blob->blb_segment = const_cast<BLOB_PTR*>(from); // safe cast
	blob->blb_space_remaining = length;
	length = to - segment;
	blob->blb_seek += length;

	// If this is a stream blob, fake fragment unless we're at the end

	if (!SEGMENTED(blob)) { // stream blob
		blob->blb_fragment_size = (blob->blb_seek == blob->blb_length) ? 0 : 1;
	}

	return length;
}


SLONG BLB_get_slice(thread_db* tdbb,
					jrd_tra* transaction,
					const bid* blob_id,
					const UCHAR* sdl,
					USHORT param_length,
					const UCHAR* param, SLONG slice_length, UCHAR* slice_addr)
{
/**************************************
 *
 *      B L B _ g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *      Fetch a slice of an array.
 *
 **************************************/
	SET_TDBB(tdbb);
    //Database* database = GET_DBB();
	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

	// Checkout slice description language
	SLONG variables[64];
	sdl_info info;
	memcpy(variables, param, MIN(sizeof(variables), param_length));

	if (SDL_info(tdbb->tdbb_status_vector, sdl, &info, variables)) {
		ERR_punt();
	}

	SLONG stuff[IAD_LEN(16) / 4];
	Ods::InternalArrayDesc* desc = (Ods::InternalArrayDesc*) stuff;
	blb* blob = BLB_get_array(tdbb, transaction, blob_id, desc);
	SLONG length = desc->iad_total_length;

	// Get someplace to put data

	UCharBuffer data_buffer;
	UCHAR* const data = data_buffer.getBuffer(desc->iad_total_length);

	// zero out memory, so that it does not have to be done for each element

	memset(data, 0, desc->iad_total_length);

	SLONG offset = 0;

	array_slice arg;

	// If we know something about the subscript bounds, prepare
	// to fetch only stuff we really care about

	if (info.sdl_info_dimensions)
	{
		const SLONG from = SDL_compute_subscript(tdbb->tdbb_status_vector, desc,
								  				 info.sdl_info_dimensions, info.sdl_info_lower);
		const SLONG to = SDL_compute_subscript(tdbb->tdbb_status_vector, desc,
								  			   info.sdl_info_dimensions, info.sdl_info_upper);
		if (from != -1 && to != -1)
		{
			if (from)
			{
				offset = from * desc->iad_element_length;
				BLB_lseek(blob, 0, offset + (SLONG) desc->iad_length);
			}
			length = (to - from + 1) * desc->iad_element_length;
		}
	}

	length = BLB_get_data(tdbb, blob, data + offset, length) + offset;

	// Walk array
	arg.slice_desc = info.sdl_info_element;
	arg.slice_desc.dsc_address = slice_addr;
	arg.slice_end = slice_addr + slice_length;
	arg.slice_count = 0;
	arg.slice_element_length = info.sdl_info_element.dsc_length;
	arg.slice_direction = array_slice::slc_reading_array;	// fetching from array
	arg.slice_high_water = data + length;
	arg.slice_base = data + offset;

	ISC_STATUS status = SDL_walk(tdbb->tdbb_status_vector, sdl,
								 data, desc, variables, slice_callback, &arg);

	if (status) {
		ERR_punt();
	}

	return (SLONG) (arg.slice_count * arg.slice_element_length);
}


SLONG BLB_lseek(blb* blob, USHORT mode, SLONG offset)
{
/**************************************
 *
 *      B L B _ l s e e k
 *
 **************************************
 *
 * Functional description
 *      Position a blob for direct access.  Seek is only defined for stream
 *      type blobs.
 *
 **************************************/

	if (!(blob->blb_flags & BLB_stream))
		ERR_post(Arg::Gds(isc_bad_segstr_type));

	if (mode == 1)
		offset += blob->blb_seek;
	else if (mode == 2)
		offset = blob->blb_length + offset;

	if (offset < 0)
		offset = 0;

	if (offset > (SLONG) blob->blb_length)
		offset = blob->blb_length;

	blob->blb_seek = offset;
	blob->blb_flags |= BLB_seek;
	blob->blb_flags &= ~BLB_eof;

	return offset;
}


// This function can't take from_desc as const because it may call store_array,
// which in turn calls BLB_create2 that writes in the blob id. Although the
// compiler allows to modify from_desc->dsc_address' contents when from_desc is
// constant, this is misleading so I didn't make the source descriptor constant.
void BLB_move(thread_db* tdbb, dsc* from_desc, dsc* to_desc, jrd_nod* field)
{
/**************************************
 *
 *      B L B _ m o v e
 *
 **************************************
 *
 * Functional description
 *      Perform an assignment to a blob field or a blob descriptor.
 *      When assigning to a field, unless the blob is null,
 *      this requires that either a temporary blob be materialized or that
 *      a permanent blob be copied.  Note: it is illegal to have a blob
 *      field in a message.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (to_desc->dsc_dtype == dtype_array)
	{
		// only array->array conversions are allowed
		if (from_desc->dsc_dtype != dtype_array && from_desc->dsc_dtype != dtype_quad)
		{
			ERR_post(Arg::Gds(isc_array_convert_error));
		}
	}
	else if (DTYPE_IS_BLOB_OR_QUAD(to_desc->dsc_dtype))
	{
		if (!DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype))
		{
			// anything that can be copied into a string can be copied into a blob
			move_from_string(tdbb, from_desc, to_desc, field);
			return;
		}
	}
	else if (DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype))
	{
		move_to_string(tdbb, from_desc, to_desc);
		return;
	}
	else
		fb_assert(false);

	bool simpleMove = true;

	// If the target node is a field, we need more work to do.
	if (field)
	{
		switch (field->nod_type)
		{
			case nod_field:
				// We should not materialize the blob if the destination field
				// stream (nod_union, for example) doesn't have a relation.
				simpleMove =
					tdbb->getRequest()->req_rpb[(IPTR)field->nod_arg[e_fld_stream]].rpb_relation == NULL;
				break;
			case nod_argument:
			case nod_variable:
				break;
			default:
				BUGCHECK(199);			// msg 199 expected field node
		}
	}

	bid* source = (bid*) from_desc->dsc_address;
	bid* destination = (bid*) to_desc->dsc_address;

	// If nothing changed, do nothing.  If it isn't broken, don't fix it.
	if (*source == *destination)
		return;

	UCHAR fromCharSet = from_desc->getCharSet();
	UCHAR toCharSet = to_desc->getCharSet();

	const bool needFilter =
		(from_desc->dsc_sub_type != isc_blob_untyped && to_desc->dsc_sub_type != isc_blob_untyped &&
			(from_desc->dsc_sub_type != to_desc->dsc_sub_type ||
		 		(from_desc->dsc_sub_type == isc_blob_text &&
					fromCharSet != toCharSet &&
					toCharSet != CS_NONE && toCharSet != CS_BINARY &&
					fromCharSet != CS_NONE && fromCharSet != CS_BINARY)));

	if (!needFilter && to_desc->isBlob() &&
        (fromCharSet == CS_NONE || fromCharSet == CS_BINARY) &&
        (toCharSet != CS_NONE && toCharSet != CS_BINARY))
	{
		AutoBlb blob(tdbb, BLB_open(tdbb, tdbb->getTransaction(), source));
		BLB_check_well_formed(tdbb, to_desc, blob.getBlb());
	}

	// If the target node is not a field, just copy the blob id and return.
	if (simpleMove)
	{
		// But if the sub_type or charset is different, create a new blob.
		if (DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype) &&
			DTYPE_IS_BLOB_OR_QUAD(to_desc->dsc_dtype) &&
			needFilter)
		{
			UCharBuffer bpb;
			BLB_gen_bpb_from_descs(from_desc, to_desc, bpb);

			Database* dbb = tdbb->getDatabase();
			const USHORT pageSpace = dbb->dbb_flags & DBB_read_only ?
				dbb->dbb_page_manager.getTempPageSpaceID(tdbb) : DB_PAGE_SPACE;

			copy_blob(tdbb, source, destination, bpb.getCount(), bpb.begin(), pageSpace);
		}
		else
			*destination = *source;

		return;
	}

	jrd_req* request = tdbb->getRequest();
	const USHORT id = (USHORT) (IPTR) field->nod_arg[e_fld_id];
	record_param* rpb = &request->req_rpb[(IPTR)field->nod_arg[e_fld_stream]];
	jrd_rel* relation = rpb->rpb_relation;

	if (relation->isVirtual()) {
		ERR_post(Arg::Gds(isc_read_only));
	}

	RelationPages* relPages = relation->getPages(tdbb);
	Record* record = rpb->rpb_record;

	// If either the source value is null or the blob id itself is null
	// (all zeros), then the blob is null.

	if ((request->req_flags & req_null) || source->isEmpty())
	{
		SET_NULL(record, id);
		destination->clear();
		return;
	}

	CLEAR_NULL(record, id);
	jrd_tra* transaction = request->req_transaction;

	// If the target is a view, this must be from a view update trigger.
	// Just pass the blob id thru.

	if (relation->rel_view_rse)
	{
		// But if the sub_type or charset is different, create a new blob.
		if (DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype) &&
			DTYPE_IS_BLOB_OR_QUAD(to_desc->dsc_dtype) &&
			needFilter)
		{
			UCharBuffer bpb;
			BLB_gen_bpb_from_descs(from_desc, to_desc, bpb);

			Database* dbb = tdbb->getDatabase();
			const USHORT pageSpace = dbb->dbb_flags & DBB_read_only ?
				dbb->dbb_page_manager.getTempPageSpaceID(tdbb) : DB_PAGE_SPACE;

			copy_blob(tdbb, source, destination, bpb.getCount(), bpb.begin(), pageSpace);
		}
		else
			*destination = *source;

		return;
	}

	// If the source is a permanent blob, then the blob must be copied.
	// Otherwise find the temporary blob referenced.

	ArrayField* array = NULL;
	BlobIndex* blobIndex;
	blb* blob = NULL;
	bool materialized_blob; // Set if we materialized temporary blob in this routine

	UCharBuffer bpb;
	if (needFilter)
		BLB_gen_bpb_from_descs(from_desc, to_desc, bpb);

	while (true)
	{
		materialized_blob = false;
		blobIndex = NULL;
		if (source->bid_internal.bid_relation_id || needFilter)
		{
			blob = copy_blob(tdbb, source, destination,
							 bpb.getCount(), bpb.begin(), relPages->rel_pg_space_id);
		}
		else if ((to_desc->dsc_dtype == dtype_array) && (array = find_array(transaction, source)) &&
			(blob = store_array(tdbb, transaction, source)))
		{
			materialized_blob = true;
		}
		else
		{
			if (transaction->tra_blobs->locate(source->bid_temp_id()))
			{
				blobIndex = &transaction->tra_blobs->current();
				if (blobIndex->bli_materialized)
				{
					if (blobIndex->bli_request)
					{
						// Walk through call stack looking if our BLOB is
						// owned by somebody from our call chain
						jrd_req* temp_req = request;
						do {
							if (blobIndex->bli_request == temp_req)
								break;
							temp_req = temp_req->req_caller;
						} while (temp_req);
						if (!temp_req)
						{
							// Trying to use temporary id of materialized blob from another request
							ERR_post(Arg::Gds(isc_bad_segstr_id));
						}
					}
					source = &blobIndex->bli_blob_id;
					continue;
				}

				materialized_blob = true;
				blob = blobIndex->bli_blob_object;

				if (!blob || !(blob->blb_flags & BLB_closed))
				{
					ERR_post(Arg::Gds(isc_bad_segstr_id));
				}

				if (blob->blb_level && (blob->blb_pg_space_id != relPages->rel_pg_space_id))
				{
					const ULONG oldTempID = blob->blb_temp_id;
					blb* newBlob = copy_blob(tdbb, source, destination,
						bpb.getCount(), bpb.begin(), relPages->rel_pg_space_id);

					transaction->tra_blobs->locate(newBlob->blb_temp_id);
					BlobIndex* newBlobIndex = &transaction->tra_blobs->current();

					transaction->tra_blobs->locate(oldTempID);
					blobIndex = &transaction->tra_blobs->current();

					newBlobIndex->bli_blob_object = blob;
					blobIndex->bli_blob_object = newBlob;

					blob->blb_temp_id = newBlob->blb_temp_id;
					newBlob->blb_temp_id = oldTempID;

					BLB_cancel(tdbb, blob);
					blob = newBlob;

					transaction->tra_blobs->locate(blob->blb_temp_id);
					blobIndex = &transaction->tra_blobs->current();
				}
			}
		}

		if (!blob || !(blob->blb_flags & BLB_closed))
		{
			ERR_post(Arg::Gds(isc_bad_segstr_id));
		}

		break;
	}

	blob->blb_relation = relation;
	blob->blb_sub_type = to_desc->getBlobSubType();
	blob->blb_charset = to_desc->getCharSet();
	destination->set_permanent(relation->rel_id, DPM_store_blob(tdbb, blob, record));
	// This is the only place in the engine where blobs are materialized
	// If new places appear code below should transform to common sub-routine
	if (materialized_blob)
	{
		// hvlad: we have possible thread switch in DPM_store_blob above and somebody
		// can modify transaction->tra_blobs therefore we must update our blobIndex
		if (!transaction->tra_blobs->locate(blob->blb_temp_id))
		{
			// If we didn't find materialized blob in transaction blob index it
			// means memory structures are inconsistent and crash is appropriate
			BUGCHECK(305); // msg 305 Blobs accounting is inconsistent
		}
		blobIndex = &transaction->tra_blobs->current();

		blobIndex->bli_materialized = true;
		blobIndex->bli_blob_id = *destination;
		// Assign temporary BLOB ownership to top-level request if it is not assigned yet
		jrd_req* own_request;
		if (blobIndex->bli_request) {
			own_request = blobIndex->bli_request;
		}
		else
		{
			own_request = request;
			while (own_request->req_caller)
				own_request = own_request->req_caller;
			blobIndex->bli_request = own_request;
			own_request->req_blobs.add(blob->blb_temp_id);
		}
		// Not sure that this ownership is entirely correct for arrays, but
		// even if I make mistake here widening array lifetime this should not hurt much
		if (array)
			array->arr_request = own_request;
	}

	release_blob(blob, !materialized_blob);
}


blb* BLB_open(thread_db* tdbb, jrd_tra* transaction, const bid* blob_id)
{
/**************************************
 *
 *      B L B _ o p e n
 *
 **************************************
 *
 * Functional description
 *      Open an existing blob.
 *
 **************************************/

	SET_TDBB(tdbb);
	return BLB_open2(tdbb, transaction, blob_id, 0, 0);
}


blb* BLB_open2(thread_db* tdbb,
			  jrd_tra* transaction, const bid* blob_id,
			  USHORT bpb_length, const UCHAR* bpb,
			  bool external_call)
{
/**************************************
 *
 *      B L B _ o p e n 2
 *
 **************************************
 *
 * Functional description
 *      Open an existing blob.
 *      Basically BLB_open() with BPB structure.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// Handle filter case
	SSHORT from, to;
	SSHORT from_charset, to_charset;
	bool from_type_specified;
	bool from_charset_specified;
	bool to_type_specified;
	bool to_charset_specified;

	gds__parse_bpb2(bpb_length,
					bpb,
					&from,
					&to,
					reinterpret_cast<USHORT*>(&from_charset),	// safe - alignment not changed
					reinterpret_cast<USHORT*>(&to_charset),		// safe - alignment not changed
					&from_type_specified, &from_charset_specified,
					&to_type_specified, &to_charset_specified);

	blb* const blob = allocate_blob(tdbb, transaction);

	bool try_relations = false;
	BlobIndex* current = NULL;

	if (!blob_id->bid_internal.bid_relation_id)
	{
		if (blob_id->isEmpty())
			blob->blb_flags |= BLB_eof;
		else
		{
			/* Note: Prior to 1991, we would immediately report bad_segstr_id here,
			 * but then we decided to allow a newly created blob to be opened,
			 * leaving the possibility of receiving a garbage blob ID from
			 * the application.
			 * The following does some checks to try and product ourselves
			 * better.  94-Jan-07 Daves.
			 */

			// Search the index of transaction blobs for a match
			const blb* new_blob = NULL;
			if (transaction->tra_blobs->locate(blob_id->bid_temp_id()))
			{
				current = &transaction->tra_blobs->current();
				if (!current->bli_materialized)
					new_blob = current->bli_blob_object;
				else
					blob_id = &current->bli_blob_id;
			}

			if (!current || !current->bli_materialized)
			{
				if (!new_blob || !(new_blob->blb_flags & BLB_temporary) ||
					!(new_blob->blb_flags & BLB_closed))
				{
					ERR_post(Arg::Gds(isc_bad_segstr_id));
				}

				blob->blb_lead_page = new_blob->blb_lead_page;
				blob->blb_max_sequence = new_blob->blb_max_sequence;
				blob->blb_count = new_blob->blb_count;
				blob->blb_length = new_blob->blb_length;
				blob->blb_max_segment = new_blob->blb_max_segment;
				blob->blb_level = new_blob->blb_level;
				blob->blb_flags = new_blob->blb_flags & BLB_stream;
				blob->blb_pg_space_id = new_blob->blb_pg_space_id;

				if (new_blob->blb_temp_size > 0)
				{
					transaction->getBlobSpace()->read(new_blob->blb_temp_offset,
						blob->getBuffer(), new_blob->blb_temp_size);
				}

				const vcl* pages = new_blob->blb_pages;
				if (pages)
				{
					vcl* new_pages = vcl::newVector(*transaction->tra_pool, *pages);
					blob->blb_pages = new_pages;
				}

				if (blob->blb_level == 0)
				{
					blob->blb_space_remaining =
						new_blob->blb_clump_size - new_blob->blb_space_remaining;
					blob->blb_segment =
						(UCHAR*) ((blob_page*) blob->getBuffer())->blp_page;
				}
			}
			else
				try_relations = true;
		}
	}
	else
		try_relations = true;

	if (try_relations)
	{
		// Ordinarily, we would call MET_relation to get the relation id.
		// However, since the blob id must be considered suspect, this is
		// not a good idea.  On the other hand, if we don't already
		// know about the relation, the blob id has got to be invalid
		// anyway.

		vec<jrd_rel*>* vector = dbb->dbb_relations;

		if (blob_id->bid_internal.bid_relation_id >= vector->count() ||
			!(blob->blb_relation = (*vector)[blob_id->bid_internal.bid_relation_id] ) )
		{
				ERR_post(Arg::Gds(isc_bad_segstr_id));
		}

		blob->blb_pg_space_id = blob->blb_relation->getPages(tdbb)->rel_pg_space_id;
		DPM_get_blob(tdbb, blob, blob_id->get_permanent_number(), false, (SLONG) 0);

		// If the blob is known to be damaged, ignore it.

		if (blob->blb_flags & BLB_damaged)
		{
			if (!(dbb->dbb_flags & DBB_damaged))
				IBERROR(194);	// msg 194 blob not found

			blob->blb_flags |= BLB_eof;
			blob->blb_count = 0;
			blob->blb_max_segment = 0;
			blob->blb_length = 0;
			return blob;
		}

		// Get first data page in anticipation of reading.

		if (blob->blb_level == 0)
			blob->blb_segment = blob->getBuffer();
	}

	UCharBuffer new_bpb;

	if (external_call && ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		if (!from_type_specified)
			from = blob->blb_sub_type;
		if (!from_charset_specified)
			from_charset = blob->blb_charset;

		if (!to_type_specified && from == isc_blob_text)
			to = isc_blob_text;
		if (!to_charset_specified && from == isc_blob_text)
			to_charset = CS_dynamic;

		BLB_gen_bpb(from, to, from_charset, to_charset, new_bpb);
		bpb = new_bpb.begin();
		bpb_length = new_bpb.getCount();
	}

	//blob->blb_target_interp = to_charset;
	//blob->blb_source_interp = from_charset;

	BlobFilter* filter = NULL;
	bool filter_required = false;
	if (to && from != to)
	{
		filter = find_filter(tdbb, from, to);
		filter_required = true;
	}
	else if (to == isc_blob_text && (from_charset != to_charset))
	{
		if (from_charset == CS_dynamic)
			from_charset = tdbb->getAttachment()->att_charset;
		if (to_charset == CS_dynamic)
			to_charset = tdbb->getAttachment()->att_charset;

		if ((to_charset != CS_NONE) && (from_charset != CS_NONE) &&
			(to_charset != CS_BINARY) && (from_charset != CS_BINARY) &&
			(from_charset != to_charset))
		{
			filter = find_filter(tdbb, from, to);
			filter_required = true;
		}
	}

	if (filter_required)
	{
		BlobControl* control = 0;
		BLF_open_blob(tdbb, transaction, &control, blob_id, bpb_length, bpb, blob_filter, filter);
		blob->blb_filter = control;
		blob->blb_max_segment = control->ctl_max_segment;
		blob->blb_count = control->ctl_number_segments;
		blob->blb_length = control->ctl_total_length;
		return blob;
	}

	return blob;
}


void BLB_put_data(thread_db* tdbb, blb* blob, const UCHAR* buffer, SLONG length)
{
/**************************************
 *
 *      B L B _ p u t _ d a t a
 *
 **************************************
 *
 * Functional description
 *      Write data to a blob.
 *      Don't worry about segment boundaries.
 *
 **************************************/
	SET_TDBB(tdbb);
	const BLOB_PTR* p = buffer;

	while (length > 0)
	{
		// ASF: the comment below was copied from BLB_get_data
		// I have no idea why this limit is 32768 instead of 32767
		// 1994-August-12 David Schnepper
		const USHORT n = (USHORT) MIN(length, (SLONG) 32768);
		BLB_put_segment(tdbb, blob, p, n);
		p += n;
		length -= n;
	}
}


void BLB_put_segment(thread_db* tdbb, blb* blob, const UCHAR* seg, USHORT segment_length)
{
/**************************************
 *
 *      B L B _ p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *      Add a segment to a blob.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	const BLOB_PTR* segment = seg;

	// Make sure blob is a temporary blob.  If not, complain bitterly.

	if (!(blob->blb_flags & BLB_temporary))
		IBERROR(195);			// msg 195 cannot update old blob

	if (blob->blb_filter)
	{
		BLF_put_segment(tdbb, &blob->blb_filter, segment_length, segment);
		return;
	}

	// Account for new segment

	blob->blb_count++;
	blob->blb_length += segment_length;

	if (segment_length > blob->blb_max_segment)
		blob->blb_max_segment = segment_length;

	// Compute the effective length of the segment (counts length unless
	// the blob is a stream blob).

	ULONG length;				// length of segment + overhead
	bool length_flag;
	if (SEGMENTED(blob))
	{
		length = segment_length + 2;
		length_flag = true;
	}
	else
	{
		length = segment_length;
		length_flag = false;
	}

	// Case 0: Transition from small blob to medium size blob.  This really
	// just does a form transformation and drops into the next case.

	if (blob->blb_level == 0 && length > (ULONG) blob->blb_space_remaining)
	{
		jrd_tra* transaction = blob->blb_transaction;
		blob->blb_pages = vcl::newVector(*transaction->tra_pool, 0);
		const USHORT l = dbb->dbb_page_size - BLP_SIZE;
		blob->blb_space_remaining += l - blob->blb_clump_size;
		blob->blb_clump_size = l;
		blob->blb_level = 1;
	}

	// Case 1: The segment fits.  In what is immaterial.  Just move the segment and get out!

	BLOB_PTR* p = blob->blb_segment;

	if (length_flag && blob->blb_space_remaining >= 2)
	{
		const BLOB_PTR* q = (UCHAR*) &segment_length;
		*p++ = *q++;
		*p++ = *q++;
		blob->blb_space_remaining -= 2;
		length_flag = false;
	}

	if (!length_flag && segment_length <= blob->blb_space_remaining)
	{
		blob->blb_space_remaining -= segment_length;
		memcpy(p, segment, segment_length);
		blob->blb_segment = p + segment_length;
		return;
	}

	// The segment cannot be contained in the current clump.  What does
	// bit is copied to the current page image.  Then allocate a page to
	// hold the current page, copy the page image to the buffer, and release
	// the page.  Since a segment can be much larger than a page, this whole
	// mess is done in a loop.

	while (length_flag || segment_length)
	{
		// Move what fits.  At this point, the length is known not to fit.

		const USHORT l = MIN(segment_length, blob->blb_space_remaining);

		if (!length_flag && l)
		{
			segment_length -= l;
			blob->blb_space_remaining -= l;

			memcpy(p, segment, l);
			p += l;
			segment += l;

			if (segment_length == 0)
			{
				blob->blb_segment = p;
				return;
			}
		}

		// Data page is full.  Add the page to the blob data structure.

		insert_page(tdbb, blob);
		blob->blb_sequence++;

		// Get ready to start filling the next page.

		blob_page* page = (blob_page*) blob->getBuffer();
		p = blob->blb_segment = (UCHAR *) page->blp_page;
		blob->blb_space_remaining = blob->blb_clump_size;

		// If there's still a length waiting to be moved, move it already!

		if (length_flag)
		{
			const BLOB_PTR* q = (UCHAR*) &segment_length;
			*p++ = *q++;
			*p++ = *q++;
			blob->blb_space_remaining -= 2;
			length_flag = false;
			blob->blb_segment = p;
		}
	}

}


void BLB_put_slice(	thread_db*	tdbb,
					jrd_tra*		transaction,
					bid*		blob_id,
					const UCHAR*	sdl,
					USHORT	param_length,
					const UCHAR*	param,
					SLONG	slice_length,
					UCHAR*	slice_addr)
{
/**************************************
 *
 *      B L B _ p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *      Put a slice of an array.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

	// Do initial parse of slice description to get relation and field identification
	sdl_info info;
	if (SDL_info(tdbb->tdbb_status_vector, sdl, &info, 0))
		ERR_punt();

	jrd_rel* relation;
	if (info.sdl_info_relation.length()) {
		relation = MET_lookup_relation(tdbb, info.sdl_info_relation);
	}
	else {
		relation = MET_relation(tdbb, info.sdl_info_rid);
	}

	if (!relation) {
		IBERROR(196);			// msg 196 relation for array not known
	}

	SSHORT	n;
	if (info.sdl_info_field.length()) {
	    n = MET_lookup_field(tdbb, relation, info.sdl_info_field);
	}
	else {
		n = info.sdl_info_fid;
	}

	// Make sure relation is scanned
	MET_scan_relation(tdbb, relation);

	jrd_fld* field;
	if (n < 0 || !(field = MET_get_field(relation, n))) {
		IBERROR(197);			// msg 197 field for array not known
	}

	ArrayField* array_desc = field->fld_array;
	if (!array_desc)
	{
		ERR_post(Arg::Gds(isc_invalid_dimension) << Arg::Num(0) << Arg::Num(1));
	}

	// Find and/or allocate array block.  There are three distinct cases:

	// 1.  Array is totally new.
	// 2.  Array is still in "temporary" state.
	// 3.  Array exists and is being updated.

	ArrayField* array = 0;
	array_slice arg;
	if (blob_id->bid_internal.bid_relation_id)
	{
		for (array = transaction->tra_arrays; array; array = array->arr_next)
		{
			if (array->arr_blob && array->arr_blob->blb_blob_id == *blob_id)
			{
				break;
			}
		}
		if (array)
		{
			arg.slice_high_water = array->arr_data + array->arr_effective_length;
		}
		else
		{
			// CVC: maybe char temp[IAD_LEN(16)]; may work but it won't be aligned.
			SLONG temp[IAD_LEN(16) / 4];
			Ods::InternalArrayDesc* p_ads = reinterpret_cast<Ods::InternalArrayDesc*>(temp);
			blb* blob = BLB_get_array(tdbb, transaction, blob_id, p_ads);
			array =	alloc_array(transaction, p_ads);
			array->arr_effective_length = blob->blb_length - array->arr_desc.iad_length;
			BLB_get_data(tdbb, blob, array->arr_data, array->arr_desc.iad_total_length);
			arg.slice_high_water = array->arr_data + array->arr_effective_length;
			array->arr_blob = allocate_blob(tdbb, transaction);
			(array->arr_blob)->blb_blob_id = *blob_id;
		}
	}
	else if (blob_id->bid_temp_id())
	{
		array = find_array(transaction, blob_id);
		if (!array) {
			ERR_post(Arg::Gds(isc_invalid_array_id));
		}

		arg.slice_high_water = array->arr_data + array->arr_effective_length;
	}
	else
	{
		array = alloc_array(transaction, &array_desc->arr_desc);
		arg.slice_high_water = array->arr_data;
	}

	// Walk array

	arg.slice_desc = info.sdl_info_element;
	arg.slice_desc.dsc_address = slice_addr;
	arg.slice_end = slice_addr + slice_length;
	arg.slice_count = 0;
	arg.slice_element_length = info.sdl_info_element.dsc_length;
	arg.slice_direction = array_slice::slc_writing_array;	// storing INTO array
	arg.slice_base = array->arr_data;

	SLONG variables[64];
	memcpy(variables, param, MIN(sizeof(variables), param_length));

	if (SDL_walk(tdbb->tdbb_status_vector, sdl, array->arr_data, &array_desc->arr_desc,
				 variables, slice_callback, &arg))
	{
		ERR_punt();
	}

	const SLONG length = arg.slice_high_water - array->arr_data;

	if (length > array->arr_effective_length) {
		array->arr_effective_length = length;
	}

	blob_id->set_temporary(array->arr_temp_id);
}


void BLB_release_array(ArrayField* array)
{
/**************************************
 *
 *      B L B _ r e l e a s e _ a r r a y
 *
 **************************************
 *
 * Functional description
 *      Release an array block and friends and relations.
 *
 **************************************/

	delete[] array->arr_data;

	jrd_tra* transaction = array->arr_transaction;
	if (transaction)
	{
		for (ArrayField** ptr = &transaction->tra_arrays; *ptr; ptr = &(*ptr)->arr_next)
		{
			if (*ptr == array)
			{
				*ptr = array->arr_next;
				break;
			}
		}
	}

	delete array;
}


void BLB_scalar(thread_db*		tdbb,
				jrd_tra*		transaction,
				const bid*		blob_id,
				USHORT			count,
				const SLONG*	subscripts,
				impure_value*	value)
{
/**************************************
 *
 *      B L B _ s c a l a r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	SLONG stuff[IAD_LEN(16) / 4];

	SET_TDBB(tdbb);

	Ods::InternalArrayDesc* array_desc = (Ods::InternalArrayDesc*) stuff;
	blb* blob = BLB_get_array(tdbb, transaction, blob_id, array_desc);

	// Get someplace to put data.
	// We need DOUBLE_ALIGNed buffer, that's why some tricks
	HalfStaticArray<double, 64> temp;
	dsc desc = array_desc->iad_rpt[0].iad_desc;
	desc.dsc_address = reinterpret_cast<UCHAR*>(temp.getBuffer((desc.dsc_length / sizeof(double)) +
												(desc.dsc_length % sizeof(double) ? 1 : 0)));

	const SLONG number = SDL_compute_subscript(tdbb->tdbb_status_vector, array_desc, count, subscripts);
	if (number < 0)
	{
		BLB_close(tdbb, blob);
		ERR_punt();
	}

	const SLONG offset = number * array_desc->iad_element_length;
	BLB_lseek(blob, 0, offset + (SLONG) array_desc->iad_length);
	BLB_get_segment(tdbb, blob, desc.dsc_address, desc.dsc_length);

	// If we have run out of data, then clear the data buffer.

	if (blob->blb_flags & BLB_eof) {
		memset(desc.dsc_address, 0, (int) desc.dsc_length);
	}
	EVL_make_value(tdbb, &desc, value);
	BLB_close(tdbb, blob);
}


static ArrayField* alloc_array(jrd_tra* transaction, Ods::InternalArrayDesc* proto_desc)
{
/**************************************
 *
 *      a l l o c _ a r r a y
 *
 **************************************
 *
 * Functional description
 *      Allocate an array block based on a prototype array descriptor.
 *
 **************************************/

	// Compute size and allocate block

	const USHORT n = MAX(proto_desc->iad_struct_count, proto_desc->iad_dimensions);
	ArrayField* array = FB_NEW_RPT(*transaction->tra_pool, n) ArrayField();

	// Copy prototype descriptor

	memcpy(&array->arr_desc, proto_desc, proto_desc->iad_length);

	// Link into transaction block

	array->arr_next = transaction->tra_arrays;
	transaction->tra_arrays = array;
	array->arr_transaction = transaction;

	// Allocate large block to hold array

	array->arr_data = FB_NEW(*transaction->tra_pool) UCHAR[array->arr_desc.iad_total_length];
	array->arr_temp_id = ++transaction->tra_next_blob_id;

	return array;
}


static blb* allocate_blob(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *      a l l o c a t e _ b l o b
 *
 **************************************
 *
 * Functional description
 *      Create a shiney, new, empty blob.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// If we are in an autonomous transaction, link the blob on the transaction started by the user.
	while (transaction->tra_outer)
		transaction = transaction->tra_outer;

	// Create a blob large enough to hold a single data page.

	blb* blob = FB_NEW(*transaction->tra_pool) blb(*transaction->tra_pool, dbb->dbb_page_size);
	blob->blb_attachment = tdbb->getAttachment();
	blob->blb_transaction = transaction;

	// Compute some parameters governing various maximum sizes based on
	// database page size.

	blob->blb_clump_size = dbb->dbb_page_size -
							sizeof(Ods::data_page) -
							sizeof(Ods::data_page::dpg_repeat) -
							sizeof(Ods::blh);
	blob->blb_max_pages = blob->blb_clump_size >> SHIFTLONG;
	blob->blb_pointers = (dbb->dbb_page_size - BLP_SIZE) >> SHIFTLONG;
	// This code is to handle huge number of blob updates done in one transaction.
	// Blob index counter may wrap in this case
	do {
		transaction->tra_next_blob_id++;
		// Do not generate null blob ID
		if (!transaction->tra_next_blob_id)
			transaction->tra_next_blob_id++;
	} while (!transaction->tra_blobs->add(BlobIndex(transaction->tra_next_blob_id, blob)));
	blob->blb_temp_id = transaction->tra_next_blob_id;

	return blob;
}


static ISC_STATUS blob_filter(USHORT	action,
							  BlobControl*	control)
{
/**************************************
 *
 *      b l o b _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *      Filter of last resort for filtered blob access handled by Y-valve.
 *
 **************************************/

	// Note: Cannot remove this JRD_get_thread_data without API change to blob filter routines

	thread_db* tdbb = JRD_get_thread_data();

	jrd_tra* const transaction = reinterpret_cast<jrd_tra*>(control->ctl_internal[1]);
	bid* blob_id = reinterpret_cast<bid*>(control->ctl_internal[2]);

#ifdef DEV_BUILD
	if (transaction) {
		BLKCHK(transaction, type_tra);
	}
#endif

	blb* blob = NULL;

	switch (action)
	{
	case isc_blob_filter_open:
		blob = BLB_open2(tdbb, transaction, blob_id, 0, 0);
		control->source_handle = blob;
		control->ctl_total_length = blob->blb_length;
		control->ctl_max_segment = blob->blb_max_segment;
		control->ctl_number_segments = blob->blb_count;
		return FB_SUCCESS;

	case isc_blob_filter_get_segment:
		blob = control->source_handle;
		control->ctl_segment_length =
			BLB_get_segment(tdbb, blob, control->ctl_buffer, control->ctl_buffer_length);
		if (blob->blb_flags & BLB_eof) {
			return isc_segstr_eof;
		}
		if (blob->blb_fragment_size) {
			return isc_segment;
		}
		return FB_SUCCESS;

	case isc_blob_filter_create:
		control->source_handle = BLB_create2(tdbb, transaction, blob_id, 0, NULL);
		return FB_SUCCESS;

	case isc_blob_filter_put_segment:
		blob = control->source_handle;
		BLB_put_segment(tdbb, blob, control->ctl_buffer, control->ctl_buffer_length);
		return FB_SUCCESS;

	case isc_blob_filter_close:
		BLB_close(tdbb, control->source_handle);
		return FB_SUCCESS;

	case isc_blob_filter_alloc:
	    // pointer to ISC_STATUS!!!
		return (ISC_STATUS) FB_NEW(*transaction->tra_pool) BlobControl(*transaction->tra_pool);

	case isc_blob_filter_free:
		delete control;
		return FB_SUCCESS;

	case isc_blob_filter_seek:
		fb_assert(false);
		// fail through ...

	default:
		ERR_post(Arg::Gds(isc_uns_ext));
		return FB_SUCCESS;
	}
}


static blb* copy_blob(thread_db* tdbb, const bid* source, bid* destination,
					  USHORT bpb_length, const UCHAR* bpb,
					  USHORT destPageSpaceID)
{
/**************************************
 *
 *      c o p y _ b l o b
 *
 **************************************
 *
 * Functional description
 *      Make a copy of an existing blob.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request ? request->req_transaction : tdbb->getTransaction();
	blb* input = BLB_open2(tdbb, transaction, source, bpb_length, bpb);
	blb* output = BLB_create(tdbb, transaction, destination);
	output->blb_sub_type = input->blb_sub_type;
	if (destPageSpaceID) {
		output->blb_pg_space_id = destPageSpaceID;
	}

	if (input->blb_flags & BLB_stream) {
		output->blb_flags |= BLB_stream;
	}

	HalfStaticArray<UCHAR, 2048> buffer;
	UCHAR* buff = buffer.getBuffer(input->blb_max_segment);

	while (true)
	{
		const USHORT length = BLB_get_segment(tdbb, input, buff, input->blb_max_segment);
		if (input->blb_flags & BLB_eof) {
			break;
		}
		BLB_put_segment(tdbb, output, buff, length);
	}

	BLB_close(tdbb, input);
	BLB_close(tdbb, output);

	return output;
}


static void delete_blob(thread_db* tdbb, blb* blob, ULONG prior_page)
{
/**************************************
 *
 *      d e l e t e _ b l o b
 *
 **************************************
 *
 * Functional description
 *      Delete all disk storage associated with blob.  This can be used
 *      to either abort a temporary blob or get rid of an unwanted and
 *      unloved permanent blob.  The routine deletes only blob page --
 *      somebody else will have to worry about the blob root.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const USHORT pageSpaceID = blob->blb_pg_space_id;

	if (dbb->dbb_flags & DBB_read_only)
	{
		const USHORT tempSpaceID = dbb->dbb_page_manager.getTempPageSpaceID(tdbb);

		if (pageSpaceID != tempSpaceID)
		{
			ERR_post(Arg::Gds(isc_read_only_database));
		}
	}

	// Level 0 blobs don't need cleanup

	if (blob->blb_level == 0)
		return;

	const PageNumber prior(pageSpaceID, prior_page);

	// Level 1 blobs just need the root page level released

	vcl* vector = blob->blb_pages;
	vcl::iterator ptr = vector->begin();
	const vcl::iterator end = vector->end();

	if (blob->blb_level == 1)
	{
		for (; ptr < end; ++ptr)
		{
			if (*ptr) {
				PAG_release_page(tdbb, PageNumber(pageSpaceID, *ptr), prior);
			}
		}
		return;
	}

	// Level 2 blobs need a little more work to keep the page precedence
	// in order.  The basic problem is that the pointer page has to be
	// released before the data pages that it points to.  Sigh.

	WIN window(pageSpaceID, -1);
	window.win_flags = WIN_large_scan;
	window.win_scans = 1;

	Array<UCHAR> data(dbb->dbb_page_size);
	UCHAR* const buffer = data.begin();

	for (; ptr < end; ptr++)
	{
		if ( (window.win_page = *ptr) )
		{
			const blob_page* page = (blob_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_blob);
			memcpy(buffer, page, dbb->dbb_page_size);
			CCH_RELEASE_TAIL(tdbb, &window);

			const PageNumber page1(pageSpaceID, *ptr);
			PAG_release_page(tdbb, page1, prior);
			page = (blob_page*) buffer;
			const SLONG* ptr2 = page->blp_page;
			for (const SLONG* const end2 = ptr2 + blob->blb_pointers; ptr2 < end2; ptr2++)
			{
				if (*ptr2) {
					PAG_release_page(tdbb, PageNumber(pageSpaceID, *ptr2), page1);
				}
			}
		}
	}
}


static void delete_blob_id(thread_db* tdbb,
						   const bid* blob_id, SLONG prior_page, jrd_rel* relation)
{
/**************************************
 *
 *      d e l e t e _ b l o b _ i d
 *
 **************************************
 *
 * Functional description
 *      Delete an existing blob for purpose of garbage collection.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// If the blob is null, don't bother to delete it.  Reasonable?

	if (blob_id->isEmpty())
		return;

	if (blob_id->bid_internal.bid_relation_id != relation->rel_id)
		CORRUPT(200);			// msg 200 invalid blob id

	// Fetch blob

	blb* blob = allocate_blob(tdbb, dbb->dbb_sys_trans);
	blob->blb_relation = relation;
	blob->blb_pg_space_id = relation->getPages(tdbb)->rel_pg_space_id;
	prior_page = DPM_get_blob(tdbb, blob, blob_id->get_permanent_number(), true, prior_page);

	if (!(blob->blb_flags & BLB_damaged))
		delete_blob(tdbb, blob, prior_page);

	release_blob(blob, true);
}


static ArrayField* find_array(jrd_tra* transaction, const bid* blob_id)
{
/**************************************
 *
 *      f i n d _ a r r a y
 *
 **************************************
 *
 * Functional description
 *      Find array from temporary blob id.
 *
 **************************************/
	ArrayField* array = transaction->tra_arrays;

	for (; array; array = array->arr_next)
	{
		if (array->arr_temp_id == blob_id->bid_temp_id()) {
			break;
		}
	}

	return array;
}


static BlobFilter* find_filter(thread_db* tdbb, SSHORT from, SSHORT to)
{
/**************************************
 *
 *      f i n d _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *      Find blob filter.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	BlobFilter* cache = dbb->dbb_blob_filters;
	for (; cache; cache = cache->blf_next)
	{
		if (cache->blf_from == from && cache->blf_to == to)
			return cache;
	}

	cache = BLF_lookup_internal_filter(tdbb, from, to);
	if (!cache) {
		cache = MET_lookup_filter(tdbb, from, to);
	}

	if (cache)
	{
		cache->blf_next = dbb->dbb_blob_filters;
		dbb->dbb_blob_filters = cache;
	}

	return cache;
}


static blob_page* get_next_page(thread_db* tdbb, blb* blob, WIN * window)
{
/**************************************
 *
 *      g e t _ n e x t _ p a g e
 *
 **************************************
 *
 * Functional description
 *      Read a blob page and copy it into the blob data area.  Return
 *      the next page. if there's no next page, return NULL.
 *
 **************************************/
	if (blob->blb_level == 0 || blob->blb_sequence > blob->blb_max_sequence)
	{
		blob->blb_space_remaining = 0;
		return NULL;
	}

	SET_TDBB(tdbb);
#ifdef SUPERSERVER_V2
	Database* dbb = tdbb->getDatabase();
	SLONG pages[PREFETCH_MAX_PAGES];
#endif

	const vcl* vector = blob->blb_pages;

	blob_page* page = 0;
	// Level 1 blobs are much easier -- page number is in vector.
	if (blob->blb_level == 1)
	{
#ifdef SUPERSERVER_V2
		// Perform prefetch of blob level 1 data pages.

		if (!(blob->blb_sequence % dbb->dbb_prefetch_sequence))
		{
			USHORT sequence = blob->blb_sequence;
			USHORT i = 0;
			while (i < dbb->dbb_prefetch_pages && sequence <= blob->blb_max_sequence)
			{
				 pages[i++] = (*vector)[sequence++];
			}

			CCH_PREFETCH(tdbb, pages, i);
		}
#endif
		window->win_page = (*vector)[blob->blb_sequence];
		page = (blob_page*) CCH_FETCH(tdbb, window, LCK_read, pag_blob);
	}
	else
	{
		window->win_page = (*vector)[blob->blb_sequence / blob->blb_pointers];
		page = (blob_page*) CCH_FETCH(tdbb, window, LCK_read, pag_blob);
#ifdef SUPERSERVER_V2
		// Perform prefetch of blob level 2 data pages.

		USHORT sequence = blob->blb_sequence % blob->blb_pointers;
		if (!(sequence % dbb->dbb_prefetch_sequence))
		{
			ULONG abs_sequence = blob->blb_sequence;
			USHORT i = 0;
			while (i < dbb->dbb_prefetch_pages && sequence < blob->blb_pointers &&
				abs_sequence <= blob->blb_max_sequence)
			{
				pages[i++] = page->blp_page[sequence++];
				abs_sequence++;
			}

			CCH_PREFETCH(tdbb, pages, i);
		}
#endif
		page = (blob_page*) CCH_HANDOFF(tdbb, window,
										page->blp_page[blob->blb_sequence % blob->blb_pointers],
										LCK_read, pag_blob);
	}

	if (page->blp_sequence != (SLONG) blob->blb_sequence)
		CORRUPT(201);			// msg 201 cannot find blob page

	blob->blb_sequence++;

	return page;
}


static void insert_page(thread_db* tdbb, blb* blob)
{
/**************************************
 *
 *      i n s e r t _ p a g e
 *
 **************************************
 *
 * Functional description
 *      A data page has been formatted.  Allocate a physical page,
 *      move the data page to the buffer, and insert the page number
 *      of the new page into the blob data structure.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const USHORT length = dbb->dbb_page_size - blob->blb_space_remaining;
	vcl* vector = blob->blb_pages;
	blob->blb_max_sequence = blob->blb_sequence;

	// Allocate a page for the now full blob data page.  Move the page
	// image to the buffer, and release the page.

	const USHORT pageSpaceID = blob->blb_pg_space_id;

	WIN window(pageSpaceID, -1);
	blob_page* page = (blob_page*) DPM_allocate(tdbb, &window);
	const PageNumber page_number = window.win_page;

	if (blob->blb_sequence == 0)
		blob->blb_lead_page = page_number.getPageNum();

	// Page header is partially populated by DPM_allocate. Preserve it.
	memcpy(
		reinterpret_cast<char*>(page) + sizeof(Ods::pag),
		reinterpret_cast<const char*>(blob->getBuffer()) + sizeof(Ods::pag),
		length - sizeof(Ods::pag));
	page->blp_header.pag_type = pag_blob;

	page->blp_sequence = blob->blb_sequence;
	page->blp_lead_page = blob->blb_lead_page;
	page->blp_length = length - BLP_SIZE;
	CCH_RELEASE(tdbb, &window);

	// If the blob is at level 1, there are two cases.  First, and easiest,
	// is that there is still room in the page vector to hold the pointer.
	// The second case is that the vector is full, and the blob must be
	// transformed into a level 2 blob.

	if (blob->blb_level == 1)
	{
		// See if there is room in the page vector.  If so, just update the vector.

		if (blob->blb_sequence < blob->blb_max_pages)
		{
			if (blob->blb_sequence >= vector->count()) {
				vector->resize(blob->blb_sequence + 1);
			}
			(*vector)[blob->blb_sequence] = page_number.getPageNum();
			return;
		}

		// The vector just overflowed.  Sigh.  Transform blob to level 2.

		blob->blb_level = 2;
		page = (blob_page*) DPM_allocate(tdbb, &window);
		page->blp_header.pag_flags = Ods::blp_pointers;
		page->blp_header.pag_type = pag_blob;
		page->blp_lead_page = blob->blb_lead_page;
		page->blp_length = vector->count() << SHIFTLONG;
		memcpy(page->blp_page, vector->memPtr(), page->blp_length);
		vector->resize(1);
		(*vector)[0] = window.win_page.getPageNum();
		CCH_RELEASE(tdbb, &window);
	}

	// The blob must be level 2.  Find the appropriate pointer page (creating
	// it if need be, and stick the pointer in the appropriate slot.

	USHORT l = blob->blb_sequence / blob->blb_pointers;

	if (l < vector->count())
	{
		window.win_page = (*vector)[l];
		window.win_flags = 0;
		page = (blob_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_blob);
	}
	else if (l < blob->blb_pointers)
	{
		page = (blob_page*) DPM_allocate(tdbb, &window);
		page->blp_header.pag_flags = Ods::blp_pointers;
		page->blp_header.pag_type = pag_blob;
		page->blp_lead_page = blob->blb_lead_page;
		vector->resize(l + 1);
		(*vector)[l] = window.win_page.getPageNum();
	}
	else {
		ERR_post(Arg::Gds(isc_imp_exc) << Arg::Gds(isc_blobtoobig));
	}

	CCH_precedence(tdbb, &window, page_number);
	CCH_MARK(tdbb, &window);
	l = blob->blb_sequence % blob->blb_pointers;
	page->blp_page[l] = page_number.getPageNum();
	page->blp_length = (l + 1) << SHIFTLONG;
	CCH_RELEASE(tdbb, &window);
}


static void move_from_string(thread_db* tdbb, const dsc* from_desc, dsc* to_desc, jrd_nod* field)
{
/**************************************
 *
 *      m o v e _ f r o m _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Perform an assignment to a blob field.  It's capable of handling
 *      strings (and anything that could be converted to strings) by
 *      doing an internal conversion to blob and then calling BLB_move
 *      with that new blob.
 *
 **************************************/
	SET_TDBB (tdbb);

	const UCHAR charSet = INTL_GET_CHARSET(from_desc);
	UCHAR* fromstr = NULL;

	MoveBuffer buffer;
	const int length = MOV_make_string2(tdbb, from_desc, charSet, &fromstr, buffer);
	const UCHAR toCharSet = to_desc->getCharSet();

	if ((charSet == CS_NONE || charSet == CS_BINARY || charSet == toCharSet) &&
		toCharSet != CS_NONE && toCharSet != CS_BINARY)
	{
		if (!INTL_charset_lookup(tdbb, toCharSet)->wellFormed(length, fromstr))
			status_exception::raise(Arg::Gds(isc_malformed_string));
	}

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request ? request->req_transaction : tdbb->getTransaction();

	UCharBuffer bpb;
	BLB_gen_bpb_from_descs(from_desc, to_desc, bpb);

	bid temp_bid;
	temp_bid.clear();
	blb* blob = BLB_create2(tdbb, transaction, &temp_bid, bpb.getCount(), bpb.begin());

	DSC blob_desc;
	blob_desc.clear();

	blob_desc.dsc_scale = to_desc->dsc_scale;	// blob charset
	blob_desc.dsc_flags = (blob_desc.dsc_flags & 0xFF) | (to_desc->dsc_flags & 0xFF00);	// blob collation
	blob_desc.dsc_sub_type = to_desc->dsc_sub_type;
	blob_desc.dsc_dtype = dtype_blob;
	blob_desc.dsc_length = sizeof(ISC_QUAD);
	blob_desc.dsc_address = reinterpret_cast<UCHAR*>(&temp_bid);
	BLB_put_segment(tdbb, blob, fromstr, length);
	BLB_close(tdbb, blob);
	ULONG blob_temp_id = blob->blb_temp_id;
	BLB_move(tdbb, &blob_desc, to_desc, field);

	// 14-June-2004. Nickolay Samofatov
	// The code below saves a lot of memory when bunches of records are
	// converted to blobs from strings. If BLB_move is materialized blob we
	// can discard it without consequences since we know there are no other
	// descriptors using temporary ID of blob we just created. If blob is
	// still temporary we cannot free it as it may now be used inside
	// trigger for updatable view. In theory we could make this decision
	// solely via checking that destination field belongs to updatable
	// view, but direct check that blob is fully materialized should be
	// more future proof.
	// ASF: Blob ID could now be stored in any descriptor of parameters or
	// variables. - 2007-03-24

	if (request)
	{
		if (transaction->tra_blobs->locate(blob_temp_id))
		{
			BlobIndex* current = &transaction->tra_blobs->current();
			if (current->bli_materialized)
			{
				// Delete BLOB from request owned blob list
				jrd_req* blob_request = current->bli_request;
				if (blob_request)
				{
					if (blob_request->req_blobs.locate(blob_temp_id)) {
						blob_request->req_blobs.fastRemove();
					}
					else
					{
						// We should never get here because when bli_request is assigned
						// item should be added to req_blobs array
						fb_assert(false);
					}
				}

				// Free materialized blob handle
				transaction->tra_blobs->fastRemove();
			}
			else
			{
				// But even in bad case when we cannot free blob immediately
				// we may still bind lifetime of blob to current top level request.
				if (!current->bli_request)
				{
					jrd_req* blob_request = request;
					while (blob_request->req_caller)
						blob_request = blob_request->req_caller;

					current->bli_request = blob_request;
					current->bli_request->req_blobs.add(blob_temp_id);
				}
			}
		}
	}
}


static void move_to_string(thread_db* tdbb, dsc* fromDesc, dsc* toDesc)
{
/**************************************
 *
 *      m o v e _ t o _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Perform an assignment from a blob to another datatype.
 *
 **************************************/
	SET_TDBB(tdbb);

	fb_assert(DTYPE_IS_BLOB_OR_QUAD(fromDesc->dsc_dtype));
	fb_assert(!DTYPE_IS_BLOB_OR_QUAD(toDesc->dsc_dtype));

	dsc blobAsText;
	blobAsText.dsc_dtype = dtype_text;

	if (DTYPE_IS_TEXT(toDesc->dsc_dtype))
		blobAsText.dsc_ttype() = toDesc->dsc_ttype();
	else
		blobAsText.dsc_ttype() = ttype_ascii;

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request ? request->req_transaction : tdbb->getTransaction();

	UCharBuffer bpb;
	BLB_gen_bpb_from_descs(fromDesc, &blobAsText, bpb);

	blb* blob = BLB_open2(tdbb, transaction,
		(bid*) fromDesc->dsc_address, bpb.getCount(), bpb.begin());

	const CharSet* fromCharSet = INTL_charset_lookup(tdbb, fromDesc->dsc_scale);
	const CharSet* toCharSet = INTL_charset_lookup(tdbb, INTL_GET_CHARSET(&blobAsText));

	HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;
	buffer.getBuffer((blob->blb_length / fromCharSet->minBytesPerChar()) * toCharSet->maxBytesPerChar());
	const ULONG len = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCapacity(), true);

	if (len > MAX_COLUMN_SIZE - sizeof(USHORT))
		ERR_post(Arg::Gds(isc_arith_except) << Arg::Gds(isc_blob_truncation));

	blobAsText.dsc_address = buffer.begin();
	blobAsText.dsc_length = (USHORT) len;

	MOV_move(tdbb, &blobAsText, toDesc);
}


static void release_blob(blb* blob, const bool purge_flag)
{
/**************************************
 *
 *      r e l e a s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *      Release a blob and associated blocks.  Among other things,
 *      disconnect it from the transaction.  However, if purge_flag
 *      is false, then only release the associated blocks.
 *
 **************************************/
	jrd_tra* const transaction = blob->blb_transaction;

	// Disconnect blob from transaction block.

	if (purge_flag)
	{
		if (transaction->tra_blobs->locate(blob->blb_temp_id))
		{
			jrd_req* blob_request = transaction->tra_blobs->current().bli_request;

			if (blob_request)
			{
				if (blob_request->req_blobs.locate(blob->blb_temp_id))
					blob_request->req_blobs.fastRemove();
				else
				{
					// We should never get here because when bli_request is assigned
					// item should be added to req_blobs array
					fb_assert(false);
				}
			}

			transaction->tra_blobs->fastRemove();
		}
		else
		{
			// We should never get here because allocate_blob stores each blob object
			// in tra_blobs
			fb_assert(false);
		}
	}

	delete blob->blb_pages;
	blob->blb_pages = NULL;

	if ((blob->blb_flags & BLB_temporary) && blob->blb_temp_size > 0)
	{
		blob->blb_transaction->getBlobSpace()->releaseSpace(blob->blb_temp_offset, blob->blb_temp_size);
	}

	delete blob;
}


static void slice_callback(array_slice* arg, ULONG /*count*/, DSC* descriptors)
{
/**************************************
 *
 *      s l i c e _ c a l l b a c k
 *
 **************************************
 *
 * Functional description
 *      Perform slice assignment.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsc* array_desc = descriptors;
	dsc* slice_desc = &arg->slice_desc;
	BLOB_PTR* const next = slice_desc->dsc_address + arg->slice_element_length;

	if (next > arg->slice_end)
		ERR_post(Arg::Gds(isc_out_of_bounds));

	if (array_desc->dsc_address < arg->slice_base)
		ERR_error(198);			// msg 198 array subscript computation error

	if (arg->slice_direction == array_slice::slc_writing_array)
	{
		// Storing INTO array
		// FROM slice_desc TO array_desc

		// If storing beyond the high-water mark, ensure elements
		// from the high-water mark to current position are zeroed

		// Since we are only initializing, it makes sense to throw away
		// the constness of arg->slice_high_water.
		const SLONG l = array_desc->dsc_address - arg->slice_high_water;
		if (l > 0)
			memset(const_cast<BLOB_PTR*>(arg->slice_high_water), 0, l);

		// The individual elements of a varying string array may not be aligned
		// correctly.  If they aren't, some RISC machines may break.  In those
		// cases, calculate the actual length and then move the length and text manually.

		if (array_desc->dsc_dtype == dtype_varying &&
			(U_IPTR) array_desc->dsc_address !=
				FB_ALIGN((U_IPTR) array_desc->dsc_address, (MIN(sizeof(USHORT), FB_ALIGNMENT))))
		{
			// Note: cannot remove this JRD_get_thread_data without api change
			// to slice callback routines
			/*thread_db* tdbb = */ JRD_get_thread_data();

			DynamicVaryStr<1024> tmp_buffer;
			const USHORT tmp_len = array_desc->dsc_length;
			const char* p;
			const USHORT len = MOV_make_string(slice_desc, INTL_TEXT_TYPE(*array_desc), &p,
											   tmp_buffer.getBuffer(tmp_len), tmp_len);
			memcpy(array_desc->dsc_address, &len, sizeof(USHORT));
			memcpy(array_desc->dsc_address + sizeof(USHORT), p, (int) len);
		}
		else
		{
			MOV_move(tdbb, slice_desc, array_desc);
		}
		const BLOB_PTR* const end = array_desc->dsc_address + array_desc->dsc_length;
		if (end > arg->slice_high_water)
			arg->slice_high_water = end;
	}
	else
	{
		// Fetching FROM array
		// FROM array_desc TO slice_desc

		// If the element is under the high-water mark, fetch it,
		// otherwise just zero it

		if (array_desc->dsc_address < arg->slice_high_water)
		{
			// If a varying string isn't aligned correctly, calculate the actual
			// length and then treat the string as if it had type text.

			if (array_desc->dsc_dtype == dtype_varying &&
				(U_IPTR) array_desc->dsc_address !=
					FB_ALIGN((U_IPTR) array_desc->dsc_address, (MIN(sizeof(USHORT), FB_ALIGNMENT))))
			{
			    // temp_desc will vanish at the end of the block, but it's used
			    // only as a way to transfer blocks of memory.
				dsc temp_desc;
				temp_desc.dsc_dtype = dtype_text;
				temp_desc.dsc_sub_type = array_desc->dsc_sub_type;
				temp_desc.dsc_scale = array_desc->dsc_scale;
				temp_desc.dsc_flags = array_desc->dsc_flags;
				memcpy(&temp_desc.dsc_length, array_desc->dsc_address, sizeof(USHORT));
				temp_desc.dsc_address = array_desc->dsc_address + sizeof(USHORT);
				MOV_move(tdbb, &temp_desc, slice_desc);
			}
			else
				MOV_move(tdbb, array_desc, slice_desc);
			++arg->slice_count;
		}
		else
		{
		    const SLONG l = slice_desc->dsc_length;
			if (l)
				memset(slice_desc->dsc_address, 0, l);
		}
	}

	slice_desc->dsc_address = next;
}


static blb* store_array(thread_db* tdbb, jrd_tra* transaction, bid* blob_id)
{
/**************************************
 *
 *      s t o r e _ a r r a y
 *
 **************************************
 *
 * Functional description
 *      Actually store an array.  Oh boy!
 *
 **************************************/
	SET_TDBB(tdbb);

	// Validate array

	const ArrayField* array = find_array(transaction, blob_id);
	if (!array)
		return NULL;

	// Create blob for array

	blb* blob = BLB_create2(tdbb, transaction, blob_id, 0, NULL);
	blob->blb_flags |= BLB_stream;

	// Write out array descriptor

	BLB_put_segment(tdbb, blob, reinterpret_cast<const UCHAR*>(&array->arr_desc),
					array->arr_desc.iad_length);

	// Write out actual array
	const USHORT seg_limit = 32768;
	const BLOB_PTR* p = array->arr_data;
	SLONG length = array->arr_effective_length;
	while (length > seg_limit)
	{
		BLB_put_segment(tdbb, blob, p, seg_limit);
		length -= seg_limit;
		p += seg_limit;
	}

	if (length)
		BLB_put_segment(tdbb, blob, p, (USHORT) length);

	BLB_close(tdbb, blob);

	return blob;
}
