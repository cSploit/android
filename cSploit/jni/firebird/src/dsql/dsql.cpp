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
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
#include "../common/intlobj_new.h"
#include "../jrd/jrd.h"
#include "../common/CharSet.h"
#include "../dsql/Parser.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/dsql_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/movd_proto.h"
#include "../dsql/pass1_proto.h"
#include "../dsql/metd_proto.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/ini_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../jrd/trace/TraceManager.h"
#include "../jrd/trace/TraceDSQLHelpers.h"
#include "../common/classes/init.h"
#include "../common/utils_proto.h"
#include "../common/StatusArg.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

using namespace Jrd;
using namespace Firebird;


static void		close_cursor(thread_db*, dsql_req*);
static ULONG	get_request_info(thread_db*, dsql_req*, ULONG, UCHAR*);
static dsql_dbb*	init(Jrd::thread_db*, Jrd::Attachment*);
static void		map_in_out(dsql_req*, bool, const dsql_msg*, IMessageMetadata*, UCHAR*,
	const UCHAR* = NULL);
static USHORT	parse_metadata(dsql_req*, IMessageMetadata*, const Array<dsql_par*>&);
static dsql_req* prepareRequest(thread_db*, dsql_dbb*, jrd_tra*, ULONG, const TEXT*, USHORT, USHORT, bool);
static dsql_req* prepareStatement(thread_db*, dsql_dbb*, jrd_tra*, ULONG, const TEXT*, USHORT, USHORT, bool);
static UCHAR*	put_item(UCHAR, const USHORT, const UCHAR*, UCHAR*, const UCHAR* const);
static void		release_statement(DsqlCompiledStatement* statement);
static void		sql_info(thread_db*, dsql_req*, ULONG, const UCHAR*, ULONG, UCHAR*);
static UCHAR*	var_info(const dsql_msg*, const UCHAR*, const UCHAR* const, UCHAR*,
	const UCHAR* const, USHORT, bool);
static void		check(IStatus*);

static inline bool reqTypeWithCursor(DsqlCompiledStatement::Type type)
{
	switch (type)
	{
		case DsqlCompiledStatement::TYPE_SELECT:
		case DsqlCompiledStatement::TYPE_SELECT_BLOCK:
		case DsqlCompiledStatement::TYPE_SELECT_UPD:
			return true;
	}

	return false;
}

#ifdef DSQL_DEBUG
unsigned DSQL_debug = 0;
#endif

namespace
{
	const UCHAR record_info[] =
	{
		isc_info_req_update_count, isc_info_req_delete_count,
		isc_info_req_select_count, isc_info_req_insert_count
	};
}	// namespace


#ifdef DSQL_DEBUG
IMPLEMENT_TRACE_ROUTINE(dsql_trace, "DSQL")
#endif

dsql_dbb::~dsql_dbb()
{
}


// Execute a non-SELECT dynamic SQL statement.
void DSQL_execute(thread_db* tdbb,
				  jrd_tra** tra_handle,
				  dsql_req* request,
				  bool flOpenCursor,
				  IMessageMetadata* in_meta, const UCHAR* in_msg,
				  IMessageMetadata* out_meta, UCHAR* out_msg)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->getPool());

	const DsqlCompiledStatement* statement = request->getStatement();

	if (statement->getFlags() & DsqlCompiledStatement::FLAG_ORPHAN)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
		          Arg::Gds(isc_bad_req_handle));
	}

	// Only allow NULL trans_handle if we're starting a transaction

	if (!*tra_handle && statement->getType() != DsqlCompiledStatement::TYPE_START_TRANS)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_bad_trans_handle));
	}

	// A select with a non zero output length is a singleton select
	const bool singleton = reqTypeWithCursor(statement->getType()) && out_msg;

	// If the request is a SELECT or blob statement then this is an open.
	// Make sure the cursor is not already open.

	if (reqTypeWithCursor(statement->getType()))
	{
		if (request->req_flags & dsql_req::FLAG_OPENED_CURSOR)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
					  Arg::Gds(isc_dsql_cursor_open_err));
		}
		if (!(flOpenCursor || singleton))
		{
			(Arg::Gds(isc_random) << "Can not execute select statement").raise();
		}
	}
	else if (flOpenCursor)
	{
		if (request->req_flags & dsql_req::FLAG_OPENED_CURSOR)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
					  Arg::Gds(isc_dsql_cursor_open_err));
		}
	}

	request->req_transaction = *tra_handle;
	request->execute(tdbb, tra_handle, in_meta, in_msg, out_meta, out_msg, singleton);

	// If the output message length is zero on a TYPE_SELECT then we must
	// be doing an OPEN cursor operation.
	// If we do have an output message length, then we're doing
	// a singleton SELECT.  In that event, we don't add the cursor
	// to the list of open cursors (it's not really open).

	if (reqTypeWithCursor(statement->getType()) && !singleton)
	{
		fb_assert(flOpenCursor);
		request->req_flags |= dsql_req::FLAG_OPENED_CURSOR;
		TRA_link_cursor(request->req_transaction, request);
	}
	else
	{
		///fb_assert(!flOpenCursor);
	}
}


// Provide backward-compatibility
void DsqlDmlRequest::setDelayedFormat(thread_db* tdbb, Firebird::IMessageMetadata* metadata)
{
	if (!needDelayedFormat)
	{
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
			Arg::Gds(isc_dsql_sqlda_err) <<
			Arg::Gds(isc_req_sync));
	}

	needDelayedFormat = false;
	delayedFormat = metadata;
}


