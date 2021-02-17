/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2014   Frediano Ziglio
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

/*
 * This test test tds_convert_stream which is supposed to be the main
 * character conversion routine
 *
 * Check that error are reported to error handler if found and tds
 * is not NULL.
 *
 * Check that conversions works with NULL tds.
 *
 * Check all types of errors (EILSEQ, EINVAL, E2BIG).
 *
 * Check that error are capture in middle and on end of stream.
 */

#include "common.h"
#include <freetds/iconv.h>
#include <freetds/stream.h>

#if HAVE_UNISTD_H
#undef getpid
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <assert.h>

/* test tds_iconv_fread */

static char buf[4096+80];
static char buf_out[4096+80];

int last_errno = 0;

static TDSRET
convert(TDSSOCKET *tds, TDSICONV *conv, TDS_ICONV_DIRECTION direction,
	const char *from, size_t from_len, char *dest, size_t *dest_len)
{
	/* copy to make valgrind test fail on memory problems */
	char *in = malloc(from_len);
	char *out = malloc(*dest_len);
	int res;
	TDSSTATICINSTREAM r;
	TDSSTATICOUTSTREAM w;

	assert(in && out);

	memcpy(in, from, from_len);
	tds_staticin_stream_init(&r, in, from_len);
	tds_staticout_stream_init(&w, out, *dest_len);
	last_errno = 0;
	res = tds_convert_stream(tds, conv, direction, &r.stream, &w.stream);
	last_errno = errno;
	memcpy(dest, out, *dest_len - w.stream.buf_len);
	*dest_len = *dest_len - w.stream.buf_len;

	free(in);
	free(out);
	return res;
}

enum Odd {
	ODD_NONE = 0,
	ODD_NORMAL,
	ODD_INVALID,
	ODD_INCOMPLETE
};

static const char *odd_names[] = {
	"None", "Normal", "Invalid", "Incomplete"
};

static int
add_odd(char *buf, int *pos, enum Odd type)
{
	const unsigned char x = 0xa0;

	switch (type) {
	case ODD_NONE:
		return 0;

	case ODD_NORMAL:
		buf[*pos] = 0xC0 + (x >> 6);
		++*pos;
		buf[*pos] = 0x80 + (x & 0x3f);
		++*pos;
		return 0;

	case ODD_INVALID:
		buf[*pos] = 0xff;
		++*pos;
		return EILSEQ;

	case ODD_INCOMPLETE:
		buf[*pos] = 0xC2;
		++*pos;
		return EINVAL;

	default:
		assert(0);
	}
	return 0;
}

static void
add_odd2(char *buf, int *pos, enum Odd type)
{
	const unsigned char x = 0xa0;

	switch (type) {
	case ODD_NONE:
		return;
	case ODD_NORMAL:
		buf[*pos] = x;
		++*pos;
		break;
	case ODD_INVALID:
		break;
	case ODD_INCOMPLETE:
		break;
	default:
		assert(0);
	}	
}

static int captured_errno = 0;

static int
err_handler(const TDSCONTEXT * tds_ctx, TDSSOCKET * tds, TDSMESSAGE * msg)
{
	int old_err = captured_errno;
	if (msg->msgno == TDSEICONVAVAIL) {
		captured_errno = EINVAL;
	} else if (strstr(msg->message, "could not be converted")) {
		captured_errno = EILSEQ;
	} else if (msg->msgno == TDSEICONVIU) {
		captured_errno = E2BIG;
	} else {
		fprintf(stderr, "Unexpected error: %s\n", msg->message);
		exit(1);
	}
	assert(old_err == 0 || old_err == captured_errno);
	return TDS_INT_CANCEL;
}

static TDSICONV * conv = NULL;
static int pos_type = 0;

