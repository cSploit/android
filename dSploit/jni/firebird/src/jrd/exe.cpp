/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe.cpp
 *	DESCRIPTION:	Statement execution
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
 * 2001.6.21 Claudio Valderrama: Allow inserting strings into blob fields.
 * 2001.6.28 Claudio Valderrama: Move code to cleanup_rpb() as directed
 * by Ann Harrison and cleanup of new record in store() routine.
 * 2001.10.11 Claudio Valderrama: Fix SF Bug #436462: From now, we only
 * count real store, modify and delete operations either in an external
 * file or in a table. Counting on a view caused up to three operations
 * being reported instead of one.
 * 2001.12.03 Claudio Valderrama: new visit to the same issue: views need
 * to count virtual operations, not real I/O on the underlying tables.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 *
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"
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

#include "../jrd/common.h"
#include "../jrd/ibsetjmp.h"
#include "../common/classes/VaryStr.h"
#include <string.h>
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/extds/ExtDS.h"
#include "../jrd/tra.h"
#include "gen/iberror.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/rse.h"
#include "../jrd/lck.h"
#include "../jrd/intl.h"
#include "../jrd/sbm.h"
#include "../jrd/blb.h"
#include "../jrd/blr.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/rlck_proto.h"

#include "../jrd/rse_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/isc_s_proto.h"

#include "../jrd/execute_statement.h"
#include "../dsql/dsql_proto.h"
#include "../jrd/rpb_chain.h"
#include "../jrd/VirtualTable.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceJrdHelpers.h"

#include "../dsql/Nodes.h"


using namespace Jrd;
using namespace Firebird;

// AffectedRows class implementation

AffectedRows::AffectedRows()
{
	clear();
}

void AffectedRows::clear()
{
	writeFlag = false;
	fetchedRows = modifiedRows = 0;
}

void AffectedRows::bumpFetched()
{
	fetchedRows++;
}

void AffectedRows::bumpModified(bool increment)
{
	if (increment) {
		modifiedRows++;
	}
	else {
		writeFlag = true;
	}
}

int AffectedRows::getCount() const
{
	return writeFlag ? modifiedRows : fetchedRows;
}

// StatusXcp class implementation

StatusXcp::StatusXcp()
{
	clear();
}

void StatusXcp::clear()
{
	fb_utils::init_status(status);
}

void StatusXcp::init(const ISC_STATUS* vector)
{
	memcpy(status, vector, sizeof(ISC_STATUS_ARRAY));
}

void StatusXcp::copyTo(ISC_STATUS* vector) const
{
	memcpy(vector, status, sizeof(ISC_STATUS_ARRAY));
}

bool StatusXcp::success() const
{
	return (status[1] == FB_SUCCESS);
}

SLONG StatusXcp::as_gdscode() const
{
	return status[1];
}

SLONG StatusXcp::as_sqlcode() const
{
	return gds__sqlcode(status);
}

void StatusXcp::as_sqlstate(char* sqlstate) const
{
	fb_sqlstate(sqlstate, status);
}

static void cleanup_rpb(thread_db*, record_param*);
static jrd_nod* erase(thread_db*, jrd_nod*, SSHORT);
static void execute_looper(thread_db*, jrd_req*, jrd_tra*, jrd_req::req_s);
static void exec_sql(thread_db*, jrd_req*, DSC *);
static void execute_procedure(thread_db*, jrd_nod*);
static jrd_nod* execute_statement(thread_db*, jrd_req*, jrd_nod*);
static jrd_req* execute_triggers(thread_db*, trig_vec**, record_param*, record_param*,
	jrd_req::req_ta, SSHORT);
static void get_string(thread_db*, jrd_req*, jrd_nod*, Firebird::string&, bool = false);
static void looper_seh(thread_db*, jrd_req*);
static jrd_nod* modify(thread_db*, jrd_nod*, SSHORT);
static jrd_nod* receive_msg(thread_db*, jrd_nod*);
static void release_blobs(thread_db*, jrd_req*);
static void release_proc_save_points(jrd_req*);
#ifdef SCROLLABLE_CURSORS
static jrd_nod* seek_rse(thread_db*, jrd_req*, jrd_nod*);
static void seek_rsb(thread_db*, jrd_req*, RecordSource*, USHORT, SLONG);
#endif
static jrd_nod* selct(thread_db*, jrd_nod*);
static jrd_nod* send_msg(thread_db*, jrd_nod*);
static void set_error(thread_db*, const xcp_repeat*, jrd_nod*);
static jrd_nod* stall(thread_db*, jrd_nod*);
static jrd_nod* store(thread_db*, jrd_nod*, SSHORT);
static bool test_and_fixup_error(thread_db*, const PsqlException*, jrd_req*);
static void trigger_failure(thread_db*, jrd_req*);
static void validate(thread_db*, jrd_nod*);
inline void verb_cleanup(thread_db*, jrd_tra*);
inline void PreModifyEraseTriggers(thread_db*, trig_vec**, SSHORT, record_param*,
	record_param*, jrd_req::req_ta);
static void stuff_stack_trace(const jrd_req*);


/* macro definitions */

#if (defined SUPERSERVER) && (defined WIN_NT)
const int MAX_CLONES	= 750;
#else
const int MAX_CLONES	= 1000;
#endif

const int ALL_TRIGS	= 0;
const int PRE_TRIG	= 1;
const int POST_TRIG	= 2;

const size_t MAX_STACK_TRACE = 2048;

#ifdef SCROLLABLE_CURSORS
static const rse_get_mode g_RSE_get_mode = RSE_get_next;
#else
static const rse_get_mode g_RSE_get_mode = RSE_get_forward;
#endif


void EXE_assignment(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	E X E _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Perform an assignment
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	BLKCHK(node, type_nod);

	// Get descriptors of src field/parameter/variable, etc.
	request->req_flags &= ~req_null;
	dsc* from_desc = EVL_expr(tdbb, node->nod_arg[e_asgn_from]);

	EXE_assignment(tdbb, node->nod_arg[e_asgn_to], from_desc, (request->req_flags & req_null),
		node->nod_arg[e_asgn_missing], node->nod_arg[e_asgn_missing2]);

	request->req_operation = jrd_req::req_return;
}


void EXE_assignment(thread_db* tdbb, jrd_nod* to, dsc* from_desc, bool from_null,
	jrd_nod* missing_node, jrd_nod* missing2_node)
{
/**************************************
 *
 *	E X E _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Perform an assignment
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();

	// Get descriptors of receiving and sending fields/parameters, variables, etc.

	dsc* missing = NULL;
	if (missing_node) {
		missing = EVL_expr(tdbb, missing_node);
	}

	// Get descriptor of target field/parameter/variable, etc.
	DSC* to_desc = EVL_assign_to(tdbb, to);

	request->req_flags &= ~req_null;

	// NS: If we are assigning to NULL, we finished.
	// This functionality is currently used to allow calling UDF routines
	// without assigning resulting value anywhere.
	if (!to_desc)
		return;

	SSHORT null = from_null ? -1 : 0;

	if (!null && missing && MOV_compare(missing, from_desc) == 0) {
		null = -1;
	}

	USHORT* impure_flags = NULL;

	switch (to->nod_type)
	{
		case nod_variable:
			if (to->nod_arg[e_var_info])
			{
				EVL_validate(tdbb,
					Item(nod_variable, (IPTR) to->nod_arg[e_var_id]),
					reinterpret_cast<const ItemInfo*>(to->nod_arg[e_var_info]),
					from_desc, null == -1);
			}
			impure_flags = &((impure_value*) ((SCHAR *) request +
				to->nod_arg[e_var_variable]->nod_impure))->vlu_flags;
			break;

		case nod_argument:
			if (to->nod_arg[e_arg_info])
			{
				EVL_validate(tdbb,
					Item(nod_argument, (IPTR) to->nod_arg[e_arg_message]->nod_arg[e_msg_number],
						(IPTR) to->nod_arg[e_arg_number]),
					reinterpret_cast<const ItemInfo*>(to->nod_arg[e_arg_info]),
					from_desc, null == -1);
			}
			impure_flags = (USHORT*) ((UCHAR *) request +
				(IPTR) to->nod_arg[e_arg_message]->nod_arg[e_msg_impure_flags] +
				(sizeof(USHORT) * (IPTR) to->nod_arg[e_arg_number]));
			break;
	}

	if (impure_flags != NULL)
		*impure_flags |= VLU_checked;

	// If the value is non-missing, move/convert it.  Otherwise fill the
	// field with appropriate nulls.
	dsc temp;

	if (!null)
	{
		// if necessary and appropriate, use the indicator variable

		if (to->nod_type == nod_argument && to->nod_arg[e_arg_indicator])
		{
			dsc* indicator    = EVL_assign_to(tdbb, to->nod_arg[e_arg_indicator]);
			temp.dsc_dtype    = dtype_short;
			temp.dsc_length   = sizeof(SSHORT);
			temp.dsc_scale    = 0;
			temp.dsc_sub_type = 0;

			SSHORT len;

			if ((from_desc->dsc_dtype <= dtype_varying) && (to_desc->dsc_dtype <= dtype_varying) &&
				(TEXT_LEN(from_desc) > TEXT_LEN(to_desc)))
			{
				len = TEXT_LEN(from_desc);
			}
			else {
				len = 0;
			}

			temp.dsc_address = (UCHAR *) &len;
			MOV_move(tdbb, &temp, indicator);

			if (len) {
				temp = *from_desc;
				temp.dsc_length = TEXT_LEN(to_desc);
				if (temp.dsc_dtype == dtype_cstring) {
					temp.dsc_length += 1;
				}
				else if (temp.dsc_dtype == dtype_varying) {
					temp.dsc_length += 2;
				}
				from_desc = &temp;
			}
		}

		// Validate range for datetime values

		if (DTYPE_IS_DATE(from_desc->dsc_dtype))
		{
			switch (from_desc->dsc_dtype)
			{
				case dtype_sql_date:
					if (!Firebird::TimeStamp::isValidDate(*(GDS_DATE*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_date_range_exceeded));
					}
					break;

				case dtype_sql_time:
					if (!Firebird::TimeStamp::isValidTime(*(GDS_TIME*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_time_range_exceeded));
					}
					break;

				case dtype_timestamp:
					if (!Firebird::TimeStamp::isValidTimeStamp(*(GDS_TIMESTAMP*) from_desc->dsc_address))
					{
						ERR_post(Arg::Gds(isc_datetime_range_exceeded));
					}
					break;

				default:
					fb_assert(false);
			}
		}

		if (DTYPE_IS_BLOB_OR_QUAD(from_desc->dsc_dtype) || DTYPE_IS_BLOB_OR_QUAD(to_desc->dsc_dtype))
		{
			// ASF: Don't let MOV_move call BLB_move because MOV
			// will not pass the destination field to BLB_move.
			BLB_move(tdbb, from_desc, to_desc, to);
		}
		else if (!DSC_EQUIV(from_desc, to_desc, false))
		{
			MOV_move(tdbb, from_desc, to_desc);
		}
		else if (from_desc->dsc_dtype == dtype_short)
		{
			*((SSHORT*) to_desc->dsc_address) = *((SSHORT*) from_desc->dsc_address);
		}
		else if (from_desc->dsc_dtype == dtype_long)
		{
			*((SLONG*) to_desc->dsc_address) = *((SLONG*) from_desc->dsc_address);
		}
		else if (from_desc->dsc_dtype == dtype_int64)
		{
			*((SINT64*) to_desc->dsc_address) = *((SINT64*) from_desc->dsc_address);
		}
		else
		{
			memcpy(to_desc->dsc_address, from_desc->dsc_address, from_desc->dsc_length);
		}

		to_desc->dsc_flags &= ~DSC_null;
	}
	else
	{
		if (missing2_node && (missing = EVL_expr(tdbb, missing2_node)))
		{
			MOV_move(tdbb, missing, to_desc);
		}
		else
		{
			memset(to_desc->dsc_address, 0, to_desc->dsc_length);
		}

		to_desc->dsc_flags |= DSC_null;
	}

	// Handle the null flag as appropriate for fields and message arguments.

	if (to->nod_type == nod_field)
	{
		const SSHORT id = (USHORT)(IPTR) to->nod_arg[e_fld_id];
		Record* record = request->req_rpb[(int) (IPTR) to->nod_arg[e_fld_stream]].rpb_record;
		if (null) {
			SET_NULL(record, id);
		}
		else {
			CLEAR_NULL(record, id);
		}
	}
	else if (to->nod_type == nod_argument && to->nod_arg[e_arg_flag])
	{
		to_desc = EVL_assign_to(tdbb, to->nod_arg[e_arg_flag]);

		// If the null flag is a string with an effective length of one,
		// then -1 will not fit.  Therefore, store 1 instead.

		if (null && to_desc->dsc_dtype <= dtype_varying)
		{
			USHORT minlen;
			switch (to_desc->dsc_dtype)
			{
			case dtype_text:
				minlen = 1;
				break;
			case dtype_cstring:
				minlen = 2;
				break;
			case dtype_varying:
				minlen = 3;
				break;
			}
			if (to_desc->dsc_length <= minlen)
			{
				null = 1;
			}
		}

		temp.dsc_dtype = dtype_short;
		temp.dsc_length = sizeof(SSHORT);
		temp.dsc_scale = 0;
		temp.dsc_sub_type = 0;
		temp.dsc_address = (UCHAR*) &null;
		MOV_move(tdbb, &temp, to_desc);
		if (null && to->nod_arg[e_arg_indicator]) {
			to_desc = EVL_assign_to(tdbb, to->nod_arg[e_arg_indicator]);
			MOV_move(tdbb, &temp, to_desc);
		}
	}
}


void EXE_execute_db_triggers(thread_db* tdbb, jrd_tra* transaction, jrd_req::req_ta trigger_action)
{
/**************************************
 *
 *	E X E _ e x e c u t e _ d b _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Execute database triggers
 *
 **************************************/
 	// do nothing if user doesn't want database triggers
	if (tdbb->getAttachment()->att_flags & ATT_no_db_triggers)
		return;

	int type = 0;

	switch (trigger_action)
	{
		case jrd_req::req_trigger_connect:
			type = DB_TRIGGER_CONNECT;
			break;

		case jrd_req::req_trigger_disconnect:
			type = DB_TRIGGER_DISCONNECT;
			break;

		case jrd_req::req_trigger_trans_start:
			type = DB_TRIGGER_TRANS_START;
			break;

		case jrd_req::req_trigger_trans_commit:
			type = DB_TRIGGER_TRANS_COMMIT;
			break;

		case jrd_req::req_trigger_trans_rollback:
			type = DB_TRIGGER_TRANS_ROLLBACK;
			break;

		default:
			fb_assert(false);
			return;
	}

	jrd_req* trigger = NULL;

	if (tdbb->getDatabase()->dbb_triggers[type])
	{
		jrd_tra* old_transaction = tdbb->getTransaction();
		tdbb->setTransaction(transaction);

		try
		{
			trigger = execute_triggers(tdbb, &tdbb->getDatabase()->dbb_triggers[type],
				NULL, NULL, trigger_action, ALL_TRIGS);
			tdbb->setTransaction(old_transaction);
		}
		catch (...)
		{
			tdbb->setTransaction(old_transaction);
			throw;
		}
	}

	if (trigger)
		trigger_failure(tdbb, trigger);
}


