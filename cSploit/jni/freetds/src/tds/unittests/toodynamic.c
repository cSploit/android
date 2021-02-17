/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2008 Frediano Ziglio
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

/*
 * Test creating a lot of dynamics. This can cause some problems cause
 * generated IDs are reused on a base of 2^16
 */

static char software_version[] = "$Id: toodynamic.c,v 1.2 2011-05-16 13:31:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static void
fatal_error(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

int
main(int argc, char **argv)
{
	TDSLOGIN *login;
	TDSSOCKET *tds;
	int verbose = 0;
	TDSDYNAMIC *dyn = NULL;
	int rc;
	unsigned int n;

	fprintf(stdout, "%s: Test creating a lot of dynamic queries\n", __FILE__);
	rc = try_tds_login(&login, &tds, __FILE__, verbose);
	if (rc != TDS_SUCCESS)
		fatal_error("try_tds_login() failed");

	run_query(tds, "DROP TABLE #test");
	if (run_query(tds, "CREATE TABLE #test (i INT, c VARCHAR(40))") != TDS_SUCCESS)
		fatal_error("creating table error");

	if (tds->cur_dyn)
		fatal_error("already a dynamic query??");

	/* prepare to insert */
	if (tds_submit_prepare(tds, "UPDATE #test SET c = 'test' WHERE i = ?", NULL, &dyn, NULL) != TDS_SUCCESS)
		fatal_error("tds_submit_prepare() error");
	if (tds_process_simple_query(tds) != TDS_SUCCESS)
		fatal_error("tds_process_simple_query() error");
	if (!dyn)
		fatal_error("dynamic not present??");

	/* waste some ids */
	for (n = 0; n < 65525; ++n) {
		TDSDYNAMIC *dyn;

		dyn = tds_alloc_dynamic(tds->conn, NULL);
		if (!dyn)
			fatal_error("create dynamic");

		tds_dynamic_deallocated(tds->conn, dyn);
		tds_release_dynamic(&dyn);
	}

	/* this should not cause duplicate IDs or erros*/
	for (n = 0; n < 20; ++n) {
		TDSDYNAMIC *dyn2 = NULL;

		if (tds_submit_prepare(tds, "INSERT INTO #test(i,c) VALUES(?,?)", NULL, &dyn2, NULL) != TDS_SUCCESS)
			fatal_error("tds_submit_prepare() error");
		if (dyn == dyn2)
			fatal_error("got duplicated dynamic");
		if (tds_process_simple_query(tds) != TDS_SUCCESS)
			fatal_error("tds_process_simple_query() error");
		if (!dyn2)
			fatal_error("dynamic not present??");
		if (tds_submit_unprepare(tds, dyn2) != TDS_SUCCESS || tds_process_simple_query(tds) != TDS_SUCCESS)
			fatal_error("unprepare error");
		tds_dynamic_deallocated(tds->conn, dyn2);
		tds_release_dynamic(&dyn2);
	}

	tds_dynamic_deallocated(tds->conn, dyn);
	tds_release_dynamic(&dyn);

	try_tds_logout(login, tds, verbose);
	return 0;
}

