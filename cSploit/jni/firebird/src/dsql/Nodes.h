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

#include "../jrd/jrd.h"
#include "../dsql/DsqlCompilerScratch.h"
#include "../dsql/Visitors.h"
#include "../common/classes/array.h"
#include "../common/classes/NestConst.h"

namespace Jrd {

class AggregateSort;
class CompilerScratch;
class Cursor;
class ExprNode;
class OptimizerBlk;
class OptimizerRetrieval;
class RecordSource;
class RseNode;
class SlidingWindow;
class TypeClause;
class ValueExprNode;


// Must be less then MAX_SSHORT. Not used for static arrays.
const int MAX_CONJUNCTS	= 32000;

// New: MAX_STREAMS should be a multiple of BITS_PER_LONG (32 and hard to believe it will change)

const StreamType INVALID_STREAM = ~StreamType(0);
const StreamType MAX_STREAMS = 4096;

const StreamType STREAM_MAP_LENGTH = MAX_STREAMS + 2;

// New formula is simply MAX_STREAMS / BITS_PER_LONG
const int OPT_STREAM_BITS = MAX_STREAMS / BITS_PER_LONG; // 128 with 4096 streams

// Number of streams, conjuncts, indices that will be statically allocated
// in various arrays. Larger numbers will have to be allocated dynamically
// CVC: I think we need to have a special, higher value for streams.
const int OPT_STATIC_ITEMS = 64;

typedef Firebird::HalfStaticArray<StreamType, OPT_STATIC_ITEMS> StreamList;
typedef Firebird::SortedArray<StreamType> SortedStreamList;

typedef Firebird::Array<NestConst<ValueExprNode> > NestValueArray;


template <typename T>
class RegisterNode
{
public:
	explicit RegisterNode(UCHAR blr)
	{
		PAR_register(blr, T::parse);
	}
};

template <typename T>
class RegisterBoolNode
{
public:
	explicit RegisterBoolNode(UCHAR blr)
	{
		PAR_register(blr, T::parse);
	}
};


class Node : public Firebird::PermanentStorage
{
public:
	explicit Node(MemoryPool& pool)
		: PermanentStorage(pool),
		  line(0),
		  column(0)
	{
	}

	virtual ~Node()
	{
	}

	// Compile a parsed statement into something more interesting.
	template <typename T>
	static T* doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T>& node)
	{
		if (!node)
			return NULL;

		return node->dsqlPass(dsqlScratch);
	}

	// Compile a parsed statement into something more interesting and assign it to target.
	template <typename T1, typename T2>
	static void doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T1>& target, NestConst<T2>& node)
	{
		if (!node)
			target = NULL;
		else
			target = node->dsqlPass(dsqlScratch);
	}

	// Changes dsqlScratch->isPsql() value, calls doDsqlPass and restore dsqlScratch->isPsql().
	template <typename T>
	static T* doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T>& node, bool psql)
	{
		PsqlChanger changer(dsqlScratch, psql);
		return doDsqlPass(dsqlScratch, node);
	}

	// Changes dsqlScratch->isPsql() value, calls doDsqlPass and restore dsqlScratch->isPsql().
	template <typename T1, typename T2>
	static void doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T1>& target, NestConst<T2>& node, bool psql)
	{
		PsqlChanger changer(dsqlScratch, psql);
		doDsqlPass(dsqlScratch, target, node);
	}

	virtual void print(Firebird::string& text) const = 0;

	virtual Node* dsqlPass(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		return this;
	}

public:
	ULONG line;
	ULONG column;
};


class DdlNode : public Node
{
public:
	explicit DdlNode(MemoryPool& pool)
		: Node(pool)
	{
	}

	static bool deleteSecurityClass(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::MetaName& secClass);

	static void storePrivileges(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::MetaName& name, int type, const char* privileges);

public:
	// Set the scratch's transaction when executing a node. Fact of accessing the scratch during
	// execution is a hack.
	void executeDdl(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
	{
		// dsqlScratch should be NULL with CREATE DATABASE.
		if (dsqlScratch)
			dsqlScratch->setTransaction(transaction);

		execute(tdbb, dsqlScratch, transaction);
	}

	virtual DdlNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_DDL);
		return this;
	}

