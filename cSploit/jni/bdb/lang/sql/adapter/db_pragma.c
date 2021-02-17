/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

/*
** This file implements the Berkeley DB specific pragmas.
*/
#include "sqliteInt.h"
#include "btreeInt.h"

extern void returnSingleInt(Parse *, const char *, i64);
extern int __os_exists (ENV *, const char *, int *);
extern int __os_unlink (ENV *, const char *, int);
extern int __os_mkdir (ENV *, const char *, int);
extern void __db_chksum (void *, u_int8_t *, size_t, u_int8_t *, u_int8_t *);
extern int __db_check_chksum (ENV *, void *, DB_CIPHER *, u_int8_t *, void *,
    size_t, int);
extern int __env_ref_get (DB_ENV *, u_int32_t *);

static const char *PRAGMA_FILE = "pragma";
static const char *PRAGMA_VERSION = "1.0";

static const char *ACK_POLICY_ALL = "all_sites";
static const char *ACK_POLICY_ALL_AVAILABLE = "all_available";
static const char *ACK_POLICY_NONE = "none";
static const char *ACK_POLICY_ONE = "one";
static const char *ACK_POLICY_QUORUM = "quorum";

static const char *REP_SITE_MASTER = "MASTER";
static const char *REP_SITE_CLIENT = "CLIENT";
static const char *REP_SITE_UNKNOWN = "UNKNOWN";

static const u32 HDR_SIZE = 256;
static const u32 RECORD_HDR_SIZE = 8;
static const u32 VERSION_RECORD_SIZE = 12;
static const char *pragma_names[] = {"persistent_version", "persistent_echo",
    "replication_init", "replication_local_site", "replication_remote_site",
    "replication_verbose_output", "replication_verbose_file",
    "replication"};
static const u32 DEFINED_PRAGMAS = 8;

/* Checks if the given pragma has been loaded into the cache. */
#define	PRAGMA_LOADED(index)	(p->pBt->pragma[index].offset)

/* Checks if the entire cache has been loaded. */
#define	CACHE_LOADED	(p->pBt->cache_loaded)

/* Returns the location of the given pragma offset/size in the file header. */
#define	RECORD_OFFSET(pragma_index) ((pragma_index * 8) + 12)
#define	RECORD_SIZE(pragma_index) ((pragma_index * 8) + 8)

#define	pPragma (p->pBt->pragma)

#define	dbExists (pDb->pBt->pBt->full_name != NULL && \
    !__os_exists(NULL, pDb->pBt->pBt->full_name, NULL))

/* Translates a text ack policy to its DB value. */
static int textToAckPolicy(const char *policy) 
{
	int len;
	if (policy == NULL)
		return (-1);

	len = (int)strlen(policy);

	if ((sqlite3StrNICmp(policy, ACK_POLICY_ALL, len)) == 0)
		return (DB_REPMGR_ACKS_ALL);
	else if ((sqlite3StrNICmp(policy, ACK_POLICY_ALL_AVAILABLE, len)) == 0)
		return (DB_REPMGR_ACKS_ALL_AVAILABLE);
	else if ((sqlite3StrNICmp(policy, ACK_POLICY_NONE, len)) == 0)
		return (DB_REPMGR_ACKS_NONE);
	else if ((sqlite3StrNICmp(policy, ACK_POLICY_ONE, len)) == 0)
		return (DB_REPMGR_ACKS_ONE);
	else if ((sqlite3StrNICmp(policy, ACK_POLICY_QUORUM, len)) == 0)
		return (DB_REPMGR_ACKS_QUORUM);
	else
		return (-1);
}

/* Translates a DB value ack policy to text. */
static const char *ackPolicyToText(int policy)
{
	if (policy == DB_REPMGR_ACKS_ALL)
		return (ACK_POLICY_ALL);
	else if (policy == DB_REPMGR_ACKS_ALL_AVAILABLE)
		return (ACK_POLICY_ALL_AVAILABLE);
	else if (policy == DB_REPMGR_ACKS_NONE)
		return (ACK_POLICY_NONE);
	else if (policy == DB_REPMGR_ACKS_ONE)
		return (ACK_POLICY_ONE);
	else if (policy == DB_REPMGR_ACKS_QUORUM)
		return (ACK_POLICY_QUORUM);
	else
		return (NULL);
}

/* Translates a replication site type to text. */
static const char *repSiteTypeToText(rep_site_type_t type)
{
	if (type == BDBSQL_REP_MASTER)
		return (REP_SITE_MASTER);
	else if (type == BDBSQL_REP_CLIENT)
		return (REP_SITE_CLIENT);
	else
		return (REP_SITE_UNKNOWN);
}

static u8 envIsClosed(Parse *pParse, Btree *p, const char *pragmaName)
{
	int rc;

	rc = btreeUpdateBtShared(p, 1);
	if (rc != SQLITE_OK) {
		sqlite3ErrorMsg(pParse, "Error setting %s", pragmaName);
		sqlite3Error(p->db, rc,
		    "Error checking environment while setting %s",
		    pragmaName);
		return 0;
	}

	if (p->pBt->env_opened) {
		sqlite3ErrorMsg(pParse,
		    "Cannot set %s after accessing the database",
		    pragmaName);
		return 0;
	}
	return 1;
}

static const int MVCC_MULTIPIER = 2;
int bdbsqlPragmaMultiversion(Parse *pParse, Btree *p, u8 on)
{
	int ret;
	BtShared *pBt;
	sqlite3_mutex *mutexOpen;

	ret = 0;

	if (!envIsClosed(pParse, p, "multiversion"))
		return 1;
	pBt = p->pBt;

	if (pBt->blobs_enabled && on) {
		sqlite3ErrorMsg(pParse,
	"Cannot enable both multiversion and large record optimization.");
		return 1;
	}

	/* Do not want other processes opening the environment */
	mutexOpen = sqlite3MutexAlloc(OPEN_MUTEX(pBt->dbStorage));
	sqlite3_mutex_enter(mutexOpen);

	if (on) {
		pBt->env_oflags |= DB_MULTIVERSION;
		pBt->read_txn_flags |= DB_TXN_SNAPSHOT;
		pBt->dbenv->set_flags(pBt->dbenv, DB_MULTIVERSION, 1);
		/*
		 * Database locking only optimizes for singled-threaded access
		 * and introduces additional hangs under multi-threaded access.
		 * Therefore, we disable database locking optimization if the
		 * user wants MVCC, since MVCC implies multi-threaded access.
		 */
		pBt->dbenv->set_flags(pBt->dbenv, DB_DATABASE_LOCKING, 0);
		/*
		 * Increase the system resources if they have not
		 * already been changed.
		 */
		if (pBt->cacheSize == SQLITE_DEFAULT_CACHE_SIZE)
			pBt->cacheSize = pBt->cacheSize * MVCC_MULTIPIER;
	} else {
		pBt->env_oflags &= ~DB_MULTIVERSION;
		pBt->read_txn_flags &= ~DB_TXN_SNAPSHOT;
		pBt->dbenv->set_flags(pBt->dbenv, DB_MULTIVERSION, 0);
		/*
		 * Reset the system resources if they have not
		 * already been changed.
		 */
		if (pBt->cacheSize == SQLITE_DEFAULT_CACHE_SIZE *
		    MVCC_MULTIPIER)
			pBt->cacheSize = SQLITE_DEFAULT_CACHE_SIZE;
	}

	sqlite3_mutex_leave(mutexOpen);
	return ret;
}

static int supportsReplication(Btree *p)
{
	DB_ENV *dbenv;
	u_int32_t val = 0;

	if (p->pBt->env_opened) {
		dbenv = p->pBt->dbenv;
		dbenv->get_open_flags(dbenv, &val);
	}
	return (val & DB_INIT_REP);
}

static int hasDatabaseConnections(Btree *p)
{
	u_int32_t refCount;
	int ret;

	/*
	 * We are using __env_ref_get() to spot check whether there are
	 * any other processes or threads using the underlying BDB environment
	 * for this SQL database.  It gets the value of the environment
	 * shared region's refcnt.  Note that this does not prevent another
	 * thread or process from accessing the environment immediately after
	 * this call.
	 */
	ret = __env_ref_get(p->pBt->dbenv, &refCount);
	return (refCount > 1);
}

int setRepVerboseFile(BtShared *pBt, DB_ENV *dbenv, const char *fname,
    char *msg)
{
	FILE *vfptr;
	int rc = SQLITE_OK;

	vfptr = fopen(fname, "wb");
	if (vfptr == 0) {
		msg = "Error opening replication verbose file";
		rc = SQLITE_ERROR;
	} else {
		(void)dbenv->set_msgfile(dbenv, vfptr);
		pBt->repVerbFile = vfptr;
	}
	return (rc);
}

