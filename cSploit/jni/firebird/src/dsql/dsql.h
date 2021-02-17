/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		dsql.h
 *	DESCRIPTION:	General Definitions for V4 DSQL module
 *
 *
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
 *
 * 2001.11.26 Claudio Valderrama: include udf_arguments and udf_flags
 *   in the udf struct, so we can load the arguments and check for
 *   collisions between dropping and redefining the udf concurrently.
 *   This closes SF Bug# 409769.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 * Adriano dos Santos Fernandes
 */

#ifndef DSQL_DSQL_H
#define DSQL_DSQL_H

#include "../jrd/RuntimeStatistics.h"
#include "../jrd/ntrace.h"
#include "../jrd/val.h"  // Get rid of duplicated FUN_T enum.
#include "../jrd/Attachment.h"
#include "../dsql/BlrDebugWriter.h"
#include "../dsql/ddl_proto.h"
#include "../common/classes/array.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/stack.h"
#include "../common/classes/auto.h"
#include "../common/classes/NestConst.h"

#ifdef DEV_BUILD
// This macro enables DSQL tracing code
#define DSQL_DEBUG
#endif

#ifdef DSQL_DEBUG
DEFINE_TRACE_ROUTINE(dsql_trace);
#endif

// generic block used as header to all allocated structures
#include "../include/fb_blk.h"

#include "../dsql/sym.h"

#include "../jrd/EngineInterface.h"

// Context aliases used in triggers
const char* const OLD_CONTEXT_NAME = "OLD";
const char* const NEW_CONTEXT_NAME = "NEW";

const int OLD_CONTEXT_VALUE = 0;
const int NEW_CONTEXT_VALUE = 1;

namespace Jrd
{
	class Attachment;
	class Database;
	class DsqlCompilerScratch;
	class DdlNode;
	class RseNode;
	class StmtNode;
	class TransactionNode;
	class ValueExprNode;
	class ValueListNode;
	class jrd_tra;
	class jrd_req;
	class blb;
	struct bid;

	class dsql_ctx;
	class dsql_msg;
	class dsql_par;
	class dsql_map;
	class dsql_intlsym;

	typedef Firebird::Stack<dsql_ctx*> DsqlContextStack;
}

namespace Firebird
{
	class MetaName;
}

//======================================================================
// remaining node definitions for local processing
//

/// Include definition of descriptor

#include "../common/dsc.h"

namespace Jrd {

// blocks used to cache metadata

// Database Block
class dsql_dbb : public pool_alloc<dsql_type_dbb>
{
public:
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, class dsql_rel*> > > dbb_relations;			// known relations in database
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::QualifiedName, class dsql_prc*> > > dbb_procedures;	// known procedures in database
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::QualifiedName, class dsql_udf*> > > dbb_functions;	// known functions in database
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, class dsql_intlsym*> > > dbb_charsets;		// known charsets in database
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, class dsql_intlsym*> > > dbb_collations;	// known collations in database
	Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<
		SSHORT, dsql_intlsym*> > > dbb_charsets_by_id;	// charsets sorted by charset_id
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::string, class dsql_req*> > > dbb_cursors;			// known cursors in database

	MemoryPool&		dbb_pool;			// The current pool for the dbb
	Attachment*		dbb_attachment;
	Firebird::MetaName dbb_dfl_charset;
	bool			dbb_no_charset;
	bool			dbb_read_only;
	USHORT			dbb_db_SQL_dialect;
	USHORT			dbb_ods_version;	// major ODS version number
	USHORT			dbb_minor_version;	// minor ODS version number

	explicit dsql_dbb(MemoryPool& p)
		: dbb_relations(p),
		  dbb_procedures(p),
		  dbb_functions(p),
		  dbb_charsets(p),
		  dbb_collations(p),
		  dbb_charsets_by_id(p),
		  dbb_cursors(p),
		  dbb_pool(p),
		  dbb_dfl_charset(p)
	{}

	~dsql_dbb();

	MemoryPool* createPool()
	{
		return dbb_attachment->createPool();
	}

	void deletePool(MemoryPool* pool)
	{
		dbb_attachment->deletePool(pool);
	}
};

//! Relation block
class dsql_rel : public pool_alloc<dsql_type_rel>
{
public:
	explicit dsql_rel(MemoryPool& p)
		: rel_name(p),
		  rel_owner(p)
	{
	}

