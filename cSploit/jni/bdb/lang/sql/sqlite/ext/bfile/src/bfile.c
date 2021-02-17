/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010 Oracle.  All rights reserved.
 *
 */

/*
** This file implements the sqlite bfile extension for Berkeley DB.
*/
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef BFILE_USE_CAPIS 
#include <bfile.h>
#endif
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#define DIRECTORY		"'BFILE_DIRECTORY'"
#define BFILE_PREFIX		"B"
#define PATH_SEPARATOR		"/"
#define INTERNAL_ERR_MSG	"internal error"
#define UNIQUE_ERR_MSG		"Directory already exist"
#define NOT_EXIST_ERR_MSG	"Directory does not exist"

static void BFileCreateDirectoryFunc(sqlite3_context *, int, sqlite3_value **);
static void BFileReplaceDirectoryFunc(sqlite3_context *, int, sqlite3_value **);
static void BFileDropDirectoryFunc(sqlite3_context *, int, sqlite3_value **);
static void BFileNameFunc(sqlite3_context *, int, sqlite3_value **);
static void BFileFullPathFunc(sqlite3_context *, int, sqlite3_value **);
static int search_path_by_alias(sqlite3 *, char *, char *, int, char **);
static int get_full_path(sqlite3 *, char *, int, char **);

/*
 * The BFileCreateDirectoryFunc() SQL function create a directory object.
 */
static void BFileCreateDirectoryFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	sqlite3 *db;
	sqlite3_stmt *stmt = NULL;
	char *alias, *path;
	int alias_size, path_size, rc = 0;
#define DIR_INS "insert into "DIRECTORY" values(?,?);"

	assert(context != NULL && argv != NULL && argc == 2);

	alias = (char *)sqlite3_value_text(argv[0]);
	alias_size = sqlite3_value_bytes(argv[0]);
	path = (char *)sqlite3_value_text(argv[1]);
	path_size = sqlite3_value_bytes(argv[1]);

	db = (sqlite3 *)sqlite3_user_data(context);

	if (sqlite3_prepare_v2(db, DIR_INS, sizeof(DIR_INS) - 1, &stmt, NULL))
		goto err;
		
	if (sqlite3_bind_text(stmt, 1, alias, alias_size, SQLITE_STATIC))
		goto err;

	if (sqlite3_bind_text(stmt, 2, path, path_size, SQLITE_STATIC))
		goto err;

	if ((rc = sqlite3_step(stmt)) != SQLITE_DONE)
		goto err;
	
	sqlite3_finalize(stmt);

	return;
err:
	if (stmt) 
		sqlite3_finalize(stmt);

	if (rc == SQLITE_CONSTRAINT)
		sqlite3_result_error(context, UNIQUE_ERR_MSG, -1);
	else
		sqlite3_result_error(context, INTERNAL_ERR_MSG, -1);
}

/*
 * The BFileReplaceDirectoryFunc() SQL function replace a directory object.
 */
static void BFileReplaceDirectoryFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	sqlite3 *db;
	sqlite3_stmt *stmt = NULL;
	char *alias, *path;
	int alias_size, path_size, changed = 1;
#define DIR_UPD "update "DIRECTORY" set PATH=? where ALIAS=?;"

	assert(context != NULL && argv != NULL && argc == 2);

	alias = (char *)sqlite3_value_text(argv[0]);
	alias_size = sqlite3_value_bytes(argv[0]);
	path = (char *)sqlite3_value_text(argv[1]);
	path_size = sqlite3_value_bytes(argv[1]);

	db = (sqlite3 *)sqlite3_user_data(context);

	if (sqlite3_prepare(db, DIR_UPD, sizeof(DIR_UPD) - 1, &stmt, NULL))
		goto err;
		
	if (sqlite3_bind_text(stmt, 1, path, path_size, SQLITE_STATIC))
		goto err;

	if (sqlite3_bind_text(stmt, 2, alias, alias_size, SQLITE_STATIC))
		goto err;

	if (sqlite3_step(stmt) != SQLITE_DONE)
		goto err;

	if ((changed = sqlite3_changes(db)) < 1)
		goto err;

	sqlite3_finalize(stmt);

	return;