public:
	enum DdlTriggerWhen { DTW_BEFORE, DTW_AFTER };

	static void executeDdlTrigger(thread_db* tdbb, jrd_tra* transaction,
		DdlTriggerWhen when, int action, const Firebird::MetaName& objectName,
		const Firebird::string& sqlText);

protected:
	typedef Firebird::Pair<Firebird::Left<Firebird::MetaName, bid> > MetaNameBidPair;
	typedef Firebird::GenericMap<MetaNameBidPair> MetaNameBidMap;

	// Return exception code based on combination of create and alter clauses.
	static ISC_STATUS createAlterCode(bool create, bool alter, ISC_STATUS createCode,
		ISC_STATUS alterCode, ISC_STATUS createOrAlterCode)
	{
		if (create && alter)
			return createOrAlterCode;

		if (create)
			return createCode;

		if (alter)
			return alterCode;

		fb_assert(false);
		return 0;
	}

	void executeDdlTrigger(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction,
		DdlTriggerWhen when, int action, const Firebird::MetaName& objectName);
	void storeGlobalField(thread_db* tdbb, jrd_tra* transaction, Firebird::MetaName& name,
		const TypeClause* field,
		const Firebird::string& computedSource = "",
		const BlrDebugWriter::BlrData& computedValue = BlrDebugWriter::BlrData());

public:
	// Prefix DDL exceptions. To be implemented in each command.
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector) = 0;

	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction) = 0;
};


class TransactionNode : public Node
{
public:
	explicit TransactionNode(MemoryPool& pool)
		: Node(pool)
	{
	}

public:
	virtual TransactionNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		Node::dsqlPass(dsqlScratch);
		return this;
	}

	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** transaction) const = 0;
};


class DmlNode : public Node
{
public:
	// DML node kinds
	enum Kind
	{
		KIND_STATEMENT,
		KIND_VALUE,
		KIND_BOOLEAN,
		KIND_REC_SOURCE,
		KIND_LIST
	};

	explicit DmlNode(MemoryPool& pool, Kind aKind)
		: Node(pool),
		  kind(aKind)
	{
	}

	// Merge missing values, computed values, validation expressions, and views into a parsed request.
	template <typename T> static void doPass1(thread_db* tdbb, CompilerScratch* csb, T** node)
	{
		if (!*node)
			return;

		*node = (*node)->pass1(tdbb, csb);
	}

public:
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch) = 0;
	virtual DmlNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual DmlNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual DmlNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;

public:
	const Kind kind;
};


template <typename T, typename T::Type typeConst>
class TypedNode : public T
{
public:
	explicit TypedNode(MemoryPool& pool)
		: T(typeConst, pool)
	{
	}

public:
	const static typename T::Type TYPE = typeConst;
};


// Stores a reference to a specialized ExprNode.
// This class and NodeRefImpl exists for nodes to replace themselves (eg. pass1) in a type-safe way.
class NodeRef
{
public:
	virtual ~NodeRef()
	{
	}

	bool operator !() const
	{
		return !getExpr();
	}

	operator bool() const
	{
		return getExpr() != NULL;
	}

	virtual ExprNode* getExpr() = 0;
	virtual const ExprNode* getExpr() const = 0;

	virtual void remap(FieldRemapper& visitor) = 0;
	virtual void pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	void pass2(thread_db* tdbb, CompilerScratch* csb);

protected:
	virtual void internalPass2(thread_db* tdbb, CompilerScratch* csb) = 0;
};

template <typename T> class NodeRefImpl : public NodeRef
{
public:
	explicit NodeRefImpl(T** aPtr)
		: ptr(aPtr)
	{
		fb_assert(aPtr);
	}

	virtual ExprNode* getExpr();
	virtual const ExprNode* getExpr() const;
	virtual void remap(FieldRemapper& visitor);
	virtual void pass1(thread_db* tdbb, CompilerScratch* csb);

protected:
	virtual inline void internalPass2(thread_db* tdbb, CompilerScratch* csb);

private:
	T** ptr;
};


