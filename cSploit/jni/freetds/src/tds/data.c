/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2003-2011 Frediano Ziglio
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

/**
 * @file
 * @brief Handle different data handling from network
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <freetds/tds.h>
#include <freetds/bytes.h>
#include <freetds/iconv.h>
#include "tds_checks.h"
#include <freetds/stream.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: data.c,v 1.45 2011-10-30 16:47:18 freddy77 Exp $");

#define USE_ICONV tds_conn(tds)->use_iconv

int determine_adjusted_size(const TDSICONV * char_conv, int size);
static const TDSCOLUMNFUNCS *tds_get_column_funcs(TDSCONNECTION *conn, int type);

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/**
 * Set type of column initializing all dependency 
 * @param curcol column to set
 * @param type   type to set
 */
void
tds_set_column_type(TDSCONNECTION * conn, TDSCOLUMN * curcol, int type)
{
	/* set type */
	curcol->on_server.column_type = type;
	curcol->funcs = tds_get_column_funcs(conn, type);
	curcol->column_type = tds_get_cardinal_type(type, curcol->column_usertype);

	/* set size */
	curcol->column_cur_size = -1;
	curcol->column_varint_size = tds_get_varint_size(conn, type);
	if (curcol->column_varint_size == 0)
		curcol->column_cur_size = curcol->on_server.column_size = curcol->column_size = tds_get_size_by_type(type);

}

/**
 * Set type of column initializing all dependency
 * \param tds    state information for the socket and the TDS protocol
 * \param curcol column to set
 * \param type   type to set
 */
void
tds_set_param_type(TDSCONNECTION * conn, TDSCOLUMN * curcol, TDS_SERVER_TYPE type)
{
	if (IS_TDS7_PLUS(conn)) {
		switch (type) {
		case SYBVARCHAR:
			type = XSYBVARCHAR;
			break;
		case SYBCHAR:
			type = XSYBCHAR;
			break;
		case SYBVARBINARY:
			type = XSYBVARBINARY;
			break;
		case SYBBINARY:
			type = XSYBBINARY;
			break;
			/* avoid warning on other types */
		default:
			break;
		}
	} else if (IS_TDS50(conn)) {
		if (type == SYBINT8)
			type = SYB5INT8;
	}
	tds_set_column_type(conn, curcol, type);

	if (is_collate_type(type)) {
		curcol->char_conv = conn->char_convs[is_unicode_type(type) ? client2ucs2 : client2server_chardata];
		memcpy(curcol->column_collation, conn->collation, sizeof(conn->collation));
	}

	/* special case, GUID, varint != 0 but only a size */
	/* TODO VARIANT, when supported */
	switch (type) {
	case SYBUNIQUE:
		curcol->on_server.column_size = curcol->column_size = sizeof(TDS_UNIQUE);
		break;
	case SYBBITN:
		curcol->on_server.column_size = curcol->column_size = sizeof(TDS_TINYINT);
		break;
	/* mssql 2005 don't like SYBINT4 as parameter closing connection  */
	case SYBINT1:
	case SYBINT2:
	case SYBINT4:
	case SYBINT8:
		curcol->on_server.column_type = SYBINTN;
		curcol->column_varint_size = 1;
		curcol->column_cur_size = -1;
		break;
	case SYBMONEY4:
	case SYBMONEY:
		curcol->on_server.column_type = SYBMONEYN;
		curcol->column_varint_size = 1;
		curcol->column_cur_size = -1;
		break;
	case SYBDATETIME:
	case SYBDATETIME4:
		curcol->on_server.column_type = SYBDATETIMN;
		curcol->column_varint_size = 1;
		curcol->column_cur_size = -1;
		break;
	case SYBFLT8:
	case SYBREAL:
		curcol->on_server.column_type = SYBFLTN;
		curcol->column_varint_size = 1;
		curcol->column_cur_size = -1;
		break;
	case SYBNTEXT:
		if (IS_TDS72_PLUS(conn)) {
			curcol->column_varint_size = 8;
			curcol->on_server.column_type = XSYBNVARCHAR;
		}
		break;
	case SYBTEXT:
		if (IS_TDS72_PLUS(conn)) {
			curcol->column_varint_size = 8;
			curcol->on_server.column_type = XSYBVARCHAR;
		}
		break;
	case SYBIMAGE:
		if (IS_TDS72_PLUS(conn)) {
			curcol->column_varint_size = 8;
			curcol->on_server.column_type = XSYBVARBINARY;
		}
		break;
	default:
		break;
	}
}

