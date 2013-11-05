/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		jrd_proto.h
 *	DESCRIPTION:	Prototype header file for jrd.cpp
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


#ifndef JRD_JRD_PROTO_H
#define JRD_JRD_PROTO_H

#include "../common/classes/fb_string.h"

namespace Jrd {
	class Database;
	class Attachment;
	class jrd_tra;
	class blb;
	struct bid;
	class jrd_req;
	class Service;
	class thread_db;
	struct teb;
	class dsql_req;
}

extern "C" {

ISC_STATUS jrd8_attach_database(ISC_STATUS*, const TEXT*, Jrd::Attachment**, SSHORT, const UCHAR*);
ISC_STATUS jrd8_blob_info(ISC_STATUS*, Jrd::blb**, SSHORT, const SCHAR*, SSHORT, SCHAR*);
ISC_STATUS jrd8_cancel_blob(ISC_STATUS*, Jrd::blb **);
ISC_STATUS jrd8_cancel_events(ISC_STATUS*, Jrd::Attachment**, SLONG *);

ISC_STATUS jrd8_cancel_operation(ISC_STATUS*, Jrd::Attachment**, USHORT);

ISC_STATUS jrd8_close_blob(ISC_STATUS*, Jrd::blb **);
ISC_STATUS jrd8_commit_transaction(ISC_STATUS*, Jrd::jrd_tra **);
ISC_STATUS jrd8_commit_retaining(ISC_STATUS*, Jrd::jrd_tra **);
ISC_STATUS jrd8_compile_request(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_req**, SSHORT, const SCHAR*);
ISC_STATUS jrd8_create_blob2(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, Jrd::blb**,
										   Jrd::bid*, USHORT, const UCHAR*);
ISC_STATUS jrd8_create_database(ISC_STATUS*, const TEXT*, Jrd::Attachment**, USHORT, const UCHAR*);
ISC_STATUS jrd8_database_info(ISC_STATUS*, Jrd::Attachment**, SSHORT,
											const SCHAR*, SSHORT, SCHAR*);
ISC_STATUS jrd8_ddl(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, USHORT, const SCHAR*);
ISC_STATUS jrd8_detach_database(ISC_STATUS*, Jrd::Attachment**);
ISC_STATUS jrd8_drop_database(ISC_STATUS*, Jrd::Attachment**);
ISC_STATUS jrd8_get_segment(ISC_STATUS*, Jrd::blb**, USHORT *, USHORT, UCHAR *);
ISC_STATUS jrd8_get_slice(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, ISC_QUAD*, USHORT,
										const UCHAR*, USHORT, const UCHAR*, SLONG, UCHAR*, SLONG*);
ISC_STATUS jrd8_open_blob2(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, Jrd::blb**,
										 Jrd::bid*, USHORT, const UCHAR*);
ISC_STATUS jrd8_prepare_transaction(ISC_STATUS*, Jrd::jrd_tra **, USHORT, const UCHAR*);
ISC_STATUS jrd8_put_segment(ISC_STATUS*, Jrd::blb**, USHORT, const UCHAR*);
ISC_STATUS jrd8_put_slice(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, ISC_QUAD*, USHORT,
										const UCHAR*, USHORT, const UCHAR*, SLONG, UCHAR*);
ISC_STATUS jrd8_que_events(ISC_STATUS*, Jrd::Attachment**, SLONG*, SSHORT, const UCHAR*,
										 FPTR_EVENT_CALLBACK, void*);
ISC_STATUS jrd8_receive(ISC_STATUS*, Jrd::jrd_req**, USHORT, USHORT, SCHAR *, SSHORT);
ISC_STATUS jrd8_reconnect_transaction(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, SSHORT,
													const UCHAR*);
ISC_STATUS jrd8_release_request(ISC_STATUS*, Jrd::jrd_req**);
ISC_STATUS jrd8_request_info(ISC_STATUS*, Jrd::jrd_req**, SSHORT, SSHORT, const SCHAR*, SSHORT, SCHAR*);
ISC_STATUS jrd8_rollback_transaction(ISC_STATUS*, Jrd::jrd_tra **);
ISC_STATUS jrd8_rollback_retaining(ISC_STATUS*, Jrd::jrd_tra **);
ISC_STATUS jrd8_seek_blob(ISC_STATUS*, Jrd::blb **, SSHORT, SLONG, SLONG *);
ISC_STATUS jrd8_send(ISC_STATUS *, Jrd::jrd_req**, USHORT, USHORT, SCHAR *, SSHORT);
ISC_STATUS jrd8_service_attach(ISC_STATUS*, const TEXT*, Jrd::Service**, USHORT, const SCHAR*);
ISC_STATUS jrd8_service_detach(ISC_STATUS*, Jrd::Service**);
ISC_STATUS jrd8_service_query(ISC_STATUS*, Jrd::Service**, ULONG*,
											USHORT, const SCHAR*,
											USHORT, const SCHAR*,
											USHORT, SCHAR*);
ISC_STATUS jrd8_service_start(ISC_STATUS*, Jrd::Service**, ULONG*, USHORT, const SCHAR*);
ISC_STATUS jrd8_start_and_send(ISC_STATUS*, Jrd::jrd_req**, Jrd::jrd_tra **, USHORT, USHORT,
											 SCHAR *, SSHORT);
ISC_STATUS jrd8_start_request(ISC_STATUS*, Jrd::jrd_req**, Jrd::jrd_tra **, SSHORT);
ISC_STATUS jrd8_start_multiple(ISC_STATUS*, Jrd::jrd_tra **, USHORT, Jrd::teb *);
ISC_STATUS jrd8_start_transaction(ISC_STATUS*, Jrd::jrd_tra **, SSHORT, ...);
ISC_STATUS jrd8_transaction_info(ISC_STATUS*, Jrd::jrd_tra**, SSHORT, const SCHAR*, SSHORT, SCHAR*);
ISC_STATUS jrd8_transact_request(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, USHORT, const SCHAR*,
											   USHORT, const SCHAR*, USHORT, SCHAR*);
ISC_STATUS jrd8_unwind_request(ISC_STATUS*, Jrd::jrd_req**, SSHORT);
int jrd8_shutdown_all(unsigned int);
ISC_STATUS jrd8_allocate_statement(ISC_STATUS*, Jrd::Attachment**, Jrd::dsql_req**);
ISC_STATUS jrd8_execute(ISC_STATUS*, Jrd::jrd_tra**, Jrd::dsql_req**, USHORT, const SCHAR*,
						USHORT, USHORT, const SCHAR*, USHORT, SCHAR*, USHORT, USHORT, SCHAR*);
ISC_STATUS jrd8_execute_immediate(ISC_STATUS*, Jrd::Attachment**, Jrd::jrd_tra**, USHORT, const TEXT*,
								  USHORT, USHORT, const SCHAR*, USHORT, USHORT, const SCHAR*,
								  USHORT, SCHAR*, USHORT, USHORT, SCHAR*);
#ifdef SCROLLABLE_CURSORS
ISC_STATUS jrd8_fetch(ISC_STATUS*, Jrd::dsql_req**, USHORT, const SCHAR*, USHORT, USHORT, SCHAR*,
					  USHORT, SLONG);
#else
ISC_STATUS jrd8_fetch(ISC_STATUS*, Jrd::dsql_req**, USHORT, const SCHAR*, USHORT, USHORT, SCHAR*);
#endif // SCROLLABLE_CURSORS
ISC_STATUS jrd8_free_statement(ISC_STATUS*, Jrd::dsql_req**, USHORT);
ISC_STATUS jrd8_insert(ISC_STATUS*, Jrd::dsql_req**, USHORT, const SCHAR*, USHORT, USHORT, const SCHAR*);
ISC_STATUS jrd8_prepare(ISC_STATUS*, Jrd::jrd_tra**, Jrd::dsql_req**, USHORT, const TEXT*,
						USHORT, USHORT, const SCHAR*, USHORT, SCHAR*);
ISC_STATUS jrd8_set_cursor(ISC_STATUS*, Jrd::dsql_req**, const TEXT*, USHORT);
ISC_STATUS jrd8_sql_info(ISC_STATUS*, Jrd::dsql_req**, USHORT, const SCHAR*, USHORT, SCHAR*);
ISC_STATUS jrd8_ping_attachment(ISC_STATUS*, Jrd::Attachment**);

} // extern "C"

