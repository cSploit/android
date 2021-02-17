/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
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

static char software_version[] = "$Id: t0005.c,v 1.19 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

char *value_as_string(TDSSOCKET * tds, int col_idx);

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 0;
	int rc;
	int i;

	int result_type;

	const char *len200 =
		"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
	char large_sql[1000];

	fprintf(stdout, "%s: Test large (>512 bytes) replies\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	/* do not test error, remove always table */
	rc = run_query(tds, "DROP TABLE #test_table");
	rc = run_query(tds, "CREATE TABLE #test_table (id int, name varchar(255))");
	if (rc != TDS_SUCCESS) {
		return 1;
	}

	sprintf(large_sql, "INSERT #test_table (id, name) VALUES (0, 'A%s')", len200);
	rc = run_query(tds, large_sql);
	if (rc != TDS_SUCCESS) {
		return 1;
	}
	sprintf(large_sql, "INSERT #test_table (id, name) VALUES (1, 'B%s')", len200);
	rc = run_query(tds, large_sql);
	if (rc != TDS_SUCCESS) {
		return 1;
	}
	sprintf(large_sql, "INSERT #test_table (id, name) VALUES (2, 'C%s')", len200);
	rc = run_query(tds, large_sql);
	if (rc != TDS_SUCCESS) {
		return 1;
	}

	/*
	 * The heart of the test
	 */
	rc = tds_submit_query(tds, "SELECT * FROM #test_table");
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROW)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROW_RESULT:
			for (i = 0; i < tds->res_info->num_cols; i++) {
				if (verbose) {
					printf("col %i is %s\n", i, value_as_string(tds, i));
				}
			}
			break;
		default:
			fprintf(stderr, "tds_process_tokens() returned unexpected result\n");
			break;
		}
	}
	if (rc == TDS_FAIL) {
		fprintf(stderr, "tds_process_tokens() returned TDS_FAIL for SELECT\n");
		return 1;
	} else if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}

	/* do not test error, remove always table */
	rc = run_query(tds, "DROP TABLE #test_table");

	try_tds_logout(login, tds, verbose);
	return 0;
}

char *
value_as_string(TDSSOCKET * tds, int col_idx)
{
	static char result[256];
	const int type = tds->res_info->columns[col_idx]->column_type;
	const void *value = tds->res_info->columns[col_idx]->column_data;

	switch (type) {
	case SYBVARCHAR:
		strncpy(result, (const char *) value, sizeof(result) - 1);
		result[sizeof(result) - 1] = '\0';
		break;
	case SYBINT4:
		sprintf(result, "%d", *(const int *) value);
		break;
	default:
		sprintf(result, "Unexpected column_type %d", type);
		break;
	}
	return result;
}
