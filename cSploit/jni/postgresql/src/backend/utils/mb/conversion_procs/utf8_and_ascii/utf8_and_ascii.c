/*-------------------------------------------------------------------------
 *
 *	  ASCII <--> UTF8
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/mb/conversion_procs/utf8_and_ascii/utf8_and_ascii.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(ascii_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_ascii);

extern Datum ascii_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_ascii(PG_FUNCTION_ARGS);

/* ----------
 * conv_proc(
 *		INTEGER,	-- source encoding id
 *		INTEGER,	-- destination encoding id
 *		CSTRING,	-- source string (null terminated C string)
 *		CSTRING,	-- destination string (null terminated C string)
 *		INTEGER		-- source string length
 * ) returns VOID;
 * ----------
 */

Datum
ascii_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_SQL_ASCII, PG_UTF8);

	/* this looks wrong, but basically we're just rejecting high-bit-set */
	pg_ascii2mic(src, dest, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_ascii(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_SQL_ASCII);

	/* this looks wrong, but basically we're just rejecting high-bit-set */
	pg_mic2ascii(src, dest, len);

	PG_RETURN_VOID();
}