// Fetch next record from a dynamic SQL cursor.
bool DsqlDmlRequest::fetch(thread_db* tdbb, UCHAR* msgBuffer)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &getPool());

	const DsqlCompiledStatement* statement = getStatement();

	// if the cursor isn't open, we've got a problem
	if (reqTypeWithCursor(statement->getType()))
	{
		if (!(req_flags & dsql_req::FLAG_OPENED_CURSOR))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_dsql_cursor_err) <<
					  Arg::Gds(isc_dsql_cursor_not_open));
		}
	}

	dsql_msg* message = (dsql_msg*) statement->getReceiveMsg();

	// Set up things for tracing this call
	Jrd::Attachment* att = req_dbb->dbb_attachment;
	TraceDSQLFetch trace(att, this);

	UCHAR* dsqlMsgBuffer = req_msg_buffers[message->msg_buffer_number];
	JRD_receive(tdbb, req_request, message->msg_number, message->msg_length, dsqlMsgBuffer);

	const dsql_par* const eof = statement->getEof();
	const USHORT* eofPtr = eof ? (USHORT*) (dsqlMsgBuffer + (IPTR) eof->par_desc.dsc_address) : NULL;
	const bool eofReached = eof && !(*eofPtr);

	if (eofReached)
	{
		trace.fetch(true, res_successful);
		return false;
	}

	map_in_out(this, true, message, delayedFormat, msgBuffer);
	delayedFormat = NULL;

	trace.fetch(false, res_successful);
	return true;
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

	Jrd::ContextPoolHolder context(tdbb, &request->getPool());

	const DsqlCompiledStatement* statement = request->getStatement();

	if (option & DSQL_drop)
	{
		// Release everything associated with the request
		dsql_req::destroy(tdbb, request, true);
	}
	/*
	else if (option & DSQL_unprepare)
	{
		// Release everything but the request itself
		dsql_req::destroy(tdbb, request, false);
	}
	*/
	else if (option & DSQL_close)
	{
		// Just close the cursor associated with the request
		if (reqTypeWithCursor(statement->getType()))
		{
			if (!(request->req_flags & dsql_req::FLAG_OPENED_CURSOR))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-501) <<
						  Arg::Gds(isc_dsql_cursor_close_err));
			}

			close_cursor(tdbb, request);
		}
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
dsql_req* DSQL_prepare(thread_db* tdbb,
					   Attachment* attachment, jrd_tra* transaction,
					   ULONG length, const TEXT* string, USHORT dialect,
					   Array<UCHAR>* items, Array<UCHAR>* buffer,
					   bool isInternalRequest)
{
	SET_TDBB(tdbb);

	dsql_dbb* database = init(tdbb, attachment);
	dsql_req* request = NULL;

	try
	{
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
		else
		{
			parser_version = dialect % 10;
			dialect /= 10;
		}

		// Allocate a new request block and then prepare the request.

		request = prepareRequest(tdbb, database, transaction, length, string, dialect, parser_version,
			isInternalRequest);

		// Can not prepare a CREATE DATABASE/SCHEMA statement

		const DsqlCompiledStatement* statement = request->getStatement();
		if (statement->getType() == DsqlCompiledStatement::TYPE_CREATE_DB)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-530) <<
					  Arg::Gds(isc_dsql_crdb_prepare_err));
		}

		if (items && buffer)
		{
			Jrd::ContextPoolHolder context(tdbb, &request->getPool());
			sql_info(tdbb, request, items->getCount(), items->begin(),
				buffer->getCount(), buffer->begin());
		}

		return request;
	}
	catch (const Firebird::Exception&)
	{
		if (request)
		{
			Jrd::ContextPoolHolder context(tdbb, &request->getPool());
			dsql_req::destroy(tdbb, request, true);
		}
		throw;
	}
}


// Set a cursor name for a dynamic request.
void DsqlDmlRequest::setCursor(thread_db* tdbb, const TEXT* name)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &getPool());

	const size_t MAX_CURSOR_LENGTH = 132 - 1;
	string cursor = name;

	if (cursor.hasData() && cursor[0] == '\"')
	{
		// Quoted cursor names eh? Strip'em.
		// Note that "" will be replaced with ".
		// The code is very strange, because it doesn't check for "" really
		// and thus deletes one isolated " in the middle of the cursor.
		for (string::iterator i = cursor.begin(); i < cursor.end(); ++i)
		{
			if (*i == '\"')
				cursor.erase(i);
		}
	}
	else	// not quoted name
	{
		const string::size_type i = cursor.find(' ');
		if (i != string::npos)
			cursor.resize(i);

		cursor.upper();
	}

	USHORT length = (USHORT) fb_utils::name_length(cursor.c_str());

	if (length == 0)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_invalid));
	}

	if (length > MAX_CURSOR_LENGTH)
		length = MAX_CURSOR_LENGTH;

	cursor.resize(length);

	// If there already is a different cursor by the same name, bitch

	dsql_req* const* symbol = req_dbb->dbb_cursors.get(cursor);
	if (symbol)
	{
		if (this == *symbol)
			return;

		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_redefined) << cursor);
	}

	// If there already is a cursor and its name isn't the same, ditto.
	// We already know there is no cursor by this name in the hash table

	if (req_cursor.isEmpty())
	{
		req_cursor = cursor;
		req_dbb->dbb_cursors.put(cursor, this);
	}
	else
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_redefined) << req_cursor);
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
				   ULONG item_length, const UCHAR* items,
				   ULONG info_length, UCHAR* info)
{
	SET_TDBB(tdbb);

	Jrd::ContextPoolHolder context(tdbb, &request->getPool());

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

	Jrd::Attachment* attachment = request->req_dbb->dbb_attachment;
	const DsqlCompiledStatement* statement = request->getStatement();

	if (request->req_request)
	{
		ThreadStatusGuard status_vector(tdbb);
		try
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

			JRD_unwind_request(tdbb, request->req_request);
		}
		catch (Firebird::Exception&)
		{
		}
	}

	request->req_flags &= ~dsql_req::FLAG_OPENED_CURSOR;
	TRA_unlink_cursor(request->req_transaction, request);
}


// Common part of prepare and execute a statement.
void DSQL_execute_immediate(thread_db* tdbb, Jrd::Attachment* attachment, jrd_tra** tra_handle,
	ULONG length, const TEXT* string, USHORT dialect,
	Firebird::IMessageMetadata* in_meta, const UCHAR* in_msg,
	Firebird::IMessageMetadata* out_meta, UCHAR* out_msg,
	bool isInternalRequest)
{
	SET_TDBB(tdbb);

	dsql_dbb* const database = init(tdbb, attachment);
	dsql_req* request = NULL;

	try
	{
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
		//  parser = (combined) % 10 == 1
		//  dialect = (combined) / 19 == 1
		//
		// If the parser version is not part of the dialect, then assume that the
		// connection being made is a local classic connection.

		USHORT parser_version;
		if ((dialect / 10) == 0)
			parser_version = 2;
		else
		{
			parser_version = dialect % 10;
			dialect /= 10;
		}

		request = prepareRequest(tdbb, database, *tra_handle, length, string, dialect,
			parser_version, isInternalRequest);

		const DsqlCompiledStatement* statement = request->getStatement();

		// Only allow NULL trans_handle if we're starting a transaction

		if (!*tra_handle && statement->getType() != DsqlCompiledStatement::TYPE_START_TRANS)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_bad_trans_handle));
		}

		Jrd::ContextPoolHolder context(tdbb, &request->getPool());

		// A select with a non zero output length is a singleton select
		const bool singleton = reqTypeWithCursor(statement->getType()) && out_msg;

		request->req_transaction = *tra_handle;

		request->execute(tdbb, tra_handle, in_meta, in_msg, out_meta, out_msg, singleton);

		dsql_req::destroy(tdbb, request, true);
	}
	catch (const Firebird::Exception&)
	{
		if (request)
		{
			Jrd::ContextPoolHolder context(tdbb, &request->getPool());
			dsql_req::destroy(tdbb, request, true);
		}
		throw;
	}
}