class ExprNode : public DmlNode
{
public:
	enum Type
	{
		// Value types
		TYPE_AGGREGATE,
		TYPE_ALIAS,
		TYPE_ARITHMETIC,
		TYPE_ARRAY,
		TYPE_BOOL_AS_VALUE,
		TYPE_CAST,
		TYPE_COALESCE,
		TYPE_COLLATE,
		TYPE_CONCATENATE,
		TYPE_CURRENT_DATE,
		TYPE_CURRENT_TIME,
		TYPE_CURRENT_TIMESTAMP,
		TYPE_CURRENT_ROLE,
		TYPE_CURRENT_USER,
		TYPE_DERIVED_EXPR,
		TYPE_DECODE,
		TYPE_DERIVED_FIELD,
		TYPE_DOMAIN_VALIDATION,
		TYPE_EXTRACT,
		TYPE_FIELD,
		TYPE_GEN_ID,
		TYPE_INTERNAL_INFO,
		TYPE_LITERAL,
		TYPE_MAP,
		TYPE_NEGATE,
		TYPE_NULL,
		TYPE_ORDER,
		TYPE_OVER,
		TYPE_PARAMETER,
		TYPE_RECORD_KEY,
		TYPE_SCALAR,
		TYPE_STMT_EXPR,
		TYPE_STR_CASE,
		TYPE_STR_LEN,
		TYPE_SUBQUERY,
		TYPE_SUBSTRING,
		TYPE_SUBSTRING_SIMILAR,
		TYPE_SYSFUNC_CALL,
		TYPE_TRIM,
		TYPE_UDF_CALL,
		TYPE_VALUE_IF,
		TYPE_VARIABLE,

		// Bool types
		TYPE_BINARY_BOOL,
		TYPE_COMPARATIVE_BOOL,
		TYPE_MISSING_BOOL,
		TYPE_NOT_BOOL,
		TYPE_RSE_BOOL,

		// RecordSource types
		TYPE_AGGREGATE_SOURCE,
		TYPE_PROCEDURE,
		TYPE_RELATION,
		TYPE_RSE,
		TYPE_SELECT_EXPR,
		TYPE_UNION,
		TYPE_WINDOW,

		// List types
		TYPE_REC_SOURCE_LIST,
		TYPE_VALUE_LIST
	};

	// Generic flags.
	static const unsigned FLAG_INVARIANT	= 0x01;	// Node is recognized as being invariant.

	// Boolean flags.
	static const unsigned FLAG_DEOPTIMIZE	= 0x02;	// Boolean which requires deoptimization.
	static const unsigned FLAG_RESIDUAL		= 0x04;	// Boolean which must remain residual.
	static const unsigned FLAG_ANSI_NOT		= 0x08;	// ANY/ALL predicate is prefixed with a NOT one.

	// Value flags.
	static const unsigned FLAG_DOUBLE		= 0x10;
	static const unsigned FLAG_DATE			= 0x20;
	static const unsigned FLAG_VALUE		= 0x40;	// Full value area required in impure space.

	explicit ExprNode(Type aType, MemoryPool& pool, Kind aKind)
		: DmlNode(pool, aKind),
		  type(aType),
		  nodFlags(0),
		  impureOffset(0),
		  dsqlCompatDialectVerb(NULL),
		  dsqlChildNodes(pool),
		  jrdChildNodes(pool)
	{
	}

	template <typename T> T* as()
	{
		return this && type == T::TYPE ? static_cast<T*>(this) : NULL;
	}

	template <typename T> const T* as() const
	{
		return this && type == T::TYPE ? static_cast<const T*>(this) : NULL;
	}

	template <typename T> bool is() const
	{
		return this && type == T::TYPE;
	}

	template <typename T, typename LegacyType> static T* as(LegacyType* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename LegacyType> static const T* as(const LegacyType* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename LegacyType> static bool is(const LegacyType* node)
	{
		return node ? node->template is<T>() : false;
	}

	// Allocate and assign impure space for various nodes.
	template <typename T> static void doPass2(thread_db* tdbb, CompilerScratch* csb, T** node)
	{
		if (!*node)
			return;

		*node = (*node)->pass2(tdbb, csb);
	}

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor)
	{
		bool ret = false;

		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			ret |= visitor.visit((*i)->getExpr());

		return ret;
	}

	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor)
	{
		bool ret = false;

		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			ret |= visitor.visit((*i)->getExpr());

		return ret;
	}

