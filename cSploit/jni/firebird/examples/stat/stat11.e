/*
 *  Program type:   Embedded Static SQL
 *
 *	Description:
 *		This program demonstrates 'set transaction' statements
 *		with the three isolation options:
 *
 *			- snapshot
 *			- read committed
 *			- shapshot table stability.
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
#include <stdio.h>

EXEC SQL
	BEGIN DECLARE SECTION;

long *t1;
long *t2;
long *t3;
char Db_name[128];

EXEC SQL
	SET DATABASE empdb = COMPILETIME "employee.fdb" RUNTIME :Db_name;

int		cust_no;
int		tot;
char	ord_stat[8];

EXEC SQL
	END DECLARE SECTION;



int main(int argc, char** argv)
{

	if (argc > 1)
		strcpy (Db_name, argv[1]);
	else
		strcpy (Db_name, "employee.fdb");
	/* Connect to the database. */

	EXEC SQL 
		WHENEVER SQLERROR GOTO :err;
	EXEC SQL
		CONNECT empdb;

	/*
	 *	Start a transaction with SNAPSHOT isolation option.
	 *	This transaction wants to see a stable, unchanging view
	 *	of the sales orders, while it computes the totals.
	 *	Name the transaction t1.
	 */

	printf("Starting a transaction with SNAPSHOT isolation option.\n\n");

	EXEC SQL
		SET TRANSACTION NAME t1 READ WRITE SNAPSHOT;

	EXEC SQL
		DECLARE s CURSOR FOR
		SELECT cust_no, SUM(qty_ordered)
		FROM sales GROUP BY cust_no;

	EXEC SQL
		OPEN TRANSACTION t1 s;
	EXEC SQL
		FETCH s INTO :cust_no, :tot;		/* get the first row only */
	if (!SQLCODE)
		printf("\tCustomer: %ld    Quantity Ordered: %ld\n\n", cust_no, tot);
	EXEC SQL
		CLOSE s;

	EXEC SQL
		COMMIT TRANSACTION t1;


	/*
	 *	Start a transaction with READ COMMITTED isolation option.
	 *	This transaction wants to see changes for the order status
	 *	as they come in.
	 *	Name the transaction t2.
	 */

	printf("Starting a transaction with READ COMMITTED isolation option.\n\n");

	EXEC SQL
		SET TRANSACTION NAME t2 READ WRITE READ COMMITTED;

	EXEC SQL
		DECLARE c CURSOR FOR
		SELECT cust_no, order_status
		FROM sales
		WHERE order_status IN ("open", "shipping");

	EXEC SQL
		OPEN TRANSACTION t2 c;
	EXEC SQL
		FETCH c INTO :cust_no, :ord_stat;	/* get the first row only */
	if (!SQLCODE)
		printf("\tCustomer number: %ld   Status: %s\n\n", cust_no, ord_stat);
		
	EXEC SQL
		CLOSE c;

	EXEC SQL
		COMMIT TRANSACTION t2;


	/*
	 *	Start a transaction with SNAPSHOT TABLE STABILITY isolation
	 *	option.  This transaction wants to lock out all other users
	 *	from making any changes to the customer table, while it goes
	 *	through and updates customer status.
	 *	Name the transaction t3.
	 */

	printf("Starting a transaction with SNAPSHOT TABLE STABILITY.\n\n");

	EXEC SQL
		SET TRANSACTION NAME t3 READ WRITE SNAPSHOT TABLE STABILITY;

	EXEC SQL
		DECLARE h CURSOR FOR
		SELECT cust_no
		FROM customer
		WHERE on_hold = '*'
		FOR UPDATE OF on_hold;

	EXEC SQL
		OPEN TRANSACTION t3 h;
	EXEC SQL
		FETCH h INTO :cust_no;				/* get the first row only */
	if (!SQLCODE)
		printf("\tCustomer on hold: %ld\n\n", cust_no);
		
	EXEC SQL
		CLOSE h;

	EXEC SQL
		COMMIT TRANSACTION t3;


	EXEC SQL
		DISCONNECT empdb;
	return (0);
err:
	isc_print_status(isc_status);
	return 1;
}