int unsetRepVerboseFile(BtShared *pBt, DB_ENV *dbenv, char **msg)
{
	int rc = SQLITE_OK;

	if (pBt->repVerbFile != NULL) {
		if (fclose(pBt->repVerbFile) != 0) {
			*msg = "Error closing replication verbose file";
			rc = SQLITE_ERROR;
		}
		(void)dbenv->set_msgfile(dbenv, NULL);
		pBt->repVerbFile = NULL;
	}
	return (rc);
}

/*
 * Parse and validate a DBT string containing host:port.
 *
 * Return SQLITE_OK and fill in host string and port number if successful.
 * When successful, the caller is responsible for deallocating host string.
 */
int getHostPort(const char *hpstr, char **host, u_int *port)
{
	char *colonpos;
	int tmpport = -1;
	int rc = SQLITE_OK;

	*port = 0;

	/* Copy entire host:port string from value. */
	*host = sqlite3_malloc((int)strlen(hpstr) + 1);
	if (*host == NULL)
		return SQLITE_NOMEM;
	strcpy(*host, hpstr);

	/* Make sure we have two strings separated by colon. */
	colonpos = strchr(*host, ':');
	if (colonpos <= *host ||
	    colonpos == *host + strlen(*host) - 1)
		rc = SQLITE_ERROR;

	/* Make sure we have numeric value for port. */
	if (rc == SQLITE_OK &&
	    sqlite3GetInt32(colonpos + 1, &tmpport) && tmpport > 0)
			*port = (u_int)tmpport;
	else
		rc = SQLITE_ERROR;

	/* Now that we have port number, only need to return host string. */
	if (rc == SQLITE_OK)
		*colonpos = '\0';

	if (rc != SQLITE_OK)
		sqlite3_free(*host);
	return rc;
}

static int bdbsqlPragmaStartReplication(Parse *pParse, Db *pDb)
{
	Btree *pBt;
	char *value;
	int rc = SQLITE_OK;
	u8 hadRemSite = 0;

	pBt = pDb->pBt;

	if (supportsReplication(pBt)) {
		sqlite3ErrorMsg(pParse, "Replication is already running");
		goto done;
	}

	/* Make sure there is a default local site value. */
	value = NULL;
	if ((rc = getPersistentPragma(pBt,
	    "replication_local_site", &value, pParse)) == SQLITE_OK &&
	    value)
		sqlite3_free(value);
	else {
		sqlite3ErrorMsg(pParse, "Must specify local site "
		    "before starting replication");
		goto done;
	}

	if (dbExists) {
		if (!pBt->pBt->env_opened) {
			if ((rc = btreeOpenEnvironment(pBt, 1)) != SQLITE_OK)
				sqlite3ErrorMsg(pParse, "Could not start "
				    "replication on an existing database");
			goto done;
		}

		/*
		 * Opening the environment started repmgr if it was
		 * configured for it, so we are done here.
		 */
		if (supportsReplication(pBt))
			goto done;
		/*
		 * Turning on replication on an existing environment not
		 * configured for replication requires recovery on the
		 * BDB environment, which requires single-threaded access.
		 * Make sure there are no other processes or threads accessing
		 * it.  This is not foolproof, because there is a small chance
		 * that another process or thread could slip in there between
		 * this call and the recovery, but it should cover most cases.
		 */
		if (hasDatabaseConnections(pBt) || pBt->pBt->nRef > 1) {
			sqlite3ErrorMsg(pParse, "Close all database "
			    "connections before turning on replication");
			goto done;
		}

		if (pBt->pBt->repStartMaster != 1) {
			sqlite3ErrorMsg(pParse, "Must be initial master to "
			    "start replication on an existing database");
			goto done;
		}

		/*
		 * Close and reopen SQL database to add replication.  Opening
		 * the underlying BDB environment with replication always
		 * performs a recovery.
		 */
		pBt->pBt->repForceRecover = 1;
		if ((rc = btreeReopenEnvironment(pBt, 0)) != SQLITE_OK)
			sqlite3ErrorMsg(pParse, "Could not "
			    "start replication on an existing database");
	} else {
		/*
		 * When starting replication without an existing database,
		 * must either start as initial master or have a remote
		 * site defined to join an existing replication group.
		 */
		value = NULL;
		if ((rc = getPersistentPragma(pBt,
		    "replication_remote_site", &value, pParse)) == SQLITE_OK &&
		    value) {
			hadRemSite = 1;
			sqlite3_free(value);
		}
		if (!dbExists && !hadRemSite &&
		    pBt->pBt->repStartMaster != 1) {
			sqlite3ErrorMsg(pParse, "Must either be initial "
			    "master or specify a remote site");
			goto done;
		}

		/* Create replication environment for new SQL database. */
		rc = btreeOpenEnvironment(pBt, 1);
	}

done:
	return rc;
}

static int bdbsqlPragmaStopReplication(Parse *pParse, Db *pDb)
{
	Btree *pBt;
	char *old_addr = NULL;
	int rc = SQLITE_OK;

	pBt = pDb->pBt;

	if (!supportsReplication(pBt)) {
		sqlite3ErrorMsg(pParse, "Replication is not currently "
		    "running");
		goto done;
	}

	/*
	 * Turning off replication requires recovery on the underlying
	 * BDB environment, which requires single-threaded access.
	 * Make sure there are no other processes or threads accessing
	 * it.  This is not foolproof, because there is a small chance
	 * that another process or thread could slip in there between
	 * this call and the recovery, but it should cover most cases.
	 */
	if (hasDatabaseConnections(pBt)) {
		sqlite3ErrorMsg(pParse, "Close all database connections "
		    "before turning off replication");
		goto done;
	}

	/*
	 * Close and reopen SQL database to remove replication.  This
	 * forces a recovery on the underlying BDB environment to remove
	 * replication region and prepare for use of FAILCHK.
	 */
	pBt->pBt->repForceRecover = 1;
	rc = btreeReopenEnvironment(pBt, 1);
	sqlite3_mutex_enter(pBt->pBt->mutex);
	pBt->pBt->repRole = BDBSQL_REP_UNKNOWN;
	old_addr = pBt->pBt->master_address;
	pBt->pBt->master_address = NULL;
	sqlite3_mutex_leave(pBt->pBt->mutex);
	if (old_addr)
		sqlite3_free(old_addr);

done:
	return rc;
}

/*
 * Parse all Berkeley DB specific pragma options.
 *
 * Return non zero to indicate the pragma parsing should continue,
 * 0 if the pragma has been fully processed.
 */
