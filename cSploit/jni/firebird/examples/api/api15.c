/*
 *    Program type:  API
 *
 *    Description:
 *        This program demonstrates constructing a database
 *        parameter buffer and attaching to a database.
 *        First manually set sweep interval, then
 *        The user and password is set to "guest" with expand_dpb
 *        Then get back the sweep interval and other useful numbers
 *        with an info call.
 *        This program will accept up to 4 args.  All are positional:
 *        api15 <db_name> <user> <password> <sweep_interval>
 *
 *        Note: The system administrator needs to create the account
 *        "guest" with password "guest" on the server before this
 *        example can be run.
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

int main (int argc, char** argv)
{
    isc_db_handle   db = NULL;      /* database handle */
    isc_tr_handle   trans = NULL;     /* transaction handle */
    isc_stmt_handle stmt = NULL;
    ISC_STATUS_ARRAY status;         /* status vector */
    char            dbname[128];
    char            user_name[31], uname[81];
    short           nullind;
    char            password[31];

    /* Query to find current user name */
    static char *query = "SELECT USER FROM RDB$DATABASE";
    char *  dpb = NULL,    /* DB parameter buffer */
                    *d, *p, *copy;
    XSQLDA *sqlda;

    short           dpb_length = 0;
    long            l,sweep_interval = 16384;

    /* All needed for analyzing an info packet */
    char            buffer[100];    /* Buffer for db info */
    char            item;
    short           length;
    long            value_out;

    /* List of items for db_info call */
    static char    db_items [] = {
                        isc_info_page_size,
                        isc_info_allocation,
                        isc_info_sweep_interval,
                        isc_info_end
                    };

    strcpy(user_name, "guest");
    strcpy(password, "guest");
    strcpy(dbname, "employee.fdb");

    if (argc > 1)
        strcpy(dbname, argv[1]);
    if (argc > 2)
        strcpy(user_name, argv[2]);
    if (argc > 3)
        strcpy(password, argv[3]);
    if (argc > 4)
        sweep_interval = atoi(argv[4]);

    /* Adding sweep interval will be done by hand 
    **  First byte is a version (1), next byte is the isc_dpb_sweep
    **  byte, then a length (4) then the byte-swapped int we want
    **  to provide -- 7 bytes total
    */

    copy = dpb = (char *) malloc(7);
    p = dpb;
    *p++ = '\1';
    *p++ = isc_dpb_sweep_interval;
    *p++ = '\4';
    l = isc_vax_integer((char *) &sweep_interval, 4);
    d = (char *) &l;
    *p++ = *d++;
    *p++ = *d++;
    *p++ = *d++;
    *p = *d;
    dpb_length = 7;

    /* Add user and password to dpb, much easier.  The dpb will be
    **  new memory.
    */
    isc_expand_dpb(&dpb, (short *) &dpb_length,
                   isc_dpb_user_name, user_name,
                   isc_dpb_password, password,  NULL);

    /*
    **  Connect to the database with the given user and pw.
    */            
    printf("Attaching to %s with user name: %s, password: %s\n",
           dbname, user_name, password);
    printf("Resetting sweep interval to %ld\n", sweep_interval);

    if (isc_attach_database(status, 0, dbname, &db, dpb_length, dpb))
        isc_print_status(status);


    /* Prove we are "guest" . Query a single row with the
    ** key word "USER" to find out who we are
    */     
    if (isc_start_transaction(status, &trans, 1, &db, 0, NULL))
        isc_print_status(status);
    
    /* Prepare sqlda for singleton fetch */
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
    sqlda->sqln = sqlda->sqld = 1;
    sqlda->version = 1;
    sqlda->sqlvar[0].sqldata = uname;
    sqlda->sqlvar[0].sqlind = &nullind;

    /* Yes, it is possible to execute a singleton select without
    ** a cursor.  You must prepare the sqlda by hand.
    */
    isc_dsql_allocate_statement(status, &db, &stmt);
    if (isc_dsql_prepare(status, &trans, &stmt, 0, query, 1, sqlda))
    {
        ERREXIT(status, 1)
    }
    /* Force to type sql_text */
    sqlda->sqlvar[0].sqltype = SQL_TEXT;

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }
    /* There will only be one row.  If it isn't there, something is
    **  seriously wrong.
    */
    if (isc_dsql_fetch(status, &stmt, 1, sqlda))
    {
        ERREXIT(status, 1)
    }

    isc_dsql_free_statement(status, &stmt, DSQL_close);

    uname[sqlda->sqlvar[0].sqllen] = '\0';
    printf ("New user = %s\n", uname);
    if (isc_commit_transaction(status, &trans))
    {
        ERREXIT(status, 1)
    }

    /* Look at the database to see some parameters (like sweep
    ** The database_info call returns a buffer full of type, length,
    **  and value sets until info_end is reached
    */

    if (isc_database_info(status, &db, sizeof(db_items), db_items,
                          sizeof (buffer), buffer))
    {
        ERREXIT(status, 1)
    }

    for (d = buffer; *d != isc_info_end;)
    {
        value_out = 0;
        item = *d++;
        length = (short) isc_vax_integer (d, 2);
        d += 2;
        switch (item)
        {    
            case isc_info_end:
                break;

            case isc_info_page_size:
                value_out = isc_vax_integer (d, length);
                printf ("PAGE_SIZE %ld \n", value_out);
                break;

            case isc_info_allocation:
                value_out = isc_vax_integer (d, length);
                printf ("Number of DB pages allocated = %ld \n", 
                value_out);
                break;

            case isc_info_sweep_interval:
                value_out = isc_vax_integer (d, length);
                printf ("Sweep interval = %ld \n", value_out);
                break;
        }
        d += length;
    }    

    isc_detach_database(status, &db);
    isc_free(dpb);
    free(copy);
    free(sqlda);

    return 0;
}

