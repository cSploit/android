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
#include <freetds/convert.h>

static char software_version[] = "$Id: t0006.c,v 1.26 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static TDSCONTEXT ctx;

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 0;
	int rc;
	int row_count, i;

	/* variables for conversions */
	TDSCOLUMN *curcol;
	TDSRESULTINFO *resinfo;
	unsigned char *src;

	CONV_RESULT cr;
	TDS_INT srctype, srclen;

	int src_id = 0;
	double src_val;
	double src_err;
	double tolerance = 0.000001;

	char sql[256];
	int num_sybreal = 5;
	float sybreal[5];
	int num_sybflt8 = 7;
	double sybflt8[7];
	int result_type;

	memset(&ctx, 0, sizeof(ctx));

	sybreal[0] = 1.1;
	sybreal[1] = 12345678;
	sybreal[2] = 0.012345678;
	sybreal[3] = 1.234567890e+20;
	sybreal[4] = 1.234567890e-20;

	sybflt8[0] = 1.1;
	sybflt8[1] = 1234567890123456.0;
	sybflt8[2] = 0.01234567890123456;
	sybflt8[3] = 1.234567890123456e+20;
	sybflt8[4] = 1.234567890123456e-20;
	sybflt8[5] = 1.234567890123456e+200;
	sybflt8[6] = 1.234567890123456e-200;

	printf("%s: Test SYBREAL, SYBFLT8 values\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS) {
		fprintf(stderr, "try_tds_login() failed\n");
		return 1;
	}


	/*
	 * SYBREAL tests
	 */
	if (verbose)
		printf("Starting SYBREAL tests\n");
	rc = run_query(tds, "DROP TABLE #test_table");
	if (rc != TDS_SUCCESS) {
		return 1;
	}
	rc = run_query(tds, "CREATE TABLE #test_table (id int, val real)");
	if (rc != TDS_SUCCESS) {
		return 1;
	}

	for (i = 0; i < num_sybreal; i++) {
		sprintf(sql, "INSERT #test_table (id, val) VALUES (%d, %.8g)", i, sybreal[i]);
		if (verbose)
			printf("%s\n", sql);
		rc = run_query(tds, sql);
		if (rc != TDS_SUCCESS) {
			return 1;
		}
	}

	rc = tds_submit_query(tds, "SELECT * FROM #test_table");

	row_count = 0;
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROW_RESULT:
			resinfo = tds->res_info;
			for (i = 0; i < resinfo->num_cols; i++) {
				curcol = resinfo->columns[i];
				src = curcol->column_data;
				if (verbose) {
					srctype = curcol->column_type;
					srclen = curcol->column_size;
					tds_convert(&ctx, srctype, (TDS_CHAR *) src, srclen, SYBCHAR, &cr);
					printf("col %i is %s\n", i, cr.c);
				}
				if (i == 0) {
					src_id = *(int *) src;
				} else {
					src_val = *(float *) src;
					src_err = src_val - sybreal[src_id];
					if (src_err != 0.0) {
						src_err = src_err / src_val;
					}
					if (src_err < -tolerance || src_err > tolerance) {
						fprintf(stderr, "SYBREAL expected %.8g  got %.8g\n",
							sybreal[src_id], src_val);
						fprintf(stderr, "Error was %.4g%%\n", 100 * src_err);
						return 1;
					}
				}
			}
			row_count++;
		case TDS_COMPUTE_RESULT:
			break;
		default:
			fprintf(stderr, "tds_process_tokens() unexpected result\n");
			break;
		}
	}
	if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}


	/*
	 * SYBFLT8 tests
	 */
	if (verbose)
		printf("Starting SYBFLT8 tests\n");
	rc = run_query(tds, "DROP TABLE #test_table");
	if (rc != TDS_SUCCESS) {
		return 1;
	}
	rc = run_query(tds, "CREATE TABLE #test_table (id int, val float(48))");
	if (rc != TDS_SUCCESS) {
		return 1;
	}

	for (i = 0; i < num_sybflt8; i++) {
		sprintf(sql, "INSERT #test_table (id, val) VALUES (%d, %.15g)", i, sybflt8[i]);
		if (verbose)
			printf("%s\n", sql);
		rc = run_query(tds, sql);
		if (rc != TDS_SUCCESS) {
			return 1;
		}
	}

	rc = tds_submit_query(tds, "SELECT * FROM #test_table");
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCESS) {
		switch (result_type) {
		case TDS_ROW_RESULT:
			resinfo = tds->res_info;
			for (i = 0; i < resinfo->num_cols; i++) {
				curcol = resinfo->columns[i];
				src = curcol->column_data;
				if (verbose) {
					srctype = curcol->column_type;
					srclen = curcol->column_size;
					tds_convert(&ctx, srctype, (TDS_CHAR *) src, srclen, SYBCHAR, &cr);
					printf("col %i is %s\n", i, cr.c);
				}
				if (i == 0) {
					src_id = *(int *) src;
				} else {
					memcpy(&src_val, src, 8);
					src_err = src_val - sybflt8[src_id];
					if (src_err != 0.0) {
						src_err = src_err / src_val;
					}
					if (src_err < -tolerance || src_err > tolerance) {
						fprintf(stderr, "SYBFLT8 expected %.16g  got %.16g\n",
							sybflt8[src_id], src_val);
						fprintf(stderr, "Error was %.4g%%\n", 100 * src_err);
						return 1;
					}
				}
			}
		case TDS_COMPUTE_RESULT:
			break;
		default:
			fprintf(stderr, "tds_process_tokens() returned unexpected result\n");
			break;
		}
	}
	if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
	}

	try_tds_logout(login, tds, verbose);
	return 0;
}