void jrd_vtof(const char*, char*, SSHORT);

// Defines for parameter 3 of JRD_num_attachments
enum JRD_info_tag
{
	JRD_info_none,
	JRD_info_drivemask,
	JRD_info_dbnames
};

UCHAR*	JRD_num_attachments(UCHAR* const, USHORT, JRD_info_tag, ULONG*, ULONG*, ULONG*);

bool	JRD_reschedule(Jrd::thread_db*, SLONG, bool);

#ifdef DEBUG_PROCS
void	JRD_print_procedure_info(Jrd::thread_db*, const char*);
#endif


void JRD_autocommit_ddl(Jrd::thread_db* tdbb, Jrd::jrd_tra* transaction);
void JRD_ddl(Jrd::thread_db* tdbb, /*Jrd::Attachment* attachment,*/ Jrd::jrd_tra* transaction,
	USHORT ddl_length, const UCHAR* ddl);
void JRD_receive(Jrd::thread_db* tdbb, Jrd::jrd_req* request, USHORT msg_type, USHORT msg_length,
	UCHAR* msg, SSHORT level
#ifdef SCROLLABLE_CURSORS
	, USHORT direction, ULONG offset
#endif
	);
void JRD_request_info(Jrd::thread_db* tdbb, Jrd::jrd_req* request, SSHORT level, SSHORT item_length,
	const UCHAR* items, SLONG buffer_length, UCHAR* buffer);