jrd_req* EXE_find_request(thread_db* tdbb, jrd_req* request, bool validate)
{
/**************************************
 *
 *	E X E _ f i n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Find an inactive incarnation of a trigger request.  If necessary,
 *	clone it.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	Attachment* const attachment = tdbb->getAttachment();

/* I found a core file from my test runs that came from a NULL request -
 * but have no idea what test was running.  Let's bugcheck so we can
 * figure it out
 */
	if (!request)
		BUGCHECK /* REQUEST */ (167);	/* msg 167 invalid SEND request */

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_exe_clone_mutex);

	jrd_req* clone = NULL;
	USHORT count = 0;
	if (!(request->req_flags & req_in_use))
		clone = request;
	else
	{
		if (request->req_attachment == attachment)
			count++;

		/* Request exists and is in use.  Search clones for one in use by
		   this attachment. If not found, return first inactive request. */

		vec<jrd_req*>* vector = request->req_sub_requests;
		const USHORT clones = vector ? (vector->count() - 1) : 0;

		USHORT n;
		for (n = 1; n <= clones; n++)
		{
			jrd_req* next = CMP_clone_request(tdbb, request, n, validate);
			if (next->req_attachment == attachment) {
				if (!(next->req_flags & req_in_use)) {
					clone = next;
					break;
				}

				count++;
			}
			else if (!(next->req_flags & req_in_use) && !clone)
				clone = next;
		}

		if (count > MAX_CLONES) {
			ERR_post(Arg::Gds(isc_req_max_clones_exceeded));
		}
		if (!clone)
			clone = CMP_clone_request(tdbb, request, n, validate);
	}

	clone->req_attachment = attachment;
	clone->req_stats.reset();
	clone->req_base_stats.reset();
	clone->req_flags |= req_in_use;

	return clone;
}


void EXE_receive(thread_db*		tdbb,
				 jrd_req*		request,
				 USHORT		msg,
				 USHORT		length,
				 UCHAR*		buffer,
				 bool		top_level)
{
/**************************************
 *
 *	E X E _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Move a message from JRD to the host program.  This corresponds to
 *	a JRD BLR/jrd_nod* send.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(request, type_req);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_tra* transaction = request->req_transaction;

	if (!(request->req_flags & req_active)) {
		ERR_post(Arg::Gds(isc_req_sync));
	}

	if (request->req_flags & req_proc_fetch)
	{
		/* request->req_proc_sav_point stores all the request savepoints.
		   When going to continue execution put request save point list
		   into transaction->tra_save_point so that it is used in looper.
		   When we come back to EXE_receive() restore
		   transaction->tra_save_point and merge all work done under
		   stored procedure savepoints into the current transaction
		   savepoint, which is the savepoint for fetch. */

		Savepoint* const save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;

		if (!transaction->tra_save_point) {
			VIO_start_save_point(tdbb, transaction);
		}
	}

	try
	{

	if (request->req_message->nod_type == nod_stall
#ifdef SCROLLABLE_CURSORS
		|| request->req_flags & req_fetch_required
#endif
		)
		execute_looper(tdbb, request, transaction, jrd_req::req_sync);

	if (!(request->req_flags & req_active) || request->req_operation != jrd_req::req_send)
	{
		ERR_post(Arg::Gds(isc_req_sync));
	}

	const jrd_nod* message = request->req_message;
	const Format* format = (Format*) message->nod_arg[e_msg_format];

	if (msg != (USHORT)(IPTR) message->nod_arg[e_msg_number])
		ERR_post(Arg::Gds(isc_req_sync));

	if (length != format->fmt_length) {
		ERR_post(Arg::Gds(isc_port_len) << Arg::Num(length) << Arg::Num(format->fmt_length));
	}

	memcpy(buffer, (SCHAR*) request + message->nod_impure, length);

	// ASF: temporary blobs returned to the client should not be released
	// with the request, but in the transaction end.
	if (top_level)
	{
		for (int i = 0; i < format->fmt_count; ++i)
		{
			const DSC* desc = &format->fmt_desc[i];

			if (desc->isBlob())
			{
				const bid* id = (bid*)
					((UCHAR*)request + message->nod_impure + (ULONG)(IPTR)desc->dsc_address);

				if (transaction->tra_blobs->locate(id->bid_temp_id()))
				{
					BlobIndex* current = &transaction->tra_blobs->current();

					if (current->bli_request &&
						current->bli_request->req_blobs.locate(id->bid_temp_id()))
					{
						current->bli_request->req_blobs.fastRemove();
						current->bli_request = NULL;
					}
				}
			}
		}
	}

	execute_looper(tdbb, request, transaction, jrd_req::req_proceed);

	}	//try
	catch (const Firebird::Exception&)
	{
		if (request->req_flags & req_proc_fetch)
		{
			Savepoint* const save_sav_point = transaction->tra_save_point;
			transaction->tra_save_point = request->req_proc_sav_point;
			request->req_proc_sav_point = save_sav_point;
			release_proc_save_points(request);
		}
		throw;
	}

	if (request->req_flags & req_proc_fetch) {
		Savepoint* const save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;
		VIO_merge_proc_sav_points(tdbb, transaction, &request->req_proc_sav_point);
	}
}


#ifdef SCROLLABLE_CURSORS
void EXE_seek(thread_db* tdbb, jrd_req* request, USHORT direction, ULONG offset)
{
/**************************************
 *
 *      E X E _ s e e k
 *
 **************************************
 *
 * Functional description
 *	Seek a given request in a particular direction
 *	for offset records.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);

/* loop through all RSEs in the request,
   and describe the rsb tree for that rsb;
   go backwards because items were popped
   off the stack backwards */

/* find the top-level rsb in the request and seek it */

	for (SLONG i = request->req_fors.getCount() - 1; i >= 0; i--)
	{
		RecordSource* rsb = request->req_fors[i];
		if (rsb) {
			seek_rsb(tdbb, request, rsb, direction, offset);
			break;
		}
	}
}
#endif


void EXE_send(thread_db*		tdbb,
			  jrd_req*		request,
			  USHORT	msg,
			  USHORT	length,
			  const UCHAR*	buffer)
{
/**************************************
 *
 *	E X E _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Send a message from the host program to the engine.
 *	This corresponds to a blr_receive or blr_select statement.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);

	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	if (!(request->req_flags & req_active))
		ERR_post(Arg::Gds(isc_req_sync));

	jrd_nod* message;
	jrd_nod* node;
#ifdef SCROLLABLE_CURSORS
/* look for an asynchronous send message--if such
   a message was defined, we allow the user to send
   us a message at any time during request execution */
	jrd_nod* save_next = NULL;
	jrd_nod* save_message = NULL;
	jrd_req::req_s save_operation = jrd_req::req_evaluate;

	if ((message = request->req_async_message) && (node = message->nod_arg[e_send_message]) &&
		(msg == (USHORT)(ULONG) node->nod_arg[e_msg_number]))
	{
		/* save the current state of the request so we can go
		   back to what was interrupted */

		save_operation = request->req_operation;
		save_message = request->req_message;
		save_next = request->req_next;

		request->req_operation = jrd_req::req_receive;
		request->req_message = node;
		request->req_next = message->nod_arg[e_send_statement];

		/* indicate that we are processing an asynchronous message */

		request->req_flags |= req_async_processing;
	}
	else {
#endif
		if (request->req_operation != jrd_req::req_receive)
			ERR_post(Arg::Gds(isc_req_sync));
		node = request->req_message;
#ifdef SCROLLABLE_CURSORS
	}
#endif

	jrd_tra* transaction = request->req_transaction;

	switch (node->nod_type)
	{
	case nod_message:
		message = node;
		break;
	case nod_select:
		{
			jrd_nod** ptr = node->nod_arg;
			for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
			{
				message = (*ptr)->nod_arg[e_send_message];
				if ((USHORT)(IPTR) message->nod_arg[e_msg_number] == msg) {
					request->req_next = *ptr;
					break;
				}
			}
		}
		break;
	default:
		BUGCHECK(167);			/* msg 167 invalid SEND request */
	}

	const Format* format = (Format*) message->nod_arg[e_msg_format];

	if (msg != (USHORT)(IPTR) message->nod_arg[e_msg_number])
		ERR_post(Arg::Gds(isc_req_sync));

	if (length != format->fmt_length) {
		ERR_post(Arg::Gds(isc_port_len) << Arg::Num(length) << Arg::Num(format->fmt_length));
	}

	memcpy((SCHAR*) request + message->nod_impure, buffer, length);

	for (USHORT i = 0; i < format->fmt_count; ++i)
	{
		const DSC* desc = &format->fmt_desc[i];

		// ASF: I'll not test for dtype_cstring because usage is only internal
		if (desc->dsc_dtype == dtype_text || desc->dsc_dtype == dtype_varying)
		{
			const UCHAR* p = (UCHAR*)request + message->nod_impure + (ULONG)(IPTR)desc->dsc_address;
			USHORT len;

			switch (desc->dsc_dtype)
			{
				case dtype_text:
					len = desc->dsc_length;
					break;

				case dtype_varying:
					len = reinterpret_cast<const vary*>(p)->vary_length;
					p += sizeof(USHORT);
					break;
			}

			CharSet* charSet = INTL_charset_lookup(tdbb, DSC_GET_CHARSET(desc));

			if (!charSet->wellFormed(len, p))
				ERR_post(Arg::Gds(isc_malformed_string));
		}
		else if (desc->isBlob())
		{
			if (desc->getCharSet() != CS_NONE && desc->getCharSet() != CS_BINARY)
			{
				const Jrd::bid* bid = (Jrd::bid*) ((UCHAR*)request +
					message->nod_impure + (ULONG)(IPTR)desc->dsc_address);

				if (!bid->isEmpty())
				{
					AutoBlb blob(tdbb, BLB_open(tdbb, transaction/*tdbb->getTransaction()*/, bid));
					BLB_check_well_formed(tdbb, desc, blob.getBlb());
				}
			}
		}
	}

	execute_looper(tdbb, request, transaction, jrd_req::req_proceed);

#ifdef SCROLLABLE_CURSORS
	if (save_next) {
		/* if the message was sent asynchronously, restore all the
		   previous values so that whatever we were trying to do when
		   the message came in is what we do next */

		request->req_operation = save_operation;
		request->req_message = save_message;
		request->req_next = save_next;
	}
#endif
}


void EXE_start(thread_db* tdbb, jrd_req* request, jrd_tra* transaction)
{
/**************************************
 *
 *	E X E _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start an execution running.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	BLKCHK(request, type_req);
	BLKCHK(transaction, type_tra);

	if (request->req_flags & req_active)
		ERR_post(Arg::Gds(isc_req_sync) << Arg::Gds(isc_reqinuse));

	if (transaction->tra_flags & TRA_prepared)
		ERR_post(Arg::Gds(isc_req_no_trans));

/* Post resources to transaction block.  In particular, the interest locks
   on relations/indices are copied to the transaction, which is very
   important for (short-lived) dynamically compiled requests.  This will
   provide transaction stability by preventing a relation from being
   dropped after it has been referenced from an active transaction. */

	TRA_post_resources(tdbb, transaction, request->req_resources);

	Lock* lock = transaction->tra_cancel_lock;
	if (lock && lock->lck_logical == LCK_none)
		LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);

	TRA_attach_request(transaction, request);
	request->req_flags &= REQ_FLAGS_INIT_MASK;
	request->req_flags |= req_active;
	request->req_flags &= ~req_reserved;
	request->req_operation = jrd_req::req_evaluate;

/* set up to count records affected by request */

	request->req_records_selected = 0;
	request->req_records_updated = 0;
	request->req_records_inserted = 0;
	request->req_records_deleted = 0;

	request->req_records_affected.clear();

/* CVC: set up to count virtual operations on SQL views. */

	request->req_view_flags = 0;
	request->req_top_view_store = NULL;
	request->req_top_view_modify = NULL;
	request->req_top_view_erase = NULL;

	// Store request start time for timestamp work
	request->req_timestamp.validate();

	// Set all invariants to not computed.
	jrd_nod **ptr, **end;
	for (ptr = request->req_invariants.begin(), end = request->req_invariants.end(); ptr < end; ++ptr)
	{
		impure_value* impure = (impure_value*) ((SCHAR *) request + (*ptr)->nod_impure);
		impure->vlu_flags = 0;
	}

	if (request->req_sql_text)
	{
		tdbb->bumpStats(RuntimeStatistics::STMT_EXECUTES);
	}

	// Start a save point if not in middle of one
	if (transaction && (transaction != dbb->dbb_sys_trans)) {
		VIO_start_save_point(tdbb, transaction);
	}

	request->req_src_line = 0;
	request->req_src_column = 0;

	looper_seh(tdbb, request); //, request->req_top_node);

	// If any requested modify/delete/insert ops have completed, forget them

	if (transaction && (transaction != dbb->dbb_sys_trans) && transaction->tra_save_point &&
	    !(transaction->tra_save_point->sav_flags & SAV_user) &&
	    !transaction->tra_save_point->sav_verb_count)
	{
		// Forget about any undo for this verb

		VIO_verb_cleanup(tdbb, transaction);
	}
}


