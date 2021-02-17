/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program uses a filter to display a blob data type.
 *        Job descriptions are read through a filter, which formats
 *        the text into 40-character-wide paragraphs.
 *
 *    IMPORTANT NOTE!
 *        The server side file, api9f.c, must have been compiled
 *        and linked and must be running on the server before
 *        this example can be run.
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

#define COUNTRYLEN 15
#define CODELEN    5

char    *sel_str =
    "SELECT job_requirement, job_code, job_grade, job_country \
     FROM job WHERE job_code = 'SRep'";

/* Blob parameter buffer. */
char bpb[] = {1,2,2,-4,-1,1,2,1,0};

int main (int argc, char** argv)
{
    char                job_code[CODELEN + 2];
    short               job_grade;
    char                job_country[COUNTRYLEN + 2];
    short               flag0 = 0, flag1 = 0,
                        flag2 = 0, flag3 = 0;
    ISC_QUAD            blob_id;
    isc_blob_handle     blob_handle = NULL;
    short               blob_seg_len;
    char                blob_segment[401];
    isc_db_handle       DB = NULL;
    isc_tr_handle       trans = NULL;
    ISC_STATUS_ARRAY    status;
    isc_stmt_handle     stmt = NULL;
    XSQLDA *            sqlda;
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

    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(4));
    sqlda->sqln = 4;
    sqlda->version = 1;

    if (isc_dsql_prepare(status, &trans, &stmt, 0, sel_str, 1, sqlda))
    {
        ERREXIT(status, 1)
    }

    sqlda->sqlvar[0].sqldata = (char *) &blob_id;
    sqlda->sqlvar[0].sqltype = SQL_BLOB + 1;
    sqlda->sqlvar[0].sqlind  = &flag0;

    sqlda->sqlvar[1].sqldata = (char *) job_code;
    sqlda->sqlvar[1].sqltype = SQL_TEXT + 1;
    sqlda->sqlvar[1].sqlind  = &flag1;

    sqlda->sqlvar[2].sqldata = (char *) &job_grade;
    sqlda->sqlvar[2].sqltype = SQL_SHORT + 1;
    sqlda->sqlvar[2].sqlind  = &flag2;

    sqlda->sqlvar[3].sqldata = (char *) job_country;
    sqlda->sqlvar[3].sqltype = SQL_TEXT + 1;
    sqlda->sqlvar[3].sqlind  = &flag3;

    if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
    {
        ERREXIT(status, 1)
    }
            
    /*
     *  Display job descriptions.
     */

    while ((fetch_stat = isc_dsql_fetch(status, &stmt, 1, sqlda)) == 0)
    {
        job_code[CODELEN] = '\0';
        job_country[COUNTRYLEN] = '\0';
        printf("\nJOB CODE: %5s  GRADE: %d", job_code, job_grade);
        printf("  COUNTRY:  %-20s\n\n", job_country);

        /* Open the blob with the fetched blob_id. */
        if (isc_open_blob2(status, &DB, &trans, &blob_handle, &blob_id, 9, bpb))
        {
            ERREXIT(status, 1)
        }

        /* Get blob segments and their lengths and print each segment. */
        while (isc_get_segment(status, &blob_handle,
               (unsigned short *) &blob_seg_len, sizeof(blob_segment),
               blob_segment) == 0)
            printf("  %*.*s", blob_seg_len, blob_seg_len, blob_segment);

        /* Close the blob.  */
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
        isc_print_status(status);

    if (isc_dsql_free_statement(status, &stmt, DSQL_drop))
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

    free(sqlda);

    return 0;
}

