/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		inf.cpp
 *	DESCRIPTION:	Information handler
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 * 2001.08.09 Claudio Valderrama - Added new isc_info_* tokens to INF_database_info():
 *	oldest_transaction, oldest_active, oldest_snapshot and next_transaction.
 *      Make INF_put_item() to reserve 4 bytes: item + length as short + info_end;
 *	otherwise to signal output buffer truncation.
 *
 * 2001.11.28 Ann Harrison - the dbb has to be refreshed before reporting
 *      oldest_transaction, oldest_active, oldest_snapshot and next_transaction.
 *
 * 2001.11.29 Paul Reeves - Added refresh of dbb to ensure forced_writes
 *      reports correctly when called immediately after a create database
 *      operation.
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ibase.h"
#include "../jrd/tra.h"
#include "../jrd/blb.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/scl.h"
#include "../jrd/lck.h"
#include "../jrd/cch.h"
#include "../jrd/license.h"
#include "../jrd/cch_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/nbak.h"
#include "../common/StatusArg.h"

using namespace Jrd;


/*
 * The variable DBSERVER_BASE_LEVEL was originally IB_MAJOR_VER but with
 * the change to Firebird this number could no longer be used.
 * The DBSERVER_BASE_LEVEL for Firebird starts at 6 which is the base level
 * of InterBase(r) from which Firebird was derived.
 * It is expected that this value will increase as changes are added to
 * Firebird
 */

#define DBSERVER_BASE_LEVEL 6


#define STUFF_WORD(p, value)	{*p++ = value; *p++ = value >> 8;}
#define STUFF(p, value)		*p++ = value

typedef Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> CountsBuffer;

static USHORT get_counts(USHORT, CountsBuffer&);

#define CHECK_INPUT(fcn) \
	{ \
		if (!items || item_length <= 0 || !info || output_length <= 0) \
			ERR_post(Firebird::Arg::Gds(isc_internal_rejected_params) << Firebird::Arg::Str(fcn)); \
	}



void INF_blob_info(const blb* blob,
				   const UCHAR* items,
				   const SSHORT item_length,
				   UCHAR* info,
				   const SSHORT output_length)
{
/**************************************
 *
 *	I N F _ b l o b _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Process requests for blob info.
 *
 **************************************/
	CHECK_INPUT("INF_blob_info");

	UCHAR buffer[BUFFER_TINY];
	USHORT length;

	const UCHAR* const end_items = items + item_length;
	const UCHAR* const end = info + output_length;
	UCHAR* start_info;

	if (items[0] == isc_info_length)
	{
		start_info = info;
		items++;
	}
	else {
		start_info = 0;
	}

	while (items < end_items && *items != isc_info_end)
	{
		UCHAR item = *items++;

		switch (item)
		{
		case isc_info_end:
			break;

		case isc_info_blob_num_segments:
			length = INF_convert(blob->blb_count, buffer);
			break;

		case isc_info_blob_max_segment:
			length = INF_convert(static_cast<ULONG>(blob->blb_max_segment), buffer);
			break;

		case isc_info_blob_total_length:
			length = INF_convert(blob->blb_length, buffer);
			break;

		case isc_info_blob_type:
			buffer[0] = (blob->blb_flags & BLB_stream) ? 1 : 0;
			length = 1;
			break;

		default:
			buffer[0] = item;
			item = isc_info_error;
			length = 1 + INF_convert(isc_infunk, buffer + 1);
			break;
		}

		if (!(info = INF_put_item(item, length, buffer, info, end)))
			return;
	}

	*info++ = isc_info_end;

	if (start_info && (end - info >= 7))
	{
		const SLONG number = info - start_info;
		fb_assert(number > 0);
		memmove(start_info + 7, start_info, number);
		length = INF_convert(number, buffer);
		fb_assert(length == 4); // We only accept SLONG
		INF_put_item(isc_info_length, length, buffer, start_info, end, true);
	}
}


