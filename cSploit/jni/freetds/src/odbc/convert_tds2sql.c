/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 * Copyright (C) 2003-2010  Frediano Ziglio
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

#include <config.h>

#include <assert.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <ctype.h>

#include <freetds/odbc.h>
#include <freetds/convert.h>
#include <freetds/iconv.h>
#include <freetds/string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: convert_tds2sql.c,v 1.80 2011-09-18 17:22:29 freddy77 Exp $");

#define TDS_ISSPACE(c) isspace((unsigned char) (c))

/**
 * Handle conversions from TDS (N)CHAR to ODBC (W)CHAR
 */
static SQLLEN
odbc_convert_char(TDS_STMT * stmt, TDSCOLUMN * curcol, TDS_CHAR * src, TDS_UINT srclen, int desttype, TDS_CHAR * dest, SQLULEN destlen)
{
	const char *ib;
	char *ob;
	size_t il, ol, char_size;

	/* FIXME MARS not correct cause is the global tds but stmt->tds can be NULL on SQLGetData */
	TDSSOCKET *tds = stmt->dbc->tds_socket;

	TDSICONV *conv = curcol->char_conv;
	if (!conv)
		conv = tds->conn->char_convs[client2server_chardata];
	if (desttype == SQL_C_WCHAR) {
		/* SQL_C_WCHAR, convert to wide encode */
		conv = tds_iconv_get(tds->conn, ODBC_WIDE_NAME, conv->to.charset.name);
		if (!conv)
			conv = tds_iconv_get(tds->conn, ODBC_WIDE_NAME, "ISO-8859-1");
#ifdef ENABLE_ODBC_WIDE
	} else {
		conv = tds_iconv_get(tds->conn, tds_dstr_cstr(&stmt->dbc->original_charset), conv->to.charset.name);
		if (!conv)
			conv = tds_iconv_get(tds->conn, tds_dstr_cstr(&stmt->dbc->original_charset), "ISO-8859-1");
		if (!conv)
			conv = tds_iconv_get(tds->conn, "ISO-8859-1", "ISO-8859-1");
#endif
	}

	ib = src;
	il = srclen;
	ob = dest;
	ol = 0;
	char_size = desttype == SQL_C_CHAR ? 1 : SIZEOF_SQLWCHAR;
	if (destlen >= char_size) {
		ol = destlen - char_size;
		memset(&conv->suppress, 0, sizeof(conv->suppress));
		conv->suppress.e2big = 1;
		/* TODO check return value */
		tds_iconv(tds, conv, to_client, &ib, &il, &ob, &ol);
		ol = ob - dest; /* bytes written */
		if (curcol)
			curcol->column_text_sqlgetdatapos += ib - src;
		/* terminate string */
		memset(ob, 0, char_size);
	}

	/* returned size have to take into account buffer left unconverted */
	if (il == 0 || (conv->from.charset.min_bytes_per_char == conv->from.charset.max_bytes_per_char
	    && conv->to.charset.min_bytes_per_char == conv->to.charset.max_bytes_per_char)) {
		ol += il * conv->from.charset.min_bytes_per_char / conv->to.charset.min_bytes_per_char;
	} else {
		/* TODO convert and discard ?? or return proper SQL_NO_TOTAL values ?? */
		return SQL_NO_TOTAL;
	}
	return ol;
}

/**
 * Handle conversions from TDS NCHAR to ISO8859-1 striping spaces (for fixed types)
 */