int
tds_get_cardinal_type(int datatype, int usertype)
{
	switch (datatype) {
	case XSYBVARBINARY:
		return SYBVARBINARY;
	case XSYBBINARY:
		return SYBBINARY;
	case SYBNTEXT:
		return SYBTEXT;
	case XSYBNVARCHAR:
	case XSYBVARCHAR:
		return SYBVARCHAR;
	case XSYBNCHAR:
	case XSYBCHAR:
		return SYBCHAR;
	case SYB5INT8:
		return SYBINT8;
	case SYBLONGBINARY:
		switch (usertype) {
		case USER_UNICHAR_TYPE:
		case USER_UNIVARCHAR_TYPE:
			return SYBTEXT;
		}
		break;
	}
	return datatype;
}

static TDSRET
tds_generic_get_info(TDSSOCKET *tds, TDSCOLUMN *col)
{
	switch (col->column_varint_size) {
	case 8:
		col->column_size = 0x7ffffffflu;
		break;
	case 5:
	case 4:
		col->column_size = tds_get_int(tds);
		break;
	case 2:
		/* assure > 0 */
		col->column_size = tds_get_smallint(tds);
		/* under TDS9 this means ?var???(MAX) */
		if (col->column_size < 0 && IS_TDS72_PLUS(tds->conn)) {
			col->column_size = 0x3ffffffflu;
			col->column_varint_size = 8;
		}
		break;
	case 1:
		col->column_size = tds_get_byte(tds);
		break;
	case 0:
		col->column_size = tds_get_size_by_type(col->column_type);
		break;
	}

	if (IS_TDS71_PLUS(tds->conn) && is_collate_type(col->on_server.column_type)) {
		/* based on true type as sent by server */
		/*
		 * first 2 bytes are windows code (such as 0x409 for english)
		 * other 2 bytes ???
		 * last bytes is id in syscharsets
		 */
		tds_get_n(tds, col->column_collation, 5);
		col->char_conv =
			tds_iconv_from_collate(tds->conn, col->column_collation);
	}

	/* Only read table_name for blob columns (eg. not for SYBLONGBINARY) */
	if (is_blob_type(col->on_server.column_type)) {
		/* discard this additional byte */
		if (IS_TDS72_PLUS(tds->conn)) {
			unsigned char num_parts = tds_get_byte(tds);
			/* TODO do not discard first ones */
			for (; num_parts; --num_parts) {
				tds_dstr_get(tds, &col->table_name, tds_get_usmallint(tds));
			}
		} else {
			tds_dstr_get(tds, &col->table_name, tds_get_usmallint(tds));
		}
	} else if (IS_TDS72_PLUS(tds->conn) && col->on_server.column_type == SYBMSXML) {
		unsigned char has_schema = tds_get_byte(tds);
		if (has_schema) {
			/* discard schema informations */
			tds_get_string(tds, tds_get_byte(tds), NULL, 0);        /* dbname */
			tds_get_string(tds, tds_get_byte(tds), NULL, 0);        /* schema owner */
			tds_get_string(tds, tds_get_usmallint(tds), NULL, 0);    /* schema collection */
		}
	}
	return TDS_SUCCESS;
}

/* tds_generic_row_len support also variant and return size to hold blob */
TDS_COMPILE_CHECK(variant_size, sizeof(TDSBLOB) >= sizeof(TDSVARIANT));

static TDS_INT
tds_generic_row_len(TDSCOLUMN *col)
{
	CHECK_COLUMN_EXTRA(col);

	if (is_blob_col(col))
		return sizeof(TDSBLOB);
	return col->column_size;
}

typedef struct tds_varmax_stream {
	TDSINSTREAM stream;
	TDSSOCKET *tds;
	TDS_INT chunk_left;
} TDSVARMAXSTREAM;

static int
tds_varmax_stream_read(TDSINSTREAM *stream, void *ptr, size_t len)
{
	TDSVARMAXSTREAM *s = (TDSVARMAXSTREAM *) stream;

	/* read chunk len if needed */
	if (s->chunk_left == 0) {
		TDS_INT l = tds_get_int(s->tds);
		if (l <= 0) l = -1;
		s->chunk_left = l;
	}

	/* no more data ?? */
	if (s->chunk_left < 0)
		return 0;

	/* read part of data */
	if (len > s->chunk_left)
		len = s->chunk_left;
	s->chunk_left -= len;
	if (tds_get_n(s->tds, ptr, len))
		return len;
	return -1;
}

static TDSRET
tds72_get_varmax(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	TDS_INT8 len;
	TDSRET res;
	TDSVARMAXSTREAM r;
	TDSDYNAMICSTREAM w;

	len = tds_get_int8(tds);

	/* NULL */
	if (len == -1) {
		curcol->column_cur_size = -1;
		return TDS_SUCCESS;
	}

	r.stream.read = tds_varmax_stream_read;
	r.tds = tds;
	r.chunk_left = 0;

	res = tds_dynamic_stream_init(&w, (void**) &(((TDSBLOB*) curcol->column_data)->textvalue), 0);
	if (TDS_FAILED(res))
		return res;

	if (USE_ICONV && curcol->char_conv)
		res = tds_convert_stream(tds, curcol->char_conv, to_client, &r.stream, &w.stream);
	else
		res = tds_copy_stream(tds, &r.stream, &w.stream);
	if (TDS_FAILED(res))
		return res;

	curcol->column_cur_size = w.size;
	return TDS_SUCCESS;
}