	class dsql_fld*	rel_fields;		// Field block
	//dsql_rel*	rel_base_relation;	// base relation for an updatable view
	Firebird::MetaName rel_name;	// Name of relation
	Firebird::MetaName rel_owner;	// Owner of relation
	USHORT		rel_id;				// Relation id
	USHORT		rel_dbkey_length;
	USHORT		rel_flags;
};

// rel_flags bits
enum rel_flags_vals {
	REL_new_relation	= 1, // relation exists in sys tables, not committed yet
	REL_dropped			= 2, // relation has been dropped
	REL_view			= 4, // relation is a view
	REL_external		= 8, // relation is an external table
	REL_creating		= 16 // we are creating the bare relation in memory
};

class TypeClause
{
public:
	TypeClause(MemoryPool& pool, const Firebird::MetaName& aCollate)
		: dtype(dtype_unknown),
		  length(0),
		  scale(0),
		  subType(0),
		  segLength(0),
		  precision(0),
		  charLength(0),
		  charSetId(0),
		  collationId(0),
		  textType(0),
		  fullDomain(false),
		  notNull(false),
		  fieldSource(pool),
		  typeOfTable(pool),
		  typeOfName(pool),
		  collate(pool, aCollate),
		  charSet(pool),
		  subTypeName(pool, NULL),
		  flags(0),
		  elementDtype(0),
		  elementLength(0),
		  dimensions(0),
		  ranges(NULL),
		  explicitCollation(false)
	{
	}

	virtual ~TypeClause()
	{
	}

public:
	virtual void print(Firebird::string& text) const
	{
		text.printf("typeOfTable: '%s'  typeOfName: '%s'  notNull: %d  fieldSource: '%s'",
			typeOfTable.c_str(), typeOfName.c_str(), notNull, fieldSource.c_str());
	}

public:
	USHORT dtype;
	FLD_LENGTH length;
	SSHORT scale;
	SSHORT subType;
	USHORT segLength;					// Segment length for blobs
	USHORT precision;					// Precision for exact numeric types
	USHORT charLength;					// Length of field in characters
	SSHORT charSetId;
	SSHORT collationId;
	SSHORT textType;
	bool fullDomain;					// Domain name without TYPE OF prefix
	bool notNull;						// NOT NULL was explicit specified
	Firebird::MetaName fieldSource;
	Firebird::MetaName typeOfTable;		// TYPE OF table name
	Firebird::MetaName typeOfName;		// TYPE OF
	Firebird::MetaName collate;
	Firebird::MetaName charSet;		// empty means not specified
	Firebird::MetaName subTypeName;	// Subtype name for later resolution
	USHORT flags;
	USHORT elementDtype;			// Data type of array element
	USHORT elementLength;			// Length of array element
	SSHORT dimensions;				// Non-zero means array
	ValueListNode* ranges;			// ranges for multi dimension array
	bool explicitCollation;			// COLLATE was explicit specified
};

class dsql_fld : public TypeClause
{
public:
	explicit dsql_fld(MemoryPool& p)
		: TypeClause(p, NULL),
		  fld_next(NULL),
		  fld_relation(NULL),
		  fld_procedure(NULL),
		  fld_id(0),
		  fld_name(p)
	{
	}

public:
	void resolve(DsqlCompilerScratch* dsqlScratch, bool modifying = false)
	{
		DDL_resolve_intl_type(dsqlScratch, this, collate, modifying);
	}

public:
	dsql_fld*	fld_next;				// Next field in relation
	dsql_rel*	fld_relation;			// Parent relation
	dsql_prc*	fld_procedure;			// Parent procedure
	USHORT		fld_id;					// Field in in database
	Firebird::MetaName fld_name;
};

// values used in fld_flags

enum fld_flags_vals {
	FLD_computed	= 1,
	FLD_national	= 2, // field uses NATIONAL character set
	FLD_nullable	= 4,
	FLD_system		= 8
};

//! Stored Procedure block
class dsql_prc : public pool_alloc<dsql_type_prc>
{
public:
	explicit dsql_prc(MemoryPool& p)
		: prc_name(p),
		  prc_owner(p)
	{
	}