err:
	if (stmt) 
		sqlite3_finalize(stmt);
	
	if (changed < 1)
		sqlite3_result_error(context, NOT_EXIST_ERR_MSG, -1);
	else
		sqlite3_result_error(context, INTERNAL_ERR_MSG, -1);
}

/*
 * The BFileDropDirectoryFunc() SQL function drop a directory object.
 */
static void BFileDropDirectoryFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	sqlite3 *db;
	sqlite3_stmt *stmt = NULL;
	char *alias;
	int alias_size, changed = 1;
#define DIR_DEL "delete from "DIRECTORY" where ALIAS=?;"

	assert(context != NULL && argv != NULL && argc == 1);

	alias = (char *)sqlite3_value_text(argv[0]);
	alias_size = sqlite3_value_bytes(argv[0]);

	db = (sqlite3 *)sqlite3_user_data(context);

	if (sqlite3_prepare(db, DIR_DEL, sizeof(DIR_DEL) - 1, &stmt, NULL))
		goto err;
		
	if (sqlite3_bind_text(stmt, 1, alias, alias_size, SQLITE_STATIC))
		goto err;

	if (sqlite3_step(stmt) != SQLITE_DONE)
		goto err;

	if ((changed = sqlite3_changes(db)) < 1) 
		goto err;

	sqlite3_finalize(stmt);

	return;
err:
	if (stmt) 
		sqlite3_finalize(stmt);

	if (changed < 1)
		sqlite3_result_error(context, NOT_EXIST_ERR_MSG, -1);
	else
		sqlite3_result_error(context, INTERNAL_ERR_MSG, -1);
}

/*
 * The BFileNameFunc() SQL function returns locator of a BFile.
 */
static void BFileNameFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	int dir_alias_size, filename_size, n;
	char *locator, *pLoc;

	assert(context != NULL && argc == 2 && argv != NULL);

	dir_alias_size = sqlite3_value_bytes(argv[0]);
	filename_size = sqlite3_value_bytes(argv[1]);

	if (filename_size == 0) {
		sqlite3_result_null(context);
		return;
	}

	n = strlen(BFILE_PREFIX) + 2 * sizeof(char) + dir_alias_size +
	    filename_size;

	locator = (char *)sqlite3_malloc(n);
	if (locator == NULL) {
		sqlite3_result_error_nomem(context);
		return;
	}

	/* prefix to avoid be converted to number */
	pLoc = locator;
	memcpy(pLoc, BFILE_PREFIX, strlen(BFILE_PREFIX));

	pLoc += strlen(BFILE_PREFIX);
	*pLoc = dir_alias_size & 0xff;
	*(pLoc + 1) = (dir_alias_size >> 8) & 0xff;

	pLoc += 2 * sizeof(char);
	memcpy(pLoc, (char *)sqlite3_value_text(argv[0]), dir_alias_size);

	pLoc += dir_alias_size;
	memcpy(pLoc, (char *)sqlite3_value_text(argv[1]), filename_size);

	sqlite3_result_blob(context, locator, n, SQLITE_TRANSIENT);
}

/*
 * The BFileFullPathFunc() SQL function  returns full path of a BFile
 */
static void BFileFullPathFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	int rc;
	sqlite3 *db;
	int loc_size;
	char *pLoc, *full_path;

	assert(context != NULL && argc == 1 && argv != NULL);
	
	loc_size = sqlite3_value_bytes(argv[0]);
	if (loc_size == 0) {
		sqlite3_result_null(context);
		return;
	}

	pLoc = (char *)sqlite3_value_text(argv[0]);
	db = (sqlite3 *)sqlite3_user_data(context);

	rc = get_full_path(db, pLoc, loc_size, &full_path);
	if (rc) {
		if (rc == SQLITE_NOMEM)
			sqlite3_result_error_nomem(context);
		else
			sqlite3_result_error(context, "internal error", -1);

		return;
	}

	sqlite3_result_text(context, full_path, strlen(full_path), sqlite3_free);
}

