/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

#include "ex_sql_utils.h"

/*
 * This example shows how to create, maintain, and query an R-Tree index.
 *
 * A new R*Tree index is created as follows:
 * 		CREATE VIRTUAL TABLE <name> USING rtree(<column-names>);
 *
 * The usual INSERT, UPDATE, and DELETE commands work on an R*Tree index just
 * like on regular tables.
 *
 * Any valid query will work against an R*Tree index. Especially, you can
 * efficiently do inequality queries against the coordinate ranges with an
 * R-Tree index.
 */

const double data[8][4] = {{1, 4, 2, 4}, {3, 6, 4, 7},
{5, 7, 1, 8}, {11, 14, 2, 4}, {10, 15, 1, 5},
{8, 12,3, 6}, {9, 11, 7, 9}, {9, 14, 7, 10}};


/* Create a 2-dimention rtree index. */
int create_rtree(db)
	db_handle *db;
{
	const char* sql;

	/*
	 * create an rtree which has 5 cloumns: id, minx, maxX, minY, maxY.

	 */

	sql = "DROP TABLE IF EXISTS rtree_index;\n"
		"\tCREATE VIRTUAL TABLE rtree_index USING rtree(\n"
		"\tid,\n"
		"\tminX, maxX,\n"
		"\tminY, maxY\n"
		"\t);";

	return exec_sql(db, sql);
}

/* Insert rectangles into the rtree-index. */
int populate_data(db)
	db_handle *db;
{
	int i, j, rc, errflag;
	const char* sql;
	sqlite3_stmt* stmt;

	/* Prepare a statement for rtree insert. */
	sql = "INSERT INTO rtree_index VALUES(?, ?, ?, ?, ?)";

	sqlite3_prepare_v2(db, sql, (int)strlen(sql), &stmt, NULL);
	error_handler(db);

	errflag = 0;

	/* Insert 8 rectangles with the id from 1-8 into the rtree-index. */
	for (i = 0; i < 8; i++) {

		/* Bind id with the value i+1. */
		sqlite3_bind_int(stmt, 1, i + 1);
		error_handler(db);

		/*
		 * Get the values of minX, maxX, minY, maxY from data[i] and
		 * bind them.
		 */
		for (j = 0; j < 4; j++) {
			sqlite3_bind_double(stmt, j+2, data[i][j]);
			error_handler(db);
		}

		/* Execute the query expression. */
		sqlite3_step(stmt);

		/* Reset stmt when SQLITE_DONE. The sqlite3_reset() function
		 * is called to reset a prepared statement object back to its
		 * initial state, ready to be re-executed. Any SQL statement
		 * variables that had values bound to them using the
		 * sqlite3_bind_*() API retain their values.
		 */
		rc = sqlite3_errcode(db);
		switch(rc) {
		case SQLITE_DONE:
			sqlite3_reset(stmt);
			break;
		default:
			fprintf(stderr, "ERROR: %s. ERRCODE: %d\n",
				sqlite3_errmsg(db), rc);
			errflag = 1;
			break;
		}
		if (errflag)
			break;
	}

	/* Final cleanup. */
	sqlite3_finalize(stmt);
	error_handler(db);

	return errflag;
}

/* Query the rtree-index. */
int query_rtree(db)
	db_handle *db;
{
	const char* sql;

	/* Select all records in rtree_index. */
	sql = "SELECT * FROM rtree_index";
	exec_sql(db, sql);

	/*
	 * Select the IDs of rectangles which are contained within the query
	 * box (5, 15, 1, 8).
	 */
	sql = "SELECT id FROM rtree_index\n"
		"\tWHERE minX >= 5 AND maxX <= 15\n"
		"\tAND minY >= 1 AND maxY <= 8";
	exec_sql(db, sql);

	/*
	 * Select the IDs of rectangles which overlap the query box
	 * (5, 15, 1, 8).
	 */
	sql = "SELECT id FROM rtree_index\n"
		"\tWHERE maxX >= 5 AND minX <= 12\n"
		"\tAND maxY >= 3 AND minY <= 7";
	exec_sql(db, sql);

	return 0;
}

int
main()
{
	db_handle *db;

	/* Setup environment */
	db = setup("./ex_rtree.db");

	create_rtree(db);

	populate_data(db);

	query_rtree(db);

	/* End. */
	cleanup(db);
	return 0;
}
