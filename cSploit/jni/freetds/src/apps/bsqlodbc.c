/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2004, 2005  James K. Lowden
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
#include <assert.h>
#include <ctype.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#include "tds_sysdep_public.h"
#include <sql.h>
#include <sqlext.h>
#include "replacements.h"

static char software_version[] = "$Id: bsqlodbc.c,v 1.18 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static char * next_query(void);
static void print_results(SQLHSTMT hStmt);
/* static int get_printable_size(int type, int size); */
static void usage(const char invoked_as[]);

static SQLINTEGER print_error_message(SQLSMALLINT hType, SQLHANDLE handle);
static SQLRETURN odbc_herror(SQLSMALLINT hType, SQLHANDLE handle, SQLRETURN erc, const char func[], const char msg[]);
static SQLRETURN odbc_perror(SQLHSTMT hStmt, SQLRETURN erc, const char func[], const char msg[]);
static const char * prret(SQLRETURN erc);


struct METADATA
{
	char *name, *format_string;
	const char *source;
	SQLSMALLINT type;
	SQLINTEGER nchars;
	SQLULEN size, width;
};
struct DATA { char *buffer; SQLLEN len; int status; };

static int set_format_string(struct METADATA * meta, const char separator[]);

typedef struct _options 
{ 
	int 	fverbose, 
		fquiet;
	size_t	odbc_version;
	FILE 	*headers, 
		*verbose;
	char 	*servername, 
		*database, 
		*appname, 
		 hostname[128];
	const char *colsep;
	char	*input_filename, 
		*output_filename, 
		*error_filename; 
} OPTIONS;

typedef struct
{
	int connect_timeout, query_timeout;
	char 	*client_hostname,
		*username,
		*password,
		*client_charset;
} LOGINREC;

static LOGINREC* get_login(int argc, char *argv[], OPTIONS *poptions);

/* global variables */
OPTIONS options;
static const char default_colsep[] = "  ";
/* end global variables */


/**
 * The purpose of this program is threefold:
 *
 * 1.  To provide a generalized SQL processor suitable for testing the FreeTDS ODBC driver.
 * 2.  To offer a robust batch-oriented SQL processor suitable for use in a production environment.  
 * 3.  To serve as a model example of how to use ODBC functions.  
 *
 * These purposes may be somewhat impossible.  The original author is not an experienced ODBC programmer.  
 * Nevertheless the first objective can be met and is useful by itself.  
 * If others join in, perhaps the others can be met, too. 
 */


/**
 * When SQLExecute returns either SQL_ERROR or SQL_SUCCESS_WITH_INFO, 
 * an associated SQLSTATE value can be obtained by calling SQLGetDiagRec 
 * with a HandleType of SQL_HANDLE_STMT and a Handle of StatementHandle.  
 */
static SQLINTEGER
print_error_message(SQLSMALLINT hType, SQLHANDLE handle) {
	int i;
	SQLINTEGER ndiag=0;
	SQLRETURN ret;
	SQLCHAR state[6];
	SQLINTEGER error, maxerror=0;
	SQLCHAR text[1024];
	SQLSMALLINT len;

	ret = SQLGetDiagField(hType, handle, 0, SQL_DIAG_NUMBER, &ndiag, sizeof(ndiag), NULL);
	assert(ret == SQL_SUCCESS);

	for(i=1; i <= ndiag; i++) {
		memset(text, '\0', sizeof(text));
		ret = SQLGetDiagRec(hType, handle, i, state, &error, text, sizeof(text), &len);
		
		if (ret == SQL_SUCCESS && error == 0) {
			fprintf(stdout, "\"%s\"\n", text);
			continue;
		}
		
		fprintf(stderr, "%s: error %d: %s: %s\n", options.appname, (int)error, state, text);
		assert(ret == SQL_SUCCESS);
		
		if (error > maxerror)
			maxerror = error;
	}
	return maxerror;
}

