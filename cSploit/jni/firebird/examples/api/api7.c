/*
 *  Program type:  API Interface
 *
 *    Description:
 *      This program selects a blob data type.
 *      A set of project descriptions is printed.
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

#define TYPELEN        12
#define PROJLEN        20
#define BUFLEN        512

/* This macro is used to declare structures representing SQL VARCHAR types */
#define SQL_VARCHAR(len) struct {short vary_length; char vary_string[(len)+1];}

int main (int argc, char** argv)
{
    SQL_VARCHAR(PROJLEN + 2)    proj_name;
    char                        prod_type[TYPELEN + 2];
    char                        sel_str[BUFLEN + 1];
    ISC_QUAD                    blob_id;
    isc_blob_handle             blob_handle = NULL;
    short                       blob_seg_len;
    char                        blob_segment[11];
    isc_db_handle               DB = NULL;        /* database handle */
    isc_tr_handle               trans = NULL;     /* transaction handle */
    ISC_STATUS_ARRAY            status;           /* status vector */
    isc_stmt_handle             stmt = NULL;      /* statement handle */
    XSQLDA *                    sqlda;
    long                        fetch_stat, blob_stat;
    short                       flag0 = 0,
                                flag1 = 0,
                                flag2 = 0;
    char                        empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");


    strcpy(sel_str, "SELECT proj_name, proj_desc, product FROM project WHERE \
           product IN ('software', 'hardware', 'other') ORDER BY proj_name");

    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
    {
        ERREXIT(status, 1)
    }

    /*
     *    Allocate and prepare the select statement.
     */

    if (isc_dsql_allocate_statement(status, &DB, &stmt))
    {
        ERREXIT(status, 1)
    }
    
    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(3));
    sqlda->sqln = 3;
    sqlda->version = 1;

    if (isc_dsql_prepare(status, &trans, &stmt, 0, sel_str, 1, sqlda))
    {
        ERREXIT(status, 1)
    }

    sqlda->sqlvar[0].sqldata = (char *)&proj_name;
    sqlda->sqlvar[0].sqltype = SQL_VARYING + 1;
    sqlda->sqlvar[0].sqlind  = &flag0;

    sqlda->sqlvar[1].sqldata = (char *) &blob_id;
    sqlda->sqlvar[1].sqltype = SQL_BLOB + 1;
    sqlda->sqlvar[1].sqlind  = &flag1;

    sqlda->sqlvar[2].sqldata = prod_type;
    sqlda->sqlvar[2].sqltype = SQL_TEXT + 1;
    sqlda->sqlvar[2].sqlind  = &flag2;

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }

    /*
     *    For each project in the select statement, get and display
     *    project descriptions.
     */

    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, sqlda)) == 0)
    {
        prod_type[TYPELEN] = '\0';
        printf("\nPROJECT:  %-20.*s   TYPE:  %-15s\n\n",
               proj_name.vary_length, proj_name.vary_string, prod_type);

        /* Open the blob with the fetched blob_id.   Notice that the
        *  segment length is shorter than the average segment fetched.
        *  Each partial fetch should return isc_segment.
        */
        if (isc_open_blob(status, &DB, &trans, &blob_handle, &blob_id))
        {
            ERREXIT(status, 1)
        }

        /* Get blob segments and their lengths and print each segment. */
        blob_stat = isc_get_segment(status, &blob_handle,
                                    (unsigned short *) &blob_seg_len,
                                    sizeof(blob_segment), blob_segment);
        while (blob_stat == 0 || status[1] == isc_segment)
        {
            printf("%*.*s", blob_seg_len, blob_seg_len, blob_segment);
            blob_stat = isc_get_segment(status, &blob_handle,
                                        (unsigned short *)&blob_seg_len,
                                        sizeof(blob_segment), blob_segment);
        }
        /* Close the blob.  Should be blob_stat to check */
        if (status[1] == isc_segstr_eof)
        {
            if (isc_close_blob(status, &blob_handle))
            {
                ERREXIT(status, 1)
            }
        }
        else
            isc_print_status(status);

        printf("\n");
    }

    if (fetch_stat != 100L)
    {
        ERREXIT(status, 1)
    }

    if (isc_dsql_free_statement(status, &stmt, DSQL_close))
    {
        ERREXIT(status, 1)
    }

    if (isc_commit_transaction (status, &trans))
    {
        ERREXIT(status, 1)
    }

    if (isc_detach_database(status, &DB))
    {
        ERREXIT(status, 1)
    }

    free(sqlda);

    return 0;
}

