/*
 *	PROGRAM:	Object oriented API samples.
 *	MODULE:		02.update.cpp
 *	DESCRIPTION:	Run once prepared statement with parameters
 *					a few times, committing transaction after each run.
 *					Learns how to prepare statement, manually define parameters
 *					for it and execute that statement with different parameters.
 *
 *					Example for the following interfaces:
 *					IAttachment - database attachment
 *					ITransaction - transaction
 *					IStatement - SQL statement execution
 *					IMessageMetadata - describe input and output data format
 *					IMetadataBuilder - tool to modify/create metadata
 *					IStatus - return state holder
 *
 *					Note that all updates are rolled back in this version. (see *** later)
 *
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
 *					Alex Peshkov, 2013
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ibase.h>
#include <firebird/Provider.h>

using namespace Firebird;

static IMaster* master = fb_get_master_interface();

int get_input(char*, double*);
static const char* Dept_data[] =
     {"622", "100", "116", "900", 0};
static double Percent_data[] =
    {0.05,  1.00,  0.075,  0.10, 0};
int Input_ptr = 0;

static void check(IStatus* s, const char* text)
{
	if (!s->isSuccess())
		throw text;
}

int main()
{
	int rc = 0;

	// set default password if none specified in environment
	setenv("ISC_USER", "sysdba", 0);
	setenv("ISC_PASSWORD", "masterkey", 0);

	// declare pointers to required interfaces
	IStatus* st = NULL;
	IProvider* prov = NULL;
	IAttachment* att = NULL;
	ITransaction* tra = NULL;

	// Interface executes prepared SQL statement
	IStatement* stmt = NULL;

	// Interfaces provides access to format of data in messages
	IMessageMetadata* meta = NULL;

	// Interface makes it possible to change format of data or define it yourself
	IMetadataBuilder* builder = NULL;

	const char* updstr =
	    "UPDATE department SET budget = ? * budget + budget WHERE dept_no = ?";

	try
	{
		// status vector and main dispatcher
		st = master->getStatus();
		prov = master->getDispatcher();

		// attach employee db
		att = prov->attachDatabase(st, "employee", 0, NULL);
		check(st, "attachDatabase");

		// start transaction
		tra = att->startTransaction(st, 0, NULL);
		check(st, "startTransaction");

		// prepare statement
		stmt = att->prepare(st, tra, 0, updstr, 3, 0);
		check(st, "prepare");

		// build metadata
		// IMaster creates empty new metadata in builder
		builder = master->getMetadataBuilder(st, 2);
		check(st, "getMetadataBuilder");
		// set required info on fields
		builder->setType(st, 0, SQL_DOUBLE + 1);
		check(st, "setType");
		builder->setType(st, 1, SQL_TEXT + 1);
		check(st, "setType");
		builder->setLength(st, 1, 3);
		check(st, "setLength");
		// IMetadata should be ready
		meta = builder->getMetadata(st);
		check(st, "getMetadata");
		// no need in builder any more
		builder->release();
		builder = NULL;

		// allocate buffer
		char buffer[256];
		unsigned len = meta->getMessageLength(st);
		check(st, "getMessageLength");
		if (len > 256)
		{
			throw "Input message length too big - can't continue";
		}

		// locations of parameters in input message
		char* dept_no = &buffer[meta->getOffset(st, 1)];
		check(st, "getOffset");
 		double* percent_inc = (double*) &buffer[meta->getOffset(st, 0)];
		check(st, "getOffset");

		// null IDs (set to NOT NULL)
 		short* flag = (short*)&buffer[meta->getNullOffset(st, 0)];
		check(st, "getNullOffset");
 		*flag = 0;
 		flag = (short*) &buffer[meta->getNullOffset(st, 1)];
		check(st, "getNullOffset");
 		*flag = 0;

		// Get the next department-percent increase input pair.
	    while (get_input(dept_no, percent_inc))
    	{
	        printf("\nIncreasing budget for department:  %s  by %5.2lf percent.\n",
    	           dept_no, *percent_inc);

			// Update the budget.
	        stmt->execute(st, tra, meta, buffer, NULL, NULL);
	        if (!st->isSuccess())
    	    {
		        int sqlcode = isc_sqlcode(st->get());
    	        // Don't save the update, if the new budget exceeds the limit.
        	    if (sqlcode == -625)
            	{
	                printf("\tExceeded budget limit -- not updated.\n");

    	            tra->rollbackRetaining(st);
    	            check(st, "rollback");
	                continue;
    	        }

    	        // use default handler
    	        check(st, "execute");
            }

			// Save each department's update independently.
			// *** Change to commitRetaining() to see changes
			// *** tra->commitRetaining(st);
			tra->rollbackRetaining(st);
			check(st, "rollback");
        }

		// close interfaces
		stmt->free(st);
		check(st, "free");
		stmt = NULL;

		meta->release();
		meta = NULL;

		tra->commit(st);
		check(st, "commit");
		tra = NULL;

		att->detach(st);
		check(st, "detach");
		att = NULL;
	}
	catch (const char* text)
	{
		// handle error
		rc = 1;
		fprintf(stderr, "%s:\n", text);
		if (st)
			isc_print_status(st->get());
	}

	// release interfaces after error caught
	if (builder)
		builder->release();
	if (meta)
		meta->release();
	if (stmt)
		stmt->release();
	if (tra)
		tra->release();
	if (att)
		att->release();
	if (prov)
		prov->release();
	if (st)
		st->dispose();

	return rc;
}


/*
 *  Get the department and percent parameters for an example to run.
 */

int get_input (char *dept_no, double *percent)
{
    if (Dept_data[Input_ptr] == 0)
        return 0;
 
    strcpy(dept_no, Dept_data[Input_ptr]);
 
    if ((*percent = Percent_data[Input_ptr++]) == 0)
        return 0;
 
    return 1;
}

