/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2005-2010  Frediano Ziglio
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <assert.h>

#include <freetds/odbc.h>
#include <freetds/iconv.h>
#include <freetds/string.h>
#include <freetds/convert.h>
#include <freetds/enum_cap.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: odbc_util.c,v 1.130 2011-09-07 09:40:47 freddy77 Exp $");

/**
 * \ingroup odbc_api
 * \defgroup odbc_util ODBC utility
 * Functions called within \c ODBC driver.
 */

/**
 * \addtogroup odbc_util
 * @{ 
 */

#ifdef ENABLE_ODBC_WIDE
static char *odbc_iso2utf(const char *s, int len);
static char *odbc_mb2utf(TDS_DBC *dbc, const char *s, int len);
static char *odbc_wide2utf(const SQLWCHAR *s, int len);
#endif

static char *
odbc_strndup(const char *s, int len)
{
	char *out = (char*) malloc(len+1);
	if (!out)
		return NULL;
	memcpy(out, s, len);
	out[len] = 0;
	return out;
}

static int
odbc_set_stmt(TDS_STMT * stmt, char **dest, const ODBC_CHAR *sql, int sql_len _WIDE)
{
	char *p;

	assert(dest == &stmt->prepared_query || dest == &stmt->query);

	if (sql_len == SQL_NTS)
#ifdef ENABLE_ODBC_WIDE
		sql_len = wide ? sqlwcslen(sql->wide) : strlen(sql->mb);
#else
		sql_len = strlen((const char*) sql);
#endif
	else if (sql_len <= 0)
		return SQL_ERROR;

	/* TODO already NULL ?? */
	tds_free_param_results(stmt->params);
	stmt->params = NULL;
	stmt->param_num = 0;
	stmt->param_count = 0;
	stmt->prepared_query_is_func = 0;
	stmt->prepared_query_is_rpc = 0;
	stmt->prepared_pos = NULL;
	stmt->curr_param_row = 0;
	stmt->num_param_rows = 1;
	stmt->need_reprepare = 0;

	if (stmt->prepared_query)
		TDS_ZERO_FREE(stmt->prepared_query);

	if (stmt->query)
		TDS_ZERO_FREE(stmt->query);

#ifdef ENABLE_ODBC_WIDE
	*dest = p = wide ? odbc_wide2utf(sql->wide, sql_len) : odbc_mb2utf(stmt->dbc, sql->mb, sql_len);
#else
	*dest = p = odbc_strndup((const char*) sql, sql_len);
#endif
	if (!p)
		return SQL_ERROR;

	return SQL_SUCCESS;
}

int
odbc_set_stmt_query(TDS_STMT * stmt, const ODBC_CHAR *sql, int sql_len _WIDE)
{
	return odbc_set_stmt(stmt, &stmt->query, sql, sql_len _wide);
}


int
odbc_set_stmt_prepared_query(TDS_STMT * stmt, const ODBC_CHAR *sql, int sql_len _WIDE)
{
	return odbc_set_stmt(stmt, &stmt->prepared_query, sql, sql_len _wide);
}

int
odbc_get_string_size(int size, ODBC_CHAR * str _WIDE)
{
	if (str) {
		if (size == SQL_NTS)
#ifdef ENABLE_ODBC_WIDE
			return wide ? sqlwcslen(str->wide) : strlen(str->mb);
#else
			return strlen((const char*) str);
#endif
		if (size >= 0)
			return size;
	}
	/* SQL_NULL_DATA or any other strange value */
	return 0;
}

#ifdef ENABLE_ODBC_WIDE
static char *
odbc_iso2utf(const char *s, int len)
{
	int i, o_len = len + 1;
	char *out, *p;

	assert(s && len >= 0);
	for (i = 0; i < len; ++i)
		if ((s[i] & 0x80) != 0)
			++o_len;

	out = (char *) malloc(o_len);
	if (!out) return NULL;

	for (p = out; len > 0; --len) {
		unsigned char u = (unsigned char) *s++;
		if ((u & 0x80) != 0) {
			*p++ = 0xc0 | (0x1f & (u >> 6));
			*p++ = 0x80 | (0x3f & u);
		} else {
			*p++ = u;
		}
	}
	*p = 0;
	assert(p+1-out <= o_len);
	return out;
}

static char *
odbc_wide2utf(const SQLWCHAR *s, int len)
{
	int i, o_len = len + 1;
	char *out, *p;
#if SIZEOF_SQLWCHAR > 2
# define MASK(n) ((0xffffffffu << (n)) & 0xffffffffu)
#else
# define MASK(n) ((0xffffu << (n)) & 0xffffu)
#endif

	assert(len >= 0 && (s || len == 0));
	for (i = 0; i < len; ++i) {
		if ((s[i] & MASK(7)) == 0)
			continue;
		++o_len;
		if ((s[i] & MASK(11)) == 0)
			continue;
		++o_len;
#if SIZEOF_SQLWCHAR > 2
		if ((s[i] & MASK(16)) == 0)
			continue;
		++o_len;
		if ((s[i] & MASK(21)) == 0)
			continue;
		++o_len;
		if ((s[i] & MASK(26)) == 0)
			continue;
		++o_len;
#endif
	}

	out = (char *) malloc(o_len);
	if (!out) return NULL;

	for (p = out; len > 0; --len) {
		SQLWCHAR u = *s++;
		if ((u & MASK(7)) == 0) {
			*p++ = u;
			continue;
		}
		if ((u & MASK(11)) == 0) {
			*p++ = 0xc0 | (0x1f & (u >> 6));
		} else {
#if SIZEOF_SQLWCHAR > 2
			if ((u & MASK(16)) == 0) {
				*p++ = 0xe0 | (0x0f & (u >> 12));
			} else {
				if ((u & MASK(21)) == 0) {
					if ((u & MASK(26)) == 0) {
						*p++ = 0xfc | (0x01 & (u >> 30));
						*p++ = 0x80 | (0x3f & (u >> 24));
					} else {
						*p++ = 0xf8 | (0x03 & (u >> 24));
					}
					*p++ = 0x80 | (0x3f & (u >> 18));
				} else {
					*p++ = 0xf0 | (0x07 & (u >> 18));
				}
				*p++ = 0x80 | (0x3f & (u >> 12));
			}
			*p++ = 0x80 | (0x3f & (u >> 6));
#else
			*p++ = 0xe0 | (0x0f & (u >> 12));
			*p++ = 0x80 | (0x3f & (u >> 6));
#endif
		}
		*p++ = 0x80 | (0x3f & u);
	}
	*p = 0;
	assert(p+1-out <= o_len);
	return out;
}

