/*
 *	MODULE:		ibase.h
 *	DESCRIPTION:	OSRI entrypoints and defines
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
 * 2001.07.28: John Bellardo:  Added blr_skip
 * 2001.09.18: Ann Harrison:   New info codes
 * 17-Oct-2001 Mike Nordell: CPU affinity
 * 2001-04-16 Paul Beach: ISC_TIME_SECONDS_PRECISION_SCALE modified for HP10
 * Compiler Compatibility
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - EPSON, XENIX, MAC (MAC_AUX), Cray and OS/2
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 *
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 *
 * 2006.09.06 Steve Boyd - Added various prototypes required by Cobol ESQL
 *                         isc_embed_dsql_length
 *                         isc_event_block_a
 *                         isc_sqlcode_s
 *                         isc_embed_dsql_fetch_a
 *                         isc_event_block_s
 *                         isc_baddress
 *                         isc_baddress_s
 *
 */

#ifndef JRD_IBASE_H
#define JRD_IBASE_H

#define FB_API_VER 25
#define isc_version4

#define  ISC_TRUE	1
#define  ISC_FALSE	0
#if !(defined __cplusplus)
#define  ISC__TRUE	ISC_TRUE
#define  ISC__FALSE	ISC_FALSE
#endif

#define ISC_FAR

#if defined _MSC_VER && _MSC_VER >= 1300
#define FB_API_DEPRECATED __declspec(deprecated)
#elif defined __GNUC__ && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#define FB_API_DEPRECATED __attribute__((__deprecated__))
#else
#define FB_API_DEPRECATED
#endif

#include "types_pub.h"

/********************************/
/* Firebird Handle Definitions */
/********************************/

typedef FB_API_HANDLE isc_att_handle;
typedef FB_API_HANDLE isc_blob_handle;
typedef FB_API_HANDLE isc_db_handle;
typedef FB_API_HANDLE isc_req_handle;
typedef FB_API_HANDLE isc_stmt_handle;
typedef FB_API_HANDLE isc_svc_handle;
typedef FB_API_HANDLE isc_tr_handle;
typedef void (* isc_callback) ();
typedef ISC_LONG isc_resv_handle;

typedef void (*ISC_PRINT_CALLBACK) (void*, ISC_SHORT, const char*);
typedef void (*ISC_VERSION_CALLBACK)(void*, const char*);
typedef void (*ISC_EVENT_CALLBACK)(void*, ISC_USHORT, const ISC_UCHAR*);

/*******************************************************************/
/* Blob id structure                                               */
/*******************************************************************/

#if !(defined __cplusplus)
typedef GDS_QUAD GDS__QUAD;
#endif /* !(defined __cplusplus) */

typedef struct
{
	short array_bound_lower;
	short array_bound_upper;
} ISC_ARRAY_BOUND;

typedef struct
{
	ISC_UCHAR	array_desc_dtype;
	ISC_SCHAR			array_desc_scale;
	unsigned short	array_desc_length;
	ISC_SCHAR			array_desc_field_name[32];
	ISC_SCHAR			array_desc_relation_name[32];
	short			array_desc_dimensions;
	short			array_desc_flags;
	ISC_ARRAY_BOUND	array_desc_bounds[16];
} ISC_ARRAY_DESC;

typedef struct
{
	short			blob_desc_subtype;
	short			blob_desc_charset;
	short			blob_desc_segment_size;
	ISC_UCHAR	blob_desc_field_name[32];
	ISC_UCHAR	blob_desc_relation_name[32];
} ISC_BLOB_DESC;

/***************************/
/* Blob control structure  */
/***************************/

typedef struct isc_blob_ctl
{
	ISC_STATUS	(* ctl_source)();	/* Source filter */
	struct isc_blob_ctl*	ctl_source_handle;	/* Argument to pass to source filter */
	short					ctl_to_sub_type;		/* Target type */
	short					ctl_from_sub_type;		/* Source type */
	unsigned short			ctl_buffer_length;		/* Length of buffer */
	unsigned short			ctl_segment_length;		/* Length of current segment */
	unsigned short			ctl_bpb_length;			/* Length of blob parameter  block */
	/* Internally, this is const UCHAR*, but this public struct probably can't change. */
	ISC_SCHAR*					ctl_bpb;				/* Address of blob parameter block */
	ISC_UCHAR*			ctl_buffer;				/* Address of segment buffer */
	ISC_LONG				ctl_max_segment;		/* Length of longest segment */
	ISC_LONG				ctl_number_segments;	/* Total number of segments */
	ISC_LONG				ctl_total_length;		/* Total length of blob */
	ISC_STATUS*				ctl_status;				/* Address of status vector */
	long					ctl_data[8];			/* Application specific data */
} * ISC_BLOB_CTL;

