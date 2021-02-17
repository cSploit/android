/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program displays employee names and phone extensions.
 *
 *        It allocates an output SQLDA, prepares and executes a statement,
 *        and loops fetching multiple rows.
 *
 *        The SQLCODE returned by fetch is checked.
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
#include <ibase.h>
#include "example.h"

#define    LASTLEN     20
#define    FIRSTLEN    15
#define    EXTLEN       4

/* This macro is used to declare structures representing SQL VARCHAR types */
#define SQL_VARCHAR(len) struct {short vary_length; char vary_string[(len)+1];}

int main (int argc, char** argv)
{
    SQL_VARCHAR(LASTLEN)    last_name;
    SQL_VARCHAR(FIRSTLEN)   first_name;
    char                    phone_ext[EXTLEN + 2];
    short                   flag0 = 0, flag1 = 0;
    short                   flag2 = 0;
    isc_stmt_handle         stmt = NULL;                /* statement handle */
    isc_db_handle           DB = NULL;                  /* database handle */
    isc_tr_handle           trans = NULL;               /* transaction handle */
    ISC_STATUS_ARRAY        status;                     /* status vector */
    XSQLDA  *               sqlda;
    long                    fetch_stat;
    char                    empdb[128];
    char                    *sel_str =
        "SELECT last_name, first_name, phone_ext FROM phone_list \
        WHERE location = 'Monterey' ORDER BY last_name, first_name;";

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
        isc_print_status(status);

    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }
    
    /* Allocate an output SQLDA. */
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(3));
    sqlda->sqln = 3;
    sqlda->sqld = 3;
    sqlda->version = 1;

    /* Allocate a statement. */
    if (isc_dsql_allocate_statement(status, &DB, &stmt))
    {
        ERREXIT(status, 1)
    }
    
    /* Prepare the statement. */
    if (isc_dsql_prepare(status, &trans, &stmt, 0, sel_str, 1, sqlda))
    {
        ERREXIT(status, 1)
    }
    
    /*
     *  Although all three selected columns are of type varchar, the
     *  third field's type is changed and printed as type TEXT.
     */

    sqlda->sqlvar[0].sqldata = (char *)&last_name;
    sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
    sqlda->sqlvar[0].sqlind  = &flag0;

    sqlda->sqlvar[1].sqldata = (char *)&first_name;
    sqlda->sqlvar[1].sqltype = SQL_VARYING + 1;
    sqlda->sqlvar[1].sqlind  = &flag1;

    sqlda->sqlvar[2].sqldata = (char *) phone_ext;
    sqlda->sqlvar[2].sqltype = SQL_TEXT + 1;
    sqlda->sqlvar[2].sqlind  = &flag2;

    printf("\n%-20s %-15s %-10s\n\n", "LAST NAME", "FIRST NAME", "EXTENSION");

    /* Execute the statement. */
    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }
            
    /*
     *    Fetch and print the records.
     *    Status is 100 after the last row is fetched.
     */
    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, sqlda)) == 0)
    {
        printf("%-20.*s ", last_name.vary_length, last_name.vary_string);

        printf("%-15.*s ", first_name.vary_length, first_name.vary_string);

        phone_ext[sqlda->sqlvar[2].sqllen] = '\0';
        printf("%s\n", phone_ext);
    }

    if (fetch_stat != 100L)
    {
        ERREXIT(status, 1)
    }

    /* Free statement handle. */
    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
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

    free( sqlda);

    return 0;
}            