void DsqlDmlRequest::dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
	ntrace_result_t* traceResult)
{
	node = Node::doDsqlPass(scratch, node);

	if (scratch->clientDialect > SQL_DIALECT_V5)
		scratch->getStatement()->setBlrVersion(5);
	else
		scratch->getStatement()->setBlrVersion(4);

	GEN_request(scratch, node);

	// Create the messages buffers
	for (size_t i = 0; i < scratch->ports.getCount(); ++i)
	{
		dsql_msg* message = scratch->ports[i];

		// Allocate buffer for message
		const ULONG newLen = message->msg_length + FB_DOUBLE_ALIGN - 1;
		UCHAR* msgBuffer = FB_NEW(*tdbb->getDefaultPool()) UCHAR[newLen];
		msgBuffer = (UCHAR*) FB_ALIGN((U_IPTR) msgBuffer, FB_DOUBLE_ALIGN);
		message->msg_buffer_number = req_msg_buffers.add(msgBuffer);
	}

	// have the access method compile the statement

#ifdef DSQL_DEBUG
	if (DSQL_debug & 64)
	{
		dsql_trace("Resulting BLR code for DSQL:");
		gds__trace_raw("Statement:\n");
		gds__trace_raw(statement->getSqlText()->c_str(), statement->getSqlText()->length());
		gds__trace_raw("\nBLR:\n");
		fb_print_blr(scratch->getBlrData().begin(),
			(ULONG) scratch->getBlrData().getCount(),
			gds__trace_printer, 0, 0);
	}
#endif

	ISC_STATUS_ARRAY localStatus;
	MOVE_CLEAR(localStatus, sizeof(localStatus));

	// check for warnings
	if (tdbb->tdbb_status_vector[2] == isc_arg_warning)
	{
		// save a status vector
		memcpy(localStatus, tdbb->tdbb_status_vector, sizeof(ISC_STATUS_ARRAY));
		fb_utils::init_status(tdbb->tdbb_status_vector);
	}

	ISC_STATUS status = FB_SUCCESS;

	try
	{
		JRD_compile(tdbb, scratch->getAttachment()->dbb_attachment, &req_request,
			scratch->getBlrData().getCount(), scratch->getBlrData().begin(),
			statement->getSqlText(),
			scratch->getDebugData().getCount(), scratch->getDebugData().begin(),
			(scratch->flags & DsqlCompilerScratch::FLAG_INTERNAL_REQUEST));
	}
	catch (const Exception&)
	{
		status = tdbb->tdbb_status_vector[1];
		*traceResult = (status == isc_no_priv ? res_unauthorized : res_failed);
	}

	// restore warnings (if there are any)
	if (localStatus[2] == isc_arg_warning)
	{
		size_t indx, len, warning;

		// find end of a status vector
		PARSE_STATUS(tdbb->tdbb_status_vector, indx, warning);
		if (indx)
			--indx;

		// calculate length of saved warnings
		PARSE_STATUS(localStatus, len, warning);
		len -= 2;

		if ((len + indx - 1) < ISC_STATUS_LENGTH)
			memcpy(&tdbb->tdbb_status_vector[indx], &localStatus[2], sizeof(ISC_STATUS) * len);
	}

	// free blr memory
	scratch->getBlrData().free();

	if (status)
		status_exception::raise(tdbb->tdbb_status_vector);
}

// Execute a dynamic SQL statement.
void DsqlDmlRequest::execute(thread_db* tdbb, jrd_tra** traHandle,
	Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
	Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
	bool singleton)
{
	// If there is no data required, just start the request

	const dsql_msg* message = statement->getSendMsg();
	if (message)
		map_in_out(this, false, message, inMetadata, NULL, inMsg);

	// we need to map_in_out before tracing of execution start to let trace
	// manager know statement parameters values
	TraceDSQLExecute trace(req_dbb->dbb_attachment, this);

	if (!message)
		JRD_start(tdbb, req_request, req_transaction);
	else
	{
		UCHAR* msgBuffer = req_msg_buffers[message->msg_buffer_number];
		JRD_start_and_send(tdbb, req_request, req_transaction, message->msg_number,
			message->msg_length, msgBuffer);
	}

	// Selectable execute block should get the "proc fetch" flag assigned,
	// which ensures that the savepoint stack is preserved while suspending
	if (statement->getType() == DsqlCompiledStatement::TYPE_SELECT_BLOCK)
	{
		fb_assert(req_request);
		req_request->req_flags |= req_proc_fetch;
	}

	// TYPE_EXEC_BLOCK has no outputs so there are no out_msg
	// supplied from client side, but TYPE_EXEC_BLOCK requires
	// 2-byte message for EOS synchronization
	const bool isBlock = (statement->getType() == DsqlCompiledStatement::TYPE_EXEC_BLOCK);

	message = statement->getReceiveMsg();

	if (outMetadata == DELAYED_OUT_FORMAT)
	{
		needDelayedFormat = true;
		outMetadata = NULL;
	}

	if (outMetadata && message)
		parse_metadata(this, outMetadata, message->msg_parameters);

	if ((outMsg && message) || isBlock)
	{
		UCHAR temp_buffer[FB_DOUBLE_ALIGN * 2];
		dsql_msg temp_msg(*getDefaultMemoryPool());

		// Insure that the metadata for the message is parsed, regardless of
		// whether anything is found by the call to receive.

		UCHAR* msgBuffer = req_msg_buffers[message->msg_buffer_number];

		if (!outMetadata && isBlock)
		{
			message = &temp_msg;
			temp_msg.msg_number = 1;
			temp_msg.msg_length = 2;
			msgBuffer = (UCHAR*) FB_ALIGN((U_IPTR) temp_buffer, FB_DOUBLE_ALIGN);
		}

		JRD_receive(tdbb, req_request, message->msg_number, message->msg_length, msgBuffer);

		if (outMsg)
			map_in_out(this, true, message, NULL, outMsg);

		// if this is a singleton select, make sure there's in fact one record

		if (singleton)
		{
			USHORT counter;

			// Create a temp message buffer and try two more receives.
			// If both succeed then the first is the next record and the
			// second is either another record or the end of record message.
			// In either case, there's more than one record.

			UCHAR* message_buffer = (UCHAR*) gds__alloc(message->msg_length);

			ISC_STATUS status = FB_SUCCESS;
			ISC_STATUS_ARRAY localStatus;

			for (counter = 0; counter < 2 && !status; counter++)
			{
				AutoSetRestore<ISC_STATUS*> autoStatus(&tdbb->tdbb_status_vector, localStatus);
				fb_utils::init_status(localStatus);

				try
				{
					JRD_receive(tdbb, req_request, message->msg_number,
						message->msg_length, message_buffer);
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

	switch (statement->getType())
	{
		case DsqlCompiledStatement::TYPE_UPDATE_CURSOR:
			if (!req_request->req_records_updated)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-913) <<
						  Arg::Gds(isc_deadlock) <<
						  Arg::Gds(isc_update_conflict));
			}
			break;

		case DsqlCompiledStatement::TYPE_DELETE_CURSOR:
			if (!req_request->req_records_deleted)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-913) <<
						  Arg::Gds(isc_deadlock) <<
						  Arg::Gds(isc_update_conflict));
			}
			break;
	}

	const bool have_cursor = reqTypeWithCursor(statement->getType()) && !singleton;
	trace.finish(have_cursor, res_successful);
}

