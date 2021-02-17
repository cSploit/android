/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 * Copyright (C) 2005-2014  Frediano Ziglio
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
 * \file
 * \brief Grab data from TDS packets
 */

#include <config.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <assert.h>

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/bytes.h>
#include <freetds/stream.h>
#include <freetds/string.h>
#include "tds_checks.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

static size_t read_and_convert(TDSSOCKET * tds, TDSICONV * char_conv,
			       size_t * wire_size, char *outbuf, size_t outbytesleft);

/**
 * \ingroup libtds
 * \defgroup network Network functions
 * Functions for reading or writing from network.
 */

/**
 * \addtogroup network
 * @{ 
 */

/**
 * Return a single byte from the input buffer
 * \tds
 */
unsigned char
tds_get_byte(TDSSOCKET * tds)
{
	while (tds->in_pos >= tds->in_len) {
		if (tds_read_packet(tds) < 0)
			return 0;
	}
	return tds->in_buf[tds->in_pos++];
}

/**
 * Unget will always work as long as you don't call it twice in a row.  It
 * it may work if you call it multiple times as long as you don't backup
 * over the beginning of network packet boundary which can occur anywhere in
 * the token stream.
 * \tds
 */
void
tds_unget_byte(TDSSOCKET * tds)
{
	/* this is a one trick pony...don't call it twice */
	tds->in_pos--;
}

/**
 * Reads a byte from the TDS stream without removing it
 * \tds
 */
unsigned char
tds_peek(TDSSOCKET * tds)
{
	unsigned char result = tds_get_byte(tds);
	if (tds->in_pos > 0)
		--tds->in_pos;
	return result;
}				/* tds_peek()  */


/**
 * Get an int16 from the server.
 */
TDS_USMALLINT
tds_get_usmallint(TDSSOCKET * tds)
{
	unsigned char bytes[2];

	tds_get_n(tds, bytes, 2);
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian)
		return (TDS_USMALLINT) TDS_GET_A2LE(bytes);
#endif
	return (TDS_USMALLINT) TDS_GET_A2(bytes);
}


/**
 * Get an int32 from the server.
 * \tds
 */
TDS_UINT
tds_get_uint(TDSSOCKET * tds)
{
	unsigned char bytes[4];

	tds_get_n(tds, bytes, 4);
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian)
		return TDS_GET_A4LE(bytes);
#endif
	return TDS_GET_A4(bytes);
}

/**
 * Get an uint64 from the server.
 * \tds
 */
TDS_UINT8
tds_get_uint8(TDSSOCKET * tds)
{
	TDS_UINT h;
	TDS_UINT l;
	unsigned char bytes[8];

	tds_get_n(tds, bytes, 8);
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian) {
		l = TDS_GET_A4LE(bytes);
		h = TDS_GET_A4LE(bytes+4);
	} else {
		h = TDS_GET_A4(bytes);
		l = TDS_GET_A4(bytes+4);
	}
#else
	l = TDS_GET_A4(bytes);
	h = TDS_GET_A4(bytes+4);
#endif
	return (((TDS_UINT8) h) << 32) | l;
}

/**
 * Fetch a string from the wire.
 * Output string is NOT null terminated.
 * If TDS version is 7 or 8 read unicode string and convert it.
 * This function should be use to read server default encoding strings like 
 * columns name, table names, etc, not for data (use tds_get_char_data instead)
 * @return bytes written to \a dest
 * @param tds  connection information
 * @param string_len length of string to read from wire 
 *        (in server characters, bytes for tds4-tds5, ucs2 for tds7+)
 * @param dest destination buffer, if NULL string is read and discarded
 * @param dest_size destination buffer size, in bytes
 */
size_t
tds_get_string(TDSSOCKET * tds, size_t string_len, char *dest, size_t dest_size)
{
	size_t wire_bytes;

	/*
	 * FIX: 02-Jun-2000 by Scott C. Gray (SCG)
	 * Bug to malloc(0) on some platforms.
	 */
	if (string_len == 0)
		return 0;

	wire_bytes = IS_TDS7_PLUS(tds->conn) ? string_len * 2 : string_len;

	if (IS_TDS7_PLUS(tds->conn)) {
		if (dest == NULL) {
			tds_get_n(tds, NULL, wire_bytes);
			return string_len;
		}

		return read_and_convert(tds, tds->conn->char_convs[client2ucs2], &wire_bytes, dest, dest_size);
	} else {
		/* FIXME convert to client charset */
		assert(dest_size >= (size_t) string_len);
		tds_get_n(tds, dest, string_len);
		return string_len;
	}
}

/**
 * Fetch character data the wire.
 * Output is NOT null terminated.
 * If \a char_conv is not NULL, convert data accordingly.
 * \param tds         state information for the socket and the TDS protocol
 * \param row_buffer  destination buffer in current_row. Can't be NULL
 * \param wire_size   size to read from wire (in bytes)
 * \param curcol      column information
 * \return TDS_SUCCESS or TDS_FAIL (probably memory error on text data)
 * \todo put a TDSICONV structure in every TDSCOLUMN
 */
