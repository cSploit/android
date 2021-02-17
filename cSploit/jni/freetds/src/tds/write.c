/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 * Copyright (C) 2005  Frediano Ziglio
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

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

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

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/bytes.h>
#include <freetds/stream.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: write.c,v 1.84 2011-06-18 17:52:24 freddy77 Exp $");

/**
 * \addtogroup network
 * @{ 
 */

/*
 * CRE 01262002 making buf a void * means we can put any type without casting
 *		much like read() and memcpy()
 */
int
tds_put_n(TDSSOCKET * tds, const void *buf, size_t n)
{
	size_t left;
	const unsigned char *bufp = (const unsigned char *) buf;

	for (; n;) {
		left = tds->out_buf_max - tds->out_pos;
		if (left <= 0) {
			tds_write_packet(tds, 0x0);
			continue;
		}
		if (left > n)
			left = n;
		if (bufp) {
			memcpy(tds->out_buf + tds->out_pos, bufp, left);
			bufp += left;
		} else {
			memset(tds->out_buf + tds->out_pos, 0, left);
		}
		tds->out_pos += (unsigned int)left;
		n -= left;
	}
	return 0;
}

/**
 * Output a string to wire
 * automatic translate string to unicode if needed
 * \return bytes written to wire
 * \param tds state information for the socket and the TDS protocol
 * \param s   string to write
 * \param len length of string in characters, or -1 for null terminated
 */
int
tds_put_string(TDSSOCKET * tds, const char *s, int len)
{
	int res;
	TDSSTATICINSTREAM r;
	TDSDATAOUTSTREAM w;


	if (len < 0) {
		TDS_ENCODING *client;
		client = &tds->conn->char_convs[client2ucs2]->from.charset;

		if (client->min_bytes_per_char == 1) {	/* ascii or UTF-8 */
			len = (int)strlen(s);
		} else if (client->min_bytes_per_char == 2) {	/* UCS-2 or variant */
			const char *p = s;

			while (p[0] || p[1])
				p += 2;
			len = (int)(p - s);

		} else if (client->min_bytes_per_char == 4) {	/* UCS-4 or variant */
			const char *p = s;

			while (p[0] || p[1] || p[2] || p[3])
				p += 4;
			len = (int)(p - s);
		} else {
			assert(client->min_bytes_per_char < 3);	/* FIXME */
		}
	}

	assert(len >= 0);

	/* valid test only if client and server share a character set. TODO conversions for Sybase */
	if (!IS_TDS7_PLUS(tds->conn))	{
		tds_put_n(tds, s, len);
		return len;
	}

	tds_staticin_stream_init(&r, s, len);
	tds_dataout_stream_init(&w, tds);

	res = tds_convert_stream(tds, tds->conn->char_convs[client2ucs2], to_server, &r.stream, &w.stream);
	return w.written;
}

int
tds_put_buf(TDSSOCKET * tds, const unsigned char *buf, int dsize, int ssize)
{
	int cpsize;

	cpsize = ssize > dsize ? dsize : ssize;
	tds_put_n(tds, buf, cpsize);
	dsize -= cpsize;
	tds_put_n(tds, NULL, dsize);
	return tds_put_byte(tds, cpsize);
}

