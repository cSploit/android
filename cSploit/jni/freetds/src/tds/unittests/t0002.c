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

static char software_version[] = "$Id: t0002.c,v 1.15 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

char *value_as_string(TDSSOCKET * tds, int col_idx);

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
}				/* value_as_string()  */


int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 0;
	int num_cols = 2;
	TDS_INT result_type;
	int rc;
	int i, done_flags;

	fprintf(stdout, "%s: Test basic submit query, results\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	rc = tds_submit_query(tds, "select db_name() dbname, user_name() username");
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "tds_submit_query() failed\n");
		return 1;
	}

	while ((rc = tds_process_tokens(tds, &result_type, &done_flags, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROWFMT_RESULT:
			if (tds->res_info->num_cols != num_cols) {
				fprintf(stderr, "Error:  num_cols != %d in %s\n", num_cols, __FILE__);
				return 1;
			}
			if (tds->res_info->columns[0]->column_type != SYBVARCHAR
			    || tds->res_info->columns[1]->column_type != SYBVARCHAR) {
				fprintf(stderr, "Wrong column_type in %s\n", __FILE__);
				return 1;
			}
			if (strcmp(tds->res_info->columns[0]->column_name, "dbname")
			    || strcmp(tds->res_info->columns[1]->column_name, "username")) {
				fprintf(stderr, "Wrong column_name in %s\n", __FILE__);
				return 1;
			}
			break;

		case TDS_ROW_RESULT:

			while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {
				if (result_type != TDS_ROW_RESULT || result_type != TDS_COMPUTE_RESULT)
					break;
				if (verbose) {
					for (i = 0; i < num_cols; i++) {
						printf("col %i is %s\n", i, value_as_string(tds, i));
					}
				}
			}
			if (rc != TDS_SUCCESS) {
				fprintf(stderr, "tds_process_tokens() unexpected return\n");
			}
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
			if (!(done_flags & TDS_DONE_ERROR))
				break;

		default:
			fprintf(stderr, "tds_process_tokens() unexpected result_type\n");
			break;
		}
	}
	if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}

	try_tds_logout(login, tds, verbose);
	return 0;
}
