/*
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
 * Adriano dos Santos Fernandes
 */

#ifndef JRD_RECORD_SOURCE_NODES_H
#define JRD_RECORD_SOURCE_NODES_H

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/NestConst.h"
#include "../common/classes/QualifiedName.h"
#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../dsql/Visitors.h"
#include "../dsql/pass1_proto.h"

namespace Jrd {

class IndexRetrieval;
class OptimizerRetrieval;
class ProcedureScan;
class BoolExprNode;
class MessageNode;
class RecSourceListNode;
class RelationSourceNode;
class RseNode;
class SelectExprNode;
class ValueListNode;


class SortNode : public Firebird::PermanentStorage
{
public:
	explicit SortNode(MemoryPool& pool)
		: PermanentStorage(pool),
		  unique(false),
		  expressions(pool),
		  descending(pool),
		  nullOrder(pool)
	{
	}

	SortNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	SortNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	SortNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	bool computable(CompilerScratch* csb, StreamType stream, bool allowOnlyCurrentStream);
	void findDependentFromStreams(const OptimizerRetrieval* optRet, SortedStreamList* streamList);

	bool unique;						// sorts using unique key - for distinct and group by
	NestValueArray expressions;			// sort expressions
	Firebird::Array<bool> descending;	// true = descending / false = ascending
	Firebird::Array<int> nullOrder;		// rse_nulls_*
};

class MapNode : public Firebird::PermanentStorage
{
public:
	explicit MapNode(MemoryPool& pool)
		: PermanentStorage(pool),
		  sourceList(pool),
		  targetList(pool)
	{
	}

	MapNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	MapNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	MapNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	void aggPostRse(thread_db* tdbb, CompilerScratch* csb);

	NestValueArray sourceList;
	NestValueArray targetList;
};

class PlanNode : public Firebird::PermanentStorage
{
public:
	enum Type
	{
		TYPE_JOIN,
		TYPE_RETRIEVE
	};

	struct AccessItem
	{
		explicit AccessItem(MemoryPool& pool)
			: relationId(0),
			  indexId(0),
			  indexName(pool)
		{
		}

		explicit AccessItem(MemoryPool& pool, const AccessItem& o)
			: relationId(o.relationId),
			  indexId(o.indexId),
			  indexName(pool, o.indexName)
		{
		}

		SLONG relationId;
		SLONG indexId;
		Firebird::MetaName indexName;
	};

	struct AccessType
	{
		enum Type
		{
			TYPE_SEQUENTIAL,
			TYPE_NAVIGATIONAL,
			TYPE_INDICES
		};

		AccessType(MemoryPool& pool, Type aType)
			: type(aType),
			  items(pool)
		{
		}

		Type const type;
		Firebird::ObjectsArray<AccessItem> items;
	};

public:
	PlanNode(MemoryPool& pool, Type aType)
		: PermanentStorage(pool),
		  type(aType),
		  accessType(NULL),
		  relationNode(NULL),
		  subNodes(pool),
		  dsqlRecordSourceNode(NULL),
		  dsqlNames(NULL)
	{
	}

	PlanNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

private:
	dsql_ctx* dsqlPassAliasList(DsqlCompilerScratch* dsqlScratch);
	static dsql_ctx* dsqlPassAlias(DsqlCompilerScratch* dsqlScratch, DsqlContextStack& stack,
		const Firebird::MetaName& alias);

public:
	Type const type;
	AccessType* accessType;
	RelationSourceNode* relationNode;
	Firebird::Array<NestConst<PlanNode> > subNodes;
	RecordSourceNode* dsqlRecordSourceNode;
	Firebird::ObjectsArray<Firebird::MetaName>* dsqlNames;
};

class InversionNode
{
public:
	enum Type
	{
		TYPE_AND,
		TYPE_OR,
		TYPE_IN,
		TYPE_DBKEY,
		TYPE_INDEX
	};

	InversionNode(Type aType, InversionNode* aNode1, InversionNode* aNode2)
		: type(aType),
		  impure(0),
		  retrieval(NULL),
		  node1(aNode1),
		  node2(aNode2),
		  value(NULL),
		  id(0)
	{
	}

	explicit InversionNode(IndexRetrieval* aRetrieval)
		: type(TYPE_INDEX),
		  impure(0),
		  retrieval(aRetrieval),
		  node1(NULL),
		  node2(NULL),
		  value(NULL),
		  id(0)
	{
	}

	InversionNode(ValueExprNode* aValue, USHORT aId)
		: type(TYPE_DBKEY),
		  impure(0),
		  retrieval(NULL),
		  node1(NULL),
		  node2(NULL),
		  value(aValue),
		  id(aId)
	{
	}