int bdbsqlPragma(Parse *pParse, char *zLeft, char *zRight, int iDb)
{
	Btree *pBt;
	Db *pDb;
	sqlite3 *db;
	int isRep, isLSite, isRSite, isVerb, isVFile, parsed, ret;

	db = pParse->db;
	pDb = &db->aDb[iDb];
	if (pDb != NULL)
		pBt = pDb->pBt;
	else
		pBt = NULL;

	parsed = 0;
	isRep = isLSite = isRSite = isVerb = isVFile = 0;
	/*
	 * Deal with the Berkeley DB specific autodetect page_size
	 * setting.
	 */
	if (sqlite3StrNICmp(zLeft, "page_size", 10) == 0 && zRight != 0) {
		int n = sqlite3Strlen30(zRight);
		if (pBt != NULL &&
		    sqlite3StrNICmp(zRight, "autodetect", n) == 0) {
			if (SQLITE_NOMEM ==
			    sqlite3BtreeSetPageSize(pBt, 0, -1, 0))
				db->mallocFailed = 1;
			parsed = 1;
		}
	} else if (sqlite3StrNICmp(zLeft, "txn_bulk", 8) == 0) {
		if (zRight)
			pBt->txn_bulk = sqlite3GetBoolean(zRight, 0);
		returnSingleInt(pParse, "txn_bulk", (i64)pBt->txn_bulk);
		parsed = 1;
		/* Enables MVCC and transactions snapshots. */
	} else if (sqlite3StrNICmp(zLeft, "multiversion", 12) == 0) {
		if (zRight)
			bdbsqlPragmaMultiversion(pParse, pDb->pBt,
			    sqlite3GetBoolean(zRight, 0));

		returnSingleInt(pParse, "multiversion",
		    (i64)((pDb->pBt->pBt->env_oflags & DB_MULTIVERSION)?
		    1 : 0));
		parsed = 1;
		/*
		 * Enables or disables transaction snapshots if MVCC is enabled.
		 */
	} else if (sqlite3StrNICmp(zLeft, "snapshot_isolation", 18) == 0) {
		if (zRight) {
			if (sqlite3GetBoolean(zRight, 0)) {
				if (pDb->pBt->pBt->env_oflags &
				    DB_MULTIVERSION) {
					pDb->pBt->pBt->read_txn_flags
					    |= DB_TXN_SNAPSHOT;
				} else {
					sqlite3ErrorMsg(pParse,
					    "Must enable read write "
					    "concurrency before enabling "
					    "snapshot isolation");
				}
			} else
				pDb->pBt->pBt->read_txn_flags &=
				    ~DB_TXN_SNAPSHOT;
		}
		returnSingleInt(pParse, "snapshot_isolation",
		    (i64)((pDb->pBt->pBt->read_txn_flags & DB_TXN_SNAPSHOT)?
		    1 : 0));
		parsed = 1;
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_system_memory", 20) == 0) {
		int iLimit = -2;
		if (zRight &&
		    envIsClosed(pParse, pDb->pBt, "bdbsql_system_memory")) {
			if (!sqlite3GetInt32(zRight, &iLimit) ||
			    (iLimit < 1 && iLimit != -1)) {
				sqlite3ErrorMsg(pParse,
				    "Invalid value bdbsql_system_memory "
				    "%s needs to be a base segment id, see "
				    "DB_ENV->set_shm_key documentation for "
				    "more information.", zRight);
			} else if (iLimit == -1)
				pDb->pBt->pBt->env_oflags &= ~DB_SYSTEM_MEM;
			else {
				if ((ret = pDb->pBt->pBt->dbenv->set_shm_key(
				    pDb->pBt->pBt->dbenv, iLimit)) != 0)
					sqlite3ErrorMsg(pParse,
					    "Failed to set shared memory key "
					    "error: %d.", ret);
				else
					pDb->pBt->pBt->env_oflags |=
					    DB_SYSTEM_MEM;
			}
		} else
			iLimit = ((pDb->pBt->pBt->env_oflags &
			    DB_SYSTEM_MEM) ? 1 : 0);

		returnSingleInt(pParse, "bdbsql_system_memory", iLimit);
		parsed = 1;
		/*
		 * This pragma is just used to test the persistent pragma API.
		 * It returns whatever it is set too.
		 */
	} else if (sqlite3StrNICmp(zLeft, "persistent_echo", 15) == 0 ||
	    (isLSite = (sqlite3StrNICmp(zLeft,
	    "replication_local_site", 22) == 0)) ||
	    (isRSite = (sqlite3StrNICmp(zLeft,
	    "replication_remote_site", 23) == 0)) ||
	    (isVFile = sqlite3StrNICmp(zLeft,
	    "replication_verbose_file", 24) == 0)) {
		int rc;
		char *value;
		u_int port;
		char *host, *msg;

		value = NULL;
		rc = SQLITE_OK;
		if (zRight) {
			if (isLSite && supportsReplication(pBt)) {
				sqlite3ErrorMsg(pParse, "Cannot change local "
				    "site after replication is turned on");
				rc = SQLITE_ERROR;
			}
			if (rc == SQLITE_OK && (isLSite || isRSite)) {
				if ((rc = getHostPort(zRight,
				    &host, &port)) == SQLITE_OK)
					sqlite3_free(host);
				else
					sqlite3ErrorMsg(pParse, "Format of "
					    "value must be host:port");
			}
			if (rc == SQLITE_OK)
				rc = setPersistentPragma(pDb->pBt, zLeft,
				    zRight, pParse);
			/*
			 * Change replication verbose file here only if
			 * database exists.
			 */
			if (isVFile && dbExists && rc == SQLITE_OK) {
				if ((rc = unsetRepVerboseFile(pBt->pBt,
				    pBt->pBt->dbenv, &msg)) != SQLITE_OK)
					sqlite3ErrorMsg(pParse, msg);
				if (rc == SQLITE_OK && strlen(zRight) > 0 &&
				    (rc = setRepVerboseFile(pBt->pBt,
				    pBt->pBt->dbenv, zRight, msg))
				    != SQLITE_OK)
					sqlite3ErrorMsg(pParse, msg);
			}
			value = zRight;
		} else
			rc = getPersistentPragma(pDb->pBt, zLeft, &value,
			    pParse);
		if (rc == SQLITE_OK) {
			sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
			sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
			    zLeft, SQLITE_STATIC);
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    (value ? value : ""), 0);
			sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		} else {
			if (pDb->pBt->db->errCode != SQLITE_OK)
				sqlite3Error(pDb->pBt->db, rc, "error in ",
				    zLeft);
		}
		if (value && value != zRight)
			sqlite3_free(value);
		parsed = 1;
	} else if ((isRep = (sqlite3StrNICmp(zLeft, "replication", 11) == 0 &&
	    strlen(zLeft) == 11)) ||
	    (isVerb = sqlite3StrNICmp(zLeft,
	    "replication_verbose_output", 26) == 0)) {
		DB_ENV *dbenv;
		int rc, startedRep, stoppedRep, turningOn;
		char setValue[2], *value;

		value = NULL;
		rc = SQLITE_OK;
		setValue[0] = '\0';
		startedRep = stoppedRep = turningOn = 0;
		if (zRight) {
			turningOn = (sqlite3GetBoolean(zRight, 0) == 1);
			strcpy(setValue, turningOn ? "1" : "0");
			rc = setPersistentPragma(pDb->pBt, zLeft,
			    setValue, pParse);

			if (rc == SQLITE_OK) {
				if (isRep && turningOn) {
					if (bdbsqlPragmaStartReplication(
					    pParse, pDb) == SQLITE_OK)
						startedRep = 1;
					else {
						sqlite3ErrorMsg(pParse,
						    "Error starting "
						    "replication");
						rc = SQLITE_ERROR;
					}
				}
				if (isRep && !turningOn) {
					if (bdbsqlPragmaStopReplication(
					    pParse, pDb) == SQLITE_OK)
						stoppedRep = 1;
					else {
						sqlite3ErrorMsg(pParse,
						    "Error stopping "
						    "replication");
						rc = SQLITE_ERROR;
					}
				}

				dbenv = pDb->pBt->pBt->dbenv;
				if (isVerb && dbExists &&
				    dbenv->set_verbose(dbenv,
					DB_VERB_REPLICATION, turningOn) != 0) {
					sqlite3ErrorMsg(pParse, "Error "
					    "in replication set_verbose call");
					rc = SQLITE_ERROR;
				}
			}
		} else
			rc = getPersistentPragma(pDb->pBt, zLeft,
			    &value, pParse);
		if (rc == SQLITE_OK) {
			sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
			sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
			    zLeft, SQLITE_STATIC);
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    (value ? value :
			    (startedRep ? "Replication started" :
			    (stoppedRep ? "Replication stopped" : setValue))),
			    0);
			sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		} else {
			if (pDb->pBt->db->errCode != SQLITE_OK)
				sqlite3Error(pDb->pBt->db, rc, "error in ",
				    zLeft);
		}
		if (value && value != zRight)
			sqlite3_free(value);
		parsed = 1;
	} else if (sqlite3StrNICmp(zLeft,
	    "replication_initial_master", 26) == 0) {
		char outValue[2];

		outValue[0] = '\0';
		if (zRight)
			pDb->pBt->pBt->repStartMaster =
			    sqlite3GetBoolean(zRight, 0) == 1 ? 1 : 0;
		strcpy(outValue,
		    pDb->pBt->pBt->repStartMaster == 1 ? "1" : "0");
		sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
		sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
		    zLeft, SQLITE_STATIC);
		sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
		    outValue, 0);
		sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		parsed = 1;
	} else if (sqlite3StrNICmp(zLeft,
	    "replication_remove_site", 23) == 0) {
		int rc;
		u_int port;
		char *host;
		DB_ENV *dbenv;
		DB_SITE *remsite = NULL;

		rc = SQLITE_OK;
		if (zRight) {
			rc = getHostPort(zRight, &host, &port);
			if (rc == SQLITE_OK) {
				dbenv = pBt->pBt->dbenv;
				if (dbenv->repmgr_site(dbenv,
				    host, port, &remsite, 0) != 0) {
					sqlite3Error(db, SQLITE_ERROR,
					    "Cannot find site to remove");
					rc = SQLITE_ERROR;
				}
				/* The remove method deallocates remsite. */
				if (rc != SQLITE_ERROR &&
				    remsite->remove(remsite) != 0) {
					sqlite3Error(db, SQLITE_ERROR, "Error "
					    "in replication call site remove");
					rc = SQLITE_ERROR;
				}
				sqlite3_free(host);
			} else
				sqlite3ErrorMsg(pParse, "Format of value must "
				    "be host:port");
		} else
			rc = SQLITE_ERROR;
		if (rc == SQLITE_OK) {
			sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
			sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
			    zLeft, SQLITE_STATIC);
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    "Replication site removed", 0);
			sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		} else {
			sqlite3ErrorMsg(pParse,
			    "Replication site not removed");
			if (pDb->pBt->db->errCode != SQLITE_OK)
				sqlite3Error(pDb->pBt->db, rc, "error in ",
				    zLeft);
		}
		parsed = 1;
		/*
		 * The schema version of the persistent pragma database.  The
		 * value of this cannot be set by the pragma, zRight is
		 * ignored.
		 */
	} else if (sqlite3StrNICmp(zLeft, "persistent_version", 18) == 0) {
		char *value;
		int rc;

		value = NULL;
		rc = SQLITE_OK;
		rc = getPersistentPragma(pDb->pBt, zLeft, &value, pParse);
		if (rc == SQLITE_OK) {
			sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
			sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
			    zLeft, SQLITE_STATIC);
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    (value ? value : ""), 0);
			sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		} else {
			if (pDb->pBt->db->errCode != SQLITE_OK)
			    sqlite3Error(pDb->pBt->db, rc, "error in ",
			    zLeft);
		}
		if (value && value != PRAGMA_VERSION)
			sqlite3_free(value);
		parsed = 1;
	} else if (sqlite3StrNICmp(zLeft, "trickle", 7) == 0) {
		int iLimit = -2;
		DB_ENV *dbenv;

		btreeUpdateBtShared(pDb->pBt, 1);
		dbenv = pDb->pBt->pBt->dbenv;
		if (pDb->pBt->pBt->env_opened && zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) ||
			    iLimit < 1 || iLimit > 100)
				sqlite3ErrorMsg(pParse,
				    "Invalid trickle value %s, must be a "
				    "percentage.", zRight);
			else if (dbenv->memp_trickle(dbenv, iLimit, NULL) != 0)
				sqlite3ErrorMsg(pParse, "trickle failed.");
		}
		returnSingleInt(pParse, "trickle",
		    (i64)((iLimit > 0 && iLimit < 101) ? iLimit : 0));
		parsed = 1;
	/*
	* This pragma is used internally for testing deadlock.  It sets
	* the priority of a transaction so it will not loose its locks
	* when deadlock occurs with a non-exclusive transaction.
	* txn_priority=#       sets the transaction priority of all
	* non-exclusive transactions in this session to #
	*/
	} else if (sqlite3StrNICmp(zLeft, "txn_priority", 12) == 0) {
		int priority;
		u_int32_t excl_priority;

		priority = 0;
		/* set the excl_priority to the max u_int32 */
		excl_priority = -1;
		if (pDb->pBt->savepoint_txn != NULL) {
			if (zRight) {
				/*
				 * Txn priority must be less than
				 * the priority given to exclusive
				 * transactions.
				 */
				if (sqlite3GetInt32(zRight, &priority) &&
				    (u_int32_t)priority < excl_priority)
					pDb->pBt->txn_priority =
					    (u_int32_t)priority;
				else
					sqlite3ErrorMsg(pParse,
					    "Invalid transaction priority %s,"
					    " must be a number.", zRight);
			}
		}
		returnSingleInt(pParse, "txn_priority",
		    pDb->pBt->txn_priority);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_log_buffer; -- DB_ENV->get_lg_bsize
	 * PRAGMA bdbsql_log_buffer = N; -- DB_ENV->set_lg_bsize
	 *   N provides the size of the log buffer size in bytes.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_log_buffer", 17) == 0) {
    		int iLimit = -2;
		u_int32_t val;

		if (zRight &&
		    envIsClosed(pParse, pDb->pBt, "bdbsql_log_buffer")) {
      			if (!sqlite3GetInt32(zRight, &iLimit) ||
			    iLimit < 0)
				sqlite3ErrorMsg(pParse,
				    "Invalid value bdbsql_log_buffer %s",
				    zRight);
			else {
				val = iLimit;
				if ((ret =
				    pDb->pBt->pBt->dbenv->set_lg_bsize(
				    pDb->pBt->pBt->dbenv, val)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set log buffer size "
					    "error: %d.", ret);
				}
			}
		} else if (zRight == NULL) {
			/* Get existing value */
			pDb->pBt->pBt->dbenv->get_lg_bsize(
				pDb->pBt->pBt->dbenv, &val);

			returnSingleInt(pParse, "bdbsql_log_buffer", val);
		}
		parsed = 1;
	/*
	 * PRAGMA bdbsql_single_process = boolean;
	 *   Turn on/off omit sharing.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_single_process", 19) == 0) {
		int new_value, is_changed, rc;
		is_changed = 0;
		rc = 0;

		if (zRight &&
		    envIsClosed(pParse, pDb->pBt, "bdbsql_single_process")) {
			new_value = sqlite3GetBoolean(zRight, 0);
			if (new_value != pDb->pBt->pBt->single_process)
				is_changed = 1;
		}

		/* Only do actual change when the value has been changed */
		if (is_changed) {
#ifdef BDBSQL_SINGLE_PROCESS
			if (pDb->pBt->pBt->single_process != 0) {
				sqlite3ErrorMsg(pParse, "Can not turn off "
				    "SINGLE_PROCESS since compiler flag "
				    "BDBSQL_SINGLE_PROCESS has been defined");
				rc = 1;
			}
#endif
			if (rc == 0)
				pDb->pBt->pBt->single_process = new_value;
		}

		returnSingleInt(pParse, "bdbsql_single_process",
				(i64)pDb->pBt->pBt->single_process);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_vacuum_fillpercent = N;
	 *   Set fill percent for Vacuum.
	 *   N provides the goal for filling pages, specified as a percentage
	 *   between 1 and 100. Any page in the database not at or above this
	 *   percentage full will be considered for vacuum.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_vacuum_fillpercent", 25)
	    == 0) {
		int iLimit = -2;
		if (zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) ||
			    (iLimit > 100 || iLimit <= 0)) {
				sqlite3ErrorMsg(pParse,
					"Invalid value "
					"bdbsql_vacuum_fillpercent %s",
					    zRight);
			} else {
				pDb->pBt->fillPercent = (u_int32_t)iLimit;
			}
		}
		iLimit = pDb->pBt->fillPercent;
		returnSingleInt(pParse, "bdbsql_vacuum_fillpercent", iLimit);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_vacuum_pages = N;
	 *   Set number of pages for IncrVacuum. If non-zero,
	 *   incremental_vacuum will return after the specified number (N) of
	 *   pages have been freed, or no more pages can be freed.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_vacuum_pages", 19) == 0) {
		int iLimit = -2;
		if (zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) || (iLimit < 1)) {
				sqlite3ErrorMsg(pParse,
				"Invalid value bdbsql_vacuum_pages %s", zRight);
			} else {
				pDb->pBt->vacuumPages = (u_int32_t)iLimit;
			}
		}
		iLimit = pDb->pBt->vacuumPages;
		returnSingleInt(pParse, "bdbsql_vacuum_pages", iLimit);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_error_file = filename;
	 * Redirect the Berkeley DB error output to the file specified by
	 * the given filename. When the filename doesn't be specified, echo
	 * current setting.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_error_file", 17) == 0) {
		char filename[BT_MAX_PATH];
		char *err_file;

		btreeUpdateBtShared(pDb->pBt, 1);
		err_file = NULL;

		if (zRight) {
			FILE *tmpfile;
			tmpfile = fopen(zRight, "a");
			if (tmpfile == NULL)
				sqlite3ErrorMsg(pParse,
				    "Can't open error file %s", zRight);
			else {
				fclose(tmpfile);
				sqlite3_mutex_enter(pBt->pBt->mutex);
				sqlite3_free(pBt->pBt->err_file);
				pBt->pBt->err_file =
				    sqlite3_mprintf("%s", zRight);
				sqlite3_mutex_leave(pBt->pBt->mutex);
				err_file = zRight;
			}
		}

		if (err_file == NULL) {
			err_file = filename;
			btreeGetErrorFile(pDb->pBt->pBt, err_file);
		}
		sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
		sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
		    zLeft, SQLITE_STATIC);
		if (err_file)
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    err_file, 0);
		else 
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    "NULL", 0);
		sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_shared_resources; -- get_shared_resources
	 * PRAGMA bdbsql_shared_resources = N; -- set_shared_resources
	 *   Set maximum memory allocation.
	 *   N provides the size of the maximum amount of memory to be used by
	 *   shared structures in the main environment region.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_shared_resources", 23) == 0) {
    		i64 iLimit = -2;
		u_int32_t gbyte, byte;
		gbyte = 0;
		byte = 0;

		if (zRight &&
		    envIsClosed(pParse, pDb->pBt, "bdbsql_shared_resources")) {
			if (pDb->pBt->pBt->database_existed) {
				/*
				 * The DBENV->set_memory_max() method must be
				 * called prior to opening/creating the database
				 * environment.
				 */
				sqlite3ErrorMsg(pParse, 
				    "Can't set shared_resources for an open "
				    "or existing environment");
      			} else if (sqlite3Atoi64(zRight, &iLimit, 100000,
			    SQLITE_UTF8) ||
			    iLimit > (GIGABYTE * (SQLITE_MAX_U32 + 1) - 1))
				sqlite3ErrorMsg(pParse,
				    "Invalid value bdbsql_shared_resources %s",
				    zRight);
			else {
				/*
				 * We need to calculate gbyte and byte for
				 * DBENV->set_memory_max.
				 */
				gbyte = (u_int32_t)(iLimit / GIGABYTE);
				byte  = (u_int32_t)(iLimit % GIGABYTE);
				if ((ret = pDb->pBt->pBt->dbenv->set_memory_max(
				    pDb->pBt->pBt->dbenv, gbyte, byte)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set memory max "
					    "error: %d.", ret);
				}
			}
		}

		/* Get existing value */
		pDb->pBt->pBt->dbenv->get_memory_max(
		    pDb->pBt->pBt->dbenv, &gbyte, &byte);

		iLimit = (i64)gbyte * GIGABYTE + byte;
		returnSingleInt(pParse, "bdbsql_shared_resources", iLimit);
		parsed = 1;
	/*
	 * PRAGMA bdbsql_lock_tablesize; -- DB_ENV->get_lk_tablesize
	 * PRAGMA bdbsql_lock_tablesize = N; -- DB_ENV->set_lk_tablesize
	 *   N provides the size of the lock object hash table to be 
	 *   configured in the Berkeley DB environment.
	 */
	} else if (sqlite3StrNICmp(zLeft, "bdbsql_lock_tablesize", 19) == 0) {
    		int iLimit = -2;
		u_int32_t val;

		if (zRight &&
		    envIsClosed(pParse, pDb->pBt, "bdbsql_lock_tablesize")) {
			if (pDb->pBt->pBt->database_existed) {
				/*
				 * The DB_ENV->set_lk_tablesize() method must be
				 * called prior to opening/creating the database
				 * environment.
				 */
				sqlite3ErrorMsg(pParse, "Can't set lk_tablesize"
				    " for open or existing environment");
      			} else if (!sqlite3GetInt32(zRight, &iLimit) ||
			    iLimit < 0)
				sqlite3ErrorMsg(pParse,
				    "Invalid value bdbsql_lock_tablesize %s",
				    zRight);
			else {
				val = iLimit;
				if ((ret =
				    pDb->pBt->pBt->dbenv->set_lk_tablesize(
				    pDb->pBt->pBt->dbenv, val)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set lk_tablesize "
					    "error: %d.", ret);
				}
			}
		}

		/* Get existing value */
		pDb->pBt->pBt->dbenv->get_lk_tablesize(
			pDb->pBt->pBt->dbenv, &val);

		returnSingleInt(pParse, "bdbsql_lock_tablesize", val);
		parsed = 1;
	/*
	 * PRAGMA large_record_opt; -- Gets the blob threshold
	 * PRAGMA large_record_opt = N; -- Sets the blob threshold of all
	 * tables opened in the database.  The blob threshold is set to
	 * N.  A value of 0 disables blobs.
	 */
	} else if (sqlite3StrNICmp(zLeft, "large_record_opt", 16) == 0) {
    		int iLimit = -2;
		u_int32_t val;

		if (zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) || iLimit < 0) {
				sqlite3ErrorMsg(pParse,
				    "Invalid value large_record_opt %s",
				    zRight);
			} else {
				/* 
				 * SQL only supports records up to 1GB in
				 * size, so reject page counts that result
				 * in a blob threshold that will always be
				 * larger than 1 GB, to prevent overflow.
				 */
				if (iLimit > GIGABYTE) {
					sqlite3ErrorMsg(pParse,
		"large_record_opt must be less than or equal to 1 gigabyte",
					    zRight);
				} else {
					pDb->pBt->pBt->blob_threshold
					    = iLimit;
					if (iLimit > 0) {
						pDb->pBt->pBt->blobs_enabled
						    = 1;
					}
				}
			}
		}

		if (pDb->pBt->pBt->blob_threshold < 0)
			val = 0;
		else
		    val = pDb->pBt->pBt->blob_threshold;
		returnSingleInt(pParse, "large_record_opt", val);
		parsed = 1;
	/*
	 * PRAGMA replication_ack_policy; -- DB_ENV->repmgr_get_ack_policy
	 * PRAGMA replication_ack_policy =
	 * [all_sites|all_available|none|one|quorum];
	 * -- DB_ENV->repmgr_set_ack_policy
	 *   Sets the policy on how many clients must acknowledge a transaction
	 *   commit message before the master will consider it durable.
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_ack_policy", 22) == 0) {
		int val = -1;
		const char *policy;

		if (zRight) {
			if ((val = textToAckPolicy(zRight)) == -1)
				sqlite3ErrorMsg(pParse,
				    "Invalid value replication_ack_policy %s",
				    zRight);
			else {
				if ((ret =
				    pDb->pBt->pBt->dbenv->repmgr_set_ack_policy(
				    pDb->pBt->pBt->dbenv, val)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set "
					    "replication_ack_policy. "
					    "Error: %d.", ret);
				}
			}
		}

		/* Get existing value */
		pDb->pBt->pBt->dbenv->repmgr_get_ack_policy(
			pDb->pBt->pBt->dbenv, &val);

		policy = ackPolicyToText(val);
		sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
		sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
		    zLeft, SQLITE_STATIC);
		if (policy != NULL)
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    policy, 0);
		else 
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0,
			    "No policy", 0);
		sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		parsed = 1;
	/*
	 * PRAGMA replication_ack_timeout; -- DB_ENV->rep_get_timeout
	 * PRAGMA replication_ack_timeout = N; -- DB_ENV->rep_set_timeout
	 *   N provides the greater than -1 timeout on acks from clients
	 *   when the master sends a transaction commit message.
	 */
	} else if
	    (sqlite3StrNICmp(zLeft, "replication_ack_timeout", 23) == 0) {
		int iLimit = -2;
		db_timeout_t val;

		if (zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) || iLimit < 0)
				sqlite3ErrorMsg(pParse,
				    "Invalid value replication_ack_timeout %s",
				    zRight);
			else {
				val = iLimit;
				if ((ret = 
				    pDb->pBt->pBt->dbenv->rep_set_timeout(
				    pDb->pBt->pBt->dbenv, DB_REP_ACK_TIMEOUT,
				    val)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set "
					    "replication_ack_timeout. "
					    "Error: %d.", ret);
				}
			}
		}

		/* Get existing value */
		pDb->pBt->pBt->dbenv->rep_get_timeout(
			pDb->pBt->pBt->dbenv, DB_REP_ACK_TIMEOUT, &val);

		returnSingleInt(pParse, "replication_ack_timeout", val);
		parsed = 1;
	/*
	 * PRAGMA replication_priority; -- DB_ENV->rep_get_priority
	 * PRAGMA replication_priority = N; -- DB_ENV->rep_set_priority
	 *   N provides the 1 or greater priority of the replication site,
	 *   which is used to decide what site is elected master.
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_priority", 20) == 0) {
    		int iLimit = -2;
		u_int32_t val;

		if (zRight) {
			if (!sqlite3GetInt32(zRight, &iLimit) || iLimit < 1)
				sqlite3ErrorMsg(pParse,
				    "Invalid value replication_priority %s",
				    zRight);
			else {
				val = iLimit;
				if ((ret =
				    pDb->pBt->pBt->dbenv->rep_set_priority(
				    pDb->pBt->pBt->dbenv, val)) != 0) {
					sqlite3ErrorMsg(pParse,
					    "Failed to set replication "
					    "priority. Error: %d.", ret);
				}
			}
		}

		/* Get existing value */
		pDb->pBt->pBt->dbenv->rep_get_priority(
			pDb->pBt->pBt->dbenv, &val);

		returnSingleInt(pParse, "replication_priority", val);
		parsed = 1;
	/*
	 * PRAGMA replication_num_sites; -- DB_ENV->repmgr_site_list
	 *   Returns the number of sites in the replication group.
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_num_sites", 21) == 0) {
		u_int32_t val = -1;
		DB_REPMGR_SITE *sites = NULL;

		/* Get existing value */
		pDb->pBt->pBt->dbenv->repmgr_site_list(
			pDb->pBt->pBt->dbenv, &val, &sites);

		/* 
		 * repmgr_site_list only gets the list of remote sites, 
		 * so add one for the local site if replication has been
		 * started.
		 */
		if (pDb->pBt->pBt->repStarted)
			val++;

		if (sites)
			sqlite3_free(sites);

		returnSingleInt(pParse, "replication_num_sites", val);
		parsed = 1;
	/*
	 * PRAGMA replication_perm_failed; -- DB_REP_PERM_FAILED
	 *   Returns the number of times since the last time this pragma was
	 *   called the master was unable to get enough acknowledgments from
	 *   clients when committing a transaction.
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_perm_failed", 23) == 0) {
		u_int32_t val = 0;
		
		sqlite3_mutex_enter(pDb->pBt->pBt->mutex);
		val = pDb->pBt->pBt->permFailures;
		pDb->pBt->pBt->permFailures = 0;
		sqlite3_mutex_leave(pDb->pBt->pBt->mutex);

		returnSingleInt(pParse, "replication_perm_failed", val);
		parsed = 1;
	/*
	 * PRAGMA replication_site_status;
	 *   Returns MASTER if the site is a replication master, CLIENT if the
	 *   site is a replication client, and UNKNOWN if replication is not
	 *   enabled, or the site status is still being decided (such as during
	 *   an election).
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_site_status", 23) == 0) {
		rep_site_type_t val;
		const char *type;
		
		sqlite3_mutex_enter(pDb->pBt->pBt->mutex);
		val = pDb->pBt->pBt->repRole;
		sqlite3_mutex_leave(pDb->pBt->pBt->mutex);

		type = repSiteTypeToText(val);
		sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
		sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
		    zLeft, SQLITE_STATIC);
		sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0, type, 0);
		sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		parsed = 1;
	/*
	 * PRAGMA replication_get_master;
	 *   Returns the host:port of the master, or NULL if replication has
	 *   not started, or there is none.
	 */
	} else if (sqlite3StrNICmp(zLeft, "replication_get_master", 23) == 0) {
		const char *address;
		
		sqlite3_mutex_enter(pDb->pBt->pBt->mutex);
		address = pDb->pBt->pBt->master_address;
		sqlite3_mutex_leave(pDb->pBt->pBt->mutex);

		sqlite3VdbeSetNumCols(pParse->pVdbe, 1);
		sqlite3VdbeSetColName(pParse->pVdbe, 0, COLNAME_NAME,
		    zLeft, SQLITE_STATIC);
		if (address)
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0, address, 0);
		else
			sqlite3VdbeAddOp4(pParse->pVdbe, OP_String8, 0, 1, 0, "NULL", 0);
		sqlite3VdbeAddOp2(pParse->pVdbe, OP_ResultRow, 1, 1);
		parsed = 1;
	}

	/* Return semantics to match strcmp. */
	return (!parsed);
}

