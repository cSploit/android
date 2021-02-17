/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program executes a stored procedure and selects from
 *		a stored procedure.  First, a list of projects an employee
 *		is involved in is printed.  Then the employee is added to
 *		another project.  The new list of projects is printed again.
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

EXEC SQL INCLUDE SQLCA;

void select_projects (short emp_no);
void get_params (short *emp_no, char* proj_id);
void pr_error (long status);
void add_emp_proj (short emp_no,char * proj_id);



int main (void)
{
	BASED_ON employee.emp_no	emp_no;
	BASED_ON project.proj_id	proj_id;

	/*
	 *	Add employee with id 8 to project 'MAPDB'.
	 */
	get_params(&emp_no, proj_id);

	/*
	 *	Display employee's current projects.
	 */
	printf("\nCurrent projects for employee id: %d\n\n", emp_no);
	select_projects(emp_no);

	/*
	 *	Insert a new employee project row.
	 */
	printf("\nAdd employee id: %d to project: %s\n", emp_no, proj_id);
	add_emp_proj(emp_no, proj_id);

	/*
	 *	Display employee's new current projects.
	 */
	printf("\nCurrent projects for employee id: %d\n\n", emp_no);
	select_projects(emp_no);

}


/*
 *	Select from a stored procedure.
 *	Procedure 'get_emp_proj' gets employee's projects.
 */
void select_projects(BASED_ON employee.emp_no emp_no)
{
	BASED_ON project.proj_id	proj_id;

	EXEC SQL
		WHENEVER SQLERROR GO TO SelError;

	/* Declare a cursor on the stored procedure. */
	EXEC SQL
		DECLARE projects CURSOR FOR
		SELECT proj_id FROM get_emp_proj (:emp_no)
		ORDER BY proj_id;

	EXEC SQL
		OPEN projects;
	
	/* Print employee projects. */
	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH projects INTO :proj_id;

		if (SQLCODE == 100)
			break;

		printf("\t%s\n", proj_id);
	}

	EXEC SQL
		CLOSE projects;
	
	EXEC SQL
		COMMIT RETAIN;

SelError:
	if (SQLCODE)
		pr_error((long)gds__status);
}


/*
 *	Execute a stored procedure.
 *	Procedure 'add_emp_proj' adds an employee to a project.
 */
void add_emp_proj(BASED_ON employee.emp_no emp_no, BASED_ON project.proj_id proj_id)
{
	EXEC SQL
		WHENEVER SQLERROR GO TO AddError;

	EXEC SQL
		EXECUTE PROCEDURE add_emp_proj :emp_no, :proj_id;

	EXEC SQL
		COMMIT;

AddError:
	if (SQLCODE) 
		pr_error((long)gds__status);
}


/*
 *	Set-up procedure parameters and clean-up old data.
 */
void get_params(BASED_ON employee.emp_no *emp_no, BASED_ON project.proj_id proj_id)
{
	*emp_no = 8;
	strcpy(proj_id, "MAPDB");

	EXEC SQL
		WHENEVER SQLERROR GO TO CleanupError;

	/* Cleanup:  delete row from the previous run. */
	EXEC SQL
		DELETE FROM employee_project
		WHERE emp_no = 8 AND proj_id = "MAPDB";

CleanupError:
	return;
}


/*
 *	Print an error message.
 */
void pr_error(long status)
{
	isc_print_sqlerror(SQLCODE, gds__status);
}