	Type type;
	ULONG impure;
	NestConst<IndexRetrieval> retrieval;
	NestConst<InversionNode> node1;
	NestConst<InversionNode> node2;
	NestConst<ValueExprNode> value;
	USHORT id;
};

class WithClause : public Firebird::Array<SelectExprNode*>
{
public:
	explicit WithClause(Firebird::MemoryPool& pool)
		: Firebird::Array<SelectExprNode*>(pool),
		  recursive(false)
	{
	}

public:
	bool recursive;
};


class RelationSourceNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_RELATION>
{
public:
	explicit RelationSourceNode(MemoryPool& pool, const Firebird::MetaName& aDsqlName = NULL)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_RELATION>(pool),
		  dsqlName(pool, aDsqlName),
		  alias(pool),
		  relation(NULL),
		  context(0),
		  view(NULL)
	{
	}

	static RelationSourceNode* parse(thread_db* tdbb, CompilerScratch* csb, const SSHORT blrOp,
		bool parseContext);

	virtual RecordSourceNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlSubSelectFinder(SubSelectFinder& /*visitor*/)
	{
		return false;
	}

	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual RelationSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const;

	virtual RecordSourceNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);

	virtual RecordSourceNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);

	virtual bool containsStream(StreamType checkStream) const
	{
		return checkStream == stream;
	}

	virtual void computeDbKeyStreams(StreamList& streamList) const
	{
		streamList.add(getStream());
	}

	virtual bool computable(CompilerScratch* /*csb*/, StreamType /*stream*/,
		bool /*allowOnlyCurrentStream*/, ValueExprNode* /*value*/)
	{
		return true;
	}

	virtual void findDependentFromStreams(const OptimizerRetrieval* /*optRet*/,
		SortedStreamList* /*streamList*/)
	{
	}

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

public:
	Firebird::MetaName dsqlName;
	Firebird::string alias;	// SQL alias for the relation
	jrd_rel* relation;
	SSHORT context;			// user-specified context number for the relation reference

private:
	jrd_rel* view;		// parent view for posting access
};

class ProcedureSourceNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_PROCEDURE>
{
public:
	explicit ProcedureSourceNode(MemoryPool& pool,
			const Firebird::QualifiedName& aDsqlName = Firebird::QualifiedName())
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_PROCEDURE>(pool),
		  dsqlName(pool, aDsqlName),
		  alias(pool),
		  sourceList(NULL),
		  targetList(NULL),
		  in_msg(NULL),
		  procedure(NULL),
		  view(NULL),
		  context(0)
	{
	}

	static ProcedureSourceNode* parse(thread_db* tdbb, CompilerScratch* csb, const SSHORT blrOp);

	virtual RecordSourceNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual RecordSourceNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual ProcedureSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void ignoreDbKey(thread_db* /*tdbb*/, CompilerScratch* /*csb*/) const
	{
	}

	virtual RecordSourceNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);

	virtual bool containsStream(StreamType checkStream) const
	{
		return checkStream == stream;
	}

	virtual void computeDbKeyStreams(StreamList& /*streamList*/) const
	{
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);
	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

private:
	ProcedureScan* generate(thread_db* tdbb, OptimizerBlk* opt);

public:
	Firebird::QualifiedName dsqlName;
	Firebird::string alias;
	NestConst<ValueListNode> sourceList;
	NestConst<ValueListNode> targetList;

private:
	NestConst<MessageNode> in_msg;
	jrd_prc* procedure;
	jrd_rel* view;
	SSHORT context;
};

class AggregateSourceNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_AGGREGATE_SOURCE>
{
public:
	explicit AggregateSourceNode(MemoryPool& pool)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_AGGREGATE_SOURCE>(pool),
		  dsqlGroup(NULL),
		  dsqlRse(NULL),
		  dsqlWindow(false),
		  group(NULL),
		  map(NULL),
		  rse(NULL)
	{
	}

	static AggregateSourceNode* parse(thread_db* tdbb, CompilerScratch* csb);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual RecordSourceNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual AggregateSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const;
	virtual RecordSourceNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);
	virtual bool containsStream(StreamType checkStream) const;

	virtual void computeDbKeyStreams(StreamList& /*streamList*/) const
	{
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);
	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

private:
	void genMap(DsqlCompilerScratch* dsqlScratch, dsql_map* map);

	RecordSource* generate(thread_db* tdbb, OptimizerBlk* opt, BoolExprNodeStack* parentStack,
		StreamType shellStream);

public:
	NestConst<ValueListNode> dsqlGroup;
	NestConst<RseNode> dsqlRse;
	bool dsqlWindow;
	NestConst<SortNode> group;
	NestConst<MapNode> map;