/***************************/
/* Blob stream definitions */
/***************************/

typedef struct bstream
{
	isc_blob_handle	bstr_blob;		/* Blob handle */
	ISC_SCHAR *			bstr_buffer;	/* Address of buffer */
	ISC_SCHAR *			bstr_ptr;		/* Next character */
	short			bstr_length;	/* Length of buffer */
	short			bstr_cnt;		/* Characters in buffer */
	char			bstr_mode;		/* (mode) ? OUTPUT : INPUT */
} BSTREAM, *FB_BLOB_STREAM;

/* Three ugly macros, one even using octal radix... sigh... */
#define getb(p)	(--(p)->bstr_cnt >= 0 ? *(p)->bstr_ptr++ & 0377: BLOB_get (p))
#define putb(x, p) (((x) == '\n' || (!(--(p)->bstr_cnt))) ? BLOB_put ((x),p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))
#define putbx(x, p) ((!(--(p)->bstr_cnt)) ? BLOB_put ((x),p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))

/********************************************************************/
/* CVC: Public blob interface definition held in val.h.             */
/* For some unknown reason, it was only documented in langRef       */
/* and being the structure passed by the engine to UDFs it never    */
/* made its way into this public definitions file.                  */
/* Being its original name "blob", I renamed it blobcallback here.  */
/* I did the full definition with the proper parameters instead of  */
/* the weak C declaration with any number and type of parameters.   */
/* Since the first parameter -BLB- is unknown outside the engine,   */
/* it's more accurate to use void* than int* as the blob pointer    */
/********************************************************************/

#if !defined(JRD_VAL_H)
/* Blob passing structure */

/* This enum applies to parameter "mode" in blob_lseek */
enum blob_lseek_mode {blb_seek_relative = 1, blb_seek_from_tail = 2};
/* This enum applies to the value returned by blob_get_segment */
enum blob_get_result {blb_got_fragment = -1, blb_got_eof = 0, blb_got_full_segment = 1};

typedef struct blobcallback {
    short (*blob_get_segment)
		(void* hnd, ISC_UCHAR* buffer, ISC_USHORT buf_size, ISC_USHORT* result_len);
    void*		blob_handle;
    ISC_LONG	blob_number_segments;
    ISC_LONG	blob_max_segment;
    ISC_LONG	blob_total_length;
    void (*blob_put_segment)
		(void* hnd, const ISC_UCHAR* buffer, ISC_USHORT buf_size);
    ISC_LONG (*blob_lseek)
		(void* hnd, ISC_USHORT mode, ISC_LONG offset);
}  *BLOBCALLBACK;
#endif /* !defined(JRD_VAL_H) */


/********************************************************************/
/* CVC: Public descriptor interface held in dsc2.h.                  */
/* We need it documented to be able to recognize NULL in UDFs.      */
/* Being its original name "dsc", I renamed it paramdsc here.       */
/* Notice that I adjust to the original definition: contrary to     */
/* other cases, the typedef is the same struct not the pointer.     */
/* I included the enumeration of dsc_dtype possible values.         */
/* Ultimately, dsc2.h should be part of the public interface.        */
/********************************************************************/

#if !defined(JRD_DSC_H)
/* This is the famous internal descriptor that UDFs can use, too. */
typedef struct paramdsc {
    ISC_UCHAR	dsc_dtype;
    signed char		dsc_scale;
    ISC_USHORT		dsc_length;
    short		dsc_sub_type;
    ISC_USHORT		dsc_flags;
    ISC_UCHAR	*dsc_address;
} PARAMDSC;

#if !defined(JRD_VAL_H)
/* This is a helper struct to work with varchars. */
typedef struct paramvary {
    ISC_USHORT		vary_length;
    ISC_UCHAR		vary_string[1];
} PARAMVARY;
#endif /* !defined(JRD_VAL_H) */

#include "../jrd/dsc_pub.h"

#endif /* !defined(JRD_DSC_H) */

/***************************/
/* Dynamic SQL definitions */
/***************************/

#include "../dsql/sqlda_pub.h"

/***************************/
/* OSRI database functions */
/***************************/

