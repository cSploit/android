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

#define CHECK_ERR						\
	if (rc != SQLITE_OK) {					\
		fprintf(stderr, "  SQL error: %s\n", zErrMsg);  \
		sqlite3_free(zErrMsg);				\
		return -1;					\
	}

#define EXEC(sql)						\
	printf("\nRuning:\t%s\n",(sql));			\
	rc = sqlite3_exec(db, (sql), NULL, 0, &zErrMsg);	\
	CHECK_ERR

#define QUERY(sql)						\
	printf("\nRuning:\t%s\n",(sql));			\
	rc = sqlite3_prepare(db, (sql), -1, &pStmt, 0);		\
	if (rc != SQLITE_OK) {					\
		fprintf(stderr,"  SQL error: %d\n", rc);	\
		return -1;					\
	}							\
	printf("\tname\tphone   \taddress \tphoto_path\n");	\
	while (SQLITE_ROW == sqlite3_step(pStmt)) {		\
		const char *name =				\
		    (const char *)sqlite3_column_text(pStmt, 0);\
		const char *phone_num =				\
		    (const char *)sqlite3_column_text(pStmt, 1);\
		const char *address =				\
		    (const char *)sqlite3_column_text(pStmt, 2);\
		const char *photo_path =			\
		    (const char *)sqlite3_column_text(pStmt, 3);\
		printf("\t%s\t%s\t%s\t%s\n", name, phone_num,	\
		    address, photo_path);			\
	}

#define cleandb(db)							\
	sqlite3_exec(db,						\
		"DELETE FROM BFILE_DIRECTORY WHERE alias='IMG';", NULL, \
		0, &zErrMsg);						\
	sqlite3_exec(db, "DROP TABLE address_book;", NULL, 0, &zErrMsg);\

#define BUFF_SIZE 80
static int query_with_api(sqlite3* db, char * sql) {
	int rc, len, i;
	sqlite3_stmt *pStmt;
	sqlite3_bfile *pBfile;
	char *name, *phone_num, *address, buf[BUFF_SIZE];
	off_t offset;

	printf("\nRuning:\t%s with BFILE APIs\n",(sql));

	rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr,"  SQL error: %d\n", rc);
		return -1;
	}

	printf("\tname\tphone   \taddress \n");

	while (SQLITE_ROW == sqlite3_step(pStmt)) {
		name = (char *)sqlite3_column_text(pStmt, 0);
		phone_num = (char *)sqlite3_column_text(pStmt, 1);
		address = (char *)sqlite3_column_text(pStmt, 2);
		printf("\t%s\t%s\t%s\n", name, phone_num, address);

		/* Bfile APIs */
		sqlite3_column_bfile(pStmt, 3, &pBfile);

		rc = sqlite3_bfile_open(pBfile);
		if (rc != SQLITE_OK)
			return -1;

		offset = 0;
		do {
			rc = sqlite3_bfile_read(pBfile, buf, BUFF_SIZE, 
				offset, &len);
			if (rc != SQLITE_OK)
				return -1;

			for (i = 0; i < len; i++)
				putchar(buf[i]);

			offset += len;
		} while(len > 0);
		printf("\n");

		rc = sqlite3_bfile_close(pBfile);
		if (rc != SQLITE_OK)
			return -1;

		rc = sqlite3_bfile_final(pBfile);
		if (rc != SQLITE_OK)
			return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	sqlite3 *db;
	sqlite3_stmt *pStmt;
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open("example.db", &db);
	CHECK_ERR

	sqlite3_enable_load_extension(db, 1);

	rc = sqlite3_load_extension(db, "libbfile_ext.so", NULL, &zErrMsg);
	CHECK_ERR

	/* clean the database */
	cleandb(db);

	/* create directory alias */
	//EXEC("INSERT INTO BFILE_DIRECTORY VALUES('IMG','./temp');")
	EXEC("SELECT BFILE_CREATE_DIRECTORY('IMG', './temp');")

	/* create a table with BFILE type column */
	EXEC("CREATE TABLE address_book(name TEXT, phone_num TEXT, address TEXT, photo BFILE);")

	/* insert */
	EXEC("INSERT INTO address_book VALUES('Tony', '+861082798880', 'China Beijing', BFILE_NAME('IMG','Tony.jpg'));")

	/* insert */
	EXEC("INSERT INTO address_book VALUES('Rachel', '+861082798881', 'China Beijing', BFILE_NAME('IMG','Rachel.jpg'));")

	/* insert */
	EXEC("INSERT INTO address_book VALUES('Tracy', '+861082798882', 'China Beijing', BFILE_NAME('IMG','Tracy.jpg'));")

	/* query */
	QUERY("SELECT name, phone_num, address, BFILE_FULLPATH(photo) FROM address_book;")
	rc = query_with_api(db, "SELECT name, phone_num, address, photo FROM address_book;");
	if (rc) return -1;

	/* update */
	EXEC("UPDATE address_book SET photo = BFILE_NAME('IMG', 'Tony_new.jpg') where name = 'Tony';")

	/* query */
	QUERY("SELECT name, phone_num, address, BFILE_FULLPATH(photo) FROM address_book;")
	rc = query_with_api(db, "SELECT name, phone_num, address, photo FROM address_book;");
	if (rc) return -1;

	/* update alias */    
	//EXEC("UPDATE BFILE_DIRECTORY SET PATH = './photo' WHERE ALIAS = 'IMG';");
	EXEC("SELECT BFILE_REPLACE_DIRECTORY('IMG', './photo');")

	/* query */
	QUERY("SELECT name, phone_num, address, BFILE_FULLPATH(photo) FROM address_book;")
	rc = query_with_api(db, "SELECT name, phone_num, address, photo FROM address_book;");
	if (rc) return -1;

	sqlite3_close(db);
	return 0;
}
