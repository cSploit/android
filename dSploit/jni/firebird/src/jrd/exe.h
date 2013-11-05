/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe.h
 *	DESCRIPTION:	Execution block definitions
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
 * 2001.07.28: Added rse_skip to class RecordSelExpr to support LIMIT.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * Adriano dos Santos Fernandes
 */

#ifndef JRD_EXE_H
#define JRD_EXE_H

#include "../jrd/blb.h"
#include "../jrd/Relation.h"
#include "../common/classes/array.h"
#include "../common/classes/MetaName.h"

#include "gen/iberror.h"

#include "../jrd/dsc.h"
#include "../jrd/rse.h"

#include "../jrd/err_proto.h"
#include "../jrd/scl.h"
#include "../jrd/sbm.h"

#include "../jrd/DebugInterface.h"
#include "../jrd/BlrReader.h"

// This macro enables DSQL tracing code
//#define CMP_DEBUG

#ifdef CMP_DEBUG
DEFINE_TRACE_ROUTINE(cmp_trace);
#define CMP_TRACE(args) cmp_trace args
#else
#define CMP_TRACE(args) // nothing
#endif

class VaryingString;
struct dsc;

namespace Jrd {

#define NODE(type, name, keyword) type,

enum nod_t {
#include "../jrd/nod.h"
	nod_MAX
#undef NODE
};

class jrd_rel;
class jrd_nod;
struct sort_key_def;
template <typename T> class vec;
class jrd_prc;
class Collation;
struct index_desc;
struct IndexDescAlloc;
class Format;

// NOTE: The definition of structures RecordSelExpr and lit must be defined in
//       exactly the same way as structure jrd_nod through item nod_count.
//       Now, inheritance takes care of those common data members.
class jrd_node_base : public pool_alloc_rpt<jrd_nod*, type_nod>
{
public:
	jrd_nod*	nod_parent;
	SLONG		nod_impure;			// Inpure offset from request block
	nod_t		nod_type;			// Type of node
	USHORT		nod_flags;
	SCHAR		nod_scale;			// Target scale factor
	USHORT		nod_count;			// Number of arguments
};


class jrd_nod : public jrd_node_base
{
public:
/*	jrd_nod()
	:	nod_parent(0),
		nod_impure(0),
		nod_type(nod_nop),
		nod_flags(0),
		nod_scale(0),
		nod_count(0)
	{
		nod_arg[0] = 0;
	}*/
	jrd_nod*	nod_arg[1];
};

const int nod_comparison	= 1;
const int nod_id			= 1;		// marks a field node as a blr_fid guy
const int nod_quad			= 2;		// compute in quad (default is long)
const int nod_double		= 4;
const int nod_date			= 8;
const int nod_value			= 16;		// full value area required in impure space
const int nod_deoptimize	= 32;		// boolean which requires deoptimization
const int nod_agg_dbkey		= 64;		// dbkey of an aggregate
const int nod_invariant		= 128;		// node is recognized as being invariant
const int nod_recurse		= 256;		// union node is a recursive union
const int nod_unique_sort	= 512;		// sorts using unique key - for distinct and group by

// Special RecordSelExpr node

class RecordSelExpr : public jrd_node_base
{
public:
	USHORT		rse_count;
	USHORT		rse_jointype;		// inner, left, full
	bool		rse_writelock;
#ifdef SCROLLABLE_CURSORS
	RecordSource*	rse_rsb;
#endif
	jrd_nod*	rse_first;
	jrd_nod*	rse_skip;
	jrd_nod*	rse_boolean;
	jrd_nod*	rse_sorted;
	jrd_nod*	rse_projection;
	jrd_nod*	rse_aggregate;	// singleton aggregate for optimizing to index
	jrd_nod*	rse_plan;		// user-specified access plan
	VarInvariantArray *rse_invariants; // Invariant nodes bound to top-level RSE
#ifdef SCROLLABLE_CURSORS
	jrd_nod*	rse_async_message;	// asynchronous message to send for scrolling
#endif
	jrd_nod*	rse_relation[1];
};


// First one is obsolete: was used for PC_ENGINE
//const int rse_stream	= 1;	// flags RecordSelExpr-type node as a blr_stream type
const int rse_singular	= 2;	// flags RecordSelExpr-type node as from a singleton select
const int rse_variant	= 4;	// flags RecordSelExpr as variant (not invariant?)

// Number of nodes may fit into nod_arg of normal node to get to rse_relation
const size_t rse_delta = (sizeof(RecordSelExpr) - sizeof(jrd_nod)) / sizeof(jrd_nod::blk_repeat_type);

// Types of nulls placement for each column in sort order
const int rse_nulls_default	= 0;
const int rse_nulls_first	= 1;
const int rse_nulls_last	= 2;


// Literal value

class Literal : public jrd_node_base
{
public:
	dsc		lit_desc;
	SINT64	lit_data[1]; // Defined this way to prevent SIGBUS error in 64-bit ports
};

const size_t lit_delta	= ((sizeof(Literal) - sizeof(jrd_nod) - sizeof(SINT64)) / sizeof(jrd_nod**));


// Aggregate Sort Block (for DISTINCT aggregates)

class AggregateSort : public pool_alloc<type_asb>
{
public:
	jrd_nod*	nod_parent;
	SLONG	nod_impure;			// Impure offset from request block
	nod_t	nod_type;			// Type of node
	UCHAR	nod_flags;
	SCHAR	nod_scale;
	USHORT	nod_count;
	dsc		asb_desc;
	USHORT	asb_length;
	bool	asb_intl;
	sort_key_def* asb_key_desc;	// for the aggregate
	UCHAR	asb_key_data[1];
};

const size_t asb_delta	= ((sizeof(AggregateSort) - sizeof(jrd_nod)) / sizeof (jrd_nod**));


// Various structures in the impure area

struct impure_state
{
	SSHORT sta_state;
};

struct impure_value
{
	dsc vlu_desc;
	USHORT vlu_flags; // Computed/invariant flags
	VaryingString* vlu_string;
	union
	{
		UCHAR vlu_uchar;
		SSHORT vlu_short;
		SLONG vlu_long;
		SINT64 vlu_int64;
		SQUAD vlu_quad;
		SLONG vlu_dbkey[2];
		float vlu_float;
		double vlu_double;
		GDS_TIMESTAMP vlu_timestamp;
		GDS_TIME vlu_sql_time;
		GDS_DATE vlu_sql_date;
		bid vlu_bid;
		void* vlu_invariant; // Pre-compiled invariant object for nod_like and other string functions
	} vlu_misc;