static char *
odbc_mb2utf(TDS_DBC *dbc, const char *s, int len)
{
	char *buf;

	const char *ib;
	char *ob;
	size_t il, ol;
	TDSICONV *char_conv = dbc->mb_conv;

	if (!char_conv)
		return odbc_iso2utf(s, len);

	if (char_conv->flags == TDS_ENCODING_MEMCPY)
		return odbc_strndup(s, len);

	il = len;

	/* allocate needed buffer (+1 is to exclude 0 case) */
	ol = il * char_conv->to.charset.max_bytes_per_char / char_conv->from.charset.min_bytes_per_char + 1;
	assert(ol > 0);
	buf = (char *) malloc(ol);
	if (!buf)
		return NULL;

	ib = s;
	ob = buf;
	--ol; /* leave space for terminator */

	/* char_conv is only mostly const */
	memset((TDS_ERRNO_MESSAGE_FLAGS*) &char_conv->suppress, 0, sizeof(char_conv->suppress));
	if (tds_iconv(dbc->tds_socket, char_conv, to_server, &ib, &il, &ob, &ol) == (size_t)-1) {
		free(buf);
		return NULL;
	}
	*ob = 0;
	return buf;
}
#endif

#ifdef ENABLE_ODBC_WIDE
DSTR*
odbc_dstr_copy_flag(TDS_DBC *dbc, DSTR *s, int size, ODBC_CHAR * str, int flag)
{
	int wide = flag&1;
	int len = odbc_get_string_size((flag&0x21) == 0x21 && size >= 0 ? size/SIZEOF_SQLWCHAR : size, str, wide);
	char *buf;

	if (wide)
		buf = odbc_wide2utf(str->wide, len);
	else
		buf = odbc_mb2utf(dbc, str->mb, len);
	if (!buf)
		return NULL;

	return tds_dstr_set(s, buf);
}
#else
DSTR*
odbc_dstr_copy(TDS_DBC *dbc, DSTR *s, int size, ODBC_CHAR * str)
{
	return tds_dstr_copyn(s, (const char *) str, odbc_get_string_size(size, str));
}
#endif

/**
 * Copy a string to client setting size according to ODBC convenction
 * @param dbc       database connection. Can be NULL
 * @param buffer    client buffer
 * @param cbBuffer  client buffer size (in bytes)
 * @param pcbBuffer pointer to SQLSMALLINT to hold string size
 * @param s         string to copy
 * @param len       len of string to copy. <0 null terminated
 * @param flag      set of flag 0x10 SQLINTEGER
 */
SQLRETURN
odbc_set_string_flag(TDS_DBC *dbc, SQLPOINTER buffer, SQLINTEGER cbBuffer, void FAR * pcbBuffer, const char *s, int len, int flag)
{
	SQLRETURN result = SQL_SUCCESS;
	int out_len = 0;
#if !defined(NDEBUG) && defined(ENABLE_ODBC_WIDE)
	size_t initial_size;
#endif

	if (len < 0)
		len = strlen(s);

#ifdef ENABLE_ODBC_WIDE
	if ((flag & 1) != 0) {
		/* wide characters */
		const unsigned char *p = (const unsigned char*) s;
		SQLWCHAR *dest = (SQLWCHAR*) buffer;

		if (cbBuffer < 0)
			cbBuffer = 0;
		if (flag&0x20)
			cbBuffer /= SIZEOF_SQLWCHAR;
#ifndef NDEBUG
		initial_size = cbBuffer;
#endif
		while (len) {
			unsigned char mask;
			unsigned u;
			int l;

			if (!(p[0] & 0x80)) {
				mask = 0x7f; l = 1;
			} else if ((p[0] & 0xe0) == 0xc0) {
				mask = 0x1f; l = 2;
			} else if ((p[0] & 0xf0) == 0xe0) {
				mask = 0x0f; l = 3;
			} else if ((p[0] & 0xf8) == 0xf0) {
				mask = 0x07; l = 4;
			} else if ((p[0] & 0xfc) == 0xf8) {
				mask = 0x03; l = 5;
			} else {
				mask = 0x7f; l = 1;
			}
			if (len < l)
				break;
			len -= l;
			u = *p++ & mask;
			while(--l)
				u = (u << 6) | (*p++ & 0x3f);
			++out_len;
			if (!dest)
				continue;
			if (cbBuffer > 1) {
				*dest++ = (SQLWCHAR) u;
				--cbBuffer;
				continue;
			}
			result = SQL_SUCCESS_WITH_INFO;
		}
		/* terminate buffer */
		assert(dest == NULL || dest-(SQLWCHAR*) buffer == out_len
		       || (dest-(SQLWCHAR*) buffer <= out_len && cbBuffer <= 1));
		if (dest && cbBuffer) {
			*dest++ = 0;
			assert(dest-(SQLWCHAR*) buffer <= initial_size);
		}
		assert(dest == NULL || dest-(SQLWCHAR*) buffer <= initial_size);
		if (flag&0x20)
			out_len *= SIZEOF_SQLWCHAR;
	} else if (!dbc || !dbc->mb_conv) {
		/* to ISO-8859-1 */
		const unsigned char *p = (const unsigned char*) s;
		unsigned char *dest = (unsigned char*) buffer;

		assert(cbBuffer >= 0);
#ifndef NDEBUG
		initial_size = cbBuffer;
#endif
		while (len) {
			unsigned char mask;
			unsigned u;
			int l;

			if (!(p[0] & 0x80)) {
				mask = 0x7f; l = 1;
			} else if ((p[0] & 0xe0) == 0xc0) {
				mask = 0x1f; l = 2;
			} else if ((p[0] & 0xf0) == 0xe0) {
				mask = 0x0f; l = 3;
			} else if ((p[0] & 0xf8) == 0xf0) {
				mask = 0x07; l = 4;
			} else if ((p[0] & 0xfc) == 0xf8) {
				mask = 0x03; l = 5;
			} else {
				mask = 0x7f; l = 1;
			}
			if (len < l)
				break;
			len -= l;
			u = *p++ & mask;
			while(--l)
				u = (u << 6) | (*p++ & 0x3f);
			++out_len;
			if (!dest)
				continue;
			if (cbBuffer > 1) {
				*dest++ = u > 0x100 ? '?' : u;
				--cbBuffer;
				continue;
			}
			result = SQL_SUCCESS_WITH_INFO;
		}
		assert(dest == NULL || dest-(unsigned char*) buffer == out_len
		       || (dest-(unsigned char*) buffer <= out_len && cbBuffer <= 1));
		/* terminate buffer */
		if (dest && cbBuffer) {
			*dest++ = 0;
			assert(dest-(unsigned char*) buffer <= initial_size);
		}
		assert(dest == NULL || dest-(unsigned char*) buffer <= initial_size);
	} else if (dbc->mb_conv->flags == TDS_ENCODING_MEMCPY) {
		/* to UTF-8 */
		out_len = len;
		if (len >= cbBuffer) {
			len = cbBuffer - 1;
			result = SQL_SUCCESS_WITH_INFO;
		}
		if (buffer && len >= 0) {
			/* buffer can overlap, use memmove, thanks to Valgrind */
			memmove((char *) buffer, s, len);
			((char *) buffer)[len] = 0;
		}
	} else {
		const char *ib;
		char *ob;
		size_t il, ol;
		TDSICONV *char_conv = dbc->mb_conv;

		il = len;
		ib = s;
		ol = cbBuffer;
		ob = (char *) buffer;

		/* char_conv is only mostly const */
		memset((TDS_ERRNO_MESSAGE_FLAGS*) &char_conv->suppress, 0, sizeof(char_conv->suppress));
		char_conv->suppress.e2big = 1;
		if (tds_iconv(dbc->tds_socket, char_conv, to_client, &ib, &il, &ob, &ol) == (size_t)-1)
			result = SQL_ERROR;
		out_len = cbBuffer - ol;
		while (result != SQL_ERROR && il) {
			char discard[128];
			ol = sizeof(discard);
			ob = discard;
			char_conv->suppress.e2big = 1;
			if (tds_iconv(dbc->tds_socket, char_conv, to_client, &ib, &il, &ob, &ol) == (size_t)-1)
				result = SQL_ERROR;
			if (out_len < cbBuffer) {
				int max_copy = out_len - cbBuffer;
				if (max_copy > sizeof(discard) - ol)
					max_copy = sizeof(discard) - ol;
				memcpy(((char *) buffer) + out_len, discard, max_copy);
			}
			out_len += sizeof(discard) - ol;
		}
		if (out_len >= cbBuffer && result != SQL_ERROR)
			result = SQL_SUCCESS_WITH_INFO;
		if (buffer && cbBuffer >= 0)
			((char *) buffer)[cbBuffer-1 < out_len ? cbBuffer-1:out_len] = 0;
	}
#else
	out_len = len;
	if (len >= cbBuffer) {
		len = cbBuffer - 1;
		result = SQL_SUCCESS_WITH_INFO;
	}
	if (buffer && len >= 0) {
		/* buffer can overlap, use memmove, thanks to Valgrind */
		memmove((char *) buffer, s, len);
		((char *) buffer)[len] = 0;
	}
#endif

	/* set output length */
	if (pcbBuffer) {
		if (flag & 0x10)
			*((SQLINTEGER *) pcbBuffer) = out_len;
		else
			*((SQLSMALLINT *) pcbBuffer) = out_len;
	}
	return result;
}


