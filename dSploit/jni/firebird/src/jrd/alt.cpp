/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         alt.cpp
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
 * Contributor(s): ______________________________________.
 *
 * 23-May-2001 Claudio Valderrama - isc_add_user allowed user names
 *                           up to 32 characters length; changed to 31.
 *
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <string.h>
#include <stdio.h>
#include "../jrd/common.h"
#include "../common/classes/init.h"

#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/gds_proto.h"
#include "../jrd/utl_proto.h"
#include "../jrd/why_proto.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/secur_proto.h"
#include "../utilities/gsec/call_service.h"

#include "../jrd/event.h"
#include "../jrd/alt_proto.h"
#include "../jrd/constants.h"

#if !defined(SUPERSERVER) || defined(SUPERCLIENT)
#if !defined(BOOT_BUILD)
static ISC_STATUS executeSecurityCommand(ISC_STATUS*, const USER_SEC_DATA*, internal_user_data&);
#endif // BOOT_BUILD
#endif

SLONG API_ROUTINE_VARARG isc_event_block(UCHAR** event_buffer,
										 UCHAR** result_buffer,
										 USHORT count, ...)
{
/**************************************
 *
 *      i s c _ e v e n t _ b l o c k
 *
 **************************************
 *
 * Functional description
 *      Create an initialized event parameter block from a
 *      variable number of input arguments.
 *      Return the size of the block.
 *
 *	Return 0 if any error occurs.
 *
 **************************************/
	va_list ptr;

	va_start(ptr, count);

	// calculate length of event parameter block, setting initial length to include version
	// and counts for each argument

	SLONG length = 1;
	USHORT i = count;
	while (i--)
	{
		const char* q = va_arg(ptr, SCHAR *);
		length += strlen(q) + 5;
	}
	va_end(ptr);

	UCHAR* p = *event_buffer = (UCHAR *) gds__alloc((SLONG) length);
	// FREE: apparently never freed
	if (!*event_buffer)			// NOMEM:
		return 0;
	if ((*result_buffer = (UCHAR *) gds__alloc((SLONG) length)) == NULL)
	{
		// NOMEM:
		// FREE: apparently never freed
		gds__free(*event_buffer);
		*event_buffer = NULL;
		return 0;
	}

#ifdef DEBUG_GDS_ALLOC
	// I can find no place where these are freed
	// 1994-October-25 David Schnepper
	gds_alloc_flag_unfreed((void *) *event_buffer);
	gds_alloc_flag_unfreed((void *) *result_buffer);
#endif // DEBUG_GDS_ALLOC

	// initialize the block with event names and counts

	*p++ = EPB_version1;

	va_start(ptr, count);

	i = count;
	while (i--)
	{
		const char* q = va_arg(ptr, SCHAR *);

		// Strip the blanks from the ends
		const char* end = q + strlen(q);
		while (--end >= q && *end == ' ')
			;
		*p++ = end - q + 1;
		while (q <= end)
			*p++ = *q++;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}
	va_end(ptr);

	return static_cast<SLONG>(p - *event_buffer);
}


USHORT API_ROUTINE isc_event_block_a(SCHAR** event_buffer,
									 SCHAR** result_buffer,
									 USHORT count, TEXT** name_buffer)
{
/**************************************
 *
 *	i s c _ e v e n t _ b l o c k _ a
 *
 **************************************
 *
 * Functional description
 *	Create an initialized event parameter block from a
 *	vector of input arguments. (Ada needs this)
 *	Assume all strings are 31 characters long.
 *	Return the size of the block.
 *
 **************************************/
	const int MAX_NAME_LENGTH = 31;

	// calculate length of event parameter block, setting initial length to include version
	// and counts for each argument

	USHORT i = count;
	TEXT** nb = name_buffer;
	SLONG length = 0;
	while (i--)
	{
		const TEXT* const q = *nb++;

		// Strip trailing blanks from string
		const char* end = q + MAX_NAME_LENGTH;
		while (--end >= q && *end == ' ')
			;
		length += end - q + 1 + 5;
	}


	i = count;
	char* p = *event_buffer = (SCHAR *) gds__alloc((SLONG) length);
	// FREE: apparently never freed
	if (!(*event_buffer))		// NOMEM:
		return 0;
	if ((*result_buffer = (SCHAR *) gds__alloc((SLONG) length)) == NULL)
	{
		// NOMEM:
		// FREE: apparently never freed
		gds__free(*event_buffer);
		*event_buffer = NULL;
		return 0;
	}

#ifdef DEBUG_GDS_ALLOC
	// I can find no place where these are freed
	// 1994-October-25 David Schnepper
	gds_alloc_flag_unfreed((void *) *event_buffer);
	gds_alloc_flag_unfreed((void *) *result_buffer);
#endif // DEBUG_GDS_ALLOC

	*p++ = EPB_version1;

	nb = name_buffer;

	while (i--)
	{
		const TEXT* q = *nb++;

		// Strip trailing blanks from string
		const char* end = q + MAX_NAME_LENGTH;
		while (--end >= q && *end == ' ')
			;
		*p++ = end - q + 1;
		while (q <= end)
			*p++ = *q++;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}

	return (p - *event_buffer);
}


