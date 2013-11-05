/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		execute_statement.cpp
 *	DESCRIPTION:	Dynamic SQL statements execution
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
 *  Copyright (c) 2003 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "gen/iberror.h"

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/dsc.h"
#include "../jrd/err_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/execute_statement.h"
#include "../dsql/dsql.h"
#include "../common/classes/auto.h"

#include <string.h>
#include <math.h>

using namespace Jrd;
using namespace Firebird;
using Firebird::AutoPtr;


void ExecuteStatement::execute(Jrd::thread_db* tdbb, jrd_req* request, DSC* desc)
{
	SET_TDBB(tdbb);

	Attachment* attachment = tdbb->getAttachment();
	jrd_tra* const transaction = tdbb->getTransaction();

	if (transaction->tra_callback_count >= MAX_CALLBACKS)
	{
		ERR_post(Arg::Gds(isc_exec_sql_max_call_exceeded));
	}

	Firebird::string sqlStatementText;
	getString(tdbb, sqlStatementText, desc, request);

	transaction->tra_callback_count++;

	try
	{
		AutoPtr<PreparedStatement> stmt(attachment->prepareStatement(
			tdbb, *tdbb->getDefaultPool(), transaction, sqlStatementText));

		// Other requests appear to be incorrect in this context
		const long requests =
			(1 << REQ_INSERT) | (1 << REQ_DELETE) | (1 << REQ_UPDATE) |
			(1 << REQ_DDL) | (1 << REQ_SET_GENERATOR) | (1 << REQ_EXEC_PROCEDURE) |
			(1 << REQ_EXEC_BLOCK);

		if (!((1 << stmt->getRequest()->req_type) & requests))
		{
			ERR_post(Arg::Gds(isc_sqlerr) << Arg::Num(-902) <<
					 Arg::Gds(isc_exec_sql_invalid_req) << Arg::Str(sqlStatementText));
		}

		stmt->execute(tdbb, transaction);

		fb_assert(transaction == tdbb->getTransaction());
	}
	catch (const Firebird::Exception&)
	{
		transaction->tra_callback_count--;
		throw;
	}

	transaction->tra_callback_count--;
}


void ExecuteStatement::open(thread_db* tdbb, jrd_nod* sql, SSHORT nVars, bool singleton)
{
	SET_TDBB(tdbb);

	Attachment* const attachment = tdbb->getAttachment();
	jrd_tra* transaction = tdbb->getTransaction();

	if (transaction->tra_callback_count >= MAX_CALLBACKS)
	{
		ERR_post(Arg::Gds(isc_exec_sql_max_call_exceeded));
	}

	varCount = nVars;
	singleMode = singleton;

	Firebird::string sqlText;
	getString(tdbb, sqlText, EVL_expr(tdbb, sql), tdbb->getRequest());
	memcpy(startOfSqlOperator, sqlText.c_str(), sizeof(startOfSqlOperator) - 1);
	startOfSqlOperator[sizeof(startOfSqlOperator) - 1] = 0;

	transaction->tra_callback_count++;

	try
	{
		stmt = attachment->prepareStatement(tdbb, *tdbb->getDefaultPool(), transaction, sqlText);

		if (stmt->getResultCount() == 0)
		{
			delete stmt;
			stmt = NULL;

			ERR_post(Arg::Gds(isc_exec_sql_invalid_req) << Arg::Str(startOfSqlOperator));
		}

		if (stmt->getResultCount() != varCount)
		{
			delete stmt;
			stmt = NULL;

			ERR_post(Arg::Gds(isc_wronumarg));
		}

		resultSet = stmt->executeQuery(tdbb, transaction);

		fb_assert(transaction == tdbb->getTransaction());
	}
	catch (const Firebird::Exception&)
	{
		transaction->tra_callback_count--;
		throw;
	}

	transaction->tra_callback_count--;
}


bool ExecuteStatement::fetch(thread_db* tdbb, jrd_nod** jrdVar)
{
	if (!resultSet->fetch(tdbb))
	{
		delete resultSet;
		resultSet = NULL;
		delete stmt;
		stmt = NULL;
		return false;
	}

	for (int i = 0; i < varCount; i++)
	{
		dsc& desc = resultSet->getDesc(i + 1);
		bool nullFlag = resultSet->isNull(i + 1);
		EXE_assignment(tdbb, jrdVar[i], &desc, nullFlag, NULL, NULL);
	}

	if (singleMode)
	{
		if (!resultSet->fetch(tdbb))
		{
			delete resultSet;
			resultSet = NULL;
			delete stmt;
			stmt = NULL;
			return false;
		}

		ERR_post(Arg::Gds(isc_sing_select_err));
	}

	return true;
}


void ExecuteStatement::close(thread_db* tdbb)
{
	delete resultSet;
	resultSet = NULL;
	delete stmt;
	stmt = NULL;
}


void ExecuteStatement::getString(thread_db* tdbb,
								 Firebird::string& sql,
								 const dsc* desc,
								 const jrd_req* request)
{
	MoveBuffer buffer;

	UCHAR* ptr = NULL;

	const SSHORT len = (desc && !(request->req_flags & req_null)) ?
		MOV_make_string2(tdbb, desc, desc->getTextType(), &ptr, buffer) : 0; // !!! How call Msgs ?

	if (!ptr)
	{
		ERR_post(Arg::Gds(isc_exec_sql_invalid_arg));
	}

	sql.assign((const char*) ptr, len);
}