void
odbc_set_return_status(struct _hstmt *stmt, unsigned int n_row)
{
	TDSSOCKET *tds = stmt->tds;

	/* TODO handle different type results (functions) on mssql2k */
	if (stmt->prepared_query_is_func && tds->has_status) {
		struct _drecord *drec;
		int len;
		const TDS_DESC* axd = stmt->apd;
		TDS_INTPTR len_offset;
		char *data_ptr;

		if (axd->header.sql_desc_count < 1)
			return;
		drec = &axd->records[0];
		data_ptr = (char*) drec->sql_desc_data_ptr;

		if (axd->header.sql_desc_bind_type != SQL_BIND_BY_COLUMN) {
			len_offset = axd->header.sql_desc_bind_type * n_row;
			if (axd->header.sql_desc_bind_offset_ptr)
				len_offset += *axd->header.sql_desc_bind_offset_ptr;
			data_ptr += len_offset;
		} else {
			len_offset = sizeof(SQLLEN) * n_row;
			data_ptr += sizeof(SQLINTEGER) * n_row;
		}
#define LEN(ptr) *((SQLLEN*)(((char*)(ptr)) + len_offset))

		len = odbc_tds2sql(stmt, NULL, SYBINT4, (TDS_CHAR *) & tds->ret_status, sizeof(TDS_INT),
				   drec->sql_desc_concise_type, (TDS_CHAR *) data_ptr, drec->sql_desc_octet_length, NULL);
		if (len == SQL_NULL_DATA)
			return /* SQL_ERROR */ ;
		if (drec->sql_desc_indicator_ptr)
			LEN(drec->sql_desc_indicator_ptr) = 0;
		if (drec->sql_desc_octet_length_ptr)
			LEN(drec->sql_desc_octet_length_ptr) = len;
	}
#undef LEN
}

void
odbc_set_return_params(struct _hstmt *stmt, unsigned int n_row)
{
	TDSSOCKET *tds = stmt->tds;
	TDSPARAMINFO *info = tds->current_results;

	int i_begin = stmt->prepared_query_is_func ? 1 : 0;
	int i;
	int nparam = i_begin;

	/* I don't understand why but this happen -- freddy77 */
	/* TODO check why, put an assert ? */
	if (!info)
		return;

	for (i = 0; i < info->num_cols; ++i) {
		const TDS_DESC* axd = stmt->apd;
		const struct _drecord *drec_apd, *drec_ipd;
		TDSCOLUMN *colinfo = info->columns[i];
		TDS_CHAR *src;
		int srclen;
		SQLINTEGER len;
		int c_type;
		char *data_ptr;
		TDS_INTPTR len_offset;

		/* find next output parameter */
		for (;;) {
			drec_apd = NULL;
			/* TODO best way to stop */
			if (nparam >= axd->header.sql_desc_count || nparam >= stmt->ipd->header.sql_desc_count)
				return;
			drec_apd = &axd->records[nparam];
			drec_ipd = &stmt->ipd->records[nparam];
			if (stmt->ipd->records[nparam++].sql_desc_parameter_type != SQL_PARAM_INPUT)
				break;
		}

		data_ptr = (char*) drec_apd->sql_desc_data_ptr;
		if (axd->header.sql_desc_bind_type != SQL_BIND_BY_COLUMN) {
			len_offset = axd->header.sql_desc_bind_type * n_row;
			if (axd->header.sql_desc_bind_offset_ptr)
				len_offset += *axd->header.sql_desc_bind_offset_ptr;
			data_ptr += len_offset;
		} else {
			len_offset = sizeof(SQLLEN) * n_row;
			data_ptr += odbc_get_octet_len(drec_apd->sql_desc_concise_type, drec_apd) * n_row;
		}
#define LEN(ptr) *((SQLLEN*)(((char*)(ptr)) + len_offset))

		/* null parameter ? */
		if (colinfo->column_cur_size < 0) {
			/* FIXME error if NULL */
			if (drec_apd->sql_desc_indicator_ptr)
				LEN(drec_apd->sql_desc_indicator_ptr) = SQL_NULL_DATA;
			continue;
		}

		src = (TDS_CHAR *) colinfo->column_data;
		colinfo->column_text_sqlgetdatapos = 0;
		srclen = colinfo->column_cur_size;
		c_type = drec_apd->sql_desc_concise_type;
		if (c_type == SQL_C_DEFAULT)
			c_type = odbc_sql_to_c_type_default(drec_ipd->sql_desc_concise_type);
		/* 
		 * TODO why IPD ?? perhaps SQLBindParameter it's not correct ??
		 * Or tests are wrong ??
		 */
		len = odbc_tds2sql(stmt, colinfo, tds_get_conversion_type(colinfo->on_server.column_type, colinfo->on_server.column_size), src, srclen,
				   c_type, (TDS_CHAR*) data_ptr, drec_apd->sql_desc_octet_length, drec_ipd);
		if (len == SQL_NULL_DATA)
			return /* SQL_ERROR */ ;
		if (drec_apd->sql_desc_indicator_ptr)
			LEN(drec_apd->sql_desc_indicator_ptr) = 0;
		if (drec_apd->sql_desc_octet_length_ptr)
			LEN(drec_apd->sql_desc_octet_length_ptr) = len;
#undef LEN
	}
}

