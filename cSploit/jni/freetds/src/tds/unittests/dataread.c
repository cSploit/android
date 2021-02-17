/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
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

#include <assert.h>
#include <freetds/convert.h>

static char software_version[] = "$Id: dataread.c,v 1.21 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int g_result = 0;
static TDSLOGIN *login;
static TDSSOCKET *tds;

static void test0(const char *type, ...);

static void
test(const char *type, const char *value, const char *result)
{
	test0(type, value, result, NULL);
}

static void
exec_query(const char *query)
{
	if (tds_submit_query(tds, query) != TDS_SUCCESS || tds_process_simple_query(tds) != TDS_SUCCESS) {
                fprintf(stderr, "executing query failed\n");
                exit(1);
        }
}

static void
test0(const char *type, ...)
{
	char buf[512];
	CONV_RESULT cr;
	int rc;
	TDS_INT result_type;
	int done_flags;
	va_list ap;
	struct {
		const char *value;
		const char *result;
	} data[10];
	int num_data = 0, i_row;

	sprintf(buf, "CREATE TABLE #tmp(a %s)", type);
	exec_query(buf);

        va_start(ap, type);
	for (;;) {
		const char * value = va_arg(ap, const char *);
		const char * result;
		if (!value)
			break;
		result = va_arg(ap, const char *);
		if (!result)
			result = value;
		data[num_data].value = value;
		data[num_data].result = result;
		sprintf(buf, "INSERT INTO #tmp VALUES(CONVERT(%s,'%s'))", type, value);
		exec_query(buf);
		++num_data;
	}
        va_end(ap);

	assert(num_data > 0);

	/* execute it */
	rc = tds_submit_query(tds, "SELECT * FROM #tmp");
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

	i_row = 0;
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS && (result_type == TDS_ROW_RESULT || result_type == TDS_COMPUTE_RESULT)) {

		TDSCOLUMN *curcol = tds->current_results->columns[0];
		TDS_CHAR *src = (TDS_CHAR *) curcol->column_data;
		int conv_type = tds_get_conversion_type(curcol->column_type, curcol->column_size);

		assert(i_row < num_data);

		if (is_blob_col(curcol)) {
			TDSBLOB *blob = (TDSBLOB *) src;

			src = blob->textvalue;
		}

		if (tds_convert(test_context, conv_type, src, curcol->column_cur_size, SYBVARCHAR, &cr) < 0) {
			fprintf(stderr, "Error converting\n");
			g_result = 1;
		} else {
			if (strcmp(data[i_row].result, cr.c) != 0) {
				fprintf(stderr, "Failed! Is \n%s\nShould be\n%s\n", cr.c, data[i_row].result);
				g_result = 1;
			}
			free(cr.c);
		}
		++i_row;
	}

	if (rc != TDS_NO_MORE_RESULTS && rc != TDS_SUCCESS) {
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

	exec_query("DROP TABLE #tmp");
}

int
main(int argc, char **argv)
{
	fprintf(stdout, "%s: Testing conversion from server\n", __FILE__);
	if (try_tds_login(&login, &tds, __FILE__, 0) != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	/* bit */
	test("BIT", "0", NULL);
	test("BIT", "1", NULL);

	/* integers */
	test("TINYINT", "234", NULL);
	test("SMALLINT", "-31789", NULL);
	test("INT", "16909060", NULL);

	/* floating point */
	test("REAL", "1.23", NULL);
	test("FLOAT", "-49586.345", NULL);

	/* money */
	test("MONEY", "-123.3400", "-123.34");
	test("MONEY", "-123.3450", "-123.35");
	test("MONEY", "123.3450", "123.35");
	/* very long money, this test int64 operations too */
	test("MONEY", "123456789012345.67", NULL);
	/* test smaller money */
	test("MONEY", "-922337203685477.5808", "-922337203685477.58");
	test("SMALLMONEY", "89123.12", NULL);
	test("SMALLMONEY", "-123.3400", "-123.34");
	test("SMALLMONEY", "-123.3450", "-123.35");
	test("SMALLMONEY", "123.3450", "123.35");
	/* test smallest smallmoney */
	test("SMALLMONEY", "-214748.3648", "-214748.36");

	/* char */
	test("CHAR(10)", "pippo", "pippo     ");
	test("VARCHAR(20)", "pippo", NULL);
	test0("TEXT", "a", NULL, "foofoo", NULL, "try with a relatively long value, we hope for the best", NULL, NULL);

	/* binary */
	test("VARBINARY(6)", "foo", "666f6f");
	test("BINARY(6)", "foo", "666f6f000000");
	test0("IMAGE", "foo", "666f6f", "foofoofoofoo", "666f6f666f6f666f6f666f6f", NULL);

	/* numeric */
	test("NUMERIC(10,2)", "12765.76", NULL);
	test("NUMERIC(18,4)", "12765.761234", "12765.7612");

	/* date */
	free(test_context->locale->date_fmt);
	test_context->locale->date_fmt = strdup("%Y-%m-%d %H:%M:%S");

	test("DATETIME", "2003-04-21 17:50:03", NULL);
	test("SMALLDATETIME", "2003-04-21 17:50:03", "2003-04-21 17:50:00");

	if (IS_TDS7_PLUS(tds->conn)) {
		test("UNIQUEIDENTIFIER", "12345678-1234-A234-9876-543298765432", NULL);
		test("NVARCHAR(20)", "Excellent test", NULL);
		test("NCHAR(20)", "Excellent test", "Excellent test      ");
		test("NTEXT", "Excellent test", NULL);
	}

	try_tds_logout(login, tds, 0);
	return g_result;
}
