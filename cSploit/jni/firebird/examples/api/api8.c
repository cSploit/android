/*
 *    Program type:  API Interface
 *
 *    Description:
 *        This program updates a blob data type.
 *        Project descriptions are added for a set of projects.
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

#define PROJLEN     5
#define BUFLEN      512

char *    get_line (void);

static char *Proj_data[] =
{
    "VBASE",
        "Design a video data base management system for ",
        "controlling on-demand video distribution.",
        0,
    "DGPII",
        "Develop second generation digital pizza maker ",
        "with flash-bake heating element and ",
        "digital ingredient measuring system.",
        0,
    "GUIDE",
        "Develop a prototype for the automobile version of ",
        "the hand-held map browsing device.",
        0,
    "MAPDB",
        "Port the map browsing database software to run ",
        "on the automobile model.",
        0,
    "HWRII",
        "Integrate the hand-writing recognition module into the ",
        "universal language translator.",
        0,
    0
};

int Inp_ptr = 0;

int main (int argc, char** argv)
{
    char                proj_id[PROJLEN + 1];
    char                upd_stmt[BUFLEN + 1];
    ISC_QUAD            blob_id;
    isc_blob_handle     blob_handle = NULL;
    isc_db_handle       DB = NULL;           /* database handle */
    isc_tr_handle       trans = NULL;        /* transaction handle */
    ISC_STATUS_ARRAY    status;              /* status vector */
    XSQLDA *            sqlda;
    unsigned short      len;
    char *              line;
    int                 rec_cnt = 0;
    char                empdb[128];

    if (argc > 1)
        strcpy(empdb, argv[1]);
    else
        strcpy(empdb, "employee.fdb");

    strcpy(upd_stmt, "UPDATE project SET proj_desc = ? WHERE proj_id = ?");

    if (isc_attach_database(status, 0, empdb, &DB, 0, NULL))
        isc_print_status(status);

    /*
     *  Set-up the SQLDA for the update statement.
     */

    sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(2));
    sqlda->sqln = 2;
    sqlda->sqld = 2;
    sqlda->version = 1;

    sqlda->sqlvar[0].sqldata = (char *) &blob_id;
    sqlda->sqlvar[0].sqltype = SQL_BLOB;
    sqlda->sqlvar[0].sqllen  = sizeof(ISC_QUAD);

    sqlda->sqlvar[1].sqldata = proj_id;
    sqlda->sqlvar[1].sqltype = SQL_TEXT;
    sqlda->sqlvar[1].sqllen  = 5;

    if (isc_start_transaction(status, &trans, 1, &DB, 0, NULL))
        isc_print_status(status);

    /*
     *  Get the next project id and update the project description.
     */
    line = get_line();
    while (line)
    {
        strcpy(proj_id, line);
        printf("\nUpdating description for project:  %s\n\n", proj_id);

        blob_handle = 0;
        if (isc_create_blob(status, &DB, &trans, &blob_handle, &blob_id))
        {
            ERREXIT(status, 1)
        }
        line = get_line();
        while (line)
        {
            printf("  Inserting segment:  %s\n", line);
            len = (unsigned short)strlen(line);

            if (isc_put_segment(status, &blob_handle, len, line))
            {
                ERREXIT (status, 1)
            }
            line = get_line();
        }

        if (isc_close_blob(status, &blob_handle))
        {
            ERREXIT(status, 1)
        }

        if (isc_dsql_execute_immediate(status, &DB, &trans, 0, upd_stmt, 1,
                                       sqlda))
        {
            ERREXIT (status, 1)
        }

        if (isc_sqlcode(status) == 0)
            rec_cnt++;
        else
            printf("Input error -- no project record with key: %s\n", proj_id);
        line = get_line();
    }
  
    if (isc_rollback_transaction (status, &trans))
    {
        ERREXIT(status, 1)
    }
    printf("\n\nAdded %d project descriptions.\n", rec_cnt);

    if (isc_detach_database(status, &DB))
    {
        ERREXIT(status, 1)
    }

    free(sqlda);

    return 0;
}


/*
 *    Get the next input line, which is either a project id
 *    or a project description segment.
 */

char *get_line (void)
{
    return Proj_data[Inp_ptr++];
}

