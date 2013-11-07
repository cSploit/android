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

#ifndef INCLUDE_TYPES_PUB_H
#define INCLUDE_TYPES_PUB_H

#include <stddef.h>

#if defined(__GNUC__) || defined (__HP_cc) || defined (__HP_aCC)
#include <inttypes.h>
#else

#if !defined(_INTPTR_T_DEFINED)
#if defined(_WIN64)
typedef __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#else
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif
#endif

#endif

/******************************************************************/
/* API handles                                                    */
/******************************************************************/

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
typedef unsigned int	FB_API_HANDLE;
#else
typedef void*		FB_API_HANDLE;
#endif

/******************************************************************/
/* Status vector                                                  */
/******************************************************************/

typedef intptr_t ISC_STATUS;

#define ISC_STATUS_LENGTH	20
typedef ISC_STATUS ISC_STATUS_ARRAY[ISC_STATUS_LENGTH];

/* SQL State as defined in the SQL Standard. */
#define FB_SQLSTATE_LENGTH	5
#define FB_SQLSTATE_SIZE	(FB_SQLSTATE_LENGTH + 1)
typedef char FB_SQLSTATE_STRING[FB_SQLSTATE_SIZE];

/******************************************************************/
/* Define type, export and other stuff based on c/c++ and Windows */
/******************************************************************/
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#define  ISC_EXPORT	__stdcall
	#define  ISC_EXPORT_VARARG	__cdecl
#else
	#define  ISC_EXPORT
	#define  ISC_EXPORT_VARARG
#endif

/*
 * It is difficult to detect 64-bit long from the redistributable header
 * we do not care of 16-bit platforms anymore thus we may use plain "int"
 * which is 32-bit on all platforms we support
 *
 * We'll move to this definition in future API releases.
 *
 */

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__)
typedef	int		ISC_LONG;
typedef	unsigned int	ISC_ULONG;
#else
typedef	signed long	ISC_LONG;
typedef	unsigned long	ISC_ULONG;
#endif

typedef	signed short	ISC_SHORT;
typedef	unsigned short	ISC_USHORT;

typedef	unsigned char	ISC_UCHAR;
typedef char		ISC_SCHAR;

/*******************************************************************/
/* 64 bit Integers                                                 */
/*******************************************************************/

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) && !defined(__GNUC__)
typedef __int64				ISC_INT64;
typedef unsigned __int64	ISC_UINT64;
#else
typedef long long int			ISC_INT64;
typedef unsigned long long int	ISC_UINT64;
#endif

/*******************************************************************/
/* Time & Date support                                             */
/*******************************************************************/

#ifndef ISC_TIMESTAMP_DEFINED
typedef int			ISC_DATE;
typedef unsigned int	ISC_TIME;
typedef struct
{
	ISC_DATE timestamp_date;
	ISC_TIME timestamp_time;
} ISC_TIMESTAMP;
#define ISC_TIMESTAMP_DEFINED
#endif	/* ISC_TIMESTAMP_DEFINED */

/*******************************************************************/
/* Blob Id support                                                 */
/*******************************************************************/

struct GDS_QUAD_t {
	ISC_LONG gds_quad_high;
	ISC_ULONG gds_quad_low;
};

typedef struct GDS_QUAD_t GDS_QUAD;
typedef struct GDS_QUAD_t ISC_QUAD;

#define	isc_quad_high	gds_quad_high
#define	isc_quad_low	gds_quad_low

typedef int (*FB_SHUTDOWN_CALLBACK)(const int reason, const int mask, void* arg);

#endif /* INCLUDE_TYPES_PUB_H */


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


#ifndef JRD_DSC_PUB_H
#define JRD_DSC_PUB_H


/*
 * The following flags are used in an internal structure dsc (dsc.h) or in the external one paramdsc (ibase.h)
 */

/* values for dsc_flags
 * Note: DSC_null is only reliably set for local variables (blr_variable)
 */
#define DSC_null		1
#define DSC_no_subtype	2	/* dsc has no sub type specified */
#define DSC_nullable  	4	/* not stored. instead, is derived
							   from metadata primarily to flag
							   SQLDA (in DSQL)               */

#define dtype_unknown	0
#define dtype_text		1
#define dtype_cstring	2
#define dtype_varying	3

#define dtype_packed	6
#define dtype_byte		7
#define dtype_short		8
#define dtype_long		9
#define dtype_quad		10
#define dtype_real		11
#define dtype_double	12
#define dtype_d_float	13
#define dtype_sql_date	14
#define dtype_sql_time	15
#define dtype_timestamp	16
#define dtype_blob		17
#define dtype_array		18
#define dtype_int64		19
#define dtype_dbkey		20
#define DTYPE_TYPE_MAX	21

#define ISC_TIME_SECONDS_PRECISION		10000
#define ISC_TIME_SECONDS_PRECISION_SCALE	(-4)

#endif /* JRD_DSC_PUB_H */

#endif /* !defined(JRD_DSC_H) */

/***************************/
/* Dynamic SQL definitions */
/***************************/


#ifndef DSQL_SQLDA_PUB_H
#define DSQL_SQLDA_PUB_H

/* Definitions for DSQL free_statement routine */

#define DSQL_close		1
#define DSQL_drop		2
#define DSQL_unprepare	4

/* Declare the extended SQLDA */

typedef struct
{
	ISC_SHORT	sqltype;			/* datatype of field */
	ISC_SHORT	sqlscale;			/* scale factor */
	ISC_SHORT	sqlsubtype;			/* datatype subtype - currently BLOBs only */
	ISC_SHORT	sqllen;				/* length of data area */
	ISC_SCHAR*	sqldata;			/* address of data */
	ISC_SHORT*	sqlind;				/* address of indicator variable */
	ISC_SHORT	sqlname_length;		/* length of sqlname field */
	ISC_SCHAR	sqlname[32];		/* name of field, name length + space for NULL */
	ISC_SHORT	relname_length;		/* length of relation name */
	ISC_SCHAR	relname[32];		/* field's relation name + space for NULL */
	ISC_SHORT	ownname_length;		/* length of owner name */
	ISC_SCHAR	ownname[32];		/* relation's owner name + space for NULL */
	ISC_SHORT	aliasname_length;	/* length of alias name */
	ISC_SCHAR	aliasname[32];		/* relation's alias name + space for NULL */
} XSQLVAR;

#define SQLDA_VERSION1		1

typedef struct
{
	ISC_SHORT	version;			/* version of this XSQLDA */
	ISC_SCHAR	sqldaid[8];			/* XSQLDA name field */
	ISC_LONG	sqldabc;			/* length in bytes of SQLDA */
	ISC_SHORT	sqln;				/* number of fields allocated */
	ISC_SHORT	sqld;				/* actual number of fields */
	XSQLVAR	sqlvar[1];			/* first field address */
} XSQLDA;

#define XSQLDA_LENGTH(n)	(sizeof (XSQLDA) + (n - 1) * sizeof (XSQLVAR))

#define SQL_TEXT                           452
#define SQL_VARYING                        448
#define SQL_SHORT                          500
#define SQL_LONG                           496
#define SQL_FLOAT                          482
#define SQL_DOUBLE                         480
#define SQL_D_FLOAT                        530
#define SQL_TIMESTAMP                      510
#define SQL_BLOB                           520
#define SQL_ARRAY                          540
#define SQL_QUAD                           550
#define SQL_TYPE_TIME                      560
#define SQL_TYPE_DATE                      570
#define SQL_INT64                          580
#define SQL_NULL                         32766

/* Historical alias for pre v6 code */
#define SQL_DATE                           SQL_TIMESTAMP

/***************************/
/* SQL Dialects            */
/***************************/

#define SQL_DIALECT_V5				1	/* meaning is same as DIALECT_xsqlda */
#define SQL_DIALECT_V6_TRANSITION	2	/* flagging anything that is delimited
										   by double quotes as an error and
										   flagging keyword DATE as an error */
#define SQL_DIALECT_V6				3	/* supports SQL delimited identifier,
										   SQLDATE/DATE, TIME, TIMESTAMP,
										   CURRENT_DATE, CURRENT_TIME,
										   CURRENT_TIMESTAMP, and 64-bit exact
										   numeric type */
#define SQL_DIALECT_CURRENT		SQL_DIALECT_V6	/* latest IB DIALECT */

#endif /* DSQL_SQLDA_PUB_H */

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

#ifndef JRD_BLR_H
#define JRD_BLR_H

/*  WARNING: if you add a new BLR representing a data type, and the value
 *           is greater than the numerically greatest value which now
 *           represents a data type, you must change the define for
 *           DTYPE_BLR_MAX in jrd/align.h, and add the necessary entries
 *           to all the arrays in that file.
 */

#define blr_text		(unsigned char)14
#define blr_text2		(unsigned char)15	/* added in 3.2 JPN */
#define blr_short		(unsigned char)7
#define blr_long		(unsigned char)8
#define blr_quad		(unsigned char)9
#define blr_float		(unsigned char)10
#define blr_double		(unsigned char)27
#define blr_d_float		(unsigned char)11
#define blr_timestamp		(unsigned char)35
#define blr_varying		(unsigned char)37
#define blr_varying2		(unsigned char)38	/* added in 3.2 JPN */
#define blr_blob		(unsigned short)261
#define blr_cstring		(unsigned char)40
#define blr_cstring2    	(unsigned char)41	/* added in 3.2 JPN */
#define blr_blob_id     	(unsigned char)45	/* added from gds.h */
#define blr_sql_date		(unsigned char)12
#define blr_sql_time		(unsigned char)13
#define blr_int64               (unsigned char)16
#define blr_blob2			(unsigned char)17
#define blr_domain_name		(unsigned char)18
#define blr_domain_name2	(unsigned char)19
#define blr_not_nullable	(unsigned char)20
#define blr_column_name		(unsigned char)21
#define blr_column_name2	(unsigned char)22

// first sub parameter for blr_domain_name[2]
#define blr_domain_type_of	(unsigned char)0
#define blr_domain_full		(unsigned char)1

/* Historical alias for pre V6 applications */
#define blr_date		blr_timestamp

#define blr_inner		(unsigned char)0
#define blr_left		(unsigned char)1
#define blr_right		(unsigned char)2
#define blr_full		(unsigned char)3

#define blr_gds_code		(unsigned char)0
#define blr_sql_code		(unsigned char)1
#define blr_exception		(unsigned char)2
#define blr_trigger_code 	(unsigned char)3
#define blr_default_code 	(unsigned char)4
#define blr_raise			(unsigned char)5
#define blr_exception_msg	(unsigned char)6

#define blr_version4		(unsigned char)4
#define blr_version5		(unsigned char)5
#define blr_eoc			(unsigned char)76
#define blr_end			(unsigned char)255

#define blr_assignment		(unsigned char)1
#define blr_begin		(unsigned char)2
#define blr_dcl_variable  	(unsigned char)3	/* added from gds.h */
#define blr_message		(unsigned char)4
#define blr_erase		(unsigned char)5
#define blr_fetch		(unsigned char)6
#define blr_for			(unsigned char)7
#define blr_if			(unsigned char)8
#define blr_loop		(unsigned char)9
#define blr_modify		(unsigned char)10
#define blr_handler		(unsigned char)11
#define blr_receive		(unsigned char)12
#define blr_select		(unsigned char)13
#define blr_send		(unsigned char)14
#define blr_store		(unsigned char)15
#define blr_label		(unsigned char)17
#define blr_leave		(unsigned char)18
#define blr_store2		(unsigned char)19
#define blr_post		(unsigned char)20
#define blr_literal		(unsigned char)21
#define blr_dbkey		(unsigned char)22
#define blr_field		(unsigned char)23
#define blr_fid			(unsigned char)24
#define blr_parameter		(unsigned char)25
#define blr_variable		(unsigned char)26
#define blr_average		(unsigned char)27
#define blr_count		(unsigned char)28
#define blr_maximum		(unsigned char)29
#define blr_minimum		(unsigned char)30
#define blr_total		(unsigned char)31
/* count 2
#define blr_count2		32
*/
#define blr_add			(unsigned char)34
#define blr_subtract		(unsigned char)35
#define blr_multiply		(unsigned char)36
#define blr_divide		(unsigned char)37
#define blr_negate		(unsigned char)38
#define blr_concatenate		(unsigned char)39
#define blr_substring		(unsigned char)40
#define blr_parameter2		(unsigned char)41
#define blr_from		(unsigned char)42
#define blr_via			(unsigned char)43
#define blr_parameter2_old	(unsigned char)44	/* Confusion */
#define blr_user_name   	(unsigned char)44	/* added from gds.h */
#define blr_null		(unsigned char)45

#define blr_equiv			(unsigned char)46
#define blr_eql			(unsigned char)47
#define blr_neq			(unsigned char)48
#define blr_gtr			(unsigned char)49
#define blr_geq			(unsigned char)50
#define blr_lss			(unsigned char)51
#define blr_leq			(unsigned char)52
#define blr_containing		(unsigned char)53
#define blr_matching		(unsigned char)54
#define blr_starting		(unsigned char)55
#define blr_between		(unsigned char)56
#define blr_or			(unsigned char)57
#define blr_and			(unsigned char)58
#define blr_not			(unsigned char)59
#define blr_any			(unsigned char)60
#define blr_missing		(unsigned char)61
#define blr_unique		(unsigned char)62
#define blr_like		(unsigned char)63

//#define blr_stream      	(unsigned char)65
//#define blr_set_index   	(unsigned char)66

#define blr_rse			(unsigned char)67
#define blr_first		(unsigned char)68
#define blr_project		(unsigned char)69
#define blr_sort		(unsigned char)70
#define blr_boolean		(unsigned char)71
#define blr_ascending		(unsigned char)72
#define blr_descending		(unsigned char)73
#define blr_relation		(unsigned char)74
#define blr_rid			(unsigned char)75
#define blr_union		(unsigned char)76
#define blr_map			(unsigned char)77
#define blr_group_by		(unsigned char)78
#define blr_aggregate		(unsigned char)79
#define blr_join_type		(unsigned char)80

#define blr_agg_count		(unsigned char)83
#define blr_agg_max		(unsigned char)84
#define blr_agg_min		(unsigned char)85
#define blr_agg_total		(unsigned char)86
#define blr_agg_average		(unsigned char)87
#define	blr_parameter3		(unsigned char)88	/* same as Rdb definition */
#define blr_run_max		(unsigned char)89
#define blr_run_min		(unsigned char)90
#define blr_run_total		(unsigned char)91
#define blr_run_average		(unsigned char)92
#define blr_agg_count2		(unsigned char)93
#define blr_agg_count_distinct	(unsigned char)94
#define blr_agg_total_distinct	(unsigned char)95
#define blr_agg_average_distinct (unsigned char)96

#define blr_function		(unsigned char)100
#define blr_gen_id		(unsigned char)101
#define blr_prot_mask		(unsigned char)102
#define blr_upcase		(unsigned char)103
#define blr_lock_state		(unsigned char)104
#define blr_value_if		(unsigned char)105
#define blr_matching2		(unsigned char)106
#define blr_index		(unsigned char)107
#define blr_ansi_like		(unsigned char)108
//#define blr_bookmark		(unsigned char)109
//#define blr_crack		(unsigned char)110
//#define blr_force_crack		(unsigned char)111
#define blr_seek		(unsigned char)112
//#define blr_find		(unsigned char)113

/* these indicate directions for blr_seek and blr_find */

#define blr_continue		(unsigned char)0
#define blr_forward		(unsigned char)1
#define blr_backward		(unsigned char)2
#define blr_bof_forward		(unsigned char)3
#define blr_eof_backward	(unsigned char)4

//#define blr_lock_relation 	(unsigned char)114
//#define blr_lock_record		(unsigned char)115
//#define blr_set_bookmark 	(unsigned char)116
//#define blr_get_bookmark 	(unsigned char)117

#define blr_run_count		(unsigned char)118	/* changed from 88 to avoid conflict with blr_parameter3 */
#define blr_rs_stream		(unsigned char)119
#define blr_exec_proc		(unsigned char)120
//#define blr_begin_range 	(unsigned char)121
//#define blr_end_range 		(unsigned char)122
//#define blr_delete_range 	(unsigned char)123
#define blr_procedure		(unsigned char)124
#define blr_pid			(unsigned char)125
#define blr_exec_pid		(unsigned char)126
#define blr_singular		(unsigned char)127
#define blr_abort		(unsigned char)128
#define blr_block	 	(unsigned char)129
#define blr_error_handler	(unsigned char)130

#define blr_cast		(unsigned char)131
//#define blr_release_lock	(unsigned char)132
//#define blr_release_locks	(unsigned char)133
#define blr_start_savepoint	(unsigned char)134
#define blr_end_savepoint	(unsigned char)135
//#define blr_find_dbkey		(unsigned char)136
//#define blr_range_relation	(unsigned char)137
//#define blr_delete_ranges	(unsigned char)138

#define blr_plan		(unsigned char)139	/* access plan items */
#define blr_merge		(unsigned char)140
#define blr_join		(unsigned char)141
#define blr_sequential		(unsigned char)142
#define blr_navigational	(unsigned char)143
#define blr_indices		(unsigned char)144
#define blr_retrieve		(unsigned char)145

#define blr_relation2		(unsigned char)146
#define blr_rid2		(unsigned char)147
//#define blr_reset_stream	(unsigned char)148
//#define blr_release_bookmark	(unsigned char)149

#define blr_set_generator       (unsigned char)150

#define blr_ansi_any		(unsigned char)151   /* required for NULL handling */
#define blr_exists		(unsigned char)152   /* required for NULL handling */
//#define blr_cardinality		(unsigned char)153

#define blr_record_version	(unsigned char)154	/* get tid of record */
#define blr_stall		(unsigned char)155	/* fake server stall */

//#define blr_seek_no_warn	(unsigned char)156
//#define blr_find_dbkey_version	(unsigned char)157   /* find dbkey with record version */
#define blr_ansi_all		(unsigned char)158   /* required for NULL handling */

#define blr_extract		(unsigned char)159

/* sub parameters for blr_extract */

#define blr_extract_year		(unsigned char)0
#define blr_extract_month		(unsigned char)1
#define blr_extract_day			(unsigned char)2
#define blr_extract_hour		(unsigned char)3
#define blr_extract_minute		(unsigned char)4
#define blr_extract_second		(unsigned char)5
#define blr_extract_weekday		(unsigned char)6
#define blr_extract_yearday		(unsigned char)7
#define blr_extract_millisecond	(unsigned char)8
#define blr_extract_week		(unsigned char)9

#define blr_current_date	(unsigned char)160
#define blr_current_timestamp	(unsigned char)161
#define blr_current_time	(unsigned char)162

/* These codes reuse BLR code space */

#define blr_post_arg		(unsigned char)163
#define blr_exec_into		(unsigned char)164
#define blr_user_savepoint	(unsigned char)165
#define blr_dcl_cursor		(unsigned char)166
#define blr_cursor_stmt		(unsigned char)167
#define blr_current_timestamp2	(unsigned char)168
#define blr_current_time2	(unsigned char)169
#define blr_agg_list		(unsigned char)170
#define blr_agg_list_distinct	(unsigned char)171
#define blr_modify2			(unsigned char)172

/* FB 1.0 specific BLR */

#define blr_current_role	(unsigned char)174
#define blr_skip		(unsigned char)175

/* FB 1.5 specific BLR */

#define blr_exec_sql		(unsigned char)176
#define blr_internal_info	(unsigned char)177
#define blr_nullsfirst		(unsigned char)178
#define blr_writelock		(unsigned char)179
#define blr_nullslast       (unsigned char)180

/* FB 2.0 specific BLR */

#define blr_lowcase			(unsigned char)181
#define blr_strlen			(unsigned char)182

/* sub parameter for blr_strlen */
#define blr_strlen_bit		(unsigned char)0
#define blr_strlen_char		(unsigned char)1
#define blr_strlen_octet	(unsigned char)2

#define blr_trim			(unsigned char)183

/* first sub parameter for blr_trim */
#define blr_trim_both		(unsigned char)0
#define blr_trim_leading	(unsigned char)1
#define blr_trim_trailing	(unsigned char)2

/* second sub parameter for blr_trim */
#define blr_trim_spaces		(unsigned char)0
#define blr_trim_characters	(unsigned char)1

/* These codes are actions for user-defined savepoints */

#define blr_savepoint_set	(unsigned char)0
#define blr_savepoint_release	(unsigned char)1
#define blr_savepoint_undo	(unsigned char)2
#define blr_savepoint_release_single	(unsigned char)3

/* These codes are actions for cursors */

#define blr_cursor_open			(unsigned char)0
#define blr_cursor_close		(unsigned char)1
#define blr_cursor_fetch		(unsigned char)2

/* FB 2.1 specific BLR */

#define blr_init_variable	(unsigned char)184
#define blr_recurse			(unsigned char)185
#define blr_sys_function	(unsigned char)186

// FB 2.5 specific BLR

#define blr_auto_trans		(unsigned char)187
#define blr_similar			(unsigned char)188
#define blr_exec_stmt		(unsigned char)189

// subcodes of blr_exec_stmt
#define blr_exec_stmt_inputs		(unsigned char) 1	// input parameters count
#define blr_exec_stmt_outputs		(unsigned char) 2	// output parameters count
#define blr_exec_stmt_sql			(unsigned char) 3
#define blr_exec_stmt_proc_block	(unsigned char) 4
#define blr_exec_stmt_data_src		(unsigned char) 5
#define blr_exec_stmt_user			(unsigned char) 6
#define blr_exec_stmt_pwd			(unsigned char) 7
#define blr_exec_stmt_tran    		(unsigned char) 8	// not implemented yet
#define blr_exec_stmt_tran_clone	(unsigned char) 9	// make transaction parameters equal to current transaction
#define blr_exec_stmt_privs			(unsigned char) 10
#define blr_exec_stmt_in_params		(unsigned char) 11	// not named input parameters
#define blr_exec_stmt_in_params2	(unsigned char) 12	// named input parameters
#define blr_exec_stmt_out_params	(unsigned char) 13	// output parameters
#define blr_exec_stmt_role			(unsigned char) 14

#define blr_stmt_expr				(unsigned char) 190
#define blr_derived_expr			(unsigned char) 191

#endif // JRD_BLR_H

#ifndef INCLUDE_CONSTS_PUB_H
#define INCLUDE_CONSTS_PUB_H

/**********************************/
/* Database parameter block stuff */
/**********************************/

