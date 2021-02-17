/* datacopy - Program to move database table between servers
 * Copyright (C) 2004-2005  Bill Thompson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#include <sybfront.h>
#include <sybdb.h>

#include "replacements.h"

typedef struct
{
	char *user;
	char *pass;
	char *server;
	char *db;
	char *dbobject;
} OBJECTINFO;

typedef struct pd
{
	int batchsize;
	int packetsize;
	OBJECTINFO src, dest;
	char *owner;
	int tflag;
	int aflag;
	int cflag;
	int Sflag;
	int Dflag;
	int bflag;
	int pflag;
	int vflag;
} BCPPARAMDATA;

static void pusage(void);
static int process_parameters(int, char **, struct pd *);
static int login_to_databases(BCPPARAMDATA * pdata, DBPROCESS ** dbsrc, DBPROCESS ** dbdest);
static int create_target_table(char *sobjname, char *owner, char *dobjname, DBPROCESS * dbsrc, DBPROCESS * dbdest);
static int check_table_structures(char *sobjname, char *dobjname, DBPROCESS * dbsrc, DBPROCESS * dbdest);
static int transfer_data(BCPPARAMDATA params, DBPROCESS * dbsrc, DBPROCESS * dbdest);

static int err_handler(DBPROCESS *, int, int, int, char *, char *);
static int msg_handler(DBPROCESS *, DBINT, int, int, char *, char *, char *, int);

int tdsdump_open(const char *filename);

int
main(int argc, char **argv)
{
	BCPPARAMDATA params;

	DBPROCESS *dbsrc;
	DBPROCESS *dbtarget;

	setlocale(LC_ALL, "");

	memset(&params, '\0', sizeof(params));

	if (process_parameters(argc, argv, &params) == FALSE) {
		pusage();
		return 1;
	}

	if (login_to_databases(&params, &dbsrc, &dbtarget) == FALSE)
		return 1;

	if (params.cflag) {
		if (create_target_table(params.src.dbobject, params.owner, params.dest.dbobject, dbsrc, dbtarget) == FALSE) {
			printf("datacopy: could not create target table %s.%s . terminating\n", params.owner, params.dest.dbobject);
			dbclose(dbsrc);
			dbclose(dbtarget);
			return 1;
		}
	}

	if (check_table_structures(params.src.dbobject, params.dest.dbobject, dbsrc, dbtarget) == FALSE) {
		printf("datacopy: table structures do not match. terminating\n");
		dbclose(dbsrc);
		dbclose(dbtarget);
		return 1;
	}

	if (transfer_data(params, dbsrc, dbtarget) == FALSE) {
		printf("datacopy: table copy failed.\n");
		printf("           the data may have been partially copied into the target database \n");
		dbclose(dbsrc);
		dbclose(dbtarget);
		return 1;
	}


	dbclose(dbsrc);
	dbclose(dbtarget);

	return 0;
}

static char *
gets_alloc(void)
{
	char reply[256];
	char *p = NULL;

	if (fgets(reply, sizeof(reply), stdin) != NULL) {
		p = strchr(reply, '\n');
		if (p)
			*p = 0;
		p = strdup(reply);
	}
	memset(reply, 0, sizeof(reply));
	return p;
}

static int
process_objectinfo(OBJECTINFO *oi, char *arg, const char *prompt)
{
	char *tok;

	tok = strsep(&arg, "/");
	if (!tok)
		return FALSE;
	oi->server = strdup(tok);

	tok = strsep(&arg, "/");
	if (!tok)
		return FALSE;
	oi->user = strdup(tok);

	tok = strsep(&arg, "/");
	if (!tok)
		return FALSE;
	if (strcmp(tok,"-") == 0) {
		printf("%s", prompt);
		oi->pass = gets_alloc();
	} else {
		oi->pass = strdup(tok);
		memset(tok, '*', strlen(tok));
	}

	tok = strsep(&arg, "/");
	if (!tok)
		return FALSE;
	oi->db = strdup(tok);

	tok = strsep(&arg, "/");
	if (!tok)
		return FALSE;
	oi->dbobject = strdup(tok);

	return TRUE;
}

static int
process_parameters(int argc, char **argv, BCPPARAMDATA * pdata)
{
	int opt;

	/* set some defaults */

	pdata->batchsize = 1000;

	/* get the rest of the arguments */

	while ((opt = getopt(argc, argv, "b:p:tacdS:D:v")) != -1) {
		switch (opt) {
		case 'b':
			pdata->bflag++;
			pdata->batchsize = atoi(optarg);
			break;
		case 'p':
			pdata->pflag++;
			pdata->packetsize = atoi(optarg);
			break;
		case 't':
			pdata->tflag++;
			break;
		case 'a':
			pdata->aflag++;
			break;
		case 'c':
			pdata->cflag++;
			if (optarg[0] == '-') {
				fprintf(stderr, "Invalid owner specified.\n");
				return FALSE;
			}
			pdata->owner = strdup(optarg);
			break;
		case 'd':
			tdsdump_open(NULL);
			break;
		case 'S':
			pdata->Sflag++;
			if (process_objectinfo(&pdata->src, optarg, "Enter Source Password: ") == FALSE)
				return FALSE;
			break;
		case 'D':
			pdata->Dflag++;
			if (process_objectinfo(&pdata->dest, optarg, "Enter Destination Password: ") == FALSE)
				return FALSE;
			break;
		case 'v':
			pdata->vflag++;
			break;
		default:
			return FALSE;
		}
	}
	/* one of these must be specified */

	if ((pdata->tflag + pdata->aflag + pdata->cflag) != 1) {
		fprintf(stderr, "one (and only one) of -t, -a or -c must be specified\n");
		return FALSE;
	}

	if (!pdata->Sflag) {

		printf("\nNo [-S]ource information supplied.\n\n");
		printf("Enter Server   : ");
		pdata->src.server = gets_alloc();

		printf("Enter Login    : ");
		pdata->src.user = gets_alloc();

		printf("Enter Password : ");
		pdata->src.pass = gets_alloc();

		printf("Enter Database : ");
		pdata->src.db = gets_alloc();

		printf("Enter Table    : ");
		pdata->src.dbobject = gets_alloc();
	}

	if (!pdata->Dflag) {

		printf("\nNo [-D]estination information supplied.\n\n");
		printf("Enter Server   : ");
		pdata->dest.server = gets_alloc();

		printf("Enter Login    : ");
		pdata->dest.user = gets_alloc();

		printf("Enter Password : ");
		pdata->dest.pass = gets_alloc();

		printf("Enter Database : ");
		pdata->dest.db = gets_alloc();

		printf("Enter Table    : ");
		pdata->dest.dbobject = gets_alloc();
	}

	return TRUE;

}

