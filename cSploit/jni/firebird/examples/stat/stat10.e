/*
 *	Program type:   Embedded Static SQL
 *
 *	Description:
 *		This program demonstrates 'set database', 'connect',
 *		and 'set transaction' statements.
 *		Each time a database is connected to, a sample table
 *		is accessed as a test.
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
#include <stdio.h>

int count_types (void);
int count_records (void);
long pr_error (void);

char *dbname = "employee.fdb";

EXEC SQL INCLUDE SQLCA;



int main  (void)
{
	/*
	 *	Declare 2 database handles for future use.
	 */

	EXEC SQL
		SET DATABASE db1 = "employee.fdb";

	EXEC SQL
		SET DATABASE db2 = "employe2.fdb";


	/*
	 *	Open a single database.
	 */

	printf("\n1. Opening database employee.fdb.\n");

	EXEC SQL
		CONNECT db1;

	if( pr_error())
	    return 1;;

	EXEC SQL
		SET TRANSACTION USING db1;

	if (count_types())
		return 1;

	EXEC SQL
		COMMIT RELEASE;

	EXEC SQL
		DISCONNECT db1;


	/*
	 *	Use a database name supplied at run-time.
	 *	Connect to this database using an existing database handle.
	 */

	printf("\n2. Opening database with name: %s supplied at run-time.\n",
					dbname);
	EXEC SQL
		CONNECT TO :dbname AS db1;

	if( pr_error())
	    return 1;;

	EXEC SQL
		SET TRANSACTION USING db1;

	if( count_types())
		return 1;

	EXEC SQL
		COMMIT RELEASE;

	EXEC SQL
		DISCONNECT DEFAULT;


	/*
	 *	Open the second database within the same program,
	 *	while the first database remains disconnected.
	 */
	printf("\n3. Opening a second database after closing the first one.\n");

	EXEC SQL
		CONNECT db2;

	if( pr_error())
	    return 1;;

	EXEC SQL
		SET TRANSACTION USING db2;

	if (count_records())
		return 1;

	EXEC SQL
		COMMIT RELEASE;

	EXEC SQL
		DISCONNECT db2;


	/*
	 *	Open two databases simultaneously.
	 */
	printf("\n4. Opening two databases simultaneously.\n");

	EXEC SQL
		CONNECT TO db1, db2;

	if( pr_error())
	    return 1;;

	EXEC SQL
		SET TRANSACTION USING db1, db2;

	if (count_types())
		return 1;;
	if (count_records())
		return 1;

	EXEC SQL
		COMMIT RELEASE;

	EXEC SQL
		DISCONNECT db1, db2;


	/*
	 *	Open all databases (in this case just two) at the same time.
	 */
	printf("\n5. Opening all databases.\n");

	EXEC SQL
		CONNECT TO ALL;

	if( pr_error())
	    return 1;;

	EXEC SQL
		SET TRANSACTION;

	if (count_types())
		return 1;
	if (count_records())
		return 1;

	EXEC SQL
		COMMIT RELEASE;

	EXEC SQL
		DISCONNECT ALL;
	return (0);
}


/*
 *	Access a table in database 1.
 */
int count_types (void)
{
	int cnt;

	EXEC SQL
		SELECT COUNT(DISTINCT currency) INTO :cnt FROM country;

	if (SQLCODE == 0)
		printf("\tNumber of currency types    :  %d\n", cnt);
	else 
		if( pr_error())
		    return 1;;
	return (0);
}


/*
 *	Access a table in database 2.
 */
int count_records (void)
{
	int cnt;

	/* Use the database handle along with the table name. */
	EXEC SQL
		SELECT COUNT(DISTINCT to_currency) INTO :cnt FROM db2.cross_rate;

	if (SQLCODE == 0)
		printf("\tNumber of conversion records:  %d\n", cnt);
	else 
		if( pr_error())
			return 1;;
	return (0);
}


/*
 *	Print an error message.
 */
long pr_error (void)
{
	if (SQLCODE)
	{
		isc_print_sqlerror(SQLCODE, isc_status);
		printf("\n");
	}
	return (SQLCODE);
}

