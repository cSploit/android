/*
 *  Program type:  API Interface
 *
 *  Desription:
 *      This program updates departments' budgets, given
 *      the department and the new budget information parameters.
 *
 *      An input SQLDA is allocated for the update query
 *      with parameter markers.
 *      Note that all updates are rolled back in this version.
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

int get_input (char* , double*);

static char *Dept_data[] =
     {"622", "100", "116", "900", 0};

static double Percent_data[] =
    {0.05,  1.00,  0.075,  0.10, 0};

int Input_ptr = 0;

char *updstr =
    "UPDATE department SET budget = ? * budget + budget WHERE dept_no = ?";

isc_db_handle    DB = NULL;                       /* database handle */
isc_tr_handle    trans = NULL;                    /* transaction handle */
ISC_STATUS_ARRAY status;                          /* status vector */

int main (int argc, char** argv)
{
    char            dept_no[4];
    double          percent_inc;
    short           flag0 = 0, flag1 = 0;
    XSQLDA         *sqlda;
    long            sqlcode;
    char            empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");
    
    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Allocate an input SQLDA.  There are two unknown parameters. */
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(2));
    sqlda->sqln = 2;
    sqlda->sqld = 2;
    sqlda->version = 1;

    sqlda->sqlvar[0].sqldata = (char *) &percent_inc;
    sqlda->sqlvar[0].sqltype = SQL_DOUBLE + 1;
    sqlda->sqlvar[0].sqllen  = sizeof(percent_inc);
    sqlda->sqlvar[0].sqlind  = &flag0;
    flag0 = 0;

    sqlda->sqlvar[1].sqldata = dept_no;
    sqlda->sqlvar[1].sqltype = SQL_TEXT + 1;
    sqlda->sqlvar[1].sqllen  = 3;
    sqlda->sqlvar[1].sqlind  = &flag1;
    flag1 = 0;               

    /*
     *  Get the next department-percent increase input pair.
     */
    while (get_input(dept_no, &percent_inc))
    {
        printf("\nIncreasing budget for department:  %s  by %5.2lf percent.\n",
               dept_no, percent_inc);

        if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
        {
            ERREXIT(status, 1)
        }

        /* Update the budget. */
        isc_dsql_execute_immediate(status, &DB, &trans, 0, updstr, 1, sqlda);
        sqlcode = isc_sqlcode(status);
        if (sqlcode)
        {
            /* Don't save the update, if the new budget exceeds the limit. */
            if (sqlcode == -625)
            {
                printf("\tExceeded budget limit -- not updated.\n");

                if (isc_rollback_transaction(status, &trans))
                {
                    ERREXIT(status, 1)
                }
                continue;
            }
            /* Undo all changes, in case of an error. */
            else
            {
                isc_print_status(status);
                printf("SQLCODE=%d\n", sqlcode);

                isc_rollback_transaction(status, &trans);
                ERREXIT(status, 1)
            }
        }

        /* Save each department's update independently. 
        ** Change to isc_commit_transaction to see changes
         */
        if (isc_rollback_transaction (status, &trans))
        {
            ERREXIT(status, 1)
        }
    }

    if (isc_detach_database(status, &DB))
    {
        ERREXIT(status, 1)
    }

    free(sqlda);

    return 0;
}


/*
 *  Get the department and percent parameters.
 */

int get_input (char *dept_no, double *percent)
{
    if (Dept_data[Input_ptr] == 0)
        return 0;
 
    strcpy(dept_no, Dept_data[Input_ptr]);
 
    if ((*percent = Percent_data[Input_ptr++]) == 0)
        return 0;
 
    return 1;
}