TDS_COMPILE_CHECK(tds_variant_size,  sizeof(((TDSVARIANT*)0)->data) == sizeof(((TDSBLOB*)0)->textvalue));
TDS_COMPILE_CHECK(tds_variant_offset,TDS_OFFSET(TDSVARIANT, data) == TDS_OFFSET(TDSBLOB, textvalue));

/*
 * This strange type has following structure 
 * 0 len (int32) -- NULL 
 * len (int32), type (int8), data -- ints, date, etc
 * len (int32), type (int8), 7 (int8), collation, column size (int16) -- [n]char, [n]varchar, binary, varbinary 
 * BLOBS (text/image) not supported
 */
static TDSRET
tds_variant_get(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	int colsize = tds_get_int(tds), varint;
	TDS_UCHAR type, info_len;
	TDSVARIANT *v;

	/* NULL */
	curcol->column_cur_size = -1;
	if (colsize < 2) {
		tds_get_n(tds, NULL, colsize);
		return TDS_SUCCESS;
	}

	v = (TDSVARIANT*) curcol->column_data;
	v->type = type = tds_get_byte(tds);
	info_len = tds_get_byte(tds);
	colsize -= 2;
	if (info_len > colsize)
		goto error_type;
	if (is_collate_type(type)) {
		if (sizeof(v->collation) > info_len)
			goto error_type;
		tds_get_n(tds, v->collation, sizeof(v->collation));
		colsize -= sizeof(v->collation);
		info_len -= sizeof(v->collation);
		curcol->char_conv = is_unicode_type(type) ? 
			tds->conn->char_convs[client2ucs2] : tds_iconv_from_collate(tds->conn, v->collation);
	}
	/* special case for numeric */
	if (is_numeric_type(type)) {
		TDS_NUMERIC *num;
		if (info_len != 2)
			goto error_type;
		if (v->data)
			TDS_ZERO_FREE(v->data);
		v->data_len = sizeof(TDS_NUMERIC);
		num = (TDS_NUMERIC*) calloc(1, sizeof(TDS_NUMERIC));
		v->data = (TDS_CHAR *) num;
		num->precision = tds_get_byte(tds);
		num->scale     = tds_get_byte(tds);
		colsize -= 2;
		/* FIXME check prec/scale, don't let server crash us */
		if (colsize > sizeof(num->array))
			goto error_type;
		curcol->column_cur_size = colsize;
		tds_get_n(tds, num->array, colsize);
		if (IS_TDS7_PLUS(tds->conn))
			tds_swap_numeric(num);
		return TDS_SUCCESS;
	}
	varint = (type == SYBUNIQUE) ? 0 : tds_get_varint_size(tds->conn, type);
	if (varint != info_len)
		goto error_type;
	switch (varint) {
	case 0:
		v->size = tds_get_size_by_type(type);
		break;
	case 1:
		v->size = tds_get_byte(tds);
		break;
	case 2:
		v->size = tds_get_smallint(tds);
		break;
	default:
		goto error_type;
	}
	colsize -= info_len;
	curcol->column_cur_size = colsize;
	if (v->data)
		TDS_ZERO_FREE(v->data);
	if (colsize) {
		if (USE_ICONV && curcol->char_conv) {
			curcol->column_cur_size = determine_adjusted_size(curcol->char_conv, colsize);
			v->data = (TDS_CHAR*) malloc(curcol->column_cur_size);
			if (!v->data)
				return TDS_FAIL;
			if (TDS_FAILED(tds_get_char_data(tds, (char *) v, colsize, curcol)))
				return TDS_FAIL;
			colsize = curcol->column_cur_size;
			v->type = tds_get_cardinal_type(type, 0);
		} else {
			v->data = (TDS_CHAR*) malloc(colsize);
			if (!v->data)
				return TDS_FAIL;
			tds_get_n(tds, v->data, colsize);
		}
	}
	v->data_len = colsize;
	return TDS_SUCCESS;

error_type:
	tds_get_n(tds, NULL, colsize);
	return TDS_FAIL;
}

/**
 * Read a data from wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where store column information
 * \return TDS_FAIL on error or TDS_SUCCESS
 */