	void make_long(const SLONG val, const signed char scale = 0);
	void make_int64(const SINT64 val, const signed char scale = 0);

};

// Do not use these methods where dsc_sub_type is not explicitly set to zero.
inline void impure_value::make_long(const SLONG val, const signed char scale)
{
	this->vlu_misc.vlu_long = val;
	this->vlu_desc.dsc_dtype = dtype_long;
	this->vlu_desc.dsc_length = sizeof(SLONG);
	this->vlu_desc.dsc_scale = scale;
	this->vlu_desc.dsc_sub_type = 0;
	this->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&this->vlu_misc.vlu_long);
}

inline void impure_value::make_int64(const SINT64 val, const signed char scale)
{
	this->vlu_misc.vlu_int64 = val;
	this->vlu_desc.dsc_dtype = dtype_int64;
	this->vlu_desc.dsc_length = sizeof(SINT64);
	this->vlu_desc.dsc_scale = scale;
	this->vlu_desc.dsc_sub_type = 0;
	this->vlu_desc.dsc_address = reinterpret_cast<UCHAR*>(&this->vlu_misc.vlu_int64);
}


struct impure_value_ex : public impure_value
{
	SLONG vlux_count;
	blb* vlu_blob;
};


const int VLU_computed	= 1;	// An invariant sub-query has been computed
const int VLU_null		= 2;	// An invariant sub-query computed to null
const int VLU_checked	= 4;	// Constraint already checked in first read or assignment to argument/variable

