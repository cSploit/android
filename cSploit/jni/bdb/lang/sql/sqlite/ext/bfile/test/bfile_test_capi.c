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
#include <sqlite3.h>
#include <bfile.h>

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
static int query_with_api(sqlite3* db, char * sql) {
	int rc, len, exist, row, isopen;
	sqlite3_stmt *pStmt;
	sqlite3_bfile *pBfile;
	char buf[BUFF_SIZE];
	off_t offset, fsize;

	rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr,"  SQL error: %d\n", rc);
		_exit(-1);
	}

	row = 0;

	while (SQLITE_ROW == sqlite3_step(pStmt)) {
		row++;
		printf("-----------------row %d--------------------\n", row);

		/* Bfile APIs */
		printf("test sqlite3_column_bfile:\n");
		
		rc = sqlite3_column_bfile(NULL, 0, &pBfile);
		CHECK_API_ERR("  test null pStmt", SQLITE_ERROR, __LINE__)

		rc = sqlite3_column_bfile(pStmt, 0, NULL);
		CHECK_API_ERR("  test null ppBfile", SQLITE_ERROR, __LINE__)

		rc = sqlite3_column_bfile(pStmt, 1, &pBfile);
		CHECK_API_ERR("  test wrong iCol", SQLITE_ERROR, __LINE__)

		rc = sqlite3_column_bfile(pStmt, 0, &pBfile);
		switch (row) {
		case 1:	/* existed bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (pBfile == NULL)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 2: /* not existied bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (pBfile == NULL)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 3: /* null */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (pBfile != NULL)
				printf("    logic error @ line: %d", __LINE__);
			break;
		default:
			break;
		}

		
		printf("test sqlite3_bfile_exists:\n");
		
		rc = sqlite3_bfile_file_exists(pBfile, NULL);
		CHECK_API_ERR("  test null exist", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_file_exists(pBfile, &exist);
		switch (row) {
		case 1:	/* existed bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (exist != 1)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 2: /* not existied bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (exist != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 3: /* null */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (exist != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		default:
			break;
		}

		printf("test sqlite3_bfile_open:\n");

		rc = sqlite3_bfile_open(pBfile);
		CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)

		
		printf("test sqlite3_bfile_is_open:\n");

		rc = sqlite3_bfile_is_open(pBfile, NULL);
		CHECK_API_ERR("  test null isopen", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_is_open(pBfile, &isopen);
		switch (row) {
		case 1:	/* existed bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (isopen != 1)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 2: /* not existied bfile */
			CHECK_API_ERR("  test normal use", SQLITE_ERROR, __LINE__)
			if (isopen != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 3: /* null */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (isopen != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		default:
			break;
		}


		printf("test sqlite3_bfile_size:\n");

		rc = sqlite3_bfile_size(pBfile, NULL);
		CHECK_API_ERR("  test null size", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_size(pBfile, &fsize);
		switch (row) {
		case 1:	/* existed bfile */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (fsize != sizeof(pic) - 1)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 2: /* not existied bfile */
			CHECK_API_ERR("  test normal use", SQLITE_ERROR, __LINE__)
			if (fsize != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		case 3: /* null */
			CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
			if (fsize != 0)
				printf("    logic error @ line: %d", __LINE__);
			break;
		default:
			break;
		}

		offset = 0;

		printf("test sqlite3_bfile_read:\n");

		rc = sqlite3_bfile_read(pBfile, NULL, BUFF_SIZE, offset, &len);
		CHECK_API_ERR("  test null buf", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_read(pBfile, buf, 0, offset, &len);
		CHECK_API_ERR("  test 0 buff size", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_read(pBfile, buf, BUFF_SIZE, -1, &len);
		CHECK_API_ERR("  test bad offset", SQLITE_ERROR, __LINE__)

		rc = sqlite3_bfile_read(pBfile, buf, BUFF_SIZE, 10000, &len);
		CHECK_API_ERR("  test bad offset2", SQLITE_ERROR, __LINE__)
		if (len != 0)
			printf("    logic error @ line: %d", __LINE__);

		rc = sqlite3_bfile_read(pBfile, buf, BUFF_SIZE, offset, NULL);
		CHECK_API_ERR("  test null readed", SQLITE_ERROR, __LINE__)

		do {
			rc = sqlite3_bfile_read(pBfile, buf, BUFF_SIZE, offset, &len);
			switch (row) {
			case 1:	/* existed bfile */
				CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
				if (memcmp(buf, &(pic[offset]), len))
					printf("    logic error @ line: %d", __LINE__);
				break;
			case 2: /* not existied bfile */
				CHECK_API_ERR("  test normal use", SQLITE_ERROR, __LINE__)
				if (len != 0)
					printf("    logic error @ line: %d", __LINE__);
				break;
			case 3: /* null */
				CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
				if (len != 0)
					printf("    logic error @ line: %d", __LINE__);
				break;
			default:
				break;
			}

			offset += len;
		}while(len > 0);
		printf("\n");


		printf("test sqlite3_bfile_close:\n");

		rc = sqlite3_bfile_close(pBfile);
		CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
		

		printf("test sqlite3_bfile_final:\n");
		
		rc = sqlite3_bfile_final(pBfile);
		CHECK_API_ERR("  test normal use", SQLITE_OK, __LINE__)
	}

	return 0;
}

static void test_api(sqlite3 *db) {
	int rc;

	/* query */
	rc = query_with_api(db, "SELECT photo FROM address_book;");
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