static const char *
prret(SQLRETURN erc)
{
	switch(erc) {
	case SQL_SUCCESS: 		return "SQL_SUCCESS";
	case SQL_SUCCESS_WITH_INFO: 	return "SQL_SUCCESS_WITH_INFO";
	case SQL_ERROR: 		return "SQL_ERROR";
	case SQL_STILL_EXECUTING: 	return "SQL_STILL_EXECUTING";
	case SQL_INVALID_HANDLE: 	return "SQL_INVALID_HANDLE";
	case SQL_NO_DATA: 		return "SQL_NO_DATA";
	}
	fprintf(stderr, "error:%d: prret cannot interpret SQLRETURN code %d\n", __LINE__, erc);
	return "unknown";
}

static SQLRETURN
odbc_herror(SQLSMALLINT hType, SQLHANDLE handle, SQLRETURN erc, const char func[], const char msg[])
{
	char * ercstr;

	assert(func && msg);
	
	switch(erc) {
	case SQL_SUCCESS:
		return erc;
	case SQL_SUCCESS_WITH_INFO:
		ercstr = "SQL_SUCCESS_WITH_INFO";
		break;
	case SQL_ERROR:
		ercstr = "SQL_ERROR";
		break;
	case SQL_STILL_EXECUTING:
		ercstr = "SQL_STILL_EXECUTING";
		break;
	case SQL_INVALID_HANDLE:
		fprintf(stderr, "%s: error %d: %s: invalid handle: %s\n", options.appname, (int)erc, func, msg);
		exit(EXIT_FAILURE);
	default:
		fprintf(stderr, "%s: error: %d is an unknown return code for %s\n", options.appname, (int)erc, func);
		exit(EXIT_FAILURE);
	}
	
	fprintf(stderr, "%s: error %d: %s: %s: %s\n", options.appname, (int)erc, func, ercstr, msg);

	print_error_message(hType, handle); 
	
	return erc;
}

static SQLRETURN
odbc_perror(SQLHSTMT hStmt, SQLRETURN erc, const char func[], const char msg[])
{
	return odbc_herror(SQL_HANDLE_STMT, hStmt, erc, func, msg);
}

int
main(int argc, char *argv[])
{
	LOGINREC *login;
	SQLHENV hEnv = 0;
	SQLHDBC hDbc = 0;
	SQLRETURN erc;
	const char *sql;

	setlocale(LC_ALL, "");

	memset(&options, 0, sizeof(options));
	options.headers = stderr;
	login = get_login(argc, argv, &options); /* get command-line parameters and call dblogin() */
	assert(login != NULL);

	/* 
	 * Override stdin, stdout, and stderr, as required 
	 */
	if (options.input_filename) {
		if (freopen(options.input_filename, "r", stdin) == NULL) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.input_filename, strerror(errno));
			exit(1);
		}
	}

	if (options.output_filename) {
		if (freopen(options.output_filename, "w", stdout) == NULL) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.output_filename, strerror(errno));
			exit(1);
		}
	}
	
	if (options.error_filename) {
		if (freopen(options.error_filename, "w", stderr) == NULL) {
			fprintf(stderr, "%s: unable to open %s: %s\n", options.appname, options.error_filename, strerror(errno));
			exit(1);
		}
	}

	if (options.fverbose) {
		options.verbose = stderr;
	} else {
		static const char null_device[] = "/dev/null";
		options.verbose = fopen(null_device, "w");
		if (options.verbose == NULL) {
			fprintf(stderr, "%s:%d unable to open %s for verbose operation: %s\n", 
					options.appname, __LINE__, null_device, strerror(errno));
			exit(1);
		}
	}

	fprintf(options.verbose, "%s:%d: Verbose operation enabled\n", options.appname, __LINE__);
	
	/* 
	 * Connect to the server 
	 */
	if ((erc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv)) != SQL_SUCCESS) {
		odbc_herror(SQL_HANDLE_ENV, hEnv, erc, "SQLAllocHandle", "failed to allocate an environment");
		exit(EXIT_FAILURE);
	}
	assert(hEnv);

	if ((erc = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)options.odbc_version, SQL_IS_UINTEGER)) != SQL_SUCCESS){
		odbc_herror(SQL_HANDLE_DBC, hDbc, erc, "SQLSetEnvAttr", "failed to set SQL_OV_ODBC3");
		exit(EXIT_FAILURE);
	}

	if ((erc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)) != SQL_SUCCESS) {
		odbc_herror(SQL_HANDLE_DBC, hDbc, erc, "SQLAllocHandle", "failed to allocate a connection");
		exit(EXIT_FAILURE);
	}
	assert(hDbc);

	if ((erc = SQLConnect(hDbc, 	(SQLCHAR *) options.servername, SQL_NTS, 
					(SQLCHAR *) login->username, SQL_NTS, 
					(SQLCHAR *) login->password, SQL_NTS)) != SQL_SUCCESS) {
		odbc_herror(SQL_HANDLE_DBC, hDbc, erc, "SQLConnect", "failed");
		exit(EXIT_FAILURE);
	}