static int
login_to_databases(BCPPARAMDATA * pdata, DBPROCESS ** dbsrc, DBPROCESS ** dbdest)
{
	LOGINREC *slogin;
	LOGINREC *dlogin;

	/* Initialize DB-Library. */

	if (dbinit() == FAIL)
		return FALSE;

	/*
	 * Install the user-supplied error-handling and message-handling
	 * routines. They are defined at the bottom of this source file.
	 */

	dberrhandle(err_handler);
	dbmsghandle(msg_handler);

	/*
	 * Allocate and initialize the LOGINREC structure to be used
	 * to open a connection to SQL Server.
	 */

	slogin = dblogin();

	if (pdata->src.user)
		DBSETLUSER(slogin, pdata->src.user);
	if (pdata->src.pass)
		DBSETLPWD(slogin, pdata->src.pass);
	DBSETLAPP(slogin, "Migrate Data");

	/* if packet size specified, set in login record */

	if (pdata->pflag && pdata->packetsize > 0) {
		DBSETLPACKET(slogin, pdata->packetsize);
	}

	/*
	 * Get a connection to the database.
	 */

	if ((*dbsrc = dbopen(slogin, pdata->src.server)) == (DBPROCESS *) NULL) {
		fprintf(stderr, "Can't connect to source server.\n");
		return FALSE;
	}

	if (dbuse(*dbsrc, pdata->src.db) == FAIL) {
		fprintf(stderr, "Can't change database to %s .\n", pdata->src.db);
		return FALSE;
	}

	dlogin = dblogin();

	if (pdata->dest.user)
		DBSETLUSER(dlogin, pdata->dest.user);
	if (pdata->dest.pass)
		DBSETLPWD(dlogin, pdata->dest.pass);
	DBSETLAPP(dlogin, "Migrate Data");

	/* Enable bulk copy for this connection. */

	BCP_SETL(dlogin, TRUE);

	/* if packet size specified, set in login record */

	if (pdata->pflag && pdata->packetsize > 0) {
		DBSETLPACKET(dlogin, pdata->packetsize);
	}

	/*
	 * Get a connection to the database.
	 */

	if ((*dbdest = dbopen(dlogin, pdata->dest.server)) == (DBPROCESS *) NULL) {
		fprintf(stderr, "Can't connect to destination server.\n");
		return FALSE;
	}

	if (dbuse(*dbdest, pdata->dest.db) == FAIL) {
		fprintf(stderr, "Can't change database to %s .\n", pdata->dest.db);
		return FALSE;
	}

	return TRUE;

}