void EXE_unwind(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	E X E _ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Unwind a request, maybe active, maybe not.  This is particularly
 *	simple since nothing really needs to be done.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);

	if (request->req_flags & req_active)
	{
		if (request->req_fors.getCount() || request->req_exec_sta.getCount() || request->req_ext_stmt)
		{
			Jrd::ContextPoolHolder context(tdbb, request->req_pool);
			jrd_req* old_request = tdbb->getRequest();
			jrd_tra* old_transaction = tdbb->getTransaction();
			try {
				tdbb->setRequest(request);
				tdbb->setTransaction(request->req_transaction);

				RecordSource** ptr = request->req_fors.begin();
				for (const RecordSource* const* const end = request->req_fors.end(); ptr < end; ptr++)
				{
					if (*ptr)
						RSE_close(tdbb, *ptr);
				}

				for (size_t i = 0; i < request->req_exec_sta.getCount(); ++i)
				{
					jrd_nod* node = request->req_exec_sta[i];
					ExecuteStatement* impure =(ExecuteStatement*) ((char*) request + node->nod_impure);
					impure->close(tdbb);
				}

				while (request->req_ext_stmt) {
					request->req_ext_stmt->close(tdbb);
				}
			}
			catch (const Firebird::Exception&)
			{
				tdbb->setRequest(old_request);
				tdbb->setTransaction(old_transaction);
				throw;
			}

			tdbb->setRequest(old_request);
			tdbb->setTransaction(old_transaction);
		}
		release_blobs(tdbb, request);
	}

	if (request->req_proc_sav_point && (request->req_flags & req_proc_fetch))
		release_proc_save_points(request);

	TRA_detach_request(request);

	request->req_flags &= ~(req_active | req_proc_fetch | req_reserved);
	request->req_flags |= req_abort | req_stall;
	request->req_timestamp.invalidate();
	request->req_proc_inputs = NULL;
	request->req_proc_caller = NULL;
}


/* CVC: Moved to its own routine, originally in store(). */
static void cleanup_rpb(thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	c l e a n u p _ r p b
 *
 **************************************
 *
 * Functional description
 *	Perform cleaning of rpb, zeroing unassigned fields and
 * the impure tail of varying fields that we don't want to carry
 * when the RLE algorithm is applied.
 *
 **************************************/
	Record* record = rpb->rpb_record;
	const Format* format = record->rec_format;

	SET_TDBB(tdbb); /* Is it necessary? */

/*
    Starting from the format, walk through its
    array of descriptors.  If the descriptor has
    no address, its a computed field and we shouldn't
    try to fix it.  Get a pointer to the actual data
    and see if that field is null by indexing into
    the null flags between the record header and the
    record data.
*/

	for (USHORT n = 0; n < format->fmt_count; n++)
	{
		const dsc* desc = &format->fmt_desc[n];
		if (!desc->dsc_address)
			continue;
		UCHAR* const p = record->rec_data + (IPTR) desc->dsc_address;
		if (TEST_NULL(record, n))
		{
			USHORT length = desc->dsc_length;
			if (length) {
				memset(p, 0, length);
			}
		}
		else if (desc->dsc_dtype == dtype_varying)
		{
			vary* varying = reinterpret_cast<vary*>(p);
			USHORT length = desc->dsc_length - sizeof(USHORT);
			if (length > varying->vary_length)
			{
				char* trail = varying->vary_string + varying->vary_length;
				length -= varying->vary_length;
				memset(trail, 0, length);
			}
		}
	}
}

inline void PreModifyEraseTriggers(thread_db* tdbb,
								   trig_vec** trigs,
								   SSHORT which_trig,
								   record_param* rpb,
								   record_param* rec,
								   jrd_req::req_ta op)
{
/******************************************************
 *
 *	P r e M o d i f y E r a s e T r i g g e r s
 *
 ******************************************************
 *
 * Functional description
 *	Perform operation's pre-triggers,
 *  storing active rpb in chain.
 *
 ******************************************************/
	if (! tdbb->getTransaction()->tra_rpblist) {
		tdbb->getTransaction()->tra_rpblist =
			FB_NEW(*tdbb->getTransaction()->tra_pool) traRpbList(*tdbb->getTransaction()->tra_pool);
	}
	const int rpblevel = tdbb->getTransaction()->tra_rpblist->PushRpb(rpb);
	jrd_req* trigger = NULL;
	if ((*trigs) && (which_trig != POST_TRIG)) {
		trigger = execute_triggers(tdbb, trigs, rpb, rec, op, PRE_TRIG);
	}
	tdbb->getTransaction()->tra_rpblist->PopRpb(rpb, rpblevel);
	if (trigger) {
		trigger_failure(tdbb, trigger);
	}
}

static jrd_nod* erase(thread_db* tdbb, jrd_nod* node, SSHORT which_trig)
{
/**************************************
 *
 *	e r a s e
 *
 **************************************
 *
 * Functional description
 *	Perform erase operation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request->req_transaction;
	record_param* rpb = &request->req_rpb[(int) (IPTR) node->nod_arg[e_erase_stream]];
	jrd_rel* relation = rpb->rpb_relation;

	if (rpb->rpb_number.isBof() || (!relation->rel_view_rse && !rpb->rpb_number.isValid()))
	{
		ERR_post(Arg::Gds(isc_no_cur_rec));
	}

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		{
			request->req_records_affected.bumpModified(false);
			if (!node->nod_arg[e_erase_statement])
				break;
			const Format* format = MET_current(tdbb, rpb->rpb_relation);
			Record* record = VIO_record(tdbb, rpb, format, tdbb->getDefaultPool());
			rpb->rpb_address = record->rec_data;
			rpb->rpb_length = format->fmt_length;
			rpb->rpb_format_number = format->fmt_version;
			return node->nod_arg[e_erase_statement];
		}

	case jrd_req::req_return:
		break;

	default:
		return node->nod_parent;
	}

	request->req_operation = jrd_req::req_return;
	RLCK_reserve_relation(tdbb, transaction, relation, true);

/* If the stream was sorted, the various fields in the rpb are
   probably junk.  Just to make sure that everything is cool,
   refetch and release the record. */

	if (rpb->rpb_stream_flags & RPB_s_refetch) {
		VIO_refetch_record(tdbb, rpb, transaction);
		rpb->rpb_stream_flags &= ~RPB_s_refetch;
	}

	if (transaction != dbb->dbb_sys_trans)
		++transaction->tra_save_point->sav_verb_count;

/* Handle pre-operation trigger */
	PreModifyEraseTriggers(tdbb, &relation->rel_pre_erase, which_trig, rpb, NULL,
						   jrd_req::req_trigger_delete);

	if (relation->rel_file) {
		EXT_erase(rpb, transaction);
	}
	else if (relation->isVirtual()) {
		VirtualTable::erase(tdbb, rpb);
	}
	else if (!relation->rel_view_rse) {
		VIO_erase(tdbb, rpb, transaction);
	}

/* Handle post operation trigger */
	jrd_req* trigger;
	if (relation->rel_post_erase && which_trig != PRE_TRIG &&
		(trigger = execute_triggers(tdbb, &relation->rel_post_erase,
									rpb, NULL, jrd_req::req_trigger_delete, POST_TRIG)))
	{
		trigger_failure(tdbb, trigger);
	}

/* call IDX_erase (which checks constraints) after all post erase triggers
   have fired. This is required for cascading referential integrity, which
   can be implemented as post_erase triggers */

	if (!relation->rel_file && !relation->rel_view_rse && !relation->isVirtual())
	{
		jrd_rel* bad_relation = 0;
		USHORT bad_index;

		const idx_e error_code = IDX_erase(tdbb, rpb, transaction, &bad_relation, &bad_index);

		if (error_code) {
			ERR_duplicate_error(error_code, bad_relation, bad_index);
		}
	}

	/* CVC: Increment the counter only if we called VIO/EXT_erase() and
			we were successful. */
	if (!(request->req_view_flags & req_first_erase_return)) {
		request->req_view_flags |= req_first_erase_return;
		if (relation->rel_view_rse) {
			request->req_top_view_erase = relation;
		}
	}
	if (relation == request->req_top_view_erase) {
		if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) {
			request->req_records_deleted++;
			request->req_records_affected.bumpModified(true);
		}
	}
	else if (relation->rel_file || !relation->rel_view_rse) {
		request->req_records_deleted++;
		request->req_records_affected.bumpModified(true);
	}

	if (transaction != dbb->dbb_sys_trans) {
		--transaction->tra_save_point->sav_verb_count;
	}

	rpb->rpb_number.setValid(false);

	return node->nod_parent;
}


static void execute_looper(thread_db* tdbb,
						   jrd_req* request,
						   jrd_tra* transaction, jrd_req::req_s next_state)
{
/**************************************
 *
 *	e x e c u t e _ l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Wrapper around looper. This will execute
 *	looper with the save point mechanism.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* Start a save point */

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
	{
		if (transaction && (transaction != dbb->dbb_sys_trans))
			VIO_start_save_point(tdbb, transaction);
	}

	request->req_flags &= ~req_stall;
	request->req_operation = next_state;

	EXE_looper(tdbb, request, request->req_next);

/* If any requested modify/delete/insert ops have completed, forget them */

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
	{
		if (transaction && (transaction != dbb->dbb_sys_trans) && transaction->tra_save_point &&
			!transaction->tra_save_point->sav_verb_count)
		{
			/* Forget about any undo for this verb */

			VIO_verb_cleanup(tdbb, transaction);
		}
	}
}


static void exec_sql(thread_db* tdbb, jrd_req* request, DSC* dsc)
{
/**************************************
 *
 *	e x e c _ s q l
 *
 **************************************
 *
 * Functional description
 *	Execute a string as SQL operator.
 *
 **************************************/
	SET_TDBB(tdbb);
	ExecuteStatement::execute(tdbb, request, dsc);
}


static void execute_procedure(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Execute a stored procedure.  Begin by
 *	assigning the input parameters.  End
 *	by assigning the output parameters.
 *
 **************************************/
	SET_TDBB(tdbb);
	BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();

	jrd_nod* temp = node->nod_arg[e_esp_inputs];
	if (temp) {
		jrd_nod** ptr;
		jrd_nod** end;
		for (ptr = temp->nod_arg, end = ptr + temp->nod_count; ptr < end; ptr++)
		{
			EXE_assignment(tdbb, *ptr);
		}
	}

	USHORT in_msg_length = 0;
	UCHAR* in_msg = NULL;
	jrd_nod* in_message = node->nod_arg[e_esp_in_msg];
	if (in_message) {
		const Format* format = (Format*) in_message->nod_arg[e_msg_format];
		in_msg_length = format->fmt_length;
		in_msg = (UCHAR*) request + in_message->nod_impure;
	}

	USHORT out_msg_length = 0;
	UCHAR* out_msg = NULL;
	jrd_nod* out_message = node->nod_arg[e_esp_out_msg];
	if (out_message) {
		const Format* format = (Format*) out_message->nod_arg[e_msg_format];
		out_msg_length = format->fmt_length;
		out_msg = (UCHAR*) request + out_message->nod_impure;
	}

	jrd_prc* procedure = (jrd_prc*) node->nod_arg[e_esp_procedure];
	jrd_req* proc_request = EXE_find_request(tdbb, procedure->prc_request, false);

	// trace procedure execution start
	TraceProcExecute trace(tdbb, proc_request, request, node->nod_arg[e_esp_inputs]);

	Firebird::Array<UCHAR> temp_buffer;

	if (!out_message) {
		const Format* format = (Format*) procedure->prc_output_msg->nod_arg[e_msg_format];
		out_msg_length = format->fmt_length;
		out_msg = temp_buffer.getBuffer(out_msg_length + FB_DOUBLE_ALIGN - 1);
		out_msg = (UCHAR*) FB_ALIGN((U_IPTR) out_msg, FB_DOUBLE_ALIGN);
	}


/* Catch errors so we can unwind cleanly */

	try {
		// Save the old pool
		Jrd::ContextPoolHolder context(tdbb, proc_request->req_pool);

		jrd_tra* transaction = request->req_transaction;
		const SLONG save_point_number = transaction->tra_save_point ?
			transaction->tra_save_point->sav_number : 0;

		proc_request->req_timestamp = request->req_timestamp;
		EXE_start(tdbb, proc_request, transaction);
		if (in_message) {
			EXE_send(tdbb, proc_request, 0, in_msg_length, in_msg);
		}

		EXE_receive(tdbb, proc_request, 1, out_msg_length, out_msg);

/* Clean up all savepoints started during execution of the
   procedure */

		if (transaction != tdbb->getDatabase()->dbb_sys_trans) {
			for (const Savepoint* save_point = transaction->tra_save_point;
				 save_point && save_point_number < save_point->sav_number;
				 save_point = transaction->tra_save_point)
			{
				VIO_verb_cleanup(tdbb, transaction);
			}
		}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		const bool no_priv = (ex.stuff_exception(tdbb->tdbb_status_vector) == isc_no_priv);
		trace.finish(false, no_priv ? res_unauthorized : res_failed);

		tdbb->setRequest(request);
		EXE_unwind(tdbb, proc_request);
		proc_request->req_attachment = NULL;
		proc_request->req_flags &= ~(req_in_use | req_proc_fetch);
		proc_request->req_timestamp.invalidate();
		throw;
	}

	// trace procedure execution finish
	trace.finish(false, res_successful);

	EXE_unwind(tdbb, proc_request);
	tdbb->setRequest(request);

	temp = node->nod_arg[e_esp_outputs];
	if (temp) {
		jrd_nod** ptr;
		jrd_nod** end;
		for (ptr = temp->nod_arg, end = ptr + temp->nod_count; ptr < end; ptr++)
		{
			EXE_assignment(tdbb, *ptr);
		}
	}

	proc_request->req_attachment = NULL;
	proc_request->req_flags &= ~(req_in_use | req_proc_fetch);
	proc_request->req_timestamp.invalidate();
}


