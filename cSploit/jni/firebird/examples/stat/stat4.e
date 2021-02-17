/*
 *  Program type:   Embedded Static SQL
 *
 *  Description:
 *		This program declares and creates a new table.
 *		Some rows, describing a department structure,
 *		are added to the new table as a test.
 *
 *		The table is created temporarily for figuring
 *		out which level in the department structure each
 *		department belongs too.
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

void build_tree (void);
void pr_error (void);

EXEC SQL
	BEGIN DECLARE SECTION;
EXEC SQL
	END DECLARE SECTION;


int main (void)
{
	char	dept[4];
	int		lvl = 1;

	/* Describe the new table's structure. */
	EXEC SQL
		DECLARE tmp_dept_tree TABLE (
			dept_no CHAR(3) NOT NULL PRIMARY KEY,
			tree_level INTEGER);

	/* Drop the table in case it exists. Ignore error */
	EXEC SQL
		DROP TABLE tmp_dept_tree;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error1;
	/* Create the new table. */
	printf ("creating tmp_dept_tree\n");
	EXEC SQL
		CREATE TABLE tmp_dept_tree (
			dept_no CHAR(3) NOT NULL PRIMARY KEY,
			tree_level INTEGER);

	/* Fill the new table. */
	build_tree();

	/* Look at it */
	EXEC SQL DECLARE dc CURSOR FOR
		SELECT dept_no, tree_level
		FROM tmp_dept_tree;

	EXEC SQL 
		OPEN dc;

	EXEC SQL 
		FETCH dc INTO :dept, :lvl;

	while (SQLCODE == 0)
	{
	printf ("Dept = %s:  Level = %d\n", dept, lvl);
	EXEC SQL 
		FETCH dc INTO :dept, :lvl;
	}
	EXEC SQL
		COMMIT RELEASE;

	return 0;

Error1:
		pr_error();
	return 1;
}


/*
 *  For each department find its sub-departments and mark them
 *  with the next level number.
 */
void build_tree (void)
{
	char	dept[4];
	int		lvl = 1;

	EXEC SQL
		WHENEVER SQLERROR GO TO Error2;

	/* Initialize the department structure by adding the first level.  */
	EXEC SQL
		INSERT INTO tmp_dept_tree VALUES ('000', :lvl);

	/* Declare the cursor for selecting all departments with level 'lvl'. */
	EXEC SQL
		DECLARE ds CURSOR FOR
		SELECT dept_no
		FROM tmp_dept_tree
		WHERE tree_level = :lvl;
	
	/* For each department with level 'lvl', find its sub-departments. */
	while (SQLCODE == 0)
	{
		EXEC SQL
			OPEN ds;

		/* Initialize the next level. */
		lvl++;

		/* Add all the sub-departments of the next level to the table. */
		for (;;)
		{
			EXEC SQL
				FETCH ds INTO :dept;

			if (SQLCODE == 100)
				break;

			EXEC SQL
				INSERT INTO tmp_dept_tree
				SELECT dept_no, :lvl
				FROM department
				WHERE head_dept = :dept;
		}

		EXEC SQL
			CLOSE ds;
		EXEC SQL
			COMMIT;

		/* Done, if no next level was created by the INSERT above. */
		EXEC SQL
			SELECT tree_level
			FROM tmp_dept_tree
			WHERE tree_level = :lvl;

		if (SQLCODE == 100)
			return;
	}

Error2:
	pr_error();
	return;
}


/*
 *	Print any error and exit.
 */
void pr_error (void)
{
	isc_print_sqlerror((short)SQLCODE, gds__status);
}

