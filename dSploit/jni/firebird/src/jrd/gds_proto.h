/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		gds_proto.h
 *	DESCRIPTION:	Prototype header file for gds.cpp
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef JRD_GDS_PROTO_H
#define JRD_GDS_PROTO_H

#include "../jrd/common.h"

const SSHORT IB_PREFIX_TYPE			= 0;
const SSHORT IB_PREFIX_LOCK_TYPE	= 1;
const SSHORT IB_PREFIX_MSG_TYPE		= 2;

// flags for gds_alloc_report
const ULONG ALLOC_dont_report	= 1L << 0;	/* Don't report this block */
const ULONG ALLOC_silent		= 1L << 1;	/* Don't report new leaks */
const ULONG ALLOC_verbose		= 1L << 2;	/* Report all leaks, even old */
const ULONG ALLOC_mark_current	= 1L << 3;	/* Mark all current leaks */
const ULONG ALLOC_check_each_call	= 1L << 4;	/* Check memory integrity on each alloc/free call */
const ULONG ALLOC_dont_check	= 1L << 5;	/* Stop checking integrity on each call */

#ifdef __cplusplus
extern "C" {
#endif

typedef void* VoidPtr;

VoidPtr	API_ROUTINE gds__alloc_debug(SLONG, const TEXT*, ULONG);
void	API_ROUTINE gds_alloc_flag_unfreed(void*);
void	API_ROUTINE gds_alloc_report(ULONG, const char*, int);

VoidPtr	API_ROUTINE gds__alloc(SLONG);

#ifdef DEBUG_GDS_ALLOC
#define gds__alloc(s)		gds__alloc_debug ((s), (TEXT*)__FILE__, (ULONG)__LINE__)
#endif /* DEBUG_GDS_ALLOC */


ISC_STATUS	API_ROUTINE gds__decode(ISC_STATUS, USHORT*, USHORT*);
void	API_ROUTINE isc_decode_date(const ISC_QUAD*, void*);
void	API_ROUTINE isc_decode_sql_date(const GDS_DATE*, void*);
void	API_ROUTINE isc_decode_sql_time(const GDS_TIME*, void*);
void	API_ROUTINE isc_decode_timestamp(const GDS_TIMESTAMP*, void*);
ISC_STATUS	API_ROUTINE gds__encode(ISC_STATUS, USHORT);
void	API_ROUTINE isc_encode_date(const void*, ISC_QUAD*);
void	API_ROUTINE isc_encode_sql_date(const void*, GDS_DATE*);
void	API_ROUTINE isc_encode_sql_time(const void*, GDS_TIME*);
void	API_ROUTINE isc_encode_timestamp(const void*, GDS_TIMESTAMP*);
ULONG	API_ROUTINE gds__free(void*);

/* CVC: This function was created to be used inside the engine, but I don't see
a problem if it's used from outside, too.
This function has been renamed and made public. */
SLONG	API_ROUTINE fb_interpret(char*, unsigned int, const ISC_STATUS**);
/* CVC: This non-const signature is needed for compatibility, see gds.cpp. */
SLONG	API_ROUTINE gds__interprete(char*, ISC_STATUS**);
void	API_ROUTINE gds__interprete_a(SCHAR*, SSHORT*, ISC_STATUS*, SSHORT*);

void	API_ROUTINE gds__log(const TEXT*, ...);
void	API_ROUTINE gds__trace(const char*);
void	API_ROUTINE gds__trace_raw(const char*, unsigned int = 0);
void	API_ROUTINE gds__log_status(const TEXT*, const ISC_STATUS*);
int		API_ROUTINE gds__msg_close(void*);
SSHORT	API_ROUTINE gds__msg_format(void*  handle,
									USHORT facility,
									USHORT msgNumber,
									USHORT bufsize,
									TEXT*  buffer,
									const TEXT* arg1,
									const TEXT* arg2,
									const TEXT* arg3,
									const TEXT* arg4,
									const TEXT* arg5);
SSHORT	API_ROUTINE gds__msg_lookup(void*, USHORT, USHORT, USHORT,
									  TEXT*, USHORT*);
int		API_ROUTINE gds__msg_open(void**, const TEXT*);
void	API_ROUTINE gds__msg_put(void*, USHORT, USHORT, const TEXT*,
							const TEXT*, const TEXT*, const TEXT*, const TEXT*);
void	API_ROUTINE gds__prefix(TEXT*, const TEXT*);
void	API_ROUTINE gds__prefix_lock(TEXT*, const TEXT*);
void	API_ROUTINE gds__prefix_msg(TEXT*, const TEXT*);

SLONG	API_ROUTINE gds__get_prefix(SSHORT, const TEXT*);
ISC_STATUS	API_ROUTINE gds__print_status(const ISC_STATUS*);
USHORT	API_ROUTINE gds__parse_bpb(USHORT, const UCHAR*, USHORT*, USHORT*);
USHORT	API_ROUTINE gds__parse_bpb2(USHORT, const UCHAR*, SSHORT*, SSHORT*,
	USHORT*, USHORT*, bool*, bool*, bool*, bool*);
SLONG API_ROUTINE gds__ftof(const SCHAR*, const USHORT length1, SCHAR*, const USHORT length2);
int		API_ROUTINE fb_print_blr(const UCHAR*, ULONG, FPTR_PRINT_CALLBACK, void*, SSHORT);
int		API_ROUTINE gds__print_blr(const UCHAR*, FPTR_PRINT_CALLBACK, void*, SSHORT);
void	API_ROUTINE gds__put_error(const TEXT*);
void	API_ROUTINE gds__qtoq(const void*, void*);
void	API_ROUTINE gds__register_cleanup(FPTR_VOID_PTR, void*);
SLONG	API_ROUTINE gds__sqlcode(const ISC_STATUS*);
void	API_ROUTINE gds__sqlcode_s(const ISC_STATUS*, ULONG*);
VoidPtr	API_ROUTINE gds__temp_file(BOOLEAN, const TEXT*, TEXT*, TEXT* = NULL, BOOLEAN = FALSE);
void	API_ROUTINE gds__unregister_cleanup(FPTR_VOID_PTR, void*);
BOOLEAN	API_ROUTINE gds__validate_lib_path(const TEXT*, const TEXT*, TEXT*, SLONG);
SLONG	API_ROUTINE gds__vax_integer(const UCHAR*, SSHORT);
void	API_ROUTINE gds__vtof(const SCHAR*, SCHAR*, USHORT);
void	API_ROUTINE gds__vtov(const SCHAR*, char*, SSHORT);
void	API_ROUTINE isc_print_sqlerror(SSHORT, const ISC_STATUS*);
void	API_ROUTINE isc_sql_interprete(SSHORT, TEXT*, SSHORT);
SINT64	API_ROUTINE isc_portable_integer(const UCHAR*, SSHORT);

// 14-June-2004. Nickolay Samofatov. The routines below are not part of the
// API and are not exported. Maybe use another prefix like GDS_ for them?
void	gds__cleanup();
void	gds__ulstr(char* buffer, ULONG value, const int minlen, const char filler);

void	FB_EXPORTED gds__default_printer(void*, SSHORT, const TEXT*);
void	gds__trace_printer(void*, SSHORT, const TEXT*);
void	gds__print_pool(Firebird::MemoryPool*, const TEXT*, ...);
void	GDS_init_prefix();

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* JRD_GDS_PROTO_H */

