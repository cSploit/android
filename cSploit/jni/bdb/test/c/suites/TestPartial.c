/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
/*
 * Test combination of DB_DBT_PARTIAL and flags in DB->get, DB->pget,
 * DBcursor->get, DBcursor->pget.
 *
 * TestPartial.c
 */
#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

const char *dbName = "TestPartial.db";
const char *sdbName = "TestPartialSec.db";

int expected = EINVAL;

/* Test utilities declaration */
static void CheckDbPartial(CuTest *, DB *, int, u_int32_t, int);
static void CheckCursorPartial(CuTest *, DB *, int, u_int32_t, int);
static int CloseDb(DB *);
static int Getkeystring(DB *, const DBT *, const DBT *, DBT *);
static void OpenDb(CuTest *, DB *, DB **, const char *, DBTYPE, u_int32_t);
static void PopulateDb(CuTest *, DB *, u_int32_t, u_int32_t);

int TestPartialSuiteSetup(CuSuite *ct) {
	return (0);
}

int TestPartialSuiteTeardown(CuSuite *ct) {
	return (0);
}

int TestPartialTestSetup(CuTest *ct) {
	remove(dbName);
	remove(sdbName);
	return (0);
}

int TestPartialTestTeardown(CuTest *ct) {
	remove(dbName);
	remove(sdbName);
	return (0);
}

/* Test cases */
int TestDbPartialGet(CuTest *ct) {
/* Run this test only when queue is supported. */
#ifdef HAVE_QUEUE
	DB *pdb;

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	PopulateDb(ct, pdb, 100, 1);
	CheckDbPartial(ct, pdb, 0, 0, expected);
	CheckDbPartial(ct, pdb, 0, DB_GET_BOTH, expected);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, DB_RECNUM);
	PopulateDb(ct, pdb, 100, 1);
	CheckDbPartial(ct, pdb, 0, DB_SET_RECNO, 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_QUEUE, 0);
	PopulateDb(ct, pdb, 100, 1);
	CheckDbPartial(ct, pdb, 0, DB_CONSUME, 0);
	CheckDbPartial(ct, pdb, 0, DB_CONSUME_WAIT, 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);
#else
	printf("TestDbPartialGet is not supported by the build.\n");
#endif /* HAVE_QUEUE */

	return (0);
}