void JRD_start(Jrd::thread_db* tdbb, Jrd::jrd_req* request, Jrd::jrd_tra* transaction, SSHORT level);

void JRD_commit_transaction(Jrd::thread_db* tdbb, Jrd::jrd_tra** transaction);
void JRD_commit_retaining(Jrd::thread_db* tdbb, Jrd::jrd_tra** transaction);
void JRD_rollback_transaction(Jrd::thread_db* tdbb, Jrd::jrd_tra** transaction);
void JRD_rollback_retaining(Jrd::thread_db* tdbb, Jrd::jrd_tra** transaction);
void JRD_start_and_send(Jrd::thread_db* tdbb, Jrd::jrd_req* request, Jrd::jrd_tra* transaction,
	USHORT msg_type, USHORT msg_length, UCHAR* msg, SSHORT level);
void JRD_start_multiple(Jrd::thread_db* tdbb, Jrd::jrd_tra** tra_handle, USHORT count, Jrd::teb* vector);
void JRD_start_transaction(Jrd::thread_db* tdbb, Jrd::jrd_tra** transaction, SSHORT count, ...);
void JRD_unwind_request(Jrd::thread_db* tdbb, Jrd::jrd_req* request, SSHORT level);
void JRD_compile(Jrd::thread_db* tdbb, Jrd::Attachment* attachment, Jrd::jrd_req** req_handle,
	ULONG blr_length, const UCHAR* blr, Firebird::RefStrPtr,
	USHORT dbginfo_length, const UCHAR* dbginfo);
bool JRD_verify_database_access(const Firebird::PathName&);
void JRD_shutdown_attachments(const Jrd::Database* dbb);

#endif /* JRD_JRD_PROTO_H */