static jrd_nod* execute_statement(thread_db* tdbb, jrd_req* request, jrd_nod* node)
{
	SET_TDBB(tdbb);
	BLKCHK(node, type_nod);

	EDS::Statement** stmt_ptr = (EDS::Statement**) ((char*) request + node->nod_impure);
	EDS::Statement* stmt = *stmt_ptr;

	const int inputs = (SSHORT)(IPTR) node->nod_arg[node->nod_count + e_exec_stmt_extra_inputs];
	const int outputs = (SSHORT)(IPTR) node->nod_arg[node->nod_count + e_exec_stmt_extra_outputs];

	jrd_nod** node_inputs = inputs ?
		node->nod_arg + e_exec_stmt_fixed_count + e_exec_stmt_extra_inputs : NULL;

	jrd_nod** node_outputs = outputs ?
		node->nod_arg + e_exec_stmt_fixed_count + inputs : NULL;

	jrd_nod* node_proc_block = node->nod_arg[e_exec_stmt_proc_block];

	if (request->req_operation == jrd_req::req_evaluate)
	{
		fb_assert(*stmt_ptr == 0);

		const EDS::ParamNames* inputs_names =
			(EDS::ParamNames*) node->nod_arg[node->nod_count + e_exec_stmt_extra_input_names];

		const jrd_nod* tra_node = node->nod_arg[node->nod_count + e_exec_stmt_extra_tran];
		const EDS::TraScope tra_scope = tra_node ? (EDS::TraScope)(IPTR) tra_node : EDS::traCommon;

		const jrd_nod* privs_node = node->nod_arg[node->nod_count + e_exec_stmt_extra_privs];
		const bool caller_privs = (privs_node != NULL);

		Firebird::string sSql;
		get_string(tdbb, request, node->nod_arg[e_exec_stmt_stmt_sql], sSql, true);

		Firebird::string sDataSrc;
		get_string(tdbb, request, node->nod_arg[e_exec_stmt_data_src], sDataSrc);

		Firebird::string sUser;
		get_string(tdbb, request, node->nod_arg[e_exec_stmt_user], sUser);

		Firebird::string sPwd;
		get_string(tdbb, request, node->nod_arg[e_exec_stmt_password], sPwd);

		Firebird::string sRole;
		get_string(tdbb, request, node->nod_arg[e_exec_stmt_role], sRole);

		EDS::Connection* conn = EDS::Manager::getConnection(tdbb, sDataSrc, sUser, sPwd, sRole, tra_scope);

		stmt = conn->createStatement(sSql);

		EDS::Transaction* tran = EDS::Transaction::getTransaction(tdbb, stmt->getConnection(), tra_scope);

		stmt->bindToRequest(request, stmt_ptr);
		stmt->setCallerPrivileges(caller_privs);

		const Firebird::string* const * inp_names = inputs_names ? inputs_names->begin() : NULL;
		stmt->prepare(tdbb, tran, sSql, inputs_names != NULL);
		if (stmt->isSelectable())
			stmt->open(tdbb, tran, inputs, inp_names, node_inputs, !node_proc_block);
		else
			stmt->execute(tdbb, tran, inputs, inp_names, node_inputs, outputs, node_outputs);

		request->req_operation = jrd_req::req_return;
	}  // jrd_req::req_evaluate

	if (request->req_operation == jrd_req::req_return || request->req_operation == jrd_req::req_sync)
	{
		fb_assert(stmt);
		if (stmt->isSelectable())
		{
			if (stmt->fetch(tdbb, outputs, node_outputs))
			{
				request->req_operation = jrd_req::req_evaluate;
				return node_proc_block;
			}
			request->req_operation = jrd_req::req_return;
		}
	}

	if (stmt) {
		stmt->close(tdbb);
	}

	return node->nod_parent;
}


static jrd_req* execute_triggers(thread_db* tdbb,
								trig_vec** triggers,
								record_param* old_rpb,
								record_param* new_rpb,
								jrd_req::req_ta trigger_action, SSHORT which_trig)
{
/**************************************
 *
 *	e x e c u t e _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Execute group of triggers.  Return pointer to failing trigger
 *	if any blow up.
 *
 **************************************/
	if (!*triggers) {
		return NULL;
	}

	SET_TDBB(tdbb);

	jrd_tra* transaction =
		(tdbb->getRequest() ? tdbb->getRequest()->req_transaction : tdbb->getTransaction());
	trig_vec* vector = *triggers;
	jrd_req* result = NULL;
	Record* const old_rec = old_rpb ? old_rpb->rpb_record : NULL;
	Record* const new_rec = new_rpb ? new_rpb->rpb_record : NULL;

	Record* null_rec = NULL;

	if (!old_rec && !new_rec)
	{
		// this is a database trigger
	}
	else if (!old_rec || !new_rec)
	{
		const Record* record = old_rec ? old_rec : new_rec;
		fb_assert(record && record->rec_format);
		// copy the record
		null_rec = FB_NEW_RPT(record->rec_pool, record->rec_length) Record(record->rec_pool);
		null_rec->rec_length = record->rec_length;
		null_rec->rec_format = record->rec_format;
		// zero the record buffer
		memset(null_rec->rec_data, 0, record->rec_length);
		// initialize all fields to missing
		const SSHORT n = (record->rec_format->fmt_count + 7) >> 3;
		memset(null_rec->rec_data, 0xFF, n);
	}

	jrd_req* trigger = NULL;
	const Firebird::TimeStamp timestamp(Firebird::TimeStamp::getCurrentTimeStamp());

	try
	{
		for (trig_vec::iterator ptr = vector->begin(); ptr != vector->end(); ++ptr)
		{
			ptr->compile(tdbb);
			trigger = EXE_find_request(tdbb, ptr->request, false);
			trigger->req_rpb[0].rpb_record = old_rec ? old_rec : null_rec;
			trigger->req_rpb[1].rpb_record = new_rec ? new_rec : null_rec;

			if (old_rec && trigger_action != jrd_req::req_trigger_insert)
			{
				trigger->req_rpb[0].rpb_number = old_rpb->rpb_number;
				trigger->req_rpb[0].rpb_number.setValid(true);
			}
			else
				trigger->req_rpb[0].rpb_number.setValid(false);

			if (new_rec && !(which_trig == PRE_TRIG && trigger_action == jrd_req::req_trigger_insert))
			{
				if (which_trig == PRE_TRIG && trigger_action == jrd_req::req_trigger_update)
					new_rpb->rpb_number = old_rpb->rpb_number;

				trigger->req_rpb[1].rpb_number = new_rpb->rpb_number;
				trigger->req_rpb[1].rpb_number.setValid(true);
			}
			else
				trigger->req_rpb[1].rpb_number.setValid(false);

			if (tdbb->getRequest())
				trigger->req_timestamp = tdbb->getRequest()->req_timestamp;
			else
				trigger->req_timestamp = timestamp;

			trigger->req_trigger_action = trigger_action;

			TraceTrigExecute trace(tdbb, trigger, which_trig);

			EXE_start(tdbb, trigger, transaction);

			const bool ok = (trigger->req_operation != jrd_req::req_unwind);
			trace.finish(ok ? res_successful : res_failed);

			EXE_unwind(tdbb, trigger);

			trigger->req_attachment = NULL;
			trigger->req_flags &= ~req_in_use;
			trigger->req_timestamp.invalidate();

			if (!ok)
			{
				result = trigger;
				break;
			}
			trigger = NULL;
		}

		delete null_rec;
		if (vector != *triggers) {
			MET_release_triggers(tdbb, &vector);
		}

		return result;
	}
	catch (const Firebird::Exception& ex)
	{
		delete null_rec;
		if (vector != *triggers) {
			MET_release_triggers(tdbb, &vector);
		}
		if (!trigger) {
		  throw; // trigger probally fails to compile
		}
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		return trigger;
	}
}


static void get_string(thread_db* tdbb, jrd_req* request, jrd_nod* node, Firebird::string& str, bool useAttCS)
{
	MoveBuffer buffer;
	UCHAR* p = NULL;
	SSHORT len = 0;
	const dsc* dsc = node ? EVL_expr(tdbb, node) : NULL;
	if (dsc && !(request->req_flags & req_null)) 
	{
		const Attachment* att = tdbb->getAttachment();
		len = MOV_make_string2(tdbb, dsc, useAttCS ? att->att_charset : dsc->getTextType(), &p, buffer);
	}
	str = Firebird::string((char*) p, len);
	str.trim();
}


static void stuff_stack_trace(const jrd_req* request)
{
	Firebird::string sTrace;
	bool isEmpty = true;

	for (const jrd_req* req = request; req; req = req->req_caller)
	{
		Firebird::string name;

		if (req->req_trg_name.length()) {
			name = "At trigger '";
			name += req->req_trg_name.c_str();
		}
		else if (req->req_procedure) {
			name = "At procedure '";
			name += req->req_procedure->prc_name.c_str();
		}

		if (! name.isEmpty())
		{
			name.trim();

			if (sTrace.length() + name.length() + 2 > MAX_STACK_TRACE)
				break;

			if (isEmpty) {
				isEmpty = false;
				sTrace += name + "'";
			}
			else {
				sTrace += "\n" + name + "'";
			}

			if (req->req_src_line)
			{
				Firebird::string src_info;
				src_info.printf(" line: %u, col: %u", req->req_src_line, req->req_src_column);

				if (sTrace.length() + src_info.length() > MAX_STACK_TRACE)
					break;

				sTrace += src_info;
			}
		}
	}

	if (!isEmpty)
		ERR_post_nothrow(Arg::Gds(isc_stack_trace) << Arg::Str(sTrace));
}


