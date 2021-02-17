/*
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "ibase.h"
#include "firebird/UdrCppEngine.h"
#include <assert.h>
#include <stdio.h>


using namespace Firebird;
using namespace Firebird::Udr;


namespace
{
	template <typename T>
	class AutoReleaseClear
	{
	public:
		static void clear(T* ptr)
		{
			if (ptr)
				ptr->release();
		}
	};

	template <typename T>
	class AutoDisposeClear
	{
	public:
		static void clear(T* ptr)
		{
			if (ptr)
				ptr->dispose();
		}
	};

	template <typename T>
	class AutoDeleteClear
	{
	public:
		static void clear(T* ptr)
		{
			delete ptr;
		}
	};

	template <typename T>
	class AutoArrayDeleteClear
	{
	public:
		static void clear(T* ptr)
		{
			delete [] ptr;
		}
	};

	template <typename T, typename Clear>
	class AutoImpl
	{
	public:
		AutoImpl<T, Clear>(T* aPtr = NULL)
			: ptr(aPtr)
		{
		}

		~AutoImpl()
		{
			Clear::clear(ptr);
		}

		AutoImpl<T, Clear>& operator =(T* aPtr)
		{
			Clear::clear(ptr);
			ptr = aPtr;
			return *this;
		}

		operator T*()
		{
			return ptr;
		}

		operator const T*() const
		{
			return ptr;
		}

		bool operator !() const
		{
			return !ptr;
		}

		bool hasData() const
		{
			return ptr != NULL;
		}

		T* operator ->()
		{
			return ptr;
		}

		T* release()
		{
			T* tmp = ptr;
			ptr = NULL;
			return tmp;
		}

		void reset(T* aPtr = NULL)
		{
			if (aPtr != ptr)
			{
				Clear::clear(ptr);
				ptr = aPtr;
			}
		}

	private:
		// not implemented
		AutoImpl<T, Clear>(AutoImpl<T, Clear>&);
		void operator =(AutoImpl<T, Clear>&);

	private:
		T* ptr;
	};

	template <typename T> class AutoDispose : public AutoImpl<T, AutoDisposeClear<T> >
	{
	public:
		AutoDispose(T* ptr = NULL)
			: AutoImpl<T, AutoDisposeClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoRelease : public AutoImpl<T, AutoReleaseClear<T> >
	{
	public:
		AutoRelease(T* ptr = NULL)
			: AutoImpl<T, AutoReleaseClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoDelete : public AutoImpl<T, AutoDeleteClear<T> >
	{
	public:
		AutoDelete(T* ptr = NULL)
			: AutoImpl<T, AutoDeleteClear<T> >(ptr)
		{
		}
	};

	template <typename T> class AutoArrayDelete : public AutoImpl<T, AutoArrayDeleteClear<T> >
	{
	public:
		AutoArrayDelete(T* ptr = NULL)
			: AutoImpl<T, AutoArrayDeleteClear<T> >(ptr)
		{
		}
	};
}


//------------------------------------------------------------------------------


/***
create function wait_event (
    event_name varchar(31) character set utf8 not null
) returns integer not null
    external name 'udrcpp_example!wait_event'
    engine udr;
***/
FB_UDR_BEGIN_FUNCTION(wait_event)
	FB_MESSAGE(InMessage,
		(FB_VARCHAR(31 * 4), name)
	);

	FB_MESSAGE(OutMessage,
		(FB_INTEGER, result)
	);

	FB_UDR_EXECUTE_FUNCTION
	{
		char* s = new char[in->name.length + 1];
		memcpy(s, in->name.str, in->name.length);
		s[in->name.length] = '\0';

		unsigned char* eveBuffer;
		unsigned char* eveResult;
		int eveLen = isc_event_block(&eveBuffer, &eveResult, 1, s);

		delete [] s;

		ISC_STATUS_ARRAY statusVector = {0};
		isc_db_handle dbHandle = getIscDbHandle(context);
		ISC_ULONG counter = 0;

		StatusException::checkStatus(isc_wait_for_event(
			statusVector, &dbHandle, eveLen, eveBuffer, eveResult), statusVector);
		isc_event_counts(&counter, eveLen, eveBuffer, eveResult);
		StatusException::checkStatus(isc_wait_for_event(
			statusVector, &dbHandle, eveLen, eveBuffer, eveResult), statusVector);
		isc_event_counts(&counter, eveLen, eveBuffer, eveResult);

		isc_free((char*) eveBuffer);
		isc_free((char*) eveResult);

		out->resultNull = FB_FALSE;
		out->result = counter;
	}
