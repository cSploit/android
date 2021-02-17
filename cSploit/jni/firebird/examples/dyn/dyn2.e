/*
 *	Program type:	Embedded Dynamic SQL
 *
 *	Description:
 *		This program adds several departments with small default
 *		budgets, using 'execute immediate' statement.
 *		Then, a prepared statement, which doubles budgets for
 *		departments with low budgets, is executed.
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

#define	MAXLEN		256

int get_line (char *line);
void clean_up (void);

static char *Dept_data[] =
{
	"117", "Field Office: Hong Kong",   "110",
	"118", "Field Office: Australia",   "110",
	"119", "Field Office: New Zealand", "110",
	0
};
int Dept_ptr = 0;
char Db_name[128];

EXEC SQL 
	SET DATABASE empdb = "employee.fdb" RUNTIME :Db_name;


int main(int argc, char** argv)
{
	BASED_ON department.department	dept_name;
	BASED_ON department.dept_no		dept_id;
	char	exec_str[MAXLEN], prep_str[MAXLEN];

	if (argc > 1)
		strcpy(Db_name, argv[1]);
	else
		strcpy(Db_name, "employee.fdb");

	EXEC SQL
		WHENEVER SQLERROR GO TO MainError;

	EXEC SQL
		CONNECT empdb;

	clean_up();


	EXEC SQL
		SET TRANSACTION;

	/*
	 *	Prepare a statement, which may be executed more than once.
	 */
	strcpy(prep_str,
		"UPDATE DEPARTMENT SET budget = budget * 2 WHERE budget < 100000");

	EXEC SQL
		PREPARE double_small_budget FROM :prep_str;

	/*
	 *  Add new departments, using 'execute immediate'.
	 *  Build each 'insert' statement, using the supplied parameters.
	 *  Since these statements will not be needed after they are executed,
	 *  use 'execute immediate'.
	 */


	while (get_line(exec_str))
	{
		printf("\nExecuting statement:\n\t%s;\n", exec_str);

		EXEC SQL
			EXECUTE IMMEDIATE :exec_str;
	}

	EXEC SQL
		COMMIT RETAIN;

	/*
	 *	Execute a previously prepared statement.
	 */
	printf("\nExecuting a prepared statement:\n\t%s;\n\n", prep_str);

	EXEC SQL
		EXECUTE double_small_budget;

	EXEC SQL
		COMMIT;

	EXEC SQL
		DISCONNECT empdb;

	exit(0);

MainError:
	EXEC SQL
		WHENEVER SQLERROR CONTINUE;

	isc_print_status(gds__status);
	printf("SQLCODE=%d\n", SQLCODE);
	EXEC SQL ROLLBACK;
     
        exit(1);

}


/*
 *  Construct an 'insert' statement from the supplied parameters.
 */
int get_line(char* line)
{
	if (Dept_data[Dept_ptr] == 0)
		return 0;
	if (Dept_data[Dept_ptr + 1] == 0)
		return 0;
	if (Dept_data[Dept_ptr + 2] == 0)
		return 0;

	sprintf(line, "INSERT INTO DEPARTMENT (dept_no, department, head_dept) \
			   VALUES ('%s', '%s', '%s')", Dept_data[Dept_ptr],
			   Dept_data[Dept_ptr + 1], Dept_data[Dept_ptr + 2]);
	Dept_ptr += 3;

	return(1);
}


/*
 *	Delete old data.
 */
void clean_up (void)
{
	EXEC SQL WHENEVER SQLERROR GO TO CleanErr;

	EXEC SQL SET TRANSACTION;

	EXEC SQL EXECUTE IMMEDIATE
		"DELETE FROM department WHERE dept_no IN ('117', '118', '119')";

	EXEC SQL COMMIT;
	return;

CleanErr:

	isc_print_status(gds__status);
	printf("SQLCODE=%d\n", SQLCODE);
	exit(1);
}

