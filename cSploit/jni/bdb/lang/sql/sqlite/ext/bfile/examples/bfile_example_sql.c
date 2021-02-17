/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010 Oracle.  All rights reserved.
 *
 */

/*
 * This file show a exmple of using bfile interface for OSL.
 */

#include <stdio.h>
#include <sqlite3.h>
#include <bfile.h>

typedef sqlite3_int64 bfile_handle;

int exec(sqlite3 *db, char *sql)
{
	int rc;
	printf("\nRuning:\t%s\n",(sql));
	rc = sqlite3_exec(db, (sql), NULL, 0, NULL);
	if (rc != SQLITE_OK) {
		printf("sqlite3_exec error\n");
		return -1;
	}

	return 0;
}

int query(sqlite3 *db, char *sql)
{
	int rc;
	sqlite3_stmt *pStmt;

	printf("\nRuning:\t%s\n",(sql));
	rc = sqlite3_prepare(db, (sql), -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		printf("sqlite3_prepare error: \n");
		return -1;
	}
	printf("\tid\tphoto_path\n");
	while (SQLITE_ROW == sqlite3_step(pStmt)) {
		int res_id = sqlite3_column_int(pStmt, 0);
		const char *res_photo =
			(const char *)sqlite3_column_text(pStmt, 1);
		printf("\t%d\t%s\n", res_id,
			res_photo != NULL ? res_photo : "NULL");
	}
	sqlite3_finalize(pStmt);

	return 0;
}

#define cleandb(db)								\
	sqlite3_exec(db, "DELETE FROM BFILE_DIRECTORY WHERE alias='IMG';",	\
		NULL, 0, NULL);							\
	sqlite3_exec(db, "DROP TABLE test_bfile;", NULL, 0, NULL);

#define is_error(rc) (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW)
#define cleanup_if_error(rc, pt) if (is_error(rc)) goto pt

static int query_bfile(sqlite3* db) {
	int rc, rc2;
	sqlite3_stmt *pStmt, *pBfileStmt;
	char *photo;
	off_t offset;
	int row, id, len, i;
	bfile_handle bHdl;
	const char *sql_bfile_open =
		"SELECT id, BFILE_OPEN(photo) FROM test_bfile;";
	const char *sql_bfile_read = "SELECT BFILE_READ(?, ?, ?);";
	const char *sql_bfile_close = "SELECT BFILE_CLOSE(?);";
	const int amount = 80;

	pStmt = NULL;
	printf("\nRUNNING: query id, photo\n");

	/* BFILE_OPEN */
	rc = sqlite3_prepare(db, sql_bfile_open, -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		printf("BFILE_OPEN: prepare error\n");
		return -1;
	}

	row = 0;
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt))) {
		printf("-------------row%d------------\n", ++row);

		id = sqlite3_column_int(pStmt, 0);
		bHdl = sqlite3_column_int64(pStmt, 1);

		printf("id:\n%d\nphoto:\n", id);

		if ((void*)bHdl == NULL) {
			printf("NULL\n");
			continue;
		}

		offset = 0;

		/* BFILE_READ */
		rc2 = sqlite3_prepare(db, sql_bfile_read, -1, &pBfileStmt, 0);
		cleanup_if_error(rc2, cleanup_read);

		while (1) {
			rc2 = sqlite3_bind_int64(pBfileStmt, 1, bHdl);
			cleanup_if_error(rc2, cleanup_read);

			rc2 = sqlite3_bind_int(pBfileStmt, 2, amount);
			cleanup_if_error(rc2, cleanup_read);

			rc2 = sqlite3_bind_int(pBfileStmt, 3, offset);
			cleanup_if_error(rc2, cleanup_read);

			rc2 = sqlite3_step(pBfileStmt);
			cleanup_if_error(rc2, cleanup_read);

			if (rc2 == SQLITE_ROW) {
				photo = 
					(char*)sqlite3_column_blob(pBfileStmt, 0);
				len = sqlite3_column_bytes(pBfileStmt, 0);

				/* EOF */
				if (len == 0)
					break;

				for(i=0; i<len; i++)
					putchar(photo[i]);
			}

			rc2 = sqlite3_reset(pBfileStmt);
			cleanup_if_error(rc2, cleanup_read);

			rc2 = sqlite3_clear_bindings(pBfileStmt);
			cleanup_if_error(rc2, cleanup_read);

			offset += amount;
		}

cleanup_read:		
		if (pBfileStmt != NULL) {
			sqlite3_finalize(pBfileStmt);
			pBfileStmt = NULL;
		}

		/* BFILE_CLOSE */
		rc2 = sqlite3_prepare(db, sql_bfile_close, -1, &pBfileStmt, 0);
		cleanup_if_error(rc2, cleanup_close);

		rc2 = sqlite3_bind_int64(pBfileStmt, 1, bHdl);
		cleanup_if_error(rc2, cleanup_close);

		rc2 = sqlite3_step(pBfileStmt);

cleanup_close:
		if (pBfileStmt != NULL) {
			sqlite3_finalize(pBfileStmt);
			pBfileStmt = NULL;
		}
	}

	if (pStmt != NULL) {
		sqlite3_finalize(pStmt);
		pStmt = NULL;
	}

	if (!is_error(rc) && is_error(rc2))
		rc = rc2;

	rc = !is_error(rc) ? 0 : -1;
	return rc;
}

int prepare_db(sqlite3 *db)
{
	/* create directory alias */
	exec(db, "SELECT BFILE_CREATE_DIRECTORY('IMG', './temp');");

	/* create table */
	exec(db, "CREATE TABLE test_bfile(id INT, photo BFILE);");

	/* insert */
	exec(db, "INSERT INTO test_bfile VALUES(1, BFILE_NAME('IMG','Tony.jpg'));");

	/* insert */
	exec(db, "INSERT INTO test_bfile VALUES(2, BFILE_NAME('IMG','Rachel.jpg'));");

	/* insert */
	exec(db, "INSERT INTO test_bfile VALUES(3, BFILE_NAME('IMG','Tracy.jpg'));");

	/* insert */
	exec(db, "INSERT INTO test_bfile VALUES(4, NULL);");

	return 0;
}

int change_dir(sqlite3 *db)
{
	/* query */
	query(db, "SELECT id, BFILE_FULLPATH(photo) FROM test_bfile;");

	/* update alias */
	exec(db, "SELECT BFILE_REPLACE_DIRECTORY('IMG', './photo');");

	/* query */
	query(db, "SELECT id, BFILE_FULLPATH(photo) FROM test_bfile;");

	return 0;
}

int main(int argc, char **argv) {
	sqlite3 *db;
	sqlite3_stmt *pStmt;
	int rc;

	rc = sqlite3_open("example.db", &db);
	if (rc != SQLITE_OK) {
		printf("db open error\n");
		return -1;
	}

	sqlite3_enable_load_extension(db, 1);

	rc = sqlite3_load_extension(db, "libbfile_ext.so", NULL, NULL);
	if (rc != SQLITE_OK) {
		printf("load bfile extension error\n");
		return -1;
	}

	/* clean the database */
	cleandb(db);

	/* create table & directory, and insert data */
	rc = prepare_db(db);
	if (rc) return -1;

	/* query bfile data */
	rc = query_bfile(db);
	if (rc) return -1;

	/* replace directory alias */
	rc = change_dir(db);
	if (rc) return -1;

	sqlite3_close(db);
	return 0;
}