/**
 * Convert type from database to ODBC
 */
SQLSMALLINT
odbc_server_to_sql_type(int col_type, int col_size)
{
	/* FIXME finish */
	switch ((TDS_SERVER_TYPE) tds_get_conversion_type(col_type, col_size)) {
	case XSYBCHAR:
	case SYBCHAR:
		return SQL_CHAR;
	case XSYBVARCHAR:
	case SYBVARCHAR:
		return SQL_VARCHAR;
	case SYBTEXT:
		return SQL_LONGVARCHAR;
	case XSYBNCHAR:
		return SQL_WCHAR;
	/* TODO really sure ?? SYBNVARCHAR sybase only ?? */
	case SYBNVARCHAR:
	case XSYBNVARCHAR:
		return SQL_WVARCHAR;
	case SYBNTEXT:
		return SQL_WLONGVARCHAR;
	case SYBBIT:
		return SQL_BIT;
#if (ODBCVER >= 0x0300)
	case SYBUINT8:
	case SYB5INT8:
	case SYBINT8:
		/* TODO return numeric for odbc2 and convert bigint to numeric */
		return SQL_BIGINT;
#endif
	case SYBINT4:
	case SYBUINT4:
		return SQL_INTEGER;
	case SYBUINT2:
	case SYBINT2:
		return SQL_SMALLINT;
	case SYBUINT1:
	case SYBSINT1:
	case SYBINT1:
		return SQL_TINYINT;
	case SYBREAL:
		return SQL_REAL;
	case SYBFLT8:
		return SQL_DOUBLE;
	case SYBMONEY:
	case SYBMONEY4:
		return SQL_DECIMAL;
	case SYBMSTIME:
		return SQL_SS_TIME2;
	case SYBMSDATE:
		return SQL_TYPE_DATE;
	case SYBMSDATETIMEOFFSET:
		return SQL_SS_TIMESTAMPOFFSET;
	case SYBMSDATETIME2:
		return SQL_TYPE_TIMESTAMP;
	case SYBDATETIME:
	case SYBDATETIME4:
#if (ODBCVER >= 0x0300)
		return SQL_TYPE_TIMESTAMP;
#else
		return SQL_TIMESTAMP;
#endif
	case XSYBBINARY:
	case SYBBINARY:
		return SQL_BINARY;
	case SYBLONGBINARY:
	case SYBIMAGE:
		return SQL_LONGVARBINARY;
	case XSYBVARBINARY:
	case SYBVARBINARY:
		return SQL_VARBINARY;
	case SYBNUMERIC:
	case SYBDECIMAL:
		return SQL_NUMERIC;
#if (ODBCVER >= 0x0300)
	case SYBUNIQUE:
#ifdef SQL_GUID
		return SQL_GUID;
#else
		return SQL_CHAR;
#endif
	case SYBVARIANT:
		break;
#endif
	case SYBMSXML:
		return SQL_CHAR;
		/*
		 * TODO what should I do with these types ?? 
		 * return other types can cause additional problems
		 */
	case SYBVOID:
	case SYBDATE:
	case SYBDATEN:
	case SYBINTERVAL:
	case SYBTIME:
	case SYBTIMEN:
	case SYBUNITEXT:
	case SYBXML:
	case SYBMSUDT:
		/* these types are handled by tds_get_conversion_type */
	case SYBINTN:
	case SYBBITN:
	case SYBFLTN:
	case SYBMONEYN:
	case SYBDATETIMN:
	case SYBUINTN:
		break;
	}
	return SQL_UNKNOWN_TYPE;
}

/**
 * Pass this an SQL_C_* type and get a SYB* type which most closely corresponds
 * to the SQL_C_* type.
 * This function can return XSYBNVARCHAR or SYBUINTx even if server do not support it
 */
int
odbc_c_to_server_type(int c_type)
{
	switch (c_type) {
		/* FIXME this should be dependent on size of data !!! */
	case SQL_C_BINARY:
		return SYBBINARY;
#ifdef SQL_C_WCHAR
	case SQL_C_WCHAR:
		return XSYBNVARCHAR;
#endif
		/* TODO what happen if varchar is more than 255 characters long */
	case SQL_C_CHAR:
		return SYBVARCHAR;
	case SQL_C_FLOAT:
		return SYBREAL;
	case SQL_C_DOUBLE:
		return SYBFLT8;
	case SQL_C_BIT:
		return SYBBIT;
#if (ODBCVER >= 0x0300)
	case SQL_C_UBIGINT:
		return SYBUINT8;
	case SQL_C_SBIGINT:
		return SYBINT8;
#ifdef SQL_C_GUID
	case SQL_C_GUID:
		return SYBUNIQUE;
#endif
#endif
	case SQL_C_ULONG:
		return SYBUINT4;
	case SQL_C_LONG:
	case SQL_C_SLONG:
		return SYBINT4;
	case SQL_C_USHORT:
		return SYBUINT2;
	case SQL_C_SHORT:
	case SQL_C_SSHORT:
		return SYBINT2;
	case SQL_C_STINYINT:
	case SQL_C_TINYINT:
	case SQL_C_UTINYINT:
		return SYBINT1;
		/* ODBC date formats are completely differect from SQL one */
	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_C_TYPE_DATE:
	case SQL_C_TYPE_TIME:
	case SQL_C_TYPE_TIMESTAMP:
		return SYBMSDATETIME2;
		/* ODBC numeric/decimal formats are completely differect from tds one */
	case SQL_C_NUMERIC:
		return SYBNUMERIC;
		/* not supported */
	case SQL_C_INTERVAL_YEAR:
	case SQL_C_INTERVAL_MONTH:
	case SQL_C_INTERVAL_DAY:
	case SQL_C_INTERVAL_HOUR:
	case SQL_C_INTERVAL_MINUTE:
	case SQL_C_INTERVAL_SECOND:
	case SQL_C_INTERVAL_YEAR_TO_MONTH:
	case SQL_C_INTERVAL_DAY_TO_HOUR:
	case SQL_C_INTERVAL_DAY_TO_MINUTE:
	case SQL_C_INTERVAL_DAY_TO_SECOND:
	case SQL_C_INTERVAL_HOUR_TO_MINUTE:
	case SQL_C_INTERVAL_HOUR_TO_SECOND:
	case SQL_C_INTERVAL_MINUTE_TO_SECOND:
		break;
	}
	return 0;
}

