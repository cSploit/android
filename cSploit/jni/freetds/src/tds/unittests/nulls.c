/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2014 Mikhail Denisenko
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

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 1;
	int rc;
	int i;

	TDSCOLUMN *curcol;
	TDSRESULTINFO *resinfo;

	int result_type;

	printf("%s: Test null values\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}

	rc = tds_submit_query(tds, "SELECT NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL");

	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROW_RESULT:
			resinfo = tds->res_info;
			for (i = 0; i < 14; i++) {
				curcol = resinfo->columns[i];
				if (curcol->column_cur_size != -1) {
					fprintf(stderr, "NULL value expected\n");
					return 1;
				}
			}
		case TDS_COMPUTE_RESULT:
			break;
		default:
			fprintf(stderr, "tds_process_tokens() unexpected result\n");
			break;
		}
	}
	if (rc == TDS_FAIL) {
		fprintf(stderr, "query failed\n");
		return 1;
	}
	if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}

	rc = tds_submit_query(tds, "SELECT 12, 'abc', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL");
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROW_RESULT:
			resinfo = tds->res_info;

			curcol = resinfo->columns[0];
			if (curcol->column_type != SYBINT4) {
				fprintf(stderr, "SYBINT value expected\n");
				return 1;
			}
			if (*(TDS_INT*)curcol->column_data != 12) {
				fprintf(stderr, "invalid integer returned\n");
				return 1;
			}

			curcol = resinfo->columns[1];
			if (curcol->column_type != SYBVARCHAR) {
				fprintf(stderr, "SYBVARCHAR value expected\n");
				return 1;
			}
			if (strcmp((char *) curcol->column_data, "abc") != 0) {
				fprintf(stderr, "invalid string returned\n");
				return 1;
			}

			for (i = 2; i < 14; i++) {
				curcol = resinfo->columns[i];
				if (curcol->column_cur_size != -1) {
					fprintf(stderr, "NULL value expected\n");
					return 1;
				}
			}
		case TDS_COMPUTE_RESULT:
			break;
		default:
			fprintf(stderr, "tds_process_tokens() unexpected result\n");
			break;
		}
	}
	if (rc == TDS_FAIL) {
		fprintf(stderr, "query failed\n");
		return 1;
	}
	if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}

	try_tds_logout(login, tds, verbose);
	return 0;
}