#if 0
	/* Switch to the specified database, if any */
	if (options.database)
		dbuse(dbproc, options.database);
#endif

	/* 
	 * Read the queries and write the results
	 */
	while ((sql = next_query()) != NULL ) {
		SQLHSTMT hStmt;

		if ((erc = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt)) != SQL_SUCCESS) {
			odbc_perror(hStmt, erc, "SQLAllocHandle", "failed to allocate a statement handle");
			exit(EXIT_FAILURE);
		}

		/* "Prepare" the query and send it to the server */
		if ((erc = SQLPrepare(hStmt, (SQLCHAR *) sql, SQL_NTS)) != SQL_SUCCESS) {
			odbc_perror(hStmt, erc, "SQLPrepare", "failed");
			exit(EXIT_FAILURE);
		}

		if((erc = SQLExecute(hStmt)) != SQL_SUCCESS) {
			switch(erc) {
			case SQL_NEED_DATA: 
				goto FreeStatement;
			case SQL_SUCCESS_WITH_INFO:
				if (0 != print_error_message(SQL_HANDLE_STMT, hStmt)) {
					fprintf(stderr, "SQLExecute: continuing...\n");
				}
				break;
			default:
				odbc_perror(hStmt, erc, "SQLExecute", "failed");
				exit(EXIT_FAILURE);
			}
		}

		/* Write the output */
		print_results(hStmt);
		
		FreeStatement:
		if ((erc = SQLFreeHandle(SQL_HANDLE_STMT, hStmt)) != SQL_SUCCESS){
			odbc_perror(hStmt, erc, "SQLFreeHandle", "failed");
			exit(EXIT_FAILURE);
		} 
	}

	SQLDisconnect(hDbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
	SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

	return 0;
}

static char *
next_query()
{
	char query_line[4096];
	static char *sql = NULL;

	if (feof(stdin))
		return NULL;

	fprintf(options.verbose, "%s:%d: Query:\n", options.appname, __LINE__);

	free(sql);
	sql = NULL;

	while (fgets(query_line, sizeof(query_line), stdin)) {
		/* 'go' or 'GO' separates command batches */
		char *p = query_line;
		if (strncasecmp(p, "go", 2) == 0) {
			for (p+=2; isspace((unsigned char) *p); p++) {
				if (*p == '\n')
					return sql;
			}
		}

		fprintf(options.verbose, "\t%s", query_line);
		
		/* Add the query line to the command to be sent to the server */
		if (!strlen(query_line))
			continue;
		p = realloc(sql, 1 + (sql? strlen(sql) : 0) + strlen(query_line));
		if (!p) {
			fprintf(stderr, "%s:%d: could not allocate memory\n", options.appname, __LINE__);
			return NULL;
		}
		if (!sql) {
			strcpy(p, query_line);
		} else {
			strcat(p, query_line);
		}
		sql = p;
	}
	
	if (feof(stdin))
		return sql;
			
	if (ferror(stdin)) {
		fprintf(stderr, "%s:%d: next_query() failed\n", options.appname, __LINE__);
		perror(NULL);
		return NULL;
	}
	
	return sql;
}

