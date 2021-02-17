/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2011  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
#include <ctype.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

/* These should be in stdlib */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#include <freetds/tds.h>
#include "replacements.h"
#include <sybfront.h>
#include <sybdb.h>
#include "freebcp.h"

static char software_version[] = "$Id: freebcp.c,v 1.64 2011-10-14 22:49:34 berryc Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

void pusage(void);
int process_parameters(int, char **, struct pd *);
static int unescape(char arg[]);
int login_to_database(struct pd *, DBPROCESS **);

int file_character(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir);
int file_native(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir);
int file_formatted(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir);
int setoptions (DBPROCESS * dbproc, BCPPARAMDATA * params);

int err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
int msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname,
		int line);
static int set_bcp_hints(BCPPARAMDATA *pdata, DBPROCESS *pdbproc);

int
main(int argc, char **argv)
{
	BCPPARAMDATA params;
	DBPROCESS *dbproc;
	int ok = FALSE;

	setlocale(LC_ALL, "");

#ifdef __VMS
        /* Convert VMS-style arguments to Unix-style */
        parse_vms_args(&argc, &argv);
#endif

	memset(&params, '\0', sizeof(params));

	params.textsize = 4096;	/* our default text size is 4K */

	if (process_parameters(argc, argv, &params) == FALSE) {
		exit(EXIT_FAILURE);
	}
	if (getenv("FREEBCP")) {
		fprintf(stderr, "User name: \"%s\"\n", params.user);
	}


	if (login_to_database(&params, &dbproc) == FALSE) {
		exit(EXIT_FAILURE);
	}

	if (!setoptions(dbproc, &params)) 
		return FALSE;

	if (params.cflag) {	/* character format file */
		ok = file_character(&params, dbproc, params.direction);
	} else if (params.nflag) {	/* native format file    */
		ok = file_native(&params, dbproc, params.direction);
	} else if (params.fflag) {	/* formatted file        */
		ok = file_formatted(&params, dbproc, params.direction);
	} else {
		ok = FALSE;
	}

	exit((ok == TRUE) ? EXIT_SUCCESS : EXIT_FAILURE);

	return 0;
}


static int unescape(char arg[])
{
	char *p = arg, *next;
	char escaped;
	while ((next = strchr(p, '\\')) != NULL) {

		p = next;

		switch (p[1]) {
		case '0':
			escaped = '\0';
			break;
		case 't':
			escaped = '\t';
			break;
		case 'r':
			escaped = '\r';
			break;
		case 'n':
			escaped = '\n';
			break;
		case '\\':
			escaped = '\\';
			break;
		default:
			++p;
			continue;
		}

		/* Overwrite the backslash with the intended character, and shift everything down one */
		*p++ = escaped;
		memmove(p, p+1, 1 + strlen(p+1));
	}
	return strchr(p, 0) - arg;
}