void DsqlDdlRequest::dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
	ntrace_result_t* traceResult)
{
	internalScratch = scratch;

	scratch->flags |= DsqlCompilerScratch::FLAG_DDL;

	try
	{
		node = Node::doDsqlPass(scratch, node);
	}
	catch (status_exception& ex)
	{
		rethrowDdlException(ex, false);
	}

	if (scratch->getAttachment()->dbb_read_only)
		ERRD_post(Arg::Gds(isc_read_only_database));

	if ((scratch->flags & DsqlCompilerScratch::FLAG_AMBIGUOUS_STMT) &&
		scratch->getAttachment()->dbb_db_SQL_dialect != scratch->clientDialect)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-817) <<
				  Arg::Gds(isc_ddl_not_allowed_by_db_sql_dial) <<
					  Arg::Num(scratch->getAttachment()->dbb_db_SQL_dialect));
	}

	if (scratch->clientDialect > SQL_DIALECT_V5)
		scratch->getStatement()->setBlrVersion(5);
	else
		scratch->getStatement()->setBlrVersion(4);
}

// Execute a dynamic SQL statement.
void DsqlDdlRequest::execute(thread_db* tdbb, jrd_tra** traHandle,
	Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
	Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
	bool singleton)
{
	TraceDSQLExecute trace(req_dbb->dbb_attachment, this);

	fb_utils::init_status(tdbb->tdbb_status_vector);

	// run all statements under savepoint control
	{	// scope
		AutoSavePoint savePoint(tdbb, req_transaction);

		try
		{
			node->executeDdl(tdbb, internalScratch, req_transaction);
		}
		catch (status_exception& ex)
		{
			rethrowDdlException(ex, true);
		}

		savePoint.release();	// everything is ok
	}

	JRD_autocommit_ddl(tdbb, req_transaction);

	trace.finish(false, res_successful);
}

// Rethrow an exception with isc_no_meta_update and prefix codes.
void DsqlDdlRequest::rethrowDdlException(status_exception& ex, bool metadataUpdate)
{
	Arg::StatusVector newVector;

	if (metadataUpdate)
		newVector << Arg::Gds(isc_no_meta_update);

	node->putErrorPrefix(newVector);

	const ISC_STATUS* status = ex.value();

	if (status[1] == isc_no_meta_update)
		status += 2;

	newVector.append(Arg::StatusVector(status));

	status_exception::raise(newVector);
}


void DsqlTransactionRequest::dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
	ntrace_result_t* /*traceResult*/)
{
	node = Node::doDsqlPass(scratch, node);

	// Don't trace pseudo-statements (without requests associated).
	req_traced = false;
}

// Execute a dynamic SQL statement.
void DsqlTransactionRequest::execute(thread_db* tdbb, jrd_tra** traHandle,
	Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
	Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
	bool singleton)
{
	node->execute(tdbb, this, traHandle);
}


/**

 	get_request_info

    @brief	Get the records updated/deleted for record


    @param request
    @param buffer_length
    @param buffer

 **/
static ULONG get_request_info(thread_db* tdbb, dsql_req* request, ULONG buffer_length, UCHAR* buffer)
{
	if (!request->req_request)	// DDL
		return 0;

	// get the info for the request from the engine

	try
	{
		return INF_request_info(request->req_request, sizeof(record_info), record_info,
			buffer_length, buffer);
	}
	catch (Firebird::Exception&)
	{
		return 0;
	}
}


/**

 	init

    @brief	Initialize dynamic SQL.  This is called only once.


    @param db_handle

 **/
static dsql_dbb* init(thread_db* tdbb, Jrd::Attachment* attachment)
{
	SET_TDBB(tdbb);

	if (attachment->att_dsql_instance)
		return attachment->att_dsql_instance;

	MemoryPool& pool = *attachment->createPool();
	dsql_dbb* const database = FB_NEW(pool) dsql_dbb(pool);
	database->dbb_attachment = attachment;
	attachment->att_dsql_instance = database;

	INI_init_dsql(tdbb, database);

	Database* const dbb = tdbb->getDatabase();
	database->dbb_db_SQL_dialect =
		(dbb->dbb_flags & DBB_DB_SQL_dialect_3) ? SQL_DIALECT_V6 : SQL_DIALECT_V5;

	database->dbb_ods_version = dbb->dbb_ods_version;
	database->dbb_minor_version = dbb->dbb_minor_version;

	database->dbb_read_only = dbb->readOnly();

#ifdef DSQL_DEBUG
	DSQL_debug = Config::getTraceDSQL();
#endif

	return attachment->att_dsql_instance;
}


/**

 	map_in_out

    @brief	Map data from external world into message or
 	from message to external world.


    @param request
    @param toExternal
    @param message
    @param meta
    @param dsql_msg_buf
    @param in_dsql_msg_buf

 **/
