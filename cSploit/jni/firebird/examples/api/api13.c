/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program performs a multi-database transaction
 *        with a two-phase commit.  A 'currency' field is updated
 *        in database 1 for table country, and corresponding
 *        'from_currency' or 'to_currency' fields are updated in
 *        database 2 for table cross_rate.  The transaction is
 *        committed only if all instances of the string are
 *        changed successfully in both databases.
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

#define BUFLEN        512
#define CURRENLEN    10

char    *sel_str1 =
    "SELECT currency FROM country WHERE country = 'Canada'";


int main (int argc, char** argv)
{
    char                *country = "Canada";        /* passed as a parameter */
    char                *new_name = "CdnDollar";    /* passed as a parameter */
    char                orig_name[CURRENLEN + 1];
    char                buf[BUFLEN + 1];
    isc_db_handle       db1 = NULL,        /* handle for database 1 */
                        db2 = NULL;        /* handle for database 2 */
    isc_tr_handle       trans1 = NULL;     /* transaction handle */
    ISC_STATUS_ARRAY    status;
    XSQLDA *            sel_sqlda;
    isc_stmt_handle     stmt = NULL;
    long                stat1, stat2, stat3;
    char                empdb[128], empdb2[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");
    if (argc > 2)
        strcpy(empdb2, argv[2]);
    else
        strcpy(empdb2, "employe2.fdb");


    /* Open database 1. */
    printf("Attaching to database %s\n", empdb);
    if (isc_attach_database(status, 0, empdb, &db1, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Open database 2. */
    printf ("Attaching to database %s\n", empdb2);
    if (isc_attach_database(status, 0, empdb2, &db2, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /* Start a two-database transaction. */
    if (isc_start_transaction(status, &trans1, 2, &db1, 0, NULL, &db2, 0, NULL))
    {
        ERREXIT(status, 1)
    }          

    /*
     *  Get the string, which is to be globally changed.
     */

    sprintf(buf, "SELECT currency FROM country WHERE country = '%s'", country);

    sel_sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sel_sqlda->sqln = 1;
    sel_sqlda->version = 1;
 
    if (isc_dsql_allocate_statement(status, &db1, &stmt))
    {
        ERREXIT(status, 1)
    }
    if (isc_dsql_prepare(status, &trans1, &stmt, 0, sel_str1, 1, sel_sqlda))
    {
        ERREXIT(status, 1)
    }
 
    sel_sqlda->sqlvar[0].sqldata = orig_name;
    sel_sqlda->sqlvar[0].sqltype = SQL_TEXT;
    sel_sqlda->sqlvar[0].sqllen = CURRENLEN;
 
    if (isc_dsql_execute(status, &trans1, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }
    if (isc_dsql_fetch(status, &stmt, 1, sel_sqlda))
    {
        ERREXIT(status, 1)
    }

    orig_name[CURRENLEN] = '\0';
    printf("Modifying currency string:  %s\n", orig_name);
                   
    /*
     *  Change the string in database 1.
     */

    sprintf(buf, "UPDATE country SET currency = '%s' WHERE country = 'Canada'",
            new_name);

    stat1 = 0L;
    if (isc_dsql_execute_immediate(status, &db1, &trans1, 0, buf, 1, NULL))
    {
        isc_print_status(status);
        stat1 = isc_sqlcode(status);
    }       

    /*
     *  Change all corresponding occurences of the string in database 2.
     */

    sprintf(buf, "UPDATE cross_rate SET from_currency = '%s' WHERE \
            from_currency = '%s'", new_name, orig_name);

    stat2 = 0L;
    if (isc_dsql_execute_immediate(status, &db2, &trans1, 0, buf, 1, NULL))
    {
        isc_print_status(status);
        stat2 = isc_sqlcode(status);
    }
    
    sprintf(buf, "UPDATE cross_rate SET to_currency = '%s' WHERE \
            to_currency = '%s'", new_name, orig_name);

    stat3 = 0L;
    if (isc_dsql_execute_immediate(status, &db2, &trans1, 0, buf, 1, NULL))
    {
        isc_print_status(status);
        stat3 = isc_sqlcode(status);
    }

    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
        isc_print_status(status);

    /*
     *    If all statements executed successfully, commit the transaction.
     *    Otherwise, undo all work.
     */
    if (!stat1 && !stat2 && !stat3)
    {
        if (isc_commit_transaction (status, &trans1))
            isc_print_status(status);
        printf("Changes committed.\n");
    }
    else
    {
        printf("update1:  %d\n", stat1);
        printf("update2:  %d\n", stat2);
        printf("update3:  %d\n", stat3);
        if (isc_rollback_transaction(status, &trans1))
            isc_print_status(status);
        printf("Changes undone.\n");
    }

    /* Close database 1. */
    if (isc_detach_database(status, &db1))
        isc_print_status(status);

    /* Close database 2. */
    if (isc_detach_database(status, &db2))
        isc_print_status(status);
    
    free(sel_sqlda);

    return 0;
}