	dsql_fld*	prc_inputs;		// Input parameters
	dsql_fld*	prc_outputs;	// Output parameters
	Firebird::QualifiedName prc_name;	// Name of procedure
	Firebird::MetaName prc_owner;	// Owner of procedure
	SSHORT		prc_in_count;
	SSHORT		prc_def_count;	// number of inputs with default values
	SSHORT		prc_out_count;
	USHORT		prc_id;			// Procedure id
	USHORT		prc_flags;
	bool		prc_private;	// Packaged private procedure
};

// prc_flags bits

enum prc_flags_vals {
	PRC_new_procedure	= 1,	// procedure is newly defined, not committed yet
	PRC_dropped			= 2,	// procedure has been dropped
	PRC_subproc			= 4		// Sub procedure
};

//! User defined function block
class dsql_udf : public pool_alloc<dsql_type_udf>
{
public:
	explicit dsql_udf(MemoryPool& p)
		: udf_name(p), udf_arguments(p)
	{
	}

	USHORT		udf_dtype;
	SSHORT		udf_scale;
	SSHORT		udf_sub_type;
	USHORT		udf_length;
	SSHORT		udf_character_set_id;
	//USHORT		udf_character_length;
    USHORT      udf_flags;
	Firebird::QualifiedName udf_name;
	Firebird::Array<dsc> udf_arguments;
	bool		udf_private;	// Packaged private function
};

// udf_flags bits

enum udf_flags_vals {
	UDF_new_udf		= 1,	// udf is newly declared, not committed yet
	UDF_dropped		= 2,	// udf has been dropped
	UDF_subfunc		= 4		// sub function
};

// Variables - input, output & local

//! Variable block
class dsql_var : public Firebird::PermanentStorage
{
public:
	enum Type
	{
		TYPE_INPUT,
		TYPE_OUTPUT,
		TYPE_LOCAL,
		TYPE_HIDDEN
	};

public:
	explicit dsql_var(MemoryPool& p)
		: PermanentStorage(p),
		  field(NULL),
		  type(TYPE_INPUT),
		  msgNumber(0),
		  msgItem(0),
		  number(0)
	{
		desc.clear();
	}

	dsql_fld* field;	// Field on which variable is based
	Type type;			// Input, output, local or hidden variable
	USHORT msgNumber;	// Message number containing variable
	USHORT msgItem;		// Item number in message
	USHORT number;		// Local variable number
	dsc desc;
};


// Symbolic names for international text types
// (either collation or character set name)

//! International symbol
class dsql_intlsym : public pool_alloc<dsql_type_intlsym>
{
public:
	explicit dsql_intlsym(MemoryPool& p)
		: intlsym_name(p)
	{
	}

	Firebird::MetaName intlsym_name;
	USHORT		intlsym_type;		// what type of name
	USHORT		intlsym_flags;
	SSHORT		intlsym_ttype;		// id of implementation
	SSHORT		intlsym_charset_id;
	SSHORT		intlsym_collate_id;
	USHORT		intlsym_bytes_per_char;
};

// values used in intlsym_flags

enum intlsym_flags_vals {
	INTLSYM_dropped	= 1  // intlsym has been dropped
};


// Compiled statement - shared by multiple requests.
class DsqlCompiledStatement : public Firebird::PermanentStorage
{
public:
	enum Type	// statement type
	{
		TYPE_SELECT, TYPE_SELECT_UPD, TYPE_INSERT, TYPE_DELETE, TYPE_UPDATE, TYPE_UPDATE_CURSOR,
		TYPE_DELETE_CURSOR, TYPE_COMMIT, TYPE_ROLLBACK, TYPE_CREATE_DB, TYPE_DDL, TYPE_START_TRANS,
		TYPE_EXEC_PROCEDURE, TYPE_COMMIT_RETAIN, TYPE_ROLLBACK_RETAIN, TYPE_SET_GENERATOR,
		TYPE_SAVEPOINT, TYPE_EXEC_BLOCK, TYPE_SELECT_BLOCK, TYPE_SET_ROLE
	};