FB_UDR_END_FUNCTION


/***
create function sum_args (
    n1 integer,
    n2 integer,
    n3 integer
) returns integer
    external name 'udrcpp_example!sum_args'
    engine udr;
***/
FB_UDR_BEGIN_FUNCTION(sum_args)
	// Without InMessage/OutMessage definitions, messages will be byte-based.

	FB_UDR_CONSTRUCTOR
		// , inCount(0)
	{
		// Get input metadata.
		AutoRelease<IMessageMetadata> inMetadata(StatusException::check(status,
			metadata->getInputMetadata(status)));

		// Get count of input parameters.
		inCount = StatusException::check(status, inMetadata->getCount(status));

		inNullOffsets.reset(new unsigned[inCount]);
		inOffsets.reset(new unsigned[inCount]);

		for (unsigned i = 0; i < inCount; ++i)
		{
			// Get null offset of the i-th input parameter.
			inNullOffsets[i] = StatusException::check(status, inMetadata->getNullOffset(status, i));

			// Get the offset of the i-th input parameter.
			inOffsets[i] = StatusException::check(status, inMetadata->getOffset(status, i));
		}

		// Get output metadata.
		AutoRelease<IMessageMetadata> outMetadata(StatusException::check(status,
			metadata->getOutputMetadata(status)));

		// Get null offset of the return value.
		outNullOffset = StatusException::check(status, outMetadata->getNullOffset(status, 0));

		// Get offset of the return value.
		outOffset = StatusException::check(status, outMetadata->getOffset(status, 0));
	}

	// This function requires the INTEGER parameters and return value, otherwise it will crash.
	// Metadata is inspected dynamically (in execute). This is not the fastest method.
	FB_UDR_EXECUTE_FUNCTION
	{
		*(ISC_SHORT*) (out + outNullOffset) = FB_FALSE;

		// Get a reference to the return value.
		ISC_LONG& ret = *(ISC_LONG*) (out + outOffset);

		// The return value is automatically initialized to 0.
		///ret = 0;

		for (unsigned i = 0; i < inCount; ++i)
		{
			// If the i-th input parameter is NULL, set the output to NULL and finish.
			if (*(ISC_SHORT*) (in + inNullOffsets[i]))
			{
				*(ISC_SHORT*) (out + outNullOffset) = FB_TRUE;
				return;
			}

			// Read the i-th input parameter value and sum it in the referenced return value.
			ret += *(ISC_LONG*) (in + inOffsets[i]);
		}
	}

	unsigned inCount;
	AutoArrayDelete<unsigned> inNullOffsets;
	AutoArrayDelete<unsigned> inOffsets;
	unsigned outNullOffset;
	unsigned outOffset;
FB_UDR_END_FUNCTION


