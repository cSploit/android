/*
 *  Program type:  API Interface
 *
 *  Description:
 *        This program adds several departments with small default
 *        budgets, using 'execute immediate' statement.
 *        Then, a prepared statement, which doubles budgets for
 *        departments with low budgets, is executed.
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
#include <ibase.h>

#define MAXLEN      256
#define DNAMELEN    25
#define DNOLEN      3


int getins (char *, int);
int cleanup (void);



isc_db_handle    DB = 0;                /* database handle */
isc_tr_handle    trans = 0;             /* transaction handle */
ISC_STATUS_ARRAY status;                /* status vector */
char             Db_name[128];

int main (int argc, char** argv)
{
    int                 n = 0;
    char                exec_str[MAXLEN];
    char                prep_str[MAXLEN];
    isc_stmt_handle     double_budget = 0;    /* statement handle */

    if (argc > 1)
        strcpy(Db_name, argv[1]);
    else
        strcpy(Db_name, "employee.fdb");

    if (isc_attach_database(status, 0, Db_name, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    cleanup();

    /*
     *  Prepare a statement, which may be executed more than once.
     */

    strcpy(prep_str,
           "UPDATE DEPARTMENT SET budget = budget * 2 WHERE budget < 100000");

    /* Allocate a statement. */
    if (isc_dsql_allocate_statement(status, &DB, &double_budget))
    {
        ERREXIT(status, 1)
    }

    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Prepare the statement. */
    if (isc_dsql_prepare(status, &trans, &double_budget, 0, prep_str, 3, NULL))
    {
        ERREXIT(status, 1)
    }

    /*
     *  Add new departments, using 'execute immediate'.
     *  Build each 'insert' statement, using the supplied parameters.
     *  Since these statements will not be needed after they are executed,
     *  use 'execute immediate'.
     */
    
    while (getins(exec_str, n++))
    {
        printf("\nExecuting statement:\n%d:\t%s;\n", n, exec_str);

        if (isc_dsql_execute_immediate(status, &DB, &trans, 0, exec_str, 3, NULL))
        {
            ERREXIT(status, 1)
        }
    }   

    /*
     *    Execute a previously prepared statement.
     */
    printf("\nExecuting a prepared statement:\n\t%s;\n\n", prep_str);

    if (isc_dsql_execute(status, &trans, &double_budget, 3, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_free_statement(status, &double_budget, DSQL_drop))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    if (isc_detach_database(status, &DB))
    {
        ERREXIT(status, 1)
    }

    return 0;
}

/*
 *  Construct an 'insert' statement from the supplied parameters.
 */

int getins (char* line, int line_no)
{
    static char * Dept_data[] =
        {
            "117", "Field Office: Hong Kong",   "110",
            "118", "Field Office: Australia",   "110",
            "119", "Field Office: New Zealand", "110",
            0
        };

    char    dept_no[4];
    char    department[30];
    char    dept_head[4];

    if (Dept_data[3 * line_no] == 0)
        return 0;

    strcpy(dept_no, Dept_data[3 * line_no]);
    strcpy(department, Dept_data[3 * line_no + 1]);
    strcpy(dept_head, Dept_data[3 * line_no + 2]);

    sprintf(line, "INSERT INTO DEPARTMENT (dept_no, department, head_dept) \
            VALUES ('%s', '%s', '%s')", dept_no, department, dept_head);

    return 1;
}

/*
 *  Delete old data.
 */
int cleanup (void)
{
    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_execute_immediate(status, &DB, &trans, 0,
        "DELETE FROM department WHERE dept_no IN ('117', '118', '119')", 1, 0L))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    return 0;
}