void API_ROUTINE isc_event_block_s(SCHAR** event_buffer, SCHAR** result_buffer, USHORT count,
								   TEXT** name_buffer, USHORT* return_count)
{
/**************************************
 *
 *	i s c _ e v e n t _ b l o c k _ s
 *
 **************************************
 *
 * Functional description
 *	THIS IS THE COBOL VERSION of gds__event_block_a for Cobols
 *	that don't permit return values.
 *
 **************************************/

	*return_count = isc_event_block_a(event_buffer, result_buffer, count, name_buffer);
}


ISC_STATUS API_ROUTINE_VARARG gds__start_transaction(ISC_STATUS* status_vector,
													 FB_API_HANDLE* tra_handle,
													 SSHORT count, ...)
{
	// This infamous structure is defined several times in different places
	struct teb_t
	{
		FB_API_HANDLE* teb_database;
		int teb_tpb_length;
		UCHAR* teb_tpb;
	};

	teb_t tebs[16];
	teb_t* teb = tebs;

	if (count > FB_NELEM(tebs))
		teb = (teb_t*) gds__alloc(((SLONG) sizeof(teb_t) * count));
	// FREE: later in this module

	if (!teb)
	{
		// NOMEM:
		status_vector[0] = isc_arg_gds;
		status_vector[1] = isc_virmemexh;
		status_vector[2] = isc_arg_end;
		return status_vector[1];
	}

	const teb_t* const end = teb + count;
	va_list ptr;
	va_start(ptr, count);

	for (teb_t* teb_iter = teb; teb_iter < end; ++teb_iter)
	{
		teb_iter->teb_database = va_arg(ptr, FB_API_HANDLE*);
		teb_iter->teb_tpb_length = va_arg(ptr, int);
		teb_iter->teb_tpb = va_arg(ptr, UCHAR *);
	}
	va_end(ptr);

	const ISC_STATUS status = isc_start_multiple(status_vector, tra_handle, count, teb);

	if (teb != tebs)
		gds__free(teb);

	return status;
}


ISC_STATUS API_ROUTINE gds__attach_database(ISC_STATUS* status_vector,
											SSHORT file_length,
											const SCHAR* file_name,
											FB_API_HANDLE* db_handle,
											SSHORT dpb_length, const SCHAR* dpb)
{
	return isc_attach_database(status_vector, file_length, file_name, db_handle, dpb_length, dpb);
}

ISC_STATUS API_ROUTINE gds__blob_info(ISC_STATUS* status_vector,
									  FB_API_HANDLE* blob_handle,
									  SSHORT msg_length,
									  const SCHAR* msg,
									  SSHORT buffer_length, SCHAR* buffer)
{
	return isc_blob_info(status_vector, blob_handle, msg_length, msg, buffer_length, buffer);
}

ISC_STATUS API_ROUTINE gds__cancel_blob(ISC_STATUS* status_vector,
										FB_API_HANDLE* blob_handle)
{
	return isc_cancel_blob(status_vector, blob_handle);
}

ISC_STATUS API_ROUTINE gds__cancel_events(ISC_STATUS * status_vector,
										  FB_API_HANDLE* db_handle, SLONG * event_id)
{
	return isc_cancel_events(status_vector, db_handle, event_id);
}

ISC_STATUS API_ROUTINE gds__close_blob(ISC_STATUS* status_vector, FB_API_HANDLE* blob_handle)
{
	return isc_close_blob(status_vector, blob_handle);
}

ISC_STATUS API_ROUTINE gds__commit_retaining(ISC_STATUS* status_vector, FB_API_HANDLE* tra_handle)
{
	return isc_commit_retaining(status_vector, tra_handle);
}

ISC_STATUS API_ROUTINE gds__commit_transaction(ISC_STATUS* status_vector, FB_API_HANDLE *tra_handle)
{
	return isc_commit_transaction(status_vector, tra_handle);
}

ISC_STATUS API_ROUTINE gds__compile_request(ISC_STATUS* status_vector,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* req_handle,
											SSHORT blr_length, const SCHAR* blr)
{
	return isc_compile_request(status_vector, db_handle, req_handle, blr_length, blr);
}

