/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2003, 2004  Frediano Ziglio
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
#include "common.h"

#include <ctype.h>
#include <assert.h>

/* try conversion from utf8 to iso8859-1 */
static char software_version[] = "$Id: utf8_2.c,v 1.17 2011-06-03 21:04:15 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static TDSSOCKET *tds;
static int g_result = 0;
static int einval_error = 0;
static int eilseq_error = 0;
static int einval_count = 0;
static int eilseq_count = 0;
static int e2big_count = 0;
static char test_name[128];

static int invalid_char = -1;

static void
test(int n, int type)
{
	int rc;
	TDS_INT result_type;
	char buf[1024], tmp[1024];
	TDSCOLUMN *curcol;
	char *src;
	int done_flags;
	int i;
	char prefix[32], suffix[32];

	sprintf(test_name, "test %d len %d", type, n);

	/* do a select and check all results */
	prefix[0] = 0;
	suffix[0] = 0;
	tmp[0] = 0;
	switch (type) {
	case 0:
		strcpy(suffix, "C280C290");
		break;
	case 1:
		/* try two invalid in different part */
		strcpy(prefix, "C480C290");
		strcpy(suffix, "C480C290");
		break;
	}

	for (i = 0; i < n; ++i)
		sprintf(strchr(tmp, 0), "%02X", 0x30 + (i % 10));
	sprintf(buf, "select convert(varchar(255), 0x%s%s%s)", prefix, tmp, suffix);
	rc = tds_submit_query(tds, buf);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "tds_submit_query() failed\n");
		exit(1);
	}

	if (tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS) != TDS_SUCCESS) {
		fprintf(stderr, "tds_process_tokens() failed\n");
		exit(1);
	}

	if (result_type != TDS_ROWFMT_RESULT) {
		fprintf(stderr, "expected row fmt() failed\n");
		exit(1);
	}

	if (tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS) != TDS_SUCCESS) {
		fprintf(stderr, "tds_process_tokens() failed\n");
		exit(1);
	}

	if (result_type != TDS_ROW_RESULT) {
		fprintf(stderr, "expected row result() failed\n");
		exit(1);
	}

	/* force tds to convert from utf8 to iso8859-1 (even on Sybase) */
	tds_srv_charset_changed(tds->conn, "UTF-8");
	tds->current_results->columns[0]->char_conv = tds->conn->char_convs[client2server_chardata];

	rc = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "tds_process_tokens() failed\n");
		exit(1);
	}

	curcol = tds->current_results->columns[0];
	src = (char*) curcol->column_data;

	if (is_blob_col(curcol)) {
		TDSBLOB *blob = (TDSBLOB *) src;

		src = blob->textvalue;
	}

	prefix[0] = 0;
	suffix[0] = 0;
	tmp[0] = 0;
	switch (type) {
	case 0:
		strcpy(suffix, "\x80\x90");
		break;
	case 1:
		/* try two invalid in different part */
		strcpy(prefix, "?\x90");
		strcpy(suffix, "?\x90");
		/* some platforms replace invalid sequence with a fixed char */
		if (invalid_char < 0)
			invalid_char = (unsigned char) src[0];
		prefix[0] = (char) invalid_char;
		suffix[0] = (char) invalid_char;
		break;
	}

	for (i = 0; i < n; ++i)
		sprintf(strchr(tmp, 0), "%c", "0123456789"[i % 10]);
	sprintf(buf, "%s%s%s", prefix, tmp, suffix);

	if (strlen(buf) != curcol->column_cur_size || strncmp(buf, src, curcol->column_cur_size) != 0) {
		int l = curcol->column_cur_size;

		if (l > 1000)
			l = 1000;
		strncpy(tmp, src, l);
		tmp[l] = 0;
		fprintf(stderr, "Wrong result in %s\n Got: '%s' len %d\n Expected: '%s' len %u\n", test_name, tmp,
			curcol->column_cur_size, buf, (unsigned int) strlen(buf));
		exit(1);
	}

	rc = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE);
	if (rc != TDS_SUCCESS || result_type == TDS_ROW_RESULT) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
		exit(1);
	}

	while ((rc = tds_process_tokens(tds, &result_type, &done_flags, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_NO_MORE_RESULTS:
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
			if (!(done_flags & TDS_DONE_ERROR))
				break;

		default:
			fprintf(stderr, "tds_process_tokens() unexpected result_type\n");
			exit(1);
			break;
		}
	}
}

static int
err_handler(const TDSCONTEXT * tds_ctx, TDSSOCKET * tds, TDSMESSAGE * msg)
{
	int error = 0;

	if (strstr(msg->message, "EINVAL")) {
		++einval_count;
		if (einval_error)
			error = 1;
	} else if (strstr(msg->message, "could not be converted")) {
		++eilseq_count;
		if (eilseq_error)
			error = 1;
	} else if (strstr(msg->message, "E2BIG")) {
		++e2big_count;
		error = 1;
	} else {
		error = 1;
	}

	if (error) {
		fprintf(stderr, "Unexpected in %s error: %s\n", test_name, msg->message);
		g_result = 1;
	}
	return TDS_INT_CANCEL;
}

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	int ret;
	int verbose = 0;
	int i;
	typedef int (*perr)(const TDSCONTEXT *, TDSSOCKET *, TDSMESSAGE *);
	const perr * my_err;

	/* use ISO8859-1 as our coding */
	strcpy(CHARSET, "ISO8859-1");

	ret = try_tds_login(&login, &tds, __FILE__, verbose);
	if (ret != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	/* override a const in a safe way */
	my_err = &tds_get_ctx(tds)->err_handler;
	*((perr*)my_err) = err_handler;

	/* prepend some characters to check part of sequence error */
	einval_error = 1;
	eilseq_error = 1;
	/* do not stop on first error so we check conversion correct */
	for (i = 0; i <= 192; ++i)
		test(i, 0);

	/* try creating a double conversion warning */
	eilseq_error = 0;
	einval_error = 0;	/* we already tested this error above */
	for (i = 0; i <= 192; ++i) {
		eilseq_count = 0;
		test(i, 1);
		if (eilseq_count > 1) {
			fprintf(stderr, "Two warnings returned instead of one in %s\n", test_name);
			g_result = 1;
			break;
		}
		if (eilseq_count < 1 && invalid_char == '?') {
			fprintf(stderr, "No warning returned in %s\n", test_name);
			g_result = 1;
		}
	}

	try_tds_logout(login, tds, verbose);
	return g_result;
}