static TDSRET
tds_generic_get(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	unsigned char *dest;
	int len, colsize;
	int fillchar;
	TDSBLOB *blob = NULL;

	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	tdsdump_log(TDS_DBG_INFO1, "tds_get_data: type %d, varint size %d\n", curcol->column_type, curcol->column_varint_size);
	switch (curcol->column_varint_size) {
	case 4:
		/*
		 * LONGBINARY
		 * This type just stores a 4-byte length
		 */
		if (curcol->column_type == SYBLONGBINARY) {
			colsize = tds_get_int(tds);
			break;
		}
		
		/* It's a BLOB... */
		len = tds_get_byte(tds);
		blob = (TDSBLOB *) curcol->column_data;
		if (len == 16) {	/*  Jeff's hack */
			tds_get_n(tds, blob->textptr, 16);
			tds_get_n(tds, blob->timestamp, 8);
			blob->valid_ptr = 1;
			if (IS_TDS72_PLUS(tds->conn) &&
			    memcmp(blob->textptr, "dummy textptr\0\0",16) == 0)
				blob->valid_ptr = 0;
			colsize = tds_get_int(tds);
		} else {
			colsize = -1;
		}
		break;
	case 5:
		colsize = tds_get_int(tds);
		if (colsize == 0)
			colsize = -1;
		break;
	case 8:
		return tds72_get_varmax(tds, curcol);
	case 2:
		colsize = tds_get_smallint(tds);
		break;
	case 1:
		colsize = tds_get_byte(tds);
		if (colsize == 0)
			colsize = -1;
		break;
	case 0:
		/* TODO this should be column_size */
		colsize = tds_get_size_by_type(curcol->column_type);
		break;
	default:
		colsize = -1;
		break;
	}
	if (IS_TDSDEAD(tds))
		return TDS_FAIL;

	tdsdump_log(TDS_DBG_INFO1, "tds_get_data(): wire column size is %d \n", colsize);
	/* set NULL flag in the row buffer */
	if (colsize < 0) {
		curcol->column_cur_size = -1;
		return TDS_SUCCESS;
	}

	/* 
	 * We're now set to read the data from the wire.  For varying types (e.g. char/varchar)
	 * make sure that curcol->column_cur_size reflects the size of the read data, 
	 * after any charset conversion.  tds_get_char_data() does that for you, 
	 * but of course tds_get_n() doesn't.  
	 *
	 * colsize == wire_size, bytes to read
	 * curcol->column_cur_size == sizeof destination buffer, room to write
	 */
	dest = curcol->column_data;
	if (is_blob_col(curcol)) {
		TDS_CHAR *p;
		int new_blob_size;

		blob = (TDSBLOB *) dest; 	/* cf. column_varint_size case 4, above */

		/* 
		 * Blobs don't use a column's fixed buffer because the official maximum size is 2 GB.
		 * Instead, they're reallocated as necessary, based on the data's size.  
		 * Here we allocate memory, if need be.  
		 */
		/* TODO this can lead to a big waste of memory */
		if (USE_ICONV)
			new_blob_size = determine_adjusted_size(curcol->char_conv, colsize);
		else
			new_blob_size = colsize;
		if (new_blob_size == 0) {
			curcol->column_cur_size = 0;
			if (blob->textvalue)
				TDS_ZERO_FREE(blob->textvalue);
			return TDS_SUCCESS;
		}

		p = blob->textvalue; /* save pointer in case realloc fails */
		if (!p) {
			p = (TDS_CHAR *) malloc(new_blob_size);
		} else {
			/* TODO perhaps we should store allocated bytes too ? */
			if (new_blob_size > curcol->column_cur_size ||  (curcol->column_cur_size - new_blob_size) > 10240) {
				p = (TDS_CHAR *) realloc(p, new_blob_size);
			}
		}
		
		if (!p)
			return TDS_FAIL;
		blob->textvalue = p;
		curcol->column_cur_size = new_blob_size;
		
		/* read the data */
		if (USE_ICONV && curcol->char_conv) {
			if (TDS_FAILED(tds_get_char_data(tds, (char *) blob, colsize, curcol)))
				return TDS_FAIL;
		} else {
			assert(colsize == new_blob_size);
			tds_get_n(tds, blob->textvalue, colsize);
		}
	} else {		/* non-numeric and non-blob */
		curcol->column_cur_size = colsize;

		if (USE_ICONV && curcol->char_conv) {
			if (TDS_FAILED(tds_get_char_data(tds, (char *) dest, colsize, curcol)))
				return TDS_FAIL;
		} else {	
			/*
			 * special case, some servers seem to return more data in some conditions 
			 * (ASA 7 returning 4 byte nullable integer)
			 */
			int discard_len = 0;
			if (colsize > curcol->column_size) {
				discard_len = colsize - curcol->column_size;
				colsize = curcol->column_size;
			}
			if (tds_get_n(tds, dest, colsize) == NULL)
				return TDS_FAIL;
			if (discard_len > 0)
				tds_get_n(tds, NULL, discard_len);
			curcol->column_cur_size = colsize;
		}

		/* pad (UNI)CHAR and BINARY types */
		fillchar = 0;
		switch (curcol->column_type) {
		/* extra handling for SYBLONGBINARY */
		case SYBLONGBINARY:
			if (curcol->column_usertype != USER_UNICHAR_TYPE)
				break;
		case SYBCHAR:
		case XSYBCHAR:
			if (curcol->column_size != curcol->on_server.column_size)
				break;
			/* FIXME use client charset */
			fillchar = ' ';
		case SYBBINARY:
		case XSYBBINARY:
			if (colsize < curcol->column_size)
				memset(dest + colsize, fillchar, curcol->column_size - colsize);
			colsize = curcol->column_size;
			break;
		}
	}

#ifdef WORDS_BIGENDIAN
	/*
	 * MS SQL Server 7.0 has broken date types from big endian
	 * machines, this swaps the low and high halves of the
	 * affected datatypes
	 *
	 * Thought - this might be because we don't have the
	 * right flags set on login.  -mjs
	 *
	 * Nope its an actual MS SQL bug -bsb
	 */
	/* TODO test on login, remove configuration -- freddy77 */
	if (tds_conn(tds)->broken_dates &&
	    (curcol->column_type == SYBDATETIME ||
	     curcol->column_type == SYBDATETIME4 ||
	     curcol->column_type == SYBDATETIMN ||
	     curcol->column_type == SYBMONEY ||
	     curcol->column_type == SYBMONEY4 || (curcol->column_type == SYBMONEYN && curcol->column_size > 4)))
		/*
		 * above line changed -- don't want this for 4 byte SYBMONEYN
		 * values (mlilback, 11/7/01)
		 */
	{
		unsigned char temp_buf[8];

		memcpy(temp_buf, dest, colsize / 2);
		memcpy(dest, &dest[colsize / 2], colsize / 2);
		memcpy(&dest[colsize / 2], temp_buf, colsize / 2);
	}
	if (tds_conn(tds)->emul_little_endian) {
		tdsdump_log(TDS_DBG_INFO1, "swapping coltype %d\n", tds_get_conversion_type(curcol->column_type, colsize));
		tds_swap_datatype(tds_get_conversion_type(curcol->column_type, colsize), dest);
	}
#endif
	return TDS_SUCCESS;
}

