/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		execute_statement.h
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

#ifndef JRD_EXECUTE_STATEMENT_H
#define JRD_EXECUTE_STATEMENT_H

#include "../include/fb_blk.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/ResultSet.h"
#include "../jrd/exe.h"
#include "../jrd/ibase.h"

namespace Jrd {


class ExecuteStatement
{
public:
	static void execute(Jrd::thread_db* tdbb, Jrd::jrd_req* request, DSC* desc);
	void open(Jrd::thread_db* tdbb, Jrd::jrd_nod* sql, SSHORT nVars, bool singleton);
	bool fetch(Jrd::thread_db* tdbb, Jrd::jrd_nod** jrdVar);
	void close(Jrd::thread_db* tdbb);

	static void getString(Jrd::thread_db* tdbb, Firebird::string& sql, const dsc* desc,
		const Jrd::jrd_req* request);

private:
	PreparedStatement* stmt;
	ResultSet* resultSet;
	int varCount;
	bool singleMode;
	TEXT startOfSqlOperator[32];
};


} // namespace

#endif // JRD_EXECUTE_STATEMENT_H
