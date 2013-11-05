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

#ifndef DSQL_NODES_H
#define DSQL_NODES_H

#include "../jrd/common.h"
#include "../dsql/dsql.h"

namespace Jrd {

class CompilerScratch;


class Node : public Firebird::PermanentStorage
{
public:
	explicit Node(MemoryPool& pool)
		: PermanentStorage(pool),
		  compiledStatement(NULL)
	{
	}

	virtual ~Node()
	{
	}

public:
	Node* dsqlPass(CompiledStatement* aCompiledStatement)
	{
		compiledStatement = aCompiledStatement;
		return dsqlPass();
	}

public:
	virtual void print(Firebird::string& text, Firebird::Array<dsql_nod*>& nodes) const = 0;

protected:
	virtual Node* dsqlPass()
	{
		return this;
	}

protected:
	CompiledStatement* compiledStatement;
};


class DdlNode : public Node
{
public:
	explicit DdlNode(MemoryPool& pool)
		: Node(pool)
	{
	}

protected:
	virtual Node* dsqlPass()
	{
		compiledStatement->req_type = REQ_DDL;
		return this;
	}

public:
	virtual void execute(thread_db* tdbb, jrd_tra* transaction) = 0;
};


class DmlNode : public Node
{
public:
	explicit DmlNode(MemoryPool& pool)
		: Node(pool)
	{
	}

public:
	virtual DmlNode* pass2(thread_db* tdbb, CompilerScratch* csb, jrd_nod* aNode);

public:
	virtual void genBlr() = 0;
	virtual DmlNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual DmlNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual jrd_nod* execute(thread_db* tdbb, jrd_req* request) = 0;

protected:
	jrd_nod* node;
};


class StmtNode : public DmlNode
{
public:
	explicit StmtNode(MemoryPool& pool)
		: DmlNode(pool)
	{
	}
};


} // namespace

#endif // DSQL_NODES_H