/*
 * Returns the index that is the key for the given pragma name.
 *  0	pragma schema version
 *  1	persistent_echo
 *  2	replication_init (undocumented)
 *  3	replication_local_site
 *  4	replication_remote_site
 *  5	replication_verbose_output
 *  6	replication_verbose_file
 *  7	replication
 */
static int getPragmaIndex(const char *pragma_name)
{
	u32 i;
	for (i = 0; i < DEFINED_PRAGMAS; i++) {
		if (sqlite3StrNICmp(pragma_name, pragma_names[i],
		    (int)strlen(pragma_names[i])) == 0)
			return i;
	}
	return -1;
}

/* Inverse of getPragmaIndex. */
static const char *getPragmaName(u32 pragma_index)
{
	if (pragma_index < DEFINED_PRAGMAS)
		return pragma_names[pragma_index];
	else
		return NULL;
}

static int openPragmaFile(Btree *p, sqlite3_file **file, int flags,
    int lock_type)
{
	sqlite3_file *pragma_file;
	char buf[BT_MAX_PATH];
	int attrs, rc, dir_exists, ret, is_dir;

	ret = 0;
	rc = SQLITE_OK;
	/*
	 * If the environment directory does not exist, create it, otherwise
	 * creating the pragma file may fail.
	 */
	pragma_file = NULL;
	ret = __os_exists(NULL, p->pBt->dir_name, &is_dir);
	if ((ret != ENOENT && ret != EFAULT && ret != 0) ||
	    (ret == 0 && !is_dir))
		return dberr2sqlite(ret, p);
	else
		dir_exists = !ret;
	ret = 0;
	if (!dir_exists) {
		if ((ret = __os_mkdir(NULL, p->pBt->dir_name, 0777)) != 0)
			return dberr2sqlite(ret, p);

	}
	sqlite3_snprintf(sizeof(buf), buf, "%s/%s", p->pBt->dir_name,
	    PRAGMA_FILE);

	/* Open or create the pragma file. */
	pragma_file = (sqlite3_file *)sqlite3_malloc(p->db->pVfs->szOsFile);
	if (pragma_file == NULL)
		return SQLITE_NOMEM;
	memset(pragma_file, 0, p->db->pVfs->szOsFile);

	*file = pragma_file;
	/*
	 * Open the file, SQLITE_OPEN_SUBJOURNAL is included to avoid an
	 * assertion failure in Unix, its inclusion has no other effect.
	 */
	rc = sqlite3OsOpen(p->db->pVfs, buf, pragma_file,
	    flags | SQLITE_OPEN_SUBJOURNAL, &attrs);
	if (rc != SQLITE_OK)
		goto err;

	/*
	 * These are the only locks supported at the moment in this
	 * function.
	 */
	assert(lock_type == SHARED_LOCK || lock_type == EXCLUSIVE_LOCK);

	/*
	 * EXCLUSIVE_LOCK requires a SHARED_LOCK be gotten first on
	 * windows.
	 */
	if ((rc = sqlite3OsLock(pragma_file, SHARED_LOCK)) != SQLITE_OK) {
	    sqlite3OsClose(pragma_file);
	    goto err;
	}
	if (lock_type == EXCLUSIVE_LOCK) {
	    if ((rc = sqlite3OsLock(pragma_file, lock_type))
		!= SQLITE_OK) {
		    sqlite3OsUnlock(pragma_file, NO_LOCK);
		    sqlite3OsClose(pragma_file);
		    goto err;
	    }
	}

	return SQLITE_OK;

err:	sqlite3_free(pragma_file);
	pragma_file = NULL;
	return rc;
}