/*
 * The search_path_by_alias() function  returns the path of an alias
 */
static int search_path_by_alias(
	sqlite3 *db,
	char *zSql,
	char *alias,
	int alias_size,
	char **old_path)
{
	int rc;
	sqlite3_stmt *stmt;
	char *result;

	assert(db != NULL && zSql != NULL && old_path != NULL && alias != NULL);
	*old_path = NULL;

	rc = sqlite3_prepare(db, zSql, strlen(zSql), &stmt, NULL);
	if (rc) {
		rc = SQLITE_ERROR;
		goto err;
	}

	rc = sqlite3_bind_text(stmt, 1, alias, alias_size, SQLITE_STATIC);
	if (rc) {
		rc = SQLITE_ERROR;
		goto err;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_BUSY){};
	if (rc == SQLITE_DONE) {
		rc = SQLITE_OK;
		goto err;
	}

	if (rc != SQLITE_ROW) {
		rc = SQLITE_ERROR;
		goto err;
	}

	result = (char *)sqlite3_column_text(stmt, 0);
	*old_path = sqlite3_malloc(strlen(result) + 1);
	if (*old_path == NULL)
		rc = SQLITE_NOMEM;
	else {
		strcpy(*old_path, result);
		rc = SQLITE_OK;
	}

err:
	if (stmt)
		sqlite3_finalize(stmt);
	return rc;
}

/*
 * The get_full_path() function returns the full path of a locator
 */
static int get_full_path(
	sqlite3 *db,
	char *pLoc,
	int loc_size,
	char **pFull_path)
{
	int rc;
	char *sql = "select path from "DIRECTORY" where alias=?;";
	int  dir_alias_size, dir_size, filename_size;
	char *dir_path;

	assert(db != NULL && pLoc != NULL && loc_size > 0 &&
		pFull_path != NULL);

	if (loc_size <= strlen(BFILE_PREFIX) + 2 * sizeof(char))
		return SQLITE_ERROR;

	if (strncmp(pLoc, BFILE_PREFIX, strlen(BFILE_PREFIX)))
		return SQLITE_ERROR;

	pLoc += strlen(BFILE_PREFIX);

	dir_alias_size = ((*(pLoc + 1)) << 8) | (*pLoc);
	filename_size = loc_size - strlen(BFILE_PREFIX) - 2 * sizeof(char) -
	    dir_alias_size;
	pLoc += 2 * sizeof(char);

	if ((rc = search_path_by_alias(db, sql, pLoc, dir_alias_size,
	    &dir_path)))
		return rc;

	dir_size = dir_path == NULL ? dir_alias_size : strlen(dir_path);
	*pFull_path = sqlite3_malloc(dir_size + filename_size + 
	    strlen(PATH_SEPARATOR) + 1);
	if (*pFull_path == NULL)
		return SQLITE_NOMEM;

	memcpy(*pFull_path, dir_path == NULL ? pLoc : dir_path, dir_size);
	if (dir_path != NULL)
		sqlite3_free(dir_path);

	memcpy(*pFull_path + dir_size, PATH_SEPARATOR, strlen(PATH_SEPARATOR));
	memcpy(*pFull_path + dir_size + strlen(PATH_SEPARATOR), pLoc +
	    dir_alias_size, filename_size);

	*(*pFull_path + dir_size + strlen(PATH_SEPARATOR) + filename_size) = 0;

	return SQLITE_OK;
}

/* 
 * SQLite invokes this routine once when it loads the extension.
 * Create new functions, collating sequences, and virtual table
 * modules here.  This is usually the only exported symbol in
 * the shared library.
 */
#define FIND_DIRECTORY						\
	"SELECT COUNT(name) FROM sqlite_master			\
		WHERE name = "DIRECTORY" AND TYPE = 'table';"