#define isc_dpb_version1                  1
#define isc_dpb_cdd_pathname              1
#define isc_dpb_allocation                2
#define isc_dpb_journal                   3
#define isc_dpb_page_size                 4
#define isc_dpb_num_buffers               5
#define isc_dpb_buffer_length             6
#define isc_dpb_debug                     7
#define isc_dpb_garbage_collect           8
#define isc_dpb_verify                    9
#define isc_dpb_sweep                     10
#define isc_dpb_enable_journal            11
#define isc_dpb_disable_journal           12
#define isc_dpb_dbkey_scope               13
#define isc_dpb_number_of_users           14
#define isc_dpb_trace                     15
#define isc_dpb_no_garbage_collect        16
#define isc_dpb_damaged                   17
#define isc_dpb_license                   18
#define isc_dpb_sys_user_name             19
#define isc_dpb_encrypt_key               20
#define isc_dpb_activate_shadow           21
#define isc_dpb_sweep_interval            22
#define isc_dpb_delete_shadow             23
#define isc_dpb_force_write               24
#define isc_dpb_begin_log                 25
#define isc_dpb_quit_log                  26
#define isc_dpb_no_reserve                27
#define isc_dpb_user_name                 28
#define isc_dpb_password                  29
#define isc_dpb_password_enc              30
#define isc_dpb_sys_user_name_enc         31
#define isc_dpb_interp                    32
#define isc_dpb_online_dump               33
#define isc_dpb_old_file_size             34
#define isc_dpb_old_num_files             35
#define isc_dpb_old_file                  36
#define isc_dpb_old_start_page            37
#define isc_dpb_old_start_seqno           38
#define isc_dpb_old_start_file            39
#define isc_dpb_drop_walfile              40
#define isc_dpb_old_dump_id               41
#define isc_dpb_wal_backup_dir            42
#define isc_dpb_wal_chkptlen              43
#define isc_dpb_wal_numbufs               44
#define isc_dpb_wal_bufsize               45
#define isc_dpb_wal_grp_cmt_wait          46
#define isc_dpb_lc_messages               47
#define isc_dpb_lc_ctype                  48
#define isc_dpb_cache_manager             49
#define isc_dpb_shutdown                  50
#define isc_dpb_online                    51
#define isc_dpb_shutdown_delay            52
#define isc_dpb_reserved                  53
#define isc_dpb_overwrite                 54
#define isc_dpb_sec_attach                55
#define isc_dpb_disable_wal               56
#define isc_dpb_connect_timeout           57
#define isc_dpb_dummy_packet_interval     58
#define isc_dpb_gbak_attach               59
#define isc_dpb_sql_role_name             60
#define isc_dpb_set_page_buffers          61
#define isc_dpb_working_directory         62
#define isc_dpb_sql_dialect               63
#define isc_dpb_set_db_readonly           64
#define isc_dpb_set_db_sql_dialect        65
#define isc_dpb_gfix_attach               66
#define isc_dpb_gstat_attach              67
#define isc_dpb_set_db_charset            68
#define isc_dpb_gsec_attach               69
#define isc_dpb_address_path              70
#define isc_dpb_process_id                71
#define isc_dpb_no_db_triggers            72
#define isc_dpb_trusted_auth			  73
#define isc_dpb_process_name              74
#define isc_dpb_trusted_role			  75
#define isc_dpb_org_filename			  76
#define isc_dpb_utf8_filename			  77
#define isc_dpb_ext_call_depth			  78

/**************************************************/
/* clumplet tags used inside isc_dpb_address_path */
/*						 and isc_spb_address_path */
/**************************************************/

/* Format of this clumplet is the following:

 <address-path-clumplet> ::=
	isc_dpb_address_path <byte-clumplet-length> <address-stack>

 <address-stack> ::=
	<address-descriptor> |
	<address-stack> <address-descriptor>

 <address-descriptor> ::=
	isc_dpb_address <byte-clumplet-length> <address-elements>

 <address-elements> ::=
	<address-element> |
	<address-elements> <address-element>

 <address-element> ::=
	isc_dpb_addr_protocol <byte-clumplet-length> <protocol-string> |
	isc_dpb_addr_endpoint <byte-clumplet-length> <remote-endpoint-string>

 <protocol-string> ::=
	"TCPv4" |
	"TCPv6" |
	"XNET" |
	"WNET" |
	....

 <remote-endpoint-string> ::=
	<IPv4-address> | // such as "172.20.1.1"
	<IPv6-address> | // such as "2001:0:13FF:09FF::1"
	<xnet-process-id> | // such as "17864"
	...
*/

#define isc_dpb_address 1

#define isc_dpb_addr_protocol 1
#define isc_dpb_addr_endpoint 2

/*********************************/
/* isc_dpb_verify specific flags */
/*********************************/

#define isc_dpb_pages                     1
#define isc_dpb_records                   2
#define isc_dpb_indices                   4
#define isc_dpb_transactions              8
#define isc_dpb_no_update                 16
#define isc_dpb_repair                    32
#define isc_dpb_ignore                    64

/***********************************/
/* isc_dpb_shutdown specific flags */
/***********************************/

#define isc_dpb_shut_cache               0x1
#define isc_dpb_shut_attachment          0x2
#define isc_dpb_shut_transaction         0x4
#define isc_dpb_shut_force               0x8
#define isc_dpb_shut_mode_mask          0x70

#define isc_dpb_shut_default             0x0
#define isc_dpb_shut_normal             0x10
#define isc_dpb_shut_multi              0x20
#define isc_dpb_shut_single             0x30
#define isc_dpb_shut_full               0x40

/**************************************/
/* Bit assignments in RDB$SYSTEM_FLAG */
/**************************************/

#define RDB_system                         1
#define RDB_id_assigned                    2
/* 2 is for QLI. See jrd/constants.h for more Firebird-specific values. */


/*************************************/
/* Transaction parameter block stuff */
/*************************************/

#define isc_tpb_version1                  1
#define isc_tpb_version3                  3
#define isc_tpb_consistency               1
#define isc_tpb_concurrency               2
#define isc_tpb_shared                    3
#define isc_tpb_protected                 4
#define isc_tpb_exclusive                 5
#define isc_tpb_wait                      6
#define isc_tpb_nowait                    7
#define isc_tpb_read                      8
#define isc_tpb_write                     9
#define isc_tpb_lock_read                 10
#define isc_tpb_lock_write                11
#define isc_tpb_verb_time                 12
#define isc_tpb_commit_time               13
#define isc_tpb_ignore_limbo              14
#define isc_tpb_read_committed	          15
#define isc_tpb_autocommit                16
#define isc_tpb_rec_version               17
#define isc_tpb_no_rec_version            18
#define isc_tpb_restart_requests          19
#define isc_tpb_no_auto_undo              20
#define isc_tpb_lock_timeout              21


/************************/
/* Blob Parameter Block */
/************************/

#define isc_bpb_version1                  1
#define isc_bpb_source_type               1
#define isc_bpb_target_type               2
#define isc_bpb_type                      3
#define isc_bpb_source_interp             4
#define isc_bpb_target_interp             5
#define isc_bpb_filter_parameter          6
#define isc_bpb_storage                   7

#define isc_bpb_type_segmented            0x0
#define isc_bpb_type_stream               0x1
#define isc_bpb_storage_main              0x0
#define isc_bpb_storage_temp              0x2


/*********************************/
/* Service parameter block stuff */
/*********************************/

#define isc_spb_version1                  1
#define isc_spb_current_version           2
#define isc_spb_version                   isc_spb_current_version
#define isc_spb_user_name                 isc_dpb_user_name
#define isc_spb_sys_user_name             isc_dpb_sys_user_name
#define isc_spb_sys_user_name_enc         isc_dpb_sys_user_name_enc
#define isc_spb_password                  isc_dpb_password
#define isc_spb_password_enc              isc_dpb_password_enc
#define isc_spb_command_line              105
#define isc_spb_dbname                    106
#define isc_spb_verbose                   107
#define isc_spb_options                   108
#define isc_spb_address_path              109
#define isc_spb_process_id                110
#define isc_spb_trusted_auth			  111
#define isc_spb_process_name              112
#define isc_spb_trusted_role              113


#define isc_spb_connect_timeout           isc_dpb_connect_timeout
#define isc_spb_dummy_packet_interval     isc_dpb_dummy_packet_interval
#define isc_spb_sql_role_name             isc_dpb_sql_role_name

/*****************************
 * Service action items      *
 *****************************/

#define isc_action_svc_backup          1	/* Starts database backup process on the server */
#define isc_action_svc_restore         2	/* Starts database restore process on the server */
#define isc_action_svc_repair          3	/* Starts database repair process on the server */
#define isc_action_svc_add_user        4	/* Adds a new user to the security database */
#define isc_action_svc_delete_user     5	/* Deletes a user record from the security database */
#define isc_action_svc_modify_user     6	/* Modifies a user record in the security database */
#define isc_action_svc_display_user    7	/* Displays a user record from the security database */
#define isc_action_svc_properties      8	/* Sets database properties */
#define isc_action_svc_add_license     9	/* Adds a license to the license file */
#define isc_action_svc_remove_license 10	/* Removes a license from the license file */
#define isc_action_svc_db_stats	      11	/* Retrieves database statistics */
#define isc_action_svc_get_ib_log     12	/* Retrieves the InterBase log file from the server */
#define isc_action_svc_get_fb_log     12	/* Retrieves the Firebird log file from the server */
#define isc_action_svc_nbak           20	/* Incremental nbackup */
#define isc_action_svc_nrest          21	/* Incremental database restore */
#define isc_action_svc_trace_start    22	// Start trace session
#define isc_action_svc_trace_stop     23	// Stop trace session
#define isc_action_svc_trace_suspend  24	// Suspend trace session
#define isc_action_svc_trace_resume   25	// Resume trace session
#define isc_action_svc_trace_list     26	// List existing sessions
#define isc_action_svc_set_mapping    27	// Set auto admins mapping in security database
#define isc_action_svc_drop_mapping   28	// Drop auto admins mapping in security database
#define isc_action_svc_display_user_adm 29	// Displays user(s) from security database with admin info
#define isc_action_svc_last			  30	// keep it last !

/*****************************
 * Service information items *
 *****************************/

#define isc_info_svc_svr_db_info      50	/* Retrieves the number of attachments and databases */
#define isc_info_svc_get_license      51	/* Retrieves all license keys and IDs from the license file */
#define isc_info_svc_get_license_mask 52	/* Retrieves a bitmask representing licensed options on the server */
#define isc_info_svc_get_config       53	/* Retrieves the parameters and values for IB_CONFIG */
#define isc_info_svc_version          54	/* Retrieves the version of the services manager */
#define isc_info_svc_server_version   55	/* Retrieves the version of the Firebird server */
#define isc_info_svc_implementation   56	/* Retrieves the implementation of the Firebird server */
#define isc_info_svc_capabilities     57	/* Retrieves a bitmask representing the server's capabilities */
#define isc_info_svc_user_dbpath      58	/* Retrieves the path to the security database in use by the server */
#define isc_info_svc_get_env	      59	/* Retrieves the setting of $FIREBIRD */
#define isc_info_svc_get_env_lock     60	/* Retrieves the setting of $FIREBIRD_LCK */
#define isc_info_svc_get_env_msg      61	/* Retrieves the setting of $FIREBIRD_MSG */
#define isc_info_svc_line             62	/* Retrieves 1 line of service output per call */
#define isc_info_svc_to_eof           63	/* Retrieves as much of the server output as will fit in the supplied buffer */
#define isc_info_svc_timeout          64	/* Sets / signifies a timeout value for reading service information */
#define isc_info_svc_get_licensed_users 65	/* Retrieves the number of users licensed for accessing the server */
#define isc_info_svc_limbo_trans	66	/* Retrieve the limbo transactions */
#define isc_info_svc_running		67	/* Checks to see if a service is running on an attachment */
#define isc_info_svc_get_users		68	/* Returns the user information from isc_action_svc_display_users */
#define isc_info_svc_stdin			78	/* Returns size of data, needed as stdin for service */

/******************************************************
 * Parameters for isc_action_{add|del|mod|disp)_user  *
 ******************************************************/

#define isc_spb_sec_userid            5
#define isc_spb_sec_groupid           6
#define isc_spb_sec_username          7
#define isc_spb_sec_password          8
#define isc_spb_sec_groupname         9
#define isc_spb_sec_firstname         10
#define isc_spb_sec_middlename        11
#define isc_spb_sec_lastname          12
#define isc_spb_sec_admin             13

/*******************************************************
 * Parameters for isc_action_svc_(add|remove)_license, *
 * isc_info_svc_get_license                            *
 *******************************************************/

#define isc_spb_lic_key               5
#define isc_spb_lic_id                6
#define isc_spb_lic_desc              7


/*****************************************
 * Parameters for isc_action_svc_backup  *
 *****************************************/

#define isc_spb_bkp_file                 5
#define isc_spb_bkp_factor               6
#define isc_spb_bkp_length               7
#define isc_spb_bkp_ignore_checksums     0x01
#define isc_spb_bkp_ignore_limbo         0x02
#define isc_spb_bkp_metadata_only        0x04
#define isc_spb_bkp_no_garbage_collect   0x08
#define isc_spb_bkp_old_descriptions     0x10
#define isc_spb_bkp_non_transportable    0x20
#define isc_spb_bkp_convert              0x40
#define isc_spb_bkp_expand				 0x80
#define isc_spb_bkp_no_triggers			 0x8000

/********************************************
 * Parameters for isc_action_svc_properties *
 ********************************************/

#define isc_spb_prp_page_buffers		5
#define isc_spb_prp_sweep_interval		6
#define isc_spb_prp_shutdown_db			7
#define isc_spb_prp_deny_new_attachments	9
#define isc_spb_prp_deny_new_transactions	10
#define isc_spb_prp_reserve_space		11
#define isc_spb_prp_write_mode			12
#define isc_spb_prp_access_mode			13
#define isc_spb_prp_set_sql_dialect		14
#define isc_spb_prp_activate			0x0100
#define isc_spb_prp_db_online			0x0200
#define isc_spb_prp_force_shutdown			41
#define isc_spb_prp_attachments_shutdown	42
#define isc_spb_prp_transactions_shutdown	43
#define isc_spb_prp_shutdown_mode		44
#define isc_spb_prp_online_mode			45

/********************************************
 * Parameters for isc_spb_prp_shutdown_mode *
 *            and isc_spb_prp_online_mode   *
 ********************************************/
#define isc_spb_prp_sm_normal		0
#define isc_spb_prp_sm_multi		1
#define isc_spb_prp_sm_single		2
#define isc_spb_prp_sm_full			3

/********************************************
 * Parameters for isc_spb_prp_reserve_space *
 ********************************************/

#define isc_spb_prp_res_use_full	35
#define isc_spb_prp_res				36

/******************************************
 * Parameters for isc_spb_prp_write_mode  *
 ******************************************/

#define isc_spb_prp_wm_async		37
#define isc_spb_prp_wm_sync			38

/******************************************
 * Parameters for isc_spb_prp_access_mode *
 ******************************************/

#define isc_spb_prp_am_readonly		39
#define isc_spb_prp_am_readwrite	40

/*****************************************
 * Parameters for isc_action_svc_repair  *
 *****************************************/

#define isc_spb_rpr_commit_trans		15
#define isc_spb_rpr_rollback_trans		34
#define isc_spb_rpr_recover_two_phase	17
#define isc_spb_tra_id					18
#define isc_spb_single_tra_id			19
#define isc_spb_multi_tra_id			20
#define isc_spb_tra_state				21
#define isc_spb_tra_state_limbo			22
#define isc_spb_tra_state_commit		23
#define isc_spb_tra_state_rollback		24
#define isc_spb_tra_state_unknown		25
#define isc_spb_tra_host_site			26
#define isc_spb_tra_remote_site			27
#define isc_spb_tra_db_path				28
#define isc_spb_tra_advise				29
#define isc_spb_tra_advise_commit		30
#define isc_spb_tra_advise_rollback		31
#define isc_spb_tra_advise_unknown		33

#define isc_spb_rpr_validate_db			0x01
#define isc_spb_rpr_sweep_db			0x02
#define isc_spb_rpr_mend_db				0x04
#define isc_spb_rpr_list_limbo_trans	0x08
#define isc_spb_rpr_check_db			0x10
#define isc_spb_rpr_ignore_checksum		0x20
#define isc_spb_rpr_kill_shadows		0x40
#define isc_spb_rpr_full				0x80

/*****************************************
 * Parameters for isc_action_svc_restore *
 *****************************************/

#define isc_spb_res_buffers				9
#define isc_spb_res_page_size			10
#define isc_spb_res_length				11
#define isc_spb_res_access_mode			12
#define isc_spb_res_fix_fss_data		13
#define isc_spb_res_fix_fss_metadata	14
#define isc_spb_res_metadata_only		isc_spb_bkp_metadata_only
#define isc_spb_res_deactivate_idx		0x0100
#define isc_spb_res_no_shadow			0x0200
#define isc_spb_res_no_validity			0x0400
#define isc_spb_res_one_at_a_time		0x0800
#define isc_spb_res_replace				0x1000
#define isc_spb_res_create				0x2000
#define isc_spb_res_use_all_space		0x4000

/******************************************
 * Parameters for isc_spb_res_access_mode  *
 ******************************************/

#define isc_spb_res_am_readonly			isc_spb_prp_am_readonly
#define isc_spb_res_am_readwrite		isc_spb_prp_am_readwrite

/*******************************************
 * Parameters for isc_info_svc_svr_db_info *
 *******************************************/

#define isc_spb_num_att			5
#define isc_spb_num_db			6

/*****************************************
 * Parameters for isc_info_svc_db_stats  *
 *****************************************/

#define isc_spb_sts_data_pages		0x01
#define isc_spb_sts_db_log			0x02
#define isc_spb_sts_hdr_pages		0x04
#define isc_spb_sts_idx_pages		0x08
#define isc_spb_sts_sys_relations	0x10
#define isc_spb_sts_record_versions	0x20
#define isc_spb_sts_table			0x40
#define isc_spb_sts_nocreation		0x80

/***********************************/
/* Server configuration key values */
/***********************************/

/* Not available in Firebird 1.5 */

/***************************************
 * Parameters for isc_action_svc_nbak  *
 ***************************************/

#define isc_spb_nbk_level			5
#define isc_spb_nbk_file			6
#define isc_spb_nbk_direct			7
#define isc_spb_nbk_no_triggers		0x01

/***************************************
 * Parameters for isc_action_svc_trace *
 ***************************************/

#define isc_spb_trc_id				1
#define isc_spb_trc_name			2
#define isc_spb_trc_cfg				3

/**********************************************/
/* Dynamic Data Definition Language operators */
/**********************************************/

/******************/
/* Version number */
/******************/

#define isc_dyn_version_1                 1
#define isc_dyn_eoc                       255

/******************************/
/* Operations (may be nested) */
/******************************/

#define isc_dyn_begin                     2
#define isc_dyn_end                       3
#define isc_dyn_if                        4
#define isc_dyn_def_database              5
#define isc_dyn_def_global_fld            6
#define isc_dyn_def_local_fld             7
#define isc_dyn_def_idx                   8
#define isc_dyn_def_rel                   9
#define isc_dyn_def_sql_fld               10
#define isc_dyn_def_view                  12
#define isc_dyn_def_trigger               15
#define isc_dyn_def_security_class        120
#define isc_dyn_def_dimension             140
#define isc_dyn_def_generator             24
#define isc_dyn_def_function              25
#define isc_dyn_def_filter                26
#define isc_dyn_def_function_arg          27
#define isc_dyn_def_shadow                34
#define isc_dyn_def_trigger_msg           17
#define isc_dyn_def_file                  36
#define isc_dyn_mod_database              39
#define isc_dyn_mod_rel                   11
#define isc_dyn_mod_global_fld            13
#define isc_dyn_mod_idx                   102
#define isc_dyn_mod_local_fld             14
#define isc_dyn_mod_sql_fld		  216
#define isc_dyn_mod_view                  16
#define isc_dyn_mod_security_class        122
#define isc_dyn_mod_trigger               113
#define isc_dyn_mod_trigger_msg           28
#define isc_dyn_delete_database           18
#define isc_dyn_delete_rel                19
#define isc_dyn_delete_global_fld         20
#define isc_dyn_delete_local_fld          21
#define isc_dyn_delete_idx                22
#define isc_dyn_delete_security_class     123
#define isc_dyn_delete_dimensions         143
#define isc_dyn_delete_trigger            23
#define isc_dyn_delete_trigger_msg        29
#define isc_dyn_delete_filter             32
#define isc_dyn_delete_function           33
#define isc_dyn_delete_shadow             35
#define isc_dyn_grant                     30
#define isc_dyn_revoke                    31
#define isc_dyn_revoke_all                246
#define isc_dyn_def_primary_key           37
#define isc_dyn_def_foreign_key           38
#define isc_dyn_def_unique                40
#define isc_dyn_def_procedure             164
#define isc_dyn_delete_procedure          165
#define isc_dyn_def_parameter             135
#define isc_dyn_delete_parameter          136
#define isc_dyn_mod_procedure             175
/* Deprecated.
#define isc_dyn_def_log_file              176
#define isc_dyn_def_cache_file            180
*/
#define isc_dyn_def_exception             181
#define isc_dyn_mod_exception             182
#define isc_dyn_del_exception             183
/* Deprecated.
#define isc_dyn_drop_log                  194
#define isc_dyn_drop_cache                195
#define isc_dyn_def_default_log           202
*/
#define isc_dyn_def_difference            220
#define isc_dyn_drop_difference           221
#define isc_dyn_begin_backup              222
#define isc_dyn_end_backup                223
#define isc_dyn_debug_info                240

/***********************/
/* View specific stuff */
/***********************/

#define isc_dyn_view_blr                  43
#define isc_dyn_view_source               44
#define isc_dyn_view_relation             45
#define isc_dyn_view_context              46
#define isc_dyn_view_context_name         47

/**********************/
/* Generic attributes */
/**********************/

#define isc_dyn_rel_name                  50
#define isc_dyn_fld_name                  51
#define isc_dyn_new_fld_name              215
#define isc_dyn_idx_name                  52
#define isc_dyn_description               53
#define isc_dyn_security_class            54
#define isc_dyn_system_flag               55
#define isc_dyn_update_flag               56
#define isc_dyn_prc_name                  166
#define isc_dyn_prm_name                  137
#define isc_dyn_sql_object                196
#define isc_dyn_fld_character_set_name    174

/********************************/
/* Relation specific attributes */
/********************************/

#define isc_dyn_rel_dbkey_length          61
#define isc_dyn_rel_store_trig            62
#define isc_dyn_rel_modify_trig           63
#define isc_dyn_rel_erase_trig            64
#define isc_dyn_rel_store_trig_source     65
#define isc_dyn_rel_modify_trig_source    66
#define isc_dyn_rel_erase_trig_source     67
#define isc_dyn_rel_ext_file              68
#define isc_dyn_rel_sql_protection        69
#define isc_dyn_rel_constraint            162
#define isc_dyn_delete_rel_constraint     163

#define isc_dyn_rel_temporary				238
#define isc_dyn_rel_temp_global_preserve	1
#define isc_dyn_rel_temp_global_delete		2

/************************************/
/* Global field specific attributes */
/************************************/

#define isc_dyn_fld_type                  70
#define isc_dyn_fld_length                71
#define isc_dyn_fld_scale                 72
#define isc_dyn_fld_sub_type              73
#define isc_dyn_fld_segment_length        74
#define isc_dyn_fld_query_header          75
#define isc_dyn_fld_edit_string           76
#define isc_dyn_fld_validation_blr        77
#define isc_dyn_fld_validation_source     78
#define isc_dyn_fld_computed_blr          79
#define isc_dyn_fld_computed_source       80
#define isc_dyn_fld_missing_value         81
#define isc_dyn_fld_default_value         82
#define isc_dyn_fld_query_name            83
#define isc_dyn_fld_dimensions            84
#define isc_dyn_fld_not_null              85
#define isc_dyn_fld_precision             86
#define isc_dyn_fld_char_length           172
#define isc_dyn_fld_collation             173
#define isc_dyn_fld_default_source        193
#define isc_dyn_del_default               197
#define isc_dyn_del_validation            198
#define isc_dyn_single_validation         199
#define isc_dyn_fld_character_set         203
#define isc_dyn_del_computed              242

/***********************************/
/* Local field specific attributes */
/***********************************/

#define isc_dyn_fld_source                90
#define isc_dyn_fld_base_fld              91
#define isc_dyn_fld_position              92
#define isc_dyn_fld_update_flag           93

/*****************************/
/* Index specific attributes */
/*****************************/

#define isc_dyn_idx_unique                100
#define isc_dyn_idx_inactive              101
#define isc_dyn_idx_type                  103
#define isc_dyn_idx_foreign_key           104
#define isc_dyn_idx_ref_column            105
#define isc_dyn_idx_statistic             204

/*******************************/
/* Trigger specific attributes */
/*******************************/

#define isc_dyn_trg_type                  110
#define isc_dyn_trg_blr                   111
#define isc_dyn_trg_source                112
#define isc_dyn_trg_name                  114
#define isc_dyn_trg_sequence              115
#define isc_dyn_trg_inactive              116
#define isc_dyn_trg_msg_number            117
#define isc_dyn_trg_msg                   118

/**************************************/
/* Security Class specific attributes */
/**************************************/

