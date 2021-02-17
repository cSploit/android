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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sqlite3.h>
#include <bfile.h>

typedef sqlite3_int64 bfile_handle;

#define CHECK_ERR						\
	if (rc != SQLITE_OK) {					\
		printf("  SQL error: %s\n", zErrMsg);		\
		sqlite3_free(zErrMsg);				\
		_exit(-1);					\
	}

#define EXEC(sql)						\
	rc = sqlite3_exec(db, (sql), NULL, 0, &zErrMsg);	\
	CHECK_ERR

#define cleandb(db)										\
	sqlite3_exec(db, "DELETE FROM BFILE_DIRECTORY WHERE alias='IMG';", NULL, 0, &zErrMsg);	\
	sqlite3_exec(db, "DROP TABLE address_book;", NULL, 0, &zErrMsg);

static char pic[] = 
"       |||||       ""\n"\
"       (o o)       ""\n"\
"+-oooO--(_)-------+""\n"\
"|                 |""\n"\
"|    I am Tony    |""\n"\
"|                 |""\n"\
"+------------Ooo--+""\n"\
"      |__|__|      ""\n"\
"       || ||       ""\n"\
"      ooO Ooo      ""\n";

static void  creat_file(void) {
	int fd;
	char *zErrMsg = NULL;

	fd = creat("./Tony.jpg", S_IRWXU);

	if (fd != -1) {
		if (write(fd, pic, sizeof(pic) - 1 ) == -1) {
			perror(zErrMsg);
			fprintf(stderr, "%s\n", zErrMsg);
			_exit(-1);
		}
		close(fd);
	} else {
		perror(zErrMsg);
		fprintf(stderr, "%s\n", zErrMsg);
		_exit(-1);
	}

	printf("File ./Tony.jpg created.\n");
}

static void pre_test(sqlite3 **pdb) {
	char *zErrMsg = 0;
	int rc;
	sqlite3 *db;

	creat_file();

	rc = sqlite3_open("test.db", pdb);
	CHECK_ERR

	db = *pdb;

	sqlite3_enable_load_extension(db, 1);

	rc = sqlite3_load_extension(db, "libbfile_ext.so", NULL, &zErrMsg);
	CHECK_ERR

	/* clean the database */
	cleandb(db);

	/* create directory alias */
	//EXEC("INSERT INTO BFILE_DIRECTORY VALUES('IMG','./');")
	EXEC("SELECT BFILE_CREATE_DIRECTORY('IMG', './');")

	/* create a table with BFILE type column */
	EXEC("CREATE TABLE address_book(name TEXT, phone_num TEXT, address TEXT, photo BFILE);")

	/* insert an existed bfile */
	EXEC("INSERT INTO address_book VALUES('Tony', '+861082798880', 'China Beijing', BFILE_NAME('IMG','Tony.jpg'));")

	/* insert a not existed bfile */
	EXEC("INSERT INTO address_book VALUES('Rachel', '+861082798881', 'China Beijing', BFILE_NAME('IMG','Rachel.jpg'));")

	/* insert a null bfile */
	EXEC("INSERT INTO address_book VALUES('Jhon', '+861082798883', 'China Beijing', NULL);")
}

#define CHECK_API_ERR(title, errno, line)			\
	if (rc != (errno)) {					\
		printf(title" error @ line: %d\n", (line));	\
		_exit(-1);					\
	} else {						\
		printf(title" OK\n");				\
	}

#define BUFF_SIZE 20
static void test_BfileSize(sqlite3* db) {
	int rc, row, size;
	sqlite3_stmt *pStmt;
	char *sql = "SELECT BFILE_SIZE(photo) FROM address_book";

	/* get size of each bfile */
	printf("test BFILE_SIZE(icol)\n");

	pStmt = NULL;
	rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		printf("\t[Failed]\tBFILE_SIZE: prepare error\n");
		return;
	}

	row = 0;

	while (SQLITE_ROW == (rc = sqlite3_step(pStmt))) {
		row++;
		printf("-----------------row %d--------------------", row);

		size = sqlite3_column_int(pStmt, 0);
		switch (row) {
		case 1:	/* existed bfile */
			if (size != sizeof(pic) - 1)
				printf("\t[Failed]\tlogic error @ line: %d", __LINE__);
			else
				printf("\t[OK]\n");
			break;
		case 2: /* not existied bfile */
		case 3: /* null */
			if (size != -1)
				printf("\t[Failed]\tlogic error @ line: %d", __LINE__);
			else
				printf("\t[OK]\n");
			break;
		default:
			assert(0);
		}
	}

	if (pStmt != NULL)
		sqlite3_finalize(pStmt);

	if (rc && rc != SQLITE_DONE)
		printf("\t[Failed]\tBFILE_SIZE: step error\n");
}

