/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		dsql.cpp
 *	DESCRIPTION:	Local processing for External entry points.
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
 * December 2001 Mike Nordell: Major overhaul to (try to) make it C++
 * 2001.6.3 Claudio Valderrama: fixed a bad behaved loop in get_plan_info()
 * and get_rsb_item() that caused a crash when plan info was requested.
 * 2001.6.9 Claudio Valderrama: Added nod_del_view, nod_current_role and nod_breakleave.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2004.01.16 Vlad Horsun: added support for EXECUTE BLOCK statement
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
#include "../jrd/jrd.h"
#include "../dsql/Parser.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/dsql_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/movd_proto.h"
#include "../dsql/parse_proto.h"
#include "../dsql/pass1_proto.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceDSQLHelpers.h"
#include "../common/classes/init.h"
#include "../common/utils_proto.h"
#ifdef SCROLLABLE_CURSORS
#include "../jrd/scroll_cursors.h"
#endif
#include "../common/StatusArg.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

using namespace Jrd;
using namespace Dsql;
using namespace Firebird;


static void		close_cursor(thread_db*, dsql_req*);
static USHORT	convert(SLONG, UCHAR*);
static void		execute_blob(thread_db*, dsql_req*, USHORT, const UCHAR*, USHORT, const UCHAR*,
						 USHORT, UCHAR*, USHORT, UCHAR*);
static void		execute_immediate(thread_db*, Attachment*, jrd_tra**,
							  USHORT, const TEXT*, USHORT,
							  USHORT, const UCHAR*, /*USHORT,*/ USHORT, const UCHAR*,
							  USHORT, UCHAR*, /*USHORT,*/ USHORT, UCHAR*);
static void		execute_request(thread_db*, dsql_req*, jrd_tra**, USHORT, const UCHAR*,
	USHORT, const UCHAR*, USHORT, UCHAR*, USHORT, UCHAR*, bool);
static SSHORT	filter_sub_type(const dsql_nod*);
static bool		get_indices(SLONG*, const UCHAR**, SLONG*, SCHAR**);
static USHORT	get_request_info(thread_db*, dsql_req*, SLONG, UCHAR*);
static bool		get_rsb_item(SLONG*, const UCHAR**, SLONG*, SCHAR**, USHORT*, USHORT*);
static dsql_dbb*	init(Attachment*);
static void		map_in_out(dsql_req*, dsql_msg*, USHORT, const UCHAR*, USHORT, UCHAR*, const UCHAR* = 0);
static USHORT	parse_blr(USHORT, const UCHAR*, const USHORT, dsql_par*);
static dsql_req*		prepare(thread_db*, dsql_dbb*, jrd_tra*, USHORT, const TEXT*, USHORT, USHORT);
static UCHAR*	put_item(UCHAR, const USHORT, const UCHAR*, UCHAR*, const UCHAR* const, const bool copy = true);
static void		release_request(thread_db*, dsql_req*, bool);
static void		sql_info(thread_db*, dsql_req*, USHORT, const UCHAR*, ULONG, UCHAR*);
static UCHAR*	var_info(dsql_msg*, const UCHAR*, const UCHAR* const, UCHAR*,
	const UCHAR* const, USHORT, bool);

static inline bool reqTypeWithCursor(REQ_TYPE req_type)
{
	switch (req_type)
	{
	case REQ_SELECT:
	case REQ_SELECT_BLOCK:
	case REQ_SELECT_UPD:
	case REQ_EMBED_SELECT:
	case REQ_GET_SEGMENT:
	case REQ_PUT_SEGMENT:
		return true;
	}

	return false;
}

#ifdef DSQL_DEBUG
unsigned DSQL_debug = 0;
#endif

namespace
{
	const UCHAR db_hdr_info_items[] =
	{
		isc_info_db_sql_dialect,
		isc_info_ods_version,
		isc_info_ods_minor_version,
#ifdef SCROLLABLE_CURSORS
		isc_info_base_level,
#endif
		isc_info_db_read_only,
		isc_info_end
	};

	const UCHAR explain_info[] =
	{
		isc_info_access_path
	};

	const UCHAR record_info[] =
	{
		isc_info_req_update_count, isc_info_req_delete_count,
		isc_info_req_select_count, isc_info_req_insert_count
	};

	const UCHAR sql_records_info[] =
	{
		isc_info_sql_records
	};

}	// namespace


#ifdef DSQL_DEBUG
IMPLEMENT_TRACE_ROUTINE(dsql_trace, "DSQL")
#endif

dsql_dbb::~dsql_dbb()
{
	thread_db* tdbb = JRD_get_thread_data();

	while (!dbb_requests.isEmpty())
		release_request(tdbb, dbb_requests[0], true);

	HSHD_finish(this);
}


/**

 	DSQL_allocate_statement

    @brief	Allocate a statement handle.


    @param tdbb
    @param attachment

 **/
dsql_req* DSQL_allocate_statement(thread_db* tdbb, Attachment* attachment)
{
	SET_TDBB(tdbb);

	dsql_dbb* const database = init(attachment);
	Jrd::ContextPoolHolder context(tdbb, database->createPool());

	// allocate the request block

	MemoryPool& pool = *tdbb->getDefaultPool();
	dsql_req* const request = FB_NEW(pool) CompiledStatement(pool);
	request->req_dbb = database;
	database->dbb_requests.add(request);

	return request;
}


/**

 	DSQL_execute

    @brief	Execute a non-SELECT dynamic SQL statement.


    @param tdbb
    @param tra_handle
    @param request
    @param in_blr_length
    @param in_blr
    @param in_msg_type
    @param in_msg_length
    @param in_msg
    @param out_blr_length
    @param out_blr
    @param out_msg_type OBSOLETE
    @param out_msg_length
    @param out_msg

 **/
void DSQL_execute(thread_db* tdbb,
				  jrd_tra** tra_handle,
				  dsql_req* request,
				  USHORT in_blr_length, const UCHAR* in_blr,
				  USHORT in_msg_type, USHORT in_msg_length, const UCHAR* in_msg,
				  USHORT out_blr_length, UCHAR* out_blr,
				  /*USHORT out_msg_type,*/ USHORT out_msg_length, UCHAR* out_msg)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	if (request->req_flags & REQ_orphan)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
		          Arg::Gds(isc_bad_req_handle));
	}

	if ((SSHORT) in_msg_type == -1) {
		request->req_type = REQ_EMBED_SELECT;
	}

	// Only allow NULL trans_handle if we're starting a transaction

	if (!*tra_handle && request->req_type != REQ_START_TRANS)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_bad_trans_handle));
	}

	// If the request is a SELECT or blob statement then this is an open.
	// Make sure the cursor is not already open.

	if (reqTypeWithCursor(request->req_type)) {
		if (request->req_flags & REQ_cursor_open)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
					  Arg::Gds(isc_dsql_cursor_open_err));
		}
	}

	// A select with a non zero output length is a singleton select
	const bool singleton = (reqTypeWithCursor(request->req_type) && out_msg_length != 0);

	if (request->req_type != REQ_EMBED_SELECT)
	{
		execute_request(tdbb, request, tra_handle,
						in_blr_length, in_blr, in_msg_length, in_msg,
						out_blr_length, out_blr, out_msg_length, out_msg,
						singleton);
	}
	else
	{
		request->req_transaction = *tra_handle;
	}

	// If the output message length is zero on a REQ_SELECT then we must
	// be doing an OPEN cursor operation.
	// If we do have an output message length, then we're doing
	// a singleton SELECT.  In that event, we don't add the cursor
	// to the list of open cursors (it's not really open).

	if (reqTypeWithCursor(request->req_type) && !singleton)
	{
		request->req_flags |= REQ_cursor_open;
		TRA_link_cursor(request->req_transaction, request);
	}
}


/**

 	DSQL_execute_immediate

    @brief	Execute a non-SELECT dynamic SQL statement.


    @param tdbb
    @param attachment
    @param tra_handle
    @param length
    @param string
    @param dialect
    @param in_blr_length
    @param in_blr
    @param in_msg_type OBSOLETE
    @param in_msg_length
    @param in_msg
    @param out_blr_length
    @param out_blr
    @param out_msg_type OBSOLETE
    @param out_msg_length
    @param out_msg

 **/
void DSQL_execute_immediate(thread_db* tdbb,
							Attachment* attachment,
							jrd_tra** tra_handle,
							USHORT length, const TEXT* string, USHORT dialect,
							USHORT in_blr_length, const UCHAR* in_blr,
							/*USHORT in_msg_type,*/ USHORT in_msg_length, const UCHAR* in_msg,
							USHORT out_blr_length, UCHAR* out_blr,
							/*USHORT out_msg_type,*/ USHORT out_msg_length, UCHAR* out_msg)
{
	execute_immediate(tdbb, attachment, tra_handle, length,
		string, dialect,
		in_blr_length, in_blr, /*in_msg_type,*/ in_msg_length, in_msg,
		out_blr_length, out_blr, /*out_msg_type,*/ out_msg_length, out_msg);
}


/**

 	DSQL_fetch

    @brief	Fetch next record from a dynamic SQL cursor


    @param user_status
    @param req_handle
    @param blr_length
    @param blr
    @param msg_type OBSOLETE
    @param msg_length
    @param dsql_msg
    @param direction
    @param offset

 **/
ISC_STATUS DSQL_fetch(thread_db* tdbb,
					  dsql_req* request,
					  USHORT blr_length, const UCHAR* blr,
					  /*USHORT msg_type,*/ USHORT msg_length, UCHAR* dsql_msg_buf
#ifdef SCROLLABLE_CURSORS
						  , USHORT direction, SLONG offset
#endif
						  )
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	// if the cursor isn't open, we've got a problem
	if (reqTypeWithCursor(request->req_type))
	{
		if (!(request->req_flags & REQ_cursor_open))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_dsql_cursor_err) <<
					  Arg::Gds(isc_dsql_cursor_not_open));
		}
	}