/**
 * Put data information to wire
 * \param tds   state information for the socket and the TDS protocol
 * \param col   column where to store information
 * \return TDS_SUCCESS or TDS_FAIL
 */
static TDSRET
tds_generic_put_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	size_t size;

	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(col);

	size = tds_fix_column_size(tds, col);
	switch (col->column_varint_size) {
	case 0:
		break;
	case 1:
		tds_put_byte(tds, size);
		break;
	case 2:
		tds_put_smallint(tds, size);
		break;
	case 5:
	case 4:
		tds_put_int(tds, size);
		break;
	case 8:
		tds_put_smallint(tds, 0xffff);
		break;
	}

	/* TDS7.1 output collate information */
	if (IS_TDS71_PLUS(tds->conn) && is_collate_type(col->on_server.column_type))
		tds_put_n(tds, tds->conn->collation, 5);

	return TDS_SUCCESS;
}

/**
 * Write data to wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where store column information
 * \return TDS_FAIL on error or TDS_SUCCESS
 */
static TDSRET
tds_generic_put(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	unsigned char *src;
	TDSBLOB *blob = NULL;
	size_t colsize, size;

	const char *s;
	int converted = 0;

	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	tdsdump_log(TDS_DBG_INFO1, "tds_generic_put: colsize = %d\n", (int) curcol->column_cur_size);

	/* output NULL data */
	if (curcol->column_cur_size < 0) {
		tdsdump_log(TDS_DBG_INFO1, "tds_generic_put: null param\n");
		switch (curcol->column_varint_size) {
		case 5:
			tds_put_int(tds, 0);
			break;
		case 4:
			tds_put_int(tds, -1);
			break;
		case 2:
			tds_put_smallint(tds, -1);
			break;
		case 8:
			tds_put_int8(tds, -1);
			break;
		default:
			assert(curcol->column_varint_size);
			/* FIXME not good for SYBLONGBINARY/SYBLONGCHAR (still not supported) */
			tds_put_byte(tds, 0);
			break;
		}
		return TDS_SUCCESS;
	}
	colsize = curcol->column_cur_size;

	size = tds_fix_column_size(tds, curcol);

	src = curcol->column_data;
	if (is_blob_col(curcol)) {
		blob = (TDSBLOB *) src;
		src = (unsigned char *) blob->textvalue;
	}

	s = (char *) src;

	/* convert string if needed */
	if (curcol->char_conv && curcol->char_conv->flags != TDS_ENCODING_MEMCPY && colsize) {
		size_t output_size;
#if 0
		/* TODO this case should be optimized */
		/* we know converted bytes */
		if (curcol->char_conv->client_charset.min_bytes_per_char == curcol->char_conv->client_charset.max_bytes_per_char 
		    && curcol->char_conv->server_charset.min_bytes_per_char == curcol->char_conv->server_charset.max_bytes_per_char) {
			converted_size = colsize * curcol->char_conv->server_charset.min_bytes_per_char / curcol->char_conv->client_charset.min_bytes_per_char;

		} else {
#endif
		/* we need to convert data before */
		/* TODO this can be a waste of memory... */
		converted = 1;
		s = tds_convert_string(tds, curcol->char_conv, s, colsize, &output_size);
		colsize = (TDS_INT)output_size;
		if (!s) {
			/* on conversion error put a empty string */
			/* TODO on memory failure we should compute converted size and use chunks */
			colsize = 0;
			converted = -1;
		}
	}

	/*
	 * TODO here we limit data sent with MIN, should mark somewhere
	 * and inform client ??
	 * Test proprietary behavior
	 */
	if (IS_TDS7_PLUS(tds->conn)) {
		tdsdump_log(TDS_DBG_INFO1, "tds_generic_put: not null param varint_size = %d\n",
			    curcol->column_varint_size);

		switch (curcol->column_varint_size) {
		case 8:
			tds_put_int8(tds, colsize);
			tds_put_int(tds, colsize);
			break;
		case 4:	/* It's a BLOB... */
			colsize = MIN(colsize, size);
			/* mssql require only size */
			tds_put_int(tds, colsize);
			break;
		case 2:
			colsize = MIN(colsize, size);
			tds_put_smallint(tds, colsize);
			break;
		case 1:
			colsize = MIN(colsize, size);
			tds_put_byte(tds, colsize);
			break;
		case 0:
			/* TODO should be column_size */
			colsize = tds_get_size_by_type(curcol->on_server.column_type);
			break;
		}

		/* conversion error, exit with an error */
		if (converted < 0)
			return TDS_FAIL;

		/* put real data */
		if (blob) {
			tds_put_n(tds, s, colsize);
		} else {
#ifdef WORDS_BIGENDIAN
			unsigned char buf[64];

			if (tds_conn(tds)->emul_little_endian && !converted && colsize < 64) {
				tdsdump_log(TDS_DBG_INFO1, "swapping coltype %d\n",
					    tds_get_conversion_type(curcol->column_type, colsize));
				memcpy(buf, s, colsize);
				tds_swap_datatype(tds_get_conversion_type(curcol->column_type, colsize), buf);
				s = (char *) buf;
			}
#endif
			tds_put_n(tds, s, colsize);
		}
		/* finish chunk for varchar/varbinary(max) */
		if (curcol->column_varint_size == 8 && colsize)
			tds_put_int(tds, 0);
	} else {
		/* TODO ICONV handle charset conversions for data */
		/* put size of data */
		switch (curcol->column_varint_size) {
		case 5:	/* It's a LONGBINARY */
			colsize = MIN(colsize, 0x7fffffff);
			tds_put_int(tds, colsize);
			break;
		case 4:	/* It's a BLOB... */
			tds_put_byte(tds, 16);
			tds_put_n(tds, blob->textptr, 16);
			tds_put_n(tds, blob->timestamp, 8);
			colsize = MIN(colsize, 0x7fffffff);
			tds_put_int(tds, colsize);
			break;
		case 2:
			colsize = MIN(colsize, 8000);
			tds_put_smallint(tds, colsize);
			break;
		case 1:
			if (!colsize) {
				tds_put_byte(tds, 1);
				if (is_char_type(curcol->column_type))
					tds_put_byte(tds, ' ');
				else
					tds_put_byte(tds, 0);
				return TDS_SUCCESS;
			}
			colsize = MIN(colsize, 255);
			tds_put_byte(tds, colsize);
			break;
		case 0:
			/* TODO should be column_size */
			colsize = tds_get_size_by_type(curcol->column_type);
			break;
		}

		/* conversion error, exit with an error */
		if (converted < 0)
			return TDS_FAIL;

		/* put real data */
		if (blob) {
			tds_put_n(tds, s, colsize);
		} else {
#ifdef WORDS_BIGENDIAN
			unsigned char buf[64];

			if (tds_conn(tds)->emul_little_endian && !converted && colsize < 64) {
				tdsdump_log(TDS_DBG_INFO1, "swapping coltype %d\n",
					    tds_get_conversion_type(curcol->column_type, colsize));
				memcpy(buf, s, colsize);
				tds_swap_datatype(tds_get_conversion_type(curcol->column_type, colsize), buf);
				s = (char *) buf;
			}
#endif
			tds_put_n(tds, s, colsize);
		}
	}
	if (converted)
		tds_convert_string_free((char*)src, s);
	return TDS_SUCCESS;
}

#define DEFINE_FUNCS(name) \
const TDSCOLUMNFUNCS name ## _funcs = { \
	tds_ ## name ## _get_info, \
	tds_ ## name ## _get, \
	tds_ ## name ## _row_len, \
	tds_ ## name ## _put_info, \
	tds_ ## name ## _put, \
};

