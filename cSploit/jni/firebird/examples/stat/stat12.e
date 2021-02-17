/*
 *  Program type:   Embedded Static SQL
 *
 *	Description:
 *		This program utilizes the event mechanism for processing
 *		newly entered sales orders.  It initializes an event called
 *		"new_order", and then loops waiting for and processing new
 *		orders as they come in.
 *
 *		When a new sales order is entered, a trigger defined in the
 *		database posts the "new_order" event.  When the program is
 *		notified of the event, it opens the cursor for selecting all
 *		orders with status "new".  For each fetched order, a transaction
 *		is started, which changes the order status from "new" to "open
 *		and takes some action to initiate order processing.  After all
 *		the new orders (if there were more than one) are processed, it
 *		goes back to waiting for new orders.
 *
 *		To trigger the event, while running this program, run stat12t.
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

int process_order (char *);

EXEC SQL
	BEGIN DECLARE SECTION;

EXEC SQL
	SET DATABASE empdb = "employee.fdb";

long	*t1;
long	*t2;

EXEC SQL
	END DECLARE SECTION;


int main(int argc, char** argv)
{
	int	ret = 0;
	BASED_ON sales.po_number pon;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	EXEC SQL
		CONNECT empdb;

	/* Go with read committed to see updates */
	EXEC SQL
		SET TRANSACTION READ COMMITTED;

	EXEC SQL
		DECLARE get_order CURSOR FOR
		SELECT po_number
		FROM sales
		WHERE order_status = "new"
		FOR UPDATE OF order_status;

	EXEC SQL
		EVENT INIT order_wait empdb ("new_order");

	while (!ret)
	{
		printf("\nStat 12 Waiting ...\n\n");
		EXEC SQL
			EVENT WAIT order_wait;

		EXEC SQL
			OPEN get_order;
		for (;;)	
		{
			EXEC SQL
				FETCH get_order INTO :pon;

			if (SQLCODE == 100)
				break;

			EXEC SQL
				UPDATE sales
				SET order_status = "open"
				WHERE CURRENT OF get_order;

			ret = process_order(pon);

		}
		EXEC SQL
			CLOSE get_order;

	}
	EXEC SQL 
		COMMIT;

	EXEC SQL
		DISCONNECT empdb;

	exit(0);
Error:
	isc_print_sqlerror(SQLCODE, gds__status);
	exit(1);
}


/*
 *	Initiate order processing for a newly received sales order.
 */
int process_order(char* pon)
{
	/*
	 *	This function would start a back-ground job, such as
	 *	sending the new orders to the printer, or generating
	 *	e-mail messages to the appropriate departments.
	 */

	printf("Stat12:  Received order:  %s\n", pon);
	if (!strncmp(pon, "VNEW4", 5))
	{
		printf ("Stat12: exiting\n");
		return 1;
	}
	return 0;
}

