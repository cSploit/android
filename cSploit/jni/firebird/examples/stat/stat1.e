/*
 *	Program type:   Embedded Static SQL
 *
 *	Description:
 *		This program performs a simple update to an existing
 *		table, asks the user whether to save the update, and
 *		commits or undoes the transaction accordingly.
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
#include <stdio.h>
#include <stdlib.h>

int do_save (void);
void clean_up (void);

EXEC SQL	
	BEGIN DECLARE SECTION;
EXEC SQL	
	END DECLARE SECTION;


int main (void)
{
	clean_up();

	/* Insert a new row. */
	EXEC SQL
		INSERT INTO country (country, currency)
		VALUES ('Mexico', 'Peso');

	/* Check the SQLCODE directly */
	if (SQLCODE)
	{
		isc_print_sqlerror((short)SQLCODE, gds__status);
		exit(1);
	}

	printf("\nAdding:  country = 'Mexico', currency = 'Peso'\n\n");

	/* Confirm whether to commit the update. */
	if (do_save())
	{
		EXEC SQL
			COMMIT RELEASE;
		printf("\nSAVED.\n\n");
	}
	else
	{
		EXEC SQL
			ROLLBACK RELEASE;
		printf("\nUNDONE.\n\n");
	}
	return 0;
}


/*
 *	Ask the user whether to save the newly added row.
 */
int do_save (void)
{
	char	answer[10];

	printf("Save?  Enter 'y' for yes, 'n' for no:  ");
	gets(answer);

	return (*answer == 'y' ? 1 : 0);
}


/*
 *	If this is not the first time this program is run,
 *	the example row may already exist -- delete the example
 *	row in order to avoid a duplicate value error.
 */
void clean_up (void)
{
	EXEC SQL
		DELETE FROM country
		WHERE country =  'Mexico';

	EXEC SQL
		COMMIT WORK;
}

