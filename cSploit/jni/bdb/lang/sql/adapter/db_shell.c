/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
** This file contains code used to implement the .stat command. Any other shell
** features should also be added here.
*/
#include <errno.h>
#include <assert.h>

#include "sqlite3.h"
#include "btreeInt.h"

/*
 * Print statistics for tables and/or indexes using DB->stat_print()
 *
 * If msgfile is NULL, then statistics will be printed into stdout, otherwise
 * the statistics will be printed into the designated output stream.
 *
 * If name input is NULL, then statistics for all tables and indexes are
 * printed, otherwise only the statistics for that table or index is printed.
 *
 * Returns SQLITE_OK if there are no errors, -1 if an error occurred.
 */
SQLITE_API int bdbSqlDbStatPrint(sqlite3 *db, FILE *msgfile, char *name)
{
	BtCursor cur, *pCur = NULL;
	Btree *p;
	char **azResult = NULL;
	char *zErrMsg = NULL;
	char *zSql = NULL;
	DB *dbp;
	FILE *out;
	int i, rc, nRow, iTable, openTransaction = 0;

	if (!db || !db->aDb)
		return -1;

	p = db->aDb[0].pBt;
	assert(p);

	if (!(out = msgfile))
		out = stdout;

	/* Construct query to get root page number(s) */
	if (!name) {
		zSql = sqlite3_mprintf(
		    "SELECT type,name,rootpage FROM sqlite_master");
	}
	else {
		zSql = sqlite3_mprintf(
		    "SELECT type,name,rootpage FROM sqlite_master "
		    "WHERE name='%q'", name);
	}

	if (!zSql) {
		fprintf(stderr, "Error: memory allocation failed\n");
		goto err;
	}

	rc = sqlite3_get_table(db, zSql, &azResult, &nRow, 0, &zErrMsg);
	(void)sqlite3_free(zSql);

	if (zErrMsg) {
		fprintf(stderr, "Error: %s\n", zErrMsg);
		(void)sqlite3_free(zErrMsg);
		if (rc == SQLITE_OK)
			rc = -1;
		goto err;
	}
	else if (rc != SQLITE_OK) {
		fprintf(stderr, "Error: querying sqlite_master\n");
		goto err;
	}
	else if (nRow < 1)
		goto err;

	rc = sqlite3BtreeBeginTrans(p, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error: could not enter a transaction\n");
		goto err;
	}
	openTransaction = 1;

	/* Print stats for all tables in query's result */
	for (i = 1; i <= nRow; i++) {
		fprintf(out, "Statistics for %s \"%s\"\n",
		    azResult[3*i], azResult[3*i + 1]);
		iTable = atoi(azResult[3*i + 2]);

		pCur = &cur;
		(void)sqlite3BtreeCursorZero(pCur);

		/* Acquire a read cursor to retrieve the dbp for the table */
		rc = sqlite3BtreeCursor(p, iTable, 0, NULL, pCur);
		if (pCur->eState == CURSOR_FAULT)
			rc = pCur->error;
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Error: could not create cursor\n");
			goto err;
		}

		assert(pCur->cached_db && pCur->cached_db->dbp);
		dbp = pCur->cached_db->dbp;
		(void)dbp->set_msgfile(dbp, out);
		(void)dbp->stat_print(dbp, DB_STAT_ALL);

		(void)sqlite3BtreeCloseCursor(&cur);
		pCur = NULL;
	}

err:
	if (pCur)
		(void)sqlite3BtreeCloseCursor(pCur);
	if (openTransaction)
		sqlite3BtreeCommit(p);
	if (azResult)
		(void)sqlite3_free_table(azResult);

	return rc;
}

/*
 * Print statistics for the database using DB_ENV->stat_print()
 *
 * If msgfile is NULL, then statistics will be printed into stdout, otherwise
 * the statistics will be printed into the designated output stream.
 *
 * Returns SQLITE_OK if there are no errors, -1 if an error occurred.
 */
SQLITE_API int bdbSqlEnvStatPrint(sqlite3 *db, FILE *msgfile)
{
	BtShared *pBt;
	Btree *p;
	FILE *out;

	if (!db || !db->aDb)
		return -1;

	p = db->aDb[0].pBt;
	assert(p);

	pBt = p->pBt;
	assert(pBt);

	if (!p->connected || !(pBt->dbenv))
		return -1;

	if (!(out = msgfile))
		out = stdout;

	fprintf(out, "Statistics for environment\n");
	(void)(pBt->dbenv)->set_msgfile(pBt->dbenv, out);
	(void)(pBt->dbenv)->stat_print(pBt->dbenv, DB_STAT_ALL);

	return SQLITE_OK;
}

/*
 * Print replication summary statistics using DB_ENV->rep_stat_print()
 *
 * If msgfile is NULL, then statistics will be printed into stdout, otherwise
 * the statistics will be printed into the designated output stream.
 *
 * Returns SQLITE_OK if there are no errors, -1 if an error occurred.
 */
SQLITE_API int bdbSqlRepSumStatPrint(sqlite3 *db, FILE *msgfile)
{
	BtShared *pBt;
	Btree *p;
	FILE *out;

	if (!db || !db->aDb)
		return -1;

	p = db->aDb[0].pBt;
	assert(p);

	pBt = p->pBt;
	assert(pBt);

	if (!p->connected || !(pBt->dbenv))
		return -1;

	if (!(out = msgfile))
		out = stdout;

	fprintf(out, "Replication summary statistics\n");
	(void)(pBt->dbenv)->set_msgfile(pBt->dbenv, out);
	(void)(pBt->dbenv)->rep_stat_print(pBt->dbenv, DB_STAT_SUMMARY);

	return SQLITE_OK;
}