ISC_STATUS API_ROUTINE gds__compile_request2(ISC_STATUS* status_vector,
											 FB_API_HANDLE* db_handle,
											 FB_API_HANDLE* req_handle,
											 SSHORT blr_length, const SCHAR* blr)
{
	return isc_compile_request2(status_vector, db_handle, req_handle, blr_length, blr);
}

ISC_STATUS API_ROUTINE gds__create_blob(ISC_STATUS* status_vector,
										FB_API_HANDLE* db_handle,
										FB_API_HANDLE* tra_handle,
										FB_API_HANDLE* blob_handle, GDS_QUAD* blob_id)
{
	return isc_create_blob(status_vector, db_handle, tra_handle, blob_handle, blob_id);
}

ISC_STATUS API_ROUTINE gds__create_blob2(ISC_STATUS* status_vector,
										 FB_API_HANDLE* db_handle,
										 FB_API_HANDLE* tra_handle,
										 FB_API_HANDLE* blob_handle,
										 GDS_QUAD* blob_id,
										 SSHORT bpb_length, const SCHAR* bpb)
{
	return isc_create_blob2(status_vector, db_handle, tra_handle, blob_handle,
							blob_id, bpb_length, bpb);
}

ISC_STATUS API_ROUTINE gds__create_database(ISC_STATUS* status_vector,
											SSHORT file_length,
											const SCHAR* file_name,
											FB_API_HANDLE* db_handle,
											SSHORT dpb_length,
											const SCHAR* dpb, SSHORT db_type)
{
	return isc_create_database(status_vector, file_length, file_name, db_handle,
							   dpb_length, dpb, db_type);
}

ISC_STATUS API_ROUTINE gds__database_cleanup(ISC_STATUS * status_vector,
											FB_API_HANDLE* db_handle,
											AttachmentCleanupRoutine *routine, void* arg)
{

	return isc_database_cleanup(status_vector, db_handle, routine, arg);
}

ISC_STATUS API_ROUTINE gds__database_info(ISC_STATUS* status_vector,
										  FB_API_HANDLE* db_handle,
										  SSHORT msg_length,
										  const SCHAR* msg,
										  SSHORT buffer_length, SCHAR* buffer)
{
	return isc_database_info(status_vector, db_handle, msg_length, msg, buffer_length, buffer);
}

ISC_STATUS API_ROUTINE gds__detach_database(ISC_STATUS * status_vector,
											FB_API_HANDLE* db_handle)
{
	return isc_detach_database(status_vector, db_handle);
}

ISC_STATUS API_ROUTINE gds__get_segment(ISC_STATUS* status_vector,
										FB_API_HANDLE* blob_handle,
										USHORT * return_length,
										USHORT buffer_length, SCHAR * buffer)
{
	return isc_get_segment(status_vector, blob_handle, return_length, buffer_length, buffer);
}

ISC_STATUS API_ROUTINE gds__get_slice(ISC_STATUS* status_vector,
									  FB_API_HANDLE* db_handle,
									  FB_API_HANDLE* tra_handle,
									  GDS_QUAD* array_id,
									  SSHORT sdl_length,
									  const SCHAR* sdl,
									  SSHORT parameters_leng,
									  const SLONG* parameters,
									  SLONG slice_length,
									  void* slice, SLONG* return_length)
{
	return isc_get_slice(status_vector, db_handle, tra_handle, array_id,
						 sdl_length, sdl, parameters_leng, parameters,
						 slice_length, (SCHAR *) slice, return_length);
}

ISC_STATUS API_ROUTINE gds__open_blob(ISC_STATUS* status_vector,
								  FB_API_HANDLE* db_handle,
								  FB_API_HANDLE* tra_handle,
								  FB_API_HANDLE* blob_handle, GDS_QUAD* blob_id)
{
	return isc_open_blob(status_vector, db_handle, tra_handle, blob_handle, blob_id);
}

ISC_STATUS API_ROUTINE gds__open_blob2(ISC_STATUS* status_vector,
									   FB_API_HANDLE* db_handle,
									   FB_API_HANDLE* tra_handle,
									   FB_API_HANDLE* blob_handle,
									   GDS_QUAD* blob_id,
									   SSHORT bpb_length, const SCHAR* bpb)
{
	return isc_open_blob2(status_vector, db_handle, tra_handle, blob_handle,
						  blob_id, bpb_length, reinterpret_cast<const UCHAR*>(bpb));
}

ISC_STATUS API_ROUTINE gds__prepare_transaction(ISC_STATUS* status_vector, FB_API_HANDLE* tra_handle)
{
	return isc_prepare_transaction(status_vector, tra_handle);
}

ISC_STATUS API_ROUTINE gds__prepare_transaction2(ISC_STATUS* status_vector,
												 FB_API_HANDLE* tra_handle,
												 SSHORT msg_length, const SCHAR* msg)
{
	return isc_prepare_transaction2(status_vector, tra_handle, msg_length,
									reinterpret_cast<const UCHAR*>(msg));
}