void
odbc_set_sql_type_info(TDSCOLUMN * col, struct _drecord *drec, SQLINTEGER odbc_ver)
{
#define SET_INFO(type, prefix, suffix) do { \
	drec->sql_desc_literal_prefix = prefix; \
	drec->sql_desc_literal_suffix = suffix; \
	drec->sql_desc_type_name = type; \
	return; \
	} while(0)
#define SET_INFO2(type, prefix, suffix, len) do { \
	drec->sql_desc_length = (len); \
	SET_INFO(type, prefix, suffix); \
	} while(0)

	drec->sql_desc_unsigned = SQL_FALSE;
	drec->sql_desc_octet_length = drec->sql_desc_length = col->on_server.column_size;

	switch (tds_get_conversion_type(col->column_type, col->column_size)) {
	case XSYBCHAR:
	case SYBCHAR:
		if (col->on_server.column_type == XSYBNCHAR)
			SET_INFO2("nchar", "'", "'", col->on_server.column_size / 2);
		SET_INFO("char", "'", "'");
	case XSYBVARCHAR:
	case SYBVARCHAR:
		if (col->on_server.column_type == SYBNVARCHAR || col->on_server.column_type == XSYBNVARCHAR)
			SET_INFO2("nvarchar", "'", "'", col->on_server.column_size / 2);
		SET_INFO("varchar", "'", "'");
	case SYBTEXT:
		if (col->on_server.column_type == SYBNTEXT)
			SET_INFO2("ntext", "'", "'", col->on_server.column_size / 2);
		SET_INFO("text", "'", "'");
	case SYBBIT:
	case SYBBITN:
		drec->sql_desc_unsigned = SQL_TRUE;
		SET_INFO2("bit", "", "", 1);
#if (ODBCVER >= 0x0300)
	case SYBINT8:
		/* TODO return numeric for odbc2 and convert bigint to numeric */
		SET_INFO2("bigint", "", "", 19);
#endif
	case SYBINT4:
		SET_INFO2("int", "", "", 10);
	case SYBINT2:
		SET_INFO2("smallint", "", "", 5);
	case SYBUINT1:
	case SYBINT1:
		drec->sql_desc_unsigned = SQL_TRUE;
		SET_INFO2("tinyint", "", "", 3);
#if (ODBCVER >= 0x0300)
	case SYBUINT8:
		drec->sql_desc_unsigned = SQL_TRUE;
		/* TODO return numeric for odbc2 and convert bigint to numeric */
		SET_INFO2("unsigned bigint", "", "", 19);
#endif
	case SYBUINT4:
		drec->sql_desc_unsigned = SQL_TRUE;
		SET_INFO2("unsigned int", "", "", 10);
	case SYBUINT2:
		drec->sql_desc_unsigned = SQL_TRUE;
		SET_INFO2("unsigned smallint", "", "", 5);
	case SYBREAL:
		SET_INFO2("real", "", "", odbc_ver == SQL_OV_ODBC3 ? 24 : 7);
	case SYBFLT8:
		SET_INFO2("float", "", "", odbc_ver == SQL_OV_ODBC3 ? 53 : 15);
	case SYBMONEY:
		drec->sql_desc_octet_length = 21;
		SET_INFO2("money", "$", "", 19);
	case SYBMONEY4:
		drec->sql_desc_octet_length = 12;
		SET_INFO2("money", "$", "", 10);
	case SYBDATETIME:
		drec->sql_desc_octet_length = sizeof(TIMESTAMP_STRUCT);
		SET_INFO2("datetime", "'", "'", 23);
	case SYBDATETIME4:
		drec->sql_desc_octet_length = sizeof(TIMESTAMP_STRUCT);
		SET_INFO2("datetime", "'", "'", 16);
	case SYBBINARY:
		/* handle TIMESTAMP using usertype */
		if (col->column_usertype == 80)
			SET_INFO("timestamp", "0x", "");
		SET_INFO("binary", "0x", "");
	case SYBIMAGE:
		SET_INFO("image", "0x", "");
	case SYBVARBINARY:
		SET_INFO("varbinary", "0x", "");
	case SYBNUMERIC:
		drec->sql_desc_octet_length = col->column_prec + 2;
		SET_INFO2("numeric", "", "", col->column_prec);
	case SYBDECIMAL:
		drec->sql_desc_octet_length = col->column_prec + 2;
		SET_INFO2("decimal", "", "", col->column_prec);
	case SYBINTN:
	case SYBDATETIMN:
	case SYBFLTN:
	case SYBMONEYN:
		assert(0);
	case SYBVOID:
	case SYBNTEXT:
	case SYBNVARCHAR:
	case XSYBNVARCHAR:
	case XSYBNCHAR:
		break;
#if (ODBCVER >= 0x0300)
	case SYBUNIQUE:
		/* FIXME for Sybase ?? */
		SET_INFO2("uniqueidentifier", "'", "'", 36);
	case SYBVARIANT:
		SET_INFO("sql_variant", "", "");
		break;
#endif
	case SYBMSDATETIMEOFFSET:
		SET_INFO2("datetimeoffset", "'", "'", col->column_prec + 27);
	case SYBMSDATETIME2:
		SET_INFO2("datetime2", "'", "'", col->column_prec + 20);
	case SYBMSTIME:
		SET_INFO2("time", "'", "'", col->column_prec + 9);
	case SYBMSDATE:
		SET_INFO2("date", "'", "'", 10);
	case SYBMSXML:
		SET_INFO("xml", "'", "'");
	}
	SET_INFO("", "", "");
#undef SET_INFO
#undef SET_INFO2
}