static int
create_target_table(char *sobjname, char *owner, char *dobjname, DBPROCESS * dbsrc, DBPROCESS * dbdest)
{
	char ls_command[2048];
	int i;

	DBINT num_cols;
	DBCHAR column_type[33];
	DBCOL colinfo;


	sprintf(ls_command, "SET FMTONLY ON select * from %s SET FMTONLY OFF", sobjname);

	if (dbcmd(dbsrc, ls_command) == FAIL) {
		printf("dbcmd failed\n");
		return FALSE;
	}

	if (dbsqlexec(dbsrc) == FAIL) {
		printf("table %s not found on SOURCE\n", sobjname);
		return FALSE;
	}

	while (NO_MORE_RESULTS != dbresults(dbsrc));

	sprintf(ls_command, "CREATE TABLE %s.%s ", owner, dobjname);

	num_cols = dbnumcols(dbsrc);

	for (i = 1; i <= num_cols; i++) {

		if (dbtablecolinfo(dbsrc, i, &colinfo) != SUCCEED)
			return FALSE;

		if (i == 1)
			strcat(ls_command, "( ");
		else
			strcat(ls_command, ", ");

		strcat(ls_command, colinfo.Name);
		strcat(ls_command, " ");

		switch (colinfo.Type) {
		case SYBINT1:
			strcat(ls_command, "tinyint");
			break;
		case SYBBIT:
			strcat(ls_command, "bit");
			break;
		case SYBINT2:
			strcat(ls_command, "smallint");
			break;
		case SYBINT4:
			strcat(ls_command, "int");
			break;
		case SYBDATETIME:
			strcat(ls_command, "datetime");
			break;
		case SYBDATETIME4:
			strcat(ls_command, "smalldatetime");
			break;
		case SYBREAL:
			strcat(ls_command, "real");
			break;
		case SYBMONEY:
			strcat(ls_command, "money");
			break;
		case SYBMONEY4:
			strcat(ls_command, "smallmoney");
			break;
		case SYBFLT8:
			strcat(ls_command, "float");
			break;
		case SYBDECIMAL:
			sprintf(column_type, "decimal(%d,%d)", colinfo.Precision, colinfo.Scale);
			strcat(ls_command, column_type);
			break;
		case SYBNUMERIC:
			sprintf(column_type, "numeric(%d,%d)", colinfo.Precision, colinfo.Scale);
			strcat(ls_command, column_type);
			break;
		case SYBCHAR:
			sprintf(column_type, "char(%d)", colinfo.MaxLength);
			strcat(ls_command, column_type);
			break;
		case SYBVARCHAR:
			sprintf(column_type, "varchar(%d)", colinfo.MaxLength);
			strcat(ls_command, column_type);
			break;
		case SYBTEXT:
			strcat(ls_command, "text");
			break;
		case SYBIMAGE:
			strcat(ls_command, "image");
			break;
		}

		if (colinfo.Null == TRUE) {
			strcat(ls_command, " NULL");
		} else {
			strcat(ls_command, " NOT NULL");
		}
	}
	strcat(ls_command, " ) ");

	if (dbcmd(dbdest, ls_command) == FAIL) {
		printf("dbcmd failed\n");
		return FALSE;
	}

	if (dbsqlexec(dbdest) == FAIL) {
		printf("create table on DESTINATION failed\n");
		return FALSE;
	}

	while (NO_MORE_RESULTS != dbresults(dbdest));

	return TRUE;
}