ISC_STATUS API_ROUTINE gds__put_segment(ISC_STATUS* status_vector,
										FB_API_HANDLE* blob_handle,
										USHORT segment_length, const SCHAR* segment)
{
	return isc_put_segment(status_vector, blob_handle, segment_length, segment);
}

ISC_STATUS API_ROUTINE gds__put_slice(ISC_STATUS* status_vector,
									  FB_API_HANDLE* db_handle,
									  FB_API_HANDLE* tra_handle,
									  GDS_QUAD* array_id,
									  SSHORT sdl_length,
									  const SCHAR* sdl,
									  SSHORT parameters_leng,
									  const SLONG* parameters,
									  SLONG slice_length, void* slice)
{
	return isc_put_slice(status_vector, db_handle, tra_handle, array_id,
						 sdl_length, sdl, parameters_leng, parameters,
						 slice_length, reinterpret_cast<SCHAR*>(slice));
}

ISC_STATUS API_ROUTINE gds__que_events(ISC_STATUS* status_vector,
									   FB_API_HANDLE* db_handle,
									   SLONG* event_id,
									   SSHORT events_length,
									   const UCHAR* events,
									   FPTR_EVENT_CALLBACK ast_address,
									   void* ast_argument)
{
	return isc_que_events(status_vector, db_handle, event_id, events_length,
						  events, ast_address, ast_argument);
}

ISC_STATUS API_ROUTINE gds__receive(ISC_STATUS * status_vector,
									FB_API_HANDLE* req_handle,
									SSHORT msg_type,
									SSHORT msg_length,
									void *msg, SSHORT req_level)
{
	return isc_receive(status_vector, req_handle, msg_type, msg_length, (SCHAR*) msg, req_level);
}

ISC_STATUS API_ROUTINE gds__reconnect_transaction(ISC_STATUS* status_vector,
												  FB_API_HANDLE* db_handle,
												  FB_API_HANDLE* tra_handle,
												  SSHORT msg_length,
												  const SCHAR* msg)
{
	return isc_reconnect_transaction(status_vector, db_handle, tra_handle, msg_length, msg);
}

ISC_STATUS API_ROUTINE gds__release_request(ISC_STATUS * status_vector,
											FB_API_HANDLE* req_handle)
{
	return isc_release_request(status_vector, req_handle);
}

ISC_STATUS API_ROUTINE gds__request_info(ISC_STATUS* status_vector,
										 FB_API_HANDLE* req_handle,
										 SSHORT req_level,
										 SSHORT msg_length,
										 const SCHAR* msg,
										 SSHORT buffer_length, SCHAR* buffer)
{
	return isc_request_info(status_vector, req_handle, req_level, msg_length,
							msg, buffer_length, buffer);
}

ISC_STATUS API_ROUTINE gds__rollback_transaction(ISC_STATUS * status_vector,
												 FB_API_HANDLE* tra_handle)
{
	return isc_rollback_transaction(status_vector, tra_handle);
}

ISC_STATUS API_ROUTINE gds__seek_blob(ISC_STATUS * status_vector,
									  FB_API_HANDLE* blob_handle,
									  SSHORT mode, SLONG offset, SLONG * result)
{
	return isc_seek_blob(status_vector, blob_handle, mode, offset, result);
}

ISC_STATUS API_ROUTINE gds__send(ISC_STATUS* status_vector,
								 FB_API_HANDLE* req_handle,
								 SSHORT msg_type,
								 SSHORT msg_length, const void* msg,
								 SSHORT req_level)
{
	return isc_send(status_vector, req_handle, msg_type, msg_length,
					static_cast<const SCHAR*>(msg), req_level);
}

ISC_STATUS API_ROUTINE gds__start_and_send(ISC_STATUS* status_vector,
										   FB_API_HANDLE* req_handle,
										   FB_API_HANDLE* tra_handle,
										   SSHORT msg_type,
										   SSHORT msg_length,
										   const void* msg, SSHORT req_level)
{
	return isc_start_and_send(status_vector, req_handle, tra_handle, msg_type,
							  msg_length, (const SCHAR*) msg, req_level);
}

ISC_STATUS API_ROUTINE gds__start_multiple(ISC_STATUS * status_vector,
										   FB_API_HANDLE* tra_handle,
										   SSHORT db_count, void *teb_vector)
{
	return isc_start_multiple(status_vector, tra_handle, db_count, (SCHAR*) teb_vector);
}

ISC_STATUS API_ROUTINE gds__start_request(ISC_STATUS * status_vector,
										  FB_API_HANDLE* req_handle,
										  FB_API_HANDLE* tra_handle, SSHORT req_level)
{
	return isc_start_request(status_vector, req_handle, tra_handle, req_level);
}

