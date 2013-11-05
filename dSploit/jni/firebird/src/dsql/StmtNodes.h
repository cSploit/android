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

#ifndef DSQL_STMT_NODES_H
#define DSQL_STMT_NODES_H

#include "../jrd/common.h"
#include "../dsql/Nodes.h"

namespace Jrd {


class InAutonomousTransactionNode : public StmtNode
{
public:
	explicit InAutonomousTransactionNode(MemoryPool& pool)
		: StmtNode(pool),
		  dsqlAction(NULL),
		  action(NULL),
		  savNumberOffset(0)
	{
	}

public:
	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb);

protected:
	virtual InAutonomousTransactionNode* dsqlPass();

public:
	virtual void print(Firebird::string& text, Firebird::Array<dsql_nod*>& nodes) const;
	virtual void genBlr();
	virtual InAutonomousTransactionNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual InAutonomousTransactionNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual jrd_nod* execute(thread_db* tdbb, jrd_req* request);

public:
	dsql_nod* dsqlAction;
	jrd_nod* action;
	SLONG savNumberOffset;
};


} // namespace

#endif // DSQL_STMT_NODES_H
