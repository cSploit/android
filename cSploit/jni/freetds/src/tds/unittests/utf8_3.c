/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2010 Frediano Ziglio
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

static char software_version[] = "$Id: utf8_3.c,v 1.8 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static TDSSOCKET *tds;

static void
test(const char *buf)
{
	char query[1024];
	char tmp[129 * 3];
	int i;
	int rc;
	TDS_INT result_type;
	int done_flags;

	to_utf8(buf, tmp);
	sprintf(query, "SELECT 1 AS [%s]", tmp);

	/* do a select and check all results */
	rc = tds_submit_query(tds, query);
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

	i = 0;
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_STOPAT_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {

		TDSCOLUMN *curcol;

		if (result_type != TDS_ROW_RESULT)
			break;

		curcol = tds->current_results->columns[0];

		if (strlen(tmp) != curcol->column_namelen || strncmp(tmp, curcol->column_name, curcol->column_namelen) != 0) {
			int l = curcol->column_namelen;

			if (l > (sizeof(query) -1))
				l = (sizeof(query) -1);
			strncpy(query, curcol->column_name, l);
			query[l] = 0;
			fprintf(stderr, "Wrong result Got: '%s' len %d\n        Expected: '%s' len %u\n", query,
				curcol->column_namelen, tmp, (unsigned int) strlen(tmp));
			exit(1);
		}
		++i;
	}

	if (rc != TDS_SUCCESS || result_type == TDS_ROW_RESULT || result_type == TDS_COMPUTE_RESULT) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
		exit(1);
	}

	while ((rc = tds_process_tokens(tds, &result_type, &done_flags, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_NO_MORE_RESULTS:
			return;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
			if (!(done_flags & TDS_DONE_ERROR))
				break;

		default:
			fprintf(stderr, "tds_proces_tokens() unexpected result_type\n");
			exit(1);
			break;
		}
	}
}

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	int ret;
	int verbose = 0;

	/* use UTF-8 as our coding */
	strcpy(CHARSET, "UTF-8");

	ret = try_tds_login(&login, &tds, __FILE__, verbose);
	if (ret != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	if (IS_TDS7_PLUS(tds->conn)) {
		char buf[129 * 8];
		int i;

		/* build a string of length 128 */
		strcpy(buf, "");
		for (i = 1; i <= 128; ++i) {
			sprintf(strchr(buf, 0), "&#x%04x;", 0x4000 + i);
		}

		/* do all test */
		for (i = 1;;) {
			printf("Testing len %d\n", i);
			test(buf + 8 * (128 - i));
			if (i == 128)
				break;
			++i;
			if (i > 12)
				i += 3;
			if (i >= 128)
				i = 128;
		}
	}

	try_tds_logout(login, tds, verbose);
	return 0;
}