/***
create procedure gen_rows (
    start_n integer not null,
    end_n integer not null
) returns (
    n integer not null
)
    external name 'udrcpp_example!gen_rows'
    engine udr;
***/
FB_UDR_BEGIN_PROCEDURE(gen_rows)
	// Without InMessage/OutMessage definitions, messages will be byte-based.

	// Procedure variables.
	unsigned inOffsetStart, inOffsetEnd, outNullOffset, outOffset;

	// Get offsets once per procedure.
	FB_UDR_CONSTRUCTOR
	{
		AutoRelease<IMessageMetadata> inMetadata(StatusException::check(status,
			metadata->getInputMetadata(status)));

		inOffsetStart = StatusException::check(status, inMetadata->getOffset(status, 0));
		inOffsetEnd = StatusException::check(status, inMetadata->getOffset(status, 1));

		AutoRelease<IMessageMetadata> outMetadata(StatusException::check(status,
			metadata->getOutputMetadata(status)));

		outNullOffset = StatusException::check(status, outMetadata->getNullOffset(status, 0));
		outOffset = StatusException::check(status, outMetadata->getOffset(status, 0));
	}

	/*** Procedure destructor.
	FB_UDR_DESTRUCTOR
	{
	}
	***/

	FB_UDR_EXECUTE_PROCEDURE
	{
		counter = *(ISC_LONG*) (in + procedure->inOffsetStart);
		end = *(ISC_LONG*) (in + procedure->inOffsetEnd);

		*(ISC_SHORT*) (out + procedure->outNullOffset) = FB_FALSE;
	}

	// After procedure's execute definition, starts the result set definition.

	FB_UDR_FETCH_PROCEDURE
	{
		if (counter > end)
			return false;

		*(ISC_LONG*) (out + procedure->outOffset) = counter++;
		return true;
	}

	/*** ResultSet destructor.
	~ResultSet()
	{
	}
	***/

	// ResultSet variables.
	ISC_LONG counter;
	ISC_LONG end;
FB_UDR_END_PROCEDURE


/***
create procedure gen_rows2 (
    start_n integer not null,
    end_n integer not null
) returns (
    n integer not null
)
    external name 'udrcpp_example!gen_rows2'
    engine udr;
***/
FB_UDR_BEGIN_PROCEDURE(gen_rows2)
	FB_MESSAGE(InMessage,
		(FB_INTEGER, start)
		(FB_INTEGER, end)
	);

	FB_MESSAGE(OutMessage,
		(FB_INTEGER, result)
	);

	FB_UDR_EXECUTE_PROCEDURE
	{
		out->resultNull = FB_FALSE;
		out->result = in->start - 1;
	}

	FB_UDR_FETCH_PROCEDURE
	{
		return out->result++ < in->end;
	}
FB_UDR_END_PROCEDURE


/***
create procedure inc (
    count_n integer not null
) returns (
    n0 integer not null,
    n1 integer not null,
    n2 integer not null,
    n3 integer not null,
    n4 integer not null
)
    external name 'udrcpp_example!inc'
    engine udr;
***/
// This is a sample procedure demonstrating how the scopes of variables works.
// n1 and n2 are on the Procedure scope, i.e., they're shared for each execution of the same cached
// metadata object.
// n3 and n4 are on the ResultSet scope, i.e., each procedure execution have they own instances.
FB_UDR_BEGIN_PROCEDURE(inc)
	FB_MESSAGE(InMessage,
		(FB_INTEGER, count)
	);

	FB_MESSAGE(OutMessage,
		(FB_INTEGER, n0)
		(FB_INTEGER, n1)
		(FB_INTEGER, n2)
		(FB_INTEGER, n3)
		(FB_INTEGER, n4)
	);

	ISC_LONG n1;

	// This is how a procedure (class) initializer is written.
	// ResultSet variables are not accessible here.
	// If there is nothing to initialize, it can be completelly suppressed.
	FB_UDR_CONSTRUCTOR
		, n1(0),
		  n2(0)
	{
	}

	ISC_LONG n2;

	// FB_UDR_EXECUTE_PROCEDURE starts the ResultSet scope.
	FB_UDR_EXECUTE_PROCEDURE
		// This is the ResultSet (class) initializer.
		, n3(procedure->n1),	// n3 will start with the next value for n1 of the last execution
		  n4(0)
	{
		out->n0Null = out->n1Null = out->n2Null = out->n3Null = out->n4Null = FB_FALSE;

		out->n0 = 0;

		// In the execute method, the procedure scope must be accessed using the 'procedure' pointer.
		procedure->n1 = 0;

		// We don't touch n2 here, so it incremented counter will be kept after each execution.

		// The ResultSet scope must be accessed directly, i.e., they're member variables of the
		// 'this' pointer.
		++n4;
	}

	ISC_LONG n3;

	// FB_UDR_FETCH_PROCEDURE must be always after FB_UDR_EXECUTE_PROCEDURE.
	FB_UDR_FETCH_PROCEDURE
	{
		if (out->n0++ <= in->count)
		{
			out->n1 = ++procedure->n1;
			out->n2 = ++procedure->n2;
			out->n3 = ++n3;
			out->n4 = ++n4;
			return true;
		}

		return false;
	}

	ISC_LONG n4;
