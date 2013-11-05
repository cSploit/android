/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		entry.h
 *	DESCRIPTION:	Y-value entrypoints
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

/*
    Order of battle in ENTRY macro:
	current local access method entrypoint
	remote entrypoint
*/

/*****************************************************
 *  IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
 *
 *  The order in which these entrypoints appear MUST match
 *  the order in which the defines for PROC_XXX appear in
 *  source/jrd/why.cpp.  Failure to do so will result in
 *  much frustration
 ******************************************************/

/*
 * TMN: Equally important: Don't add a compiler directive like
 * extern "C" { } to this file since it's included to be the body
 * of an array (!) from why.cpp.
 */

// 0
ENTRYPOINT( jrd8_attach_database,
            REM_attach_database)
// 1
ENTRYPOINT( jrd8_blob_info,
            REM_blob_info)
// 2
ENTRYPOINT( jrd8_cancel_blob,
            REM_cancel_blob)
// 3
ENTRYPOINT( jrd8_close_blob,
            REM_close_blob)
// 4
ENTRYPOINT( jrd8_commit_transaction,
            REM_commit_transaction)
// 5
ENTRYPOINT( jrd8_compile_request,
            REM_compile_request)
// 6
ENTRYPOINT( /* jrd8_create_blob */ no_entrypoint,
			/* REM_create_blob */ no_entrypoint)
// 7
ENTRYPOINT( jrd8_create_database,
            REM_create_database)
// 8
ENTRYPOINT( jrd8_database_info,
            REM_database_info)
// 9
ENTRYPOINT( jrd8_detach_database,
            REM_detach_database)
// 10
ENTRYPOINT( jrd8_get_segment,
            REM_get_segment)
// 11
ENTRYPOINT( /* jrd8_open_blob */ no_entrypoint,
			/* REM_open_blob */ no_entrypoint)
// 12
ENTRYPOINT( jrd8_prepare_transaction,
            REM_prepare_transaction)
// 13
ENTRYPOINT( jrd8_put_segment,
            REM_put_segment)
// 14
ENTRYPOINT( jrd8_reconnect_transaction,
            REM_reconnect_transaction)
// 15
ENTRYPOINT( jrd8_receive,
            REM_receive)
// 16
ENTRYPOINT( jrd8_release_request,
            REM_release_request)
// 17
ENTRYPOINT( jrd8_request_info,
            REM_request_info)
// 18
ENTRYPOINT( jrd8_rollback_transaction,
            REM_rollback_transaction)
// 19
ENTRYPOINT( jrd8_send,
            REM_send)
// 20
ENTRYPOINT( jrd8_start_and_send,
            REM_start_and_send)
// 21
ENTRYPOINT( jrd8_start_request,
            REM_start_request)
// 22
ENTRYPOINT( jrd8_start_multiple,
			/* REM_start_multiple */ no_entrypoint)
// 23
ENTRYPOINT( jrd8_start_transaction,
            REM_start_transaction)
// 24
ENTRYPOINT( jrd8_transaction_info,
            REM_transaction_info)
// 25
ENTRYPOINT( jrd8_unwind_request,
            REM_unwind_request)
// 26
ENTRYPOINT( jrd8_commit_retaining,
            REM_commit_retaining)
// 27
ENTRYPOINT( jrd8_que_events,
            REM_que_events)
// 28
ENTRYPOINT( jrd8_cancel_events,
            REM_cancel_events)
// 29
ENTRYPOINT( jrd8_ddl,
            REM_ddl)
// 30
ENTRYPOINT( jrd8_open_blob2,
            REM_open_blob2)
// 31
ENTRYPOINT( jrd8_create_blob2,
            REM_create_blob2)
// 32
ENTRYPOINT( jrd8_get_slice,
            REM_get_slice)
// 33
ENTRYPOINT( jrd8_put_slice,
            REM_put_slice)
// 34
ENTRYPOINT( jrd8_seek_blob,
            REM_seek_blob)
// 35
ENTRYPOINT( jrd8_transact_request,
            REM_transact_request)
// 36
ENTRYPOINT( jrd8_drop_database,
            REM_drop_database)

/* and the DSQL entry points */

// 37
ENTRYPOINT( jrd8_allocate_statement,
            REM_allocate_statement)
// 38
ENTRYPOINT( /* jrd8_execute */ no_entrypoint,
            REM_execute)
// 39
ENTRYPOINT( jrd8_execute,
            REM_execute2)
// 40
ENTRYPOINT( /* jrd8_execute_immediate */ no_entrypoint,
            REM_execute_immediate)
// 41
ENTRYPOINT( jrd8_execute_immediate,
            REM_execute_immediate2)
// 42
ENTRYPOINT( jrd8_fetch,
            REM_fetch)
// 43
ENTRYPOINT( jrd8_free_statement,
            REM_free_statement)
// 44
ENTRYPOINT( jrd8_insert,
            REM_insert)
// 45
ENTRYPOINT( jrd8_prepare,
            REM_prepare)
// 46
ENTRYPOINT( jrd8_set_cursor,
            REM_set_cursor_name)
// 47
ENTRYPOINT( jrd8_sql_info,
            REM_sql_info)

/* and service entry points */

// 48
ENTRYPOINT( jrd8_service_attach,
            REM_service_attach)
// 49
ENTRYPOINT( jrd8_service_detach,
            REM_service_detach)
// 50
ENTRYPOINT( jrd8_service_query,
            REM_service_query)
// 51
ENTRYPOINT( jrd8_service_start,
            REM_service_start)
// 52
ENTRYPOINT( jrd8_rollback_retaining,
            REM_rollback_retaining)
// 53
ENTRYPOINT( jrd8_cancel_operation,
			REM_cancel_operation)
// 54
ENTRYPOINT( jrd8_shutdown_all,
			/* REM_shutdown_all */ no_entrypoint)
// 55
ENTRYPOINT( jrd8_ping_attachment,
			/* REM_ping_attachment */ no_entrypoint)

#undef ENTRYPOINT