// Inversion (i.e. nod_index) impure area

struct impure_inversion
{
	RecordBitmap* inv_bitmap;
};


// AggregateSort impure area

struct impure_agg_sort
{
	sort_context* iasb_sort_handle;
};


// Various field positions

const int e_for_re			= 0;
const int e_for_statement	= 1;
const int e_for_stall		= 2;
const int e_for_rsb			= 3;
const int e_for_length		= 4;

const int e_arg_flag		= 0;
const int e_arg_indicator	= 1;
const int e_arg_message		= 2;
const int e_arg_number		= 3;
const int e_arg_info		= 4;
const int e_arg_length		= 5;

const int e_msg_number			= 0;
const int e_msg_format			= 1;
const int e_msg_next			= 2;
const int e_msg_impure_flags	= 3;
const int e_msg_length			= 4;

const int e_fld_stream		= 0;
const int e_fld_id			= 1;
const int e_fld_format		= 2;		// relation or procedure latest format when compiling
const int e_fld_default_value	= 3;	// hold column default value info if any, (Literal*)
const int e_fld_length		= 4;

const int e_sto_statement	= 0;
const int e_sto_statement2	= 1;
const int e_sto_sub_store	= 2;
const int e_sto_validate	= 3;
const int e_sto_relation	= 4;
const int e_sto_length		= 5;

const int e_erase_statement	= 0;
const int e_erase_sub_erase	= 1;
const int e_erase_stream	= 2;
const int e_erase_rsb		= 3;
const int e_erase_length	= 4;

const int e_sav_operation	= 0;
const int e_sav_name		= 1;
const int e_sav_length		= 2;

const int e_mod_statement	= 0;
const int e_mod_statement2	= 1;
const int e_mod_sub_mod		= 2;
const int e_mod_validate	= 3;
const int e_mod_map_view	= 4;
const int e_mod_org_stream	= 5;
const int e_mod_new_stream	= 6;
const int e_mod_rsb			= 7;
const int e_mod_length		= 8;

const int e_send_statement	= 0;
const int e_send_message	= 1;
const int e_send_length		= 2;

const int e_asgn_from		= 0;
const int e_asgn_to			= 1;
const int e_asgn_missing	= 2;	// Value for comparison for missing
const int e_asgn_missing2	= 3;	// Value for substitute for missing
const int e_asgn_length		= 4;

const int e_rel_stream		= 0;
const int e_rel_relation	= 1;
const int e_rel_view		= 2;	// parent view for posting access
const int e_rel_alias		= 3;	// SQL alias for the relation
const int e_rel_context		= 4;	// user-specified context number for the relation reference
const int e_rel_length		= 5;

const int e_idx_retrieval	= 0;
const int e_idx_length		= 1;

const int e_lbl_statement	= 0;
const int e_lbl_label		= 1;
const int e_lbl_length		= 2;

const int e_any_rse			= 0;
const int e_any_rsb			= 1;
const int e_any_length		= 2;

const int e_if_boolean		= 0;
const int e_if_true			= 1;
const int e_if_false		= 2;
const int e_if_length		= 3;

const int e_val_boolean		= 0;
const int e_val_value		= 1;
const int e_val_length		= 2;

const int e_uni_stream		= 0;	// Stream for union
const int e_uni_clauses		= 1;	// RecordSelExpr's for union
const int e_uni_map_stream	= 2;	// stream for next level record of recursive union
const int e_uni_length		= 3;

const int e_agg_stream		= 0;
const int e_agg_rse			= 1;
const int e_agg_group		= 2;
const int e_agg_map			= 3;
const int e_agg_length		= 4;

// Statistical expressions

const int e_stat_rse		= 0;
const int e_stat_value		= 1;
const int e_stat_default	= 2;
const int e_stat_rsb		= 3;
const int e_stat_length		= 4;

// Execute stored procedure

const int e_esp_inputs		= 0;
const int e_esp_in_msg		= 1;
const int e_esp_outputs		= 2;
const int e_esp_out_msg		= 3;
const int e_esp_procedure	= 4;
const int e_esp_length		= 5;