	virtual bool dsqlFieldFinder(FieldFinder& visitor)
	{
		bool ret = false;

		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			ret |= visitor.visit((*i)->getExpr());

		return ret;
	}

	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor)
	{
		bool ret = false;

		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			ret |= visitor.visit((*i)->getExpr());

		return ret;
	}

	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor)
	{
		bool ret = false;

		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			ret |= visitor.visit((*i)->getExpr());

		return ret;
	}

	virtual ExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		for (NodeRef* const* i = dsqlChildNodes.begin(); i != dsqlChildNodes.end(); ++i)
			(*i)->remap(visitor);

		return this;
	}

	template <typename T>
	static void doDsqlFieldRemapper(FieldRemapper& visitor, NestConst<T>& node)
	{
		if (node)
			node = node->dsqlFieldRemapper(visitor);
	}

	template <typename T1, typename T2>
	static void doDsqlFieldRemapper(FieldRemapper& visitor, NestConst<T1>& target, NestConst<T2> node)
	{
		target = node ? node->dsqlFieldRemapper(visitor) : NULL;
	}

	// Check if expression could return NULL or expression can turn NULL into a true/false.
	virtual bool possiblyUnknown()
	{
		for (NodeRef** i = jrdChildNodes.begin(); i != jrdChildNodes.end(); ++i)
		{
			if (**i && (*i)->getExpr()->possiblyUnknown())
				return true;
		}

		return false;
	}

	// Verify if this node is allowed in an unmapped boolean.
	virtual bool unmappable(const MapNode* mapNode, StreamType shellStream)
	{
		for (NodeRef** i = jrdChildNodes.begin(); i != jrdChildNodes.end(); ++i)
		{
			if (**i && !(*i)->getExpr()->unmappable(mapNode, shellStream))
				return false;
		}

		return true;
	}

	// Return all streams referenced by the expression.
	virtual void collectStreams(SortedStreamList& streamList) const
	{
		for (const NodeRef* const* i = jrdChildNodes.begin(); i != jrdChildNodes.end(); ++i)
		{
			if (**i)
				(*i)->getExpr()->collectStreams(streamList);
		}
	}

	virtual bool findStream(StreamType stream)
	{
		SortedStreamList streams;
		collectStreams(streams);

		return streams.exist(stream);
	}

	virtual void print(Firebird::string& text) const;
	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;

	virtual ExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		DmlNode::dsqlPass(dsqlScratch);
		return this;
	}

	// Determine if two expression trees are the same.
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;

	// See if node is presently computable.
	// A node is said to be computable, if all the streams involved
	// in that node are csb_active. The csb_active flag defines
	// all the streams available in the current scope of the query.
	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value = NULL);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);
	virtual ExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual ExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;

protected:
	template <typename T1, typename T2>
	void addChildNode(NestConst<T1>& dsqlNode, NestConst<T2>& jrdNode)
	{
		addDsqlChildNode(dsqlNode);
		addChildNode(jrdNode);
	}

	template <typename T>
	void addDsqlChildNode(NestConst<T>& dsqlNode)
	{
		dsqlChildNodes.add(FB_NEW(getPool()) NodeRefImpl<T>(dsqlNode.getAddress()));
	}

	template <typename T>
	void addChildNode(NestConst<T>& jrdNode)
	{
		jrdChildNodes.add(FB_NEW(getPool()) NodeRefImpl<T>(jrdNode.getAddress()));
	}

public:
	const Type type;
	unsigned nodFlags;
	ULONG impureOffset;
	const char* dsqlCompatDialectVerb;
	Firebird::Array<NodeRef*> dsqlChildNodes;
	Firebird::Array<NodeRef*> jrdChildNodes;
};


template <typename T>
inline ExprNode* NodeRefImpl<T>::getExpr()
{
	return *ptr;
}

template <typename T>
inline const ExprNode* NodeRefImpl<T>::getExpr() const
{
	return *ptr;
}

template <typename T>
inline void NodeRefImpl<T>::remap(FieldRemapper& visitor)
{
	if (*ptr)
		*ptr = (*ptr)->dsqlFieldRemapper(visitor);
}

template <typename T>
inline void NodeRefImpl<T>::pass1(thread_db* tdbb, CompilerScratch* csb)
{
	DmlNode::doPass1(tdbb, csb, ptr);
}

