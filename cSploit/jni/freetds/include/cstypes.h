/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _cstypes_h_
#define _cstypes_h_

#include "tds_sysdep_public.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

static const char rcsid_cstypes_h[] = "$Id: cstypes.h,v 1.7 2008-05-06 00:14:02 jklowden Exp $";
static const void *const no_unused_cstypes_h_warn[] = { rcsid_cstypes_h, no_unused_cstypes_h_warn };

typedef tds_sysdep_int32_type 		CS_INT;
typedef unsigned tds_sysdep_int32_type 	CS_UINT;
typedef tds_sysdep_int64_type 		CS_BIGINT;
typedef unsigned tds_sysdep_int64_type 	CS_UBIGINT;
typedef tds_sysdep_int16_type 		CS_SMALLINT;
typedef unsigned tds_sysdep_int16_type 	CS_USMALLINT;
typedef unsigned char 			CS_TINYINT;
typedef char 				CS_CHAR;
typedef unsigned char 			CS_BYTE;
typedef tds_sysdep_real32_type 		CS_REAL;
typedef tds_sysdep_real64_type 		CS_FLOAT;
typedef tds_sysdep_int32_type 		CS_BOOL;
typedef void 				CS_VOID;
typedef unsigned char 			CS_IMAGE;
typedef unsigned char 			CS_TEXT;
typedef unsigned char 			CS_LONGBINARY;
typedef unsigned char 			CS_LONGCHAR;
typedef long 				CS_LONG;
typedef unsigned char 			CS_BINARY;
typedef unsigned tds_sysdep_int16_type 	CS_USHORT;
typedef unsigned char 			CS_BIT;

typedef CS_INT CS_RETCODE;

#define CS_MAX_NAME 132
#define CS_MAX_SCALE 77
#define CS_MAX_PREC 77		/* used by php */
#define CS_MAX_NUMLEN 33	/* used by roguewave */
#define CS_MAX_MSG 1024
#define CS_SQLSTATE_SIZE 8
#define CS_OBJ_NAME 400
#define CS_TP_SIZE  16		/* text pointer */
#define CS_TS_SIZE  8		/* length of timestamp */


typedef struct _cs_numeric
{
	unsigned char precision;
	unsigned char scale;
	unsigned char array[CS_MAX_NUMLEN];
} CS_NUMERIC;

typedef CS_NUMERIC CS_DECIMAL;

typedef struct _cs_varbinary
{
	CS_SMALLINT len;
	CS_CHAR array[256];
} CS_VARBINARY;

typedef struct _cs_varchar
{
    CS_SMALLINT len;                /* length of the string */
    CS_CHAR     str[256];   /* string, no NULL terminator */
} CS_VARCHAR;

typedef struct _cs_config CS_CONFIG;
typedef struct _cs_context CS_CONTEXT;
typedef struct _cs_connection CS_CONNECTION;
typedef struct _cs_locale CS_LOCALE;
typedef struct _cs_command CS_COMMAND;
typedef struct _cs_blk_row CS_BLK_ROW;

typedef struct _cs_iodesc
{
	CS_INT iotype;
	CS_INT datatype;
	CS_LOCALE *locale;
	CS_INT usertype;
	CS_INT total_txtlen;
	CS_INT offset;
	CS_BOOL log_on_update;
	CS_CHAR name[CS_OBJ_NAME];
	CS_INT namelen;
	CS_BYTE timestamp[CS_TS_SIZE];
	CS_INT timestamplen;
	CS_BYTE textptr[CS_TP_SIZE];
	CS_INT textptrlen;
} CS_IODESC;

typedef struct _cs_datafmt
{
	CS_CHAR name[CS_MAX_NAME];
	CS_INT namelen;
	CS_INT datatype;
	CS_INT format;
	CS_INT maxlength;
	CS_INT scale;
	CS_INT precision;
	CS_INT status;
	CS_INT count;
	CS_INT usertype;
	CS_LOCALE *locale;
} CS_DATAFMT;

typedef struct _cs_money
{
	CS_INT mnyhigh;
	CS_UINT mnylow;
} CS_MONEY;

typedef struct _cs_money4
{
	CS_INT mny4;
} CS_MONEY4;

typedef CS_INT CS_DATE;

typedef CS_INT CS_TIME;

typedef struct _cs_datetime
{
	CS_INT dtdays;
	CS_INT dttime;
} CS_DATETIME;

typedef struct _cs_datetime4
{
	CS_USHORT days;
	CS_USHORT minutes;
} CS_DATETIME4;

typedef struct _cs_daterec
{
	CS_INT dateyear;
	CS_INT datemonth;
	CS_INT datedmonth;
	CS_INT datedyear;
	CS_INT datedweek;
	CS_INT datehour;
	CS_INT dateminute;
	CS_INT datesecond;
	CS_INT datemsecond;
	CS_INT datetzone;
} CS_DATEREC;

typedef CS_INT CS_MSGNUM;

typedef struct _cs_clientmsg
{
	CS_INT severity;
	CS_MSGNUM msgnumber;
	CS_CHAR msgstring[CS_MAX_MSG];
	CS_INT msgstringlen;
	CS_INT osnumber;
	CS_CHAR osstring[CS_MAX_MSG];
	CS_INT osstringlen;
	CS_INT status;
	CS_BYTE sqlstate[CS_SQLSTATE_SIZE];
	CS_INT sqlstatelen;
} CS_CLIENTMSG;

typedef struct _cs_servermsg
{
	CS_MSGNUM msgnumber;
	CS_INT state;
	CS_INT severity;
	CS_CHAR text[CS_MAX_MSG];
	CS_INT textlen;
	CS_CHAR svrname[CS_MAX_NAME];
	CS_INT svrnlen;
	CS_CHAR proc[CS_MAX_NAME];
	CS_INT proclen;
	CS_INT line;
	CS_INT status;
	CS_BYTE sqlstate[CS_SQLSTATE_SIZE];
	CS_INT sqlstatelen;
} CS_SERVERMSG;

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
