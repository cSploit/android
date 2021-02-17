/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 *
 * A c unit test program for encrypting a database. [#19539].
 *
 * There are two ways to encrypt a database: 
 *    i) If the database is attached to an opened environment, use
 *        DB_ENV->set_encrypt() and DB->set_flags();
 *    ii) Otherwise, use DB->set_encrypt().
 * 
 * Note:
 * 1) DB->set_encrypt() cannot used in an opened environment,
 *    no matter whether this environment is encrypted or not.
 * 2) Single DB_ENV->set_encrypt() is not enough to encrypt a database
 *    inside an encrypted environment, DB->set_flag should also be applied to
 *    this specific database.
 * 3) DB->set_flag() should only be called in an encrypted environment.
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "db.h"
#include "CuTest.h"
#include "test_util.h"

const char *progname_crypt = "encryption";
#define	DATABASE	"encryption.db"
#define	PASSWORD	"ENCRYPT_KEY"

typedef struct crypt_config {
	int in_env;
	int is_env_encrypt;
	int is_db_encrypt;
	int is_db_flags_encrypt;
} CRYPT_CONFIG;

int	closeDb __P((DB_ENV *, DB *));
int	dbPutGet __P((CuTest *, DB *));
int	encryptTestCase __P((CuTest *, int, int, int, int));
int	initConfig __P((CRYPT_CONFIG *, int, int, int, int));
int	openDb __P((CuTest *, DB **dbp, DB_ENV *, char *, CRYPT_CONFIG *));
int	openEnv __P((CuTest *, DB_ENV **, char *, int));
int	reOpen __P((CuTest *, DB **, char *, CRYPT_CONFIG *));

int TestNoEncryptedDb(CuTest *ct) {
	CuAssert(ct, "TestNoEncryption", encryptTestCase(ct, 0, 0, 0, 0) == 0);
	return (0);
}

int TestEncryptedDbFlag(CuTest *ct) {
	CuAssert(ct, "TestEncryptedDbFlag",
	    encryptTestCase(ct, 0, 0, 0, 1) == 0);
	return (0);
}

int TestEncryptedDb(CuTest *ct) {
/* Run this test only when cryptography is supported. */
#ifdef HAVE_CRYPTO
	CuAssert(ct, "TestEncryptedDb", encryptTestCase(ct, 0, 0, 1, 0) == 0);
#else
	printf("TestEncryptedDb is not supported by the build.\n");
#endif /* HAVE_CRYPTO */
	return (0);
}

int TestEncryptedDbFlagAndDb(CuTest *ct) {
	CuAssert(ct, "TestEncryptedDbFlagAndDb",
	    encryptTestCase(ct, 0, 0, 1, 1) == 0);
	return (0);
}

int TestEnvWithNoEncryption(CuTest *ct) {
	CuAssert(ct, "TestEnvWithNoEncryption",
	    encryptTestCase(ct, 1, 0, 0, 0) == 0);
	return (0);
}

int TestEnvWithEncryptedDbFlag(CuTest *ct) {
	CuAssert(ct, "TestEnvWithEncryptedDbFlag",
	    encryptTestCase(ct, 1, 0, 0, 1) == 0);
	return (0);
}

int TestEnvWithEncryptedDb(CuTest *ct) {
	CuAssert(ct, "TestEnvWithEncryptedDb",
	    encryptTestCase(ct, 1, 0, 1, 0) == 0);
	return (0);
}

int TestEnvWithEncryptedDbFlagAndDb(CuTest *ct) {
	CuAssert(ct, "TestEnvWithEncryptedDbFlagAndDb",
	    encryptTestCase(ct, 1, 0, 1, 1) == 0);
	return (0);
}

int TestEncyptedEnv(CuTest *ct) {
/* Run this test only when cryptography is supported. */
#ifdef HAVE_CRYPTO
	CuAssert(ct, "TestEncyptedEnv", encryptTestCase(ct, 1, 1, 0, 0) == 0);
#else
	printf("TestEyptedEnv is not supported by the build.\n");
#endif /* HAVE_CRYPTO */
	return (0);
}

int TestEncyptedEnvWithEncyptedDbFlag(CuTest *ct) {
/* Run this test only when cryptography is supported. */
#ifdef HAVE_CRYPTO
	CuAssert(ct, "TestEncyptedEnvWithEncyptedDbFlag",
	    encryptTestCase(ct, 1, 1, 0, 1) == 0);
#else
	printf("TestEncyptedEnvWithEncyptedDbFlag "
	    "is not supported by the build.\n");
#endif /* HAVE_CRYPTO */
	return (0);
}

