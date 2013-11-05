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

#include "../jrd/common.h"
#include "../jrd/RuntimeStatistics.h"
#include "../jrd/val.h"  // Get rid of duplicated FUN_T enum.
#include "../jrd/Database.h"
#include "../common/classes/array.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/stack.h"
#include "../common/classes/auto.h"

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

// Context aliases used in triggers
const char* const OLD_CONTEXT		= "OLD";
const char* const NEW_CONTEXT		= "NEW";
const char* const TEMP_CONTEXT		= "TEMP";

namespace Jrd
{
	class Database;
	class Attachment;
	class jrd_tra;
	class jrd_req;
	class blb;
	struct bid;

	class dsql_ctx;
	class dsql_str;
	class dsql_nod;
	class dsql_intlsym;

	typedef Firebird::Stack<dsql_ctx*> DsqlContextStack;
	typedef Firebird::Stack<const dsql_str*> DsqlStrStack;
	typedef Firebird::Stack<dsql_nod*> DsqlNodStack;
}

namespace Firebird
{
	class MetaName;
}

//======================================================================
// remaining node definitions for local processing
//

/// Include definition of descriptor

#include "../jrd/dsc.h"

namespace Jrd {

//! generic data type used to store strings
class dsql_str : public pool_alloc_rpt<char, dsql_type_str>
{
public:
	enum Type
	{
		TYPE_SIMPLE = 0,
		TYPE_HEXA,
		TYPE_DELIMITED
	};

public:
	const char* str_charset;	// ASCIIZ Character set identifier for string
	Type	type;
	ULONG	str_length;		// length of string in BYTES
	char	str_data[2];	// one for ALLOC and one for the NULL
};

// blocks used to cache metadata

//! Database Block
typedef Firebird::SortedArray
	<
		dsql_intlsym*,
		Firebird::EmptyStorage<dsql_intlsym*>,
		SSHORT,
		dsql_intlsym,
		Firebird::DefaultComparator<SSHORT>
	> IntlSymArray;

class dsql_dbb : public pool_alloc<dsql_type_dbb>
{
public:
	class dsql_rel* dbb_relations;		// known relations in database
	class dsql_prc*	dbb_procedures;		// known procedures in database
	class dsql_udf*	dbb_functions;		// known functions in database
	MemoryPool&		dbb_pool;			// The current pool for the dbb
	Database*		dbb_database;
	Attachment*		dbb_attachment;
	Firebird::SortedArray<dsql_req*> dbb_requests;
	dsql_str*		dbb_dfl_charset;
#ifdef SCROLLABLE_CURSORS
	USHORT			dbb_base_level;		// indicates the version of the engine code itself
#endif
	bool			dbb_no_charset;
	bool			dbb_read_only;
	USHORT			dbb_db_SQL_dialect;
	IntlSymArray	dbb_charsets_by_id;	// charsets sorted by charset_id
	USHORT			dbb_ods_version;	// major ODS version number
	USHORT			dbb_minor_version;	// minor ODS version number
	Firebird::Mutex dbb_cache_mutex;	// mutex protecting the DSQL metadata cache

	explicit dsql_dbb(MemoryPool& p) :
		dbb_pool(p), 
		dbb_requests(p),
		dbb_charsets_by_id(p, 16)
	{}

	~dsql_dbb();

	MemoryPool* createPool()
	{
		return dbb_database->createPool();
	}

