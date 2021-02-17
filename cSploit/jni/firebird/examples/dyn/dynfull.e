/*
 *  Program type:   Embedded Dynamic SQL
 *
 *	Description:
 *		This program prompts for and executes unknown SQL statements.
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "example.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <ibase.h>
#include "align.h"

#define	MAXLEN	1024
#define EOFIND -1

void process_statement (XSQLDA **sqlda, char *query);
void print_column (XSQLVAR *var);
int get_statement (char *buf);

typedef struct vary2 {
	short       vary_length;
	char        vary_string [1];
} VARY2;

#ifndef ISC_INT64_FORMAT

/* Define a format string for printf.  Printing of 64-bit integers
   is not standard between platforms */

#if (defined(_MSC_VER) && defined(WIN32))
#define	ISC_INT64_FORMAT	"I64"
#else
#define	ISC_INT64_FORMAT	"ll"
#endif
#endif

EXEC SQL
	SET SQL DIALECT 3;

EXEC SQL
	SET DATABASE db = COMPILETIME "employee.fdb";



int main(int argc, char** argv)
{
	char	query[MAXLEN];
	XSQLDA	*sqlda;
	char	db_name[128];

	if (argc < 2)
	{
		printf("Enter the database name:  ");
		gets(db_name);
	}
	else
		strcpy(db_name, *(++argv));

	EXEC SQL
		CONNECT :db_name AS db;

	if (SQLCODE)
	{
		printf("Could not open database %s\n", db_name);
		return(1);
	}

	/*
	 *	Allocate enough space for 20 fields.
	 *	If more fields get selected, re-allocate SQLDA later.
	 */
	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH (20));
	sqlda->sqln = 20;
	sqlda->version = 1;


	/*
	 *	Process SQL statements.
	 */
	while (get_statement(query))
	{
		process_statement(&sqlda, query);
	}

	EXEC SQL
		DISCONNECT db;

	return(0);
}


void process_statement(XSQLDA** sqldap, char* query)
{
	int	buffer[MAXLEN];
	XSQLVAR	*var;
	XSQLDA *sqlda;
	short	num_cols, i;
	short	length, alignment, type, offset;

	sqlda = *sqldap;
	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/* Start a transaction for each statement */

	EXEC SQL
		SET TRANSACTION;

	EXEC SQL
		PREPARE q INTO SQL DESCRIPTOR sqlda FROM :query;

	EXEC SQL
		DESCRIBE q INTO SQL DESCRIPTOR sqlda;

	/*
	 *	Execute a non-select statement.
	 */
	if (!sqlda->sqld)
	{
		EXEC SQL
			EXECUTE q;

		EXEC SQL
			COMMIT;

	return ;
	}

	/*
	 *	Process select statements.
	 */

	num_cols = sqlda->sqld;

	/* Need more room. */
	if (sqlda->sqln < num_cols)
	{
		free(sqlda);
		sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH (num_cols));
		sqlda->sqln = num_cols;
		sqlda->sqld = num_cols;
		sqlda->version = 1;

		EXEC SQL
			DESCRIBE q INTO SQL DESCRIPTOR sqlda;

		num_cols = sqlda->sqld;
	}

	/* Open the cursor. */
	EXEC SQL
		DECLARE c CURSOR FOR q;
	EXEC SQL
		OPEN c;

	/*
	 *	 Set up SQLDA.
	 */
	for (var = sqlda->sqlvar, offset = 0, i = 0; i < num_cols; var++, i++)
	{
		length = alignment = var->sqllen;
		type = var->sqltype & ~1;

		if (type == SQL_TEXT)
			alignment = 1;
		else if (type == SQL_VARYING)
		{   
			/* Allow space for vary's strlen & space to insert
			   a null byte for printing */
			length += sizeof (short) + 1;
			alignment = sizeof (short);
		}   
		offset = FB_ALIGN(offset, alignment);
		var->sqldata = (char*) buffer + offset;
		offset += length;
		offset = FB_ALIGN(offset, sizeof (short));
		var->sqlind = (short*) ((char*) buffer + offset);
		offset += sizeof  (short);
	}

	/*
	 *	Print rows.
	 */
	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH c USING SQL DESCRIPTOR sqlda;

		if (SQLCODE == 100)
			break;

		for (i = 0; i < num_cols; i++)
		{
			print_column(&sqlda->sqlvar[i]);
		}
		printf("\n");
	}

	EXEC SQL
		CLOSE c;

	EXEC SQL
		COMMIT;
	return;

Error:

	EXEC SQL
	    WHENEVER SQLERROR CONTINUE;

	printf("Statement failed.  SQLCODE = %d\n", SQLCODE);
	fflush (stdout);
	isc_print_status(gds__status);

	EXEC SQL
	    ROLLBACK;

	return;
}


/*
 *	Print column's data.
 */
