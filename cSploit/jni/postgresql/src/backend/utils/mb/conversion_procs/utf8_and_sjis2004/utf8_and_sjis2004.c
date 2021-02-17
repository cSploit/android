/*-------------------------------------------------------------------------
 *
 *	  SHIFT_JIS_2004 <--> UTF8
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/mb/conversion_procs/utf8_and_sjis2004/utf8_and_sjis2004.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/shift_jis_2004_to_utf8.map"
#include "../../Unicode/utf8_to_shift_jis_2004.map"
#include "../../Unicode/shift_jis_2004_to_utf8_combined.map"
#include "../../Unicode/utf8_to_shift_jis_2004_combined.map"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(shift_jis_2004_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_shift_jis_2004);

extern Datum shift_jis_2004_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_shift_jis_2004(PG_FUNCTION_ARGS);

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
shift_jis_2004_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_SHIFT_JIS_2004, PG_UTF8);

	LocalToUtf(src, dest, LUmapSHIFT_JIS_2004, LUmapSHIFT_JIS_2004_combined,
			   sizeof(LUmapSHIFT_JIS_2004) / sizeof(pg_local_to_utf),
	 sizeof(LUmapSHIFT_JIS_2004_combined) / sizeof(pg_local_to_utf_combined),
			   PG_SHIFT_JIS_2004, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_shift_jis_2004(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_SHIFT_JIS_2004);

	UtfToLocal(src, dest, ULmapSHIFT_JIS_2004, ULmapSHIFT_JIS_2004_combined,
			   sizeof(ULmapSHIFT_JIS_2004) / sizeof(pg_utf_to_local),
	 sizeof(ULmapSHIFT_JIS_2004_combined) / sizeof(pg_utf_to_local_combined),
			   PG_SHIFT_JIS_2004, len);

	PG_RETURN_VOID();
}
