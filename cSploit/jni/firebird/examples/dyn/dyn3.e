/*
 *	Program type:   Embedded Dynamic SQL
 *
 *	Description:
 *		This program displays employee names and phone extensions.
 *
 *		It allocates an output SQLDA, declares and opens a cursor,
 *		and loops fetching multiple rows.
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
#include <stdlib.h>
#include <string.h>

void print_error (void);

char	*sel_str =
	"SELECT last_name, first_name, phone_ext FROM phone_list \
	WHERE location = 'Monterey' ORDER BY last_name, first_name;";

/* This macro is used to declare structures representing SQL VARCHAR types */
#define SQL_VARCHAR(len) struct {short vary_length; char vary_string[(len)+1];}

char Db_name[128];

EXEC SQL
	SET DATABASE empdb = "employee.fdb" RUNTIME :Db_name;


int main(int argc, char** argv)
{

	SQL_VARCHAR(15) first_name;
	SQL_VARCHAR(20) last_name;
	char phone_ext[6];

	XSQLDA	*sqlda;
	short	flag0 = 0, flag1 = 0, flag2 = 0;

	if (argc > 1)
                strcpy(Db_name, argv[1]);
        else
                strcpy(Db_name, "employee.fdb");

	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	EXEC SQL
		CONNECT empdb;

	EXEC SQL
		SET TRANSACTION;

	/* Allocate an output SQLDA. */
	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(3));
	sqlda->sqln = 3;
	sqlda->version = 1;

	/* Prepare the query. */
	EXEC SQL
		PREPARE q INTO SQL DESCRIPTOR sqlda FROM :sel_str;

	/*
	 *  Although, all three selected columns are of type varchar, the
	 *  third field's type is changed and printed as type TEXT.
	 */

	sqlda->sqlvar[0].sqldata = (char *)&last_name;
	sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
	sqlda->sqlvar[0].sqlind = &flag0;

	sqlda->sqlvar[1].sqldata = (char *)&first_name;
	sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
	sqlda->sqlvar[1].sqlind = &flag1;

	sqlda->sqlvar[2].sqldata = phone_ext;
	sqlda->sqlvar[2].sqltype = SQL_TEXT + 1;
	sqlda->sqlvar[2].sqlind = &flag2;

	/* Declare the cursor for the prepared query. */
	EXEC SQL
		DECLARE s CURSOR FOR q;

	EXEC SQL
		OPEN s;

	printf("\n%-20s %-15s %-10s\n\n", "LAST NAME", "FIRST NAME", "EXTENSION");

	/*
	 *  Fetch and print the records.
	 */
	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH s USING SQL DESCRIPTOR sqlda;

		if (SQLCODE == 100)
			break;

		printf("%-20.*s ", last_name.vary_length, last_name.vary_string);

		printf("%-15.*s ", first_name.vary_length, first_name.vary_string);

		phone_ext[sqlda->sqlvar[2].sqllen] = '\0';
		printf("%-10s\n", phone_ext);
	}

	EXEC SQL
		CLOSE s;

	EXEC SQL
		COMMIT;

	EXEC SQL
		DISCONNECT empdb;

	free( sqlda);
	return(0);
		
Error:
	print_error();
}

void print_error (void)
{
	isc_print_status(gds__status);
	printf("SQLCODE=%d\n", SQLCODE);
}