static void removeCorruptedRecords(Btree *p, int *corrupted, int num_corrupted,
    sqlite3_file *pragma_file, Parse *pParse)
{
	int i;
	char buf[BT_MAX_PATH];
	/*
	 * If no list of corrupted records is supplied then the entire file is
	 * corrupted and needs to be deleted.
	 */
	if (corrupted == NULL) {
		if (pParse) {
			sqlite3ErrorMsg(pParse,
			    "Persistent pragma database corrupted. "
			    "All persistent pragma values lost. "
			    "Please re-enter all pragmas.");
		}
		sqlite3Error(p->db, SQLITE_CORRUPT,
		    "Persistent pragma database corrupted. "
		    "All persistent pragma values lost. "
		    "Please re-enter all pragmas.");
		sqlite3_snprintf(sizeof(buf), buf, "%s/%s", p->pBt->dir_name,
		    PRAGMA_FILE);

		/* Reset the cache. */
		cleanPragmaCache(p);
		memset(pPragma, 0, sizeof(pPragma[0]) * NUM_DB_PRAGMA);

		/* Delete the pragma file. */
		sqlite3OsUnlock(pragma_file, NO_LOCK);
		(void)sqlite3OsClose(pragma_file);
		sqlite3_free(pragma_file);
		sqlite3OsDelete(p->db->pVfs, buf, 0);
		return;
	}
	sprintf(buf, "Persistent pragma %s corrupted, please re-enter.",
	    getPragmaName(corrupted[0]));
	if (pParse)
		sqlite3ErrorMsg(pParse, buf);
	sqlite3Error(p->db, SQLITE_CORRUPT, buf);

	for (i = 0; i < num_corrupted; i++) {
		u32 invalid = 0;
		u64 invalid2 = 0;
		int idx, offset;

		idx = corrupted[i];
		offset = pPragma[idx].offset;
		/*
		 * Invalidate the corrupted record  by setting its offset and
		 * size to 0 in the header, and setting the pragma index to 0
		 * in its record.  Also clear the entry in the cache.
		 */
		if (sqlite3OsWrite(pragma_file, &invalid2, 8, RECORD_SIZE(i))
			!= SQLITE_OK)
			break;
		if (sqlite3OsWrite(pragma_file, &invalid, 4, offset)
		    != SQLITE_OK)
			break;
		if (pPragma[idx].value != NULL && idx != 0) {
			sqlite3_free(pPragma[idx].value);
			pPragma[idx].value = NULL;
		}
		pPragma[idx].offset = pPragma[idx].size = 0;
	}

	/* Read in the header file and recalculate the checksum. */
	if (sqlite3OsRead(pragma_file, buf, HDR_SIZE, 0) != SQLITE_OK)
		goto err;
	__db_chksum(
	    NULL, (u_int8_t *)&buf[4], HDR_SIZE - 4, NULL, (u_int8_t *)buf);
	sqlite3OsWrite(pragma_file, buf, HDR_SIZE, 0);

err:	return;
}