static void
free_metadata(struct METADATA *metadata, struct DATA *data, int ncols)
{
	int c;
	
	for (c=0; c < ncols; c++) {
		free(metadata[c].format_string);
		free(data[c].buffer);
	}
	free(metadata);
	free(data);
}

static const char *
prtype(SQLSMALLINT type)
{
	static char buffer[64];
	
	switch(type) {
	case SQL_UNKNOWN_TYPE:		return "SQL_UNKNOWN_TYPE";
	case SQL_CHAR:			return "SQL_CHAR";
	case SQL_NUMERIC:		return "SQL_NUMERIC";
	case SQL_DECIMAL:		return "SQL_DECIMAL";
	case SQL_INTEGER:		return "SQL_INTEGER";
	case SQL_SMALLINT:		return "SQL_SMALLINT";
	case SQL_FLOAT:			return "SQL_FLOAT";
	case SQL_REAL:			return "SQL_REAL";
	case SQL_DOUBLE:		return "SQL_DOUBLE";
#if (ODBCVER >= 0x0300)
	case SQL_DATETIME:		return "SQL_DATETIME";
#else
	case SQL_DATE:			return "SQL_DATE";
#endif
	case SQL_VARCHAR:		return "SQL_VARCHAR";
#if (ODBCVER >= 0x0300)
	case SQL_TYPE_DATE:		return "SQL_TYPE_DATE";
	case SQL_TYPE_TIME:		return "SQL_TYPE_TIME";
	case SQL_TYPE_TIMESTAMP:	return "SQL_TYPE_TIMESTAMP";
#endif
#if (ODBCVER >= 0x0300)
	case SQL_INTERVAL:		return "SQL_INTERVAL";
#else
	case SQL_TIME:			return "SQL_TIME";
#endif  /* ODBCVER >= 0x0300 */
	case SQL_TIMESTAMP:		return "SQL_TIMESTAMP";
	case SQL_LONGVARCHAR:		return "SQL_LONGVARCHAR";
	case SQL_BINARY:		return "SQL_BINARY";
	case SQL_VARBINARY:		return "SQL_VARBINARY";
	case SQL_LONGVARBINARY:		return "SQL_LONGVARBINARY";
	case SQL_BIGINT:		return "SQL_BIGINT";
	case SQL_TINYINT:		return "SQL_TINYINT";
	case SQL_BIT:			return "SQL_BIT";
#if (ODBCVER >= 0x0350)
	case SQL_GUID:			return "SQL_GUID";
#endif  /* ODBCVER >= 0x0350 */
	}

	sprintf(buffer, "unknown: %d", (int)type);
	return buffer;
}

#define is_character_data(x)   (x==SQL_CHAR	    || \
				x==SQL_VARCHAR	    || \
				x==SQL_LONGVARCHAR  || \
				x==SQL_WCHAR	    || \
				x==SQL_WVARCHAR     || \
				x==SQL_WLONGVARCHAR)

static SQLLEN 
bufsize(const struct METADATA *meta)
{
	assert(meta);
	return meta->size > meta->width? meta->size : meta->width;
}