SQLINTEGER
odbc_sql_to_displaysize(int sqltype, TDSCOLUMN *col)
{
	SQLINTEGER size = 0;

	switch (sqltype) {
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
		size = col->on_server.column_size;
		break;
	/* FIXME sure ?? *2 or not ?? */
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
		size = col->on_server.column_size / 2;
		break;
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
		size = col->column_size * 2;
		break;
	case SQL_BIGINT:
		size = 20;
		break;
	case SQL_INTEGER:
		size = 11;	/* -1000000000 */
		break;
	case SQL_SMALLINT:
		size = 6;	/* -10000 */
		break;
	case SQL_BIT:
		size = 1;
		break;
	case SQL_TINYINT:
		size = 3;	/* 255 */
		break;
	case SQL_DECIMAL:
	case SQL_NUMERIC:
		/* TODO check money format returned by propretary ODBC, scale == 4 but we use 2 digits */
		if (col->column_type == SYBMONEY || (col->column_type == SYBMONEYN && col->column_size == 8))
			size = 21;
		else if (col->column_type == SYBMONEY4 || (col->column_type == SYBMONEYN && col->column_size == 4))
			size = 12;
		else
			size = col->column_prec + 2;
		break;
	case SQL_DATE:
	case SQL_TYPE_DATE:
		/* FIXME check always yyyy-mm-dd ?? */
		size = 19;
		break;
	case SQL_TIME:
	case SQL_TYPE_TIME:
		/* FIXME check always hh:mm:ss[.fff] */
		size = 19;
		break;
	case SQL_TYPE_TIMESTAMP:
	case SQL_TIMESTAMP:
		/* TODO dependent on precision (decimal second digits) */
		/* we always format using yyyy-mm-dd hh:mm:ss[.fff], see convert_tds2sql.c */
		size = 19;
		if (col->column_type == SYBDATETIME || (col->column_type == SYBDATETIMN && col->column_size == 8))
			size = 23;
		break;
	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
		/* TODO check REAL/FLOAT format */
		if (col->column_type == SYBREAL || (col->column_type == SYBFLTN && col->column_size == 4))
			size = 14;
		else 
			size = 24;	/* FIXME -- what should the correct size be? */
		break;
#ifdef SQL_GUID
	case SQL_GUID:
		size = 36;
		break;
#endif
	default:
		/* FIXME TODO finish, should support ALL types (interval, binary) */
		size = 40;
		tdsdump_log(TDS_DBG_INFO1, "odbc_sql_to_displaysize: unknown sql type %d\n", (int) sqltype);
		break;
	}
	return size;
}

int
odbc_sql_to_c_type_default(int sql_type)
{

	switch (sql_type) {

	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	/* these types map to SQL_C_CHAR for compatibility with old applications */
#ifdef SQL_C_WCHAR
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
#endif
		return SQL_C_CHAR;
		/* for compatibility numeric are converted to CHAR, not to structure */
	case SQL_DECIMAL:
	case SQL_NUMERIC:
		return SQL_C_CHAR;
#ifdef SQL_GUID
	case SQL_GUID:
		/* TODO return SQL_C_CHAR for Sybase ?? */
		return SQL_C_GUID;
#endif
	case SQL_BIT:
		return SQL_C_BIT;
	case SQL_TINYINT:
		return SQL_C_UTINYINT;
	case SQL_SMALLINT:
		return SQL_C_SSHORT;
	case SQL_INTEGER:
		return SQL_C_SLONG;
	case SQL_BIGINT:
		return SQL_C_SBIGINT;
	case SQL_REAL:
		return SQL_C_FLOAT;
	case SQL_FLOAT:
	case SQL_DOUBLE:
		return SQL_C_DOUBLE;
	case SQL_DATE:
	case SQL_TYPE_DATE:
		return SQL_C_TYPE_DATE;
	case SQL_TIME:
	case SQL_TYPE_TIME:
		return SQL_C_TYPE_TIME;
	case SQL_TIMESTAMP:
	case SQL_TYPE_TIMESTAMP:
		return SQL_C_TYPE_TIMESTAMP;
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
		return SQL_C_BINARY;
		/* TODO interval types */
	default:
		return 0;
	}
}

int
odbc_sql_to_server_type(TDSCONNECTION * conn, int sql_type, int sql_unsigned)
{

	switch (sql_type) {
	case SQL_WCHAR:
		if (IS_TDS7_PLUS(conn))
			return XSYBNCHAR;
	case SQL_CHAR:
		return SYBCHAR;
	case SQL_WVARCHAR:
		if (IS_TDS7_PLUS(conn))
			return XSYBNVARCHAR;
	case SQL_VARCHAR:
		return SYBVARCHAR;
	case SQL_WLONGVARCHAR:
		if (IS_TDS7_PLUS(conn))
			return SYBNTEXT;
	case SQL_LONGVARCHAR:
		return SYBTEXT;
	case SQL_DECIMAL:
		return SYBDECIMAL;
	case SQL_NUMERIC:
		return SYBNUMERIC;
#ifdef SQL_GUID
	case SQL_GUID:
		if (IS_TDS7_PLUS(conn))
			return SYBUNIQUE;
		return 0;
#endif
	case SQL_BIT:
		if (IS_TDS7_PLUS(conn))
			return SYBBITN;
		return SYBBIT;
	case SQL_TINYINT:
		return SYBINT1;
	case SQL_SMALLINT:
		if (sql_unsigned && tds_capability_has_req(conn, TDS_REQ_DATA_UINT2))
			return SYBUINT2;
		return SYBINT2;
	case SQL_INTEGER:
		if (sql_unsigned && tds_capability_has_req(conn, TDS_REQ_DATA_UINT4))
			return SYBUINT4;
		return SYBINT4;
	case SQL_BIGINT:
		if (sql_unsigned && tds_capability_has_req(conn, TDS_REQ_DATA_UINT8))
			return SYBUINT8;
		return SYBINT8;
	case SQL_REAL:
		return SYBREAL;
	case SQL_FLOAT:
	case SQL_DOUBLE:
		return SYBFLT8;
		/* ODBC version 2 */
	case SQL_DATE:
	case SQL_TIME:
	case SQL_TIMESTAMP:
		/* ODBC version 3 */
	case SQL_TYPE_DATE:
		if (IS_TDS73_PLUS(conn))
			return SYBMSDATE;
	case SQL_TYPE_TIME:
		if (IS_TDS73_PLUS(conn))
			return SYBMSTIME;
	case SQL_TYPE_TIMESTAMP:
		if (IS_TDS73_PLUS(conn))
			return SYBMSDATETIME2;
		return SYBDATETIME;
	case SQL_SS_TIME2:
		if (IS_TDS73_PLUS(conn))
			return SYBMSTIME;
		return SYBDATETIME;
	case SQL_SS_TIMESTAMPOFFSET:
		if (IS_TDS73_PLUS(conn))
			return SYBMSDATETIMEOFFSET;
		return SYBDATETIME;
	case SQL_BINARY:
		return SYBBINARY;
	case SQL_VARBINARY:
		return SYBVARBINARY;
	case SQL_LONGVARBINARY:
		return SYBIMAGE;
		/* TODO interval types */
	default:
		return 0;
	}
}

/** Returns the version of the RDBMS in the ODBC format */
void
odbc_rdbms_version(TDSSOCKET * tds, char *pversion_string)
{
	TDS_UINT version = tds_conn(tds)->product_version;
	sprintf(pversion_string, "%.02d.%.02d.%.04d", (int) ((version & 0x7F000000) >> 24),
		(int) ((version & 0x00FF0000) >> 16), (int) (version & 0x0000FFFF));
}