static TDSRET
tds_numeric_get_info(TDSSOCKET *tds, TDSCOLUMN *col)
{
	col->column_size = tds_get_byte(tds);
	col->column_prec = tds_get_byte(tds);        /* precision */
	col->column_scale = tds_get_byte(tds);       /* scale */

	/* check prec/scale, don't let server crash us */
	if (col->column_prec < 1 || col->column_prec > 77
	    || col->column_scale > col->column_prec)
		return TDS_FAIL;

	return TDS_SUCCESS;
}

static TDS_INT
tds_numeric_row_len(TDSCOLUMN *col)
{
	return sizeof(TDS_NUMERIC);
}

static TDSRET
tds_numeric_get(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	int colsize;
	TDS_NUMERIC *num;

	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	colsize = tds_get_byte(tds);

	/* set NULL flag in the row buffer */
	if (colsize <= 0) {
		curcol->column_cur_size = -1;
		return TDS_SUCCESS;
	}

	/* 
	 * Since these can be passed around independent
	 * of the original column they came from, we embed the TDS_NUMERIC datatype in the row buffer
	 * instead of using the wire representation, even though it uses a few more bytes.  
	 */
	num = (TDS_NUMERIC *) curcol->column_data;
	memset(num, '\0', sizeof(TDS_NUMERIC));
	/* TODO perhaps it would be fine to change format ?? */
	num->precision = curcol->column_prec;
	num->scale = curcol->column_scale;

	/* server is going to crash freetds ?? */
	/* TODO close connection it server try to do so ?? */
	if (colsize > sizeof(num->array))
		return TDS_FAIL;
	tds_get_n(tds, num->array, colsize);

	if (IS_TDS7_PLUS(tds->conn))
		tds_swap_numeric(num);

	/* corrected colsize for column_cur_size */
	curcol->column_cur_size = sizeof(TDS_NUMERIC);

	return TDS_SUCCESS;
}