// Stored procedure view

const int e_prc_inputs		= 0;
const int e_prc_in_msg		= 1;
const int e_prc_stream		= 2;
const int e_prc_procedure	= 3;
const int e_prc_view		= 4;
const int e_prc_context		= 5;
const int e_prc_length		= 6;

// Function expression

const int e_fun_args		= 0;
const int e_fun_function	= 1;
const int e_fun_length		= 2;

// Generate id

const int e_gen_value		= 0;
const int e_gen_id			= 1;
const int e_gen_length		= 2;

// Protection mask

const int e_pro_class		= 0;
const int e_pro_relation	= 1;
const int e_pro_length		= 2;

// Exception

const int e_xcp_desc		= 0;
const int e_xcp_msg			= 1;
const int e_xcp_length		= 2;

// Variable declaration

const int e_var_id			= 0;
const int e_var_variable	= 1;
const int e_var_info		= 2;
const int e_var_length		= 3;

const int e_dcl_id			= 0;
const int e_dcl_desc		= 1;
const int e_dcl_length		= (1 + sizeof (DSC) / sizeof(::Jrd::jrd_nod*));	// Room for descriptor

const int e_dep_object		= 0;	// node for registering dependencies
const int e_dep_object_type	= 1;
const int e_dep_field		= 2;
const int e_dep_length		= 3;

const int e_scl_field		= 0;	// Scalar expression (blr_index)
const int e_scl_subscripts	= 1;
const int e_scl_length		= 2;

const int e_blk_action		= 0;
const int e_blk_handlers	= 1;
const int e_blk_length		= 2;

const int e_err_action		= 0;
const int e_err_conditions	= 1;
const int e_err_length		= 2;

// Datatype cast operator

const int e_cast_source		= 0;
const int e_cast_fmt		= 1;
const int e_cast_iteminfo	= 2;
const int e_cast_length		= 3;


// CVC: These belong to SCROLLABLE_CURSORS, but I can't mark them with the macro
// because e_seek_length is used in blrtable.h.
const int e_seek_offset		= 0;	// for seeking through a stream
const int e_seek_direction	= 1;
const int e_seek_rse		= 2;
const int e_seek_length		= 3;


// This is for the plan node
const int e_retrieve_relation		= 0;
const int e_retrieve_access_type	= 1;
const int e_retrieve_length			= 2;

// This is for the plan's access_type subnode
const int e_access_type_relation	= 0;
const int e_access_type_index		= 1;
const int e_access_type_index_name	= 2;
const int e_access_type_length		= 3;

// SQL Date supporting nodes
const int e_extract_value	= 0;	// Node
const int e_extract_part	= 1;	// Integer
const int e_extract_count	= 1;	// Number of nodes
const int e_extract_length	= 2;	// Number of entries in nod_args

const int e_current_date_length		= 1;
const int e_current_time_length		= 1;
const int e_current_timestamp_length= 1;

const int e_dcl_cursor_rse		= 0;
const int e_dcl_cursor_refs		= 1;
const int e_dcl_cursor_number	= 2;
const int e_dcl_cursor_rsb		= 3;
const int e_dcl_cursor_length	= 4;

const int e_cursor_stmt_op		= 0;
const int e_cursor_stmt_number	= 1;
const int e_cursor_stmt_seek	= 2;
const int e_cursor_stmt_into	= 3;
const int e_cursor_stmt_length	= 4;

const int e_strlen_value	= 0;
const int e_strlen_type		= 1;
const int e_strlen_count	= 1;
const int e_strlen_length	= 2;

const int e_trim_value			= 0;
const int e_trim_characters		= 1;
const int e_trim_specification	= 2;
const int e_trim_count			= 2;
const int e_trim_length			= 3;

// nod_src_info
const int e_src_info_line		= 0;
const int e_src_info_col		= 1;
const int e_src_info_node		= 2;
const int e_src_info_length		= 3;

// nod_init_variable
const int e_init_var_id			= 0;
const int e_init_var_variable	= 1;
const int e_init_var_info		= 2;
const int e_init_var_length		= 3;

