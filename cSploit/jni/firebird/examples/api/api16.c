/********************************************************************8
**
**	Program type: 		API
**
**	Description:	This program does an asynchronous event wait
**			on a trigger set up in employee.sales.  
**			Somebody must add a new sales order to alert
**			this program.  That role can be accomplished 
**			by running programs stat12t or api16t at the
**			same time.
**	
**			When the event is fired, this program selects
**			back any new records and updates (current) those
**			records to status "open".  The entire program runs
**			in a single read-committed (wait, no version) 
**			transaction, so transaction doesn't need to be started
**			after an event.
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


/*  Sleep for Windows is a stupid loop with I/O */
#if defined _MSC_VER
#include <windows.h>
#define SLEEP(x)	Sleep(x * 1000 )
#else
#define SLEEP(x) 	sleep(x)
#endif

short 			event_flag = 0;
void			ast_routine (void *, USHORT, const UCHAR *);

int main(int argc, char** argv)
{
	isc_db_handle	DB = NULL;  	/* database handle */
	isc_tr_handle 	trans = NULL; 	/* transaction handle */
	isc_stmt_handle stmt = NULL; 	/* transaction handle */
	ISC_STATUS_ARRAY 	status;		/* status vector */
	char *			event_buffer; 
	char *			result_buffer;
	long		event_id;
	short		length;
	char		dbname[128];
	char		po_number[9];
	XSQLDA *	 sqlda;
	short		nullind = 0;
	char		query[128], update[128];
	long		count[2], i = 0;
	ISC_STATUS_ARRAY Vector;
	char		ids[2][15];
	int             first = 1;
	int	 	ret = 0;
	/* Transaction parameter block for read committed */
	static char  	isc_tpb[5] = {isc_tpb_version1,
				      isc_tpb_write,
				      isc_tpb_read_committed,
				      isc_tpb_wait,
				      isc_tpb_no_rec_version};
	if (argc > 1)
                strcpy(dbname, argv[1]);
    else
		strcpy(dbname, "employee.fdb");


	strcpy (ids[0], "new_order");
	strcpy (ids[1], "change_order");
	count[0] = 0;
	count[1] = 0;

	sqlda = (XSQLDA *) malloc(XSQLDA_LENGTH(1));
	sqlda->sqln = 1;
	sqlda->version = 1;
	sqlda->sqlvar[0].sqldata = po_number;
	sqlda->sqlvar[0].sqlind = &nullind;

	if (isc_attach_database (status, 0, dbname, &DB, 0, NULL))
		{ERREXIT(status, 1)}

	/* SET TRANSACTION ISOLATION LEVEL READ COMMITTED */
	if (isc_start_transaction (status, &trans, 1, &DB, 5, isc_tpb))
		{ERREXIT(status, 1)}

	/* Prepare the query to look at the result tables */
	strcpy(query, " SELECT po_number FROM sales \
		WHERE order_status = 'new'  FOR UPDATE");

	/* This is for the update where current of */
	strcpy(update, 
		"UPDATE sales SET order_status = 'open' WHERE CURRENT OF C");
	isc_dsql_allocate_statement(status, &DB, &stmt);

	if (isc_dsql_prepare(status, &trans, &stmt, 0, query, 1, sqlda))
		{ERREXIT(status, 1)};

	/* Call the cursor "C" */
	isc_dsql_set_cursor_name(status, &stmt, "C", 0);

	/* Allocate an event block and initialize its values 
	**  This will wait on two named events 
	*/
	length = (short)  isc_event_block((char **) &event_buffer,
				(char **) &result_buffer,

				 2, ids[0], ids[1], 0);

	/* Request the server to notify us of our events of interest.
	** When one of the two named events is triggered, the ast_routine
	** will be called with event_buffer, an updated buffer and length
	** que_events returns ast_returns value
	*/
	if(isc_que_events(status, &DB, &event_id, length,
			 event_buffer, ast_routine, result_buffer))
		{ERREXIT(status, 1)}


	while (!ret)
	{

		i++;
		/* If the event was triggered, reset the buffer and re-queue */
		if (event_flag)
		{


                 /* Check for first ast_call.  isc_que_events fires
                    each event to get processing started */

            if ( first )
            {
                      first = 0;
                      event_flag = 0;
            }
            else
            {
                    
			event_flag = 0; 
			/* event_counts will compare the various events 
			** listed in the two buffers and update the 
			** appropriate count_array elements with the difference
			** in the counts in the two buffers.
			*/

			isc_event_counts(Vector, length, (char *) event_buffer,
				(char *) result_buffer);

			/* Look through the count array */
			count[0] += Vector[0];
			count[1] += Vector[1];

			/* Select query to look at triggered events */
			if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
				{ERREXIT(status, 1)}

			while (!isc_dsql_fetch(status, &stmt, 1, sqlda))
			{
				po_number[8] = 0;
				printf("api16: %s\n", po_number);

				/* exit on processing the last example record*/
				if (!strncmp(po_number, "VNEW4", 5))
					ret = 1;

				/* Update current row */
				if(isc_dsql_execute_immediate(status, &DB, 
					&trans, 0, update, 1, NULL))
					{ERREXIT(status, 1)}
			}
			/* Close cursor */
			isc_dsql_free_statement(status, &stmt, DSQL_close);

			}

 	 	 /* Re-queue for the next event */

		 	if (isc_que_events(status, &DB, &event_id, length,
		    	event_buffer, ast_routine, result_buffer))
   		      {ERREXIT(status, 1)}
		}

		/* This does not block, but as a sample program there is nothing
		** else for us to do, so we will take a nap
		*/
		SLEEP(1);
	}
	isc_commit_transaction(status, &trans);

	isc_detach_database (status, &DB);

	printf("Event complete, exiting\n");
	free( sqlda);
	return 0;

}

/*
** The called routine is always called with these three buffers.  Any
** event will land us here .  PLus we get called once when the 
** program first starts.
*/
void ast_routine(void *result, USHORT length, /*const*/ UCHAR *updated)
{
	/* Set the global event flag */


	event_flag++;
	printf("ast routine was called\n");
	/* Copy the updated buffer to the result buffer */
	while (length--)
		*(UCHAR*)result++ = *updated++;
}