jrd_nod* EXE_looper(thread_db* tdbb, jrd_req* request, jrd_nod* in_node)
{
/**************************************
 *
 *	E X E _ l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Cycle thru request execution tree.  Return next node for
 *	execution on stall or request complete.
 *
 **************************************/
	SSHORT which_erase_trig = 0;
	SSHORT which_sto_trig   = 0;
	SSHORT which_mod_trig   = 0;
	jrd_nod* top_node = 0;
	jrd_nod* prev_node;

	jrd_tra* transaction = request->req_transaction;
	if (!transaction) {
		ERR_post(Arg::Gds(isc_req_no_trans));
	}

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BLKCHK(in_node, type_nod);

	// Save the old pool and request to restore on exit
	MemoryPool* old_pool = tdbb->getDefaultPool();
	Jrd::ContextPoolHolder context(tdbb, request->req_pool);

	jrd_req* old_request = tdbb->getRequest();
	tdbb->setRequest(request);
	jrd_tra* old_transaction = tdbb->getTransaction();
	tdbb->setTransaction(transaction);

	if (in_node->nod_type != nod_stmt_expr)
	{
	    fb_assert(request->req_caller == NULL);
		request->req_caller = old_request;
	}
	else
		request->req_operation = jrd_req::req_evaluate;

	const SLONG save_point_number = (transaction->tra_save_point) ?
		transaction->tra_save_point->sav_number : 0;

	jrd_nod* node = in_node;

	// Catch errors so we can unwind cleanly

	bool error_pending = false;
	bool catch_disabled = false;
	tdbb->tdbb_flags &= ~(TDBB_stack_trace_done | TDBB_sys_error);

	// Execute stuff until we drop

	while (node && !(request->req_flags & req_stall))
	{
	try {

		if (request->req_operation == jrd_req::req_evaluate && (--tdbb->tdbb_quantum < 0))
		{
			JRD_reschedule(tdbb, 0, true);
		}

		// Reset the local transaction pointer as the current transaction (req_transaction)
		// could have been changed inside the loop (see InAutonomousTransactionNode::execute)
		transaction = request->req_transaction;

		switch (node->nod_type)
		{
		case nod_asn_list:
			if (request->req_operation == jrd_req::req_evaluate) {
				jrd_nod** ptr = node->nod_arg;
				for (const jrd_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
				{
					EXE_assignment(tdbb, *ptr);
				}
				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_assignment:
			if (request->req_operation == jrd_req::req_evaluate)
				EXE_assignment(tdbb, node);
			node = node->nod_parent;
			break;

		case nod_dcl_variable:
			{
				impure_value* variable = (impure_value*) ((SCHAR*) request + node->nod_impure);
				variable->vlu_desc = *(DSC*) (node->nod_arg + e_dcl_desc);
				variable->vlu_desc.dsc_flags = 0;
				variable->vlu_desc.dsc_address = (UCHAR*) &variable->vlu_misc;

				if (variable->vlu_desc.dsc_dtype <= dtype_varying && !variable->vlu_string)
				{
					const USHORT len = variable->vlu_desc.dsc_length;
					variable->vlu_string = FB_NEW_RPT(*tdbb->getDefaultPool(), len) VaryingString();
					variable->vlu_string->str_length = len;
					variable->vlu_desc.dsc_address = variable->vlu_string->str_data;
				}

				request->req_operation = jrd_req::req_return;
				node = node->nod_parent;
			}
			break;

		case nod_erase:
			if (request->req_operation == jrd_req::req_unwind) {
				node = node->nod_parent;
			}
			else if ((request->req_operation == jrd_req::req_return) &&
				(node->nod_arg[e_erase_sub_erase]))
			{
				if (!top_node) {
					top_node = node;
					which_erase_trig = PRE_TRIG;
				}
				prev_node = node;
				node = erase(tdbb, node, which_erase_trig);
				if (which_erase_trig == PRE_TRIG) {
					node = prev_node->nod_arg[e_erase_sub_erase];
					node->nod_parent = prev_node;
				}
				if (top_node == prev_node && which_erase_trig == POST_TRIG) {
					top_node = NULL;
					which_erase_trig = ALL_TRIGS;
				}
				else
					request->req_operation = jrd_req::req_evaluate;
			}
			else {
				prev_node = node;
				node = erase(tdbb, node, ALL_TRIGS);
				if (!(prev_node->nod_arg[e_erase_sub_erase]) && which_erase_trig == PRE_TRIG)
				{
					which_erase_trig = POST_TRIG;
				}
			}
			break;

		case nod_exec_proc:
			if (request->req_operation == jrd_req::req_unwind) {
				node = node->nod_parent;
				break;
			}
			execute_procedure(tdbb, node);
			node = node->nod_parent;
			request->req_operation = jrd_req::req_return;
			break;

		case nod_for:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				RSE_open(tdbb, (RecordSource*) node->nod_arg[e_for_rsb]);
				request->req_records_affected.clear();
			case jrd_req::req_return:
				if (node->nod_arg[e_for_stall]) {
					node = node->nod_arg[e_for_stall];
					break;
				}
			case jrd_req::req_sync:
				if (RSE_get_record(tdbb, (RecordSource*) node->nod_arg[e_for_rsb], g_RSE_get_mode))
				{
					request->req_records_selected++;
					request->req_records_affected.bumpFetched();
					node = node->nod_arg[e_for_statement];
					request->req_operation = jrd_req::req_evaluate;
					break;
				}
				request->req_operation = jrd_req::req_return;
			default:
				RSE_close(tdbb, (RecordSource*) node->nod_arg[e_for_rsb]);
				node = node->nod_parent;
			}
			break;

		case nod_dcl_cursor:
			if (request->req_operation == jrd_req::req_evaluate) {
				const USHORT number = (USHORT) (IPTR) node->nod_arg[e_dcl_cursor_number];
				// set up the cursors vector
				request->req_cursors = vec<RecordSource*>::newVector(*request->req_pool,
					request->req_cursors, number + 1);
				// store RecordSource in the vector
				(*request->req_cursors)[number] = (RecordSource*) node->nod_arg[e_dcl_cursor_rsb];
				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_cursor_stmt:
			{
				const UCHAR op = (UCHAR) (IPTR) node->nod_arg[e_cursor_stmt_op];
				const USHORT number = (USHORT) (IPTR) node->nod_arg[e_cursor_stmt_number];
				// get RecordSource and the impure area
				fb_assert(request->req_cursors && number < request->req_cursors->count());
				RecordSource* rsb = (*request->req_cursors)[number];
				IRSB impure = (IRSB) ((UCHAR*) tdbb->getRequest() + rsb->rsb_impure);
				switch (op)
				{
				case blr_cursor_open:
					if (request->req_operation == jrd_req::req_evaluate) {
						// check cursor state
						if (impure->irsb_flags & irsb_open) {
							ERR_post(Arg::Gds(isc_cursor_already_open));
						}
						// open cursor
						RSE_open(tdbb, rsb);
						request->req_operation = jrd_req::req_return;
					}
					node = node->nod_parent;
					break;
				case blr_cursor_close:
					if (request->req_operation == jrd_req::req_evaluate) {
						// check cursor state
						if (!(impure->irsb_flags & irsb_open)) {
							ERR_post(Arg::Gds(isc_cursor_not_open));
						}
						// close cursor
						RSE_close(tdbb, rsb);
						request->req_operation = jrd_req::req_return;
					}
					node = node->nod_parent;
					break;
				case blr_cursor_fetch:
					switch (request->req_operation)
					{
					case jrd_req::req_evaluate:
						// check cursor state
						if (!(impure->irsb_flags & irsb_open)) {
							ERR_post(Arg::Gds(isc_cursor_not_open));
						}
						if (impure->irsb_flags & irsb_eof) {
							ERR_post(Arg::Gds(isc_stream_eof));
						}
						request->req_records_affected.clear();
						// perform preliminary navigation, if specified
						if (node->nod_arg[e_cursor_stmt_seek]) {
							node = node->nod_arg[e_cursor_stmt_seek];
							break;
						}
						// fetch one record
						if (RSE_get_record(tdbb, rsb, g_RSE_get_mode))
						{
							request->req_records_selected++;
							request->req_records_affected.bumpFetched();
							node = node->nod_arg[e_cursor_stmt_into];
							request->req_operation = jrd_req::req_evaluate;
							break;
						}
						request->req_operation = jrd_req::req_return;
					default:
						node = node->nod_parent;
					}
					break;
				}
			}
			break;

		case nod_abort:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				{
					PsqlException* xcp_node = reinterpret_cast<PsqlException*>(node->nod_arg[e_xcp_desc]);
					if (xcp_node)
					{
						/* PsqlException is defined,
						   so throw an exception */
						set_error(tdbb, &xcp_node->xcp_rpt[0], node->nod_arg[e_xcp_msg]);
					}
					else if (!request->req_last_xcp.success())
					{
						/* PsqlException is undefined, but there was a known exception before,
						   so re-initiate it */
						set_error(tdbb, NULL, NULL);
					}
					else
					{
						/* PsqlException is undefined and there weren't any exceptions before,
						   so just do nothing */
						request->req_operation = jrd_req::req_return;
					}
				}

			default:
				node = node->nod_parent;
			}
			break;

		case nod_user_savepoint:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				if (transaction != dbb->dbb_sys_trans)
				{

					const UCHAR operation = (UCHAR) (IPTR) node->nod_arg[e_sav_operation];
					const TEXT* node_savepoint_name = (TEXT*) node->nod_arg[e_sav_name];

					// Skip the savepoint created by EXE_start
					Savepoint* savepoint = transaction->tra_save_point->sav_next;
					Savepoint* previous = transaction->tra_save_point;

					// Find savepoint
					bool found = false;
					while (true)
					{
						if (!savepoint || !(savepoint->sav_flags & SAV_user))
							break;

						if (!strcmp(node_savepoint_name, savepoint->sav_name)) {
							found = true;
							break;
						}

						previous = savepoint;
						savepoint = savepoint->sav_next;
					}
					if (!found && operation != blr_savepoint_set) {
						ERR_post(Arg::Gds(isc_invalid_savepoint) << Arg::Str(node_savepoint_name));
					}

					switch (operation)
					{
					case blr_savepoint_set:
						// Release the savepoint
						if (found) {
							Savepoint* const current = transaction->tra_save_point;
							transaction->tra_save_point = savepoint;
							verb_cleanup(tdbb, transaction);
							previous->sav_next = transaction->tra_save_point;
							transaction->tra_save_point = current;
						}

						// Use the savepoint created by EXE_start
						transaction->tra_save_point->sav_flags |= SAV_user;
						strcpy(transaction->tra_save_point->sav_name, node_savepoint_name);
						break;
					case blr_savepoint_release_single:
					{
						// Release the savepoint
						Savepoint* const current = transaction->tra_save_point;
						transaction->tra_save_point = savepoint;
						verb_cleanup(tdbb, transaction);
						previous->sav_next = transaction->tra_save_point;
						transaction->tra_save_point = current;
						break;
					}
					case blr_savepoint_release:
					{
						const SLONG sav_number = savepoint->sav_number;

						// Release the savepoint and all subsequent ones
						while (transaction->tra_save_point &&
							transaction->tra_save_point->sav_number >= sav_number)
						{
							verb_cleanup(tdbb, transaction);
						}

						// Restore the savepoint initially created by EXE_start
						VIO_start_save_point(tdbb, transaction);
						break;
					}
					case blr_savepoint_undo:
					{
						const SLONG sav_number = savepoint->sav_number;

						// Undo the savepoint
						while (transaction->tra_save_point &&
							transaction->tra_save_point->sav_number >= sav_number)
						{
							transaction->tra_save_point->sav_verb_count++;
							verb_cleanup(tdbb, transaction);
						}

						// Now set the savepoint again to allow to return to it later
						VIO_start_save_point(tdbb, transaction);
						transaction->tra_save_point->sav_flags |= SAV_user;
						strcpy(transaction->tra_save_point->sav_name, node_savepoint_name);
						break;
					}
					default:
						BUGCHECK(232);
						break;
					}
				}
			default:
				node = node->nod_parent;
				request->req_operation = jrd_req::req_return;
			}
			break;

		case nod_start_savepoint:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				/* Start a save point */

				if (transaction != dbb->dbb_sys_trans)
					VIO_start_save_point(tdbb, transaction);

			default:
				node = node->nod_parent;
				request->req_operation = jrd_req::req_return;
			}
			break;

		case nod_end_savepoint:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
			case jrd_req::req_unwind:
				/* If any requested modify/delete/insert
				   ops have completed, forget them */
				if (transaction != dbb->dbb_sys_trans) {
					/* If an error is still pending when the savepoint is
					   supposed to end, then the application didn't handle the
					   error and the savepoint should be undone. */
					if (error_pending) {
						++transaction->tra_save_point->sav_verb_count;
					}
					verb_cleanup(tdbb, transaction);
				}

			default:
				node = node->nod_parent;
				request->req_operation = jrd_req::req_return;
			}
			break;

		case nod_handler:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				node = node->nod_arg[0];
				break;

			case jrd_req::req_unwind:
				if (!request->req_label)
					request->req_operation = jrd_req::req_return;

			default:
				node = node->nod_parent;
			}
			break;

		case nod_block:
			switch (request->req_operation)
			{
			SLONG count;

			case jrd_req::req_evaluate:
				if (transaction != dbb->dbb_sys_trans) {
					VIO_start_save_point(tdbb, transaction);
					const Savepoint* save_point = transaction->tra_save_point;
					count = save_point->sav_number;
					memcpy((SCHAR*) request + node->nod_impure, &count, sizeof(SLONG));
				}
				node = node->nod_arg[e_blk_action];
				break;

			case jrd_req::req_unwind:
				{
					if (request->req_flags & req_leave)
					{
						// Although the req_operation is set to req_unwind,
						// it's not an error case if req_leave bit is set.
						// req_leave bit indicates that we hit an EXIT or
						// BREAK/LEAVE statement in the SP/trigger code.
						// Do not perform the error handling stuff.

						if (transaction != dbb->dbb_sys_trans) {
							memcpy(&count, (SCHAR*) request + node->nod_impure, sizeof(SLONG));

							for (const Savepoint* save_point = transaction->tra_save_point;
								save_point && count <= save_point->sav_number;
								save_point = transaction->tra_save_point)
							{
								verb_cleanup(tdbb, transaction);
							}
						}

						node = node->nod_parent;
						break;
					}
					if (transaction != dbb->dbb_sys_trans)
					{
						memcpy(&count, (SCHAR*) request + node->nod_impure, sizeof(SLONG));
						/* Since there occurred an error (req_unwind), undo all savepoints
						   up to, but not including, the savepoint of this block.  The
						   savepoint of this block will be dealt with below. */
						for (const Savepoint* save_point = transaction->tra_save_point;
							save_point && count < save_point->sav_number;
							save_point = transaction->tra_save_point)
						{
							++transaction->tra_save_point->sav_verb_count;
							verb_cleanup(tdbb, transaction);
						}
					}

					jrd_nod* handlers = node->nod_arg[e_blk_handlers];
					if (handlers)
					{
						node = node->nod_parent;
						jrd_nod** ptr = handlers->nod_arg;
						for (const jrd_nod* const* const end = ptr + handlers->nod_count;
							ptr < end; ptr++)
						{
							const PsqlException* xcp_node =
								reinterpret_cast<PsqlException*>((*ptr)->nod_arg[e_err_conditions]);
							if (test_and_fixup_error(tdbb, xcp_node, request))
							{
								request->req_operation = jrd_req::req_evaluate;
								node = (*ptr)->nod_arg[e_err_action];
								error_pending = false;

								/* On entering looper old_request etc. are saved.
								   On recursive calling we will loose the actual old
								   request for that invocation of looper. Avoid this. */

								{
									Jrd::ContextPoolHolder contextLooper(tdbb, old_pool);
									tdbb->setRequest(old_request);
									fb_assert(request->req_caller == old_request);
									request->req_caller = NULL;

									/* Save the previous state of req_error_handler
									   bit. We need to restore it later. This is
									   necessary if the error handler is deeply
									   nested. */

									const ULONG prev_req_error_handler =
										request->req_flags & req_error_handler;
									request->req_flags |= req_error_handler;
									node = EXE_looper(tdbb, request, node);
									request->req_flags &= ~req_error_handler;
									request->req_flags |= prev_req_error_handler;

									transaction = request->req_transaction;

									/* Note: Previously the above call
									   "node = looper (tdbb, request, node);"
									   never returned back till the node tree
									   was executed completely. Now that the looper
									   has changed its behaviour such that it
									   returns back after handling error. This
									   makes it necessary that the jmpbuf be reset
									   so that looper can proceede with the
									   processing of execution tree. If this is
									   not done then anymore errors will take the
									   engine out of looper there by abruptly
									   terminating the processing. */

									catch_disabled = false;
									tdbb->setRequest(request);
									fb_assert(request->req_caller == NULL);
									request->req_caller = old_request;
								}

								/* The error is dealt with by the application, cleanup
								   this block's savepoint. */

								if (transaction != dbb->dbb_sys_trans)
								{
									for (const Savepoint* save_point = transaction->tra_save_point;
										save_point && count <= save_point->sav_number;
										save_point = transaction->tra_save_point)
									{
										verb_cleanup(tdbb, transaction);
									}
								}
							}
						}
					}
					else
					{
						node = node->nod_parent;
					}

					/* If the application didn't have an error handler, then
					   the error will still be pending.  Undo the block by
					   using its savepoint. */

					if (error_pending && transaction != dbb->dbb_sys_trans)
					{
						for (const Savepoint* save_point = transaction->tra_save_point;
							save_point && count <= save_point->sav_number;
							save_point = transaction->tra_save_point)
						{
							++transaction->tra_save_point->sav_verb_count;
							verb_cleanup(tdbb, transaction);
						}
					}
				}
				break;

			case jrd_req::req_return:
				if (transaction != dbb->dbb_sys_trans)
				{
					memcpy(&count, (SCHAR*) request + node->nod_impure, sizeof(SLONG));

					for (const Savepoint* save_point = transaction->tra_save_point;
						 save_point && count <= save_point->sav_number;
						 save_point = transaction->tra_save_point)
					{
						verb_cleanup(tdbb, transaction);
					}
				}
			default:
				node = node->nod_parent;
			}
			break;

		case nod_error_handler:
			if (request->req_flags & req_error_handler && !error_pending)
			{
				fb_assert(request->req_caller == old_request);
				request->req_caller = NULL;
				return node;
			}
			node = node->nod_parent;
			node = node->nod_parent;
			if (request->req_operation == jrd_req::req_unwind) {
				node = node->nod_parent;
			}
			request->req_last_xcp.clear();
			break;

		case nod_label:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
				node = node->nod_arg[e_lbl_statement];
				break;

			case jrd_req::req_unwind:
				if ((request->req_label == (USHORT)(IPTR) node->nod_arg[e_lbl_label]) &&
					(request->req_flags & (req_leave | req_error_handler)))
				{
					request->req_flags &= ~req_leave;
					request->req_operation = jrd_req::req_return;
				}

			default:
				node = node->nod_parent;
			}
			break;

		case nod_leave:
			request->req_flags |= req_leave;
			request->req_operation = jrd_req::req_unwind;
			request->req_label = (USHORT)(IPTR) node->nod_arg[0];
			node = node->nod_parent;
			break;

		case nod_list:
			{
				impure_state* impure = (impure_state*) ((SCHAR *) request + node->nod_impure);
				switch (request->req_operation)
				{
				case jrd_req::req_evaluate:
					impure->sta_state = 0;
				case jrd_req::req_return:
				case jrd_req::req_sync:
					if (impure->sta_state < node->nod_count) {
						request->req_operation = jrd_req::req_evaluate;
						node = node->nod_arg[impure->sta_state++];
						break;
					}
					request->req_operation = jrd_req::req_return;
				default:
					node = node->nod_parent;
				}
			}
			break;

		case nod_loop:
			switch (request->req_operation)
			{
			case jrd_req::req_evaluate:
			case jrd_req::req_return:
				node = node->nod_arg[0];
				request->req_operation = jrd_req::req_evaluate;
				break;

			default:
				node = node->nod_parent;
			}
			break;

		case nod_if:
			if (request->req_operation == jrd_req::req_evaluate)
			{
				if (EVL_boolean(tdbb, node->nod_arg[e_if_boolean])) {
					node = node->nod_arg[e_if_true];
					request->req_operation = jrd_req::req_evaluate;
					break;
				}

				if (node->nod_arg[e_if_false]) {
					node = node->nod_arg[e_if_false];
					request->req_operation = jrd_req::req_evaluate;
					break;
				}

				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_modify:
			{
				impure_state* impure = (impure_state*) ((SCHAR *) request + node->nod_impure);
				if (request->req_operation == jrd_req::req_unwind) {
					node = node->nod_parent;
				}
				else if ((request->req_operation == jrd_req::req_return) &&
					(!impure->sta_state) && (node->nod_arg[e_mod_sub_mod]))
				{
					if (!top_node) {
						top_node = node;
						which_mod_trig = PRE_TRIG;
					}
					prev_node = node;
					node = modify(tdbb, node, which_mod_trig);
					if (which_mod_trig == PRE_TRIG) {
						node = prev_node->nod_arg[e_mod_sub_mod];
						node->nod_parent = prev_node;
					}
					if (top_node == prev_node && which_mod_trig == POST_TRIG) {
						top_node = NULL;
						which_mod_trig = ALL_TRIGS;
					}
					else {
						request->req_operation = jrd_req::req_evaluate;
					}
				}
				else {
					prev_node = node;
					node = modify(tdbb, node, ALL_TRIGS);
					if (!(prev_node->nod_arg[e_mod_sub_mod]) && which_mod_trig == PRE_TRIG)
					{
						which_mod_trig = POST_TRIG;
					}
				}
			}
			break;

		case nod_nop:
			request->req_operation = jrd_req::req_return;
			node = node->nod_parent;
			break;

		case nod_receive:
			node = receive_msg(tdbb, node);
			break;

		case nod_exec_sql:
			if (request->req_operation == jrd_req::req_unwind) {
				node = node->nod_parent;
				break;
			}
			exec_sql(tdbb, request, EVL_expr(tdbb, node->nod_arg[0]));
			if (request->req_operation == jrd_req::req_evaluate)
				request->req_operation = jrd_req::req_return;
			node = node->nod_parent;
			break;

		case nod_exec_into:
			{
				ExecuteStatement* impure = (ExecuteStatement*) ((SCHAR*) request + node->nod_impure);

				switch (request->req_operation)
				{
				case jrd_req::req_evaluate:
					impure->open(tdbb, node->nod_arg[0], node->nod_count - 2, !node->nod_arg[1]);
					// fall into ???
				case jrd_req::req_return:
				case jrd_req::req_sync:
					if (impure->fetch(tdbb, &node->nod_arg[2])) {
						request->req_operation = jrd_req::req_evaluate;
						node = node->nod_arg[1];
						break;
					}
					request->req_operation = jrd_req::req_return;
				default:
					// if have active opened request - close it
					impure->close(tdbb);
					node = node->nod_parent;
				}
			}
			break;

		case nod_exec_stmt:
			node = execute_statement(tdbb, request, node);
			break;

		case nod_post:
			{
				DeferredWork* work =
					DFW_post_work(transaction, dfw_post_event, EVL_expr(tdbb, node->nod_arg[0]), 0);
				if (node->nod_arg[1])
					DFW_post_work_arg(transaction, work, EVL_expr(tdbb, node->nod_arg[1]), 0);
			}

			// for an autocommit transaction, events can be posted
			// without any updates

			if (transaction->tra_flags & TRA_autocommit)
				transaction->tra_flags |= TRA_perform_autocommit;

			if (request->req_operation == jrd_req::req_evaluate)
				request->req_operation = jrd_req::req_return;
			node = node->nod_parent;
			break;

		case nod_message:
			if (request->req_operation == jrd_req::req_evaluate)
			{
				const Format* format = (Format*) node->nod_arg[e_msg_format];
				USHORT* impure_flags =
					(USHORT*) ((UCHAR*) request + (IPTR) node->nod_arg[e_msg_impure_flags]);
				memset(impure_flags, 0, sizeof(USHORT) * format->fmt_count);
				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_stall:
			node = stall(tdbb, node);
			break;

		case nod_select:
			node = selct(tdbb, node);
			break;

		case nod_send:
			node = send_msg(tdbb, node);
			break;

		case nod_store:
			{
				impure_state* impure = (impure_state*) ((SCHAR *) request + node->nod_impure);
				if ((request->req_operation == jrd_req::req_return) &&
					(!impure->sta_state) && (node->nod_arg[e_sto_sub_store]))
				{
					if (!top_node) {
						top_node = node;
						which_sto_trig = PRE_TRIG;
					}
					prev_node = node;
					node = store(tdbb, node, which_sto_trig);
					if (which_sto_trig == PRE_TRIG) {
						node = prev_node->nod_arg[e_sto_sub_store];
						node->nod_parent = prev_node;
					}
					if (top_node == prev_node && which_sto_trig == POST_TRIG) {
						top_node = NULL;
						which_sto_trig = ALL_TRIGS;
					}
					else
						request->req_operation = jrd_req::req_evaluate;
				}
				else {
					prev_node = node;
					node = store(tdbb, node, ALL_TRIGS);
					if (!(prev_node->nod_arg[e_sto_sub_store]) && which_sto_trig == PRE_TRIG)
						which_sto_trig = POST_TRIG;
				}
			}
			break;

#ifdef SCROLLABLE_CURSORS
		case nod_seek:
			node = seek_rse(tdbb, request, node);
			break;
#endif

		case nod_set_generator:
		case nod_set_generator2:
			if (request->req_operation == jrd_req::req_evaluate) {
				dsc* desc = EVL_expr(tdbb, node->nod_arg[e_gen_value]);
				DPM_gen_id(tdbb, (IPTR) node->nod_arg[e_gen_id], true, MOV_get_int64(desc, 0));
				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_src_info:
			if (request->req_operation == jrd_req::req_evaluate) {
				request->req_src_line = (USHORT) (IPTR) node->nod_arg[e_src_info_line];
				request->req_src_column = (USHORT) (IPTR) node->nod_arg[e_src_info_col];
				//request->req_operation = jrd_req::req_return;
				node = node->nod_arg[e_src_info_node];
			}
			else
				node = node->nod_parent;
			break;

		case nod_init_variable:
			if (request->req_operation == jrd_req::req_evaluate)
			{
				const ItemInfo* itemInfo =
					reinterpret_cast<const ItemInfo*>(node->nod_arg[e_init_var_info]);
				if (itemInfo)
				{
					jrd_nod* var_node = node->nod_arg[e_init_var_variable];
					DSC* to_desc = &((impure_value*) ((SCHAR *) request + var_node->nod_impure))->vlu_desc;

					to_desc->dsc_flags |= DSC_null;

					MapFieldInfo::ValueType fieldInfo;
					if (itemInfo->fullDomain &&
						request->req_map_field_info.get(itemInfo->field, fieldInfo) &&
						fieldInfo.defaultValue)
					{
						dsc* value = EVL_expr(tdbb, fieldInfo.defaultValue);

						if (value && !(request->req_flags & req_null))
						{
							to_desc->dsc_flags &= ~DSC_null;
							MOV_move(tdbb, value, to_desc);
						}
					}
				}

				request->req_operation = jrd_req::req_return;
			}
			node = node->nod_parent;
			break;

		case nod_class_node_jrd:
			node = reinterpret_cast<StmtNode*>(node->nod_arg[0])->execute(tdbb, request);
			break;

		case nod_stmt_expr:
			if (request->req_operation == jrd_req::req_evaluate)
				node = node->nod_arg[e_stmt_expr_stmt];
			else
				node = NULL;
			break;

		default:
			BUGCHECK(168);		/* msg 168 looper: action not yet implemented */
		}
	}	// try
	catch (const Firebird::Exception& ex)
	{

		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);

		request->adjustCallerStats();

		// Reset the local transaction pointer as the current transaction (req_transaction)
		// could have been changed inside the loop (see InAutonomousTransactionNode::execute)
		transaction = request->req_transaction;

		// Skip this handling for errors coming from the nested looper calls,
		// as they're already handled properly. The only need is to undo
		// our own savepoints.
		if (catch_disabled)
		{
			if (transaction != dbb->dbb_sys_trans) {
				for (const Savepoint* save_point = transaction->tra_save_point;
					((save_point) && (save_point_number <= save_point->sav_number));
					save_point = transaction->tra_save_point)
				{
					++transaction->tra_save_point->sav_verb_count;
					verb_cleanup(tdbb, transaction);
				}
			}

			ERR_punt();
		}

		// If the database is already bug-checked, then get out
		if (dbb->dbb_flags & DBB_bugcheck) {
			Firebird::status_exception::raise(tdbb->tdbb_status_vector);
		}

		// Since an error happened, the current savepoint needs to be undone
		if (transaction != dbb->dbb_sys_trans && transaction->tra_save_point)
		{
			++transaction->tra_save_point->sav_verb_count;
			verb_cleanup(tdbb, transaction);
		}

		error_pending = true;
		catch_disabled = true;
		request->req_operation = jrd_req::req_unwind;
		request->req_label = 0;

		if (!(tdbb->tdbb_flags & TDBB_stack_trace_done) && !(tdbb->tdbb_flags & TDBB_sys_error))
		{
			stuff_stack_trace(request);
			tdbb->tdbb_flags |= TDBB_stack_trace_done;
		}
	}
	} // while()

	request->adjustCallerStats();

	// Reset the local transaction pointer as the current transaction (req_transaction)
	// could have been changed inside the loop (see InAutonomousTransactionNode::execute)
	transaction = request->req_transaction;

	fb_assert(request->req_auto_trans.getCount() == 0);

	// If there is no node, assume we have finished processing the
	// request unless we are in the middle of processing an
	// asynchronous message

	if (in_node->nod_type != nod_stmt_expr && !node
#ifdef SCROLLABLE_CURSORS
		&& !(request->req_flags & req_async_processing)
#endif
		)
	{
		// Close active cursors
		if (request->req_cursors)
		{
			for (vec<RecordSource*>::iterator ptr = request->req_cursors->begin(),
				end = request->req_cursors->end(); ptr < end; ++ptr)
			{
				if (*ptr)
					RSE_close(tdbb, *ptr);
			}
		}

		request->req_flags &= ~(req_active | req_reserved);
		request->req_timestamp.invalidate();
		release_blobs(tdbb, request);
	}

	request->req_next = node;
	tdbb->setTransaction(old_transaction);
	tdbb->setRequest(old_request);

	if (in_node->nod_type != nod_stmt_expr)
	{
		fb_assert(request->req_caller == old_request);
		request->req_caller = NULL;
	}

	// In the case of a pending error condition (one which did not
	// result in a exception to the top of looper), we need to
	// delete the last savepoint

	if (error_pending)
	{
		if (transaction != dbb->dbb_sys_trans) {
			for (const Savepoint* save_point = transaction->tra_save_point;
				((save_point) && (save_point_number <= save_point->sav_number));
				 save_point = transaction->tra_save_point)
			{
				++transaction->tra_save_point->sav_verb_count;
				verb_cleanup(tdbb, transaction);
			}
		}

		ERR_punt();
	}

	// if the request was aborted, assume that we have already
	// longjmp'ed to the top of looper, and therefore that the
	// last savepoint has already been deleted

	if (request->req_flags & req_abort) {
		ERR_post(Arg::Gds(isc_req_sync));
	}

	return node;
}