int TestDbPartialPGet(CuTest *ct) {
	DB *pdb, *sdb;

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	OpenDb(ct, pdb, &sdb, sdbName, DB_BTREE, 0);
	PopulateDb(ct, pdb, 100, 1);
	CheckDbPartial(ct, sdb, 1, 0, expected);
	CheckDbPartial(ct, sdb, 1, DB_GET_BOTH, expected);
	CuAssert(ct, "DB->close", CloseDb(sdb) == 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);
	CuAssert(ct, "remove()", remove(sdbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	OpenDb(ct, pdb, &sdb, sdbName, DB_BTREE, DB_RECNUM);
	PopulateDb(ct, pdb, 100, 1);
	CheckDbPartial(ct, sdb, 1, DB_SET_RECNO, 0);
	CuAssert(ct, "DB->close", CloseDb(sdb) == 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);
	CuAssert(ct, "remove()", remove(sdbName) == 0);

	/* DB_CONSUME makes no sense on a secondary index, so no test. */

	return (0);
}

int TestCursorPartialGet(CuTest *ct) {
	DB *pdb;

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	PopulateDb(ct, pdb, 100, 1);
	CheckCursorPartial(ct, pdb, 0, DB_GET_BOTH, expected);
	CheckCursorPartial(ct, pdb, 0, DB_GET_BOTH_RANGE, expected);
	CheckCursorPartial(ct, pdb, 0, DB_SET, expected);
	CheckCursorPartial(ct, pdb, 0, DB_SET_RANGE, 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, DB_RECNUM);
	PopulateDb(ct, pdb, 100, 1);
	CheckCursorPartial(ct, pdb, 0, DB_GET_RECNO, 0);
	CheckCursorPartial(ct, pdb, 0, DB_SET_RECNO, 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, DB_DUP);
	PopulateDb(ct, pdb, 100, 3);
	CheckCursorPartial(ct, pdb, 0, DB_CURRENT, 0);
	CheckCursorPartial(ct, pdb, 0, DB_FIRST, 0);
	CheckCursorPartial(ct, pdb, 0, DB_LAST, 0);
	CheckCursorPartial(ct, pdb, 0, DB_NEXT, 0);
	CheckCursorPartial(ct, pdb, 0, DB_NEXT_DUP, 0);
	CheckCursorPartial(ct, pdb, 0, DB_NEXT_NODUP, 0);
	CheckCursorPartial(ct, pdb, 0, DB_PREV, 0);
	CheckCursorPartial(ct, pdb, 0, DB_PREV_DUP, 0);
	CheckCursorPartial(ct, pdb, 0, DB_PREV_NODUP, 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);

	return (0);
}

int TestCursorPartialPGet(CuTest *ct) {
	DB *pdb, *sdb;

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	OpenDb(ct, pdb, &sdb, sdbName, DB_BTREE, 0);
	PopulateDb(ct, pdb, 100, 1);
	CheckCursorPartial(ct, sdb, 1, DB_GET_BOTH, expected);
	CheckCursorPartial(ct, sdb, 1, DB_GET_BOTH_RANGE, expected);
	CheckCursorPartial(ct, sdb, 1, DB_SET, expected);
	CheckCursorPartial(ct, sdb, 1, DB_SET_RANGE, 0);
	CuAssert(ct, "DB->close", CloseDb(sdb) == 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);
	CuAssert(ct, "remove()", remove(sdbName) == 0);

	OpenDb(ct, NULL, &pdb, dbName, DB_BTREE, 0);
	OpenDb(ct, pdb, &sdb, sdbName, DB_BTREE, DB_RECNUM);
	PopulateDb(ct, pdb, 100, 1);
	CheckCursorPartial(ct, sdb, 1, DB_SET_RECNO, 0);
	CuAssert(ct, "DB->close", CloseDb(sdb) == 0);
	CuAssert(ct, "DB->close", CloseDb(pdb) == 0);
	CuAssert(ct, "remove()", remove(dbName) == 0);
	CuAssert(ct, "remove()", remove(sdbName) == 0);

	return (0);
}

/* Test utilities */
static void CheckDbPartial(CuTest *ct,
    DB *dbp, int isSec, u_int32_t flags, int value) {
	DBT key, data, pkey;
	char kstr[10], dstr[14];
	u_int32_t indx = 5, dupindx = 0;
	db_recno_t rid = 5;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	memset(&pkey, 0, sizeof(DBT));
	memset(kstr, 0, sizeof(char) * 10);
	memset(dstr, 0, sizeof(char) * 14);

	key.doff = 2;
	key.dlen = 1;
	key.flags = DB_DBT_PARTIAL;
	if (flags == DB_CONSUME || flags == DB_CONSUME_WAIT ||
	    flags == DB_SET_RECNO) {
		if (flags == DB_SET_RECNO) {
			key.data = &rid;
			key.size = sizeof(db_recno_t);
		}
	} else if (flags == DB_GET_BOTH || flags == 0) {
		memcpy(kstr, &indx, sizeof(u_int32_t));
		memcpy(kstr + 4, "hello", sizeof(char) * 5);
		key.data = kstr;
		key.size = 10;
		if (flags == DB_GET_BOTH) {
			if (isSec == 0) {
				memcpy(dstr, kstr, sizeof(char) * 9);
				memcpy(dstr + 9, &dupindx, sizeof(u_int32_t));
				data.data = dstr;
				data.size = 14;
			} else {
				pkey.data = kstr;
				pkey.size = 10;
			}
		}
	} else {
		fprintf(stderr, "Invalid test flags: %d\n", flags);
		return;
	}

	if (isSec == 0) {
		CuAssert(ct, "DB->get",
		    dbp->get(dbp, NULL, &key, &data, flags) == value);
		if (value == 0)
			CuAssert(ct, "Partial DBT size", key.size == key.dlen);
	} else {
		CuAssert(ct, "DB->pget",
		    dbp->pget(dbp, NULL, &key, &pkey, &data, flags) == value);
		if (value == 0)
			CuAssert(ct, "Partial DBT size", key.size == key.dlen);

		/* Partial pkey is always invalid. */
		pkey.doff = 1;
		pkey.dlen = 2;
		pkey.flags = DB_DBT_PARTIAL;
		CuAssert(ct, "DB->pget", dbp->pget(dbp, NULL, &key, &pkey,
		    &data, flags) == expected);
	}
}

static void CheckCursorPartial(CuTest *ct,
    DB *dbp, int isSec, u_int32_t flags, int value) {
	DBT key, data, pkey;
	DBC *cursor = NULL;
	char kstr[10], dstr[14];
	u_int32_t indx = 5, dupindx = 0;
	db_recno_t rid = 5;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	memset(&pkey, 0, sizeof(DBT));
	memset(kstr, 0, sizeof(char) * 10);
	memcpy(kstr, &indx, sizeof(u_int32_t));
	memcpy(kstr + 4, "hello", sizeof(char) * 5);
	memset(dstr, 0, sizeof(char) * 14);
	memcpy(dstr, kstr, sizeof(char) * 9);
	memcpy(dstr + 9, &dupindx, sizeof(u_int32_t));

	CuAssert(ct, "DB->cursor", dbp->cursor(dbp, NULL, &cursor, 0) == 0);

	key.doff = 2;
	key.dlen = 1;
	key.flags = DB_DBT_PARTIAL;
	if (flags == DB_SET_RECNO) {
		key.data = &rid;
		key.size = sizeof(db_recno_t);
	} else if (flags == DB_GET_BOTH || flags == DB_GET_BOTH_RANGE ||
	   flags == DB_SET || flags == DB_SET_RANGE) {
		key.data = kstr;
		key.size = 10;
		if (flags == DB_GET_BOTH || flags ==  DB_GET_BOTH_RANGE) {
			if (isSec == 0) {
				data.data = dstr;
				data.size = 14;
			} else {
				pkey.data = kstr;
				pkey.size = 10;
			}
		}
	} else if (flags == DB_CURRENT || flags == DB_GET_RECNO || 
	    flags == DB_NEXT || flags == DB_NEXT_DUP || 
	    flags == DB_NEXT_NODUP) {
		CuAssert(ct, "DBC->get", 
		    cursor->get(cursor, &key, &data, DB_FIRST) == 0);
	} else if (flags == DB_PREV || flags == DB_PREV_DUP ||
	    flags == DB_PREV_NODUP) {
		CuAssert(ct, "DBC->get", 
		    cursor->get(cursor, &key, &data, DB_LAST) == 0);
	} else {
		if (flags != DB_FIRST && flags != DB_LAST) {
			fprintf(stderr, "Invalid test flags: %d\n", flags);
			return;
		}
	}

	if (isSec == 0) {
		CuAssert(ct, "DBC->get", 
		    cursor->get(cursor, &key, &data, flags) == value);
		if (value == 0)
			CuAssert(ct, "Partial DBT size", key.size == key.dlen);
	} else {
		CuAssert(ct, "DBC->pget",
		    cursor->pget(cursor, &key, &pkey, &data, flags) == value);
		if (value == 0)
			CuAssert(ct, "Partial DBT size", key.size == key.dlen);

		/* Partial pkey is always invalid. */
		pkey.doff = 1;
		pkey.dlen = 2;
		pkey.flags = DB_DBT_PARTIAL;
		CuAssert(ct, "DBC->pget", cursor->pget(cursor, &key, &pkey,
		    &data, flags) == expected);
	}

	cursor->close(cursor);
}

static int CloseDb(DB *dbp) {
	return dbp->close(dbp, 0);
}

static void OpenDb(CuTest *ct, DB *ldbp, DB **dbpp, const char *dbName,
    DBTYPE type, u_int32_t openFlags) {
	DB *dbp = NULL;

	CuAssert(ct, "db_create", db_create(&dbp, NULL, 0) == 0);
	dbp->set_errcall(dbp, NULL);
	CuAssert(ct, "DB->set_flags", dbp->set_flags(dbp, openFlags) == 0);
	if (type == DB_QUEUE)
		CuAssert(ct, "DB->set_re_len",
		    dbp->set_re_len(dbp, ldbp == NULL ? 14 : 10) == 0);
	CuAssert(ct, "DB->open", dbp->open(dbp, NULL, dbName, NULL, type,
	    DB_CREATE, 0) == 0);
	if (ldbp != NULL)
		CuAssert(ct, "DB->associate", ldbp->associate(ldbp, NULL, dbp,
		    Getkeystring, 0) == 0);
	*dbpp = dbp;
}

static int Getkeystring(
    DB *secondary, const DBT *pkey, const DBT *pdata, DBT *skey)
{
 	memset(skey, 0, sizeof(DBT));
	skey->data = pdata->data;
	skey->size = pkey->size;
	return (0);
}

static void PopulateDb(CuTest *ct, DB *db, u_int32_t nkeys, u_int32_t ndups) {
	DBT key, data;
	char kstr[10], dstr[14];
	DBTYPE type;
	u_int32_t i, j, start;

	start = 0;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	memset(kstr, 0, sizeof(char) * 10);
	memcpy(kstr + 4, "hello", 5 * sizeof(char));
	memset(dstr, 0, sizeof(char) * 14);

	CuAssert(ct, "DB->get_type", db->get_type(db, &type) == 0);

	for (i = 1; i <= nkeys; i++) {
		key.data = kstr;
		if (type == DB_BTREE || type == DB_HASH) {
			memcpy(kstr, &i, sizeof(u_int32_t));
			key.size = sizeof(char) * 10;
		} else {
			memcpy(kstr, &i, sizeof(u_int32_t));
			key.size = sizeof(db_recno_t);
		}
		for (j = 0; j < ndups; j++) {
			memcpy(dstr, kstr, sizeof(char) * 9);
			memcpy(dstr + 9, &j, sizeof(u_int32_t));
			data.data = dstr;
			data.size = sizeof(char) * 14;

			CuAssert(ct, "DB->put",
			    db->put(db, NULL, &key, &data, 0) == 0);
		}
	}
}