FB_UDR_END_PROCEDURE


/***
Sample usage:

create database 'c:\temp\slave.fdb';
create table persons (
    id integer not null,
    name varchar(60) not null,
    address varchar(60),
    info blob sub_type text
);
commit;

create database 'c:\temp\master.fdb';
create table persons (
    id integer not null,
    name varchar(60) not null,
    address varchar(60),
    info blob sub_type text
);

create table replicate_config (
    name varchar(31) not null,
    data_source varchar(255) not null
);

insert into replicate_config (name, data_source)
   values ('ds1', 'c:\temp\slave.fdb');

create trigger persons_replicate
    after insert on persons
    external name 'udrcpp_example!replicate!ds1'
    engine udr;

create trigger persons_replicate2
    after insert on persons
    external name 'udrcpp_example!replicate_persons!ds1'
    engine udr;
***/
FB_UDR_BEGIN_TRIGGER(replicate)
	// Without FieldsMessage definition, messages will be byte-based.

	FB_UDR_CONSTRUCTOR
		, triggerMetadata(StatusException::check(status, metadata->getTriggerMetadata(status)))
	{
		ISC_STATUS_ARRAY statusVector = {0};
		isc_db_handle dbHandle = getIscDbHandle(context);
		isc_tr_handle trHandle = getIscTrHandle(context);

		isc_stmt_handle stmtHandle = 0;
		StatusException::checkStatus(isc_dsql_allocate_statement(
			statusVector, &dbHandle, &stmtHandle), statusVector);
		StatusException::checkStatus(isc_dsql_prepare(statusVector, &trHandle, &stmtHandle, 0,
			"select data_source from replicate_config where name = ?",
			SQL_DIALECT_CURRENT, NULL), statusVector);

		const char* table = StatusException::check(status, metadata->getTriggerTable(status));

		// Skip the first exclamation point, separating the module name and entry point.
		const char* info = StatusException::check(status,
			strchr(metadata->getEntryPoint(status), '!'));

		// Skip the second exclamation point, separating the entry point and the misc info (config).
		if (info)
			info = strchr(info + 1, '!');

		if (info)
			++info;
		else
			info = "";

		XSQLDA* inSqlDa = reinterpret_cast<XSQLDA*>(new char[(XSQLDA_LENGTH(1))]);
		inSqlDa->version = SQLDA_VERSION1;
		inSqlDa->sqln = 1;
		StatusException::checkStatus(isc_dsql_describe_bind(statusVector, &stmtHandle,
			SQL_DIALECT_CURRENT, inSqlDa), statusVector);
		inSqlDa->sqlvar[0].sqldata = new char[sizeof(short) + inSqlDa->sqlvar[0].sqllen];
		strncpy(inSqlDa->sqlvar[0].sqldata + sizeof(short), info, inSqlDa->sqlvar[0].sqllen);
		*reinterpret_cast<short*>(inSqlDa->sqlvar[0].sqldata) = strlen(info);

		XSQLDA* outSqlDa = reinterpret_cast<XSQLDA*>(new char[(XSQLDA_LENGTH(1))]);
		outSqlDa->version = SQLDA_VERSION1;
		outSqlDa->sqln = 1;
		StatusException::checkStatus(isc_dsql_describe(statusVector, &stmtHandle,
			SQL_DIALECT_CURRENT, outSqlDa), statusVector);
		outSqlDa->sqlvar[0].sqldata = new char[sizeof(short) + outSqlDa->sqlvar[0].sqllen + 1];
		outSqlDa->sqlvar[0].sqldata[sizeof(short) + outSqlDa->sqlvar[0].sqllen] = '\0';

		StatusException::checkStatus(isc_dsql_execute2(statusVector, &trHandle, &stmtHandle,
			SQL_DIALECT_CURRENT, inSqlDa, outSqlDa), statusVector);
		StatusException::checkStatus(isc_dsql_free_statement(
			statusVector, &stmtHandle, DSQL_unprepare), statusVector);

		delete [] inSqlDa->sqlvar[0].sqldata;
		delete [] reinterpret_cast<char*>(inSqlDa);

		unsigned count = StatusException::check(status, triggerMetadata->getCount(status));

		char buffer[65536];
		strcpy(buffer, "execute block (\n");

		for (unsigned i = 0; i < count; ++i)
		{
			if (i > 0)
				strcat(buffer, ",\n");

			const char* name = StatusException::check(status, triggerMetadata->getField(status, i));

			strcat(buffer, "    p");
			sprintf(buffer + strlen(buffer), "%d type of column \"%s\".\"%s\" = ?", i, table, name);
		}

		strcat(buffer,
			")\n"
			"as\n"
			"begin\n"
			"    execute statement ('insert into \"");

		strcat(buffer, table);
		strcat(buffer, "\" (");

		for (unsigned i = 0; i < count; ++i)
		{
			if (i > 0)
				strcat(buffer, ", ");

			const char* name = StatusException::check(status, triggerMetadata->getField(status, i));

			strcat(buffer, "\"");
			strcat(buffer, name);
			strcat(buffer, "\"");
		}

		strcat(buffer, ") values (");

		for (unsigned i = 0; i < count; ++i)
		{
			if (i > 0)
				strcat(buffer, ", ");
			strcat(buffer, "?");
		}

		strcat(buffer, ")') (");

		for (unsigned i = 0; i < count; ++i)
		{
			if (i > 0)
				strcat(buffer, ", ");
			strcat(buffer, ":p");
			sprintf(buffer + strlen(buffer), "%d", i);
		}

		strcat(buffer, ")\n        on external data source '");
		strcat(buffer, outSqlDa->sqlvar[0].sqldata + sizeof(short));
		strcat(buffer, "';\nend");

		IAttachment* attachment = StatusException::check(status, context->getAttachment(status));
		ITransaction* transaction = StatusException::check(status, context->getTransaction(status));

		stmt.reset(StatusException::check(status,
			attachment->prepare(status, transaction, 0, buffer, SQL_DIALECT_CURRENT, 0)));

		delete [] outSqlDa->sqlvar[0].sqldata;
		delete [] reinterpret_cast<char*>(outSqlDa);
	}

	/***
	FB_UDR_DESTRUCTOR
	{
	}
	***/

	FB_UDR_EXECUTE_TRIGGER
	{
		ITransaction* transaction = StatusException::check(status, context->getTransaction(status));

		// This will not work if the table has computed fields.
		stmt->execute(status, transaction, triggerMetadata, newFields, NULL, NULL);
		StatusException::check(status->get());
	}

	AutoRelease<IMessageMetadata> triggerMetadata;
	AutoRelease<IStatement> stmt;