	// Statement flags.
	static const unsigned FLAG_ORPHAN		= 0x01;
	static const unsigned FLAG_NO_BATCH		= 0x02;
	//static const unsigned FLAG_BLR_VERSION4	= 0x04;
	//static const unsigned FLAG_BLR_VERSION5	= 0x08;
	static const unsigned FLAG_SELECTABLE	= 0x10;

public:
	explicit DsqlCompiledStatement(MemoryPool& p)
		: PermanentStorage(p),
		  type(TYPE_SELECT),
		  flags(0),
		  blrVersion(5),
		  sendMsg(NULL),
		  receiveMsg(NULL),
		  eof(NULL),
		  dbKey(NULL),
		  recVersion(NULL),
		  parentRecVersion(NULL),
		  parentDbKey(NULL),
		  parentRequest(NULL)
	{
	}

public:
	MemoryPool& getPool() { return PermanentStorage::getPool(); }

	Type getType() const { return type; }
	void setType(Type value) { type = value; }

	ULONG getFlags() const { return flags; }
	void setFlags(ULONG value) { flags = value; }
	void addFlags(ULONG value) { flags |= value; }

	unsigned getBlrVersion() const { return blrVersion; }
	void setBlrVersion(unsigned value) { blrVersion = value; }

	Firebird::RefStrPtr& getSqlText() { return sqlText; }
	const Firebird::RefStrPtr& getSqlText() const { return sqlText; }
	void setSqlText(Firebird::RefString* value) { sqlText = value; }

	dsql_msg* getSendMsg() { return sendMsg; }
	const dsql_msg* getSendMsg() const { return sendMsg; }
	void setSendMsg(dsql_msg* value) { sendMsg = value; }

	dsql_msg* getReceiveMsg() { return receiveMsg; }
	const dsql_msg* getReceiveMsg() const { return receiveMsg; }
	void setReceiveMsg(dsql_msg* value) { receiveMsg = value; }

	dsql_par* getEof() { return eof; }
	const dsql_par* getEof() const { return eof; }
	void setEof(dsql_par* value) { eof = value; }

	dsql_par* getDbKey() { return dbKey; }
	const dsql_par* getDbKey() const { return dbKey; }
	void setDbKey(dsql_par* value) { dbKey = value; }

	dsql_par* getRecVersion() { return recVersion; }
	const dsql_par* getRecVersion() const { return recVersion; }
	void setRecVersion(dsql_par* value) { recVersion = value; }

	dsql_par* getParentRecVersion() { return parentRecVersion; }
	const dsql_par* getParentRecVersion() const { return parentRecVersion; }
	void setParentRecVersion(dsql_par* value) { parentRecVersion = value; }

	dsql_par* getParentDbKey() { return parentDbKey; }
	const dsql_par* getParentDbKey() const { return parentDbKey; }
	void setParentDbKey(dsql_par* value) { parentDbKey = value; }

	dsql_req* getParentRequest() const { return parentRequest; }
	void setParentRequest(dsql_req* value) { parentRequest = value; }

private:
	Type type;					// Type of statement
	ULONG flags;				// generic flag
	unsigned blrVersion;
	Firebird::RefStrPtr sqlText;
	dsql_msg* sendMsg;			// Message to be sent to start request
	dsql_msg* receiveMsg;		// Per record message to be received
	dsql_par* eof;				// End of file parameter
	dsql_par* dbKey;			// Database key for current of
	dsql_par* recVersion;		// Record Version for current of
	dsql_par* parentRecVersion;	// parent record version
	dsql_par* parentDbKey;		// Parent database key for current of
	dsql_req* parentRequest;	// Source request, if cursor update
};

class dsql_req : public pool_alloc<dsql_type_req>
{
public:
	static const unsigned FLAG_OPENED_CURSOR	= 0x01;

public:
	explicit dsql_req(MemoryPool& pool);

public:
	MemoryPool& getPool()
	{
		return req_pool;
	}

	jrd_tra* getTransaction()
	{
		return req_transaction;
	}

	const DsqlCompiledStatement* getStatement() const
	{
		return statement;
	}

	virtual void dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
		ntrace_result_t* traceResult) = 0;

	virtual void execute(thread_db* tdbb, jrd_tra** traHandle,
		Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
		Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
		bool singleton) = 0;

	virtual void setCursor(thread_db* tdbb, const TEXT* name);

	virtual bool fetch(thread_db* tdbb, UCHAR* buffer);

	virtual void setDelayedFormat(thread_db* tdbb, Firebird::IMessageMetadata* metadata);

	static void destroy(thread_db* tdbb, dsql_req* request, bool drop);

private:
	MemoryPool&	req_pool;

public:
	const DsqlCompiledStatement* statement;
	Firebird::Array<DsqlCompiledStatement*> cursors;	// Cursor update statements