static void
test(TDSSOCKET *tds, enum Odd odd_type)
{
	int i, l;

	captured_errno = 0;

	/* do not complete incomplete */
	if (odd_type == ODD_INCOMPLETE && pos_type > 0)
		return;

	printf("test pos %d type %s\n", pos_type, odd_names[odd_type]);

	for (i = 0; i < 4096+20; ++i) {
		size_t out_len;
		TDSRET res;
		int err;

		if (i == 34)
			i = 4096-20;

		l = i;
		memset(buf, 'a', sizeof(buf));
		switch (pos_type) {
		case 0: /* end */
			err = add_odd(buf, &l, odd_type);
			break;
		case 1: /* start */
			l = 0;
			err = add_odd(buf, &l, odd_type);
			if (l > i) continue;
			l = i;
			break;
		case 2: /* middle */
			err = add_odd(buf, &l, odd_type);
			l = 4096+30;
			break;
		}

		/* convert it */
		out_len = sizeof(buf_out);
		res = convert(tds, conv, to_server, buf, l, buf_out, &out_len);
		printf("i %d res %d out_len %u errno %d captured_errno %d\n",
		       i, (int) res, (unsigned int) out_len, last_errno, captured_errno);

		/* test */
		l = i;
		memset(buf, 'a', sizeof(buf));
		switch (pos_type) {
		case 0: /* end */
			add_odd2(buf, &l, odd_type);
			break;
		case 1: /* start */
			l = 0;
			add_odd2(buf, &l, odd_type);
			l = i;
			if (odd_type == ODD_NORMAL) l = i-1;
			if (odd_type == ODD_INVALID) l = 0;
			break;
		case 2: /* middle */
			add_odd2(buf, &l, odd_type);
			l = 4096+30;
			if (odd_type == ODD_NORMAL) --l;
			if (odd_type == ODD_INVALID) l = i;
			break;
		}


		if (err) {
			assert(last_errno == err);
			assert(TDS_FAILED(res));
			assert(!tds || captured_errno == last_errno);
		} else {
			assert(TDS_SUCCEED(res));
			assert(captured_errno == 0);
		}
		if (out_len != l) {
			fprintf(stderr, "out %u bytes expected %d\n",
				(unsigned int) out_len, l);
			exit(1);
		}
		assert(memcmp(buf_out, buf, l) == 0);
	}
}

static void
big_test(TDSSOCKET *tds)
{
	int i, l;
	const int limit = 1025;

	captured_errno = 0;

	printf("E2BIG test\n");

	for (i = 0; i < 4096+20; ++i) {
		size_t out_len;
		TDSRET res;
		int err;

		if (i == 32)
			i = 490;

		l = i;
		memset(buf, 0xa0, sizeof(buf));

		/* convert it */
		out_len = limit;
		res = convert(tds, conv, to_client, buf, l, buf_out, &out_len);
		printf("i %d res %d out_len %u errno %d captured_errno %d\n",
		       i, (int) res, (unsigned int) out_len, last_errno, captured_errno);
		err = l * 2 > limit ? E2BIG : 0;

		if (err) {
			assert(last_errno == err);
			assert(TDS_FAILED(res));
			assert(!tds || captured_errno == last_errno);
		} else {
			assert(TDS_SUCCEED(res));
			assert(captured_errno == 0);
		}
		if (out_len != i*2 && i*2 <= limit) {
			fprintf(stderr, "out %u bytes expected %d\n",
				(unsigned int) out_len, i*2);
			exit(1);
		}
	}
}


int
main(int argc, char **argv)
{
	int i;
	TDSCONTEXT *ctx = tds_alloc_context(NULL);
	TDSSOCKET *tds = tds_alloc_socket(ctx, 512);
	const char *tdsdump;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	ctx->err_handler = err_handler;

	/* allow dumps, we don't have a connection here */
	tdsdump = getenv("TDSDUMP");
	if (tdsdump)
		tdsdump_open(tdsdump);

	if (!ctx || !tds) {
		fprintf(stderr, "Error creating socket!\n");
		return 1;
	}

	tds_iconv_open(tds_conn(tds), "ISO-8859-1");

	conv = tds_iconv_get(tds_conn(tds), "UTF-8", "ISO-8859-1");
	if (conv == NULL) {
		fprintf(stderr, "Error creating conversion, giving up!\n");
		return 1;
	}

	for (i = 0; i < 4*3*2; ++i) {
		int n = i;
		int odd_type = n % 4; n /= 4;
		pos_type = n % 3; n /= 3;

		test(n ? tds : NULL, odd_type);
	}

	/* not try E2BIG error */
	big_test(NULL);
	big_test(tds);

	tds_free_socket(tds);
	tds_free_context(ctx);
	return 0;
}