static TDSRET
tds_numeric_put_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(col);

#if 1
	tds_put_byte(tds, tds_numeric_bytes_per_prec[col->column_prec]);
	tds_put_byte(tds, col->column_prec);
	tds_put_byte(tds, col->column_scale);
#else
	TDS_NUMERIC *num = (TDS_NUMERIC *) col->column_data;
	tds_put_byte(tds, tds_numeric_bytes_per_prec[num->precision]);
	tds_put_byte(tds, num->precision);
	tds_put_byte(tds, num->scale);
#endif

	return TDS_SUCCESS;
}

static TDSRET
tds_numeric_put(TDSSOCKET *tds, TDSCOLUMN *col)
{
	TDS_NUMERIC *num = (TDS_NUMERIC *) col->column_data, buf;
	unsigned char colsize;

	if (col->column_cur_size < 0) {
		tds_put_byte(tds, 0);
		return TDS_SUCCESS;
	}
	colsize = tds_numeric_bytes_per_prec[num->precision];
	tds_put_byte(tds, colsize);

	buf = *num;
	if (IS_TDS7_PLUS(tds->conn))
		tds_swap_numeric(&buf);
	tds_put_n(tds, buf.array, colsize);
	return TDS_SUCCESS;
}

#define tds_variant_get_info tds_generic_get_info
#define tds_variant_row_len  tds_generic_row_len

static TDSRET
tds_variant_put_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	/* TODO */
	return TDS_FAIL;
}

static TDSRET
tds_variant_put(TDSSOCKET *tds, TDSCOLUMN *col)
{
	/* TODO */
	return TDS_FAIL;
}

static TDSRET
tds_msdatetime_get_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	col->column_scale = col->column_prec = 0;
	if (col->column_type != SYBMSDATE) {
		col->column_scale = col->column_prec = tds_get_byte(tds);
		if (col->column_prec > 7)
			return TDS_FAIL;
	}
	col->on_server.column_size = col->column_size = sizeof(TDS_DATETIMEALL);
	return TDS_SUCCESS;
}

static TDS_INT
tds_msdatetime_row_len(TDSCOLUMN *col)
{
	return sizeof(TDS_DATETIMEALL);
}

