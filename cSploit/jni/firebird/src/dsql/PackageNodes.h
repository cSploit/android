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
 *  Copyright (c) 2009 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef DSQL_PACKAGE_NODES_H
#define DSQL_PACKAGE_NODES_H

#include "../dsql/DdlNodes.h"
#include "../common/classes/array.h"

namespace Jrd {


class CreateAlterPackageNode : public DdlNode
{
public:
	struct Item
	{
		static Item create(CreateAlterFunctionNode* function)
		{
			Item item;
			item.type = FUNCTION;
			item.function = function;
			item.dsqlScratch = NULL;
			return item;
		}

		static Item create(CreateAlterProcedureNode* procedure)
		{
			Item item;
			item.type = PROCEDURE;
			item.procedure = procedure;
			item.dsqlScratch = NULL;
			return item;
		}

		enum
		{
			FUNCTION,
			PROCEDURE
		} type;

		union
		{
			CreateAlterFunctionNode* function;
			CreateAlterProcedureNode* procedure;
		};

		DsqlCompilerScratch* dsqlScratch;
	};

public:
	CreateAlterPackageNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: DdlNode(pool),
		  name(pool, aName),
		  create(true),
		  alter(false),
		  source(pool),
		  items(NULL),
		  functionNames(pool),
		  procedureNames(pool),
		  owner(pool)
	{
	}

public:
	virtual DdlNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void print(Firebird::string& text) const;
	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);

protected:
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector)
	{
		statusVector <<
			Firebird::Arg::Gds(createAlterCode(create, alter,
					isc_dsql_create_pack_failed, isc_dsql_alter_pack_failed,
					isc_dsql_create_alter_pack_failed)) <<
				name;
	}

private:
	void executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);
	bool executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);

public:
	Firebird::MetaName name;
	bool create;
	bool alter;
	Firebird::string source;
	Firebird::Array<Item>* items;
	Firebird::SortedArray<Firebird::MetaName> functionNames;
	Firebird::SortedArray<Firebird::MetaName> procedureNames;

private:
	Firebird::string owner;
};


class DropPackageNode : public DdlNode
{
public:
	DropPackageNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: DdlNode(pool),
		  name(pool, aName),
		  silent(false)
	{
	}

public:
	virtual void print(Firebird::string& text) const;
	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);

protected:
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector)
	{
		statusVector << Firebird::Arg::Gds(isc_dsql_drop_pack_failed) << name;
	}

public:
	Firebird::MetaName name;
	bool silent;
};


typedef RecreateNode<CreateAlterPackageNode, DropPackageNode, isc_dsql_recreate_pack_failed>
	RecreatePackageNode;


class CreatePackageBodyNode : public DdlNode
{
public:
	CreatePackageBodyNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: DdlNode(pool),
		  name(pool, aName),
		  source(pool),
		  declaredItems(NULL),
		  items(NULL),
		  owner(pool)
	{
	}

public:
	virtual DdlNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void print(Firebird::string& text) const;
	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);

protected:
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector)
	{
		statusVector << Firebird::Arg::Gds(isc_dsql_create_pack_body_failed) << name;
	}

public:
	Firebird::MetaName name;
	Firebird::string source;
	Firebird::Array<CreateAlterPackageNode::Item>* declaredItems;
	Firebird::Array<CreateAlterPackageNode::Item>* items;

private:
	Firebird::string owner;
};


class DropPackageBodyNode : public DdlNode
{
public:
	DropPackageBodyNode(MemoryPool& pool, const Firebird::MetaName& aName)
		: DdlNode(pool),
		  name(pool, aName),
		  silent(false)
	{
	}

public:
	virtual void print(Firebird::string& text) const;
	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction);

protected:
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector)
	{
		statusVector << Firebird::Arg::Gds(isc_dsql_drop_pack_body_failed) << name;
	}

public:
	Firebird::MetaName name;
	bool silent;	// Unused. Just to please RecreateNode template.
};


typedef RecreateNode<CreatePackageBodyNode, DropPackageBodyNode, isc_dsql_recreate_pack_body_failed>
	RecreatePackageBodyNode;


} // namespace

#endif // DSQL_PACKAGE_NODES_H