#define CREATE_DIRECTORY					\
	"CREATE TABLE "DIRECTORY"(				\
		ALIAS TEXT PRIMARY KEY,				\
		PATH TEXT					\
	);"

#ifdef BFILE_USE_CAPIS
typedef struct BfileHdl BfileHdl;
struct BfileHdl {
	char *full_path;		/* full path of the BFILE */
	int fd;				/* file discriptor of the BFILE */
};

/*
 * Access BFILE element of the current row in the row set
 */
SQLITE_API int sqlite3_column_bfile(
	sqlite3_stmt *pStmt,
	int iCol,
	sqlite3_bfile **ppBfile
)
{
	BfileHdl *pHdl;
	char * pLoc;
	int loc_size, rc;
	sqlite3 *db;
#define IS_ERROR(rc) \
	((rc)!= SQLITE_OK && (rc) != SQLITE_ROW && (rc) != SQLITE_DONE)
	
	if (pStmt == NULL || iCol < 0 || iCol >= sqlite3_column_count(pStmt) 
	    || ppBfile == NULL)
		return SQLITE_ERROR;

	db = sqlite3_db_handle(pStmt);

	/*
	 * If a memory allocation error occurs during the evaluation of any of
	 * these routines, a default value is returned. The default value is
	 * either the integer 0, the floating point number 0.0, or a NULL
	 * pointer. Subsequent calls to sqlite3_errcode() will return
	 * SQLITE_NOMEM.
	 */
	pLoc = (char *)sqlite3_column_blob(pStmt, iCol);
	if (pLoc == NULL) {
		*ppBfile = NULL;
		rc = sqlite3_errcode(db);
		return (IS_ERROR(rc) ? SQLITE_ERROR : SQLITE_OK);
	}

	pHdl = sqlite3_malloc(sizeof(BfileHdl));
	if (pHdl == NULL) {
		*ppBfile = NULL;
		return SQLITE_ERROR;
	}

	pHdl->fd = -1;

	loc_size = sqlite3_column_bytes(pStmt, iCol);

	rc = get_full_path(db, pLoc, loc_size,&(pHdl->full_path));

	if (rc) {
		if (pHdl != NULL)
			sqlite3_free(pHdl);
		*ppBfile = NULL;
		return SQLITE_ERROR;
	}

	*ppBfile = (sqlite3_bfile *)pHdl;

	return SQLITE_OK;
}

/*
 * Open a BFILE
 */
SQLITE_API int sqlite3_bfile_open(
	sqlite3_bfile *pBfile)
{
	BfileHdl *pHdl;

	if (pBfile == NULL)
		return SQLITE_OK;


	pHdl = (BfileHdl *)pBfile;

	if (pHdl->fd == -1)
		pHdl->fd = open(pHdl->full_path, O_RDONLY);

	return SQLITE_OK;
}

/*
 * Check if file is open using this BFLIE
 */
SQLITE_API int sqlite3_bfile_close(
	sqlite3_bfile *pBfile)
{
	BfileHdl *pHdl;

	if (pBfile == NULL)
		return SQLITE_OK;

	pHdl = (BfileHdl *)pBfile;

	close(pHdl->fd);

	pHdl->fd = -1;

	return SQLITE_OK;
}

/*
 * Check if file is open using this BFLIE
 */
SQLITE_API int sqlite3_bfile_is_open(
	sqlite3_bfile *pBfile,
	int *open)
{
	BfileHdl *pHdl;
	int rc, exist;

	if (open == NULL)
		return SQLITE_ERROR;

	if (pBfile == NULL) {
		*open = 0;
		return SQLITE_OK;
	}

	rc = sqlite3_bfile_file_exists(pBfile, &exist);
	if (rc != SQLITE_OK || exist != 1) {
		*open = 0;
		return SQLITE_ERROR;
	}

	pHdl = (BfileHdl *)pBfile;

	*open = (pHdl->fd == -1) ? 0 : 1;

	return SQLITE_OK;
}

/*
 * Read a portion of a BFILE
 */