template <typename T>
inline void NodeRefImpl<T>::internalPass2(thread_db* tdbb, CompilerScratch* csb)
{
	ExprNode::doPass2(tdbb, csb, ptr);
}


class BoolExprNode : public ExprNode
{
public:
	BoolExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool, KIND_BOOLEAN)
	{
	}

	virtual BoolExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual BoolExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value = NULL);

	virtual BoolExprNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual BoolExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void pass2Boolean1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
	}

	virtual void pass2Boolean2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
	}

	virtual BoolExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual bool execute(thread_db* tdbb, jrd_req* request) const = 0;
};

class ValueExprNode : public ExprNode
{
public:
	ValueExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool, KIND_VALUE),
		  nodScale(0)
	{
		nodDesc.clear();
	}

	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual bool setParameterType(DsqlCompilerScratch* /*dsqlScratch*/,
		const dsc* /*desc*/, bool /*forceVarChar*/)
	{
		return false;
	}

	virtual void setParameterName(dsql_par* parameter) const = 0;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc) = 0;

	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	// Compute descriptor for value expression.
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc) = 0;

	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const = 0;

public:
	SCHAR nodScale;
	dsc nodDesc;
};

class AggNode : public TypedNode<ValueExprNode, ExprNode::TYPE_AGGREGATE>
{
protected:
	struct AggInfo
	{
		AggInfo(const char* aName, UCHAR aBlr, UCHAR aDistinctBlr)
			: name(aName),
			  blr(aBlr),
			  distinctBlr(aDistinctBlr)
		{
		}

		const char* const name;
		const UCHAR blr;
		const UCHAR distinctBlr;
	};

public:
	template <typename T>
	class Register : public AggInfo
	{
	public:
		explicit Register(const char* aName, UCHAR blr, UCHAR blrDistinct)
			: AggInfo(aName, blr, blrDistinct),
			  registerNode1(blr),
			  registerNode2(blrDistinct)
		{
		}

		explicit Register(const char* aName, UCHAR blr)
			: AggInfo(aName, blr, blr),
			  registerNode1(blr),
			  registerNode2(blr)
		{
		}

	private:
		RegisterNode<T> registerNode1, registerNode2;
	};

	explicit AggNode(MemoryPool& pool, const AggInfo& aAggInfo, bool aDistinct, bool aDialect1,
		ValueExprNode* aArg = NULL);

	virtual void print(Firebird::string& text) const;

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool dsqlMatch(const ExprNode* other, bool ignoreMapCast) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual AggNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ValueExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual bool possiblyUnknown()
	{
		return true;
	}

	virtual void collectStreams(SortedStreamList& /*streamList*/) const
	{
		// ASF: Although in v2.5 the visitor happens normally for the node childs, nod_count has
		// been set to 0 in CMP_pass2, so that doesn't happens.
		return;
	}

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return false;
	}

	virtual void checkOrderedWindowCapable() const
	{
		if (distinct)
		{
			Firebird::status_exception::raise(
				Firebird::Arg::Gds(isc_wish_list) <<
				Firebird::Arg::Gds(isc_random) <<
					"DISTINCT is not supported in ordered windows");
		}
	}

	virtual bool shouldCallWinPass() const
	{
		return false;
	}

	virtual dsc* winPass(thread_db* /*tdbb*/, jrd_req* /*request*/, SlidingWindow* /*window*/) const
	{
		return NULL;
	}

	// Used to allocate aggregate impure inside the impure area of recursive CTEs.
	virtual void aggPostRse(thread_db* tdbb, CompilerScratch* csb);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const = 0;	// pure, but defined
	virtual void aggFinish(thread_db* tdbb, jrd_req* request) const;
	virtual bool aggPass(thread_db* tdbb, jrd_req* request) const;
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const = 0;
	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const = 0;

	virtual AggNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/ = 0;

public:
	const AggInfo& aggInfo;
	bool distinct;
	bool dialect1;
	NestConst<ValueExprNode> arg;
	const AggregateSort* asb;
	bool indexed;
};


// Base class for window functions.
class WinFuncNode : public AggNode
{
private:
	// Base factory to create instance of subclasses.
	class Factory : public AggInfo
	{
	public:
		explicit Factory(const char* aName)
			: AggInfo(aName, 0, 0)
		{
		}