#define isc_dyn_scl_acl                   121
#define isc_dyn_grant_user                130
#define isc_dyn_grant_user_explicit       219
#define isc_dyn_grant_proc                186
#define isc_dyn_grant_trig                187
#define isc_dyn_grant_view                188
#define isc_dyn_grant_options             132
#define isc_dyn_grant_user_group          205
#define isc_dyn_grant_role                218
#define isc_dyn_grant_grantor			  245


/**********************************/
/* Dimension specific information */
/**********************************/

#define isc_dyn_dim_lower                 141
#define isc_dyn_dim_upper                 142

/****************************/
/* File specific attributes */
/****************************/

#define isc_dyn_file_name                 125
#define isc_dyn_file_start                126
#define isc_dyn_file_length               127
#define isc_dyn_shadow_number             128
#define isc_dyn_shadow_man_auto           129
#define isc_dyn_shadow_conditional        130

/********************************/
/* Log file specific attributes */
/********************************/
/* Deprecated.
#define isc_dyn_log_file_sequence         177
#define isc_dyn_log_file_partitions       178
#define isc_dyn_log_file_serial           179
#define isc_dyn_log_file_overflow         200
#define isc_dyn_log_file_raw              201
*/

/***************************/
/* Log specific attributes */
/***************************/
/* Deprecated.
#define isc_dyn_log_group_commit_wait     189
#define isc_dyn_log_buffer_size           190
#define isc_dyn_log_check_point_length    191
#define isc_dyn_log_num_of_buffers        192
*/

/********************************/
/* Function specific attributes */
/********************************/

#define isc_dyn_function_name             145
#define isc_dyn_function_type             146
#define isc_dyn_func_module_name          147
#define isc_dyn_func_entry_point          148
#define isc_dyn_func_return_argument      149
#define isc_dyn_func_arg_position         150
#define isc_dyn_func_mechanism            151
#define isc_dyn_filter_in_subtype         152
#define isc_dyn_filter_out_subtype        153


#define isc_dyn_description2              154
#define isc_dyn_fld_computed_source2      155
#define isc_dyn_fld_edit_string2          156
#define isc_dyn_fld_query_header2         157
#define isc_dyn_fld_validation_source2    158
#define isc_dyn_trg_msg2                  159
#define isc_dyn_trg_source2               160
#define isc_dyn_view_source2              161
#define isc_dyn_xcp_msg2                  184

/*********************************/
/* Generator specific attributes */
/*********************************/

#define isc_dyn_generator_name            95
#define isc_dyn_generator_id              96

/*********************************/
/* Procedure specific attributes */
/*********************************/

#define isc_dyn_prc_inputs                167
#define isc_dyn_prc_outputs               168
#define isc_dyn_prc_source                169
#define isc_dyn_prc_blr                   170
#define isc_dyn_prc_source2               171
#define isc_dyn_prc_type                  239

#define isc_dyn_prc_t_selectable          1
#define isc_dyn_prc_t_executable          2

/*********************************/
/* Parameter specific attributes */
/*********************************/

#define isc_dyn_prm_number                138
#define isc_dyn_prm_type                  139
#define isc_dyn_prm_mechanism             241

/********************************/
/* Relation specific attributes */
/********************************/

#define isc_dyn_xcp_msg                   185

/**********************************************/
/* Cascading referential integrity values     */
/**********************************************/
#define isc_dyn_foreign_key_update        205
#define isc_dyn_foreign_key_delete        206
#define isc_dyn_foreign_key_cascade       207
#define isc_dyn_foreign_key_default       208
#define isc_dyn_foreign_key_null          209
#define isc_dyn_foreign_key_none          210

/***********************/
/* SQL role values     */
/***********************/
#define isc_dyn_def_sql_role              211
#define isc_dyn_sql_role_name             212
#define isc_dyn_grant_admin_options       213
#define isc_dyn_del_sql_role              214
/* 215 & 216 are used some lines above. */

/**********************************************/
/* Generators again                           */
/**********************************************/

#define isc_dyn_delete_generator          217

// New for comments in objects.
#define isc_dyn_mod_function              224
#define isc_dyn_mod_filter                225
#define isc_dyn_mod_generator             226
#define isc_dyn_mod_sql_role              227
#define isc_dyn_mod_charset               228
#define isc_dyn_mod_collation             229
#define isc_dyn_mod_prc_parameter         230

/***********************/
/* collation values    */
/***********************/
#define isc_dyn_def_collation						231
#define isc_dyn_coll_for_charset					232
#define isc_dyn_coll_from							233
#define isc_dyn_coll_from_external					239
#define isc_dyn_coll_attribute						234
#define isc_dyn_coll_specific_attributes_charset	235
#define isc_dyn_coll_specific_attributes			236
#define isc_dyn_del_collation						237

/******************************************/
/* Mapping OS security objects to DB ones */
/******************************************/
#define isc_dyn_mapping								243
#define isc_dyn_map_role							1
#define isc_dyn_unmap_role							2
#define isc_dyn_map_user							3
#define isc_dyn_unmap_user							4
#define isc_dyn_automap_role						5
#define isc_dyn_autounmap_role						6

/********************/
/* Users control    */
/********************/
#define isc_dyn_user								244
#define isc_dyn_user_add							1
#define isc_dyn_user_mod							2
#define isc_dyn_user_del							3
#define isc_dyn_user_passwd							4
#define isc_dyn_user_first							5
#define isc_dyn_user_middle							6
#define isc_dyn_user_last							7
#define isc_dyn_user_admin							8
#define isc_user_end								0

/****************************/
/* Last $dyn value assigned */
/****************************/
#define isc_dyn_last_dyn_value            247

/******************************************/
/* Array slice description language (SDL) */
/******************************************/

#define isc_sdl_version1                  1
#define isc_sdl_eoc                       255
#define isc_sdl_relation                  2
#define isc_sdl_rid                       3
#define isc_sdl_field                     4
#define isc_sdl_fid                       5
#define isc_sdl_struct                    6
#define isc_sdl_variable                  7
#define isc_sdl_scalar                    8
#define isc_sdl_tiny_integer              9
#define isc_sdl_short_integer             10
#define isc_sdl_long_integer              11
#define isc_sdl_literal                   12
#define isc_sdl_add                       13
#define isc_sdl_subtract                  14
#define isc_sdl_multiply                  15
#define isc_sdl_divide                    16
#define isc_sdl_negate                    17
#define isc_sdl_eql                       18
#define isc_sdl_neq                       19
#define isc_sdl_gtr                       20
#define isc_sdl_geq                       21
#define isc_sdl_lss                       22
#define isc_sdl_leq                       23
#define isc_sdl_and                       24
#define isc_sdl_or                        25
#define isc_sdl_not                       26
#define isc_sdl_while                     27
#define isc_sdl_assignment                28
#define isc_sdl_label                     29
#define isc_sdl_leave                     30
#define isc_sdl_begin                     31
#define isc_sdl_end                       32
#define isc_sdl_do3                       33
#define isc_sdl_do2                       34
#define isc_sdl_do1                       35
#define isc_sdl_element                   36

/********************************************/
/* International text interpretation values */
/********************************************/

#define isc_interp_eng_ascii              0
#define isc_interp_jpn_sjis               5
#define isc_interp_jpn_euc                6

/*****************/
/* Blob Subtypes */
/*****************/

/* types less than zero are reserved for customer use */

#define isc_blob_untyped                  0

/* internal subtypes */

#define isc_blob_text                     1
#define isc_blob_blr                      2
#define isc_blob_acl                      3
#define isc_blob_ranges                   4
#define isc_blob_summary                  5
#define isc_blob_format                   6
#define isc_blob_tra                      7
#define isc_blob_extfile                  8
#define isc_blob_debug_info               9
#define isc_blob_max_predefined_subtype   10

/* the range 20-30 is reserved for dBASE and Paradox types */

#define isc_blob_formatted_memo           20
#define isc_blob_paradox_ole              21
#define isc_blob_graphic                  22
#define isc_blob_dbase_ole                23
#define isc_blob_typed_binary             24

/* Deprecated definitions maintained for compatibility only */

#define isc_info_db_SQL_dialect           62
#define isc_dpb_SQL_dialect               63
#define isc_dpb_set_db_SQL_dialect        65

/***********************************/
/* Masks for fb_shutdown_callback  */
/***********************************/

#define fb_shut_confirmation			  1
#define fb_shut_preproviders			  2
#define fb_shut_postproviders			  4
#define fb_shut_finish					  8

/****************************************/
/* Shutdown reasons, used by engine     */
/* Users should provide positive values */
/****************************************/

#define fb_shutrsn_svc_stopped			  -1
#define fb_shutrsn_no_connection		  -2
#define fb_shutrsn_app_stopped			  -3
#define fb_shutrsn_device_removed		  -4
#define fb_shutrsn_signal				  -5
#define fb_shutrsn_services				  -6
#define fb_shutrsn_exit_called			  -7

/****************************************/
/* Cancel types for fb_cancel_operation */
/****************************************/

#define fb_cancel_disable				  1
#define fb_cancel_enable				  2
#define fb_cancel_raise					  3
#define fb_cancel_abort					  4

/********************************************/
/* Debug information items					*/
/********************************************/

#define fb_dbg_version				1
#define fb_dbg_end					255
#define fb_dbg_map_src2blr			2
#define fb_dbg_map_varname			3
#define fb_dbg_map_argument			4

// sub code for fb_dbg_map_argument
#define fb_dbg_arg_input			0
#define fb_dbg_arg_output			1

#endif // ifndef INCLUDE_CONSTS_PUB_H

/*********************************/
/* Information call declarations */
/*********************************/

#ifndef JRD_INF_PUB_H
#define JRD_INF_PUB_H

/* Common, structural codes */
/****************************/

#define isc_info_end			1
#define isc_info_truncated		2
#define isc_info_error			3
#define isc_info_data_not_ready	          4
#define isc_info_length			126
#define isc_info_flag_end		127

/******************************/
/* Database information items */
/******************************/

enum db_info_types
{
	isc_info_db_id			= 4,
	isc_info_reads			= 5,
	isc_info_writes		    = 6,
	isc_info_fetches		= 7,
	isc_info_marks			= 8,

	isc_info_implementation = 11,
	isc_info_isc_version		= 12,
	isc_info_base_level		= 13,
	isc_info_page_size		= 14,
	isc_info_num_buffers	= 15,
	isc_info_limbo			= 16,
	isc_info_current_memory	= 17,
	isc_info_max_memory		= 18,
	isc_info_window_turns	= 19,
	isc_info_license		= 20,

	isc_info_allocation		= 21,
	isc_info_attachment_id	 = 22,
	isc_info_read_seq_count	= 23,
	isc_info_read_idx_count	= 24,
	isc_info_insert_count		= 25,
	isc_info_update_count		= 26,
	isc_info_delete_count		= 27,
	isc_info_backout_count	 	= 28,
	isc_info_purge_count		= 29,
	isc_info_expunge_count		= 30,

	isc_info_sweep_interval	= 31,
	isc_info_ods_version		= 32,
	isc_info_ods_minor_version	= 33,
	isc_info_no_reserve		= 34,
	/* Begin deprecated WAL and JOURNAL items. */
	isc_info_logfile		= 35,
	isc_info_cur_logfile_name	= 36,
	isc_info_cur_log_part_offset	= 37,
	isc_info_num_wal_buffers	= 38,
	isc_info_wal_buffer_size	= 39,
	isc_info_wal_ckpt_length	= 40,

	isc_info_wal_cur_ckpt_interval = 41,
	isc_info_wal_prv_ckpt_fname	= 42,
	isc_info_wal_prv_ckpt_poffset	= 43,
	isc_info_wal_recv_ckpt_fname	= 44,
	isc_info_wal_recv_ckpt_poffset = 45,
	isc_info_wal_grpc_wait_usecs	= 47,
	isc_info_wal_num_io		= 48,
	isc_info_wal_avg_io_size	= 49,
	isc_info_wal_num_commits	= 50,
	isc_info_wal_avg_grpc_size	= 51,
	/* End deprecated WAL and JOURNAL items. */

	isc_info_forced_writes		= 52,
	isc_info_user_names = 53,
	isc_info_page_errors = 54,
	isc_info_record_errors = 55,
	isc_info_bpage_errors = 56,
	isc_info_dpage_errors = 57,
	isc_info_ipage_errors = 58,
	isc_info_ppage_errors = 59,
	isc_info_tpage_errors = 60,

	isc_info_set_page_buffers = 61,
	isc_info_db_sql_dialect = 62,
	isc_info_db_read_only = 63,
	isc_info_db_size_in_pages = 64,

	/* Values 65 -100 unused to avoid conflict with InterBase */

	frb_info_att_charset = 101,
	isc_info_db_class = 102,
	isc_info_firebird_version = 103,
	isc_info_oldest_transaction = 104,
	isc_info_oldest_active = 105,
	isc_info_oldest_snapshot = 106,
	isc_info_next_transaction = 107,
	isc_info_db_provider = 108,
	isc_info_active_transactions = 109,
	isc_info_active_tran_count = 110,
	isc_info_creation_date = 111,
	isc_info_db_file_size = 112,
	fb_info_page_contents = 113,

	isc_info_db_last_value   /* Leave this LAST! */
};

#define isc_info_version isc_info_isc_version


/**************************************/
/* Database information return values */
/**************************************/

enum  info_db_implementations
{
	isc_info_db_impl_rdb_vms = 1,
	isc_info_db_impl_rdb_eln = 2,
	isc_info_db_impl_rdb_eln_dev = 3,
	isc_info_db_impl_rdb_vms_y = 4,
	isc_info_db_impl_rdb_eln_y = 5,
	isc_info_db_impl_jri = 6,
	isc_info_db_impl_jsv = 7,

	isc_info_db_impl_isc_apl_68K = 25,
	isc_info_db_impl_isc_vax_ultr = 26,
	isc_info_db_impl_isc_vms = 27,
	isc_info_db_impl_isc_sun_68k = 28,
	isc_info_db_impl_isc_os2 = 29,
	isc_info_db_impl_isc_sun4 = 30,	   /* 30 */

	isc_info_db_impl_isc_hp_ux = 31,
	isc_info_db_impl_isc_sun_386i = 32,
	isc_info_db_impl_isc_vms_orcl = 33,
	isc_info_db_impl_isc_mac_aux = 34,
	isc_info_db_impl_isc_rt_aix = 35,
	isc_info_db_impl_isc_mips_ult = 36,
	isc_info_db_impl_isc_xenix = 37,
	isc_info_db_impl_isc_dg = 38,
	isc_info_db_impl_isc_hp_mpexl = 39,
	isc_info_db_impl_isc_hp_ux68K = 40,	  /* 40 */

	isc_info_db_impl_isc_sgi = 41,
	isc_info_db_impl_isc_sco_unix = 42,
	isc_info_db_impl_isc_cray = 43,
	isc_info_db_impl_isc_imp = 44,
	isc_info_db_impl_isc_delta = 45,
	isc_info_db_impl_isc_next = 46,
	isc_info_db_impl_isc_dos = 47,
	isc_info_db_impl_m88K = 48,
	isc_info_db_impl_unixware = 49,
	isc_info_db_impl_isc_winnt_x86 = 50,

	isc_info_db_impl_isc_epson = 51,
	isc_info_db_impl_alpha_osf = 52,
	isc_info_db_impl_alpha_vms = 53,
	isc_info_db_impl_netware_386 = 54,
	isc_info_db_impl_win_only = 55,
	isc_info_db_impl_ncr_3000 = 56,
	isc_info_db_impl_winnt_ppc = 57,
	isc_info_db_impl_dg_x86 = 58,
	isc_info_db_impl_sco_ev = 59,
	isc_info_db_impl_i386 = 60,

	isc_info_db_impl_freebsd = 61,
	isc_info_db_impl_netbsd = 62,
	isc_info_db_impl_darwin_ppc = 63,
	isc_info_db_impl_sinixz = 64,

	isc_info_db_impl_linux_sparc = 65,
	isc_info_db_impl_linux_amd64 = 66,

	isc_info_db_impl_freebsd_amd64 = 67,

	isc_info_db_impl_winnt_amd64 = 68,

	isc_info_db_impl_linux_ppc = 69,
	isc_info_db_impl_darwin_x86 = 70,
	isc_info_db_impl_linux_mipsel = 71,
	isc_info_db_impl_linux_mips = 72,
	isc_info_db_impl_darwin_x64 = 73,
	isc_info_db_impl_sun_amd64 = 74,

	isc_info_db_impl_linux_arm = 75,
	isc_info_db_impl_linux_ia64 = 76,

	isc_info_db_impl_darwin_ppc64 = 77,
	isc_info_db_impl_linux_s390x = 78,
	isc_info_db_impl_linux_s390 = 79,

	isc_info_db_impl_linux_sh = 80,
	isc_info_db_impl_linux_sheb = 81,
	isc_info_db_impl_linux_hppa = 82,
	isc_info_db_impl_linux_alpha = 83,

	isc_info_db_impl_last_value   // Leave this LAST!
};


enum info_db_class
{
	isc_info_db_class_access = 1,
	isc_info_db_class_y_valve = 2,
	isc_info_db_class_rem_int = 3,
	isc_info_db_class_rem_srvr = 4,
	isc_info_db_class_pipe_int = 7,
	isc_info_db_class_pipe_srvr = 8,
	isc_info_db_class_sam_int = 9,
	isc_info_db_class_sam_srvr = 10,
	isc_info_db_class_gateway = 11,
	isc_info_db_class_cache = 12,
	isc_info_db_class_classic_access = 13,
	isc_info_db_class_server_access = 14,

	isc_info_db_class_last_value   /* Leave this LAST! */
};

enum info_db_provider
{
	isc_info_db_code_rdb_eln = 1,
	isc_info_db_code_rdb_vms = 2,
	isc_info_db_code_interbase = 3,
	isc_info_db_code_firebird = 4,

	isc_info_db_code_last_value   /* Leave this LAST! */
};


/*****************************/
/* Request information items */
/*****************************/

#define isc_info_number_messages	4
#define isc_info_max_message		5
#define isc_info_max_send		6
#define isc_info_max_receive		7
#define isc_info_state			8
#define isc_info_message_number	9
#define isc_info_message_size		10
#define isc_info_request_cost		11
#define isc_info_access_path		12
#define isc_info_req_select_count	13
#define isc_info_req_insert_count	14
#define isc_info_req_update_count	15
#define isc_info_req_delete_count	16


/*********************/
/* Access path items */
/*********************/

#define isc_info_rsb_end		0
#define isc_info_rsb_begin		1
#define isc_info_rsb_type		2
#define isc_info_rsb_relation		3
#define isc_info_rsb_plan			4

/*************/
/* RecordSource (RSB) types */
/*************/

#define isc_info_rsb_unknown		1
#define isc_info_rsb_indexed		2
#define isc_info_rsb_navigate		3
#define isc_info_rsb_sequential	4
#define isc_info_rsb_cross		5
#define isc_info_rsb_sort		6
#define isc_info_rsb_first		7
#define isc_info_rsb_boolean		8
#define isc_info_rsb_union		9
#define isc_info_rsb_aggregate		10
#define isc_info_rsb_merge		11
#define isc_info_rsb_ext_sequential	12
#define isc_info_rsb_ext_indexed	13
#define isc_info_rsb_ext_dbkey		14
#define isc_info_rsb_left_cross	15
#define isc_info_rsb_select		16
#define isc_info_rsb_sql_join		17
#define isc_info_rsb_simulate		18
#define isc_info_rsb_sim_cross		19
#define isc_info_rsb_once		20
#define isc_info_rsb_procedure		21
#define isc_info_rsb_skip		22
#define isc_info_rsb_virt_sequential	23
#define isc_info_rsb_recursive	24

/**********************/
/* Bitmap expressions */
/**********************/

#define isc_info_rsb_and		1
#define isc_info_rsb_or		2
#define isc_info_rsb_dbkey		3
#define isc_info_rsb_index		4

#define isc_info_req_active		2
#define isc_info_req_inactive		3
#define isc_info_req_send		4
#define isc_info_req_receive		5
#define isc_info_req_select		6
#define isc_info_req_sql_stall		7

/**************************/
/* Blob information items */
/**************************/

#define isc_info_blob_num_segments	4
#define isc_info_blob_max_segment	5
#define isc_info_blob_total_length	6
#define isc_info_blob_type		7

/*********************************/
/* Transaction information items */
/*********************************/

#define isc_info_tra_id						4
#define isc_info_tra_oldest_interesting		5
#define isc_info_tra_oldest_snapshot		6
#define isc_info_tra_oldest_active			7
#define isc_info_tra_isolation				8
#define isc_info_tra_access					9
#define isc_info_tra_lock_timeout			10

// isc_info_tra_isolation responses
#define isc_info_tra_consistency		1
#define isc_info_tra_concurrency		2
#define isc_info_tra_read_committed		3

// isc_info_tra_read_committed options
#define isc_info_tra_no_rec_version		0
#define isc_info_tra_rec_version		1

// isc_info_tra_access responses
#define isc_info_tra_readonly	0
#define isc_info_tra_readwrite	1


/*************************/
/* SQL information items */
/*************************/

#define isc_info_sql_select		4
#define isc_info_sql_bind		5
#define isc_info_sql_num_variables	6
#define isc_info_sql_describe_vars	7
#define isc_info_sql_describe_end	8
#define isc_info_sql_sqlda_seq		9
#define isc_info_sql_message_seq	10
#define isc_info_sql_type		11
#define isc_info_sql_sub_type		12
#define isc_info_sql_scale		13
#define isc_info_sql_length		14
#define isc_info_sql_null_ind		15
#define isc_info_sql_field		16
#define isc_info_sql_relation		17
#define isc_info_sql_owner		18
#define isc_info_sql_alias		19
#define isc_info_sql_sqlda_start	20
#define isc_info_sql_stmt_type		21
#define isc_info_sql_get_plan             22
#define isc_info_sql_records		  23
#define isc_info_sql_batch_fetch	  24
#define isc_info_sql_relation_alias		25

/*********************************/
/* SQL information return values */
/*********************************/

#define isc_info_sql_stmt_select          1
#define isc_info_sql_stmt_insert          2
#define isc_info_sql_stmt_update          3
#define isc_info_sql_stmt_delete          4
#define isc_info_sql_stmt_ddl             5
#define isc_info_sql_stmt_get_segment     6
#define isc_info_sql_stmt_put_segment     7
#define isc_info_sql_stmt_exec_procedure  8
#define isc_info_sql_stmt_start_trans     9
#define isc_info_sql_stmt_commit          10
#define isc_info_sql_stmt_rollback        11
#define isc_info_sql_stmt_select_for_upd  12
#define isc_info_sql_stmt_set_generator   13
#define isc_info_sql_stmt_savepoint       14

#endif /* JRD_INF_PUB_H */


#ifndef JRD_GEN_IBERROR_H
#define JRD_GEN_IBERROR_H
/*
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
 * The content of this file was generated by the Firebird project
 * using the program jrd/codes.epp
 */
/*
 *
 * *** WARNING *** - This file is automatically generated by codes.epp - do not edit!
 *
 */
/*
 *	MODULE:		iberror.h
 *	DESCRIPTION:	ISC error codes
 *
 */



/***********************/
/*   ISC Error Codes   */
/***********************/


#ifdef __cplusplus /* c++ definitions */

