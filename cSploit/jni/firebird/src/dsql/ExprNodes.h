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
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef DSQL_EXPR_NODES_H
#define DSQL_EXPR_NODES_H

#include "../jrd/blr.h"
#include "../dsql/Nodes.h"
#include "../dsql/pass1_proto.h"

class SysFunction;

namespace Jrd {

struct ItemInfo;
class DeclareVariableNode;
class RecordSource;
class RelationSourceNode;
class ValueListNode;


class ArithmeticNode : public TypedNode<ValueExprNode, ExprNode::TYPE_ARITHMETIC>
{
public:
	ArithmeticNode(MemoryPool& pool, UCHAR aBlrOp, bool aDialect1,
		ValueExprNode* aArg1 = NULL, ValueExprNode* aArg2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	// add and add2 are used in somewhat obscure way in aggregation.
	static dsc* add(const dsc* desc, impure_value* value, const ValueExprNode* node, const UCHAR blrOp);
	static dsc* add2(const dsc* desc, impure_value* value, const ValueExprNode* node, const UCHAR blrOp);

private:
	dsc* multiply(const dsc* desc, impure_value* value) const;
	dsc* multiply2(const dsc* desc, impure_value* value) const;
	dsc* divide2(const dsc* desc, impure_value* value) const;
	dsc* addDateTime(const dsc* desc, impure_value* value) const;
	dsc* addSqlDate(const dsc* desc, impure_value* value) const;
	dsc* addSqlTime(const dsc* desc, impure_value* value) const;
	dsc* addTimeStamp(const dsc* desc, impure_value* value) const;

private:
	void makeDialect1(dsc* desc, dsc& desc1, dsc& desc2);
	void makeDialect3(dsc* desc, dsc& desc1, dsc& desc2);

	void getDescDialect1(thread_db* tdbb, dsc* desc, dsc& desc1, dsc& desc2);
	void getDescDialect3(thread_db* tdbb, dsc* desc, dsc& desc1, dsc& desc2);

public:
	const UCHAR blrOp;
	bool dialect1;
	Firebird::string label;
	NestConst<ValueExprNode> arg1;
	NestConst<ValueExprNode> arg2;
};


class ArrayNode : public TypedNode<ValueExprNode, ExprNode::TYPE_ARRAY>
{
public:
	ArrayNode(MemoryPool& pool, FieldNode* aField);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	// This class is used only in the parser. It turns in a FieldNode in dsqlPass.

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	NestConst<FieldNode> field;
};


class BoolAsValueNode : public TypedNode<ValueExprNode, ExprNode::TYPE_BOOL_AS_VALUE>
{
public:
	explicit BoolAsValueNode(MemoryPool& pool, BoolExprNode* aBoolean = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
	}

	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<BoolExprNode> boolean;
};


class CastNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CAST>
{
public:
	explicit CastNode(MemoryPool& pool, ValueExprNode* aSource = NULL, dsql_fld* aDsqlField = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::MetaName dsqlAlias;
	dsql_fld* dsqlField;
	dsc castDesc;
	NestConst<ValueExprNode> source;
	NestConst<ItemInfo> itemInfo;
};


class CoalesceNode : public TypedNode<ValueExprNode, ExprNode::TYPE_COALESCE>
{
public:
	explicit CoalesceNode(MemoryPool& pool, ValueListNode* aArgs = NULL)
		: TypedNode<ValueExprNode, ExprNode::TYPE_COALESCE>(pool),
		  args(aArgs)
	{
		addChildNode(args, args);
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueListNode> args;
};


class CollateNode : public TypedNode<ValueExprNode, ExprNode::TYPE_COLLATE>
{
public:
	CollateNode(MemoryPool& pool, ValueExprNode* aArg, const Firebird::MetaName& aCollation);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	static ValueExprNode* pass1Collate(DsqlCompilerScratch* dsqlScratch, ValueExprNode* input,
		const Firebird::MetaName& collation);