// nod_domain_validation
const int e_domval_desc			= 0;
const int e_domval_length		= sizeof (DSC) / sizeof(::Jrd::jrd_nod*);	// Room for descriptor

// System function expression
const int e_sysfun_args		= 0;
const int e_sysfun_function	= 1;
const int e_sysfun_count	= 1;
const int e_sysfun_length	= 2;

// nod_exec_stmt
const int e_exec_stmt_stmt_sql		= 0;
const int e_exec_stmt_data_src		= 1;
const int e_exec_stmt_user			= 2;
const int e_exec_stmt_password		= 3;
const int e_exec_stmt_role			= 4;
const int e_exec_stmt_proc_block	= 5;
const int e_exec_stmt_fixed_count	= 6;

const int e_exec_stmt_extra_inputs		= 0;
const int e_exec_stmt_extra_input_names	= 1;
const int e_exec_stmt_extra_outputs		= 2;
const int e_exec_stmt_extra_tran		= 3;
const int e_exec_stmt_extra_privs		= 4;
const int e_exec_stmt_extra_count		= 5;

// nod_stmt_expr
const int e_stmt_expr_stmt		= 0;
const int e_stmt_expr_expr		= 1;
const int e_stmt_expr_length	= 2;

// nod_derived_expr
const int e_derived_expr_expr			= 0;
const int e_derived_expr_stream_count	= 1;
const int e_derived_expr_stream_list	= 2;
const int e_derived_expr_count			= 1;
const int e_derived_expr_length			= 3;

// Request resources

struct Resource
{
	enum rsc_s
	{
		rsc_relation,
		rsc_procedure,
		rsc_index,
		rsc_collation
	};

	enum rsc_s	rsc_type;
	USHORT		rsc_id;		// Id of the resource
	jrd_rel*	rsc_rel;	// Relation block
	jrd_prc*	rsc_prc;	// Procedure block
	Collation*	rsc_coll;	// Collation block

	static bool greaterThan(const Resource& i1, const Resource& i2)
	{
		// A few places of the engine depend on fact that rsc_type
		// is the first field in ResourceList ordering
		if (i1.rsc_type != i2.rsc_type)
			return i1.rsc_type > i2.rsc_type;
		if (i1.rsc_type == rsc_index) {
			// Sort by relation ID for now
			if (i1.rsc_rel->rel_id != i2.rsc_rel->rel_id)
				return i1.rsc_rel->rel_id > i2.rsc_rel->rel_id;
		}
		return i1.rsc_id > i2.rsc_id;
	}

	Resource(rsc_s type, USHORT id, jrd_rel* rel, jrd_prc* prc, Collation* coll)
		: rsc_type(type), rsc_id(id), rsc_rel(rel), rsc_prc(prc), rsc_coll(coll)
	{ }
};

typedef Firebird::SortedArray<Resource, Firebird::EmptyStorage<Resource>,
	Resource, Firebird::DefaultKeyValue<Resource>, Resource> ResourceList;

// Access items
// In case we start to use MetaName with required pool parameter,
// access item to be reworked!

struct AccessItem
{
	Firebird::MetaName		acc_security_name;
	SLONG					acc_view_id;
	Firebird::MetaName		acc_name, acc_r_name;
	const TEXT*				acc_type;
	SecurityClass::flags_t	acc_mask;

	static bool greaterThan(const AccessItem& i1, const AccessItem& i2)
	{
		int v;

		// Relations and procedures should be sorted before
		// columns, hence such a tricky inverted condition
		if ((v = -strcmp(i1.acc_type, i2.acc_type)) != 0)
			return v > 0;

		if ((v = i1.acc_security_name.compare(i2.acc_security_name)) != 0)
			return v > 0;

		if (i1.acc_view_id != i2.acc_view_id)
			return i1.acc_view_id > i2.acc_view_id;

		if (i1.acc_mask != i2.acc_mask)
			return i1.acc_mask > i2.acc_mask;

		if ((v = i1.acc_name.compare(i2.acc_name)) != 0)
			return v > 0;

		if ((v = i1.acc_r_name.compare(i2.acc_r_name)) != 0)
			return v > 0;

		return false; // Equal
	}

