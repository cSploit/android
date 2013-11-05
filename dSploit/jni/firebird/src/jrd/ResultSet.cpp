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

#include "../jrd/ResultSet.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/align.h"
#include "../jrd/jrd.h"
#include "../jrd/dsc.h"
#include "../dsql/dsql.h"
#include "../dsql/sqlda_pub.h"
#include "../dsql/dsql_proto.h"

namespace Jrd {


ResultSet::ResultSet(thread_db* tdbb, PreparedStatement* aStmt, jrd_tra* aTransaction)
	: stmt(aStmt),
	  transaction(aTransaction),
	  firstFetchDone(false)
{
	DSQL_execute(tdbb, &transaction, stmt->request, 0, NULL, 0, 0, NULL, 0, NULL, /*0,*/ 0, NULL);

	stmt->resultSet = this;
}


ResultSet::~ResultSet()
{
	if (!stmt)
		return;

	thread_db* tdbb = JRD_get_thread_data();

	stmt->resultSet = NULL;

	if (stmt->request->req_type != REQ_EXEC_PROCEDURE)
		DSQL_free_statement(tdbb, stmt->request, DSQL_close);
}


bool ResultSet::fetch(thread_db* tdbb)
{
	if (stmt->request->req_type == REQ_EXEC_PROCEDURE && firstFetchDone)
		return false;

	memset(stmt->message.begin(), 0, stmt->message.getCount());

	ISC_STATUS status = DSQL_fetch(tdbb, stmt->request, stmt->blr.getCount(), stmt->blr.begin(),
		/*0,*/ stmt->message.getCount(), stmt->message.begin());

	if (status == 100)
		return false;

	firstFetchDone = true;

	return true;
}


bool ResultSet::isNull(int param) const
{
	const dsc* desc = &stmt->values[(param - 1) * 2 + 1];
	fb_assert(desc->dsc_dtype == dtype_short);

	return *reinterpret_cast<SSHORT*>(desc->dsc_address) != 0;
}


dsc& ResultSet::getDesc(int param)
{
	return stmt->values[(param - 1) * 2];
}


}	// namespace
