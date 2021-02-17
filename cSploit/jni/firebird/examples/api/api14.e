/*
 *	Program type:  API Interface
 *
 *	Description:
 *		This program combines the three programming styles:
 *		static SQL, dynamic SQL, and the API interface.
 *
 *		Employee information is retrieved and printed for some set
 *		of employees.  A predefined set of columns is always printed.
 *		However, the 'where' clause, defining which employees the report
 *		is to be printed for, is unknown.
 *
 *		Dynamic SQL is utilized to construct the select statement.
 *		The 'where' clause is assumed to be passed as a parameter.
 *
 *		Static SQL is used for known SQL statements.
 *
 *		The API interface is used to access the two databases, and
 *		to control most transactions.  The database and transaction
 *		handles are shared between the three interfaces.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "example.h"

#define BUFLEN		1024

char    *sel_str =
    "SELECT full_name, dept_no, salary, job_code, job_grade, job_country \
	 FROM employee";

char	*where_str =
	"WHERE job_code = 'SRep' AND dept_no IN (110, 140, 115, 125, 123, 121)";

/* This macro is used to declare structures representing SQL VARCHAR types */
#define SQL_VARCHAR(len) struct {short vary_length; char vary_string[(len)+1];}

char	Db_name[128];

EXEC SQL
	SET DATABASE db1 = "employee.fdb" RUNTIME :Db_name;



int main(int argc, char** argv)
{
	BASED_ON employee.salary		salary;
	SQL_VARCHAR(5)				job_code;
	BASED_ON employee.job_grade		job_grade;
	SQL_VARCHAR(15)				job_country;
	SQL_VARCHAR(37)				full_name;
	BASED_ON country.currency		currency;
	BASED_ON department.dept_no		dept_no;
	BASED_ON department.department		department;
	char	buf[BUFLEN + 1];
	float	rate;
	void	*trans1 = NULL;			/* transaction handle */
	void	*trans2 = NULL;			/* transaction handle */
	ISC_STATUS_ARRAY status;
	XSQLDA	*sqlda;
        char    empdb2[128];

        if (argc > 1)
                strcpy(Db_name, argv[1]);
        else
                strcpy(Db_name, "employee.fdb");
        if (argc > 2)
                strcpy(empdb2, argv[2]);
        else
                strcpy(empdb2, "employe2.fdb");


	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/*
	 *	Set-up the select query.  The select portion of the query is
	 *	static, while the 'where' clause is determined elsewhere and
	 *	passed as a parameter.
	 */
	 sprintf(buf, "%s %s", sel_str, where_str);


	/*
	 *	Open the employee database.
	 */
	if (isc_attach_database(status, 0, Db_name, &db1, 0, NULL))
		isc_print_status(status);

	/*
	 *	Prepare the select query.
	 */

	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(6));
	sqlda->sqln = 6;
	sqlda->sqld = 6;
	sqlda->version = 1;

	EXEC SQL
		SET TRANSACTION USING db1;
	EXEC SQL
		PREPARE q INTO SQL DESCRIPTOR sqlda FROM :buf;
	EXEC SQL
		COMMIT ;
 
	sqlda->sqlvar[0].sqldata = (char *)&full_name;
	sqlda->sqlvar[0].sqltype = SQL_VARYING;

	sqlda->sqlvar[1].sqldata = dept_no;
	sqlda->sqlvar[1].sqltype = SQL_TEXT;

	sqlda->sqlvar[2].sqldata = (char *) &salary;
	sqlda->sqlvar[2].sqltype = SQL_DOUBLE;

	sqlda->sqlvar[3].sqldata = (char *)&job_code;
	sqlda->sqlvar[3].sqltype = SQL_VARYING;

	sqlda->sqlvar[4].sqldata = (char *) &job_grade;
	sqlda->sqlvar[4].sqltype = SQL_SHORT;

	sqlda->sqlvar[5].sqldata = (char *)&job_country;
	sqlda->sqlvar[5].sqltype = SQL_VARYING;


	/*
	 *	Open the second database.
	 */

	EXEC SQL
        	SET DATABASE db2 = "employe2.fdb";

	if (isc_attach_database(status, 0, empdb2, &db2, 0, NULL))
		isc_print_status(status);


	/*
	 *	Select the employees, using the dynamically allocated SQLDA.
	 */
 
	EXEC SQL
		DECLARE emp CURSOR FOR q;
	 
	if (isc_start_transaction(status, &trans1, 1, &db1, 0, NULL))
		isc_print_status(status);

	EXEC SQL
		OPEN TRANSACTION trans1 emp;

	while (SQLCODE == 0)
	{
		EXEC SQL
			FETCH emp USING SQL DESCRIPTOR sqlda;

		if (SQLCODE == 100)
			break;

		/*
		 *	Get the department name, using a static SQL statement.
		 */
		EXEC SQL
			SELECT TRANSACTION trans1 department
			INTO :department
			FROM department
			WHERE dept_no = :dept_no;

		/*
		 *	If the job country is not USA, access the second database
		 *	in order to get the conversion rate between different money
		 *	types.  Even though the conversion rate may fluctuate, all
		 *	salaries will be presented in US dollars for relative comparison.
		 */
		job_country.vary_string[job_country.vary_length] = '\0';
		if (strcmp(job_country.vary_string, "USA"))
		{
			EXEC SQL
				SELECT TRANSACTION trans1 currency
				INTO :currency
				FROM country
				WHERE country = :job_country.vary_string :job_country.vary_length;

			if (isc_start_transaction(status, &trans2, 1, &db2, 0, NULL))
				isc_print_status(status);

			EXEC SQL
				SELECT TRANSACTION trans2 conv_rate
				INTO :rate
				FROM cross_rate
				WHERE from_currency = 'Dollar'
				AND to_currency = :currency;

			if (!SQLCODE)
				salary = salary / rate;

			if (isc_commit_transaction (status, &trans2))
				isc_print_status(status);
		}

		/*
		 *	Print the results.
		 */

		printf("%-20.*s ", full_name.vary_length, full_name.vary_string);
		fflush (stdout);
		printf("%8.2f ", salary);
		fflush (stdout);
		printf("   %-5.*s %d", job_code.vary_length, job_code.vary_string, job_grade);
		fflush (stdout);
		printf("    %-20s\n", department);
		fflush (stdout);
	}
 
	EXEC SQL
		CLOSE emp;
	 
	if (isc_commit_transaction (status, &trans1))
		isc_print_status(status);

	isc_detach_database(status, &db1);
	isc_detach_database(status, &db2);
 
	return(0);


Error:
	printf("\n");
    isc_print_sqlerror(SQLCODE, isc_status);
    return(1);
}