const ISC_STATUS isc_facility	= 20;
const ISC_STATUS isc_base = 335544320L;
const ISC_STATUS isc_factor = 1;
const ISC_STATUS isc_arg_end			= 0;	// end of argument list
const ISC_STATUS isc_arg_gds			= 1;	// generic DSRI status value
const ISC_STATUS isc_arg_string		= 2;	// string argument
const ISC_STATUS isc_arg_cstring		= 3;	// count & string argument
const ISC_STATUS isc_arg_number		= 4;	// numeric argument (long)
const ISC_STATUS isc_arg_interpreted	= 5;	// interpreted status code (string)
const ISC_STATUS isc_arg_vms			= 6;	// VAX/VMS status code (long)
const ISC_STATUS isc_arg_unix			= 7;	// UNIX error code
const ISC_STATUS isc_arg_domain		= 8;	// Apollo/Domain error code
const ISC_STATUS isc_arg_dos			= 9;	// MSDOS/OS2 error code
const ISC_STATUS isc_arg_mpexl			= 10;	// HP MPE/XL error code
const ISC_STATUS isc_arg_mpexl_ipc		= 11;	// HP MPE/XL IPC error code
const ISC_STATUS isc_arg_next_mach		= 15;	// NeXT/Mach error code
const ISC_STATUS isc_arg_netware		= 16;	// NetWare error code
const ISC_STATUS isc_arg_win32			= 17;	// Win32 error code
const ISC_STATUS isc_arg_warning		= 18;	// warning argument
const ISC_STATUS isc_arg_sql_state		= 19;	// SQLSTATE

