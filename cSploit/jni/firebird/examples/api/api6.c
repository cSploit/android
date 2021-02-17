/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program performs a positioned update.
 *        Department budgets are examined and updated using some
 *        percent increase factor, determined at run-time.
 *
 *        The update statement is constructed using a dynamic cursor
 *        name.  The statement handle is freed and re-used by the
 *        update cursor, after being used by another statement.
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

#define    DEPTLEN        3
#define    PROJLEN        5
#define    BUFLEN        256

float increase_factor (double budget);

/*
 *  A cursor is declared on this select statement, allowing for
 *  the update of projected_budget field.
 */
char *sel_str =
    "SELECT proj_id, dept_no, projected_budget \
     FROM proj_dept_budget WHERE fiscal_year = 1994 \
     FOR UPDATE OF projected_budget";

/* This query is executed prior to the positioned update. */
char *tot_str =
    "SELECT SUM(projected_budget) FROM proj_dept_budget WHERE fiscal_year = 1994";



int main (int argc, char** argv)
{
    char                dept_no[DEPTLEN + 2];
    char                proj_id[PROJLEN + 2];
    char                upd_str[BUFLEN];
    double              budget;
    double              tot_budget;
    short               flag0 = 0,
                        flag1 = 0,
                        flag2 = 0,
                        flag3 = 0;
    isc_db_handle       DB = NULL;              /* Database handle */
    isc_tr_handle       trans = NULL;           /* transaction handle */
    ISC_STATUS_ARRAY    status;                 /* status vector */
    char                *cursor = "budget";     /* dynamic cursor name */
    isc_stmt_handle     stmt = NULL;            /* statement handle */
    XSQLDA  *           osqlda;                 /* output SQLDA */
    XSQLDA  *           isqlda;                 /* input SQLDA */
    long                fetch_stat;
    char                empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

 
    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /*
     *    Prepare and execute the first select statement.
     *    Free the statement handle, when done.
     */

    if (isc_dsql_allocate_statement(status, &DB, &stmt))
    {
        ERREXIT(status, 1)
    }

    osqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    osqlda->sqln = 1;
    osqlda->sqld = 1;
    osqlda->version = 1;

    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_prepare(status, &trans, &stmt, 0, tot_str, 1, osqlda))
        isc_print_status(status);

    osqlda->sqlvar[0].sqldata = (char *) &tot_budget;
    osqlda->sqlvar[0].sqltype = SQL_DOUBLE + 1;
    osqlda->sqlvar[0].sqllen  = sizeof(budget);
    osqlda->sqlvar[0].sqlind  = &flag3;

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    fetch_stat = isc_dsql_fetch(status, &stmt, 1, osqlda);

    printf("\nTotal budget:  %16.2f\n\n", tot_budget);

    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    /*
     *    Prepare and execute the positioned update.
     *    Re-use the statement handle as the select cursor.
     */
 
    sprintf(upd_str, "UPDATE proj_dept_budget SET projected_budget = ? \
            WHERE CURRENT OF %s", cursor);

    /* Allocate an input SQLDA for the update statement. */
    isqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    isqlda->sqln = isqlda->sqld = 1;
    isqlda->version = 1;

    isqlda->sqlvar[0].sqldata = (char *) &budget;
    isqlda->sqlvar[0].sqltype = SQL_DOUBLE + 1;
    isqlda->sqlvar[0].sqllen  = sizeof(budget);
    isqlda->sqlvar[0].sqlind  = &flag3;
                              
    /* Free the output SQLDA, which was used previously. */
    free(osqlda);

    /* Re-allocate the output SQLDA. */
    osqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(3));
    osqlda->sqln = 3;
    osqlda->sqld = 3;
    osqlda->version = 1;

    osqlda->sqlvar[0].sqldata = proj_id;
    osqlda->sqlvar[0].sqlind  = &flag0;

    osqlda->sqlvar[1].sqldata = dept_no;
    osqlda->sqlvar[1].sqlind  = &flag1;

    osqlda->sqlvar[2].sqldata = (char *) &budget;
    osqlda->sqlvar[2].sqlind  = &flag2;
                              
    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Zero the statement handle. */
    stmt = NULL;

    if (isc_dsql_allocate_statement(status, &DB, &stmt))
        isc_print_status(status);
    
    if (isc_dsql_prepare(status, &trans, &stmt, 0, sel_str, 1, osqlda))
        isc_print_status(status);
               
    /* Declare the cursor. */
    isc_dsql_set_cursor_name(status, &stmt, cursor, 0);

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    printf("\n%-15s%-10s%-18s%-18s\n\n",
           "PROJ", "DEPT", " CURRENT BUDGET",  "  CHANGED TO");

    /*
     *    Fetch and update department budgets.
     */

    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, osqlda)) == 0)
    {
        /* Determine the increase percentage. */
        proj_id[PROJLEN] = '\0';
        dept_no[DEPTLEN] = '\0';
        printf("%-15s%-10s%15.2f", proj_id, dept_no, budget);
        budget = budget + budget * increase_factor(budget);
        printf("%15.2f\n", budget);

        /* Increase the budget. */
        isc_dsql_exec_immed2(status, &DB, &trans, 0, upd_str, 1, isqlda, NULL);

        if (isc_sqlcode(status) == -625)
        {
            printf("\tExceeded budget limit -- not updated.\n");
            continue;
        }
        else
            isc_print_status(status);
    }

    if (fetch_stat != 100L)
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
    {
        ERREXIT(status, 1)
    }

    if (isc_rollback_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    if (isc_detach_database(status, &DB))
    {
        ERREXIT(status, 1)
    }

    free(osqlda);
    free(isqlda);

    return 0;
}
    
/*
 *    Determine a percent increase for the department's budget.
 */
float increase_factor (double budget)
{
    if (budget < 100000L)
        return (float)0.15;
    else if (budget < 500000L)
        return (float)0.10;
    else 
        return (float)0.5;
}