	dsql_dbb* req_dbb;			// DSQL attachment
	jrd_tra* req_transaction;	// JRD transaction
	jrd_req* req_request;		// JRD request

	unsigned req_flags;			// flags

	Firebird::Array<UCHAR*>	req_msg_buffers;
	Firebird::string req_cursor;	// Cursor name, if any
	Firebird::GenericMap<Firebird::NonPooled<const dsql_par*, dsc> > req_user_descs; // SQLDA data type

	Firebird::AutoPtr<Jrd::RuntimeStatistics> req_fetch_baseline; // State of request performance counters when we reported it last time
	SINT64 req_fetch_elapsed;		// Number of clock ticks spent while fetching rows for this request since we reported it last time
	SINT64 req_fetch_rowcount;		// Total number of rows returned by this request
	bool req_traced;				// request is traced via TraceAPI

	JStatement* req_interface;

protected:
	// Request should never be destroyed using delete.
	// It dies together with it's pool in release_request().
	~dsql_req()
	{
	}

	// To avoid posix warning about missing public destructor declare
	// MemoryPool as friend class. In fact IT releases request memory!
	friend class Firebird::MemoryPool;
};

class DsqlDmlRequest : public dsql_req
{
public:
	explicit DsqlDmlRequest(MemoryPool& pool, StmtNode* aNode)
		: dsql_req(pool),
		  node(aNode),
		  needDelayedFormat(false)
	{
	}

	virtual void dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
		ntrace_result_t* traceResult);

	virtual void execute(thread_db* tdbb, jrd_tra** traHandle,
		Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
		Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
		bool singleton);

	virtual void setCursor(thread_db* tdbb, const TEXT* name);

	virtual bool fetch(thread_db* tdbb, UCHAR* buffer);

	virtual void setDelayedFormat(thread_db* tdbb, Firebird::IMessageMetadata* metadata);

private:
	NestConst<StmtNode> node;
	Firebird::RefPtr<Firebird::IMessageMetadata> delayedFormat;
	bool needDelayedFormat;
};

class DsqlDdlRequest : public dsql_req
{
public:
	explicit DsqlDdlRequest(MemoryPool& pool, DdlNode* aNode)
		: dsql_req(pool),
		  node(aNode),
		  internalScratch(NULL)
	{
	}

	virtual void dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
		ntrace_result_t* traceResult);

	virtual void execute(thread_db* tdbb, jrd_tra** traHandle,
		Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
		Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
		bool singleton);

private:
	// Rethrow an exception with isc_no_meta_update and prefix codes.
	void rethrowDdlException(Firebird::status_exception& ex, bool metadataUpdate);

private:
	NestConst<DdlNode> node;
	DsqlCompilerScratch* internalScratch;
};

class DsqlTransactionRequest : public dsql_req
{
public:
	explicit DsqlTransactionRequest(MemoryPool& pool, TransactionNode* aNode)
		: dsql_req(pool),
		  node(aNode)
	{
		req_traced = false;
	}

	virtual void dsqlPass(thread_db* tdbb, DsqlCompilerScratch* scratch,
		ntrace_result_t* traceResult);

	virtual void execute(thread_db* tdbb, jrd_tra** traHandle,
		Firebird::IMessageMetadata* inMetadata, const UCHAR* inMsg,
		Firebird::IMessageMetadata* outMetadata, UCHAR* outMsg,
		bool singleton);

private:
	NestConst<TransactionNode> node;
};

//! Implicit (NATURAL and USING) joins
class ImplicitJoin : public pool_alloc<dsql_type_imp_join>
{
public:
	ValueExprNode* value;
	dsql_ctx* visibleInContext;
};

struct PartitionMap
{
	PartitionMap(ValueListNode* aPartition, ValueListNode* aOrder)
		: partition(aPartition),
		  partitionRemapped(NULL),
		  order(aOrder),
		  map(NULL),
		  context(0)
	{
	}

	NestConst<ValueListNode> partition;
	NestConst<ValueListNode> partitionRemapped;
	NestConst<ValueListNode> order;
	dsql_map* map;
	USHORT context;
};