const ISC_STATUS isc_arith_except                     = 335544321L;
const ISC_STATUS isc_bad_dbkey                        = 335544322L;
const ISC_STATUS isc_bad_db_format                    = 335544323L;
const ISC_STATUS isc_bad_db_handle                    = 335544324L;
const ISC_STATUS isc_bad_dpb_content                  = 335544325L;
const ISC_STATUS isc_bad_dpb_form                     = 335544326L;
const ISC_STATUS isc_bad_req_handle                   = 335544327L;
const ISC_STATUS isc_bad_segstr_handle                = 335544328L;
const ISC_STATUS isc_bad_segstr_id                    = 335544329L;
const ISC_STATUS isc_bad_tpb_content                  = 335544330L;
const ISC_STATUS isc_bad_tpb_form                     = 335544331L;
const ISC_STATUS isc_bad_trans_handle                 = 335544332L;
const ISC_STATUS isc_bug_check                        = 335544333L;
const ISC_STATUS isc_convert_error                    = 335544334L;
const ISC_STATUS isc_db_corrupt                       = 335544335L;
const ISC_STATUS isc_deadlock                         = 335544336L;
const ISC_STATUS isc_excess_trans                     = 335544337L;
const ISC_STATUS isc_from_no_match                    = 335544338L;
const ISC_STATUS isc_infinap                          = 335544339L;
const ISC_STATUS isc_infona                           = 335544340L;
const ISC_STATUS isc_infunk                           = 335544341L;
const ISC_STATUS isc_integ_fail                       = 335544342L;
const ISC_STATUS isc_invalid_blr                      = 335544343L;
const ISC_STATUS isc_io_error                         = 335544344L;
const ISC_STATUS isc_lock_conflict                    = 335544345L;
const ISC_STATUS isc_metadata_corrupt                 = 335544346L;
const ISC_STATUS isc_not_valid                        = 335544347L;
const ISC_STATUS isc_no_cur_rec                       = 335544348L;
const ISC_STATUS isc_no_dup                           = 335544349L;
const ISC_STATUS isc_no_finish                        = 335544350L;
const ISC_STATUS isc_no_meta_update                   = 335544351L;
const ISC_STATUS isc_no_priv                          = 335544352L;
const ISC_STATUS isc_no_recon                         = 335544353L;
const ISC_STATUS isc_no_record                        = 335544354L;
const ISC_STATUS isc_no_segstr_close                  = 335544355L;
const ISC_STATUS isc_obsolete_metadata                = 335544356L;
const ISC_STATUS isc_open_trans                       = 335544357L;
const ISC_STATUS isc_port_len                         = 335544358L;
const ISC_STATUS isc_read_only_field                  = 335544359L;
const ISC_STATUS isc_read_only_rel                    = 335544360L;
const ISC_STATUS isc_read_only_trans                  = 335544361L;
const ISC_STATUS isc_read_only_view                   = 335544362L;
const ISC_STATUS isc_req_no_trans                     = 335544363L;
const ISC_STATUS isc_req_sync                         = 335544364L;
const ISC_STATUS isc_req_wrong_db                     = 335544365L;
const ISC_STATUS isc_segment                          = 335544366L;
const ISC_STATUS isc_segstr_eof                       = 335544367L;
const ISC_STATUS isc_segstr_no_op                     = 335544368L;
const ISC_STATUS isc_segstr_no_read                   = 335544369L;
const ISC_STATUS isc_segstr_no_trans                  = 335544370L;
const ISC_STATUS isc_segstr_no_write                  = 335544371L;
const ISC_STATUS isc_segstr_wrong_db                  = 335544372L;
const ISC_STATUS isc_sys_request                      = 335544373L;
const ISC_STATUS isc_stream_eof                       = 335544374L;
const ISC_STATUS isc_unavailable                      = 335544375L;
const ISC_STATUS isc_unres_rel                        = 335544376L;
const ISC_STATUS isc_uns_ext                          = 335544377L;
const ISC_STATUS isc_wish_list                        = 335544378L;
const ISC_STATUS isc_wrong_ods                        = 335544379L;
const ISC_STATUS isc_wronumarg                        = 335544380L;
const ISC_STATUS isc_imp_exc                          = 335544381L;
const ISC_STATUS isc_random                           = 335544382L;
const ISC_STATUS isc_fatal_conflict                   = 335544383L;
const ISC_STATUS isc_badblk                           = 335544384L;
const ISC_STATUS isc_invpoolcl                        = 335544385L;
const ISC_STATUS isc_nopoolids                        = 335544386L;
const ISC_STATUS isc_relbadblk                        = 335544387L;
const ISC_STATUS isc_blktoobig                        = 335544388L;
const ISC_STATUS isc_bufexh                           = 335544389L;
const ISC_STATUS isc_syntaxerr                        = 335544390L;
const ISC_STATUS isc_bufinuse                         = 335544391L;
const ISC_STATUS isc_bdbincon                         = 335544392L;
const ISC_STATUS isc_reqinuse                         = 335544393L;
const ISC_STATUS isc_badodsver                        = 335544394L;
const ISC_STATUS isc_relnotdef                        = 335544395L;
const ISC_STATUS isc_fldnotdef                        = 335544396L;
const ISC_STATUS isc_dirtypage                        = 335544397L;
const ISC_STATUS isc_waifortra                        = 335544398L;
const ISC_STATUS isc_doubleloc                        = 335544399L;
const ISC_STATUS isc_nodnotfnd                        = 335544400L;
const ISC_STATUS isc_dupnodfnd                        = 335544401L;
const ISC_STATUS isc_locnotmar                        = 335544402L;
const ISC_STATUS isc_badpagtyp                        = 335544403L;
const ISC_STATUS isc_corrupt                          = 335544404L;
const ISC_STATUS isc_badpage                          = 335544405L;
const ISC_STATUS isc_badindex                         = 335544406L;
const ISC_STATUS isc_dbbnotzer                        = 335544407L;
const ISC_STATUS isc_tranotzer                        = 335544408L;
const ISC_STATUS isc_trareqmis                        = 335544409L;
const ISC_STATUS isc_badhndcnt                        = 335544410L;
const ISC_STATUS isc_wrotpbver                        = 335544411L;
const ISC_STATUS isc_wroblrver                        = 335544412L;
const ISC_STATUS isc_wrodpbver                        = 335544413L;
const ISC_STATUS isc_blobnotsup                       = 335544414L;
const ISC_STATUS isc_badrelation                      = 335544415L;
const ISC_STATUS isc_nodetach                         = 335544416L;
const ISC_STATUS isc_notremote                        = 335544417L;
const ISC_STATUS isc_trainlim                         = 335544418L;
const ISC_STATUS isc_notinlim                         = 335544419L;
const ISC_STATUS isc_traoutsta                        = 335544420L;
const ISC_STATUS isc_connect_reject                   = 335544421L;
const ISC_STATUS isc_dbfile                           = 335544422L;
const ISC_STATUS isc_orphan                           = 335544423L;
const ISC_STATUS isc_no_lock_mgr                      = 335544424L;
const ISC_STATUS isc_ctxinuse                         = 335544425L;
const ISC_STATUS isc_ctxnotdef                        = 335544426L;
const ISC_STATUS isc_datnotsup                        = 335544427L;
const ISC_STATUS isc_badmsgnum                        = 335544428L;
const ISC_STATUS isc_badparnum                        = 335544429L;
const ISC_STATUS isc_virmemexh                        = 335544430L;
const ISC_STATUS isc_blocking_signal                  = 335544431L;
const ISC_STATUS isc_lockmanerr                       = 335544432L;
const ISC_STATUS isc_journerr                         = 335544433L;
const ISC_STATUS isc_keytoobig                        = 335544434L;
const ISC_STATUS isc_nullsegkey                       = 335544435L;
const ISC_STATUS isc_sqlerr                           = 335544436L;
const ISC_STATUS isc_wrodynver                        = 335544437L;
const ISC_STATUS isc_funnotdef                        = 335544438L;
const ISC_STATUS isc_funmismat                        = 335544439L;
const ISC_STATUS isc_bad_msg_vec                      = 335544440L;
const ISC_STATUS isc_bad_detach                       = 335544441L;
const ISC_STATUS isc_noargacc_read                    = 335544442L;
const ISC_STATUS isc_noargacc_write                   = 335544443L;
const ISC_STATUS isc_read_only                        = 335544444L;
const ISC_STATUS isc_ext_err                          = 335544445L;
const ISC_STATUS isc_non_updatable                    = 335544446L;
const ISC_STATUS isc_no_rollback                      = 335544447L;
const ISC_STATUS isc_bad_sec_info                     = 335544448L;
const ISC_STATUS isc_invalid_sec_info                 = 335544449L;
const ISC_STATUS isc_misc_interpreted                 = 335544450L;
const ISC_STATUS isc_update_conflict                  = 335544451L;
const ISC_STATUS isc_unlicensed                       = 335544452L;
const ISC_STATUS isc_obj_in_use                       = 335544453L;
const ISC_STATUS isc_nofilter                         = 335544454L;
const ISC_STATUS isc_shadow_accessed                  = 335544455L;
const ISC_STATUS isc_invalid_sdl                      = 335544456L;
const ISC_STATUS isc_out_of_bounds                    = 335544457L;
const ISC_STATUS isc_invalid_dimension                = 335544458L;
const ISC_STATUS isc_rec_in_limbo                     = 335544459L;
const ISC_STATUS isc_shadow_missing                   = 335544460L;
const ISC_STATUS isc_cant_validate                    = 335544461L;
const ISC_STATUS isc_cant_start_journal               = 335544462L;
const ISC_STATUS isc_gennotdef                        = 335544463L;
const ISC_STATUS isc_cant_start_logging               = 335544464L;
const ISC_STATUS isc_bad_segstr_type                  = 335544465L;
const ISC_STATUS isc_foreign_key                      = 335544466L;
const ISC_STATUS isc_high_minor                       = 335544467L;
const ISC_STATUS isc_tra_state                        = 335544468L;
const ISC_STATUS isc_trans_invalid                    = 335544469L;
const ISC_STATUS isc_buf_invalid                      = 335544470L;
const ISC_STATUS isc_indexnotdefined                  = 335544471L;
const ISC_STATUS isc_login                            = 335544472L;
const ISC_STATUS isc_invalid_bookmark                 = 335544473L;
const ISC_STATUS isc_bad_lock_level                   = 335544474L;
const ISC_STATUS isc_relation_lock                    = 335544475L;
const ISC_STATUS isc_record_lock                      = 335544476L;
const ISC_STATUS isc_max_idx                          = 335544477L;
const ISC_STATUS isc_jrn_enable                       = 335544478L;
const ISC_STATUS isc_old_failure                      = 335544479L;
const ISC_STATUS isc_old_in_progress                  = 335544480L;
const ISC_STATUS isc_old_no_space                     = 335544481L;
const ISC_STATUS isc_no_wal_no_jrn                    = 335544482L;
const ISC_STATUS isc_num_old_files                    = 335544483L;
const ISC_STATUS isc_wal_file_open                    = 335544484L;
const ISC_STATUS isc_bad_stmt_handle                  = 335544485L;
const ISC_STATUS isc_wal_failure                      = 335544486L;
const ISC_STATUS isc_walw_err                         = 335544487L;
const ISC_STATUS isc_logh_small                       = 335544488L;
const ISC_STATUS isc_logh_inv_version                 = 335544489L;
const ISC_STATUS isc_logh_open_flag                   = 335544490L;
const ISC_STATUS isc_logh_open_flag2                  = 335544491L;
const ISC_STATUS isc_logh_diff_dbname                 = 335544492L;
const ISC_STATUS isc_logf_unexpected_eof              = 335544493L;
const ISC_STATUS isc_logr_incomplete                  = 335544494L;
const ISC_STATUS isc_logr_header_small                = 335544495L;
const ISC_STATUS isc_logb_small                       = 335544496L;
const ISC_STATUS isc_wal_illegal_attach               = 335544497L;
const ISC_STATUS isc_wal_invalid_wpb                  = 335544498L;
const ISC_STATUS isc_wal_err_rollover                 = 335544499L;
const ISC_STATUS isc_no_wal                           = 335544500L;
const ISC_STATUS isc_drop_wal                         = 335544501L;
const ISC_STATUS isc_stream_not_defined               = 335544502L;
const ISC_STATUS isc_wal_subsys_error                 = 335544503L;
const ISC_STATUS isc_wal_subsys_corrupt               = 335544504L;
const ISC_STATUS isc_no_archive                       = 335544505L;
const ISC_STATUS isc_shutinprog                       = 335544506L;
const ISC_STATUS isc_range_in_use                     = 335544507L;
const ISC_STATUS isc_range_not_found                  = 335544508L;
const ISC_STATUS isc_charset_not_found                = 335544509L;
const ISC_STATUS isc_lock_timeout                     = 335544510L;
const ISC_STATUS isc_prcnotdef                        = 335544511L;
const ISC_STATUS isc_prcmismat                        = 335544512L;
const ISC_STATUS isc_wal_bugcheck                     = 335544513L;
const ISC_STATUS isc_wal_cant_expand                  = 335544514L;
const ISC_STATUS isc_codnotdef                        = 335544515L;
const ISC_STATUS isc_xcpnotdef                        = 335544516L;
const ISC_STATUS isc_except                           = 335544517L;
const ISC_STATUS isc_cache_restart                    = 335544518L;
const ISC_STATUS isc_bad_lock_handle                  = 335544519L;
const ISC_STATUS isc_jrn_present                      = 335544520L;
const ISC_STATUS isc_wal_err_rollover2                = 335544521L;
const ISC_STATUS isc_wal_err_logwrite                 = 335544522L;
const ISC_STATUS isc_wal_err_jrn_comm                 = 335544523L;
const ISC_STATUS isc_wal_err_expansion                = 335544524L;
const ISC_STATUS isc_wal_err_setup                    = 335544525L;
const ISC_STATUS isc_wal_err_ww_sync                  = 335544526L;
const ISC_STATUS isc_wal_err_ww_start                 = 335544527L;
const ISC_STATUS isc_shutdown                         = 335544528L;
const ISC_STATUS isc_existing_priv_mod                = 335544529L;
const ISC_STATUS isc_primary_key_ref                  = 335544530L;
const ISC_STATUS isc_primary_key_notnull              = 335544531L;
const ISC_STATUS isc_ref_cnstrnt_notfound             = 335544532L;
const ISC_STATUS isc_foreign_key_notfound             = 335544533L;
const ISC_STATUS isc_ref_cnstrnt_update               = 335544534L;
const ISC_STATUS isc_check_cnstrnt_update             = 335544535L;
const ISC_STATUS isc_check_cnstrnt_del                = 335544536L;
const ISC_STATUS isc_integ_index_seg_del              = 335544537L;
const ISC_STATUS isc_integ_index_seg_mod              = 335544538L;
const ISC_STATUS isc_integ_index_del                  = 335544539L;
const ISC_STATUS isc_integ_index_mod                  = 335544540L;
const ISC_STATUS isc_check_trig_del                   = 335544541L;
const ISC_STATUS isc_check_trig_update                = 335544542L;
const ISC_STATUS isc_cnstrnt_fld_del                  = 335544543L;
const ISC_STATUS isc_cnstrnt_fld_rename               = 335544544L;
const ISC_STATUS isc_rel_cnstrnt_update               = 335544545L;
const ISC_STATUS isc_constaint_on_view                = 335544546L;
const ISC_STATUS isc_invld_cnstrnt_type               = 335544547L;
const ISC_STATUS isc_primary_key_exists               = 335544548L;
const ISC_STATUS isc_systrig_update                   = 335544549L;
const ISC_STATUS isc_not_rel_owner                    = 335544550L;
const ISC_STATUS isc_grant_obj_notfound               = 335544551L;
const ISC_STATUS isc_grant_fld_notfound               = 335544552L;
const ISC_STATUS isc_grant_nopriv                     = 335544553L;
const ISC_STATUS isc_nonsql_security_rel              = 335544554L;
const ISC_STATUS isc_nonsql_security_fld              = 335544555L;
const ISC_STATUS isc_wal_cache_err                    = 335544556L;
const ISC_STATUS isc_shutfail                         = 335544557L;
const ISC_STATUS isc_check_constraint                 = 335544558L;
const ISC_STATUS isc_bad_svc_handle                   = 335544559L;
const ISC_STATUS isc_shutwarn                         = 335544560L;
const ISC_STATUS isc_wrospbver                        = 335544561L;
const ISC_STATUS isc_bad_spb_form                     = 335544562L;
const ISC_STATUS isc_svcnotdef                        = 335544563L;
const ISC_STATUS isc_no_jrn                           = 335544564L;
const ISC_STATUS isc_transliteration_failed           = 335544565L;
const ISC_STATUS isc_start_cm_for_wal                 = 335544566L;
const ISC_STATUS isc_wal_ovflow_log_required          = 335544567L;
const ISC_STATUS isc_text_subtype                     = 335544568L;
const ISC_STATUS isc_dsql_error                       = 335544569L;
const ISC_STATUS isc_dsql_command_err                 = 335544570L;
const ISC_STATUS isc_dsql_constant_err                = 335544571L;
const ISC_STATUS isc_dsql_cursor_err                  = 335544572L;
const ISC_STATUS isc_dsql_datatype_err                = 335544573L;
const ISC_STATUS isc_dsql_decl_err                    = 335544574L;
const ISC_STATUS isc_dsql_cursor_update_err           = 335544575L;
const ISC_STATUS isc_dsql_cursor_open_err             = 335544576L;
const ISC_STATUS isc_dsql_cursor_close_err            = 335544577L;
const ISC_STATUS isc_dsql_field_err                   = 335544578L;
const ISC_STATUS isc_dsql_internal_err                = 335544579L;
const ISC_STATUS isc_dsql_relation_err                = 335544580L;
const ISC_STATUS isc_dsql_procedure_err               = 335544581L;
const ISC_STATUS isc_dsql_request_err                 = 335544582L;
const ISC_STATUS isc_dsql_sqlda_err                   = 335544583L;
const ISC_STATUS isc_dsql_var_count_err               = 335544584L;
const ISC_STATUS isc_dsql_stmt_handle                 = 335544585L;
const ISC_STATUS isc_dsql_function_err                = 335544586L;
const ISC_STATUS isc_dsql_blob_err                    = 335544587L;
const ISC_STATUS isc_collation_not_found              = 335544588L;
const ISC_STATUS isc_collation_not_for_charset        = 335544589L;
const ISC_STATUS isc_dsql_dup_option                  = 335544590L;
const ISC_STATUS isc_dsql_tran_err                    = 335544591L;
const ISC_STATUS isc_dsql_invalid_array               = 335544592L;
const ISC_STATUS isc_dsql_max_arr_dim_exceeded        = 335544593L;
const ISC_STATUS isc_dsql_arr_range_error             = 335544594L;
const ISC_STATUS isc_dsql_trigger_err                 = 335544595L;
const ISC_STATUS isc_dsql_subselect_err               = 335544596L;
const ISC_STATUS isc_dsql_crdb_prepare_err            = 335544597L;
const ISC_STATUS isc_specify_field_err                = 335544598L;
const ISC_STATUS isc_num_field_err                    = 335544599L;
const ISC_STATUS isc_col_name_err                     = 335544600L;
const ISC_STATUS isc_where_err                        = 335544601L;
const ISC_STATUS isc_table_view_err                   = 335544602L;
const ISC_STATUS isc_distinct_err                     = 335544603L;
const ISC_STATUS isc_key_field_count_err              = 335544604L;
const ISC_STATUS isc_subquery_err                     = 335544605L;
const ISC_STATUS isc_expression_eval_err              = 335544606L;
const ISC_STATUS isc_node_err                         = 335544607L;
const ISC_STATUS isc_command_end_err                  = 335544608L;
const ISC_STATUS isc_index_name                       = 335544609L;
const ISC_STATUS isc_exception_name                   = 335544610L;
const ISC_STATUS isc_field_name                       = 335544611L;
const ISC_STATUS isc_token_err                        = 335544612L;
const ISC_STATUS isc_union_err                        = 335544613L;
const ISC_STATUS isc_dsql_construct_err               = 335544614L;
const ISC_STATUS isc_field_aggregate_err              = 335544615L;
const ISC_STATUS isc_field_ref_err                    = 335544616L;
const ISC_STATUS isc_order_by_err                     = 335544617L;
const ISC_STATUS isc_return_mode_err                  = 335544618L;
const ISC_STATUS isc_extern_func_err                  = 335544619L;
const ISC_STATUS isc_alias_conflict_err               = 335544620L;
const ISC_STATUS isc_procedure_conflict_error         = 335544621L;
const ISC_STATUS isc_relation_conflict_err            = 335544622L;
const ISC_STATUS isc_dsql_domain_err                  = 335544623L;
const ISC_STATUS isc_idx_seg_err                      = 335544624L;
const ISC_STATUS isc_node_name_err                    = 335544625L;
const ISC_STATUS isc_table_name                       = 335544626L;
const ISC_STATUS isc_proc_name                        = 335544627L;
const ISC_STATUS isc_idx_create_err                   = 335544628L;
const ISC_STATUS isc_wal_shadow_err                   = 335544629L;
const ISC_STATUS isc_dependency                       = 335544630L;
const ISC_STATUS isc_idx_key_err                      = 335544631L;
const ISC_STATUS isc_dsql_file_length_err             = 335544632L;
const ISC_STATUS isc_dsql_shadow_number_err           = 335544633L;
const ISC_STATUS isc_dsql_token_unk_err               = 335544634L;
const ISC_STATUS isc_dsql_no_relation_alias           = 335544635L;
const ISC_STATUS isc_indexname                        = 335544636L;
const ISC_STATUS isc_no_stream_plan                   = 335544637L;
const ISC_STATUS isc_stream_twice                     = 335544638L;
const ISC_STATUS isc_stream_not_found                 = 335544639L;
const ISC_STATUS isc_collation_requires_text          = 335544640L;
const ISC_STATUS isc_dsql_domain_not_found            = 335544641L;
const ISC_STATUS isc_index_unused                     = 335544642L;
const ISC_STATUS isc_dsql_self_join                   = 335544643L;
const ISC_STATUS isc_stream_bof                       = 335544644L;
const ISC_STATUS isc_stream_crack                     = 335544645L;
const ISC_STATUS isc_db_or_file_exists                = 335544646L;
const ISC_STATUS isc_invalid_operator                 = 335544647L;
const ISC_STATUS isc_conn_lost                        = 335544648L;
const ISC_STATUS isc_bad_checksum                     = 335544649L;
const ISC_STATUS isc_page_type_err                    = 335544650L;
const ISC_STATUS isc_ext_readonly_err                 = 335544651L;
const ISC_STATUS isc_sing_select_err                  = 335544652L;
const ISC_STATUS isc_psw_attach                       = 335544653L;
const ISC_STATUS isc_psw_start_trans                  = 335544654L;
const ISC_STATUS isc_invalid_direction                = 335544655L;
const ISC_STATUS isc_dsql_var_conflict                = 335544656L;
const ISC_STATUS isc_dsql_no_blob_array               = 335544657L;
const ISC_STATUS isc_dsql_base_table                  = 335544658L;
const ISC_STATUS isc_duplicate_base_table             = 335544659L;
const ISC_STATUS isc_view_alias                       = 335544660L;
const ISC_STATUS isc_index_root_page_full             = 335544661L;
const ISC_STATUS isc_dsql_blob_type_unknown           = 335544662L;
const ISC_STATUS isc_req_max_clones_exceeded          = 335544663L;
const ISC_STATUS isc_dsql_duplicate_spec              = 335544664L;
const ISC_STATUS isc_unique_key_violation             = 335544665L;
const ISC_STATUS isc_srvr_version_too_old             = 335544666L;
const ISC_STATUS isc_drdb_completed_with_errs         = 335544667L;
const ISC_STATUS isc_dsql_procedure_use_err           = 335544668L;
const ISC_STATUS isc_dsql_count_mismatch              = 335544669L;
const ISC_STATUS isc_blob_idx_err                     = 335544670L;
const ISC_STATUS isc_array_idx_err                    = 335544671L;
const ISC_STATUS isc_key_field_err                    = 335544672L;
const ISC_STATUS isc_no_delete                        = 335544673L;
const ISC_STATUS isc_del_last_field                   = 335544674L;
const ISC_STATUS isc_sort_err                         = 335544675L;
const ISC_STATUS isc_sort_mem_err                     = 335544676L;
const ISC_STATUS isc_version_err                      = 335544677L;
const ISC_STATUS isc_inval_key_posn                   = 335544678L;
const ISC_STATUS isc_no_segments_err                  = 335544679L;
const ISC_STATUS isc_crrp_data_err                    = 335544680L;
const ISC_STATUS isc_rec_size_err                     = 335544681L;
const ISC_STATUS isc_dsql_field_ref                   = 335544682L;
const ISC_STATUS isc_req_depth_exceeded               = 335544683L;
const ISC_STATUS isc_no_field_access                  = 335544684L;
const ISC_STATUS isc_no_dbkey                         = 335544685L;
const ISC_STATUS isc_jrn_format_err                   = 335544686L;
const ISC_STATUS isc_jrn_file_full                    = 335544687L;
const ISC_STATUS isc_dsql_open_cursor_request         = 335544688L;
const ISC_STATUS isc_ib_error                         = 335544689L;
const ISC_STATUS isc_cache_redef                      = 335544690L;
const ISC_STATUS isc_cache_too_small                  = 335544691L;
const ISC_STATUS isc_log_redef                        = 335544692L;
const ISC_STATUS isc_log_too_small                    = 335544693L;
const ISC_STATUS isc_partition_too_small              = 335544694L;
const ISC_STATUS isc_partition_not_supp               = 335544695L;
const ISC_STATUS isc_log_length_spec                  = 335544696L;
const ISC_STATUS isc_precision_err                    = 335544697L;
const ISC_STATUS isc_scale_nogt                       = 335544698L;
const ISC_STATUS isc_expec_short                      = 335544699L;
const ISC_STATUS isc_expec_long                       = 335544700L;
const ISC_STATUS isc_expec_ushort                     = 335544701L;
const ISC_STATUS isc_escape_invalid                   = 335544702L;
const ISC_STATUS isc_svcnoexe                         = 335544703L;
const ISC_STATUS isc_net_lookup_err                   = 335544704L;
const ISC_STATUS isc_service_unknown                  = 335544705L;
const ISC_STATUS isc_host_unknown                     = 335544706L;
const ISC_STATUS isc_grant_nopriv_on_base             = 335544707L;
const ISC_STATUS isc_dyn_fld_ambiguous                = 335544708L;
const ISC_STATUS isc_dsql_agg_ref_err                 = 335544709L;
const ISC_STATUS isc_complex_view                     = 335544710L;
const ISC_STATUS isc_unprepared_stmt                  = 335544711L;
const ISC_STATUS isc_expec_positive                   = 335544712L;
const ISC_STATUS isc_dsql_sqlda_value_err             = 335544713L;
const ISC_STATUS isc_invalid_array_id                 = 335544714L;
const ISC_STATUS isc_extfile_uns_op                   = 335544715L;
const ISC_STATUS isc_svc_in_use                       = 335544716L;
const ISC_STATUS isc_err_stack_limit                  = 335544717L;
const ISC_STATUS isc_invalid_key                      = 335544718L;
const ISC_STATUS isc_net_init_error                   = 335544719L;
const ISC_STATUS isc_loadlib_failure                  = 335544720L;
const ISC_STATUS isc_network_error                    = 335544721L;
const ISC_STATUS isc_net_connect_err                  = 335544722L;
const ISC_STATUS isc_net_connect_listen_err           = 335544723L;
const ISC_STATUS isc_net_event_connect_err            = 335544724L;
const ISC_STATUS isc_net_event_listen_err             = 335544725L;
const ISC_STATUS isc_net_read_err                     = 335544726L;
const ISC_STATUS isc_net_write_err                    = 335544727L;
const ISC_STATUS isc_integ_index_deactivate           = 335544728L;
const ISC_STATUS isc_integ_deactivate_primary         = 335544729L;
const ISC_STATUS isc_cse_not_supported                = 335544730L;
const ISC_STATUS isc_tra_must_sweep                   = 335544731L;
const ISC_STATUS isc_unsupported_network_drive        = 335544732L;
const ISC_STATUS isc_io_create_err                    = 335544733L;
const ISC_STATUS isc_io_open_err                      = 335544734L;
const ISC_STATUS isc_io_close_err                     = 335544735L;
const ISC_STATUS isc_io_read_err                      = 335544736L;
const ISC_STATUS isc_io_write_err                     = 335544737L;
const ISC_STATUS isc_io_delete_err                    = 335544738L;
const ISC_STATUS isc_io_access_err                    = 335544739L;
const ISC_STATUS isc_udf_exception                    = 335544740L;
const ISC_STATUS isc_lost_db_connection               = 335544741L;
const ISC_STATUS isc_no_write_user_priv               = 335544742L;
const ISC_STATUS isc_token_too_long                   = 335544743L;
const ISC_STATUS isc_max_att_exceeded                 = 335544744L;
const ISC_STATUS isc_login_same_as_role_name          = 335544745L;
const ISC_STATUS isc_reftable_requires_pk             = 335544746L;
const ISC_STATUS isc_usrname_too_long                 = 335544747L;
const ISC_STATUS isc_password_too_long                = 335544748L;
const ISC_STATUS isc_usrname_required                 = 335544749L;
const ISC_STATUS isc_password_required                = 335544750L;
const ISC_STATUS isc_bad_protocol                     = 335544751L;
const ISC_STATUS isc_dup_usrname_found                = 335544752L;
const ISC_STATUS isc_usrname_not_found                = 335544753L;
const ISC_STATUS isc_error_adding_sec_record          = 335544754L;
const ISC_STATUS isc_error_modifying_sec_record       = 335544755L;
const ISC_STATUS isc_error_deleting_sec_record        = 335544756L;
const ISC_STATUS isc_error_updating_sec_db            = 335544757L;
const ISC_STATUS isc_sort_rec_size_err                = 335544758L;
const ISC_STATUS isc_bad_default_value                = 335544759L;
const ISC_STATUS isc_invalid_clause                   = 335544760L;
const ISC_STATUS isc_too_many_handles                 = 335544761L;
const ISC_STATUS isc_optimizer_blk_exc                = 335544762L;
const ISC_STATUS isc_invalid_string_constant          = 335544763L;
const ISC_STATUS isc_transitional_date                = 335544764L;
const ISC_STATUS isc_read_only_database               = 335544765L;
const ISC_STATUS isc_must_be_dialect_2_and_up         = 335544766L;
const ISC_STATUS isc_blob_filter_exception            = 335544767L;
const ISC_STATUS isc_exception_access_violation       = 335544768L;
const ISC_STATUS isc_exception_datatype_missalignment = 335544769L;
const ISC_STATUS isc_exception_array_bounds_exceeded  = 335544770L;
const ISC_STATUS isc_exception_float_denormal_operand = 335544771L;
const ISC_STATUS isc_exception_float_divide_by_zero   = 335544772L;
const ISC_STATUS isc_exception_float_inexact_result   = 335544773L;
const ISC_STATUS isc_exception_float_invalid_operand  = 335544774L;
const ISC_STATUS isc_exception_float_overflow         = 335544775L;
const ISC_STATUS isc_exception_float_stack_check      = 335544776L;
const ISC_STATUS isc_exception_float_underflow        = 335544777L;
const ISC_STATUS isc_exception_integer_divide_by_zero = 335544778L;
const ISC_STATUS isc_exception_integer_overflow       = 335544779L;
const ISC_STATUS isc_exception_unknown                = 335544780L;
const ISC_STATUS isc_exception_stack_overflow         = 335544781L;
const ISC_STATUS isc_exception_sigsegv                = 335544782L;
const ISC_STATUS isc_exception_sigill                 = 335544783L;
const ISC_STATUS isc_exception_sigbus                 = 335544784L;
const ISC_STATUS isc_exception_sigfpe                 = 335544785L;
const ISC_STATUS isc_ext_file_delete                  = 335544786L;
const ISC_STATUS isc_ext_file_modify                  = 335544787L;
const ISC_STATUS isc_adm_task_denied                  = 335544788L;
const ISC_STATUS isc_extract_input_mismatch           = 335544789L;
const ISC_STATUS isc_insufficient_svc_privileges      = 335544790L;
const ISC_STATUS isc_file_in_use                      = 335544791L;
const ISC_STATUS isc_service_att_err                  = 335544792L;
const ISC_STATUS isc_ddl_not_allowed_by_db_sql_dial   = 335544793L;
const ISC_STATUS isc_cancelled                        = 335544794L;
const ISC_STATUS isc_unexp_spb_form                   = 335544795L;
const ISC_STATUS isc_sql_dialect_datatype_unsupport   = 335544796L;
const ISC_STATUS isc_svcnouser                        = 335544797L;
const ISC_STATUS isc_depend_on_uncommitted_rel        = 335544798L;
const ISC_STATUS isc_svc_name_missing                 = 335544799L;
const ISC_STATUS isc_too_many_contexts                = 335544800L;
const ISC_STATUS isc_datype_notsup                    = 335544801L;
const ISC_STATUS isc_dialect_reset_warning            = 335544802L;
const ISC_STATUS isc_dialect_not_changed              = 335544803L;
const ISC_STATUS isc_database_create_failed           = 335544804L;
const ISC_STATUS isc_inv_dialect_specified            = 335544805L;
const ISC_STATUS isc_valid_db_dialects                = 335544806L;
const ISC_STATUS isc_sqlwarn                          = 335544807L;
const ISC_STATUS isc_dtype_renamed                    = 335544808L;
const ISC_STATUS isc_extern_func_dir_error            = 335544809L;
const ISC_STATUS isc_date_range_exceeded              = 335544810L;
const ISC_STATUS isc_inv_client_dialect_specified     = 335544811L;
const ISC_STATUS isc_valid_client_dialects            = 335544812L;
const ISC_STATUS isc_optimizer_between_err            = 335544813L;
const ISC_STATUS isc_service_not_supported            = 335544814L;
const ISC_STATUS isc_generator_name                   = 335544815L;
const ISC_STATUS isc_udf_name                         = 335544816L;
const ISC_STATUS isc_bad_limit_param                  = 335544817L;
const ISC_STATUS isc_bad_skip_param                   = 335544818L;
const ISC_STATUS isc_io_32bit_exceeded_err            = 335544819L;
const ISC_STATUS isc_invalid_savepoint                = 335544820L;
const ISC_STATUS isc_dsql_column_pos_err              = 335544821L;
const ISC_STATUS isc_dsql_agg_where_err               = 335544822L;
const ISC_STATUS isc_dsql_agg_group_err               = 335544823L;
const ISC_STATUS isc_dsql_agg_column_err              = 335544824L;
const ISC_STATUS isc_dsql_agg_having_err              = 335544825L;
const ISC_STATUS isc_dsql_agg_nested_err              = 335544826L;
const ISC_STATUS isc_exec_sql_invalid_arg             = 335544827L;
const ISC_STATUS isc_exec_sql_invalid_req             = 335544828L;
const ISC_STATUS isc_exec_sql_invalid_var             = 335544829L;
const ISC_STATUS isc_exec_sql_max_call_exceeded       = 335544830L;
const ISC_STATUS isc_conf_access_denied               = 335544831L;
const ISC_STATUS isc_wrong_backup_state               = 335544832L;
const ISC_STATUS isc_wal_backup_err                   = 335544833L;
const ISC_STATUS isc_cursor_not_open                  = 335544834L;
const ISC_STATUS isc_bad_shutdown_mode                = 335544835L;
const ISC_STATUS isc_concat_overflow                  = 335544836L;
const ISC_STATUS isc_bad_substring_offset             = 335544837L;
const ISC_STATUS isc_foreign_key_target_doesnt_exist  = 335544838L;
const ISC_STATUS isc_foreign_key_references_present   = 335544839L;
const ISC_STATUS isc_no_update                        = 335544840L;
const ISC_STATUS isc_cursor_already_open              = 335544841L;
const ISC_STATUS isc_stack_trace                      = 335544842L;
const ISC_STATUS isc_ctx_var_not_found                = 335544843L;
const ISC_STATUS isc_ctx_namespace_invalid            = 335544844L;
const ISC_STATUS isc_ctx_too_big                      = 335544845L;
const ISC_STATUS isc_ctx_bad_argument                 = 335544846L;
const ISC_STATUS isc_identifier_too_long              = 335544847L;
const ISC_STATUS isc_except2                          = 335544848L;
const ISC_STATUS isc_malformed_string                 = 335544849L;
const ISC_STATUS isc_prc_out_param_mismatch           = 335544850L;
const ISC_STATUS isc_command_end_err2                 = 335544851L;
const ISC_STATUS isc_partner_idx_incompat_type        = 335544852L;
const ISC_STATUS isc_bad_substring_length             = 335544853L;
const ISC_STATUS isc_charset_not_installed            = 335544854L;
const ISC_STATUS isc_collation_not_installed          = 335544855L;
const ISC_STATUS isc_att_shutdown                     = 335544856L;
const ISC_STATUS isc_blobtoobig                       = 335544857L;
const ISC_STATUS isc_must_have_phys_field             = 335544858L;
const ISC_STATUS isc_invalid_time_precision           = 335544859L;
const ISC_STATUS isc_blob_convert_error               = 335544860L;
const ISC_STATUS isc_array_convert_error              = 335544861L;
const ISC_STATUS isc_record_lock_not_supp             = 335544862L;
const ISC_STATUS isc_partner_idx_not_found            = 335544863L;
const ISC_STATUS isc_tra_num_exc                      = 335544864L;
const ISC_STATUS isc_field_disappeared                = 335544865L;
const ISC_STATUS isc_met_wrong_gtt_scope              = 335544866L;
const ISC_STATUS isc_subtype_for_internal_use         = 335544867L;
const ISC_STATUS isc_illegal_prc_type                 = 335544868L;
const ISC_STATUS isc_invalid_sort_datatype            = 335544869L;
const ISC_STATUS isc_collation_name                   = 335544870L;
const ISC_STATUS isc_domain_name                      = 335544871L;
const ISC_STATUS isc_domnotdef                        = 335544872L;
const ISC_STATUS isc_array_max_dimensions             = 335544873L;
const ISC_STATUS isc_max_db_per_trans_allowed         = 335544874L;
const ISC_STATUS isc_bad_debug_format                 = 335544875L;
const ISC_STATUS isc_bad_proc_BLR                     = 335544876L;
const ISC_STATUS isc_key_too_big                      = 335544877L;
const ISC_STATUS isc_concurrent_transaction           = 335544878L;
const ISC_STATUS isc_not_valid_for_var                = 335544879L;
const ISC_STATUS isc_not_valid_for                    = 335544880L;
const ISC_STATUS isc_need_difference                  = 335544881L;
const ISC_STATUS isc_long_login                       = 335544882L;
const ISC_STATUS isc_fldnotdef2                       = 335544883L;
const ISC_STATUS isc_invalid_similar_pattern          = 335544884L;
const ISC_STATUS isc_bad_teb_form                     = 335544885L;
const ISC_STATUS isc_tpb_multiple_txn_isolation       = 335544886L;
const ISC_STATUS isc_tpb_reserv_before_table          = 335544887L;
const ISC_STATUS isc_tpb_multiple_spec                = 335544888L;
const ISC_STATUS isc_tpb_option_without_rc            = 335544889L;
const ISC_STATUS isc_tpb_conflicting_options          = 335544890L;
const ISC_STATUS isc_tpb_reserv_missing_tlen          = 335544891L;
const ISC_STATUS isc_tpb_reserv_long_tlen             = 335544892L;
const ISC_STATUS isc_tpb_reserv_missing_tname         = 335544893L;
const ISC_STATUS isc_tpb_reserv_corrup_tlen           = 335544894L;
const ISC_STATUS isc_tpb_reserv_null_tlen             = 335544895L;
const ISC_STATUS isc_tpb_reserv_relnotfound           = 335544896L;
const ISC_STATUS isc_tpb_reserv_baserelnotfound       = 335544897L;
const ISC_STATUS isc_tpb_missing_len                  = 335544898L;
const ISC_STATUS isc_tpb_missing_value                = 335544899L;
const ISC_STATUS isc_tpb_corrupt_len                  = 335544900L;
const ISC_STATUS isc_tpb_null_len                     = 335544901L;
const ISC_STATUS isc_tpb_overflow_len                 = 335544902L;
const ISC_STATUS isc_tpb_invalid_value                = 335544903L;
const ISC_STATUS isc_tpb_reserv_stronger_wng          = 335544904L;
const ISC_STATUS isc_tpb_reserv_stronger              = 335544905L;
const ISC_STATUS isc_tpb_reserv_max_recursion         = 335544906L;
const ISC_STATUS isc_tpb_reserv_virtualtbl            = 335544907L;
const ISC_STATUS isc_tpb_reserv_systbl                = 335544908L;
const ISC_STATUS isc_tpb_reserv_temptbl               = 335544909L;
const ISC_STATUS isc_tpb_readtxn_after_writelock      = 335544910L;
const ISC_STATUS isc_tpb_writelock_after_readtxn      = 335544911L;
const ISC_STATUS isc_time_range_exceeded              = 335544912L;
const ISC_STATUS isc_datetime_range_exceeded          = 335544913L;
const ISC_STATUS isc_string_truncation                = 335544914L;
const ISC_STATUS isc_blob_truncation                  = 335544915L;
const ISC_STATUS isc_numeric_out_of_range             = 335544916L;
const ISC_STATUS isc_shutdown_timeout                 = 335544917L;
const ISC_STATUS isc_att_handle_busy                  = 335544918L;
const ISC_STATUS isc_bad_udf_freeit                   = 335544919L;
const ISC_STATUS isc_eds_provider_not_found           = 335544920L;
const ISC_STATUS isc_eds_connection                   = 335544921L;
const ISC_STATUS isc_eds_preprocess                   = 335544922L;
const ISC_STATUS isc_eds_stmt_expected                = 335544923L;
const ISC_STATUS isc_eds_prm_name_expected            = 335544924L;
const ISC_STATUS isc_eds_unclosed_comment             = 335544925L;
const ISC_STATUS isc_eds_statement                    = 335544926L;
const ISC_STATUS isc_eds_input_prm_mismatch           = 335544927L;
const ISC_STATUS isc_eds_output_prm_mismatch          = 335544928L;
const ISC_STATUS isc_eds_input_prm_not_set            = 335544929L;
const ISC_STATUS isc_too_big_blr                      = 335544930L;
const ISC_STATUS isc_montabexh                        = 335544931L;
const ISC_STATUS isc_modnotfound                      = 335544932L;
const ISC_STATUS isc_nothing_to_cancel                = 335544933L;
const ISC_STATUS isc_ibutil_not_loaded                = 335544934L;
const ISC_STATUS isc_circular_computed                = 335544935L;
const ISC_STATUS isc_psw_db_error                     = 335544936L;
const ISC_STATUS isc_invalid_type_datetime_op         = 335544937L;
const ISC_STATUS isc_onlycan_add_timetodate           = 335544938L;
const ISC_STATUS isc_onlycan_add_datetotime           = 335544939L;
const ISC_STATUS isc_onlycansub_tstampfromtstamp      = 335544940L;
const ISC_STATUS isc_onlyoneop_mustbe_tstamp          = 335544941L;
const ISC_STATUS isc_invalid_extractpart_time         = 335544942L;
const ISC_STATUS isc_invalid_extractpart_date         = 335544943L;
const ISC_STATUS isc_invalidarg_extract               = 335544944L;
const ISC_STATUS isc_sysf_argmustbe_exact             = 335544945L;
const ISC_STATUS isc_sysf_argmustbe_exact_or_fp       = 335544946L;
const ISC_STATUS isc_sysf_argviolates_uuidtype        = 335544947L;
const ISC_STATUS isc_sysf_argviolates_uuidlen         = 335544948L;
const ISC_STATUS isc_sysf_argviolates_uuidfmt         = 335544949L;
const ISC_STATUS isc_sysf_argviolates_guidigits       = 335544950L;
const ISC_STATUS isc_sysf_invalid_addpart_time        = 335544951L;
const ISC_STATUS isc_sysf_invalid_add_datetime        = 335544952L;
const ISC_STATUS isc_sysf_invalid_addpart_dtime       = 335544953L;
const ISC_STATUS isc_sysf_invalid_add_dtime_rc        = 335544954L;
const ISC_STATUS isc_sysf_invalid_diff_dtime          = 335544955L;
const ISC_STATUS isc_sysf_invalid_timediff            = 335544956L;
const ISC_STATUS isc_sysf_invalid_tstamptimediff      = 335544957L;
const ISC_STATUS isc_sysf_invalid_datetimediff        = 335544958L;
const ISC_STATUS isc_sysf_invalid_diffpart            = 335544959L;
const ISC_STATUS isc_sysf_argmustbe_positive          = 335544960L;
const ISC_STATUS isc_sysf_basemustbe_positive         = 335544961L;
const ISC_STATUS isc_sysf_argnmustbe_nonneg           = 335544962L;
const ISC_STATUS isc_sysf_argnmustbe_positive         = 335544963L;
const ISC_STATUS isc_sysf_invalid_zeropowneg          = 335544964L;
const ISC_STATUS isc_sysf_invalid_negpowfp            = 335544965L;
const ISC_STATUS isc_sysf_invalid_scale               = 335544966L;
const ISC_STATUS isc_sysf_argmustbe_nonneg            = 335544967L;
const ISC_STATUS isc_sysf_binuuid_mustbe_str          = 335544968L;
const ISC_STATUS isc_sysf_binuuid_wrongsize           = 335544969L;
const ISC_STATUS isc_missing_required_spb             = 335544970L;
const ISC_STATUS isc_net_server_shutdown              = 335544971L;
const ISC_STATUS isc_bad_conn_str                     = 335544972L;
const ISC_STATUS isc_bad_epb_form                     = 335544973L;
const ISC_STATUS isc_no_threads                       = 335544974L;
const ISC_STATUS isc_net_event_connect_timeout        = 335544975L;
const ISC_STATUS isc_sysf_argmustbe_nonzero           = 335544976L;
const ISC_STATUS isc_sysf_argmustbe_range_inc1_1      = 335544977L;
const ISC_STATUS isc_sysf_argmustbe_gteq_one          = 335544978L;
const ISC_STATUS isc_sysf_argmustbe_range_exc1_1      = 335544979L;
const ISC_STATUS isc_internal_rejected_params         = 335544980L;
const ISC_STATUS isc_sysf_fp_overflow                 = 335544981L;
const ISC_STATUS isc_udf_fp_overflow                  = 335544982L;
const ISC_STATUS isc_udf_fp_nan                       = 335544983L;
const ISC_STATUS isc_instance_conflict                = 335544984L;
const ISC_STATUS isc_out_of_temp_space                = 335544985L;
const ISC_STATUS isc_eds_expl_tran_ctrl               = 335544986L;
const ISC_STATUS isc_no_trusted_spb                   = 335544987L;
const ISC_STATUS isc_async_active                     = 335545017L;
const ISC_STATUS isc_gfix_db_name                     = 335740929L;
const ISC_STATUS isc_gfix_invalid_sw                  = 335740930L;
const ISC_STATUS isc_gfix_incmp_sw                    = 335740932L;
const ISC_STATUS isc_gfix_replay_req                  = 335740933L;
const ISC_STATUS isc_gfix_pgbuf_req                   = 335740934L;
const ISC_STATUS isc_gfix_val_req                     = 335740935L;
const ISC_STATUS isc_gfix_pval_req                    = 335740936L;
const ISC_STATUS isc_gfix_trn_req                     = 335740937L;
const ISC_STATUS isc_gfix_full_req                    = 335740940L;
const ISC_STATUS isc_gfix_usrname_req                 = 335740941L;
const ISC_STATUS isc_gfix_pass_req                    = 335740942L;
const ISC_STATUS isc_gfix_subs_name                   = 335740943L;
const ISC_STATUS isc_gfix_wal_req                     = 335740944L;
const ISC_STATUS isc_gfix_sec_req                     = 335740945L;
const ISC_STATUS isc_gfix_nval_req                    = 335740946L;
const ISC_STATUS isc_gfix_type_shut                   = 335740947L;
const ISC_STATUS isc_gfix_retry                       = 335740948L;
const ISC_STATUS isc_gfix_retry_db                    = 335740951L;
const ISC_STATUS isc_gfix_exceed_max                  = 335740991L;
const ISC_STATUS isc_gfix_corrupt_pool                = 335740992L;
const ISC_STATUS isc_gfix_mem_exhausted               = 335740993L;
const ISC_STATUS isc_gfix_bad_pool                    = 335740994L;
const ISC_STATUS isc_gfix_trn_not_valid               = 335740995L;
const ISC_STATUS isc_gfix_unexp_eoi                   = 335741012L;
const ISC_STATUS isc_gfix_recon_fail                  = 335741018L;
const ISC_STATUS isc_gfix_trn_unknown                 = 335741036L;
const ISC_STATUS isc_gfix_mode_req                    = 335741038L;
const ISC_STATUS isc_gfix_pzval_req                   = 335741042L;
const ISC_STATUS isc_dsql_dbkey_from_non_table        = 336003074L;
const ISC_STATUS isc_dsql_transitional_numeric        = 336003075L;
const ISC_STATUS isc_dsql_dialect_warning_expr        = 336003076L;
const ISC_STATUS isc_sql_db_dialect_dtype_unsupport   = 336003077L;
const ISC_STATUS isc_isc_sql_dialect_conflict_num     = 336003079L;
const ISC_STATUS isc_dsql_warning_number_ambiguous    = 336003080L;
const ISC_STATUS isc_dsql_warning_number_ambiguous1   = 336003081L;
const ISC_STATUS isc_dsql_warn_precision_ambiguous    = 336003082L;
const ISC_STATUS isc_dsql_warn_precision_ambiguous1   = 336003083L;
const ISC_STATUS isc_dsql_warn_precision_ambiguous2   = 336003084L;
const ISC_STATUS isc_dsql_ambiguous_field_name        = 336003085L;
const ISC_STATUS isc_dsql_udf_return_pos_err          = 336003086L;
const ISC_STATUS isc_dsql_invalid_label               = 336003087L;
const ISC_STATUS isc_dsql_datatypes_not_comparable    = 336003088L;
const ISC_STATUS isc_dsql_cursor_invalid              = 336003089L;
const ISC_STATUS isc_dsql_cursor_redefined            = 336003090L;
const ISC_STATUS isc_dsql_cursor_not_found            = 336003091L;
const ISC_STATUS isc_dsql_cursor_exists               = 336003092L;
const ISC_STATUS isc_dsql_cursor_rel_ambiguous        = 336003093L;
const ISC_STATUS isc_dsql_cursor_rel_not_found        = 336003094L;
const ISC_STATUS isc_dsql_cursor_not_open             = 336003095L;
const ISC_STATUS isc_dsql_type_not_supp_ext_tab       = 336003096L;
const ISC_STATUS isc_dsql_feature_not_supported_ods   = 336003097L;
const ISC_STATUS isc_primary_key_required             = 336003098L;
const ISC_STATUS isc_upd_ins_doesnt_match_pk          = 336003099L;
const ISC_STATUS isc_upd_ins_doesnt_match_matching    = 336003100L;
const ISC_STATUS isc_upd_ins_with_complex_view        = 336003101L;
const ISC_STATUS isc_dsql_incompatible_trigger_type   = 336003102L;
const ISC_STATUS isc_dsql_db_trigger_type_cant_change = 336003103L;
const ISC_STATUS isc_dyn_dup_table                    = 336068740L;
const ISC_STATUS isc_dyn_column_does_not_exist        = 336068784L;
const ISC_STATUS isc_dyn_role_does_not_exist          = 336068796L;
const ISC_STATUS isc_dyn_no_grant_admin_opt           = 336068797L;
const ISC_STATUS isc_dyn_user_not_role_member         = 336068798L;
const ISC_STATUS isc_dyn_delete_role_failed           = 336068799L;
const ISC_STATUS isc_dyn_grant_role_to_user           = 336068800L;
const ISC_STATUS isc_dyn_inv_sql_role_name            = 336068801L;
const ISC_STATUS isc_dyn_dup_sql_role                 = 336068802L;
const ISC_STATUS isc_dyn_kywd_spec_for_role           = 336068803L;
const ISC_STATUS isc_dyn_roles_not_supported          = 336068804L;
const ISC_STATUS isc_dyn_domain_name_exists           = 336068812L;
const ISC_STATUS isc_dyn_field_name_exists            = 336068813L;
const ISC_STATUS isc_dyn_dependency_exists            = 336068814L;
const ISC_STATUS isc_dyn_dtype_invalid                = 336068815L;
const ISC_STATUS isc_dyn_char_fld_too_small           = 336068816L;
const ISC_STATUS isc_dyn_invalid_dtype_conversion     = 336068817L;
const ISC_STATUS isc_dyn_dtype_conv_invalid           = 336068818L;
const ISC_STATUS isc_dyn_zero_len_id                  = 336068820L;
const ISC_STATUS isc_max_coll_per_charset             = 336068829L;
const ISC_STATUS isc_invalid_coll_attr                = 336068830L;
const ISC_STATUS isc_dyn_wrong_gtt_scope              = 336068840L;
const ISC_STATUS isc_dyn_scale_too_big                = 336068852L;
const ISC_STATUS isc_dyn_precision_too_small          = 336068853L;
const ISC_STATUS isc_dyn_miss_priv_warning            = 336068855L;
const ISC_STATUS isc_dyn_ods_not_supp_feature         = 336068856L;
const ISC_STATUS isc_dyn_cannot_addrem_computed       = 336068857L;
const ISC_STATUS isc_dyn_no_empty_pw                  = 336068858L;
const ISC_STATUS isc_dyn_dup_index                    = 336068859L;
const ISC_STATUS isc_gbak_unknown_switch              = 336330753L;
const ISC_STATUS isc_gbak_page_size_missing           = 336330754L;
const ISC_STATUS isc_gbak_page_size_toobig            = 336330755L;
const ISC_STATUS isc_gbak_redir_ouput_missing         = 336330756L;
const ISC_STATUS isc_gbak_switches_conflict           = 336330757L;
const ISC_STATUS isc_gbak_unknown_device              = 336330758L;
const ISC_STATUS isc_gbak_no_protection               = 336330759L;
const ISC_STATUS isc_gbak_page_size_not_allowed       = 336330760L;
const ISC_STATUS isc_gbak_multi_source_dest           = 336330761L;
const ISC_STATUS isc_gbak_filename_missing            = 336330762L;
const ISC_STATUS isc_gbak_dup_inout_names             = 336330763L;
const ISC_STATUS isc_gbak_inv_page_size               = 336330764L;
const ISC_STATUS isc_gbak_db_specified                = 336330765L;
const ISC_STATUS isc_gbak_db_exists                   = 336330766L;
const ISC_STATUS isc_gbak_unk_device                  = 336330767L;
const ISC_STATUS isc_gbak_blob_info_failed            = 336330772L;
const ISC_STATUS isc_gbak_unk_blob_item               = 336330773L;
const ISC_STATUS isc_gbak_get_seg_failed              = 336330774L;
const ISC_STATUS isc_gbak_close_blob_failed           = 336330775L;
const ISC_STATUS isc_gbak_open_blob_failed            = 336330776L;
const ISC_STATUS isc_gbak_put_blr_gen_id_failed       = 336330777L;
const ISC_STATUS isc_gbak_unk_type                    = 336330778L;
const ISC_STATUS isc_gbak_comp_req_failed             = 336330779L;
const ISC_STATUS isc_gbak_start_req_failed            = 336330780L;
const ISC_STATUS isc_gbak_rec_failed                  = 336330781L;
const ISC_STATUS isc_gbak_rel_req_failed              = 336330782L;
const ISC_STATUS isc_gbak_db_info_failed              = 336330783L;
const ISC_STATUS isc_gbak_no_db_desc                  = 336330784L;
const ISC_STATUS isc_gbak_db_create_failed            = 336330785L;
const ISC_STATUS isc_gbak_decomp_len_error            = 336330786L;
const ISC_STATUS isc_gbak_tbl_missing                 = 336330787L;
const ISC_STATUS isc_gbak_blob_col_missing            = 336330788L;
const ISC_STATUS isc_gbak_create_blob_failed          = 336330789L;
const ISC_STATUS isc_gbak_put_seg_failed              = 336330790L;
const ISC_STATUS isc_gbak_rec_len_exp                 = 336330791L;
const ISC_STATUS isc_gbak_inv_rec_len                 = 336330792L;
const ISC_STATUS isc_gbak_exp_data_type               = 336330793L;
const ISC_STATUS isc_gbak_gen_id_failed               = 336330794L;
const ISC_STATUS isc_gbak_unk_rec_type                = 336330795L;
const ISC_STATUS isc_gbak_inv_bkup_ver                = 336330796L;
const ISC_STATUS isc_gbak_missing_bkup_desc           = 336330797L;
const ISC_STATUS isc_gbak_string_trunc                = 336330798L;
const ISC_STATUS isc_gbak_cant_rest_record            = 336330799L;
const ISC_STATUS isc_gbak_send_failed                 = 336330800L;
const ISC_STATUS isc_gbak_no_tbl_name                 = 336330801L;
const ISC_STATUS isc_gbak_unexp_eof                   = 336330802L;
const ISC_STATUS isc_gbak_db_format_too_old           = 336330803L;
const ISC_STATUS isc_gbak_inv_array_dim               = 336330804L;
const ISC_STATUS isc_gbak_xdr_len_expected            = 336330807L;
const ISC_STATUS isc_gbak_open_bkup_error             = 336330817L;
const ISC_STATUS isc_gbak_open_error                  = 336330818L;
const ISC_STATUS isc_gbak_missing_block_fac           = 336330934L;
const ISC_STATUS isc_gbak_inv_block_fac               = 336330935L;
const ISC_STATUS isc_gbak_block_fac_specified         = 336330936L;
const ISC_STATUS isc_gbak_missing_username            = 336330940L;
const ISC_STATUS isc_gbak_missing_password            = 336330941L;
const ISC_STATUS isc_gbak_missing_skipped_bytes       = 336330952L;
const ISC_STATUS isc_gbak_inv_skipped_bytes           = 336330953L;
const ISC_STATUS isc_gbak_err_restore_charset         = 336330965L;
const ISC_STATUS isc_gbak_err_restore_collation       = 336330967L;
const ISC_STATUS isc_gbak_read_error                  = 336330972L;
const ISC_STATUS isc_gbak_write_error                 = 336330973L;
const ISC_STATUS isc_gbak_db_in_use                   = 336330985L;
const ISC_STATUS isc_gbak_sysmemex                    = 336330990L;
const ISC_STATUS isc_gbak_restore_role_failed         = 336331002L;
const ISC_STATUS isc_gbak_role_op_missing             = 336331005L;
const ISC_STATUS isc_gbak_page_buffers_missing        = 336331010L;
const ISC_STATUS isc_gbak_page_buffers_wrong_param    = 336331011L;
const ISC_STATUS isc_gbak_page_buffers_restore        = 336331012L;
const ISC_STATUS isc_gbak_inv_size                    = 336331014L;
const ISC_STATUS isc_gbak_file_outof_sequence         = 336331015L;
const ISC_STATUS isc_gbak_join_file_missing           = 336331016L;
const ISC_STATUS isc_gbak_stdin_not_supptd            = 336331017L;
const ISC_STATUS isc_gbak_stdout_not_supptd           = 336331018L;
const ISC_STATUS isc_gbak_bkup_corrupt                = 336331019L;
const ISC_STATUS isc_gbak_unk_db_file_spec            = 336331020L;
const ISC_STATUS isc_gbak_hdr_write_failed            = 336331021L;
const ISC_STATUS isc_gbak_disk_space_ex               = 336331022L;
const ISC_STATUS isc_gbak_size_lt_min                 = 336331023L;
const ISC_STATUS isc_gbak_svc_name_missing            = 336331025L;
const ISC_STATUS isc_gbak_not_ownr                    = 336331026L;
const ISC_STATUS isc_gbak_mode_req                    = 336331031L;
const ISC_STATUS isc_gbak_just_data                   = 336331033L;
const ISC_STATUS isc_gbak_data_only                   = 336331034L;
const ISC_STATUS isc_gbak_invalid_metadata            = 336331093L;
const ISC_STATUS isc_gbak_invalid_data                = 336331094L;
const ISC_STATUS isc_dsql_too_old_ods                 = 336397205L;
const ISC_STATUS isc_dsql_table_not_found             = 336397206L;
const ISC_STATUS isc_dsql_view_not_found              = 336397207L;
const ISC_STATUS isc_dsql_line_col_error              = 336397208L;
const ISC_STATUS isc_dsql_unknown_pos                 = 336397209L;
const ISC_STATUS isc_dsql_no_dup_name                 = 336397210L;
const ISC_STATUS isc_dsql_too_many_values             = 336397211L;
const ISC_STATUS isc_dsql_no_array_computed           = 336397212L;
const ISC_STATUS isc_dsql_implicit_domain_name        = 336397213L;
const ISC_STATUS isc_dsql_only_can_subscript_array    = 336397214L;
const ISC_STATUS isc_dsql_max_sort_items              = 336397215L;
const ISC_STATUS isc_dsql_max_group_items             = 336397216L;
const ISC_STATUS isc_dsql_conflicting_sort_field      = 336397217L;
const ISC_STATUS isc_dsql_derived_table_more_columns  = 336397218L;
const ISC_STATUS isc_dsql_derived_table_less_columns  = 336397219L;
const ISC_STATUS isc_dsql_derived_field_unnamed       = 336397220L;
const ISC_STATUS isc_dsql_derived_field_dup_name      = 336397221L;
const ISC_STATUS isc_dsql_derived_alias_select        = 336397222L;
const ISC_STATUS isc_dsql_derived_alias_field         = 336397223L;
const ISC_STATUS isc_dsql_auto_field_bad_pos          = 336397224L;
const ISC_STATUS isc_dsql_cte_wrong_reference         = 336397225L;
const ISC_STATUS isc_dsql_cte_cycle                   = 336397226L;
const ISC_STATUS isc_dsql_cte_outer_join              = 336397227L;
const ISC_STATUS isc_dsql_cte_mult_references         = 336397228L;
const ISC_STATUS isc_dsql_cte_not_a_union             = 336397229L;
const ISC_STATUS isc_dsql_cte_nonrecurs_after_recurs  = 336397230L;
const ISC_STATUS isc_dsql_cte_wrong_clause            = 336397231L;
const ISC_STATUS isc_dsql_cte_union_all               = 336397232L;
const ISC_STATUS isc_dsql_cte_miss_nonrecursive       = 336397233L;
const ISC_STATUS isc_dsql_cte_nested_with             = 336397234L;
const ISC_STATUS isc_dsql_col_more_than_once_using    = 336397235L;
const ISC_STATUS isc_dsql_unsupp_feature_dialect      = 336397236L;
const ISC_STATUS isc_dsql_cte_not_used                = 336397237L;
const ISC_STATUS isc_dsql_col_more_than_once_view     = 336397238L;
const ISC_STATUS isc_dsql_unsupported_in_auto_trans   = 336397239L;
const ISC_STATUS isc_dsql_eval_unknode                = 336397240L;
const ISC_STATUS isc_dsql_agg_wrongarg                = 336397241L;
const ISC_STATUS isc_dsql_agg2_wrongarg               = 336397242L;
const ISC_STATUS isc_dsql_nodateortime_pm_string      = 336397243L;
const ISC_STATUS isc_dsql_invalid_datetime_subtract   = 336397244L;
const ISC_STATUS isc_dsql_invalid_dateortime_add      = 336397245L;
const ISC_STATUS isc_dsql_invalid_type_minus_date     = 336397246L;
const ISC_STATUS isc_dsql_nostring_addsub_dial3       = 336397247L;
const ISC_STATUS isc_dsql_invalid_type_addsub_dial3   = 336397248L;
const ISC_STATUS isc_dsql_invalid_type_multip_dial1   = 336397249L;
const ISC_STATUS isc_dsql_nostring_multip_dial3       = 336397250L;
const ISC_STATUS isc_dsql_invalid_type_multip_dial3   = 336397251L;
const ISC_STATUS isc_dsql_mustuse_numeric_div_dial1   = 336397252L;
const ISC_STATUS isc_dsql_nostring_div_dial3          = 336397253L;
const ISC_STATUS isc_dsql_invalid_type_div_dial3      = 336397254L;
const ISC_STATUS isc_dsql_nostring_neg_dial3          = 336397255L;
const ISC_STATUS isc_dsql_invalid_type_neg            = 336397256L;
const ISC_STATUS isc_dsql_max_distinct_items          = 336397257L;
const ISC_STATUS isc_gsec_cant_open_db                = 336723983L;
const ISC_STATUS isc_gsec_switches_error              = 336723984L;
const ISC_STATUS isc_gsec_no_op_spec                  = 336723985L;
const ISC_STATUS isc_gsec_no_usr_name                 = 336723986L;
const ISC_STATUS isc_gsec_err_add                     = 336723987L;
const ISC_STATUS isc_gsec_err_modify                  = 336723988L;
const ISC_STATUS isc_gsec_err_find_mod                = 336723989L;
const ISC_STATUS isc_gsec_err_rec_not_found           = 336723990L;
const ISC_STATUS isc_gsec_err_delete                  = 336723991L;
const ISC_STATUS isc_gsec_err_find_del                = 336723992L;
const ISC_STATUS isc_gsec_err_find_disp               = 336723996L;
const ISC_STATUS isc_gsec_inv_param                   = 336723997L;
const ISC_STATUS isc_gsec_op_specified                = 336723998L;
const ISC_STATUS isc_gsec_pw_specified                = 336723999L;
const ISC_STATUS isc_gsec_uid_specified               = 336724000L;
const ISC_STATUS isc_gsec_gid_specified               = 336724001L;
const ISC_STATUS isc_gsec_proj_specified              = 336724002L;
const ISC_STATUS isc_gsec_org_specified               = 336724003L;
const ISC_STATUS isc_gsec_fname_specified             = 336724004L;
const ISC_STATUS isc_gsec_mname_specified             = 336724005L;
const ISC_STATUS isc_gsec_lname_specified             = 336724006L;
const ISC_STATUS isc_gsec_inv_switch                  = 336724008L;
const ISC_STATUS isc_gsec_amb_switch                  = 336724009L;
const ISC_STATUS isc_gsec_no_op_specified             = 336724010L;
const ISC_STATUS isc_gsec_params_not_allowed          = 336724011L;
const ISC_STATUS isc_gsec_incompat_switch             = 336724012L;
const ISC_STATUS isc_gsec_inv_username                = 336724044L;
const ISC_STATUS isc_gsec_inv_pw_length               = 336724045L;
const ISC_STATUS isc_gsec_db_specified                = 336724046L;
const ISC_STATUS isc_gsec_db_admin_specified          = 336724047L;
const ISC_STATUS isc_gsec_db_admin_pw_specified       = 336724048L;
const ISC_STATUS isc_gsec_sql_role_specified          = 336724049L;
const ISC_STATUS isc_license_no_file                  = 336789504L;
const ISC_STATUS isc_license_op_specified             = 336789523L;
const ISC_STATUS isc_license_op_missing               = 336789524L;
const ISC_STATUS isc_license_inv_switch               = 336789525L;
const ISC_STATUS isc_license_inv_switch_combo         = 336789526L;
const ISC_STATUS isc_license_inv_op_combo             = 336789527L;
const ISC_STATUS isc_license_amb_switch               = 336789528L;
const ISC_STATUS isc_license_inv_parameter            = 336789529L;
const ISC_STATUS isc_license_param_specified          = 336789530L;
const ISC_STATUS isc_license_param_req                = 336789531L;
const ISC_STATUS isc_license_syntx_error              = 336789532L;
const ISC_STATUS isc_license_dup_id                   = 336789534L;
const ISC_STATUS isc_license_inv_id_key               = 336789535L;
const ISC_STATUS isc_license_err_remove               = 336789536L;
const ISC_STATUS isc_license_err_update               = 336789537L;
const ISC_STATUS isc_license_err_convert              = 336789538L;
const ISC_STATUS isc_license_err_unk                  = 336789539L;
const ISC_STATUS isc_license_svc_err_add              = 336789540L;
const ISC_STATUS isc_license_svc_err_remove           = 336789541L;
const ISC_STATUS isc_license_eval_exists              = 336789563L;
const ISC_STATUS isc_gstat_unknown_switch             = 336920577L;
const ISC_STATUS isc_gstat_retry                      = 336920578L;
const ISC_STATUS isc_gstat_wrong_ods                  = 336920579L;
const ISC_STATUS isc_gstat_unexpected_eof             = 336920580L;
const ISC_STATUS isc_gstat_open_err                   = 336920605L;
const ISC_STATUS isc_gstat_read_err                   = 336920606L;
const ISC_STATUS isc_gstat_sysmemex                   = 336920607L;
const ISC_STATUS isc_fbsvcmgr_bad_am                  = 336986113L;
const ISC_STATUS isc_fbsvcmgr_bad_wm                  = 336986114L;
const ISC_STATUS isc_fbsvcmgr_bad_rs                  = 336986115L;
const ISC_STATUS isc_fbsvcmgr_info_err                = 336986116L;
const ISC_STATUS isc_fbsvcmgr_query_err               = 336986117L;
const ISC_STATUS isc_fbsvcmgr_switch_unknown          = 336986118L;
const ISC_STATUS isc_fbsvcmgr_bad_sm                  = 336986159L;
const ISC_STATUS isc_fbsvcmgr_fp_open                 = 336986160L;
const ISC_STATUS isc_fbsvcmgr_fp_read                 = 336986161L;
const ISC_STATUS isc_fbsvcmgr_fp_empty                = 336986162L;
const ISC_STATUS isc_fbsvcmgr_bad_arg                 = 336986164L;
const ISC_STATUS isc_utl_trusted_switch               = 337051649L;
const ISC_STATUS isc_err_max                          = 964;