ISC_STATUS API_ROUTINE gds__transaction_info(ISC_STATUS* status_vector,
											 FB_API_HANDLE* tra_handle,
											 SSHORT msg_length,
											 const SCHAR* msg,
											 SSHORT buffer_length, SCHAR* buffer)
{
	return isc_transaction_info(status_vector, tra_handle, msg_length, msg, buffer_length, buffer);
}

ISC_STATUS API_ROUTINE gds__unwind_request(ISC_STATUS * status_vector,
										   FB_API_HANDLE* req_handle, SSHORT req_level)
{
	return isc_unwind_request(status_vector, req_handle, req_level);
}

ISC_STATUS API_ROUTINE gds__ddl(ISC_STATUS* status_vector,
								FB_API_HANDLE* db_handle,
								FB_API_HANDLE* tra_handle,
								SSHORT ddl_length, const SCHAR* ddl)
{
	return isc_ddl(status_vector, db_handle, tra_handle, ddl_length, ddl);
}

void API_ROUTINE gds__event_counts(ULONG* result_vector,
								   SSHORT length,
								   UCHAR* before, const UCHAR* after)
{
	isc_event_counts(result_vector, length, before, after);
}

SLONG API_ROUTINE isc_free(SCHAR * blk)
{

	return gds__free(blk);
}

SLONG API_ROUTINE isc_ftof(const SCHAR* string1, const USHORT length1,
						   SCHAR* string2, const USHORT length2)
{
	return gds__ftof(string1, length1, string2, length2);
}

void API_ROUTINE gds__get_client_version(SCHAR * buffer)
{
	isc_get_client_version(buffer);
}

int API_ROUTINE gds__get_client_major_version()
{
	return isc_get_client_major_version();
}

int API_ROUTINE gds__get_client_minor_version()
{
	return isc_get_client_minor_version();
}

ISC_STATUS API_ROUTINE isc_print_blr(const SCHAR* blr,
									 FPTR_PRINT_CALLBACK callback,
									 void* callback_argument, SSHORT language)
{
	return gds__print_blr(reinterpret_cast<const UCHAR*>(blr), callback, callback_argument, language);
}

ISC_STATUS API_ROUTINE isc_print_status(const ISC_STATUS* status_vector)
{
	return gds__print_status(status_vector);
}

void API_ROUTINE isc_qtoq(const GDS_QUAD* quad1, GDS_QUAD* quad2)
{
	gds__qtoq(quad1, quad2);
}

SLONG API_ROUTINE isc_sqlcode(const ISC_STATUS* status_vector)
{
	return gds__sqlcode(status_vector);
}

void API_ROUTINE isc_sqlcode_s(const ISC_STATUS* status_vector, ULONG * sqlcode)
{
	*sqlcode = gds__sqlcode(status_vector);
	return;
}

void API_ROUTINE isc_vtof(const SCHAR* string1, SCHAR* string2, USHORT length)
{
	gds__vtof(string1, string2, length);
}

void API_ROUTINE isc_vtov(const SCHAR* string1, SCHAR* string2, SSHORT length)
{
	gds__vtov(string1, string2, length);
}

void API_ROUTINE gds__decode_date(const GDS_QUAD* date, void* time_structure)
{
	isc_decode_date(date, time_structure);
}

void API_ROUTINE gds__encode_date(const void* time_structure, GDS_QUAD* date)
{
	isc_encode_date(time_structure, date);
}

SLONG API_ROUTINE isc_vax_integer(const SCHAR* input, SSHORT length)
{
	return gds__vax_integer(reinterpret_cast<const UCHAR*>(input), length);
}

ISC_STATUS API_ROUTINE gds__event_wait(ISC_STATUS * status_vector,
									   FB_API_HANDLE* db_handle,
									   SSHORT events_length,
									   const UCHAR* events,
									   UCHAR* events_update)
{
	return isc_wait_for_event(status_vector, db_handle, events_length, events, events_update);
}

// CVC: This non-const signature is needed for compatibility, see gds.cpp.
SLONG API_ROUTINE isc_interprete(SCHAR* buffer, ISC_STATUS** status_vector_p)
{
	return gds__interprete(buffer, status_vector_p);
}

int API_ROUTINE gds__version(FB_API_HANDLE* db_handle,
							 FPTR_VERSION_CALLBACK callback,
							 void* callback_argument)
{
	return isc_version(db_handle, callback, callback_argument);
}

void API_ROUTINE gds__set_debug(int flag)
{
#ifndef SUPERCLIENT
	isc_set_debug(flag);
#endif
}