	// This class is used only in the parser. It turns in a CastNode in dsqlPass.

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

private:
	static void assignFieldDtypeFromDsc(dsql_fld* field, const dsc* desc);

public:
	NestConst<ValueExprNode> arg;
	Firebird::MetaName collation;
};


class ConcatenateNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CONCATENATE>
{
public:
	explicit ConcatenateNode(MemoryPool& pool, ValueExprNode* aArg1 = NULL, ValueExprNode* aArg2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> arg1;
	NestConst<ValueExprNode> arg2;
};


class CurrentDateNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_DATE>
{
public:
	explicit CurrentDateNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_DATE>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;
};


class CurrentTimeNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIME>
{
public:
	CurrentTimeNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIME>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	unsigned precision;
};


class CurrentTimeStampNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIMESTAMP>
{
public:
	CurrentTimeStampNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIMESTAMP>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	unsigned precision;
};


class CurrentRoleNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_ROLE>
{
public:
	explicit CurrentRoleNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_ROLE>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;
};


class CurrentUserNode : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_USER>
{
public:
	explicit CurrentUserNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_USER>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;
};


class DecodeNode : public TypedNode<ValueExprNode, ExprNode::TYPE_DECODE>
{
public:
	explicit DecodeNode(MemoryPool& pool, ValueExprNode* aTest = NULL,
				ValueListNode* aConditions = NULL, ValueListNode* aValues = NULL)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DECODE>(pool),
		  label(pool),
		  test(aTest),
		  conditions(aConditions),
		  values(aValues)
	{
		addChildNode(test, test);
		addChildNode(conditions, conditions);
		addChildNode(values, values);

		label = "DECODE";
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::string label;
	NestConst<ValueExprNode> test;
	NestConst<ValueListNode> conditions;
	NestConst<ValueListNode> values;
};


class DerivedExprNode : public TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_EXPR>
{
public:
	explicit DerivedExprNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_EXPR>(pool),
		  arg(NULL),
		  internalStreamList(pool)
	{
		addChildNode(arg);
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void print(Firebird::string& /*text*/) const
	{
		fb_assert(false);
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> arg;
	Firebird::Array<StreamType> internalStreamList;
};


class DomainValidationNode : public TypedNode<ValueExprNode, ExprNode::TYPE_DOMAIN_VALIDATION>
{
public:
	explicit DomainValidationNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DOMAIN_VALIDATION>(pool)
	{
		domDesc.clear();
	}

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
	}

	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	dsc domDesc;
};


class ExtractNode : public TypedNode<ValueExprNode, ExprNode::TYPE_EXTRACT>
{
public:
	ExtractNode(MemoryPool& pool, UCHAR aBlrSubOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	UCHAR blrSubOp;
	NestConst<ValueExprNode> arg;
};


class FieldNode : public TypedNode<ValueExprNode, ExprNode::TYPE_FIELD>
{
public:
	FieldNode(MemoryPool& pool, dsql_ctx* context = NULL, dsql_fld* field = NULL, ValueListNode* indices = NULL);
	FieldNode(MemoryPool& pool, StreamType stream, USHORT id, bool aById);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	ValueExprNode* internalDsqlPass(DsqlCompilerScratch* dsqlScratch, RecordSourceNode** list);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;

	virtual bool possiblyUnknown()
	{
		return false;
	}

	virtual void collectStreams(SortedStreamList& streamList) const
	{
		if (!streamList.exist(fieldStream))
			streamList.add(fieldStream);
	}

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return true;
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

private:
	static dsql_fld* resolveContext(DsqlCompilerScratch* dsqlScratch,
		const Firebird::MetaName& qualifier, dsql_ctx* context, bool resolveByAlias);

public:
	Firebird::MetaName dsqlQualifier;
	Firebird::MetaName dsqlName;
	dsql_ctx* const dsqlContext;
	dsql_fld* const dsqlField;
	NestConst<ValueListNode> dsqlIndices;
	const StreamType fieldStream;
	const Format* format;
	const USHORT fieldId;
	const bool byId;
};


class GenIdNode : public TypedNode<ValueExprNode, ExprNode::TYPE_GEN_ID>
{
public:
	GenIdNode(MemoryPool& pool, bool aDialect1,
			  const Firebird::MetaName& name,
			  ValueExprNode* aArg, bool aImplicit);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	const bool dialect1;
	GeneratorItem generator;
	NestConst<ValueExprNode> arg;

private:
	SLONG step;
	bool sysGen;
	const bool implicit;
};


class InternalInfoNode : public TypedNode<ValueExprNode, ExprNode::TYPE_INTERNAL_INFO>
{
public:
	struct InfoAttr
	{
		const char* alias;
		unsigned mask;
	};

	static const InfoAttr INFO_TYPE_ATTRIBUTES[MAX_INFO_TYPE];