FB_UDR_END_TRIGGER


FB_UDR_BEGIN_TRIGGER(replicate_persons)
	// Order of fields does not need to match the fields order in the table, but it should match
	// the order of fields in the SQL command constructed in the initialization.
	FB_TRIGGER_MESSAGE(FieldsMessage,
		(FB_INTEGER, id, "ID")
		(FB_BLOB, info, "INFO")
		///(FB_VARCHAR(60 * 4), address, "ADDRESS")
		(FB_VARCHAR(60 * 4), name, "NAME")
	);

	FB_UDR_CONSTRUCTOR
		, triggerMetadata(StatusException::check(status, metadata->getTriggerMetadata(status)))
	{
		ISC_STATUS_ARRAY statusVector = {0};
		isc_db_handle dbHandle = getIscDbHandle(context);
		isc_tr_handle trHandle = getIscTrHandle(context);

		isc_stmt_handle stmtHandle = 0;
		StatusException::checkStatus(isc_dsql_allocate_statement(
			statusVector, &dbHandle, &stmtHandle), statusVector);
		StatusException::checkStatus(isc_dsql_prepare(statusVector, &trHandle, &stmtHandle, 0,
			"select data_source from replicate_config where name = ?",
			SQL_DIALECT_CURRENT, NULL), statusVector);

		const char* table = StatusException::check(status, metadata->getTriggerTable(status));

		// Skip the first exclamation point, separating the module name and entry point.
		const char* info = StatusException::check(status,
			strchr(metadata->getEntryPoint(status), '!'));

		// Skip the second exclamation point, separating the entry point and the misc info (config).
		if (info)
			info = strchr(info + 1, '!');

		if (info)
			++info;
		else
			info = "";

		XSQLDA* inSqlDa = reinterpret_cast<XSQLDA*>(new char[(XSQLDA_LENGTH(1))]);
		inSqlDa->version = SQLDA_VERSION1;
		inSqlDa->sqln = 1;
		StatusException::checkStatus(isc_dsql_describe_bind(
			statusVector, &stmtHandle, SQL_DIALECT_CURRENT, inSqlDa), statusVector);
		inSqlDa->sqlvar[0].sqldata = new char[sizeof(short) + inSqlDa->sqlvar[0].sqllen];
		strncpy(inSqlDa->sqlvar[0].sqldata + sizeof(short), info, inSqlDa->sqlvar[0].sqllen);
		*reinterpret_cast<short*>(inSqlDa->sqlvar[0].sqldata) = strlen(info);

		XSQLDA* outSqlDa = reinterpret_cast<XSQLDA*>(new char[(XSQLDA_LENGTH(1))]);
		outSqlDa->version = SQLDA_VERSION1;
		outSqlDa->sqln = 1;
		StatusException::checkStatus(isc_dsql_describe(
			statusVector, &stmtHandle, SQL_DIALECT_CURRENT, outSqlDa), statusVector);
		outSqlDa->sqlvar[0].sqldata = new char[sizeof(short) + outSqlDa->sqlvar[0].sqllen + 1];
		outSqlDa->sqlvar[0].sqldata[sizeof(short) + outSqlDa->sqlvar[0].sqllen] = '\0';

		StatusException::checkStatus(isc_dsql_execute2(statusVector, &trHandle, &stmtHandle,
			SQL_DIALECT_CURRENT, inSqlDa, outSqlDa), statusVector);
		StatusException::checkStatus(isc_dsql_free_statement(
			statusVector, &stmtHandle, DSQL_unprepare), statusVector);

		delete [] inSqlDa->sqlvar[0].sqldata;
		delete [] reinterpret_cast<char*>(inSqlDa);

		char buffer[65536];
		strcpy(buffer,
			"execute block (\n"
			"    id type of column PERSONS.ID = ?,\n"
			"    info type of column PERSONS.INFO = ?,\n"
			///"    address type of column PERSONS.ADDRESS = ?,\n"
			"    name type of column PERSONS.NAME = ?\n"
			")"
			"as\n"
			"begin\n"
			"    execute statement ('insert into persons (id, name/***, address***/, info)\n"
			"        values (?, ?/***, ?***/, ?)') (:id, :name/***, :address***/, :info)\n"
			"        on external data source '");
		strcat(buffer, outSqlDa->sqlvar[0].sqldata + sizeof(short));
		strcat(buffer, "';\nend");

		IAttachment* attachment = StatusException::check(status, context->getAttachment(status));
		ITransaction* transaction = StatusException::check(status, context->getTransaction(status));

		stmt.reset(StatusException::check(status,
			attachment->prepare(status, transaction, 0, buffer, SQL_DIALECT_CURRENT, 0)));

		delete [] outSqlDa->sqlvar[0].sqldata;
		delete [] reinterpret_cast<char*>(outSqlDa);
	}

	/***
	FB_UDR_DESTRUCTOR
	{
	}
	***/

	FB_UDR_EXECUTE_TRIGGER
	{
		ITransaction* transaction = StatusException::check(status, context->getTransaction(status));

		stmt->execute(status, transaction, triggerMetadata, newFields, NULL, NULL);
		StatusException::check(status->get());
	}

	AutoRelease<IMessageMetadata> triggerMetadata;
	AutoRelease<IStatement> stmt;
FB_UDR_END_TRIGGER