int API_ROUTINE isc_blob_display(ISC_STATUS* status_vector,
								 ISC_QUAD* blob_id,
								 FB_API_HANDLE* database,
								 FB_API_HANDLE* transaction,
								 const SCHAR* field_name, const SSHORT* name_length)
{
/**************************************
 *
 *	i s c _ b l o b _ d i s p l a y
 *
 **************************************
 *
 * Functional description
 *	PASCAL callable version of EDIT_blob.
 *
 **************************************/

	if (status_vector)
		status_vector[1] = 0;

	return blob__display((SLONG *) blob_id, database, transaction, field_name, name_length);
}

int API_ROUTINE isc_blob_dump(ISC_STATUS* status_vector,
							  ISC_QUAD* blob_id,
							  FB_API_HANDLE* database,
							  FB_API_HANDLE* transaction,
							  const SCHAR* file_name, const SSHORT* name_length)
{
/**************************************
 *
 *	i s c _ b l o b _ d u m p
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable dump
 *	into an internal dump call.
 *
 **************************************/

	if (status_vector)
		status_vector[1] = 0;

	return blob__dump((SLONG *) blob_id, database, transaction, file_name, name_length);
}

int API_ROUTINE isc_blob_edit(ISC_STATUS* status_vector,
							  ISC_QUAD* blob_id,
							  FB_API_HANDLE* database,
							  FB_API_HANDLE* transaction,
							  const SCHAR* field_name, const SSHORT* name_length)
{
/**************************************
 *
 *	b l o b _ $ e d i t
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable edit
 *	into an internal edit call.
 *
 **************************************/

	if (status_vector)
		status_vector[1] = 0;

	return blob__edit((SLONG*) blob_id, database, transaction, field_name, name_length);
}

int API_ROUTINE isc_blob_load(ISC_STATUS* status_vector,
							  ISC_QUAD* blob_id,
							  FB_API_HANDLE* database,
							  FB_API_HANDLE* transaction,
							  const SCHAR* file_name, const SSHORT* name_length)
{
/**************************************
 *
 *	i s c _ b l o b _ l o a d
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable load
 *	into an internal load call.
 *
 **************************************/

	if (status_vector)
		status_vector[1] = 0;

	return blob__load((SLONG *) blob_id, database, transaction, file_name, name_length);
}

void API_ROUTINE CVT_move(const dsc*, dsc*, FPTR_ERROR err)
// I believe noone could use this private API in his routines.
// This requires knowledge about our descriptors, which are hardly usable
// outside Firebird,	AP-2008.
{
	err(isc_random, isc_arg_string, "CVT_move() private API not supported any more", isc_arg_end);
}

#ifndef WIN_NT
// This function was exported earlier for reasons, not completely clear to me.
// It MUST not be ever exported - looks like I've missed bad commit in CVS.
// Keep it here to avoid senseless API change.

void API_ROUTINE SCH_ast(enum ast_t)
{
	// call to this function may be safely ingored
}
#endif

#if !defined(SUPERSERVER) || defined(SUPERCLIENT)
// AP: isc_*_user entrypoints are used only in any kind of embedded
// server (both posix and windows) and fbclient

#ifndef BOOT_BUILD
namespace {
	ISC_STATUS user_error(ISC_STATUS* vector, ISC_STATUS code)
	{
		vector[0] = isc_arg_gds;
		vector[1] = code;
		vector[2] = isc_arg_end;

		return vector[1];
	}
}
#endif // BOOT_BUILD