// Start looper under Windows SEH (Structured Exception Handling) control
static void looper_seh(thread_db* tdbb, jrd_req* request)
{
#ifdef WIN_NT
	START_CHECK_FOR_EXCEPTIONS(NULL);
#endif
	// TODO:
	// 1. Try to fix the problem with MSVC C++ runtime library, making
	// even C++ exceptions that are implemented in terms of Win32 SEH
	// getting catched by the SEH handler below.
	// 2. Check if it really is correct that only Win32 catches CPU
	// exceptions (such as SEH) here. Shouldn't any platform capable
	// of handling signals use this stuff?
	// (see jrd/ibsetjmp.h for implementation of these macros)

	EXE_looper(tdbb, request, request->req_top_node);

#ifdef WIN_NT
	END_CHECK_FOR_EXCEPTIONS(NULL);
#endif
}


static jrd_nod* modify(thread_db* tdbb, jrd_nod* node, SSHORT which_trig)
{
/**************************************
 *
 *	m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Execute a MODIFY statement.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request->req_transaction;
	impure_state* impure = (impure_state*) ((SCHAR *) request + node->nod_impure);

	const SSHORT org_stream = (USHORT)(IPTR) node->nod_arg[e_mod_org_stream];
	record_param* org_rpb = &request->req_rpb[org_stream];
	jrd_rel* relation = org_rpb->rpb_relation;

	if (org_rpb->rpb_number.isBof() || (!relation->rel_view_rse && !org_rpb->rpb_number.isValid()))
	{
		ERR_post(Arg::Gds(isc_no_cur_rec));
	}

	const SSHORT new_stream = (USHORT)(IPTR) node->nod_arg[e_mod_new_stream];
	record_param* new_rpb = &request->req_rpb[new_stream];

	/* If the stream was sorted, the various fields in the rpb are
	probably junk.  Just to make sure that everything is cool,
	refetch and release the record. */

	if (org_rpb->rpb_stream_flags & RPB_s_refetch) {
		VIO_refetch_record(tdbb, org_rpb, transaction);
		org_rpb->rpb_stream_flags &= ~RPB_s_refetch;
	}

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		request->req_records_affected.bumpModified(false);
		break;

	case jrd_req::req_return:
		if (impure->sta_state == 1)
		{
			impure->sta_state = 0;
			Record* org_record = org_rpb->rpb_record;
			const Record* new_record = new_rpb->rpb_record;
			memcpy(org_record->rec_data, new_record->rec_data, new_record->rec_length);
			request->req_operation = jrd_req::req_evaluate;
			return node->nod_arg[e_mod_statement];
		}

		if (impure->sta_state == 0)
		{
			/* CVC: This call made here to clear the record in each NULL field and
					varchar field whose tail may contain garbage. */
			cleanup_rpb(tdbb, new_rpb);

			if (transaction != dbb->dbb_sys_trans)
				++transaction->tra_save_point->sav_verb_count;

			PreModifyEraseTriggers(tdbb, &relation->rel_pre_modify, which_trig, org_rpb, new_rpb,
								   jrd_req::req_trigger_update);

			if (node->nod_arg[e_mod_validate]) {
				validate(tdbb, node->nod_arg[e_mod_validate]);
			}

			if (relation->rel_file)
			{
				EXT_modify(org_rpb, new_rpb, transaction);
			}
			else if (relation->isVirtual()) {
				VirtualTable::modify(tdbb, org_rpb, new_rpb);
			}
			else if (!relation->rel_view_rse)
			{
				USHORT bad_index;
				jrd_rel* bad_relation = 0;

				VIO_modify(tdbb, org_rpb, new_rpb, transaction);
				const idx_e error_code =
					IDX_modify(tdbb, org_rpb, new_rpb, transaction, &bad_relation, &bad_index);

				if (error_code) {
					ERR_duplicate_error(error_code, bad_relation, bad_index);
				}
			}

			new_rpb->rpb_number = org_rpb->rpb_number;
			new_rpb->rpb_number.setValid(true);

			jrd_req* trigger;
			if (relation->rel_post_modify && which_trig != PRE_TRIG &&
				(trigger = execute_triggers(tdbb, &relation->rel_post_modify,
											org_rpb, new_rpb, jrd_req::req_trigger_update, POST_TRIG)))
			{
				trigger_failure(tdbb, trigger);
			}

			/* now call IDX_modify_check_constrints after all post modify triggers
			have fired.  This is required for cascading referential integrity,
			which can be implemented as post_erase triggers */

			if (!relation->rel_file && !relation->rel_view_rse && !relation->isVirtual())
			{
				USHORT bad_index;
				jrd_rel* bad_relation = 0;

				const idx_e error_code =
					IDX_modify_check_constraints(tdbb, org_rpb, new_rpb, transaction,
												 &bad_relation, &bad_index);

				if (error_code) {
					ERR_duplicate_error(error_code, bad_relation, bad_index);
				}
			}

			if (transaction != dbb->dbb_sys_trans) {
				--transaction->tra_save_point->sav_verb_count;
			}

			/* CVC: Increment the counter only if we called VIO/EXT_modify() and
					we were successful. */
			if (!(request->req_view_flags & req_first_modify_return)) {
				request->req_view_flags |= req_first_modify_return;
				if (relation->rel_view_rse) {
					request->req_top_view_modify = relation;
				}
			}
			if (relation == request->req_top_view_modify) {
				if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) {
					request->req_records_updated++;
					request->req_records_affected.bumpModified(true);
				}
			}
			else if (relation->rel_file || !relation->rel_view_rse) {
				request->req_records_updated++;
				request->req_records_affected.bumpModified(true);
			}

			if (node->nod_arg[e_mod_statement2]) {
				impure->sta_state = 2;
				request->req_operation = jrd_req::req_evaluate;
				return node->nod_arg[e_mod_statement2];
			}
		}

		if (which_trig != PRE_TRIG) {
			Record* org_record = org_rpb->rpb_record;
			org_rpb->rpb_record = new_rpb->rpb_record;
			new_rpb->rpb_record = org_record;
		}

	default:
		return node->nod_parent;
	}

	impure->sta_state = 0;
	RLCK_reserve_relation(tdbb, transaction, relation, true);