static void
print_results(SQLHSTMT hStmt) 
{
	static const char dashes[] = "----------------------------------------------------------------" /* each line is 64 */
				     "----------------------------------------------------------------"
				     "----------------------------------------------------------------"
				     "----------------------------------------------------------------";
	
	struct METADATA *metadata = NULL;
	
	struct DATA *data = NULL;
	
	SQLSMALLINT ncols = 0;
	RETCODE erc;
	int c, ret;

	/* 
	 * Process each resultset
	 */
	do {
		/* free metadata, in case it was previously allocated */
		free_metadata(metadata, data, ncols);
		metadata = NULL;
		data = NULL;
		ncols = 0;
		
		/* 
		 * Allocate memory for metadata and bound columns 
		 */
		if ((erc = SQLNumResultCols(hStmt, &ncols)) != SQL_SUCCESS){
			odbc_perror(hStmt, erc, "SQLNumResultCols", "failed");
			exit(EXIT_FAILURE);
		} 
		
		metadata = (struct METADATA*) calloc(ncols, sizeof(struct METADATA));
		assert(metadata);

		data = (struct DATA*) calloc(ncols, sizeof(struct DATA));
		assert(data);
		
		/* 
		 * For each column, get its name, type, and size. 
		 * Allocate a buffer to hold the data, and bind the buffer to the column.
		 * "bind" here means to give the address of the buffer we want filled as each row is fetched.
		 */

		fprintf(options.verbose, "Metadata\n");
		fprintf(options.verbose, "%-6s  %-30s  %-10s  %-18s  %-6s  %-6s  \n", 
					 "col", "name", "type value", "type name", "size", "varies");
		fprintf(options.verbose, "%.6s  %.30s  %.10s  %.18s  %.6s  %.6s  \n", 
					 dashes, dashes, dashes, dashes, dashes, dashes);
		for (c=0; c < ncols; c++) {
			/* Get and print the metadata.  Optional: get only what you need. */
			SQLCHAR name[512];
			SQLSMALLINT namelen, ndigits, fnullable;

			if ((erc = SQLDescribeCol(hStmt, c+1, name, sizeof(name), &namelen, 
							&metadata[c].type, &metadata[c].size,
							&ndigits, &fnullable)) != SQL_SUCCESS) {
				odbc_perror(hStmt, erc, "SQLDescribeCol", "failed");
				exit(EXIT_FAILURE);
			} 
			assert(namelen < sizeof(name));
			name[namelen] = '\0';
			metadata[c].name = strdup((char *) name);
			metadata[c].width = (ndigits > metadata[c].size)? ndigits : metadata[c].size;
			
			if (is_character_data(metadata[c].type)) {
				SQLHDESC hDesc;
				SQLINTEGER buflen;

				metadata[c].nchars = metadata[c].size;

				if ((erc = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hDesc, sizeof(hDesc), NULL)) != SQL_SUCCESS) {
					odbc_perror(hStmt, erc, "SQLGetStmtAttr", "failed");
					exit(EXIT_FAILURE);
				} 
				if ((erc = SQLGetDescField(hDesc, c+1, SQL_DESC_OCTET_LENGTH, 
								&metadata[c].size, sizeof(metadata[c].size), 
								&buflen)) != SQL_SUCCESS) {
					odbc_perror(hStmt, erc, "SQLGetDescField", "failed");
					exit(EXIT_FAILURE);
				}
				metadata[c].size = metadata[c].size * 2 + 1;
			}

			fprintf(options.verbose, "%6d  %30s  %10d  %18s  %6lu  %6d  \n", 
				c+1, metadata[c].name, (int)metadata[c].type, prtype(metadata[c].type), 
				(long unsigned int) metadata[c].size,  -1);

#if 0
			fprintf(options.verbose, "%6d  %30s  %30s  %15s  %6d  %6d  \n", 
				c+1, metadata[c].name, metadata[c].source, dbprtype(metadata[c].type), 
				metadata[c].size,  dbvarylen(dbproc, c+1));

			metadata[c].width = get_printable_size(metadata[c].type, metadata[c].size);
			if (metadata[c].width < strlen(metadata[c].name))
				metadata[c].width = strlen(metadata[c].name);
#endif				
			/* 
			 * Build the column header format string, based on the column width. 
			 * This is just one solution to the question, "How wide should my columns be when I print them out?"
			 */
			ret = set_format_string(&metadata[c], (c+1 < ncols)? options.colsep : "\n");
			if (ret <= 0) {
				fprintf(stderr, "%s:%d: asprintf(), column %d failed\n", options.appname, __LINE__, c+1);
				return;
			}

			/* 
			 * Bind the column to our variable.
			 * We bind everything to strings, because we want to convert everything to strings for us.
			 * If you're performing calculations on the data in your application, you'd bind the numeric data
			 * to C integers and floats, etc. instead. 
			 * 
			 * It is not necessary to bind to every column returned by the query.  
			 * Data in unbound columns are simply never copied to the user's buffers and are thus 
			 * inaccesible to the application.  
			 */

			data[c].buffer = calloc(1, bufsize(&metadata[c]));
			assert(data[c].buffer);

			if ((erc = SQLBindCol(hStmt, c+1, SQL_C_CHAR, (SQLPOINTER)data[c].buffer, 
						bufsize(&metadata[c]), &data[c].len)) != SQL_SUCCESS){
				odbc_perror(hStmt, erc, "SQLBindCol", "failed");
				exit(EXIT_FAILURE);
			} 

		}
		
		if (!options.fquiet) {
			/* Print the column headers to stderr to keep them separate from the data.  */
			for (c=0; c < ncols; c++) {
				fprintf(options.headers, metadata[c].format_string, metadata[c].name);
			}

			/* Underline the column headers.  */
			for (c=0; c < ncols; c++) {
				fprintf(options.headers, metadata[c].format_string, dashes);
			}
		}
		/* 
		 * Print the data to stdout.  
		 */
		while (ncols > 0 && (erc = SQLFetch(hStmt)) != SQL_NO_DATA) {
			switch(erc) {
			case SQL_SUCCESS_WITH_INFO:
				print_error_message(SQL_HANDLE_STMT, hStmt);
			case SQL_SUCCESS:
				break;
			default:
				odbc_perror(hStmt, erc, "SQLFetch", "failed");
				exit(EXIT_FAILURE);
			}
			for (c=0; c < ncols; c++) {
				switch (data[c].len) { /* handle nulls */
				case SQL_NULL_DATA: /* is null */
					fprintf(stdout, metadata[c].format_string, "NULL");
					break;
				default:
					assert(data[c].len >= 0);
				case SQL_NO_TOTAL:
					fprintf(stdout, metadata[c].format_string, data[c].buffer);
					break;
				}
			}
		}
		if (ncols > 0 && erc == SQL_NO_DATA)
			print_error_message(SQL_HANDLE_STMT, hStmt);

		erc = SQLMoreResults(hStmt);
		fprintf(options.verbose, "SQLMoreResults returned %s\n", prret(erc));
		switch (erc) {
		case SQL_NO_DATA:
			print_error_message(SQL_HANDLE_STMT, hStmt);
			break;
		case SQL_SUCCESS_WITH_INFO:
			print_error_message(SQL_HANDLE_STMT, hStmt);
		case SQL_SUCCESS:
			continue;
		default:
			odbc_perror(hStmt, erc, "SQLMoreResults", "failed");
			exit(EXIT_FAILURE);
		}
	} while (erc != SQL_NO_DATA);
	
	if (erc != SQL_NO_DATA) {
		assert(erc != SQL_STILL_EXECUTING);
		odbc_perror(hStmt, erc, "SQLMoreResults", "failed");
		exit(EXIT_FAILURE);
	} 
}

