/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program updates a blob data type.
 *		Project descriptions are added for a set of projects.
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

#include "example.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *get_line (void);

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

EXEC SQL
	BEGIN DECLARE SECTION;
EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	BASED_ON project.proj_id	proj_id;
	ISC_QUAD	blob_id;
	int		len;
	char *		line; 
	int		rec_cnt = 0;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/* Declare a blob insert cursor. */
	EXEC SQL
		DECLARE bc CURSOR FOR
		INSERT BLOB proj_desc INTO project;

	/*
	 *  Get the next project id and update the project description.
	 */
	line = get_line();
	while (line)
	{
		/* Open the blob cursor. */
		EXEC SQL
			OPEN bc INTO :blob_id;

		strcpy(proj_id, line);
		printf("\nUpdating description for project:  %s\n\n", proj_id);

		/* Get a project description segment. */
		line = get_line();	
		while (line)
		{
			printf("  Inserting segment:  %s\n", line);

			/* Calculate the length of the segment. */
			len = strlen(line);

			/* Write the segment. */
			EXEC SQL INSERT CURSOR bc VALUES (:line INDICATOR :len);
			line = get_line();
		}

		/* Close the blob cursor. */
		EXEC SQL
			CLOSE bc;

		/* Save the blob id in the project record. */
		EXEC SQL
			UPDATE project
			SET proj_desc = :blob_id
			WHERE proj_id = :proj_id;

		if (SQLCODE == 0L)
			rec_cnt++;
		else
			printf("Input error -- no project record with key: %s\n", proj_id);
		line = get_line();
	}

	EXEC SQL
		COMMIT RELEASE;

	printf("\n\nAdded %d project descriptions.\n", rec_cnt);
	return 0;
		
Error:
	isc_print_sqlerror((short) SQLCODE, isc_status);
	return 1;
}


/*
 *	Get the next input line, which is either a project id
 *	or a project description segment.
 */
char *get_line (void)
{
	return Proj_data[Inp_ptr++];
}