#ifdef SCROLLABLE_CURSORS

	// check whether we need to send an asynchronous scrolling message
	// to the engine; the engine will automatically advance one record
	// in the same direction as before, so optimize out messages of that
	// type

	if (request->req_type == REQ_SELECT && request->req_dbb->dbb_base_level >= 5)
	{
		switch (direction)
		{
		case isc_fetch_next:
			if (!(request->req_flags & REQ_backwards))
				offset = 0;
			else {
				direction = blr_forward;
				offset = 1;
				request->req_flags &= ~REQ_backwards;
			}
			break;

		case isc_fetch_prior:
			if (request->req_flags & REQ_backwards)
				offset = 0;
			else {
				direction = blr_backward;
				offset = 1;
				request->req_flags |= REQ_backwards;
			}
			break;

		case isc_fetch_first:
			direction = blr_bof_forward;
			offset = 1;
			request->req_flags &= ~REQ_backwards;
			break;

		case isc_fetch_last:
			direction = blr_eof_backward;
			offset = 1;
			request->req_flags |= REQ_backwards;
			break;

		case isc_fetch_absolute:
			direction = blr_bof_forward;
			request->req_flags &= ~REQ_backwards;
			break;

		case isc_fetch_relative:
			if (offset < 0) {
				direction = blr_backward;
				offset = -offset;
				request->req_flags |= REQ_backwards;
			}
			else {
				direction = blr_forward;
				request->req_flags &= ~REQ_backwards;
			}
			break;

		default:
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_sqlda_err));
		}

		if (offset)
		{
			DSC desc;

			dsql_msg* message = (dsql_msg*) request->req_async;

			desc.dsc_dtype = dtype_short;
			desc.dsc_scale = 0;
			desc.dsc_length = sizeof(USHORT);
			desc.dsc_flags = 0;
			desc.dsc_address = (UCHAR*) & direction;

			dsql_par* offset_parameter = message->msg_parameters;
			dsql_par* parameter = offset_parameter->par_next;
			MOVD_move(tdbb, &desc, &parameter->par_desc);

			desc.dsc_dtype = dtype_long;
			desc.dsc_scale = 0;
			desc.dsc_length = sizeof(SLONG);
			desc.dsc_flags = 0;
			desc.dsc_address = (UCHAR*) & offset;

			MOVD_move(tdbb, &desc, &offset_parameter->par_desc);

			DsqlCheckout dcoHolder(request->req_dbb);

			if (isc_receive2(tdbb->tdbb_status_vector, &request->req_request,
							 message->msg_number, message->msg_length,
							 message->msg_buffer, 0, direction, offset))
			{
				Firebird::status_exception::raise(tdbb->tdbb_status_vector);
			}
		}
	}
#endif

	dsql_msg* message = (dsql_msg*) request->req_receive;

	// Set up things for tracing this call
	Attachment* att = request->req_dbb->dbb_attachment;
	TraceDSQLFetch trace(att, request);

	// Insure that the blr for the message is parsed, regardless of
	// whether anything is found by the call to receive.

	if (blr_length) {
		parse_blr(blr_length, blr, msg_length, message->msg_parameters);
	}

	if (request->req_type == REQ_GET_SEGMENT)
	{
		// For get segment, use the user buffer and indicator directly.

		dsql_par* parameter = request->req_blob->blb_segment;
		dsql_par* null = parameter->par_null;
		USHORT* ret_length = (USHORT *) (dsql_msg_buf + (IPTR) null->par_user_desc.dsc_address);
		UCHAR* buffer = dsql_msg_buf + (IPTR) parameter->par_user_desc.dsc_address;

		*ret_length = BLB_get_segment(tdbb, request->req_blob->blb_blob,
			buffer, parameter->par_user_desc.dsc_length);

		if (request->req_blob->blb_blob->blb_flags & BLB_eof)
			return 100;

		if (request->req_blob->blb_blob->blb_fragment_size)
			return 101;

		return 0;
	}

	JRD_receive(tdbb, request->req_request, message->msg_number, message->msg_length,
				message->msg_buffer, 0);

	const dsql_par* const eof = request->req_eof;

	const bool eof_reached = eof && !*((USHORT*) eof->par_desc.dsc_address);

	if (eof_reached)
	{
		trace.fetch(true, res_successful);
		return 100;
	}

	map_in_out(NULL, message, 0, blr, msg_length, dsql_msg_buf);

	trace.fetch(false, res_successful);
	return FB_SUCCESS;
}


/**

 	DSQL_free_statement

    @brief	Release request for a dsql statement


    @param user_status
    @param req_handle
    @param option

 **/
void DSQL_free_statement(thread_db* tdbb, dsql_req* request, USHORT option)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	if (option & DSQL_drop) {
		// Release everything associated with the request
		release_request(tdbb, request, true);
	}
	else if (option & DSQL_unprepare) {
		// Release everything but the request itself
		release_request(tdbb, request, false);
	}
	else if (option & DSQL_close) {
		// Just close the cursor associated with the request
		if (reqTypeWithCursor(request->req_type))
		{
			if (!(request->req_flags & REQ_cursor_open))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-501) <<
						  Arg::Gds(isc_dsql_cursor_close_err));
			}

			close_cursor(tdbb, request);
		}
	}
}


/**

 	DSQL_insert

    @brief	Insert next record into a dynamic SQL cursor


    @param user_status
    @param req_handle
    @param blr_length
    @param blr
    @param msg_type OBSOLETE
    @param msg_length
    @param dsql_msg

 **/
void DSQL_insert(thread_db* tdbb,
				 dsql_req* request,
				 USHORT blr_length, const UCHAR* blr,
				 /*USHORT msg_type,*/ USHORT msg_length, const UCHAR* dsql_msg_buf)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	if (request->req_flags & REQ_orphan)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
		          Arg::Gds(isc_bad_req_handle));
	}

	// if the cursor isn't open, we've got a problem

	if (request->req_type == REQ_PUT_SEGMENT)
	{
		if (!(request->req_flags & REQ_cursor_open))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_dsql_cursor_err) <<
					  Arg::Gds(isc_dsql_cursor_not_open));
		}
	}

	dsql_msg* message = (dsql_msg*) request->req_receive;

	// Insure that the blr for the message is parsed, regardless of
	// whether anything is found by the call to receive.

	if (blr_length)
		parse_blr(blr_length, blr, msg_length, message->msg_parameters);

	if (request->req_type == REQ_PUT_SEGMENT) {
		// For put segment, use the user buffer and indicator directly.

		dsql_par* parameter = request->req_blob->blb_segment;
		const UCHAR* buffer = dsql_msg_buf + (IPTR) parameter->par_user_desc.dsc_address;

		BLB_put_segment(tdbb, request->req_blob->blb_blob, buffer,
			parameter->par_user_desc.dsc_length);
	}
}


/**

 	DSQL_prepare

    @brief	Prepare a statement for execution.


    @param user_status
    @param trans_handle
    @param req_handle
    @param length
    @param string
    @param dialect
    @param item_length
    @param items
    @param buffer_length
    @param buffer

 **/
void DSQL_prepare(thread_db* tdbb,
				  jrd_tra* transaction,
				  dsql_req** req_handle,
				  USHORT length, const TEXT* string, USHORT dialect,
				  USHORT item_length, const UCHAR* items,
				  USHORT buffer_length, UCHAR* buffer)
{
	SET_TDBB(tdbb);

	dsql_req* const old_request = *req_handle;

	if (!old_request) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
		          Arg::Gds(isc_bad_req_handle));
	}

	dsql_dbb* database = old_request->req_dbb;
	if (!database) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
                  Arg::Gds(isc_bad_req_handle));
	}

	// check to see if old request has an open cursor

	if (old_request && (old_request->req_flags & REQ_cursor_open)) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-519) <<
				  Arg::Gds(isc_dsql_open_cursor_request));
	}

	dsql_req* request = NULL;

	try {

		// Figure out which parser version to use
		// Since the API to dsql8_prepare is public and can not be changed, there needs to
		// be a way to send the parser version to DSQL so that the parser can compare the keyword
		// version to the parser version.  To accomplish this, the parser version is combined with
		// the client dialect and sent across that way.  In dsql8_prepare_statement, the parser version
		// and client dialect are separated and passed on to their final destinations.  The information
		// is combined as follows:
		//     Dialect * 10 + parser_version
		//
		// and is extracted in dsql8_prepare_statement as follows:
		//      parser_version = ((dialect *10)+parser_version)%10
		//      client_dialect = ((dialect *10)+parser_version)/10
		//
		// For example, parser_version = 1 and client dialect = 1
		//
		//  combined = (1 * 10) + 1 == 11
		//
		//  parser = (combined) %10 == 1
		//  dialect = (combined) / 19 == 1
		//
		// If the parser version is not part of the dialect, then assume that the
		// connection being made is a local classic connection.

		USHORT parser_version;
		if ((dialect / 10) == 0)
			parser_version = 2;
		else {
			parser_version = dialect % 10;
			dialect /= 10;
		}

		// Allocate a new request block and then prepare the request.  We want to
		// keep the old request around, as is, until we know that we are able
		// to prepare the new one.
		// It would be really *nice* to know *why* we want to
		// keep the old request around -- 1994-October-27 David Schnepper
		// Because that's the client's allocated statement handle and we
		// don't want to trash the context in it -- 2001-Oct-27 Ann Harrison

		request = prepare(tdbb, database, transaction, length, string, dialect, parser_version);

		// Can not prepare a CREATE DATABASE/SCHEMA statement

		if (request->req_type == REQ_CREATE_DB)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-530) <<
					  Arg::Gds(isc_dsql_crdb_prepare_err));
		}

		request->req_flags |= REQ_prepared;

		// Now that we know that the new request exists, zap the old one.

		{
			Jrd::ContextPoolHolder context(tdbb, &old_request->req_pool);
			release_request(tdbb, old_request, true);
		}

		*req_handle = request;

		Jrd::ContextPoolHolder context(tdbb, &request->req_pool);
		sql_info(tdbb, request, item_length, items, buffer_length, buffer);
	}
	catch (const Firebird::Exception&)
	{
		if (request)
		{
			Jrd::ContextPoolHolder context(tdbb, &request->req_pool);
			release_request(tdbb, request, true);
		}
		throw;
	}
}


/**

 	DSQL_set_cursor_name

    @brief	Set a cursor name for a dynamic request


    @param user_status
    @param req_handle
    @param input_cursor
    @param type OBSOLETE

 **/
void DSQL_set_cursor(thread_db* tdbb,
				     dsql_req* request,
					 const TEXT* input_cursor)
					 //USHORT type)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	const size_t MAX_CURSOR_LENGTH = 132 - 1;
	Firebird::string cursor = input_cursor;

	if (cursor[0] == '\"')
	{
		// Quoted cursor names eh? Strip'em.
		// Note that "" will be replaced with ".
		// The code is very strange, because it doesn't check for "" really
		// and thus deletes one isolated " in the middle of the cursor.
		for (Firebird::string::iterator i = cursor.begin(); i < cursor.end(); ++i)
		{
			if (*i == '\"') {
				cursor.erase(i);
			}
		}
	}
	else	// not quoted name
	{
		const Firebird::string::size_type i = cursor.find(' ');
		if (i != Firebird::string::npos)
		{
			cursor.resize(i);
		}
		cursor.upper();
	}
	USHORT length = (USHORT) fb_utils::name_length(cursor.c_str());

	if (length == 0) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_invalid));
	}
	if (length > MAX_CURSOR_LENGTH) {
		length = MAX_CURSOR_LENGTH;
	}
	cursor.resize(length);

	// If there already is a different cursor by the same name, bitch

	const dsql_sym* symbol = HSHD_lookup(request->req_dbb, cursor.c_str(), length, SYM_cursor, 0);
	if (symbol)
	{
		if (request->req_cursor == symbol)
			return;

		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_redefined) << Arg::Str(symbol->sym_string));
	}

	// If there already is a cursor and its name isn't the same, ditto.
	// We already know there is no cursor by this name in the hash table

	if (!request->req_cursor) {
		request->req_cursor = MAKE_symbol(request->req_dbb, cursor.c_str(), length, SYM_cursor, request);
	}
	else {
		fb_assert(request->req_cursor != symbol);
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_redefined) << Arg::Str(request->req_cursor->sym_string));
	}
}