int TestEncyptedEnvWithEncyptedDb(CuTest *ct) {
/* Run this test only when cryptography is supported. */
#ifdef HAVE_CRYPTO
	CuAssert(ct, "TestEncyptedEnvWithEncyptedDb",
	    encryptTestCase(ct, 1, 1, 1, 0) == 0);
#else
	printf("TestEncyptedEnvWithEncyptedDb "
	    "is not supported by the build.\n");
#endif /* HAVE_CRYPTO */
	return (0);
}

int TestEncyptedEnvWithEncryptedDbFlagAndDb(CuTest *ct) {
/* Run this test only when cryptography is supported. */
#ifdef HAVE_CRYPTO
	CuAssert(ct, "TestEncyptedEnvWithEncryptedDbFlagAndDb",
	    encryptTestCase(ct, 1, 1, 1, 1) == 0);
#else
	printf("TestEncyptedEnvWithEncyptedDbFlagAndDb "
	    "is not supported by the build.\n");
#endif /* HAVE_CRYPTO */
	return (0);
}

int initConfig(CRYPT_CONFIG *crypt, int in_env, int is_env_encrypt,
    int is_db_encrypt, int is_db_flags_encrypt)
{
	memset(crypt, 0, sizeof(CRYPT_CONFIG));
	crypt->in_env = in_env;
	crypt->is_env_encrypt = is_env_encrypt;
	crypt->is_db_encrypt = is_db_encrypt;
	crypt->is_db_flags_encrypt = is_db_flags_encrypt;

	return (0);
}

/*
 * in_env: whether open the database in an environment
 * is_env_encrypt: whether call DB_ENV->set_encrypt() with DB_ENCRYPT_AES
 * is_db_encrypt: whether call DB->set_encrypt() with DB_ENCRYPT_AES
 * is_db_flags_encrypt: whether call DB->set_flags() with DB_ENCRYPT
 */
int encryptTestCase(CuTest *ct, int in_env, int is_env_encrypt,
    int is_db_encrypt, int is_db_flags_encrypt)
{
	CRYPT_CONFIG crypt;
	DB_ENV *dbenv;
	DB *dbp;
	char dbname[100], *home;
	int ret;

	dbenv = NULL;
	dbp = NULL;
	home = "TESTDIR";
	ret = 0;

	TestEnvConfigTestSetup(ct);
	initConfig(&crypt, in_env, is_env_encrypt, is_db_encrypt,
	    is_db_flags_encrypt);

	if (in_env) {
		strcpy(dbname, DATABASE);
		CuAssert(ct, "Open environment",
		    openEnv(ct, &dbenv, home, is_env_encrypt) == 0);
	} else
		sprintf(dbname, ".//%s//%s", home, DATABASE);

	ret = openDb(ct, &dbp, dbenv, dbname, &crypt);

	/* Close the dbenv, dbp handle.*/
	CuAssert(ct, "closeDb", closeDb(dbenv, dbp) == 0);
	dbenv = NULL;
	dbp = NULL;

	/* Re-open the database and do some operations. */
	if (!ret) {
		sprintf(dbname, ".//%s//%s", home, DATABASE);
		CuAssert(ct, "Re-open database",
		    reOpen(ct, &dbp, dbname, &crypt) == 0);

		/* Close the dbenv, dbp handle. */
		CuAssert(ct, "closeDb", closeDb(dbenv, dbp) == 0);
		dbenv = NULL;
		dbp = NULL;
	}

	TestEnvConfigTestTeardown(ct);

	return (0);
}

int openEnv(CuTest *ct, DB_ENV **dbenvp, char *home, int is_encrypt)
{
	DB_ENV *dbenv;

	dbenv = NULL;

	CuAssert(ct, "db_env_create", db_env_create(&dbenv, 0) == 0);

	*dbenvp = dbenv;

	dbenv->set_errcall(dbenv, NULL);

	if (is_encrypt)
		CuAssert(ct, "DB_ENV->set_encrypt:DB_ENCRYPT_AES",
		    dbenv->set_encrypt(dbenv, PASSWORD, DB_ENCRYPT_AES) == 0);

	CuAssert(ct, "DB_ENV->open", dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0) == 0);

	return (0);
}

