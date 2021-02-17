/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
** This file contains code used to implement the VACUUM command.
*/
#include "sqliteInt.h"
#include "btreeInt.h"
#include "vdbeInt.h"

#if !defined(SQLITE_OMIT_VACUUM)
/*
** The non-standard VACUUM command is used to clean up the database,
** collapse free space, etc.  It is modelled after the VACUUM command
** in PostgreSQL.
*/
void sqlite3Vacuum(Parse *pParse) {
	Vdbe *v = sqlite3GetVdbe(pParse);

	if (v)
		sqlite3VdbeAddOp2(v, OP_Vacuum, 0, 0);
}

int btreeVacuum(Btree *p, char **pzErrMsg) {
	sqlite3 *db;
	int rc;
	u_int32_t truncatedPages;

	db = p->db;

	/* Return directly if vacuum is on progress */
	if (p->inVacuum)
		return SQLITE_OK;

	/*
	 * We're going to do updates in this transaction at the Berkeley DB
	 * Core level (i.e., call DB->compact), but we start it read-only at
	 * the SQL level to avoid overhead from checkpoint-on-commit.
	 */
	if ((rc = btreeBeginTransInternal(p, 0)) != SQLITE_OK) {
		sqlite3SetString(pzErrMsg, db,
		    "failed to begin a vacuum transaction");
		return rc;
	}

	p->inVacuum = 1;

	truncatedPages = 0;
	/* Go through all tables */
	do {
		rc = btreeIncrVacuum(p, &truncatedPages);
	} while (rc == SQLITE_OK);
	p->needVacuum = 0;

	if (rc != SQLITE_DONE) {
		sqlite3SetString(pzErrMsg, db,
		    "error during vacuum, rolled back");
		(void)sqlite3BtreeRollback(p, SQLITE_OK);
	} else if ((rc = sqlite3BtreeCommit(p)) != SQLITE_OK) {
		sqlite3SetString(pzErrMsg, db,
		    "failed to commit the vacuum transaction");
	}

	p->inVacuum = 0;

	return rc;
}

/*
** Free internal link list of vacuum info for Btree object
**/
void btreeFreeVacuumInfo(Btree *p)
{
	struct VacuumInfo *pInfo, *pInfoNext;

	/* Free DBT for vacuum start */
	for (pInfo = p->vacuumInfo; pInfo != NULL; pInfo = pInfoNext) {
		pInfoNext = pInfo->next;
		if (pInfo->start.data)
			sqlite3_free(pInfo->start.data);
		sqlite3_free(pInfo);
	}
	p->vacuumInfo = NULL;
	p->needVacuum = 0;
	return;
}

