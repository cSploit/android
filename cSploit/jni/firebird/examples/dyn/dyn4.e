/*
 *  Program type:   Embedded Dynamic SQL
 *
 *  Description:
 *		This program updates departments' budgets, given
 *		the department and the new budget information parameters.
 *
 *		An input SQLDA is allocated for the update query
 *		with parameter markers.
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

int get_input (char *dept_no, double *percent);

static char *Dept_data[] =
	{"622", "100", "116", "900", 0};

static double Percent_data[] =
	{0.05,  1.00,  0.075,  0.10, 0};

int	Input_ptr = 0;
char Db_name[128];

EXEC SQL
	SET DATABASE empdb = "employee.fdb" RUNTIME :Db_name;

char *upd_str =
	"UPDATE department SET budget = ? * budget + budget WHERE dept_no = ?";


int main(int argc, char** argv)
{
	BASED_ON department.dept_no	dept_no;
	double	percent_inc;
	short	nullind = 0;
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
		SET TRANSACTION USING empdb;

	/* Allocate an input SQLDA.  There are two unknown parameters. */
	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(2));
	sqlda->sqln = 2;
	sqlda->sqld = 2;
	sqlda->version = 1;

	/* Prepare the query. */
	EXEC SQL
		PREPARE q FROM :upd_str;

	/* Prepare the input sqlda, only data and indicator to set */

	EXEC SQL 
		DESCRIBE INPUT q USING SQL DESCRIPTOR sqlda;

	sqlda->sqlvar[0].sqldata = (char *) &percent_inc;
	sqlda->sqlvar[0].sqlind = &nullind;

	sqlda->sqlvar[1].sqldata = dept_no;
	/* FOrce the type to char instead of varchar */
	sqlda->sqlvar[1].sqltype = SQL_TEXT +1;
	sqlda->sqlvar[1].sqlind = &nullind;

	/* Expect an error, trap it */
	EXEC SQL WHENEVER SQLERROR CONTINUE;

	/*
	 *	Get the next department-percent increase input pair.
	 */
	while (get_input(dept_no, &percent_inc))
	{
		printf("\nIncreasing budget for department:  %s  by %5.2f percent.\n",
				dept_no, percent_inc);


		/* Update the budget. */
		EXEC SQL
			EXECUTE q USING SQL DESCRIPTOR sqlda;

		/* Don't save the update, if the new budget exceeds 
		** the limit. Detect an integrity violation 
		*/
		if (SQLCODE == -625) 
		{
			printf("\tExceeded budget limit -- not updated.\n");
			continue;
		}
		/* Undo all changes, in case of an error. */
		else if (SQLCODE)
			goto Error;

		/* Save each department's update independently. */
		EXEC SQL
			COMMIT RETAIN;
	}

	EXEC SQL
		COMMIT RELEASE;
	return (0);
Error:
	isc_print_status(gds__status);
	printf("SQLCODE=%d\n", SQLCODE);

	EXEC SQL
		ROLLBACK RELEASE;

	EXEC SQL
		DISCONNECT empdb;

	free( sqlda);
	return(1);
}


/*
 *	Get the department and percent parameters.
 */
int get_input(char* dept_no, double* percent)
{
	if (Dept_data[Input_ptr] == 0)
		return 0;

	strcpy(dept_no, Dept_data[Input_ptr]);

	if ((*percent = Percent_data[Input_ptr++]) == 0)
		return 0;

	return(1);
}