	explicit InternalInfoNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> arg;
};


class LiteralNode : public TypedNode<ValueExprNode, ExprNode::TYPE_LITERAL>
{
public:
	explicit LiteralNode(MemoryPool& pool);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);
	static void genConstant(DsqlCompilerScratch* dsqlScratch, const dsc* desc, bool negateValue);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	SLONG getSlong() const
	{
		fb_assert(litDesc.dsc_dtype == dtype_long);
		return *reinterpret_cast<SLONG*>(litDesc.dsc_address);
	}

public:
	const IntlString* dsqlStr;
	dsc litDesc;
};


class DsqlAliasNode : public TypedNode<ValueExprNode, ExprNode::TYPE_ALIAS>
{
public:
	DsqlAliasNode(MemoryPool& pool, const Firebird::MetaName& aName, ValueExprNode* aValue)
		: TypedNode<ValueExprNode, ExprNode::TYPE_ALIAS>(pool),
		  name(aName),
		  value(aValue),
		  implicitJoin(NULL)
	{
		addDsqlChildNode(value);
	}

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	const Firebird::MetaName name;
	NestConst<ValueExprNode> value;
	NestConst<ImplicitJoin> implicitJoin;
};


class DsqlMapNode : public TypedNode<ValueExprNode, ExprNode::TYPE_MAP>
{
public:
	DsqlMapNode(MemoryPool& pool, dsql_ctx* aContext, dsql_map* aMap);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	dsql_ctx* context;
	dsql_map* map;
};


class DerivedFieldNode : public TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_FIELD>
{
public:
	DerivedFieldNode(MemoryPool& pool, const Firebird::MetaName& aName, USHORT aScope,
		ValueExprNode* aValue);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	Firebird::MetaName name;
	USHORT scope;
	NestConst<ValueExprNode> value;
	dsql_ctx* context;
};


class NegateNode : public TypedNode<ValueExprNode, ExprNode::TYPE_NEGATE>
{
public:
	explicit NegateNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> arg;
};


class NullNode : public TypedNode<ValueExprNode, ExprNode::TYPE_NULL>
{
public:
	explicit NullNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_NULL>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;
};


class OrderNode : public TypedNode<ValueExprNode, ExprNode::TYPE_ORDER>
{
public:
	enum NullsPlacement
	{
		NULLS_DEFAULT,
		NULLS_FIRST,
		NULLS_LAST
	};

	OrderNode(MemoryPool& pool, ValueExprNode* aValue);