/*
** A write transaction must be opened before calling this function.
** It performs a single unit of work towards an incremental vacuum.
** Specifically, in the Berkeley DB storage manager, it attempts to compact
** one table.
**
** If the incremental vacuum is finished after this function has run,
** SQLITE_DONE is returned. If it is not finished, but no error occurred,
** SQLITE_OK is returned. Otherwise an SQLite error code.
**
** The caller can get and accumulate the number of truncated pages truncated
** with input parameter truncatedPages. Also, btreeIncrVacuum would skip
** the vacuum if enough pages has been truncated for optimization.
*/
int btreeIncrVacuum(Btree *p, u_int32_t *truncatedPages)
{
	BtShared *pBt;
	CACHED_DB *cached_db;
	DB *dbp;
	DBT key, data;
	char *fileName, *tableName, tableNameBuf[DBNAME_SIZE];
	void *app;
	int iTable, rc, ret, t_ret;
	u_int32_t was_create;
	DB_COMPACT compact_data;
	DBT *pStart, end;	/* start/end of db_compact() */
	struct VacuumInfo *pInfo;
	int vacuumMode;

	assert(p->pBt->dbStorage == DB_STORE_NAMED);

	if (!p->connected && (rc = btreeOpenEnvironment(p, 1)) != SQLITE_OK)
		return rc;

	pBt = p->pBt;
	rc = SQLITE_OK;
	cached_db = NULL;
	dbp = NULL;
	memset(&end, 0, sizeof(end));
#ifndef BDBSQL_OMIT_LEAKCHECK
	/* Let BDB use the user-specified malloc function (btreeMalloc) */
	end.flags |= DB_DBT_MALLOC;
#endif

	/*
	 * Turn off DB_CREATE: we don't want to create any tables that don't
	 * already exist.
	 */
	was_create = (pBt->db_oflags & DB_CREATE);
	pBt->db_oflags &= ~DB_CREATE;

	memset(&key, 0, sizeof(key));
	key.data = tableNameBuf;
	key.ulen = sizeof(tableNameBuf);
	key.flags = DB_DBT_USERMEM;
	memset(&data, 0, sizeof(data));
	data.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;

	UPDATE_DURING_BACKUP(p);

	if (p->compact_cursor == NULL) {
		if ((ret = pTablesDb->cursor(pTablesDb, pReadTxn,
		    &p->compact_cursor, 0)) != 0)
			goto err;
	}
	if ((ret = p->compact_cursor->get(p->compact_cursor,
	    &key, &data, DB_NEXT)) == DB_NOTFOUND) {
		(void)p->compact_cursor->close(p->compact_cursor);
		p->compact_cursor = NULL;
		pBt->db_oflags |= was_create;
		return SQLITE_DONE;
	} else if (ret != 0)
		goto err;

	tableNameBuf[key.size] = '\0';
	if (strncmp(tableNameBuf, "table", 5) != 0) {
		iTable = 0;
#ifdef BDBSQL_FILE_PER_TABLE
		/* Cannot compact the metadata file */
		goto err;
#endif

		/* Open a DB handle on that table. */
		if ((ret = db_create(&dbp, pDbEnv, 0)) != 0)
			goto err;
		if (pBt->encrypted &&
		    (ret = dbp->set_flags(dbp, DB_ENCRYPT)) != 0)
			goto err;

		tableName = tableNameBuf;
		FIX_TABLENAME(pBt, fileName, tableName);

		/*
		 * We know we're not creating this table, open it using the
		 * family transaction because that keeps the dbreg records out
		 * of the vacuum transaction, reducing pressure on the log
		 * region (since we copy the filename of every open DB handle
		 * into the log region).
		 */
		if ((ret = dbp->open(dbp, pFamilyTxn, fileName, tableName,
		    DB_BTREE, GET_AUTO_COMMIT(pBt, pFamilyTxn), 0)) != 0)
			goto err;
	} else {
		if ((ret = btreeTableNameToId(tableNameBuf,
		    key.size, &iTable)) != 0)
			goto err;

		/* Try to retrieve the matching handle from the cache. */
		rc = btreeFindOrCreateDataTable(p, &iTable, &cached_db, 0);
		if (rc != SQLITE_OK)
			goto err;
		assert(cached_db != NULL && cached_db->dbp != NULL);

		dbp = cached_db->dbp;
		if ((iTable & 1) == 0) {
			/*
			 * Attach the DB handle to a SQLite index, required for
			 * the key comparator to work correctly.  If we can't
			 * find an Index struct, just skip this database.  It
			 * may not be open yet (c.f. whereA-1.7).
			 */
#ifdef BDBSQL_SINGLE_THREAD
			rc = btreeGetKeyInfo(p, iTable,
			    (KeyInfo **)&(dbp->app_private));
#else
			rc = btreeGetKeyInfo(p, iTable,
			    &((TableInfo *)dbp->app_private)->pKeyInfo);
#endif
			if (rc != SQLITE_OK)
				goto err;
		}
	}

	/*
	 * In following db_compact, we use the family transaction because
	 * DB->compact will then auto-commit, and it has built-in smarts
	 * about retrying on deadlock.
	 */
	/* Setup compact_data as configured */
	memset(&compact_data, 0, sizeof(compact_data));
	compact_data.compact_fillpercent = p->fillPercent;

	vacuumMode = sqlite3BtreeGetAutoVacuum(p);
	if (vacuumMode == BTREE_AUTOVACUUM_NONE) {
		ret = dbp->compact(dbp, pFamilyTxn,
		    NULL, NULL, &compact_data, DB_FREE_SPACE, NULL);
	/* Skip current table if we have truncated enough pages */
	} else if (truncatedPages == NULL ||
	    (truncatedPages != NULL && *truncatedPages < p->vacuumPages)) {
		/* Find DBT for db_compact start */
		for (pInfo = p->vacuumInfo, pStart = NULL;
		     pInfo != NULL; pInfo = pInfo->next) {
			if (pInfo->iTable == iTable)
				break;
		}

		/* Create new VacuumInfo for current iTable as needed */
		if (pInfo == NULL) {
			/* Create info for current iTable */
			if ((pInfo = (struct VacuumInfo *)sqlite3_malloc(
			    sizeof(struct VacuumInfo))) == NULL) {
				rc = SQLITE_NOMEM;
				goto err;
			}
			memset(pInfo, 0, sizeof(struct VacuumInfo));
			pInfo->iTable = iTable;
			pInfo->next = p->vacuumInfo;
			p->vacuumInfo = pInfo;
		}
		pStart = &(pInfo->start);

		/* Do page compact for IncrVacuum */
		if (vacuumMode == BTREE_AUTOVACUUM_INCR) {
			/* Do compact with given arguments */
			compact_data.compact_pages = p->vacuumPages;
			if ((ret = dbp->compact(dbp, pFamilyTxn,
				(pStart->data == NULL) ? NULL : pStart,
				NULL, &compact_data, 0, &end)) != 0)
				goto err;

			/* Save current vacuum position */
			if (pStart->data != NULL)
				sqlite3_free(pStart->data);
			memcpy(pStart, &end, sizeof(DBT));
			memset(&end, 0, sizeof(end));

			/* Rewind to start if we reach the end of subdb */
			if (compact_data.compact_pages_free < p->vacuumPages ||
			    p->vacuumPages == 0) {
				if (pStart->data != NULL)
					sqlite3_free(pStart->data);
				memset(pStart, 0, sizeof(DBT));
			}
		}
		/* Because of the one-pass nature of the compaction algorithm,
		 * any unemptied page near the end of the file inhibits
		 * returning pages to the file system.
		 * A repeated call to the DB->compact() method with a low
		 * compact_fillpercent may be used to return pages in this case.
		 */
		memset(&compact_data, 0, sizeof(compact_data));
		compact_data.compact_fillpercent = 1;
		if ((ret = dbp->compact(dbp, pFamilyTxn, NULL, NULL,
			    &compact_data, DB_FREE_SPACE, NULL)) != 0)
			goto err;
		if (truncatedPages != NULL && *truncatedPages > 0)
			*truncatedPages += compact_data.compact_pages_truncated;
	}

err:	/* Free cursor and DBT if run into error */
	if (ret != 0) {
		if (p->compact_cursor != NULL) {
			(void)p->compact_cursor->close(p->compact_cursor);
			p->compact_cursor = NULL;
		}
		if (end.data != NULL)
			sqlite3_free(end.data);
		btreeFreeVacuumInfo(p);
	}

	if (cached_db != NULL) {
#ifdef BDBSQL_SINGLE_THREAD
		if ((app = dbp->app_private) != NULL)
			sqlite3DbFree(p->db, app);
#else
		if (dbp->app_private != NULL &&
		    (app = ((TableInfo *)dbp->app_private)->pKeyInfo) != NULL) {
			sqlite3DbFree(p->db, app);
			((TableInfo *)dbp->app_private)->pKeyInfo = NULL;
		}
#endif
	} else if (dbp != NULL) {
		app = dbp->app_private;
		if ((t_ret = dbp->close(dbp, DB_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
		if (app != NULL)
			sqlite3DbFree(p->db, app);
	}

	pBt->db_oflags |= was_create;

	return MAP_ERR(rc, ret, p);
}

/*
** This routine implements the OP_Vacuum opcode of the VDBE.
*/
int sqlite3RunVacuum(char **pzErrMsg, sqlite3 *db) {
	int rc;
	Btree *p;

	p = db->aDb[0].pBt;
	rc = SQLITE_OK;

	if (p->pBt->dbStorage != DB_STORE_NAMED)
		return SQLITE_OK;

	if ((rc = sqlite3Init(db, pzErrMsg)) != SQLITE_OK)
		return rc;

	if (!db->autoCommit) {
		sqlite3SetString(pzErrMsg, db,
		    "cannot VACUUM from within a transaction");
		return SQLITE_ERROR;
	}

	assert(sqlite3_mutex_held(db->mutex));
	rc = btreeVacuum(p, pzErrMsg);

	return rc;
}
#endif  /* SQLITE_OMIT_VACUUM */