	void deletePool(MemoryPool* pool)
	{
		dbb_database->deletePool(pool);
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

	dsql_rel*	rel_next;			// Next relation in database
	dsql_sym*	rel_symbol;			// Hash symbol for relation
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

class dsql_fld : public pool_alloc<dsql_type_fld>
{
public:
	explicit dsql_fld(MemoryPool& p)
		: fld_type_of_name(p),
		  fld_type_of_table(p),
		  fld_name(p),
		  fld_source(p)
	{
	}

	dsql_fld*	fld_next;				// Next field in relation
	dsql_rel*	fld_relation;			// Parent relation
	class dsql_prc*	fld_procedure;		// Parent procedure
	dsql_nod*	fld_ranges;				// ranges for multi dimension array
	dsql_nod*	fld_character_set;		// null means not specified
	dsql_nod*	fld_sub_type_name;		// Subtype name for later resolution
	USHORT		fld_flags;
	USHORT		fld_id;					// Field in in database
	USHORT		fld_dtype;				// Data type of field
	FLD_LENGTH	fld_length;				// Length of field
	USHORT		fld_element_dtype;		// Data type of array element
	USHORT		fld_element_length;		// Length of array element
	SSHORT		fld_scale;				// Scale factor of field
	SSHORT		fld_sub_type;			// Subtype for text & blob fields
	USHORT		fld_precision;			// Precision for exact numeric types
	USHORT		fld_character_length;	// length of field in characters
	USHORT		fld_seg_length;			// Segment length for blobs
	SSHORT		fld_dimensions;			// Non-zero means array
	SSHORT		fld_character_set_id;	// ID of field's character set
	SSHORT		fld_collation_id;		// ID of field's collation
	SSHORT		fld_ttype;				// ID of field's language_driver
	Firebird::string fld_type_of_name;	// TYPE OF
	Firebird::string fld_type_of_table;	// TYPE OF table name
	bool		fld_explicit_collation;	// COLLATE was explicit specified
	bool		fld_not_nullable;		// NOT NULL was explicit specified
	bool		fld_full_domain;		// Domain name without TYPE OF prefix
	Firebird::string fld_name;
	Firebird::MetaName fld_source;
};

// values used in fld_flags

enum fld_flags_vals {
	FLD_computed	= 1,
	FLD_national	= 2, // field uses NATIONAL character set
	FLD_nullable	= 4,
	FLD_system		= 8
};

//! database/log/cache file block
class dsql_fil : public pool_alloc<dsql_type_fil>
{
public:
	SLONG	fil_length;			// File length in pages
	SLONG	fil_start;			// Starting page
	dsql_str*	fil_name;			// File name
	//dsql_fil*	fil_next;			// next file
	//SSHORT	fil_shadow_number;	// shadow number if part of shadow
	//SSHORT	fil_manual;			// flag to indicate manual shadow
	//SSHORT	fil_partitions;		// number of log file partitions
	//USHORT	fil_flags;
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

	dsql_prc*	prc_next;		// Next relation in database
	dsql_sym*	prc_symbol;		// Hash symbol for procedure
	dsql_fld*	prc_inputs;		// Input parameters
	dsql_fld*	prc_outputs;	// Output parameters
	Firebird::MetaName prc_name;	// Name of procedure
	Firebird::MetaName prc_owner;	// Owner of procedure
	SSHORT		prc_in_count;
	SSHORT		prc_def_count;	// number of inputs with default values
	SSHORT		prc_out_count;
	USHORT		prc_id;			// Procedure id
	USHORT		prc_flags;
};

// prc_flags bits

enum prc_flags_vals {
	PRC_new_procedure	= 1, // procedure is newly defined, not committed yet
	PRC_dropped			= 2  // procedure has been dropped
};

//! User defined function block
class dsql_udf : public pool_alloc<dsql_type_udf>
{
public:
	explicit dsql_udf(MemoryPool& p)
		: udf_name(p), udf_arguments(p)
	{
	}

	dsql_udf*	udf_next;
	dsql_sym*	udf_symbol;		// Hash symbol for udf
	USHORT		udf_dtype;
	SSHORT		udf_scale;
	SSHORT		udf_sub_type;
	USHORT		udf_length;
	SSHORT		udf_character_set_id;
	//USHORT		udf_character_length;
    USHORT      udf_flags;
	Firebird::MetaName udf_name;
	Firebird::Array<dsc> udf_arguments;
};

// udf_flags bits

enum udf_flags_vals {
	UDF_new_udf		= 1, // udf is newly declared, not committed yet
	UDF_dropped		= 2  // udf has been dropped
};

// Variables - input, output & local

//! Variable block
class dsql_var : public pool_alloc_rpt<SCHAR, dsql_type_var>
{
public:
	dsql_fld*	var_field;		// Field on which variable is based
	//USHORT	var_flags;			// Reserved
	//dsql_var_type	var_type;	// Too cumbersome to compile the right data type.
	int		var_type;			// Input, output or local var.
	USHORT	var_msg_number;		// Message number containing variable
	USHORT	var_msg_item;		// Item number in message
	USHORT	var_variable_number;	// Local variable number
	TEXT	var_name[2];
};


// Symbolic names for international text types
// (either collation or character set name)

//! International symbol
class dsql_intlsym : public pool_alloc_rpt<SCHAR, dsql_type_intlsym>
{
public:
	dsql_sym*	intlsym_symbol;		// Hash symbol for intlsym
	USHORT		intlsym_type;		// what type of name
	USHORT		intlsym_flags;
	SSHORT		intlsym_ttype;		// id of implementation
	SSHORT		intlsym_charset_id;
	SSHORT		intlsym_collate_id;
	USHORT		intlsym_bytes_per_char;
	TEXT		intlsym_name[2];

	static SSHORT generate(const void*, const dsql_intlsym* Item)
	{
		return Item->intlsym_charset_id;
	}
};

// values used in intlsym_flags

enum intlsym_flags_vals {
	INTLSYM_dropped	= 1  // intlsym has been dropped
};


// Forward declaration.
class dsql_par;

//! Request information
enum REQ_TYPE
{
	REQ_SELECT, REQ_SELECT_UPD, REQ_INSERT, REQ_DELETE, REQ_UPDATE,
	REQ_UPDATE_CURSOR, REQ_DELETE_CURSOR,
	REQ_COMMIT, REQ_ROLLBACK, REQ_CREATE_DB, REQ_DDL, REQ_EMBED_SELECT,
	REQ_START_TRANS, REQ_GET_SEGMENT, REQ_PUT_SEGMENT, REQ_EXEC_PROCEDURE,
	REQ_COMMIT_RETAIN, REQ_ROLLBACK_RETAIN, REQ_SET_GENERATOR, REQ_SAVEPOINT,
	REQ_EXEC_BLOCK, REQ_SELECT_BLOCK
};


class dsql_req : public pool_alloc<dsql_type_req>
{
public:
	dsql_req*	req_parent;		// Source request, if cursor update
	dsql_req*	req_sibling;	// Next sibling request, if cursor update
	dsql_req*	req_offspring;	// Cursor update requests
	MemoryPool&	req_pool;

	dsql_sym* req_name;			// Name of request
	dsql_sym* req_cursor;		// Cursor symbol, if any
	dsql_dbb* req_dbb;			// DSQL attachment
	jrd_tra* req_transaction;	// JRD transaction
	dsql_nod* req_ddl_node;		// Store metadata request
	class dsql_blb* req_blob;	// Blob info for blob requests
	jrd_req*	req_request;	// JRD request
	//dsql_str*	req_blr_string;	// String block during BLR generation
	Firebird::HalfStaticArray<BLOB_PTR, 1024> req_blr_data;
	class dsql_msg* req_send;		// Message to be sent to start request
	class dsql_msg* req_receive;	// Per record message to be received
	class dsql_msg* req_async;		// Message for sending scrolling information
	dsql_par* req_eof;			// End of file parameter
	dsql_par* req_dbkey;		// Database key for current of
	dsql_par* req_rec_version;	// Record Version for current of
	dsql_par* req_parent_rec_version;	// parent record version
	dsql_par* req_parent_dbkey;	// Parent database key for current of
	//BLOB_PTR* req_blr;		// Running blr address
	//BLOB_PTR* req_blr_yellow;	// Threshold for upping blr buffer size
	ULONG	req_inserts;		// records processed in request
	ULONG	req_deletes;
	ULONG	req_updates;
	ULONG	req_selects;
	REQ_TYPE req_type;			// Type of request
	ULONG	req_flags;			// generic flag

	Firebird::RefStrPtr req_sql_text;

	Firebird::AutoPtr<Jrd::RuntimeStatistics> req_fetch_baseline; // State of request performance counters when we reported it last time
	SINT64 req_fetch_elapsed;		// Number of clock ticks spent while fetching rows for this request since we reported it last time
	SINT64 req_fetch_rowcount;		// Total number of rows returned by this request
	bool req_traced;				// request is traced via TraceAPI

protected:
	dsql_req(MemoryPool& p)
		: req_pool(p),
		  req_blr_data(p)
	{
	}
	// Request should never be destroyed using delete.
	// It dies together with it's pool in release_request().
	~dsql_req()
	{
	}

	// To avoid posix warning about missing public destructor declare
	// MemoryPool as friend class. In fact IT releases request memory!
	friend class Firebird::MemoryPool;
};


class CompiledStatement : public dsql_req
{
public:
	explicit CompiledStatement(MemoryPool& p)
		: dsql_req(p),
		  req_debug_data(p),
		  req_main_context(p),
		  req_context(&req_main_context),
		  req_union_context(p),
		  req_dt_context(p),
		  req_labels(p),
		  req_cursors(p),
		  req_hidden_vars(p),
		  req_curr_ctes(p),
		  req_ctes(p),
		  req_cte_aliases(p)
	{
	}

protected:
	// Request should never be destroyed using delete.
	// It dies together with its pool in release_request().
	~CompiledStatement();

public:
	// begin - member functions that should be private
	void append_uchar(UCHAR byte)
	{
		req_blr_data.add(byte);
	}

	void append_ushort(USHORT val)
	{
		append_uchar(val);
		append_uchar(val >> 8);
	}

	void append_ulong(ULONG val)
	{
		append_ushort(val);
		append_ushort(val >> 16);
	}

	void		append_cstring(UCHAR verb, const char* string);
	void		append_meta_string(const char* string);
	void		append_user_string(UCHAR verb, const dsql_str* str);
	void        append_raw_string(const char* string, USHORT len);
	void        append_raw_string(const UCHAR* string, USHORT len);
	void		append_string(UCHAR verb, const char* string, USHORT len);
	void		append_string(UCHAR verb, const Firebird::MetaName& name);
	void		append_string(UCHAR verb, const Firebird::string& name);
	void		append_number(UCHAR verb, SSHORT number);
	void		begin_blr(UCHAR verb);
	void		end_blr();
	void		append_uchars(UCHAR byte, int count);
	void		append_ushort_with_length(USHORT val);
	void		append_ulong_with_length(ULONG val);
	void		append_file_length(ULONG length);
	void		append_file_start(ULONG start);
	void		generate_unnamed_trigger_beginning(	bool		on_update_trigger,
													const char*	prim_rel_name,
													const dsql_nod* prim_columns,
													const char*	for_rel_name,
													const dsql_nod* for_columns);

	void	begin_debug();
	void	end_debug();
	void	put_debug_src_info(USHORT, USHORT);
	void	put_debug_variable(USHORT, const TEXT*);
	void	put_debug_argument(UCHAR, USHORT, const TEXT*);
	void	append_debug_info();
	// end - member functions that should be private

	void addCTEs(dsql_nod* list);
	dsql_nod* findCTE(const dsql_str* name);
	void clearCTEs();
	void checkUnusedCTEs() const;

	// hvlad: each member of recursive CTE can refer to CTE itself (only once) via
	// CTE name or via alias. We need to substitute this aliases when processing CTE
	// member to resolve field names. Therefore we store all aliases in order of
	// occurrence and later use it in backward order (since our parser is right-to-left).
	// Also we put CTE name after all such aliases to distinguish aliases for 
	// different CTE's.
	// We also need to repeat this process if main select expression contains union with
	// recursive CTE
	void addCTEAlias(const dsql_str* alias)
	{
		req_cte_aliases.add(alias);
	}
	const dsql_str* getNextCTEAlias()
	{
		return *(--req_curr_cte_alias);
	}
	void resetCTEAlias(const dsql_str* alias)
	{
		const dsql_str* const* begin = req_cte_aliases.begin();

		req_curr_cte_alias = req_cte_aliases.end() - 1;
		fb_assert(req_curr_cte_alias >= begin);

		const dsql_str* curr = *(req_curr_cte_alias);
		while (strcmp(curr->str_data, alias->str_data)) 
		{
			req_curr_cte_alias--;
			fb_assert(req_curr_cte_alias >= begin);

			curr = *(req_curr_cte_alias);
		}
	}

	bool isPsql() const
	{
		return psql;
	}

	void setPsql(bool value)
	{
		psql = value;
	}

	dsql_nod* req_blk_node;		// exec_block node
	dsql_rel* req_relation;		// relation created by this request (for DDL)
	dsql_prc* req_procedure;	// procedure created by this request (for DDL)
	Firebird::HalfStaticArray<BLOB_PTR, 128> req_debug_data;
	DsqlContextStack	req_main_context;
	DsqlContextStack*	req_context;
	DsqlContextStack	req_union_context;	// Save contexts for views of unions
	DsqlContextStack	req_dt_context;		// Save contexts for views of derived tables
	class dsql_ctx* req_outer_agg_context;	// agg context for outer ref
	ULONG	req_base_offset;		// place to go back and stuff in blr length
	USHORT	req_context_number;	// Next available context number
	USHORT	req_derived_context_number;	// Next available context number for derived tables
	USHORT	req_scope_level;		// Scope level for parsing aliases in subqueries
	//USHORT	req_message_number;	// Next available message number
	USHORT	req_loop_level;		// Loop level
	DsqlStrStack	req_labels;		// Loop labels
	USHORT	req_cursor_number;		// Cursor number
	DsqlNodStack	req_cursors;	// Cursors
	USHORT	req_in_select_list;		// now processing "select list"
	USHORT	req_in_where_clause;	// processing "where clause"
	USHORT	req_in_group_by_clause;	// processing "group by clause"
	USHORT	req_in_having_clause;	// processing "having clause"
	USHORT	req_in_order_by_clause;	// processing "order by clause"
	USHORT	req_error_handlers;	// count of active error handlers
	USHORT	req_client_dialect;	// dialect passed into the API call
	USHORT	req_in_outer_join;	// processing inside outer-join part
	dsql_str*		req_alias_relation_prefix;	// prefix for every relation-alias.
	DsqlNodStack	req_hidden_vars;			// hidden variables
	USHORT			req_hidden_vars_number;		// next hidden variable number

	DsqlNodStack req_curr_ctes;			// current processing CTE's
	class dsql_ctx* req_recursive_ctx;	// context of recursive CTE
	USHORT req_recursive_ctx_id;		// id of recursive union stream context

private:
	Firebird::HalfStaticArray<dsql_nod*, 4> req_ctes; // common table expressions
	Firebird::HalfStaticArray<const dsql_str*, 4> req_cte_aliases; // CTE aliases in recursive members
	const dsql_str* const* req_curr_cte_alias;

	bool psql;
};


class PsqlChanger
{
public:
	PsqlChanger(CompiledStatement* aStatement, bool value)
		: statement(aStatement),
		  oldValue(statement->isPsql())
	{
		statement->setPsql(value);
	}

	~PsqlChanger()
	{
		statement->setPsql(oldValue);
	}

private:
	// copying is prohibited
	PsqlChanger(const PsqlChanger&);
	PsqlChanger& operator =(const PsqlChanger&);

	CompiledStatement* statement;
	const bool oldValue;
};


// values used in req_flags
enum req_flags_vals {
	REQ_cursor_open			= 0x00001,
	REQ_save_metadata		= 0x00002,
	REQ_prepared			= 0x00004, // Set in DSQL_prepare but never checked
	REQ_procedure			= 0x00008,
	REQ_trigger				= 0x00010,
	REQ_orphan				= 0x00020,
	//REQ_enforce_scope		= 0x00040, // NOT USED
	REQ_no_batch			= 0x00080,
#ifdef SCROLLABLE_CURSORS
	REQ_backwards			= 0x00100,
#endif
	REQ_blr_version4		= 0x00200,
	REQ_blr_version5		= 0x00400,
	REQ_block				= 0x00800,
	REQ_selectable			= 0x01000,
	REQ_CTE_recursive		= 0x02000,
	REQ_dsql_upd_or_ins		= 0x04000,
	REQ_returning_into		= 0x08000,
	REQ_in_auto_trans_block	= 0x10000
};

//! Blob
class dsql_blb : public pool_alloc<dsql_type_blb>
{
public:
	// blb_field is currently assigned in one place and never used
	//dsql_nod*	blb_field;			// Related blob field
	dsql_par*	blb_blob_id;		// Parameter to hold blob id
	dsql_par*	blb_segment;		// Parameter for segments
	dsql_nod*	blb_from;
	dsql_nod*	blb_to;
	dsql_msg*	blb_open_in_msg;	// Input message to open cursor
	dsql_msg*	blb_open_out_msg;	// Output message from open cursor
	dsql_msg*	blb_segment_msg;	// Segment message
	blb*		blb_blob;			// JRD blob
};

//! Transaction block
/* UNUSED
class dsql_tra : public pool_alloc<dsql_type_tra>
{
public:
	dsql_tra* tra_next;		// Next open transaction
};
*/

//! Implicit (NATURAL and USING) joins
class ImplicitJoin : public pool_alloc<dsql_type_imp_join>
{
public:
	dsql_nod* value;
	dsql_ctx* visibleInContext;
};

//! Context block used to create an instance of a relation reference
class dsql_ctx : public pool_alloc<dsql_type_ctx>
{
public:
	explicit dsql_ctx(MemoryPool &p)
		: ctx_main_derived_contexts(p),
		  ctx_childs_derived_table(p),
	      ctx_imp_join(p)
	{
	}

	dsql_req*			ctx_request;		// Parent request
	dsql_rel*			ctx_relation;		// Relation for context
	dsql_prc*			ctx_procedure;		// Procedure for context
	dsql_nod*			ctx_proc_inputs;	// Procedure input parameters
	class dsql_map*		ctx_map;			// Map for aggregates
	dsql_nod*			ctx_rse;			// Sub-rse for aggregates
	dsql_ctx*			ctx_parent;			// Parent context for aggregates
	const TEXT*			ctx_alias;			// Context alias (can include concatenated derived table alias)
	const TEXT*			ctx_internal_alias;	// Alias as specified in query
	USHORT				ctx_context;		// Context id
	USHORT				ctx_recursive;		// Secondary context id for recursive UNION (nobody referred to this context)
	USHORT				ctx_scope_level;	// Subquery level within this request
	USHORT				ctx_flags;			// Various flag values
	USHORT				ctx_in_outer_join;	// req_in_outer_join when context was created
	DsqlContextStack	ctx_main_derived_contexts;	// contexts used for blr_derived_expr
	DsqlContextStack	ctx_childs_derived_table;	// Childs derived table context
	Firebird::GenericMap<Firebird::Pair<Firebird::Left<
		Firebird::MetaName, ImplicitJoin*> > > ctx_imp_join;	// Map of USING fieldname to ImplicitJoin

	dsql_ctx& operator=(dsql_ctx& v)
	{
		ctx_request = v.ctx_request;
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

		return *this;
	}

	bool getImplicitJoinField(const Firebird::MetaName& name, dsql_nod*& node);
};

// Flag values for ctx_flags

const USHORT CTX_outer_join = 0x01;	// reference is part of an outer join
const USHORT CTX_system		= 0x02;	// Context generated by system (NEW/OLD in triggers, check-constraint, RETURNING)
const USHORT CTX_null		= 0x04;	// Fields of the context should be resolved to NULL constant
const USHORT CTX_returning	= 0x08;	// Context generated by RETURNING
const USHORT CTX_recursive	= 0x10;	// Context has secondary number (ctx_recursive) generated for recursive UNION

//! Aggregate/union map block to map virtual fields to their base
//! TMN: NOTE! This datatype should definitely be renamed!
class dsql_map : public pool_alloc<dsql_type_map>
{
public:
	dsql_map*	map_next;			// Next map in item
	dsql_nod*	map_node;			// Value for map item
	USHORT		map_position;		// Position in map
};

//! Message block used in communicating with a running request
class dsql_msg : public pool_alloc<dsql_type_msg>
{
public:
	dsql_par*	msg_parameters;	// Parameter list
	UCHAR*		msg_buffer;		// Message buffer
	USHORT		msg_number;		// Message number
	USHORT		msg_length;		// Message length
	USHORT		msg_parameter;	// Next parameter number
	USHORT		msg_index;		// Next index into SQLDA
};

//! Parameter block used to describe a parameter of a message
class dsql_par : public pool_alloc<dsql_type_par>
{
public:
	dsql_msg*	par_message;		// Parent message
	dsql_par*	par_next;			// Next parameter in linked list
	dsql_par*	par_null;			// Null parameter, if used
	dsql_nod*	par_node;			// Associated value node, if any
	dsql_ctx*	par_dbkey_ctx;		// Context of internally requested dbkey
	dsql_ctx*	par_rec_version_ctx;	// Context of internally requested record version
	const TEXT*	par_name;			// Parameter name, if any
	const TEXT*	par_rel_name;		// Relation name, if any
	const TEXT*	par_owner_name;		// Owner name, if any
	const TEXT*	par_rel_alias;		// Relation alias, if any
	const TEXT*	par_alias;			// Alias, if any
	DSC			par_desc;			// Field data type
	DSC			par_user_desc;		// SQLDA data type
	USHORT		par_parameter;		// BLR parameter number
	USHORT		par_index;			// Index into SQLDA, if appropriate
};

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

// CVC: Enumeration used for the COMMENT command.
enum
{
	ddl_database, ddl_domain, ddl_relation, ddl_view, ddl_procedure, ddl_trigger,
	ddl_udf, ddl_blob_filter, ddl_exception, ddl_generator, ddl_index, ddl_role,
	ddl_charset, ddl_collation//, ddl_sec_class
};

} // namespace

// macros for error generation

#ifdef DSQL_DEBUG
	extern unsigned DSQL_debug;
#endif

#endif // DSQL_DSQL_H