static int
odbc_tds_convert_wide_iso(TDSCOLUMN *curcol, TDS_CHAR *src, TDS_UINT srclen, TDS_CHAR *buf, TDS_UINT buf_len)
{
	TDS_CHAR *p;
	/*
	 * TODO check for endian
	 * This affect for instance Sybase under little endian system
	 */
	
	/* skip white spaces */
	while (srclen > 1 && src[1] == 0 && TDS_ISSPACE(src[0])) {
		srclen -= 2;
		src += 2;
	}

	/* convert */
	p = buf;
	while (buf_len > 1 && srclen > 1 && src[1] == 0) {
		*p++ = src[0];
		--buf_len;
		srclen -= 2;
		src += 2;
	}

	/* skip white spaces */
	while (srclen > 1 && src[1] == 0 && TDS_ISSPACE(src[0])) {
		srclen -= 2;
		src += 2;
	}

	/* still characters, wrong format */
	if (srclen)
		return -1;

	*p = 0;
	return p - buf;
}

/**
 * Handle conversions from MSSQL 2008 DATE/TIME types to binary.
 * These types have a different binary representation in libTDS.
 */
static SQLLEN
odbc_convert_msdatetime_to_binary(TDS_STMT * stmt, TDSCOLUMN *curcol, int srctype, TDS_DATETIMEALL * dta, TDS_CHAR * dest, SQLULEN destlen)
{
	size_t len, cplen;
	TDS_USMALLINT buf[10];
	TDSDATEREC when;

	tds_datecrack(srctype, dta, &when);

	len = 0;
	if (srctype != SYBMSTIME) {
		buf[0] = when.year;
		buf[1] = when.month + 1;
		buf[2] = when.day;
		len = 3;
	}
	if (srctype != SYBMSDATE) {
		buf[len++] = when.hour;
		buf[len++] = when.minute;
		buf[len++] = when.second;
		if ((len % 2) != 0)
			buf[len++] = 0;
		*((TDS_UINT*) (buf+len)) = when.decimicrosecond * 100u;
		len += 2;
	}
	if (srctype == SYBMSDATETIMEOFFSET) {
		/* TODO check for negative hour/minutes */
		buf[8] = dta->offset / 60;
		buf[9] = dta->offset % 60;
		len = 10;
	}
	len *= 2;

	/* just return length */
	if (destlen == 0)
		return len;

	cplen = (destlen > len) ? len : destlen;
	memcpy(dest, buf, cplen);
	if (curcol)
		curcol->column_text_sqlgetdatapos += cplen;
	return len;
}

static SQLLEN
odbc_convert_to_binary(TDS_STMT * stmt, TDSCOLUMN *curcol, int srctype, TDS_CHAR * src, TDS_UINT srclen, TDS_CHAR * dest, SQLULEN destlen)
{
	SQLLEN ret = srclen;

	/* special case for MS date/time */
	switch (srctype) {
	case SYBMSTIME:
	case SYBMSDATE:
	case SYBMSDATETIME2:
	case SYBMSDATETIMEOFFSET:
		return odbc_convert_msdatetime_to_binary(stmt, curcol, srctype, (TDS_DATETIMEALL *) src, dest, destlen);
	}

	if (destlen > 0) {
		size_t cplen = (destlen > srclen) ? srclen : destlen;
		/* do not NULL terminate binary buffer */
		memcpy(dest, src, cplen);
		if (curcol)
			curcol->column_text_sqlgetdatapos += cplen;
	} else {
		/* if destlen == 0 we return only length */
		if (destlen != 0) {
			odbc_errs_add(&stmt->errs, "07006", NULL);
			return SQL_NULL_DATA;
		}
	}
	return ret;
}

