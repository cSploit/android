/*
 *	PROGRAM:	Object oriented API samples.
 *	MODULE:		03.select.cpp
 *	DESCRIPTION:	A sample of running SELECT statement without parameters.
 *					Prints string fields in a table, coercing VARCHAR to CHAR.
 *					Learns how to coerce output data in prepared statement
 *					and execute it.
 *
 *					Example for the following interfaces:
 *
 *					IStatement - SQL statement execution
 *					IMessageMetadata - describe input and output data format
 *					IResultSet - fetch data returned by statement after execution
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2013 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ibase.h>
#include <firebird/Provider.h>

using namespace Firebird;

static IMaster* master = fb_get_master_interface();

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
	IStatement* stmt = NULL;
	IMessageMetadata* meta = NULL;
	IMetadataBuilder* builder = NULL;

	// Interface provides access to data returned by SELECT statement
	IResultSet* curs = NULL;

	try
	{
		// status vector and main dispatcher
		st = master->getStatus();
		prov = master->getDispatcher();

		// attach employee db
		att = prov->attachDatabase(st, "employee", 0, NULL);
		check(st, "attachDatabase");

		// start default transaction
		tra = att->startTransaction(st, 0, NULL);
		check(st, "startTransaction");

		// prepare statement
		stmt = att->prepare(st, tra, 0, "select last_name, first_name, phone_ext from phone_list "
										"where location = 'Monterey' order by last_name, first_name",
			3, IStatement::PREPARE_PREFETCH_METADATA);
		check(st, "prepare");

		// get list of columns
		meta = stmt->getOutputMetadata(st);
		check(st, "getOutputMetadata");
		builder = meta->getBuilder(st);
		check(st, "getBuilder");
		unsigned cols = meta->getCount(st);
		check(st, "getCount");

		// struct to cache received metadata
		struct MyField
		{
			const char* name;
			unsigned length, offset;
		};
		MyField* fields = new MyField[cols];
		memset(fields, 0, sizeof(MyField) * cols);

		// parse columns list & coerce datatype(s)
		for (unsigned j = 0; j < cols; ++j)
		{
			unsigned t = meta->getType(st, j);
			check(st, "getType");

			if (t == SQL_VARYING || t == SQL_TEXT)
			{
				builder->setType(st, j, SQL_TEXT);
				check(st, "setType");
				fields[j].name = meta->getField(st, j);
				check(st, "getField");
			}
		}

		// release automatically created metadata
		// metadata is not database object, therefore no specific call to close it
		meta->release();

		// get metadata with coerced datatypes
		meta = builder->getMetadata(st);
		check(st, "getMetadata");

		// builder not needed any more
		builder->release();
		builder = NULL;

		// now we may also get offsets info
		for (unsigned j = 0; j < cols; ++j)
		{
			if (fields[j].name)
			{
				fields[j].length = meta->getLength(st, j);
				check(st, "getLength");
				fields[j].offset = meta->getOffset(st, j);
				check(st, "getOffset");
			}
		}

		// open cursor
		curs = stmt->openCursor(st, tra, NULL, NULL, meta);
		check(st, "openCursor");

		// allocate output buffer
		unsigned l = meta->getMessageLength(st);
		check(st, "getMessageLength");
		unsigned char* buffer = new unsigned char[l];

		// fetch records from cursor and print them
		while (curs->fetchNext(st, buffer))
		{
			for (unsigned j = 0; j < cols; ++j)
			{
				if (fields[j].name)
				{
					printf("%s: %*.*s\n", fields[j].name, fields[j].length, fields[j].length,
						buffer + fields[j].offset);
				}
			}
			printf("\n");
		}
		check(st, "fetchNext");

		// close interfaces
		curs->close(st);
		check(st, "close");
		curs = NULL;

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
	if (meta)
		meta->release();
	if (builder)
		builder->release();
	if (curs)
		curs->release();
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