// CVC: Who was the genius that named the input param "user_data" when the
// function uses "struct user_data userInfo" to define a different variable type
// only few lines below? Same for the other two isc_*_user functions.
ISC_STATUS API_ROUTINE isc_add_user(ISC_STATUS* status, const USER_SEC_DATA* input_user_data)
{
/**************************************
 *
 *      i s c _ a d d _ u s e r
 *
 **************************************
 *
 * Functional description
 *      Adds a user to the server's security
 *	database.
 *      Return 0 if the user was added
 *
 *	    Return > 0 if any error occurs.
 *
 **************************************/
#ifdef BOOT_BUILD
	return 1;
#else // BOOT_BUILD
	internal_user_data userInfo;
	userInfo.operation = ADD_OPER;

	if (input_user_data->user_name)
	{
		if (strlen(input_user_data->user_name) > USERNAME_LENGTH) {
			return user_error(status, isc_usrname_too_long);
		}
		size_t l;
		for (l = 0;
			 input_user_data->user_name[l] != ' ' && l < strlen(input_user_data->user_name);
			 l++)
		{
			userInfo.user_name[l] = UPPER(input_user_data->user_name[l]);
		}

		userInfo.user_name[l] = '\0';
		userInfo.user_name_entered = true;
	}
	else {
		return user_error(status, isc_usrname_required);
	}

	if (input_user_data->password)
	{
		if (strlen(input_user_data->password) > 8) {
			return user_error(status, isc_password_too_long);
		}
		size_t l;
		for (l = 0;
			 l < strlen(input_user_data->password) && input_user_data->password[l] != ' ';
			 l++)
		{
			userInfo.password[l] = input_user_data->password[l];
		}

		userInfo.password[l] = '\0';
		userInfo.password_entered = true;
		userInfo.password_specified = true;
	}
	else {
		return user_error(status, isc_password_required);
	}


	if ((input_user_data->sec_flags & sec_uid_spec) &&
		(userInfo.uid_entered = (input_user_data->uid)))
	{
		userInfo.uid = input_user_data->uid;
		userInfo.uid_specified = true;
	}
	else
	{
		userInfo.uid_specified = false;
		userInfo.uid_entered = false;
	}

	if ((input_user_data->sec_flags & sec_gid_spec) &&
		(userInfo.gid_entered = (input_user_data->gid)))
	{
		userInfo.gid = input_user_data->gid;
		userInfo.gid_specified = true;
	}
	else
	{
		userInfo.gid_specified = false;
		userInfo.gid_entered = false;
	}

	if ((input_user_data->sec_flags & sec_group_name_spec) && input_user_data->group_name)
	{
		int l = MIN(ALT_NAME_LEN - 1, strlen(input_user_data->group_name));
		strncpy(userInfo.group_name, input_user_data->group_name, l);
		userInfo.group_name[l] = '\0';
		userInfo.group_name_entered = true;
		userInfo.group_name_specified = true;
	}
	else
	{
		userInfo.group_name_entered = false;
		userInfo.group_name_specified = false;
	}

	if ((input_user_data->sec_flags & sec_first_name_spec) && input_user_data->first_name)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->first_name));
		strncpy(userInfo.first_name, input_user_data->first_name, l);
		userInfo.first_name[l] = '\0';
		userInfo.first_name_entered = true;
		userInfo.first_name_specified = true;
	}
	else
	{
		userInfo.first_name_entered = false;
		userInfo.first_name_specified = false;
	}

	if ((input_user_data->sec_flags & sec_middle_name_spec) &&
		input_user_data->middle_name)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->middle_name));
		strncpy(userInfo.middle_name, input_user_data->middle_name, l);
		userInfo.middle_name[l] = '\0';
		userInfo.middle_name_entered = true;
		userInfo.middle_name_specified = true;
	}
	else
	{
		userInfo.middle_name_entered = false;
		userInfo.middle_name_specified = false;
	}

	if ((input_user_data->sec_flags & sec_last_name_spec) && input_user_data->last_name)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->last_name));
		strncpy(userInfo.last_name, input_user_data->last_name, l);
		userInfo.last_name[l] = '\0';
		userInfo.last_name_entered = true;
		userInfo.last_name_specified = true;
	}
	else
	{
		userInfo.last_name_entered = false;
		userInfo.last_name_specified = false;
	}

	return executeSecurityCommand(status, input_user_data, userInfo);
#endif // BOOT_BUILD
}

ISC_STATUS API_ROUTINE isc_delete_user(ISC_STATUS* status, const USER_SEC_DATA* input_user_data)
{
/**************************************
 *
 *      i s c _ d e l e t e _ u s e r
 *
 **************************************
 *
 * Functional description
 *      Deletes a user from the server's security
 *	    database.
 *      Return 0 if the user was deleted
 *
 *	    Return > 0 if any error occurs.
 *
 **************************************/
#ifdef BOOT_BUILD
	return 1;
#else // BOOT_BUILD
	internal_user_data userInfo;
	userInfo.operation = DEL_OPER;

	if (input_user_data->user_name)
	{
		if (strlen(input_user_data->user_name) > USERNAME_LENGTH) {
			return user_error(status, isc_usrname_too_long);
		}
		size_t l;
		for (l = 0;
			 input_user_data->user_name[l] != ' ' && l < strlen(input_user_data->user_name);
			 l++)
		{
			userInfo.user_name[l] = UPPER(input_user_data->user_name[l]);
		}

		userInfo.user_name[l] = '\0';
		userInfo.user_name_entered = true;
	}
	else {
		return user_error(status, isc_usrname_required);
	}

	return executeSecurityCommand(status, input_user_data, userInfo);
#endif // BOOT_BUILD
}