static int
check_table_structures(char *sobjname, char *dobjname, DBPROCESS * dbsrc, DBPROCESS * dbdest)
{
	char ls_command[256];
	int i;

	DBINT src_numcols = 0;
	DBINT dest_numcols = 0;

	DBINT src_coltype, dest_coltype;
	DBINT src_collen, dest_collen;


	sprintf(ls_command, "SET FMTONLY ON select * from %s SET FMTONLY OFF", sobjname);

	if (dbcmd(dbsrc, ls_command) == FAIL) {
		printf("dbcmd failed\n");
		return FALSE;
	}

	if (dbsqlexec(dbsrc) == FAIL) {
		printf("table %s not found on SOURCE\n", sobjname);
		return FALSE;
	}

	while (NO_MORE_RESULTS != dbresults(dbsrc));
	{

		if (0 == (src_numcols = dbnumcols(dbsrc))) {
			printf("Error in dbnumcols\n");
			return FALSE;
		}
	}

	sprintf(ls_command, "SET FMTONLY ON select * from %s SET FMTONLY OFF", dobjname);

	if (dbcmd(dbdest, ls_command) == FAIL) {
		printf("dbcmd failed\n");
		return FALSE;
	}

	if (dbsqlexec(dbdest) == FAIL) {
		printf("table %s not found on DEST\n", sobjname);
		return FALSE;
	}

	while (NO_MORE_RESULTS != dbresults(dbdest));
	{

		if (0 == (dest_numcols = dbnumcols(dbdest))) {
			printf("Error in dbnumcols\n");
			return FALSE;
		}
	}

	if (src_numcols != dest_numcols) {
		printf("number of columns do not match. source : %d , dest: %d\n", src_numcols, dest_numcols);
		return FALSE;
	}

	for (i = 1; i <= src_numcols; i++) {

		src_coltype = dbcoltype(dbsrc, i);
		src_collen = dbcollen(dbsrc, i);
		dest_coltype = dbcoltype(dbdest, i);
		dest_collen = dbcollen(dbdest, i);

		if ((src_coltype == SYBNUMERIC && dest_coltype == SYBNUMERIC) ||
		    (src_coltype == SYBDECIMAL && dest_coltype == SYBDECIMAL)
			) {
			continue;
		}

		if (src_coltype != dest_coltype || src_collen != dest_collen) {
			printf("COLUMN TYPE MISMATCH: column %d\n", i);
			printf("source: type %d, length %d\n", src_coltype, src_collen);
			printf("dest  : type %d, length %d\n", dest_coltype, dest_collen);
			return FALSE;
		}
	}
	return TRUE;
}