SQLITE_API int sqlite3_bfile_read(
	sqlite3_bfile *pBfile,
	void *z,
	int nSize,
	off_t iOffset,
	int *nRead)
{
	BfileHdl *pHdl;
	int rc;
	off_t size;

	if (z == NULL || nSize <= 0 || iOffset < 0 || nRead == NULL)
		return SQLITE_ERROR;
	
	rc = sqlite3_bfile_size(pBfile, &size);
	if (rc != SQLITE_OK || iOffset > size) {
		*nRead = 0;
		return SQLITE_ERROR;
	}
	
	if (pBfile == NULL) {
		*(char *)z = 0;
		*nRead = 0;
		return SQLITE_OK;
	}

	pHdl = (BfileHdl *)pBfile;

	if (pHdl->fd == -1) {
		*nRead = 0;
		return SQLITE_ERROR;
	}

	if ((rc = lseek(pHdl->fd, iOffset, SEEK_SET)) == -1) {
		*nRead = 0;
		return SQLITE_ERROR;
	}

	if ((*nRead = read(pHdl->fd, z, nSize)) == -1) {
		*nRead = 0;
		return SQLITE_ERROR;
	}

	return SQLITE_OK;
}

/*
 * Check if a file exists
 */
SQLITE_API int sqlite3_bfile_file_exists(
	sqlite3_bfile *pBfile,
	int *exist
)
{
	BfileHdl *pHdl;

	if (exist == NULL)
		return SQLITE_ERROR;

	if (pBfile == NULL) {
		*exist = 0;
		return SQLITE_OK;
	}

	pHdl = (BfileHdl *)pBfile;

	if ((pHdl->fd != -1) || (!access(pHdl->full_path, F_OK)))
		*exist = 1;
	else
		*exist = 0;
	
	return SQLITE_OK;
}

/*
 * Get File size of a BFILE
 */
SQLITE_API int sqlite3_bfile_size(
	sqlite3_bfile *pBfile,
	off_t *size)
{
	BfileHdl *pHdl;
	struct stat st;
	int rc;

	if (size == NULL)
		return SQLITE_ERROR;

	if (pBfile == NULL) {
		*size = 0;
		return SQLITE_OK;
	}

	pHdl = (BfileHdl *)pBfile;

	memset(&st, 0, sizeof(st));

	if ((rc = stat(pHdl->full_path, &st)) == -1) {
		*size = 0;
		return SQLITE_ERROR;
	}

	*size = st.st_size;
	
	return SQLITE_OK;
}

/*
 * Finalize a BFILE
 */
SQLITE_API int sqlite3_bfile_final(
	sqlite3_bfile *pBfile)
{
	BfileHdl *pHdl;

	pHdl = (BfileHdl *)pBfile;

	if (pHdl != NULL) {
		if (pHdl->full_path != NULL)
			sqlite3_free(pHdl->full_path);

		sqlite3_free(pHdl);
	}

	return SQLITE_OK;
}
#else
typedef struct BfileHdl BfileHdl;
struct BfileHdl {
	char *full_path;	/* full path of the BFILE */
	int fd;			/* file discriptor of the BFILE*/
	char *zBuf;		/* read buf */
	int zBufSiz;		/* read buf size */
};

/*
 * Open a BFILE
 */
