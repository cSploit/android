/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program declares a cursor, opens the cursor, and loops
 *		fetching multiple rows.  All departments that need to hire
 *		a manager are selected and displayed.
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

EXEC SQL
	BEGIN DECLARE SECTION;
EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	BASED_ON department.department	department;
	BASED_ON department.department	parent_dept;
	BASED_ON department.location	location;

	/* Trap all errors. */
	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/* Trap SQLCODE = -100 (end of file reached during a fetch). */
	EXEC SQL
		WHENEVER NOT FOUND GO TO AllDone;

	/* Ignore all warnings. */
	EXEC SQL
		WHENEVER SQLWARNING CONTINUE;

	/* Declare the cursor for selecting all departments without a manager. */
	EXEC SQL
		DECLARE to_be_hired CURSOR FOR
		SELECT d.department, d.location, p.department
		FROM department d, department p
		WHERE d.mngr_no IS NULL
		AND d.head_dept = p.dept_no;

	/* Open the cursor. */
	EXEC SQL
		OPEN to_be_hired;

	printf("\n%-25s %-15s %-25s\n\n",
				"DEPARTMENT", "LOCATION", "HEAD DEPARTMENT");

	/*
	 *	Select and display all rows.
	 */
	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH to_be_hired INTO :department, :location, :parent_dept;

		/* 
		 *  If FETCH returns with -100, the processing will jump
		 *  to AllDone before the following printf is executed.
		 */
		
		printf("%-25s %-15s %-25s\n", department, location, parent_dept);
	}

	/*
	 *	Close the cursor and release all resources.
	 */
AllDone:
	EXEC SQL
		CLOSE to_be_hired;

	EXEC SQL
		COMMIT RELEASE;

	return 0;

	/*
	 *	Print the error, and exit.
	 */
Error:
	isc_print_sqlerror((short)SQLCODE, gds__status);
	return 1;
}