static void map_in_out(dsql_req* request, bool toExternal, const dsql_msg* message,
	IMessageMetadata* meta, UCHAR* dsql_msg_buf, const UCHAR* in_dsql_msg_buf)
{
	thread_db* tdbb = JRD_get_thread_data();

	USHORT count = parse_metadata(request, meta, message->msg_parameters);

	bool err = false;

	for (size_t i = 0; i < message->msg_parameters.getCount(); ++i)
	{
		dsql_par* parameter = message->msg_parameters[i];

		if (parameter->par_index)
		{
			 // Make sure the message given to us is long enough

			dsc desc;
			if (!request->req_user_descs.get(parameter, desc))
				desc.clear();

			//ULONG length = (IPTR) desc.dsc_address + desc.dsc_length;

			if (/*length > msg_length || */!desc.dsc_dtype)
			{
				err = true;
				break;
			}

			UCHAR* msgBuffer = request->req_msg_buffers[parameter->par_message->msg_buffer_number];

			SSHORT* flag = NULL;
			dsql_par* const null_ind = parameter->par_null;
			if (null_ind != NULL)
			{
				dsc userNullDesc;
				if (!request->req_user_descs.get(null_ind, userNullDesc))
					userNullDesc.clear();

				const ULONG null_offset = (IPTR) userNullDesc.dsc_address;

				/*
				length = null_offset + sizeof(SSHORT);
				if (length > msg_length)
				{
					err = true;
					break;
				}
				*/

				dsc nullDesc = null_ind->par_desc;
				nullDesc.dsc_address = msgBuffer + (IPTR) nullDesc.dsc_address;

				if (toExternal)
				{
					flag = reinterpret_cast<SSHORT*>(dsql_msg_buf + null_offset);
					*flag = *reinterpret_cast<const SSHORT*>(nullDesc.dsc_address);
				}
				else
				{
					flag = reinterpret_cast<SSHORT*>(nullDesc.dsc_address);
					*flag = *reinterpret_cast<const SSHORT*>(in_dsql_msg_buf + null_offset);
				}
			}

			dsc parDesc = parameter->par_desc;
			parDesc.dsc_address = msgBuffer + (IPTR) parDesc.dsc_address;

			if (toExternal)
			{
				desc.dsc_address = dsql_msg_buf + (IPTR) desc.dsc_address;

				if (!flag || *flag >= 0)
					MOVD_move(tdbb, &parDesc, &desc);
				else
					memset(desc.dsc_address, 0, desc.dsc_length);
			}
			else if (!flag || *flag >= 0)
			{
				if (!(parDesc.dsc_flags & DSC_null))
				{
					// Safe cast because desc is used as source only.
					desc.dsc_address = const_cast<UCHAR*>(in_dsql_msg_buf) + (IPTR) desc.dsc_address;
					MOVD_move(tdbb, &desc, &parDesc);
				}
			}
			else
				memset(parDesc.dsc_address, 0, parDesc.dsc_length);

			count--;
		}
	}

	// If we got here because the loop was exited early or if part of the
	// message given to us hasn't been used, complain.

	if (err || count)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
				  << Arg::Gds(isc_random) << (err ? "Message buffer too short" : "Wrong number of message parameters")
#endif
				  );
	}

	const DsqlCompiledStatement* statement = request->getStatement();
	const dsql_par* parameter;

	const dsql_par* dbkey;
	if (!toExternal && (dbkey = statement->getParentDbKey()) &&
		(parameter = statement->getDbKey()))
	{
		UCHAR* parentMsgBuffer = statement->getParentRequest() ?
			statement->getParentRequest()->req_msg_buffers[dbkey->par_message->msg_buffer_number] : NULL;
		UCHAR* msgBuffer = request->req_msg_buffers[parameter->par_message->msg_buffer_number];

		fb_assert(parentMsgBuffer);

		dsc parentDesc = dbkey->par_desc;
		parentDesc.dsc_address = parentMsgBuffer + (IPTR) parentDesc.dsc_address;

		dsc desc = parameter->par_desc;
		desc.dsc_address = msgBuffer + (IPTR) desc.dsc_address;

		MOVD_move(tdbb, &parentDesc, &desc);

		dsql_par* null_ind = parameter->par_null;
		if (null_ind != NULL)
		{
			desc = null_ind->par_desc;
			desc.dsc_address = msgBuffer + (IPTR) desc.dsc_address;

			SSHORT* flag = (SSHORT*) desc.dsc_address;
			*flag = 0;
		}
	}

	const dsql_par* rec_version;
	if (!toExternal && (rec_version = statement->getParentRecVersion()) &&
		(parameter = statement->getRecVersion()))
	{
		UCHAR* parentMsgBuffer = statement->getParentRequest() ?
			statement->getParentRequest()->req_msg_buffers[rec_version->par_message->msg_buffer_number] :
			NULL;
		UCHAR* msgBuffer = request->req_msg_buffers[parameter->par_message->msg_buffer_number];

		fb_assert(parentMsgBuffer);

		dsc parentDesc = rec_version->par_desc;
		parentDesc.dsc_address = parentMsgBuffer + (IPTR) parentDesc.dsc_address;

		dsc desc = parameter->par_desc;
		desc.dsc_address = msgBuffer + (IPTR) desc.dsc_address;

		MOVD_move(tdbb, &parentDesc, &desc);

		dsql_par* null_ind = parameter->par_null;
		if (null_ind != NULL)
		{
			desc = null_ind->par_desc;
			desc.dsc_address = msgBuffer + (IPTR) desc.dsc_address;

			SSHORT* flag = (SSHORT*) desc.dsc_address;
			*flag = 0;
		}
	}
}


/**

 	parse_metadata

    @brief	Parse the message of a request.


    @param request
    @param meta
    @param parameters_list

 **/
static USHORT parse_metadata(dsql_req* request, IMessageMetadata* meta,
	const Array<dsql_par*>& parameters_list)
{
	HalfStaticArray<const dsql_par*, 16> parameters;

	for (size_t i = 0; i < parameters_list.getCount(); ++i)
	{
		dsql_par* param = parameters_list[i];

		if (param->par_index)
		{
			if (param->par_index > parameters.getCount())
				parameters.grow(param->par_index);
			fb_assert(!parameters[param->par_index - 1]);
			parameters[param->par_index - 1] = param;
		}
	}

	// If there's no metadata, then the format of the current message buffer
	// is identical to the format of the previous one.

	if (!meta)
		return parameters.getCount();

	LocalStatus st;
	unsigned count = meta->getCount(&st);
	check(&st);

	if (count != parameters.getCount())
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
				  << Arg::Gds(isc_random) << "Wrong number of message parameters"
#endif
				  );
	}

	unsigned offset = 0;

	for (USHORT index = 0; index < count; index++)
	{
		unsigned sqlType = meta->getType(&st, index);
		check(&st);
		unsigned sqlLength = meta->getLength(&st, index);
		check(&st);

		dsc desc;
		desc.dsc_flags = 0;

		unsigned dataOffset, nullOffset, dtype, dlength;
		offset = fb_utils::sqlTypeToDsc(offset, sqlType, sqlLength,
			&dtype, &dlength, &dataOffset, &nullOffset);
		desc.dsc_dtype = dtype;
		desc.dsc_length = dlength;

		desc.dsc_scale = meta->getScale(&st, index);
		check(&st);
		desc.dsc_sub_type = meta->getSubType(&st, index);
		check(&st);
		unsigned textType = meta->getCharSet(&st, index);
		check(&st);
		desc.setTextType(textType);
		desc.dsc_address = (UCHAR*)(IPTR) dataOffset;

		const dsql_par* const parameter = parameters[index];
		fb_assert(parameter);

		// ASF: Older than 2.5 engine hasn't validating strings in DSQL. After this has been
		// implemented in 2.5, selecting a NONE column with UTF-8 attachment charset started
		// failing. The real problem is that the client encodes SQL_TEXT/SQL_VARYING using
		// blr_text/blr_varying (i.e. with the connection charset). I'm reseting the charset
		// here at the server as a way to make older (and not yet changed) client work
		// correctly.
		if (desc.isText() && desc.getTextType() == ttype_dynamic)
			desc.setTextType(ttype_none);

		request->req_user_descs.put(parameter, desc);

		dsql_par* null = parameter->par_null;
		if (null)
		{
			desc.clear();
			desc.dsc_dtype = dtype_short;
			desc.dsc_scale = 0;
			desc.dsc_length = sizeof(SSHORT);
			desc.dsc_address = (UCHAR*)(IPTR) nullOffset;

			request->req_user_descs.put(null, desc);
		}
	}

	return count;
}


// raise error if one present
static void check(IStatus* st)
{
	if (!st->isSuccess())
	{
		ERRD_post(Arg::StatusVector(st->get()));
	}
}