/**

 	DSQL_sql_info

    @brief	Provide information on dsql statement


    @param user_status
    @param req_handle
    @param item_length
    @param items
    @param info_length
    @param info

 **/
void DSQL_sql_info(thread_db* tdbb,
				   dsql_req* request,
				   USHORT item_length, const UCHAR* items,
				   ULONG info_length, UCHAR* info)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

	sql_info(tdbb, request, item_length, items, info_length, info);
}


/**

 	close_cursor

    @brief	Close an open cursor.


    @param request
    @param tdbb

 **/
static void close_cursor(thread_db* tdbb, dsql_req* request)
{
	SET_TDBB(tdbb);

	Attachment* attachment = request->req_dbb->dbb_attachment;
	if (request->req_request)
	{
		ThreadStatusGuard status_vector(tdbb);
		try
		{
			if (request->req_type == REQ_GET_SEGMENT || request->req_type == REQ_PUT_SEGMENT)
			{
				BLB_close(tdbb, request->req_blob->blb_blob);
				request->req_blob->blb_blob = NULL;
			}
			else
			{
				// Report some remaining fetches if any
				if (request->req_fetch_baseline)
				{
					TraceDSQLFetch trace(attachment, request);
					trace.fetch(true, res_successful);
				}

				if (request->req_traced && TraceManager::need_dsql_free(attachment))
				{
					TraceSQLStatementImpl stmt(request, NULL);

					TraceManager::event_dsql_free(attachment, &stmt, DSQL_close);
				}

				JRD_unwind_request(tdbb, request->req_request, 0);
			}
		}
		catch (Firebird::Exception&)
		{
		}
	}

	request->req_flags &= ~REQ_cursor_open;
	TRA_unlink_cursor(request->req_transaction, request);
}


/**

 	convert

    @brief	Convert a number to VAX form -- least significant bytes first.
 	Return the length.


    @param number
    @param buffer

 **/

// CVC: This routine should disappear in favor of a centralized function.
static USHORT convert( SLONG number, UCHAR* buffer)
{
	const UCHAR* p;

#ifndef WORDS_BIGENDIAN
	p = (UCHAR*) &number;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;

#else

	p = (UCHAR*) (&number + 1);
	*buffer++ = *--p;
	*buffer++ = *--p;
	*buffer++ = *--p;
	*buffer++ = *--p;

#endif

	return 4;
}


/**

 	execute_blob

    @brief	Open or create a blob.


	@param tdbb
    @param request
    @param in_blr_length
    @param in_blr
    @param in_msg_length
    @param in_msg
    @param out_blr_length
    @param out_blr
    @param out_msg_length
    @param out_msg

 **/
static void execute_blob(thread_db* tdbb,
						 dsql_req* request,
						 USHORT in_blr_length,
						 const UCHAR* in_blr,
						 USHORT in_msg_length,
						 const UCHAR* in_msg,
						 USHORT out_blr_length,
						 UCHAR* out_blr,
						 USHORT out_msg_length,
						 UCHAR* out_msg)
{
	UCHAR bpb[24];

	dsql_blb* blob = request->req_blob;
	map_in_out(request, blob->blb_open_in_msg, in_blr_length, in_blr, in_msg_length, NULL, in_msg);

	UCHAR* p = bpb;
	*p++ = isc_bpb_version1;
	SSHORT filter = filter_sub_type(blob->blb_to);
	if (filter) {
		*p++ = isc_bpb_target_type;
		*p++ = 2;
		*p++ = static_cast<UCHAR>(filter);
		*p++ = filter >> 8;
	}
	filter = filter_sub_type(blob->blb_from);
	if (filter) {
		*p++ = isc_bpb_source_type;
		*p++ = 2;
		*p++ = static_cast<UCHAR>(filter);
		*p++ = filter >> 8;
	}
	USHORT bpb_length = p - bpb;
	if (bpb_length == 1) {
		bpb_length = 0;
	}

	dsql_par* parameter = blob->blb_blob_id;
	const dsql_par* null = parameter->par_null;

	if (request->req_type == REQ_GET_SEGMENT)
	{
		bid* blob_id = (bid*) parameter->par_desc.dsc_address;
		if (null && *((SSHORT *) null->par_desc.dsc_address) < 0) {
			memset(blob_id, 0, sizeof(bid));
		}

		request->req_blob->blb_blob =
			BLB_open2(tdbb, request->req_transaction, blob_id, bpb_length, bpb, true);
	}
	else
	{
		request->req_request = NULL;
		bid* blob_id = (bid*) parameter->par_desc.dsc_address;
		memset(blob_id, 0, sizeof(bid));

		request->req_blob->blb_blob =
			BLB_create2(tdbb, request->req_transaction, blob_id, bpb_length, bpb);

		map_in_out(NULL, blob->blb_open_out_msg, out_blr_length, out_blr, out_msg_length, out_msg);
	}
}


/**

 	execute_immediate

    @brief	Common part of prepare and execute a statement.


    @param tdbb
    @param attachment
    @param tra_handle
	@param length
	@param string
	@param dialect
    @param in_blr_length
    @param in_blr
    @param in_msg_type OBSOLETE
    @param in_msg_length
    @param in_msg
    @param out_blr_length
    @param out_blr
    @param out_msg_type OBSOLETE
    @param out_msg_length
    @param out_msg

 **/
static void execute_immediate(thread_db* tdbb,
							  Attachment* attachment,
							  jrd_tra** tra_handle,
							  USHORT length, const TEXT* string, USHORT dialect,
							  USHORT in_blr_length, const UCHAR* in_blr,
							  /*USHORT in_msg_type,*/ USHORT in_msg_length, const UCHAR* in_msg,
							  USHORT out_blr_length, UCHAR* out_blr,
							  /*USHORT out_msg_type,*/ USHORT out_msg_length, UCHAR* out_msg)
{
	SET_TDBB(tdbb);

	dsql_dbb* const database = init(attachment);
	dsql_req* request = NULL;

	try {

		// Figure out which parser version to use
		// Since the API to dsql8_execute_immediate is public and can not be changed, there needs to
		// be a way to send the parser version to DSQL so that the parser can compare the keyword
		// version to the parser version.  To accomplish this, the parser version is combined with
		// the client dialect and sent across that way.  In dsql8_execute_immediate, the parser version
		// and client dialect are separated and passed on to their final destinations.  The information
		// is combined as follows:
		//     Dialect * 10 + parser_version
		//
		// and is extracted in dsql8_execute_immediate as follows:
		//      parser_version = ((dialect *10)+parser_version)%10
		//      client_dialect = ((dialect *10)+parser_version)/10
		//
		// For example, parser_version = 1 and client dialect = 1
		//
		//  combined = (1 * 10) + 1 == 11
		//
		//  parser = (combined) %10 == 1
		//  dialect = (combined) / 19 == 1
		//
		// If the parser version is not part of the dialect, then assume that the
		// connection being made is a local classic connection.

		USHORT parser_version;
		if ((dialect / 10) == 0)
			parser_version = 2;
		else {
			parser_version = dialect % 10;
			dialect /= 10;
		}

		request = prepare(tdbb, database, *tra_handle, length, string, dialect, parser_version);

		// Only allow NULL trans_handle if we're starting a transaction

		if (!*tra_handle && request->req_type != REQ_START_TRANS)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_bad_trans_handle));
		}

		Jrd::ContextPoolHolder context(tdbb, &request->req_pool);

		// A select with a non zero output length is a singleton select
		const bool singleton = (reqTypeWithCursor(request->req_type) && out_msg_length != 0);

		execute_request(tdbb, request, tra_handle,
						in_blr_length, in_blr, in_msg_length, in_msg,
						out_blr_length, out_blr, out_msg_length, out_msg,
						singleton);

		release_request(tdbb, request, true);
	}
	catch (const Firebird::Exception&)
	{
		if (request)
		{
			Jrd::ContextPoolHolder context(tdbb, &request->req_pool);
			release_request(tdbb, request, true);
		}
		throw;
	}
}


/**

 	execute_request

    @brief	Execute a dynamic SQL statement.


	@param tdbb
    @param request
    @param trans_handle
    @param in_blr_length
    @param in_blr
    @param in_msg_length
    @param in_msg
    @param out_blr_length
    @param out_blr
    @param out_msg_length
    @param out_msg
    @param singleton

 **/
