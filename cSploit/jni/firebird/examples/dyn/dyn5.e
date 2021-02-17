/*
 *  Program type:   Embedded Dynamic SQL
 *
 *	Description:
 *		This program demonstrates the reallocation of SQLDA and
 *		the 'describe' statement.  After a query is examined with
 *		'describe', an SQLDA of correct size is reallocated, and some
 *		information is printed about the query:  its type (select,
 *		non-select), the number of columns, etc.
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

char *sel_str =
	"SELECT department, mngr_no, location, head_dept \
	 FROM department WHERE head_dept in ('100', '900', '600')";

char Db_name[128];

EXEC SQL
	SET DATABASE empdb = "employee.fdb" RUNTIME :Db_name;


int main(int argc, char** argv)
{
	short	num_cols, i;
	XSQLDA	*sqlda;

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

	/* Allocate SQLDA of an arbitrary size. */
	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(4));
	sqlda->sqln = 4;
	sqlda->sqld = 4;
	sqlda->version = 1;

	/* Prepare an unknown statement. */
	EXEC SQL
		PREPARE q INTO SQL DESCRIPTOR sqlda FROM :sel_str;

	/* Describe the statement. */
	EXEC SQL
		DESCRIBE q INTO SQL DESCRIPTOR sqlda;

	/* This is a non-select statement, which can now be executed. */
	if (sqlda->sqld == 0)
		return(0);
	
	/* If this is a select statement, print more information about it. */
	else
		printf("Query Type:  SELECT\n\n");
	
	num_cols = sqlda->sqld;

	printf("Number of columns selected:  %d\n", num_cols);

	/* Reallocate SQLDA if necessary. */
	if (sqlda->sqln < sqlda->sqld)
	{
		free(sqlda);

		sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(num_cols));
		sqlda->sqln = num_cols;
		sqlda->sqld = num_cols;

		/* Re-describe the statement. */
		EXEC SQL
			DESCRIBE q INTO SQL DESCRIPTOR sqlda;

		num_cols = sqlda->sqld;
	}

	/* List column names, types, and lengths. */
	for (i = 0; i < num_cols; i++)
	{
		printf("\nColumn name:    %s\n", sqlda->sqlvar[i].sqlname);
		printf("Column type:    %d\n", sqlda->sqlvar[i].sqltype);
		printf("Column length:  %d\n", sqlda->sqlvar[i].sqllen);
	}

	EXEC SQL
		COMMIT;

	EXEC SQL
		DISCONNECT empdb;

	free( sqlda);
	return(0);

Error:
	isc_print_status(gds__status);
	printf("SQLCODE=%d\n", SQLCODE);
	return(1);
}