// Prepare a request for execution. Return SQL status code.
// Note: caller is responsible for pool handling.
static dsql_req* prepareRequest(thread_db* tdbb, dsql_dbb* database, jrd_tra* transaction,
	ULONG textLength, const TEXT* text, USHORT clientDialect, USHORT parserVersion,
	bool isInternalRequest)
{
	return prepareStatement(tdbb, database, transaction, textLength, text, clientDialect,
		parserVersion, isInternalRequest);
}


// Prepare a statement for execution. Return SQL status code.
// Note: caller is responsible for pool handling.
static dsql_req* prepareStatement(thread_db* tdbb, dsql_dbb* database, jrd_tra* transaction,
	ULONG textLength, const TEXT* text, USHORT clientDialect, USHORT parserVersion,
	bool isInternalRequest)
{
	if (text && textLength == 0)
		textLength = static_cast<ULONG>(strlen(text));

	textLength = MIN(textLength, MAX_SQL_LENGTH);

	TraceDSQLPrepare trace(database->dbb_attachment, transaction, textLength, text);

	if (clientDialect > SQL_DIALECT_CURRENT)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_wish_list));
	}

	if (!text || textLength == 0)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Unexpected end of command
				  // CVC: Nothing will be line 1, column 1 for the user.
				  Arg::Gds(isc_command_end_err2) << Arg::Num(1) << Arg::Num(1));
	}

	// Get rid of the trailing ";" if there is one.

	for (const TEXT* p = text + textLength; p-- > text;)
	{
		if (*p != ' ')
		{
			if (*p == ';')
				textLength = p - text;
			break;
		}
	}

	// allocate the statement block, then prepare the statement

	Jrd::ContextPoolHolder context(tdbb, database->createPool());
	MemoryPool& pool = *tdbb->getDefaultPool();

	DsqlCompiledStatement* statement = FB_NEW(pool) DsqlCompiledStatement(pool);
	DsqlCompilerScratch* scratch = FB_NEW(pool) DsqlCompilerScratch(pool, database,
		transaction, statement);
	scratch->clientDialect = clientDialect;

	if (isInternalRequest)
		scratch->flags |= DsqlCompilerScratch::FLAG_INTERNAL_REQUEST;

	dsql_req* request = NULL;

	try
	{
		// Parse the SQL statement.  If it croaks, return

		Parser parser(*tdbb->getDefaultPool(), scratch, clientDialect,
			scratch->getAttachment()->dbb_db_SQL_dialect, parserVersion, text, textLength,
			tdbb->getAttachment()->att_charset);

		request = parser.parse();

		request->req_dbb = scratch->getAttachment();
		request->req_transaction = scratch->getTransaction();
		request->statement = scratch->getStatement();

		if (parser.isStmtAmbiguous())
			scratch->flags |= DsqlCompilerScratch::FLAG_AMBIGUOUS_STMT;

		string transformedText = parser.getTransformedString();
		SSHORT charSetId = database->dbb_attachment->att_charset;

		// If the attachment charset is NONE, replace non-ASCII characters by question marks, so
		// that engine internals doesn't receive non-mappeable data to UTF8. If an attachment
		// charset is used, validate the string.
		if (charSetId == CS_NONE)
		{
			for (char* p = transformedText.begin(), *end = transformedText.end(); p < end; ++p)
			{
				if (UCHAR(*p) > 0x7F)
					*p = '?';
			}
		}
		else
		{
			CharSet* charSet = INTL_charset_lookup(tdbb, charSetId);

			if (!charSet->wellFormed(transformedText.length(),
					(const UCHAR*) transformedText.begin(), NULL))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_malformed_string));
			}

			UCharBuffer temp;

			CsConvert conversor(charSet->getStruct(),
				INTL_charset_lookup(tdbb, CS_METADATA)->getStruct());
			conversor.convert(transformedText.length(), (const UCHAR*) transformedText.c_str(), temp);

			transformedText.assign(temp.begin(), temp.getCount());
		}

		statement->setSqlText(FB_NEW(pool) RefString(pool, transformedText));

		// allocate the send and receive messages

		statement->setSendMsg(FB_NEW(pool) dsql_msg(pool));
		dsql_msg* message = FB_NEW(pool) dsql_msg(pool);
		statement->setReceiveMsg(message);
		message->msg_number = 1;

		statement->setType(DsqlCompiledStatement::TYPE_SELECT);

		request->req_traced = true;
		trace.setStatement(request);

		ntrace_result_t traceResult = res_successful;
		try
		{
			request->dsqlPass(tdbb, scratch, &traceResult);
		}
		catch (const Exception&)
		{
			trace.prepare(traceResult);
			throw;
		}
		trace.prepare(traceResult);

		return request;
	}
	catch (const Firebird::Exception&)
	{
		trace.prepare(res_failed);

		if (request)
		{
			request->req_traced = false;
			dsql_req::destroy(tdbb, request, true);
		}

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

 **/
static UCHAR* put_item(	UCHAR	item,
						const USHORT	length,
						const UCHAR* string,
						UCHAR*	ptr,
						const UCHAR* const end)
{
	if (ptr + length + 3 >= end)
	{
		*ptr = isc_info_truncated;
		return NULL;
	}

	*ptr++ = item;

	*ptr++ = (UCHAR) length;
	*ptr++ = length >> 8;

	if (length)
		memcpy(ptr, string, length);

	return ptr + length;
}


// Release a compiled statement.
static void release_statement(DsqlCompiledStatement* statement)
{
	if (statement->getParentRequest())
	{
		dsql_req* parent = statement->getParentRequest();

		size_t pos;
		if (parent->cursors.find(statement, pos))
			parent->cursors.remove(pos);

		statement->setParentRequest(NULL);
	}

	statement->setSqlText(NULL);
}


dsql_req::dsql_req(MemoryPool& pool)
	: req_pool(pool),
	  statement(NULL),
	  cursors(req_pool),
	  req_dbb(NULL),
	  req_transaction(NULL),
	  req_msg_buffers(req_pool),
	  req_cursor(req_pool),
	  req_user_descs(req_pool),
	  req_traced(false),
	  req_interface(NULL)
{
}

void dsql_req::setCursor(thread_db* /*tdbb*/, const TEXT* /*name*/)
{
	status_exception::raise(
		Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		Arg::Gds(isc_dsql_sqlda_err) <<
		Arg::Gds(isc_req_sync));
}

void dsql_req::setDelayedFormat(thread_db* /*tdbb*/, Firebird::IMessageMetadata* /*metadata*/)
{
	status_exception::raise(
		Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		Arg::Gds(isc_dsql_sqlda_err) <<
		Arg::Gds(isc_req_sync));
}

bool dsql_req::fetch(thread_db* /*tdbb*/, UCHAR* /*msgBuffer*/)
{
	status_exception::raise(
		Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		Arg::Gds(isc_dsql_sqlda_err) <<
		Arg::Gds(isc_req_sync));

	return false;	// avoid warning
}

// Release a dynamic request.
void dsql_req::destroy(thread_db* tdbb, dsql_req* request, bool drop)
{
	SET_TDBB(tdbb);

	// If request is parent, orphan the children and release a portion of their requests

	for (size_t i = 0; i < request->cursors.getCount(); ++i)
	{
		DsqlCompiledStatement* child = request->cursors[i];
		child->addFlags(DsqlCompiledStatement::FLAG_ORPHAN);
		child->setParentRequest(NULL);

		// hvlad: lines below is commented out as
		// - child is already unlinked from its parent request
		// - we should not free child's sql text until its owner request is alive
		// It seems to me we should destroy owner request here, not a child
		// statement - as it always was before

		//Jrd::ContextPoolHolder context(tdbb, &child->getPool());
		//release_statement(child);
	}

	// If the request had an open cursor, close it

	if (request->req_flags & dsql_req::FLAG_OPENED_CURSOR)
		close_cursor(tdbb, request);

	Jrd::Attachment* att = request->req_dbb->dbb_attachment;
	const bool need_trace_free = request->req_traced && TraceManager::need_dsql_free(att);
	if (need_trace_free)
	{
		TraceSQLStatementImpl stmt(request, NULL);
		TraceManager::event_dsql_free(att, &stmt, DSQL_drop);
	}
	request->req_traced = false;

	if (request->req_cursor.hasData())
	{
		request->req_dbb->dbb_cursors.remove(request->req_cursor);
		request->req_cursor = "";
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

	const DsqlCompiledStatement* statement = request->getStatement();
	release_statement(const_cast<DsqlCompiledStatement*>(statement));

	// Release the entire request if explicitly asked for

	if (drop)
	{
		request->req_dbb->deletePool(&request->getPool());
	}
}


// Return as UTF8
string IntlString::toUtf8(DsqlCompilerScratch* dsqlScratch) const
{
	CHARSET_ID id = CS_dynamic;

	if (charset.hasData())
	{
		const dsql_intlsym* resolved = METD_get_charset(dsqlScratch->getTransaction(),
			charset.length(), charset.c_str());

		if (!resolved)
		{
			// character set name is not defined
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_charset_not_found) << charset);
		}

		id = resolved->intlsym_charset_id;
	}

	string utf;
	return DataTypeUtil::convertToUTF8(s, utf, id) ? utf : s;
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
					 ULONG item_length,
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

	if (*items == isc_info_length)
	{
		start_info = info;
		items++;
	}
	else
		start_info = NULL;

	// CVC: Is it the idea that this pointer remains with its previous value
	// in the loop or should it be made NULL in each iteration?
	const dsql_msg* message = NULL;
	bool messageFound = false;
	USHORT first_index = 0;

	const DsqlCompiledStatement* statement = request->getStatement();

	while (items < end_items && *items != isc_info_end)
	{
		ULONG length;
		USHORT number;
		ULONG value;
		const UCHAR item = *items++;

		switch (item)
		{
		case isc_info_sql_select:
		case isc_info_sql_bind:
			message = (item == isc_info_sql_select) ?
				statement->getReceiveMsg() : statement->getSendMsg();
			messageFound = true;
			if (info + 1 >= end_info)
			{
				*info = isc_info_truncated;
				return;
			}
			*info++ = item;
			break;

		case isc_info_sql_stmt_flags:
			value = IStatement::FLAG_REPEAT_EXECUTE;
			switch (statement->getType())
			{
			case DsqlCompiledStatement::TYPE_CREATE_DB:
			case DsqlCompiledStatement::TYPE_DDL:
				value &= ~IStatement::FLAG_REPEAT_EXECUTE;
				break;
			case DsqlCompiledStatement::TYPE_SELECT:
			case DsqlCompiledStatement::TYPE_SELECT_UPD:
			case DsqlCompiledStatement::TYPE_SELECT_BLOCK:
				value |= IStatement::FLAG_HAS_CURSOR;
				break;
			}
			length = put_vax_long(buffer, value);
			info = put_item(item, length, buffer, info, end_info);
			if (!info)
				return;
			break;

		case isc_info_sql_stmt_type:
			switch (statement->getType())
			{
			case DsqlCompiledStatement::TYPE_SELECT:
				number = isc_info_sql_stmt_select;
				break;
			case DsqlCompiledStatement::TYPE_SELECT_UPD:
				number = isc_info_sql_stmt_select_for_upd;
				break;
			case DsqlCompiledStatement::TYPE_CREATE_DB:
			case DsqlCompiledStatement::TYPE_DDL:
			case DsqlCompiledStatement::TYPE_SET_ROLE:
				number = isc_info_sql_stmt_ddl;
				break;
			case DsqlCompiledStatement::TYPE_COMMIT:
			case DsqlCompiledStatement::TYPE_COMMIT_RETAIN:
				number = isc_info_sql_stmt_commit;
				break;
			case DsqlCompiledStatement::TYPE_ROLLBACK:
			case DsqlCompiledStatement::TYPE_ROLLBACK_RETAIN:
				number = isc_info_sql_stmt_rollback;
				break;
			case DsqlCompiledStatement::TYPE_START_TRANS:
				number = isc_info_sql_stmt_start_trans;
				break;
			case DsqlCompiledStatement::TYPE_INSERT:
				number = isc_info_sql_stmt_insert;
				break;
			case DsqlCompiledStatement::TYPE_UPDATE:
			case DsqlCompiledStatement::TYPE_UPDATE_CURSOR:
				number = isc_info_sql_stmt_update;
				break;
			case DsqlCompiledStatement::TYPE_DELETE:
			case DsqlCompiledStatement::TYPE_DELETE_CURSOR:
				number = isc_info_sql_stmt_delete;
				break;
			case DsqlCompiledStatement::TYPE_EXEC_PROCEDURE:
				number = isc_info_sql_stmt_exec_procedure;
				break;
			case DsqlCompiledStatement::TYPE_SET_GENERATOR:
				number = isc_info_sql_stmt_set_generator;
				break;
			case DsqlCompiledStatement::TYPE_SAVEPOINT:
				number = isc_info_sql_stmt_savepoint;
				break;
			case DsqlCompiledStatement::TYPE_EXEC_BLOCK:
				number = isc_info_sql_stmt_exec_procedure;
				break;
			case DsqlCompiledStatement::TYPE_SELECT_BLOCK:
				number = isc_info_sql_stmt_select;
				break;
			default:
				number = 0;
				break;
			}
			length = put_vax_long(buffer, (SLONG) number);
			info = put_item(item, length, buffer, info, end_info);
			if (!info)
				return;
			break;

		case isc_info_sql_sqlda_start:
			length = *items++;
			first_index = static_cast<USHORT>(gds__vax_integer(items, length));
			items += length;
			break;

		case isc_info_sql_batch_fetch:
			if (statement->getFlags() & DsqlCompiledStatement::FLAG_NO_BATCH)
				number = 0;
			else
				number = 1;
			length = put_vax_long(buffer, (SLONG) number);
			if (!(info = put_item(item, length, buffer, info, end_info)))
				return;
			break;

		case isc_info_sql_records:
			length = get_request_info(tdbb, request, sizeof(buffer), buffer);
			if (length && !(info = put_item(item, length, buffer, info, end_info)))
				return;
			break;

		case isc_info_sql_get_plan:
		case isc_info_sql_explain_plan:
			{
				const bool detailed = (item == isc_info_sql_explain_plan);
				string plan = OPT_get_plan(tdbb, request->req_request, detailed);

				if (plan.hasData())
				{
					const ULONG buffer_length = end_info - info - 3; // 1-byte item + 2-byte length
					const ULONG max_length = MIN(buffer_length, MAX_USHORT);

					if (plan.length() > max_length)
					{
						// If the plan doesn't fit the supplied buffer or exceeds the API limits,
						// truncate it to the rightmost space and add ellipsis to the end
						plan.resize(max_length);

						while (plan.length() > max_length - 4)
						{
							const size_t pos = plan.find_last_of(' ');
							if (pos == string::npos)
								break;
							plan.resize(pos);
						}

						plan += " ...";
					}

					if (plan.length() > max_length)
					{
						// If it's still too long, give up
						fb_assert(info < end_info);
						*info = isc_info_truncated;
						info = NULL;
					}
					else
					{
						info = put_item(item, plan.length(), reinterpret_cast<const UCHAR*>(plan.c_str()),
										info, end_info);
					}
				}

				if (!info)
					return;
			}
			break;

		case isc_info_sql_num_variables:
		case isc_info_sql_describe_vars:
			if (messageFound)
			{
				number = message ? message->msg_index : 0;
				length = put_vax_long(buffer, (SLONG) number);
				if (!(info = put_item(item, length, buffer, info, end_info)))
					return;
				if (item == isc_info_sql_num_variables)
					continue;

				const UCHAR* end_describe = items;
				while (end_describe < end_items &&
					*end_describe != isc_info_end && *end_describe != isc_info_sql_describe_end)
				{
					end_describe++;
				}

				info = var_info(message, items, end_describe, info, end_info, first_index,
					message == statement->getSendMsg());
				if (!info)
					return;

				items = end_describe;
				if (*items == isc_info_sql_describe_end)
					items++;
				break;
			}
			// else fall into

		default:
			buffer[0] = item;
			length = 1 + put_vax_long(buffer + 1, (SLONG) isc_infunk);
			if (!(info = put_item(isc_info_error, length, buffer, info, end_info)))
				return;
		}
	}

	*info++ = isc_info_end;

	if (start_info && (end_info - info >= 7))
	{
		const SLONG number = info - start_info;
		fb_assert(number > 0);
		memmove(start_info + 7, start_info, number);
		USHORT length = put_vax_long(buffer, number);
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
static UCHAR* var_info(const dsql_msg* message,
					   const UCHAR* items,
					   const UCHAR* const end_describe,
					   UCHAR* info,
					   const UCHAR* const end,
					   USHORT first_index,
					   bool input_message)
{
	if (!message || !message->msg_index)
		return info;

	thread_db* tdbb = JRD_get_thread_data();
	Jrd::Attachment* attachment = tdbb->getAttachment();

	HalfStaticArray<const dsql_par*, 16> parameters;

	for (size_t i = 0; i < message->msg_parameters.getCount(); ++i)
	{
		const dsql_par* param = message->msg_parameters[i];

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
			SLONG sql_len, sql_sub_type, sql_scale, sql_type;
			param->par_desc.getSqlInfo(&sql_len, &sql_sub_type, &sql_scale, &sql_type);

			if (input_message &&
				(param->par_desc.dsc_dtype == dtype_text || param->par_is_text) &&
				(param->par_desc.dsc_flags & DSC_null))
			{
				sql_type = SQL_NULL;
				sql_len = 0;
				sql_sub_type = 0;
			}
			else if (param->par_desc.dsc_dtype == dtype_varying && param->par_is_text)
				sql_type = SQL_TEXT;

			if (sql_type && (param->par_desc.dsc_flags & DSC_nullable))
				sql_type |= 0x1;

			for (const UCHAR* describe = items; describe < end_describe;)
			{
				USHORT length;
				MetaName name;
				const UCHAR* buffer = buf;
				UCHAR item = *describe++;

				switch (item)
				{
				case isc_info_sql_sqlda_seq:
					length = put_vax_long(buf, (SLONG) param->par_index);
					break;

				case isc_info_sql_message_seq:
					length = 0;
					break;

				case isc_info_sql_type:
					length = put_vax_long(buf, (SLONG) sql_type);
					break;

				case isc_info_sql_sub_type:
					length = put_vax_long(buf, (SLONG) sql_sub_type);
					break;

				case isc_info_sql_scale:
					length = put_vax_long(buf, (SLONG) sql_scale);
					break;

				case isc_info_sql_length:
					length = put_vax_long(buf, (SLONG) sql_len);
					break;

				case isc_info_sql_null_ind:
					length = put_vax_long(buf, (SLONG) (sql_type & 1));
					break;

				case isc_info_sql_field:
					if (param->par_name.hasData())
					{
						name = attachment->nameToUserCharSet(tdbb, param->par_name);
						length = name.length();
						buffer = reinterpret_cast<const UCHAR*>(name.c_str());
					}
					else
						length = 0;
					break;

				case isc_info_sql_relation:
					if (param->par_rel_name.hasData())
					{
						name = attachment->nameToUserCharSet(tdbb, param->par_rel_name);
						length = name.length();
						buffer = reinterpret_cast<const UCHAR*>(name.c_str());
					}
					else
						length = 0;
					break;

				case isc_info_sql_owner:
					if (param->par_owner_name.hasData())
					{
						name = attachment->nameToUserCharSet(tdbb, param->par_owner_name);
						length = name.length();
						buffer = reinterpret_cast<const UCHAR*>(name.c_str());
					}
					else
						length = 0;
					break;

				case isc_info_sql_relation_alias:
					if (param->par_rel_alias.hasData())
					{
						name = attachment->nameToUserCharSet(tdbb, param->par_rel_alias);
						length = name.length();
						buffer = reinterpret_cast<const UCHAR*>(name.c_str());
					}
					else
						length = 0;
					break;

				case isc_info_sql_alias:
					if (param->par_alias.hasData())
					{
						name = attachment->nameToUserCharSet(tdbb, param->par_alias);
						length = name.length();
						buffer = reinterpret_cast<const UCHAR*>(name.c_str());
					}
					else
						length = 0;
					break;

				default:
					buf[0] = item;
					item = isc_info_error;
					length = 1 + put_vax_long(buf + 1, (SLONG) isc_infunk);
					break;
				}

				if (!(info = put_item(item, length, buffer, info, end)))
					return info;
			}

			if (info + 1 >= end)
			{
				*info = isc_info_truncated;
				return NULL;
			}
			*info++ = isc_info_sql_describe_end;
		} // if()
	} // for()

	return info;
}