static void execute_request(thread_db* tdbb,
							dsql_req* request,
							jrd_tra** tra_handle,
							USHORT in_blr_length, const UCHAR* in_blr,
							USHORT in_msg_length, const UCHAR* in_msg,
							USHORT out_blr_length, UCHAR* out_blr,
							USHORT out_msg_length, UCHAR* out_msg,
							bool singleton)
{
	request->req_transaction = *tra_handle;

	switch (request->req_type)
	{
	case REQ_START_TRANS:
		JRD_start_transaction(tdbb, &request->req_transaction, 1, &request->req_dbb->dbb_attachment,
							  request->req_blr_data.getCount(), request->req_blr_data.begin());
		*tra_handle = request->req_transaction;
		return;

	case REQ_COMMIT:
		JRD_commit_transaction(tdbb, &request->req_transaction);
		*tra_handle = NULL;
		return;

	case REQ_COMMIT_RETAIN:
		JRD_commit_retaining(tdbb, &request->req_transaction);
		return;

	case REQ_ROLLBACK:
		JRD_rollback_transaction(tdbb, &request->req_transaction);
		*tra_handle = NULL;
		return;

	case REQ_ROLLBACK_RETAIN:
		JRD_rollback_retaining(tdbb, &request->req_transaction);
		return;

	case REQ_CREATE_DB:
	case REQ_DDL:
	{
		TraceDSQLExecute trace(request->req_dbb->dbb_attachment, request);
		DDL_execute(request);
		trace.finish(false, res_successful);
		return;
	}

	case REQ_GET_SEGMENT:
		execute_blob(tdbb, request,
					 in_blr_length, in_blr, in_msg_length, in_msg,
					 out_blr_length, out_blr, out_msg_length, out_msg);
		return;

	case REQ_PUT_SEGMENT:
		execute_blob(tdbb, request,
					 in_blr_length, in_blr, in_msg_length, in_msg,
					 out_blr_length, out_blr, out_msg_length, out_msg);
		return;

	case REQ_SELECT:
	case REQ_SELECT_UPD:
	case REQ_EMBED_SELECT:
	case REQ_INSERT:
	case REQ_UPDATE:
	case REQ_UPDATE_CURSOR:
	case REQ_DELETE:
	case REQ_DELETE_CURSOR:
	case REQ_EXEC_PROCEDURE:
	case REQ_SET_GENERATOR:
	case REQ_SAVEPOINT:
	case REQ_EXEC_BLOCK:
	case REQ_SELECT_BLOCK:
		break;

	default:
		// Catch invalid request types
		fb_assert(false);
	}

	// If there is no data required, just start the request

	dsql_msg* message = request->req_send;
	if (message)
		map_in_out(request, message, in_blr_length, in_blr, in_msg_length, NULL, in_msg);

	// we need to map_in_out before tracing of execution start to let trace
	// manager know statement parameters values
	TraceDSQLExecute trace(request->req_dbb->dbb_attachment, request);

	if (!message)
		JRD_start(tdbb, request->req_request, request->req_transaction, 0);
	else
	{
		JRD_start_and_send(tdbb, request->req_request, request->req_transaction, message->msg_number,
						   message->msg_length, message->msg_buffer,
						   0);
	}

	// Selectable execute block should get the "proc fetch" flag assigned,
	// which ensures that the savepoint stack is preserved while suspending
	if (request->req_type == REQ_SELECT_BLOCK)
	{
		fb_assert(request->req_request);
		request->req_request->req_flags |= req_proc_fetch;
	}

	// REQ_EXEC_BLOCK has no outputs so there are no out_msg
	// supplied from client side, but REQ_EXEC_BLOCK requires
	// 2-byte message for EOS synchronization
	const bool isBlock = (request->req_type == REQ_EXEC_BLOCK);

	message = request->req_receive;
	if ((out_msg_length && message) || isBlock)
	{
		char temp_buffer[FB_DOUBLE_ALIGN * 2];
		dsql_msg temp_msg;

		// Insure that the blr for the message is parsed, regardless of
		// whether anything is found by the call to receive.

		if (out_msg_length && out_blr_length) {
			parse_blr(out_blr_length, out_blr, out_msg_length, message->msg_parameters);
		}
		else if (!out_msg_length && isBlock) {
			message = &temp_msg;
			message->msg_number = 1;
			message->msg_length = 2;
			message->msg_buffer = (UCHAR*) FB_ALIGN((U_IPTR) temp_buffer, FB_DOUBLE_ALIGN);
		}

		JRD_receive(tdbb, request->req_request, message->msg_number, message->msg_length,
			message->msg_buffer, 0);

		if (out_msg_length)
			map_in_out(NULL, message, 0, out_blr, out_msg_length, out_msg);

		// if this is a singleton select, make sure there's in fact one record

		if (singleton)
		{
			USHORT counter;

			// Create a temp message buffer and try two more receives.
			// If both succeed then the first is the next record and the
			// second is either another record or the end of record message.
			// In either case, there's more than one record.

			UCHAR* message_buffer = (UCHAR*) gds__alloc((ULONG) message->msg_length);

			ISC_STATUS status = FB_SUCCESS;
			ISC_STATUS_ARRAY localStatus;

			for (counter = 0; counter < 2 && !status; counter++)
			{
				AutoSetRestore<ISC_STATUS*> autoStatus(&tdbb->tdbb_status_vector, localStatus);
				fb_utils::init_status(localStatus);

				try
				{
					JRD_receive(tdbb, request->req_request, message->msg_number,
						message->msg_length, message_buffer, 0);
					status = FB_SUCCESS;
				}
				catch (Firebird::Exception&)
				{
					status = tdbb->tdbb_status_vector[1];
				}
			}

			gds__free(message_buffer);

			// two successful receives means more than one record
			// a req_sync error on the first pass above means no records
			// a non-req_sync error on any of the passes above is an error

			if (!status)
				status_exception::raise(Arg::Gds(isc_sing_select_err));
			else if (status == isc_req_sync && counter == 1)
				status_exception::raise(Arg::Gds(isc_stream_eof));
			else if (status != isc_req_sync)
				status_exception::raise(localStatus);
		}
	}

	UCHAR buffer[20]; // Not used after retrieved
	if (request->req_type == REQ_UPDATE_CURSOR)
	{
		sql_info(tdbb, request, sizeof(sql_records_info), sql_records_info, sizeof(buffer), buffer);

		if (!request->req_updates)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-913) <<
					  Arg::Gds(isc_deadlock) <<
					  Arg::Gds(isc_update_conflict));
		}
	}
	else if (request->req_type == REQ_DELETE_CURSOR)
	{
		sql_info(tdbb, request, sizeof(sql_records_info), sql_records_info, sizeof(buffer), buffer);

		if (!request->req_deletes)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-913) <<
					  Arg::Gds(isc_deadlock) <<
					  Arg::Gds(isc_update_conflict));
		}
	}

	const bool have_cursor = reqTypeWithCursor(request->req_type) && !singleton;
	trace.finish(have_cursor, res_successful);
}

/**

 	filter_sub_type

    @brief	Determine the sub_type to use in filtering
 	a blob.


    @param node

 **/
static SSHORT filter_sub_type(const dsql_nod* node)
{
	if (node->nod_type == nod_constant)
		return (SSHORT) node->getSlong();

	const dsql_par* parameter = (dsql_par*) node->nod_arg[e_par_parameter];
	const dsql_par* null = parameter->par_null;
	if (null)
	{
		if (*((SSHORT *) null->par_desc.dsc_address))
			return 0;
	}

	return *((SSHORT *) parameter->par_desc.dsc_address);
}


/**

 	get_indices

    @brief	Retrieve the indices from the index tree in
 	the request info buffer (explain_ptr), and print them out
 	in the plan buffer. Return true on success and false on failure.


    @param explain_length_ptr
    @param explain_ptr
    @param plan_length_ptr
    @param plan_ptr

 **/
static bool get_indices(SLONG* explain_length_ptr, const UCHAR** explain_ptr,
						SLONG* plan_length_ptr, SCHAR** plan_ptr)
{
	USHORT length;

	SLONG explain_length = *explain_length_ptr;
	const UCHAR* explain = *explain_ptr;
	SLONG& plan_length = *plan_length_ptr;
	SCHAR*& plan = *plan_ptr;

	// go through the index tree information, just
	// extracting the indices used

	explain_length--;
	switch (*explain++)
	{
	case isc_info_rsb_and:
	case isc_info_rsb_or:
		if (!get_indices(&explain_length, &explain, &plan_length, &plan))
			return false;
		if (!get_indices(&explain_length, &explain, &plan_length, &plan))
			return false;
		break;

	case isc_info_rsb_dbkey:
		break;

	case isc_info_rsb_index:
		explain_length--;
		length = *explain++;

		// if this isn't the first index, put out a comma

		if (plan[-1] != '(' && plan[-1] != ' ') {
			plan_length -= 2;
			if (plan_length < 0)
				return false;
			*plan++ = ',';
			*plan++ = ' ';
		}

		// now put out the index name

		if ((plan_length -= length) < 0)
			return false;
		explain_length -= length;
		while (length--)
			*plan++ = *explain++;
		break;

	default:
		return false;
	}

	*explain_length_ptr = explain_length;
	*explain_ptr = explain;
	//*plan_length_ptr = plan_length;
	//*plan_ptr = plan;

	return true;
}


/**

 	DSQL_get_plan_info

    @brief	Get the access plan for the request and turn
 	it into a textual representation suitable for
 	human reading.


    @param request
    @param buffer_length
    @param out_buffer
	@param realloc

 **/
ULONG DSQL_get_plan_info(thread_db* tdbb,
							const dsql_req* request,
							SLONG buffer_length,
							SCHAR** out_buffer,
							const bool realloc)
{
	if (!request->req_request)	// DDL
		return 0;

	Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> explain_buffer;
	explain_buffer.resize(BUFFER_LARGE);

	// get the access path info for the underlying request from the engine

	try
	{
		JRD_request_info(tdbb, request->req_request, 0,
						 sizeof(explain_info), explain_info,
						 explain_buffer.getCount(), explain_buffer.begin());

		if (explain_buffer[0] == isc_info_truncated)
		{
			explain_buffer.resize(MAX_USHORT);

			JRD_request_info(tdbb, request->req_request, 0,
							 sizeof(explain_info), explain_info,
							 explain_buffer.getCount(), explain_buffer.begin());

			if (explain_buffer[0] == isc_info_truncated)
			{
				return 0;
			}
		}
	}
	catch (Firebird::Exception&)
	{
		return 0;
	}

	SCHAR* buffer_ptr = *out_buffer;
	SCHAR* plan;
	for (int i = 0; i < 2; i++)
	{
		const UCHAR* explain = explain_buffer.begin();

		if (*explain++ != isc_info_access_path)
		{
			return 0;
		}

		SLONG explain_length = (ULONG) *explain++;
		explain_length += (ULONG) (*explain++) << 8;

		plan = buffer_ptr;

		// CVC: What if we need to do 2nd pass? Those variables were only initialized
		// at the begining of the function hence they had trash the second time.
		USHORT join_count = 0, level = 0;

		const ULONG full_len = buffer_length;
		memset(plan, 0, full_len);
		// This is testing code for the limit case,
		// please do not enable for normal operations.
		/*
		if (full_len == ULONG(MAX_USHORT) - 4)
		{
			const size_t test_offset = 55000;
			memset(plan, '.', test_offset);
			plan += test_offset;
			buffer_length -= test_offset;
		}
		*/

		// keep going until we reach the end of the explain info
		while (explain_length > 0 && buffer_length > 0)
		{
			if (!get_rsb_item(&explain_length, &explain, &buffer_length, &plan, &join_count, &level))
			{
				// don't allocate buffer of the same length second time
				// and let user know plan is incomplete

				if (buffer_ptr != *out_buffer ||
					(!realloc && full_len == ULONG(MAX_USHORT) - 4))
				{
					const ptrdiff_t diff = buffer_ptr + full_len - plan;
					if (diff < 3) {
						plan -= 3 - diff;
					}
					fb_assert(plan > buffer_ptr);
					*plan++ = '.';
					*plan++ = '.';
					*plan++ = '.';
					if (!realloc)
						return plan - buffer_ptr;

					++i;
					break;
				}

				if (!realloc)
					return full_len - buffer_length;

				// assume we have run out of room in the buffer, try again with a larger one
				const size_t new_length = MAX_USHORT;
				char* const temp = static_cast<char*>(gds__alloc(new_length));
				if (!temp) {
					// NOMEM. Do not attempt one more try
					i++;
					break;
				}

				buffer_ptr = temp;
				buffer_length = (SLONG) new_length;
				break;
			}
		}

		if (buffer_ptr == *out_buffer)
			break;
	}

	*out_buffer = buffer_ptr;
	return plan - *out_buffer;
}


/**

 	get_request_info

    @brief	Get the records updated/deleted for record


    @param request
    @param buffer_length
    @param buffer

 **/
static USHORT get_request_info(thread_db* tdbb,
							   dsql_req* request,
							   SLONG buffer_length,
							   UCHAR* buffer)
{
	if (!request->req_request)	// DDL
		return 0;

	// get the info for the request from the engine

	try
	{
		JRD_request_info(tdbb, request->req_request, 0,
						 sizeof(record_info), record_info,
						 buffer_length, buffer);
	}
	catch (Firebird::Exception&)
	{
		return 0;
	}

	const UCHAR* data = buffer;

	request->req_updates = request->req_deletes = 0;
	request->req_selects = request->req_inserts = 0;

	UCHAR p;
	while ((p = *data++) != isc_info_end)
	{
		const USHORT data_length = static_cast<USHORT>(gds__vax_integer(data, 2));
		data += 2;

		switch (p)
		{
		case isc_info_req_update_count:
			request->req_updates = gds__vax_integer(data, data_length);
			break;

		case isc_info_req_delete_count:
			request->req_deletes = gds__vax_integer(data, data_length);
			break;

		case isc_info_req_select_count:
			request->req_selects = gds__vax_integer(data, data_length);
			break;

		case isc_info_req_insert_count:
			request->req_inserts = gds__vax_integer(data, data_length);
			break;

		default:
			break;
		}

		data += data_length;
	}

	return data - buffer;
}