static int insertPragmaIntoFile(Btree *p, u32 pragma_index,
    sqlite3_file *pragma_file, int exists, Parse *pParse)
{
	unsigned char buf[BT_MAX_PATH], *data;
	int ret, rc;
	u_int8_t pragma_version[12];
	u8 corrupted;
	u32 int_value, size;

	ret = 0;
	corrupted = 0;
	rc = SQLITE_OK;
	data = NULL;

	if (!p->pBt || p->pBt->dbStorage != DB_STORE_NAMED)
		return SQLITE_OK;

	size = pPragma[pragma_index].size;

	/*
	 * Create the record, which consists of
	 * [pragma index][checksum][value].
	 */
	if ((data = sqlite3_malloc(size + RECORD_HDR_SIZE)) == NULL) {
		rc = SQLITE_NOMEM;
		goto err;
	}

	memcpy(data, &pragma_index, 4);
	memcpy(&data[RECORD_HDR_SIZE], pPragma[pragma_index].value, size);
	__db_chksum(NULL, &data[RECORD_HDR_SIZE], size, NULL, &data[4]);
	/*
	 * If creating the file then add in the file header and schema pragma
	 * version record.
	 */
	if (!exists) {
		memcpy(pragma_version, &pragma_index, 4);
		memcpy(&pragma_version[RECORD_HDR_SIZE], PRAGMA_VERSION, 4);
		__db_chksum(NULL, &pragma_version[RECORD_HDR_SIZE], 4, NULL,
		    &pragma_version[4]);
		memset(buf, 0, sizeof(unsigned char) * BT_MAX_PATH);
		/* Set the offset for the next new record. */
		int_value = HDR_SIZE + VERSION_RECORD_SIZE + size
		    + RECORD_HDR_SIZE;
		memcpy(&buf[4], &int_value, 4);
		int_value = 4;
		memcpy(&buf[RECORD_SIZE(0)], &int_value, 4);
		memcpy(&buf[RECORD_OFFSET(0)], &HDR_SIZE, 4);
		memcpy(&buf[RECORD_SIZE(pragma_index)], &size, 4);
		pPragma[pragma_index].offset = HDR_SIZE + VERSION_RECORD_SIZE;
		memcpy(&buf[RECORD_OFFSET(pragma_index)],
		    &pPragma[pragma_index].offset, 4);

		__db_chksum(NULL, &buf[4], HDR_SIZE - 4, NULL, buf);
		if ((rc = sqlite3OsWrite(pragma_file, buf, HDR_SIZE, 0))
		    != SQLITE_OK)
			goto err;
		if ((rc = sqlite3OsWrite(pragma_file, pragma_version,
		    VERSION_RECORD_SIZE, HDR_SIZE)) != SQLITE_OK)
			goto err;
		if ((rc = sqlite3OsWrite(pragma_file, data, size
		    + RECORD_HDR_SIZE, HDR_SIZE + VERSION_RECORD_SIZE))
		    != SQLITE_OK)
			goto err;
	} else {
		memset(buf, 0, sizeof(unsigned char) * BT_MAX_PATH);
		if ((rc = sqlite3OsRead(pragma_file, buf, HDR_SIZE, 0))
		    != SQLITE_OK) {
			/*
			 * If something was written to the buffer then the
			 * header was not completely written and it is a
			 * corruption error.
			 */
			if (rc > 15) {
				rc = SQLITE_CORRUPT;
				corrupted = 1;
				goto err;
			} else
				goto err;
		}
		/* Check that the offsets have not been corrupted. */
		ret = __db_check_chksum(NULL, NULL, NULL, buf, &buf[4],
		    HDR_SIZE - 4, 0);
		if (ret == -1) {
			rc = SQLITE_CORRUPT;
			corrupted = 1;
			goto err;
		} else if (ret != 0)
			goto err;
		/* If there is an old record, invalidate it. */
		if ((u32)buf[RECORD_OFFSET(pragma_index)] != 0) {
			u32 invalid_record, offset;

			invalid_record = 0;
			memcpy(&offset, &buf[RECORD_OFFSET(pragma_index)], 4);
			if ((rc = sqlite3OsWrite(pragma_file, &invalid_record,
			    4, offset)) != SQLITE_OK)
				goto err;
		}
		/* Set the pragma offset and size. */
		memcpy(&buf[RECORD_SIZE(pragma_index)], &size, 4);
		memcpy(&buf[RECORD_OFFSET(pragma_index)], &buf[4], 4);
		memcpy(&pPragma[pragma_index].offset, &buf[4], 4);

		/* Recalculate the offset for new records. */
		int_value = pPragma[pragma_index].offset + size
		    + RECORD_HDR_SIZE;
		memcpy(&buf[4], &int_value, 4);

		/* Recalculate the header checksum. */
		__db_chksum(NULL, &buf[4], HDR_SIZE - 4, NULL, buf);
		if ((rc = sqlite3OsWrite(pragma_file, buf, HDR_SIZE, 0))
		    != SQLITE_OK)
			goto err;
		if ((rc = sqlite3OsWrite(pragma_file, data,
		    pPragma[pragma_index].size + RECORD_HDR_SIZE,
		   pPragma[pragma_index].offset)) != SQLITE_OK)
			goto err;
	}
err:	if (corrupted)
		removeCorruptedRecords(p, NULL, 0, pragma_file, pParse);
	if (data)
		sqlite3_free(data);
	return MAP_ERR(rc, ret, p);
}