//! Context block used to create an instance of a relation reference
class dsql_ctx : public pool_alloc<dsql_type_ctx>
{
public:
	explicit dsql_ctx(MemoryPool& p)
		: ctx_alias(p),
		  ctx_internal_alias(p),
		  ctx_main_derived_contexts(p),
		  ctx_childs_derived_table(p),
	      ctx_imp_join(p),
	      ctx_win_maps(p)
	{
	}

	dsql_rel*			ctx_relation;		// Relation for context
	dsql_prc*			ctx_procedure;		// Procedure for context
	NestConst<ValueListNode> ctx_proc_inputs;	// Procedure input parameters
	dsql_map*			ctx_map;			// Maps for aggregates and unions
	RseNode*			ctx_rse;			// Sub-rse for aggregates
	dsql_ctx*			ctx_parent;			// Parent context for aggregates
	USHORT				ctx_context;		// Context id
	USHORT				ctx_recursive;		// Secondary context id for recursive UNION (nobody referred to this context)
	USHORT				ctx_scope_level;	// Subquery level within this request
	USHORT				ctx_flags;			// Various flag values
	USHORT				ctx_in_outer_join;	// inOuterJoin when context was created
	Firebird::string	ctx_alias;			// Context alias (can include concatenated derived table alias)
	Firebird::string	ctx_internal_alias;	// Alias as specified in query
	DsqlContextStack	ctx_main_derived_contexts;	// contexts used for blr_derived_expr
	DsqlContextStack	ctx_childs_derived_table;	// Childs derived table context
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, ImplicitJoin*> > > ctx_imp_join;	// Map of USING fieldname to ImplicitJoin
	Firebird::Array<PartitionMap*> ctx_win_maps;	// Maps for window functions

	dsql_ctx& operator=(dsql_ctx& v)
	{
		ctx_relation = v.ctx_relation;
		ctx_procedure = v.ctx_procedure;
		ctx_proc_inputs = v.ctx_proc_inputs;
		ctx_map = v.ctx_map;
		ctx_rse = v.ctx_rse;
		ctx_parent = v.ctx_parent;
		ctx_alias = v.ctx_alias;
		ctx_context = v.ctx_context;
		ctx_recursive = v.ctx_recursive;
		ctx_scope_level = v.ctx_scope_level;
		ctx_flags = v.ctx_flags;
		ctx_in_outer_join = v.ctx_in_outer_join;
		ctx_main_derived_contexts.assign(v.ctx_main_derived_contexts);
		ctx_childs_derived_table.assign(v.ctx_childs_derived_table);
		ctx_imp_join.assign(v.ctx_imp_join);
		ctx_win_maps.assign(v.ctx_win_maps);

		return *this;
	}

	Firebird::string getObjectName() const
	{
		if (ctx_relation)
			return ctx_relation->rel_name.c_str();
		if (ctx_procedure)
			return ctx_procedure->prc_name.toString();
		return "";
	}

	bool getImplicitJoinField(const Firebird::MetaName& name, NestConst<ValueExprNode>& node);
	PartitionMap* getPartitionMap(DsqlCompilerScratch* dsqlScratch,
		ValueListNode* partitionNode, ValueListNode* orderNode);
};

// Flag values for ctx_flags

const USHORT CTX_outer_join 			= 0x01;	// reference is part of an outer join
const USHORT CTX_system					= 0x02;	// Context generated by system (NEW/OLD in triggers, check-constraint, RETURNING)
const USHORT CTX_null					= 0x04;	// Fields of the context should be resolved to NULL constant
const USHORT CTX_returning				= 0x08;	// Context generated by RETURNING
const USHORT CTX_recursive				= 0x10;	// Context has secondary number (ctx_recursive) generated for recursive UNION
const USHORT CTX_view_with_check_store	= 0x20;	// Context of WITH CHECK OPTION view's store trigger
const USHORT CTX_view_with_check_modify	= 0x40;	// Context of WITH CHECK OPTION view's modify trigger

//! Aggregate/union map block to map virtual fields to their base
//! TMN: NOTE! This datatype should definitely be renamed!
class dsql_map : public pool_alloc<dsql_type_map>
{
public:
	dsql_map* map_next;						// Next map in item
	NestConst<ValueExprNode> map_node;		// Value for map item
	USHORT map_position;					// Position in map
	NestConst<PartitionMap> map_partition;	// Partition
};