USHORT INF_convert(SINT64 number, UCHAR* buffer)
{
/**************************************
 *
 *	I N F _ c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Convert a number to VAX form -- least significant bytes first.
 *	Return the length.
 *
 **************************************/
	if (number >= MIN_SLONG && number <= MAX_SLONG)
	{
		put_vax_long(buffer, (SLONG) number);
		return sizeof(SLONG);
	}
	else
	{
		put_vax_int64(buffer, number);
		return sizeof(SINT64);
	}
}


void INF_database_info(const UCHAR* items,
					   const SSHORT item_length,
					   UCHAR* info,
					   const SSHORT output_length)
{
/**************************************
 *
 *	I N F _ d a t a b a s e _ i n f o	( J R D )
 *
 **************************************
 *
 * Functional description
 *	Process requests for database info.
 *
 **************************************/
 	CHECK_INPUT("INF_database_info");

	CountsBuffer counts_buffer;
	UCHAR* buffer = counts_buffer.getBuffer(BUFFER_SMALL);
	SSHORT length;
	SLONG err_val;
	bool header_refreshed = false;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	jrd_tra* transaction = NULL;
	const UCHAR* const end_items = items + item_length;
	const UCHAR* const end = info + output_length;

	const Attachment* err_att = tdbb->getAttachment();

	while (items < end_items && *items != isc_info_end)
	{
		UCHAR* p = buffer;
		UCHAR item = *items++;

		switch (item)
		{
		case isc_info_end:
			break;

		case isc_info_reads:
			length = INF_convert(dbb->dbb_stats.getValue(RuntimeStatistics::PAGE_READS), buffer);
			break;

		case isc_info_writes:
			length = INF_convert(dbb->dbb_stats.getValue(RuntimeStatistics::PAGE_WRITES), buffer);
			break;

		case isc_info_fetches:
			length = INF_convert(dbb->dbb_stats.getValue(RuntimeStatistics::PAGE_FETCHES), buffer);
			break;

		case isc_info_marks:
			length = INF_convert(dbb->dbb_stats.getValue(RuntimeStatistics::PAGE_MARKS), buffer);
			break;

		case isc_info_page_size:
			length = INF_convert(dbb->dbb_page_size, buffer);
			break;

		case isc_info_num_buffers:
			length = INF_convert(dbb->dbb_bcb->bcb_count, buffer);
			break;

		case isc_info_set_page_buffers:
			length = INF_convert(dbb->dbb_page_buffers, buffer);
			break;

		case isc_info_logfile:
			length = INF_convert(FALSE, buffer);
			break;

		case isc_info_cur_logfile_name:
			*p++ = 0;
			length = p - buffer;
			break;

		case isc_info_cur_log_part_offset:
			length = INF_convert(0, buffer);
			break;

		case isc_info_num_wal_buffers:
		case isc_info_wal_buffer_size:
		case isc_info_wal_ckpt_length:
		case isc_info_wal_cur_ckpt_interval:
		case isc_info_wal_recv_ckpt_fname:
		case isc_info_wal_recv_ckpt_poffset:
		case isc_info_wal_grpc_wait_usecs:
		case isc_info_wal_num_io:
		case isc_info_wal_avg_io_size:
		case isc_info_wal_num_commits:
		case isc_info_wal_avg_grpc_size:
			// WAL obsolete
			length = 0;
			break;

		case isc_info_wal_prv_ckpt_fname:
			*p++ = 0;
			length = p - buffer;
			break;

		case isc_info_wal_prv_ckpt_poffset:
			length = INF_convert(0, buffer);
			break;

		case isc_info_current_memory:
			length = INF_convert(dbb->dbb_memory_stats.getCurrentUsage(), buffer);
			break;

		case isc_info_max_memory:
			length = INF_convert(dbb->dbb_memory_stats.getMaximumUsage(), buffer);
			break;

		case isc_info_attachment_id:
			length = INF_convert(PAG_attachment_id(tdbb), buffer);
			break;

		case isc_info_ods_version:
			length = INF_convert(dbb->dbb_ods_version, buffer);
			break;

		case isc_info_ods_minor_version:
			length = INF_convert(dbb->dbb_minor_version, buffer);
			break;

		case isc_info_allocation:
			CCH_flush(tdbb, FLUSH_ALL, 0L);
			length = INF_convert(PageSpace::maxAlloc(dbb), buffer);
			break;

		case isc_info_sweep_interval:
			length = INF_convert(dbb->dbb_sweep_interval, buffer);
			break;

		case isc_info_read_seq_count:
			length = get_counts(DBB_read_seq_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_read_idx_count:
			length = get_counts(DBB_read_idx_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_update_count:
			length = get_counts(DBB_update_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_insert_count:
			length = get_counts(DBB_insert_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_delete_count:
			length = get_counts(DBB_delete_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_backout_count:
			length = get_counts(DBB_backout_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_purge_count:
			length = get_counts(DBB_purge_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_expunge_count:
			length = get_counts(DBB_expunge_count, counts_buffer);
			buffer = counts_buffer.begin();
			break;

		case isc_info_implementation:
			STUFF(p, 1);		/* Count */
			STUFF(p, IMPLEMENTATION);
			STUFF(p, 1);		/* Class */
			length = p - buffer;
			break;

		case isc_info_base_level:
			/* info_base_level is used by the client to represent
			 * what the server is capable of.  It is equivalent to the
			 * ods version of a database.  For example,
			 * ods_version represents what the database 'knows'
			 * base_level represents what the server 'knows'
			 */
			STUFF(p, 1);		/* Count */
#ifdef SCROLLABLE_CURSORS
			UPDATE WITH VERSION OF SERVER SUPPORTING
				SCROLLABLE CURSORS STUFF(p, 5);	/* base level of scrollable cursors */
#else
			/* IB_MAJOR_VER is defined as a character string */
			STUFF(p, DBSERVER_BASE_LEVEL);	/* base level of current version */
#endif
			length = p - buffer;
			break;

		case isc_info_isc_version:
			STUFF(p, 1);
			STUFF(p, sizeof(ISC_VERSION) - 1);
			for (const char* q = ISC_VERSION; *q;)
				STUFF(p, *q++);
			length = p - buffer;
			break;

		case isc_info_firebird_version:
		    STUFF(p, 1);
			STUFF(p, sizeof(FB_VERSION) - 1);
			for (const char* q = FB_VERSION; *q;)
				STUFF(p, *q++);
			length = p - buffer;
			break;

		case isc_info_db_id:
			{
				counts_buffer.resize(BUFFER_SMALL);
				const UCHAR* const end_buf = counts_buffer.end();
				// May be simpler to code using a server-side version of isql's Extender class.
				const Firebird::PathName& str_fn = dbb->dbb_database_name;
				STUFF(p, 2);
				USHORT len = str_fn.length();
				if (p + len + 1 >= end_buf)
					len = end_buf - p - 1;
				if (len > 255)
					len = 255; // Cannot put more in one byte, will truncate instead.
				*p++ = len;
				memcpy(p, str_fn.c_str(), len);
				p += len;
				if (p + 2 < end_buf)
				{
					SCHAR site[256];
					ISC_get_host(site, sizeof(site));
					len = strlen(site);
					if (p + len + 1 >= end_buf)
						len = end_buf - p - 1;
					*p++ = len;
					memcpy(p, site, len);
					p += len;
				}
				length = p - buffer;
			}
			break;

		case isc_info_creation_date:
			{
				const ISC_TIMESTAMP ts = dbb->dbb_creation_date.value();
				length = INF_convert(ts.timestamp_date, p);
				p += length;
				length += INF_convert(ts.timestamp_time, p);
			}
			break;

		case isc_info_no_reserve:
			*p++ = (dbb->dbb_flags & DBB_no_reserve) ? 1 : 0;
			length = p - buffer;
			break;

		case isc_info_forced_writes:
			if (!header_refreshed)
			{
				PAG_header(tdbb, true);
				header_refreshed = true;
			}
			*p++ = (dbb->dbb_flags & DBB_force_write) ? 1 : 0;
			length = p - buffer;
			break;

		case isc_info_limbo:
			if (!transaction)
				transaction = TRA_start(tdbb, 0, NULL);
			for (SLONG id = transaction->tra_oldest; id < transaction->tra_number; id++)
			{
				if (TRA_snapshot_state(tdbb, transaction, id) == tra_limbo &&
					TRA_wait(tdbb, transaction, id, jrd_tra::tra_wait) == tra_limbo)
				{
					length = INF_convert(id, buffer);
					if (!(info = INF_put_item(item, length, buffer, info, end)))
					{
						if (transaction)
							TRA_commit(tdbb, transaction, false);
						return;
					}
				}
			}
			continue;

		case isc_info_active_transactions:
			if (!transaction)
				transaction = TRA_start(tdbb, 0, NULL);
			for (SLONG id = transaction->tra_oldest_active; id < transaction->tra_number; id++)
			{
				if (TRA_snapshot_state(tdbb, transaction, id) == tra_active)
				{
					length = INF_convert(id, buffer);
					if (!(info = INF_put_item(item, length, buffer, info, end)))
					{
						if (transaction)
							TRA_commit(tdbb, transaction, false);
						return;
					}
				}
			}
			continue;

		case isc_info_active_tran_count:
			if (!transaction)
				transaction = TRA_start(tdbb, 0, NULL);
			{ // scope
				SLONG cnt = 0;
				for (SLONG id = transaction->tra_oldest_active; id < transaction->tra_number; id++)
				{
					if (TRA_snapshot_state(tdbb, transaction, id) == tra_active) {
						cnt++;
					}
				}
				length = INF_convert(cnt, buffer);
			}
			break;

		case isc_info_db_file_size:
			{
				BackupManager *bm = dbb->dbb_backup_manager;
				length = INF_convert(bm ? bm->getPageCount() : 0, buffer);
			}
			break;

		case isc_info_user_names:
			// Assumes user names will be smaller than sizeof(buffer) - 1.
			if (!(tdbb->getAttachment()->locksmith()))
			{
				const UserId* user = tdbb->getAttachment()->att_user;
				const char* uname = (user && user->usr_user_name.hasData()) ?
					user->usr_user_name.c_str() : "<Unknown>";
				const SSHORT len = strlen(uname);
				*p++ = len;
				memcpy(p, uname, len);
				if (!(info = INF_put_item(item, len + 1, buffer, info, end)))
				{
					if (transaction)
						TRA_commit(tdbb, transaction, false);
					return;
				}
				continue;
			}

			for (const Attachment* att = dbb->dbb_attachments; att; att = att->att_next)
			{
				if (att->att_flags & ATT_shutdown)
					continue;

                const UserId* user = att->att_user;
				if (user)
				{
					const char* user_name = user->usr_user_name.hasData() ?
						user->usr_user_name.c_str() : "(Firebird Worker Thread)";
					p = buffer;
					const SSHORT len = strlen(user_name);
					*p++ = len;
					memcpy(p, user_name, len);
					if (!(info = INF_put_item(item, len + 1, buffer, info, end)))
					{
						if (transaction)
							TRA_commit(tdbb, transaction, false);
						return;
					}
				}
			}
			continue;

		case isc_info_page_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_PAG_WRONG_TYPE] +
						  (*err_att->att_val_errors)[VAL_PAG_CHECKSUM_ERR] +
						  (*err_att->att_val_errors)[VAL_PAG_DOUBLE_ALLOC] +
						  (*err_att->att_val_errors)[VAL_PAG_IN_USE] +
						  (*err_att->att_val_errors)[VAL_PAG_ORPHAN];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_bpage_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_BLOB_INCONSISTENT] +
						  (*err_att->att_val_errors)[VAL_BLOB_CORRUPT] +
						  (*err_att->att_val_errors)[VAL_BLOB_TRUNCATED];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_record_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_REC_CHAIN_BROKEN] +
						  (*err_att->att_val_errors)[VAL_REC_DAMAGED] +
						  (*err_att->att_val_errors)[VAL_REC_BAD_TID] +
						  (*err_att->att_val_errors)[VAL_REC_FRAGMENT_CORRUPT] +
						  (*err_att->att_val_errors)[VAL_REC_WRONG_LENGTH] +
						  (*err_att->att_val_errors)[VAL_REL_CHAIN_ORPHANS];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_dpage_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_DATA_PAGE_CONFUSED] +
						  (*err_att->att_val_errors)[VAL_DATA_PAGE_LINE_ERR];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_ipage_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_INDEX_PAGE_CORRUPT] +
						  (*err_att->att_val_errors)[VAL_INDEX_ROOT_MISSING] +
						  (*err_att->att_val_errors)[VAL_INDEX_MISSING_ROWS] +
						  (*err_att->att_val_errors)[VAL_INDEX_ORPHAN_CHILD] +
						  (*err_att->att_val_errors)[VAL_INDEX_CYCLE];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_ppage_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_P_PAGE_LOST] +
						  (*err_att->att_val_errors)[VAL_P_PAGE_INCONSISTENT];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_tpage_errors:
			if (err_att->att_val_errors)
			{
				err_val = (*err_att->att_val_errors)[VAL_TIP_LOST] +
						  (*err_att->att_val_errors)[VAL_TIP_LOST_SEQUENCE] +
						  (*err_att->att_val_errors)[VAL_TIP_CONFUSED];
			}
			else
				err_val = 0;

			length = INF_convert(err_val, buffer);
			break;

		case isc_info_db_sql_dialect:
			/*
			   **
			   ** there are 3 types of databases:
			   **
			   **   1. a DB that is created before V6.0. This DB only speak SQL
			   **        dialect 1 and 2.
			   **
			   **   2. a non ODS 10 DB is backed up/restored in IB V6.0. Since
			   **        this DB contained some old SQL dialect, therefore it
			   **        speaks SQL dialect 1, 2, and 3
			   **
			   **   3. a DB that is created in V6.0. This DB speak SQL
			   **        dialect 1, 2 or 3 depending the DB was created
			   **        under which SQL dialect.
			   **
			 */
			if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_10_0)
			{
				if (dbb->dbb_flags & DBB_DB_SQL_dialect_3)
				{
					/*
					   ** DB created in IB V6.0 by client SQL dialect 3
					 */
					*p++ = SQL_DIALECT_V6;
				}
				else
				{
					/*
					   ** old DB was gbaked in IB V6.0
					 */
					*p++ = SQL_DIALECT_V5;
				}
			}
			else
				*p++ = SQL_DIALECT_V5;	/* pre ODS 10 DB */

			length = p - buffer;
			break;

		case isc_info_db_read_only:
			*p++ = (dbb->dbb_flags & DBB_read_only) ? 1 : 0;
			length = p - buffer;

			break;

		case isc_info_db_size_in_pages:
			CCH_flush(tdbb, FLUSH_ALL, 0L);
			length = INF_convert(PageSpace::actAlloc(dbb), buffer);
			break;

		case isc_info_oldest_transaction:
			if (!header_refreshed)
			{
				PAG_header(tdbb, true);
				header_refreshed = true;
			}
			length = INF_convert(dbb->dbb_oldest_transaction, buffer);
			break;

		case isc_info_oldest_active:
			if (!header_refreshed)
			{
				PAG_header(tdbb, true);
				header_refreshed = true;
			}
		    length = INF_convert(dbb->dbb_oldest_active, buffer);
		    break;

		case isc_info_oldest_snapshot:
			if (!header_refreshed)
			{
				PAG_header(tdbb, true);
				header_refreshed = true;
			}
			length = INF_convert(dbb->dbb_oldest_snapshot, buffer);
			break;

		case isc_info_next_transaction:
			if (!header_refreshed)
			{
				PAG_header(tdbb, true);
				header_refreshed = true;
			}
			length = INF_convert(dbb->dbb_next_transaction, buffer);
			break;

		case isc_info_db_provider:
		    length = INF_convert(isc_info_db_code_firebird, buffer);
			break;

		case isc_info_db_class:
		    length = INF_convert(FB_ARCHITECTURE, buffer);
			break;

		case frb_info_att_charset:
			length = INF_convert(tdbb->getAttachment()->att_charset, buffer);
			break;

		case fb_info_page_contents:
			if (tdbb->getAttachment()->locksmith())
			{
				length = gds__vax_integer(items, 2);
				items += 2;
				const SLONG page_num = gds__vax_integer(items, length);
				items += length;

				win window(PageNumber(DB_PAGE_SPACE, page_num));

				Ods::pag* page = CCH_FETCH_NO_CHECKSUM(tdbb, &window, LCK_WAIT, pag_undefined);
				info = INF_put_item(item, dbb->dbb_page_size, reinterpret_cast<UCHAR*>(page), info, end);
				CCH_RELEASE_TAIL(tdbb, &window);

				if (!info)
				{
					if (transaction)
						TRA_commit(tdbb, transaction, false);
					return;
				}
				continue;
			}

			buffer[0] = item;
			item = isc_info_error;
			length = 1 + INF_convert(isc_adm_task_denied, buffer + 1);
			break;

		default:
			buffer[0] = item;
			item = isc_info_error;
			length = 1 + INF_convert(isc_infunk, buffer + 1);
			break;
		}

		if (!(info = INF_put_item(item, length, buffer, info, end)))
		{
			if (transaction)
				TRA_commit(tdbb, transaction, false);
			return;
		}
	}

	if (transaction)
		TRA_commit(tdbb, transaction, false);

	*info++ = isc_info_end;
}