ISC_STATUS API_ROUTINE isc_modify_user(ISC_STATUS* status, const USER_SEC_DATA* input_user_data)
{
/**************************************
 *
 *      i s c _ m o d i f y _ u s e r
 *
 **************************************
 *
 * Functional description
 *      Adds a user to the server's security
 *	database.
 *      Return 0 if the user was added
 *
 *	    Return > 0 if any error occurs.
 *
 **************************************/
#ifdef BOOT_BUILD
	return 1;
#else // BOOT_BUILD
	internal_user_data userInfo;
	userInfo.operation = MOD_OPER;

	if (input_user_data->user_name)
	{
		if (strlen(input_user_data->user_name) > USERNAME_LENGTH) {
			return user_error(status, isc_usrname_too_long);
		}
		size_t l;
		for (l = 0;
			 input_user_data->user_name[l] != ' ' && l < strlen(input_user_data->user_name);
			 l++)
		{
			userInfo.user_name[l] = UPPER(input_user_data->user_name[l]);
		}

		userInfo.user_name[l] = '\0';
		userInfo.user_name_entered = true;
	}
	else {
		return user_error(status, isc_usrname_required);
	}

	if (input_user_data->sec_flags & sec_password_spec)
	{
		if (strlen(input_user_data->password) > 8) {
			return user_error(status, isc_password_too_long);
		}
		size_t l;
		for (l = 0;
			 l < strlen(input_user_data->password) && input_user_data->password[l] != ' ';
			 l++)
		{
			userInfo.password[l] = input_user_data->password[l];
		}

		userInfo.password[l] = '\0';
		userInfo.password_entered = true;
		userInfo.password_specified = true;
	}
	else
	{
		userInfo.password_specified = false;
		userInfo.password_entered = false;
	}


	if (input_user_data->sec_flags & sec_uid_spec)
	{
		userInfo.uid = input_user_data->uid;
		userInfo.uid_specified = true;
		userInfo.uid_entered = true;
	}
	else
	{
		userInfo.uid_specified = false;
		userInfo.uid_entered = false;
	}

	if (input_user_data->sec_flags & sec_gid_spec)
	{
		userInfo.gid = input_user_data->gid;
		userInfo.gid_specified = true;
		userInfo.gid_entered = true;
	}
	else
	{
		userInfo.gid_specified = false;
		userInfo.gid_entered = false;
	}

	if (input_user_data->sec_flags & sec_group_name_spec)
	{
		int l = MIN(ALT_NAME_LEN - 1, strlen(input_user_data->group_name));
		strncpy(userInfo.group_name, input_user_data->group_name, l);
		userInfo.group_name[l] = '\0';
		userInfo.group_name_entered = true;
		userInfo.group_name_specified = true;
	}
	else
	{
		userInfo.group_name_entered = false;
		userInfo.group_name_specified = false;
	}

	if (input_user_data->sec_flags & sec_first_name_spec)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->first_name));
		strncpy(userInfo.first_name, input_user_data->first_name, l);
		userInfo.first_name[l] = '\0';
		userInfo.first_name_entered = true;
		userInfo.first_name_specified = true;
	}
	else
	{
		userInfo.first_name_entered = false;
		userInfo.first_name_specified = false;
	}

	if (input_user_data->sec_flags & sec_middle_name_spec)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->middle_name));
		strncpy(userInfo.middle_name, input_user_data->middle_name, l);
		userInfo.middle_name[l] = '\0';
		userInfo.middle_name_entered = true;
		userInfo.middle_name_specified = true;
	}
	else
	{
		userInfo.middle_name_entered = false;
		userInfo.middle_name_specified = false;
	}

	if (input_user_data->sec_flags & sec_last_name_spec)
	{
		int l = MIN(NAME_LEN - 1, strlen(input_user_data->last_name));
		strncpy(userInfo.last_name, input_user_data->last_name, l);
		userInfo.last_name[l] = '\0';
		userInfo.last_name_entered = true;
		userInfo.last_name_specified = true;
	}
	else
	{
		userInfo.last_name_entered = false;
		userInfo.last_name_specified = false;
	}

	return executeSecurityCommand(status, input_user_data, userInfo);
#endif // BOOT_BUILD
}


#if !defined(BOOT_BUILD)

static ISC_STATUS executeSecurityCommand(ISC_STATUS* status,
										const USER_SEC_DATA* input_user_data,
										internal_user_data& userInfo
)
{
/**************************************
 *
 *      e x e c u t e S e c u r i t y C o m m a n d
 *
 **************************************
 *
 * Functional description
 *
 *    Executes command according to input_user_data
 *    and userInfo. Calls service manager to do job.
 **************************************/

	isc_svc_handle handle = attachRemoteServiceManager(status,
													   input_user_data->dba_user_name,
													   input_user_data->dba_password,
													   false,
													   input_user_data->protocol,
													   input_user_data->server);
	if (handle)
	{
		callRemoteServiceManager(status, handle, userInfo, 0, 0);
		Firebird::makePermanentVector(status);

		ISC_STATUS_ARRAY user_status;
		detachRemoteServiceManager(user_status, handle);
	}

	return status[1];
}

#endif // BOOT_BUILD

#endif // !defined(SUPERSERVER) || defined(SUPERCLIENT)