static int
transfer_data(BCPPARAMDATA params, DBPROCESS * dbsrc, DBPROCESS * dbdest)
{
	char ls_command[256];
	int col;

	DBINT src_numcols = 0;

	DBINT src_datlen;

	typedef struct migcoldata
	{
		DBINT coltype;
		DBINT collen;
		DBINT nullind;
		DBCHAR *data;
	} MIGCOLDATA;

	MIGCOLDATA **srcdata;

	DBINT rows_read = 0;
	DBINT rows_sent = 0;
	DBINT rows_done = 0;
	DBINT ret;

	struct timeval start_time;
	struct timeval end_time;
	double elapsed_time;

	if (params.vflag) {
		printf("\nStarting copy...\n");
	}

	if (params.tflag) {

		sprintf(ls_command, "truncate table %s", params.dest.dbobject);

		if (dbcmd(dbdest, ls_command) == FAIL) {
			printf("dbcmd failed\n");
			return FALSE;
		}

		if (dbsqlexec(dbdest) == FAIL) {
			printf("dbsqlexec failed\n");
			return FALSE;
		}

		if (dbresults(dbdest) == FAIL) {
			printf("Error in dbresults\n");
			return FALSE;
		}
	}


	sprintf(ls_command, "select * from %s", params.src.dbobject);

	if (dbcmd(dbsrc, ls_command) == FAIL) {
		printf("dbcmd failed\n");
		return FALSE;
	}

	if (dbsqlexec(dbsrc) == FAIL) {
		printf("dbsqlexec failed\n");
		return FALSE;
	}

	if (NO_MORE_RESULTS != dbresults(dbsrc));
	{
		if (0 == (src_numcols = dbnumcols(dbsrc))) {
			printf("Error in dbnumcols\n");
			return FALSE;
		}
	}



	if (bcp_init(dbdest, params.dest.dbobject, (char *) NULL, (char *) NULL, DB_IN) == FAIL) {
		printf("Error in bcp_init\n");
		return FALSE;
	}

	srcdata = (MIGCOLDATA **) malloc(sizeof(MIGCOLDATA *) * src_numcols);

	for (col = 0; col < src_numcols; col++) {

		srcdata[col] = (MIGCOLDATA *) calloc(1, sizeof(MIGCOLDATA));
		srcdata[col]->coltype = dbcoltype(dbsrc, col + 1);
		srcdata[col]->collen = dbcollen(dbsrc, col + 1);

		switch (srcdata[col]->coltype) {
		case SYBBIT:
			srcdata[col]->data = malloc(sizeof(DBBIT));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, BITBIND, sizeof(DBBIT), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBBIT, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;
		case SYBINT1:
			srcdata[col]->data = malloc(sizeof(DBTINYINT));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, TINYBIND, sizeof(DBTINYINT), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBINT1, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBINT2:
			srcdata[col]->data = malloc(sizeof(DBSMALLINT));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, SMALLBIND, sizeof(DBSMALLINT), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBINT2, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBINT4:
			srcdata[col]->data = malloc(sizeof(DBINT));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, INTBIND, sizeof(DBINT), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBINT4, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBFLT8:
			srcdata[col]->data = malloc(sizeof(DBFLT8));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, FLT8BIND, sizeof(DBFLT8), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBFLT8, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBREAL:
			srcdata[col]->data = malloc(sizeof(DBREAL));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, REALBIND, sizeof(DBREAL), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBREAL, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBMONEY:
			srcdata[col]->data = malloc(sizeof(DBMONEY));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, MONEYBIND, sizeof(DBMONEY), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBMONEY, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBMONEY4:
			srcdata[col]->data = malloc(sizeof(DBMONEY4));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, SMALLMONEYBIND, sizeof(DBMONEY4), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBMONEY4, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBDATETIME:
			srcdata[col]->data = malloc(sizeof(DBDATETIME));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, DATETIMEBIND, sizeof(DBDATETIME), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBDATETIME, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBDATETIME4:
			srcdata[col]->data = malloc(sizeof(DBDATETIME4));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, SMALLDATETIMEBIND, sizeof(DBDATETIME), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBDATETIME4, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBNUMERIC:
			srcdata[col]->data = malloc(sizeof(DBNUMERIC));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, NUMERICBIND, sizeof(DBNUMERIC), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, sizeof(DBNUMERIC), NULL, 0, SYBNUMERIC, col + 1) ==
			    FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;

		case SYBDECIMAL:
			srcdata[col]->data = malloc(sizeof(DBDECIMAL));
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, DECIMALBIND, sizeof(DBDECIMAL), (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, sizeof(DBDECIMAL), NULL, 0, SYBDECIMAL, col + 1) ==
			    FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;
		case SYBTEXT:
		case SYBCHAR:
			srcdata[col]->data = malloc(srcdata[col]->collen + 1);
			if (srcdata[col]->data == (char *) NULL) {
				printf("allocation error\n");
				return FALSE;
			}
			dbbind(dbsrc, col + 1, NTBSTRINGBIND, srcdata[col]->collen + 1, (BYTE *) srcdata[col]->data);
			dbnullbind(dbsrc, col + 1, &(srcdata[col]->nullind));
			if (bcp_bind(dbdest, (BYTE *) srcdata[col]->data, 0, -1, NULL, 0, SYBCHAR, col + 1) == FAIL) {
				printf("bcp_bind error\n");
				return FALSE;
			}
			break;
		}
	}

	gettimeofday(&start_time, 0);

	while (dbnextrow(dbsrc) != NO_MORE_ROWS) {
		rows_read++;
		for (col = 0; col < src_numcols; col++) {
			switch (srcdata[col]->coltype) {
			case SYBBIT:
			case SYBINT1:
			case SYBINT2:
			case SYBINT4:
			case SYBFLT8:
			case SYBREAL:
			case SYBDATETIME:
			case SYBDATETIME4:
			case SYBMONEY:
			case SYBMONEY4:
				if (srcdata[col]->nullind == -1) {	/* NULL data retrieved from source */
					bcp_collen(dbdest, 0, col + 1);
				} else {
					bcp_collen(dbdest, -1, col + 1);
				}
				break;
			case SYBNUMERIC:
				if (srcdata[col]->nullind == -1) {	/* NULL data retrieved from source */
					bcp_collen(dbdest, 0, col + 1);
				} else {
					bcp_collen(dbdest, sizeof(DBNUMERIC), col + 1);
				}
				break;
			case SYBDECIMAL:
				if (srcdata[col]->nullind == -1) {	/* NULL data retrieved from source */
					bcp_collen(dbdest, 0, col + 1);
				} else {
					bcp_collen(dbdest, sizeof(DBDECIMAL), col + 1);
				}
				break;
			case SYBTEXT:
			case SYBCHAR:
				if (srcdata[col]->nullind == -1) {	/* NULL data retrieved from source */
					bcp_collen(dbdest, 0, col + 1);
				} else {

					/*
					 * if there is zero length data, then the
					 * input data MUST have been all blanks,
					 * trimmed down to nothing by the bind
					 * type of NTBSTRINGBIND.
					 * so find out the source data length and
					 * re-set the data accordingly...
					 */

					if (strlen(srcdata[col]->data) == 0) {
						src_datlen = dbdatlen(dbsrc, col + 1);
						memset(srcdata[col]->data, ' ', src_datlen);
						srcdata[col]->data[src_datlen] = '\0';
					}
					bcp_collen(dbdest, strlen(srcdata[col]->data), col + 1);
				}
				break;
			}
		}
		if (bcp_sendrow(dbdest) == FAIL) {
			fprintf(stderr, "bcp_sendrow failed.  \n");
			return FALSE;
		} else {
			rows_sent++;
			if (rows_sent == params.batchsize) {
				ret = bcp_batch(dbdest);
				if (ret == -1) {
					printf("bcp_batch error\n");
					return FALSE;
				} else {
					rows_done += ret;
					printf("%d rows successfully copied (total %d)\n", ret, rows_done);
					rows_sent = 0;
				}
			}
		}
	}

	if (rows_read) {
		ret = bcp_done(dbdest);
		if (ret == -1) {
			fprintf(stderr, "bcp_done failed.  \n");
			return FALSE;
		} else {
			rows_done += ret;
		}
	}

	gettimeofday(&end_time, 0);


	elapsed_time = (double) (end_time.tv_sec - start_time.tv_sec) +
		((double) (end_time.tv_usec - start_time.tv_usec) / 1000000.00);

	if (params.vflag) {
		printf("\n");
		printf("rows read            : %d\n", rows_read);
		printf("rows written         : %d\n", rows_done);
		printf("elapsed time (secs)  : %f\n", elapsed_time);
		printf("rows per second      : %f\n", rows_done / elapsed_time);
	}

	return TRUE;


}