UCHAR* INF_put_item(UCHAR item,
					USHORT length,
					const UCHAR* string,
					UCHAR* ptr,
					const UCHAR* end, const bool inserting)
{
/**************************************
 *
 *	I N F _ p u t _ i t e m
 *
 **************************************
 *
 * Functional description
 *	Put information item in output buffer if there is room, and
 *	return an updated pointer.  If there isn't room for the item,
 *	indicate truncation and return NULL.
 *	If we are inserting, we don't need space for isc_info_end, since it was calculated already.
 *
 **************************************/

	if (ptr + length + (inserting ? 3 : 4) >= end)
	{
		*ptr = isc_info_truncated;
		return NULL;
	}

	*ptr++ = item;
	STUFF_WORD(ptr, length);

	if (length)
	{
		memmove(ptr, string, length);
		ptr += length;
	}

	return ptr;
}


void INF_request_info(const jrd_req* request,
					  const UCHAR* items,
					  const SSHORT item_length,
					  UCHAR* info,
					  const SLONG output_length)
{
/**************************************
 *
 *	I N F _ r e q u e s t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Return information about requests.
 *
 **************************************/
	CHECK_INPUT("INF_request_info");

	ULONG length = 0;

	const UCHAR* const end_items = items + item_length;
	const UCHAR* const end = info + output_length;
	UCHAR* start_info;

	if (items[0] == isc_info_length)
	{
		start_info = info;
		items++;
	}
	else {
		start_info = 0;
	}

	Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer;
	UCHAR* buffer_ptr = buffer.getBuffer(BUFFER_TINY);

	while (items < end_items && *items != isc_info_end)
	{
		UCHAR item = *items++;

		switch (item)
		{
		case isc_info_end:
			break;

		case isc_info_number_messages:
			//length = INF_convert(request->req_nmsgs, buffer_ptr);
			length = INF_convert(0, buffer_ptr); // never used
			break;

		case isc_info_max_message:
			//length = INF_convert(request->req_mmsg, buffer_ptr);
			length = INF_convert(0, buffer_ptr); // never used
			break;

		case isc_info_max_send:
			//length = INF_convert(request->req_msend, buffer_ptr);
			length = INF_convert(0, buffer_ptr); // never used
			break;

		case isc_info_max_receive:
			//length = INF_convert(request->req_mreceive, buffer_ptr);
			length = INF_convert(0, buffer_ptr); // never used
			break;

		case isc_info_req_select_count:
			length = INF_convert(request->req_records_selected, buffer_ptr);
			break;

		case isc_info_req_insert_count:
			length = INF_convert(request->req_records_inserted, buffer_ptr);
			break;

		case isc_info_req_update_count:
			length = INF_convert(request->req_records_updated, buffer_ptr);
			break;

		case isc_info_req_delete_count:
			length = INF_convert(request->req_records_deleted, buffer_ptr);
			break;

		case isc_info_access_path:
			buffer_ptr = buffer.getBuffer(output_length);
			if (!OPT_access_path(request, buffer_ptr, buffer.getCount(), &length))
			{
				*info = isc_info_truncated;
				return;
			}
			if (length > MAX_USHORT) // damn INF_put_item, it only handles USHORT lengths
			{
				*info = isc_info_truncated;
				return;
			}
			break;

		case isc_info_state:
			if (!(request->req_flags & req_active))
				length = INF_convert(isc_info_req_inactive, buffer_ptr);
			else
			{
				SSHORT state = isc_info_req_active;
				if (request->req_operation == jrd_req::req_send)
					state = isc_info_req_send;
				else if (request->req_operation == jrd_req::req_receive)
				{
					const jrd_nod* node = request->req_next;
					if (node->nod_type == nod_select)
						state = isc_info_req_select;
					else
						state = isc_info_req_receive;
				}
				else if ((request->req_operation == jrd_req::req_return) &&
					(request->req_flags & req_stall))
				{
					state = isc_info_req_sql_stall;
				}
				length = INF_convert(state, buffer_ptr);
			}
			break;

		case isc_info_message_number:
		case isc_info_message_size:
			if (!(request->req_flags & req_active) ||
				(request->req_operation != jrd_req::req_receive &&
					request->req_operation != jrd_req::req_send))
			{
				buffer_ptr[0] = item;
				item = isc_info_error;
				length = 1 + INF_convert(isc_infinap, buffer_ptr + 1);
			}
			else
			{
				const jrd_nod* node = request->req_message;
				if (item == isc_info_message_number)
					length = INF_convert((IPTR) node->nod_arg[e_msg_number], buffer_ptr);
				else
				{
					const Format* format = (Format*) node->nod_arg[e_msg_format];
					length = INF_convert(format->fmt_length, buffer_ptr);
				}
			}
			break;

		case isc_info_request_cost:
		default:
			buffer_ptr[0] = item;
			item = isc_info_error;
			length = 1 + INF_convert(isc_infunk, buffer_ptr + 1);
			break;
		}

		info = INF_put_item(item, length, buffer_ptr, info, end);

		if (!info)
			return;
	}

	*info++ = isc_info_end;

	if (start_info && (end - info >= 7))
	{
		const SLONG number = info - start_info;
		fb_assert(number > 0);
		memmove(start_info + 7, start_info, number);
		length = INF_convert(number, buffer.begin());
		fb_assert(length == 4); // We only accept SLONG
		INF_put_item(isc_info_length, length, buffer.begin(), start_info, end, true);
	}
}


