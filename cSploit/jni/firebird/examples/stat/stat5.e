/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program performs a positioned update.
 *		All job grades are selected, and the salary range
 *		for the job may be increased by some factor, if any
 *		of the employees have a salary close to the upper
 *		limit of their job grade.
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

BASED_ON job.job_code		job;
BASED_ON job.job_grade		grade;
BASED_ON job.job_country	country;
BASED_ON job.max_salary		max_salary;

EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	char	jobstr[25];
	float	mult_factor;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/* Declare the cursor, allowing for the update of max_salary field. */
	EXEC SQL
		DECLARE sal_range CURSOR FOR
		SELECT job_grade, job_code, job_country, max_salary
		FROM job
		FOR UPDATE OF max_salary;

	EXEC SQL
		OPEN sal_range;

	printf("\nIncreasing maximum salary limit for the following jobs:\n\n");
	printf("%-25s%-22s%-22s\n\n", "  JOB NAME", "CURRENT MAX", "NEW MAX");

	for (;;)
	{
		EXEC SQL
			FETCH sal_range INTO :grade, :job, :country, :max_salary;

		if (SQLCODE == 100)
			break;

		/* Check if any of the employees in this job category are within
		 * 10% of the maximum salary.
		 */
		EXEC SQL
			SELECT salary
			FROM employee
			WHERE job_grade = :grade
			AND job_code = :job
			AND job_country = :country
			AND salary * 0.1 + salary > :max_salary;

		/* If so, increase the maximum salary. */
		if (SQLCODE == 0)
		{
			/* Determine the increase amount;  for example, 5%. */
			mult_factor = 0.05;

			sprintf(jobstr, "%s %d  (%s)", job, grade, country);
			printf("%-25s%10.2f%20.2f\n", jobstr,
						max_salary, max_salary * mult_factor + max_salary);

			EXEC SQL
				UPDATE job
				SET max_salary = :max_salary + :max_salary * :mult_factor
				WHERE CURRENT OF sal_range;
		}
	}

	printf("\n");

	EXEC SQL
		CLOSE sal_range;

	/* Don't actually save the changes. */
	EXEC SQL
		ROLLBACK RELEASE;

	return  0;

Error:
	isc_print_sqlerror(SQLCODE, gds__status);
	return  1 ;
}