/**

 	get_rsb_item

    @brief	Use recursion to print out a reverse-polish
 	access plan of joins and join types. Return true on success
 	and false on failure


    @param explain_length_ptr
    @param explain_ptr
    @param plan_length_ptr
    @param plan_ptr
    @param parent_join_count
    @param level_ptr

 **/
static bool get_rsb_item(SLONG*		explain_length_ptr,
							const UCHAR**	explain_ptr,
							SLONG*		plan_length_ptr,
							SCHAR**		plan_ptr,
							USHORT*		parent_join_count,
							USHORT*		level_ptr)
{
	const SCHAR* p;
	SSHORT rsb_type;

	SLONG explain_length = *explain_length_ptr;
	const UCHAR* explain = *explain_ptr;
	SLONG& plan_length = *plan_length_ptr;
	SCHAR*& plan = *plan_ptr;

	explain_length--;
	switch (*explain++)
	{
	case isc_info_rsb_begin:
		if (!*level_ptr)
		{
			// put out the PLAN prefix

			p = "\nPLAN ";
			if ((plan_length -= strlen(p)) < 0)
				return false;
			while (*p)
				*plan++ = *p++;
		}

		(*level_ptr)++;
		break;

	case isc_info_rsb_end:
		if (*level_ptr) {
			(*level_ptr)--;
		}
		// else --*parent_join_count; ???
		break;

	case isc_info_rsb_relation:

		// for the single relation case, initiate
		// the relation with a parenthesis

		if (!*parent_join_count) {
			if (--plan_length < 0)
				return false;
			*plan++ = '(';
		}

		// if this isn't the first relation, put out a comma

		if (plan[-1] != '(') {
			plan_length -= 2;
			if (plan_length < 0)
				return false;
			*plan++ = ',';
			*plan++ = ' ';
		}

		// put out the relation name
		{ // scope to keep length local.
			explain_length--;
			SSHORT length = (USHORT) *explain++;
			explain_length -= length;
			if ((plan_length -= length) < 0)
				return false;
			while (length--)
				*plan++ = *explain++;
		} // scope
		break;

	case isc_info_rsb_type:
		explain_length--;
		// for stream types which have multiple substreams, print out
		// the stream type and recursively print out the substreams so
		// we will know where to put the parentheses
		switch (rsb_type = *explain++)
		{
		case isc_info_rsb_union:
		case isc_info_rsb_recursive:

			// put out all the substreams of the join
			{ // scope to have union_count, union_level and union_join_count local.
				explain_length--;
				fb_assert(*explain > 0U);
				USHORT union_count = (USHORT) *explain++ - 1;

				// finish the first union member

				USHORT union_level = *level_ptr;
				USHORT union_join_count = 0;
				while (explain_length > 0 && plan_length > 0)
				{
					if (!get_rsb_item(&explain_length, &explain, &plan_length, &plan,
									  &union_join_count, &union_level))
					{
						return false;
					}
					if (union_level == *level_ptr)
						break;
				}

				// for the rest of the members, start the level at 0 so each
				// gets its own "PLAN ... " line

				while (union_count)
				{
					union_join_count = 0;
					union_level = 0;
					while (explain_length > 0 && plan_length > 0)
					{
						if (!get_rsb_item(&explain_length, &explain, &plan_length,
										  &plan, &union_join_count, &union_level))
						{
							return false;
						}
						if (!union_level)
							break;
					}
					union_count--;
				}
			} // scope
			break;

		case isc_info_rsb_cross:
		case isc_info_rsb_left_cross:
		case isc_info_rsb_merge:

			// if this join is itself part of a join list,
			// but not the first item, then put out a comma

			if (*parent_join_count && plan[-1] != '(')
			{
				plan_length -= 2;
				if (plan_length < 0)
					return false;
				*plan++ = ',';
				*plan++ = ' ';
			}

			// put out the join type

			if (rsb_type == isc_info_rsb_cross || rsb_type == isc_info_rsb_left_cross)
			{
				p = "JOIN (";
			}
			else {
				p = "MERGE (";
			}

			if ((plan_length -= strlen(p)) < 0)
				return false;
			while (*p)
				*plan++ = *p++;

			// put out all the substreams of the join

			explain_length--;
			{ // scope to have join_count local.
				USHORT join_count = (USHORT) *explain++;
				while (join_count && explain_length > 0 && plan_length > 0)
				{
					if (!get_rsb_item(&explain_length, &explain, &plan_length,
									  &plan, &join_count, level_ptr))
					{
						return false;
					}
					// CVC: Here's the additional stop condition.
					if (!*level_ptr) {
						break;
					}
				}
			} // scope

			// put out the final parenthesis for the join

			if (--plan_length < 0)
				return false;
			*plan++ = ')';

			// this qualifies as a stream, so decrement the join count

			if (*parent_join_count)
				-- * parent_join_count;
			break;

		case isc_info_rsb_indexed:
		case isc_info_rsb_navigate:
		case isc_info_rsb_sequential:
		case isc_info_rsb_ext_sequential:
		case isc_info_rsb_ext_indexed:
		case isc_info_rsb_virt_sequential:
			switch (rsb_type)
			{
			case isc_info_rsb_indexed:
			case isc_info_rsb_ext_indexed:
				p = " INDEX (";
				break;
			case isc_info_rsb_navigate:
				p = " ORDER ";
				break;
			default:
				p = " NATURAL";
			}

			if ((plan_length -= strlen(p)) < 0)
				return false;
			while (*p)
				*plan++ = *p++;

			// print out additional index information

			if (rsb_type == isc_info_rsb_indexed || rsb_type == isc_info_rsb_navigate ||
				rsb_type == isc_info_rsb_ext_indexed)
			{
				if (!get_indices(&explain_length, &explain, &plan_length, &plan))
					return false;
			}

			if (rsb_type == isc_info_rsb_navigate && *explain == isc_info_rsb_indexed)
			{
				USHORT idx_count = 1;
				if (!get_rsb_item(&explain_length, &explain, &plan_length, &plan, &idx_count, level_ptr))
				{
					return false;
				}
			}

			if (rsb_type == isc_info_rsb_indexed || rsb_type == isc_info_rsb_ext_indexed)
			{
				if (--plan_length < 0)
					return false;
				*plan++ = ')';
			}

			// detect the end of a single relation and put out a final parenthesis

			if (!*parent_join_count)
			{
				if (--plan_length < 0)
					return false;
				*plan++ = ')';
			}

			// this also qualifies as a stream, so decrement the join count

			if (*parent_join_count)
				-- * parent_join_count;
			break;

		case isc_info_rsb_sort:

			// if this sort is on behalf of a union, don't bother to
			// print out the sort, because unions handle the sort on all
			// substreams at once, and a plan maps to each substream
			// in the union, so the sort doesn't really apply to a particular plan

			if (explain_length > 2 && (explain[0] == isc_info_rsb_begin) &&
				(explain[1] == isc_info_rsb_type) && (explain[2] == isc_info_rsb_union))
			{
				break;
			}

			// if this isn't the first item in the list, put out a comma

			if (*parent_join_count && plan[-1] != '(')
			{
				plan_length -= 2;
				if (plan_length < 0)
					return false;
				*plan++ = ',';
				*plan++ = ' ';
			}

			p = "SORT (";

			if ((plan_length -= strlen(p)) < 0)
				return false;
			while (*p)
				*plan++ = *p++;

			// the rsb_sort should always be followed by a begin...end block,
			// allowing us to include everything inside the sort in parentheses

			{ // scope to have save_level local.
				const USHORT save_level = *level_ptr;
				while (explain_length > 0 && plan_length > 0)
				{
					if (!get_rsb_item(&explain_length, &explain, &plan_length,
									  &plan, parent_join_count, level_ptr))
					{
						return false;
					}
					if (*level_ptr == save_level)
						break;
				}

				if (--plan_length < 0)
					return false;
				*plan++ = ')';
			} // scope
			break;

		default:
			break;
		} // switch (rsb_type = *explain++)
		break;

	default:
		break;
	}

	*explain_length_ptr = explain_length;
	*explain_ptr = explain;
	//*plan_length_ptr = plan_length;
	//*plan_ptr = plan;

	return true;
}


/**

 	init

    @brief	Initialize dynamic SQL.  This is called only once.


    @param db_handle

 **/
static dsql_dbb* init(Attachment* attachment)
{
	thread_db* tdbb = JRD_get_thread_data();

	if (!attachment->att_dsql_instance)
	{
		MemoryPool& pool = *attachment->att_database->createPool();
		dsql_dbb* const database = FB_NEW(pool) dsql_dbb(pool);
		database->dbb_attachment = attachment;
		database->dbb_database = attachment->att_database;
		attachment->att_dsql_instance = database;

		UCHAR buffer[BUFFER_TINY];

		try
		{
			ThreadStatusGuard status_vector(tdbb);

			INF_database_info(db_hdr_info_items, sizeof(db_hdr_info_items), buffer, sizeof(buffer));
		}
		catch (Firebird::Exception&)
		{
			return database;
		}

		const UCHAR* data = buffer;
		UCHAR p;
		while ((p = *data++) != isc_info_end)
		{
			const SSHORT l = static_cast<SSHORT>(gds__vax_integer(data, 2));
			data += 2;

			switch (p)
			{
			case isc_info_db_sql_dialect:
				fb_assert(l == 1);
				database->dbb_db_SQL_dialect = (USHORT) data[0];
				break;

			case isc_info_ods_version:
				database->dbb_ods_version = gds__vax_integer(data, l);
				if (database->dbb_ods_version <= 7)
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_too_old_ods) << Arg::Num(8));
				}
				break;

			case isc_info_ods_minor_version:
				database->dbb_minor_version = gds__vax_integer(data, l);
				break;

			// This flag indicates the version level of the engine
			// itself, so we can tell what capabilities the engine
			// code itself (as opposed to the on-disk structure).
			// Apparently the base level up to now indicated the major
			// version number, but for 4.1 the base level is being
			// incremented, so the base level indicates an engine version
			// as follows:
			// 1 == v1.x
			// 2 == v2.x
			// 3 == v3.x
			// 4 == v4.0 only
			// 5 == v4.1. (v5, too?)
			// 6 == v6, FB1
			// Note: this info item is so old it apparently uses an
			// archaic format, not a standard vax integer format.

#ifdef SCROLLABLE_CURSORS
			case isc_info_base_level:
				fb_assert(l == 2 && data[0] == UCHAR(1));
				database->dbb_base_level = (USHORT) data[1];
				break;
#endif

			case isc_info_db_read_only:
				fb_assert(l == 1);
				database->dbb_read_only = (USHORT) data[0] ? true : false;
				break;

			default:
				break;
			}

			data += l;
		}
	}

	return attachment->att_dsql_instance;
}