/*
 * Reads the given pragma from the pragma file and loads it into the cache.
 * If pragma_index == -1 then load the entire file into the cache.
 */
static int readPragmaFromFile(Btree *p, sqlite3_file *pragma_file,
    int pragma_index, Parse *pParse)
{
	unsigned char buf[BT_MAX_PATH], *data;
	int ret, rc, attrs, i, start, end, num_corrupted;
	int corrupted_buf[NUM_DB_PRAGMA];
	int *corrupted;

	ret = attrs = num_corrupted = 0;
	rc = SQLITE_OK;
	corrupted = NULL;
	data = NULL;

	if (!p->pBt || p->pBt->dbStorage != DB_STORE_NAMED)
		return SQLITE_OK;

	/* A negative pragma index means read in all the pragmas. */
	if (pragma_index < 0) {
		start = 0;
		end = NUM_DB_PRAGMA;
	} else
		start = end = pragma_index;

	memset(buf, 0, sizeof(unsigned char) * BT_MAX_PATH);

	/* Load the offsets. */
	if ((rc = sqlite3OsRead(pragma_file, buf, HDR_SIZE, 0)) != SQLITE_OK) {
		/*
		 * If something was written to the buffer then the
		 * header was not completely written and it is a
		 * corruption error.
		 */
		if (rc > 15) {
			rc = SQLITE_CORRUPT;
			num_corrupted = 1;
			goto err;
		} else
			goto err;
	}

	/*
	 * Check that the offsets have not been corrupted.  If they have then
	 * the pragma file has to be deleted since there is now no way to tell
	 * which records are good and which are not.
	 */
	ret = __db_check_chksum(NULL, NULL, NULL, buf, &buf[4], HDR_SIZE - 4,
	    0);
	if (ret == -1) {
		ret = 0;
		rc = SQLITE_CORRUPT;
		num_corrupted = 1;
		goto err;
	} else if (ret != 0)
		goto err;

	/* Set the offsets in the cache. */
	for (i = start; i <= end; i++) {
		memcpy(&pPragma[i].offset , &buf[RECORD_OFFSET(i)], 4);
		memcpy(&pPragma[i].size, &buf[RECORD_SIZE(i)], 4);
	}

	/* Load the data into the cache. */
	for (i = start; i <= end; i++) {
		/* If the offset is 0 then the pragma has not been set. */
		if (pPragma[i].offset == 0)
			continue;
		/*
		 * Allocated enough space to read the record if the buffer is
		 * not large enough.
		 */
		if (data != NULL && data != buf) {
			sqlite3_free(data);
			data = NULL;
		}
		if ((pPragma[i].size + RECORD_HDR_SIZE) > BT_MAX_PATH) {
			if ((data = sqlite3_malloc(pPragma[i].size
			    + RECORD_HDR_SIZE)) == NULL) {
				rc = SQLITE_NOMEM;
				goto err;
			}
		} else
			data = buf;
		/* Read the record and check that it is not corrupted. */
		if ((rc = sqlite3OsRead(pragma_file, data,
		    pPragma[i].size + RECORD_HDR_SIZE, pPragma[i].offset))
		    != SQLITE_OK)
			goto err;
		if ((ret = __db_check_chksum(NULL, NULL, NULL, &data[4],
		    data + RECORD_HDR_SIZE, pPragma[i].size, 0)) != 0) {
			if (ret == -1) {
				if (corrupted == NULL)
					corrupted = corrupted_buf;
				corrupted[num_corrupted] = i;
				num_corrupted++;
				pPragma[i].offset = pPragma[i].size = 0;
				continue;
			} else
				goto err;
		}
		/* Copy the record data into the cache. */
		if (pPragma[i].value != NULL &&
		    pPragma[i].value != PRAGMA_VERSION) {
			sqlite3_free(pPragma[i].value);
			pPragma[i].value = NULL;
		}
		if ((pPragma[i].value =
		    sqlite3_malloc(pPragma[i].size)) == NULL) {
			rc = SQLITE_NOMEM;
			goto err;
		}
		memcpy(pPragma[i].value, &data[RECORD_HDR_SIZE],
		    pPragma[i].size);
	}
	p->pBt->cache_loaded = 1;
err:	if (num_corrupted != 0) {
		/*
		 * On corruption have to close and reopen the file with an
		 * exclusive lock.
		 */
		sqlite3OsUnlock(pragma_file, NO_LOCK);
		(void)sqlite3OsClose(pragma_file);
		sqlite3_free(pragma_file);
		rc = openPragmaFile(p, &pragma_file, SQLITE_OPEN_READWRITE,
		    EXCLUSIVE_LOCK);
		if (rc == SQLITE_OK) {
			removeCorruptedRecords(p, corrupted, num_corrupted,
			    pragma_file, pParse);
			if (corrupted != NULL) {
				sqlite3OsUnlock(pragma_file, NO_LOCK);
				(void)sqlite3OsClose(pragma_file);
				sqlite3_free(pragma_file);
			}
		}
		rc = SQLITE_CORRUPT;
	}
	if (data != NULL && data != buf)
		sqlite3_free(data);
	return MAP_ERR(rc, ret, p);
}