		virtual WinFuncNode* newInstance(MemoryPool& pool) const = 0;

	public:
		const Factory* next;
	};

public:
	// Concrete implementation of the factory.
	template <typename T>
	class Register : public Factory
	{
	public:
		explicit Register(const char* aName)
			: Factory(aName)
		{
			next = factories;
			factories = this;
		}

		WinFuncNode* newInstance(MemoryPool& pool) const
		{
			return new T(pool);
		}
	};

	explicit WinFuncNode(MemoryPool& pool, const AggInfo& aAggInfo, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

protected:
	virtual void parseArgs(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, unsigned count)
	{
		fb_assert(count == 0);
	}

private:
	static Factory* factories;
};


class RecordSourceNode : public ExprNode
{
public:
	static const unsigned DFLAG_SINGLETON				= 0x01;
	static const unsigned DFLAG_VALUE					= 0x02;
	static const unsigned DFLAG_RECURSIVE				= 0x04;	// recursive member of recursive CTE
	static const unsigned DFLAG_DERIVED					= 0x08;
	static const unsigned DFLAG_DT_IGNORE_COLUMN_CHECK	= 0x10;
	static const unsigned DFLAG_DT_CTE_USED				= 0x20;

	RecordSourceNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool, KIND_REC_SOURCE),
		  dsqlFlags(0),
		  dsqlContext(NULL),
		  stream(INVALID_STREAM)
	{
	}

	virtual StreamType getStream() const
	{
		return stream;
	}

	void setStream(StreamType value)
	{
		stream = value;
	}

	virtual RecordSourceNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual RecordSourceNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual RecordSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const = 0;
	virtual RecordSourceNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack) = 0;
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual bool containsStream(StreamType checkStream) const = 0;
	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual bool possiblyUnknown()
	{
		return true;
	}

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return false;
	}

	virtual void collectStreams(SortedStreamList& streamList) const
	{
		if (!streamList.exist(getStream()))
			streamList.add(getStream());
	}

	virtual bool sameAs(const ExprNode* /*other*/, bool /*ignoreStreams*/) const
	{
		return false;
	}

	// Identify all of the streams for which a dbkey may need to be carried through a sort.
	virtual void computeDbKeyStreams(StreamList& streamList) const = 0;

	// Identify the streams that make up an RseNode.
	virtual void computeRseStreams(StreamList& streamList) const
	{
		streamList.add(getStream());
	}

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream) = 0;

public:
	unsigned dsqlFlags;
	dsql_ctx* dsqlContext;

protected:
	StreamType stream;
};