/**

 	map_in_out

    @brief	Map data from external world into message or
 	from message to external world.


    @param request
    @param message
    @param blr_length
    @param blr
    @param msg_length
    @param dsql_msg_buf
    @param in_dsql_msg_buf

 **/
static void map_in_out(	dsql_req*		request,
						dsql_msg*		message,
						USHORT	blr_length,
						const UCHAR*	blr,
						USHORT	msg_length,
						UCHAR*	dsql_msg_buf,
						const UCHAR* in_dsql_msg_buf)
{
	thread_db* tdbb = JRD_get_thread_data();

	USHORT count = parse_blr(blr_length, blr, msg_length, message->msg_parameters);

	// When mapping data from the external world, request will be non-NULL.
	// When mapping data from an internal message, request will be NULL.

	dsql_par* parameter;

	for (parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		if (parameter->par_index)
		{
			 // Make sure the message given to us is long enough

			dsc desc = parameter->par_user_desc;
			USHORT length = (IPTR) desc.dsc_address + desc.dsc_length;
			if (length > msg_length)
				break;
			if (!desc.dsc_dtype)
				break;

			SSHORT* flag = NULL;
			dsql_par* const null_ind = parameter->par_null;
			if (null_ind != NULL)
			{
				const USHORT null_offset = (IPTR) null_ind->par_user_desc.dsc_address;
				length = null_offset + sizeof(SSHORT);
				if (length > msg_length)
					break;

				if (!request) {
					flag = reinterpret_cast<SSHORT*>(dsql_msg_buf + null_offset);
					*flag = *reinterpret_cast<const SSHORT*>(null_ind->par_desc.dsc_address);
				}
				else {
					flag = reinterpret_cast<SSHORT*>(null_ind->par_desc.dsc_address);
					*flag = *reinterpret_cast<const SSHORT*>(in_dsql_msg_buf + null_offset);
				}
			}

			if (!request)
			{
				desc.dsc_address = dsql_msg_buf + (IPTR) desc.dsc_address;

				if (!flag || *flag >= 0)
				{
					MOVD_move(tdbb, &parameter->par_desc, &desc);
				}
				else
				{
					memset(desc.dsc_address, 0, desc.dsc_length);
				}
			}
			else if (!flag || *flag >= 0)
			{
				if (!(parameter->par_desc.dsc_flags & DSC_null))
				{
					// Safe cast because desc is used as source only.
					desc.dsc_address = const_cast<UCHAR*>(in_dsql_msg_buf) + (IPTR) desc.dsc_address;
					MOVD_move(tdbb, &desc, &parameter->par_desc);
				}
			}
			else
			{
				memset(parameter->par_desc.dsc_address, 0, parameter->par_desc.dsc_length);
			}

			count--;
		}
	}

	// If we got here because the loop was exited early or if part of the
	// message given to us hasn't been used, complain.

	if (parameter || count)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err));
	}

	dsql_par* dbkey;
	if (request && ((dbkey = request->req_parent_dbkey) != NULL) &&
		((parameter = request->req_dbkey) != NULL))
	{
		MOVD_move(tdbb, &dbkey->par_desc, &parameter->par_desc);
		dsql_par* null_ind = parameter->par_null;
		if (null_ind != NULL)
		{
			SSHORT* flag = (SSHORT *) null_ind->par_desc.dsc_address;
			*flag = 0;
		}
	}

	dsql_par* rec_version;
	if (request && ((rec_version = request->req_parent_rec_version) != NULL) &&
		((parameter = request->req_rec_version) != NULL))
	{
		MOVD_move(tdbb, &rec_version->par_desc, &parameter->par_desc);
		dsql_par* null_ind = parameter->par_null;
		if (null_ind != NULL)
		{
			SSHORT* flag = (SSHORT *) null_ind->par_desc.dsc_address;
			*flag = 0;
		}
	}
}


/**

 	parse_blr

    @brief	Parse the message of a blr request.


    @param blr_length
    @param blr
    @param msg_length
    @param parameters

 **/
static USHORT parse_blr(USHORT blr_length,
						const UCHAR* blr, const USHORT msg_length, dsql_par* parameters_list)
{
	Firebird::HalfStaticArray<dsql_par*, 16> parameters;

	for (dsql_par* param = parameters_list; param; param = param->par_next)
	{
		if (param->par_index)
		{
			if (param->par_index > parameters.getCount())
				parameters.grow(param->par_index);
			fb_assert(!parameters[param->par_index - 1]);
			parameters[param->par_index - 1] = param;
		}
	}

	// If there's no blr length, then the format of the current message buffer
	// is identical to the format of the previous one.

	if (!blr_length)
	{
		return parameters.getCount();
	}

	if (*blr != blr_version4 && *blr != blr_version5)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err));
	}
	blr++;						// skip the blr_version
	if (*blr++ != blr_begin || *blr++ != blr_message)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err));
	}

	++blr;						// skip the message number
	USHORT count = *blr++;
	count += (*blr++) << 8;
	count /= 2;

	if (count != parameters.getCount())
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err));
	}

	USHORT offset = 0;
	for (USHORT index = 1; index <= count; index++)
	{
		dsc desc;
		desc.dsc_scale = 0;
		desc.dsc_sub_type = 0;
		desc.dsc_flags = 0;

		switch (*blr++)
		{
		case blr_text:
			desc.dsc_dtype = dtype_text;
			desc.dsc_sub_type = ttype_dynamic;
			desc.dsc_length = *blr++;
			desc.dsc_length += (*blr++) << 8;
			break;

		case blr_varying:
			desc.dsc_dtype = dtype_varying;
			desc.dsc_sub_type = ttype_dynamic;
			desc.dsc_length = *blr++ + sizeof(USHORT);
			desc.dsc_length += (*blr++) << 8;
			break;

		case blr_text2:
			desc.dsc_dtype = dtype_text;
			desc.dsc_sub_type = *blr++;
			desc.dsc_sub_type += (*blr++) << 8;
			desc.dsc_length = *blr++;
			desc.dsc_length += (*blr++) << 8;
			break;

		case blr_varying2:
			desc.dsc_dtype = dtype_varying;
			desc.dsc_sub_type = *blr++;
			desc.dsc_sub_type += (*blr++) << 8;
			desc.dsc_length = *blr++ + sizeof(USHORT);
			desc.dsc_length += (*blr++) << 8;
			break;

		case blr_short:
			desc.dsc_dtype = dtype_short;
			desc.dsc_length = sizeof(SSHORT);
			desc.dsc_scale = *blr++;
			break;

		case blr_long:
			desc.dsc_dtype = dtype_long;
			desc.dsc_length = sizeof(SLONG);
			desc.dsc_scale = *blr++;
			break;

		case blr_int64:
			desc.dsc_dtype = dtype_int64;
			desc.dsc_length = sizeof(SINT64);
			desc.dsc_scale = *blr++;
			break;

		case blr_quad:
			desc.dsc_dtype = dtype_quad;
			desc.dsc_length = sizeof(SLONG) * 2;
			desc.dsc_scale = *blr++;
			break;

		case blr_float:
			desc.dsc_dtype = dtype_real;
			desc.dsc_length = sizeof(float);
			break;

		case blr_double:
		case blr_d_float:
			desc.dsc_dtype = dtype_double;
			desc.dsc_length = sizeof(double);
			break;

		case blr_timestamp:
			desc.dsc_dtype = dtype_timestamp;
			desc.dsc_length = sizeof(SLONG) * 2;
			break;

		case blr_sql_date:
			desc.dsc_dtype = dtype_sql_date;
			desc.dsc_length = sizeof(SLONG);
			break;

		case blr_sql_time:
			desc.dsc_dtype = dtype_sql_time;
			desc.dsc_length = sizeof(SLONG);
			break;

		case blr_blob2:
			{
				desc.dsc_dtype = dtype_blob;
				desc.dsc_length = sizeof(ISC_QUAD);
				desc.dsc_sub_type = *blr++;
				desc.dsc_sub_type += (*blr++) << 8;

				USHORT textType = *blr++;
				textType += (*blr++) << 8;
				desc.setTextType(textType);
			}
			break;

		default:
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_sqlda_err));
		}

		USHORT align = type_alignments[desc.dsc_dtype];
		if (align)
			offset = FB_ALIGN(offset, align);
		desc.dsc_address = (UCHAR*)(IPTR) offset;
		offset += desc.dsc_length;

		if (*blr++ != blr_short || *blr++ != 0)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_sqlda_err));
		}

		align = type_alignments[dtype_short];
		if (align)
			offset = FB_ALIGN(offset, align);
		USHORT null_offset = offset;
		offset += sizeof(SSHORT);

		dsql_par* const parameter = parameters[index - 1];
		fb_assert(parameter);

		parameter->par_user_desc = desc;

		// ASF: Older than 2.5 engine hasn't validating strings in DSQL. After this has been
		// implemented in 2.5, selecting a NONE column with UTF-8 attachment charset started
		// failing. The real problem is that the client encodes SQL_TEXT/SQL_VARYING using
		// blr_text/blr_varying (i.e. with the connection charset). I'm reseting the charset
		// here at the server as a way to make older (and not yet changed) client work
		// correctly.
		if (parameter->par_user_desc.isText())
			parameter->par_user_desc.setTextType(ttype_none);

		dsql_par* null = parameter->par_null;
		if (null)
		{
			null->par_user_desc.dsc_dtype = dtype_short;
			null->par_user_desc.dsc_scale = 0;
			null->par_user_desc.dsc_length = sizeof(SSHORT);
			null->par_user_desc.dsc_address = (UCHAR*)(IPTR) null_offset;
		}
	}

	if (*blr++ != (UCHAR) blr_end || offset != msg_length)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err));
	}

	return count;
}


/**

 	prepare

    @brief	Prepare a statement for execution.  Return SQL status
 	code.  Note: caller is responsible for pool handling.


    @param request
    @param string_length
    @param string
    @param client_dialect
    @param parser_version

 **/
