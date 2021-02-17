/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program selects a blob data type.
 *		A set of project descriptions is printed.
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
#include <stdio.h>

EXEC SQL
	BEGIN DECLARE SECTION;

BASED ON project.proj_name			proj_name;
BASED ON project.product			prod_type;

BASED ON project.proj_desc			blob_id;
BASED ON project.proj_desc.SEGMENT		blob_segment;
unsigned short					blob_seg_len;

EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	EXEC SQL
		WHENEVER SQLERROR GO TO Error;

	/* Declare a table read cursor. */
	EXEC SQL
		DECLARE proj_cur CURSOR FOR
		SELECT proj_name, proj_desc, product
		FROM project
		WHERE product IN ('software', 'hardware', 'other')
		ORDER BY proj_name;

	/* Declare a blob read cursor. */
	EXEC SQL
		DECLARE blob_cur CURSOR FOR
		READ BLOB proj_desc
		FROM project;

	/* Open the table cursor. */
	EXEC SQL
		OPEN proj_cur;

	/*
	 *  For each project get and display project description.
	 */

	while (SQLCODE == 0)
	{
		/* Fetch the blob id along with some other columns. */
		EXEC SQL
			FETCH proj_cur INTO :proj_name, :blob_id, :prod_type;

		if (SQLCODE == 100)
			break;

		printf("\nPROJECT:  %-30s   TYPE:  %-15s\n\n", proj_name, prod_type);

		/* Open the blob cursor. */
		EXEC SQL
			OPEN blob_cur USING :blob_id;

		while (SQLCODE == 0)
		{
			/* Fetch a blob segment and a blob segment length. */
			EXEC SQL FETCH blob_cur INTO :blob_segment :blob_seg_len;

			if (SQLCODE == 100)
				break;

			printf("  %*.*s\n", blob_seg_len, blob_seg_len, blob_segment);
		}
		printf("\n");

		/* Close the blob cursor. */
		EXEC SQL
			CLOSE blob_cur;
	}

	EXEC SQL
		CLOSE proj_cur;

	EXEC SQL
		COMMIT;

	return 0;

Error:
	isc_print_sqlerror((short) SQLCODE, gds__status);
	return 1;
}

