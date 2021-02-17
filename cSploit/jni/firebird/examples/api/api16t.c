/*
 *  Program type:   API
 *
 *  Description:
 *        If run from a Windows Client, the winevent program
 *        should be started before running this program and should
 *        be terminated upon the completion of this program.
 *        For other platforms, this program should be run in
 *        conjunction with api16.
 *
 *        This Program adds some sales records, in order to trigger
 *        the event that api16 or winevent is waiting for.
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
#include <stdio.h>
#include <string.h>
#include "example.h"
#include <ibase.h>


int main (int argc, char** argv)
{        
    struct {
        short    len;
        char    data [9];
    } po_number;
    short               nullind = 0;
    isc_stmt_handle     stmt = NULL;     /* statement handle */
    isc_db_handle       DB = NULL;       /* database handle */
    isc_tr_handle       trans = NULL;    /* transaction handle */
    ISC_STATUS_ARRAY    status;          /* status vector */
    XSQLDA  *           sqlda;
    char                empdb[128];
    char                *delete_str =
        "DELETE FROM sales WHERE po_number LIKE 'VNEW%'";
    char                *insert_str =
        "INSERT INTO sales (po_number, cust_no, order_status, total_value) \
        VALUES (?, 1015, 'new', 0)";

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

    /* Allocate an input SQLDA for po_number in insert string. */
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda->sqln = 1;
    sqlda->sqld = 1;
    sqlda->version = 1;

    /* Allocate a statement. */
    if (isc_dsql_allocate_statement(status, &DB, &stmt))
    {
        ERREXIT(status, 1)
    }

    /* Start out by deleting any existing records */

    if (isc_dsql_execute_immediate(status, &DB, &trans, 0, delete_str,
                                   1, NULL))
    {
        ERREXIT(status, 2)
    }

    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 2)
    }


    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 3)
    }

    /* Insert three records in one transaction */
    if (isc_dsql_prepare(status, &trans, &stmt, 0, insert_str, 1, NULL))
    {
        ERREXIT(status, 4)
    }
    if (isc_dsql_describe_bind(status, &stmt, 1, sqlda))
    {
        ERREXIT(status, 5)
    }

    sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
    sqlda->sqlvar[0].sqllen  = sizeof (po_number.data);
    sqlda->sqlvar[0].sqldata = (char *) &po_number;
    sqlda->sqlvar[0].sqlind  = &nullind;

    /* Add batch 1. */
    po_number.len = strlen("VNEW1");
    strncpy(po_number.data, "VNEW1", sizeof (po_number.data));
    printf("api16t:  Adding %s\n", po_number.data);
    if (isc_dsql_execute(status, &trans, &stmt, 1, sqlda))
    {
        ERREXIT(status, 6)
    }

    po_number.len = strlen("VNEW2");
    strncpy(po_number.data, "VNEW2", sizeof (po_number.data));
    printf("api16t:  Adding %s\n", po_number.data);
    if (isc_dsql_execute(status, &trans, &stmt, 1, sqlda))
    {
        ERREXIT(status, 6)
    }

    po_number.len = strlen("VNEW3");
    strncpy(po_number.data, "VNEW3", sizeof (po_number.data));
    printf("api16t:  Adding %s\n", po_number.data);
    if (isc_dsql_execute(status, &trans, &stmt, 1, sqlda))
    {
        ERREXIT(status, 6)
    }

    /* This will fire the triggers for the first three records */
    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 7)
    }
     
    /* Add batch 2. */
    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 8)
    }

    po_number.len = strlen("VNEW4");
    strncpy(po_number.data, "VNEW4", sizeof (po_number.data));
    printf("api16t:  Adding %s\n", po_number.data);
    if (isc_dsql_execute(status, &trans, &stmt, 1, sqlda))
    {
        ERREXIT(status, 9)
    }

    if (isc_dsql_free_statement(status, &stmt, DSQL_drop))
    {
        ERREXIT(status, 10)
    }

    /* This will fire the triggers for the fourth record */
    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 11)
    }

    isc_detach_database(status, &DB);

    free(sqlda);

    return 0;
}