static dsql_req* prepare(thread_db* tdbb, dsql_dbb* database, jrd_tra* transaction,
				   USHORT string_length,
				   const TEXT* string,
				   USHORT client_dialect, USHORT parser_version)
{
	ISC_STATUS_ARRAY local_status;
	MOVE_CLEAR(local_status, sizeof(local_status));

	TraceDSQLPrepare trace(database->dbb_attachment, string_length, string);

	if (client_dialect > SQL_DIALECT_CURRENT)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_wish_list));
	}

	if (string && !string_length)
	{
		size_t sql_length = strlen(string);
		if (sql_length > MAX_USHORT)
			sql_length = MAX_USHORT;
		string_length = static_cast<USHORT>(sql_length);
	}

	if (!string || !string_length) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Unexpected end of command
				  // CVC: Nothing will be line 1, column 1 for the user.
				  Arg::Gds(isc_command_end_err2) << Arg::Num(1) << Arg::Num(1));
	}

	// Get rid of the trailing ";" if there is one.

	for (const TEXT* p = string + string_length; p-- > string;)
	{
		if (*p != ' ') {
			if (*p == ';')
				string_length = p - string;
			break;
		}
	}

	// allocate the statement block, then prepare the statement

	Jrd::ContextPoolHolder context(tdbb, database->createPool());

	MemoryPool& pool = *tdbb->getDefaultPool();
	CompiledStatement* statement = FB_NEW(pool) CompiledStatement(pool);
	statement->req_dbb = database;
	database->dbb_requests.add(statement);
	statement->req_transaction = transaction;
	statement->req_client_dialect = client_dialect;
	statement->req_traced = true;

	trace.setStatement(statement);

	try {

	// Parse the SQL statement.  If it croaks, return

	Parser parser(*tdbb->getDefaultPool(), client_dialect, statement->req_dbb->dbb_db_SQL_dialect,
		parser_version, string, string_length, tdbb->getAttachment()->att_charset);

	dsql_nod* node = parser.parse();

	statement->req_sql_text = FB_NEW(pool) RefString(pool, parser.getTransformedString());

	if (!node)
	{
		// CVC: Apparently, dsql_ypparse won't return if the command is incomplete,
		// because yyerror() will call ERRD_post().
		// This may be a special case, but we don't know about positions here.
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Unexpected end of command
				  Arg::Gds(isc_command_end_err));
	}

	// allocate the send and receive messages

	statement->req_send = FB_NEW(pool) dsql_msg;
	dsql_msg* message = FB_NEW(pool) dsql_msg;
	statement->req_receive = message;
	message->msg_number = 1;

#ifdef SCROLLABLE_CURSORS
	if (statement->req_dbb->dbb_base_level >= 5) {
		// allocate a message in which to send scrolling information
		// outside of the normal send/receive protocol

		statement->req_async = message = FB_NEW(*tdsql->getDefaultPool()) dsql_msg;
		message->msg_number = 2;
	}
#endif

	statement->req_type = REQ_SELECT;
	statement->req_flags &= ~REQ_cursor_open;

	// No work is done during pass1 for set transaction - like
	// checking for valid table names.  This is because that will
	// require a valid transaction handle.
	// Error will be caught at execute time.

	node = PASS1_statement(statement, node);
	if (!node)
		return statement;

	switch (statement->req_type)
	{
	case REQ_COMMIT:
	case REQ_COMMIT_RETAIN:
	case REQ_ROLLBACK:
	case REQ_ROLLBACK_RETAIN:
	case REQ_GET_SEGMENT:
	case REQ_PUT_SEGMENT:
	case REQ_START_TRANS:
		statement->req_traced = false;
		break;
	default:
		statement->req_traced = true;
	}

	// stop here for statements not requiring code generation

	if (statement->req_type == REQ_DDL && parser.isStmtAmbiguous() &&
		statement->req_dbb->dbb_db_SQL_dialect != client_dialect)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-817) <<
				  Arg::Gds(isc_ddl_not_allowed_by_db_sql_dial) << Arg::Num(statement->req_dbb->dbb_db_SQL_dialect));
	}

	switch (statement->req_type)
	{
	case REQ_COMMIT:
	case REQ_COMMIT_RETAIN:
	case REQ_ROLLBACK:
	case REQ_ROLLBACK_RETAIN:
		return statement;

	// Work on blob segment statements
	case REQ_GET_SEGMENT:
	case REQ_PUT_SEGMENT:
		GEN_port(statement, statement->req_blob->blb_open_in_msg);
		GEN_port(statement, statement->req_blob->blb_open_out_msg);
		GEN_port(statement, statement->req_blob->blb_segment_msg);
		return statement;

	// Generate BLR, DDL or TPB for statement
	// Start transactions takes parameters via a parameter block.
	// The statement blr string is used for that
	case REQ_START_TRANS:
		GEN_start_transaction(statement, node);
		return statement;
	}

	if (client_dialect > SQL_DIALECT_V5)
		statement->req_flags |= REQ_blr_version5;
	else
		statement->req_flags |= REQ_blr_version4;

	GEN_request(statement, node);
	const ULONG length = (ULONG) statement->req_blr_data.getCount();

	// stop here for ddl statements

	if (statement->req_type == REQ_CREATE_DB || statement->req_type == REQ_DDL)
	{
		// Notify Trace API manager about new DDL request cooked.
		trace.prepare(res_successful);
		return statement;
	}

	// have the access method compile the statement

#ifdef DSQL_DEBUG
	if (DSQL_debug & 64) {
		dsql_trace("Resulting BLR code for DSQL:");
		gds__trace_raw("Statement:\n");
		gds__trace_raw(string, string_length);
		gds__trace_raw("\nBLR:\n");
		fb_print_blr(statement->req_blr_data.begin(),
			(ULONG) statement->req_blr_data.getCount(),
			gds__trace_printer, 0, 0);
	}
#endif

	// check for warnings
	if (tdbb->tdbb_status_vector[2] == isc_arg_warning) 
	{
		// save a status vector
		memcpy(local_status, tdbb->tdbb_status_vector, sizeof(ISC_STATUS_ARRAY));
		fb_utils::init_status(tdbb->tdbb_status_vector);
	}

	ISC_STATUS status = FB_SUCCESS;

	try
	{
		JRD_compile(tdbb,
					statement->req_dbb->dbb_attachment,
					&statement->req_request,
					length,
					statement->req_blr_data.begin(),
					statement->req_sql_text,
					statement->req_debug_data.getCount(),
					statement->req_debug_data.begin());
	}
	catch (const Firebird::Exception&)
	{
		status = tdbb->tdbb_status_vector[1];
		trace.prepare(status == isc_no_priv ? res_unauthorized : res_failed);
	}

	// restore warnings (if there are any)
	if (local_status[2] == isc_arg_warning)
	{
		int indx, len, warning;

		// find end of a status vector
		PARSE_STATUS(tdbb->tdbb_status_vector, indx, warning);
		if (indx)
			--indx;

		// calculate length of saved warnings
		PARSE_STATUS(local_status, len, warning);
		len -= 2;

		if ((len + indx - 1) < ISC_STATUS_LENGTH)
			memcpy(&tdbb->tdbb_status_vector[indx], &local_status[2], sizeof(ISC_STATUS) * len);
	}

	// free blr memory
	statement->req_blr_data.free();

	if (status)
		Firebird::status_exception::raise(tdbb->tdbb_status_vector);

	// Notify Trace API manager about new request cooked.
	trace.prepare(res_successful);

	return statement;

	}
	catch (const Firebird::Exception&)
	{
		trace.prepare(res_failed);
		statement->req_traced = false;
		release_request(tdbb, statement, true);
		throw;
	}
}


/**

 	put_item

    @brief	Put information item in output buffer if there is room, and
 	return an updated pointer.  If there isn't room for the item,
 	indicate truncation and return NULL.


    @param item
    @param length
    @param string
    @param ptr
    @param end
	@param copy

 **/
static UCHAR* put_item(	UCHAR	item,
						const USHORT	length,
						const UCHAR* string,
						UCHAR*	ptr,
						const UCHAR* const end,
						const bool copy)
{
	if (ptr + length + 3 >= end) {
		*ptr = isc_info_truncated;
		return NULL;
	}

	*ptr++ = item;

	*ptr++ = (UCHAR) length;
	*ptr++ = length >> 8;

	if (length && copy)
		memcpy(ptr, string, length);

	return ptr + length;
}


/**

 	release_request

    @brief	Release a dynamic request.


    @param request
    @param top_level

 **/
static void release_request(thread_db* tdbb, dsql_req* request, bool drop)
{
	SET_TDBB(tdbb);

	// If request is parent, orphan the children and
	// release a portion of their requests

	for (dsql_req* child = request->req_offspring; child; child = child->req_sibling)
	{
		child->req_flags |= REQ_orphan;
		child->req_parent = NULL;
		Jrd::ContextPoolHolder context(tdbb, &child->req_pool);
		release_request(tdbb, child, false);
	}

	// For requests that are linked to a parent, unlink it

	if (request->req_parent)
	{
		dsql_req* parent = request->req_parent;
		for (dsql_req** ptr = &parent->req_offspring; *ptr; ptr = &(*ptr)->req_sibling)
		{
			if (*ptr == request) {
				*ptr = request->req_sibling;
				break;
			}
		}
		request->req_parent = NULL;
	}

	// If the request had an open cursor, close it

	if (request->req_flags & REQ_cursor_open) {
		close_cursor(tdbb, request);
	}

	Attachment* att = request->req_dbb->dbb_attachment;
	const bool need_trace_free = request->req_traced && TraceManager::need_dsql_free(att);
	if (need_trace_free)
	{
		TraceSQLStatementImpl stmt(request, NULL);
		TraceManager::event_dsql_free(att, &stmt, DSQL_drop);
	}
	request->req_traced = false;

	// If request is named, clear it from the hash table

	if (request->req_name) {
		HSHD_remove(request->req_name);
		request->req_name = NULL;
	}

	if (request->req_cursor) {
		HSHD_remove(request->req_cursor);
		request->req_cursor = NULL;
	}

	// If a request has been compiled, release it now

	if (request->req_request)
	{
		ThreadStatusGuard status_vector(tdbb);

		try
		{
			CMP_release(tdbb, request->req_request);
			request->req_request = NULL;
		}
		catch (Firebird::Exception&)
		{
		}
	}

	request->req_sql_text = NULL;

	// free blr memory
	request->req_blr_data.free();

	// Release the entire request if explicitly asked for

	if (drop)
	{
		dsql_dbb* dbb = request->req_dbb;
		size_t pos;
		if (dbb->dbb_requests.find(request, pos)) {
			dbb->dbb_requests.remove(pos);
		}
		dbb->deletePool(&request->req_pool);
	}
}


/**

	sql_info

	@brief	Return DSQL information buffer.

	@param request
	@param item_length
	@param items
	@param info_length
	@param info

 **/