private:
	NestConst<RseNode> rse;
};

class UnionSourceNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_UNION>
{
public:
	explicit UnionSourceNode(MemoryPool& pool)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_UNION>(pool),
		  dsqlAll(false),
		  recursive(false),
		  dsqlClauses(NULL),
		  dsqlParentRse(NULL),
		  clauses(pool),
		  maps(pool),
		  mapStream(0)
	{
	}

	static UnionSourceNode* parse(thread_db* tdbb, CompilerScratch* csb, const SSHORT blrOp);

	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual UnionSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const;

	virtual RecordSourceNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);
	virtual bool containsStream(StreamType checkStream) const;
	virtual void computeDbKeyStreams(StreamList& streamList) const;
	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);
	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

private:
	RecordSource* generate(thread_db* tdbb, OptimizerBlk* opt, const StreamType* streams, size_t nstreams,
		BoolExprNodeStack* parentStack, StreamType shellStream);

public:
	bool dsqlAll;		// UNION ALL
	bool recursive;		// union node is a recursive union
	RecSourceListNode* dsqlClauses;
	RseNode* dsqlParentRse;

private:
	Firebird::Array<NestConst<RseNode> > clauses;	// RseNode's for union
	Firebird::Array<NestConst<MapNode> > maps;		// RseNode's maps
	StreamType mapStream;	// stream for next level record of recursive union
};

class WindowSourceNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_WINDOW>
{
public:
	struct Partition
	{
		explicit Partition(MemoryPool&)
			: stream(INVALID_STREAM)
		{
		}

		StreamType stream;
		NestConst<SortNode> group;
		NestConst<SortNode> regroup;
		NestConst<SortNode> order;
		NestConst<MapNode> map;
	};

	explicit WindowSourceNode(MemoryPool& pool)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_WINDOW>(pool),
		  rse(NULL),
		  partitions(pool)
	{
	}

	static WindowSourceNode* parse(thread_db* tdbb, CompilerScratch* csb);

private:
	void parsePartitionBy(thread_db* tdbb, CompilerScratch* csb);

public:
	virtual StreamType getStream() const
	{
		fb_assert(false);
		return 0;
	}

	virtual WindowSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const;
	virtual RecordSourceNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);
	virtual bool containsStream(StreamType checkStream) const;
	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual void computeDbKeyStreams(StreamList& /*streamList*/) const
	{
	}

	virtual void computeRseStreams(StreamList& streamList) const;

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);
	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);
	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

private:
	NestConst<RseNode> rse;
	Firebird::ObjectsArray<Partition> partitions;
};

class RseNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_RSE>
{
public:
	static const unsigned FLAG_VARIANT			= 0x01;	// variant (not invariant?)
	static const unsigned FLAG_SINGULAR			= 0x02;	// singleton select
	static const unsigned FLAG_WRITELOCK		= 0x04;	// locked for write
	static const unsigned FLAG_SCROLLABLE		= 0x08;	// scrollable cursor
	static const unsigned FLAG_DSQL_COMPARATIVE	= 0x10;	// transformed from DSQL ComparativeBoolNode
	static const unsigned FLAG_OPT_FIRST_ROWS	= 0x20;	// optimize retrieval for first rows

	explicit RseNode(MemoryPool& pool)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_RSE>(pool),
		  dsqlFirst(NULL),
		  dsqlSkip(NULL),
		  dsqlDistinct(NULL),
		  dsqlSelectList(NULL),
		  dsqlFrom(NULL),
		  dsqlWhere(NULL),
		  dsqlJoinUsing(NULL),
		  dsqlGroup(NULL),
		  dsqlHaving(NULL),
		  dsqlOrder(NULL),
		  dsqlStreams(NULL),
		  dsqlExplicitJoin(false),
		  rse_jointype(0),
		  rse_invariants(NULL),
		  rse_relations(pool),
		  flags(0)
	{
		addDsqlChildNode(dsqlStreams);
		addDsqlChildNode(dsqlWhere);
		addDsqlChildNode(dsqlJoinUsing);
		addDsqlChildNode(dsqlOrder);
		addDsqlChildNode(dsqlDistinct);
		addDsqlChildNode(dsqlSelectList);
		addDsqlChildNode(dsqlFirst);
		addDsqlChildNode(dsqlSkip);
	}