// Message block used in communicating with a running request
class dsql_msg : public Firebird::PermanentStorage
{
public:
	explicit dsql_msg(MemoryPool& p)
		: PermanentStorage(p),
		  msg_parameters(p),
		  msg_number(0),
		  msg_buffer_number(0),
		  msg_length(0),
		  msg_parameter(0),
		  msg_index(0)
	{
	}

	Firebird::Array<dsql_par*> msg_parameters;	// Parameter list
	USHORT		msg_number;		// Message number
	USHORT		msg_buffer_number;	// Message buffer number (used instead of msg_number for blob msgs)
	ULONG		msg_length;		// Message length
	USHORT		msg_parameter;	// Next parameter number
	USHORT		msg_index;		// Next index into SQLDA
};

// Parameter block used to describe a parameter of a message
class dsql_par : public Firebird::PermanentStorage
{
public:
	explicit dsql_par(MemoryPool& p)
		: PermanentStorage(p),
		  par_message(NULL),
		  par_null(NULL),
		  par_node(NULL),
		  par_dbkey_relname(p),
		  par_rec_version_relname(p),
		  par_name(p),
		  par_rel_name(p),
		  par_owner_name(p),
		  par_rel_alias(p),
		  par_alias(p),
		  par_parameter(0),
		  par_index(0),
		  par_is_text(false)
	{
		par_desc.clear();
	}

	dsql_msg*	par_message;		// Parent message
	dsql_par*	par_null;			// Null parameter, if used
	ValueExprNode* par_node;					// Associated value node, if any
	Firebird::MetaName par_dbkey_relname;		// Context of internally requested dbkey
	Firebird::MetaName par_rec_version_relname;	// Context of internally requested rec. version
	Firebird::MetaName par_name;				// Parameter name, if any
	Firebird::MetaName par_rel_name;			// Relation name, if any
	Firebird::MetaName par_owner_name;			// Owner name, if any
	Firebird::MetaName par_rel_alias;			// Relation alias, if any
	Firebird::MetaName par_alias;				// Alias, if any
	dsc			par_desc;			// Field data type
	USHORT		par_parameter;		// BLR parameter number
	USHORT		par_index;			// Index into SQLDA, if appropriate
	bool		par_is_text;		// Parameter should be dtype_text (SQL_TEXT) externaly
};

class CStrCmp
{
public:
	static int greaterThan(const char* s1, const char* s2)
	{
		return strcmp(s1, s2) > 0;
	}
};

typedef Firebird::SortedArray<const char*,
			Firebird::EmptyStorage<const char*>, const char*,
			Firebird::DefaultKeyValue<const char*>,
			CStrCmp>
		StrArray;

class IntlString
{
public:
	IntlString(Firebird::MemoryPool& p, const Firebird::string& str,
		const Firebird::MetaName& cs = NULL)
		: charset(p, cs),
		  s(p, str)
	{ }

	explicit IntlString(const Firebird::string& str, const Firebird::MetaName& cs = NULL)
		: charset(cs),
		  s(str)
	{ }

	IntlString(Firebird::MemoryPool& p, const IntlString& o)
		: charset(p, o.charset),
		  s(p, o.s)
	{ }

	explicit IntlString(Firebird::MemoryPool& p)
		: charset(p),
		  s(p)
	{ }

	Firebird::string toUtf8(DsqlCompilerScratch*) const;

	const Firebird::MetaName& getCharSet() const
	{
		return charset;
	}

	void setCharSet(const Firebird::MetaName& value)
	{
		charset = value;
	}

	const Firebird::string& getString() const
	{
		return s;
	}

	bool hasData() const
	{
		return s.hasData();
	}

	bool isEmpty() const
	{
		return s.isEmpty();
	}

private:
	Firebird::MetaName charset;
	Firebird::string s;
};

} // namespace

/*! \var unsigned DSQL_debug
    \brief Debug level

    0       No output
    1       Display output tree in PASS1_statment
    2       Display input tree in PASS1_statment
    4       Display ddl BLR
    8       Display BLR
    16      Display PASS1_rse input tree
    32      Display SQL input string
    64      Display BLR in dsql/prepare
    > 256   Display yacc parser output level = DSQL_level>>8
*/

#ifdef DSQL_DEBUG
extern unsigned DSQL_debug;
#endif

#endif // DSQL_DSQL_H