	AccessItem(const Firebird::MetaName& security_name, SLONG view_id,
		const Firebird::MetaName& name, const TEXT* type,
		SecurityClass::flags_t mask, const Firebird::MetaName& relName)
		: acc_security_name(security_name), acc_view_id(view_id), acc_name(name),
			acc_r_name(relName), acc_type(type), acc_mask(mask)
	{}
};

typedef Firebird::SortedArray<AccessItem, Firebird::EmptyStorage<AccessItem>,
	AccessItem, Firebird::DefaultKeyValue<AccessItem>, AccessItem> AccessItemList;

// Triggers and procedures the request accesses
struct ExternalAccess
{
	enum exa_act
	{
		exa_procedure,
		exa_insert,
		exa_update,
		exa_delete
	};
	exa_act exa_action;
	USHORT exa_prc_id;
	USHORT exa_rel_id;
	USHORT exa_view_id;

	// Procedure
	ExternalAccess(USHORT prc_id) :
		exa_action(exa_procedure), exa_prc_id(prc_id), exa_rel_id(0), exa_view_id(0)
	{ }

	// Trigger
	ExternalAccess(exa_act action, USHORT rel_id, USHORT view_id) :
		exa_action(action), exa_prc_id(0), exa_rel_id(rel_id), exa_view_id(view_id)
	{ }

	static bool greaterThan(const ExternalAccess& i1, const ExternalAccess& i2)
	{
		if (i1.exa_action != i2.exa_action)
			return i1.exa_action > i2.exa_action;
		if (i1.exa_prc_id != i2.exa_prc_id)
			return i1.exa_prc_id > i2.exa_prc_id;
		if (i1.exa_rel_id != i2.exa_rel_id)
			return i1.exa_rel_id > i2.exa_rel_id;
		if (i1.exa_view_id != i2.exa_view_id)
			return i1.exa_view_id > i2.exa_view_id;
		return false; // Equal
	}
};

typedef Firebird::SortedArray<ExternalAccess, Firebird::EmptyStorage<ExternalAccess>,
	ExternalAccess, Firebird::DefaultKeyValue<ExternalAccess>, ExternalAccess> ExternalAccessList;

// The three structs below are used for domains DEFAULT and constraints in PSQL
struct Item
{
	Item(nod_t aType, UCHAR aSubType, USHORT aIndex)
		: type(aType),
		  subType(aSubType),
		  index(aIndex)
	{
	}

	Item(nod_t aType, USHORT aIndex = 0)
		: type(aType),
		  subType(0),
		  index(aIndex)
	{
	}

	nod_t type;
	UCHAR subType;
	USHORT index;

	bool operator >(const Item& x) const
	{
		if (type == x.type)
		{
			if (subType == x.subType)
				return index > x.index;

			return subType > x.subType;
		}

		return type > x.type;
	}
};

struct FieldInfo
{
	FieldInfo()
		: nullable(false), defaultValue(0), validation(0)
	{}

	bool nullable;
	jrd_nod* defaultValue;
	jrd_nod* validation;
};

struct ItemInfo
{
	ItemInfo(MemoryPool& p, const ItemInfo& o)
		: name(p, o.name),
		  field(p, o.field),
		  nullable(o.nullable),
		  explicitCollation(o.explicitCollation),
		  fullDomain(o.fullDomain)
	{
	}

	explicit ItemInfo(MemoryPool& p)
		: name(p),
		  field(p),
		  nullable(true),
		  explicitCollation(false),
		  fullDomain(false)
	{
	}

	ItemInfo()
		: name(),
		  field(),
		  nullable(true),
		  explicitCollation(false),
		  fullDomain(false)
	{
	}

	bool isSpecial() const
	{
		return !nullable || fullDomain;
	}

	Firebird::MetaName name;
	Firebird::MetaNamePair field;
	bool nullable;
	bool explicitCollation;
	bool fullDomain;
};