/** Return length of parameter from parameter information */
SQLINTEGER
odbc_get_param_len(const struct _drecord *drec_axd, const struct _drecord *drec_ixd, const TDS_DESC* axd, unsigned int n_row)
{
	SQLINTEGER len;
	int size;
	TDS_INTPTR len_offset;

	if (axd->header.sql_desc_bind_type != SQL_BIND_BY_COLUMN) {
		len_offset = axd->header.sql_desc_bind_type * n_row;
		if (axd->header.sql_desc_bind_offset_ptr)
			len_offset += *axd->header.sql_desc_bind_offset_ptr;
	} else {
		len_offset = sizeof(SQLLEN) * n_row;
	}
#define LEN(ptr) *((SQLLEN*)(((char*)(ptr)) + len_offset))

	if (drec_axd->sql_desc_indicator_ptr && LEN(drec_axd->sql_desc_indicator_ptr) == SQL_NULL_DATA)
		len = SQL_NULL_DATA;
	else if (drec_axd->sql_desc_octet_length_ptr)
		len = LEN(drec_axd->sql_desc_octet_length_ptr);
	else {
		len = 0;
		/* TODO add XML if defined */
		/* FIXME, other types available */
		if (drec_axd->sql_desc_concise_type == SQL_C_CHAR || drec_axd->sql_desc_concise_type == SQL_C_WCHAR
		    || drec_axd->sql_desc_concise_type == SQL_C_BINARY) {
			len = SQL_NTS;
		} else {
			int type = drec_axd->sql_desc_concise_type;

			if (type == SQL_C_DEFAULT)
				type = odbc_sql_to_c_type_default(drec_ixd->sql_desc_concise_type);
			type = odbc_c_to_server_type(type);

			/* FIXME check what happen to DATE/TIME types */
			size = tds_get_size_by_type(type);
			if (size > 0)
				len = size;
		}
	}
	return len;
#undef LEN
}

#ifdef SQL_GUID
# define TYPE_NORMAL_SQL_GUID TYPE_NORMAL(SQL_GUID)
#else
# define TYPE_NORMAL_SQL_GUID
#endif
#define SQL_TYPES \
	TYPE_NORMAL(SQL_BIT) \
	TYPE_NORMAL(SQL_SMALLINT) \
	TYPE_NORMAL(SQL_TINYINT) \
	TYPE_NORMAL(SQL_INTEGER) \
	TYPE_NORMAL(SQL_BIGINT) \
\
	TYPE_NORMAL_SQL_GUID \
\
	TYPE_NORMAL(SQL_BINARY) \
	TYPE_NORMAL(SQL_VARBINARY) \
	TYPE_NORMAL(SQL_LONGVARBINARY) \
\
	TYPE_NORMAL(SQL_CHAR) \
	TYPE_NORMAL(SQL_VARCHAR) \
	TYPE_NORMAL(SQL_LONGVARCHAR) \
	TYPE_NORMAL(SQL_WCHAR) \
	TYPE_NORMAL(SQL_WVARCHAR) \
	TYPE_NORMAL(SQL_WLONGVARCHAR) \
\
	TYPE_NORMAL(SQL_DECIMAL) \
	TYPE_NORMAL(SQL_NUMERIC) \
\
	TYPE_NORMAL(SQL_FLOAT) \
	TYPE_NORMAL(SQL_REAL) \
	TYPE_NORMAL(SQL_DOUBLE)\
\
	TYPE_NORMAL(SQL_SS_TIMESTAMPOFFSET) \
	TYPE_NORMAL(SQL_SS_TIME2) \
	TYPE_NORMAL(SQL_TYPE_DATE) \
\
	TYPE_VERBOSE_START(SQL_DATETIME) \
	TYPE_VERBOSE_DATE(SQL_DATETIME, SQL_CODE_TIMESTAMP, SQL_TYPE_TIMESTAMP, SQL_TIMESTAMP) \
	TYPE_VERBOSE_END(SQL_DATETIME)

SQLSMALLINT
odbc_get_concise_sql_type(SQLSMALLINT type, SQLSMALLINT interval)
{
#define TYPE_NORMAL(t) case t: return type;
#define TYPE_VERBOSE_START(t) \
	case t: switch (interval) {
#define TYPE_VERBOSE_DATE(t, interval, concise, old) \
	case interval: return concise;
#define TYPE_VERBOSE_END(t) \
	}

	switch (type) {
		SQL_TYPES;
	}
	return 0;
#undef TYPE_NORMAL
#undef TYPE_VERBOSE_START
#undef TYPE_VERBOSE_DATE
#undef TYPE_VERBOSE_END
}

/**
 * Set concise type and all cascading field.
 * @param concise_type concise type to set
 * @param drec         record to set. NULL to test error without setting
 * @param check_only   it <>0 (true) check only, do not set type
 */
SQLRETURN
odbc_set_concise_sql_type(SQLSMALLINT concise_type, struct _drecord * drec, int check_only)
{
	SQLSMALLINT type = concise_type, interval_code = 0;

#define TYPE_NORMAL(t) case t: break;
#define TYPE_VERBOSE_START(t)
#define TYPE_VERBOSE_DATE(t, interval, concise, old) \
	case old: concise_type = concise; \
	case concise: type = t; interval_code = interval; break;
#define TYPE_VERBOSE_END(t)

	switch (type) {
		SQL_TYPES;
	default:
		return SQL_ERROR;
	}
	if (!check_only) {
		drec->sql_desc_concise_type = concise_type;
		drec->sql_desc_type = type;
		drec->sql_desc_datetime_interval_code = interval_code;

		switch (drec->sql_desc_type) {
		case SQL_NUMERIC:
		case SQL_DECIMAL:
			drec->sql_desc_precision = 38;
			drec->sql_desc_scale = 0;
			break;
			/* TODO finish */
		}
	}
	return SQL_SUCCESS;
#undef TYPE_NORMAL
#undef TYPE_VERBOSE_START
#undef TYPE_VERBOSE_DATE
#undef TYPE_VERBOSE_END
}

#ifdef SQL_C_GUID
# define TYPE_NORMAL_SQL_C_GUID TYPE_NORMAL(SQL_C_GUID)
#else
# define TYPE_NORMAL_SQL_C_GUID
#endif
#define C_TYPES \
	TYPE_NORMAL(SQL_C_BIT) \
	TYPE_NORMAL(SQL_C_SHORT) \
	TYPE_NORMAL(SQL_C_TINYINT) \
	TYPE_NORMAL(SQL_C_UTINYINT) \
	TYPE_NORMAL(SQL_C_STINYINT) \
	TYPE_NORMAL(SQL_C_LONG) \
	TYPE_NORMAL(SQL_C_SBIGINT) \
	TYPE_NORMAL(SQL_C_UBIGINT) \
	TYPE_NORMAL(SQL_C_SSHORT) \
	TYPE_NORMAL(SQL_C_SLONG) \
	TYPE_NORMAL(SQL_C_USHORT) \
	TYPE_NORMAL(SQL_C_ULONG) \