#if 0 
static int
get_printable_size(int type, int size)	/* adapted from src/dblib/dblib.c */
{
	switch (type) {
	case SYBINTN:
		switch (size) {
		case 1:
			return 3;
		case 2:
			return 6;
		case 4:
			return 11;
		case 8:
			return 21;
		}
	case SYBINT1:
		return 3;
	case SYBINT2:
		return 6;
	case SYBINT4:
		return 11;
	case SYBINT8:
		return 21;
	case SYBVARCHAR:
	case SYBCHAR:
		return size;
	case SYBFLT8:
		return 11;	/* FIX ME -- we do not track precision */
	case SYBREAL:
		return 11;	/* FIX ME -- we do not track precision */
	case SYBMONEY:
		return 12;	/* FIX ME */
	case SYBMONEY4:
		return 12;	/* FIX ME */
	case SYBDATETIME:
		return 26;	/* FIX ME */
	case SYBDATETIME4:
		return 26;	/* FIX ME */
#if 0	/* seems not to be exported to sybdb.h */
	case SYBBITN:
#endif
	case SYBBIT:
		return 1;
		/* FIX ME -- not all types present */
	default:
		return 0;
	}

}
#endif /* 0, not used */

/** 
 * Build the column header format string, based on the column width. 
 * This is just one solution to the question, "How wide should my columns be when I print them out?"
 */