static void test_BfileOpenReadClose(sqlite3* db) {
	int rc, rc2, rc3, len, row;
	bfile_handle bHdl;
	sqlite3_stmt *pStmt, *pBfileStmt;
	off_t amount, offset;
	char *sql[] =  {"SELECT BFILE_OPEN(photo) FROM address_book",
			"SELECT BFILE_READ(?, ?, ?)",
			"SELECT BFILE_CLOSE(?)" };
	const char *result;

	/* manipulate each bfile */
	printf("%s%s%s", "test BFILE_OPEN(icol)\n     ",
			"BFILE_READ(bHdl, amount, offset)\n     ",
			"BFILE_CLOSE(bHdl)\n");
	
	pStmt = NULL;
	/* BFILE_OPEN */
	rc = sqlite3_prepare(db, sql[0], -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		printf("\t[Failed]\tBFILE_OPEN: prepare error\n");
		return;
	}

	row = 0;

	pBfileStmt = NULL;
	while (SQLITE_ROW == (rc = sqlite3_step(pStmt))) {
		row++;
		printf("-----------------row %d--------------------", row);

		bHdl = sqlite3_column_int64(pStmt, 0);
		if ((void*)bHdl == NULL) {
			printf("\t[OK]\n");
			continue;
		}

		amount = BUFF_SIZE;
		offset = 0;

		/* BFILE_READ */
	        rc2 = sqlite3_prepare(db, sql[1], -1, &pBfileStmt, 0);
		if (rc2) {
			printf("\t[Failed]\tBFILE_READ: prepare error\n");
			continue;
		}

		while (1) {
			rc2 = sqlite3_bind_int64(pBfileStmt, 1, bHdl);
                	if (rc2) {
				printf("\t[Failed]\tBFILE_READ: bind error\n");
				break;
			}

			rc2 = sqlite3_bind_int(pBfileStmt, 2, amount);
                	if (rc2) {
				printf("\t[Failed]\tBFILE_READ: bind error\n");
				break;
			}
			
			rc2 = sqlite3_bind_int(pBfileStmt, 3, offset);
                	if (rc2) {
				printf("\t[Failed]\tBFILE_READ: bind error\n");
				break;
			}

			rc2 = sqlite3_step(pBfileStmt);
			if (rc2 == SQLITE_ROW) {
				result = (const char*)sqlite3_column_blob(pBfileStmt, 0);
				len = sqlite3_column_bytes(pBfileStmt, 0);
				/* EOF */
				if (len == 0)
					break;
				if (memcmp(result, &(pic[offset]), len)) {
					rc2 = -1;
					printf("\t[Failed]\tlogic error @ line: %d", __LINE__);
					break;
				}
			} else if (rc2) {
				printf("\t[Failed]\tBFILE_READ: step error\n");
				break;
			}

			sqlite3_reset(pBfileStmt);
			sqlite3_clear_bindings(pBfileStmt);

			offset += BUFF_SIZE;
		}

		if (pBfileStmt != NULL) {
			sqlite3_finalize(pBfileStmt);
			pBfileStmt = NULL;
		}

		rc3 = rc2;
		/* BFILE_CLOSE */
	        rc2 = sqlite3_prepare(db, sql[2], -1, &pBfileStmt, 0);
               	if (rc2) {
			printf("\t[Failed]\tBFILE_CLOSE: prepare error\n");
			continue;
		}

		rc2 = sqlite3_bind_int64(pBfileStmt, 1, bHdl);
               	if (rc2)
			printf("\t[Failed]\tBFILE_CLOSE: bind error\n");
		else {
			rc2 = sqlite3_step(pBfileStmt);
			if (rc2 && rc2 != SQLITE_ROW && rc2 != SQLITE_DONE)
				printf("\t[Failed]\tBFILE_CLOSE: step error\n");
		}

		if (pBfileStmt != NULL) {
			sqlite3_finalize(pBfileStmt);
			pBfileStmt = NULL;
		}

		if ((rc2 == SQLITE_OK || rc2 == SQLITE_ROW) &&
				(rc3 == SQLITE_OK || rc3 == SQLITE_ROW))
		{
			printf("\t[OK]\n");
		}
	}

	if (pStmt != NULL)
		sqlite3_finalize(pStmt);

	if (rc && rc != SQLITE_DONE)
		printf("\t[Failed]\tBFILE_OPEN: step error\n");
}

static int query_with_api(sqlite3* db) {
        test_BfileSize(db);
	printf("\n\n");

	test_BfileOpenReadClose(db);
	printf("\n\n");
	return 0;
}

static void test_api(sqlite3 *db) {
	int rc;

	/* query */
	rc = query_with_api(db);
	if (rc)
		_exit(-1);
}

int main(int argc, char **argv) {
	sqlite3 *db;
	
	pre_test(&db);

	test_api(db);

	sqlite3_close(db);

	return 0;
}
