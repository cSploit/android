/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
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

static char software_version[] = "$Id: utf8_1.c,v 1.15 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static TDSSOCKET *tds;

/* Some no-ASCII strings (XML coding) */
static const char english[] = "English";
static const char spanish[] = "Espa&#241;ol";
static const char french[] = "Fran&#231;ais";
static const char portuguese[] = "Portugu&#234;s";
static const char russian[] = "&#1056;&#1091;&#1089;&#1089;&#1082;&#1080;&#1081;";
static const char arabic[] = "&#x0627;&#x0644;&#x0639;&#x0631;&#x0628;&#x064a;&#x0629;";
static const char chinese[] = "&#x7b80;&#x4f53;&#x4e2d;&#x6587;";
static const char japanese[] = "&#26085;&#26412;&#35486;";
static const char hebrew[] = "&#x05e2;&#x05d1;&#x05e8;&#x05d9;&#x05ea;";

static const char *strings[] = {
	english,
	spanish,
	french,
	portuguese,
	russian,
	arabic,
	chinese,
	japanese,
	hebrew,
	NULL,			/* will be replaced with large data */
	NULL,			/* will be replaced with large data */
	NULL,			/* will be replaced with large data */
	NULL,			/* will be replaced with large data */
	NULL
};

static void
query(const char *sql)
{
	if (run_query(tds, sql) != TDS_SUCCESS) {
		fprintf(stderr, "error executing query: %s\n", sql);
		exit(1);
	}
}

static void
test(const char *type, const char *test_name)
{
	char buf[256];
	char tmp[256];
	int i;
	const char **s;
	int rc;
	TDS_INT result_type;
	int done_flags;

	sprintf(buf, "CREATE TABLE #tmp (i INT, t %s)", type);
	query(buf);

	/* insert all test strings in table */
	for (i = 0, s = strings; *s; ++s, ++i) {
		sprintf(buf, "insert into #tmp values(%d, N'%s')", i, to_utf8(*s, tmp));
		query(buf);
	}

	/* do a select and check all results */
	rc = tds_submit_query(tds, "select t from #tmp order by i");
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
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROWFMT|TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {

		switch (result_type) {
		case TDS_ROW_RESULT: {
			TDSCOLUMN *curcol = tds->current_results->columns[0];
			char *src = (char *) curcol->column_data;

			if (is_blob_col(curcol)) {
				TDSBLOB *blob = (TDSBLOB *) src;

				src = blob->textvalue;
			}

			strcpy(buf, to_utf8(strings[i], tmp));

			if (strlen(buf) != curcol->column_cur_size || strncmp(buf, src, curcol->column_cur_size) != 0) {
				int l = curcol->column_cur_size;

				if (l > 200)
					l = 200;
				strncpy(tmp, src, l);
				tmp[l] = 0;
				fprintf(stderr, "Wrong result in test %s\n Got: '%s' len %d\n Expected: '%s' len %u\n", test_name, tmp,
					curcol->column_cur_size, buf, (unsigned int) strlen(buf));
				exit(1);
			}
			++i;
			} break;
		default:
			fprintf(stderr, "Unexpected result\n");
			exit(1);
			break;
		}
	}

	if (rc != TDS_NO_MORE_RESULTS) {
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
			fprintf(stderr, "tds_process_tokens() unexpected result_type\n");
			exit(1);
			break;
		}
	}

	query("DROP TABLE #tmp");

	/* do sone select to test results */
	/*
	 * for (s = strings; *s; ++s) {
	 * printf("%s\n", to_utf8(*s, tmp));
	 * }
	 */
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
		char type[32];
		char buf[1024];
		int i, len;

		strcpy(buf, "aaa");
		len = 0;
		for (i = 0; strlen(buf) < 980 && len < 200; ++i) {
			char tmp[256];

			strcat(buf, japanese);
			len += strlen(to_utf8(japanese, tmp));
		}
		strings[sizeof(strings) / sizeof(strings[0]) - 5] = buf + 3;
		strings[sizeof(strings) / sizeof(strings[0]) - 4] = buf + 2;
		strings[sizeof(strings) / sizeof(strings[0]) - 3] = buf + 1;
		strings[sizeof(strings) / sizeof(strings[0]) - 2] = buf;

		test("NVARCHAR(500)", "NVARCHAR with large size");

		sprintf(type, "NVARCHAR(%d)", utf8_max_len);
		test(type, "NVARCHAR with sufficient size");

		test("NTEXT", "TEXT");

		/* TODO test parameters */
	}

	try_tds_logout(login, tds, verbose);
	return 0;
}
