/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         alt_proto.h
 *      DESCRIPTION:    Alternative entrypoints
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
 * Contributor(s): Blas Rodriguez Somoza
 *
 *
 */

#ifndef ALT_PROTO_H
#define ALT_PROTO_H

extern "C" {

//
// gds_ functions using isc_ functions (OK)
//
ISC_STATUS	API_ROUTINE_VARARG gds__start_transaction(ISC_STATUS*, FB_API_HANDLE*,
							  SSHORT, ...);

ISC_STATUS	API_ROUTINE gds__attach_database( ISC_STATUS*, SSHORT, const SCHAR*,
						  FB_API_HANDLE*, SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__blob_info( ISC_STATUS*, FB_API_HANDLE*, SSHORT,
					    const SCHAR*, SSHORT, SCHAR*);

ISC_STATUS	API_ROUTINE gds__cancel_blob(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__cancel_events(ISC_STATUS*, FB_API_HANDLE*, SLONG*);

ISC_STATUS	API_ROUTINE gds__close_blob(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__commit_retaining(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__commit_transaction(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__compile_request( ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
						  SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__compile_request2( ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
						   SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__create_blob(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
					     FB_API_HANDLE*, GDS_QUAD*);

ISC_STATUS	API_ROUTINE gds__create_blob2(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
					      FB_API_HANDLE*, GDS_QUAD*, SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__create_database( ISC_STATUS*, SSHORT, const SCHAR*,
						  FB_API_HANDLE*, SSHORT, const SCHAR*, SSHORT);

ISC_STATUS	API_ROUTINE gds__database_cleanup(ISC_STATUS*, FB_API_HANDLE*,
					  	  AttachmentCleanupRoutine*, void * );

ISC_STATUS	API_ROUTINE gds__database_info(ISC_STATUS*, FB_API_HANDLE*, SSHORT,
						const SCHAR*, SSHORT, SCHAR*);

ISC_STATUS	API_ROUTINE gds__detach_database(ISC_STATUS*, FB_API_HANDLE*);

void		API_ROUTINE gds__event_counts(ULONG*, SSHORT, UCHAR*, const UCHAR*);

void		API_ROUTINE gds__get_client_version(SCHAR*);

int			API_ROUTINE gds__get_client_major_version();

int			API_ROUTINE gds__get_client_minor_version();

ISC_STATUS	API_ROUTINE gds__event_wait(ISC_STATUS*, FB_API_HANDLE*,
					    SSHORT, const UCHAR*, UCHAR*);

ISC_STATUS	API_ROUTINE gds__get_segment(ISC_STATUS*, FB_API_HANDLE*,
					     USHORT*, USHORT, SCHAR*);

ISC_STATUS	API_ROUTINE gds__get_slice(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*, GDS_QUAD*,
					   SSHORT, const SCHAR*, SSHORT, const SLONG*,
					   SLONG, void*, SLONG*);

ISC_STATUS	API_ROUTINE gds__open_blob(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
					   FB_API_HANDLE*, GDS_QUAD*);

ISC_STATUS	API_ROUTINE gds__open_blob2(ISC_STATUS*, FB_API_HANDLE*,
					    FB_API_HANDLE*, FB_API_HANDLE*,
					    GDS_QUAD*, SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__prepare_transaction(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__prepare_transaction2(ISC_STATUS*, FB_API_HANDLE*,
						      SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__put_segment(ISC_STATUS*, FB_API_HANDLE*,
					     USHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__put_slice( ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*,
					    GDS_QUAD*, SSHORT, const SCHAR*,
					    SSHORT, const SLONG*, SLONG, void*);

ISC_STATUS	API_ROUTINE gds__que_events(	ISC_STATUS*, FB_API_HANDLE*, SLONG*,
						SSHORT, const UCHAR*,
						FPTR_EVENT_CALLBACK, void*);

ISC_STATUS	API_ROUTINE gds__receive(ISC_STATUS*, FB_API_HANDLE*,
					SSHORT, SSHORT,	void*, SSHORT);

ISC_STATUS	API_ROUTINE gds__reconnect_transaction(ISC_STATUS*, FB_API_HANDLE*,
							FB_API_HANDLE*,	SSHORT, const SCHAR*);

ISC_STATUS	API_ROUTINE gds__release_request(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__request_info(ISC_STATUS*, FB_API_HANDLE*,
						SSHORT, SSHORT,	const SCHAR*,
						SSHORT, SCHAR*);

ISC_STATUS	API_ROUTINE gds__rollback_transaction(ISC_STATUS*, FB_API_HANDLE*);

ISC_STATUS	API_ROUTINE gds__seek_blob(ISC_STATUS*, FB_API_HANDLE*,
					   SSHORT, SLONG, SLONG*);

ISC_STATUS	API_ROUTINE gds__send(	ISC_STATUS*, FB_API_HANDLE*,
					SSHORT, SSHORT, const void*,
					SSHORT);

ISC_STATUS	API_ROUTINE gds__start_and_send(ISC_STATUS*, FB_API_HANDLE*,
						FB_API_HANDLE*, SSHORT, SSHORT,
						const void*, SSHORT);


ISC_STATUS	API_ROUTINE gds__start_multiple(ISC_STATUS*, FB_API_HANDLE*,
						SSHORT, void*);

ISC_STATUS	API_ROUTINE gds__start_request( ISC_STATUS*, FB_API_HANDLE*,
						FB_API_HANDLE*, SSHORT);


ISC_STATUS	API_ROUTINE gds__transaction_info(ISC_STATUS*, FB_API_HANDLE*,
						  SSHORT, const SCHAR*,
						  SSHORT, SCHAR*);

ISC_STATUS	API_ROUTINE gds__unwind_request(ISC_STATUS*, FB_API_HANDLE*, SSHORT);

ISC_STATUS	API_ROUTINE gds__ddl(ISC_STATUS*, FB_API_HANDLE*,
					FB_API_HANDLE*, SSHORT, const SCHAR*);

void		API_ROUTINE gds__decode_date(const GDS_QUAD*, void*);

void		API_ROUTINE gds__encode_date(const void*, GDS_QUAD*);

int		API_ROUTINE gds__version(FB_API_HANDLE*,
					 FPTR_VERSION_CALLBACK, void*);

void		API_ROUTINE gds__set_debug(int);

// isc_ functions which are not mapped to gds_ functions (the gds_ ones are in utl.cpp)
// Should be analyzed
//
SLONG		API_ROUTINE_VARARG isc_event_block(UCHAR**, UCHAR**, USHORT, ...);

USHORT		API_ROUTINE isc_event_block_a(SCHAR**, SCHAR**, USHORT, TEXT**);

void		API_ROUTINE isc_event_block_s(SCHAR**, SCHAR**, USHORT, TEXT**, USHORT*);
//
// isc functions using gds_ functions (gds_ functions defined in gds.cpp)
//
SLONG		API_ROUTINE isc_free(SCHAR*);
SLONG		API_ROUTINE isc_ftof(const SCHAR*, const USHORT, SCHAR*, const USHORT);
ISC_STATUS	API_ROUTINE isc_print_blr(const SCHAR*, FPTR_PRINT_CALLBACK, void*, SSHORT);
ISC_STATUS	API_ROUTINE isc_print_status(const ISC_STATUS*);
void		API_ROUTINE isc_qtoq(const ISC_QUAD*, ISC_QUAD*);
SLONG		API_ROUTINE isc_sqlcode(const ISC_STATUS*);
void		API_ROUTINE isc_sqlcode_s(const ISC_STATUS*, ULONG*);
void		API_ROUTINE isc_vtof(const SCHAR*, SCHAR*, USHORT);
void		API_ROUTINE isc_vtov(const SCHAR*, SCHAR*, SSHORT);
SLONG		API_ROUTINE isc_vax_integer(const SCHAR*, SSHORT);
SLONG		API_ROUTINE isc_interprete(SCHAR*, ISC_STATUS**);

// isc_ functions with no gds_ equivalence
//
ISC_STATUS	API_ROUTINE isc_add_user(ISC_STATUS*, const USER_SEC_DATA*);
ISC_STATUS	API_ROUTINE isc_delete_user(ISC_STATUS*, const USER_SEC_DATA*);
ISC_STATUS	API_ROUTINE isc_modify_user(ISC_STATUS*, const USER_SEC_DATA*);

// unsupported private API call(s) - added to prevent application startup problems
struct dsc;
void		API_ROUTINE CVT_move(const dsc*, dsc*, FPTR_ERROR);
enum ast_t
{
    AST_dummy
};
void 		API_ROUTINE SCH_ast(enum ast_t);

} // extern "C"

#endif //ALT_PROTO_H