class ListExprNode : public ExprNode
{
public:
	ListExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool, KIND_LIST)
	{
	}

	virtual void print(Firebird::string& /*text*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}
};

// Container for a list of value expressions.
class ValueListNode : public TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>
{
public:
	ValueListNode(MemoryPool& pool, unsigned count)
		: TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>(pool),
		  items(pool)
	{
		items.resize(count);

		for (unsigned i = 0; i < count; ++i)
		{
			items[i] = NULL;
			addChildNode(items[i], items[i]);
		}
	}

	ValueListNode(MemoryPool& pool, ValueExprNode* arg1)
		: TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>(pool),
		  items(pool)
	{
		items.resize(1);
		addDsqlChildNode((items[0] = arg1));
	}

	ValueListNode* add(ValueExprNode* argn)
	{
		items.add(argn);
		resetChildNodes();
		return this;
	}

	ValueListNode* addFront(ValueExprNode* argn)
	{
		items.insert(0, argn);
		resetChildNodes();
		return this;
	}

	void clear()
	{
		items.clear();
		resetChildNodes();
	}

	virtual ValueListNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ValueListNode* node = FB_NEW(getPool()) ValueListNode(getPool(), items.getCount());

		NestConst<ValueExprNode>* dst = node->items.begin();

		for (NestConst<ValueExprNode>* src = items.begin(); src != items.end(); ++src, ++dst)
			*dst = doDsqlPass(dsqlScratch, *src);

		return node;
	}

	virtual ValueListNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual ValueListNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual ValueListNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	virtual ValueListNode* copy(thread_db* tdbb, NodeCopier& copier) const
	{
		ValueListNode* node = FB_NEW(*tdbb->getDefaultPool()) ValueListNode(*tdbb->getDefaultPool(),
			items.getCount());

		NestConst<ValueExprNode>* j = node->items.begin();

		for (const NestConst<ValueExprNode>* i = items.begin(); i != items.end(); ++i, ++j)
			*j = copier.copy(tdbb, *i);

		return node;
	}

private:
	void resetChildNodes()
	{
		dsqlChildNodes.clear();
		jrdChildNodes.clear();

		for (size_t i = 0; i < items.getCount(); ++i)
			addChildNode(items[i], items[i]);
	}

public:
	NestValueArray items;
};

// Container for a list of record source expressions.
class RecSourceListNode : public TypedNode<ListExprNode, ExprNode::TYPE_REC_SOURCE_LIST>
{
public:
	RecSourceListNode(MemoryPool& pool, unsigned count);
	RecSourceListNode(MemoryPool& pool, RecordSourceNode* arg1);

	RecSourceListNode* add(RecordSourceNode* argn)
	{
		items.add(argn);
		resetChildNodes();
		return this;
	}

	virtual RecSourceListNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual RecSourceListNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual RecSourceListNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual RecSourceListNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	virtual RecSourceListNode* copy(thread_db* tdbb, NodeCopier& copier) const
	{
		fb_assert(false);
		return NULL;
	}

private:
	void resetChildNodes()
	{
		dsqlChildNodes.clear();

		for (size_t i = 0; i < items.getCount(); ++i)
			addDsqlChildNode(items[i]);
	}

public:
	Firebird::Array<NestConst<RecordSourceNode> > items;
};


class StmtNode : public DmlNode
{
public:
	enum Type
	{
		TYPE_ASSIGNMENT,
		TYPE_BLOCK,
		TYPE_COMPOUND_STMT,
		TYPE_CONTINUE_LEAVE,
		TYPE_CURSOR_STMT,
		TYPE_DECLARE_CURSOR,
		TYPE_DECLARE_SUBFUNC,
		TYPE_DECLARE_SUBPROC,
		TYPE_DECLARE_VARIABLE,
		TYPE_ERASE,
		TYPE_ERROR_HANDLER,
		TYPE_EXCEPTION,
		TYPE_EXEC_BLOCK,
		TYPE_EXEC_PROCEDURE,
		TYPE_EXEC_STATEMENT,
		TYPE_EXIT,
		TYPE_IF,
		TYPE_IN_AUTO_TRANS,
		TYPE_INIT_VARIABLE,
		TYPE_FOR,
		TYPE_HANDLER,
		TYPE_LABEL,
		TYPE_LINE_COLUMN,
		TYPE_LOOP,
		TYPE_MERGE,
		TYPE_MERGE_SEND,
		TYPE_MESSAGE,
		TYPE_MODIFY,
		TYPE_POST_EVENT,
		TYPE_RECEIVE,
		TYPE_RETURN,
		TYPE_SAVEPOINT,
		TYPE_SAVEPOINT_ENCLOSE,
		TYPE_SELECT,
		TYPE_SET_GENERATOR,
		TYPE_STALL,
		TYPE_STORE,
		TYPE_SUSPEND,
		TYPE_UPDATE_OR_INSERT,
		TYPE_USER_SAVEPOINT,

		TYPE_EXT_INIT_PARAMETER,
		TYPE_EXT_TRIGGER
	};

	enum WhichTrigger
	{
		ALL_TRIGS = 0,
		PRE_TRIG = 1,
		POST_TRIG = 2
	};

	struct ExeState
	{
		explicit ExeState(thread_db* tdbb)
			: oldPool(tdbb->getDefaultPool()),
			  oldRequest(tdbb->getRequest()),
			  oldTransaction(tdbb->getTransaction()),
			  errorPending(false),
			  catchDisabled(false),
			  whichEraseTrig(ALL_TRIGS),
			  whichStoTrig(ALL_TRIGS),
			  whichModTrig(ALL_TRIGS),
			  topNode(NULL),
			  prevNode(NULL),
			  exit(false)
		{
		}

		MemoryPool* oldPool;		// Save the old pool to restore on exit.
		jrd_req* oldRequest;		// Save the old request to restore on exit.
		jrd_tra* oldTransaction;	// Save the old transcation to restore on exit.
		bool errorPending;			// Is there an error pending to be handled?
		bool catchDisabled;			// Catch errors so we can unwind cleanly.
		WhichTrigger whichEraseTrig;
		WhichTrigger whichStoTrig;
		WhichTrigger whichModTrig;
		const StmtNode* topNode;
		const StmtNode* prevNode;
		bool exit;					// Exit the looper when true.
	};

public:
	explicit StmtNode(Type aType, MemoryPool& pool)
		: DmlNode(pool, KIND_STATEMENT),
		  type(aType),
		  parentStmt(NULL),
		  impureOffset(0),
		  hasLineColumn(false)
	{
	}

	template <typename T> T* as()
	{
		return type == T::TYPE ? static_cast<T*>(this) : NULL;
	}

	template <typename T> const T* as() const
	{
		return type == T::TYPE ? static_cast<const T*>(this) : NULL;
	}

	template <typename T> bool is() const
	{
		return type == T::TYPE;
	}

	template <typename T, typename T2> static T* as(T2* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename T2> static const T* as(const T2* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename T2> static bool is(const T2* node)
	{
		return node && node->template is<T>();
	}

	// Allocate and assign impure space for various nodes.
	template <typename T> static void doPass2(thread_db* tdbb, CompilerScratch* csb, T** node,
		StmtNode* parentStmt)
	{
		if (!*node)
			return;

		if (parentStmt)
			(*node)->parentStmt = parentStmt;

		*node = (*node)->pass2(tdbb, csb);
	}

	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		DmlNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual StmtNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual StmtNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;

	virtual StmtNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		Firebird::status_exception::raise(
			Firebird::Arg::Gds(isc_cannot_copy_stmt) <<
			Firebird::Arg::Num(int(type)));

		return NULL;
	}

	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const = 0;

public:
	const Type type;
	NestConst<StmtNode> parentStmt;
	ULONG impureOffset;	// Inpure offset from request block.
	bool hasLineColumn;
};


// Used to represent nodes that don't have a specific BLR verb, i.e.,
// do not use RegisterNode.
class DsqlOnlyStmtNode : public StmtNode
{
public:
	explicit DsqlOnlyStmtNode(Type aType, MemoryPool& pool)
		: StmtNode(aType, pool)
	{
	}

public:
	virtual DsqlOnlyStmtNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return this;
	}

	virtual DsqlOnlyStmtNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return this;
	}

	virtual DsqlOnlyStmtNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	const StmtNode* execute(thread_db* /*tdbb*/, jrd_req* /*request*/, ExeState* /*exeState*/) const
	{
		fb_assert(false);
		return NULL;
	}
};


