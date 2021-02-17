/*
 *    Program type:  API
 *
 *    Description:
 *        This program selects and updates an array type.
 *        Projected head count is displayed and updated for
 *        a set of projects.
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

char    *sel_str =
    "SELECT dept_no, quart_head_cnt FROM proj_dept_budget p \
    WHERE fiscal_year = 1994 AND proj_id = 'VBASE' \
    FOR UPDATE of quart_head_cnt";

char    *upd_str =
    "UPDATE proj_dept_budget SET quart_head_cnt = ? WHERE CURRENT OF S";


int main (int argc, char** argv)
{
    long            hcnt[4];
    ISC_QUAD        array_id;
    ISC_ARRAY_DESC  desc;
    long            len;
    char            dept_no[6];
    isc_db_handle   DB = NULL;
    isc_tr_handle   trans = NULL;
    ISC_STATUS_ARRAY status;
    short           flag0 = 0, flag1 = 0;
    isc_stmt_handle stmt = NULL;
    isc_stmt_handle ustmt = NULL;
    char *          cursor = "S";
    XSQLDA *        osqlda, *isqlda;
    long            fetch_stat;
    short           i;
    char            empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }
 
    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }          

    /*
     *    Set up the array description structure
     */


    if (isc_array_lookup_bounds(status, &DB, &trans,
                                "PROJ_DEPT_BUDGET", "QUART_HEAD_CNT", &desc))
    {
        ERREXIT(status, 1)
    }
    
    /*
     *    Set-up the select statement.
     */

    osqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(2));
    osqlda->sqln = 2;
    osqlda->version = 1;

    osqlda->sqlvar[0].sqldata = (char *) dept_no;
    osqlda->sqlvar[0].sqltype = SQL_TEXT + 1;
    osqlda->sqlvar[0].sqlind  = &flag0;

    osqlda->sqlvar[1].sqldata = (char *) &array_id;
    osqlda->sqlvar[1].sqltype = SQL_ARRAY + 1;
    osqlda->sqlvar[1].sqlind  = &flag1;

    isc_dsql_allocate_statement(status, &DB, &stmt);
    isc_dsql_allocate_statement(status, &DB, &ustmt);

    /* Prepare and execute query */
    if (isc_dsql_prepare(status, &trans, &stmt, 0, sel_str, 1, osqlda))
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Needed for update current */
    isc_dsql_set_cursor_name(status, &stmt, cursor, 0);

    /*
     *    Set-up the update statement.
     */

    isqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    isqlda->sqln = 1;
    isqlda->version = 1;

    /* Use describe_bind to set up input sqlda */

    if (isc_dsql_prepare(status, &trans, &ustmt, 0, upd_str, 1, NULL))
    {
        ERREXIT(status, 1)
    }
    isc_dsql_describe_bind(status, &ustmt, 1, isqlda);

    isqlda->sqlvar[0].sqldata = (char *) &array_id;
    isqlda->sqlvar[0].sqlind  = &flag1;
                              
    /*
     *    Fetch the head count for each department's 4 quarters;
     *    increase the head count by 1 for each quarter;
     *    and save the new head count.
     */

    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, osqlda)) == 0)
    {
        /* Get the current array values. */
        if (!flag1)
        {                     
            len = sizeof(hcnt);;
            if (isc_array_get_slice(status, &DB, &trans,
                                    (ISC_QUAD *) &array_id,
                                    (ISC_ARRAY_DESC *) &desc,
                                    (void *) hcnt,
                                    (long *) &len))
                                    {ERREXIT (status, 1)}

            dept_no [osqlda->sqlvar[0].sqllen] = '\0';
            printf("Department #:  %s\n\n", dept_no);

            printf("\tCurrent counts: %ld %ld %ld %ld\n",
                   hcnt[0], hcnt[1], hcnt[2], hcnt[3]);

            /* Add 1 to each count. */
            for (i = 0; i < 4; i++)
                hcnt[i] = hcnt[i] + 1;

            /* Save new array values. */
            if (isc_array_put_slice(status, &DB, &trans,
                                    (ISC_QUAD *) &array_id,
                                    (ISC_ARRAY_DESC *) &desc,
                                    (void *) hcnt,
                                    (long *) &len))
                                    {ERREXIT (status, 1)}

            /* Update the array handle. */
            if (isc_dsql_execute(status, &trans, &ustmt, 1, isqlda))
            {
                ERREXIT(status, 1)
            }
             
            printf("\tNew counts    : %ld %ld %ld %ld\n\n",
                   hcnt[0], hcnt[1], hcnt[2], hcnt[3]);
        }
    }
       
    if (fetch_stat != 100L)
    {
        ERREXIT(status, 1)
    }

    isc_dsql_free_statement(status, &stmt, DSQL_close);
    isc_dsql_free_statement(status, &ustmt, DSQL_close);

    /* Do a rollback to keep from updating the sample db */

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