/*
 * Get the value set by the persistent pragma named in "name".  "value" is
 * set to NULL if the pragma has not been explicitly set to a value.  It is the
 * responsibility of the caller to delete value use sqlite3_free.
 */
int getPersistentPragma(Btree *p, const char *pragma_name, char **value,
    Parse *pParse)
{
	int rc, idx, ret, attrs;
	char buf[BT_MAX_PATH];
	sqlite3_file *pragma_file;

	rc = SQLITE_OK;
	pragma_file = NULL;

	if ((idx = getPragmaIndex(pragma_name)) < 0)
		goto err;

	/*
	 * Get a lock either through locking the pragma file, or using a
	 * mutex if there is no pragma file to prevent multiple threads
	 * from accessing the pragma cache at the same time.
	 */
	if (p->pBt->dbStorage == DB_STORE_NAMED) {
		/* If the pragma file does not exist return immediately. */
		sqlite3_snprintf(sizeof(buf), buf, "%s/%s", p->pBt->dir_name,
		    PRAGMA_FILE);
		ret = __os_exists(NULL, buf, &attrs);
		/*
		 * If the file has not been created then the only value
		 * that can be returned is the pragma version.
		 */
		if (ret == ENOENT || ret == EFAULT) {
			if (idx == 0) {
				*value = (char *)PRAGMA_VERSION;
				goto done;
			}
			goto err;
		}
		ret = 0;
		rc = openPragmaFile(p, &pragma_file, SQLITE_OPEN_READONLY,
		    SHARED_LOCK);
		if (rc != SQLITE_OK)
			goto err;

	} else
		sqlite3_mutex_enter(p->pBt->pragma_cache_mutex);

	/*
	 * If the value is not cached then call readPragmaFromFile to
	 * load it into the cache if the database is persistent.
	 */
	if (!PRAGMA_LOADED(idx) && p->pBt->dbStorage == DB_STORE_NAMED) {
		u32 index;

		/* If the cache has not been loaded then load it. */
		if (CACHE_LOADED)
			index = idx;
		else
			index = -1;
		if ((rc = readPragmaFromFile(p, pragma_file, index, pParse))
		    != SQLITE_OK)
			goto err;
	}

	/*
	 * If this database is not persistent and the pragma version has
	 * not been loaded into the cahce, then load it.
	 */
	if (p->pBt->dbStorage != DB_STORE_NAMED || !CACHE_LOADED) {
		p->pBt->cache_loaded = (p->pBt->dbStorage != DB_STORE_NAMED);
		pPragma[0].offset = HDR_SIZE;
		pPragma[0].value = (char *)PRAGMA_VERSION;
		pPragma[0].size = 4;
	}

	/* Return an empty value if the pragma is not cached.*/
	if (!PRAGMA_LOADED(idx))
		goto err;

	/* Copy the pragma value. */
	*value = sqlite3_malloc(pPragma[idx].size);
	if (!*value)
		goto err;
	memcpy(*value, pPragma[idx].value, pPragma[idx].size);

	if (0) {
err:        *value = NULL;
	}
done:	if (pragma_file != NULL && rc != SQLITE_CORRUPT) {
		sqlite3OsUnlock(pragma_file, SHARED_LOCK);
		(void)sqlite3OsClose(pragma_file);
		sqlite3_free(pragma_file);
	}
	if (p->pBt->dbStorage != DB_STORE_NAMED)
		sqlite3_mutex_leave(p->pBt->pragma_cache_mutex);
	return rc;
}

/*
 * Inserts the given value for the given pragma name into the pragma file
 * and cache.
 */
int setPersistentPragma(Btree *p, const char *pragma_name, const char *value,
    Parse *pParse)
{
	int rc, idx, attrs, exists;
	sqlite3_file *pragma_file;
	char buf[BT_MAX_PATH];

	rc = SQLITE_OK;
	pragma_file = NULL;

	if ((idx = getPragmaIndex(pragma_name)) < 0)
		goto err;

	/*
	 * Get a lock either through locking the pragma file, or using a
	 * mutex if there is no pragma file to prevent multiple threads
	 * from accessing the pragma cache at the same time.
	 */
	if (p->pBt->dbStorage == DB_STORE_NAMED) {
		/*
		 * Check whether the file exists now since opening it will
		 * create it.
		 */
		memset(buf, 0, BT_MAX_PATH);
		sqlite3_snprintf(sizeof(buf), buf, "%s/%s", p->pBt->dir_name,
		    PRAGMA_FILE);
		rc = __os_exists(NULL, buf, &attrs);
		if (rc != ENOENT && rc != EFAULT && rc != 0)
			return dberr2sqlite(rc, p);
		exists = !rc;
		rc = SQLITE_OK;
		/* Open or create the pragma file and lock it exclusively. */
		rc = openPragmaFile(p, &pragma_file,
		    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
		    EXCLUSIVE_LOCK);
		if (rc != SQLITE_OK)
			goto err;
	} else
		sqlite3_mutex_enter(p->pBt->pragma_cache_mutex);

	/* Cache the pragma value */
	if (pPragma[idx].value != NULL &&
	    pPragma[idx].value != PRAGMA_VERSION)
		sqlite3_free(pPragma[idx].value);
	pPragma[idx].size = (u_int32_t)strlen(value) + 1;
	pPragma[idx].value = sqlite3_malloc(pPragma[idx].size);
	if (pPragma[idx].value == NULL) {
		rc = SQLITE_NOMEM;
		goto err;
	}
	memcpy(pPragma[idx].value, value, pPragma[idx].size);
	/*
	 * If this database is not persistent and the pragma version has
	 * not been loaded into the cahce, then load it.
	 */
	if (p->pBt->dbStorage != DB_STORE_NAMED) {
		pPragma[idx].offset = 1;
		if (!CACHE_LOADED) {
			pPragma[0].offset = HDR_SIZE;
			pPragma[0].value = (char *)PRAGMA_VERSION;
			pPragma[0].size = 4;
		}
	}

	/* No-op if an in-memeory environment. */
	if ((rc = insertPragmaIntoFile(p, idx, pragma_file, exists, pParse))
	    != SQLITE_OK) {
		/* The pragma file was deleted if it was found corrupt. */
		if (rc == SQLITE_CORRUPT)
			pragma_file = NULL;
		goto err;
	}

err:	if (pragma_file != NULL) {
		sqlite3OsUnlock(pragma_file, NO_LOCK);
		(void)sqlite3OsClose(pragma_file);
		sqlite3_free(pragma_file);
	}
	if (p->pBt->dbStorage != DB_STORE_NAMED)
		sqlite3_mutex_leave(p->pBt->pragma_cache_mutex);
	return rc;
}

/*
 * This is only called if pBt->ref == 0, so no need to mutex protect it.  It
 * deletes the memory allocated by the pragma cache.
 */
int cleanPragmaCache(Btree *p)
{
	int i;

	for (i = 0; i < NUM_DB_PRAGMA; i++) {
		if (pPragma[i].value != NULL &&
		    pPragma[i].value != PRAGMA_VERSION)
			sqlite3_free(pPragma[i].value);
	}

	return 0;
}