	virtual void print(Firebird::string& text) const;
	virtual OrderNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	NestConst<ValueExprNode> value;
	bool descending;
	NullsPlacement nullsPlacement;
};


// OVER is used only in DSQL. In the engine, normal aggregate functions are used in partitioned
// maps.
class OverNode : public TypedNode<ValueExprNode, ExprNode::TYPE_OVER>
{
public:
	explicit OverNode(MemoryPool& pool, AggNode* aAggExpr = NULL, ValueListNode* aPartition = NULL,
		ValueListNode* aOrder = NULL);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> aggExpr;
	NestConst<ValueListNode> partition;
	NestConst<ValueListNode> order;
};


class ParameterNode : public TypedNode<ValueExprNode, ExprNode::TYPE_PARAMETER>
{
private:
	// CVC: This is a guess for the length of the parameter for LIKE and others, when the
	// original dtype isn't string and force_varchar is true.
	static const int LIKE_PARAM_LEN = 30;

public:
	explicit ParameterNode(MemoryPool& pool);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
	}

	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	USHORT dsqlParameterIndex;
	dsql_par* dsqlParameter;
	NestConst<MessageNode> message;
	USHORT argNumber;
	NestConst<ValueExprNode> argFlag;
	NestConst<ValueExprNode> argIndicator;
	NestConst<ItemInfo> argInfo;
};


class RecordKeyNode : public TypedNode<ValueExprNode, ExprNode::TYPE_RECORD_KEY>
{
public:
	RecordKeyNode(MemoryPool& pool, UCHAR aBlrOp, const Firebird::MetaName& aDsqlQualifier = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool possiblyUnknown()
	{
		return false;
	}

	virtual void collectStreams(SortedStreamList& streamList) const
	{
		if (!streamList.exist(recStream))
			streamList.add(recStream);
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	const char* getAlias(bool rdb) const
	{
		if (blrOp == blr_record_version2)
		{
			// ASF: It's on purpose that RDB$ prefix is always used here.
			// Absense of it with DB_KEY seems more a bug than feature.
			return RDB_RECORD_VERSION_NAME;
		}
		return (rdb ? RDB_DB_KEY_NAME : DB_KEY_NAME);
	}

private:
	static ValueExprNode* catenateNodes(thread_db* tdbb, ValueExprNodeStack& stack);

	void raiseError(dsql_ctx* context) const;

public:
	const UCHAR blrOp;
	Firebird::MetaName dsqlQualifier;
	NestConst<RecordSourceNode> dsqlRelation;
	StreamType recStream;
	bool aggregate;
};


class ScalarNode : public TypedNode<ValueExprNode, ExprNode::TYPE_SCALAR>
{
public:
	explicit ScalarNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_SCALAR>(pool),
		  field(NULL),
		  subscripts(NULL)
	{
		addChildNode(field);
		addChildNode(subscripts);
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void print(Firebird::string& /*text*/) const
	{
		fb_assert(false);
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> field;
	NestConst<ValueListNode> subscripts;
};


class StmtExprNode : public TypedNode<ValueExprNode, ExprNode::TYPE_STMT_EXPR>
{
public:
	explicit StmtExprNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_STMT_EXPR>(pool),
		  stmt(NULL),
		  expr(NULL)
	{
		// Do not add the statement. We'll manually handle it in pass1 and pass2.
		addChildNode(expr);
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void print(Firebird::string& /*text*/) const
	{
		fb_assert(false);
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<StmtNode> stmt;
	NestConst<ValueExprNode> expr;
};


class StrCaseNode : public TypedNode<ValueExprNode, ExprNode::TYPE_STR_CASE>
{
public:
	StrCaseNode(MemoryPool& pool, UCHAR aBlrOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	const UCHAR blrOp;
	NestConst<ValueExprNode> arg;
};


class StrLenNode : public TypedNode<ValueExprNode, ExprNode::TYPE_STR_LEN>
{
public:
	StrLenNode(MemoryPool& pool, UCHAR aBlrSubOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	UCHAR blrSubOp;
	NestConst<ValueExprNode> arg;
};


// This node is used for DSQL subqueries and for legacy (BLR-only) functionality.
class SubQueryNode : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBQUERY>
{
public:
	explicit SubQueryNode(MemoryPool& pool, UCHAR aBlrOp, RecordSourceNode* aDsqlRse = NULL,
		ValueExprNode* aValue1 = NULL, ValueExprNode* aValue2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return false;
	}

	virtual bool possiblyUnknown()
	{
		return true;
	}

	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	const UCHAR blrOp;
	bool ownSavepoint;
	NestConst<RecordSourceNode> dsqlRse;
	NestConst<RseNode> rse;
	NestConst<ValueExprNode> value1;
	NestConst<ValueExprNode> value2;
	NestConst<RecordSource> rsb;
};


class SubstringNode : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBSTRING>
{
public:
	explicit SubstringNode(MemoryPool& pool, ValueExprNode* aExpr = NULL,
		ValueExprNode* aStart = NULL, ValueExprNode* aLength = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	static dsc* perform(thread_db* tdbb, impure_value* impure, const dsc* valueDsc,
		const dsc* startDsc, const dsc* lengthDsc);

public:
	NestConst<ValueExprNode> expr;
	NestConst<ValueExprNode> start;
	NestConst<ValueExprNode> length;
};


class SubstringSimilarNode : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBSTRING_SIMILAR>
{
public:
	explicit SubstringSimilarNode(MemoryPool& pool, ValueExprNode* aExpr = NULL,
		ValueExprNode* aPattern = NULL, ValueExprNode* aEscape = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<ValueExprNode> expr;
	NestConst<ValueExprNode> pattern;
	NestConst<ValueExprNode> escape;
};


class SysFuncCallNode : public TypedNode<ValueExprNode, ExprNode::TYPE_SYSFUNC_CALL>
{
public:
	explicit SysFuncCallNode(MemoryPool& pool, const Firebird::MetaName& aName,
		ValueListNode* aArgs = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::MetaName name;
	bool dsqlSpecialSyntax;
	NestConst<ValueListNode> args;
	const SysFunction* function;
};


class TrimNode : public TypedNode<ValueExprNode, ExprNode::TYPE_TRIM>
{
public:
	explicit TrimNode(MemoryPool& pool, UCHAR aWhere,
		ValueExprNode* aValue = NULL, ValueExprNode* aTrimChars = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	UCHAR where;
	NestConst<ValueExprNode> value;
	NestConst<ValueExprNode> trimChars;	// may be NULL
};


class UdfCallNode : public TypedNode<ValueExprNode, ExprNode::TYPE_UDF_CALL>
{
public:
	explicit UdfCallNode(MemoryPool& pool, const Firebird::QualifiedName& aName,
		ValueListNode* aArgs = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::QualifiedName name;
	NestConst<ValueListNode> args;
	NestConst<Function> function;

private:
	dsql_udf* dsqlFunction;
};


class ValueIfNode : public TypedNode<ValueExprNode, ExprNode::TYPE_VALUE_IF>
{
public:
	explicit ValueIfNode(MemoryPool& pool, BoolExprNode* aCondition = NULL,
		ValueExprNode* aTrueValue = NULL, ValueExprNode* aFalseValue = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		const dsc* desc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool possiblyUnknown()
	{
		return true;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	NestConst<BoolExprNode> condition;
	NestConst<ValueExprNode> trueValue;
	NestConst<ValueExprNode> falseValue;
};


class VariableNode : public TypedNode<ValueExprNode, ExprNode::TYPE_VARIABLE>
{
public:
	explicit VariableNode(MemoryPool& pool);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void print(Firebird::string& text) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

public:
	Firebird::MetaName dsqlName;
	NestConst<dsql_var> dsqlVar;
	USHORT varId;
	NestConst<DeclareVariableNode> varDecl;
	NestConst<ItemInfo> varInfo;
};


} // namespace

#endif // DSQL_EXPR_NODES_H
