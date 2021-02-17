/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program creates a new database, given an SQL statement
 *        string.  The newly created database is accessed after its
 *        creation, and a sample table is added.
 *
 *        The SQLCODE is extracted from the status vector and is used
 *        to check whether the database already exists.
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

int pr_error (long *, char *);



static char *create_tbl  = "CREATE TABLE dbinfo (when_created DATE)";
static char *insert_date = "INSERT INTO dbinfo VALUES ('NOW')";

int main (int argc, char** argv)
{
    isc_db_handle   newdb = NULL;          /* database handle */
    isc_tr_handle   trans = NULL;          /* transaction handle */
    ISC_STATUS_ARRAY status;               /* status vector */
    long            sqlcode;               /* SQLCODE  */
    char            create_db[160];        /* 'create database' statement */
    char            new_dbname[128];

    if (argc > 1)
        strcpy(new_dbname, argv[1]);
    else
        strcpy(new_dbname, "new.fdb");

    /*
     *    Construct a 'create database' statement.
     *    The database name could have been passed as a parameter.
     */
    sprintf(create_db, "CREATE DATABASE '%s'", new_dbname);
    
    /*
     *    Create a new database.
     *    The database handle is zero.
     */
    
    if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, create_db, 1,
                                   NULL))
    {
        /* Extract SQLCODE from the status vector. */
        sqlcode = isc_sqlcode(status);

        /* Print a descriptive message based on the SQLCODE. */
        if (sqlcode == -902)
        {
            printf("\nDatabase already exists.\n");
            printf("Remove %s before running this program.\n\n", new_dbname);
        }

        /* In addition, print a standard error message. */
        if (pr_error(status, "create database"))
            return 1;
    }

    isc_commit_transaction(status, &trans);
    printf("Created database '%s'.\n\n", new_dbname);

    /*
     *    Connect to the new database and create a sample table.
     */

    /* newdb will be set to null on success */ 
    isc_detach_database(status, &newdb);

    if (isc_attach_database(status, 0, new_dbname, &newdb, 0, NULL))
        if (pr_error(status, "attach database"))
            return 1;


    /* Create a sample table. */
    isc_start_transaction(status, &trans, 1, &newdb, 0, NULL);
    if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, create_tbl, 1, NULL))
        if (pr_error(status, "create table"))
            return 1;
    isc_commit_transaction(status, &trans);

    /* Insert 1 row into the new table. */
    isc_start_transaction(status, &trans, 1, &newdb, 0, NULL);
    if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, insert_date, 1, NULL))
        if (pr_error(status, "insert into"))
            return 1;
    isc_commit_transaction(status, &trans);

    printf("Successfully accessed the newly created database.\n\n");

    isc_detach_database(status, &newdb);

    return 0;
}            

/*
 *    Print the status, the SQLCODE, and exit.
 *    Also, indicate which operation the error occured on.
 */
int pr_error (long* status, char* operation)
{
    printf("[\n");
    printf("PROBLEM ON \"%s\".\n", operation);

    isc_print_status(status);

    printf("SQLCODE:%d\n", isc_sqlcode(status));

    printf("]\n");

    return 1;
}