typedef Firebird::GenericMap<Firebird::Pair<Firebird::Left<Firebird::MetaNamePair, FieldInfo> > >
	MapFieldInfo;
typedef Firebird::GenericMap<Firebird::Pair<Firebird::Right<Item, ItemInfo> > > MapItemInfo;

// Compile scratch block

/*
 * TMN: I had to move the enclosed csb_repeat outside this class,
 * since it's part of the C API. Compiling as C++ would enclose it.
 */
// CVC: Mike comment seems to apply only when the conversion to C++
// was being done. It's almost impossible that a repeating structure of
// the compiler scratch block be available to outsiders.

class CompilerScratch : public pool_alloc<type_csb>
{
	CompilerScratch(MemoryPool& p, size_t len, const Firebird::MetaName& domain_validation)
	:	/*csb_node(0),
		csb_variables(0),
		csb_dependencies(0),
#ifdef SCROLLABLE_CURSORS
		csb_current_rse(0),
		csb_async_message(0),
#endif
		csb_count(0),
		csb_n_stream(0),
		csb_msg_number(0),
		csb_impure(0),
		csb_g_flags(0),*/
		csb_external(p),
		csb_access(p),
		csb_resources(p),
		csb_dependencies(p),
		csb_fors(p),
		csb_exec_sta(p),
		csb_invariants(p),
		csb_current_nodes(p),
		csb_pool(p),
		csb_dbg_info(p),
		csb_map_field_info(p),
		csb_map_item_info(p),
		csb_domain_validation(domain_validation),
		csb_rpt(p, len)
	{}

public:
	static CompilerScratch* newCsb(MemoryPool& p, size_t len, const Firebird::MetaName& domain_validation = Firebird::MetaName())
	{
		return FB_NEW(p) CompilerScratch(p, len, domain_validation);
	}

	int nextStream(bool check = true)
	{
		if (csb_n_stream >= MAX_STREAMS && check)
		{
			ERR_post(Firebird::Arg::Gds(isc_too_many_contexts));
		}
		return csb_n_stream++;
	}

	BlrReader		csb_blr_reader;
	jrd_nod*		csb_node;
	ExternalAccessList csb_external;			// Access to outside procedures/triggers to be checked
	AccessItemList	csb_access;					// Access items to be checked
	vec<jrd_nod*>*	csb_variables;				// Vector of variables, if any
	ResourceList	csb_resources;				// Resources (relations and indexes)
	NodeStack		csb_dependencies;			// objects this request depends upon
	Firebird::Array<RecordSource*> csb_fors;	// stack of fors
	Firebird::Array<jrd_nod*> csb_exec_sta;		// Array of exec_into nodes
	Firebird::Array<jrd_nod*> csb_invariants;	// stack of invariant nodes
	Firebird::Array<jrd_node_base*> csb_current_nodes;	// RecordSelExpr's and other invariant
												// candidates within whose scope we are
#ifdef SCROLLABLE_CURSORS
	RecordSelExpr*	csb_current_rse;			// this holds the RecordSelExpr currently being processed;
												// unlike the current_rses stack, it references any
												// expanded view RecordSelExpr
	jrd_nod*		csb_async_message;			// asynchronous message to send to request
#endif
	USHORT			csb_n_stream;				// Next available stream
	USHORT			csb_msg_number;				// Highest used message number
	SLONG			csb_impure;					// Next offset into impure area
	USHORT			csb_g_flags;
	MemoryPool&		csb_pool;					// Memory pool to be used by csb
	Firebird::DbgInfo	csb_dbg_info;			// Debug information
	MapFieldInfo		csb_map_field_info;		// Map field name to field info
	MapItemInfo			csb_map_item_info;		// Map item to item info
	Firebird::MetaName	csb_domain_validation;	// Parsing domain constraint in PSQL

	// used in cmp.cpp/pass1
	jrd_rel*	csb_view;
	USHORT		csb_view_stream;
	bool		csb_validate_expr;
	USHORT		csb_remap_variable;