	RseNode* clone()
	{
		RseNode* obj = FB_NEW(getPool()) RseNode(getPool());

		obj->dsqlFirst = dsqlFirst;
		obj->dsqlSkip = dsqlSkip;
		obj->dsqlDistinct = dsqlDistinct;
		obj->dsqlSelectList = dsqlSelectList;
		obj->dsqlFrom = dsqlFrom;
		obj->dsqlWhere = dsqlWhere;
		obj->dsqlJoinUsing = dsqlJoinUsing;
		obj->dsqlGroup = dsqlGroup;
		obj->dsqlHaving = dsqlHaving;
		obj->dsqlOrder = dsqlOrder;
		obj->dsqlStreams = dsqlStreams;
		obj->dsqlContext = dsqlContext;
		obj->dsqlExplicitJoin = dsqlExplicitJoin;

		obj->rse_jointype = rse_jointype;
		obj->rse_first = rse_first;
		obj->rse_skip = rse_skip;
		obj->rse_boolean = rse_boolean;
		obj->rse_sorted = rse_sorted;
		obj->rse_projection = rse_projection;
		obj->rse_aggregate = rse_aggregate;
		obj->rse_plan = rse_plan;
		obj->rse_invariants = rse_invariants;
		obj->flags = flags;
		obj->rse_relations = rse_relations;

		return obj;
	}

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual RseNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual RseNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual RseNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const;
	virtual RseNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack);

	virtual RseNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		return this;
	}

	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb);
	virtual bool containsStream(StreamType checkStream) const;
	virtual void computeDbKeyStreams(StreamList& streamList) const;
	virtual void computeRseStreams(StreamList& streamList) const;
	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);
	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);

	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream);

private:
	void planCheck(const CompilerScratch* csb) const;
	static void planSet(CompilerScratch* csb, PlanNode* plan);

public:
	NestConst<ValueExprNode> dsqlFirst;
	NestConst<ValueExprNode> dsqlSkip;
	NestConst<ValueListNode> dsqlDistinct;
	NestConst<ValueListNode> dsqlSelectList;
	NestConst<RecSourceListNode> dsqlFrom;
	NestConst<BoolExprNode> dsqlWhere;
	NestConst<ValueListNode> dsqlJoinUsing;
	NestConst<ValueListNode> dsqlGroup;
	NestConst<BoolExprNode> dsqlHaving;
	NestConst<ValueListNode> dsqlOrder;
	NestConst<RecSourceListNode> dsqlStreams;
	bool dsqlExplicitJoin;
	USHORT rse_jointype;		// inner, left, full
	NestConst<ValueExprNode> rse_first;
	NestConst<ValueExprNode> rse_skip;
	NestConst<BoolExprNode> rse_boolean;
	NestConst<SortNode> rse_sorted;
	NestConst<SortNode> rse_projection;
	NestConst<SortNode> rse_aggregate;	// singleton aggregate for optimizing to index
	NestConst<PlanNode> rse_plan;		// user-specified access plan
	NestConst<VarInvariantArray> rse_invariants; // Invariant nodes bound to top-level RSE
	Firebird::Array<NestConst<RecordSourceNode> > rse_relations;
	unsigned flags;
};

class SelectExprNode : public TypedNode<RecordSourceNode, RecordSourceNode::TYPE_SELECT_EXPR>
{
public:
	explicit SelectExprNode(MemoryPool& pool)
		: TypedNode<RecordSourceNode, RecordSourceNode::TYPE_SELECT_EXPR>(pool),
		  querySpec(NULL),
		  orderClause(NULL),
		  rowsClause(NULL),
		  withClause(NULL),
		  alias(pool),
		  columns(NULL)
	{
	}

	virtual RseNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual RseNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual void ignoreDbKey(thread_db* /*tdbb*/, CompilerScratch* /*csb*/) const
	{
		fb_assert(false);
	}

	virtual RseNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual void pass1Source(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, RseNode* /*rse*/,
		BoolExprNode** /*boolean*/, RecordSourceNodeStack& /*stack*/)
	{
		fb_assert(false);
	}

	virtual RseNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual void pass2Rse(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
	}

	virtual bool containsStream(StreamType /*checkStream*/) const
	{
		fb_assert(false);
		return false;
	}

	virtual void computeDbKeyStreams(StreamList& /*streamList*/) const
	{
		fb_assert(false);
	}

	virtual RecordSource* compile(thread_db* /*tdbb*/, OptimizerBlk* /*opt*/, bool /*innerSubStream*/)
	{
		fb_assert(false);
		return NULL;
	}

public:
	NestConst<RecordSourceNode> querySpec;
	NestConst<ValueListNode> orderClause;
	NestConst<RowsClause> rowsClause;
	NestConst<WithClause> withClause;
	Firebird::string alias;
	Firebird::ObjectsArray<Firebird::MetaName>* columns;
};


} // namespace Jrd

#endif	// JRD_RECORD_SOURCE_NODES_H
