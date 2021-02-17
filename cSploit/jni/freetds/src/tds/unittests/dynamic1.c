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

static char software_version[] = "$Id: dynamic1.c,v 1.21 2011-08-08 16:57:42 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int discard_result(TDSSOCKET * tds);

static void
fatal_error(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

static void
test(TDSSOCKET * tds, TDSDYNAMIC * dyn, TDS_INT n, const char *s)
{
	TDSPARAMINFO *params;
	TDSCOLUMN *curcol;
	int len = (int)strlen(s);

	tds_free_input_params(dyn);

	if (!(params = tds_alloc_param_result(dyn->params)))
		fatal_error("out of memory!");
	dyn->params = params;

	curcol = params->columns[0];
	tds_set_param_type(tds->conn, curcol, SYBINT4);

	/* TODO test error */
	tds_alloc_param_data(curcol);
	curcol->column_cur_size = sizeof(TDS_INT);
	memcpy(curcol->column_data, &n, sizeof(n));

	if (!(params = tds_alloc_param_result(dyn->params)))
		fatal_error("out of memory!");
	dyn->params = params;

	curcol = params->columns[1];
	tds_set_param_type(tds->conn, curcol, SYBVARCHAR);
	curcol->column_size = 40;
	curcol->column_cur_size = len;

	tds_alloc_param_data(curcol);
	memcpy(curcol->column_data, s, len);

	if (tds_submit_execute(tds, dyn) != TDS_SUCCESS)
		fatal_error("tds_submit_execute() error");
	if (discard_result(tds) != TDS_SUCCESS)
		fatal_error("tds_submit_execute() output error");
}

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 0;
	TDSDYNAMIC *dyn = NULL;
	int rc;

	fprintf(stdout, "%s: Test dynamic queries\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS)
		fatal_error("try_tds_login() failed");

	run_query(tds, "DROP TABLE #dynamic1");
	if (run_query(tds, "CREATE TABLE #dynamic1 (i INT, c VARCHAR(40))") != TDS_SUCCESS)
		fatal_error("creating table error");

	if (tds->cur_dyn)
		fatal_error("already a dynamic query??");

	/* prepare to insert */
	if (tds_submit_prepare(tds, "INSERT INTO #dynamic1(i,c) VALUES(?,?)", NULL, &dyn, NULL) != TDS_SUCCESS)
		fatal_error("tds_submit_prepare() error");
	if (discard_result(tds) != TDS_SUCCESS)
		fatal_error("tds_submit_prepare() output error");

	if (!dyn)
		fatal_error("dynamic not present??");

	/* insert one record */
	test(tds, dyn, 123, "dynamic");

	/* some test */
	if (run_query(tds, "DECLARE @n INT SELECT @n = COUNT(*) FROM #dynamic1 IF @n <> 1 SELECT 0") != TDS_SUCCESS)
		fatal_error("checking rows");

	if (run_query(tds, "DECLARE @n INT SELECT @n = COUNT(*) FROM #dynamic1 WHERE i = 123 AND c = 'dynamic' IF @n <> 1 SELECT 0")
	    != TDS_SUCCESS)
		fatal_error("checking rows 1");

	/* insert one record */
	test(tds, dyn, 654321, "a longer string");

	/* some test */
	if (run_query(tds, "DECLARE @n INT SELECT @n = COUNT(*) FROM #dynamic1 IF @n <> 2 SELECT 0") != TDS_SUCCESS)
		fatal_error("checking rows");

	if (run_query(tds, "DECLARE @n INT SELECT @n = COUNT(*) FROM #dynamic1 WHERE i = 123 AND c = 'dynamic' IF @n <> 1 SELECT 0")
	    != TDS_SUCCESS)
		fatal_error("checking rows 2");

	if (run_query
	    (tds,
	     "DECLARE @n INT SELECT @n = COUNT(*) FROM #dynamic1 WHERE i = 654321 AND c = 'a longer string' IF @n <> 1 SELECT 0") !=
	    TDS_SUCCESS)
		fatal_error("checking rows 3");

	if (run_query(tds, "DROP TABLE #dynamic1") != TDS_SUCCESS)
		fatal_error("dropping table error");

	tds_release_dynamic(&dyn);

	try_tds_logout(login, tds, verbose);
	return 0;
}

static int
discard_result(TDSSOCKET * tds)
{
	int rc;
	int result_type;

	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {

		switch (result_type) {
		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
		case TDS_DESCRIBE_RESULT:
		case TDS_STATUS_RESULT:
		case TDS_PARAM_RESULT:
			break;
		default:
			fprintf(stderr, "Error:  query should not return results\n");
			return TDS_FAIL;
		}
	}
	if (rc == TDS_FAIL) {
		fprintf(stderr, "tds_process_tokens() returned TDS_FAIL\n");
		return TDS_FAIL;
	} else if (rc != TDS_NO_MORE_RESULTS) {
		fprintf(stderr, "tds_process_tokens() unexpected return\n");
		return TDS_FAIL;
	}

	return TDS_SUCCESS;
}