	struct csb_repeat
	{
		// We must zero-initialize this one
		csb_repeat()
		:	csb_stream(0),
			csb_view_stream(0),
			csb_flags(0),
			csb_indices(0),
			csb_relation(0),
			csb_alias(0),
			csb_procedure(0),
			csb_view(0),
			csb_idx(0),
			csb_message(0),
			csb_format(0),
			csb_fields(0),
			csb_cardinality(0.0f),	// TMN: Non-natural cardinality?!
			csb_plan(0),
			csb_map(0),
			csb_rsb_ptr(0)
		{}

		UCHAR csb_stream;				// Map user context to internal stream
		UCHAR csb_view_stream;			// stream number for view relation, below
		USHORT csb_flags;
		USHORT csb_indices;				// Number of indices

		jrd_rel* csb_relation;
		Firebird::string* csb_alias;	// SQL alias name for this instance of relation
		jrd_prc* csb_procedure;
		jrd_rel* csb_view;				// parent view

		IndexDescAlloc* csb_idx;		// Packed description of indices
		jrd_nod* csb_message;			// Msg for send/receive
		Format* csb_format;				// Default Format for stream
		UInt32Bitmap* csb_fields;		// Fields referenced
		double csb_cardinality;			// Cardinality of relation
		jrd_nod* csb_plan;				// user-specified plan for this relation
		UCHAR* csb_map;					// Stream map for views
		RecordSource** csb_rsb_ptr;		// point to rsb for nod_stream
	};


	typedef csb_repeat* rpt_itr;
	typedef const csb_repeat* rpt_const_itr;
	Firebird::HalfStaticArray<csb_repeat, 5> csb_rpt;
};

const int csb_internal			= 1;	// "csb_g_flag" switch
const int csb_get_dependencies	= 2;	// we are retrieving dependencies
const int csb_ignore_perm		= 4;	// ignore permissions checks
const int csb_blr_version4		= 8;	// the BLR is of version 4
const int csb_pre_trigger		= 16;	// this is a BEFORE trigger
const int csb_post_trigger		= 32;	// this is an AFTER trigger
const int csb_validation		= 64;	// we're in a validation expression (RDB hack)
const int csb_reuse_context		= 128;	// allow context reusage

const int csb_active		= 1;		// stream is active
const int csb_used			= 2;		// context has already been defined (BLR parsing only)
const int csb_view_update	= 4;		// view update w/wo trigger is in progress
const int csb_trigger		= 8;		// NEW or OLD context in trigger
const int csb_no_dbkey		= 16;		// stream doesn't have a dbkey
const int csb_store			= 32;		// we are processing a store statement
const int csb_modify		= 64;		// we are processing a modify
const int csb_sub_stream	= 128;		// a sub-stream of the RSE being processed
const int csb_erase			= 256;		// we are processing an erase
const int csb_unmatched		= 512;		// stream has conjuncts unmatched by any index
const int csb_update		= 1024;		// erase or modify for relation
const int csb_made_river	= 2048;		// stream has been included in a river

// Exception condition list

struct xcp_repeat
{
	SSHORT xcp_type;
	SLONG xcp_code;
};

class PsqlException : public pool_alloc_rpt<xcp_repeat, type_xcp>
{
public:
	SLONG xcp_count;
	xcp_repeat xcp_rpt[1];
};

const int xcp_sql_code	= 1;
const int xcp_gds_code	= 2;
const int xcp_xcp_code	= 3;
const int xcp_default	= 4;

class StatusXcp
{
	ISC_STATUS_ARRAY status;

public:
	StatusXcp();

	void clear();
	void init(const ISC_STATUS*);
	void copyTo(ISC_STATUS*) const;
	bool success() const;
	SLONG as_gdscode() const;
	SLONG as_sqlcode() const;
	void as_sqlstate(char*) const;
};

// must correspond to the size of RDB$EXCEPTIONS.RDB$MESSAGE
// minus size of vary::vary_length (USHORT) since RDB$MESSAGE
// declared as varchar
const int XCP_MESSAGE_LENGTH	= 1023 - sizeof(USHORT);

} //namespace Jrd

#endif // JRD_EXE_H