int
tds_put_int8(TDSSOCKET * tds, TDS_INT8 i)
{
#if TDS_ADDITIONAL_SPACE < 8
#if WORDS_BIGENDIAN
	TDS_UINT h;

	if (tds_conn(tds)->emul_little_endian) {
		h = (TDS_UINT) i;
		tds_put_byte(tds, h & 0x000000FF);
		tds_put_byte(tds, (h & 0x0000FF00) >> 8);
		tds_put_byte(tds, (h & 0x00FF0000) >> 16);
		tds_put_byte(tds, (h & 0xFF000000) >> 24);
		h = (TDS_UINT) (i >> 32);
		tds_put_byte(tds, h & 0x000000FF);
		tds_put_byte(tds, (h & 0x0000FF00) >> 8);
		tds_put_byte(tds, (h & 0x00FF0000) >> 16);
		tds_put_byte(tds, (h & 0xFF000000) >> 24);
		return 0;
	}
#endif
	return tds_put_n(tds, (const unsigned char *) &i, sizeof(TDS_INT8));
#else
	TDS_UCHAR *p;

	if (tds->out_pos >= tds->out_buf_max)
		tds_write_packet(tds, 0x0);

	p = &tds->out_buf[tds->out_pos];
#if WORDS_BIGENDIAN
	if (tds->emul_little_endian) {
		TDS_PUT_UA4LE(p, (TDS_UINT) i);
		TDS_PUT_UA4LE(p+4, (TDS_UINT) (i >> 32));
	} else {
		TDS_PUT_UA4(p, (TDS_UINT) (i >> 32));
		TDS_PUT_UA4(p+4, (TDS_UINT) i);
	}
#else
	TDS_PUT_UA4(p, (TDS_UINT) i);
	TDS_PUT_UA4(p+4, (TDS_UINT) (i >> 32));
#endif
	tds->out_pos += 8;
	return 0;
#endif
}

int
tds_put_int(TDSSOCKET * tds, TDS_INT i)
{
#if TDS_ADDITIONAL_SPACE < 4
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian) {
		tds_put_byte(tds, i & 0x000000FF);
		tds_put_byte(tds, (i & 0x0000FF00) >> 8);
		tds_put_byte(tds, (i & 0x00FF0000) >> 16);
		tds_put_byte(tds, (i & 0xFF000000) >> 24);
		return 0;
	}
#endif
	return tds_put_n(tds, (const unsigned char *) &i, sizeof(TDS_INT));
#else
	TDS_UCHAR *p;

	if (tds->out_pos >= tds->out_buf_max)
		tds_write_packet(tds, 0x0);

	p = &tds->out_buf[tds->out_pos];
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian)
		TDS_PUT_UA4LE(p, i);
	else
		TDS_PUT_UA4(p, i);
#else
	TDS_PUT_UA4(p, i);
#endif
	tds->out_pos += 4;
	return 0;
#endif
}

int
tds_put_smallint(TDSSOCKET * tds, TDS_SMALLINT si)
{
#if TDS_ADDITIONAL_SPACE < 2
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian) {
		tds_put_byte(tds, si & 0x000000FF);
		tds_put_byte(tds, (si & 0x0000FF00) >> 8);
		return 0;
	}
#endif
	return tds_put_n(tds, (const unsigned char *) &si, sizeof(TDS_SMALLINT));
#else
	TDS_UCHAR *p;

	if (tds->out_pos >= tds->out_buf_max)
		tds_write_packet(tds, 0x0);

	p = &tds->out_buf[tds->out_pos];
#if WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian)
		TDS_PUT_UA2LE(p, si);
	else
		TDS_PUT_UA2(p, si);
#else
	TDS_PUT_UA2(p, si);
#endif
	tds->out_pos += 2;
	return 0;
#endif
}

int
tds_put_byte(TDSSOCKET * tds, unsigned char c)
{
	if (tds->out_pos >= (unsigned int)tds->out_buf_max)
		tds_write_packet(tds, 0x0);
	tds->out_buf[tds->out_pos++] = c;
	return 0;
}

int
tds_init_write_buf(TDSSOCKET * tds)
{
	/* TODO needed ?? */
	memset(tds->out_buf, '\0', tds->out_buf_max);
	tds->out_pos = 8;
	return 0;
}

/**
 * Flush packet to server
 * @return TDS_FAIL or TDS_SUCCESS
 */
TDSRET
tds_flush_packet(TDSSOCKET * tds)
{
	TDSRET result = TDS_FAIL;

	/* GW added check for tds->s */
	if (!IS_TDSDEAD(tds))
		result = tds_write_packet(tds, 0x01);
	return result;
}

/** @} */