#ifdef __cplusplus
extern "C" {
#endif

ISC_STATUS ISC_EXPORT isc_attach_database(ISC_STATUS*,
										  short,
										  const ISC_SCHAR*,
										  isc_db_handle*,
										  short,
										  const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_array_gen_sdl(ISC_STATUS*,
										const ISC_ARRAY_DESC*,
										ISC_SHORT*,
										ISC_UCHAR*,
										ISC_SHORT*);

ISC_STATUS ISC_EXPORT isc_array_get_slice(ISC_STATUS*,
										  isc_db_handle*,
										  isc_tr_handle*,
										  ISC_QUAD*,
										  const ISC_ARRAY_DESC*,
										  void*,
										  ISC_LONG*);

ISC_STATUS ISC_EXPORT isc_array_lookup_bounds(ISC_STATUS*,
											  isc_db_handle*,
											  isc_tr_handle*,
											  const ISC_SCHAR*,
											  const ISC_SCHAR*,
											  ISC_ARRAY_DESC*);

ISC_STATUS ISC_EXPORT isc_array_lookup_desc(ISC_STATUS*,
											isc_db_handle*,
											isc_tr_handle*,
											const ISC_SCHAR*,
											const ISC_SCHAR*,
											ISC_ARRAY_DESC*);

ISC_STATUS ISC_EXPORT isc_array_set_desc(ISC_STATUS*,
										 const ISC_SCHAR*,
										 const ISC_SCHAR*,
										 const short*,
										 const short*,
										 const short*,
										 ISC_ARRAY_DESC*);

ISC_STATUS ISC_EXPORT isc_array_put_slice(ISC_STATUS*,
										  isc_db_handle*,
										  isc_tr_handle*,
										  ISC_QUAD*,
										  const ISC_ARRAY_DESC*,
										  void*,
										  ISC_LONG *);

void ISC_EXPORT isc_blob_default_desc(ISC_BLOB_DESC*,
									  const ISC_UCHAR*,
									  const ISC_UCHAR*);

ISC_STATUS ISC_EXPORT isc_blob_gen_bpb(ISC_STATUS*,
									   const ISC_BLOB_DESC*,
									   const ISC_BLOB_DESC*,
									   unsigned short,
									   ISC_UCHAR*,
									   unsigned short*);

ISC_STATUS ISC_EXPORT isc_blob_info(ISC_STATUS*,
									isc_blob_handle*,
									short,
									const ISC_SCHAR*,
									short,
									ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_blob_lookup_desc(ISC_STATUS*,
										   isc_db_handle*,
										   isc_tr_handle*,
										   const ISC_UCHAR*,
										   const ISC_UCHAR*,
										   ISC_BLOB_DESC*,
										   ISC_UCHAR*);

ISC_STATUS ISC_EXPORT isc_blob_set_desc(ISC_STATUS*,
										const ISC_UCHAR*,
										const ISC_UCHAR*,
										short,
										short,
										short,
										ISC_BLOB_DESC*);

ISC_STATUS ISC_EXPORT isc_cancel_blob(ISC_STATUS *,
									  isc_blob_handle *);

ISC_STATUS ISC_EXPORT isc_cancel_events(ISC_STATUS *,
										isc_db_handle *,
										ISC_LONG *);

ISC_STATUS ISC_EXPORT isc_close_blob(ISC_STATUS *,
									 isc_blob_handle *);

ISC_STATUS ISC_EXPORT isc_commit_retaining(ISC_STATUS *,
										   isc_tr_handle *);

ISC_STATUS ISC_EXPORT isc_commit_transaction(ISC_STATUS *,
											 isc_tr_handle *);

ISC_STATUS ISC_EXPORT isc_create_blob(ISC_STATUS*,
									  isc_db_handle*,
									  isc_tr_handle*,
									  isc_blob_handle*,
									  ISC_QUAD*);

ISC_STATUS ISC_EXPORT isc_create_blob2(ISC_STATUS*,
									   isc_db_handle*,
									   isc_tr_handle*,
									   isc_blob_handle*,
									   ISC_QUAD*,
									   short,
									   const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_create_database(ISC_STATUS*,
										  short,
										  const ISC_SCHAR*,
										  isc_db_handle*,
										  short,
										  const ISC_SCHAR*,
										  short);

ISC_STATUS ISC_EXPORT isc_database_info(ISC_STATUS*,
										isc_db_handle*,
										short,
										const ISC_SCHAR*,
										short,
										ISC_SCHAR*);

void ISC_EXPORT isc_decode_date(const ISC_QUAD*,
								void*);

void ISC_EXPORT isc_decode_sql_date(const ISC_DATE*,
									void*);

void ISC_EXPORT isc_decode_sql_time(const ISC_TIME*,
									void*);

void ISC_EXPORT isc_decode_timestamp(const ISC_TIMESTAMP*,
									 void*);

ISC_STATUS ISC_EXPORT isc_detach_database(ISC_STATUS *,
										  isc_db_handle *);

ISC_STATUS ISC_EXPORT isc_drop_database(ISC_STATUS *,
										isc_db_handle *);

ISC_STATUS ISC_EXPORT isc_dsql_allocate_statement(ISC_STATUS *,
												  isc_db_handle *,
												  isc_stmt_handle *);

ISC_STATUS ISC_EXPORT isc_dsql_alloc_statement2(ISC_STATUS *,
												isc_db_handle *,
												isc_stmt_handle *);

ISC_STATUS ISC_EXPORT isc_dsql_describe(ISC_STATUS *,
										isc_stmt_handle *,
										unsigned short,
										XSQLDA *);

ISC_STATUS ISC_EXPORT isc_dsql_describe_bind(ISC_STATUS *,
											 isc_stmt_handle *,
											 unsigned short,
											 XSQLDA *);

ISC_STATUS ISC_EXPORT isc_dsql_exec_immed2(ISC_STATUS*,
										   isc_db_handle*,
										   isc_tr_handle*,
										   unsigned short,
										   const ISC_SCHAR*,
										   unsigned short,
										   const XSQLDA*,
										   const XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_execute(ISC_STATUS*,
									   isc_tr_handle*,
									   isc_stmt_handle*,
									   unsigned short,
									   const XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_execute2(ISC_STATUS*,
										isc_tr_handle*,
										isc_stmt_handle*,
										unsigned short,
										const XSQLDA*,
										const XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_execute_immediate(ISC_STATUS*,
												 isc_db_handle*,
												 isc_tr_handle*,
												 unsigned short,
												 const ISC_SCHAR*,
												 unsigned short,
												 const XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_fetch(ISC_STATUS *,
									 isc_stmt_handle *,
									 unsigned short,
									 const XSQLDA *);

ISC_STATUS ISC_EXPORT isc_dsql_finish(isc_db_handle *);

ISC_STATUS ISC_EXPORT isc_dsql_free_statement(ISC_STATUS *,
											  isc_stmt_handle *,
											  unsigned short);

ISC_STATUS ISC_EXPORT isc_dsql_insert(ISC_STATUS*,
									  isc_stmt_handle*,
									  unsigned short,
									  XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_prepare(ISC_STATUS*,
									   isc_tr_handle*,
									   isc_stmt_handle*,
									   unsigned short,
									   const ISC_SCHAR*,
									   unsigned short,
									   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_dsql_set_cursor_name(ISC_STATUS*,
											   isc_stmt_handle*,
											   const ISC_SCHAR*,
											   unsigned short);

ISC_STATUS ISC_EXPORT isc_dsql_sql_info(ISC_STATUS*,
										isc_stmt_handle*,
										short,
										const ISC_SCHAR*,
										short,
										ISC_SCHAR*);

void ISC_EXPORT isc_encode_date(const void*,
								ISC_QUAD*);

void ISC_EXPORT isc_encode_sql_date(const void*,
									ISC_DATE*);

void ISC_EXPORT isc_encode_sql_time(const void*,
									ISC_TIME*);

void ISC_EXPORT isc_encode_timestamp(const void*,
									 ISC_TIMESTAMP*);

ISC_LONG ISC_EXPORT_VARARG isc_event_block(ISC_UCHAR**,
										   ISC_UCHAR**,
										   ISC_USHORT, ...);

ISC_USHORT ISC_EXPORT isc_event_block_a(ISC_SCHAR**,
										ISC_SCHAR**,
										ISC_USHORT,
										ISC_SCHAR**);

void ISC_EXPORT isc_event_block_s(ISC_SCHAR**,
								  ISC_SCHAR**,
								  ISC_USHORT,
								  ISC_SCHAR**,
								  ISC_USHORT*);

void ISC_EXPORT isc_event_counts(ISC_ULONG*,
								 short,
								 ISC_UCHAR*,
								 const ISC_UCHAR *);

/* 17 May 2001 - isc_expand_dpb is DEPRECATED */
void FB_API_DEPRECATED ISC_EXPORT_VARARG isc_expand_dpb(ISC_SCHAR**,
											  			short*, ...);

int ISC_EXPORT isc_modify_dpb(ISC_SCHAR**,
							  short*,
							  unsigned short,
							  const ISC_SCHAR*,
							  short);

ISC_LONG ISC_EXPORT isc_free(ISC_SCHAR *);

ISC_STATUS ISC_EXPORT isc_get_segment(ISC_STATUS *,
									  isc_blob_handle *,
									  unsigned short *,
									  unsigned short,
									  ISC_SCHAR *);

ISC_STATUS ISC_EXPORT isc_get_slice(ISC_STATUS*,
									isc_db_handle*,
									isc_tr_handle*,
									ISC_QUAD*,
									short,
									const ISC_SCHAR*,
									short,
									const ISC_LONG*,
									ISC_LONG,
									void*,
									ISC_LONG*);

/* CVC: This non-const signature is needed for compatibility, see gds.cpp. */
ISC_LONG FB_API_DEPRECATED ISC_EXPORT isc_interprete(ISC_SCHAR*,
									 ISC_STATUS**);

/* This const params version used in the engine and other places. */
ISC_LONG ISC_EXPORT fb_interpret(ISC_SCHAR*,
								 unsigned int,
								 const ISC_STATUS**);

ISC_STATUS ISC_EXPORT isc_open_blob(ISC_STATUS*,
									isc_db_handle*,
									isc_tr_handle*,
									isc_blob_handle*,
									ISC_QUAD*);

ISC_STATUS ISC_EXPORT isc_open_blob2(ISC_STATUS*,
									 isc_db_handle*,
									 isc_tr_handle*,
									 isc_blob_handle*,
									 ISC_QUAD*,
									 ISC_USHORT,
									 const ISC_UCHAR*);

ISC_STATUS ISC_EXPORT isc_prepare_transaction2(ISC_STATUS*,
											   isc_tr_handle*,
											   ISC_USHORT,
											   const ISC_UCHAR*);

void ISC_EXPORT isc_print_sqlerror(ISC_SHORT,
								   const ISC_STATUS*);

ISC_STATUS ISC_EXPORT isc_print_status(const ISC_STATUS*);

ISC_STATUS ISC_EXPORT isc_put_segment(ISC_STATUS*,
									  isc_blob_handle*,
									  unsigned short,
									  const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_put_slice(ISC_STATUS*,
									isc_db_handle*,
									isc_tr_handle*,
									ISC_QUAD*,
									short,
									const ISC_SCHAR*,
									short,
									const ISC_LONG*,
									ISC_LONG,
									void*);

ISC_STATUS ISC_EXPORT isc_que_events(ISC_STATUS*,
									 isc_db_handle*,
									 ISC_LONG*,
									 short,
									 const ISC_UCHAR*,
									 ISC_EVENT_CALLBACK,
									 void*);

ISC_STATUS ISC_EXPORT isc_rollback_retaining(ISC_STATUS*,
											 isc_tr_handle*);

ISC_STATUS ISC_EXPORT isc_rollback_transaction(ISC_STATUS*,
											   isc_tr_handle*);

ISC_STATUS ISC_EXPORT isc_start_multiple(ISC_STATUS*,
										 isc_tr_handle*,
										 short,
										 void *);

ISC_STATUS ISC_EXPORT_VARARG isc_start_transaction(ISC_STATUS*,
												   isc_tr_handle*,
												   short, ...);

ISC_STATUS ISC_EXPORT fb_disconnect_transaction(ISC_STATUS*, isc_tr_handle*);

ISC_LONG ISC_EXPORT isc_sqlcode(const ISC_STATUS*);

void ISC_EXPORT isc_sqlcode_s(const ISC_STATUS*,
							  ISC_ULONG*);

void ISC_EXPORT fb_sqlstate(char*,
							const ISC_STATUS*);

void ISC_EXPORT isc_sql_interprete(short,
								   ISC_SCHAR*,
								   short);

ISC_STATUS ISC_EXPORT isc_transaction_info(ISC_STATUS*,
										   isc_tr_handle*,
										   short,
										   const ISC_SCHAR*,
										   short,
										   ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_transact_request(ISC_STATUS*,
										   isc_db_handle*,
										   isc_tr_handle*,
										   unsigned short,
										   ISC_SCHAR*,
										   unsigned short,
										   ISC_SCHAR*,
										   unsigned short,
										   ISC_SCHAR*);

ISC_LONG ISC_EXPORT isc_vax_integer(const ISC_SCHAR*,
									short);

ISC_INT64 ISC_EXPORT isc_portable_integer(const ISC_UCHAR*,
										  short);

/*************************************/
/* Security Functions and structures */
/*************************************/

#define sec_uid_spec		    0x01
#define sec_gid_spec		    0x02
#define sec_server_spec		    0x04
#define sec_password_spec	    0x08
#define sec_group_name_spec	    0x10
#define sec_first_name_spec	    0x20
#define sec_middle_name_spec        0x40
#define sec_last_name_spec	    0x80
#define sec_dba_user_name_spec      0x100
#define sec_dba_password_spec       0x200

#define sec_protocol_tcpip            1
#define sec_protocol_netbeui          2
#define sec_protocol_spx              3 /* -- Deprecated Protocol. Declaration retained for compatibility   */
#define sec_protocol_local            4

typedef struct {
	short sec_flags;			/* which fields are specified */
	int uid;					/* the user's id */
	int gid;					/* the user's group id */
	int protocol;				/* protocol to use for connection */
	ISC_SCHAR *server;				/* server to administer */
	ISC_SCHAR *user_name;			/* the user's name */
	ISC_SCHAR *password;				/* the user's password */
	ISC_SCHAR *group_name;			/* the group name */
	ISC_SCHAR *first_name;			/* the user's first name */
	ISC_SCHAR *middle_name;			/* the user's middle name */
	ISC_SCHAR *last_name;			/* the user's last name */
	ISC_SCHAR *dba_user_name;		/* the dba user name */
	ISC_SCHAR *dba_password;			/* the dba password */
} USER_SEC_DATA;

ISC_STATUS ISC_EXPORT isc_add_user(ISC_STATUS*, const USER_SEC_DATA*);

ISC_STATUS ISC_EXPORT isc_delete_user(ISC_STATUS*, const USER_SEC_DATA*);

ISC_STATUS ISC_EXPORT isc_modify_user(ISC_STATUS*, const USER_SEC_DATA*);

/**********************************/
/*  Other OSRI functions          */
/**********************************/

ISC_STATUS ISC_EXPORT isc_compile_request(ISC_STATUS*,
										  isc_db_handle*,
										  isc_req_handle*,
										  short,
										  const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_compile_request2(ISC_STATUS*,
										   isc_db_handle*,
										   isc_req_handle*,
										   short,
										   const ISC_SCHAR*);

ISC_STATUS FB_API_DEPRECATED ISC_EXPORT isc_ddl(ISC_STATUS*,
							  isc_db_handle*,
							  isc_tr_handle*,
							  short,
							  const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_prepare_transaction(ISC_STATUS*,
											  isc_tr_handle*);


ISC_STATUS ISC_EXPORT isc_receive(ISC_STATUS*,
								  isc_req_handle*,
								  short,
								  short,
								  void*,
								  short);

ISC_STATUS ISC_EXPORT isc_reconnect_transaction(ISC_STATUS*,
												isc_db_handle*,
												isc_tr_handle*,
												short,
												const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_release_request(ISC_STATUS*,
										  isc_req_handle*);

ISC_STATUS ISC_EXPORT isc_request_info(ISC_STATUS*,
									   isc_req_handle*,
									   short,
									   short,
									   const ISC_SCHAR*,
									   short,
									   ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_seek_blob(ISC_STATUS*,
									isc_blob_handle*,
									short,
									ISC_LONG,
									ISC_LONG*);

ISC_STATUS ISC_EXPORT isc_send(ISC_STATUS*,
							   isc_req_handle*,
							   short,
							   short,
							   const void*,
							   short);

ISC_STATUS ISC_EXPORT isc_start_and_send(ISC_STATUS*,
										 isc_req_handle*,
										 isc_tr_handle*,
										 short,
										 short,
										 const void*,
										 short);

ISC_STATUS ISC_EXPORT isc_start_request(ISC_STATUS *,
										isc_req_handle *,
										isc_tr_handle *,
										short);

ISC_STATUS ISC_EXPORT isc_unwind_request(ISC_STATUS *,
										 isc_tr_handle *,
										 short);

ISC_STATUS ISC_EXPORT isc_wait_for_event(ISC_STATUS*,
										 isc_db_handle*,
										 short,
										 const ISC_UCHAR*,
										 ISC_UCHAR*);


/*****************************/
/* Other Sql functions       */
/*****************************/

ISC_STATUS ISC_EXPORT isc_close(ISC_STATUS*,
								const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_declare(ISC_STATUS*,
								  const ISC_SCHAR*,
								  const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_describe(ISC_STATUS*,
								   const ISC_SCHAR*,
								   XSQLDA *);

ISC_STATUS ISC_EXPORT isc_describe_bind(ISC_STATUS*,
										const ISC_SCHAR*,
										XSQLDA*);

ISC_STATUS ISC_EXPORT isc_execute(ISC_STATUS*,
								  isc_tr_handle*,
								  const ISC_SCHAR*,
								  XSQLDA*);

ISC_STATUS ISC_EXPORT isc_execute_immediate(ISC_STATUS*,
											isc_db_handle*,
											isc_tr_handle*,
											short*,
											const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_fetch(ISC_STATUS*,
								const ISC_SCHAR*,
								XSQLDA*);

ISC_STATUS ISC_EXPORT isc_open(ISC_STATUS*,
							   isc_tr_handle*,
							   const ISC_SCHAR*,
							   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_prepare(ISC_STATUS*,
								  isc_db_handle*,
								  isc_tr_handle*,
								  const ISC_SCHAR*,
								  const short*,
								  const ISC_SCHAR*,
								  XSQLDA*);


/*************************************/
/* Other Dynamic sql functions       */
/*************************************/

ISC_STATUS ISC_EXPORT isc_dsql_execute_m(ISC_STATUS*,
										 isc_tr_handle*,
										 isc_stmt_handle*,
										 unsigned short,
										 const ISC_SCHAR*,
										 unsigned short,
										 unsigned short,
										 ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_execute2_m(ISC_STATUS*,
										  isc_tr_handle*,
										  isc_stmt_handle*,
										  unsigned short,
										  const ISC_SCHAR*,
										  unsigned short,
										  unsigned short,
										  ISC_SCHAR*,
										  unsigned short,
										  ISC_SCHAR*,
										  unsigned short,
										  unsigned short,
										  ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_execute_immediate_m(ISC_STATUS*,
												   isc_db_handle*,
												   isc_tr_handle*,
												   unsigned short,
												   const ISC_SCHAR*,
												   unsigned short,
												   unsigned short,
												   ISC_SCHAR*,
												   unsigned short,
												   unsigned short,
												   ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_exec_immed3_m(ISC_STATUS*,
											 isc_db_handle*,
											 isc_tr_handle*,
											 unsigned short,
											 const ISC_SCHAR*,
											 unsigned short,
											 unsigned short,
											 ISC_SCHAR*,
											 unsigned short,
											 unsigned short,
											 const ISC_SCHAR*,
											 unsigned short,
											 ISC_SCHAR*,
											 unsigned short,
											 unsigned short,
											 ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_fetch_m(ISC_STATUS*,
									   isc_stmt_handle*,
									   unsigned short,
									   ISC_SCHAR*,
									   unsigned short,
									   unsigned short,
									   ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_insert_m(ISC_STATUS*,
										isc_stmt_handle*,
										unsigned short,
										const ISC_SCHAR*,
										unsigned short,
										unsigned short,
										const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_prepare_m(ISC_STATUS*,
										 isc_tr_handle*,
										 isc_stmt_handle*,
										 unsigned short,
										 const ISC_SCHAR*,
										 unsigned short,
										 unsigned short,
										 const ISC_SCHAR*,
										 unsigned short,
										 ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_dsql_release(ISC_STATUS*,
									   const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_close(ISC_STATUS*,
										   const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_declare(ISC_STATUS*,
											 const ISC_SCHAR*,
											 const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_describe(ISC_STATUS*,
											  const ISC_SCHAR*,
											  unsigned short,
											  XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_describe_bind(ISC_STATUS*,
												   const ISC_SCHAR*,
												   unsigned short,
												   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_execute(ISC_STATUS*,
											 isc_tr_handle*,
											 const ISC_SCHAR*,
											 unsigned short,
											 XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_execute2(ISC_STATUS*,
											  isc_tr_handle*,
											  const ISC_SCHAR*,
											  unsigned short,
											  XSQLDA*,
											  XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_execute_immed(ISC_STATUS*,
												   isc_db_handle*,
												   isc_tr_handle*,
												   unsigned short,
												   const ISC_SCHAR*,
												   unsigned short,
												   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_fetch(ISC_STATUS*,
										   const ISC_SCHAR*,
										   unsigned short,
										   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_fetch_a(ISC_STATUS*,
											 int*,
											 const ISC_SCHAR*,
											 ISC_USHORT,
											 XSQLDA*);

void ISC_EXPORT isc_embed_dsql_length(const ISC_UCHAR*,
									  ISC_USHORT*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_open(ISC_STATUS*,
										  isc_tr_handle*,
										  const ISC_SCHAR*,
										  unsigned short,
										  XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_open2(ISC_STATUS*,
										   isc_tr_handle*,
										   const ISC_SCHAR*,
										   unsigned short,
										   XSQLDA*,
										   XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_insert(ISC_STATUS*,
											const ISC_SCHAR*,
											unsigned short,
											XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_prepare(ISC_STATUS*,
											 isc_db_handle*,
											 isc_tr_handle*,
											 const ISC_SCHAR*,
											 unsigned short,
											 const ISC_SCHAR*,
											 unsigned short,
											 XSQLDA*);

ISC_STATUS ISC_EXPORT isc_embed_dsql_release(ISC_STATUS*,
											 const ISC_SCHAR*);


/******************************/
/* Other Blob functions       */
/******************************/

BSTREAM* ISC_EXPORT BLOB_open(isc_blob_handle,
									  ISC_SCHAR*,
									  int);

int ISC_EXPORT BLOB_put(ISC_SCHAR,
						BSTREAM*);

int ISC_EXPORT BLOB_close(BSTREAM*);

int ISC_EXPORT BLOB_get(BSTREAM*);

int ISC_EXPORT BLOB_display(ISC_QUAD*,
							isc_db_handle,
							isc_tr_handle,
							const ISC_SCHAR*);

int ISC_EXPORT BLOB_dump(ISC_QUAD*,
						 isc_db_handle,
						 isc_tr_handle,
						 const ISC_SCHAR*);

int ISC_EXPORT BLOB_edit(ISC_QUAD*,
						 isc_db_handle,
						 isc_tr_handle,
						 const ISC_SCHAR*);

int ISC_EXPORT BLOB_load(ISC_QUAD*,
						 isc_db_handle,
						 isc_tr_handle,
						 const ISC_SCHAR*);

int ISC_EXPORT BLOB_text_dump(ISC_QUAD*,
							  isc_db_handle,
							  isc_tr_handle,
							  const ISC_SCHAR*);

int ISC_EXPORT BLOB_text_load(ISC_QUAD*,
							  isc_db_handle,
							  isc_tr_handle,
							  const ISC_SCHAR*);

BSTREAM* ISC_EXPORT Bopen(ISC_QUAD*,
								  isc_db_handle,
								  isc_tr_handle,
								  const ISC_SCHAR*);


/******************************/
/* Other Misc functions       */
/******************************/

ISC_LONG ISC_EXPORT isc_ftof(const ISC_SCHAR*,
							 const unsigned short,
							 ISC_SCHAR*,
							 const unsigned short);

ISC_STATUS ISC_EXPORT isc_print_blr(const ISC_SCHAR*,
									ISC_PRINT_CALLBACK,
									void*,
									short);

int ISC_EXPORT fb_print_blr(const ISC_UCHAR*,
							ISC_ULONG,
							ISC_PRINT_CALLBACK,
							void*,
							short);

void ISC_EXPORT isc_set_debug(int);

void ISC_EXPORT isc_qtoq(const ISC_QUAD*,
						 ISC_QUAD*);

void ISC_EXPORT isc_vtof(const ISC_SCHAR*,
						 ISC_SCHAR*,
						 unsigned short);

void ISC_EXPORT isc_vtov(const ISC_SCHAR*,
						 ISC_SCHAR*,
						 short);

int ISC_EXPORT isc_version(isc_db_handle*,
						   ISC_VERSION_CALLBACK,
						   void*);

ISC_LONG FB_API_DEPRECATED ISC_EXPORT isc_reset_fpe(ISC_USHORT);

uintptr_t	ISC_EXPORT isc_baddress(ISC_SCHAR*);
void		ISC_EXPORT isc_baddress_s(const ISC_SCHAR*,
								  uintptr_t*);

/*****************************************/
/* Service manager functions             */
/*****************************************/

#define ADD_SPB_LENGTH(p, length)	{*(p)++ = (length); \
    					 *(p)++ = (length) >> 8;}

#define ADD_SPB_NUMERIC(p, data)	{*(p)++ = (ISC_SCHAR) (ISC_UCHAR) (data); \
    					 *(p)++ = (ISC_SCHAR) (ISC_UCHAR) ((data) >> 8); \
					 *(p)++ = (ISC_SCHAR) (ISC_UCHAR) ((data) >> 16); \
					 *(p)++ = (ISC_SCHAR) (ISC_UCHAR) ((data) >> 24);}

ISC_STATUS ISC_EXPORT isc_service_attach(ISC_STATUS*,
										 unsigned short,
										 const ISC_SCHAR*,
										 isc_svc_handle*,
										 unsigned short,
										 const ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_service_detach(ISC_STATUS *,
										 isc_svc_handle *);

ISC_STATUS ISC_EXPORT isc_service_query(ISC_STATUS*,
										isc_svc_handle*,
										isc_resv_handle*,
										unsigned short,
										const ISC_SCHAR*,
										unsigned short,
										const ISC_SCHAR*,
										unsigned short,
										ISC_SCHAR*);

ISC_STATUS ISC_EXPORT isc_service_start(ISC_STATUS*,
										isc_svc_handle*,
										isc_resv_handle*,
										unsigned short,
										const ISC_SCHAR*);

/***********************/
/* Shutdown and cancel */
/***********************/

int ISC_EXPORT fb_shutdown(unsigned int, const int);

ISC_STATUS ISC_EXPORT fb_shutdown_callback(ISC_STATUS*,
										   FB_SHUTDOWN_CALLBACK,
										   const int,
										   void*);

ISC_STATUS ISC_EXPORT fb_cancel_operation(ISC_STATUS*,
										  isc_db_handle*,
										  ISC_USHORT);

/********************************/
/* Client information functions */
/********************************/

void ISC_EXPORT isc_get_client_version ( ISC_SCHAR  *);
int  ISC_EXPORT isc_get_client_major_version ();
int  ISC_EXPORT isc_get_client_minor_version ();

#ifdef __cplusplus
}	/* extern "C" */
#endif


/***************************************************/
/* Actions to pass to the blob filter (ctl_source) */
/***************************************************/

#define isc_blob_filter_open             0
#define isc_blob_filter_get_segment      1
#define isc_blob_filter_close            2
#define isc_blob_filter_create           3
#define isc_blob_filter_put_segment      4
#define isc_blob_filter_alloc            5
#define isc_blob_filter_free             6
#define isc_blob_filter_seek             7

/*******************/
/* Blr definitions */
/*******************/

#include "blr.h"

#include "consts_pub.h"

/*********************************/
/* Information call declarations */
/*********************************/

#include "../jrd/inf_pub.h"

#include "iberror.h"

#endif /* JRD_IBASE_H */

