/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program demonstrates a singleton select.
 *		A full name and phone number are displayed for
 *		the CEO of the company.
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

#define	FIRSTLEN	15
#define	LASTLEN		20
#define EXTLEN		4
#define DEPTNO		3
#define PHONELEN	20

EXEC SQL
	BEGIN DECLARE SECTION;
EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	char first[FIRSTLEN + 1];
	char last[LASTLEN + 1];
	char ext[EXTLEN + 1];
	char phone[PHONELEN + 1];
	char dept[DEPTNO + 1];

	/*
	 *  Assume there's only one CEO.
	 *  Select the name and phone extension.
	 */
	EXEC SQL
		SELECT first_name, last_name, phone_ext, dept_no
		INTO :first, :last, :ext, :dept
		FROM employee
		WHERE job_code = 'CEO';

	/* Check the SQLCODE to make sure only 1 row was selected. */
	if (SQLCODE)
	{
		isc_print_sqlerror((short)SQLCODE, gds__status);
		exit(1);
	}

	/*
	 *  Also, select the department phone number.
	 */

	EXEC SQL
		SELECT phone_no
		INTO :phone
		FROM department
		WHERE dept_no = :dept;

	if (SQLCODE)
	{
		isc_print_sqlerror((short)SQLCODE, gds__status);
		exit(1);
	}

	printf("President:  %s %s\t\t", first, last);
	printf("Phone #:  %s  x%s\n", phone, ext);

	EXEC SQL
		COMMIT RELEASE;
	exit(0);
}