static void BFileOpenFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	int rc;
	sqlite3 *db;		/* db handle */
	int loc_size;		/* BFile locater size */
	int fd;			/* file descriptor */
	char *pLoc;		/* BFile locater */
	char *full_path;	/* full path */
	BfileHdl *pHdl;		/* BFile handle */

	assert(context != NULL && argc == 1 && argv != NULL);
	full_path = NULL, pHdl = NULL;
	fd = -1;

	loc_size = sqlite3_value_bytes(argv[0]);
	if (loc_size <= strlen(BFILE_PREFIX)) {
		sqlite3_result_int(context, 0);
		return;
	}

	db = (sqlite3 *)sqlite3_user_data(context);
	pLoc = (char *)sqlite3_value_text(argv[0]);
	assert(db != NULL && pLoc != NULL);

	rc = get_full_path(db, pLoc, loc_size, &full_path);
	if (rc)
		goto err;

	fd = open(full_path, O_RDONLY);
	if (fd == -1) {
		if (errno == ENOENT)
		    sqlite3_result_int(context, 0);
		else
		    rc = -1;
		goto err;
	}

	pHdl = sqlite3_malloc(sizeof(BfileHdl));
	if (pHdl == NULL) {
		rc = SQLITE_NOMEM;
		goto err;
	}

	memset(pHdl, 0, sizeof(BfileHdl));
	pHdl->full_path = full_path;
	pHdl->fd = fd;
	sqlite3_result_int64(context, (sqlite3_int64)pHdl);
	return;

err:
	if (rc == SQLITE_NOMEM)
		sqlite3_result_error_nomem(context);
	else if (rc)
		sqlite3_result_error(context, "internal error", -1);

	if (pHdl != NULL)
		sqlite3_free(pHdl);

	if (fd >= 0)
		close(fd);

	if (full_path != NULL)
		sqlite3_free(full_path);
}

/*
 * Close a BFILE
 */
static void BFileCloseFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	BfileHdl *pHdl;

	assert(context != NULL && argc == 1 && argv != NULL);

	pHdl = (BfileHdl*)sqlite3_value_int64(argv[0]);
	if (pHdl != NULL) {
		if (pHdl->fd >= 0)
			close(pHdl->fd);
		if (pHdl->full_path != NULL)
			sqlite3_free(pHdl->full_path);
		if (pHdl->zBuf != NULL)
			sqlite3_free(pHdl->zBuf);
		sqlite3_free(pHdl);
	}
}

static int __bfile_get_size(
	char *full_path,
	off_t *size)
{
	struct stat st;
	int rc;

	assert(full_path != NULL && size != NULL);
	memset(&st, 0, sizeof(st));

	if ((rc = stat(full_path, &st)) == -1) {
		*size = 0;
		return SQLITE_ERROR;
	}

	*size = st.st_size;
	
	return SQLITE_OK;
}

/*
 * Read a portion of a BFILE
 */
static void BFileReadFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	int rc;
	BfileHdl *pHdl;		/* BFile handle */
	int nSize;		/* up to nSize bytes would be read */
	int nRead;		/* the number of bytes read actually */
	off_t size;		/* the size of the underlying file */
	off_t iOffset;		/* the position from which read would start */
	char *z;		/* the pointer of read buffer */

	assert(context != NULL && argc == 3 && argv != NULL);

	pHdl = (BfileHdl*)sqlite3_value_int64(argv[0]);
	if (pHdl == NULL || pHdl->full_path == NULL || pHdl->fd < 0) {
		sqlite3_result_error(context, "invalid bfile handle", -1);
		return;
	}

	nSize = sqlite3_value_int(argv[1]);
	iOffset = (off_t)sqlite3_value_int(argv[2]);

	rc = __bfile_get_size(pHdl->full_path, &size);
	if (rc != SQLITE_OK) {
		sqlite3_result_error(context, "internal error", -1);
		return;
	} else if (nSize <= 0) {
		sqlite3_result_error(context, "invalid size", -1);
		return;
	} else if (iOffset < 0) {
		sqlite3_result_error(context, "invalid offset", -1);
		return;
	} else if (iOffset > size) {
		sqlite3_result_null(context);
		return;
	}
	
	if ((rc = lseek(pHdl->fd, iOffset, SEEK_SET)) == -1) {
		sqlite3_result_error(context, "disk I/O error", -1);
		return;
	}

	if (pHdl->zBuf == NULL || pHdl->zBufSiz < nSize) {
		z = pHdl->zBuf == NULL ? sqlite3_malloc(nSize) :
			sqlite3_realloc(pHdl->zBuf, nSize);
		if (z == NULL) {
			sqlite3_result_error_nomem(context);
			return;
		}
		pHdl->zBuf = z;
		pHdl->zBufSiz = nSize;
	}

	if ((nRead = read(pHdl->fd, pHdl->zBuf, nSize)) == -1) {
		sqlite3_result_error(context, "disk I/O error", -1);
		return;
	}

	sqlite3_result_blob(context, pHdl->zBuf, nRead, SQLITE_STATIC);
}