\
	TYPE_NORMAL_SQL_C_GUID \
	TYPE_NORMAL(SQL_C_DEFAULT) \
\
	TYPE_NORMAL(SQL_C_BINARY) \
\
	TYPE_NORMAL(SQL_C_CHAR) \
	TYPE_NORMAL(SQL_C_WCHAR) \
\
	TYPE_NORMAL(SQL_C_NUMERIC) \
\
	TYPE_NORMAL(SQL_C_FLOAT) \
	TYPE_NORMAL(SQL_C_DOUBLE)\
\
	TYPE_VERBOSE_START(SQL_DATETIME) \
	TYPE_VERBOSE_DATE(SQL_DATETIME, SQL_CODE_DATE, SQL_C_TYPE_DATE, SQL_C_DATE) \
	TYPE_VERBOSE_DATE(SQL_DATETIME, SQL_CODE_TIME, SQL_C_TYPE_TIME, SQL_C_TIME) \
	TYPE_VERBOSE_DATE(SQL_DATETIME, SQL_CODE_TIMESTAMP, SQL_C_TYPE_TIMESTAMP, SQL_C_TIMESTAMP) \
	TYPE_VERBOSE_END(SQL_DATETIME) \
\
	TYPE_VERBOSE_START(SQL_INTERVAL) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_DAY, SQL_C_INTERVAL_DAY) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_DAY_TO_HOUR, SQL_C_INTERVAL_DAY_TO_HOUR) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_DAY_TO_MINUTE, SQL_C_INTERVAL_DAY_TO_MINUTE) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_DAY_TO_SECOND, SQL_C_INTERVAL_DAY_TO_SECOND) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_HOUR, SQL_C_INTERVAL_HOUR) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_HOUR_TO_MINUTE, SQL_C_INTERVAL_HOUR_TO_MINUTE) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_HOUR_TO_SECOND, SQL_C_INTERVAL_HOUR_TO_SECOND) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_MINUTE, SQL_C_INTERVAL_MINUTE) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_MINUTE_TO_SECOND, SQL_C_INTERVAL_MINUTE_TO_SECOND) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_MONTH, SQL_C_INTERVAL_MONTH) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_SECOND, SQL_C_INTERVAL_SECOND) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_YEAR, SQL_C_INTERVAL_YEAR) \
	TYPE_VERBOSE(SQL_INTERVAL, SQL_CODE_YEAR_TO_MONTH, SQL_C_INTERVAL_YEAR_TO_MONTH) \
	TYPE_VERBOSE_END(SQL_INTERVAL)

SQLSMALLINT
odbc_get_concise_c_type(SQLSMALLINT type, SQLSMALLINT interval)
{
#define TYPE_NORMAL(t) case t: return type;
#define TYPE_VERBOSE_START(t) \
	case t: switch (interval) {
#define TYPE_VERBOSE(t, interval, concise) \
	case interval: return concise;
#define TYPE_VERBOSE_DATE(t, interval, concise, old) \
	case interval: return concise;
#define TYPE_VERBOSE_END(t) \
	}

	switch (type) {
		C_TYPES;
	}
	return 0;
#undef TYPE_NORMAL
#undef TYPE_VERBOSE_START
#undef TYPE_VERBOSE
#undef TYPE_VERBOSE_DATE
#undef TYPE_VERBOSE_END
}

/**
 * Set concise type and all cascading field.
 * @param concise_type concise type to set
 * @param drec         record to set. NULL to test error without setting
 * @param check_only   it <>0 (true) check only, do not set type
 */
SQLRETURN
odbc_set_concise_c_type(SQLSMALLINT concise_type, struct _drecord * drec, int check_only)
{
	SQLSMALLINT type = concise_type, interval_code = 0;

#define TYPE_NORMAL(t) case t: break;
#define TYPE_VERBOSE_START(t)
#define TYPE_VERBOSE(t, interval, concise) \
	case concise: type = t; interval_code = interval; break;
#define TYPE_VERBOSE_DATE(t, interval, concise, old) \
	case concise: type = t; interval_code = interval; break; \
	case old: concise_type = concise; type = t; interval_code = interval; break;
#define TYPE_VERBOSE_END(t)

	switch (type) {
		C_TYPES;
	default:
		return SQL_ERROR;
	}
	if (!check_only) {
		drec->sql_desc_concise_type = concise_type;
		drec->sql_desc_type = type;
		drec->sql_desc_datetime_interval_code = interval_code;

		switch (drec->sql_desc_type) {
		case SQL_C_NUMERIC:
			drec->sql_desc_length = 38;
			drec->sql_desc_precision = 38;
			drec->sql_desc_scale = 0;
			break;
			/* TODO finish */
		}
	}
	return SQL_SUCCESS;
#undef TYPE_NORMAL
#undef TYPE_VERBOSE_START
#undef TYPE_VERBOSE
#undef TYPE_VERBOSE_DATE
#undef TYPE_VERBOSE_END
}

SQLLEN
odbc_get_octet_len(int c_type, const struct _drecord *drec)
{
	SQLLEN len;

	/* this shit is mine -- freddy77 */
	switch (c_type) {
	case SQL_C_CHAR:
	case SQL_C_WCHAR:
	case SQL_C_BINARY:
		len = drec->sql_desc_octet_length;
		break;
	case SQL_C_DATE:
	case SQL_C_TYPE_DATE:
		len = sizeof(DATE_STRUCT);
		break;
	case SQL_C_TIME:
	case SQL_C_TYPE_TIME:
		len = sizeof(TIME_STRUCT);
		break;
	case SQL_C_TIMESTAMP:
	case SQL_C_TYPE_TIMESTAMP:
		len = sizeof(TIMESTAMP_STRUCT);
		break;
	case SQL_C_NUMERIC:
		len = sizeof(SQL_NUMERIC_STRUCT);
		break;
	default:
		len = tds_get_size_by_type(odbc_c_to_server_type(c_type));
		break;
	}
	return len;
}

void
odbc_convert_err_set(struct _sql_errors *errs, TDS_INT err)
{
	/*
	 * TODO we really need a column offset in the _sql_error structure so that caller can
	 * invoke SQLGetDiagField to learn which column is failing
	 */
	switch (err) {
	case TDS_CONVERT_NOAVAIL:
		odbc_errs_add(errs, "HY003", NULL);
		break;
	case TDS_CONVERT_SYNTAX:
		odbc_errs_add(errs, "22018", NULL);
		break;
	case TDS_CONVERT_OVERFLOW:
		odbc_errs_add(errs, "22003", NULL);
		break;
	case TDS_CONVERT_FAIL:
		odbc_errs_add(errs, "07006", NULL);
		break;
	case TDS_CONVERT_NOMEM:
		odbc_errs_add(errs, "HY001", NULL);
		break;
	}
}

/** @} */
