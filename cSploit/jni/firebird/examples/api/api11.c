/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program executes a stored procedure and selects from
 *        a stored procedure.  First, a list of projects an employee
 *        is involved in is printed.  Then the employee is added to
 *        another project.  The new list of projects is printed again.
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
#include <ibase.h>
#include <stdio.h>
#include "example.h"

#define    PROJLEN        5
#define    BUFLEN        128

int select_projects (isc_db_handle db, int emp_no);
int add_emp_proj (isc_db_handle db, int emp_no, char * proj_id);
int get_params (isc_db_handle db, int * emp_no, char * proj_id);

int main (int argc, char** argv)
{
    int             emp_no;
    char            proj_id[PROJLEN + 2];
    isc_db_handle    db = NULL;
    ISC_STATUS_ARRAY    status;
    char            empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

    if (isc_attach_database(status, 0, empdb, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /*
     *  Add employee with id 8 to project 'MAPDB'.
     */
    if (get_params(db, &emp_no, proj_id))
        return 1;

    /*
     *  Display employee's current projects.
     */
    printf("\nCurrent projects for employee id: %d\n\n", emp_no);
    if (select_projects(db, emp_no))
        return 1;

    /*
     *  Insert a new employee project row.
     */
    printf("\nAdd employee id: %d to project: %s\n", emp_no, proj_id);
    if (add_emp_proj(db, emp_no, proj_id))
        return 1;

    /*
     *  Display employee's new current projects.
     */
    printf("\nCurrent projects for employee id: %d\n\n", emp_no);
    if (select_projects(db, emp_no))
        return 1;
 

    if (isc_detach_database(status, &db))
    {
        ERREXIT(status, 1)
    }

    return  0 ;
}

/*
 *    Select from a stored procedure.
 *    Procedure 'get_emp_proj' gets employee's projects.
 */
int select_projects (isc_db_handle db, int emp_no)
{
    char            proj_id[PROJLEN + 2];
    char            selstr[BUFLEN];
    short           flag0 = 0;
    isc_tr_handle   trans = NULL;
    ISC_STATUS_ARRAY    status;
    isc_stmt_handle stmt = NULL;
    XSQLDA          *sqlda;
    long            fetch_stat;

    sprintf(selstr, "SELECT proj_id FROM get_emp_proj (%d) ORDER BY proj_id",
            emp_no);

    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda->sqln = 1;
    sqlda->version = 1;

    if (isc_start_transaction(status, &trans, 1, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_allocate_statement(status, &db, &stmt))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_prepare(status, &trans, &stmt, 0, selstr, 1, sqlda))
    {
        ERREXIT(status, 1)
    }

    sqlda->sqlvar[0].sqldata = (char *) proj_id;
    sqlda->sqlvar[0].sqlind  = &flag0;
    sqlda->sqlvar[0].sqltype = SQL_TEXT + 1;

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, sqlda)) == 0)
    {
        proj_id[PROJLEN] = '\0';
        printf("\t%s\n", proj_id);
    }

    if (fetch_stat != 100L)
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
    {
        ERREXIT(status, 1)
    }
                              
    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    free(sqlda);

    return 0;
}

/*
 *    Execute a stored procedure.
 *    Procedure 'add_emp_proj' adds an employee to a project.
 */
int add_emp_proj (isc_db_handle db, int emp_no, char *proj_id)
{
    char            exec_str[BUFLEN];
    isc_tr_handle   trans = NULL;
    ISC_STATUS_ARRAY    status;

    sprintf(exec_str, "EXECUTE PROCEDURE add_emp_proj %d, \"%s\"",
            emp_no, proj_id);

    if (isc_start_transaction(status, &trans, 1, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_execute_immediate(status, &db, &trans, 0, exec_str, 1, NULL))
    {
        ERREXIT(status, 1)
    }
 
    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT (status, 1)
    }

    return 0;
}   

/*
 *    Set-up procedure parameters and clean-up old data.
 */
int get_params (isc_db_handle db, int *emp_no, char *proj_id)
{
    isc_tr_handle   trans = NULL;
    ISC_STATUS_ARRAY    status;

    *emp_no = 8;
    strcpy(proj_id, "MAPDB");
                     
    /* Cleanup:  delete row from the previous run. */

    if (isc_start_transaction(status, &trans, 1, &db, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_execute_immediate(status, &db, &trans, 0,
                                   "DELETE FROM employee_project \
                                   WHERE emp_no = 8 AND proj_id = 'MAPDB'", 1,
                                   NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    return 0;
}

