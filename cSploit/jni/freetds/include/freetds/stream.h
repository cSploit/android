/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2013  Frediano Ziglio
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

#ifndef _freetds_stream_h_
#define _freetds_stream_h_

#ifndef _tds_h_
#error Include tds.h first
#endif

/** define a stream of data used for input */
typedef struct tds_input_stream {
	/** read some data
	 * Return 0 if end of stream
	 * Return <0 if error (actually not defined)
	 */
	int (*read)(struct tds_input_stream *stream, void *ptr, size_t len);
} TDSINSTREAM;

/** define a stream of data used for output */
typedef struct tds_output_stream {
	/** write len bytes from buffer, return <0 if error or len */
	int (*write)(struct tds_output_stream *stream, size_t len);
	/**
	 * write buffer. client will write data into this buffer.
	 * not required that buffer is the result of any alloc function
	 * so buffer pointer can point in the middle of another buffer.
	 * client will write up to buf_len.
	 * client should not cache buffer and buf_len before a call
	 * to write as write can change these values.
	 */
	char *buffer;
	size_t buf_len;
} TDSOUTSTREAM;

/** Convert a stream from istream to ostream using a specific conversion */
TDSRET tds_convert_stream(TDSSOCKET * tds, TDSICONV * char_conv, TDS_ICONV_DIRECTION direction,
	TDSINSTREAM * istream, TDSOUTSTREAM *ostream);
/** Copy data from a stream to another */
TDSRET tds_copy_stream(TDSSOCKET * tds, TDSINSTREAM * istream, TDSOUTSTREAM * ostream);

/* Additional streams */

/** input stream to read data from tds protocol */
typedef struct tds_datain_stream {
	TDSINSTREAM stream;
	size_t wire_size;
	TDSSOCKET *tds;
} TDSDATAINSTREAM;

void tds_datain_stream_init(TDSDATAINSTREAM * stream, TDSSOCKET * tds, size_t wire_size);

/** output stream to write data to tds protocol */
typedef struct tds_dataout_stream {
	TDSOUTSTREAM stream;
	TDSSOCKET *tds;
	size_t written;
} TDSDATAOUTSTREAM;

void tds_dataout_stream_init(TDSDATAOUTSTREAM * stream, TDSSOCKET * tds);

/** input stream to read data from a static buffer */
typedef struct tds_staticin_stream {
	TDSINSTREAM stream;
	const char *buffer;
	size_t buf_left;
} TDSSTATICINSTREAM;

void tds_staticin_stream_init(TDSSTATICINSTREAM * stream, const void *ptr, size_t len);

/** output stream to write data to a static buffer */
typedef struct tds_staticout_stream {
	TDSOUTSTREAM stream;
} TDSSTATICOUTSTREAM;

void tds_staticout_stream_init(TDSSTATICOUTSTREAM * stream, void *ptr, size_t len);

/** output stream to write data to a dynamic buffer */
typedef struct tds_dynamic_stream {
	TDSOUTSTREAM stream;
	/** where is stored the pointer */
	void **buf;
	/** currently allocated buffer */
	size_t allocated;
	/** size of data inside buffer */
	size_t size;
} TDSDYNAMICSTREAM;

TDSRET tds_dynamic_stream_init(TDSDYNAMICSTREAM * stream, void **ptr, size_t allocated);


#endif

