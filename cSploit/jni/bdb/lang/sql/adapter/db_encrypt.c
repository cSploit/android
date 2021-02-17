/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
 * This file implements SQLite encryption using the Berkeley DB encryption
 * algorithms.
 */
#include "sqliteInt.h"
#include "btreeInt.h"
#include <db.h>

#ifdef SQLITE_HAS_CODEC
void sqlite3CodecGetKey(sqlite3*, int, void**, int*);

#define	pDbEnv		(pBt->dbenv)

/*
 * sqlite3_key() is called only to set the encryption key for the "main"
 * backend database, that is db->aDb[0].  The encryption key for attached
 * databases is set directly via sqlite3CodecAttach().
 */
int sqlite3_key(sqlite3 *db, const void *key, int nkey) {
	return sqlite3CodecAttach(db, 0, key, nkey);
}

int sqlite3_rekey(sqlite3 *db, const void *key, int nkey) {
	return 0;
}

void sqlite3_activate_see(const char *zPassPhrase) {
	  return;
}

/*
 * Used by the ATTACH command and by sqlite3_key, set the encryption key for the
 * backend-th database, where "main" is 0, "temp" is 1 and each additional
 * ATTACH-ed database file is 2, 3, 4, ...
 */
int sqlite3CodecAttach(sqlite3 *db, int backend, const void *key, int nkey) {
	struct BtShared *pBt;
	int ret;

	assert(db->aDb[backend].pBt != NULL);
	pBt = db->aDb[backend].pBt->pBt;

	/*
	 * An empty key means no encryption.  Also, don't try to encrypt an
	 * environment that's already been opened.  Don't encrypt an in-mem db
	 * since it will never be written to disk.
	 */
	if (nkey == 0 || pBt->env_opened || pBt->dbStorage != DB_STORE_NAMED)
		return dberr2sqlite(0, db->aDb[backend].pBt);

	sqlite3_mutex_enter(db->mutex);

	/*
	 * SQLite and BDB have slightly different semantics for the key.
	 * SQLite's key is a string of bytes whose length is specified
	 * separately, while BDB takes a NULL terminated string.  We need to
	 * ensure the key is NULL terminated before passing to BDB, but we can't
	 * modify the given key, so we have to make a copy.  BDB will make its
	 * own copy of the key, it's safe to free keystring after
	 * the set_encrypt call.
	 */
	if (pBt->encrypt_pwd != NULL)
		CLEAR_PWD(pBt);

	if ((pBt->encrypt_pwd = malloc((size_t)nkey + 1)) == NULL) {
		ret = ENOMEM;
		goto err;
	}

	memcpy(pBt->encrypt_pwd, key, nkey);
	/*
	 * We allocate nkey + 1 bytes, but will only clear nkey bytes to
	 * preserve the terminating NULL.
	 */
	pBt->encrypt_pwd_len = nkey;
	pBt->encrypt_pwd[nkey] = '\0';

	ret = pDbEnv->set_encrypt(pDbEnv, pBt->encrypt_pwd, DB_ENCRYPT_AES);

	pBt->encrypted = 1;

err:	sqlite3_mutex_leave(db->mutex);

	return dberr2sqlite(ret, db->aDb[backend].pBt);
}

/*
 * Get the current key of the given database.
 */
void sqlite3CodecGetKey(sqlite3 *db, int backend, void **keyp, int *nkeyp) {
	*keyp = db->aDb[backend].pBt->pBt->encrypt_pwd;
	*nkeyp = db->aDb[backend].pBt->pBt->encrypt_pwd_len;
	return;
}

#endif
