/*
 *  Program type:   Embedded Static SQL
 *	
 *  Description:
 *		This program selects an array data type.
 *		Projected head count is displayed for the 4
 *		quarters of some fiscal year for some project,
 *		ordered by department name.
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
	BASED_ON department.department				department;
	BASED_ON proj_dept_budget.quart_head_cnt	hcnt;
	BASED_ON proj_dept_budget.quart_head_cnt	tot;

	int		fiscal_year = 1994;				/* year parameter */
	char	*project = "VBASE";				/* project parameter */
	short	i;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/*
	 *	Declare a cursor for selecting an array, given 2
	 *	2 parameters (year and project).
	 */
	EXEC SQL
		DECLARE proj_cnt CURSOR FOR
		SELECT department, quart_head_cnt[]
		FROM proj_dept_budget p, department d
		WHERE p.dept_no = d.dept_no
		AND year = :fiscal_year
		AND proj_id = :project
		ORDER BY department;

	printf("\n\t\t\tPROJECTED HEAD-COUNT REPORT\n");
	printf("\t\t\t      (by department)\n\n");
	printf("FISCAL YEAR:  %d\n", fiscal_year);
	printf("PROJECT ID :  %s\n\n\n", project);

	printf("%-25s%10s%10s%10s%10s\n\n",
			"DEPARTMENT", "QTR1", "QTR2", "QTR3", "QTR4");

	/* Initialize quarterly totals. */
	for (i = 0; i < 4; i++)	
		tot[i] = 0;
	
	EXEC SQL
		OPEN proj_cnt;

	/* Get and display each department's counts. */
	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH proj_cnt INTO :department, :hcnt;

		if (SQLCODE == 100)
			break;

		printf("%-25s%10ld%10ld%10ld%10ld\n",
					department, hcnt[0], hcnt[1], hcnt[2], hcnt[3]);
		
		for (i = 0; i < 4; i++)
			tot[i] += hcnt[i];
	}

	/* Display quarterly totals. */
	printf("\n%-25s%10ld%10ld%10ld%10ld\n\n",
					"TOTAL", tot[0], tot[1], tot[2], tot[3]);

	EXEC SQL
		CLOSE proj_cnt;

	EXEC SQL
		COMMIT WORK;

	return 0;
		
Error:
	isc_print_sqlerror((short) SQLCODE, gds__status);
	return 1;
}