static TDSRET
tds_msdatetime_get(TDSSOCKET * tds, TDSCOLUMN * col)
{
	TDS_DATETIMEALL *dt = (TDS_DATETIMEALL*) col->column_data;
	int size = tds_get_byte(tds);

	if (size == 0) {
		col->column_cur_size = -1;
		return TDS_SUCCESS;
	}

	memset(dt, 0, sizeof(*dt));

	if (col->column_type == SYBMSDATETIMEOFFSET)
		size -= 2;
	if (col->column_type != SYBMSTIME)
		size -= 3;
	if (size < 0)
		return TDS_FAIL;

	dt->time_prec = col->column_prec;

	/* get time part */
	if (col->column_type != SYBMSDATE) {
		TDS_UINT8 u8;
		int i;

		assert(size >= 3 && size <= 5);
		if (size < 3 || size > 5)
			return TDS_FAIL;
		u8 = 0;
		tds_get_n(tds, &u8, size);
#ifdef WORDS_BIGENDIAN
		tds_swap_bytes(&u8, 8);
#endif
		for (i = col->column_prec; i < 7; ++i)
			u8 *= 10;
		dt->time = u8;
		dt->has_time = 1;
	}

	/* get date part */
	if (col->column_type != SYBMSTIME) {
		TDS_UINT ui;

		ui = 0;
		tds_get_n(tds, &ui, 3);
#ifdef WORDS_BIGENDIAN
		tds_swap_bytes(&ui, 4);
#endif
		dt->has_date = 1;
		dt->date = ui - 693595;
	}

	/* get time offset */
	if (col->column_type == SYBMSDATETIMEOFFSET) {
		dt->offset = tds_get_smallint(tds);
		if (dt->offset > 840 || dt->offset < -840)
			return TDS_FAIL;
		dt->has_offset = 1;
	}
	col->column_cur_size = sizeof(TDS_DATETIMEALL);
	return TDS_SUCCESS;
}

static TDSRET
tds_msdatetime_put_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	/* TODO precision */
	if (col->on_server.column_type != SYBMSDATE)
		tds_put_byte(tds, 7);
	return TDS_SUCCESS;
}

static TDSRET
tds_msdatetime_put(TDSSOCKET *tds, TDSCOLUMN *col)
{
	const TDS_DATETIMEALL *dta = (const TDS_DATETIMEALL *) col->column_data;
	unsigned char buf[12], *p;

	if (col->column_cur_size < 0) {
		tds_put_byte(tds, 0);
		return TDS_SUCCESS;
	}

	/* TODO precision */
	p = buf + 1;
	if (col->on_server.column_type != SYBMSDATE) {
		TDS_PUT_UA4LE(p, (TDS_UINT) dta->time);
		p[4] = (unsigned char) (dta->time >> 32);
		p += 5;
	}
	if (col->on_server.column_type != SYBMSTIME) {
		TDS_UINT ui = dta->date + 693595;
		TDS_PUT_UA4LE(p, ui);
		p += 3;
	}
	if (col->on_server.column_type == SYBMSDATETIMEOFFSET) {
		TDS_PUT_UA2LE(p, dta->offset);
		p += 2;
	}
	buf[0] = p - buf - 1;
	tds_put_n(tds, buf, p - buf);

	return TDS_SUCCESS;
}

static TDSRET
tds_clrudt_get_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	/* TODO save fields */
	/* FIXME support RPC */

	/* MAX_BYTE_SIZE */
	tds_get_usmallint(tds);

	/* DB_NAME */
	tds_get_string(tds, tds_get_byte(tds), NULL, 0);

	/* SCHEMA_NAME */
	tds_get_string(tds, tds_get_byte(tds), NULL, 0);

	/* TYPE_NAME */
	tds_get_string(tds, tds_get_byte(tds), NULL, 0);

	/* UDT_METADATA */
	tds_get_string(tds, tds_get_usmallint(tds), NULL, 0);

	col->column_size = 0x7ffffffflu;

	return TDS_SUCCESS;
}

static TDS_INT
tds_clrudt_row_len(TDSCOLUMN *col)
{
	/* TODO safe other fields */
	return sizeof(TDSBLOB);
}

#define tds_clrudt_get tds_generic_get

static TDSRET
tds_clrudt_put_info(TDSSOCKET * tds, TDSCOLUMN * col)
{
	/* FIXME support properly*/
	tds_put_byte(tds, 0);	/* db_name */
	tds_put_byte(tds, 0);	/* schema_name */
	tds_put_byte(tds, 0);	/* type_name */

	return TDS_SUCCESS;
}

#define tds_clrudt_put tds_generic_put

DEFINE_FUNCS(generic);
DEFINE_FUNCS(numeric);
DEFINE_FUNCS(variant);
DEFINE_FUNCS(msdatetime);
DEFINE_FUNCS(clrudt);

static const TDSCOLUMNFUNCS *
tds_get_column_funcs(TDSCONNECTION *conn, int type)
{
	switch (type) {
	case SYBNUMERIC:
	case SYBDECIMAL:
		return &numeric_funcs;
	case SYBMSUDT:
		return &clrudt_funcs;
	case SYBVARIANT:
		if (IS_TDS7_PLUS(conn))
			return &variant_funcs;
		break;
	case SYBMSDATE:
	case SYBMSTIME:
	case SYBMSDATETIME2:
	case SYBMSDATETIMEOFFSET:
		return &msdatetime_funcs;
	}
	return &generic_funcs;
}
#include "tds_types.h"