TDSRET
tds_get_char_data(TDSSOCKET * tds, char *row_buffer, size_t wire_size, TDSCOLUMN * curcol)
{
	size_t in_left;
	TDSBLOB *blob = NULL;
	char *dest = row_buffer;

	if (is_blob_col(curcol)) {
		blob = (TDSBLOB *) row_buffer;
		dest = blob->textvalue;
	}

	/* 
	 * dest is usually a column buffer, allocated when the column's metadata are processed 
	 * and reused for each row.  
	 * For blobs, dest is blob->textvalue, and can be reallocated or freed
	 * TODO: reallocate if blob and no space 
	 */
	 
	/* silly case, empty string */
	if (wire_size == 0) {
		curcol->column_cur_size = 0;
		if (blob)
			TDS_ZERO_FREE(blob->textvalue);
		return TDS_SUCCESS;
	}

	if (curcol->char_conv) {
		/*
		 * TODO The conversion should be selected from curcol and tds version
		 * TDS7.1/single -> use curcol collation
		 * TDS7/single -> use server single byte
		 * TDS7+/unicode -> use server (always unicode)
		 * TDS5/4.2 -> use server 
		 * TDS5/UTF-8 -> use server
		 * TDS5/UTF-16 -> use UTF-16
		 */
		in_left = blob ? curcol->column_cur_size : curcol->column_size;
		curcol->column_cur_size = read_and_convert(tds, curcol->char_conv, &wire_size, dest, in_left);
		if (TDS_UNLIKELY(wire_size > 0)) {
			tds_get_n(tds, NULL, wire_size);
			tdsdump_log(TDS_DBG_NETWORK, "error: tds_get_char_data: discarded %u on wire while reading %d into client. \n", 
							 (unsigned int) wire_size, curcol->column_cur_size);
			return TDS_FAIL;
		}
	} else {
		curcol->column_cur_size = (TDS_INT)wire_size;
		if (tds_get_n(tds, dest, wire_size) == NULL) {
			tdsdump_log(TDS_DBG_NETWORK, "error: tds_get_char_data: failed to read %u from wire. \n",
				    (unsigned int) wire_size);
			return TDS_FAIL;
		}
	}
	return TDS_SUCCESS;
}

/**
 * Get N bytes from the buffer and return them in the already allocated space  
 * given to us.  We ASSUME that the person calling this function has done the  
 * bounds checking for us since they know how many bytes they want here.
 * dest of NULL means we just want to eat the bytes.   (tetherow@nol.org)
 */
void *
tds_get_n(TDSSOCKET * tds, void *dest, size_t need)
{
	assert(need >= 0);

	for (;;) {
		unsigned int have = tds->in_len - tds->in_pos;

		if (need <= have)
			break;
		/* We need more than is in the buffer, copy what is there */
		if (dest != NULL) {
			memcpy((char *) dest, tds->in_buf + tds->in_pos, have);
			dest = (char *) dest + have;
		}
		need -= have;
		if (TDS_UNLIKELY(tds_read_packet(tds) < 0))
			return NULL;
	}
	if (need > 0) {
		/* get the remainder if there is any */
		if (dest != NULL) {
			memcpy((char *) dest, tds->in_buf + tds->in_pos, need);
		}
		tds->in_pos += need;
	}
	return dest;
}

/**
 * For UTF-8 and similar, tds_iconv() may encounter a partial sequence when the chunk boundary
 * is not aligned with the character boundary.  In that event, it will return an error, and
 * some number of bytes (less than a character) will remain in the tail end of temp[].  They are
 * moved to the beginning, ptemp is adjusted to point just behind them, and the next chunk is read.
 * \tds
 * \param char_conv conversion structure
 * \param[out] wire_size size readed from wire
 * \param outbuf buffer to write to
 * \param outbytesleft buffer length
 * \return bytes readed
 */
static size_t
read_and_convert(TDSSOCKET * tds, TDSICONV * char_conv, size_t * wire_size, char *outbuf,
		 size_t outbytesleft)
{
	int res;
	TDSDATAINSTREAM r;
	TDSSTATICOUTSTREAM w;

	tds_datain_stream_init(&r, tds, *wire_size);
	tds_staticout_stream_init(&w, outbuf, outbytesleft);

	res = tds_convert_stream(tds, char_conv, to_client, &r.stream, &w.stream);
	*wire_size = r.wire_size;
	return (char *) w.stream.buffer - outbuf;
}

/**
 * Reads a string from wire and put in a DSTR.
 * On error we read the bytes from the wire anyway.
 * \tds
 * \param[out] s output string
 * \param[in] len string length (in characters)
 * \return string or NULL on error
 */
DSTR*
tds_dstr_get(TDSSOCKET * tds, DSTR * s, size_t len)
{
	size_t out_len;

	CHECK_TDS_EXTRA(tds);

	/* assure sufficient space for every conversion */
	if (TDS_UNLIKELY(!tds_dstr_alloc(s, len * 4))) {
		tds_get_n(tds, NULL, len);
		return NULL;
	}

	out_len = tds_get_string(tds, len, tds_dstr_buf(s), len * 4);
	tds_dstr_setlen(s, out_len);
	return s;
}

/** @} */
