/*____________________________________________________________
 *
 *		PROGRAM:	General preprocessor
 *		MODULE:		fbrmc.cpp
 *		DESCRIPTION:	RM/Cobol / fbclient interface
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed on an
 *  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 *  or implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  The Original Code was created by Stephen W. Boyd
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Steve Boyd <sboydlns at gmail.com>
 *  and all contributors signed below.
 *
 *  The routines in this module comprise a very thin interface between
 *  RM/Cobol calling sequence and the native C calling sequence.  In the
 *  interests of speed, the Cobol program is assumed to be passing the correct
 *  number of parameters of the correct type.  No error checking is done
 *  on the arguments at all.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  New module by Stephen W. Boyd 31.Aug.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <crtdbg.h>

#include "autoconfig.h"
#include "fb_types.h"
#include "../jrd/ibase.h"


#ifndef U_IPTR
#define U_IPTR ISC_ULONG
#endif

// Firebird transaction existence block (as defined in why.cpp)
struct ISC_TEB
{
	isc_db_handle*	dbb_ptr;
	int				tpb_len;
	ISC_UCHAR*		tpb_ptr;
};



// RM/Cobol parameter packet definition
struct argument_entry
{
	ISC_UCHAR	*a_address;
	ISC_ULONG	a_length;
	ISC_USHORT	a_type;
	ISC_UCHAR	a_digits;
	ISC_SCHAR	a_scale;
	ISC_UCHAR	*a_picture;
};

struct entry_table
{
	char		*EntryPointCobolName;
	int			(*EntryPointAddress)(char *, int, argument_entry *, int);
	char		*EntryPointName;
};

struct date_fmt
{
	ISC_USHORT	yr_start;
	ISC_USHORT	yr_len;
	ISC_USHORT	mon_start;
	ISC_USHORT	day_start;
	ISC_USHORT	hr_start;
	ISC_USHORT	min_start;
	ISC_USHORT	sec_start;
};

#ifdef __WIN32__
#define EXPORT __declspec(dllexport)
#define CDECL  __cdecl
#else
#define EXPORT
#define CDECL
#endif
#define RM_PROTO(sub)	int sub(char *, int, argument_entry [], int)
#define RM_ENTRY(sub)	int CDECL  sub(char *, int, argument_entry arg_vector[], int)


// Define all entry points to be "C" not "C++"
#ifdef __cplusplus
extern "C" {
#endif
RM_PROTO(rmc_embed_dsql_length);
RM_PROTO(rmc_cancel_blob);
RM_PROTO(rmc_close_blob);
RM_PROTO(rmc_compile_request);
RM_PROTO(rmc_create_database);
RM_PROTO(rmc_ddl);
RM_PROTO(rmc_commit_transaction);
RM_PROTO(rmc_rollback_transaction);
RM_PROTO(rmc_drop_database);
RM_PROTO(rmc_embed_dsql_close);
RM_PROTO(rmc_embed_dsql_declare);
RM_PROTO(rmc_embed_dsql_describe);
RM_PROTO(rmc_embed_dsql_describe_bind);
RM_PROTO(rmc_embed_dsql_execute);
RM_PROTO(rmc_embed_dsql_execute2);
RM_PROTO(rmc_embed_dsql_execute_immed);
RM_PROTO(rmc_embed_dsql_insert);
RM_PROTO(rmc_embed_dsql_open);
RM_PROTO(rmc_embed_dsql_open2);
RM_PROTO(rmc_embed_dsql_prepare);
RM_PROTO(rmc_dsql_alloc_statement2);
RM_PROTO(rmc_dsql_execute_m);
RM_PROTO(rmc_dsql_free_statement);
RM_PROTO(rmc_dsql_set_cursor_name);
RM_PROTO(rmc_detach_database);
RM_PROTO(rmc_get_slice);
RM_PROTO(rmc_put_slice);
RM_PROTO(rmc_get_segment);
RM_PROTO(rmc_put_segment);
RM_PROTO(rmc_receive);
RM_PROTO(rmc_release_request);
RM_PROTO(rmc_unwind_request);
RM_PROTO(rmc_send);
RM_PROTO(rmc_start_transaction);
RM_PROTO(rmc_start_and_send);
RM_PROTO(rmc_start_request);
RM_PROTO(rmc_transact_request);
RM_PROTO(rmc_commit_retaining);
RM_PROTO(rmc_rollback_retaining);
RM_PROTO(rmc_attach_database);
RM_PROTO(rmc_modify_dpb);
RM_PROTO(rmc_free);
RM_PROTO(rmc_sqlcode);
RM_PROTO(rmc_embed_dsql_fetch);
RM_PROTO(rmc_event_block_a);
RM_PROTO(rmc_prepare_transaction);
RM_PROTO(rmc_wait_for_event);
RM_PROTO(rmc_event_counts);
RM_PROTO(rmc_baddress);
RM_PROTO(rmc_interprete);
RM_PROTO(rmc_status_address);
RM_PROTO(rmc_expand_dpb);
RM_PROTO(rmc_compile_request2);
RM_PROTO(rmc_create_blob2);
RM_PROTO(rmc_open_blob2);
RM_PROTO(rmc_qtoq);
RM_PROTO(rmc_btoc);
RM_PROTO(rmc_ctob);
RM_PROTO(rmc_decode_date);
RM_PROTO(rmc_encode_date);
RM_PROTO(rmc_dtoc);
RM_PROTO(rmc_ctod);
RM_PROTO(rmc_ftoc);
RM_PROTO(rmc_ctof);
RM_PROTO(rmc_ctos);
RM_PROTO(rmc_stoc);
#ifdef __cplusplus
}
#endif

// Globals

const int MAX_PARAMS = 20;

static bool 		stringPoolInitialized = false;
static bool 		intPoolInitialized = false;
static bool 		shortPoolInitialized = false;
static ISC_UCHAR 	*stringPool[MAX_PARAMS];
static ISC_ULONG	*intPool[MAX_PARAMS];
static ISC_USHORT	*shortPool[MAX_PARAMS];

// Clear the stringPool, freeing any previously allocated strings
static void ClearStringPool()
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (stringPoolInitialized && stringPool[i])
			free(stringPool[i]);
		stringPool[i] = NULL;
	}
	stringPoolInitialized = true;
}

// Clear the intPool, freeing any previously allocated ints
static void ClearIntPool()
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (intPoolInitialized && intPool[i])
			free(intPool[i]);
		intPool[i] = NULL;
	}
	intPoolInitialized = true;
}

// Clear the shortPool, freeing any previously allocated ints
static void ClearShortPool()
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (shortPoolInitialized && shortPool[i])
			free(shortPool[i]);
		shortPool[i] = NULL;
	}
	shortPoolInitialized = true;
}

// Clear all parameter pools
static void ClearParamPool()
{
	ClearIntPool();
	ClearShortPool();
	ClearStringPool();
}

// Allocate a buffer of n bytes in the string pool & return its buffer pointer.
// Return a NULL pointer if the pool is exhausted
static ISC_UCHAR* AllocStringPool(int n)
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (! stringPool[i])
		{
			stringPool[i] = (ISC_UCHAR *)malloc(n);
			return (stringPool[i]);
		}
	}
	return (NULL);
}

// Allocate an integer in the intPool & return a pointer to it.  Return a NULL
// pointer if the pool is exhausted
static ISC_ULONG* AllocIntPool()
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (! intPool[i])
		{
			intPool[i] = (ISC_ULONG *)malloc(sizeof(ISC_ULONG));
			return (intPool[i]);
		}
	}
	return (NULL);
}

// Allocate an integer in the shortPool & return a pointer to it.  Return a NULL
// pointer if the pool is exhausted
static ISC_USHORT* AllocShortPool()
{
	for (int i = 0; i < MAX_PARAMS; i++)
	{
		if (! shortPool[i])
		{
			shortPool[i] = (ISC_USHORT *)malloc(sizeof(ISC_USHORT));
			return (shortPool[i]);
		}
	}
	return (NULL);
}

// Allocate an ISC_STATUS array to hold  the status vector of isc_* functions.
// For convenience this is allocated from the string pool since there is only
// ever one per function call and there is no need to set up a separate pool
static ISC_STATUS* AllocStatusPool()
{
	return (ISC_STATUS*) AllocStringPool(sizeof(ISC_STATUS_ARRAY));
}

// Return a Cobol argument as a C string.  Trailing spaces are truncated.
static ISC_UCHAR* CobolToString(const argument_entry *arg)
{
	ISC_UCHAR		*s = NULL;

	if (arg->a_address)
	{
		s = AllocStringPool(arg->a_length + 1);
		if (s)
		{
			memset(s, 0, arg->a_length + 1);
			memmove(s, arg->a_address, arg->a_length);
			// Truncate trailing spaces
			for (ISC_UCHAR *p = s + arg->a_length - 1; (p >= s) && (*p == ' '); p--)
				*p = '\0';
		}
	}
	return (s);
}

// Return the int64 value of an array of ASCII characters
static ISC_UINT64 atoi64(const char *s)
{
	ISC_UINT64 rslt = 0;
	while (*s)
	{
		if ((*s >= '0') && (*s <= '9'))
			rslt = (rslt * 10) + (*s - '0');
		else
			return (0);
		s++;
	}
	return (rslt);
}
// Convert a Cobol BINARY(n) to a C integer.
static ISC_UINT64 CvtCobolToInt(const ISC_UCHAR *s, ISC_USHORT len, char type)
{
	ISC_UINT64		temp;

	switch (len)
	{
	case 2:
		temp = (*(s) << 8) + *(s + 1);
		// If source is negative and signed, extend sign to dest
		if ((*(s + 1) & 0x80) && (type == 11))
			temp |= 0xffffffffffff0000;
		break;
	case 4:
		temp = (*(s) << 24) + (*(s + 1) << 16) + (*(s + 2) << 8) + *(s + 3);
		// If source is negative and signed, extend sign to dest
		if ((*(s + 1) & 0x80) && (type == 11))
			temp |= 0xffffffff00000000;
		break;
	case 8:
		temp = ((ISC_UINT64)*(s) << 56) + ((ISC_UINT64)*(s + 1) << 48) +
			   ((ISC_UINT64)*(s + 2) << 40) + ((ISC_UINT64)*(s + 3) << 32) +
		       ((ISC_UINT64)*(s+ 4) << 24) + ((ISC_UINT64)*(s + 5) << 16) +
			   ((ISC_UINT64)*(s + 6) << 8) + (ISC_UINT64)*(s + 7);
		break;
	default:
		temp = 0;
		break;
	}

	return (temp);
}

// Convert a cobol argument to a C integer. Make allowances for different
// data types, since constants get passed as DISPLAY (NSU or NTS) and fields
// should be BINARY (NBS or NBU)
static ISC_UINT64 CvtArgToInt(const argument_entry *arg)
{
	ISC_UINT64		temp;
	char			*stemp;
	char			sign;

	switch (arg->a_type)
	{
	case 1:														// NSU
		temp = (ISC_UINT64)atoi64((char *)CobolToString(arg));
		break;
	case 2:														// NTS
		stemp = (char *)CobolToString(arg);
		sign = *(stemp + strlen(stemp) - 1);
		*(stemp + strlen(stemp) - 1) = '\0';
		if (sign == '-')
			temp = (ISC_UINT64)((ISC_INT64)atoi64(stemp) * -1);
		else
			temp = atoi64(stemp);
		break;
	case 11:													// NBS
	case 12:													// NBU
		temp = CvtCobolToInt(arg->a_address,
							 (ISC_USHORT)arg->a_length,
							 arg->a_type);
		break;
	default:
		temp = 0;
		break;
	}

	return (temp);
}

// Return a Cobol BINARY(n) (big-endian) argument as a C integer
static ISC_ULONG* CobolToInt(const argument_entry *arg)
{
	ISC_ULONG		*i = NULL;
	if (arg->a_address)
	{
		ISC_ULONG temp = CvtArgToInt(arg);
		i = AllocIntPool();
		if (i)
			*i = temp;
	}
	return (i);
}

// Return a Cobol BINARY(n) (big-endian) argument as a C short integer
static ISC_USHORT* CobolToShort(const argument_entry *arg)
{
	ISC_USHORT		*i = NULL;
	if (arg->a_address)
	{
		ISC_USHORT temp = (ISC_USHORT)CvtArgToInt(arg);
		i = AllocShortPool();
		if (i)
			*i = temp;
	}
	return (i);
}

// Store a Cobol array of BINARY(4) fields into a status vector
static void CobolToStatus(ISC_STATUS *stat, const argument_entry *arg)
{
	const ISC_UCHAR *src = arg->a_address;
	if (src)
		for (int i = 0; i < ISC_STATUS_LENGTH; i++)
		{
			*(stat++) = (ISC_STATUS)((*src << 24) +
									 (*(src + 1) << 16) +
									 (*(src + 2) << 8) +
									 *(src + 3));
			src += 4;
		}
}

// Store a C integer into a Cobol format memory buffer
static void CvtIntToCobol(ISC_UCHAR *s, ISC_UINT64 l, ISC_USHORT len)
{
	memset(s, 0, len);
	switch (len)
	{
		case 2:
			*(s) = (ISC_UCHAR)((l >> 8) & 0xff);
			*(s + 1) = (ISC_UCHAR)(l & 0xff);
			break;
		case 4:
			*(s) = (ISC_UCHAR)((l >> 24) & 0xff);
			*(s + 1) = (ISC_UCHAR)((l >> 16) & 0xff);
			*(s + 2) = (ISC_UCHAR)((l >> 8) & 0xff);
			*(s + 3) = (ISC_UCHAR)(l & 0xff);
			break;
		case 8:
			*(s) = (ISC_UCHAR)((l >> 56) & 0xff);
			*(s + 1) = (ISC_UCHAR)((l >> 48) & 0xff);
			*(s + 2) = (ISC_UCHAR)((l >> 40) & 0xff);
			*(s + 3) = (ISC_UCHAR)((l >> 32) & 0xff);
			*(s + 4) = (ISC_UCHAR)((l >> 24) & 0xff);
			*(s + 5) = (ISC_UCHAR)((l >> 16) & 0xff);
			*(s + 6) = (ISC_UCHAR)((l >> 8) & 0xff);
			*(s + 7) = (ISC_UCHAR)(l & 0xff);
			break;
	}
}

// Store a C integer into a Cobol BINARY(n) (big-endian) field
static void IntToCobol(argument_entry *arg, const ISC_UINT64 i)
{
	if (arg->a_address)
		CvtIntToCobol(arg->a_address, i, (ISC_USHORT)arg->a_length);
}

// Store a C short integer into a Cobol BINARY(n) (big-endian) field
static void ShortToCobol(argument_entry *arg, const ISC_USHORT *i)
{
	if (arg->a_address)
		CvtIntToCobol(arg->a_address, (ISC_ULONG)*i, (ISC_USHORT)arg->a_length);
}

// Store a C string into a COBOL PIC X field
static void StringToCobol(argument_entry *arg, const ISC_UCHAR *s)
{
	if (arg->a_address)
	{
		memset(arg->a_address, ' ', arg->a_length);
		memmove(arg->a_address,
				s,
				strlen((char *)s) > arg->a_length ? arg->a_length : strlen((char *)s));
	}
}

// Store a status vector into a Cobol array of BINARY(4) fields
static void StatusToCobol(argument_entry *arg, const ISC_STATUS *stat)
{
	ISC_UCHAR	*dest = arg->a_address;
	if (arg->a_address)
	{
		for (int i = 0; i < ISC_STATUS_LENGTH; i++)
		{
			*(dest++) = (ISC_UCHAR)((*stat >> 24) & 0xff);
			*(dest++) = (ISC_UCHAR)((*stat >> 16) & 0xff);
			*(dest++) = (ISC_UCHAR)((*stat >> 8) & 0xff);
			*(dest++) = (ISC_UCHAR)(*stat & 0xff);
			stat++;
		}
	}
}

// Parse a date format string.  The format string contains the following
// specifiers:
//
// YY				- 2 digit year
// YYYY			- 4 digit year
// MM				- 2 digit month
// DD				- 2 digit day
// HH				- 2 digit hour (24 hour format)
// NN				- 2 digit minute
// SS				- 2 digit second
static void ParseDateFormat(const ISC_UCHAR* s, date_fmt* fmt)
{
	ISC_UCHAR		spec = ' ';
	short			len = 0;
	short			start = 0;
	short			i = 0;

	memset(fmt, -1, sizeof(*fmt));
	while (*s)
	{
		if (toupper(*s) != spec)
		{
			switch (spec)
			{
			case 'Y':
				fmt->yr_start = start;
				fmt->yr_len = (short)(len > 4 ? 4 : len);
				break;
			case 'M':
				fmt->mon_start = start;
				break;
			case 'D':
				fmt->day_start = start;
				break;
			case 'H':
				fmt->hr_start = start;
				break;
			case 'N':
				fmt->min_start = start;
				break;
			case 'S':
				fmt->sec_start = start;
				break;
			}
			spec = (ISC_UCHAR)toupper(*s);
			start = i;
			len = 0;
		}
		s++;
		i++;
		len++;
	}
	// Save the last specification
	switch (spec)
	{
	case 'Y':
		fmt->yr_start = start;
		fmt->yr_len = (short)(len > 4 ? 4 : len);
		break;
	case 'M':
		fmt->mon_start = start;
		break;
	case 'D':
		fmt->day_start = start;
		break;
	case 'H':
		fmt->hr_start = start;
		break;
	case 'N':
		fmt->min_start = start;
		break;
	case 'S':
		fmt->sec_start = start;
		break;
	}
}

// RM/Cobol entry points.
//
// All entry points start with rmc_ to avoid conflicts with ibase.h.
// These are all mapped to the corresponding isc_ entry point names
// in fbrmc.def

EXPORT RM_ENTRY(rmc_embed_dsql_length)
{
	ClearParamPool();
	ISC_USHORT *l = CobolToShort(&arg_vector[1]);
	isc_embed_dsql_length(CobolToString(&arg_vector[0]), l);
	ShortToCobol(&arg_vector[1], l);

	return (0);
}

EXPORT RM_ENTRY(rmc_cancel_blob)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_cancel_blob(stat, (isc_blob_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_close_blob)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_close_blob(stat, (isc_blob_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_compile_request)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_compile_request(stat,
						(isc_db_handle *)arg_vector[1].a_address,
						(isc_req_handle *)arg_vector[2].a_address,
						*CobolToShort(&arg_vector[3]),
						(char *)arg_vector[4].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_compile_request2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_compile_request2(stat,
						 (isc_db_handle *)arg_vector[1].a_address,
						 (isc_req_handle *)arg_vector[2].a_address,
						 *CobolToShort(&arg_vector[3]),
						 (char *)arg_vector[4].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_create_database)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_create_database(stat,
						*CobolToShort(&arg_vector[1]),
						(char *)CobolToString(&arg_vector[2]),
						(isc_db_handle *)arg_vector[3].a_address,
						*CobolToShort(&arg_vector[4]),
						(char *)arg_vector[5].a_address,
						*CobolToShort(&arg_vector[6]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_ddl)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_ddl(stat,
			(isc_db_handle *)arg_vector[1].a_address,
			(isc_tr_handle *)arg_vector[2].a_address,
			(short)*CobolToShort(&arg_vector[3]),
			(char *)CobolToString(&arg_vector[4]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_commit_transaction)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_commit_transaction(stat, (isc_tr_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_rollback_transaction)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_rollback_transaction(stat, (isc_tr_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_drop_database)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_drop_database(stat, (isc_db_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_close)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_close(stat, (char *)CobolToString(&arg_vector[1]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_declare)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_declare(stat,
						   (char *)CobolToString(&arg_vector[1]),
						   (char *)CobolToString(&arg_vector[2]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_describe)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_describe(stat,
							(char *)CobolToString(&arg_vector[1]),
							*CobolToShort(&arg_vector[2]),
							(XSQLDA *)arg_vector[3].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_describe_bind)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_describe_bind(stat,
								 (char *)CobolToString(&arg_vector[1]),
								 *CobolToShort(&arg_vector[2]),
								 (XSQLDA *)arg_vector[3].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_execute)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_execute(stat,
						   (isc_tr_handle *)arg_vector[1].a_address,
						   (char *)CobolToString(&arg_vector[2]),
						   *CobolToShort(&arg_vector[3]),
						   (XSQLDA *)arg_vector[4].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_execute2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_execute2(stat,
							(isc_tr_handle *)arg_vector[1].a_address,
							(char *)CobolToString(&arg_vector[2]),
							*CobolToShort(&arg_vector[3]),
							(XSQLDA *)arg_vector[4].a_address,
							(XSQLDA *)arg_vector[5].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_execute_immed)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_execute_immed(stat,
								 (isc_db_handle *)arg_vector[1].a_address,
								 (isc_tr_handle *)arg_vector[2].a_address,
								 *CobolToShort(&arg_vector[3]),
								 (char *)CobolToString(&arg_vector[4]),
								 *CobolToShort(&arg_vector[5]),
								 (XSQLDA *)arg_vector[6].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_insert)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_insert(stat,
						  (char *)CobolToString(&arg_vector[1]),
						  *CobolToShort(&arg_vector[2]),
						  (XSQLDA *)arg_vector[3].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_open)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_open(stat,
						(isc_tr_handle *)arg_vector[1].a_address,
						(char *)CobolToString(&arg_vector[2]),
						*CobolToShort(&arg_vector[3]),
						(XSQLDA *)arg_vector[4].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_open2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_open2(stat,
						 (isc_tr_handle *)arg_vector[1].a_address,
						 (char *)CobolToString(&arg_vector[2]),
						 *CobolToShort(&arg_vector[3]),
						 (XSQLDA *)arg_vector[4].a_address,
						 (XSQLDA *)arg_vector[5].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_prepare)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_embed_dsql_prepare(stat,
						   (isc_db_handle *)arg_vector[1].a_address,
						   (isc_tr_handle *)arg_vector[2].a_address,
						   (char *)CobolToString(&arg_vector[3]),
						   (short)*CobolToShort(&arg_vector[4]),
						   (char *)CobolToString(&arg_vector[5]),
						   (short)*CobolToShort(&arg_vector[6]),
						   (XSQLDA *)arg_vector[7].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_dsql_allocate_statement)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_dsql_allocate_statement(stat,
							    (isc_db_handle *)arg_vector[1].a_address,
							    (isc_tr_handle *)arg_vector[2].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_dsql_alloc_statement2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_dsql_alloc_statement2(stat,
							  (isc_db_handle *)arg_vector[1].a_address,
							  (isc_tr_handle *)arg_vector[2].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_dsql_execute_m)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_dsql_execute_m(stat,
					   (isc_tr_handle *)arg_vector[1].a_address,
					   (isc_stmt_handle *)arg_vector[2].a_address,
					   *CobolToShort(&arg_vector[3]),
					   (char *)CobolToString(&arg_vector[4]),
					   *CobolToShort(&arg_vector[5]),
					   *CobolToShort(&arg_vector[6]),
					   (char *)CobolToString(&arg_vector[7]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_dsql_free_statement)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_dsql_free_statement(stat,
							(isc_stmt_handle *)arg_vector[1].a_address,
							*CobolToShort(&arg_vector[2]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_dsql_set_cursor_name)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_dsql_set_cursor_name(stat,
							 (isc_stmt_handle *)arg_vector[1].a_address,
							 (char *)CobolToString(&arg_vector[2]),
							 *CobolToShort(&arg_vector[3]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_detach_database)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_detach_database(stat,
						(isc_db_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_get_slice)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	ISC_LONG *p1 = (ISC_LONG *)CobolToInt(&arg_vector[10]);
	isc_get_slice(stat,
				  (isc_db_handle *)arg_vector[1].a_address,
				  (isc_tr_handle *)arg_vector[2].a_address,
				  (ISC_QUAD *)arg_vector[3].a_address,
				  (short)*CobolToShort(&arg_vector[4]),
				  (char *)CobolToString(&arg_vector[5]),
				  (short)*CobolToShort(&arg_vector[6]),
				  (ISC_LONG *)CobolToInt(&arg_vector[7]),
				  *CobolToInt(&arg_vector[8]),
				  (void *)arg_vector[9].a_address,
				  p1);
	IntToCobol(&arg_vector[11], (ISC_UINT64)p1);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_put_slice)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_put_slice(stat,
				  (isc_db_handle *)arg_vector[1].a_address,
				  (isc_tr_handle *)arg_vector[2].a_address,
				  (ISC_QUAD *)arg_vector[3].a_address,
				  (short)*CobolToShort(&arg_vector[4]),
				  (char *)CobolToString(&arg_vector[5]),
				  (short)*CobolToShort(&arg_vector[6]),
				  (ISC_LONG *)CobolToInt(&arg_vector[7]),
				  *CobolToInt(&arg_vector[8]),
				  (void *)arg_vector[9].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_get_segment)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	ISC_USHORT *bytesRead = CobolToShort(&arg_vector[2]);
	isc_get_segment(stat,
					(isc_blob_handle *)arg_vector[1].a_address,
					bytesRead,
					*CobolToShort(&arg_vector[3]),
					(char *)arg_vector[4].a_address);
	ShortToCobol(&arg_vector[2], bytesRead);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_put_segment)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_put_segment(stat,
					(isc_blob_handle *)arg_vector[1].a_address,
					*CobolToShort(&arg_vector[2]),
					(char *)arg_vector[3].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_receive)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_receive(stat,
				(isc_req_handle *)arg_vector[1].a_address,
				(short)*CobolToShort(&arg_vector[2]),
				(short)*CobolToShort(&arg_vector[3]),
				(void *)arg_vector[4].a_address,
				(short)*CobolToShort(&arg_vector[5]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_release_request)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_release_request(stat,
						(isc_req_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_unwind_request)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_unwind_request(stat,
					   (isc_tr_handle *)arg_vector[1].a_address,
					   (short)*CobolToShort(&arg_vector[2]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_send)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_send(stat,
			 (isc_req_handle *)arg_vector[1].a_address,
			 (short)*CobolToShort(&arg_vector[2]),
			 (short)*CobolToShort(&arg_vector[3]),
			 (void *)arg_vector[4].a_address,
			 (short)*CobolToShort(&arg_vector[5]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_start_transaction)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	short* dbCount = (short *)CobolToShort(&arg_vector[2]);
	// Create the transaction existence block from the Cobol parameter list.  Because
	// of limits imposed by the converted parameter pool we can ony support up to
	// MAX_PARAMS transactions.
	if (*dbCount > MAX_PARAMS)
		*dbCount = MAX_PARAMS;
	ISC_TEB *teb = (ISC_TEB *)malloc(sizeof(ISC_TEB) * *dbCount);
	for (int i = 0; i < *dbCount; i++)
	{
		teb[i].dbb_ptr = (isc_db_handle *)arg_vector[(i * 3) + 3].a_address;
		teb[i].tpb_len = (int)*CobolToInt(&arg_vector[(i * 3) + 4]);
		teb[i].tpb_ptr = arg_vector[(i * 3) + 5].a_address;
	}

	isc_start_multiple(stat,
					   (isc_tr_handle *)arg_vector[1].a_address,
					   *dbCount,
					   teb);

	free(teb);
	StatusToCobol(&arg_vector[0], stat);
	return (0);

}

EXPORT RM_ENTRY(rmc_start_and_send)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_start_and_send(stat,
					   (isc_req_handle *)arg_vector[1].a_address,
					   (isc_tr_handle *)arg_vector[2].a_address,
					   (short)*CobolToShort(&arg_vector[3]),
					   (short)*CobolToShort(&arg_vector[4]),
					   (void *)arg_vector[5].a_address,
					   (short)*CobolToShort(&arg_vector[6]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_start_request)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_start_request(stat,
					  (isc_req_handle *)arg_vector[1].a_address,
					  (isc_tr_handle *)arg_vector[2].a_address,
					  (short)*CobolToShort(&arg_vector[3]));
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_transact_request)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_transact_request(stat,
						 (isc_db_handle *)arg_vector[1].a_address,
						 (isc_tr_handle *)arg_vector[2].a_address,
						 *CobolToShort(&arg_vector[3]),
						 (char *)arg_vector[4].a_address,
						 *CobolToShort(&arg_vector[5]),
						 (char *)arg_vector[6].a_address,
						 *CobolToShort(&arg_vector[7]),
						 (char *)arg_vector[8].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_commit_retaining)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_commit_retaining(stat,
						 (isc_tr_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_rollback_retaining)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_rollback_retaining(stat,
						   (isc_tr_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_attach_database)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_attach_database(stat,
						(short)*CobolToShort(&arg_vector[1]),
						(char *)CobolToString(&arg_vector[2]),
						(isc_db_handle *)arg_vector[3].a_address,
						(short)*CobolToShort(&arg_vector[4]),
						*(char **)arg_vector[5].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_modify_dpb)
{
	ClearParamPool();
	short *dpbLen = (short *)CobolToShort(&arg_vector[1]);
	isc_modify_dpb((char **)arg_vector[0].a_address,
				   dpbLen,
				   *CobolToShort(&arg_vector[2]),
				   (char *)arg_vector[3].a_address,
				   (short)*CobolToShort(&arg_vector[4]));
	ShortToCobol(&arg_vector[1], (ISC_USHORT *)dpbLen);

	return (0);
}

EXPORT RM_ENTRY(rmc_free)
{
	isc_free(*(char **)arg_vector[0].a_address);

	return (0);
}

EXPORT RM_ENTRY(rmc_sqlcode)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	CobolToStatus(stat, &arg_vector[0]);
	ISC_LONG sqlcode = isc_sqlcode(stat);
	IntToCobol(&arg_vector[-1], (ISC_UINT64)sqlcode);

	return (0);
}

EXPORT RM_ENTRY(rmc_embed_dsql_fetch)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	ISC_STATUS retval = isc_embed_dsql_fetch(stat,
											 (char *)CobolToString(&arg_vector[1]),
											 *CobolToShort(&arg_vector[2]),
											 (XSQLDA *)arg_vector[3].a_address);
	IntToCobol(&arg_vector[-1], (ISC_UINT64)retval);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_event_block_a)
{
	ClearParamPool();
	ISC_USHORT retval = isc_event_block_a((char **)arg_vector[0].a_address,
										  (char **)arg_vector[1].a_address,
										  atoi((char *)CobolToString(&arg_vector[2])),
										  (char **)arg_vector[3].a_address);
	ShortToCobol(&arg_vector[-1], &retval);

	return (0);
}

EXPORT RM_ENTRY(rmc_prepare_transaction)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_prepare_transaction(stat,
							(isc_tr_handle *)arg_vector[1].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_wait_for_event)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_wait_for_event(stat,
					   (isc_db_handle *)arg_vector[1].a_address,
					   (short)*CobolToShort(&arg_vector[2]),
					   (ISC_UCHAR *)*(char **)arg_vector[3].a_address,
					   (ISC_UCHAR *)*(char **)arg_vector[4].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_event_counts)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_event_counts((ISC_ULONG *)stat,
					 (short)*CobolToShort(&arg_vector[1]),
					 (ISC_UCHAR *)*(char **)arg_vector[2].a_address,
					 (ISC_UCHAR *)*(char **)arg_vector[3].a_address);
	// Convert the event counts to Cobol format.
	ISC_UCHAR *counts = (ISC_UCHAR *)arg_vector[0].a_address;
	ISC_UCHAR *cend = counts + arg_vector[0].a_length;
	int i = 0;
	while ((counts < cend) && (i < 15))
	{
		CvtIntToCobol(counts, stat[i], sizeof(ISC_ULONG));
		counts += sizeof(ISC_ULONG);
		i++;
	}

	return (0);
}

EXPORT RM_ENTRY(rmc_baddress)
{
	U_IPTR retval = isc_baddress((char *)arg_vector[0].a_address);
	*(ISC_ULONG *)arg_vector[-1].a_address = retval;

	return (0);
}

EXPORT RM_ENTRY(rmc_interprete)
{
	ClearParamPool();
	char *bfr = (char *)AllocStringPool(arg_vector[0].a_length + 1);
	ISC_STATUS retval = isc_interprete(bfr,
									  (ISC_STATUS **)arg_vector[1].a_address);
	StringToCobol(&arg_vector[0], (ISC_UCHAR *)bfr);
	IntToCobol(&arg_vector[-1], (ISC_UINT64)retval);

	return (0);
}

EXPORT RM_ENTRY(rmc_expand_dpb)
{
	ClearParamPool();
	short *dpbLen = (short *)CobolToShort(&arg_vector[1]);
	isc_expand_dpb((char **)arg_vector[0].a_address,
				   dpbLen,
				   *CobolToShort(&arg_vector[2]),
				   (char *)CobolToString(&arg_vector[3]),
				   NULL);
	ShortToCobol(&arg_vector[1], (ISC_USHORT *)dpbLen);

	return (0);
}

EXPORT RM_ENTRY(rmc_create_blob2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_create_blob2(stat,
					 (isc_db_handle *)arg_vector[1].a_address,
					 (isc_tr_handle *)arg_vector[2].a_address,
					 (isc_blob_handle *)arg_vector[3].a_address,
					 (ISC_QUAD *)arg_vector[4].a_address,
					 (short)*CobolToShort(&arg_vector[5]),
					 (char *)arg_vector[6].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_open_blob2)
{
	ClearParamPool();
	ISC_STATUS *stat = AllocStatusPool();
	isc_open_blob2(stat,
				   (isc_db_handle *)arg_vector[1].a_address,
				   (isc_tr_handle *)arg_vector[2].a_address,
				   (isc_blob_handle *)arg_vector[3].a_address,
				   (ISC_QUAD *)arg_vector[4].a_address,
				   (short)*CobolToShort(&arg_vector[5]),
				   (ISC_UCHAR *)arg_vector[6].a_address);
	StatusToCobol(&arg_vector[0], stat);

	return (0);
}

EXPORT RM_ENTRY(rmc_qtoq)
{
	isc_qtoq((ISC_QUAD *)arg_vector[0].a_address,
			 (ISC_QUAD *)arg_vector[1].a_address);

	return (0);
}

EXPORT RM_ENTRY(rmc_decode_date)
{
	struct tm	*tim = (struct tm *)arg_vector[1].a_address;
	isc_decode_date((ISC_QUAD *)arg_vector[0].a_address, tim);
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_sec, tim->tm_sec, sizeof(tim->tm_sec));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_min, tim->tm_min, sizeof(tim->tm_min));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_hour, tim->tm_hour, sizeof(tim->tm_hour));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_mday, tim->tm_mday, sizeof(tim->tm_mday));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_mon, tim->tm_mon, sizeof(tim->tm_mon));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_year, tim->tm_year, sizeof(tim->tm_year));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_wday, tim->tm_wday, sizeof(tim->tm_wday));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_yday, tim->tm_yday, sizeof(tim->tm_yday));
	CvtIntToCobol((ISC_UCHAR *)&tim->tm_isdst, tim->tm_isdst, sizeof(tim->tm_isdst));

	return (0);
}

EXPORT RM_ENTRY(rmc_encode_date)
{
	struct tm	*tim = (struct tm *)arg_vector[0].a_address;
	tim->tm_sec = CvtCobolToInt((ISC_UCHAR *)&tim->tm_sec, sizeof(tim->tm_sec), 11);
	tim->tm_min = CvtCobolToInt((ISC_UCHAR *)&tim->tm_min, sizeof(tim->tm_min), 11);
	tim->tm_hour = CvtCobolToInt((ISC_UCHAR *)&tim->tm_hour, sizeof(tim->tm_hour), 11);
	tim->tm_mday = CvtCobolToInt((ISC_UCHAR *)&tim->tm_mday, sizeof(tim->tm_mday), 11);
	tim->tm_mon = CvtCobolToInt((ISC_UCHAR *)&tim->tm_mon, sizeof(tim->tm_mon), 11);
	tim->tm_year = CvtCobolToInt((ISC_UCHAR *)&tim->tm_year, sizeof(tim->tm_year), 11);
	tim->tm_wday = CvtCobolToInt((ISC_UCHAR *)&tim->tm_wday, sizeof(tim->tm_wday), 11);
	tim->tm_yday = CvtCobolToInt((ISC_UCHAR *)&tim->tm_yday, sizeof(tim->tm_yday), 11);
	tim->tm_isdst = CvtCobolToInt((ISC_UCHAR *)&tim->tm_isdst, sizeof(tim->tm_isdst), 11);
	isc_encode_date(tim, (ISC_QUAD *)arg_vector[1].a_address);

	return (0);
}

// The following routines are needed for the RM/Cobol interface but are not
// just wrappers around the various isc_* functions.

// Return a pointer to a C format status vector.  This is needed to allow
// Cobol programs to successfully call isc_interprete.  We convert the status
// vector from Cobol to C format and return a pointer to a static structure
// that can be passed to isc-interprete repeatedly.
EXPORT RM_ENTRY(rmc_status_address)
{
	static ISC_STATUS_ARRAY stat;

	CobolToStatus(stat, &arg_vector[0]);
	ISC_STATUS	*p = stat;
	*(ISC_ULONG *)arg_vector[-1].a_address = (ISC_ULONG)p;


	return (0);
}

// Convert a Cobol BINARY(n) field to C format.  Used to convert numeric
// data fields before calling other isc_routines
EXPORT RM_ENTRY(rmc_btoc)
{
	ISC_UINT64 temp = CvtArgToInt(&arg_vector[0]);
	if (arg_vector[-1].a_length == 2)
		*(ISC_USHORT *)arg_vector[-1].a_address = (ISC_USHORT)temp;
	else if (arg_vector[-1].a_length == 4)
		*(ISC_ULONG *)arg_vector[-1].a_address = temp;
	else
		*(ISC_UINT64 *)arg_vector[-1].a_address = temp;
	return (0);
}

// Convert a C format integer to Cobol BINARY(n) format.  Used to convert
// numeric data fields after calling other isc_ routines
EXPORT RM_ENTRY(rmc_ctob)
{
	ISC_UINT64		temp;

	if (arg_vector[0].a_length == 2)
		temp = *(ISC_USHORT *)arg_vector[0].a_address;
	else if (arg_vector[0].a_length == 4)
		temp = *(ISC_ULONG *)arg_vector[0].a_address;
	else if (arg_vector[0].a_length == 8)
		temp = *(ISC_UINT64 *)arg_vector[0].a_address;
	IntToCobol(&arg_vector[-1], temp);

	return (0);
}

// Convert a Cobol date field to ISC_QUAD format.  Used to convert date fields
// before calling other isc_ routines.
EXPORT RM_ENTRY(rmc_dtoc)
{
	char		stemp[10];
	struct tm	tim;
	date_fmt	fmt;

	ClearParamPool();
	const int dtlen = arg_vector[1].a_length;
	const ISC_UCHAR* dt = arg_vector[1].a_address;
	const ISC_UCHAR* fmtString = CobolToString(&arg_vector[2]);

	ParseDateFormat(fmtString, &fmt);
	memset(&tim, 0, sizeof(tim));
	if ((fmt.yr_start != -1) && (fmt.yr_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.yr_start, fmt.yr_len);
		tim.tm_year = atoi(stemp);
		// If the year is supplied as 4 digits, no problem just decerement by 1900
		// to keep isc_encode_date happy.  If 2 digits, assume date > 2000 and
		// add 100 to keep isc_encode_date happy.
		if (fmt.yr_len > 2)
			tim.tm_year -= 1900;
		else
			tim.tm_year += 100;
	}
	if ((fmt.mon_start != -1) && (fmt.mon_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.mon_start, 2);
		tim.tm_mon = atoi(stemp) - 1;
	}
	if ((fmt.day_start != -1) && (fmt.day_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.day_start, 2);
		tim.tm_mday = atoi(stemp);
	}
	if ((fmt.hr_start != -1) && (fmt.hr_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.hr_start, 2);
		tim.tm_hour = atoi(stemp);
	}
	if ((fmt.min_start != -1) && (fmt.min_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.min_start, 2);
		tim.tm_min = atoi(stemp);
	}
	if ((fmt.sec_start != -1) && (fmt.sec_start < dtlen))
	{
		memset(stemp, 0, sizeof(stemp));
		memmove(stemp, dt + fmt.sec_start, 2);
		tim.tm_sec = atoi(stemp);
	}

	isc_encode_date(&tim, (ISC_QUAD *)arg_vector[0].a_address);

	return (0);
}

// Convert an ISC_QUAD to Cobol date format.  Used to convert date fields after
// calling other isc _ routines
EXPORT RM_ENTRY(rmc_ctod)
{
	struct tm	tim;
	date_fmt	fmt;
	char		stemp[10];

	ClearParamPool();
	const ISC_UCHAR* fmtString = CobolToString(&arg_vector[2]);
	ISC_UCHAR* dt = AllocStringPool((int)strlen((char *)fmtString) + 1);
	memset(dt, 0, strlen((char *)fmtString) + 1);

	isc_decode_date((ISC_QUAD *)arg_vector[1].a_address, &tim);
	ParseDateFormat(fmtString, &fmt);

	if (fmt.yr_start != -1)
	{
		sprintf(stemp, "%*.*d", fmt.yr_len, fmt.yr_len, tim.tm_year + 1900);
		memmove(dt + fmt.yr_start, stemp, fmt.yr_len);
	}
	if (fmt.mon_start != -1)
	{
		sprintf(stemp, "%2.2d", tim.tm_mon + 1);
		memmove(dt + fmt.mon_start, stemp, 2);
	}
	if (fmt.day_start != -1)
	{
		sprintf(stemp, "%2.2d", tim.tm_mday);
		memmove(dt + fmt.day_start, stemp, 2);
	}
	if (fmt.hr_start != -1)
	{
		sprintf(stemp, "%2.2d", tim.tm_hour);
		memmove(dt + fmt.hr_start, stemp, 2);
	}
	if (fmt.min_start != -1)
	{
		sprintf(stemp, "%2.2d", tim.tm_min);
		memmove(dt + fmt.min_start, stemp, 2);
	}
	if (fmt.sec_start != -1)
	{
		sprintf(stemp, "%2.2d", tim.tm_sec);
		memmove(dt + fmt.sec_start, stemp, 2);
	}
	StringToCobol(&arg_vector[0], dt);

	return (0);
}

// Convert a C floating point value to Cobol format
// float = BINARY(4)
// double = BINARY(8)
EXPORT RM_ENTRY(rmc_ctof)
{
	double		arg;

	// Get floating point value
	if (arg_vector[0].a_length == 4)
		arg = *(float *)arg_vector[0].a_address;
	else
		arg = *(double *)arg_vector[0].a_address;
	// Get all significant digits into the integer part
	arg *= pow(10.0, -arg_vector[0].a_scale);
	ISC_INT64 intPart = (ISC_INT64)arg;
	// Round the integer part
	double frac  = arg - intPart;
	if (frac >= 0.5)
		intPart++;
	if (frac <= -0.5)
		intPart--;
	// Save as a Cobol format BINARY value
	ISC_UCHAR *s = (ISC_UCHAR *)arg_vector[-1].a_address;
	if (arg_vector[-1].a_length == 4)
	{
		*(s) = (ISC_UCHAR)((intPart >> 24) & 0xff);
		*(s + 1) = (ISC_UCHAR)((intPart >> 16) & 0xff);
		*(s + 2) = (ISC_UCHAR)((intPart >> 8) & 0xff);
		*(s + 3) = (ISC_UCHAR)(intPart & 0xff);
	}
	else
	{
		*(s) = (ISC_UCHAR)((intPart >> 56) & 0xff);
		*(s + 1) = (ISC_UCHAR)((intPart >> 48) & 0xff);
		*(s + 2) = (ISC_UCHAR)((intPart >> 40) & 0xff);
		*(s + 3) = (ISC_UCHAR)((intPart >> 32) & 0xff);
		*(s + 4) = (ISC_UCHAR)((intPart >> 24) & 0xff);
		*(s + 5) = (ISC_UCHAR)((intPart >> 16) & 0xff);
		*(s + 6) = (ISC_UCHAR)((intPart >> 8) & 0xff);
		*(s + 7) = (ISC_UCHAR)(intPart & 0xff);
	}

	return (0);
}

// Convert a Cobol format floating point to C format
// float = BINARY(4)
// double = BINARY(8)
EXPORT RM_ENTRY(rmc_ftoc)
{
	ISC_INT64	intPart;

	// Get Cobol value as a C 64 bit integer
	ISC_UCHAR *s = (ISC_UCHAR *)arg_vector[0].a_address;
	if (arg_vector[0].a_length == 4)
	{
		intPart = ((ISC_INT64)*(s) << 24) +
				  ((ISC_INT64)*(s + 1) << 16) +
				  ((ISC_INT64)*(s + 2) << 8) +
				  (ISC_INT64)*(s + 3);
	}
	else
	{
		intPart = ((ISC_INT64)*(s) << 56) +
				  ((ISC_INT64)*(s + 1) << 48) +
				  ((ISC_INT64)*(s + 2) << 40) +
				  ((ISC_INT64)*(s + 3) << 32) +
				  ((ISC_INT64)*(s + 4) << 24) +
				  ((ISC_INT64)*(s + 5) << 16) +
				  ((ISC_INT64)*(s + 6) << 8) +
				  (ISC_INT64)*(s + 7);
	}
	// Convert to a double & shift the fraction part right
	double arg = (double)intPart * pow(10.0, arg_vector[0].a_scale);
	// Save it into the return argument
	if (arg_vector[-1].a_length == 4)
		*(float *)arg_vector[-1].a_address = arg;
	else
		*(double *)arg_vector[-1].a_address = arg;

	return (0);
}


// Convert a Cobol alpha (PIC X) field to a C string.  This is done by trimming trailing spaces
// and adding the trailing '\0'.
EXPORT RM_ENTRY(rmc_stoc)
{
	char* dest = (char*) arg_vector[-1].a_address;
	const int dlen = arg_vector[-1].a_length;
	const char* src = (char*) arg_vector[0].a_address;
	const int slen = arg_vector[0].a_length;

	int i = slen - 1;
	while (src[i] == ' ' && i >= 0)
		--i;

	int len = (i + 1 < dlen) ? i + 1 : dlen - 1;
	memmove(dest, src, len);
	dest[len] = '\0';

	return (0);
}

// Convert a C string to a Cobol alpha (PIC X) field.  This is done by copying the original
// string and padding on the right with spaces.
EXPORT RM_ENTRY(rmc_ctos)
{
	char* dest = (char*) arg_vector[-1].a_address;
	const int dlen = arg_vector[-1].a_length;
	const char* src = (char*) arg_vector[0].a_address;
	const int slen = strlen(src);

	memset(dest, ' ', dlen);
	int len = (slen <= dlen) ? slen : dlen;
	memmove(dest, src, len);

	return (0);
}

static char* banner = "Firebird Embedded SQL Interface";

#ifdef __cplusplus
extern "C" {
#endif
char* RM_AddOnBanner();
char* RM_AddOnLoadMessage();
#ifdef __cplusplus
}
#endif

// Return additional banner message for this module
char* RM_AddOnBanner()
{
	return (banner);
}

char* RM_AddOnLoadMessage()
{
	return (banner);
}

// RM/Cobol entry point table. Required for linux/unix support
EXPORT entry_table RM_EntryPoints[] =
{
	{ "isc_embed_dsql_length", rmc_embed_dsql_length, "rmc_embed_dsql_length" },
	{ "isc_cancel_blob", rmc_cancel_blob, "rmc_cancel_blob" },
	{ "isc_close_blob", rmc_close_blob, "rmc_close_blob" },
	{ "isc_compile_request", rmc_compile_request, "rmc_compile_request" },
	{ "isc_create_database", rmc_create_database, "rmc_create_database" },
	{ "isc_ddl", rmc_ddl, "rmc_ddl" },
	{ "isc_commit_transaction", rmc_commit_transaction, "rmc_commit_transaction" },
	{ "isc_rollback_transaction", rmc_rollback_transaction, "rmc_rollback_transaction" },
	{ "isc_drop_database", rmc_drop_database, "rmc_drop_database" },
	{ "isc_embed_dsql_close", rmc_embed_dsql_close, "rmc_embed_dsql_close" },
	{ "isc_embed_dsql_declare", rmc_embed_dsql_declare, "rmc_embed_dsql_declare" },
	{ "isc_embed_dsql_describe", rmc_embed_dsql_describe, "rmc_embed_dsql_describe" },
	{ "isc_embed_dsql_describe_bind", rmc_embed_dsql_describe_bind, "rmc_embed_dsql_describe_bind" },
	{ "isc_embed_dsql_execute", rmc_embed_dsql_execute, "rmc_embed_dsql_execute" },
	{ "isc_embed_dsql_execute2", rmc_embed_dsql_execute2, "rmc_embed_dsql_execute2" },
	{ "isc_embed_dsql_execute_immed", rmc_embed_dsql_execute_immed, "rmc_embed_dsql_execute_immed" },
	{ "isc_embed_dsql_insert", rmc_embed_dsql_insert, "rmc_embed_dsql_insert" },
	{ "isc_embed_dsql_open", rmc_embed_dsql_open, "rmc_embed_dsql_open" },
	{ "isc_embed_dsql_open2", rmc_embed_dsql_open2, "rmc_embed_dsql_open2" },
	{ "isc_embed_dsql_prepare", rmc_embed_dsql_prepare, "rmc_embed_dsql_prepare" },
	{ "isc_dsql_allocate_statement", rmc_dsql_allocate_statement, "rmc_dsql_allocate_statement" },
	{ "isc_dsql_alloc_statement2", rmc_dsql_alloc_statement2, "rmc_dsql_alloc_statement2" },
	{ "isc_dsql_execute_m", rmc_dsql_execute_m, "rmc_dsql_execute_m" },
	{ "isc_dsql_free_statement", rmc_dsql_free_statement, "rmc_dsql_free_statement" },
	{ "isc_dsql_set_cursor_name", rmc_dsql_set_cursor_name, "rmc_dsql_set_cursor_name" },
	{ "isc_detach_database", rmc_detach_database, "rmc_detach_database" },
	{ "isc_get_slice", rmc_get_slice, "rmc_get_slice" },
	{ "isc_put_slice", rmc_put_slice, "rmc_put_slice" },
	{ "isc_get_segment", rmc_get_segment, "rmc_get_segment" },
	{ "isc_put_segment", rmc_put_segment, "rmc_put_segment" },
	{ "isc_receive", rmc_receive, "rmc_receive" },
	{ "isc_release_request", rmc_release_request, "rmc_release_request" },
	{ "isc_unwind_request", rmc_unwind_request, "rmc_unwind_request" },
	{ "isc_send", rmc_send, "rmc_send" },
	{ "isc_start_transaction", rmc_start_transaction, "rmc_start_transaction" },
	{ "isc_start_and_send", rmc_start_and_send, "rmc_start_and_send" },
	{ "isc_start_request", rmc_start_request, "rmc_start_request" },
	{ "isc_transact_request", rmc_transact_request, "rmc_transact_request" },
	{ "isc_commit_retaining", rmc_commit_retaining, "rmc_commit_retaining" },
	{ "isc_rollback_retaining", rmc_rollback_retaining, "rmc_rollback_retaining" },
	{ "isc_attach_database", rmc_attach_database, "rmc_attach_database" },
	{ "isc_modify_dpb", rmc_modify_dpb, "rmc_modify_dpb" },
	{ "isc_free", rmc_free, "rmc_free" },
	{ "isc_sqlcode", rmc_sqlcode, "rmc_sqlcode" },
	{ "isc_embed_dsql_fetch", rmc_embed_dsql_fetch, "rmc_embed_dsql_fetch" },
	{ "isc_event_block_a", rmc_event_block_a, "rmc_event_block_a" },
	{ "isc_prepare_transaction", rmc_prepare_transaction, "rmc_prepare_transaction" },
	{ "isc_wait_for_event", rmc_wait_for_event, "rmc_wait_for_event" },
	{ "isc_event_counts", rmc_event_counts, "rmc_event_counts" },
	{ "isc_baddress", rmc_baddress, "rmc_baddress" },
	{ "isc_interprete", rmc_interprete, "rmc_interprete" },
	{ "rmc_status_address", rmc_status_address, "rmc_status_address" },
	{ "isc_expand_dpb", rmc_expand_dpb, "rmc_expand_dpb" },
	{ "isc_compile_request2", rmc_compile_request2, "rmc_compile_request2" },
	{ "isc_create_blob2", rmc_create_blob2, "rmc_create_blob2" },
	{ "isc_open_blob2", rmc_open_blob2, "rmc_open_blob2" },
	{ "isc_qtoq", rmc_qtoq, "rmc_qtoq" },
	{ "rmc_btoc", rmc_btoc, "rmc_btoc" },
	{ "rmc_ctob", rmc_ctob, "rmc_ctob" },
	{ "isc_decode_date", rmc_decode_date, "rmc_decode_date" },
	{ "isc_encode_date", rmc_encode_date, "rmc_encode_date" },
	{ "rmc_dtoc", rmc_dtoc, "rmc_dtoc" },
	{ "rmc_ctod", rmc_ctod, "rmc_ctod" },
	{ "rmc_ftoc", rmc_ftoc, "rmc_ftoc" },
	{ "rmc_ctof", rmc_ctof, "rmc_ctof" },
	{ "rmc_ctos", rmc_ctos, "rmc_ctos" },
	{ "rmc_stoc", rmc_stoc, "RMC_stoc" },
	{ NULL, NULL, NULL }
};