static int
set_format_string(struct METADATA * meta, const char separator[])
{
	int width, ret;
	const char *size_and_width;
	assert(meta);

	if(0 == strcmp(options.colsep, default_colsep)) { 
		/* right justify numbers, left justify strings */
		size_and_width = is_character_data(meta->type)? "%%-%d.%ds%s" : "%%%d.%ds%s";
		
		width = meta->width; /* get_printable_size(meta->type, meta->size); */
		if (width < strlen(meta->name))
			width = strlen(meta->name);

		ret = asprintf(&meta->format_string, size_and_width, width, width, separator);
	} else {
		/* For anything except the default two-space separator, don't justify the strings. */
		ret = asprintf(&meta->format_string, "%%s%s", separator);
	}
		       
	return ret;
}

static void
usage(const char invoked_as[])
{
	fprintf(stderr, "usage:  %s \n"
			"        [-U username] [-P password]\n"
			"        [-S servername] [-D database]\n"
			"        [-i input filename] [-o output filename] [-e error filename]\n"
			, invoked_as);
}

static void
unescape(char arg[])
{
	char *p = arg;
	char escaped = '1'; /* any digit will do for an initial value */
	while ((p = strchr(p, '\\')) != NULL) {
	
		switch (p[1]) {
		case '0':
			/* FIXME we use strlen() of field/row terminators, which obviously won't work here */
			fprintf(stderr, "bsqlodbc, line %d: NULL terminators ('\\0') not yet supported.\n", __LINE__);
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
			break;
		}
			
		/* Overwrite the backslash with the intended character, and shift everything down one */
		if (!isdigit((unsigned char) escaped)) {
			*p++ = escaped;
			memmove(p, p+1, 1 + strlen(p+1));
			escaped = '1';
		}
	}
}

static LOGINREC *
get_login(int argc, char *argv[], OPTIONS *options)
{
	LOGINREC *login;
	int ch;

	extern char *optarg;

	assert(options && argv);
	
	options->appname = tds_basename(argv[0]);
	options->colsep = default_colsep; /* may be overridden by -t */
	options->odbc_version = SQL_OV_ODBC3;	  /* may be overridden by -V */
	
	login = calloc(1, sizeof(LOGINREC));
	
	if (!login) {
		fprintf(stderr, "%s: unable to allocate login structure\n", options->appname);
		exit(1);
	}
	
	if (-1 == gethostname(options->hostname, sizeof(options->hostname))) {
		perror("unable to get hostname");
	} 

	while ((ch = getopt(argc, argv, "U:P:S:d:D:i:o:e:t:V:hqv")) != -1) {
		char *p;
		switch (ch) {
		case 'U':
			login->username = strdup(optarg);
			break;
		case 'P':
			login->password = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			break;
		case 'S':
			options->servername = strdup(optarg);
			break;
		case 'd':
		case 'D':
			options->database = strdup(optarg);
			break;
		case 'i':
			options->input_filename = strdup(optarg);
			break;
		case 'o':
			options->output_filename = strdup(optarg);
			break;
		case 'e':
			options->error_filename = strdup(optarg);
			break;
		case 't':
			unescape(optarg);
			options->colsep = strdup(optarg);
			break;
		case 'h':
			options->headers = stdout;
			break;
		case 'q':
			options->fquiet = 1;
			break;
		case 'V':
			switch(strtol(optarg, &p, 10)) {
			case 3: 
				options->odbc_version = SQL_OV_ODBC3
				;
				break;
			case 2:
				options->odbc_version = SQL_OV_ODBC2;
				break;
			default:
				fprintf(stderr, "warning: -V must be 2 or 3, not %s.  Using default of 3\n", optarg);
				break;
			}
		case 'v':
			options->fverbose = 1;
			break;
		case '?':
		default:
			usage(options->appname);
			exit(1);
		}
	}
	
	if (!options->servername) {
		usage(options->appname);
		exit(1);
	}
	
	return login;
}