void INF_transaction_info(const jrd_tra* transaction,
						  const UCHAR* items,
						  const SSHORT item_length,
						  UCHAR* info,
						  const SSHORT output_length)
{
/**************************************
 *
 *	I N F _ t r a n s a c t i o n _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Process requests for transaction info.
 *
 **************************************/
	CHECK_INPUT("INF_transaction_info");

	UCHAR buffer[BUFFER_TINY];
	USHORT length;

	const UCHAR* const end_items = items + item_length;
	const UCHAR* const end = info + output_length;
	UCHAR* start_info;

	if (items[0] == isc_info_length)
	{
		start_info = info;
		items++;
	}
	else {
		start_info = 0;
	}

	while (items < end_items && *items != isc_info_end)
	{
		UCHAR item = *items++;

		switch (item)
		{
		case isc_info_end:
			break;

		case isc_info_tra_id:
			length = INF_convert(transaction->tra_number, buffer);
			break;

		case isc_info_tra_oldest_interesting:
			length = INF_convert(transaction->tra_oldest, buffer);
			break;

		case isc_info_tra_oldest_snapshot:
			length = INF_convert(transaction->tra_oldest_active, buffer);
			break;

		case isc_info_tra_oldest_active:
			length = INF_convert(
				transaction->tra_lock ? transaction->tra_lock->lck_data : 0,
				buffer);
			break;

		case isc_info_tra_isolation:
		{
			UCHAR* p = buffer;
			if (transaction->tra_flags & TRA_read_committed)
			{
				*p++ = isc_info_tra_read_committed;
				if (transaction->tra_flags & TRA_rec_version)
					*p++ = isc_info_tra_rec_version;
				else
					*p++ = isc_info_tra_no_rec_version;
			}
			else if (transaction->tra_flags & TRA_degree3)
				*p++ = isc_info_tra_consistency;
			else
				*p++ = isc_info_tra_concurrency;

			length = p - buffer;
			break;
		}

		case isc_info_tra_access:
		{
			UCHAR* p = buffer;
			if (transaction->tra_flags & TRA_readonly)
				*p++ = isc_info_tra_readonly;
			else
				*p++ = isc_info_tra_readwrite;

			length = p - buffer;
			break;
		}

		case isc_info_tra_lock_timeout:
			length = INF_convert(transaction->tra_lock_timeout, buffer);
			break;

		default:
			buffer[0] = item;
			item = isc_info_error;
			length = 1 + INF_convert(isc_infunk, buffer + 1);
			break;
		}

		if (!(info = INF_put_item(item, length, buffer, info, end)))
			return;
	}

	*info++ = isc_info_end;

	if (start_info && (end - info >= 7))
	{
		const SLONG number = info - start_info;
		fb_assert(number > 0);
		memmove(start_info + 7, start_info, number);
		length = INF_convert(number, buffer);
		fb_assert(length == 4); // We only accept SLONG
		INF_put_item(isc_info_length, length, buffer, start_info, end, true);
	}
}


static USHORT get_counts(USHORT count_id, CountsBuffer& buffer)
{
/**************************************
 *
 *	g e t _ c o u n t s
 *
 **************************************
 *
 * Functional description
 *	Return operation counts for relation.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	const vcl* vector = tdbb->getAttachment()->att_counts[count_id];
	if (!vector)
		return 0;

	UCHAR num_buffer[BUFFER_TINY];

	buffer.clear();
	size_t buffer_length = 0;

	vcl::const_iterator ptr = vector->begin();
	for (USHORT relation_id = 0; relation_id < vector->count(); ++relation_id)
	{
		const SLONG n = *ptr++;
		if (n)
		{
			const USHORT length = INF_convert(n, num_buffer);
			const size_t new_buffer_length = buffer_length + length + sizeof(USHORT);
			buffer.grow(new_buffer_length);
			UCHAR* p = buffer.begin() + buffer_length;
			STUFF_WORD(p, relation_id);
			memcpy(p, num_buffer, length);
			p += length;
			buffer_length = new_buffer_length;
		}
	}

	return buffer.getCount();
}