#else /* c definitions */

#define isc_facility 20
#define isc_base 335544320L
#define isc_factor 1

#define isc_arg_end		0	/* end of argument list */
#define isc_arg_gds		1	/* generic DSRI status value */
#define isc_arg_string		2	/* string argument */
#define isc_arg_cstring	3	/* count & string argument */
#define isc_arg_number		4	/* numeric argument (long) */
#define isc_arg_interpreted	5	/* interpreted status code (string) */
#define isc_arg_vms		6	/* VAX/VMS status code (long) */
#define isc_arg_unix		7	/* UNIX error code */
#define isc_arg_domain		8	/* Apollo/Domain error code */
#define isc_arg_dos		9	/* MSDOS/OS2 error code */
#define isc_arg_mpexl		10	/* HP MPE/XL error code */
#define isc_arg_mpexl_ipc	11	/* HP MPE/XL IPC error code */
#define isc_arg_next_mach	15	/* NeXT/Mach error code */
#define isc_arg_netware	16	/* NetWare error code */
#define isc_arg_win32		17	/* Win32 error code */
#define isc_arg_warning	18	/* warning argument */
#define isc_arg_sql_state	19	/* SQLSTATE */

#define isc_arith_except                     335544321L
#define isc_bad_dbkey                        335544322L
#define isc_bad_db_format                    335544323L
#define isc_bad_db_handle                    335544324L
#define isc_bad_dpb_content                  335544325L
#define isc_bad_dpb_form                     335544326L
#define isc_bad_req_handle                   335544327L
#define isc_bad_segstr_handle                335544328L
#define isc_bad_segstr_id                    335544329L
#define isc_bad_tpb_content                  335544330L
#define isc_bad_tpb_form                     335544331L
#define isc_bad_trans_handle                 335544332L
#define isc_bug_check                        335544333L
#define isc_convert_error                    335544334L
#define isc_db_corrupt                       335544335L
#define isc_deadlock                         335544336L
#define isc_excess_trans                     335544337L
#define isc_from_no_match                    335544338L
#define isc_infinap                          335544339L
#define isc_infona                           335544340L
#define isc_infunk                           335544341L
#define isc_integ_fail                       335544342L
#define isc_invalid_blr                      335544343L
#define isc_io_error                         335544344L
#define isc_lock_conflict                    335544345L
#define isc_metadata_corrupt                 335544346L
#define isc_not_valid                        335544347L
#define isc_no_cur_rec                       335544348L
#define isc_no_dup                           335544349L
#define isc_no_finish                        335544350L
#define isc_no_meta_update                   335544351L
#define isc_no_priv                          335544352L
#define isc_no_recon                         335544353L
#define isc_no_record                        335544354L
#define isc_no_segstr_close                  335544355L
#define isc_obsolete_metadata                335544356L
#define isc_open_trans                       335544357L
#define isc_port_len                         335544358L
#define isc_read_only_field                  335544359L
#define isc_read_only_rel                    335544360L
#define isc_read_only_trans                  335544361L
#define isc_read_only_view                   335544362L
#define isc_req_no_trans                     335544363L
#define isc_req_sync                         335544364L
#define isc_req_wrong_db                     335544365L
#define isc_segment                          335544366L
#define isc_segstr_eof                       335544367L
#define isc_segstr_no_op                     335544368L
#define isc_segstr_no_read                   335544369L
#define isc_segstr_no_trans                  335544370L
#define isc_segstr_no_write                  335544371L
#define isc_segstr_wrong_db                  335544372L
#define isc_sys_request                      335544373L
#define isc_stream_eof                       335544374L
#define isc_unavailable                      335544375L
#define isc_unres_rel                        335544376L
#define isc_uns_ext                          335544377L
#define isc_wish_list                        335544378L
#define isc_wrong_ods                        335544379L
#define isc_wronumarg                        335544380L
#define isc_imp_exc                          335544381L
#define isc_random                           335544382L
#define isc_fatal_conflict                   335544383L
#define isc_badblk                           335544384L
#define isc_invpoolcl                        335544385L
#define isc_nopoolids                        335544386L
#define isc_relbadblk                        335544387L
#define isc_blktoobig                        335544388L
#define isc_bufexh                           335544389L
#define isc_syntaxerr                        335544390L
#define isc_bufinuse                         335544391L
#define isc_bdbincon                         335544392L
#define isc_reqinuse                         335544393L
#define isc_badodsver                        335544394L
#define isc_relnotdef                        335544395L
#define isc_fldnotdef                        335544396L
#define isc_dirtypage                        335544397L
#define isc_waifortra                        335544398L
#define isc_doubleloc                        335544399L
#define isc_nodnotfnd                        335544400L
#define isc_dupnodfnd                        335544401L
#define isc_locnotmar                        335544402L
#define isc_badpagtyp                        335544403L
#define isc_corrupt                          335544404L
#define isc_badpage                          335544405L
#define isc_badindex                         335544406L
#define isc_dbbnotzer                        335544407L
#define isc_tranotzer                        335544408L
#define isc_trareqmis                        335544409L
#define isc_badhndcnt                        335544410L
#define isc_wrotpbver                        335544411L
#define isc_wroblrver                        335544412L
#define isc_wrodpbver                        335544413L
#define isc_blobnotsup                       335544414L
#define isc_badrelation                      335544415L
#define isc_nodetach                         335544416L
#define isc_notremote                        335544417L
#define isc_trainlim                         335544418L
#define isc_notinlim                         335544419L
#define isc_traoutsta                        335544420L
#define isc_connect_reject                   335544421L
#define isc_dbfile                           335544422L
#define isc_orphan                           335544423L
#define isc_no_lock_mgr                      335544424L
#define isc_ctxinuse                         335544425L
#define isc_ctxnotdef                        335544426L
#define isc_datnotsup                        335544427L
#define isc_badmsgnum                        335544428L
#define isc_badparnum                        335544429L
#define isc_virmemexh                        335544430L
#define isc_blocking_signal                  335544431L
#define isc_lockmanerr                       335544432L
#define isc_journerr                         335544433L
#define isc_keytoobig                        335544434L
#define isc_nullsegkey                       335544435L
#define isc_sqlerr                           335544436L
#define isc_wrodynver                        335544437L
#define isc_funnotdef                        335544438L
#define isc_funmismat                        335544439L
#define isc_bad_msg_vec                      335544440L
#define isc_bad_detach                       335544441L
#define isc_noargacc_read                    335544442L
#define isc_noargacc_write                   335544443L
#define isc_read_only                        335544444L
#define isc_ext_err                          335544445L
#define isc_non_updatable                    335544446L
#define isc_no_rollback                      335544447L
#define isc_bad_sec_info                     335544448L
#define isc_invalid_sec_info                 335544449L
#define isc_misc_interpreted                 335544450L
#define isc_update_conflict                  335544451L
#define isc_unlicensed                       335544452L
#define isc_obj_in_use                       335544453L
#define isc_nofilter                         335544454L
#define isc_shadow_accessed                  335544455L
#define isc_invalid_sdl                      335544456L
#define isc_out_of_bounds                    335544457L
#define isc_invalid_dimension                335544458L
#define isc_rec_in_limbo                     335544459L
#define isc_shadow_missing                   335544460L
#define isc_cant_validate                    335544461L
#define isc_cant_start_journal               335544462L
#define isc_gennotdef                        335544463L
#define isc_cant_start_logging               335544464L
#define isc_bad_segstr_type                  335544465L
#define isc_foreign_key                      335544466L
#define isc_high_minor                       335544467L
#define isc_tra_state                        335544468L
#define isc_trans_invalid                    335544469L
#define isc_buf_invalid                      335544470L
#define isc_indexnotdefined                  335544471L
#define isc_login                            335544472L
#define isc_invalid_bookmark                 335544473L
#define isc_bad_lock_level                   335544474L
#define isc_relation_lock                    335544475L
#define isc_record_lock                      335544476L
#define isc_max_idx                          335544477L
#define isc_jrn_enable                       335544478L
#define isc_old_failure                      335544479L
#define isc_old_in_progress                  335544480L
#define isc_old_no_space                     335544481L
#define isc_no_wal_no_jrn                    335544482L
#define isc_num_old_files                    335544483L
#define isc_wal_file_open                    335544484L
#define isc_bad_stmt_handle                  335544485L
#define isc_wal_failure                      335544486L
#define isc_walw_err                         335544487L
#define isc_logh_small                       335544488L
#define isc_logh_inv_version                 335544489L
#define isc_logh_open_flag                   335544490L
#define isc_logh_open_flag2                  335544491L
#define isc_logh_diff_dbname                 335544492L
#define isc_logf_unexpected_eof              335544493L
#define isc_logr_incomplete                  335544494L
#define isc_logr_header_small                335544495L
#define isc_logb_small                       335544496L
#define isc_wal_illegal_attach               335544497L
#define isc_wal_invalid_wpb                  335544498L
#define isc_wal_err_rollover                 335544499L
#define isc_no_wal                           335544500L
#define isc_drop_wal                         335544501L
#define isc_stream_not_defined               335544502L
#define isc_wal_subsys_error                 335544503L
#define isc_wal_subsys_corrupt               335544504L
#define isc_no_archive                       335544505L
#define isc_shutinprog                       335544506L
#define isc_range_in_use                     335544507L
#define isc_range_not_found                  335544508L
#define isc_charset_not_found                335544509L
#define isc_lock_timeout                     335544510L
#define isc_prcnotdef                        335544511L
#define isc_prcmismat                        335544512L
#define isc_wal_bugcheck                     335544513L
#define isc_wal_cant_expand                  335544514L
#define isc_codnotdef                        335544515L
#define isc_xcpnotdef                        335544516L
#define isc_except                           335544517L
#define isc_cache_restart                    335544518L
#define isc_bad_lock_handle                  335544519L
#define isc_jrn_present                      335544520L
#define isc_wal_err_rollover2                335544521L
#define isc_wal_err_logwrite                 335544522L
#define isc_wal_err_jrn_comm                 335544523L
#define isc_wal_err_expansion                335544524L
#define isc_wal_err_setup                    335544525L
#define isc_wal_err_ww_sync                  335544526L
#define isc_wal_err_ww_start                 335544527L
#define isc_shutdown                         335544528L
#define isc_existing_priv_mod                335544529L
#define isc_primary_key_ref                  335544530L
#define isc_primary_key_notnull              335544531L
#define isc_ref_cnstrnt_notfound             335544532L
#define isc_foreign_key_notfound             335544533L
#define isc_ref_cnstrnt_update               335544534L
#define isc_check_cnstrnt_update             335544535L
#define isc_check_cnstrnt_del                335544536L
#define isc_integ_index_seg_del              335544537L
#define isc_integ_index_seg_mod              335544538L
#define isc_integ_index_del                  335544539L
#define isc_integ_index_mod                  335544540L
#define isc_check_trig_del                   335544541L
#define isc_check_trig_update                335544542L
#define isc_cnstrnt_fld_del                  335544543L
#define isc_cnstrnt_fld_rename               335544544L
#define isc_rel_cnstrnt_update               335544545L
#define isc_constaint_on_view                335544546L
#define isc_invld_cnstrnt_type               335544547L
#define isc_primary_key_exists               335544548L
#define isc_systrig_update                   335544549L
#define isc_not_rel_owner                    335544550L
#define isc_grant_obj_notfound               335544551L
#define isc_grant_fld_notfound               335544552L
#define isc_grant_nopriv                     335544553L
#define isc_nonsql_security_rel              335544554L
#define isc_nonsql_security_fld              335544555L
#define isc_wal_cache_err                    335544556L
#define isc_shutfail                         335544557L
#define isc_check_constraint                 335544558L
#define isc_bad_svc_handle                   335544559L
#define isc_shutwarn                         335544560L
#define isc_wrospbver                        335544561L
#define isc_bad_spb_form                     335544562L
#define isc_svcnotdef                        335544563L
#define isc_no_jrn                           335544564L
#define isc_transliteration_failed           335544565L
#define isc_start_cm_for_wal                 335544566L
#define isc_wal_ovflow_log_required          335544567L
#define isc_text_subtype                     335544568L
#define isc_dsql_error                       335544569L
#define isc_dsql_command_err                 335544570L
#define isc_dsql_constant_err                335544571L
#define isc_dsql_cursor_err                  335544572L
#define isc_dsql_datatype_err                335544573L
#define isc_dsql_decl_err                    335544574L
#define isc_dsql_cursor_update_err           335544575L
#define isc_dsql_cursor_open_err             335544576L
#define isc_dsql_cursor_close_err            335544577L
#define isc_dsql_field_err                   335544578L
#define isc_dsql_internal_err                335544579L
#define isc_dsql_relation_err                335544580L
#define isc_dsql_procedure_err               335544581L
#define isc_dsql_request_err                 335544582L
#define isc_dsql_sqlda_err                   335544583L
#define isc_dsql_var_count_err               335544584L
#define isc_dsql_stmt_handle                 335544585L
#define isc_dsql_function_err                335544586L
#define isc_dsql_blob_err                    335544587L
#define isc_collation_not_found              335544588L
#define isc_collation_not_for_charset        335544589L
#define isc_dsql_dup_option                  335544590L
#define isc_dsql_tran_err                    335544591L
#define isc_dsql_invalid_array               335544592L
#define isc_dsql_max_arr_dim_exceeded        335544593L
#define isc_dsql_arr_range_error             335544594L
#define isc_dsql_trigger_err                 335544595L
#define isc_dsql_subselect_err               335544596L
#define isc_dsql_crdb_prepare_err            335544597L
#define isc_specify_field_err                335544598L
#define isc_num_field_err                    335544599L
#define isc_col_name_err                     335544600L
#define isc_where_err                        335544601L
#define isc_table_view_err                   335544602L
#define isc_distinct_err                     335544603L
#define isc_key_field_count_err              335544604L
#define isc_subquery_err                     335544605L
#define isc_expression_eval_err              335544606L
#define isc_node_err                         335544607L
#define isc_command_end_err                  335544608L
#define isc_index_name                       335544609L
#define isc_exception_name                   335544610L
#define isc_field_name                       335544611L
#define isc_token_err                        335544612L
#define isc_union_err                        335544613L
#define isc_dsql_construct_err               335544614L
#define isc_field_aggregate_err              335544615L
#define isc_field_ref_err                    335544616L
#define isc_order_by_err                     335544617L
#define isc_return_mode_err                  335544618L
#define isc_extern_func_err                  335544619L
#define isc_alias_conflict_err               335544620L
#define isc_procedure_conflict_error         335544621L
#define isc_relation_conflict_err            335544622L
#define isc_dsql_domain_err                  335544623L
#define isc_idx_seg_err                      335544624L
#define isc_node_name_err                    335544625L
#define isc_table_name                       335544626L
#define isc_proc_name                        335544627L
#define isc_idx_create_err                   335544628L
#define isc_wal_shadow_err                   335544629L
#define isc_dependency                       335544630L
#define isc_idx_key_err                      335544631L
#define isc_dsql_file_length_err             335544632L
#define isc_dsql_shadow_number_err           335544633L
#define isc_dsql_token_unk_err               335544634L
#define isc_dsql_no_relation_alias           335544635L
#define isc_indexname                        335544636L
#define isc_no_stream_plan                   335544637L
#define isc_stream_twice                     335544638L
#define isc_stream_not_found                 335544639L
#define isc_collation_requires_text          335544640L
#define isc_dsql_domain_not_found            335544641L
#define isc_index_unused                     335544642L
#define isc_dsql_self_join                   335544643L
#define isc_stream_bof                       335544644L
#define isc_stream_crack                     335544645L
#define isc_db_or_file_exists                335544646L
#define isc_invalid_operator                 335544647L
#define isc_conn_lost                        335544648L
#define isc_bad_checksum                     335544649L
#define isc_page_type_err                    335544650L
#define isc_ext_readonly_err                 335544651L
#define isc_sing_select_err                  335544652L
#define isc_psw_attach                       335544653L
#define isc_psw_start_trans                  335544654L
#define isc_invalid_direction                335544655L
#define isc_dsql_var_conflict                335544656L
#define isc_dsql_no_blob_array               335544657L
#define isc_dsql_base_table                  335544658L
#define isc_duplicate_base_table             335544659L
#define isc_view_alias                       335544660L
#define isc_index_root_page_full             335544661L
#define isc_dsql_blob_type_unknown           335544662L
#define isc_req_max_clones_exceeded          335544663L
#define isc_dsql_duplicate_spec              335544664L
#define isc_unique_key_violation             335544665L
#define isc_srvr_version_too_old             335544666L
#define isc_drdb_completed_with_errs         335544667L
#define isc_dsql_procedure_use_err           335544668L
#define isc_dsql_count_mismatch              335544669L
#define isc_blob_idx_err                     335544670L
#define isc_array_idx_err                    335544671L
#define isc_key_field_err                    335544672L
#define isc_no_delete                        335544673L
#define isc_del_last_field                   335544674L
#define isc_sort_err                         335544675L
#define isc_sort_mem_err                     335544676L
#define isc_version_err                      335544677L
#define isc_inval_key_posn                   335544678L
#define isc_no_segments_err                  335544679L
#define isc_crrp_data_err                    335544680L
#define isc_rec_size_err                     335544681L
#define isc_dsql_field_ref                   335544682L
#define isc_req_depth_exceeded               335544683L
#define isc_no_field_access                  335544684L
#define isc_no_dbkey                         335544685L
#define isc_jrn_format_err                   335544686L
#define isc_jrn_file_full                    335544687L
#define isc_dsql_open_cursor_request         335544688L
#define isc_ib_error                         335544689L
#define isc_cache_redef                      335544690L
#define isc_cache_too_small                  335544691L
#define isc_log_redef                        335544692L
#define isc_log_too_small                    335544693L
#define isc_partition_too_small              335544694L
#define isc_partition_not_supp               335544695L
#define isc_log_length_spec                  335544696L
#define isc_precision_err                    335544697L
#define isc_scale_nogt                       335544698L
#define isc_expec_short                      335544699L
#define isc_expec_long                       335544700L
#define isc_expec_ushort                     335544701L
#define isc_escape_invalid                   335544702L
#define isc_svcnoexe                         335544703L
#define isc_net_lookup_err                   335544704L
#define isc_service_unknown                  335544705L
#define isc_host_unknown                     335544706L
#define isc_grant_nopriv_on_base             335544707L
#define isc_dyn_fld_ambiguous                335544708L
#define isc_dsql_agg_ref_err                 335544709L
#define isc_complex_view                     335544710L
#define isc_unprepared_stmt                  335544711L
#define isc_expec_positive                   335544712L
#define isc_dsql_sqlda_value_err             335544713L
#define isc_invalid_array_id                 335544714L
#define isc_extfile_uns_op                   335544715L
#define isc_svc_in_use                       335544716L
#define isc_err_stack_limit                  335544717L
#define isc_invalid_key                      335544718L
#define isc_net_init_error                   335544719L
#define isc_loadlib_failure                  335544720L
#define isc_network_error                    335544721L
#define isc_net_connect_err                  335544722L
#define isc_net_connect_listen_err           335544723L
#define isc_net_event_connect_err            335544724L
#define isc_net_event_listen_err             335544725L
#define isc_net_read_err                     335544726L
#define isc_net_write_err                    335544727L
#define isc_integ_index_deactivate           335544728L
#define isc_integ_deactivate_primary         335544729L
#define isc_cse_not_supported                335544730L
#define isc_tra_must_sweep                   335544731L
#define isc_unsupported_network_drive        335544732L
#define isc_io_create_err                    335544733L
#define isc_io_open_err                      335544734L
#define isc_io_close_err                     335544735L
#define isc_io_read_err                      335544736L
#define isc_io_write_err                     335544737L
#define isc_io_delete_err                    335544738L
#define isc_io_access_err                    335544739L
#define isc_udf_exception                    335544740L
#define isc_lost_db_connection               335544741L
#define isc_no_write_user_priv               335544742L
#define isc_token_too_long                   335544743L
#define isc_max_att_exceeded                 335544744L
#define isc_login_same_as_role_name          335544745L
#define isc_reftable_requires_pk             335544746L
#define isc_usrname_too_long                 335544747L
#define isc_password_too_long                335544748L
#define isc_usrname_required                 335544749L
#define isc_password_required                335544750L
#define isc_bad_protocol                     335544751L
#define isc_dup_usrname_found                335544752L
#define isc_usrname_not_found                335544753L
#define isc_error_adding_sec_record          335544754L
#define isc_error_modifying_sec_record       335544755L
#define isc_error_deleting_sec_record        335544756L
#define isc_error_updating_sec_db            335544757L
#define isc_sort_rec_size_err                335544758L
#define isc_bad_default_value                335544759L
#define isc_invalid_clause                   335544760L
#define isc_too_many_handles                 335544761L
#define isc_optimizer_blk_exc                335544762L
#define isc_invalid_string_constant          335544763L
#define isc_transitional_date                335544764L
#define isc_read_only_database               335544765L
#define isc_must_be_dialect_2_and_up         335544766L
#define isc_blob_filter_exception            335544767L
#define isc_exception_access_violation       335544768L
#define isc_exception_datatype_missalignment 335544769L
#define isc_exception_array_bounds_exceeded  335544770L
#define isc_exception_float_denormal_operand 335544771L
#define isc_exception_float_divide_by_zero   335544772L
#define isc_exception_float_inexact_result   335544773L
#define isc_exception_float_invalid_operand  335544774L
#define isc_exception_float_overflow         335544775L
#define isc_exception_float_stack_check      335544776L
#define isc_exception_float_underflow        335544777L
#define isc_exception_integer_divide_by_zero 335544778L
#define isc_exception_integer_overflow       335544779L
#define isc_exception_unknown                335544780L
#define isc_exception_stack_overflow         335544781L
#define isc_exception_sigsegv                335544782L
#define isc_exception_sigill                 335544783L
#define isc_exception_sigbus                 335544784L
#define isc_exception_sigfpe                 335544785L
#define isc_ext_file_delete                  335544786L
#define isc_ext_file_modify                  335544787L
#define isc_adm_task_denied                  335544788L
#define isc_extract_input_mismatch           335544789L
#define isc_insufficient_svc_privileges      335544790L
#define isc_file_in_use                      335544791L
#define isc_service_att_err                  335544792L
#define isc_ddl_not_allowed_by_db_sql_dial   335544793L
#define isc_cancelled                        335544794L
#define isc_unexp_spb_form                   335544795L
#define isc_sql_dialect_datatype_unsupport   335544796L
#define isc_svcnouser                        335544797L
#define isc_depend_on_uncommitted_rel        335544798L
#define isc_svc_name_missing                 335544799L
#define isc_too_many_contexts                335544800L
#define isc_datype_notsup                    335544801L
#define isc_dialect_reset_warning            335544802L
#define isc_dialect_not_changed              335544803L
#define isc_database_create_failed           335544804L
#define isc_inv_dialect_specified            335544805L
#define isc_valid_db_dialects                335544806L
#define isc_sqlwarn                          335544807L
#define isc_dtype_renamed                    335544808L
#define isc_extern_func_dir_error            335544809L
#define isc_date_range_exceeded              335544810L
#define isc_inv_client_dialect_specified     335544811L
#define isc_valid_client_dialects            335544812L
#define isc_optimizer_between_err            335544813L
#define isc_service_not_supported            335544814L
#define isc_generator_name                   335544815L
#define isc_udf_name                         335544816L
#define isc_bad_limit_param                  335544817L
#define isc_bad_skip_param                   335544818L
#define isc_io_32bit_exceeded_err            335544819L
#define isc_invalid_savepoint                335544820L
#define isc_dsql_column_pos_err              335544821L
#define isc_dsql_agg_where_err               335544822L
#define isc_dsql_agg_group_err               335544823L
#define isc_dsql_agg_column_err              335544824L
#define isc_dsql_agg_having_err              335544825L
#define isc_dsql_agg_nested_err              335544826L
#define isc_exec_sql_invalid_arg             335544827L
#define isc_exec_sql_invalid_req             335544828L
#define isc_exec_sql_invalid_var             335544829L
#define isc_exec_sql_max_call_exceeded       335544830L
#define isc_conf_access_denied               335544831L
#define isc_wrong_backup_state               335544832L
#define isc_wal_backup_err                   335544833L
#define isc_cursor_not_open                  335544834L
#define isc_bad_shutdown_mode                335544835L
#define isc_concat_overflow                  335544836L
#define isc_bad_substring_offset             335544837L
#define isc_foreign_key_target_doesnt_exist  335544838L
#define isc_foreign_key_references_present   335544839L
#define isc_no_update                        335544840L
#define isc_cursor_already_open              335544841L
#define isc_stack_trace                      335544842L
#define isc_ctx_var_not_found                335544843L
#define isc_ctx_namespace_invalid            335544844L
#define isc_ctx_too_big                      335544845L
#define isc_ctx_bad_argument                 335544846L
#define isc_identifier_too_long              335544847L
#define isc_except2                          335544848L
#define isc_malformed_string                 335544849L
#define isc_prc_out_param_mismatch           335544850L
#define isc_command_end_err2                 335544851L
#define isc_partner_idx_incompat_type        335544852L
#define isc_bad_substring_length             335544853L
#define isc_charset_not_installed            335544854L
#define isc_collation_not_installed          335544855L
#define isc_att_shutdown                     335544856L
#define isc_blobtoobig                       335544857L
#define isc_must_have_phys_field             335544858L
#define isc_invalid_time_precision           335544859L
#define isc_blob_convert_error               335544860L
#define isc_array_convert_error              335544861L
#define isc_record_lock_not_supp             335544862L
#define isc_partner_idx_not_found            335544863L
#define isc_tra_num_exc                      335544864L
#define isc_field_disappeared                335544865L
#define isc_met_wrong_gtt_scope              335544866L
#define isc_subtype_for_internal_use         335544867L
#define isc_illegal_prc_type                 335544868L
#define isc_invalid_sort_datatype            335544869L
#define isc_collation_name                   335544870L
#define isc_domain_name                      335544871L
#define isc_domnotdef                        335544872L
#define isc_array_max_dimensions             335544873L
#define isc_max_db_per_trans_allowed         335544874L
#define isc_bad_debug_format                 335544875L
#define isc_bad_proc_BLR                     335544876L
#define isc_key_too_big                      335544877L
#define isc_concurrent_transaction           335544878L
#define isc_not_valid_for_var                335544879L
#define isc_not_valid_for                    335544880L
#define isc_need_difference                  335544881L
#define isc_long_login                       335544882L
#define isc_fldnotdef2                       335544883L
#define isc_invalid_similar_pattern          335544884L
#define isc_bad_teb_form                     335544885L
#define isc_tpb_multiple_txn_isolation       335544886L
#define isc_tpb_reserv_before_table          335544887L
#define isc_tpb_multiple_spec                335544888L
#define isc_tpb_option_without_rc            335544889L
#define isc_tpb_conflicting_options          335544890L
#define isc_tpb_reserv_missing_tlen          335544891L
#define isc_tpb_reserv_long_tlen             335544892L
#define isc_tpb_reserv_missing_tname         335544893L
#define isc_tpb_reserv_corrup_tlen           335544894L
#define isc_tpb_reserv_null_tlen             335544895L
#define isc_tpb_reserv_relnotfound           335544896L
#define isc_tpb_reserv_baserelnotfound       335544897L
#define isc_tpb_missing_len                  335544898L
#define isc_tpb_missing_value                335544899L
#define isc_tpb_corrupt_len                  335544900L
#define isc_tpb_null_len                     335544901L
#define isc_tpb_overflow_len                 335544902L
#define isc_tpb_invalid_value                335544903L
#define isc_tpb_reserv_stronger_wng          335544904L
#define isc_tpb_reserv_stronger              335544905L
#define isc_tpb_reserv_max_recursion         335544906L
#define isc_tpb_reserv_virtualtbl            335544907L
#define isc_tpb_reserv_systbl                335544908L
#define isc_tpb_reserv_temptbl               335544909L
#define isc_tpb_readtxn_after_writelock      335544910L
#define isc_tpb_writelock_after_readtxn      335544911L
#define isc_time_range_exceeded              335544912L
#define isc_datetime_range_exceeded          335544913L
#define isc_string_truncation                335544914L
#define isc_blob_truncation                  335544915L
#define isc_numeric_out_of_range             335544916L
#define isc_shutdown_timeout                 335544917L
#define isc_att_handle_busy                  335544918L
#define isc_bad_udf_freeit                   335544919L
#define isc_eds_provider_not_found           335544920L
#define isc_eds_connection                   335544921L
#define isc_eds_preprocess                   335544922L
#define isc_eds_stmt_expected                335544923L
#define isc_eds_prm_name_expected            335544924L
#define isc_eds_unclosed_comment             335544925L
#define isc_eds_statement                    335544926L
#define isc_eds_input_prm_mismatch           335544927L
#define isc_eds_output_prm_mismatch          335544928L
#define isc_eds_input_prm_not_set            335544929L
#define isc_too_big_blr                      335544930L
#define isc_montabexh                        335544931L
#define isc_modnotfound                      335544932L
#define isc_nothing_to_cancel                335544933L
#define isc_ibutil_not_loaded                335544934L
#define isc_circular_computed                335544935L
#define isc_psw_db_error                     335544936L
#define isc_invalid_type_datetime_op         335544937L
#define isc_onlycan_add_timetodate           335544938L
#define isc_onlycan_add_datetotime           335544939L
#define isc_onlycansub_tstampfromtstamp      335544940L
#define isc_onlyoneop_mustbe_tstamp          335544941L
#define isc_invalid_extractpart_time         335544942L
#define isc_invalid_extractpart_date         335544943L
#define isc_invalidarg_extract               335544944L
#define isc_sysf_argmustbe_exact             335544945L
#define isc_sysf_argmustbe_exact_or_fp       335544946L
#define isc_sysf_argviolates_uuidtype        335544947L
#define isc_sysf_argviolates_uuidlen         335544948L
#define isc_sysf_argviolates_uuidfmt         335544949L
#define isc_sysf_argviolates_guidigits       335544950L
#define isc_sysf_invalid_addpart_time        335544951L
#define isc_sysf_invalid_add_datetime        335544952L
#define isc_sysf_invalid_addpart_dtime       335544953L
#define isc_sysf_invalid_add_dtime_rc        335544954L
#define isc_sysf_invalid_diff_dtime          335544955L
#define isc_sysf_invalid_timediff            335544956L
#define isc_sysf_invalid_tstamptimediff      335544957L
#define isc_sysf_invalid_datetimediff        335544958L
#define isc_sysf_invalid_diffpart            335544959L
#define isc_sysf_argmustbe_positive          335544960L
#define isc_sysf_basemustbe_positive         335544961L
#define isc_sysf_argnmustbe_nonneg           335544962L
#define isc_sysf_argnmustbe_positive         335544963L
#define isc_sysf_invalid_zeropowneg          335544964L
#define isc_sysf_invalid_negpowfp            335544965L
#define isc_sysf_invalid_scale               335544966L
#define isc_sysf_argmustbe_nonneg            335544967L
#define isc_sysf_binuuid_mustbe_str          335544968L
#define isc_sysf_binuuid_wrongsize           335544969L
#define isc_missing_required_spb             335544970L
#define isc_net_server_shutdown              335544971L
#define isc_bad_conn_str                     335544972L
#define isc_bad_epb_form                     335544973L
#define isc_no_threads                       335544974L
#define isc_net_event_connect_timeout        335544975L
#define isc_sysf_argmustbe_nonzero           335544976L
#define isc_sysf_argmustbe_range_inc1_1      335544977L
#define isc_sysf_argmustbe_gteq_one          335544978L
#define isc_sysf_argmustbe_range_exc1_1      335544979L
#define isc_internal_rejected_params         335544980L
#define isc_sysf_fp_overflow                 335544981L
#define isc_udf_fp_overflow                  335544982L
#define isc_udf_fp_nan                       335544983L
#define isc_instance_conflict                335544984L
#define isc_out_of_temp_space                335544985L
#define isc_eds_expl_tran_ctrl               335544986L
#define isc_no_trusted_spb                   335544987L
#define isc_async_active                     335545017L
#define isc_gfix_db_name                     335740929L
#define isc_gfix_invalid_sw                  335740930L
#define isc_gfix_incmp_sw                    335740932L
#define isc_gfix_replay_req                  335740933L
#define isc_gfix_pgbuf_req                   335740934L
#define isc_gfix_val_req                     335740935L
#define isc_gfix_pval_req                    335740936L
#define isc_gfix_trn_req                     335740937L
#define isc_gfix_full_req                    335740940L
#define isc_gfix_usrname_req                 335740941L
#define isc_gfix_pass_req                    335740942L
#define isc_gfix_subs_name                   335740943L
#define isc_gfix_wal_req                     335740944L
#define isc_gfix_sec_req                     335740945L
#define isc_gfix_nval_req                    335740946L
#define isc_gfix_type_shut                   335740947L
#define isc_gfix_retry                       335740948L
#define isc_gfix_retry_db                    335740951L
#define isc_gfix_exceed_max                  335740991L
#define isc_gfix_corrupt_pool                335740992L
#define isc_gfix_mem_exhausted               335740993L
#define isc_gfix_bad_pool                    335740994L
#define isc_gfix_trn_not_valid               335740995L
#define isc_gfix_unexp_eoi                   335741012L
#define isc_gfix_recon_fail                  335741018L
#define isc_gfix_trn_unknown                 335741036L
#define isc_gfix_mode_req                    335741038L
#define isc_gfix_pzval_req                   335741042L
#define isc_dsql_dbkey_from_non_table        336003074L
#define isc_dsql_transitional_numeric        336003075L
#define isc_dsql_dialect_warning_expr        336003076L
#define isc_sql_db_dialect_dtype_unsupport   336003077L
#define isc_isc_sql_dialect_conflict_num     336003079L
#define isc_dsql_warning_number_ambiguous    336003080L
#define isc_dsql_warning_number_ambiguous1   336003081L
#define isc_dsql_warn_precision_ambiguous    336003082L
#define isc_dsql_warn_precision_ambiguous1   336003083L
#define isc_dsql_warn_precision_ambiguous2   336003084L
#define isc_dsql_ambiguous_field_name        336003085L
#define isc_dsql_udf_return_pos_err          336003086L
#define isc_dsql_invalid_label               336003087L
#define isc_dsql_datatypes_not_comparable    336003088L
#define isc_dsql_cursor_invalid              336003089L
#define isc_dsql_cursor_redefined            336003090L
#define isc_dsql_cursor_not_found            336003091L
#define isc_dsql_cursor_exists               336003092L
#define isc_dsql_cursor_rel_ambiguous        336003093L
#define isc_dsql_cursor_rel_not_found        336003094L
#define isc_dsql_cursor_not_open             336003095L
#define isc_dsql_type_not_supp_ext_tab       336003096L
#define isc_dsql_feature_not_supported_ods   336003097L
#define isc_primary_key_required             336003098L
#define isc_upd_ins_doesnt_match_pk          336003099L
#define isc_upd_ins_doesnt_match_matching    336003100L
#define isc_upd_ins_with_complex_view        336003101L
#define isc_dsql_incompatible_trigger_type   336003102L
#define isc_dsql_db_trigger_type_cant_change 336003103L
#define isc_dyn_dup_table                    336068740L
#define isc_dyn_column_does_not_exist        336068784L
#define isc_dyn_role_does_not_exist          336068796L
#define isc_dyn_no_grant_admin_opt           336068797L
#define isc_dyn_user_not_role_member         336068798L
#define isc_dyn_delete_role_failed           336068799L
#define isc_dyn_grant_role_to_user           336068800L
#define isc_dyn_inv_sql_role_name            336068801L
#define isc_dyn_dup_sql_role                 336068802L
#define isc_dyn_kywd_spec_for_role           336068803L
#define isc_dyn_roles_not_supported          336068804L
#define isc_dyn_domain_name_exists           336068812L
#define isc_dyn_field_name_exists            336068813L
#define isc_dyn_dependency_exists            336068814L
#define isc_dyn_dtype_invalid                336068815L
#define isc_dyn_char_fld_too_small           336068816L
#define isc_dyn_invalid_dtype_conversion     336068817L
#define isc_dyn_dtype_conv_invalid           336068818L
#define isc_dyn_zero_len_id                  336068820L
#define isc_max_coll_per_charset             336068829L
#define isc_invalid_coll_attr                336068830L
#define isc_dyn_wrong_gtt_scope              336068840L
#define isc_dyn_scale_too_big                336068852L
#define isc_dyn_precision_too_small          336068853L
#define isc_dyn_miss_priv_warning            336068855L
#define isc_dyn_ods_not_supp_feature         336068856L
#define isc_dyn_cannot_addrem_computed       336068857L
#define isc_dyn_no_empty_pw                  336068858L
#define isc_dyn_dup_index                    336068859L
#define isc_gbak_unknown_switch              336330753L
#define isc_gbak_page_size_missing           336330754L
#define isc_gbak_page_size_toobig            336330755L
#define isc_gbak_redir_ouput_missing         336330756L
#define isc_gbak_switches_conflict           336330757L
#define isc_gbak_unknown_device              336330758L
#define isc_gbak_no_protection               336330759L
#define isc_gbak_page_size_not_allowed       336330760L
#define isc_gbak_multi_source_dest           336330761L
#define isc_gbak_filename_missing            336330762L
#define isc_gbak_dup_inout_names             336330763L
#define isc_gbak_inv_page_size               336330764L
#define isc_gbak_db_specified                336330765L
#define isc_gbak_db_exists                   336330766L
#define isc_gbak_unk_device                  336330767L
#define isc_gbak_blob_info_failed            336330772L
#define isc_gbak_unk_blob_item               336330773L
#define isc_gbak_get_seg_failed              336330774L
#define isc_gbak_close_blob_failed           336330775L
#define isc_gbak_open_blob_failed            336330776L
#define isc_gbak_put_blr_gen_id_failed       336330777L
#define isc_gbak_unk_type                    336330778L
#define isc_gbak_comp_req_failed             336330779L
#define isc_gbak_start_req_failed            336330780L
#define isc_gbak_rec_failed                  336330781L
#define isc_gbak_rel_req_failed              336330782L
#define isc_gbak_db_info_failed              336330783L
#define isc_gbak_no_db_desc                  336330784L
#define isc_gbak_db_create_failed            336330785L
#define isc_gbak_decomp_len_error            336330786L
#define isc_gbak_tbl_missing                 336330787L
#define isc_gbak_blob_col_missing            336330788L
#define isc_gbak_create_blob_failed          336330789L
#define isc_gbak_put_seg_failed              336330790L
#define isc_gbak_rec_len_exp                 336330791L
#define isc_gbak_inv_rec_len                 336330792L
#define isc_gbak_exp_data_type               336330793L
#define isc_gbak_gen_id_failed               336330794L
#define isc_gbak_unk_rec_type                336330795L
#define isc_gbak_inv_bkup_ver                336330796L
#define isc_gbak_missing_bkup_desc           336330797L
#define isc_gbak_string_trunc                336330798L
#define isc_gbak_cant_rest_record            336330799L
#define isc_gbak_send_failed                 336330800L
#define isc_gbak_no_tbl_name                 336330801L
#define isc_gbak_unexp_eof                   336330802L
#define isc_gbak_db_format_too_old           336330803L
#define isc_gbak_inv_array_dim               336330804L
#define isc_gbak_xdr_len_expected            336330807L
#define isc_gbak_open_bkup_error             336330817L
#define isc_gbak_open_error                  336330818L
#define isc_gbak_missing_block_fac           336330934L
#define isc_gbak_inv_block_fac               336330935L
#define isc_gbak_block_fac_specified         336330936L
#define isc_gbak_missing_username            336330940L
#define isc_gbak_missing_password            336330941L
#define isc_gbak_missing_skipped_bytes       336330952L
#define isc_gbak_inv_skipped_bytes           336330953L
#define isc_gbak_err_restore_charset         336330965L
#define isc_gbak_err_restore_collation       336330967L
#define isc_gbak_read_error                  336330972L
#define isc_gbak_write_error                 336330973L
#define isc_gbak_db_in_use                   336330985L
#define isc_gbak_sysmemex                    336330990L
#define isc_gbak_restore_role_failed         336331002L
#define isc_gbak_role_op_missing             336331005L
#define isc_gbak_page_buffers_missing        336331010L
#define isc_gbak_page_buffers_wrong_param    336331011L
#define isc_gbak_page_buffers_restore        336331012L
#define isc_gbak_inv_size                    336331014L
#define isc_gbak_file_outof_sequence         336331015L
#define isc_gbak_join_file_missing           336331016L
#define isc_gbak_stdin_not_supptd            336331017L
#define isc_gbak_stdout_not_supptd           336331018L
#define isc_gbak_bkup_corrupt                336331019L
#define isc_gbak_unk_db_file_spec            336331020L
#define isc_gbak_hdr_write_failed            336331021L
#define isc_gbak_disk_space_ex               336331022L
#define isc_gbak_size_lt_min                 336331023L
#define isc_gbak_svc_name_missing            336331025L
#define isc_gbak_not_ownr                    336331026L
#define isc_gbak_mode_req                    336331031L
#define isc_gbak_just_data                   336331033L
#define isc_gbak_data_only                   336331034L
#define isc_gbak_invalid_metadata            336331093L
#define isc_gbak_invalid_data                336331094L
#define isc_dsql_too_old_ods                 336397205L
#define isc_dsql_table_not_found             336397206L
#define isc_dsql_view_not_found              336397207L
#define isc_dsql_line_col_error              336397208L
#define isc_dsql_unknown_pos                 336397209L
#define isc_dsql_no_dup_name                 336397210L
#define isc_dsql_too_many_values             336397211L
#define isc_dsql_no_array_computed           336397212L
#define isc_dsql_implicit_domain_name        336397213L
#define isc_dsql_only_can_subscript_array    336397214L
#define isc_dsql_max_sort_items              336397215L
#define isc_dsql_max_group_items             336397216L
#define isc_dsql_conflicting_sort_field      336397217L
#define isc_dsql_derived_table_more_columns  336397218L
#define isc_dsql_derived_table_less_columns  336397219L
#define isc_dsql_derived_field_unnamed       336397220L
#define isc_dsql_derived_field_dup_name      336397221L
#define isc_dsql_derived_alias_select        336397222L
#define isc_dsql_derived_alias_field         336397223L
#define isc_dsql_auto_field_bad_pos          336397224L
#define isc_dsql_cte_wrong_reference         336397225L
#define isc_dsql_cte_cycle                   336397226L
#define isc_dsql_cte_outer_join              336397227L
#define isc_dsql_cte_mult_references         336397228L
#define isc_dsql_cte_not_a_union             336397229L
#define isc_dsql_cte_nonrecurs_after_recurs  336397230L
#define isc_dsql_cte_wrong_clause            336397231L
#define isc_dsql_cte_union_all               336397232L
#define isc_dsql_cte_miss_nonrecursive       336397233L
#define isc_dsql_cte_nested_with             336397234L
#define isc_dsql_col_more_than_once_using    336397235L
#define isc_dsql_unsupp_feature_dialect      336397236L
#define isc_dsql_cte_not_used                336397237L
#define isc_dsql_col_more_than_once_view     336397238L
#define isc_dsql_unsupported_in_auto_trans   336397239L
#define isc_dsql_eval_unknode                336397240L
#define isc_dsql_agg_wrongarg                336397241L
#define isc_dsql_agg2_wrongarg               336397242L
#define isc_dsql_nodateortime_pm_string      336397243L
#define isc_dsql_invalid_datetime_subtract   336397244L
#define isc_dsql_invalid_dateortime_add      336397245L
#define isc_dsql_invalid_type_minus_date     336397246L
#define isc_dsql_nostring_addsub_dial3       336397247L
#define isc_dsql_invalid_type_addsub_dial3   336397248L
#define isc_dsql_invalid_type_multip_dial1   336397249L
#define isc_dsql_nostring_multip_dial3       336397250L
#define isc_dsql_invalid_type_multip_dial3   336397251L
#define isc_dsql_mustuse_numeric_div_dial1   336397252L
#define isc_dsql_nostring_div_dial3          336397253L
#define isc_dsql_invalid_type_div_dial3      336397254L
#define isc_dsql_nostring_neg_dial3          336397255L
#define isc_dsql_invalid_type_neg            336397256L
#define isc_dsql_max_distinct_items          336397257L
#define isc_gsec_cant_open_db                336723983L
#define isc_gsec_switches_error              336723984L
#define isc_gsec_no_op_spec                  336723985L
#define isc_gsec_no_usr_name                 336723986L
#define isc_gsec_err_add                     336723987L
#define isc_gsec_err_modify                  336723988L
#define isc_gsec_err_find_mod                336723989L
#define isc_gsec_err_rec_not_found           336723990L
#define isc_gsec_err_delete                  336723991L
#define isc_gsec_err_find_del                336723992L
#define isc_gsec_err_find_disp               336723996L
#define isc_gsec_inv_param                   336723997L
#define isc_gsec_op_specified                336723998L
#define isc_gsec_pw_specified                336723999L
#define isc_gsec_uid_specified               336724000L
#define isc_gsec_gid_specified               336724001L
#define isc_gsec_proj_specified              336724002L
#define isc_gsec_org_specified               336724003L
#define isc_gsec_fname_specified             336724004L
#define isc_gsec_mname_specified             336724005L
#define isc_gsec_lname_specified             336724006L
#define isc_gsec_inv_switch                  336724008L
#define isc_gsec_amb_switch                  336724009L
#define isc_gsec_no_op_specified             336724010L
#define isc_gsec_params_not_allowed          336724011L
#define isc_gsec_incompat_switch             336724012L
#define isc_gsec_inv_username                336724044L
#define isc_gsec_inv_pw_length               336724045L
#define isc_gsec_db_specified                336724046L
#define isc_gsec_db_admin_specified          336724047L
#define isc_gsec_db_admin_pw_specified       336724048L
#define isc_gsec_sql_role_specified          336724049L
#define isc_license_no_file                  336789504L
#define isc_license_op_specified             336789523L
#define isc_license_op_missing               336789524L
#define isc_license_inv_switch               336789525L
#define isc_license_inv_switch_combo         336789526L
#define isc_license_inv_op_combo             336789527L
#define isc_license_amb_switch               336789528L
#define isc_license_inv_parameter            336789529L
#define isc_license_param_specified          336789530L
#define isc_license_param_req                336789531L
#define isc_license_syntx_error              336789532L
#define isc_license_dup_id                   336789534L
#define isc_license_inv_id_key               336789535L
#define isc_license_err_remove               336789536L
#define isc_license_err_update               336789537L
#define isc_license_err_convert              336789538L
#define isc_license_err_unk                  336789539L
#define isc_license_svc_err_add              336789540L
#define isc_license_svc_err_remove           336789541L
#define isc_license_eval_exists              336789563L
#define isc_gstat_unknown_switch             336920577L
#define isc_gstat_retry                      336920578L
#define isc_gstat_wrong_ods                  336920579L
#define isc_gstat_unexpected_eof             336920580L
#define isc_gstat_open_err                   336920605L
#define isc_gstat_read_err                   336920606L
#define isc_gstat_sysmemex                   336920607L
#define isc_fbsvcmgr_bad_am                  336986113L
#define isc_fbsvcmgr_bad_wm                  336986114L
#define isc_fbsvcmgr_bad_rs                  336986115L
#define isc_fbsvcmgr_info_err                336986116L
#define isc_fbsvcmgr_query_err               336986117L
#define isc_fbsvcmgr_switch_unknown          336986118L
#define isc_fbsvcmgr_bad_sm                  336986159L
#define isc_fbsvcmgr_fp_open                 336986160L
#define isc_fbsvcmgr_fp_read                 336986161L
#define isc_fbsvcmgr_fp_empty                336986162L
#define isc_fbsvcmgr_bad_arg                 336986164L
#define isc_utl_trusted_switch               337051649L
#define isc_err_max                          964

#endif

#endif /* JRD_GEN_IBERROR_H */


#endif /* JRD_IBASE_H */