static void
pusage(void)
{
	fprintf(stderr, "usage: datacopy [-t | -a | -c owner] [-b batchsize] [-p packetsize] [-v] [-d]\n");
	fprintf(stderr, "       [-S server/username/password/database/table]\n");
	fprintf(stderr, "       [-D server/username/password/database/table]\n");
	fprintf(stderr, "       -t : truncate target table before loading data\n");
	fprintf(stderr, "       -a : append data to target table\n");
	fprintf(stderr, "       -c : create table owner.table before loading data\n");
	fprintf(stderr, "       -b : alter the number of records in each bcp batch\n");
	fprintf(stderr, "       (larger batch size = faster)\n");
	fprintf(stderr, "       -p : alter the default TDS packet size from the default\n");
	fprintf(stderr, "       (larger packet size = faster)\n");
	fprintf(stderr, "       -v : produce verbose output (timings etc.)\n");
	fprintf(stderr, "       -d : produce TDS DUMP log (serious debug only!)\n");
}

static int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	if (dberr) {
		fprintf(stderr, "Msg %d, Level %d\n", dberr, severity);
		fprintf(stderr, "%s\n\n", dberrstr);
	} else {
		fprintf(stderr, "DB-LIBRARY error:\n\t");
		fprintf(stderr, "%s\n", dberrstr);
	}

	return INT_CANCEL;
}

static int
msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	/*
	 * If it's a database change message, we'll ignore it.
	 * Also ignore language change message.
	 */
	if (msgno == 5701 || msgno == 5703)
		return 0;

	printf("Msg %ld, Level %d, State %d\n", (long int) msgno, severity, msgstate);

	if (strlen(srvname) > 0)
		printf("Server '%s', ", srvname);
	if (strlen(procname) > 0)
		printf("Procedure '%s', ", procname);
	if (line > 0)
		printf("Line %d", line);

	printf("\n\t%s\n", msgtext);

	return 0;
}