void print_column(XSQLVAR* var)
{
	short		dtype;
	char		data[MAXLEN], *p;
	char		blob_s[20], date_s[25];
	VARY2		*vary2;
	short		len; 
	struct tm	times;
	ISC_QUAD	bid;

	dtype = var->sqltype & ~1;
	p = data;

	if ((var->sqltype & 1) && (*var->sqlind < 0))
	{
		switch (dtype)
		{
			case SQL_TEXT:
			case SQL_VARYING:
				len = var->sqllen;
				break;
			case SQL_SHORT:
				len = 6;
				if (var->sqlscale > 0) len += var->sqlscale;
				break;
			case SQL_LONG:
				len = 11;
				if (var->sqlscale > 0) len += var->sqlscale;
				break;
			case SQL_INT64:
				len = 21;
				if (var->sqlscale > 0) len += var->sqlscale;
				break;
			case SQL_FLOAT:
				len = 15;
				break;
			case SQL_DOUBLE:
				len = 24;
				break;
			case SQL_TIMESTAMP:
				len = 24;
				break;
			case SQL_TYPE_DATE:
				len = 10;
				break;
			case SQL_TYPE_TIME:
				len = 13;
				break;
			case SQL_BLOB:
			case SQL_ARRAY:
			default:
				len = 17;
				break;
		}
		if ((dtype == SQL_TEXT) || (dtype == SQL_VARYING))
			sprintf(p, "%-*s ", len, "NULL");
		else
			sprintf(p, "%*s ", len, "NULL");
	}
	else
	{
		switch (dtype)
		{
			case SQL_TEXT:
				sprintf(p, "%.*s ", var->sqllen, var->sqldata);
				break;

			case SQL_VARYING:
				vary2 = (VARY2*) var->sqldata;
				vary2->vary_string[vary2->vary_length] = '\0';
				sprintf(p, "%-*s ", var->sqllen, vary2->vary_string);
				break;

			case SQL_SHORT:
			case SQL_LONG:
			case SQL_INT64:
				{
				ISC_INT64	value;
				short		field_width;
				short		dscale;
				switch (dtype)
				    {
				    case SQL_SHORT:
					value = (ISC_INT64) *(short *) var->sqldata;
					field_width = 6;
					break;
				    case SQL_LONG:
					value = (ISC_INT64) *(int *) var->sqldata;
					field_width = 11;
					break;
				    case SQL_INT64:
					value = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
					field_width = 21;
					break;
				    }
				dscale = var->sqlscale;
				if (dscale < 0)
				    {
				    ISC_INT64	tens;
				    short	i;

				    tens = 1;
				    for (i = 0; i > dscale; i--)
					tens *= 10;

				    if (value >= 0)
					sprintf (p, "%*" ISC_INT64_FORMAT "d.%0*" ISC_INT64_FORMAT "d",
						field_width - 1 + dscale, 
						(ISC_INT64) value / tens,
						-dscale, 
						(ISC_INT64) value % tens);
				    else if ((value / tens) != 0)
					sprintf (p, "%*" ISC_INT64_FORMAT "d.%0*" ISC_INT64_FORMAT "d",
						field_width - 1 + dscale, 
						(ISC_INT64) (value / tens),
						-dscale, 
						(ISC_INT64) -(value % tens));
				    else
					sprintf (p, "%*s.%0*" ISC_INT64_FORMAT "d",
						field_width - 1 + dscale, 
						"-0",
						-dscale, 
						(ISC_INT64) -(value % tens));
				    }
				else if (dscale)
				    sprintf (p, "%*" ISC_INT64_FORMAT "d%0*d", 
					    field_width, 
					    (ISC_INT64) value,
					    dscale, 0);
				else
				    sprintf (p, "%*" ISC_INT64_FORMAT "d%",
					    field_width, 
					    (ISC_INT64) value);
				}
				break;

			case SQL_FLOAT:
				sprintf(p, "%15g ", *(float *) (var->sqldata));
				break;

			case SQL_DOUBLE:
				sprintf(p, "%24g ", *(double *) (var->sqldata));
				break;

			case SQL_TIMESTAMP:
				isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
				sprintf(date_s, "%04d-%02d-%02d %02d:%02d:%02d.%04d",
						times.tm_year + 1900,
						times.tm_mon+1,
						times.tm_mday,
						times.tm_hour,
						times.tm_min,
						times.tm_sec,
						((ISC_TIMESTAMP *)var->sqldata)->timestamp_time % 10000);
				sprintf(p, "%*s ", 24, date_s);
				break;

			case SQL_TYPE_DATE:
				isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
				sprintf(date_s, "%04d-%02d-%02d",
						times.tm_year + 1900,
						times.tm_mon+1,
						times.tm_mday);
				sprintf(p, "%*s ", 10, date_s);
				break;

			case SQL_TYPE_TIME:
				isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
				sprintf(date_s, "%02d:%02d:%02d.%04d",
						times.tm_hour,
						times.tm_min,
						times.tm_sec,
						(*((ISC_TIME *)var->sqldata)) % 10000);
				sprintf(p, "%*s ", 13, date_s);
				break;

			case SQL_BLOB:
			case SQL_ARRAY:
				bid = *(ISC_QUAD *) var->sqldata;
				sprintf(blob_s, "%08x:%08x", bid.gds_quad_high, bid.gds_quad_low);

				sprintf(p, "%17s ", blob_s);
				break;

			default:
				break;
		}
	}

	while (*p)
	{
		putchar(*p++);
	}
	
}


/*
 *	Prompt for and get input.
 *	Statements are terminated by a semicolon.
 */
int get_statement(char* buf)
{
	short	c;
	char	*p;
	int		cnt;

	p = buf;
	cnt = 0;
	printf("SQL> ");

	for (;;)
	{
		if ((c = getchar()) == EOFIND)
			return 0;

		if (c == '\n')
		{
			/* accept "quit" or "exit" to terminate application */

			if (!strncmp(buf, "exit;", 5))
				return 0;
			if (!strncmp(buf, "quit;", 5))
				return 0;

			/* Search back through any white space looking for ';'.*/
			while (cnt && isspace(*(p - 1)))
			{
				p--;
				cnt--;
			}
			if (*(p - 1) == ';')
			{
				*p++ = '\0';
				return 1;
			}

			*p++ = ' ';
			printf("CON> ");
		}
		else
			*p++ = c;
		cnt++;
	}
}