/* Fall thru on evaluate to set up for modify before executing sub-statement.
   This involves finding the appropriate format, making sure a record block
   exists for the stream and is big enough, and copying fields from the
   original record to the new record. */

	const Format* new_format = MET_current(tdbb, new_rpb->rpb_relation);
	Record* new_record = VIO_record(tdbb, new_rpb, new_format, tdbb->getDefaultPool());
	new_rpb->rpb_address = new_record->rec_data;
	new_rpb->rpb_length = new_format->fmt_length;
	new_rpb->rpb_format_number = new_format->fmt_version;

	const Format* org_format;
	Record* org_record = org_rpb->rpb_record;
	if (!org_record)
	{
		org_record = VIO_record(tdbb, org_rpb, new_format, tdbb->getDefaultPool());
		org_format = org_record->rec_format;
		org_rpb->rpb_address = org_record->rec_data;
		org_rpb->rpb_length = org_format->fmt_length;
		org_rpb->rpb_format_number = org_format->fmt_version;
	}
	else
		org_format = org_record->rec_format;

/* Copy the original record to the new record.  If the format hasn't changed,
   this is a simple move.  If the format has changed, each field must be
   fetched and moved separately, remembering to set the missing flag. */

	if (new_format->fmt_version == org_format->fmt_version) {
		memcpy(new_rpb->rpb_address, org_record->rec_data, new_rpb->rpb_length);
	}
	else
	{
		DSC org_desc, new_desc;

		for (SSHORT i = 0; i < new_format->fmt_count; i++)
		{
			/* In order to "map a null to a default" value (in EVL_field()),
			 * the relation block is referenced.
			 * Reference: Bug 10116, 10424
			 */
			CLEAR_NULL(new_record, i);
			if (EVL_field(new_rpb->rpb_relation, new_record, i, &new_desc))
			{
				if (EVL_field(org_rpb->rpb_relation, org_record, i, &org_desc))
				{
					MOV_move(tdbb, &org_desc, &new_desc);
				}
				else {
					SET_NULL(new_record, i);
					if (new_desc.dsc_dtype) {
						UCHAR* p = new_desc.dsc_address;
						USHORT n = new_desc.dsc_length;
						memset(p, 0, n);
					}
				}				/* if (org_record) */
			}					/* if (new_record) */
		}						/* for (fmt_count) */
	}

	new_rpb->rpb_number = org_rpb->rpb_number;
	new_rpb->rpb_number.setValid(true);

	if (node->nod_arg[e_mod_map_view]) {
		impure->sta_state = 1;
		return node->nod_arg[e_mod_map_view];
	}

	return node->nod_arg[e_mod_statement];
}

static jrd_nod* receive_msg(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	r e c e i v e _ m s g
 *
 **************************************
 *
 * Functional description
 *	Execute a RECEIVE statement.  This can be entered either
 *	with "req_evaluate" (ordinary receive statement) or
 *	"req_proceed" (select statement).  In the latter case,
 *	the statement isn't every formalled evaluated.
 *
 **************************************/
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	BLKCHK(node, type_nod);

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		request->req_operation = jrd_req::req_receive;
		request->req_message = node->nod_arg[e_send_message];
		request->req_flags |= req_stall;
		return node;

	case jrd_req::req_proceed:
		request->req_operation = jrd_req::req_evaluate;
		return (node->nod_arg[e_send_statement]);

	default:
		return (node->nod_parent);
	}
}


static void release_blobs(thread_db* tdbb, jrd_req* request)
{
/**************************************
 *
 *	r e l e a s e _ b l o b s
 *
 **************************************
 *
 * Functional description
 *	Release temporary blobs assigned by this request.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);

	jrd_tra* transaction = request->req_transaction;
	if (transaction)
	{
		DEV_BLKCHK(transaction, type_tra);

		/* Release blobs bound to this request */

		if (request->req_blobs.getFirst())
		{
			while (true)
			{
				const ULONG blob_temp_id = request->req_blobs.current();
				if (transaction->tra_blobs->locate(blob_temp_id))
				{
					BlobIndex *current = &transaction->tra_blobs->current();
					if (current->bli_materialized)
					{
						request->req_blobs.fastRemove();
						current->bli_request = NULL;
					}
					else
					{
						// Blob was created by request, is accounted for internal needs,
						// but is not materialized. Get rid of it.
						BLB_cancel(tdbb, current->bli_blob_object);
						// Since the routine above modifies req_blobs
						// we need to reestablish accessor position
					}

					if (request->req_blobs.locate(Firebird::locGreat, blob_temp_id))
						continue;

					break;
				}

				// Blob accounting inconsistent, only detected in DEV_BUILD.
				fb_assert(false);

				if (!request->req_blobs.getNext())
					break;
			}
		}

		request->req_blobs.clear();

		/* Release arrays assigned by this request */

		for (ArrayField** array = &transaction->tra_arrays; *array;)
		{
			DEV_BLKCHK(*array, type_arr);
			if ((*array)->arr_request == request)
				BLB_release_array(*array);
			else
				array = &(*array)->arr_next;
		}
	}
}


static void release_proc_save_points(jrd_req* request)
{
/**************************************
 *
 *	r e l e a s e _ p r o c _ s a v e _ p o i n t s
 *
 **************************************
 *
 * Functional description
 *	Release temporary blobs assigned by this request.
 *
 **************************************/
	Savepoint* sav_point = request->req_proc_sav_point;

	if (request->req_transaction) {
		while (sav_point) {
			Savepoint* const temp_sav_point = sav_point->sav_next;
			delete sav_point;
			sav_point = temp_sav_point;
		}
	}
	request->req_proc_sav_point = NULL;
}


#ifdef SCROLLABLE_CURSORS
static jrd_nod* seek_rse(thread_db* tdbb, jrd_req* request, jrd_nod* node)
{
/**************************************
 *
 *      s e e k _ r s e
 *
 **************************************
 *
 * Functional description
 *	Execute a nod_seek, which specifies
 *	a direction and offset in which to
 *	scroll a record selection expression.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(node, type_nod);

	if (request->req_operation == jrd_req::req_proceed)
	{
		/* get input arguments */

		const dsc* desc = EVL_expr(tdbb, node->nod_arg[e_seek_direction]);
		const USHORT direction = (desc && !(request->req_flags & req_null)) ?
			MOV_get_long(desc, 0) : MAX_USHORT;

		desc = EVL_expr(tdbb, node->nod_arg[e_seek_offset]);
		const SLONG offset = (desc && !(request->req_flags & req_null)) ? MOV_get_long(desc, 0) : 0;

		RecordSelExpr* rse = (RecordSelExpr*) node->nod_arg[e_seek_rse];

		seek_rsb(tdbb, request, rse->rse_rsb, direction, offset);

		request->req_operation = jrd_req::req_return;
	}

	return node->nod_parent;
}
#endif


#ifdef SCROLLABLE_CURSORS
static void seek_rsb(thread_db* tdbb,
					 jrd_req* request, RecordSource* rsb, USHORT direction, SLONG offset)
{
/**************************************
 *
 *      s e e k _ r s b
 *
 **************************************
 *
 * Functional description
 *	Allow scrolling through a stream as defined
 *	by the input rsb.  Handles multiple seeking.
 *	Uses RSE_get_record() to do the actual work.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(rsb, type_rsb);
	irsb* impure = (IRSB) ((UCHAR *) request + rsb->rsb_impure);

/* look past any boolean to the actual stream */

	if (rsb->rsb_type == rsb_boolean)
	{
		seek_rsb(tdbb, request, rsb->rsb_next, direction, offset);

		/* set the backwards flag */

		const irsb* next_impure = (IRSB) ((UCHAR *) request + rsb->rsb_next->rsb_impure);

		if (next_impure->irsb_flags & irsb_last_backwards)
			impure->irsb_flags |= irsb_last_backwards;
		else
			impure->irsb_flags &= ~irsb_last_backwards;
		return;
	}

/* do simple boundary checking for bof and eof */

	switch (direction)
	{
	case blr_forward:
		if (impure->irsb_flags & irsb_eof)
			ERR_post(Arg::Gds(isc_stream_eof));
		break;

	case blr_backward:
		if (impure->irsb_flags & irsb_bof)
			ERR_post(Arg::Gds(isc_stream_bof));
		break;

	case blr_bof_forward:
	case blr_eof_backward:
		break;

	default:
		// was: BUGCHECK(232);
		// replaced with this error to be consistent with find()
		ERR_post(Arg::Gds(isc_invalid_direction));
	}

/* the actual offset to seek may be one less because the next time
   through the blr_for loop we will seek one record--flag the fact
   that a fetch is required on this stream in case it doesn't happen
   (for example when GPRE generates BLR which does not stall prior to
   the blr_for, as DSQL does) */

	if (offset > 0)
		switch (direction)
		{
		case blr_forward:
		case blr_bof_forward:
			if (!(impure->irsb_flags & irsb_last_backwards)) {
				offset--;
				if (!(impure->irsb_flags & irsb_bof))
					request->req_flags |= req_fetch_required;
			}
			break;

		case blr_backward:
		case blr_eof_backward:
			if (impure->irsb_flags & irsb_last_backwards) {
				offset--;
				if (!(impure->irsb_flags & irsb_eof))
					request->req_flags |= req_fetch_required;
			}
			break;
		}

/* now do the actual seek */

	switch (direction)
	{
	case blr_forward:			/* go forward from the current location */

		/* the rsb_backwards flag is used to indicate the direction to seek in;
		   this is sticky in the sense that after the user has seek'ed in the
		   backward direction, the next retrieval from a blr_for loop will also
		   be in the backward direction--this allows us to continue scrolling
		   without constantly sending messages to the engine */

		impure->irsb_flags &= ~irsb_last_backwards;

		while (offset) {
			offset--;
			if (!RSE_get_record(tdbb, rsb, RSE_get_next))
				break;
		}
		break;

	case blr_backward:			/* go backward from the current location */

		impure->irsb_flags |= irsb_last_backwards;

		while (offset) {
			offset--;
			if (!RSE_get_record(tdbb, rsb, RSE_get_next))
				break;
		}
		break;

	case blr_bof_forward:		/* go forward from the beginning of the stream */

		RSE_close(tdbb, rsb);
		RSE_open(tdbb, rsb);

		impure->irsb_flags &= ~irsb_last_backwards;

		while (offset) {
			offset--;
			if (!RSE_get_record(tdbb, rsb, RSE_get_next))
				break;
		}
		break;

	case blr_eof_backward:		/* go backward from the end of the stream */

		RSE_close(tdbb, rsb);
		RSE_open(tdbb, rsb);

		/* if this is a stream type which uses bof and eof flags,
		   reverse the sense of bof and eof in this case */

		if (impure->irsb_flags & irsb_bof) {
			impure->irsb_flags &= ~irsb_bof;
			impure->irsb_flags |= irsb_eof;
		}

		impure->irsb_flags |= irsb_last_backwards;

		while (offset) {
			offset--;
			if (!RSE_get_record(tdbb, rsb, RSE_get_next))
				break;
		}
		break;

	default:
		// Should never go here, because of the boundary
		// check above, but anyway...
		BUGCHECK(232);
	}
}
#endif


static jrd_nod* selct(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	s e l e c t
 *
 **************************************
 *
 * Functional description
 *	Execute a SELECT statement.  This is more than a little
 *	obscure.  We first set up the SELECT statement as the
 *	"message" and stall on receive (waiting for user send).
 *	EXE_send will then loop thru the sub-statements of select
 *	looking for the appropriate RECEIVE statement.  When (or if)
 *	it finds it, it will set it up the next statement to be
 *	executed.  The RECEIVE, then, will be entered with the
 *	operation "req_proceed."
 *
 **************************************/
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	BLKCHK(node, type_nod);

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		request->req_message = node;
		request->req_operation = jrd_req::req_receive;
		request->req_flags |= req_stall;
		return node;

	default:
		return node->nod_parent;
	}
}