int
process_parameters(int argc, char **argv, BCPPARAMDATA *pdata)
{
	extern char *optarg;
	extern int optind;
	extern int optopt;
	
	int ch;

	if (argc < 6) {
		pusage();
		return (FALSE);
	}

	/* 
	 * Set some defaults and read the table, file, and direction arguments.  
	 */
	pdata->firstrow = 0;
	pdata->lastrow = 0;
	pdata->batchsize = 1000;
	pdata->maxerrors = 10;

	/* argument 1 - the database object */
	pdata->dbobject = strdup(argv[1]);
	if (pdata->dbobject == NULL) {
		fprintf(stderr, "Out of memory!\n");
		return FALSE;
	}

	/* argument 2 - the direction */
	tds_strlcpy(pdata->dbdirection, argv[2], sizeof(pdata->dbdirection));

	if (strcasecmp(pdata->dbdirection, "in") == 0) {
		pdata->direction = DB_IN;
	} else if (strcasecmp(pdata->dbdirection, "out") == 0) {
		pdata->direction = DB_OUT;
	} else if (strcasecmp(pdata->dbdirection, "queryout") == 0) {
		pdata->direction = DB_QUERYOUT;
	} else {
		fprintf(stderr, "Copy direction must be either 'in', 'out' or 'queryout'.\n");
		return (FALSE);
	}

	/* argument 3 - the datafile name */
	free(pdata->hostfilename);
	pdata->hostfilename = strdup(argv[3]);

	/* 
	 * Get the rest of the arguments 
	 */
	optind = 4; /* start processing options after table, direction, & filename */
	while ((ch = getopt(argc, argv, "m:f:e:F:L:b:t:r:U:P:i:I:S:h:T:A:o:O:0:C:ncEdvVD:")) != -1) {
		switch (ch) {
		case 'v':
		case 'V':
			printf("freebcp version %s\n", software_version);
			return FALSE;
			break;
		case 'm':
			pdata->mflag++;
			pdata->maxerrors = atoi(optarg);
			break;
		case 'f':
			pdata->fflag++;
			free(pdata->formatfile);
			pdata->formatfile = strdup(optarg);
			break;
		case 'e':
			pdata->eflag++;
			pdata->errorfile = strdup(optarg);
			break;
		case 'F':
			pdata->Fflag++;
			pdata->firstrow = atoi(optarg);
			break;
		case 'L':
			pdata->Lflag++;
			pdata->lastrow = atoi(optarg);
			break;
		case 'b':
			pdata->bflag++;
			pdata->batchsize = atoi(optarg);
			break;
		case 'n':
			pdata->nflag++;
			break;
		case 'c':
			pdata->cflag++;
			break;
		case 'E':
			pdata->Eflag++;
			break;
		case 'd':
			tdsdump_open(NULL);
			break;
		case 't':
			pdata->tflag++;
			pdata->fieldterm = strdup(optarg);
			pdata->fieldtermlen = unescape(pdata->fieldterm);
			break;
		case 'r':
			pdata->rflag++;
			pdata->rowterm = strdup(optarg);
			pdata->rowtermlen = unescape(pdata->rowterm);
			break;
		case 'U':
			pdata->Uflag++;
			pdata->user = strdup(optarg);
			break;
		case 'P':
			pdata->Pflag++;
			pdata->pass = getpassarg(optarg);
			break;
		case 'i':
			free(pdata->inputfile);
			pdata->inputfile = strdup(optarg);
			break;
		case 'I':
			pdata->Iflag++;
			free(pdata->interfacesfile);
			pdata->interfacesfile = strdup(optarg);
			break;
		case 'S':
			pdata->Sflag++;
			pdata->server = strdup(optarg);
			break;
		case 'D':
			pdata->dbname = strdup(optarg);
			break;
		case 'h':
			pdata->hint = strdup(optarg);
			break;
		case 'o':
			free(pdata->outputfile);
			pdata->outputfile = strdup(optarg);
			break;
		case 'O':
		case '0':
			pdata->options = strdup(optarg);
			break;
		case 'T':
			pdata->Tflag++;
			pdata->textsize = atoi(optarg);
			break;
		case 'A':
			pdata->Aflag++;
			pdata->packetsize = atoi(optarg);
			break;
		case 'C':
			pdata->charset = strdup(optarg);
			break;
		case '?':
		default:
			pusage();
			return (FALSE);
		}
	}

	/* 
	 * Check for required/disallowed option combinations 
	 * If no username is provided, rely on domain login. 
	 */
	 
	/* Server */
	if (!pdata->Sflag) {
		if ((pdata->server = getenv("DSQUERY")) != NULL) {
			pdata->server = strdup(pdata->server);	/* can be freed */
			pdata->Sflag++;
		} else {
			fprintf(stderr, "-S must be supplied.\n");
			return (FALSE);
		}
	}

	/* Only one of these can be specified */
	if (pdata->cflag + pdata->nflag + pdata->fflag != 1) {
		fprintf(stderr, "Exactly one of options -c, -n, -f must be supplied.\n");
		return (FALSE);
	}

	/* Character mode file: fill in default values */
	if (pdata->cflag) {

		if (!pdata->tflag || !pdata->fieldterm) {	/* field terminator not specified */
			pdata->fieldterm = "\t";
			pdata->fieldtermlen = 1;
		}
		if (!pdata->rflag || !pdata->rowterm) {		/* row terminator not specified */
			pdata->rowterm =  "\n";
			pdata->rowtermlen = 1;
		}
	}

	/*
	 * Override stdin and/or stdout if requested.
	 */

	/* FIXME -- Since we don't implement prompting for field data types when neither -c nor -n
	 * is specified, redirecting stdin doesn't do much yet.
	 */
	if (pdata->inputfile) {
		if (freopen(pdata->inputfile, "rb", stdin) == NULL) {
			fprintf(stderr, "%s: unable to open %s: %s\n", "freebcp", pdata->inputfile, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (pdata->outputfile) {
		if (freopen(pdata->outputfile, "wb", stdout) == NULL) {
			fprintf(stderr, "%s: unable to open %s: %s\n", "freebcp", pdata->outputfile, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	return (TRUE);

}

int
login_to_database(BCPPARAMDATA * pdata, DBPROCESS ** pdbproc)
{
	LOGINREC *login;

	/* Initialize DB-Library. */

	if (dbinit() == FAIL)
		return (FALSE);

	/*
	 * Install the user-supplied error-handling and message-handling
	 * routines. They are defined at the bottom of this source file.
	 */

	dberrhandle(err_handler);
	dbmsghandle(msg_handler);

	/* If the interfaces file was specified explicitly, set it. */
	if (pdata->interfacesfile != NULL)
		dbsetifile(pdata->interfacesfile);

	/* 
	 * Allocate and initialize the LOGINREC structure to be used 
	 * to open a connection to SQL Server.
	 */

	login = dblogin();
	if (!login)
		return FALSE;

	if (pdata->user)
		DBSETLUSER(login, pdata->user);
	if (pdata->pass) {
		DBSETLPWD(login, pdata->pass);
		memset(pdata->pass, 0, strlen(pdata->pass));
	}
	
	DBSETLAPP(login, "FreeBCP");
	if (pdata->charset)
		DBSETLCHARSET(login, pdata->charset);

	if (pdata->Aflag && pdata->packetsize > 0) {
		DBSETLPACKET(login, pdata->packetsize);
	}

	if (pdata->dbname)
		DBSETLDBNAME(login, pdata->dbname);

	/* Enable bulk copy for this connection. */

	BCP_SETL(login, TRUE);

	/*
	 * Get a connection to the database.
	 */

	if ((*pdbproc = dbopen(login, pdata->server)) == NULL) {
		fprintf(stderr, "Can't connect to server \"%s\".\n", pdata->server);
		dbloginfree(login);
		return (FALSE);
	}
	dbloginfree(login);
	login = NULL;

	return (TRUE);

}

int
file_character(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir)
{
	DBINT li_rowsread = 0;
	int i;
	int li_numcols = 0;
	RETCODE ret_code = 0;

	if (dir == DB_QUERYOUT) {
		if (dbfcmd(dbproc, "SET FMTONLY ON %s SET FMTONLY OFF", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	} else {
		if (dbfcmd(dbproc, "SET FMTONLY ON select * from %s SET FMTONLY OFF", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	}

	if (dbsqlexec(dbproc) == FAIL) {
		printf("dbsqlexec failed\n");
		return FALSE;
	}

	while (NO_MORE_RESULTS != (ret_code = dbresults(dbproc))) {
		if (ret_code == SUCCEED && li_numcols == 0) {
			li_numcols = dbnumcols(dbproc);
		}
	}

	if (0 == li_numcols) {
		printf("Error in dbnumcols\n");
		return FALSE;
	}

	if (FAIL == bcp_init(dbproc, pdata->dbobject, pdata->hostfilename, pdata->errorfile, dir))
		return FALSE;

	if (!set_bcp_hints(pdata, dbproc))
		return FALSE;

	if (pdata->Eflag) {

		bcp_control(dbproc, BCPKEEPIDENTITY, 1);
	
		if (dbfcmd(dbproc, "set identity_insert %s on", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	
		if (dbsqlexec(dbproc) == FAIL) {
			printf("dbsqlexec failed\n");
			return FALSE;
		}

		while (NO_MORE_RESULTS != dbresults(dbproc));
	}

	bcp_control(dbproc, BCPFIRST, pdata->firstrow);
	bcp_control(dbproc, BCPLAST, pdata->lastrow);
	bcp_control(dbproc, BCPMAXERRS, pdata->maxerrors);

	if (bcp_columns(dbproc, li_numcols) == FAIL) {
		printf("Error in bcp_columns.\n");
		return FALSE;
	}

	for (i = 1; i < li_numcols; ++i) {
		if (bcp_colfmt(dbproc, i, SYBCHAR, 0, -1, (const BYTE *) pdata->fieldterm,
			       pdata->fieldtermlen, i) == FAIL) {
			printf("Error in bcp_colfmt col %d\n", i);
			return FALSE;
		}
	}

	if (bcp_colfmt(dbproc, li_numcols, SYBCHAR, 0, -1, (const BYTE *) pdata->rowterm,
		       pdata->rowtermlen, li_numcols) == FAIL) {
		printf("Error in bcp_colfmt col %d\n", li_numcols);
		return FALSE;
	}

	bcp_control(dbproc, BCPBATCH, pdata->batchsize);

	printf("\nStarting copy...\n");

	if (FAIL == bcp_exec(dbproc, &li_rowsread)) {
		fprintf(stderr, "bcp copy %s failed\n", (dir == DB_IN) ? "in" : "out");
		return FALSE;
	}

	printf("%d rows copied.\n", li_rowsread);

	return TRUE;
}

int
file_native(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir)
{
	DBINT li_rowsread = 0;
	int i;
	int li_numcols = 0;
	int li_coltype;
	RETCODE ret_code = 0;

	if (dir == DB_QUERYOUT) {
		if (dbfcmd(dbproc, "SET FMTONLY ON %s SET FMTONLY OFF", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	} else {
		if (dbfcmd(dbproc, "SET FMTONLY ON select * from %s SET FMTONLY OFF", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	}

	if (dbsqlexec(dbproc) == FAIL) {
		printf("dbsqlexec failed\n");
		return FALSE;
	}

	while (NO_MORE_RESULTS != (ret_code = dbresults(dbproc))) {
		if (ret_code == SUCCEED && li_numcols == 0) {
			li_numcols = dbnumcols(dbproc);
		}
	}

	if (0 == li_numcols) {
		printf("Error in dbnumcols\n");
		return FALSE;
	}

	if (FAIL == bcp_init(dbproc, pdata->dbobject, pdata->hostfilename, pdata->errorfile, dir))
		return FALSE;

	if (!set_bcp_hints(pdata, dbproc))
		return FALSE;

	if (pdata->Eflag) {

		bcp_control(dbproc, BCPKEEPIDENTITY, 1);
	
		if (dbfcmd(dbproc, "set identity_insert %s on", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	
		if (dbsqlexec(dbproc) == FAIL) {
			printf("dbsqlexec failed\n");
			return FALSE;
		}

		while (NO_MORE_RESULTS != dbresults(dbproc));
	}

	bcp_control(dbproc, BCPFIRST, pdata->firstrow);
	bcp_control(dbproc, BCPLAST, pdata->lastrow);
	bcp_control(dbproc, BCPMAXERRS, pdata->maxerrors);

	if (bcp_columns(dbproc, li_numcols) == FAIL) {
		printf("Error in bcp_columns.\n");
		return FALSE;
	}

	for (i = 1; i <= li_numcols; i++) {
		li_coltype = dbcoltype(dbproc, i);

		if (bcp_colfmt(dbproc, i, li_coltype, -1, -1, NULL, -1, i) == FAIL) {
			printf("Error in bcp_colfmt col %d\n", i);
			return FALSE;
		}
	}

	printf("\nStarting copy...\n\n");


	if (FAIL == bcp_exec(dbproc, &li_rowsread)) {
		fprintf(stderr, "bcp copy %s failed\n", (dir == DB_IN) ? "in" : "out");
		return FALSE;
	}

	printf("%d rows copied.\n", li_rowsread);

	return TRUE;
}

int
file_formatted(BCPPARAMDATA * pdata, DBPROCESS * dbproc, DBINT dir)
{

	int li_rowsread;

	if (FAIL == bcp_init(dbproc, pdata->dbobject, pdata->hostfilename, pdata->errorfile, dir))
		return FALSE;

	if (!set_bcp_hints(pdata, dbproc))
		return FALSE;

	if (pdata->Eflag) {

		bcp_control(dbproc, BCPKEEPIDENTITY, 1);
	
		if (dbfcmd(dbproc, "set identity_insert %s on", pdata->dbobject) == FAIL) {
			printf("dbfcmd failed\n");
			return FALSE;
		}
	
		if (dbsqlexec(dbproc) == FAIL) {
			printf("dbsqlexec failed\n");
			return FALSE;
		}

		while (NO_MORE_RESULTS != dbresults(dbproc));
	}

	bcp_control(dbproc, BCPFIRST, pdata->firstrow);
	bcp_control(dbproc, BCPLAST, pdata->lastrow);
	bcp_control(dbproc, BCPMAXERRS, pdata->maxerrors);

	if (FAIL == bcp_readfmt(dbproc, pdata->formatfile))
		return FALSE;

	printf("\nStarting copy...\n\n");


	if (FAIL == bcp_exec(dbproc, &li_rowsread)) {
		fprintf(stderr, "bcp copy %s failed\n", (dir == DB_IN) ? "in" : "out");
		return FALSE;
	}

	printf("%d rows copied.\n", li_rowsread);

	return TRUE;
}


int
setoptions(DBPROCESS * dbproc, BCPPARAMDATA * params){
	FILE *optFile;
	char optBuf[256];
	RETCODE fOK;

	if (dbfcmd(dbproc, "set textsize %d ", params->textsize) == FAIL) {
		fprintf(stderr, "setoptions() could not set textsize at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	/* 
	 * If the option is a filename, read the SQL text from the file.  
	 * Else pass the option verbatim to the server.
	 */
	if (params->options) {
		if ((optFile = fopen(params->options, "r")) == NULL) {
			if (dbfcmd(dbproc, params->options) == FAIL) {
				fprintf(stderr, "setoptions() failed preparing options at %s:%d\n", __FILE__, __LINE__);
				return FALSE;
			}
		} else {
			while (fgets (optBuf, sizeof(optBuf), optFile) != NULL) {
				if (dbfcmd(dbproc, optBuf) == FAIL) {
					fprintf(stderr, "setoptions() failed preparing options at %s:%d\n", __FILE__, __LINE__);
					return FALSE;
				}
			}
			if (!feof (optFile)) {
				perror("freebcp");
        			fprintf(stderr, "error reading options file \"%s\" at %s:%d\n", params->options, __FILE__, __LINE__);
				return FALSE;
			}
			fclose(optFile);
		}
	
	}
	
	if (dbsqlexec(dbproc) == FAIL) {
		fprintf(stderr, "setoptions() failed sending options at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}
	
	while ((fOK = dbresults(dbproc)) == SUCCEED) {
		while ((fOK = dbnextrow(dbproc)) == REG_ROW);
		if (fOK == FAIL) {
			fprintf(stderr, "setoptions() failed sending options at %s:%d\n", __FILE__, __LINE__);
			return FALSE;
		}
	}
	if (fOK == FAIL) {
		fprintf(stderr, "setoptions() failed sending options at %s:%d\n", __FILE__, __LINE__);
		return FALSE;
	}

	return TRUE;
}

static int
set_bcp_hints(BCPPARAMDATA *pdata, DBPROCESS *pdbproc)
{
	/* set hint if any */
	if (pdata->hint) {
		if (bcp_options(pdbproc, BCPHINTS, (BYTE *) pdata->hint, strlen(pdata->hint)) != SUCCEED) {
			fprintf(stderr, "db-lib: Unable to set hint \"%s\"\n", pdata->hint);
			return FALSE;
		}
	}
	return TRUE;
}

void
pusage(void)
{
	fprintf(stderr, "usage:  freebcp [[database_name.]owner.]table_name|query {in | out | queryout } datafile\n");
	fprintf(stderr, "        [-m maxerrors] [-f formatfile] [-e errfile]\n");
	fprintf(stderr, "        [-F firstrow] [-L lastrow] [-b batchsize]\n");
	fprintf(stderr, "        [-n] [-c] [-t field_terminator] [-r row_terminator]\n");
	fprintf(stderr, "        [-U username] [-P password] [-I interfaces_file] [-S server] [-D database]\n");
	fprintf(stderr, "        [-v] [-d] [-h \"hint [,...]\" [-O \"set connection_option on|off, ...]\"\n");
	fprintf(stderr, "        [-A packet size] [-T text or image size] [-E]\n");
	fprintf(stderr, "        [-i input_file] [-o output_file]\n");
	fprintf(stderr, "        \n");
	fprintf(stderr, "example: freebcp testdb.dbo.inserttest in inserttest.txt -S mssql -U guest -P password -c\n");
}

int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	static int sent = 0;

	if (dberr == SYBEBBCI) { /* Batch successfully bulk copied to the server */
		int batch = bcp_getbatchsize(dbproc);
		printf("%d rows sent to SQL Server.\n", sent += batch);
		return INT_CANCEL;
	}
	
	if (dberr) {
		fprintf(stderr, "Msg %d, Level %d\n", dberr, severity);
		fprintf(stderr, "%s\n\n", dberrstr);
	}

	else {
		fprintf(stderr, "DB-LIBRARY error:\n\t");
		fprintf(stderr, "%s\n", dberrstr);
	}

	return INT_CANCEL;
}

int
msg_handler(DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	/*
	 * If it's a database change message, we'll ignore it.
	 * Also ignore language change message.
	 */
	if (msgno == 5701 || msgno == 5703)
		return (0);

	printf("Msg %ld, Level %d, State %d\n", (long) msgno, severity, msgstate);

	if (strlen(srvname) > 0)
		printf("Server '%s', ", srvname);
	if (strlen(procname) > 0)
		printf("Procedure '%s', ", procname);
	if (line > 0)
		printf("Line %d", line);

	printf("\n\t%s\n", msgtext);

	return (0);
}
