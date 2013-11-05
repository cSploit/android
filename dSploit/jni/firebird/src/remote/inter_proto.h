/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		inter_proto.h
 *	DESCRIPTION:	Prototype Header file for interface.cpp
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

#ifndef REMOTE_INTER_PROTO_H
#define REMOTE_INTER_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

ISC_STATUS	REM_attach_database(ISC_STATUS* user_status, const TEXT* file_name, struct Rdb** handle,
	SSHORT dpb_length, const SCHAR* dpb);
ISC_STATUS	REM_blob_info(ISC_STATUS* user_status, struct Rbl** blob_handle,
	SSHORT item_length, const UCHAR* items,
	SSHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_cancel_blob(ISC_STATUS* user_status, struct Rbl** blob_handle);
ISC_STATUS	REM_cancel_events(ISC_STATUS* user_status, struct Rdb** handle, SLONG* id);
ISC_STATUS	REM_cancel_operation(ISC_STATUS* user_status, struct Rdb** handle, USHORT kind);
ISC_STATUS	REM_close_blob(ISC_STATUS* user_status, struct Rbl** blob_handle);
ISC_STATUS	REM_commit_transaction(ISC_STATUS* user_status, struct Rtr** rtr_handle);
ISC_STATUS	REM_commit_retaining(ISC_STATUS* user_status, struct Rtr** rtr_handle);
ISC_STATUS	REM_compile_request(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rrq** req_handle, USHORT blr_length, const UCHAR* blr);
ISC_STATUS	REM_create_blob2(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle,
	struct Rbl** blob_handle, BID blob_id, USHORT bpb_length, const UCHAR* bpb);
ISC_STATUS	REM_create_database(ISC_STATUS* user_status, const TEXT* file_name, struct Rdb** handle,
	SSHORT dpb_length, const SCHAR* dpb);
ISC_STATUS	REM_database_info(ISC_STATUS* user_status, struct Rdb** handle,
	SSHORT item_length, const UCHAR* items,
	SSHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_ddl(ISC_STATUS* user_status, struct Rdb** db_handle, struct Rtr** rtr_handle,
	USHORT blr_length, const UCHAR* blr);
ISC_STATUS	REM_detach_database(ISC_STATUS* user_status, struct Rdb** handle);
ISC_STATUS	REM_drop_database(ISC_STATUS* user_status, struct Rdb** handle);
ISC_STATUS	REM_allocate_statement(ISC_STATUS* user_status, struct Rdb ** db_handle,
	struct Rsr** stmt_handle);
ISC_STATUS	REM_execute(ISC_STATUS* user_status, struct Rtr** rtr_handle,
	struct Rsr** stmt_handle, USHORT blr_length, const UCHAR* blr,
	USHORT msg_type, USHORT msg_length, UCHAR* msg);
ISC_STATUS	REM_execute2(ISC_STATUS* user_status, struct Rtr** rtr_handle,
	struct Rsr** stmt_handle, USHORT in_blr_length, const UCHAR* in_blr,
	USHORT in_msg_type, USHORT in_msg_length, UCHAR* in_msg,
	USHORT out_blr_length, UCHAR* out_blr,
	USHORT out_msg_type, USHORT out_msg_length, UCHAR* out_msg);
ISC_STATUS	REM_execute_immediate(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle, USHORT length, const TEXT* string,
	USHORT dialect, USHORT blr_length, UCHAR* blr,
	USHORT msg_type, USHORT msg_length, UCHAR* msg);
ISC_STATUS	REM_execute_immediate2(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle, USHORT length, const TEXT* string,
	USHORT dialect, USHORT in_blr_length, UCHAR* in_blr,
	USHORT in_msg_type, USHORT in_msg_length, UCHAR* in_msg,
	USHORT out_blr_length, UCHAR* out_blr,
	USHORT out_msg_type, USHORT out_msg_length, UCHAR* out_msg);
ISC_STATUS	REM_fetch(ISC_STATUS* user_status, struct Rsr** stmt_handle,
	USHORT blr_length, UCHAR* blr, USHORT msg_type, USHORT msg_length, UCHAR* msg);
ISC_STATUS	REM_free_statement(ISC_STATUS* user_status, struct Rsr** stmt_handle, USHORT option);
ISC_STATUS	REM_insert(ISC_STATUS* user_status, struct Rsr** stmt_handle,
	USHORT blr_length, const UCHAR* blr, USHORT msg_type, USHORT msg_length, UCHAR* msg);
ISC_STATUS	REM_prepare(ISC_STATUS* user_status, struct Rtr** rtr_handle,
	struct Rsr** stmt_handle, USHORT length, const TEXT* string, USHORT dialect,
	USHORT item_length, const UCHAR* items, USHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_set_cursor_name(ISC_STATUS* user_status, struct Rsr** stmt_handle,
	const TEXT* cursor, USHORT type);
ISC_STATUS	REM_sql_info(ISC_STATUS* user_status, struct Rsr** stmt_handle,
	SSHORT item_length, const UCHAR* items, SSHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_get_segment(ISC_STATUS* user_status, struct Rbl** blob_handle,
	USHORT* length, USHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_get_slice(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** tra_handle, BID array_id, USHORT sdl_length, const UCHAR* sdl,
	USHORT param_length, const UCHAR* param, SLONG slice_length, UCHAR* slice, SLONG* return_length);
ISC_STATUS	REM_open_blob2(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle, struct Rbl** blob_handle, BID blob_id,
	USHORT bpb_length, const UCHAR* bpb);
ISC_STATUS	REM_prepare_transaction(ISC_STATUS* user_status, struct Rtr** rtr_handle,
	USHORT msg_length, const UCHAR* msg);
ISC_STATUS	REM_put_segment(ISC_STATUS* user_status, struct Rbl** blob_handle,
	USHORT segment_length, const UCHAR* segment);
ISC_STATUS	REM_put_slice(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** tra_handle, BID array_id, USHORT sdl_length, const UCHAR* sdl,
	USHORT param_length, const UCHAR* param, SLONG slice_length, UCHAR* slice);
ISC_STATUS	REM_que_events(ISC_STATUS* user_status, struct Rdb** handle, SLONG* id,
	SSHORT length, const UCHAR* items, FPTR_EVENT_CALLBACK ast, void* arg);
#ifdef SCROLLABLE_CURSORS
ISC_STATUS	REM_receive(ISC_STATUS* user_status, struct Rrq** req_handle,
	USHORT msg_type, USHORT msg_length, UCHAR* msg, SSHORT level,
	USHORT direction, ULONG offset);
#else
ISC_STATUS	REM_receive(ISC_STATUS* user_status, struct Rrq** req_handle,
	USHORT msg_type, USHORT msg_length, UCHAR* msg, SSHORT level);
#endif
ISC_STATUS	REM_reconnect_transaction(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle, USHORT length, const UCHAR* id);
ISC_STATUS	REM_release_request(ISC_STATUS* user_status, struct Rrq** req_handle);
ISC_STATUS	REM_request_info(ISC_STATUS* user_status, struct Rrq** req_handle, SSHORT level,
	SSHORT item_length, const UCHAR* items, SSHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_rollback_retaining(ISC_STATUS* user_status, Rtr** rtr_handle);
ISC_STATUS	REM_rollback_transaction(ISC_STATUS* user_status, struct Rtr** rtr_handle);
ISC_STATUS	REM_seek_blob(ISC_STATUS* user_status, struct Rbl** blob_handle, SSHORT mode,
	SLONG offset, SLONG* result);
ISC_STATUS	REM_send(ISC_STATUS* user_status, struct Rrq** req_handle, USHORT msg_type,
	USHORT msg_length, const UCHAR* msg, SSHORT level);
ISC_STATUS	REM_service_attach(ISC_STATUS* user_status, const TEXT* service_name,
	Rdb** handle, USHORT spb_length, const UCHAR* spb);
ISC_STATUS	REM_service_detach(ISC_STATUS* user_status, Rdb** handle);
ISC_STATUS	REM_service_query(ISC_STATUS* user_status, Rdb** svc_handle, ULONG* reserved,
	USHORT item_length, const UCHAR* items, USHORT recv_item_length, const UCHAR* recv_items,
	USHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_service_start(ISC_STATUS* user_status, Rdb** svc_handle, ULONG* reserved,
	USHORT item_length, const UCHAR* items);
ISC_STATUS	REM_start_and_send(ISC_STATUS* user_status, struct Rrq** req_handle,
	struct Rtr** rtr_handle, USHORT msg_type, USHORT msg_length, UCHAR* msg, SSHORT level);
ISC_STATUS	REM_start_request(ISC_STATUS* user_status, struct Rrq** req_handle,
	struct Rtr** rtr_handle, USHORT level);
ISC_STATUS	REM_start_transaction(ISC_STATUS* user_status, struct Rtr** rtr_handle,
	SSHORT count, struct Rdb** db_handle, SSHORT tpb_length, const UCHAR* tpb);
ISC_STATUS	REM_transact_request(ISC_STATUS* user_status, struct Rdb** db_handle,
	struct Rtr** rtr_handle, USHORT blr_length, UCHAR* blr,
	USHORT in_msg_length, UCHAR* in_msg, USHORT out_msg_length, UCHAR* out_msg);
ISC_STATUS	REM_transaction_info(ISC_STATUS* user_status, struct Rtr** tra_handle,
	SSHORT item_length, const UCHAR* items, SSHORT buffer_length, UCHAR* buffer);
ISC_STATUS	REM_unwind_request(ISC_STATUS* user_status, struct Rrq** req_handle, USHORT level);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif	/* REMOTE_INTER_PROTO_H */