SQLLEN
odbc_tds2sql(TDS_STMT * stmt, TDSCOLUMN *curcol, int srctype, TDS_CHAR * src, TDS_UINT srclen, int desttype, TDS_CHAR * dest, SQLULEN destlen,
	     const struct _drecord *drec_ixd)
{
	TDS_INT nDestSybType;
	TDS_INT nRetVal = TDS_CONVERT_FAIL;
	TDSCONTEXT *context = stmt->dbc->env->tds_ctx;

	CONV_RESULT ores;

	SQLLEN ret = SQL_NULL_DATA;
	int i, cplen;
	int binary_conversion = 0;
	TDS_CHAR conv_buf[256];

	tdsdump_log(TDS_DBG_FUNC, "odbc_tds2sql: src is %d dest = %d\n", srctype, desttype);

	assert(desttype != SQL_C_DEFAULT);

	if (curcol) {
		if (is_blob_col(curcol)) {
			if (srctype == SYBLONGBINARY && (
			    curcol->column_usertype == USER_UNICHAR_TYPE ||
			    curcol->column_usertype == USER_UNIVARCHAR_TYPE))
				srctype = SYBTEXT;
			if (curcol->column_type == SYBVARIANT)
				srctype = ((TDSVARIANT *) src)->type;
			src = ((TDSBLOB *) src)->textvalue;
		}
		if (is_variable_type(curcol->column_type)) {
			src += curcol->column_text_sqlgetdatapos;
			srclen -= curcol->column_text_sqlgetdatapos;
		}
	}

	nDestSybType = odbc_c_to_server_type(desttype);
	if (!nDestSybType) {
		odbc_errs_add(&stmt->errs, "HY003", NULL);
		return SQL_NULL_DATA;
	}

	/* special case for binary type */
	if (desttype == SQL_C_BINARY) {
		tdsdump_log(TDS_DBG_FUNC, "odbc_tds2sql: outputting binary data destlen = %lu \n", (unsigned long) destlen);

		if (is_numeric_type(srctype)) {
			desttype = SQL_C_NUMERIC;
			nDestSybType = SYBNUMERIC;
			/* prevent buffer overflow */
			if (destlen < sizeof(SQL_NUMERIC_STRUCT)) {
				odbc_errs_add(&stmt->errs, "07006", NULL);
				return SQL_NULL_DATA;
			}
			ores.n.precision = ((TDS_NUMERIC *) src)->precision;
			ores.n.scale = ((TDS_NUMERIC *) src)->scale;
		} else {
			return odbc_convert_to_binary(stmt, curcol, srctype, src, srclen, dest, destlen);
		}
	} else if (is_numeric_type(nDestSybType)) {
		/* TODO use descriptor information (APD) ?? However APD can contain SQL_C_DEFAULT... */
		if (drec_ixd)
			ores.n.precision = drec_ixd->sql_desc_precision;
		else
			ores.n.precision = 38;
		ores.n.scale = 0;
	}

	if (is_char_type(srctype)) {
		if (desttype == SQL_C_CHAR || desttype == SQL_C_WCHAR)
			return odbc_convert_char(stmt, curcol, src, srclen, desttype, dest, destlen);
		if (is_unicode_type(srctype)) {
			/*
			 * convert to single and then process normally.
			 * Here we processed SQL_C_BINARY and SQL_C_*CHAR so only fixed types are left
			 */
			i = odbc_tds_convert_wide_iso(curcol, src, srclen, conv_buf, sizeof(conv_buf));
			if (i < 0)
				return SQL_NULL_DATA;
			src = conv_buf;
			srclen = i;
			srctype = SYBVARCHAR;
		}
	}

	if (desttype == SQL_C_WCHAR)
		destlen /= sizeof(SQLWCHAR);
	if (desttype == SQL_C_CHAR || desttype == SQL_C_WCHAR) {
		switch (srctype) {
		case SYBLONGBINARY:
		case SYBBINARY:
		case SYBVARBINARY:
		case SYBIMAGE:
		case XSYBBINARY:
		case XSYBVARBINARY:
			binary_conversion = 1;
			if (destlen && !(destlen % 2))
				--destlen;
		}

		nDestSybType = TDS_CONVERT_CHAR;
		ores.cc.len = destlen;
		ores.cc.c = dest;
	}

	if (desttype == SQL_C_CHAR || desttype == SQL_C_WCHAR) {
		char buf[48];
		TDSDATEREC when;
		int prec = 3;
		const char *fmt = NULL;
		const TDS_DATETIMEALL *dta = (const TDS_DATETIMEALL *) src;

		switch (srctype) {
		case SYBMSDATETIMEOFFSET:
		case SYBMSDATETIME2: prec = dta->time_prec;
		case SYBDATETIME:  fmt = "%Y-%m-%d %H:%M:%S.%z"; if (prec) break;
		case SYBDATETIME4: fmt = "%Y-%m-%d %H:%M:%S"; break;
		case SYBMSTIME:
			prec = dta->time_prec;
			fmt = prec ? "%H:%M:%S.%z" : "%H:%M:%S";
			break;
		case SYBMSDATE:    fmt = "%Y-%m-%d"; break;
		}
		if (!fmt) goto normal_conversion;

		tds_datecrack(srctype, src, &when);
		tds_strftime(buf, sizeof(buf), fmt, &when, prec);

		if (srctype == SYBMSDATETIMEOFFSET) {
			char sign = '+';
			int off = dta->offset;
			if (off < 0) {
				sign = '-';
				off = -off;
			}
			sprintf(buf + strlen(buf), " %c%02d:%02d", sign, off / 60, off % 60);
		}

		nRetVal = strlen(buf);
		memcpy(dest, buf, destlen < nRetVal ? destlen : nRetVal);
	} else {
normal_conversion:
		nRetVal = tds_convert(context, srctype, src, srclen, nDestSybType, &ores);
	}
	if (nRetVal < 0) {
		odbc_convert_err_set(&stmt->errs, nRetVal);
		return SQL_NULL_DATA;
	}

	switch (desttype) {

	case SQL_C_CHAR:
		tdsdump_log(TDS_DBG_FUNC, "odbc_tds2sql: outputting character data destlen = %lu \n", (unsigned long) destlen);

		ret = nRetVal;
		/* TODO handle not terminated configuration */
		if (destlen > 0) {
			cplen = (destlen - 1) > nRetVal ? nRetVal : (destlen - 1);
			assert(cplen >= 0);
			/*
			 * odbc always terminate but do not overwrite 
			 * destination buffer more than needed
			 */
			/* update datapos only for binary source (char already handled) */
			if (curcol && binary_conversion)
				curcol->column_text_sqlgetdatapos += cplen / 2;
			dest[cplen] = 0;
		} else {
			/* if destlen == 0 we return only length */
		}
		break;

	case SQL_C_WCHAR:
		tdsdump_log(TDS_DBG_FUNC, "odbc_tds2sql: outputting character data destlen = %lu \n", (unsigned long) destlen);

		ret = nRetVal * sizeof(SQLWCHAR);
		/* TODO handle not terminated configuration */
		if (destlen > 0) {
			SQLWCHAR *wp = (SQLWCHAR *) dest;
			SQLCHAR  *p  = (SQLCHAR *)  dest;

			cplen = (destlen - 1) > nRetVal ? nRetVal : (destlen - 1);
			assert(cplen >= 0);
			/*
			 * odbc always terminate but do not overwrite 
			 * destination buffer more than needed
			 */
			/* update datapos only for binary source (char already handled) */
			if (curcol && binary_conversion)
				curcol->column_text_sqlgetdatapos += cplen / 2;
			/* convert in place and terminate */
			wp[cplen] = 0;
			while (cplen > 0) {
				--cplen;
				wp[cplen] = p[cplen];
			}
		} else {
			/* if destlen == 0 we return only length */
		}
		break;

	case SQL_C_TYPE_DATE:
	case SQL_C_DATE:
		{
			TDSDATEREC dr;
			DATE_STRUCT *dsp = (DATE_STRUCT *) dest;

			/*
			 * we've already converted the returned value to a SYBMSDATETIME2
			 * now decompose date into constituent parts...
			 */
			tds_datecrack(SYBMSDATETIME2, &(ores.dt), &dr);

			dsp->year = dr.year;
			dsp->month = dr.month + 1;
			dsp->day = dr.day;

			ret = sizeof(DATE_STRUCT);
		}
		break;

	case SQL_C_TYPE_TIME:
	case SQL_C_TIME:
		{
			TDSDATEREC dr;
			TIME_STRUCT *tsp = (TIME_STRUCT *) dest;

			/*
			 * we've already converted the returned value to a SYBMSDATETIME2
			 * now decompose date into constituent parts...
			 */
			tds_datecrack(SYBMSDATETIME2, &(ores.dt), &dr);

			tsp->hour = dr.hour;
			tsp->minute = dr.minute;
			tsp->second = dr.second;

			ret = sizeof(TIME_STRUCT);
		}
		break;

	case SQL_C_TYPE_TIMESTAMP:
	case SQL_C_TIMESTAMP: 
		{
			TDSDATEREC dr;
			TIMESTAMP_STRUCT *tssp = (TIMESTAMP_STRUCT *) dest;

			/*
			 * we've already converted the returned value to a SYBMSDATETIME2
			 * now decompose date into constituent parts...
			 */
			tds_datecrack(SYBMSDATETIME2, &(ores.dt), &dr);

			tssp->year = dr.year;
			tssp->month = dr.month + 1;
			tssp->day = dr.day;
			tssp->hour = dr.hour;
			tssp->minute = dr.minute;
			tssp->second = dr.second;
			tssp->fraction = dr.decimicrosecond * 100u;

			ret = sizeof(TIMESTAMP_STRUCT);
		}
		break;

#ifdef SQL_C_SBIGINT
	case SQL_C_SBIGINT:
	case SQL_C_UBIGINT:
		*((TDS_INT8 *) dest) = ores.bi;
		ret = sizeof(TDS_INT8);
		break;
#endif

	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
		*((TDS_INT *) dest) = ores.i;
		ret = sizeof(TDS_INT);
		break;

	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_USHORT:
		*((TDS_SMALLINT *) dest) = ores.si;
		ret = sizeof(TDS_SMALLINT);
		break;

	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
	case SQL_C_BIT:
		*((TDS_TINYINT *) dest) = ores.ti;
		ret = sizeof(TDS_TINYINT);
		break;

	case SQL_C_DOUBLE:
		*((TDS_FLOAT *) dest) = ores.f;
		ret = sizeof(TDS_FLOAT);
		break;

	case SQL_C_FLOAT:
		*((TDS_REAL *) dest) = ores.r;
		ret = sizeof(TDS_REAL);
		break;

	case SQL_C_NUMERIC:
		{
			/* ODBC numeric is quite different from TDS one ... */
			SQL_NUMERIC_STRUCT *num = (SQL_NUMERIC_STRUCT *) dest;
			num->precision = ores.n.precision;
			num->scale = ores.n.scale;
			num->sign = ores.n.array[0] ^ 1;
			/*
			 * TODO can be greater than SQL_MAX_NUMERIC_LEN ?? 
			 * seeing Sybase manual wire support bigger numeric but currently
			 * DBs so not support such precision
			 */
			i = ODBC_MIN(tds_numeric_bytes_per_prec[ores.n.precision] - 1, SQL_MAX_NUMERIC_LEN);
			memcpy(num->val, ores.n.array + 1, i);
			tds_swap_bytes(num->val, i);
			if (i < SQL_MAX_NUMERIC_LEN)
				memset(num->val + i, 0, SQL_MAX_NUMERIC_LEN - i);
			ret = sizeof(SQL_NUMERIC_STRUCT);
		}
		break;

#ifdef SQL_C_GUID
	case SQL_C_GUID:
		memcpy(dest, &(ores.u), sizeof(TDS_UNIQUE));
		ret = sizeof(TDS_UNIQUE);
		break;
#endif

	case SQL_C_BINARY:
		/* type already handled */
		assert(desttype != SQL_C_BINARY);

	default:
		break;

	}

	return ret;
}