static jrd_nod* send_msg(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	s e n d _ m s g
 *
 **************************************
 *
 * Functional description
 *	Execute a SEND statement.
 *
 **************************************/
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	BLKCHK(node, type_nod);

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		return (node->nod_arg[e_send_statement]);

	case jrd_req::req_return:
		request->req_operation = jrd_req::req_send;
		request->req_message = node->nod_arg[e_send_message];
		request->req_flags |= req_stall;
		return node;

	case jrd_req::req_proceed:
		request->req_operation = jrd_req::req_return;
		return node->nod_parent;

	default:
		return (node->nod_parent);
	}
}


static void set_error(thread_db* tdbb, const xcp_repeat* exception, jrd_nod* msg_node)
{
/**************************************
 *
 *	s e t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Set status vector according to specified error condition
 *	and jump to handle error accordingly.
 *
 **************************************/
	Firebird::MetaName name, relation_name;
	TEXT message[XCP_MESSAGE_LENGTH + 1];
	MoveBuffer temp;

	SET_TDBB(tdbb);

	jrd_req* request = tdbb->getRequest();

	if (!exception) {
		// retrieve the status vector and punt
		request->req_last_xcp.copyTo(tdbb->tdbb_status_vector);
		request->req_last_xcp.clear();
		ERR_punt();
	}

	USHORT length = 0;

	if (msg_node)
	{
		UCHAR* string = NULL;
		// evaluate exception message and convert it to string
		DSC* desc = EVL_expr(tdbb, msg_node);
		if (desc && !(request->req_flags & req_null))
		{
			length = MOV_make_string2(tdbb, desc, tdbb->getAttachment()->att_charset, &string, temp);
			length = MIN(length, sizeof(message) - 1);

			/* dimitr: or should we throw an error here, i.e.
					replace the above assignment with the following lines:

			 if (length > sizeof(message) - 1)
				ERR_post(Arg::Gds(isc_imp_exc) << Arg::Gds(isc_blktoobig));
			*/

			memcpy(message, string, length);
		}
		else
		{
			length = 0;
		}
	}
	message[length] = 0;

	switch (exception->xcp_type)
	{
	case xcp_sql_code:
		ERR_post(Arg::Gds(isc_sqlerr) << Arg::Num(exception->xcp_code));

	case xcp_gds_code:
		if (exception->xcp_code == isc_check_constraint) {
			MET_lookup_cnstrt_for_trigger(tdbb, name, relation_name, request->req_trg_name);
			ERR_post(Arg::Gds(exception->xcp_code) << Arg::Str(name) << Arg::Str(relation_name));
		}
		else
			ERR_post(Arg::Gds(exception->xcp_code));

	case xcp_xcp_code:
		{
			const TEXT* s;
			string tempStr;

			// CVC: If we have the exception name, use it instead of the number.
			// Solves SF Bug #494981.
			MET_lookup_exception(tdbb, exception->xcp_code, name, &tempStr);

			if (message[0])
				s = message;
			else if (tempStr.hasData())
				s = tempStr.c_str();
			else
				s = NULL;

			if (s && name.length())
			{
				ERR_post(Arg::Gds(isc_except) << Arg::Num(exception->xcp_code) <<
						 Arg::Gds(isc_random) << Arg::Str(name) <<
						 Arg::Gds(isc_random) << Arg::Str(s));
			}
			else if (s)
			{
				ERR_post(Arg::Gds(isc_except) << Arg::Num(exception->xcp_code) <<
						 Arg::Gds(isc_random) << Arg::Str(s));
			}
			else if (name.length())
			{
				ERR_post(Arg::Gds(isc_except) << Arg::Num(exception->xcp_code) <<
						 Arg::Gds(isc_random) << Arg::Str(name));
			}
			else
				ERR_post(Arg::Gds(isc_except) << Arg::Num(exception->xcp_code));
		}

	default:
		fb_assert(false);
	}
}


static jrd_nod* stall(thread_db* tdbb, jrd_nod* node)
{
/**************************************
 *
 *	s t a l l
 *
 **************************************
 *
 * Functional description
 *	Execute a stall statement.
 *	This is like a blr_receive, except that there is no
 *	need for a gds__send () from the user (i.e. EXE_send () in the engine).
 *	A gds__receive () will unblock the user.
 *
 **************************************/
	SET_TDBB(tdbb);
	jrd_req* request = tdbb->getRequest();
	BLKCHK(node, type_nod);

	switch (request->req_operation)
	{
	case jrd_req::req_sync:
		return node->nod_parent;

	case jrd_req::req_proceed:
		request->req_operation = jrd_req::req_return;
		return node->nod_parent;

	default:
		request->req_message = node;
		request->req_operation = jrd_req::req_return;
		request->req_flags |= req_stall;
		return node;
	}
}


static jrd_nod* store(thread_db* tdbb, jrd_nod* node, SSHORT which_trig)
{
/**************************************
 *
 *	s t o r e
 *
 **************************************
 *
 * Functional description
 *	Execute a STORE statement.
 *
 **************************************/
	jrd_req* trigger;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	BLKCHK(node, type_nod);

	jrd_req* request = tdbb->getRequest();
	jrd_tra* transaction = request->req_transaction;
	impure_state* impure = (impure_state*) ((SCHAR *) request + node->nod_impure);
	SSHORT stream = (USHORT)(IPTR) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
	record_param* rpb = &request->req_rpb[stream];
	jrd_rel* relation = rpb->rpb_relation;

	switch (request->req_operation)
	{
	case jrd_req::req_evaluate:
		if (node->nod_parent && node->nod_parent->nod_type != nod_for)
			request->req_records_affected.clear();

		request->req_records_affected.bumpModified(false);
		impure->sta_state = 0;
		RLCK_reserve_relation(tdbb, transaction, relation, true);
		break;

	case jrd_req::req_return:
		if (impure->sta_state)
			return node->nod_parent;

		if (transaction != dbb->dbb_sys_trans)
			++transaction->tra_save_point->sav_verb_count;

		if (relation->rel_pre_store && (which_trig != POST_TRIG) &&
			(trigger = execute_triggers(tdbb, &relation->rel_pre_store,
										NULL, rpb, jrd_req::req_trigger_insert, PRE_TRIG)))
		{
			trigger_failure(tdbb, trigger);
		}

		if (node->nod_arg[e_sto_validate]) {
			validate(tdbb, node->nod_arg[e_sto_validate]);
		}

		/* For optimum on-disk record compression, zero all unassigned
		   fields. In addition, zero the tail of assigned varying fields
		   so that previous remnants don't defeat compression efficiency. */

		/* CVC: The code that was here was moved to its own routine: cleanup_rpb()
				and replaced by the call shown below. */

		cleanup_rpb(tdbb, rpb);

		if (relation->rel_file) {
			EXT_store(tdbb, rpb);
		}
		else if (relation->isVirtual()) {
			VirtualTable::store(tdbb, rpb);
		}
		else if (!relation->rel_view_rse)
		{
			USHORT bad_index;
			jrd_rel* bad_relation = 0;

			VIO_store(tdbb, rpb, transaction);
			const idx_e error_code = IDX_store(tdbb, rpb, transaction, &bad_relation, &bad_index);

			if (error_code) {
				ERR_duplicate_error(error_code, bad_relation, bad_index);
			}
		}

		rpb->rpb_number.setValid(true);

		if (relation->rel_post_store && (which_trig != PRE_TRIG) &&
			(trigger = execute_triggers(tdbb, &relation->rel_post_store,
										NULL, rpb, jrd_req::req_trigger_insert, POST_TRIG)))
		{
			trigger_failure(tdbb, trigger);
		}

		/* CVC: Increment the counter only if we called VIO/EXT_store() and
				we were successful. */
		if (!(request->req_view_flags & req_first_store_return)) {
			request->req_view_flags |= req_first_store_return;
			if (relation->rel_view_rse) {
				request->req_top_view_store = relation;
			}
		}
		if (relation == request->req_top_view_store) {
			if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) {
				request->req_records_inserted++;
				request->req_records_affected.bumpModified(true);
			}
		}
		else if (relation->rel_file || !relation->rel_view_rse) {
			request->req_records_inserted++;
			request->req_records_affected.bumpModified(true);
		}

		if (transaction != dbb->dbb_sys_trans) {
			--transaction->tra_save_point->sav_verb_count;
		}

		if (node->nod_arg[e_sto_statement2]) {
			impure->sta_state = 1;
			request->req_operation = jrd_req::req_evaluate;
			return node->nod_arg[e_sto_statement2];
		}

	default:
		return node->nod_parent;
	}

/* Fall thru on evaluate to set up for store before executing sub-statement.
   This involves finding the appropriate format, making sure a record block
   exists for the stream and is big enough, and initialize all null flags
   to "missing." */

	const Format* format = MET_current(tdbb, relation);
	Record* record = VIO_record(tdbb, rpb, format, tdbb->getDefaultPool());

	rpb->rpb_address = record->rec_data;
	rpb->rpb_length = format->fmt_length;
	rpb->rpb_format_number = format->fmt_version;

	/* CVC: This small block added by Ann Harrison to
			start with a clean empty buffer and so avoid getting
			new record buffer with misleading information. Fixes
			bug with incorrect blob sharing during insertion in
			a stored procedure. */

	memset(record->rec_data, 0, rpb->rpb_length);

/* Initialize all fields to missing */

	SSHORT n = (format->fmt_count + 7) >> 3;
	if (n) {
		memset(record->rec_data, 0xff, n);
	}

	return node->nod_arg[e_sto_statement];
}


static bool test_and_fixup_error(thread_db* tdbb, const PsqlException* conditions, jrd_req* request)
{
/**************************************
 *
 *	t e s t _ a n d _ f i x u p _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Test for match of current state with list of error conditions.
 *  Fix type and code of the exception.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (tdbb->tdbb_flags & TDBB_sys_error)
		return false;

	ISC_STATUS* status_vector = tdbb->tdbb_status_vector;
	const SSHORT sqlcode = gds__sqlcode(status_vector);

	bool found = false;

	for (USHORT i = 0; i < conditions->xcp_count; i++)
	{
		switch (conditions->xcp_rpt[i].xcp_type)
		{
		case xcp_sql_code:
			if (sqlcode == conditions->xcp_rpt[i].xcp_code)
			{
				found = true;
			}
			break;

		case xcp_gds_code:
			if (status_vector[1] == conditions->xcp_rpt[i].xcp_code)
			{
				found = true;
			}
			break;

		case xcp_xcp_code:
			{
			// Look at set_error() routine to understand how the
			// exception ID info is encoded inside the status vector.
			if ((status_vector[1] == isc_except) &&
				(status_vector[3] == conditions->xcp_rpt[i].xcp_code))
			{
				found = true;
			}

			}
			break;

		case xcp_default:
			found = true;
			break;

		default:
			fb_assert(false);
		}

		if (found)
		{
			request->req_last_xcp.init(status_vector);
			fb_utils::init_status(status_vector);
			break;
		}
    }

	return found;
}


static void trigger_failure(thread_db* tdbb, jrd_req* trigger)
{
/**************************************
 *
 *	t r i g g e r _ f a i l u r e
 *
 **************************************
 *
 * Functional description
 *	Trigger failed, report error.
 *
 **************************************/

	SET_TDBB(tdbb);
	EXE_unwind(tdbb, trigger);

	trigger->req_attachment = NULL;
	trigger->req_flags &= ~req_in_use;
	trigger->req_timestamp.invalidate();

	if (trigger->req_flags & req_leave)
	{
		trigger->req_flags &= ~req_leave;
		string msg;
		MET_trigger_msg(tdbb, msg, trigger->req_trg_name, trigger->req_label);
		if (msg.hasData())
		{
			if (trigger->req_flags & req_sys_trigger)
			{
				ISC_STATUS code = PAR_symbol_to_gdscode(msg);
				if (code)
				{
					ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label) <<
							 Arg::Gds(code));
				}
			}
			ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label) <<
					 Arg::Gds(isc_random) << Arg::Str(msg));
		}
		else
		{
			ERR_post(Arg::Gds(isc_integ_fail) << Arg::Num(trigger->req_label));
		}
	}
	else
	{
		ERR_punt();
	}
}


static void validate(thread_db* tdbb, jrd_nod* list)
{
/**************************************
 *
 *	v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Execute a list of validation expressions.
 *
 **************************************/

	SET_TDBB(tdbb);
	BLKCHK(list, type_nod);

	jrd_nod** ptr1 = list->nod_arg;
	for (const jrd_nod* const* const end = ptr1 + list->nod_count; ptr1 < end; ptr1++)
	{
		jrd_req* request = tdbb->getRequest();
		if (!EVL_boolean(tdbb, (*ptr1)->nod_arg[e_val_boolean]) && !(request->req_flags & req_null))
		{
			/* Validation error -- report result */
			const char* value;
			VaryStr<128> temp;

			jrd_nod* node = (*ptr1)->nod_arg[e_val_value];
			const dsc* desc = EVL_expr(tdbb, node);
			const USHORT length = (desc && !(request->req_flags & req_null)) ?
				MOV_make_string(desc, ttype_dynamic, &value,
								&temp, sizeof(temp) - 1) : 0;

			if (!desc || (request->req_flags & req_null))
			{
				value = NULL_STRING_MARK;
			}
			else if (!length)
			{
				value = "";
			}
			else
			{
				const_cast<char*>(value)[length] = 0;	// safe cast - data is actually on the stack
			}

			const TEXT*	name = 0;
			if (node->nod_type == nod_field)
			{
				const USHORT stream = (USHORT)(IPTR) node->nod_arg[e_fld_stream];
				const USHORT id = (USHORT)(IPTR) node->nod_arg[e_fld_id];
				const jrd_rel* relation = request->req_rpb[stream].rpb_relation;

				const jrd_fld* field;
				const vec<jrd_fld*>* vector = relation->rel_fields;
				if (vector && id < vector->count() && (field = (*vector)[id]))
				{
					name = field->fld_name.c_str();
				}
			}

			if (!name)
			{
				name = UNKNOWN_STRING_MARK;
			}

			ERR_post(Arg::Gds(isc_not_valid) << Arg::Str(name) << Arg::Str(value));
		}
	}
}


inline void verb_cleanup(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	v e r b _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *  If an error happens during the backout of a savepoint, then the transaction
 *  must be marked 'dead' because that is the only way to clean up after a
 *  failed backout. The easiest way to do this is to kill the application
 *  by calling bugcheck.
 *
 **************************************/
	try {
	    VIO_verb_cleanup(tdbb, transaction);
    }
	catch (const Firebird::Exception&) {
		if (tdbb->getDatabase()->dbb_flags & DBB_bugcheck) {
			Firebird::status_exception::raise(tdbb->tdbb_status_vector);
		}
    	BUGCHECK(290); // msg 290 error during savepoint backout
	}
}
