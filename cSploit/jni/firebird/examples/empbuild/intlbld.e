/*
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "../jrd/common.h"
#include "ibase.h"

/* typedef char TEXT; */
#define FINI_OK 0
#define FINI_ERROR 44

/* 
**  Intlbld.e   International version of Empbuild.e.  Default database
**              name was changed to 'intlemp.fdb'.  Two of the files
**              executed as ISQL input files were modified: intlddl.sql
**              and intldml.sql are used by this program.
**
**              GPRE with manual switch, since it creates the database 
**		This program then calls isql with various input files
**		It installs the blobs and arrays.
**		Usage:  empbuild <db name>
*/

static int	addlang (void);
static int	addjob (void);
static int	addproj (void);
static int	addqtr (void);

static TEXT	Db_name[128];
static FILE	*Fp;
EXEC SQL SET SQL DIALECT 3;

EXEC SQL INCLUDE SQLCA;

EXEC SQL SET DATABASE DB = COMPILETIME "intlbuild.fdb" RUNTIME :Db_name;

int main (
    int		argc,
    char	*argv[])
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
TEXT	cmd [140];

if (argc > 1)
    strcpy (Db_name, argv[1]);
else
    strcpy (Db_name, "intlemp.fdb");

/* Create the database */

printf ("creating database %s\n", Db_name);
sprintf (cmd, "CREATE DATABASE \"%s\" DEFAULT CHARACTER SET ISO8859_1", Db_name);
gds__trans = 0;

EXEC SQL EXECUTE IMMEDIATE :cmd;
if (SQLCODE)
    {
    isc_print_status (gds__status);
    exit (FINI_ERROR);
    }

gds__trans = 0;

EXEC SQL DISCONNECT ALL;
if (SQLCODE)
    {
    isc_print_status (gds__status);
    exit (FINI_ERROR);
    }

/* ddl and dml file names are different for international */

printf ("Creating tables\n");
sprintf (cmd, "isql %s -q -i intlddl.sql", Db_name);
if (system (cmd))
    {
    printf ("Couldn't create tables \n");
    exit (FINI_ERROR);
    }

printf ("Turning  off indices and triggers \n");
sprintf (cmd, "isql %s -i indexoff.sql", Db_name);
system (cmd);

printf ("Loading  column data\n");
sprintf (cmd, "isql %s -ch ISO8859_1 -i intldml.sql", Db_name);
system (cmd);
printf ("Turning  on indices and triggers \n");
sprintf (cmd, "isql %s -i indexon.sql", Db_name);
system (cmd);

EXEC SQL CONNECT DB;
if (SQLCODE)
    {
    isc_print_status (gds__status);
    exit (FINI_ERROR);
    }

EXEC SQL SET TRANSACTION;
printf ("Loading Language blobs\n");
addlang();
printf ("Loading Job blobs\n");
addjob();
printf ("Loading project blobs \n");
addproj();
printf ("Loading quarter arrays \n");
addqtr();

exit (FINI_OK);
}

static int addlang (void)
{
/**************************************
 *
 *	a d d l a n g
 *
 **************************************
 *
 * Functional description
 *	Add language array to 'job' table.
 *
 **************************************/
TEXT	job_code[6], job_country[16];
TEXT	line[81];
TEXT	lang_array[5][16];
int	i, job_grade, rec_cnt = 0;

EXEC SQL SET TRANSACTION;
EXEC SQL WHENEVER SQLERROR GO TO Error;

Fp = fopen ("lang.inp", "r");

while (fgets (line, 100, Fp) != NULL)
    {
    sscanf (line, "%s %d %s", job_code, &job_grade, job_country);

    for (i = 0; i < 5; i++)
	{
	if (fgets (line, 100, Fp) == NULL)
	    break;
	strcpy (lang_array [i], line);
	}
		
    EXEC SQL
	UPDATE job
	    SET language_req = :lang_array
	    WHERE job_code = :job_code AND
		job_grade = :job_grade AND
		job_country = :job_country;

    if (SQLCODE == 0)
	rec_cnt++;
    else
	{
	printf ("Input error -- no job record with key: %s %d %s\n",
	    job_code, job_grade, job_country);
	}
    }

EXEC SQL COMMIT;
printf ("Added %d language arrays.\n", rec_cnt);
fclose (Fp);

return (0);
		
Error:

printf ("SQLCODE=%d\n", SQLCODE);
isc_print_status (gds__status);
return (1);
}