static void sql_info(thread_db* tdbb,
					 dsql_req* request,
					 USHORT item_length,
					 const UCHAR* items,
					 ULONG info_length,
					 UCHAR* info)
{
	if (!item_length || !items || !info_length || !info)
		return;

	UCHAR buffer[BUFFER_SMALL];
	memset(buffer, 0, sizeof(buffer));

	// Pre-initialize buffer. This is necessary because we don't want to transfer rubbish over the wire
	memset(info, 0, info_length);

	const UCHAR* const end_items = items + item_length;
	const UCHAR* const end_info = info + info_length;
	UCHAR *start_info;
	if (*items == isc_info_length) {
		start_info = info;
		items++;
	}
	else {
		start_info = NULL;
	}

	// CVC: Is it the idea that this pointer remains with its previous value
	// in the loop or should it be made NULL in each iteration?
	dsql_msg** message = NULL;
	USHORT first_index = 0;

	while (items < end_items && *items != isc_info_end)
	{
		ULONG length;
		USHORT number;
		const UCHAR item = *items++;
		switch (item)
		{
		case isc_info_sql_select:
		case isc_info_sql_bind:
			message = (item == isc_info_sql_select) ? &request->req_receive : &request->req_send;
			if (info + 1 >= end_info) {
				*info = isc_info_truncated;
				return;
			}
			*info++ = item;
			break;
		case isc_info_sql_stmt_type:
			switch (request->req_type)
			{
			case REQ_SELECT:
			case REQ_EMBED_SELECT:
				number = isc_info_sql_stmt_select;
				break;
			case REQ_SELECT_UPD:
				number = isc_info_sql_stmt_select_for_upd;
				break;
			case REQ_CREATE_DB:
			case REQ_DDL:
				number = isc_info_sql_stmt_ddl;
				break;
			case REQ_GET_SEGMENT:
				number = isc_info_sql_stmt_get_segment;
				break;
			case REQ_PUT_SEGMENT:
				number = isc_info_sql_stmt_put_segment;
				break;
			case REQ_COMMIT:
			case REQ_COMMIT_RETAIN:
				number = isc_info_sql_stmt_commit;
				break;
			case REQ_ROLLBACK:
			case REQ_ROLLBACK_RETAIN:
				number = isc_info_sql_stmt_rollback;
				break;
			case REQ_START_TRANS:
				number = isc_info_sql_stmt_start_trans;
				break;
			case REQ_INSERT:
				number = isc_info_sql_stmt_insert;
				break;
			case REQ_UPDATE:
			case REQ_UPDATE_CURSOR:
				number = isc_info_sql_stmt_update;
				break;
			case REQ_DELETE:
			case REQ_DELETE_CURSOR:
				number = isc_info_sql_stmt_delete;
				break;
			case REQ_EXEC_PROCEDURE:
				number = isc_info_sql_stmt_exec_procedure;
				break;
			case REQ_SET_GENERATOR:
				number = isc_info_sql_stmt_set_generator;
				break;
			case REQ_SAVEPOINT:
				number = isc_info_sql_stmt_savepoint;
				break;
			case REQ_EXEC_BLOCK:
				number = isc_info_sql_stmt_exec_procedure;
				break;
			case REQ_SELECT_BLOCK:
				number = isc_info_sql_stmt_select;
				break;
			default:
				number = 0;
				break;
			}
			length = convert((SLONG) number, buffer);
			info = put_item(item, length, buffer, info, end_info);
			if (!info) {
				return;
			}
			break;
		case isc_info_sql_sqlda_start:
			length = *items++;
			first_index = static_cast<USHORT>(gds__vax_integer(items, length));
			items += length;
			break;
		case isc_info_sql_batch_fetch:
			if (request->req_flags & REQ_no_batch)
				number = 0;
			else
				number = 1;
			length = convert((SLONG) number, buffer);
			if (!(info = put_item(item, length, buffer, info, end_info))) {
				return;
			}
			break;
		case isc_info_sql_records:
			length = get_request_info(tdbb, request, sizeof(buffer), buffer);
			if (length && !(info = put_item(item, length, buffer, info, end_info)))
			{
				return;
			}
			break;
		case isc_info_sql_get_plan:
			{
				// be careful, get_plan_info() will reallocate the buffer to a
				// larger size if it is not big enough
				//UCHAR* buffer_ptr = buffer;
				UCHAR* buffer_ptr = info + 3;
				// Somebody decided to put a platform-dependent NEWLINE at the beginning,
				// see get_rsb_item! This idea predates FB1.
				static const size_t minPlan = strlen("\nPLAN (T NATURAL)");

				if (info + minPlan + 3 >= end_info)
				{
					fb_assert(info < end_info);
					*info = isc_info_truncated;
					info = NULL;
					length = 0;
				}
				else
				{
					//length = DSQL_get_plan_info(tdbb, request, sizeof(buffer),
					//					reinterpret_cast<SCHAR**>(&buffer_ptr));
					length = DSQL_get_plan_info(tdbb, request, (end_info - info - 4),
										reinterpret_cast<SCHAR**>(&buffer_ptr),
										false);
				}

				if (length)
				{
					if (length > MAX_USHORT)
					{
						fb_assert(info < end_info);
						*info = isc_info_truncated;
						info = NULL;
					}
					else
						info = put_item(item, length, buffer_ptr, info, end_info, false);
				}

				//if (length > sizeof(buffer) || buffer_ptr != buffer) {
				//	gds__free(buffer_ptr);
				//}

				if (!info) {
					return;
				}
			}
			break;
		case isc_info_sql_num_variables:
		case isc_info_sql_describe_vars:
			if (message)
			{
				number = (*message) ? (*message)->msg_index : 0;
				length = convert((SLONG) number, buffer);
				if (!(info = put_item(item, length, buffer, info, end_info))) {
					return;
				}
				if (item == isc_info_sql_num_variables) {
					continue;
				}

				const UCHAR* end_describe = items;
				while (end_describe < end_items &&
					*end_describe != isc_info_end && *end_describe != isc_info_sql_describe_end)
				{
					end_describe++;
				}

				info = var_info(*message, items, end_describe, info, end_info, first_index,
					message == &request->req_send);
				if (!info) {
					return;
				}

				items = end_describe;
				if (*items == isc_info_sql_describe_end) {
					items++;
				}
				break;
			}
			// else fall into
		default:
			buffer[0] = item;
			length = 1 + convert((SLONG) isc_infunk, buffer + 1);
			if (!(info = put_item(isc_info_error, length, buffer, info, end_info))) {
				return;
			}
		}
	}

	*info++ = isc_info_end;

	if (start_info && (end_info - info >= 7))
	{
		const SLONG number = info - start_info;
		fb_assert(number > 0);
		memmove(start_info + 7, start_info, number);
		USHORT length = convert(number, buffer);
		fb_assert(length == 4); // We only accept SLONG
		put_item(isc_info_length, length, buffer, start_info, end_info);
	}
}


/**

 	var_info

    @brief	Provide information on an internal message.


    @param message
    @param items
    @param end_describe
    @param info
    @param end
    @param first_index

 **/
static UCHAR* var_info(dsql_msg* message,
					   const UCHAR* items,
					   const UCHAR* const end_describe,
					   UCHAR* info,
					   const UCHAR* const end,
					   USHORT first_index,
					   bool input_message)
{
	if (!message || !message->msg_index)
		return info;

	Firebird::HalfStaticArray<const dsql_par*, 16> parameters;

	for (const dsql_par* param = message->msg_parameters; param; param = param->par_next)
	{
		if (param->par_index)
		{
			if (param->par_index > parameters.getCount())
				parameters.grow(param->par_index);
			fb_assert(!parameters[param->par_index - 1]);
			parameters[param->par_index - 1] = param;
		}
	}

	UCHAR buf[128];

	for (size_t i = 0; i < parameters.getCount(); i++)
	{
		const dsql_par* param = parameters[i];
		fb_assert(param);

		if (param->par_index >= first_index)
		{
			SLONG sql_len = param->par_desc.dsc_length;
			SLONG sql_sub_type = 0;
			SLONG sql_scale = 0;
			SLONG sql_type = 0;

			switch (param->par_desc.dsc_dtype)
			{
			case dtype_real:
				sql_type = SQL_FLOAT;
				break;
			case dtype_array:
				sql_type = SQL_ARRAY;
				break;

			case dtype_timestamp:
				sql_type = SQL_TIMESTAMP;
				break;
			case dtype_sql_date:
				sql_type = SQL_TYPE_DATE;
				break;
			case dtype_sql_time:
				sql_type = SQL_TYPE_TIME;
				break;

			case dtype_double:
				sql_type = SQL_DOUBLE;
				sql_scale = param->par_desc.dsc_scale;
				break;

			case dtype_text:
				if (input_message && (param->par_desc.dsc_flags & DSC_null))
				{
					sql_type = SQL_NULL;
					sql_len = 0;
				}
				else
				{
					sql_type = SQL_TEXT;
					sql_sub_type = param->par_desc.dsc_sub_type;
				}
				break;

			case dtype_blob:
				sql_type = SQL_BLOB;
				sql_sub_type = param->par_desc.dsc_sub_type;
				sql_scale = param->par_desc.dsc_scale;
				break;

			case dtype_varying:
				sql_type = SQL_VARYING;
				sql_len -= sizeof(USHORT);
				sql_sub_type = param->par_desc.dsc_sub_type;
				break;

			case dtype_short:
			case dtype_long:
			case dtype_int64:
				switch (param->par_desc.dsc_dtype)
				{
				case dtype_short:
					sql_type = SQL_SHORT;
					break;
				case dtype_long:
					sql_type = SQL_LONG;
					break;
				default:
					sql_type = SQL_INT64;
				}
				sql_scale = param->par_desc.dsc_scale;
				if (param->par_desc.dsc_sub_type)
					sql_sub_type = param->par_desc.dsc_sub_type;
				break;

			case dtype_quad:
				sql_type = SQL_QUAD;
				sql_scale = param->par_desc.dsc_scale;
				break;

			default:
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
						  Arg::Gds(isc_dsql_datatype_err));
			}

			if (sql_type && (param->par_desc.dsc_flags & DSC_nullable))
				sql_type++;

			for (const UCHAR* describe = items; describe < end_describe;)
			{
				USHORT length;
				const TEXT* name;
				const UCHAR* buffer = buf;
				UCHAR item = *describe++;
				switch (item)
				{
				case isc_info_sql_sqlda_seq:
					length = convert((SLONG) param->par_index, buf);
					break;

				case isc_info_sql_message_seq:
					length = 0;
					break;

				case isc_info_sql_type:
					length = convert((SLONG) sql_type, buf);
					break;

				case isc_info_sql_sub_type:
					length = convert((SLONG) sql_sub_type, buf);
					break;

				case isc_info_sql_scale:
					length = convert((SLONG) sql_scale, buf);
					break;

				case isc_info_sql_length:
					length = convert((SLONG) sql_len, buf);
					break;

				case isc_info_sql_null_ind:
					length = convert((SLONG) (sql_type & 1), buf);
					break;

				case isc_info_sql_field:
					if (name = param->par_name) {
						length = strlen(name);
						buffer = reinterpret_cast<const UCHAR*>(name);
					}
					else
						length = 0;
					break;

				case isc_info_sql_relation:
					if (name = param->par_rel_name) {
						length = strlen(name);
						buffer = reinterpret_cast<const UCHAR*>(name);
					}
					else
						length = 0;
					break;

				case isc_info_sql_owner:
					if (name = param->par_owner_name) {
						length = strlen(name);
						buffer = reinterpret_cast<const UCHAR*>(name);
					}
					else
						length = 0;
					break;

				case isc_info_sql_relation_alias:
					if (name = param->par_rel_alias) {
						length = strlen(name);
						buffer = reinterpret_cast<const UCHAR*>(name);
					}
					else
						length = 0;
					break;

				case isc_info_sql_alias:
					if (name = param->par_alias) {
						length = strlen(name);
						buffer = reinterpret_cast<const UCHAR*>(name);
					}
					else
						length = 0;
					break;

				default:
					buf[0] = item;
					item = isc_info_error;
					length = 1 + convert((SLONG) isc_infunk, buf + 1);
					break;
				}

				if (!(info = put_item(item, length, buffer, info, end)))
					return info;
			}

			if (info + 1 >= end) {
				*info = isc_info_truncated;
				return NULL;
			}
			*info++ = isc_info_sql_describe_end;
		} // if()
	} // for()

	return info;
}