int openDb(CuTest *ct, DB **dbpp, DB_ENV *dbenv, char *dbname,
    CRYPT_CONFIG *crypt)
{
	DB *dbp;
	int expected_value, ret;

	dbp = NULL;
	ret = 0;

	CuAssert(ct, "openDb:db_create:%s", db_create(&dbp, dbenv, 0) == 0);

	*dbpp = dbp;

	dbp->set_errcall(dbp, NULL);

	/*
	 * DB->set_flag() with DB_ENCRYPT should only be used in
	 * an encrypted environment.
	 */
	if (crypt->is_db_flags_encrypt) {
		ret = dbp->set_flags(dbp, DB_ENCRYPT);

		if (!crypt->in_env || !crypt->is_env_encrypt)
			expected_value = EINVAL;
		else
			expected_value = 0;

		CuAssert(ct, "openDb:DB->set_flags:DB_ENCRYPT",
		    ret == expected_value);
	}

	/* DB->set_encrypt() cannot be used in an opened environment. */
	if (!ret && crypt->is_db_encrypt) {
		ret = dbp->set_encrypt(dbp, PASSWORD, DB_ENCRYPT_AES);

		if (crypt->in_env)
			expected_value = EINVAL;
		else
			expected_value = 0;

		CuAssert(ct, "openDb:DB->set_encrypt:DB_ENCRYPT_AES",
		    ret == expected_value);
	}

	if (!ret) {
		CuAssert(ct, "openDb: DB->open", dbp->open(dbp, NULL,
		    dbname, NULL, DB_BTREE, DB_CREATE, 0) == 0);
		CuAssert(ct, "dbPutGet", dbPutGet(ct, dbp) == 0);
	}

	return (ret);
}

/* Re-open the previous database and check the encryption. */
int reOpen(CuTest *ct, DB **dbpp, char *dbname, CRYPT_CONFIG *crypt)
{
	DB *dbp;
	int expected_value, ret;

	dbp = NULL;
	expected_value = ret = 0;

	CuAssert(ct, "reOpen: db_create fails",
	    db_create(&dbp, NULL, 0) == 0);

	*dbpp = dbp;

	dbp->set_errcall(dbp, NULL);

	/* Re-open a database should fail if the database is encrypted.*/
	ret = dbp->open(dbp, NULL, dbname, NULL, DB_UNKNOWN, 0, 0);

	/*
	 * If call DB_ENV->set_encrypt and DB->set_flag with DB_ENCRYPT,
	 * the database should be successfully encrypted, hence, re-open
	 * should be fail.
	 * 
	 * Or
	 * 
	 * If call DB->set_encrypt without in an environment,
	 * then the database can be successfully encrypted, hence,
	 * re-open should fails and return error code EINVAL.
	 */
	if ((crypt->in_env && crypt->is_env_encrypt &&
	    crypt->is_db_flags_encrypt) ||
	    (crypt->is_db_encrypt && !crypt->in_env))
		expected_value = EINVAL;

	CuAssert(ct, "reOpen: DB->open", ret == expected_value);

	/* Do some put and get operations if re-open is successful. */
	if (!ret)
		CuAssert(ct, "dbPutGet", dbPutGet(ct, dbp) == 0);

	return (0);
}

int dbPutGet(CuTest *ct, DB *dbp)
{
	DBT key, data;
	char buf[1024];
	const char *str = "abcdefghijklmnopqrst";
	int cnt, ret, len;

	ret = 0;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	srand((int)time(NULL));

	for (cnt = 1; cnt < 10001; cnt++) {
		len = rand() % strlen(str) + 1;
		(void)sprintf(buf, "%05d_%*s", cnt, len, str);

		key.data = &cnt;
		key.size = sizeof(cnt);

		data.data = buf;
		data.size = (u_int32_t)strlen(buf) + 1;

		CuAssert(ct, "DB->put",
		    dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}

	return (0);
}

int closeDb(DB_ENV *dbenv, DB *dbp)
{
	int ret = 0;

	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0)
		fprintf(stderr, "%s: DB->close: %s",
		    progname_crypt, db_strerror(ret));

	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0)
		fprintf(stderr, "%s: DB_ENV->close: %s",
		    progname_crypt, db_strerror(ret));

	return (ret);
}
