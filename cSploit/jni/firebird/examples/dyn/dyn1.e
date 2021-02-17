/*
 *	Program type:	Embedded Dynamic SQL
 *
 *	Description:
 *		This program creates a new database, using a static SQL string.
 *		The newly created database is accessed after its creation,
 *		and a sample table is added.
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

void pr_error (char *operation);

char    *new_dbname     = "new.fdb";

char    *create_tbl     = "CREATE TABLE dbinfo (when_created DATE)";
char    *insert_date    = "INSERT INTO dbinfo VALUES ('NOW')";


/*
 *  Declare a database handle, which will be used by the new database.
 */
EXEC SQL
	SET DATABASE db = COMPILETIME "employee.fdb";

int main(int argc, char** argv)
{

	db = NULL;

	/*
	 *  Create a new database, establishing a connection
	 *  as well.
	 */

	EXEC SQL
		EXECUTE IMMEDIATE "CREATE DATABASE 'new.fdb'";

	if (SQLCODE)
	{
		/* Print a descriptive message, if the database exists. */
		if (SQLCODE == -902)
		{
			printf("\nDatabase already exists.\n");
			printf("Remove %s before running this program.\n\n", new_dbname);
		}

		pr_error("create database");
		return 1;
	}

	EXEC SQL
		COMMIT RELEASE;

	if (SQLCODE)
	{
		pr_error("commit & release");
		return 1;
	}

	printf("Created database '%s'.\n\n", new_dbname);


	/*
	 *  Connect to the new database and create a sample table.
	 */

	/* Use the database handle declared above. */
	EXEC SQL
		CONNECT :new_dbname AS db;
	if (SQLCODE)
	{
		pr_error("connect database");
		return 1;
	}

	/* Create a sample table. */
	EXEC SQL
		SET TRANSACTION;
	EXEC SQL
		EXECUTE IMMEDIATE :create_tbl;
	if (SQLCODE)
	{
		pr_error("create table");
		return 1;
	}
	EXEC SQL
		COMMIT RETAIN;

	/* Insert 1 row into the new table. */
	EXEC SQL
		SET TRANSACTION;
	EXEC SQL
		EXECUTE IMMEDIATE :insert_date;
	if (SQLCODE)
	{
		pr_error("insert into");
		return 1;
	}
	EXEC SQL
		COMMIT RELEASE;

	printf("Successfully accessed the newly created database.\n\n");

	EXEC SQL
		DISCONNECT db;

	return 0;
}


/*
 *	Print the status, the SQLCODE, and exit.
 *	Also, indicate which operation the error occured on.
 */
void pr_error(char* operation)
{
	printf("[\n");
	printf("PROBLEM ON \"%s\".\n", operation);
		 
	isc_print_status(gds__status);

	printf("SQLCODE = %d\n", SQLCODE);
	
	printf("]\n");
}