// Add savepoint pair of nodes to statement having error handlers.
class SavepointEncloseNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_SAVEPOINT_ENCLOSE>
{
public:
	explicit SavepointEncloseNode(MemoryPool& pool, StmtNode* aStmt)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_SAVEPOINT_ENCLOSE>(pool),
		  stmt(aStmt)
	{
	}

public:
	static StmtNode* make(MemoryPool& pool, DsqlCompilerScratch* dsqlScratch, StmtNode* node);

public:
	virtual void print(Firebird::string& text) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

private:
	NestConst<StmtNode> stmt;
};


struct ScaledNumber
{
	FB_UINT64 number;
	SCHAR scale;
	bool hex;
};


class RowsClause : public Firebird::PermanentStorage
{
public:
	explicit RowsClause(MemoryPool& pool)
		: PermanentStorage(pool),
		  length(NULL),
		  skip(NULL)
	{
	}

public:
	NestConst<ValueExprNode> length;
	NestConst<ValueExprNode> skip;
};


class GeneratorItem
{
public:
	GeneratorItem(Firebird::MemoryPool& pool, const Firebird::MetaName& name)
		: id(0), name(pool, name), secName(pool)
	{}

	GeneratorItem& operator=(const GeneratorItem& other)
	{
		id = other.id;
		name = other.name;
		secName = other.secName;
		return *this;
	}

	SLONG id;
	Firebird::MetaName name;
	Firebird::MetaName secName;
};


} // namespace

#endif // DSQL_NODES_H