/*
 * Get File size of a BFILE
 */
static void BFileSizeFunc(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv)
{
	int rc;
	sqlite3 *db;
	int loc_size;
	off_t size;
	char *pLoc, *full_path;

	assert(context != NULL && argc == 1 && argv != NULL);
	full_path = NULL;

	loc_size = sqlite3_value_bytes(argv[0]);
	if (loc_size <= strlen(BFILE_PREFIX)) {
		sqlite3_result_int(context, -1);
		return;
	}

	db = (sqlite3 *)sqlite3_user_data(context);
	pLoc = (char *)sqlite3_value_text(argv[0]);
	assert(db != NULL && pLoc != NULL);

	rc = get_full_path(db, pLoc, loc_size, &full_path);
	if (rc) {
		if (rc == SQLITE_NOMEM)
			sqlite3_result_error_nomem(context);
		else
			sqlite3_result_error(context, "internal error", -1);
		return;
	}

	/* check existence, if not exits at at set size as -1 */
	if (access(full_path, F_OK))
		sqlite3_result_int(context, -1);
	else if (__bfile_get_size(full_path, &size) == SQLITE_OK)
		sqlite3_result_int(context, size);
	else
		sqlite3_result_error(context, "internal error", -1);

	sqlite3_free(full_path);
}
#endif

int sqlite3_extension_init(
	sqlite3 *db,
	char **pzErrMsg,
	const sqlite3_api_routines *pApi)
{
	int rc;
	sqlite3_stmt *pSelect;

	SQLITE_EXTENSION_INIT2(pApi)

	sqlite3_create_function(db, "BFILE_NAME", 2, SQLITE_ANY, 0,
	    BFileNameFunc, 0, 0);
	
	sqlite3_create_function(db, "BFILE_FULLPATH", 1, SQLITE_ANY, db,
	    BFileFullPathFunc, 0, 0);
	
	sqlite3_create_function(db, "BFILE_CREATE_DIRECTORY", 2, SQLITE_ANY, 
		db, BFileCreateDirectoryFunc, 0, 0);
	
	sqlite3_create_function(db, "BFILE_REPLACE_DIRECTORY", 2, SQLITE_ANY, 
		db, BFileReplaceDirectoryFunc, 0, 0);
	
	sqlite3_create_function(db, "BFILE_DROP_DIRECTORY", 1, SQLITE_ANY, 
		db, BFileDropDirectoryFunc, 0, 0);	

#ifndef BFILE_USE_CAPIS
	/* Bfile functions instead of c version */
	sqlite3_create_function(db, "BFILE_OPEN", 1, SQLITE_ANY, db,
	    BFileOpenFunc, 0, 0);	

	sqlite3_create_function(db, "BFILE_CLOSE", 1, SQLITE_ANY, 0,
	    BFileCloseFunc, 0, 0);	

	sqlite3_create_function(db, "BFILE_READ", 3, SQLITE_ANY, 0,
	    BFileReadFunc, 0, 0);	

	sqlite3_create_function(db, "BFILE_SIZE", 1, SQLITE_ANY, db,
	    BFileSizeFunc, 0, 0);	
#endif

	rc = sqlite3_prepare(db, FIND_DIRECTORY, -1, &pSelect, 0);
	if (rc != SQLITE_OK || !pSelect)
		return rc;

	rc = sqlite3_step(pSelect);
	if (rc == SQLITE_ROW && sqlite3_column_int(pSelect, 0) == 0)
		rc = sqlite3_exec(db, CREATE_DIRECTORY, 0, 0, pzErrMsg);

	rc = sqlite3_finalize(pSelect);

	return rc;
}