static int addjob (void)
{
/**************************************
 *
 *	a d d j o b
 *
 **************************************
 *
 * Functional description
 *	Add job description blobs.
 *
 **************************************/
TEXT		job_code[6];
TEXT		line[82], job_country[16];
int		len;
ISC_QUAD	job_blob;
int		job_grade, rec_cnt = 0;

EXEC SQL SET TRANSACTION;
EXEC SQL WHENEVER SQLERROR GO TO Error;

EXEC SQL DECLARE be CURSOR FOR
    INSERT BLOB job_requirement INTO job;

Fp = fopen ("job.inp", "r");

while (fgets (line, 100, Fp) != NULL)
    {
    EXEC SQL OPEN be INTO :job_blob;

    sscanf (line, "%s %d %s", job_code, &job_grade, job_country);

    while (fgets (line, 100, Fp) != NULL)
	{
	if (*line == '\n')
	    break;

	len = strlen (line);
	EXEC SQL INSERT CURSOR be VALUES (:line INDICATOR :len);
	}

    EXEC SQL CLOSE be;

    EXEC SQL
	UPDATE job
	    SET job_requirement = :job_blob
	    WHERE job_code = :job_code AND
		job_grade = :job_grade AND
		job_country = :job_country;

    if (SQLCODE == 0)
	rec_cnt++;
    else
	{
	printf ("Input error -- no job record with key: %s %d %s\n",
	    job_code, job_grade, job_country);
	}
    }

EXEC SQL COMMIT;
printf ("Added %d job requirement descriptions.\n", rec_cnt);
fclose (Fp);

return (0);
	
Error:

printf ("SQLCODE=%d\n", SQLCODE);
isc_print_status (gds__status);

return (1);
}

static int addproj (void)
{
/**************************************
 *
 *	a d d p r o j
 *
 **************************************
 *
 * Functional description
 *	Add project description blobs.
 *
 **************************************/
TEXT		proj_id[6];
TEXT		line[82];
int		len;
ISC_QUAD	proj_blob;
int		rec_cnt = 0;

EXEC SQL SET TRANSACTION;
EXEC SQL WHENEVER SQLERROR GO TO Error;

EXEC SQL DECLARE bd CURSOR FOR
    INSERT BLOB proj_desc INTO project;

Fp = fopen ("proj.inp", "r");

while (fgets (line, 100, Fp) != NULL)
    {
    EXEC SQL OPEN bd INTO :proj_blob;

    sscanf (line, "%s", proj_id);

    while (fgets (line, 100, Fp) != NULL)
	{
	if (*line == '\n')
	    break;

	len = strlen (line);
	EXEC SQL INSERT CURSOR bd VALUES (:line INDICATOR :len);
	}

    EXEC SQL CLOSE bd;

    EXEC SQL
	UPDATE project
	    SET proj_desc = :proj_blob
	    WHERE proj_id = :proj_id;

    if (SQLCODE == 0)
	rec_cnt++;
    else
	{
	printf ("Input error -- no project record with key: %s\n", proj_id);
	}
    }

EXEC SQL COMMIT;
printf ("Added %d project descriptions.\n", rec_cnt);
fclose (Fp);

return (0);
		
Error:

printf ("SQLCODE=%d\n", SQLCODE);
isc_print_status (gds__status);

return (1);
}

static int addqtr (void)
{
/**************************************
 *
 *	a d d q t r
 *
 **************************************
 *
 * Functional description
 *	Add project quarterly head-count array to 'proj_dept_budget' table.
 *
 **************************************/
TEXT		proj_id[6], dept_no[4];
int		yr;
TEXT		line[81];
int		hcnt[4];
int		rec_cnt = 0;

EXEC SQL SET TRANSACTION;
EXEC SQL WHENEVER SQLERROR GO TO Error;

Fp = fopen ("qtr.inp", "r");

while (fgets (line, 100, Fp) != NULL)
    {
    sscanf (line, "%d %s %s %d %d %d %d", &yr, proj_id, dept_no,
	&hcnt[0], &hcnt[1], &hcnt[2], &hcnt[3]);

    EXEC SQL
	UPDATE proj_dept_budget
	    SET quart_head_cnt = :hcnt
	    WHERE fiscal_year = :yr AND proj_id = :proj_id AND dept_no = :dept_no;

    if (SQLCODE == 0)
	rec_cnt++;
    else
	{
	printf ("Input error -- no job record with key: %d %s %s\n",
	   yr, proj_id, dept_no);
	}
    }

EXEC SQL COMMIT RELEASE;
printf ("Added %d quarter arrays.\n", rec_cnt);
fclose (Fp);

return (0);
		
Error:

printf ("SQLCODE=%d\n", SQLCODE);
isc_print_status (gds__status);

return (1);
}
