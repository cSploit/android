/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		pass1.cpp
 *	DESCRIPTION:	First-pass compiler for statement trees.
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
 * 2001.5.26: Claudio Valderrama: field names should be skimmed from trailing
 *
 * 2001.5.26: Claudio Valderrama: COMPUTED fields will be skipped if a dummy
 *       "insert into tbl values(...)" sentence is issued.
 *
 * 2001.5.26: Claudio Valderrama: field names should be skimmed from trailing
 *		blanks to allow reliable comparisons in pass1_field. Same for table and
 *		and index names in plans.
 *
 * 2001.5.29: Claudio Valderrama: handle DROP VIEW case in pass1_statement().
 *
 * 2001.6.12: Claudio Valderrama: add basic BREAK capability to procedures.
 *
 * 2001.6.27: Claudio Valderrama: pass1_variable() now gives the name of the
 * variable it can't find in the error message.
 *
 * 2001.6.30: Claudio Valderrama: Enhanced again to provide (line, col), see node.h.
 *
 * 2001.7.28: John Bellardo: added code to handle nod_limit and associated fields.
 *
 * 2001.08.14 Claudio Valderrama: fixed crash with trigger and CURRENT OF <cursor> syntax.
 *
 * 2001.09.10 John Bellardo: fixed gen_rse to attribute skip/first nodes to the parent_rse
 *   if present instead of the child rse.  BUG #451798
 *
 * 2001.09.26 Claudio Valderrama: ambiguous field names are rejected from now.
 *
 * 2001.10.01 Claudio Valderrama: check constraints are allowed to have ambiguous field
 *   names because they use OLD and NEW as aliases of the same table. However, if the
 *   check constraint has an embedded ambiguous SELECT statement, it won't be detected.
 *   The code should be revisited if check constraints' before delete triggers are used
 *   for whatever reason. Currently they are never generated. The code can be improved
 *   to not report errors for fields between NEW and OLD contexts but complain otherwise.
 *
 * 2001.10.05 Neil McCalden: validate udf and parameters when comparing select list and
 *   group by list, to detect invalid SQL statements when grouping by UDFs.
 *
 * 2001.10.23 Ann Harrison:  allow reasonable checking of ambiguous names in unions.
 *   Remembering, later, that LLS_PUSH expects an object, not an LLS block.  Also
 *   stuck in the code for handling variables in pass1 - it apparently doesn't happen
 *   because the code returned an uninitialized pointer.
 *
 * 2001.11.17 Neil McCalden: Add aggregate_in_list procedure to handle cases
 *   where select statement has aggregate as a parameter to a udf which does
 *   not have to be in a group by clause.
 *
 * 2001.11.21 Claudio Valderrama: don't try to detect ambiguity in pass1_field()
 *   if the field or output procedure parameter has been fully qualified!!!
 *
 * 2001.11.27 Ann Harrison:  Redo the amiguity checking so as to give better
 *   error messages, return warnings for dialect 1, and simplify.
 *
 * 2001.11.28 Claudio Valderrama: allow udf arguments to be query parameters.
 *   Honor the code in the parser that already accepts those parameters.
 *   This closes SF Bug# 409769.
 *
 * 2001.11.29 Claudio Valderrama: make the nice new ambiguity checking code do the
 *   right thing instead of crashing the engine and restore fix from 2001.11.21.
 *
 * 2001.12.21 Claudio Valderrama: Fix SF Bug #494832 - pass1_variable() should work
 *   with def_proc, mod_proc, redef_proc, def_trig and mod_trig node types.
 *
 * 2002.07.30 Arno Brinkman: Added pass1_coalesce, pass1_simple_case, pass1_searched_case
 *   and pass1_put_args_on_stack
 *
 * 2002.08.04 Arno Brinkman: Added ignore_cast as parameter to node_match,
 *   Changed invalid_reference procedure for allow EXTRACT, SUBSTRING, CASE,
 *   COALESCE and NULLIF functions in GROUP BY and as select_items.
 *   Removed aggregate_in_list procedure.
 *
 * 2002.08.07 Dmitry Yemanov: Disabled BREAK statement in triggers
 *
 * 2002.08.10 Dmitry Yemanov: ALTER VIEW
 *
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 *
 * 2002.09.29 Arno Brinkman: Adding more checking for aggregate functions
 *   and adding support for 'linking' from sub-selects to aggregate functions
 *   which are in an lower level.
 *   Modified functions pass1_field, pass1_rse, copy_field, pass1_sort.
 *   Functions pass1_found_aggregate and pass1_found_field added.
 *
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 *
 * 2002.10.25 Dmitry Yemanov: Re-allowed plans in triggers
 *
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 *
 * 2002.12.03 Dmitry Yemanov: Implemented ORDER BY clause in subqueries
 *
 * 2002.12.18 Dmitry Yemanov: Fixed bug with BREAK and partially implemented
 *							  SQL-compliant labels and LEAVE statement
 *
 * 2003.01.11 Arno Brinkman: Reworked a lot of functions for bringing back backwards compatibilty
 *							 with sub-selects and aggregates.
 *
 * 2003.01.14 Dmitry Yemanov: Fixed bug with cursors in triggers
 *
 * 2003.01.15 Dmitry Yemanov: Added support for parametrized events
 *
 * 2003.04.05 Dmitry Yemanov: Changed logic of ORDER BY with collations
 *							  (because of the parser change)
 *
 * 2003.08.14 Arno Brinkman: Added derived table support.
 *
 * 2003.08.16 Arno Brinkman: Changed ambiguous column name checking.
 *
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL.
 *
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 *
 * Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"
#include <string.h>
#include <stdio.h>
#include "../jrd/ibase.h"
#include "../dsql/dsql.h"
#include "../dsql/node.h"
#include "../dsql/Nodes.h"
#include "../jrd/intl.h"
#include "../jrd/blr.h"
#include "../jrd/jrd.h"
#include "../jrd/constants.h"
#include "../jrd/intl_classes.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/misc_func.h"
#include "../dsql/pass1_proto.h"
#include "../dsql/utld_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/why_proto.h"
#include "../jrd/SysFunction.h"
#include "../common/classes/array.h"
#include "../common/classes/auto.h"
#include "../common/utils_proto.h"
#include "../common/config/config.h"
#include "../common/StatusArg.h"

using namespace Jrd;
using namespace Dsql;
using namespace Firebird;

#ifdef DEV_BUILD
static void DSQL_pretty(const dsql_nod*, int);
#endif


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


static bool aggregate_found(const CompiledStatement*, const dsql_nod*);
static bool aggregate_found2(const CompiledStatement*, const dsql_nod*, USHORT*, USHORT*, bool);
static dsql_nod* ambiguity_check(CompiledStatement*, dsql_nod*, const dsql_str*,
	const DsqlContextStack&);
static void assign_fld_dtype_from_dsc(dsql_fld*, const dsc*);
static void check_unique_fields_names(StrArray& names, const dsql_nod* fields);
static dsql_nod* compose(dsql_nod*, dsql_nod*, NOD_TYPE);
static dsql_nod* explode_fields(dsql_rel*);
static dsql_nod* explode_outputs(CompiledStatement*, const dsql_prc*);
static void field_appears_once(const dsql_nod*, const dsql_nod*, const bool, const char*);
static void field_duplication(const TEXT*, const TEXT*, const dsql_nod*, const char*);
static void field_unknown(const TEXT*, const TEXT*, const dsql_nod*);
static dsql_par* find_dbkey(const CompiledStatement*, const dsql_nod*);
static dsql_par* find_record_version(const CompiledStatement*, const dsql_nod*);
static dsql_ctx* get_context(const dsql_nod* node);
static void get_contexts(DsqlContextStack& contexts, const dsql_nod* node);
#ifdef NOT_USED_OR_REPLACED
static bool get_object_and_field(const dsql_nod* node,
	const char** obj_name, const char** fld_name, bool do_collation);
#endif
static bool invalid_reference(const dsql_ctx*, const dsql_nod*, const dsql_nod*, bool, bool);
static bool node_match(const dsql_nod*, const dsql_nod*, bool);
static dsql_nod* nullify_returning(CompiledStatement*, dsql_nod* input);
static dsql_nod* pass1_alias_list(CompiledStatement*, dsql_nod*);
static dsql_ctx* pass1_alias(CompiledStatement*, DsqlContextStack&, dsql_str*);
static dsql_str* pass1_alias_concat(const dsql_str*, const dsql_str*);
static dsql_nod* pass1_any(CompiledStatement*, dsql_nod*, NOD_TYPE);
static dsql_rel* pass1_base_table(CompiledStatement*, const dsql_rel*, const dsql_str*);
static void pass1_blob(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_coalesce(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_collate(CompiledStatement*, dsql_nod*, const dsql_str*);
static dsql_nod* pass1_constant(CompiledStatement*, dsql_nod*);
static dsql_ctx* pass1_cursor_context(CompiledStatement*, const dsql_nod*, const dsql_nod*);
static dsql_nod* pass1_cursor_name(CompiledStatement*, const dsql_str*, USHORT, bool);
static dsql_nod* pass1_cursor_reference(CompiledStatement*, const dsql_nod*, dsql_nod*);
static dsql_nod* pass1_dbkey(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_delete(CompiledStatement*, dsql_nod*);
static void pass1_expand_contexts(DsqlContextStack& contexts, dsql_ctx* context);
static dsql_nod* pass1_derived_table(CompiledStatement*, dsql_nod*, dsql_str*);
static dsql_nod* pass1_expand_select_list(CompiledStatement*, dsql_nod*, dsql_nod*);
static void pass1_expand_select_node(CompiledStatement*, dsql_nod*, DsqlNodStack&, bool);
static dsql_nod* pass1_field(CompiledStatement*, dsql_nod*, const bool, dsql_nod*);
static bool pass1_found_aggregate(const dsql_nod*, USHORT, USHORT, bool);
static bool pass1_found_field(const dsql_nod*, USHORT, USHORT, bool*);
static bool pass1_found_sub_select(const dsql_nod*);
static dsql_nod* pass1_group_by_list(CompiledStatement*, dsql_nod*, dsql_nod*);
static dsql_nod* pass1_hidden_variable(CompiledStatement* statement, dsql_nod*& expr);
static dsql_nod* pass1_insert(CompiledStatement*, dsql_nod*, bool);
static dsql_nod* pass1_join(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_label(CompiledStatement*, dsql_nod*);
static void pass1_limit(CompiledStatement*, dsql_nod*, dsql_nod*);
static dsql_nod* pass1_lookup_alias(CompiledStatement*, const dsql_str*, dsql_nod*, bool);
static dsql_nod* pass1_make_derived_field(CompiledStatement*, thread_db*, dsql_nod*);
static dsql_nod* pass1_merge(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_node_psql(CompiledStatement*, dsql_nod*, bool);
static dsql_nod* pass1_not(CompiledStatement*, const dsql_nod*, bool);
static void	pass1_put_args_on_stack(CompiledStatement*, dsql_nod*, DsqlNodStack&);
static dsql_nod* pass1_relation(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_returning(CompiledStatement*, const dsql_nod*);
static dsql_nod* pass1_rse(CompiledStatement*, dsql_nod*, dsql_nod*, dsql_nod*, dsql_nod*, USHORT);
static dsql_nod* pass1_rse_impl(CompiledStatement*, dsql_nod*, dsql_nod*, dsql_nod*, dsql_nod*, USHORT);
static dsql_nod* pass1_searched_case(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_sel_list(CompiledStatement*, dsql_nod*, bool);
static dsql_nod* pass1_simple_case(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_sort(CompiledStatement*, dsql_nod*, dsql_nod*);
static dsql_nod* pass1_sys_function(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_udf(CompiledStatement*, dsql_nod*);
static void pass1_udf_args(CompiledStatement*, dsql_nod*, dsql_udf*, USHORT&, DsqlNodStack&);
static dsql_nod* pass1_union(CompiledStatement*, dsql_nod*, dsql_nod*, dsql_nod*, USHORT);
static void pass1_union_auto_cast(dsql_nod*, const dsc&, SSHORT, bool in_select_list = false);
static dsql_nod* pass1_update(CompiledStatement*, dsql_nod*, bool);
static dsql_nod* pass1_update_or_insert(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_variable(CompiledStatement*, dsql_nod*);
static dsql_nod* post_map(dsql_nod*, dsql_ctx*);
static dsql_nod* remap_field(CompiledStatement*, dsql_nod*, dsql_ctx*, USHORT);
static dsql_nod* remap_fields(CompiledStatement*, dsql_nod*, dsql_ctx*);
static void remap_streams_to_parent_context(dsql_nod*, dsql_ctx*);
static dsql_fld* resolve_context(CompiledStatement*, const dsql_str*, dsql_ctx*, bool, bool);
static dsql_nod* resolve_using_field(CompiledStatement* statement, dsql_str* name, DsqlNodStack& stack,
	const dsql_nod* flawedNode, const TEXT* side, dsql_ctx*& ctx);
static dsql_nod* resolve_variable_name(const dsql_nod* var_nodes, const dsql_str* var_name);
static bool set_parameter_type(CompiledStatement*, dsql_nod*, dsql_nod*, bool);
static void set_parameters_name(dsql_nod*, const dsql_nod*);
static void set_parameter_name(dsql_nod*, const dsql_nod*, const dsql_rel*);
static dsql_nod* pass1_savepoint(const CompiledStatement*, dsql_nod*);

static bool pass1_relproc_is_recursive(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_join_is_recursive(CompiledStatement*, dsql_nod*&);
static dsql_nod* pass1_rse_is_recursive(CompiledStatement*, dsql_nod*);
static dsql_nod* pass1_recursive_cte(CompiledStatement*, dsql_nod*);
static dsql_nod* process_returning(CompiledStatement*, dsql_nod*);

// CVC: more global variables???
static const dsql_str* global_temp_collation_name = NULL;

const char* DB_KEY_STRING	= "DB_KEY"; // NTX: pseudo field name
const int MAX_MEMBER_LIST	= 1500;	// Maximum members in "IN" list.
									// For eg. SELECT * FROM T WHERE
									//         F IN (1, 2, 3, ...)
									//
									// Bug 10061, bsriram - 19-Apr-1999

const int LIKE_PARAM_LEN = 30;	// CVC: This is a guess for the length of the
								// parameter for LIKE and others, when the
								// original dtype isn't string and force_varchar
								// is true.

enum field_match_val {
	FIELD_MATCH_TYPE_EQUAL = 0,
	FIELD_MATCH_TYPE_LOWER = 1,
	FIELD_MATCH_TYPE_LOWER_EQUAL = 2,
	FIELD_MATCH_TYPE_HIGHER = 3,
	FIELD_MATCH_TYPE_HIGHER_EQUAL = 4
};


/**

 	PASS1_make_context

    @brief	Generate a context for a statement.


    @param statement
    @param relation_node

 **/
dsql_ctx* PASS1_make_context(CompiledStatement* statement, const dsql_nod* relation_node)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(relation_node, dsql_type_nod);

	thread_db* const tdbb = JRD_get_thread_data();

	dsql_rel* relation = NULL;
	dsql_prc* procedure = NULL;

	// figure out whether this is a relation or a procedure
	// and give an error if it is neither

	dsql_str* relation_name;
	switch (relation_node->nod_type)
	{
	case nod_rel_proc_name:
		relation_name = (dsql_str*) relation_node->nod_arg[e_rpn_name];
		break;
	case nod_derived_table:
		relation_name = (dsql_str*) relation_node->nod_arg[e_derived_table_alias];
		break;
	default:
		relation_name = (dsql_str*) relation_node->nod_arg[e_rln_name];
		break;
	}

	// CVC: Let's skim the context, too.
	if (relation_name && relation_name->str_data) {
		fb_utils::exact_name(relation_name->str_data);
	}

	DEV_BLKCHK(relation_name, dsql_type_str);

	const dsql_nod* cte = NULL;
	if (relation_node->nod_type == nod_derived_table) {
		// No processing needed here for derived tables.
	}
	else if ((relation_node->nod_type == nod_rel_proc_name) && relation_node->nod_arg[e_rpn_inputs])
	{
		procedure = METD_get_procedure(statement, relation_name);
		if (!procedure)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_dsql_procedure_err) <<
					  Arg::Gds(isc_random) << Arg::Str(relation_name->str_data) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relation_node->nod_line) <<
					  									   Arg::Num(relation_node->nod_column));
		}
	}
	else if ((cte = statement->findCTE(relation_name)))
	{
		relation_node = cte;
	}
	else
	{
		relation = METD_get_relation(statement, relation_name->str_data);
		if (!relation && (relation_node->nod_type == nod_rel_proc_name))
		{
			procedure = METD_get_procedure(statement, relation_name);
		}
		if (!relation && !procedure)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_dsql_relation_err) <<
					  Arg::Gds(isc_random) << Arg::Str(relation_name->str_data) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relation_node->nod_line) <<
					  									   Arg::Num(relation_node->nod_column));
		}
	}

	if (procedure && !procedure->prc_out_count) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-84) <<
				  Arg::Gds(isc_dsql_procedure_use_err) << Arg::Str(relation_name->str_data) <<
				  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relation_node->nod_line) <<
				  									   Arg::Num(relation_node->nod_column));
	}

	// Set up context block.
	dsql_ctx* context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
	context->ctx_relation = relation;
	context->ctx_procedure = procedure;
	context->ctx_request = statement;

	if (relation_node->nod_type == nod_derived_table)
		context->ctx_context = USHORT(MAX_UCHAR) + 1 + statement->req_derived_context_number++;
	else
		context->ctx_context = statement->req_context_number++;

	context->ctx_scope_level = statement->req_scope_level;
	// When we're in a outer-join part mark context for it.
	if (statement->req_in_outer_join) {
		context->ctx_flags |= CTX_outer_join;
	}
	context->ctx_in_outer_join = statement->req_in_outer_join;

	// find the context alias name, if it exists.
	dsql_str* string;
	if (relation_node->nod_type == nod_rel_proc_name) {
		string = (dsql_str*) relation_node->nod_arg[e_rpn_alias];
	}
	else if (relation_node->nod_type == nod_derived_table) {
		string = (dsql_str*) relation_node->nod_arg[e_derived_table_alias];
		context->ctx_rse = relation_node->nod_arg[e_derived_table_rse];
	}
	else {
		string = (dsql_str*) relation_node->nod_arg[e_rln_alias];
	}

	if (string)
	{
		context->ctx_internal_alias = string->str_data;
	}

	DEV_BLKCHK(string, dsql_type_str);
	if (statement->req_alias_relation_prefix && !(relation_node->nod_type == nod_derived_table))
	{
		if (string) {
			string = pass1_alias_concat(statement->req_alias_relation_prefix, string);
		}
		else {
			string = pass1_alias_concat(statement->req_alias_relation_prefix, relation_name);
		}
	}

	if (string)
	{
		context->ctx_alias = string->str_data;

		// check to make sure the context is not already used at this same
		// query level (if there are no subqueries, this checks that the
		// alias is not used twice in the statement).
		for (DsqlContextStack::iterator stack(*statement->req_context); stack.hasData(); ++stack)
		{
			const dsql_ctx* conflict = stack.object();

			if (conflict->ctx_scope_level != context->ctx_scope_level) {
				continue;
			}

			const TEXT* conflict_name;
			ISC_STATUS error_code;
			if (conflict->ctx_alias) {
				conflict_name = conflict->ctx_alias;
				error_code = isc_alias_conflict_err;
				// alias %s conflicts with an alias in the same statement.
			}
			else if (conflict->ctx_procedure) {
				conflict_name = conflict->ctx_procedure->prc_name.c_str();
				error_code = isc_procedure_conflict_error;
				// alias %s conflicts with a procedure in the same statement.
			}
			else if (conflict->ctx_relation) {
				conflict_name = conflict->ctx_relation->rel_name.c_str();
				error_code = isc_relation_conflict_err;
				// alias %s conflicts with a relation in the same statement.
			}
			else {
				continue;
			}

			if (!strcmp(conflict_name, context->ctx_alias)) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(error_code) << Arg::Str(conflict_name));
			}
		}
	}

	if (procedure)
	{
		USHORT count = 0;

		if (relation_node->nod_arg[e_rpn_inputs])
		{
			context->ctx_proc_inputs =
				pass1_node_psql(statement, relation_node->nod_arg[e_rpn_inputs], false);
			count = context->ctx_proc_inputs->nod_count;
		}

		if (!(statement->req_flags & REQ_procedure))
		{
			if (count > procedure->prc_in_count ||
				count < procedure->prc_in_count - procedure->prc_def_count)
			{
				ERRD_post(Arg::Gds(isc_prcmismat) << Arg::Str(relation_name->str_data));
			}

			if (count)
			{
				// Initialize this stack variable, and make it look like a node
				Firebird::AutoPtr<dsql_nod> desc_node(FB_NEW_RPT(*tdbb->getDefaultPool(), 0) dsql_nod);

				dsql_nod* const* input = context->ctx_proc_inputs->nod_arg;
				for (dsql_fld* field = procedure->prc_inputs; *input; input++, field = field->fld_next)
				{
					DEV_BLKCHK(field, dsql_type_fld);
					DEV_BLKCHK(*input, dsql_type_nod);
					MAKE_desc_from_field(&desc_node->nod_desc, field);
					//	set_parameter_type(statement, *input, &desc_node, false);
					set_parameter_type(statement, *input, desc_node, false);
				}
			}
		}
	}

	// push the context onto the statement context stack
	// for matching fields against

	statement->req_context->push(context);

	return context;
}


/**

 	PASS1_node

    @brief	Compile a parsed statement into something more interesting.


    @param statement
    @param input

 **/
dsql_nod* PASS1_node(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	if (input == NULL) {
		return NULL;
	}

	dsql_nod* node;
	dsql_fld* field;
	dsql_nod* sub1;

	// Dispatch on node type.  Fall thru on easy ones

	switch (input->nod_type)
	{
	case nod_alias:
		node = MAKE_node(input->nod_type, e_alias_count);
		node->nod_arg[e_alias_value] = sub1 = PASS1_node(statement, input->nod_arg[e_alias_value]);
		node->nod_arg[e_alias_alias] = input->nod_arg[e_alias_alias];
		node->nod_desc = sub1->nod_desc;
		return node;

	case nod_cast:
		node = MAKE_node(input->nod_type, e_cast_count);
		node->nod_arg[e_cast_source] = sub1 = PASS1_node(statement, input->nod_arg[e_cast_source]);
		node->nod_arg[e_cast_target] = input->nod_arg[e_cast_target];
		field = (dsql_fld*) node->nod_arg[e_cast_target];
		DEV_BLKCHK(field, dsql_type_fld);
		DDL_resolve_intl_type(statement, field, NULL);
		MAKE_desc_from_field(&node->nod_desc, field);
		set_parameter_type(statement, node, NULL, false);
		// If the source is nullable, so is the target
		MAKE_desc(statement, &sub1->nod_desc, sub1, NULL);
		if (sub1->nod_desc.dsc_flags & DSC_nullable)
			node->nod_desc.dsc_flags |= DSC_nullable;
		return node;

	case nod_coalesce:
		return pass1_coalesce(statement, input);

	case nod_derived_field:
		return input;

	case nod_simple_case:
		return pass1_simple_case(statement, input);

	case nod_searched_case:
		return pass1_searched_case(statement, input);

	case nod_gen_id:
	case nod_gen_id2:
		node = MAKE_node(input->nod_type, e_gen_id_count);
		node->nod_arg[e_gen_id_value] = PASS1_node(statement, input->nod_arg[e_gen_id_value]);
		node->nod_arg[e_gen_id_name] = input->nod_arg[e_gen_id_name];
		return node;

	case nod_collate:
		global_temp_collation_name = (dsql_str*) input->nod_arg[e_coll_target];
		sub1 = PASS1_node(statement, input->nod_arg[e_coll_source]);
		global_temp_collation_name = NULL;
		node = pass1_collate(statement, sub1, (dsql_str*) input->nod_arg[e_coll_target]);
		return node;

	case nod_extract:

		// Figure out the data type of the sub parameter, and make
		// sure the requested type of information can be extracted

		sub1 = PASS1_node(statement, input->nod_arg[e_extract_value]);
		MAKE_desc(statement, &sub1->nod_desc, sub1, NULL);

		switch (input->nod_arg[e_extract_part]->getSlong())
		{
		case blr_extract_year:
		case blr_extract_month:
		case blr_extract_day:
		case blr_extract_weekday:
		case blr_extract_yearday:
		case blr_extract_week:
			if (sub1->nod_type != nod_null &&
				sub1->nod_desc.dsc_dtype != dtype_sql_date &&
				sub1->nod_desc.dsc_dtype != dtype_timestamp)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-105) <<
						  Arg::Gds(isc_extract_input_mismatch));
			}
			break;
		case blr_extract_hour:
		case blr_extract_minute:
		case blr_extract_second:
		case blr_extract_millisecond:
			if (sub1->nod_type != nod_null &&
				sub1->nod_desc.dsc_dtype != dtype_sql_time &&
				sub1->nod_desc.dsc_dtype != dtype_timestamp)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-105) <<
						  Arg::Gds(isc_extract_input_mismatch));
			}
			break;
		default:
			fb_assert(false);
			break;
		}
		node = MAKE_node(input->nod_type, e_extract_count);
		node->nod_arg[e_extract_part] = input->nod_arg[e_extract_part];
		node->nod_arg[e_extract_value] = sub1;
		if (sub1->nod_desc.dsc_flags & DSC_nullable) {
			node->nod_desc.dsc_flags |= DSC_nullable;
		}
		return node;

	case nod_strlen:
		node = MAKE_node(input->nod_type, e_strlen_count);
		node->nod_arg[e_strlen_type] = input->nod_arg[e_strlen_type];
		node->nod_arg[e_strlen_value] = PASS1_node(statement, input->nod_arg[e_strlen_value]);
		if (node->nod_arg[e_strlen_value]->nod_desc.dsc_flags & DSC_nullable)
			node->nod_desc.dsc_flags |= DSC_nullable;
		return node;

	case nod_delete:
	case nod_insert:
	case nod_merge:
	case nod_update_or_insert:
	case nod_order:
	case nod_select:
	case nod_with:
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err));

	case nod_derived_table:
		return pass1_derived_table(statement, input, NULL);

	case nod_select_expr:
		{
			const DsqlContextStack::iterator base(*statement->req_context);
			node = MAKE_node(nod_via, e_via_count);
			dsql_nod* rse = PASS1_rse(statement, input, NULL);
			node->nod_arg[e_via_rse] = rse;
			node->nod_arg[e_via_value_1] = rse->nod_arg[e_rse_items]->nod_arg[0];
			node->nod_arg[e_via_value_2] = MAKE_node(nod_null, (int) 0);
			// Finish off by cleaning up contexts
			statement->req_context->clear(base);
			return node;
		}

	case nod_exists:
	case nod_singular:
		{
			const DsqlContextStack::iterator base(*statement->req_context);
			node = MAKE_node(input->nod_type, 1);
			input = input->nod_arg[0];
			node->nod_arg[0] = PASS1_rse(statement, input, NULL);

			// Finish off by cleaning up contexts

			statement->req_context->clear(base);
			return node;
		}

	case nod_field_name:
		if (statement->isPsql())
			return pass1_variable(statement, input);
		return pass1_field(statement, input, false, NULL);

	case nod_field:
		// AB: nod_field is an already passed node.
		// This could be done in expand_select_list.
		return input;

	case nod_array:
		if (statement->isPsql())
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_invalid_array));
		}
		else
			return pass1_field(statement, input, false, NULL);

	case nod_variable:
		node = MAKE_node (input->nod_type, e_var_count);
		node->nod_arg[0] = input->nod_arg[0];
		node->nod_desc = input->nod_desc;
		return node;

	case nod_var_name:
		return pass1_variable(statement, input);

	case nod_dbkey:
		return pass1_dbkey(statement, input);

	case nod_relation_name:
	case nod_rel_proc_name:
		{
			dsql_str* rel_name;
			dsql_str* rel_alias;

			if (input->nod_type == nod_rel_proc_name) {
				rel_name = (dsql_str*) input->nod_arg[e_rpn_name];
				rel_alias = (dsql_str*) input->nod_arg[e_rpn_alias];
			}
			else {	// nod_relation_name
				rel_name = (dsql_str*) input->nod_arg[e_rln_name];
				rel_alias = (dsql_str*) input->nod_arg[e_rln_alias];
			}

			if (!rel_alias) {
				rel_alias = rel_name;
			}

			dsql_nod* cte = statement->findCTE(rel_name);
			if (cte)
			{
				cte->nod_flags |= NOD_DT_CTE_USED;

				if ((statement->req_flags & REQ_CTE_recursive) &&
					 statement->req_curr_ctes.hasData() &&
					 (statement->req_curr_ctes.object() == cte))
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  // Recursive CTE member (%s) can refer itself only in FROM clause
							  Arg::Gds(isc_dsql_cte_wrong_reference) << Arg::Str(rel_name->str_data));
				}

				for (DsqlNodStack::const_iterator stack(statement->req_curr_ctes); stack.hasData();
					++stack)
				{
					dsql_nod* cte1 = stack.object();
					if (cte1 == cte) {
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
								  // CTE %s has cyclic dependencies
								  Arg::Gds(isc_dsql_cte_cycle) << Arg::Str(rel_name->str_data));
					}
				}

				dsql_nod* const select_expr = cte->nod_arg[e_derived_table_rse];
				dsql_nod* const query = select_expr->nod_arg[e_sel_query_spec];
				const bool isRecursive =
					(query->nod_type == nod_list) && (query->nod_flags & NOD_UNION_RECURSIVE);

				dsql_str* cte_name = (dsql_str*) cte->nod_arg[e_derived_table_alias];
				if (!isRecursive) {
					cte->nod_arg[e_derived_table_alias] = (dsql_nod*) rel_alias;
				}

				statement->req_curr_ctes.push(cte);

				dsql_nod* derived_node =
					pass1_derived_table(statement, cte, (isRecursive ? rel_alias : NULL));

				if (!isRecursive) {
					cte->nod_arg[e_derived_table_alias] = (dsql_nod*) cte_name;
				}
				statement->req_curr_ctes.pop();

				return derived_node;
			}

			return pass1_relation(statement, input);
		}

	case nod_constant:
		return pass1_constant(statement, input);

	case nod_parameter:
		if (statement->isPsql())
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err));
		}

		{ // scope
			node = MAKE_node(input->nod_type, e_par_count);
			node->nod_count = 0;
			dsql_msg* tempMsg = input->nod_arg[e_par_parameter] ?
						(dsql_msg*) ((dsql_par*) input->nod_arg[e_par_parameter])->par_message :
						statement->req_send;

			dsql_par* tempPar =
				MAKE_parameter(tempMsg, true, true, (USHORT)(IPTR) input->nod_arg[e_par_index], NULL);
			node->nod_arg[e_par_parameter] = (dsql_nod*) tempPar;
			node->nod_arg[e_par_index] = (dsql_nod*) (IPTR) tempPar->par_index;
		} // end scope
		return node;

	case nod_param_val:
		node = MAKE_node(input->nod_type, e_prm_val_count);
		node->nod_arg[e_prm_val_fld] = input->nod_arg[e_prm_val_fld];
		node->nod_arg[e_prm_val_val] = PASS1_node(statement, input->nod_arg[e_prm_val_val]);

		field = (dsql_fld*) node->nod_arg[e_prm_val_fld]->nod_arg[e_dfl_field];
		DDL_resolve_intl_type(statement, field, NULL);

		{ // scope
			dsql_nod *temp	= node->nod_arg[e_prm_val_val];
			// Initialize this stack variable, and make it look like a node
			Firebird::AutoPtr<dsql_nod> desc_node(FB_NEW_RPT(*getDefaultMemoryPool(), 0) dsql_nod);

			DEV_BLKCHK(field, dsql_type_fld);
			DEV_BLKCHK(temp, dsql_type_nod);

			field->fld_flags |= FLD_nullable;
			MAKE_desc_from_field(&(desc_node->nod_desc), field);
			set_parameter_type(statement, temp, desc_node, false);
		} // end scope

		return node;

	case nod_udf:
		return pass1_udf(statement, input);

	case nod_sys_function:
		return pass1_sys_function(statement, input);

	case nod_equiv:
	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_lss:
	case nod_leq:
	case nod_eql_any:
	case nod_neq_any:
	case nod_gtr_any:
	case nod_geq_any:
	case nod_lss_any:
	case nod_leq_any:
	case nod_eql_all:
	case nod_neq_all:
	case nod_gtr_all:
	case nod_geq_all:
	case nod_lss_all:
	case nod_leq_all:
		{
			dsql_nod* sub2 = input->nod_arg[1];

			if (sub2->nod_type == nod_list)
			{
				int list_item_count = 0;

				node = NULL;
				dsql_nod** ptr = sub2->nod_arg;
				for (const dsql_nod* const* const end = ptr + sub2->nod_count;
					 ptr < end; list_item_count++, ptr++)
				{
					if (list_item_count >= MAX_MEMBER_LIST)
					{
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
								  Arg::Gds(isc_imp_exc) <<
								  Arg::Gds(isc_dsql_too_many_values) << Arg::Num(MAX_MEMBER_LIST));
					}

					DEV_BLKCHK(*ptr, dsql_type_nod);
					dsql_nod* temp = MAKE_node(input->nod_type, 2);
					temp->nod_arg[0] = input->nod_arg[0];
					temp->nod_arg[1] = *ptr;
					node = compose(node, PASS1_node(statement, temp), nod_or);
				}

				return node;
			}

			if (sub2->nod_type == nod_select_expr)
			{
				if (sub2->nod_flags & NOD_SELECT_EXPR_SINGLETON)
				{
					const DsqlContextStack::iterator base(*statement->req_context);
					node = MAKE_node(input->nod_type, 2);
					node->nod_arg[0] = pass1_node_psql(statement, input->nod_arg[0], false);
					dsql_nod* temp = MAKE_node(nod_via, e_via_count);
					node->nod_arg[1] = temp;
					dsql_nod* rse = PASS1_rse(statement, sub2, NULL);
					temp->nod_arg[e_via_rse] = rse;
					temp->nod_arg[e_via_value_1] = rse->nod_arg[e_rse_items]->nod_arg[0];
					temp->nod_arg[e_via_value_2] = MAKE_node(nod_null, (int) 0);

					// Try to force sub1 to be same type as sub2 eg: ? = (select ...) case
					sub1 = node->nod_arg[0];
					sub2 = node->nod_arg[1];
					set_parameter_type(statement, sub1, sub2, false);

					// Finish off by cleaning up contexts
					statement->req_context->clear(base);

					return node;
				}

				switch (input->nod_type)
				{
				case nod_equiv:
				case nod_eql:
				case nod_neq:
				case nod_gtr:
				case nod_geq:
				case nod_lss:
				case nod_leq:
					return pass1_any(statement, input, nod_any);

				case nod_eql_any:
				case nod_neq_any:
				case nod_gtr_any:
				case nod_geq_any:
				case nod_lss_any:
				case nod_leq_any:
					return pass1_any(statement, input, nod_ansi_any);

				case nod_eql_all:
				case nod_neq_all:
				case nod_gtr_all:
				case nod_geq_all:
				case nod_lss_all:
				case nod_leq_all:
					return pass1_any(statement, input, nod_ansi_all);
				default:	// make compiler happy
					break;
				}
			}
		}
		break;

	case nod_agg_count:
	case nod_agg_min:
	case nod_agg_max:
	case nod_agg_average:
	case nod_agg_total:
	case nod_agg_average2:
	case nod_agg_total2:
	case nod_agg_list:
		if (statement->isPsql())
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err));
		}
		if (!(statement->req_in_select_list || statement->req_in_where_clause  ||
			statement->req_in_group_by_clause  || statement->req_in_having_clause ||
			statement->req_in_order_by_clause))
		{
			// not part of a select list, where clause, group by clause,
			// having clause, or order by clause
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_agg_ref_err));
		}
		node = MAKE_node(input->nod_type, e_agg_function_count);
		node->nod_count = input->nod_count;  // copy count, because this must be exactly the same as input
		node->nod_flags = input->nod_flags;
		if (input->nod_count) {
			for (int i = 0; i < input->nod_count; i++) {
				node->nod_arg[i] = PASS1_node(statement, input->nod_arg[i]);

				Firebird::AutoPtr<dsql_nod> desc_node(FB_NEW_RPT(*getDefaultMemoryPool(), 0) dsql_nod);
				desc_node->nod_desc.dsc_dtype = dtype_text;
				desc_node->nod_desc.dsc_length = 1;
				desc_node->nod_desc.dsc_sub_type = 0;
				desc_node->nod_desc.dsc_scale = 0;
				set_parameter_type(statement, node->nod_arg[i], desc_node, false);
			}
		}
		else {
			// Scope level is needed to determine to which context COUNT(*) belongs.
			node->nod_arg[e_agg_function_scope_level] = (dsql_nod*)(IPTR) statement->req_scope_level;
		}
		return node;

		// access plan node types

	case nod_plan_item:
		node = MAKE_node(input->nod_type, 2);
		node->nod_arg[0] = sub1 = MAKE_node(nod_relation, e_rel_count);
		sub1->nod_arg[e_rel_context] = pass1_alias_list(statement, input->nod_arg[0]);
		node->nod_arg[1] = PASS1_node(statement, input->nod_arg[1]);
		return node;

	case nod_index:
		node = MAKE_node(input->nod_type, 1);
		node->nod_arg[0] = input->nod_arg[0];
		return node;

	case nod_index_order:
		node = MAKE_node(input->nod_type, 2);
		node->nod_arg[0] = input->nod_arg[0];
		node->nod_arg[1] = input->nod_arg[1];
		return node;

	case nod_dom_value:
		{
			const dsql_nod* const ddl_node =
				(statement->req_type == REQ_DDL) ? statement->req_ddl_node : NULL;
			if (!ddl_node ||
				!(ddl_node->nod_type == nod_def_domain || ddl_node->nod_type == nod_mod_domain))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
						  Arg::Gds(isc_dsql_domain_err));
			}
		}
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_desc = input->nod_desc;
		return node;

	case nod_internal_info:
		{
			const internal_info_id id =
				*reinterpret_cast<internal_info_id*>(input->nod_arg[0]->nod_desc.dsc_address);
			USHORT req_mask = InternalInfo::getMask(id);
			if (req_mask && !(statement->req_flags & req_mask))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					// Token unknown
					Arg::Gds(isc_token_err) <<
					Arg::Gds(isc_random) << Arg::Str(InternalInfo::getAlias(id)));
			}
		}
		break;

	case nod_current_time:
	case nod_current_timestamp:
		{
			const dsql_nod* const_node = input->nod_arg[0];
			if (const_node) {
				fb_assert(const_node->nod_type == nod_constant);
				const int precision = const_node->getSlong();
				fb_assert(precision >= 0);
				if (unsigned(precision) > MAX_TIME_PRECISION) {
					ERRD_post(Arg::Gds(isc_invalid_time_precision) << Arg::Num(MAX_TIME_PRECISION));
				}
			}
		}
		break;

	case nod_join:
		return pass1_join(statement, input);

	case nod_returning:
		return pass1_returning(statement, input);

	case nod_not:
		return pass1_not(statement, input, true);

	case nod_tran_params:
		return input;

	case nod_exec_stmt_privs:
		return input;

	case nod_named_param:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_named_param_name] = input->nod_arg[e_named_param_name];
		node->nod_arg[e_named_param_expr] = PASS1_node(statement, input->nod_arg[e_named_param_expr]);
		return node;

	default:
		break;
	}

	// Node is simply to be rebuilt -- just recurse merrily

	node = MAKE_node(input->nod_type, input->nod_count);
	const dsql_nod** ptr2 = const_cast<const dsql_nod**>(node->nod_arg);

	dsql_nod** ptr = input->nod_arg;
	for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		DEV_BLKCHK(*ptr, dsql_type_nod);
		*ptr2++ = PASS1_node(statement, *ptr);
		DEV_BLKCHK(*(ptr2 - 1), dsql_type_nod);
	}

	// Try to match parameters against things of known data type.
	dsql_nod* sub2 = NULL;
	dsql_nod* sub3 = NULL;

	switch (node->nod_type)
	{
	case nod_between:
		sub3 = node->nod_arg[2];
		// FALLINTO
	case nod_assign:
	case nod_equiv:
	case nod_eql:
	case nod_gtr:
	case nod_geq:
	case nod_leq:
	case nod_lss:
	case nod_neq:
	case nod_eql_any:
	case nod_gtr_any:
	case nod_geq_any:
	case nod_leq_any:
	case nod_lss_any:
	case nod_neq_any:
	case nod_eql_all:
	case nod_gtr_all:
	case nod_geq_all:
	case nod_leq_all:
	case nod_lss_all:
	case nod_neq_all:
		sub1 = node->nod_arg[0];
		sub2 = node->nod_arg[1];

		// Try to force sub1 to be same type as sub2 eg: ? = FIELD case
		set_parameter_type(statement, sub1, sub2, false);

		// Try to force sub2 to be same type as sub1 eg: FIELD = ? case
		// Try even when the above call succeeded, because "sub2" may
		// have sub-expressions that should be resolved.
		set_parameter_type(statement, sub2, sub1, false);

		// X BETWEEN Y AND ? case
		if (!set_parameter_type(statement, sub3, sub1, false))
		{
			// ? BETWEEN Y AND ? case
			set_parameter_type(statement, sub3, sub2, false);
		}
		break;

	case nod_like:
	case nod_similar:
		if (node->nod_count == 3) {
			sub3 = node->nod_arg[2];
		}
		// FALLINTO
	case nod_containing:
	case nod_starting:
		sub1 = node->nod_arg[0];
		sub2 = node->nod_arg[1];

		// Try to force sub1 to be same type as sub2 eg: ? LIKE FIELD case
		set_parameter_type(statement, sub1, sub2, true);

		// Try to force sub2 same type as sub 1 eg: FIELD LIKE ? case
		// Try even when the above call succeeded, because "sub2" may
		// have sub-expressions that should be resolved.
		set_parameter_type(statement, sub2, sub1, true);

		// X LIKE Y ESCAPE ? case
		set_parameter_type(statement, sub3, sub2, true);
		break;

	case nod_missing:
		set_parameter_type(statement, node, NULL, false);
		break;

	case nod_trim:
		sub1 = node->nod_arg[e_trim_characters];
		sub2 = node->nod_arg[e_trim_value];

		// Try to force sub1 to be same type as sub2 eg: TRIM(? FROM FIELD) case
		set_parameter_type(statement, sub1, sub2, false);
		break;

	default:
		break;
	}

	return node;
}


/**

 	PASS1_rse

    @brief	Compile a record selection expression,
 	bumping up the statement scope level
 	everytime an rse is seen.  The scope
 	level controls parsing of aliases.


    @param statement
    @param input
    @param update_lock

 **/
dsql_nod* PASS1_rse(CompiledStatement* statement, dsql_nod* input, dsql_nod* update_lock)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(update_lock, dsql_type_nod);

	fb_assert(input->nod_type == nod_select_expr);

	statement->req_scope_level++;
	dsql_nod* node = pass1_rse(statement, input, NULL, NULL, update_lock, 0);
	statement->req_scope_level--;

	return node;
}


/**

 	PASS1_statement

    @brief	Compile a parsed statement into something more interesting.


    @param statement
    @param input

 **/
dsql_nod* PASS1_statement(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

#ifdef DSQL_DEBUG
	if (DSQL_debug & 2) {
		dsql_trace("Node tree at DSQL pass1 entry:");
		DSQL_pretty(input, 0);
	}
#endif

	dsql_nod* node = NULL;
	const DsqlContextStack::iterator base(*statement->req_context);

	// Dispatch on node type.  Fall thru on easy ones

	switch (input->nod_type)
	{
	case nod_def_database:
		statement->req_type = REQ_CREATE_DB;
		return input;

	case nod_def_relation:
	case nod_redef_relation:
	case nod_mod_relation:
	case nod_del_relation:
	case nod_def_index:
	case nod_mod_index:
	case nod_del_index:
	case nod_def_view:
	case nod_redef_view:
	case nod_mod_view:
	case nod_replace_view:
	case nod_del_view:
	case nod_def_constraint:
	case nod_def_exception:
	case nod_redef_exception:
	case nod_mod_exception:
	case nod_replace_exception:
	case nod_del_exception:
	case nod_grant:
	case nod_revoke:
	case nod_mod_database:
	case nod_def_generator:
	case nod_del_generator:
	case nod_def_role:
	case nod_del_role:
	case nod_def_filter:
	case nod_del_filter:
	case nod_def_domain:
	case nod_mod_domain:
	case nod_del_domain:
	case nod_def_udf:
	case nod_del_udf:
	case nod_def_shadow:
	case nod_del_shadow:
	case nod_set_statistics:
	case nod_comment:
	case nod_mod_udf:
	case nod_mod_role:
	case nod_add_user:
	case nod_mod_user:
	case nod_del_user:
	case nod_def_collation:
	case nod_del_collation:
		statement->req_type = REQ_DDL;
		return input;

	case nod_class_node:
		node = reinterpret_cast<dsql_nod*>(
			reinterpret_cast<Node*>(input->nod_arg[0])->dsqlPass(statement));
		if (node != input->nod_arg[0])
		{
			input = MAKE_node(input->nod_type, input->nod_count);
			input->nod_arg[0] = node;
		}
		return input;

	case nod_def_trigger:
	case nod_redef_trigger:
	case nod_mod_trigger:
	case nod_replace_trigger:
	case nod_del_trigger:
		statement->req_type = REQ_DDL;
		statement->req_flags |= (REQ_block | REQ_procedure | REQ_trigger);
		return input;

	case nod_del_procedure:
		statement->req_type = REQ_DDL;
		statement->req_flags |= (REQ_block | REQ_procedure);
		return input;

	case nod_def_procedure:
	case nod_redef_procedure:
	case nod_mod_procedure:
	case nod_replace_procedure:
		{
			statement->req_type = REQ_DDL;
			statement->req_flags |= (REQ_block | REQ_procedure);

			const dsql_nod* variables = input->nod_arg[e_prc_dcls];
			if (variables)
			{

				// Ensure that variable names do not duplicate parameter names

				const dsql_nod* const* ptr = variables->nod_arg;
				for (const dsql_nod* const* const end = ptr + variables->nod_count; ptr < end; ptr++)
				{
					if ((*ptr)->nod_type == nod_def_field)
					{

						const dsql_fld* field = (dsql_fld*) (*ptr)->nod_arg[e_dfl_field];
						DEV_BLKCHK(field, dsql_type_fld);

						const dsql_nod* parameters = input->nod_arg[e_prc_inputs];
						if (parameters)
						{

							const dsql_nod* const* ptr2 = parameters->nod_arg;
							for (const dsql_nod* const* const end2 =
								 ptr2 + parameters->nod_count; ptr2 < end2; ptr2++)
							{
								const dsql_fld* field2 = (dsql_fld*) (*ptr2)->nod_arg[e_dfl_field];
								DEV_BLKCHK(field2, dsql_type_fld);

								if (field->fld_name == field2->fld_name)
								{
									ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
											  Arg::Gds(isc_dsql_var_conflict) << Arg::Str(field->fld_name));
								}
							}
						}

						parameters = input->nod_arg[e_prc_outputs];
						if (parameters)
						{

							const dsql_nod* const* ptr2 = parameters->nod_arg;
							for (const dsql_nod* const* const end2 =
								 ptr2 + parameters->nod_count; ptr2 < end2; ptr2++)
							{
								const dsql_fld* field2 = (dsql_fld*) (*ptr2)->nod_arg[e_dfl_field];
								DEV_BLKCHK(field2, dsql_type_fld);

								if (field->fld_name == field2->fld_name)
								{
									ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
											  Arg::Gds(isc_dsql_var_conflict) << Arg::Str(field->fld_name));
								}
							}
						}
					}
				}
			}
			return input;
		}

	case nod_assign:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_asgn_value] = PASS1_node(statement, input->nod_arg[e_asgn_value]);
		node->nod_arg[e_asgn_field] = PASS1_node(statement, input->nod_arg[e_asgn_field]);
		break;

	case nod_commit:
		if ((input->nod_arg[e_commit_retain]) &&
			(input->nod_arg[e_commit_retain]->nod_type == nod_retain))
		{
			statement->req_type = REQ_COMMIT_RETAIN;
		}
		else
			statement->req_type = REQ_COMMIT;
		return input;

	case nod_rollback:
		if ((input->nod_arg[e_rollback_retain]) &&
			(input->nod_arg[e_rollback_retain]->nod_type == nod_retain))
		{
			statement->req_type = REQ_ROLLBACK_RETAIN;
		}
		else
			statement->req_type = REQ_ROLLBACK;
		return input;

	case nod_delete:
		node = pass1_savepoint(statement, pass1_delete(statement, input));
		break;

	case nod_exec_procedure:
		{
			const dsql_str* name = (dsql_str*) input->nod_arg[e_exe_procedure];
			DEV_BLKCHK(name, dsql_type_str);
			dsql_prc* procedure = METD_get_procedure(statement, name);
			if (!procedure) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(isc_dsql_procedure_err) <<
						  Arg::Gds(isc_random) << Arg::Str(name->str_data));
			}

			if (!statement->isPsql())
			{
				statement->req_procedure = procedure;
				statement->req_type = REQ_EXEC_PROCEDURE;
			}

			node = MAKE_node(input->nod_type, input->nod_count);
			node->nod_arg[e_exe_procedure] = input->nod_arg[e_exe_procedure];

			// handle input parameters

			const USHORT count = input->nod_arg[e_exe_inputs] ?
				input->nod_arg[e_exe_inputs]->nod_count : 0;
			if (count > procedure->prc_in_count ||
				count < procedure->prc_in_count - procedure->prc_def_count)
			{
				ERRD_post(Arg::Gds(isc_prcmismat) << Arg::Str(name->str_data));
			}

			node->nod_arg[e_exe_inputs] = PASS1_node(statement, input->nod_arg[e_exe_inputs]);

			if (count)
			{
				// Initialize this stack variable, and make it look like a node
				Firebird::AutoPtr<dsql_nod> desc_node(FB_NEW_RPT(*getDefaultMemoryPool(), 0) dsql_nod);

				dsql_nod* const* ptr = node->nod_arg[e_exe_inputs]->nod_arg;
				for (const dsql_fld* field = procedure->prc_inputs; *ptr;
					ptr++, field = field->fld_next)
				{
					DEV_BLKCHK(field, dsql_type_fld);
					DEV_BLKCHK(*ptr, dsql_type_nod);
					MAKE_desc_from_field(&desc_node->nod_desc, field);
					// set_parameter_type(*ptr, &desc_node, false);
					set_parameter_type(statement, *ptr, desc_node, false);
				}
			}

			// handle output parameters

			dsql_nod* temp = input->nod_arg[e_exe_outputs];
			if (statement->isPsql())
			{
				const USHORT ocount = temp ? temp->nod_count : 0;
				if (ocount != procedure->prc_out_count)
				{
					ERRD_post(Arg::Gds(isc_prc_out_param_mismatch) << Arg::Str(name->str_data));
				}

				node->nod_arg[e_exe_outputs] = PASS1_node(statement, temp);
			}
			else {
				if (temp) {
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  // Token unknown
							  Arg::Gds(isc_token_err) <<
							  Arg::Gds(isc_random) << Arg::Str("RETURNING_VALUES"));
				}
				node->nod_arg[e_exe_outputs] = explode_outputs(statement, statement->req_procedure);
			}
		}
		break;

	case nod_exec_block:
		if (input->nod_arg[e_exe_blk_outputs] && input->nod_arg[e_exe_blk_outputs]->nod_count)
		{
			statement->req_type = REQ_SELECT_BLOCK;
		}
		else
			statement->req_type = REQ_EXEC_BLOCK;
		statement->req_flags |= REQ_block;

		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_exe_blk_inputs] =
			pass1_node_psql(statement, input->nod_arg[e_exe_blk_inputs], false);
		node->nod_arg[e_exe_blk_outputs] = input->nod_arg[e_exe_blk_outputs];

		node->nod_arg[e_exe_blk_dcls] = input->nod_arg[e_exe_blk_dcls];
		node->nod_arg[e_exe_blk_body] = input->nod_arg[e_exe_blk_body];

		{ // scope
			const size_t ncount =
				node->nod_arg[e_exe_blk_inputs] ?
					node->nod_arg[e_exe_blk_inputs]->nod_count : 0 +
				node->nod_arg[e_exe_blk_outputs] ?
					node->nod_arg[e_exe_blk_outputs]->nod_count : 0 +
				node->nod_arg[e_exe_blk_dcls] ?
					node->nod_arg[e_exe_blk_dcls]->nod_count : 0;

			if (ncount)
			{
				StrArray names( *getDefaultMemoryPool(), ncount);

				check_unique_fields_names(names, node->nod_arg[e_exe_blk_inputs]);
				check_unique_fields_names(names, node->nod_arg[e_exe_blk_outputs]);
				check_unique_fields_names(names, node->nod_arg[e_exe_blk_dcls]);
			}
		} // end scope
		return node;

	case nod_for_select:
		{
			node = MAKE_node(input->nod_type, input->nod_count);
			node->nod_flags = input->nod_flags;
			dsql_nod* cursor = node->nod_arg[e_flp_cursor] = input->nod_arg[e_flp_cursor];

			node->nod_arg[e_flp_select] = PASS1_statement(statement, input->nod_arg[e_flp_select]);

			if (cursor) {
				fb_assert(cursor->nod_flags > 0);
				pass1_cursor_name(statement, (dsql_str*) cursor->nod_arg[e_cur_name],
								  NOD_CURSOR_ALL, false);
				cursor->nod_arg[e_cur_rse] = node->nod_arg[e_flp_select];
				cursor->nod_arg[e_cur_number] = (dsql_nod*) (IPTR) statement->req_cursor_number++;
				statement->req_cursors.push(cursor);
			}

			dsql_nod* into_in = input->nod_arg[e_flp_into];

			if (into_in)
			{
				dsql_nod* into_out = MAKE_node(into_in->nod_type, into_in->nod_count);
				node->nod_arg[e_flp_into] = into_out;
				const dsql_nod** ptr2 = const_cast<const dsql_nod**>(into_out->nod_arg);
				dsql_nod** ptr = into_in->nod_arg;
				for (const dsql_nod* const* const end = ptr + into_in->nod_count; ptr < end; ptr++)
				{
					DEV_BLKCHK(*ptr, dsql_type_nod);
					*ptr2++ = PASS1_node(statement, *ptr);
					DEV_BLKCHK(*(ptr2 - 1), dsql_type_nod);
				}
			}

			if (input->nod_arg[e_flp_action])
			{
				// CVC: Let's add the ability to BREAK the for_select same as the while,
				// but only if the command is FOR SELECT, otherwise we have singular SELECT
				statement->req_loop_level++;
				node->nod_arg[e_flp_label] = pass1_label(statement, input);
				node->nod_arg[e_flp_action] = PASS1_statement(statement, input->nod_arg[e_flp_action]);
				statement->req_loop_level--;
				statement->req_labels.pop();
			}

			if (cursor) {
				statement->req_cursor_number--;
				statement->req_cursors.pop();
			}
		}
		break;

	case nod_get_segment:
	case nod_put_segment:
		pass1_blob(statement, input);
		return input;

	case nod_if:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_if_condition] = PASS1_node(statement, input->nod_arg[e_if_condition]);
		node->nod_arg[e_if_true] = PASS1_statement(statement, input->nod_arg[e_if_true]);
		if (input->nod_arg[e_if_false])
			node->nod_arg[e_if_false] = PASS1_statement(statement, input->nod_arg[e_if_false]);
		else
			node->nod_arg[e_if_false] = NULL;
		break;

	case nod_exception_stmt:
		node = input;
		// if exception value is defined, pass value node
		if (input->nod_arg[e_xcps_msg])
		{
			node->nod_arg[e_xcps_msg] = PASS1_node(statement, input->nod_arg[e_xcps_msg]);
		}
		else
		{
			node->nod_arg[e_xcps_msg] = 0;
		}
		return pass1_savepoint(statement, node);

	case nod_insert:
		node = pass1_savepoint(statement, pass1_insert(statement, input, false));
		break;

	case nod_merge:
		node = pass1_savepoint(statement, pass1_merge(statement, input));
		break;

	case nod_block:
		if (input->nod_arg[e_blk_errs])
			statement->req_error_handlers++;
		else if (input->nod_arg[e_blk_action]) {
			input->nod_count = 1;
			if (!statement->req_error_handlers)
				input->nod_type = nod_list;
		}
		else {
			input->nod_count = 0;
			input->nod_type = nod_list;
		}

	case nod_list:
		{
			node = MAKE_node(input->nod_type, input->nod_count);
			const dsql_nod** ptr2 = const_cast<const dsql_nod**>(node->nod_arg);
			dsql_nod* const* ptr = input->nod_arg;
			for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
			{
				DEV_BLKCHK(*ptr, dsql_type_nod);
				if ((*ptr)->nod_type == nod_assign)
					*ptr2++ = PASS1_node(statement, *ptr);
				else
					*ptr2++ = PASS1_statement(statement, *ptr);
				DEV_BLKCHK(*(ptr2 - 1), dsql_type_nod);
			}
			if (input->nod_type == nod_block && input->nod_arg[e_blk_errs])
				statement->req_error_handlers--;
			return node;
		}

	case nod_on_error:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_err_errs] = input->nod_arg[e_err_errs];
		node->nod_arg[e_err_action] = PASS1_statement(statement, input->nod_arg[e_err_action]);
		return node;

	case nod_post:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_pst_event] = PASS1_node(statement, input->nod_arg[e_pst_event]);
		node->nod_arg[e_pst_argument] = PASS1_node(statement, input->nod_arg[e_pst_argument]);
		return node;

	case nod_exec_sql:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_exec_sql_stmnt] = PASS1_node(statement, input->nod_arg[e_exec_sql_stmnt]);
		return pass1_savepoint(statement, node);

	case nod_exec_into:
		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_exec_into_stmnt] = PASS1_node(statement, input->nod_arg[e_exec_into_stmnt]);
		if (input->nod_arg[e_exec_into_block]) {
			statement->req_loop_level++;
			node->nod_arg[e_exec_into_label] = pass1_label(statement, input);
			node->nod_arg[e_exec_into_block] =
				PASS1_statement(statement, input->nod_arg[e_exec_into_block]);
			statement->req_loop_level--;
			statement->req_labels.pop();
		}

		node->nod_arg[e_exec_into_list] = PASS1_node(statement, input->nod_arg[e_exec_into_list]);
		return pass1_savepoint(statement, node);

	case nod_exec_stmt:
		// if no new features of EXECUTE STATEMENT is used, lets generate old BLR
		if (!input->nod_arg[e_exec_stmt_inputs] && !input->nod_arg[e_exec_stmt_options])
		{
			if (!input->nod_arg[e_exec_stmt_outputs])
			{
				node = MAKE_node(nod_exec_sql, e_exec_sql_count);
				node->nod_arg[e_exec_sql_stmnt] = input->nod_arg[e_exec_stmt_sql];
			}
			else
			{
				node = MAKE_node(nod_exec_into, e_exec_into_count);
				node->nod_arg[e_exec_into_stmnt] = input->nod_arg[e_exec_stmt_sql];
				node->nod_arg[e_exec_into_block] = input->nod_arg[e_exec_stmt_proc_block];
				node->nod_arg[e_exec_into_list] = input->nod_arg[e_exec_stmt_outputs];
				node->nod_arg[e_exec_into_label] = input->nod_arg[e_exec_stmt_label];
			}
			return PASS1_statement(statement, node);
		}

		node = MAKE_node(input->nod_type, input->nod_count);
		node->nod_arg[e_exec_stmt_sql] = PASS1_node(statement, input->nod_arg[e_exec_stmt_sql]);
		node->nod_arg[e_exec_stmt_inputs] = PASS1_node(statement, input->nod_arg[e_exec_stmt_inputs]);

		// check params names uniqueness, if present
		if (node->nod_arg[e_exec_stmt_inputs])
		{
			const int cnt = node->nod_arg[e_exec_stmt_inputs]->nod_count;
			StrArray names(*getDefaultMemoryPool(), cnt);

			const dsql_nod* const* prm = node->nod_arg[e_exec_stmt_inputs]->nod_arg;
			if (prm[0]->nod_arg[e_named_param_name])
			{
				for (int i = 0; i < cnt; i++)
				{
					const dsql_str* name = (dsql_str*) prm[i]->nod_arg[e_named_param_name];

					size_t pos;
					if (names.find(name->str_data, pos)) {
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
								  Arg::Gds(isc_dsql_duplicate_spec) << Arg::Str(name->str_data));
					}
					names.insert(pos, name->str_data);
				}
			}
		}

		node->nod_arg[e_exec_stmt_outputs] = PASS1_node(statement, input->nod_arg[e_exec_stmt_outputs]);

		if (input->nod_arg[e_exec_stmt_proc_block])
		{
			statement->req_loop_level++;
			node->nod_arg[e_exec_stmt_label] = pass1_label(statement, input);
			node->nod_arg[e_exec_stmt_proc_block] =
				PASS1_statement(statement, input->nod_arg[e_exec_stmt_proc_block]);
			statement->req_loop_level--;
			statement->req_labels.pop();
		}

		// process various optional arguments
		if (input->nod_arg[e_exec_stmt_options])
		{
			dsql_nod* list = input->nod_arg[e_exec_stmt_options];
			fb_assert(list->nod_type == nod_list);

			const dsql_nod* const* end = list->nod_arg + list->nod_count;

			for (dsql_nod** ptr = list->nod_arg; ptr < end; ptr++)
			{
				const char* dupClause = NULL;
				dsql_nod* opt = *ptr;

				switch (opt->nod_type)
				{
				case nod_exec_stmt_datasrc:
					if (node->nod_arg[e_exec_stmt_data_src])
						dupClause = "EXTERNAL DATA SOURCE";
					else
						node->nod_arg[e_exec_stmt_data_src] = PASS1_node(statement, opt->nod_arg[0]);
					break;

				case nod_exec_stmt_user:
					if (node->nod_arg[e_exec_stmt_user])
						dupClause = "USER";
					else
						node->nod_arg[e_exec_stmt_user] = PASS1_node(statement, opt->nod_arg[0]);
					break;

				case nod_exec_stmt_pwd:
					if (node->nod_arg[e_exec_stmt_pwd])
						dupClause = "PASSWORD";
					else
						node->nod_arg[e_exec_stmt_pwd] = PASS1_node(statement, opt->nod_arg[0]);
					break;

				case nod_exec_stmt_role:
					if (node->nod_arg[e_exec_stmt_role])
						dupClause = "ROLE";
					else
						node->nod_arg[e_exec_stmt_role] = PASS1_node(statement, opt->nod_arg[0]);
					break;

				case nod_tran_params:
					if (node->nod_arg[e_exec_stmt_tran])
						dupClause = "TRANSACTION";
					else
						node->nod_arg[e_exec_stmt_tran] = PASS1_node(statement, opt);
					break;

				case nod_exec_stmt_privs:
					if (node->nod_arg[e_exec_stmt_privs])
						dupClause = "CALLER PRIVILEGES";
					else
						node->nod_arg[e_exec_stmt_privs] = PASS1_node(statement, opt);
					break;

				default:
					fb_assert(false);
				}

				if (dupClause) {
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
							  Arg::Gds(isc_dsql_duplicate_spec) << Arg::Str(dupClause));
				}
			}
		}

		return pass1_savepoint(statement, node);

	case nod_exit:
		return input;

	case nod_breakleave:
		if (!statement->req_loop_level)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("BREAK/LEAVE"));
		}
		input->nod_arg[e_breakleave_label] = pass1_label(statement, input);
		return input;

	case nod_return:
		if (statement->req_flags & REQ_trigger)	// triggers only
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("SUSPEND"));
		}

		if (statement->req_flags & REQ_in_auto_trans_block)	// autonomous transaction
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_dsql_unsupported_in_auto_trans) << Arg::Str("SUSPEND"));
		}

		statement->req_flags |= REQ_selectable;

		input->nod_arg[e_rtn_procedure] =
			statement->req_ddl_node ? statement->req_ddl_node : statement->req_blk_node;
		return input;

	case nod_select:
		{
			node = PASS1_rse(statement, input->nod_arg[e_select_expr], input->nod_arg[e_select_lock]);

			if (input->nod_arg[e_select_update]) {
				statement->req_type = REQ_SELECT_UPD;
				statement->req_flags |= REQ_no_batch;
				break;
			}

			// If there is a union without ALL or order by or a select distinct
			// buffering is OK even if stored procedure occurs in the select
			// list. In these cases all of stored procedure is executed under
			// savepoint for open cursor.

			if (node->nod_arg[e_rse_sort] || node->nod_arg[e_rse_reduced])
			{
				statement->req_flags &= ~REQ_no_batch;
			}

		}
		break;

	case nod_trans:
		statement->req_type = REQ_START_TRANS;
		return input;

	case nod_update:
		node = pass1_savepoint(statement, pass1_update(statement, input, false));
		break;

	case nod_while:
		{
			node = MAKE_node(input->nod_type, input->nod_count);
			node->nod_arg[e_while_cond] = PASS1_node(statement, input->nod_arg[e_while_cond]);

			// CVC: loop numbers should be incremented before analyzing the body
			// to preserve nesting <==> increasing level number
			statement->req_loop_level++;
			node->nod_arg[e_while_label] = pass1_label(statement, input);
			node->nod_arg[e_while_action] = PASS1_statement(statement, input->nod_arg[e_while_action]);
			statement->req_loop_level--;
			statement->req_labels.pop();
		}
		break;

	case nod_abort:
	case nod_exception:
	case nod_sqlcode:
	case nod_gdscode:
		return input;

	case nod_user_savepoint:
		if (statement->req_flags & REQ_block) // blocks, procedures and triggers
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("SAVEPOINT"));
		}
		statement->req_type = REQ_SAVEPOINT;
		return input;

	case nod_release_savepoint:
		if (statement->req_flags & REQ_block) // blocks, procedures and triggers
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("RELEASE"));
		}
		statement->req_type = REQ_SAVEPOINT;
		return input;

	case nod_undo_savepoint:
		if (statement->req_flags & REQ_block) // blocks, procedures and triggers
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("ROLLBACK"));
		}
		statement->req_type = REQ_SAVEPOINT;
		return input;

	case nod_null:
		return NULL;

	case nod_set_generator:
		node = MAKE_node(input->nod_type, e_gen_id_count);
		node->nod_arg[e_gen_id_value] = PASS1_node(statement, input->nod_arg[e_gen_id_value]);
		node->nod_arg[e_gen_id_name] = input->nod_arg[e_gen_id_name];
		statement->req_type = REQ_SET_GENERATOR;
		break;

	case nod_set_generator2:
		node = MAKE_node(input->nod_type, e_gen_id_count);
		node->nod_arg[e_gen_id_value] = PASS1_node(statement, input->nod_arg[e_gen_id_value]);
		node->nod_arg[e_gen_id_name] = input->nod_arg[e_gen_id_name];
		statement->req_type = REQ_SET_GENERATOR;
		break;

	case nod_union:
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  // union not supported
				  Arg::Gds(isc_union_err));
		break;

	case nod_cursor:
		{
			fb_assert(input->nod_flags > 0);
			// make sure the cursor doesn't exist
			pass1_cursor_name(statement, (dsql_str*) input->nod_arg[e_cur_name], NOD_CURSOR_ALL, false);
			// temporarily hide unnecessary contexts and process our RSE
			DsqlContextStack* const base_context = statement->req_context;
			DsqlContextStack temp;
			statement->req_context = &temp;
			const dsql_nod* select = input->nod_arg[e_cur_rse];
			input->nod_arg[e_cur_rse] =
				PASS1_rse(statement, select->nod_arg[e_select_expr], select->nod_arg[e_select_lock]);
			statement->req_context->clear();
			statement->req_context = base_context;
			// assign number and store in the statement stack
			input->nod_arg[e_cur_number] = (dsql_nod*) (IPTR) statement->req_cursor_number++;
			statement->req_cursors.push(input);
		}
		return input;

	case nod_cursor_open:
	case nod_cursor_close:
	case nod_cursor_fetch:
		if (statement->req_flags & REQ_in_auto_trans_block)	// autonomous transaction
		{
			const char* stmt = NULL;

			switch (input->nod_type)
			{
				case nod_cursor_open:
					stmt = "OPEN CURSOR";
					break;
				case nod_cursor_close:
					stmt = "CLOSE CURSOR";
					break;
				case nod_cursor_fetch:
					stmt = "FETCH CURSOR";
					break;
			}

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_dsql_unsupported_in_auto_trans) << Arg::Str(stmt));
		}

		// resolve the cursor
		input->nod_arg[e_cur_stmt_id] =
			pass1_cursor_name(statement, (dsql_str*) input->nod_arg[e_cur_stmt_id],
							  NOD_CURSOR_EXPLICIT, true);
		// process a seek node, if exists
		if (input->nod_arg[e_cur_stmt_seek]) {
			input->nod_arg[e_cur_stmt_seek] = PASS1_node(statement, input->nod_arg[e_cur_stmt_seek]);
		}
		// process an assignment node, if exists
		if (input->nod_arg[e_cur_stmt_into]) {
			input->nod_arg[e_cur_stmt_into] = PASS1_node(statement, input->nod_arg[e_cur_stmt_into]);
		}
		return input;

	case nod_src_info:
		{
			input->nod_line = (USHORT) (IPTR) input->nod_arg[e_src_info_line];
			input->nod_column = (USHORT) (IPTR) input->nod_arg[e_src_info_column];
			input->nod_arg[e_src_info_stmt] = PASS1_statement(statement, input->nod_arg[e_src_info_stmt]);
			return input;
		}

	case nod_update_or_insert:
		node = pass1_savepoint(statement, pass1_update_or_insert(statement, input));
		break;

	default:
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  // Unsupported DSQL construct
				  Arg::Gds(isc_dsql_construct_err));
		break;
	}

	// Finish off by cleaning up contexts
	statement->req_context->clear(base);

#ifdef DSQL_DEBUG
	if (DSQL_debug & 1) {
		dsql_trace("Node tree at DSQL pass1 exit:");
		DSQL_pretty(node, 0);
	}
#endif

	return node;
}


/**

 	aggregate_found

    @brief	Check for an aggregate expression in an
 	rse select list.  It could be buried in
 	an expression tree.


    @param statement
    @param node

 **/
static bool aggregate_found( const CompiledStatement* statement, const dsql_nod* node)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(node, dsql_type_nod);

	USHORT current_level = statement->req_scope_level;
	USHORT deepest_level = 0;

	return aggregate_found2(statement, node, &current_level, &deepest_level, false);
}


/**

 	aggregate_found2

    @brief	Check for an aggregate expression in an
 	expression. It could be buried in an expression
	tree and therefore call itselfs again. The level
	parameters (current_level & deepest_level) are used
	to see how deep we are with passing sub-queries
	(= scope_level).

 	field is true if a non-aggregate field reference is seen.


    @param statement
    @param node
    @param current_level
    @param deepest_level
    @param ignore_sub_selects

 **/
static bool aggregate_found2(const CompiledStatement* statement, const dsql_nod* node,
								USHORT* current_level,
								USHORT* deepest_level, bool ignore_sub_selects)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(node, dsql_type_nod);

	if (!node)
		return false;

	bool aggregate = false;

	switch (node->nod_type)
	{
		// handle the simple case of a straightforward aggregate

		case nod_agg_average:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_count:
		case nod_agg_list:
			if (!ignore_sub_selects)
			{
				if (node->nod_count)
				{
					USHORT ldeepest_level = 0;
					// If we are already in a aggregate function don't search inside
					// sub-selects and other aggregate-functions for the deepest field
					// used else we would have a wrong deepest_level value.
					aggregate_found2(statement, node->nod_arg[e_agg_function_expression],
									 current_level, &ldeepest_level, true);
					if (ldeepest_level == 0) {
						*deepest_level = *current_level;
					}
					else {
						*deepest_level = ldeepest_level;
					}
					// If the deepest_value is the same as the current scope_level
					// this an aggregate that belongs to the current context.
					if (*deepest_level == statement->req_scope_level) {
						aggregate = true;
					}
					else {
						// Check also for a nested aggregate that could belong to this context
						aggregate |= aggregate_found2(statement, node->nod_arg[e_agg_function_expression],
													  current_level, &ldeepest_level, false);
					}
				}
				else {
					// we have Count(*)
					if (statement->req_scope_level ==
						(USHORT)(U_IPTR) node->nod_arg[e_agg_function_scope_level])
					{
						aggregate = true;
					}
				}
			}
			return aggregate;

		case nod_field:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_fld_context]);
				if (*deepest_level < lcontext->ctx_scope_level) {
					*deepest_level = lcontext->ctx_scope_level;
				}
				return false;
			}

		case nod_alias:
			aggregate = aggregate_found2(statement, node->nod_arg[e_alias_value],
										 current_level, deepest_level, ignore_sub_selects);
			return aggregate;

		case nod_derived_field:
			{
				// This is an derived table don't look further,
				// but don't forget to check for deepest scope_level.
				const USHORT lscope_level = (USHORT)(U_IPTR) node->nod_arg[e_derived_field_scope];
				if (*deepest_level < lscope_level) {
					*deepest_level = lscope_level;
				}
			}
			return aggregate;

		case nod_map:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_map_context]);
				if (lcontext->ctx_scope_level == statement->req_scope_level) {
					return true;
				}

				const dsql_map* lmap = reinterpret_cast<dsql_map*>(node->nod_arg[e_map_map]);
				aggregate = aggregate_found2(statement, lmap->map_node, current_level,
											 deepest_level, ignore_sub_selects);
				return aggregate;
			}

			// for expressions in which an aggregate might
			// be buried, recursively check for one

		case nod_via:
			if (!ignore_sub_selects) {
				aggregate = aggregate_found2(statement, node->nod_arg[e_via_rse], current_level,
											 deepest_level, ignore_sub_selects);
			}
			return aggregate;

		case nod_exists:
		case nod_singular:
			if (!ignore_sub_selects) {
				aggregate = aggregate_found2(statement, node->nod_arg[0], current_level,
											 deepest_level, ignore_sub_selects);
			}
			return aggregate;

		case nod_aggregate:
			if (!ignore_sub_selects) {
				aggregate = aggregate_found2(statement, node->nod_arg[e_agg_rse], current_level,
											 deepest_level, ignore_sub_selects);
			}
			return aggregate;

		case nod_rse:
			++*current_level;
			aggregate |= aggregate_found2(statement, node->nod_arg[e_rse_streams], current_level,
										  deepest_level, ignore_sub_selects);
			aggregate |= aggregate_found2(statement, node->nod_arg[e_rse_boolean],
										  current_level, deepest_level, ignore_sub_selects);
			aggregate |= aggregate_found2(statement, node->nod_arg[e_rse_items],
										  current_level, deepest_level, ignore_sub_selects);
			--*current_level;
			return aggregate;

		case nod_order:
			aggregate = aggregate_found2(statement, node->nod_arg[e_order_field], current_level,
										 deepest_level, ignore_sub_selects);
			return aggregate;

		case nod_or:
		case nod_and:
		case nod_not:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_lss:
		case nod_leq:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_lss_any:
		case nod_leq_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_lss_all:
		case nod_leq_all:
		case nod_between:
		case nod_like:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_missing:
		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_subtract2:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_strlen:
		case nod_coalesce:
		case nod_simple_case:
		case nod_searched_case:
		case nod_list:
		case nod_join:
		case nod_join_inner:
		case nod_join_left:
		case nod_join_right:
		case nod_join_full:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ++ptr)
				{
					aggregate |= aggregate_found2(statement, *ptr, current_level,
												  deepest_level, ignore_sub_selects);
				}
				return aggregate;
			}

		case nod_cast:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_udf:
		case nod_sys_function:
			if (node->nod_count == 2) {
				return (aggregate_found2(statement, node->nod_arg[1], current_level,
										 deepest_level, ignore_sub_selects));
			}
			return false;

		case nod_constant:
			return false;

		case nod_relation:
			{
				const dsql_ctx* lrelation_context =
					reinterpret_cast<dsql_ctx*>(node->nod_arg[e_rel_context]);
				// Check if relation is a procedure
				if (lrelation_context->ctx_procedure) {
					// Check if a aggregate is buried inside the input parameters
					aggregate |= aggregate_found2(statement, lrelation_context->ctx_proc_inputs,
												  current_level, deepest_level, ignore_sub_selects);
				}
				return aggregate;
			}

		case nod_hidden_var:
			return (aggregate_found2(statement, node->nod_arg[e_hidden_var_expr], current_level,
									 deepest_level, ignore_sub_selects));

		default:
			return false;
	}
}


/**

 	ambiguity

    @brief	Check for ambiguity in a field
   reference. The list with contexts where the
   field was found is checked and the necessary
   message is build from it.


    @param statement
    @param node
    @param name
    @param ambiguous_contexts

 **/
static dsql_nod* ambiguity_check(CompiledStatement* statement, dsql_nod* node,
	const dsql_str* name, const DsqlContextStack& ambiguous_contexts)
{
	// If there are no relations or only 1 there's no ambiguity, thus return.
	if (ambiguous_contexts.getCount() < 2) {
		return node;
	}

	TEXT buffer[1024];
	USHORT loop = 0;

	buffer[0] = 0;
	TEXT* b = buffer;
	TEXT* p = 0;

	for (DsqlContextStack::const_iterator stack(ambiguous_contexts); stack.hasData(); ++stack)
	{
		const dsql_ctx* context = stack.object();
		const dsql_rel* relation = context->ctx_relation;
		const dsql_prc* procedure = context->ctx_procedure;
		if (strlen(b) > (sizeof(buffer) - 50)) {
			// Buffer full
			break;
		}
		// if this is the second loop add "and " before relation.
		if (++loop > 2) {
			strcat(buffer, "and ");
		}
		// Process relation when present.
		if (relation) {
			if (!(relation->rel_flags & REL_view)) {
				strcat(buffer, "table ");
			}
			else {
				strcat(buffer, "view ");
			}
			strcat(buffer, relation->rel_name.c_str());
		}
		else if (procedure) {
			// Process procedure when present.
			strcat(b, "procedure ");
			strcat(b, procedure->prc_name.c_str());
		}
		else {
			// When there's no relation and no procedure it's a derived table.
			strcat(b, "derived table ");
			if (context->ctx_alias) {
				strcat(b, context->ctx_alias);
			}
		}
		strcat(buffer, " ");
		if (!p) {
			p = b + strlen(b);
		}
	}

	if (p) {
		*--p = 0;
	}

	if (statement->req_client_dialect >= SQL_DIALECT_V6) {
		delete node;
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
				  Arg::Gds(isc_dsql_ambiguous_field_name) << Arg::Str(buffer) << Arg::Str(++p) <<
				  Arg::Gds(isc_random) << Arg::Str(name->str_data));
		return NULL;
	}

	ERRD_post_warning(Arg::Warning(isc_sqlwarn) << Arg::Num(204) <<
					  Arg::Warning(isc_dsql_ambiguous_field_name) << Arg::Str(buffer) <<
																	 Arg::Str(++p) <<
					  Arg::Warning(isc_random) << Arg::Str(name->str_data));

	return node;
}


/**

 	assign_fld_dtype_from_dsc

    @brief	Set a field's descriptor from a DSC
 	(If dsql_fld* is ever redefined this can be removed)


    @param field
    @param nod_desc

 **/
static void assign_fld_dtype_from_dsc( dsql_fld* field, const dsc* nod_desc)
{
	DEV_BLKCHK(field, dsql_type_fld);

	field->fld_dtype = nod_desc->dsc_dtype;
	field->fld_scale = nod_desc->dsc_scale;
	field->fld_sub_type = nod_desc->dsc_sub_type;
	field->fld_length = nod_desc->dsc_length;

	if (nod_desc->dsc_dtype <= dtype_any_text) {
		field->fld_collation_id = DSC_GET_COLLATE(nod_desc);
		field->fld_character_set_id = DSC_GET_CHARSET(nod_desc);
	}
	else if (nod_desc->dsc_dtype == dtype_blob)
	{
		field->fld_character_set_id = nod_desc->dsc_scale;
		field->fld_collation_id = nod_desc->dsc_flags >> 8;
	}

	if (nod_desc->dsc_flags & DSC_nullable)
		field->fld_flags |= FLD_nullable;
}


/**
	check_unique_fields_names

	check fields (params, variables, cursors etc) names against
	sorted array
	if success, add them into array
 **/
static void check_unique_fields_names(StrArray& names, const dsql_nod* fields)
{
	if (!fields)
		return;

	const dsql_nod* const* ptr = fields->nod_arg;
	const dsql_nod* const* const end = ptr + fields->nod_count;
	const dsql_nod* temp;
	const dsql_fld* field;
	const dsql_str* str;
	const char* name = NULL;

	for (; ptr < end; ptr++)
	{
		switch ((*ptr)->nod_type)
		{
			case nod_def_field:
				field = (dsql_fld*) (*ptr)->nod_arg[e_dfl_field];
				DEV_BLKCHK(field, dsql_type_fld);
				name = field->fld_name.c_str();
			break;

			case nod_param_val:
				temp = (*ptr)->nod_arg[e_prm_val_fld];
				field = (dsql_fld*) temp->nod_arg[e_dfl_field];
				DEV_BLKCHK(field, dsql_type_fld);
				name = field->fld_name.c_str();
			break;

			case nod_cursor:
				str = (dsql_str*) (*ptr)->nod_arg[e_cur_name];
				DEV_BLKCHK(str, dsql_type_str);
				name = str->str_data;
			break;

			default:
				fb_assert(false);
		}

		size_t pos;
		if (!names.find(name, pos))
			names.insert(pos, name);
		else {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
					  Arg::Gds(isc_dsql_duplicate_spec) << Arg::Str(name));
		}
	}
}


/**

 	compose

    @brief	Compose two booleans.


    @param expr1
    @param expr2
    @param dsql_operator

 **/
static dsql_nod* compose( dsql_nod* expr1, dsql_nod* expr2, NOD_TYPE dsql_operator)
{
	DEV_BLKCHK(expr1, dsql_type_nod);
	DEV_BLKCHK(expr2, dsql_type_nod);

	if (!expr1)
		return expr2;

	if (!expr2)
		return expr1;

	dsql_nod* node = MAKE_node(dsql_operator, 2);
	node->nod_arg[0] = expr1;
	node->nod_arg[1] = expr2;

	return node;
}


/**

 	explode_fields

    @brief	Generate a field list that correspond to table fields.


    @param relation

 **/
static dsql_nod* explode_fields(dsql_rel* relation)
{
	DsqlNodStack stack;

	for (dsql_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		// CVC: Ann Harrison requested to skip COMPUTED fields in INSERT w/o field list.
		if (field->fld_flags & FLD_computed)
		{
			continue;
		}

		stack.push(MAKE_field_name(field->fld_name.c_str()));
	}

	return MAKE_list(stack);
}


/**

 	explode_outputs

    @brief	Generate a parameter list to correspond to procedure outputs.


    @param statement
    @param procedure

 **/
static dsql_nod* explode_outputs( CompiledStatement* statement, const dsql_prc* procedure)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(procedure, dsql_type_prc);

	const SSHORT count = procedure->prc_out_count;
	dsql_nod* node = MAKE_node(nod_list, count);
	dsql_nod** ptr = node->nod_arg;
	for (const dsql_fld* field = procedure->prc_outputs; field; field = field->fld_next, ptr++)
	{
		DEV_BLKCHK(field, dsql_type_fld);
		DEV_BLKCHK(*ptr, dsql_type_nod);
		dsql_nod* p_node = MAKE_node(nod_parameter, e_par_count);
		*ptr = p_node;
		p_node->nod_count = 0;
		dsql_par* parameter = MAKE_parameter(statement->req_receive, true, true, 0, NULL);
		p_node->nod_arg[e_par_index] = (dsql_nod*) (IPTR) parameter->par_index;
		p_node->nod_arg[e_par_parameter] = (dsql_nod*) parameter;
		MAKE_desc_from_field(&parameter->par_desc, field);
		parameter->par_name = parameter->par_alias = field->fld_name.c_str();
		parameter->par_rel_name = procedure->prc_name.c_str();
		parameter->par_owner_name = procedure->prc_owner.c_str();
	}

	return node;
}


/**

 	field_appears_once

    @brief	Check that a field is named only once in INSERT or UPDATE statements.


    @param fields
    @param old_fields
	@param is_insert

 **/
static void field_appears_once(const dsql_nod* fields, const dsql_nod* old_fields,
							   const bool is_insert, const char* statement)
{
	for (int i = 0; i < fields->nod_count; ++i)
	{
		const dsql_nod* elem1 = fields->nod_arg[i];
		if (elem1->nod_type == nod_assign && !is_insert)
			elem1 = elem1->nod_arg[e_asgn_field];

		if (elem1->nod_type == nod_field)
		{
			const Firebird::MetaName& n1 =
				reinterpret_cast<dsql_fld*>(elem1->nod_arg[e_fld_field])->fld_name;

			for (int j = i + 1; j < fields->nod_count; ++j)
			{
				const dsql_nod* elem2 = fields->nod_arg[j];
				if (elem2->nod_type == nod_assign && !is_insert)
					elem2 = elem2->nod_arg[e_asgn_field];

				if (elem2->nod_type == nod_field)
				{
					const Firebird::MetaName& n2 =
						reinterpret_cast<dsql_fld*>(elem2->nod_arg[e_fld_field])->fld_name;

					if (n1 == n2)
					{
						const dsql_ctx* tmp_ctx = (dsql_ctx*) elem2->nod_arg[e_fld_context];
						const dsql_rel* bad_rel = tmp_ctx ? tmp_ctx->ctx_relation : 0;
						field_duplication(bad_rel ? bad_rel->rel_name.c_str() : 0,
										n2.c_str(),
										is_insert ? old_fields->nod_arg[j]: old_fields->nod_arg[j]->nod_arg[1],
										statement);
					}
				}
			}
		}
	}
}


/**

 	field_duplication

    @brief	Report a field duplication error in INSERT or UPDATE statements.


    @param qualifier_name
    @param field_name
    @param flawed_node
	@param is_insert

 **/
static void field_duplication(const TEXT* qualifier_name, const TEXT* field_name,
	const dsql_nod* flawed_node, const char* statement)
{
	TEXT field_buffer[MAX_SQL_IDENTIFIER_SIZE * 2];

	if (qualifier_name)
	{
		sprintf(field_buffer, "%.*s.%.*s", (int) MAX_SQL_IDENTIFIER_LEN, qualifier_name,
				(int) MAX_SQL_IDENTIFIER_LEN, field_name);
		field_name = field_buffer;
	}

	ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
			  Arg::Gds(isc_dsql_no_dup_name) << Arg::Str(field_name) << Arg::Str(statement) <<
			  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(flawed_node->nod_line) <<
												   Arg::Num(flawed_node->nod_column));
}


/**

 	field_unknown

    @brief	Report a field parsing recognition error.


    @param qualifier_name
    @param field_name
    @param flawed_node

 **/
static void field_unknown(const TEXT* qualifier_name, const TEXT* field_name,
	const dsql_nod* flawed_node)
{
	TEXT field_buffer[MAX_SQL_IDENTIFIER_SIZE * 2];

	if (qualifier_name)
	{
		sprintf(field_buffer, "%.*s.%.*s", (int) MAX_SQL_IDENTIFIER_LEN, qualifier_name,
				(int) MAX_SQL_IDENTIFIER_LEN, field_name ? field_name : "*");
		field_name = field_buffer;
	}

	if (flawed_node)
	{
		if (field_name)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
					  Arg::Gds(isc_dsql_field_err) <<
					  Arg::Gds(isc_random) << Arg::Str(field_name) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(flawed_node->nod_line) <<
					  									   Arg::Num(flawed_node->nod_column));
		}
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
					  Arg::Gds(isc_dsql_field_err) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(flawed_node->nod_line) <<
					  									   Arg::Num(flawed_node->nod_column));
		}
	}
	else
	{
		if (field_name)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
					  Arg::Gds(isc_dsql_field_err) <<
					  Arg::Gds(isc_random) << Arg::Str(field_name) <<
					  Arg::Gds(isc_dsql_unknown_pos));
		}
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
					  Arg::Gds(isc_dsql_field_err) <<
					  Arg::Gds(isc_dsql_unknown_pos));
		}
	}
}


/**

 	find_dbkey

    @brief	Find dbkey for named relation in statement's saved dbkeys.


    @param statement
    @param relation_name

 **/
static dsql_par* find_dbkey(const CompiledStatement* statement, const dsql_nod* relation_name)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(relation_name, dsql_type_nod);

	dsql_msg* message = statement->req_receive;
	dsql_par* candidate = NULL;
	const dsql_str* rel_name = (dsql_str*) relation_name->nod_arg[e_rln_name];
	DEV_BLKCHK(rel_name, dsql_type_str);
	for (dsql_par* parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		DEV_BLKCHK(parameter, dsql_type_par);
		const dsql_ctx* context = parameter->par_dbkey_ctx;
		if (context)
		{
			DEV_BLKCHK(context, dsql_type_ctx);
			const dsql_rel* relation = context->ctx_relation;
			if (relation->rel_name == rel_name->str_data)
			{
				if (candidate)
					return NULL;

				candidate = parameter;
			}
		}
	}
	return candidate;
}


/**

 	find_record_version

    @brief	Find record version for relation in statement's saved record version


    @param statement
    @param relation_name

 **/
static dsql_par* find_record_version(const CompiledStatement* statement, const dsql_nod* relation_name)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(relation_name, dsql_type_nod);

	dsql_msg* message = statement->req_receive;
	dsql_par* candidate = NULL;
	const dsql_str* rel_name = (dsql_str*) relation_name->nod_arg[e_rln_name];
	DEV_BLKCHK(rel_name, dsql_type_str);
	for (dsql_par* parameter = message->msg_parameters; parameter; parameter = parameter->par_next)
	{
		DEV_BLKCHK(parameter, dsql_type_par);
		const dsql_ctx* context = parameter->par_rec_version_ctx;
		if (context)
		{
			DEV_BLKCHK(context, dsql_type_ctx);
			const dsql_rel* relation = context->ctx_relation;
			if (relation->rel_name == rel_name->str_data)
			{
				if (candidate)
					return NULL;

				candidate = parameter;
			}
		}
	}
	return candidate;
}


// Get the context of a relation, procedure or derived table.
static dsql_ctx* get_context(const dsql_nod* node)
{
	fb_assert(node->nod_type == nod_relation || node->nod_type == nod_derived_table);

	if (node->nod_type == nod_relation)
		return (dsql_ctx*) node->nod_arg[e_rel_context];

	// nod_derived_table
	return (dsql_ctx*) node->nod_arg[e_derived_table_context];
}


// Get the contexts of a relation, procedure, derived table or a list of joins.
static void get_contexts(DsqlContextStack& contexts, const dsql_nod* node)
{
	if (node->nod_type == nod_relation)
		contexts.push((dsql_ctx*) node->nod_arg[e_rel_context]);
	else if (node->nod_type == nod_derived_table)
		contexts.push((dsql_ctx*) node->nod_arg[e_derived_table_context]);
	else if (node->nod_type == nod_join)
	{
		get_contexts(contexts, node->nod_arg[e_join_left_rel]);
		get_contexts(contexts, node->nod_arg[e_join_rght_rel]);
	}
	else
	{
		fb_assert(false);
	}
}


#ifdef NOT_USED_OR_REPLACED
/**

 	get_object_and_field

    @brief	Get the relation/procedure's name and field's name from a context


    @param node
    @param obj_name
    @param fld_name
    @param do_collation

 **/
static bool get_object_and_field(const dsql_nod* node,
	const char** obj_name, const char** fld_name, bool do_collation)
{
	switch (node->nod_type)
	{
	case nod_field:
		break;
	case nod_derived_field:
		node = node->nod_arg[e_derived_field_value];
		break;
	case nod_via:
		node = node->nod_arg[e_via_value_1];
		break;
	case nod_cast:
		if (!do_collation)
			return false;
		node = node->nod_arg[e_cast_source];
		break;
	default: // nod_alias, nod_constant, expressions, etc
		return false;
	}

	if (node->nod_type != nod_field)
		return false;

	const dsql_ctx* context = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_fld_context]);
	DEV_BLKCHK(context, dsql_type_ctx);

	if (context->ctx_relation)
		*obj_name = context->ctx_relation->rel_name.c_str();
	else if (context->ctx_procedure)
		*obj_name = context->ctx_procedure->prc_name.c_str();
	else
		*obj_name = NULL;

	*fld_name = reinterpret_cast<dsql_fld*>(node->nod_arg[e_fld_field])->fld_name;

	return obj_name && fld_name;
}
#endif


/**

 	invalid_reference

    @brief	Validate that an expanded field / context
 	pair is in a specified list.  Thus is used
 	in one instance to check that a simple field selected
 	through a grouping rse is a grouping field -
 	thus a valid field reference.  For the sake of
       argument, we'll match qualified to unqualified
 	reference, but qualified reference must match
 	completely.

 	A list element containing a simple CAST for collation purposes
 	is allowed.


    @param context
    @param node
    @param list
    @param inside_map

 **/
static bool invalid_reference(const dsql_ctx* context, const dsql_nod* node,
							  const dsql_nod* list, bool inside_own_map,
							  bool inside_higher_map)
{
	DEV_BLKCHK(node, dsql_type_nod);
	DEV_BLKCHK(list, dsql_type_nod);

	if (node == NULL) {
		return false;
	}

	bool invalid = false;

	if (list)
	{
		// Check if this node (with ignoring of CASTs) appear also
		// in the list of group by. If yes then it's allowed
		const dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			if (node_match(node, *ptr, true)) {
				return false;
			}
		}
	}

	switch (node->nod_type)
	{
		default:
			fb_assert(false);
			// FALLINTO

		case nod_map:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_map_context]);
				const dsql_map* lmap = reinterpret_cast<dsql_map*>(node->nod_arg[e_map_map]);
				if (lcontext->ctx_scope_level == context->ctx_scope_level) {
					invalid |= invalid_reference(context, lmap->map_node, list, true, false);
				}
				else {
					bool linside_higher_map = lcontext->ctx_scope_level > context->ctx_scope_level;
					invalid |= invalid_reference(context, lmap->map_node, list, false, linside_higher_map);
				}
			}
			break;

		case nod_field:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_fld_context]);

				// Wouldn't it be better to call a error from this
				// point where return is true. Then we could give
				// the fieldname that's making the trouble

				// If we come here then this Field is used inside a
				// aggregate-function. The ctx_scope_level gives the
				// info how deep the context is inside the statement.

				// If the context-scope-level from this field is
				// lower or the same as the scope-level from the
				// given context then it is an invalid field
				if (lcontext->ctx_scope_level == context->ctx_scope_level) {
					// Return TRUE (invalid) if this Field isn't inside
					// the GROUP BY clause, that should already been
					// seen in the match_node above
					invalid = true;
				}
			}
			break;

		case nod_dbkey:
			if (node->nod_arg[0] && node->nod_arg[0]->nod_type == nod_relation)
			{
				const dsql_ctx* lcontext =
					reinterpret_cast<dsql_ctx*>(node->nod_arg[0]->nod_arg[e_rel_context]);

				if (lcontext && lcontext->ctx_scope_level == context->ctx_scope_level)
					invalid = true;
			}
			break;

		case nod_agg_count:
		case nod_agg_average:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_list:
			if (!inside_own_map) {
				// We are not in an aggregate from the same scope_level so
				// check for valid fields inside this aggregate
				if (node->nod_count) {
					invalid |= invalid_reference(context, node->nod_arg[e_agg_function_expression], list,
												 inside_own_map, inside_higher_map);
				}
			}
			if (!inside_higher_map)
			{
				if (node->nod_count) {
					// If there's another aggregate with the same scope_level or
					// an higher one then it's a invalid aggregate, because
					// aggregate-functions from the same context can't
					// be part of each other.
					if (pass1_found_aggregate(node->nod_arg[e_agg_function_expression],
											  context->ctx_scope_level, FIELD_MATCH_TYPE_EQUAL, true))
					{
						// Nested aggregate functions are not allowed
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
								  Arg::Gds(isc_dsql_agg_nested_err));
					}
				}
			}
			break;

		case nod_gen_id:
		case nod_gen_id2:
		case nod_cast:
		case nod_udf:
		case nod_sys_function:
			// If there are no arguments given to the UDF/SDF then it's always valid
			if (node->nod_count == 2) {
				invalid |= invalid_reference(context, node->nod_arg[1], list,
											 inside_own_map, inside_higher_map);
			}
			break;

		case nod_via:
		case nod_exists:
		case nod_singular:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
				{
					invalid |= invalid_reference(context, *ptr, list, inside_own_map, inside_higher_map);
				}
			}
			break;

		case nod_order:
			invalid |= invalid_reference(context, node->nod_arg[e_order_field], list,
										 inside_own_map, inside_higher_map);
			break;

		case nod_coalesce:
		case nod_simple_case:
		case nod_searched_case:
		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_subtract2:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_strlen:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_leq_any:
		case nod_lss_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_leq_all:
		case nod_lss_all:
		case nod_between:
		case nod_like:
		case nod_missing:
		case nod_and:
		case nod_or:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_not:
		case nod_unique:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_rse:
		case nod_join:
		case nod_join_inner:
		case nod_join_left:
		case nod_join_right:
		case nod_join_full:
		case nod_list:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
				{
					invalid |= invalid_reference(context, *ptr, list, inside_own_map, inside_higher_map);
				}
			}
			break;

		case nod_alias:
				invalid |= invalid_reference(context, node->nod_arg[e_alias_value],
											 list, inside_own_map, inside_higher_map);
			break;

		case nod_derived_field:
			{
				const USHORT lscope_level = (USHORT)(U_IPTR) node->nod_arg[e_derived_field_scope];
				if (lscope_level == context->ctx_scope_level) {
					invalid |= true;
				}
				else if (context->ctx_scope_level < lscope_level) {
					invalid |= invalid_reference(context, node->nod_arg[e_derived_field_value],
												 list, inside_own_map, inside_higher_map);
				}
			}
			break;

		case nod_aggregate:
			invalid |= invalid_reference(context, node->nod_arg[e_agg_rse],
										 list, inside_own_map, inside_higher_map);
			break;

		case nod_relation:
			{
				const dsql_ctx* lrelation_context = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_rel_context]);
				// Check if relation is a procedure
				if (lrelation_context->ctx_procedure) {
					// Check if the parameters are valid
					invalid |= invalid_reference(context, lrelation_context->ctx_proc_inputs,
												 list, inside_own_map, inside_higher_map);
				}
			}
			break;

		case nod_variable:
		case nod_constant:
		case nod_parameter:
		case nod_null:
		case nod_current_date:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_user_name:
		case nod_current_role:
		case nod_internal_info:
		case nod_dom_value:
		case nod_derived_table:
		case nod_plan_expr:
			return false;

		case nod_hidden_var:
			invalid |= invalid_reference(context, node->nod_arg[e_hidden_var_expr],
										 list, inside_own_map, inside_higher_map);
	}

	return invalid;
}


/**

 	node_match

    @brief	Compare two nodes for equality of value.

   [2002-08-04]--- Arno Brinkman
 	If ignore_map_cast is true and the node1 is
   type nod_cast or nod_map then node_match is
   calling itselfs again with the node1
   CASTs source or map->node.
   This is for allow CAST to other datatypes
   without complaining that it's an unknown
   column reference. (Aggregate functions)


    @param node1
    @param node2
    @param ignore_map_cast

 **/
static bool node_match(const dsql_nod* node1, const dsql_nod* node2,
	bool ignore_map_cast)
{
	DEV_BLKCHK(node1, dsql_type_nod);
	DEV_BLKCHK(node2, dsql_type_nod);

	if (!node1 && !node2) {
		return true;
	}

	if (!node1 || !node2) {
		return false;
	}

	if (node1->nod_type == nod_hidden_var)
		node1 = node1->nod_arg[e_hidden_var_expr];

	if (node2->nod_type == nod_hidden_var)
		node2 = node2->nod_arg[e_hidden_var_expr];

	if (ignore_map_cast && node1->nod_type == nod_cast)
	{
		// If node2 is also cast and same type continue with both sources.
		if (node2->nod_type == nod_cast &&
			node1->nod_desc.dsc_dtype == node2->nod_desc.dsc_dtype &&
			node1->nod_desc.dsc_scale == node2->nod_desc.dsc_scale &&
			node1->nod_desc.dsc_length == node2->nod_desc.dsc_length &&
			node1->nod_desc.dsc_sub_type == node2->nod_desc.dsc_sub_type)
		{
			return node_match(node1->nod_arg[e_cast_source], node2->nod_arg[e_cast_source], ignore_map_cast);
		}

		return node_match(node1->nod_arg[e_cast_source], node2, ignore_map_cast);
	}

	if (ignore_map_cast && node1->nod_type == nod_map)
	{
		const dsql_map* map1 = (dsql_map*)node1->nod_arg[e_map_map];
		DEV_BLKCHK(map1, dsql_type_map);
		if (node2->nod_type == nod_map)
		{
			const dsql_map* map2 = (dsql_map*)node2->nod_arg[e_map_map];
			DEV_BLKCHK(map2, dsql_type_map);
			if (node1->nod_arg[e_map_context] != node2->nod_arg[e_map_context]) {
				return false;
			}
			return node_match(map1->map_node, map2->map_node, ignore_map_cast);
		}

		return node_match(map1->map_node, node2, ignore_map_cast);
	}

	// We don't care about the alias itself but only about its field.
	if ((node1->nod_type == nod_alias) || (node2->nod_type == nod_alias))
	{
		if ((node1->nod_type == nod_alias) && (node2->nod_type == nod_alias))
		{
			return node_match(node1->nod_arg[e_alias_value],
							  node2->nod_arg[e_alias_value], ignore_map_cast);
		}

		if (node1->nod_type == nod_alias) {
			return node_match(node1->nod_arg[e_alias_value], node2, ignore_map_cast);
		}
		if (node2->nod_type == nod_alias) {
			return node_match(node1, node2->nod_arg[e_alias_value], ignore_map_cast);
		}
	}

	// Handle derived fields.
	if ((node1->nod_type == nod_derived_field) || (node2->nod_type == nod_derived_field))
	{
		if ((node1->nod_type == nod_derived_field) && (node2->nod_type == nod_derived_field))
		{
			const dsql_ctx* context1 = (dsql_ctx*) node1->nod_arg[e_derived_field_context];
			const dsql_ctx* context2 = (dsql_ctx*) node2->nod_arg[e_derived_field_context];
			if (context1->ctx_context != context2->ctx_context)
				return false;

			const dsql_str* alias1 = (dsql_str*) node1->nod_arg[e_derived_field_name];
			const dsql_str* alias2 = (dsql_str*) node2->nod_arg[e_derived_field_name];
			if (strcmp(alias1->str_data, alias2->str_data))
				return false;

			return node_match(node1->nod_arg[e_derived_field_value],
							  node2->nod_arg[e_derived_field_value], ignore_map_cast);
		}

		if (node1->nod_type == nod_derived_field) {
			return node_match(node1->nod_arg[e_derived_field_value], node2, ignore_map_cast);
		}
		if (node2->nod_type == nod_derived_field) {
			return node_match(node1, node2->nod_arg[e_derived_field_value], ignore_map_cast);
		}
	}

	if ((node1->nod_type != node2->nod_type) || (node1->nod_count != node2->nod_count)) {
		return false;
	}

	// This is to get rid of assertion failures when trying
	// to node_match nod_aggregate's children. This was happening because not
	// all of the children are of type "dsql_nod". Pointer to the first child
	// (argument) is actually a pointer to context structure.
	// To compare two nodes of type nod_aggregate we need first to see if they
	// both refer to same context structure. If they do not they are different
	// nodes, if they point to the same context they are the same (because
	// nod_aggregate is created for an rse that have aggregate expression,
	// group by or having clause and each rse has its own context). But just in
	// case we compare two other subtrees.

	switch (node1->nod_type)
	{
	case nod_aggregate:

		if (node1->nod_arg[e_agg_context] != node2->nod_arg[e_agg_context]) {
			return false;
		}

		return node_match(node1->nod_arg[e_agg_group], node2->nod_arg[e_agg_group], ignore_map_cast) &&
			node_match(node1->nod_arg[e_agg_rse], node2->nod_arg[e_agg_rse], ignore_map_cast);

	case nod_relation:
		if (node1->nod_arg[e_rel_context] != node2->nod_arg[e_rel_context]) {
			return false;
		}
		return true;

	case nod_field:
		if (node1->nod_arg[e_fld_field] != node2->nod_arg[e_fld_field] ||
			node1->nod_arg[e_fld_context] != node2->nod_arg[e_fld_context])
		{
			return false;
		}
		if (node1->nod_arg[e_fld_indices] || node2->nod_arg[e_fld_indices]) {
			return node_match(node1->nod_arg[e_fld_indices],
							  node2->nod_arg[e_fld_indices], ignore_map_cast);
		}
		return true;

	case nod_constant:
		if (!DSC_EQUIV(&node1->nod_desc, &node2->nod_desc, true))
			return false;

		{ // scope for the local vars
			const USHORT len = (node1->nod_desc.dsc_dtype == dtype_text) ?
				((dsql_str*) node1->nod_arg[0])->str_length : node1->nod_desc.dsc_length;
			return !memcmp(node1->nod_desc.dsc_address, node2->nod_desc.dsc_address, len);
		}

	case nod_map:
		{
			const dsql_map* map1 = (dsql_map*)node1->nod_arg[e_map_map];
			const dsql_map* map2 = (dsql_map*)node2->nod_arg[e_map_map];
			DEV_BLKCHK(map1, dsql_type_map);
			DEV_BLKCHK(map2, dsql_type_map);
			return node_match(map1->map_node, map2->map_node, ignore_map_cast);
		}

	case nod_gen_id:
	case nod_gen_id2:
	case nod_udf:
	case nod_sys_function:
	case nod_cast:
		if (node1->nod_arg[0] != node2->nod_arg[0]) {
			return false;
		}

		if (node1->nod_count == 2) {
			return node_match(node1->nod_arg[1], node2->nod_arg[1], ignore_map_cast);
		}
		return true;

	case nod_agg_count:
	case nod_agg_total:
	case nod_agg_total2:
	case nod_agg_average2:
	case nod_agg_average:
	case nod_agg_list:
		if ((node1->nod_flags & NOD_AGG_DISTINCT) != (node2->nod_flags & NOD_AGG_DISTINCT))
		{
			return false;
		}
		break;

	case nod_variable:
		{
			const dsql_var* var1 = reinterpret_cast<dsql_var*>(node1->nod_arg[e_var_variable]);
			const dsql_var* var2 = reinterpret_cast<dsql_var*>(node2->nod_arg[e_var_variable]);
			DEV_BLKCHK(var1, dsql_type_var);
			DEV_BLKCHK(var2, dsql_type_var);
			if ((strcmp(var1->var_name, var2->var_name))					||
				(var1->var_field != var2->var_field)						||
				(var1->var_variable_number != var2->var_variable_number)	||
				(var1->var_msg_item != var2->var_msg_item)					||
				(var1->var_msg_number != var2->var_msg_number))
			{
				return false;
			}
			return true;
		}

	case nod_parameter:
		{
			// Parameters are equal when there index is the same
			const dsql_par* parameter1 = (dsql_par*) node1->nod_arg[e_par_parameter];
			const dsql_par* parameter2 = (dsql_par*) node2->nod_arg[e_par_parameter];
			return (parameter1->par_index == parameter2->par_index);
		}

	case nod_derived_table:
		{
			const dsql_ctx* ctx1 = (dsql_ctx*) node1->nod_arg[e_derived_table_context];
			const dsql_ctx* ctx2 = (dsql_ctx*) node2->nod_arg[e_derived_table_context];
			fb_assert(ctx1 && ctx2);

			if (ctx1->ctx_context != ctx2->ctx_context)
				return false;

			// hvlad: i'm not sure if we need comparisons below. I.e. if two DT have the
			// same context number how it can be different in other aspects ?

			const dsql_str* alias1 = (dsql_str*) node1->nod_arg[e_derived_table_alias];
			const dsql_str* alias2 = (dsql_str*) node2->nod_arg[e_derived_table_alias];
			if ((alias1 && !alias2) || (!alias1 && alias2))
				return false;

			if (alias1 && (alias1->str_charset != alias2->str_charset ||
					alias1->str_length != alias2->str_length ||
					strncmp(alias1->str_data, alias2->str_data, alias1->str_length)))
			{
				return false;
			}

			return node_match(node1->nod_arg[e_derived_table_rse],
				node2->nod_arg[e_derived_table_rse], ignore_map_cast);
		}
	} // switch

	const dsql_nod* const* ptr1 = node1->nod_arg;
	const dsql_nod* const* ptr2 = node2->nod_arg;

	for (const dsql_nod* const* const end = ptr1 + node1->nod_count; ptr1 < end; ptr1++, ptr2++)
	{
		if (!node_match(*ptr1, *ptr2, ignore_map_cast)) {
			return false;
		}
	}

	return true;
}


/**

 	nullify_returning

    @brief	Create a compound statement to initialize returning parameters.

    @param input

 **/
static dsql_nod* nullify_returning(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* returning = NULL;

	switch (input->nod_type)
	{
		case nod_store:
			returning = input->nod_arg[e_sto_return];
			break;
		case nod_modify:
			returning = input->nod_arg[e_mod_return];
			break;
		case nod_erase:
			returning = input->nod_arg[e_era_return];
			break;
		default:
			fb_assert(false);
	}

	if (statement->isPsql() || !returning)
		return input;

	// If this is a RETURNING in DSQL, we need to initialize the output
	// parameters with NULL, to return in case of empty resultset.
	// Note: this may be changed in the future, i.e. return empty resultset
	// instead of NULLs. In this case, I suppose this function could be
	// completely removed.

	// nod_returning was already processed
	fb_assert(returning->nod_type == nod_list);

	dsql_nod* null_assign = MAKE_node(nod_list, returning->nod_count);

	dsql_nod** ret_ptr = returning->nod_arg;
	dsql_nod** null_ptr = null_assign->nod_arg;
	dsql_nod* temp;

	for (const dsql_nod* const* const end = ret_ptr + returning->nod_count; ret_ptr < end;
		++ret_ptr, ++null_ptr)
	{
		temp = MAKE_node(nod_assign, e_asgn_count);
		temp->nod_arg[e_asgn_value] = MAKE_node(nod_null, 0);
		temp->nod_arg[e_asgn_field] = (*ret_ptr)->nod_arg[1];
		*null_ptr = temp;
	}

	temp = MAKE_node(nod_list, 2);
	temp->nod_arg[0] = null_assign;
	temp->nod_arg[1] = input;

	return temp;
}


/**

 	pass1_any

    @brief	Compile a parsed statement into something more interesting.


    @param statement
    @param input
    @param ntype

 **/
static dsql_nod* pass1_any( CompiledStatement* statement, dsql_nod* input, NOD_TYPE ntype)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	// create a derived table representing our subquery
	dsql_nod* dt = MAKE_node(nod_derived_table, e_derived_table_count);
	// Ignore validation for columnames that must exist for "user" derived tables.
	dt->nod_flags |= NOD_DT_IGNORE_COLUMN_CHECK;
	dt->nod_arg[e_derived_table_rse] = input->nod_arg[1];
	dsql_nod* from = MAKE_node(nod_list, 1);
	from->nod_arg[0] = dt;
	dsql_nod* query_spec = MAKE_node(nod_query_spec, e_qry_count);
	query_spec->nod_arg[e_qry_from] = from;
	dsql_nod* select_expr = MAKE_node(nod_select_expr, e_sel_count);
	select_expr->nod_arg[e_sel_query_spec] = query_spec;

	const DsqlContextStack::iterator base(*statement->req_context);
	const DsqlContextStack::iterator baseDT(statement->req_dt_context);
	const DsqlContextStack::iterator baseUnion(statement->req_union_context);

	dsql_nod* rse = PASS1_rse(statement, select_expr, NULL);

	// create a conjunct to be injected
	dsql_nod* temp = MAKE_node(input->nod_type, 2);
	temp->nod_arg[0] = pass1_node_psql(statement, input->nod_arg[0], false);
	temp->nod_arg[1] = rse->nod_arg[e_rse_items]->nod_arg[0];

	rse->nod_arg[e_rse_boolean] = temp;

	// create output node
	dsql_nod* node = MAKE_node(ntype, 1);
	node->nod_arg[0] = rse;

	// Finish off by cleaning up contexts
	statement->req_union_context.clear(baseUnion);
	statement->req_dt_context.clear(baseDT);
	statement->req_context->clear(base);

	return node;
}


/**

 	pass1_blob

    @brief	Process a blob get or put segment.


    @param statement
    @param input

 **/
static void pass1_blob( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	thread_db* tdbb = JRD_get_thread_data();

	PASS1_make_context(statement, input->nod_arg[e_blb_relation]);
	dsql_nod* field = pass1_field(statement, input->nod_arg[e_blb_field], false, NULL);
	if (field->nod_desc.dsc_dtype != dtype_blob)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
				  Arg::Gds(isc_dsql_blob_err));
	}

	statement->req_type = (input->nod_type == nod_get_segment) ? REQ_GET_SEGMENT : REQ_PUT_SEGMENT;

	dsql_blb* blob = FB_NEW(*tdbb->getDefaultPool()) dsql_blb;
	statement->req_blob = blob;
	//blob->blb_field = field;
	blob->blb_open_in_msg = statement->req_send;
	blob->blb_open_out_msg = FB_NEW(*tdbb->getDefaultPool()) dsql_msg;
	blob->blb_segment_msg = statement->req_receive;

	// Create a parameter for the blob segment

	dsql_par* parameter = MAKE_parameter(blob->blb_segment_msg, true, true, 0, NULL);
	blob->blb_segment = parameter;
	parameter->par_desc.dsc_dtype = dtype_text;
	parameter->par_desc.dsc_ttype() = ttype_binary;
	parameter->par_desc.dsc_length = ((dsql_fld*) field->nod_arg[e_fld_field])->fld_seg_length;
	DEV_BLKCHK(field->nod_arg[e_fld_field], dsql_type_fld);

	// The Null indicator is used to pass back the segment length,
	// set DSC_nullable so that the SQL_type is set to SQL_TEXT+1 instead
	// of SQL_TEXT.

	if (input->nod_type == nod_get_segment)
		parameter->par_desc.dsc_flags |= DSC_nullable;

	// Create a parameter for the blob id

	dsql_msg* temp_msg = (input->nod_type == nod_get_segment) ?
		blob->blb_open_in_msg : blob->blb_open_out_msg;
	blob->blb_blob_id = parameter = MAKE_parameter(temp_msg, true, true, 0, NULL);
	MAKE_desc(statement, &parameter->par_desc, field, NULL);
	parameter->par_desc.dsc_dtype = dtype_quad;
	parameter->par_desc.dsc_scale = 0;

	dsql_nod* list = input->nod_arg[e_blb_filter];
	if (list) {
		if (list->nod_arg[0]) {
			blob->blb_from = pass1_node_psql(statement, list->nod_arg[0], false);
		}
		if (list->nod_arg[1]) {
			blob->blb_to = pass1_node_psql(statement, list->nod_arg[1], false);
		}
	}
	if (!blob->blb_from) {
		blob->blb_from = MAKE_const_slong(0);
	}
	if (!blob->blb_to) {
		blob->blb_to = MAKE_const_slong(0);
	}

	for (parameter = blob->blb_open_in_msg->msg_parameters; parameter; parameter = parameter->par_next)
	{
		if (parameter->par_index > ((input->nod_type == nod_get_segment) ? 1 : 0))
		{
			parameter->par_desc.dsc_dtype = dtype_short;
			parameter->par_desc.dsc_scale = 0;
			parameter->par_desc.dsc_length = sizeof(SSHORT);
		}
	}
}


/**

 	pass1_coalesce

    @brief	Handle a reference to a coalesce function.

	COALESCE(expr-1, expr-2 [, expr-n])
	is the same as :
	CASE WHEN (expr-1 IS NULL) THEN expr-2 ELSE expr-1 END

    @param statement
    @param input

 **/
static dsql_nod* pass1_coalesce( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(input->nod_arg[0], dsql_type_nod);

	dsql_nod* node = MAKE_node(nod_coalesce, 2);
	node->nod_count = 1;	// we do not want to reprocess the second list later

	// Pass list of arguments 2..n on stack and make a list from it
	DsqlNodStack stack;
	pass1_put_args_on_stack(statement, input->nod_arg[0], stack);
	pass1_put_args_on_stack(statement, input->nod_arg[1], stack);
	node->nod_arg[0] = MAKE_list(stack);

	DsqlNodStack stack2;
	dsql_nod** ptr = node->nod_arg[0]->nod_arg;
	const dsql_nod* const* end = ptr + node->nod_arg[0]->nod_count;

	for (; ptr < end - 1; ptr++)
	{
		dsql_nod* var = pass1_hidden_variable(statement, *ptr);

		if (var)
			stack2.push(var);
		else
			stack2.push(*ptr);
	}

	node->nod_arg[1] = MAKE_list(stack2);

	// Set descriptor for output node
	MAKE_desc(statement, &node->nod_desc, node, NULL);

	// Set parameter-types if parameters are there
	ptr = node->nod_arg[0]->nod_arg;
	for (; ptr < end; ptr++) {
		set_parameter_type(statement, *ptr, node, false);
	}
	ptr = node->nod_arg[1]->nod_arg;
	end = ptr + node->nod_arg[1]->nod_count;
	for (; ptr < end; ptr++) {
		set_parameter_type(statement, *ptr, node, false);
	}

	return node;
}


/**

 	pass1_collate

    @brief	Turn a collate clause into a cast clause.
 	If the source is not already text, report an error.
 	(SQL 92: Section 13.1, pg 308, item 11)


    @param statement
    @param sub1
    @param collation

 **/
static dsql_nod* pass1_collate( CompiledStatement* statement, dsql_nod* sub1,
	const dsql_str* collation)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(sub1, dsql_type_nod);
	DEV_BLKCHK(collation, dsql_type_str);

	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = MAKE_node(nod_cast, e_cast_count);
	dsql_fld* field = FB_NEW(*tdbb->getDefaultPool()) dsql_fld(*tdbb->getDefaultPool());
	node->nod_arg[e_cast_target] = (dsql_nod*) field;
	node->nod_arg[e_cast_source] = sub1;
	MAKE_desc(statement, &sub1->nod_desc, sub1, NULL);

	if (sub1->nod_desc.dsc_dtype <= dtype_any_text ||
		(sub1->nod_desc.dsc_dtype == dtype_blob && sub1->nod_desc.dsc_sub_type == isc_blob_text))
	{
		assign_fld_dtype_from_dsc(field, &sub1->nod_desc);
		field->fld_character_length = 0;
	}
	else {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
				  Arg::Gds(isc_dsql_datatype_err) <<
				  Arg::Gds(isc_collation_requires_text));
	}
	DDL_resolve_intl_type(statement, field, collation);
	MAKE_desc_from_field(&node->nod_desc, field);
	return node;
}


/**

 	pass1_constant

    @brief	Turn an international string reference into internal
 	subtype ID.


    @param statement
    @param constant

 **/
static dsql_nod* pass1_constant( CompiledStatement* statement, dsql_nod* input)
{
	thread_db* tdbb = JRD_get_thread_data();

	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	if (statement->req_in_outer_join)
		input->nod_desc.dsc_flags = DSC_nullable;

	if (input->nod_desc.dsc_dtype > dtype_any_text) {
		return input;
	}

	dsql_nod* constant = MAKE_node(input->nod_type, 1);
	constant->nod_arg[0] = input->nod_arg[0];
	constant->nod_desc = input->nod_desc;

	const dsql_str* string = (dsql_str*) constant->nod_arg[0];
	DEV_BLKCHK(string, dsql_type_str);

	if (string && string->str_charset)
	{
		const dsql_intlsym* resolved =
			METD_get_charset(statement, strlen(string->str_charset), string->str_charset);
		if (!resolved)
		{
			// character set name is not defined
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_charset_not_found) << Arg::Str(string->str_charset));
		}

		if (global_temp_collation_name)
		{
			const dsql_intlsym* resolved_collation =
				METD_get_collation(statement, global_temp_collation_name, resolved->intlsym_charset_id);
			if (!resolved_collation)
			{
				// Specified collation not found
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(isc_dsql_datatype_err) <<
						  Arg::Gds(isc_collation_not_found) << Arg::Str(global_temp_collation_name->str_data) <<
															   Arg::Str(resolved->intlsym_name));
			}
			resolved = resolved_collation;
		}

		INTL_ASSIGN_TTYPE(&constant->nod_desc, resolved->intlsym_ttype);
	}
	else
	{
		const Firebird::MetaName charSetName =
			METD_get_charset_name(statement, constant->nod_desc.getCharSet());

		const dsql_intlsym* sym = METD_get_charset(statement, charSetName.length(), charSetName.c_str());
		fb_assert(sym);

		if (sym)
			constant->nod_desc.setTextType(sym->intlsym_ttype);
	}

	USHORT adjust = 0;
	if (constant->nod_desc.dsc_dtype == dtype_varying)
		adjust = sizeof(USHORT);
	else if (constant->nod_desc.dsc_dtype == dtype_cstring)
		adjust = 1;

	constant->nod_desc.dsc_length -= adjust;

	CharSet* charSet = INTL_charset_lookup(tdbb, INTL_GET_CHARSET(&constant->nod_desc));

	if (!charSet->wellFormed(string->str_length, constant->nod_desc.dsc_address, NULL))
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_malformed_string));
	}
	else
	{
		constant->nod_desc.dsc_length =
			charSet->length(string->str_length, constant->nod_desc.dsc_address, true) *
			charSet->maxBytesPerChar();
	}

	constant->nod_desc.dsc_length += adjust;

	return constant;
}


/**

 	pass1_cursor_context

    @brief	Turn a cursor reference into a record selection expression.


    @param statement
    @param cursor
    @param relation_name

 **/
static dsql_ctx* pass1_cursor_context( CompiledStatement* statement, const dsql_nod* cursor,
	const dsql_nod* relation_name)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(cursor, dsql_type_nod);
	DEV_BLKCHK(relation_name, dsql_type_nod);

	const dsql_str* rname = (dsql_str*) relation_name->nod_arg[e_rln_name];
	DEV_BLKCHK(rname, dsql_type_str);

	const dsql_str* string = (dsql_str*) cursor->nod_arg[e_cur_name];
	DEV_BLKCHK(string, dsql_type_str);

	// this function must throw an error if no cursor was found
	const dsql_nod* node = pass1_cursor_name(statement, string, NOD_CURSOR_ALL, true);
	fb_assert(node);

	const dsql_nod* rse = node->nod_arg[e_cur_rse];
	fb_assert(rse);

	if (rse->nod_arg[e_rse_reduced]) {
		// cursor with DISTINCT is not updatable
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-510) <<
				  Arg::Gds(isc_dsql_cursor_update_err) << Arg::Str(string->str_data));
	}

	const dsql_nod* temp = rse->nod_arg[e_rse_streams];

	dsql_ctx* context = NULL;
	dsql_nod* const* ptr = temp->nod_arg;
	for (const dsql_nod* const* const end = ptr + temp->nod_count; ptr < end; ptr++)
	{
		DEV_BLKCHK(*ptr, dsql_type_nod);
		dsql_nod* r_node = *ptr;
		if (r_node->nod_type == nod_relation)
		{
			dsql_ctx* candidate = (dsql_ctx*) r_node->nod_arg[e_rel_context];
			DEV_BLKCHK(candidate, dsql_type_ctx);
			const dsql_rel* relation = candidate->ctx_relation;
			DEV_BLKCHK(rname, dsql_type_str);
			if (relation->rel_name == rname->str_data)
			{
				if (context)
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
							  Arg::Gds(isc_dsql_cursor_err) <<
							  Arg::Gds(isc_dsql_cursor_rel_ambiguous) << Arg::Str(rname->str_data) <<
							  											 Arg::Str(string->str_data));
				}
				else
					context = candidate;
			}
		}
		else if (r_node->nod_type == nod_aggregate)
		{
			// cursor with aggregation is not updatable
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-510) <<
					  Arg::Gds(isc_dsql_cursor_update_err) << Arg::Str(string->str_data));
		}
		// note that nod_union and nod_join will cause the error below,
		// as well as derived tables. Some cases deserve fixing in the future
	}

	if (!context)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_dsql_cursor_err) <<
				  Arg::Gds(isc_dsql_cursor_rel_not_found) << Arg::Str(rname->str_data) <<
															 Arg::Str(string->str_data));
	}

	return context;
}


/**

 	pass1_cursor_name

    @brief	Find a cursor.


    @param statement
    @param string
	@param mask
	@param existence_flag

 **/
static dsql_nod* pass1_cursor_name(CompiledStatement* statement, const dsql_str* string,
	USHORT mask, bool existence_flag)
{
	DEV_BLKCHK(string, dsql_type_str);
	dsql_nod* cursor = NULL;

	if (!strlen(string->str_data))
	{
		if (existence_flag) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_dsql_cursor_err) <<
					  Arg::Gds(isc_dsql_cursor_invalid));
		}
		else {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
					  Arg::Gds(isc_dsql_decl_err) <<
					  Arg::Gds(isc_dsql_cursor_invalid));
		}
	}

	for (DsqlNodStack::iterator itr(statement->req_cursors); itr.hasData(); ++itr) {
		cursor = itr.object();
		const dsql_str* cname = (dsql_str*) cursor->nod_arg[e_cur_name];
		if (!strcmp(string->str_data, cname->str_data) && (cursor->nod_flags & mask))
			break;
		cursor = NULL;
	}

	if (!cursor && existence_flag) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_dsql_cursor_err) <<
				  Arg::Gds(isc_dsql_cursor_not_found) << Arg::Str(string->str_data));
	}
	else if (cursor && !existence_flag) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_exists) << Arg::Str(string->str_data));
	}

	return cursor;
}


/**

 	pass1_cursor_reference

    @brief	Turn a cursor reference into a record selection expression.


    @param statement
    @param cursor
    @param relation_name

 **/
static dsql_nod* pass1_cursor_reference( CompiledStatement* statement,
	const dsql_nod* cursor, dsql_nod* relation_name)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(cursor, dsql_type_nod);
	DEV_BLKCHK(relation_name, dsql_type_nod);

	// Lookup parent statement

	const dsql_str* string = (dsql_str*) cursor->nod_arg[e_cur_name];
	DEV_BLKCHK(string, dsql_type_str);

	const dsql_sym* symbol =
		HSHD_lookup(statement->req_dbb, string->str_data, static_cast<SSHORT>(string->str_length),
					SYM_cursor, 0);

	if (!symbol) {
		// cursor is not found
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_dsql_cursor_err) <<
				  Arg::Gds(isc_dsql_cursor_not_found) << Arg::Str(string->str_data));
	}

	CompiledStatement* parent = (CompiledStatement*) symbol->sym_object;

	// Verify that the cursor is appropriate and updatable

	dsql_par* rv_source = find_record_version(parent, relation_name);

	dsql_par* source;
	if (parent->req_type != REQ_SELECT_UPD || !(source = find_dbkey(parent, relation_name)) ||
		!rv_source)
	{
		// cursor is not updatable
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-510) <<
				  Arg::Gds(isc_dsql_cursor_update_err) << Arg::Str(string->str_data));
	}

	statement->req_parent = parent;
	statement->req_parent_dbkey = source;
	statement->req_parent_rec_version = rv_source;
	statement->req_sibling = parent->req_offspring;
	parent->req_offspring = statement;

	// Build record selection expression

	dsql_nod* rse = MAKE_node(nod_rse, e_rse_count);
	dsql_nod* temp = MAKE_node(nod_list, 1);
	rse->nod_arg[e_rse_streams] = temp;
	dsql_nod* relation_node = pass1_relation(statement, relation_name);
	temp->nod_arg[0] = relation_node;
	dsql_nod* node = MAKE_node(nod_eql, 2);
	rse->nod_arg[e_rse_boolean] = node;
	node->nod_arg[0] = temp = MAKE_node(nod_dbkey, 1);
	temp->nod_arg[0] = relation_node;

	node->nod_arg[1] = temp = MAKE_node(nod_parameter, e_par_count);
	temp->nod_count = 0;
	dsql_par* parameter = statement->req_dbkey =
		MAKE_parameter(statement->req_send, false, false, 0, NULL);
	temp->nod_arg[e_par_index] = (dsql_nod*) (IPTR) parameter->par_index;
	temp->nod_arg[e_par_parameter] = (dsql_nod*) parameter;
	parameter->par_desc = source->par_desc;

	// record version will be set only for V4 - for the parent select cursor
	if (rv_source)
	{
		node = MAKE_node(nod_eql, 2);
		node->nod_arg[0] = temp = MAKE_node(nod_rec_version, 1);
		temp->nod_arg[0] = relation_node;
		node->nod_arg[1] = temp = MAKE_node(nod_parameter, e_par_count);
		temp->nod_count = 0;
		parameter = statement->req_rec_version =
			MAKE_parameter(statement->req_send, false, false, 0, NULL);
		temp->nod_arg[e_par_index] = (dsql_nod*) (IPTR) parameter->par_index;
		temp->nod_arg[e_par_parameter] = (dsql_nod*) parameter;
		parameter->par_desc = rv_source->par_desc;

		rse->nod_arg[e_rse_boolean] = compose(rse->nod_arg[e_rse_boolean], node, nod_and);
	}

	return rse;
}


/**

 	pass1_dbkey

    @brief	Resolve a dbkey to an available context.


    @param statement
    @param input

 **/
static dsql_nod* pass1_dbkey( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	const dsql_str* qualifier = (dsql_str*) input->nod_arg[0];
	if (!qualifier)
	{
		DEV_BLKCHK(qualifier, dsql_type_str);

		DsqlContextStack contexts;

		for (DsqlContextStack::iterator stack(*statement->req_context); stack.hasData(); ++stack)
		{
			dsql_ctx* context = stack.object();
			if (context->ctx_scope_level != statement->req_scope_level)
			{
				continue;
			}

			if (context->ctx_relation)
				contexts.push(context);
		}

		if (contexts.hasData())
		{
			dsql_ctx* context = contexts.object();

			if (!context->ctx_relation)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
						  Arg::Gds(isc_dsql_dbkey_from_non_table));
			}

			if (context->ctx_flags & CTX_null)
				return MAKE_node(nod_null, 0);

			dsql_nod* node = MAKE_node(nod_dbkey, 1);
			dsql_nod* rel_node = MAKE_node(nod_relation, e_rel_count);
			rel_node->nod_arg[0] = (dsql_nod*) context;
			node->nod_arg[0] = rel_node;

			return ambiguity_check(statement, node, MAKE_cstring("RDB$DB_KEY"), contexts);
		}
	}
	else
	{
		const bool cfgRlxAlias = Config::getRelaxedAliasChecking();
		bool rlxAlias = false;
		for (;;)
		{
			for (DsqlContextStack::iterator stack(*statement->req_context); stack.hasData(); ++stack)
			{
				dsql_ctx* context = stack.object();

				if ((!context->ctx_relation ||
						context->ctx_relation->rel_name != qualifier->str_data ||
						!rlxAlias && context->ctx_internal_alias) &&
					(!context->ctx_internal_alias ||
						strcmp(qualifier->str_data, context->ctx_internal_alias) != 0))
				{
					continue;
				}

				if (!context->ctx_relation)
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
							  Arg::Gds(isc_dsql_dbkey_from_non_table));
				}

				if (context->ctx_flags & CTX_null)
					return MAKE_node(nod_null, 0);

				dsql_nod* node = MAKE_node(nod_dbkey, 1);
				dsql_nod* rel_node = MAKE_node(nod_relation, e_rel_count);
				rel_node->nod_arg[0] = (dsql_nod*) context;
				node->nod_arg[0] = rel_node;
				return node;
			}

			if (rlxAlias == cfgRlxAlias)
				break;

			rlxAlias = cfgRlxAlias;
		}
	}

	// field unresolved

	field_unknown(qualifier ? qualifier->str_data : 0, DB_KEY_STRING, input);

	return NULL;
}


/**

 	pass1_delete

    @brief	Process DELETE statement.


    @param statement
    @param input

 **/
static dsql_nod* pass1_delete( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	const dsql_nod* cursor = input->nod_arg[e_del_cursor];
	dsql_nod* relation = input->nod_arg[e_del_relation];
	if (cursor && statement->isPsql()) {
		dsql_nod* anode = MAKE_node(nod_erase_current, e_erc_count);
		anode->nod_arg[e_erc_context] = (dsql_nod*) pass1_cursor_context(statement, cursor, relation);
		anode->nod_arg[e_erc_return] = process_returning(statement, input->nod_arg[e_del_return]);
		return anode;
	}

	statement->req_type = cursor ? REQ_DELETE_CURSOR : REQ_DELETE;
	dsql_nod* node = MAKE_node(nod_erase, e_era_count);

	// Generate record selection expression

	dsql_nod* rse;
	if (cursor)
		rse = pass1_cursor_reference(statement, cursor, relation);
	else
	{
		rse = MAKE_node(nod_rse, e_rse_count);
		dsql_nod* temp = MAKE_node(nod_list, 1);
		rse->nod_arg[e_rse_streams] = temp;
		temp->nod_arg[0] = pass1_node_psql(statement, relation, false);

		if ( (temp = input->nod_arg[e_del_boolean]) ) {
			rse->nod_arg[e_rse_boolean] = pass1_node_psql(statement, temp, false);
		}

		if ( (temp = input->nod_arg[e_del_plan]) ) {
			rse->nod_arg[e_rse_plan] = pass1_node_psql(statement, temp, false);
		}

		if ( (temp = input->nod_arg[e_del_sort]) ) {
			rse->nod_arg[e_rse_sort] = pass1_sort(statement, temp, NULL);
		}

		if ( (temp = input->nod_arg[e_del_rows]) ) {
			pass1_limit(statement, temp, rse);
		}

		if (input->nod_arg[e_del_return])
			rse->nod_flags |= NOD_SELECT_EXPR_SINGLETON;
	}

	node->nod_arg[e_era_rse] = rse;
	node->nod_arg[e_era_relation] = rse->nod_arg[e_rse_streams]->nod_arg[0];

	node->nod_arg[e_era_return] = process_returning(statement, input->nod_arg[e_del_return]);

	node = nullify_returning(statement, node);

	statement->req_context->pop();
	return node;
}


/**

 	pass1_relproc_is_recursive

    @brief	check if table reference is recursive i.e. its name is equal
		to the name of current processing CTE

    @param statement
    @param input

 **/
static bool pass1_relproc_is_recursive(CompiledStatement* statement, dsql_nod* input)
{
	const dsql_str* rel_name = NULL;
	const dsql_str* rel_alias = NULL;

	switch (input->nod_type)
	{
		case nod_rel_proc_name:
			rel_name = (dsql_str*) input->nod_arg[e_rpn_name];
			rel_alias = (dsql_str*) input->nod_arg[e_rpn_alias];
			break;

		case nod_relation_name:
			rel_name = (dsql_str*) input->nod_arg[e_rln_name];
			rel_alias = (dsql_str*) input->nod_arg[e_rln_alias];
			break;

		default:
			return false;
	}

	fb_assert(statement->req_curr_ctes.hasData());
	const dsql_nod* curr_cte = statement->req_curr_ctes.object();
	const dsql_str* cte_name = (dsql_str*) curr_cte->nod_arg[e_derived_table_alias];

	const bool recursive = (cte_name->str_length == rel_name->str_length) &&
		(strncmp(rel_name->str_data, cte_name->str_data, cte_name->str_length) == 0);

	if (recursive) {
		statement->addCTEAlias(rel_alias ? rel_alias : rel_name);
	}

	return recursive;
}


/**

 	pass1_join_is_recursive

    @brief	check if join have recursive members. If found remove this member
		from join and return its boolean (to be added into WHERE clause).
		We must remove member only if it is a table reference.
		Punt if recursive reference is found in outer join or more than one
		recursive reference is found

    @param statement
    @param input

 **/
static dsql_nod* pass1_join_is_recursive(CompiledStatement* statement, dsql_nod*& input)
{
	const NOD_TYPE join_type = input->nod_arg[e_join_type]->nod_type;
	bool remove = false;

	bool leftRecursive = false;
	dsql_nod* leftBool = NULL;
	dsql_nod** join_table = &input->nod_arg[e_join_left_rel];
	if ((*join_table)->nod_type == nod_join)
	{
		leftBool = pass1_join_is_recursive(statement, *join_table);
		leftRecursive = (leftBool != NULL);
	}
	else
	{
		leftBool = input->nod_arg[e_join_boolean];
		leftRecursive = pass1_relproc_is_recursive(statement, *join_table);
		if (leftRecursive)
			remove = true;
	}

	if (leftRecursive && join_type != nod_join_inner) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't be member of an outer join
				  Arg::Gds(isc_dsql_cte_outer_join));
	}

	bool rightRecursive = false;
	dsql_nod* rightBool = NULL;
	join_table = &input->nod_arg[e_join_rght_rel];
	if ((*join_table)->nod_type == nod_join)
	{
		rightBool = pass1_join_is_recursive(statement, *join_table);
		rightRecursive = (rightBool != NULL);
	}
	else
	{
		rightBool = input->nod_arg[e_join_boolean];
		rightRecursive = pass1_relproc_is_recursive(statement, *join_table);
		if (rightRecursive)
			remove = true;
	}

	if (rightRecursive && join_type != nod_join_inner) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't be member of an outer join
				  Arg::Gds(isc_dsql_cte_outer_join));
	}

	if (leftRecursive && rightRecursive) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive member of CTE can't reference itself more than once
				  Arg::Gds(isc_dsql_cte_mult_references));
	}

	if (leftRecursive)
	{
		if (remove)
			input = input->nod_arg[e_join_rght_rel];

		return leftBool;
	}

	if (rightRecursive)
	{
		if (remove)
			input = input->nod_arg[e_join_left_rel];

		return rightBool;
	}

	return 0;
}


/**

 	pass1_rse_is_recursive

    @brief	check if rse is recursive. If recursive reference is a table
		in the FROM list remove it. If recursive reference is a part of
		join add join boolean (returned by pass1_join_is_recursive) to the
		WHERE clause. Punt if more than one recursive reference is found

    @param statement
    @param input

 **/
static dsql_nod* pass1_rse_is_recursive(CompiledStatement* statement, dsql_nod* input)
{
	fb_assert(input->nod_type == nod_query_spec);

	dsql_nod* result = MAKE_node(nod_query_spec, e_qry_count);
	memcpy(result->nod_arg, input->nod_arg, e_qry_count * sizeof (dsql_nod*));

	dsql_nod* src_tables = input->nod_arg[e_qry_from];
	dsql_nod* dst_tables = MAKE_node(nod_list, src_tables->nod_count);
	result->nod_arg[e_qry_from] = dst_tables;

	dsql_nod** p_dst_table = dst_tables->nod_arg;
	dsql_nod** p_src_table = src_tables->nod_arg;
	dsql_nod** end = src_tables->nod_arg + src_tables->nod_count;

	bool found = false;
	for (dsql_nod** prev = p_dst_table; p_src_table < end; p_src_table++, p_dst_table++)
	{
		*prev++ = *p_dst_table = *p_src_table;
		switch ((*p_dst_table)->nod_type)
		{
			case nod_rel_proc_name:
			case nod_relation_name:
				if (pass1_relproc_is_recursive(statement, *p_dst_table))
				{
					if (found) {
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
								  // Recursive member of CTE can't reference itself more than once
								  Arg::Gds(isc_dsql_cte_mult_references));
					}
					found = true;

					prev--;
					dst_tables->nod_count--;
				}
				break;

			case nod_join:
				{
					*p_dst_table = MAKE_node(nod_join, e_join_count);
					memcpy((*p_dst_table)->nod_arg, (*p_src_table)->nod_arg, 
						e_join_count * sizeof (dsql_nod*));

					dsql_nod* joinBool = pass1_join_is_recursive(statement, *p_dst_table);
					if (joinBool)
					{
						if (found) {
							ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
									  // Recursive member of CTE can't reference itself more than once
									  Arg::Gds(isc_dsql_cte_mult_references));
						}
						found = true;

						result->nod_arg[e_qry_where] =
							compose(result->nod_arg[e_qry_where], joinBool, nod_and);
					}
				}
				break;

			case nod_derived_table:
				break;

			default:
				fb_assert(false);
		}
	}

	return found ? result : NULL;
}


/**

 	pass1_recursive_cte

    @brief	Process derived table which can be recursive CTE
		If it is non-recursive return input node unchanged
		If it is recursive return new derived table which is an union of
		union of anchor (non-recursive) queries and union of recursive
		queries. Check recursive queries to satisfy various criterias.
		Note that our parser is right-to-left therefore nested list linked 
		as first node in parent list and second node is always query spec.

		For example, if we have 4 CTE's where first two is non-recursive
	and last two is recursive :

				list							  union
			  [0]	[1]						   [0]		[1]
			list	cte3		===>		anchor		recursive
		  [0]	[1]						 [0]	[1]		[0]		[1]
		list	cte3					cte1	cte2	cte3	cte4
	  [0]	[1]
	cte1	cte2

		Also, we should not change layout of original parse tree to allow it to
	be parsed again if needed. Therefore recursive part is built using newly 
	allocated list nodes.

    @param statement
    @param input

 **/
static dsql_nod* pass1_recursive_cte(CompiledStatement* statement, dsql_nod* input)
{
	dsql_str* const cte_alias = (dsql_str*) input->nod_arg[e_derived_table_alias];
	dsql_nod* const select_expr = input->nod_arg[e_derived_table_rse];
	dsql_nod* query = select_expr->nod_arg[e_sel_query_spec];

	if (query->nod_type != nod_list && pass1_rse_is_recursive(statement, query))
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Recursive CTE (%s) must be an UNION
				  Arg::Gds(isc_dsql_cte_not_a_union) << Arg::Str(cte_alias->str_data));
	}

	// split queries list on two parts: anchor and recursive
	dsql_nod* anchor_rse = 0, *recursive_rse = 0;
	dsql_nod* qry = query;

	dsql_nod* new_qry = MAKE_node(nod_list, 2);
	new_qry->nod_flags = query->nod_flags;
	while (true)
	{
		dsql_nod* rse = 0;
		if (qry->nod_type == nod_list)
			rse = qry->nod_arg[1];
		else
			rse = qry;

		dsql_nod* new_rse = pass1_rse_is_recursive(statement, rse);
		if (new_rse) // rse is recursive
		{
			if (anchor_rse)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // CTE '%s' defined non-recursive member after recursive
						  Arg::Gds(isc_dsql_cte_nonrecurs_after_recurs) << Arg::Str(cte_alias->str_data));
			}
			if (new_rse->nod_arg[e_qry_distinct]) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Recursive member of CTE '%s' has %s clause
						  Arg::Gds(isc_dsql_cte_wrong_clause) << Arg::Str(cte_alias->str_data) <<
																 Arg::Str("DISTINCT"));
			}
			if (new_rse->nod_arg[e_qry_group]) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Recursive member of CTE '%s' has %s clause
						  Arg::Gds(isc_dsql_cte_wrong_clause) << Arg::Str(cte_alias->str_data) <<
																 Arg::Str("GROUP BY"));
			}
			if (new_rse->nod_arg[e_qry_having]) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Recursive member of CTE '%s' has %s clause
						  Arg::Gds(isc_dsql_cte_wrong_clause) << Arg::Str(cte_alias->str_data) <<
																 Arg::Str("HAVING"));
			}
			// hvlad: we need also forbid any aggregate function here
			// but for now i have no idea how to do it simple

			if ((new_qry->nod_type == nod_list) && !(new_qry->nod_flags & NOD_UNION_ALL)) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Recursive members of CTE (%s) must be linked with another members via UNION ALL
						  Arg::Gds(isc_dsql_cte_union_all) << Arg::Str(cte_alias->str_data));
			}
			if (!recursive_rse) {
				recursive_rse = new_qry;
			}
			new_rse->nod_flags |= NOD_SELECT_EXPR_RECURSIVE;

			if (qry->nod_type == nod_list)
				new_qry->nod_arg[1] = new_rse;
			else
				new_qry->nod_arg[0] = new_rse;
		}
		else
		{
			if (qry->nod_type == nod_list)
				new_qry->nod_arg[1] = rse;
			else
				new_qry->nod_arg[0] = rse;

			if (!anchor_rse) {
				if (qry->nod_type == nod_list)
					anchor_rse = new_qry;
				else
					anchor_rse = rse;
			}
		}

		if (qry->nod_type != nod_list)
			break;

		qry = qry->nod_arg[0];

		if (qry->nod_type == nod_list)
		{
			new_qry->nod_arg[0] = MAKE_node(nod_list, 2);
			new_qry = new_qry->nod_arg[0];
			new_qry->nod_flags = qry->nod_flags;
		}
	}

	if (!recursive_rse) {
		return input;
	}
	if (!anchor_rse) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Non-recursive member is missing in CTE '%s'
				  Arg::Gds(isc_dsql_cte_miss_nonrecursive) << Arg::Str(cte_alias->str_data));
	}

	qry = recursive_rse;
	dsql_nod* list = 0;
	while (qry->nod_arg[0] != anchor_rse)
	{
		list = qry;
		qry = qry->nod_arg[0];
	}
	qry->nod_arg[0] = 0;
	if (list) {
		list->nod_arg[0] = qry->nod_arg[1];
	}
	else {
		recursive_rse = qry->nod_arg[1];
	}

	dsql_nod* union_node = MAKE_node(nod_list, 2);
	union_node->nod_flags = NOD_UNION_ALL | NOD_UNION_RECURSIVE;
	union_node->nod_arg[0] = anchor_rse;
	union_node->nod_arg[1] = recursive_rse;

	dsql_nod* select = MAKE_node(nod_select_expr, e_sel_count);
	select->nod_arg[e_sel_query_spec] = union_node;
	select->nod_arg[e_sel_order] = select->nod_arg[e_sel_rows] =
		select->nod_arg[e_sel_with_list] = NULL;

	dsql_nod* node = MAKE_node(nod_derived_table, e_derived_table_count);
	dsql_str* alias = (dsql_str*) input->nod_arg[e_derived_table_alias];
	node->nod_arg[e_derived_table_alias] = (dsql_nod*) alias;
	node->nod_arg[e_derived_table_column_alias] = input->nod_arg[e_derived_table_column_alias];
	node->nod_arg[e_derived_table_rse] = select;
	node->nod_arg[e_derived_table_context] = input->nod_arg[e_derived_table_context];

	return node;
}


/**

 	process_returning

    @brief	Compile a RETURNING clause (nod_returning or not).


    @param statement
    @param input

 **/
static dsql_nod* process_returning(CompiledStatement* statement,
								   dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* node;

	if (!input || input->nod_type == nod_returning)
		node = PASS1_node(statement, input);
	else
		node = pass1_node_psql(statement, input, false);

	if (input && !statement->isPsql())
		statement->req_type = REQ_EXEC_PROCEDURE;

	return node;
}


// Extract relation and procedure context and expand derived child contexts.
static void pass1_expand_contexts(DsqlContextStack& contexts, dsql_ctx* context)
{
	if (context->ctx_relation || context->ctx_procedure || context->ctx_map)
	{
		if (context->ctx_parent)
			context = context->ctx_parent;

		contexts.push(context);
	}
	else
	{
		for (DsqlContextStack::iterator i(context->ctx_childs_derived_table); i.hasData(); ++i)
			pass1_expand_contexts(contexts, i.object());
	}
}


/**

 	pass1_derived_table

    @brief	Process derived table which is part of a from clause.


    @param statement
    @param input

 **/
static dsql_nod* pass1_derived_table(CompiledStatement* statement, dsql_nod* input, dsql_str* cte_alias)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	thread_db* tdbb = JRD_get_thread_data();

	dsql_nod* node = MAKE_node (nod_derived_table, e_derived_table_count);
	dsql_str* alias = (dsql_str*) input->nod_arg[e_derived_table_alias];
	node->nod_arg[e_derived_table_alias] = (dsql_nod*) alias;
	node->nod_arg[e_derived_table_column_alias] = input->nod_arg[e_derived_table_column_alias];

	// Create the context now, because we need to know it for the tables inside.
	dsql_ctx* const context = PASS1_make_context(statement, node);
	node->nod_arg[e_derived_table_context] = (dsql_nod*) context;

	// Save some values to restore after rse process.
	DsqlContextStack* const req_base = statement->req_context;
	dsql_str* const req_alias_relation_prefix = statement->req_alias_relation_prefix;

	// Change req_context, because when we are processing the derived table rse
	// it may not reference to other streams in the same scope_level.
	DsqlContextStack temp;
	// Put special contexts (NEW/OLD) also on the stack
	for (DsqlContextStack::iterator stack(*statement->req_context); stack.hasData(); ++stack)
	{
		dsql_ctx* local_context = stack.object();
		if ((local_context->ctx_scope_level < statement->req_scope_level) ||
			(local_context->ctx_flags & CTX_system))
		{
			temp.push(local_context);
		}
	}
	dsql_ctx* baseContext = NULL;
	if (temp.hasData()) {
		baseContext = temp.object();
	}
	statement->req_context = &temp;
	statement->req_alias_relation_prefix = pass1_alias_concat(req_alias_relation_prefix, alias);

	dsql_nod* const select_expr = input->nod_arg[e_derived_table_rse];
	dsql_nod* query = select_expr->nod_arg[e_sel_query_spec];
	dsql_nod* rse = NULL;
	const bool isRecursive = (query->nod_type == nod_list) && (query->nod_flags & NOD_UNION_RECURSIVE);
	USHORT recursive_map_ctx = 0;

	if (isRecursive)
	{
		// Create dummy, non-recursive select statement by doing a union of
		// one, non-recursive member. The dummy will be replaced at the end
		// of this function.

		query->nod_count = 1;
		query->nod_flags &= ~NOD_UNION_RECURSIVE;

		dsql_ctx* baseUnionCtx = statement->req_union_context.hasData() ?
			statement->req_union_context.object() : NULL;

		// reserve extra context number for map's secondary context
		recursive_map_ctx = statement->req_context_number++;

		statement->req_recursive_ctx_id = statement->req_context_number;
		rse = pass1_union(statement, query, NULL, NULL, 0);
		statement->req_context_number = statement->req_recursive_ctx_id + 1;

		// recursive union always have exactly 2 members
		query->nod_count = 2;
		query->nod_flags |= NOD_UNION_RECURSIVE;

		while (statement->req_union_context.hasData() &&
			   statement->req_union_context.object() != baseUnionCtx)
		{
			statement->req_union_context.pop();
		}
	}
	else
	{
		// AB: 2005-01-06
		// If our derived table contains a single query with a sub-select buried
		// inside the select items then we need a special handling, because we don't
		// want creating a new sub-select for every reference outside the derived
		// table to that sub-select.
		// To handle this we simple create a UNION ALL with derived table inside it.
		// Due this mappings are created and we simple reference to these mappings.
		// Optimizer effects:
		//   Good thing is that only 1 recordstream is made for the sub-select, but
		//   the worse thing is that a UNION currently can't be used in
		//   deciding the JOIN order.
		bool foundSubSelect = false;
		if (query->nod_type == nod_query_spec) {
			foundSubSelect = pass1_found_sub_select(query->nod_arg[e_qry_list]);
		}

		if (foundSubSelect)
		{
			dsql_nod* union_expr = MAKE_node(nod_list, 1);
			union_expr->nod_arg[0] = select_expr;
			union_expr->nod_flags = NOD_UNION_ALL;
			rse = pass1_union(statement, union_expr, NULL, NULL, 0);
		}
		else
			rse = PASS1_rse(statement, select_expr, NULL);

		USHORT minOuterJoin = MAX_USHORT;

		// Finish off by cleaning up contexts and put them into req_dt_context
		// so create view (ddl) can deal with it.
		// Also add the used contexts into the childs stack.
		while (temp.hasData() && (temp.object() != baseContext))
		{
			dsql_ctx* childCtx = temp.pop();

			statement->req_dt_context.push(childCtx);
			context->ctx_childs_derived_table.push(childCtx);

			// Collect contexts that will be used for blr_derived_expr generation.
			// We want all child contexts with minimum ctx_in_outer_join.
			if (childCtx->ctx_in_outer_join <= minOuterJoin)
			{
				DsqlContextStack contexts;
				pass1_expand_contexts(contexts, childCtx);

				for (DsqlContextStack::iterator i(contexts); i.hasData(); ++i)
				{
					if (i.object()->ctx_in_outer_join < minOuterJoin)
					{
						minOuterJoin = i.object()->ctx_in_outer_join;
						context->ctx_main_derived_contexts.clear();
					}

					context->ctx_main_derived_contexts.push(i.object());
				}
			}
		}

		while (temp.hasData())
			temp.pop();
	}
	context->ctx_rse = node->nod_arg[e_derived_table_rse] = rse;

	// CVC: prepare a truncated alias for the derived table here
	// because we need it several times.
	TEXT aliasbuffer[100] = "";
	const TEXT* aliasname = aliasbuffer;
	if (alias)
	{
		int length = alias->str_length;
		if (length > 99) {
			length = 99;
			memcpy(aliasbuffer, alias->str_data, length);
			aliasbuffer[length] = 0;
		}
		else
			aliasname = alias->str_data;
	}
	else
		aliasname = "<unnamed>";

	// If an alias-list is specified process it.
	const bool ignoreColumnChecks = (input->nod_flags & NOD_DT_IGNORE_COLUMN_CHECK);
	if (node->nod_arg[e_derived_table_column_alias] &&
		node->nod_arg[e_derived_table_column_alias]->nod_count)
	{
		dsql_nod* list = node->nod_arg[e_derived_table_column_alias];

		// Do both lists have the same number of items?
		if (list->nod_count != rse->nod_arg[e_rse_items]->nod_count)
		{
			// Column list by derived table %s [alias-name] has %s [more/fewer] columns
			// than the number of items.
			//
			int errcode = isc_dsql_derived_table_less_columns;
			if (list->nod_count > rse->nod_arg[e_rse_items]->nod_count)
				errcode = isc_dsql_derived_table_more_columns;

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(errcode) << Arg::Str(aliasname));
		}

		// Generate derived fields and assign alias-name to it.
		for (int count = 0; count < list->nod_count; count++)
		{
			dsql_nod* select_item = rse->nod_arg[e_rse_items]->nod_arg[count];
			// Make new derived field node
			dsql_nod* derived_field = MAKE_node(nod_derived_field, e_derived_field_count);
			derived_field->nod_arg[e_derived_field_value] = select_item;
			derived_field->nod_arg[e_derived_field_name] = list->nod_arg[count];
			derived_field->nod_arg[e_derived_field_scope] = (dsql_nod*)(IPTR) statement->req_scope_level;
			derived_field->nod_desc = select_item->nod_desc;

			rse->nod_arg[e_rse_items]->nod_arg[count] = derived_field;
		}
	}
	else
	{
		// For those select-items where no alias is specified try
		// to generate one from the field_name.
		for (int count = 0; count < rse->nod_arg[e_rse_items]->nod_count; count++)
		{
			dsql_nod* select_item =
				pass1_make_derived_field(statement, tdbb, rse->nod_arg[e_rse_items]->nod_arg[count]);

			// Auto-create dummy column name for pass1_any()
			if (ignoreColumnChecks && (select_item->nod_type != nod_derived_field))
			{
				// Make new derived field node
				dsql_nod* derived_field = MAKE_node(nod_derived_field, e_derived_field_count);
				derived_field->nod_arg[e_derived_field_value] = select_item;

				// Construct dummy fieldname
				char fieldname[25];
				sprintf (fieldname, "f%d", count);
				dsql_str* field_alias =
					FB_NEW_RPT(*tdbb->getDefaultPool(), strlen(fieldname)) dsql_str;
				strcpy(field_alias->str_data, fieldname);
				field_alias->str_length = strlen(fieldname);

				derived_field->nod_arg[e_derived_field_name] = (dsql_nod*) field_alias;
				derived_field->nod_arg[e_derived_field_scope] =
					(dsql_nod*)(IPTR) statement->req_scope_level;
				derived_field->nod_desc = select_item->nod_desc;
				select_item = derived_field;
			}

			rse->nod_arg[e_rse_items]->nod_arg[count] = select_item;
		}
	}

	int count;
	// Check if all root select-items have a derived field else show a message.
	for (count = 0; count < rse->nod_arg[e_rse_items]->nod_count; count++)
	{
		dsql_nod* select_item = rse->nod_arg[e_rse_items]->nod_arg[count];

		if (select_item->nod_type == nod_derived_field)
		{
			select_item->nod_arg[e_derived_field_context] = reinterpret_cast<dsql_nod*>(context);
		}
		else
		{
			// no column name specified for column number %d in derived table %s

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(isc_dsql_derived_field_unnamed) << Arg::Num(count + 1) <<
																  Arg::Str(aliasname));
		}
	}

	// Check for ambiguous column names inside this derived table.
	for (count = 0; count < rse->nod_arg[e_rse_items]->nod_count; count++)
	{
		const dsql_nod* select_item1 = rse->nod_arg[e_rse_items]->nod_arg[count];
		for (int count2 = (count + 1); count2 < rse->nod_arg[e_rse_items]->nod_count; count2++)
		{
			const dsql_nod* select_item2 = rse->nod_arg[e_rse_items]->nod_arg[count2];
			const dsql_str* name1 = (dsql_str*) select_item1->nod_arg[e_derived_field_name];
			const dsql_str* name2 = (dsql_str*) select_item2->nod_arg[e_derived_field_name];
			if (!strcmp(name1->str_data, name2->str_data))
			{
				// column %s was specified multiple times for derived table %s
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_derived_field_dup_name) << Arg::Str(name1->str_data) <<
																	   Arg::Str(aliasname));
			}
		}
	}

	// If we used a dummy rse before, replace it with the real one now.
	// We cannot do this earlier, because recursive processing needs a fully
	// developed context block.
	if (isRecursive)
	{
		statement->req_recursive_ctx = context;
		statement->req_context = &temp;

		statement->resetCTEAlias(alias);

		rse = PASS1_rse(statement, input->nod_arg[e_derived_table_rse], NULL);

		// Finish off by cleaning up contexts and put them into req_dt_context
		// so create view (ddl) can deal with it.
		// Also add the used contexts into the childs stack.
		while (temp.hasData() && (temp.object() != baseContext))
		{
			statement->req_dt_context.push(temp.object());
			context->ctx_childs_derived_table.push(temp.pop());
		}

		temp.clear();

		rse->nod_arg[e_rse_items] = context->ctx_rse->nod_arg[e_rse_items];

		dsql_nod* node2 = MAKE_node(nod_derived_table, e_derived_table_count);
		*node2 = *node;
		node2->nod_arg[e_derived_table_rse] = rse;
		node = node2;

		context->ctx_rse = rse;
		if (cte_alias) {
			context->ctx_alias = cte_alias->str_data;
		}

		statement->req_context = req_base;

		// mark union's map context as recursive and assign
		// secondary context number to it
		dsql_nod* items = rse->nod_arg[e_rse_items];
		dsql_nod* map_item = items->nod_arg[0];
		if (map_item->nod_type == nod_derived_field) {
			map_item = map_item->nod_arg[e_alias_value];
		}
		dsql_ctx* map_context = (dsql_ctx*) map_item->nod_arg[e_map_context];

		map_context->ctx_flags |= CTX_recursive;
		map_context->ctx_recursive = recursive_map_ctx;
	}

	delete statement->req_alias_relation_prefix;
	// Restore our original values.
	statement->req_context = req_base;
	statement->req_alias_relation_prefix = req_alias_relation_prefix;

	return node;
}


/**

 	pass1_expand_select_list

    @brief	Expand asterisk nodes into fields.


    @param statement
    @param list
    @param streams

 **/
static dsql_nod* pass1_expand_select_list(CompiledStatement* statement, dsql_nod* list,
	dsql_nod* streams)
{
	if (!list)
		list = streams;

	DsqlNodStack stack;
	dsql_nod** ptr = list->nod_arg;
	for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		pass1_expand_select_node(statement, *ptr, stack, true);
	}
	dsql_nod* node = MAKE_list(stack);
	return node;
}

/**

 	pass1_expand_select_node

    @brief	Expand a select item node.


    @param statement
    @param node
    @param stack

 **/
static void pass1_expand_select_node(CompiledStatement* statement, dsql_nod* node, DsqlNodStack& stack,
	bool hide_using)
{
	DEV_BLKCHK(node, dsql_type_nod);

	if (node->nod_type == nod_join)
	{
		pass1_expand_select_node(statement, node->nod_arg[e_join_left_rel], stack, true);
		pass1_expand_select_node(statement, node->nod_arg[e_join_rght_rel], stack, true);
	}
	else if (node->nod_type == nod_derived_table)
	{
		// AB: Derived table support
		thread_db* tdbb = JRD_get_thread_data(); // unused
		dsql_nod* sub_items = node->nod_arg[e_derived_table_rse]->nod_arg[e_rse_items];
		dsql_nod** ptr = sub_items->nod_arg;
		for (const dsql_nod* const* const end = ptr + sub_items->nod_count; ptr < end; ++ptr)
		{
			// Create a new alias else mappings would be mangled.
			dsql_nod* select_item = *ptr;
			// select-item should always be a derived field!
			if (select_item->nod_type != nod_derived_field) {
				// Internal dsql error: alias type expected by pass1_expand_select_node
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_derived_alias_select));
			}

			dsql_ctx* context = (dsql_ctx*) select_item->nod_arg[e_derived_field_context];
			DEV_BLKCHK(context, dsql_type_ctx);

			if (!hide_using ||
				context->getImplicitJoinField(reinterpret_cast<dsql_str*>(
					select_item->nod_arg[e_derived_field_name])->str_data, select_item))
			{
				stack.push(select_item);
			}
		}
	}
	else if (node->nod_type == nod_relation)
	{
		dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_rel_context];
		DEV_BLKCHK(context, dsql_type_ctx);
		dsql_prc* procedure;
		dsql_rel* relation = context->ctx_relation;
		if (relation) {
			for (dsql_fld* field = relation->rel_fields; field; field = field->fld_next)
			{
				DEV_BLKCHK(field, dsql_type_fld);

				dsql_nod* select_item = NULL;
				if (!hide_using || context->getImplicitJoinField(field->fld_name, select_item))
				{
					if (!select_item)
						select_item = MAKE_field(context, field, 0);
					stack.push(select_item);
				}
			}
		}
		else if (procedure = context->ctx_procedure)
		{
			for (dsql_fld* field = procedure->prc_outputs; field; field = field->fld_next)
			{
				DEV_BLKCHK(field, dsql_type_fld);

				dsql_nod* select_item = NULL;
				if (!hide_using || context->getImplicitJoinField(field->fld_name, select_item))
				{
					if (!select_item)
						select_item = MAKE_field(context, field, 0);
					stack.push(select_item);
				}
			}
		}
	}
	else if (node->nod_type == nod_field_name)
	{
		dsql_nod* select_item = pass1_field(statement, node, true, NULL);
		// The node could be a relation so call recursively.
		pass1_expand_select_node(statement, select_item, stack, false);
	}
	else {
		stack.push(node);
	}
}


/**

 	pass1_field

    @brief	Resolve a field name to an available context.
 	If list is true, then this function can detect and
 	return a relation node if there is no name.   This
 	is used for cases of "SELECT <table_name>. ...".
   CVC: The function attempts to detect
   if an unqualified field appears in more than one context
   and hence it returns the number of occurrences. This was
   added to allow the caller to detect ambiguous commands like
   select  from t1 join t2 on t1.f = t2.f order by common_field.
   While inoffensive on inner joins, it changes the result on outer joins.


    @param statement
    @param input
    @param list

 **/
static dsql_nod* pass1_field(CompiledStatement* statement, dsql_nod* input,
							 const bool list, dsql_nod* select_list)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	// CVC: This shameful hack added to allow CHECK constraint implementation via triggers
	// to be able to work.
	bool is_check_constraint = false;
	{ // scope block
		const dsql_nod* ddl_node = statement->req_ddl_node;
		if (ddl_node && ddl_node->nod_type == nod_def_constraint) {
			is_check_constraint = true;
		}
	} // end scope block

	// handle an array element.
	dsql_nod* indices;
	if (input->nod_type == nod_array) {
		indices = input->nod_arg[e_ary_indices];
		input = input->nod_arg[e_ary_array];
	}
	else {
		indices = NULL;
	}

	dsql_str* name;
	const dsql_str* qualifier; // We assume the qualifier was stripped elsewhere.
	if (input->nod_count == 1) {
		name = (dsql_str*) input->nod_arg[0];
		qualifier = NULL;
	}
	else {
		name = (dsql_str*) input->nod_arg[1];
		qualifier = (dsql_str*) input->nod_arg[0];
	}
	DEV_BLKCHK(name, dsql_type_str);
	DEV_BLKCHK(qualifier, dsql_type_str);

	// CVC: Let's strip trailing blanks or comparisons may fail in dialect 3.
	if (name && name->str_data) {
		fb_utils::exact_name(name->str_data);
	}

    /* CVC: PLEASE READ THIS EXPLANATION IF YOU NEED TO CHANGE THIS CODE.
       You should ensure that this function:
       1.- Never returns NULL. In such case, it such fall back to an invocation
       to field_unknown() near the end of this function. None of the multiple callers
       of this function (inside this same module) expect a null pointer, hence they
       will crash the engine in such case.
       2.- Doesn't allocate more than one field in "node". Either you put a break,
       keep the current "continue" or call ALLD_release if you don't want nor the
       continue neither the break if node is already allocated. If it isn't evident,
       but this variable is initialized to zero in the declaration above. You
       may write an explicit line to set it to zero here, before the loop.

       3.- Doesn't waste cycles if qualifier is not null. The problem is not the cycles
       themselves, but the fact that you'll detect an ambiguity that doesn't exist: if
       the field appears in more than one context but it's always qualified, then
       there's no ambiguity. There's PASS1_make_context() that prevents a context's
       alias from being reused. However, other places in the code don't check that you
       don't create a join or subselect with the same context without disambiguating it
       with different aliases. This is the place where resolve_context() is called for
       that purpose. In the future, it will be fine if we force the use of the alias as
       the only allowed qualifier if the alias exists. Hopefully, we will eliminate
       some day this construction: "select table.field from table t" because it
       should be "t.field" instead.

       AB: 2004-01-09
       The explained query directly above doesn't work anymore, thus the day has come ;-)
	   It's allowed to use the same fieldname between different scope levels (sub-queries)
	   without being hit by the ambiguity check. The field uses the first match starting
	   from it's own level (of course ambiguity-check on each level is done).

       4.- Doesn't verify code derived automatically from check constraints. They are
       ill-formed by nature but making that code generation more orthodox is not a
       priority. Typically, they only check a field against a contant. The problem
       appears when they check a field against a subselect, for example. For now,
       allow the user to write ambiguous subselects in check() statements.
       Claudio Valderrama - 2001.1.29.
    */


	if (select_list && !qualifier && name && name->str_data) {
		// AB: Check first against the select list for matching column.
		// When no matches at all are found we go on with our
		// normal way of field name lookup.
		dsql_nod* node = pass1_lookup_alias(statement, name, select_list, true);
		if (node) {
			return node;
		}
	}


	// Try to resolve field against various contexts;
	// if there is an alias, check only against the first matching

	dsql_nod* node = NULL; // This var must be initialized.
	DsqlContextStack ambiguous_ctx_stack;

	bool resolve_by_alias = true;
	const bool relaxedAliasChecking = Config::getRelaxedAliasChecking();

	while (true)
	{
		// AB: Loop through the scope_levels starting by its own.
		bool done = false;
		USHORT current_scope_level = statement->req_scope_level + 1;
		for (; (current_scope_level > 0) && !done; current_scope_level--) {

			// If we've found a node we're done.
			if (node) {
				break;
			}

			for (DsqlContextStack::iterator stack(*statement->req_context); stack.hasData(); ++stack)
			{
				// resolve_context() checks the type of the
				// given context, so the cast to dsql_ctx* is safe.

				dsql_ctx* context = stack.object();

				if (context->ctx_scope_level != (current_scope_level - 1))
				{
					continue;
				}

				dsql_fld* field = resolve_context(statement, qualifier, context, is_check_constraint,
												  resolve_by_alias);

				// AB: When there's no relation and no procedure then we have a derived table.
				const bool is_derived_table =
					(!context->ctx_procedure && !context->ctx_relation && context->ctx_rse);

				if (field)
				{
					// If there's no name then we have most probable an asterisk that
					// needs to be exploded. This should be handled by the caller and
					// when the caller can handle this, list is true.
					if (!name)
					{
						if (list) {
							node = MAKE_node(nod_relation, e_rel_count);
							node->nod_arg[e_rel_context] = reinterpret_cast<dsql_nod*>(stack.object());
							return node;
						}

						break;
					}

					dsql_nod* using_field = NULL;

					for (; field; field = field->fld_next)
					{
						if (field->fld_name == name->str_data)
						{
							if (!qualifier)
							{
								if (!context->getImplicitJoinField(field->fld_name, using_field))
								{
									field = NULL;
									break;
								}

								if (using_field)
									field = NULL;
							}

							ambiguous_ctx_stack.push(context);
							break;
						}
					}

					if (qualifier && !field) {
						// If a qualifier was present and we don't have found
						// a matching field then we should stop searching.
						// Column unknown error will be raised at bottom of function.
						done = true;
						break;
					}

					if (field || using_field)
					{
						// Intercept any reference to a field with datatype that
						// did not exist prior to V6 and post an error

						if (statement->req_client_dialect <= SQL_DIALECT_V5 && field &&
							(field->fld_dtype == dtype_sql_date ||
								field->fld_dtype == dtype_sql_time || field->fld_dtype == dtype_int64))
						{
								ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
										  Arg::Gds(isc_dsql_field_err) <<
										  Arg::Gds(isc_random) << Arg::Str(field->fld_name) <<
										  Arg::Gds(isc_sql_dialect_datatype_unsupport) <<
										  			Arg::Num(statement->req_client_dialect) <<
										  			Arg::Str(DSC_dtype_tostring(static_cast<UCHAR>(field->fld_dtype))));
								return NULL;
						}

						// CVC: Stop here if this is our second or third iteration.
						// Anyway, we can't report more than one ambiguity to the status vector.
						// AB: But only if we're on different scope level, because a
						// node inside the same context should have priority.
						if (node) {
							continue;
						}

						if (indices) {
							indices = pass1_node_psql(statement, indices, false);
						}

						if (context->ctx_flags & CTX_null)
							node = MAKE_node(nod_null, 0);
						else if (field)
							node = MAKE_field(context, field, indices);
						else
							node = list ? using_field : pass1_node_psql(statement, using_field, false);
					}
				}
				else if (is_derived_table)
				{
					// if an qualifier is present check if we have the same derived
					// table else continue;
					if (qualifier)
					{
						if (context->ctx_alias)
						{
							if (strcmp(qualifier->str_data, context->ctx_alias))
							{
								continue;
							}
						}
						else {
							continue;
						}
					}

					// If there's no name then we have most probable a asterisk that
					// needs to be exploded. This should be handled by the caller and
					// when the caller can handle this, list is true.
					if (!name)
					{
						if (list) {
							// Node is created so caller pass1_expand_select_node()
							// can deal with it.
							node = MAKE_node(nod_derived_table, e_derived_table_count);
							node->nod_arg[e_derived_table_rse] = context->ctx_rse;
							return node;
						}

						break;
					}

					// Because every select item has an alias we can just walk
					// through the list and return the correct node when found.
					const dsql_nod* rse_items = context->ctx_rse->nod_arg[e_rse_items];
					dsql_nod* const* ptr = rse_items->nod_arg;
					for (const dsql_nod* const* const end = ptr + rse_items->nod_count;
						ptr < end; ptr++)
					{
						dsql_nod* node_select_item = *ptr;
						// select-item should always be a alias!
						if (node_select_item->nod_type == nod_derived_field)
						{
							const dsql_str* string =
								(dsql_str*) node_select_item->nod_arg[e_derived_field_name];

							dsql_nod* using_field = NULL;

							if (!qualifier)
							{
								if (!context->getImplicitJoinField(name->str_data, using_field))
									break;
							}

							if (!strcmp(name->str_data, string->str_data) || using_field)
							{

								// This is a matching item so add the context to the ambiguous list.
								ambiguous_ctx_stack.push(context);

								// Stop here if this is our second or more iteration.
								if (node) {
									break;
								}

								node = using_field ? using_field : node_select_item;
								break;
							}
						}
						else {
							// Internal dsql error: alias type expected by pass1_field
							ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
									  Arg::Gds(isc_dsql_command_err) <<
									  Arg::Gds(isc_dsql_derived_alias_field));
						}
					}

					if (!node && qualifier) {
						// If a qualifier was present and we don't have found
						// a matching field then we should stop searching.
						// Column unknown error will be raised at bottom of function.
						done = true;
						break;
					}
				}
			}
		}

		if (node)
			break;

		if (resolve_by_alias && !is_check_constraint && relaxedAliasChecking) {
			resolve_by_alias = false;
		}
		else {
			break;
		}
	}

	// CVC: We can't return blindly if this is a check constraint, because there's
	// the possibility of an invalid field that wasn't found. The multiple places that
	// call this function pass1_field() don't expect a NULL pointer, hence will crash.
	// Don't check ambiguity if we don't have a field.

	if (node && name) {
		node = ambiguity_check(statement, node, name, ambiguous_ctx_stack);
	}

	// Clean up stack
	ambiguous_ctx_stack.clear();

	if (node)
	{
		return node;
	}

	field_unknown(qualifier ? qualifier->str_data : NULL, name ? name->str_data : NULL, input);

	// CVC: field_unknown() calls ERRD_post() that never returns, so the next line
	// is only to make the compiler happy.
	return NULL;
}


/**

 	pass1_found_aggregate

    @brief	Check the fields inside an aggregate
   and check if the field scope_level
   meets the specified conditions.
   In the first call current_scope_level_equal
   should always be true, because this is used
   internally!


    @param node
    @param check_scope_level
    @param match_type
    @param current_scope_level_equal

 **/
static bool pass1_found_aggregate(const dsql_nod* node, USHORT check_scope_level,
									 USHORT match_type, bool current_scope_level_equal)
{
	DEV_BLKCHK(node, dsql_type_nod);

	if (node == NULL)
		return false;

	bool found = false;

	switch (node->nod_type)
	{
		case nod_gen_id:
		case nod_gen_id2:
		case nod_cast:
		case nod_udf:
		case nod_sys_function:
			// If arguments are given to the UDF/SDF then there's a node list
			if (node->nod_count == 2) {
				found |= pass1_found_aggregate(node->nod_arg[1], check_scope_level,
											   match_type, current_scope_level_equal);
			}
			break;

		case nod_exists:
		case nod_singular:
		case nod_coalesce:
		case nod_simple_case:
		case nod_searched_case:
		case nod_add:
		case nod_concatenate:
		case nod_divide:
		case nod_multiply:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_strlen:
		case nod_add2:
		case nod_divide2:
		case nod_multiply2:
		case nod_subtract2:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_leq_any:
		case nod_lss_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_leq_all:
		case nod_lss_all:
		case nod_between:
		case nod_like:
		case nod_missing:
		case nod_and:
		case nod_or:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_not:
		case nod_unique:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_list:
		case nod_join:
		case nod_join_inner:
		case nod_join_left:
		case nod_join_right:
		case nod_join_full:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ++ptr)
				{
					found |= pass1_found_aggregate(*ptr, check_scope_level,
												   match_type, current_scope_level_equal);
				}
			}
			break;

		case nod_via:
			// Pass only the rse from the nod_via
			found |= pass1_found_aggregate(node->nod_arg[e_via_rse], check_scope_level,
										   match_type, current_scope_level_equal);
			break;

		case nod_rse:
			// Pass rse_boolean (where clause) and rse_items (select items)
			found |= pass1_found_aggregate(node->nod_arg[e_rse_boolean], check_scope_level,
										   match_type, false);
			found |= pass1_found_aggregate(node->nod_arg[e_rse_items], check_scope_level,
										   match_type, false);
			break;

		case nod_alias:
			found |= pass1_found_aggregate(node->nod_arg[e_alias_value], check_scope_level,
										   match_type, current_scope_level_equal);
			break;

		case nod_aggregate:
			// Pass only rse_group (group by clause)
			found |= pass1_found_aggregate(node->nod_arg[e_agg_group], check_scope_level,
										   match_type, current_scope_level_equal);
			break;

		case nod_agg_average:
		case nod_agg_count:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_list:
			{
				bool field = false;
				if (node->nod_count) {
					found |= pass1_found_field(node->nod_arg[e_agg_function_expression],
											   check_scope_level, match_type, &field);
				}
				if (!field)
				{
					// For example COUNT(*) is always same scope_level (node->nod_count = 0)
					// Normaly COUNT(*) is the only way to come here but something stupid
					// as SUM(5) is also possible.
					// If current_scope_level_equal is false scope_level is always higher
					switch (match_type)
					{
						case FIELD_MATCH_TYPE_LOWER_EQUAL:
						case FIELD_MATCH_TYPE_EQUAL:
							return current_scope_level_equal;

						case FIELD_MATCH_TYPE_HIGHER_EQUAL:
							return true;

						case FIELD_MATCH_TYPE_LOWER:
						case FIELD_MATCH_TYPE_HIGHER:
							return false;

						default:
							fb_assert(false);
					}
				}
			}
			break;

		case nod_map:
			{
				const dsql_map* map = reinterpret_cast<dsql_map*>(node->nod_arg[e_map_map]);
				found |= pass1_found_aggregate(map->map_node, check_scope_level, match_type,
											   current_scope_level_equal);
			}
			break;

		case nod_derived_field:
		case nod_dbkey:
		case nod_field:
		case nod_parameter:
		case nod_relation:
		case nod_variable:
		case nod_constant:
		case nod_null:
		case nod_current_date:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_user_name:
		case nod_current_role:
		case nod_internal_info:
		case nod_dom_value:
			return false;

		case nod_hidden_var:
			found |= pass1_found_aggregate(node->nod_arg[e_hidden_var_expr], check_scope_level,
										   match_type, current_scope_level_equal);
			break;

		default:
			fb_assert(false);
	}

	return found;
}


/**

 	pass1_found_field

    @brief	Check the fields inside an aggregate
   and check if the field scope_level
   meets the specified conditions.


    @param node
    @param check_scope_level
    @param match_type
    @param field

 **/
static bool pass1_found_field(const dsql_nod* node, USHORT check_scope_level,
								 USHORT match_type, bool* field)
{
	DEV_BLKCHK(node, dsql_type_nod);

	if (node == NULL)
		return false;

	bool found = false;

	switch (node->nod_type)
	{
		case nod_field:
			{
				const dsql_ctx* field_context = (dsql_ctx*) node->nod_arg[e_fld_context];
				DEV_BLKCHK(field_context, dsql_type_ctx);
				*field = true;
				switch (match_type)
				{
					case FIELD_MATCH_TYPE_EQUAL:
						return (field_context->ctx_scope_level == check_scope_level);

					case FIELD_MATCH_TYPE_LOWER:
						return (field_context->ctx_scope_level < check_scope_level);

					case FIELD_MATCH_TYPE_LOWER_EQUAL:
						return (field_context->ctx_scope_level <= check_scope_level);

					case FIELD_MATCH_TYPE_HIGHER:
						return (field_context->ctx_scope_level > check_scope_level);

					case FIELD_MATCH_TYPE_HIGHER_EQUAL:
						return (field_context->ctx_scope_level >= check_scope_level);

					default:
						fb_assert(false);
				}
			}
			break;

		case nod_gen_id:
		case nod_gen_id2:
		case nod_cast:
		case nod_udf:
		case nod_sys_function:
			// If arguments are given to the UDF/SDF then there's a node list
			if (node->nod_count == 2) {
				found |= pass1_found_field(node->nod_arg[1], check_scope_level, match_type, field);
			}
			break;

		case nod_exists:
		case nod_singular:
		case nod_coalesce:
		case nod_simple_case:
		case nod_searched_case:
		case nod_add:
		case nod_concatenate:
		case nod_divide:
		case nod_multiply:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_strlen:
		case nod_add2:
		case nod_divide2:
		case nod_multiply2:
		case nod_subtract2:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_leq_any:
		case nod_lss_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_leq_all:
		case nod_lss_all:
		case nod_between:
		case nod_like:
		case nod_missing:
		case nod_and:
		case nod_or:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_not:
		case nod_unique:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_list:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
				{
					found |= pass1_found_field(*ptr, check_scope_level, match_type, field);
				}
			}
			break;

		case nod_via:
			// Pass only the rse from the nod_via
			found |= pass1_found_field(node->nod_arg[e_via_rse], check_scope_level, match_type, field);
			break;

		case nod_rse:
			// Pass rse_boolean (where clause) and rse_items (select items)
			found |= pass1_found_field(node->nod_arg[e_rse_boolean], check_scope_level,
									   match_type, field);
			found |= pass1_found_field(node->nod_arg[e_rse_items], check_scope_level,
									   match_type, field);
			break;

		case nod_alias:
			found |= pass1_found_field(node->nod_arg[e_alias_value], check_scope_level,
									   match_type, field);
			break;

		case nod_derived_field:
			{
				// This is a "virtual" field
				*field = true;
				const USHORT df_scope_level = (USHORT)(U_IPTR) node->nod_arg[e_derived_field_scope];

				switch (match_type)
				{
				case FIELD_MATCH_TYPE_EQUAL:
					return (df_scope_level == check_scope_level);

				case FIELD_MATCH_TYPE_LOWER:
					return (df_scope_level < check_scope_level);

				case FIELD_MATCH_TYPE_LOWER_EQUAL:
					return (df_scope_level <= check_scope_level);

				case FIELD_MATCH_TYPE_HIGHER:
					return (df_scope_level > check_scope_level);

				case FIELD_MATCH_TYPE_HIGHER_EQUAL:
					return (df_scope_level >= check_scope_level);

				default:
					fb_assert(false);
				}
			}
			break;

		case nod_aggregate:
			// Pass only rse_group (group by clause)
			found |= pass1_found_field(node->nod_arg[e_agg_group], check_scope_level,
									   match_type, field);
			break;

		case nod_agg_average:
		case nod_agg_count:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_list:
			if (node->nod_count) {
				found |= pass1_found_field(node->nod_arg[e_agg_function_expression],
										   check_scope_level, match_type, field);
			}
			break;

		case nod_map:
			{
				const dsql_map* map = reinterpret_cast<dsql_map*>(node->nod_arg[e_map_map]);
				found |= pass1_found_field(map->map_node, check_scope_level, match_type, field);
			}
			break;

		case nod_dbkey:
		case nod_parameter:
		case nod_relation:
		case nod_variable:
		case nod_constant:
		case nod_null:
		case nod_current_date:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_user_name:
		case nod_current_role:
		case nod_internal_info:
		case nod_dom_value:
			return false;

		case nod_hidden_var:
			found |= pass1_found_field(node->nod_arg[e_hidden_var_expr], check_scope_level,
									   match_type, field);
			break;

		default:
			fb_assert(false);
	}

	return found;
}


/**

 	pass1_found_sub_select

    @brief	Search if a sub select is buried inside
   an select list from an query expression.

    @param node

 **/
static bool pass1_found_sub_select(const dsql_nod* node)
{
	DEV_BLKCHK(node, dsql_type_nod);

	if (node == NULL)
		return false;

	switch (node->nod_type)
	{
		case nod_gen_id:
		case nod_gen_id2:
		case nod_cast:
		case nod_udf:
		case nod_sys_function:
			// If arguments are given to the UDF/SDF then there's a node list
			if (node->nod_count == 2) {
				if (pass1_found_sub_select(node->nod_arg[1])) {
					return true;
				}
			}
			break;

		case nod_exists:
		case nod_singular:
		case nod_coalesce:
		case nod_simple_case:
		case nod_searched_case:
		case nod_add:
		case nod_concatenate:
		case nod_divide:
		case nod_multiply:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_strlen:
		case nod_add2:
		case nod_divide2:
		case nod_multiply2:
		case nod_subtract2:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_leq_any:
		case nod_lss_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_leq_all:
		case nod_lss_all:
		case nod_between:
		case nod_like:
		case nod_missing:
		case nod_and:
		case nod_or:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_not:
		case nod_unique:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_list:
		case nod_join:
		case nod_join_inner:
		case nod_join_left:
		case nod_join_right:
		case nod_join_full:
			{
				const dsql_nod* const* ptr = node->nod_arg;
				for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ++ptr)
				{
					if (pass1_found_sub_select(*ptr)) {
						return true;
					}
				}
			}
			break;

		case nod_via:
			return true;

		case nod_alias:
			if (pass1_found_sub_select(node->nod_arg[e_alias_value])) {
				return true;
			}
			break;

		case nod_aggregate:
		case nod_agg_average:
		case nod_agg_count:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_list:
		case nod_map:

		case nod_derived_field:
		case nod_dbkey:
		case nod_field:
		case nod_parameter:
		case nod_relation:
		case nod_variable:
		case nod_constant:
		case nod_null:
		case nod_current_date:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_user_name:
		case nod_current_role:
		case nod_internal_info:
		case nod_dom_value:
		case nod_field_name:
		case nod_var_name:
			return false;

		case nod_hidden_var:
			if (pass1_found_sub_select(node->nod_arg[e_hidden_var_expr]))
				return true;
			break;

		default:
			return true;
	}

	return false;
}


/**

 	pass1_group_by_list

    @brief	Process GROUP BY list, which may contain
			an ordinal or alias which references the
			select list.

    @param statement
    @param input
    @param select_list

 **/
static dsql_nod* pass1_group_by_list(CompiledStatement* statement, dsql_nod* input, dsql_nod* selectList)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(selectList, dsql_type_nod);

	if (input->nod_count > MAX_SORT_ITEMS) // sort, group and distinct have the same limit for now
	{
		// cannot group on more than 255 items
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_max_group_items));
	}

	DsqlNodStack stack;
	dsql_nod** ptr = input->nod_arg;
	for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		DEV_BLKCHK(*ptr, dsql_type_nod);
		dsql_nod* sub = (*ptr);
		dsql_nod* frnode = NULL;
		if (sub->nod_type == nod_field_name)
		{
			// check for alias or field node
			frnode = pass1_field(statement, sub, false, selectList);
		}
		else if ((sub->nod_type == nod_constant) && (sub->nod_desc.dsc_dtype == dtype_long))
		{
			const ULONG position = sub->getSlong();
			if ((position < 1) || !selectList ||
				(position > (ULONG) selectList->nod_count))
			{
				// Invalid column position used in the GROUP BY clause
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_column_pos_err) << Arg::Str("GROUP BY"));
			}
			frnode = pass1_node_psql(statement, selectList->nod_arg[position - 1], false);
		}
		else
		{
			frnode = pass1_node_psql(statement, *ptr, false);
		}

		stack.push(frnode);
	}
	// Finally make the complete list.
	dsql_nod* node = MAKE_list(stack);

	return node;
}


// Create (if necessary) a hidden variable to store a temporary value.
static dsql_nod* pass1_hidden_variable(CompiledStatement* statement, dsql_nod*& expr)
{
	// For some node types, it's better to not create temporary value.
	switch (expr->nod_type)
	{
		case nod_constant:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_dbkey:
		case nod_field:
		case nod_internal_info:
		case nod_null:
		case nod_parameter:
		case nod_user_name:
		case nod_variable:
			return NULL;
	}

	dsql_nod* var = MAKE_variable(NULL, "", VAR_local, 0, 0, statement->req_hidden_vars_number++);
	MAKE_desc(statement, &var->nod_desc, expr, NULL);

	dsql_nod* newExpr = MAKE_node(nod_hidden_var, e_hidden_var_count);
	newExpr->nod_arg[e_hidden_var_expr] = expr;
	newExpr->nod_arg[e_hidden_var_var] = var;
	expr = newExpr;

	statement->req_hidden_vars.push(newExpr);

	return var;
}


/**

 	pass1_insert

    @brief	Process INSERT statement.


    @param statement
    @param input

 **/
static dsql_nod* pass1_insert( CompiledStatement* statement, dsql_nod* input, bool insert_or_update)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	statement->req_type = REQ_INSERT;
	dsql_nod* node = MAKE_node(nod_store, e_sto_count);

	// Process SELECT expression, if present

	dsql_nod* values;
	dsql_nod* rse = input->nod_arg[e_ins_select];
	if (rse)
	{
		if (input->nod_arg[e_ins_return])
			rse->nod_flags |= NOD_SELECT_EXPR_SINGLETON;

		node->nod_arg[e_sto_rse] = rse = PASS1_rse(statement, rse, NULL);
		values = rse->nod_arg[e_rse_items];
	}
	else
		values = pass1_node_psql(statement, input->nod_arg[e_ins_values], false);

	// Process relation

	dsql_nod* temp_rel = pass1_relation(statement, input->nod_arg[e_ins_relation]);
	node->nod_arg[e_sto_relation] = temp_rel;
	dsql_ctx* context = (dsql_ctx*) temp_rel->nod_arg[0];
	DEV_BLKCHK(context, dsql_type_ctx);
	dsql_rel* relation = context->ctx_relation;

	// If there isn't a field list, generate one

	dsql_nod* fields = input->nod_arg[e_ins_fields];
	if (fields)
	{
		const dsql_nod* old_fields = fields; // for error reporting.
		fields = pass1_node_psql(statement, fields, false);
		// We do not allow cases like INSERT INTO T(f1, f2, f1)...
		field_appears_once(fields, old_fields, true, "INSERT");

		// begin IBO hack
		// 02-May-2004, Nickolay Samofatov. Do not constify ptr further e.g. to
		// const dsql_nod* const* .... etc. It chokes GCC 3.4.0
		dsql_nod** ptr = fields->nod_arg;
		for (const dsql_nod* const* const end = ptr + fields->nod_count; ptr < end; ptr++)
		{
			DEV_BLKCHK (*ptr, dsql_type_nod);
			const dsql_nod* temp2 = *ptr;

			const dsql_ctx* tmp_ctx = NULL;
			const TEXT* tmp_name = NULL;

			if (temp2->nod_type == nod_field)
			{
				tmp_ctx = (dsql_ctx*) temp2->nod_arg[e_fld_context];
				tmp_name = (temp2->nod_arg[e_fld_field] ?
					((dsql_fld*) temp2->nod_arg[e_fld_field])->fld_name.c_str() : NULL);
			}
			else if (temp2->nod_type == nod_derived_field)
			{
				tmp_ctx = (dsql_ctx*) temp2->nod_arg[e_derived_field_context];
				tmp_name = (temp2->nod_arg[e_derived_field_name] ?
					((dsql_str*) temp2->nod_arg[e_derived_field_name])->str_data : NULL);
			}

			if (tmp_ctx &&
				((tmp_ctx->ctx_relation && relation->rel_name != tmp_ctx->ctx_relation->rel_name) ||
				 tmp_ctx->ctx_context != context->ctx_context))
			{
				const dsql_rel* bad_rel = tmp_ctx->ctx_relation;
				// At this time, "fields" has been replaced by the processed list in
				// the same variable, so we refer again to input->nod_arg[e_ins_fields].
				// CVC: After three years, made old_fields for that purpose.

				field_unknown(bad_rel ? bad_rel->rel_name.c_str() : NULL,
								tmp_name, old_fields->nod_arg[ptr - fields->nod_arg]);
			}
		}
		// end IBO hack
	}
	else
		fields = pass1_node_psql(statement, explode_fields(relation), false);

	// Match field fields and values

	DsqlNodStack stack;

	if (values)
	{
		if (fields->nod_count != values->nod_count) {
			// count of column list and value list don't match
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_var_count_err));
		}

		dsql_nod** ptr = fields->nod_arg;
		dsql_nod** ptr2 = values->nod_arg;
		for (const dsql_nod* const* const end = ptr + fields->nod_count; ptr < end; ptr++, ptr2++)
		{
			DEV_BLKCHK(*ptr, dsql_type_nod);
			DEV_BLKCHK(*ptr2, dsql_type_nod);
			dsql_nod* temp = MAKE_node(nod_assign, e_asgn_count);
			temp->nod_arg[e_asgn_value] = *ptr2;
			temp->nod_arg[e_asgn_field] = *ptr;
			stack.push(temp);
			temp = *ptr2;
			set_parameter_type(statement, temp, *ptr, false);
		}
	}

	node->nod_arg[e_sto_statement] = MAKE_list(stack);

	if (insert_or_update)
	{
		// Clone the insert context, push with name "OLD" in the same scope level and
		// marks it with CTX_null so all fields be resolved to NULL constant.
		dsql_ctx* old_context = FB_NEW(statement->req_pool) dsql_ctx(statement->req_pool);
		*old_context = *context;
		old_context->ctx_alias = old_context->ctx_internal_alias = MAKE_cstring(OLD_CONTEXT)->str_data;
		old_context->ctx_flags |= CTX_system | CTX_null | CTX_returning;
		statement->req_context->push(old_context);

		// clone the insert context and push with name "NEW" in a greater scope level
		dsql_ctx* new_context = FB_NEW(statement->req_pool) dsql_ctx(statement->req_pool);
		*new_context = *context;
		new_context->ctx_scope_level = ++statement->req_scope_level;
		new_context->ctx_alias = new_context->ctx_internal_alias = MAKE_cstring(NEW_CONTEXT)->str_data;
		new_context->ctx_flags |= CTX_system | CTX_returning;
		statement->req_context->push(new_context);
	}

	node->nod_arg[e_sto_return] = process_returning(statement, input->nod_arg[e_ins_return]);

	if (insert_or_update)
	{
		--statement->req_scope_level;
		statement->req_context->pop();
		statement->req_context->pop();
	}

	set_parameters_name(node->nod_arg[e_sto_statement], node->nod_arg[e_sto_relation]);

	if (!insert_or_update)
		node = nullify_returning(statement, node);

	statement->req_context->pop();

	return node;
}


/**

 	pass1_join

    @brief	Make up join node and mark relations as "possibly NULL"
	if they are in outer joins (req_in_outer_join).


    @param statement
    @param input

 **/
static dsql_nod* pass1_join(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	// Recreate the node

	dsql_nod* const node = MAKE_node(input->nod_type, input->nod_count);

	// First process type

	node->nod_arg[e_join_type] = PASS1_node(statement, input->nod_arg[e_join_type]);

	// Process relations

	switch (node->nod_arg[e_join_type]->nod_type)
	{
		case nod_join_inner:
			node->nod_arg[e_join_left_rel] = PASS1_node(statement, input->nod_arg[e_join_left_rel]);
			node->nod_arg[e_join_rght_rel] = PASS1_node(statement, input->nod_arg[e_join_rght_rel]);
			break;
		case nod_join_left:
			node->nod_arg[e_join_left_rel] = PASS1_node(statement, input->nod_arg[e_join_left_rel]);
			statement->req_in_outer_join++;
			node->nod_arg[e_join_rght_rel] = PASS1_node(statement, input->nod_arg[e_join_rght_rel]);
			statement->req_in_outer_join--;
			break;
		case nod_join_right:
			statement->req_in_outer_join++;
			node->nod_arg[e_join_left_rel] = PASS1_node(statement, input->nod_arg[e_join_left_rel]);
			statement->req_in_outer_join--;
			node->nod_arg[e_join_rght_rel] = PASS1_node(statement, input->nod_arg[e_join_rght_rel]);
			break;
		case nod_join_full:
			statement->req_in_outer_join++;
			node->nod_arg[e_join_left_rel] = PASS1_node(statement, input->nod_arg[e_join_left_rel]);
			node->nod_arg[e_join_rght_rel] = PASS1_node(statement, input->nod_arg[e_join_rght_rel]);
			statement->req_in_outer_join--;
			break;

		default:
			fb_assert(false);	// join type expected
			break;
	}

	// Process boolean

	dsql_nod* boolean = input->nod_arg[e_join_boolean];
	if (boolean && (boolean->nod_type == nod_flag || boolean->nod_type == nod_list))
	{
		if (statement->req_client_dialect < SQL_DIALECT_V6)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_dsql_unsupp_feature_dialect) << Arg::Num(statement->req_client_dialect));
		}

		DsqlNodStack leftStack, rightStack;

		if (boolean->nod_type == nod_flag)	// NATURAL JOIN
		{
			StrArray leftNames(statement->req_pool);
			DsqlNodStack matched;

			pass1_expand_select_node(statement, node->nod_arg[e_join_left_rel], leftStack, true);
			pass1_expand_select_node(statement, node->nod_arg[e_join_rght_rel], rightStack, true);

			// verify columns that exist in both sides
			for (int i = 0; i < 2; ++i)
			{
				for (DsqlNodStack::const_iterator j(i == 0 ? leftStack : rightStack); j.hasData(); ++j)
				{
					const TEXT* name = NULL;
					dsql_nod* item = j.object();

					switch (item->nod_type)
					{
						case nod_alias:
							name = reinterpret_cast<dsql_str*>(item->nod_arg[e_alias_alias])->str_data;
							break;

						case nod_field:
							name = reinterpret_cast<dsql_fld*>(item->nod_arg[e_fld_field])->fld_name.c_str();
							break;

						case nod_derived_field:
							name = reinterpret_cast<dsql_str*>(item->nod_arg[e_derived_field_name])->str_data;
							break;

						default:
							break;
					}

					if (name)
					{
						if (i == 0)	// left
							leftNames.add(name);
						else	// right
						{
							if (leftNames.exist(name))
							{
								matched.push(MAKE_field_name(name));
							}
						}
					}
				}
			}

			if (matched.getCount() == 0)
			{
				// There is no match. Transform to CROSS JOIN.
				node->nod_arg[e_join_type]->nod_type = nod_join_inner;
				boolean = NULL;
			}
			else
				boolean = MAKE_list(matched);	// Transform to USING
		}

		if (boolean)	// JOIN ... USING
		{
			fb_assert(boolean->nod_type == nod_list);

			dsql_nod* newBoolean = NULL;
			StrArray usedColumns(statement->req_pool);

			for (int i = 0; i < boolean->nod_count; ++i)
			{
				dsql_nod* field = boolean->nod_arg[i];
				dsql_str* fldName = reinterpret_cast<dsql_str*>(field->nod_arg[e_fln_name]);

				// verify if the column was already used
				size_t pos;
				if (usedColumns.find(fldName->str_data, pos))
				{
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_col_more_than_once_using) << Arg::Str(fldName->str_data));
				}
				else
					usedColumns.insert(pos, fldName->str_data);

				dsql_ctx* leftCtx = NULL;
				dsql_ctx* rightCtx = NULL;

				// clear the stacks for the next pass
				leftStack.clear();
				rightStack.clear();

				// get the column names from both sides
				pass1_expand_select_node(statement, node->nod_arg[e_join_left_rel], leftStack, true);
				pass1_expand_select_node(statement, node->nod_arg[e_join_rght_rel], rightStack, true);

				// create the boolean
				dsql_nod* eqlNode = MAKE_node(nod_eql, 2);
				eqlNode->nod_arg[0] =
					resolve_using_field(statement, fldName, leftStack, field, "left", leftCtx);
				eqlNode->nod_arg[1] =
					resolve_using_field(statement, fldName, rightStack, field, "right", rightCtx);

				fb_assert(leftCtx);
				fb_assert(rightCtx);

				// We should hide the (unqualified) column in one side
				ImplicitJoin* impJoinLeft;
				if (!leftCtx->ctx_imp_join.get(fldName->str_data, impJoinLeft))
				{
					impJoinLeft = FB_NEW(statement->req_pool) ImplicitJoin();
					impJoinLeft->value = eqlNode->nod_arg[0];
					impJoinLeft->visibleInContext = leftCtx;
				}
				else
					fb_assert(impJoinLeft->visibleInContext == leftCtx);

				ImplicitJoin* impJoinRight;
				if (!rightCtx->ctx_imp_join.get(fldName->str_data, impJoinRight))
				{
					impJoinRight = FB_NEW(statement->req_pool) ImplicitJoin();
					impJoinRight->value = eqlNode->nod_arg[1];
				}
				else
					fb_assert(impJoinRight->visibleInContext == rightCtx);

				// create the COALESCE
				DsqlNodStack stack;

				dsql_nod* temp = impJoinLeft->value;
				if (temp->nod_type == nod_alias)
					temp = temp->nod_arg[e_alias_value];

				{	// scope
					PsqlChanger changer(statement, false);

					if (temp->nod_type == nod_coalesce)
					{
						pass1_put_args_on_stack(statement, temp->nod_arg[0], stack);
						pass1_put_args_on_stack(statement, temp->nod_arg[1], stack);
					}
					else
						pass1_put_args_on_stack(statement, temp, stack);

					if ((temp = impJoinRight->value)->nod_type == nod_alias)
						temp = temp->nod_arg[e_alias_value];
					if (temp->nod_type == nod_coalesce)
					{
						pass1_put_args_on_stack(statement, temp->nod_arg[0], stack);
						pass1_put_args_on_stack(statement, temp->nod_arg[1], stack);
					}
					else
						pass1_put_args_on_stack(statement, temp, stack);
				}

				dsql_nod* coalesce = MAKE_node(nod_coalesce, 2);
				coalesce->nod_arg[0] = stack.pop();
				coalesce->nod_arg[1] = MAKE_list(stack);

				impJoinLeft->value = MAKE_node(nod_alias, e_alias_count);
				impJoinLeft->value->nod_arg[e_alias_value] = PASS1_node(statement, coalesce);
				impJoinLeft->value->nod_arg[e_alias_alias] = reinterpret_cast<dsql_nod*>(fldName);
				impJoinLeft->value->nod_arg[e_alias_imp_join] = reinterpret_cast<dsql_nod*>(impJoinLeft);

				impJoinRight->visibleInContext = NULL;

				// both sides should refer to the same ImplicitJoin
				leftCtx->ctx_imp_join.put(fldName->str_data, impJoinLeft);
				rightCtx->ctx_imp_join.put(fldName->str_data, impJoinLeft);

				if (newBoolean)
				{
					temp = MAKE_node(nod_and, 2);
					temp->nod_arg[0] = newBoolean;
					temp->nod_arg[1] = eqlNode;
					newBoolean = temp;
				}
				else
					newBoolean = eqlNode;
			}

			boolean = newBoolean;
		}
	}

	node->nod_arg[e_join_boolean] = PASS1_node(statement, boolean);

	return node;
}


/**

 	pass1_label

    @brief	Process loop interruption


    @param statement
    @param input

 **/
static dsql_nod* pass1_label(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* label = NULL;

	// retrieve a label

	switch (input->nod_type)
	{
	case nod_breakleave:
		label = input->nod_arg[e_breakleave_label];
		break;
	case nod_for_select:
		label = input->nod_arg[e_flp_label];
		break;
	case nod_exec_into:
		label = input->nod_arg[e_exec_into_label];
		break;
	case nod_exec_stmt:
		label = input->nod_arg[e_exec_stmt_label];
		break;
	case nod_while:
		label = input->nod_arg[e_while_label];
		break;
	default:
		fb_assert(false);
	}

	// look for a label, if specified

	const dsql_str* string = NULL;
	USHORT position = 0;

	if (label)
	{
		fb_assert(label->nod_type == nod_label);
		string = (dsql_str*) label->nod_arg[e_label_name];
		const TEXT* label_string = string->str_data;
		int index = statement->req_loop_level;
		for (DsqlStrStack::iterator stack(statement->req_labels); stack.hasData(); ++stack)
		{
			const dsql_str* obj = stack.object();
			if (obj) {
				const TEXT* obj_string = obj->str_data;
				if (!strcmp(label_string, obj_string)) {
					position = index;
					break;
				}
			}
			index--;
		}
	}

	USHORT number = 0;
	if (input->nod_type == nod_breakleave)
	{
		if (position > 0) {
			// break the specified loop
			number = position;
		}
		else if (label) {
			// ERROR: Label %s is not found in the current scope
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(isc_dsql_invalid_label) << Arg::Str(string->str_data) <<
														  Arg::Str("is not found"));
		}
		else {
			// break the current loop
			number = statement->req_loop_level;
		}
	}
	else
	{
		if (position > 0) {
			// ERROR: Label %s already exists in the current scope
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(isc_dsql_invalid_label) << Arg::Str(string->str_data) <<
														  Arg::Str("already exists"));
		}
		else {
			// store label name, if specified
			statement->req_labels.push(string);
			number = statement->req_loop_level;
		}
	}

	fb_assert(number > 0 && number <= statement->req_loop_level);

	if (!label) {
		label = MAKE_node(nod_label, e_label_count);
		// this label is unnamed, i.e. its nod_arg[e_label_name] is NULL
	}
	label->nod_arg[e_label_number] = (dsql_nod*) (IPTR) number;

	return label;
}


/**

 	pass1_limit

    @brief	Process the limit clause (FIRST/SKIP/ROWS)


    @param statement
    @param input
	@param output

 **/
static void pass1_limit(CompiledStatement* statement, dsql_nod* input, dsql_nod* output)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(output, dsql_type_nod);

	dsql_nod *first, *skip;

	if (input->nod_type == nod_limit)
	{
		first = input->nod_arg[e_limit_length];
		skip = input->nod_arg[e_limit_skip];
	}
	else if (input->nod_type == nod_rows)
	{
		first = input->nod_arg[e_rows_length];
		skip = input->nod_arg[e_rows_skip];
	}
	else
	{
		fb_assert(false);
	}

	if (first)
	{
		dsql_nod* const sub = pass1_node_psql(statement, first, false);
		output->nod_arg[e_rse_first] = sub;
		set_parameter_type(statement, sub, input, false);
	}

	if (skip)
	{
		dsql_nod* const sub = pass1_node_psql(statement, skip, false);
		output->nod_arg[e_rse_skip] = sub;
		set_parameter_type(statement, sub, input, false);
	}
}


/**

 	pass1_lookup_alias

    @brief	Lookup a matching item in the select list.
			Return node if found else return NULL.
			If more matches are found we raise ambiguity error.

    @param statement
    @param name
    @param selectList

 **/
static dsql_nod* pass1_lookup_alias(CompiledStatement* statement, const dsql_str* name, dsql_nod* selectList,
									bool process)
{
	dsql_nod* returnNode = NULL;
	dsql_nod** ptr = selectList->nod_arg;
	const dsql_nod* const* const end = ptr + selectList->nod_count;
	for (; ptr < end; ptr++)
	{
		dsql_nod* matchingNode = NULL;
		dsql_nod* node = *ptr;
		switch (node->nod_type)
		{
			case nod_alias:
				{
					const dsql_str* alias = (dsql_str*) node->nod_arg[e_alias_alias];
					if (!strcmp(alias->str_data, name->str_data)) {
						matchingNode = node;
					}
				}
				break;

			case nod_field:
				{
					const dsql_fld* field = (dsql_fld*) node->nod_arg[e_fld_field];
					if (field->fld_name == name->str_data)
						matchingNode = node;
				}
				break;

			case nod_derived_field:
				{
					const dsql_str* alias = (dsql_str*) node->nod_arg[e_derived_field_name];
					if (!strcmp(alias->str_data, name->str_data)) {
						matchingNode = node;
					}
				}
				break;

			default:
				break;
		}
		if (matchingNode)
		{
			if (process)
				matchingNode = pass1_node_psql(statement, matchingNode, false);

			if (returnNode)
			{
				// There was already a node matched, thus raise ambiguous field name error.
				TEXT buffer1[256];
				buffer1[0] = 0;
				switch (returnNode->nod_type)
				{
					case nod_field:
						strcat(buffer1, "a field");
						break;

					case nod_alias:
						strcat(buffer1, "an alias");
						break;

					case nod_derived_field:
						strcat(buffer1, "a derived field");
						break;

					default:
						strcat(buffer1, "an item");
						break;
				}

				TEXT buffer2[256];
				buffer2[0] = 0;
				switch (matchingNode->nod_type)
				{
					case nod_field:
						strcat(buffer2, "a field");
						break;

					case nod_alias:
						strcat(buffer2, "an alias");
						break;

					case nod_derived_field:
						strcat(buffer2, "a derived field");
						break;

					default:
						strcat(buffer2, "an item");
						break;
				}
				strcat(buffer2, " in the select list with name");

				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(isc_dsql_ambiguous_field_name) << Arg::Str(buffer1) <<
																	 Arg::Str(buffer2) <<
						  Arg::Gds(isc_random) << Arg::Str(name->str_data));
			}
			returnNode = matchingNode;
		}
	}

	return returnNode;
}

/**

 	pass1_make_derived_field

    @brief	Create a derived field based on underlying expressions


    @param statement
    @param tdbb
    @param select_item

 **/
static dsql_nod* pass1_make_derived_field(CompiledStatement* statement, thread_db* tdbb,
	dsql_nod* select_item)
{
	DEV_BLKCHK(select_item, dsql_type_nod);

	switch (select_item->nod_type)
	{
		case nod_derived_field:
			{
				// Create a derived field that points to a derived field
				dsql_nod* derived_field = MAKE_node(nod_derived_field, e_derived_field_count);
				derived_field->nod_arg[e_derived_field_value] = select_item;
				derived_field->nod_arg[e_derived_field_name] = select_item->nod_arg[e_derived_field_name];
				derived_field->nod_arg[e_derived_field_scope] = (dsql_nod*)(IPTR) statement->req_scope_level;
				derived_field->nod_desc = select_item->nod_desc;
				return derived_field;
			}

		case nod_field:
			{
				const dsql_fld* field = (dsql_fld*) select_item->nod_arg[e_fld_field];
				DEV_BLKCHK(field, dsql_type_fld);

				// Copy fieldname to a new string.
				dsql_str* alias = FB_NEW_RPT(*tdbb->getDefaultPool(), field->fld_name.length()) dsql_str;
				strcpy(alias->str_data, field->fld_name.c_str());
				alias->str_length = field->fld_name.length();

				// Create a derived field and hook in.
				dsql_nod* derived_field = MAKE_node(nod_derived_field, e_derived_field_count);
				derived_field->nod_arg[e_derived_field_value] = select_item;
				derived_field->nod_arg[e_derived_field_name] = (dsql_nod*) alias;
				derived_field->nod_arg[e_derived_field_scope] = (dsql_nod*)(IPTR) statement->req_scope_level;
				derived_field->nod_desc = select_item->nod_desc;
				return derived_field;
			}

		case nod_alias:
			{
				// Copy aliasname to a new string.
				const dsql_str* alias_alias = (dsql_str*) select_item->nod_arg[e_alias_alias];
				dsql_str* alias = FB_NEW_RPT(*tdbb->getDefaultPool(), strlen(alias_alias->str_data)) dsql_str;
				strcpy(alias->str_data, alias_alias->str_data);
				alias->str_length = strlen(alias_alias->str_data);

				// Create a derived field and ignore alias node.
				dsql_nod* derived_field = MAKE_node(nod_derived_field, e_derived_field_count);
				derived_field->nod_arg[e_derived_field_value] = select_item->nod_arg[e_alias_value];
				derived_field->nod_arg[e_derived_field_name] = (dsql_nod*) alias;
				derived_field->nod_arg[e_derived_field_scope] = (dsql_nod*)(IPTR) statement->req_scope_level;
				derived_field->nod_desc = select_item->nod_desc;
				return derived_field;
			}

		case nod_map :
			{
				// Aggregate's have map on top.
				dsql_map* map = (dsql_map*) select_item->nod_arg[e_map_map];
				dsql_nod* derived_field = pass1_make_derived_field(statement, tdbb, map->map_node);

				// If we had succesfully made a derived field node change it
				// with orginal map.
				if (derived_field->nod_type == nod_derived_field) {
					derived_field->nod_arg[e_derived_field_value] = select_item;
					derived_field->nod_arg[e_derived_field_scope] = (dsql_nod*)(IPTR) statement->req_scope_level;
					derived_field->nod_desc = select_item->nod_desc;
					return derived_field;
				}

				return select_item;
			}

		case nod_via :
			{
				// Try to generate derived field from sub-select
				dsql_nod* derived_field = pass1_make_derived_field(statement, tdbb,
					select_item->nod_arg[e_via_value_1]);
				if (derived_field->nod_type == nod_derived_field)
				{
					derived_field->nod_arg[e_derived_field_value] = select_item;
					return derived_field;
				}

				return select_item;
			}

		default:
			break;
	}

	return select_item;
}


/**
 	pass1_merge

    @brief	Process MERGE statement.


    @param statement
    @param input

 **/
static dsql_nod* pass1_merge(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* source = input->nod_arg[e_mrg_using];		// USING
	dsql_nod* target = input->nod_arg[e_mrg_relation];	// INTO

	// build a join between USING and INTO tables
	dsql_nod* join = MAKE_node(nod_join, e_join_count);
	join->nod_arg[e_join_left_rel] = source;

	// left join if WHEN NOT MATCHED is present
	if (input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_not_matched])
		join->nod_arg[e_join_type] = MAKE_node(nod_join_left, 0);
	else
		join->nod_arg[e_join_type] = MAKE_node(nod_join_inner, 0);

	join->nod_arg[e_join_rght_rel] = target;
	join->nod_arg[e_join_boolean] = input->nod_arg[e_mrg_condition];

	dsql_nod* query_spec = MAKE_node(nod_query_spec, e_qry_count);
	query_spec->nod_arg[e_qry_from] = MAKE_node(nod_list, 1);
	query_spec->nod_arg[e_qry_from]->nod_arg[0] = join;

	dsql_nod* select_expr = MAKE_node(nod_select_expr, e_sel_count);
	select_expr->nod_arg[e_sel_query_spec] = query_spec;

	dsql_nod* select = MAKE_node(nod_select, e_select_count);
	select->nod_arg[e_select_expr] = select_expr;

	// build a FOR SELECT node
	dsql_nod* for_select = MAKE_node(nod_for_select, e_flp_count);
	for_select->nod_arg[e_flp_select] = select;
	for_select->nod_arg[e_flp_action] = MAKE_node(nod_list, 0);
	for_select = PASS1_statement(statement, for_select);

	// get the already processed relations
	source = for_select->nod_arg[e_flp_select]->nod_arg[e_select_expr]->
		nod_arg[0]->nod_arg[e_join_left_rel];
	target = for_select->nod_arg[e_flp_select]->nod_arg[e_select_expr]->
		nod_arg[0]->nod_arg[e_join_rght_rel];

	dsql_nod* modify = NULL;

	DsqlContextStack usingCtxs;
	get_contexts(usingCtxs, source);

	if (input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_matched])
	{
		// get the assignments of the UPDATE statement
		dsql_nod* list =
			input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_matched]->nod_arg[e_mrg_update_statement];
		fb_assert(list->nod_type == nod_list);

		Firebird::Array<dsql_nod*> org_values, new_values;

		// separate the new and org values to process in correct contexts
		for (int i = 0; i < list->nod_count; ++i)
		{
			const dsql_nod* const assign = list->nod_arg[i];
			fb_assert(assign->nod_type == nod_assign);
			org_values.add(assign->nod_arg[e_asgn_value]);
			new_values.add(assign->nod_arg[e_asgn_field]);
		}

		// build the MODIFY node
		modify = MAKE_node(nod_modify_current, e_mdc_count);
		dsql_ctx* context = get_context(target);
		dsql_nod** ptr;

		modify->nod_arg[e_mdc_context] = (dsql_nod*) context;

		statement->req_scope_level++;	// go to the same level of source and target contexts

		for (DsqlContextStack::iterator itr(usingCtxs); itr.hasData(); ++itr)
			statement->req_context->push(itr.object());	// push the USING contexts

		statement->req_context->push(context);	// process old context values

		for (ptr = org_values.begin(); ptr < org_values.end(); ++ptr)
			*ptr = pass1_node_psql(statement, *ptr, false);

		// and pop the contexts
		statement->req_context->pop();
		statement->req_context->pop();
		statement->req_scope_level--;

		// process relation
		modify->nod_arg[e_mdc_update] = pass1_relation(statement, input->nod_arg[e_mrg_relation]);

		// process new context values
		for (ptr = new_values.begin(); ptr < new_values.end(); ++ptr)
			*ptr = pass1_node_psql(statement, *ptr, false);

		statement->req_context->pop();

		// recreate list of assignments
		modify->nod_arg[e_mdc_statement] = list = MAKE_node(nod_list, list->nod_count);

		for (int i = 0; i < list->nod_count; ++i)
		{
			if (!set_parameter_type(statement, org_values[i], new_values[i], false))
				set_parameter_type(statement, new_values[i], org_values[i], false);

			dsql_nod* assign = MAKE_node(nod_assign, e_asgn_count);
			assign->nod_arg[e_asgn_value] = org_values[i];
			assign->nod_arg[e_asgn_field] = new_values[i];
			list->nod_arg[i] = assign;
		}

		// We do not allow cases like UPDATE SET f1 = v1, f2 = v2, f1 = v3...
		field_appears_once(modify->nod_arg[e_mdc_statement],
			input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_matched]->nod_arg[e_mrg_update_statement],
			false, "MERGE");
	}

	dsql_nod* insert = NULL;

	if (input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_not_matched])
	{
		statement->req_scope_level++;	// go to the same level of the source context

		for (DsqlContextStack::iterator itr(usingCtxs); itr.hasData(); ++itr)
			statement->req_context->push(itr.object());	// push the USING contexts

		// the INSERT relation should be processed in a higher level than the source context
		statement->req_scope_level++;

		// build the INSERT node
		insert = MAKE_node(nod_insert, e_ins_count);
		insert->nod_arg[e_ins_relation] = input->nod_arg[e_mrg_relation];
		insert->nod_arg[e_ins_fields] =
			input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_not_matched]->nod_arg[e_mrg_insert_fields];
		insert->nod_arg[e_ins_values] =
			input->nod_arg[e_mrg_when]->nod_arg[e_mrg_when_not_matched]->nod_arg[e_mrg_insert_values];
		insert = pass1_insert(statement, insert, false);

		// restore the scope level
		statement->req_scope_level--;

		// pop the USING context
		statement->req_scope_level--;
		statement->req_context->pop();
	}

	// build a IF (target.RDB$DB_KEY IS NULL)
	dsql_nod* action = MAKE_node(nod_if, e_if_count);
	action->nod_arg[e_if_condition] = MAKE_node(nod_missing, 1);
	action->nod_arg[e_if_condition]->nod_arg[0] = MAKE_node(nod_dbkey, 1);
	action->nod_arg[e_if_condition]->nod_arg[0]->nod_arg[0] = target;

	if (insert)
	{
		action->nod_arg[e_if_true] = insert;	// then INSERT
		action->nod_arg[e_if_false] = modify;	// else UPDATE
	}
	else
	{
		// negate the condition -> IF (target.RDB$DB_KEY IS NOT NULL)
		dsql_nod* not_node = MAKE_node(nod_not, 1);
		not_node->nod_arg[0] = action->nod_arg[e_if_condition];
		action->nod_arg[e_if_condition] = not_node;

		action->nod_arg[e_if_true] = modify;	// then UPDATE
	}

	// insert the IF inside the FOR SELECT
	for_select->nod_arg[e_flp_action] = action;

	// describe it as an INSERT
	statement->req_type = REQ_INSERT;

	// and return the whole FOR SELECT
	return for_select;
}


// Changes statement->isPsql() value, calls PASS1_node and restore statement->isPsql().
static dsql_nod* pass1_node_psql(CompiledStatement* statement, dsql_nod* input, bool psql)
{
	PsqlChanger changer(statement, psql);
	return PASS1_node(statement, input);
}


/**

 	pass1_not

    @brief	Replace NOT with an appropriately inverted condition, if
			possible. Get rid of redundant nested NOT predicates.


    @param statement
    @param input
	@param invert

 **/
static dsql_nod* pass1_not(CompiledStatement* statement, const dsql_nod* input, bool invert)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	fb_assert(input->nod_type == nod_not);
	dsql_nod* sub = input->nod_arg[0];

	if (sub->nod_type == nod_not) {
		// recurse until different node is found
		// (every even call means no inversion required)
		return pass1_not(statement, sub, !invert);
	}

	dsql_nod* node;
	nod_t node_type = input->nod_type;
	bool is_between = false, invert_args = false, no_op = false;

	if (invert)
	{
		// invert the given boolean
		switch (sub->nod_type)
		{
		case nod_eql:
			node_type = nod_neq;
			break;
		case nod_neq:
			node_type = nod_eql;
			break;
		case nod_lss:
			node_type = nod_geq;
			break;
		case nod_gtr:
			node_type = nod_leq;
			break;
		case nod_leq:
			node_type = nod_gtr;
			break;
		case nod_geq:
			node_type = nod_lss;
			break;
		case nod_eql_all:
			node_type = nod_neq_any;
			break;
		case nod_neq_all:
			node_type = nod_eql_any;
			break;
		case nod_lss_all:
			node_type = nod_geq_any;
			break;
		case nod_gtr_all:
			node_type = nod_leq_any;
			break;
		case nod_leq_all:
			node_type = nod_gtr_any;
			break;
		case nod_geq_all:
			node_type = nod_lss_any;
			break;
		case nod_eql_any:
			if (sub->nod_arg[1]->nod_type == nod_list) {
				// this is NOT IN (<list>), don't change it
				no_op = true;
			}
			else {
				node_type = nod_neq_all;
			}
			break;
		case nod_neq_any:
			node_type = nod_eql_all;
			break;
		case nod_lss_any:
			node_type = nod_geq_all;
			break;
		case nod_gtr_any:
			node_type = nod_leq_all;
			break;
		case nod_leq_any:
			node_type = nod_gtr_all;
			break;
		case nod_geq_any:
			node_type = nod_lss_all;
			break;
		case nod_between:
			node_type = nod_or;
			is_between = true;
			break;
		case nod_and:
			node_type = nod_or;
			invert_args = true;
			break;
		case nod_or:
			node_type = nod_and;
			invert_args = true;
			break;
		case nod_not:
			// this case is handled in the beginning
			fb_assert(false);
		default:
			no_op = true;
			break;
		}
	}
	else {
		// subnode type hasn't been changed
		node_type = sub->nod_type;
	}

	if (no_op)
	{
		// no inversion is possible, so just recreate the input node
		// and return immediately to avoid infinite recursion later
		fb_assert(node_type == nod_not);
		node = MAKE_node(input->nod_type, 1);
		node->nod_arg[0] = PASS1_node(statement, sub);
		return node;
	}

	if (is_between)
	{
		// handle the special BETWEEN case
		fb_assert(node_type == nod_or);
		node = MAKE_node(node_type, 2);
		node->nod_arg[0] = MAKE_node(nod_lss, 2);
		node->nod_arg[0]->nod_arg[0] = sub->nod_arg[0];
		node->nod_arg[0]->nod_arg[1] = sub->nod_arg[1];
		node->nod_arg[1] = MAKE_node(nod_gtr, 2);
		node->nod_arg[1]->nod_arg[0] = sub->nod_arg[0];
		node->nod_arg[1]->nod_arg[1] = sub->nod_arg[2];
	}
	else
	{
		// create new (possibly inverted) node
		node = MAKE_node(node_type, sub->nod_count);
		dsql_nod* const* src = sub->nod_arg;
		dsql_nod** dst = node->nod_arg;
		for (const dsql_nod* const* end = src + sub->nod_count; src < end; src++)
		{
			if (invert_args) {
				dsql_nod* temp = MAKE_node(nod_not, 1);
				temp->nod_arg[0] = *src;
				*dst++ = temp;
			}
			else {
				*dst++ = *src;
			}
		}
	}

	return PASS1_node(statement, node);
}


/**

 	pass1_put_args_on_stack

    @brief	Put recursive non list nodes on the stack


    @param statement
    @param input
    @param stack

 **/
static void pass1_put_args_on_stack( CompiledStatement* statement, dsql_nod* input,
	DsqlNodStack& stack)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	if (input->nod_type != nod_list) {
		stack.push(PASS1_node(statement, input));
		return;
	}

	dsql_nod** ptr = input->nod_arg;
	for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		pass1_put_args_on_stack(statement, *ptr, stack);
	}
}


/**

 	pass1_relation

    @brief	Prepare a relation name for processing.
 	Allocate a new relation node.


    @param statement
    @param input

 **/
static dsql_nod* pass1_relation( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* node = MAKE_node(nod_relation, e_rel_count);

	node->nod_arg[e_rel_context] = (dsql_nod*) PASS1_make_context(statement, input);

	return node;
}


/**

 	pass1_alias_list

    @brief	The passed alias list fully specifies a relation.
 	The first alias represents a relation specified in
 	the from list at this scope levels.  Subsequent
 	contexts, if there are any, represent base relations
 	in a view stack.  They are used to fully specify a
 	base relation of a view.  The aliases used in the
 	view stack are those used in the view definition.


    @param statement
    @param alias_list

 **/
static dsql_nod* pass1_alias_list(CompiledStatement* statement, dsql_nod* alias_list)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(alias_list, dsql_type_nod);

	dsql_nod** arg = alias_list->nod_arg;
	const dsql_nod* const* const end = arg + alias_list->nod_count;

	// Loop through every alias and find the context for that alias.
	// All aliases should have a corresponding context.
	int aliasCount = alias_list->nod_count;
	USHORT savedScopeLevel = statement->req_scope_level;
	dsql_ctx* context = NULL;
	while (aliasCount > 0)
	{
		if (context)
		{
			if (context->ctx_rse && !context->ctx_relation && !context->ctx_procedure)
			{
				// Derived table
				statement->req_scope_level++;
				context = pass1_alias(statement, context->ctx_childs_derived_table, (dsql_str*) *arg);
			}
			else if (context->ctx_relation)
			{
				// This must be a VIEW
				dsql_nod** startArg = arg;
				dsql_rel* relation = context->ctx_relation;
				// find the base table using the specified alias list, skipping the first one
				// since we already matched it to the context.
				for (; arg < end; arg++, aliasCount--)
				{
					if (!(relation = pass1_base_table(statement, relation, (dsql_str*) *arg)))
						break;
				}

				// Found base relation
				if ((aliasCount == 0) && relation)
				{
					// AB: Pretty ugly huh?
					// make up a dummy context to hold the resultant relation.
					thread_db* tdbb = JRD_get_thread_data();
					dsql_ctx* new_context =
						FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
					new_context->ctx_context = context->ctx_context;
					new_context->ctx_relation = relation;

					// concatenate all the contexts to form the alias name;
					// calculate the length leaving room for spaces and a null
					USHORT alias_length = alias_list->nod_count;
					dsql_nod** aliasArg = startArg;
					for (; aliasArg < end; aliasArg++)
					{
						DEV_BLKCHK(*aliasArg, dsql_type_str);
						alias_length += static_cast<USHORT>(((dsql_str*) *aliasArg)->str_length);
					}

					dsql_str* alias = FB_NEW_RPT(*tdbb->getDefaultPool(), alias_length) dsql_str;
					alias->str_length = alias_length;
					new_context->ctx_alias = alias->str_data;

					TEXT* p = alias->str_data;
					for (aliasArg = startArg; aliasArg < end; aliasArg++)
					{
						for (const TEXT* q = ((dsql_str*) *aliasArg)->str_data; *q;)
							*p++ = *q++;
						*p++ = ' ';
					}
					p[-1] = 0;

					context = new_context;
				}
				else
				{
					context = NULL;
				}
			}
			else
			{
				context = NULL;
			}
		}
		else
		{
			context = pass1_alias(statement, *statement->req_context, (dsql_str*) *arg);
		}

		if (!context) {
			break;
		}

		arg++;
		aliasCount--;
	}
	statement->req_scope_level = savedScopeLevel;

	if (!context) {
		// there is no alias or table named %s at this scope level.
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_no_relation_alias) << Arg::Str(((dsql_str*) *arg)->str_data));
	}

	return (dsql_nod*) context;
}


/**

 	pass1_alias

    @brief	The passed relation or alias represents
 	a context which was previously specified
 	in the from list.  Find and return the
 	proper context.


    @param statement
    @param alias

 **/
static dsql_ctx* pass1_alias(CompiledStatement* statement, DsqlContextStack& stack, dsql_str* alias)
{
	dsql_ctx* relation_context = NULL;

	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(alias, dsql_type_str);

	// CVC: Getting rid of trailing spaces.
	if (alias && alias->str_data) {
		fb_utils::exact_name(alias->str_data);
	}

	// look through all contexts at this scope level
	// to find one that has a relation name or alias
	// name which matches the identifier passed.
	for (DsqlContextStack::iterator itr(stack); itr.hasData(); ++itr)
	{
		dsql_ctx* context = itr.object();
		if (context->ctx_scope_level != statement->req_scope_level)
		{
			continue;
		}

		// check for matching alias.
		if (context->ctx_internal_alias)
		{
			if (!strcmp(context->ctx_internal_alias, alias->str_data))
			{
				return context;
			}
			continue;
		}

		// If an unnamed derived table and empty alias
		if (context->ctx_rse && !context->ctx_relation && !context->ctx_procedure &&
			(alias->str_length == 0))
		{
			relation_context = context;
		}

		// check for matching relation name; aliases take priority so
		// save the context in case there is an alias of the same name;
		// also to check that there is no self-join in the query.
		if (context->ctx_relation && context->ctx_relation->rel_name == alias->str_data)
		{
			if (relation_context) {
				// the table %s is referenced twice; use aliases to differentiate
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_self_join) << Arg::Str(alias->str_data));
			}
			relation_context = context;
		}
	}

	return relation_context;
}


/**

 	pass1_alias_concat

    @brief	Concatenate 2 input strings together for
	a new alias string.
	Note: Both input params can be empty.


    @param input1
    @param input2

 **/
static dsql_str* pass1_alias_concat(const dsql_str* input1, const dsql_str* input2)
{
	thread_db* tdbb = JRD_get_thread_data();

	DEV_BLKCHK(input1, dsql_type_str);
	DEV_BLKCHK(input2, dsql_type_str);

	int length = 0;
	if (input1) {
		length += input1->str_length;
	}
	if (input1 && input1->str_length && input2) {
		length++; // Room for space character.
	}
	if (input2) {
		length += input2->str_length;
	}
	dsql_str* output = FB_NEW_RPT(*tdbb->getDefaultPool(), length) dsql_str;
	output->str_length = length;
	TEXT* ptr = output->str_data;
	if (input1) {
		strcat(ptr, input1->str_data);
	}
	if (input1 && input1->str_length && input2) {
		strcat(ptr, " ");
	}
	if (input2) {
		strcat(ptr, input2->str_data);
	}
	return output;
}


/**

 	pass1_base_table

    @brief	Check if the relation in the passed context
 	has a base table which matches the passed alias.


    @param statement
    @param relation
    @param alias

 **/
static dsql_rel* pass1_base_table( CompiledStatement* statement, const dsql_rel* relation,
	const dsql_str* alias)
{

	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(relation, dsql_type_dsql_rel);
	DEV_BLKCHK(alias, dsql_type_str);

	return METD_get_view_relation(statement, relation->rel_name.c_str(), alias->str_data);
}


/**

 	pass1_returning

    @brief	Compile a RETURNING clause.


    @param statement
    @param input

 **/
static dsql_nod* pass1_returning(CompiledStatement* statement, const dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* const source = pass1_node_psql(statement, input->nod_arg[e_ret_source], false);

	statement->req_flags |= REQ_returning_into;
	dsql_nod* const target = PASS1_node(statement, input->nod_arg[e_ret_target]);
	statement->req_flags &= ~REQ_returning_into;

	if (!statement->isPsql() && target)
	{
		// RETURNING INTO is not allowed syntax for DSQL
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Token unknown
				  Arg::Gds(isc_token_err) <<
				  Arg::Gds(isc_random) << Arg::Str("INTO"));
	}
	else if (statement->isPsql() && !target)
	{
		// This trick because we don't copy lexer positions when copying lists.
		const dsql_nod* errSrc = input->nod_arg[e_ret_source];
		fb_assert(errSrc->nod_type == nod_list);
		// RETURNING without INTO is not allowed for PSQL
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Unexpected end of command
				  Arg::Gds(isc_command_end_err2) << Arg::Num(errSrc->nod_line) <<
													Arg::Num(errSrc->nod_column));
	}

	const int count = source->nod_count;
	fb_assert(count);
	dsql_nod* node = MAKE_node(nod_list, count);

	if (target)
	{
		// PSQL case
		fb_assert(statement->isPsql());
		if (count != target->nod_count) {
			// count of column list and value list don't match
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					  Arg::Gds(isc_dsql_var_count_err));
		}

		dsql_nod** src = source->nod_arg;
		dsql_nod** dst = target->nod_arg;
		dsql_nod** ptr = node->nod_arg;
		for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; src++, dst++, ptr++)
		{
			dsql_nod* temp = MAKE_node(nod_assign, e_asgn_count);
			temp->nod_arg[e_asgn_value] = *src;
			temp->nod_arg[e_asgn_field] = *dst;
			*ptr = temp;
		}
	}
	else
	{
		// DSQL case
		fb_assert(!statement->isPsql());

		dsql_nod** src = source->nod_arg;
		dsql_nod** ptr = node->nod_arg;
		for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; src++, ptr++)
		{
			dsql_par* parameter = MAKE_parameter(statement->req_receive, true, true, 0, *src);
			parameter->par_node = *src;
			MAKE_desc(statement, &parameter->par_desc, *src, NULL);
			parameter->par_desc.dsc_flags |= DSC_nullable;

			dsql_nod* p_node = MAKE_node(nod_parameter, e_par_count);
			p_node->nod_count = 0;
			p_node->nod_arg[e_par_index] = (dsql_nod*) (IPTR) parameter->par_index;
			p_node->nod_arg[e_par_parameter] = (dsql_nod*) parameter;

			dsql_nod* temp = MAKE_node(nod_assign, e_asgn_count);
			temp->nod_arg[e_asgn_value] = *src;
			temp->nod_arg[e_asgn_field] = p_node;
			*ptr = temp;
		}
	}

	if (!statement->isPsql())
	{
		statement->req_type = REQ_EXEC_PROCEDURE;
	}

	return node;
}


/**

 	pass1_rse

    @brief	wrapper for pass1_rse_impl
		substitute recursive CTE alias (if needed)
		and call pass1_rse_impl

    @param statement
    @param input
    @param order
    @param rows
    @param update_lock
	@param flags

 **/
static dsql_nod* pass1_rse( CompiledStatement* statement, dsql_nod* input, dsql_nod* order,
	dsql_nod* rows, dsql_nod* update_lock, USHORT flags)
{
	const TEXT* save_alias = NULL;
	const bool isRecursive = (input->nod_flags & NOD_SELECT_EXPR_RECURSIVE);

	if (isRecursive)
	{
		fb_assert(statement->req_recursive_ctx);
		save_alias = statement->req_recursive_ctx->ctx_alias;

		statement->req_recursive_ctx->ctx_alias = statement->getNextCTEAlias()->str_data;
	}

	dsql_nod* ret = pass1_rse_impl(statement, input, order, rows, update_lock, flags);

	if (isRecursive) {
		statement->req_recursive_ctx->ctx_alias = save_alias;
	}

	return ret;
}


/**

 	pass1_rse_impl

    @brief	Compile a record selection expression.
 	The input node may either be a "select_expression"
 	or a "list" (an implicit union) or a "query specification".


    @param statement
    @param input
    @param order
    @param rows
    @param update_lock
	@param flags

 **/
static dsql_nod* pass1_rse_impl( CompiledStatement* statement, dsql_nod* input, dsql_nod* order,
	dsql_nod* rows, dsql_nod* update_lock, USHORT flags)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(order, dsql_type_nod);

	thread_db* tdbb = JRD_get_thread_data();

	// Verify if we're processing view fields and reset flag to not pass to
	// more than required inner nodes.
	const USHORT viewFlags = input->nod_flags | (flags & NOD_SELECT_VIEW_FIELDS);
	flags &= ~NOD_SELECT_VIEW_FIELDS;

	if (input->nod_type == nod_select_expr)
	{
		dsql_nod* node_with = input->nod_arg[e_sel_with_list];
		try
		{
			if (node_with)
				statement->addCTEs(node_with);

			dsql_nod* ret =
				pass1_rse(statement, input->nod_arg[e_sel_query_spec],
						input->nod_arg[e_sel_order], input->nod_arg[e_sel_rows],
						update_lock, viewFlags);

			if (node_with)
			{
				statement->checkUnusedCTEs();
				statement->clearCTEs();
			}

			return ret;
		}
		catch (const Firebird::Exception&) {
			if (node_with)
				statement->clearCTEs();
			throw;
		}
	}
	else if (input->nod_type == nod_list)
	{
		fb_assert(input->nod_count > 1);

		if (update_lock)
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("WITH LOCK"));

		return pass1_union(statement, input, order, rows, flags);
	}

	fb_assert(input->nod_type == nod_query_spec);

	// Save the original base of the context stack and process relations

	dsql_nod* target_rse = MAKE_node(nod_rse, e_rse_count);
	dsql_nod* rse = target_rse;
	rse->nod_arg[e_rse_lock] = update_lock;

	dsql_nod* list = rse->nod_arg[e_rse_streams] =
		pass1_node_psql(statement, input->nod_arg[e_qry_from], false);

	{ // scope block
		const dsql_rel* relation;
		if (update_lock &&
			(list->nod_count != 1 || list->nod_arg[0]->nod_type != nod_relation ||
				!(relation = ((dsql_ctx*)list->nod_arg[0]->nod_arg[e_rel_context])->ctx_relation) ||
				(relation->rel_flags & REL_view) || (relation->rel_flags & REL_external)))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("WITH LOCK"));
		}
	} // end scope block

	// Process LIMIT and/or ROWS, if any

	dsql_nod* node = input->nod_arg[e_qry_limit];
	if (node && rows)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // Token unknown
				  Arg::Gds(isc_token_err) <<
				  Arg::Gds(isc_random) << Arg::Str("ROWS"));
	}
	else if (node || (node = rows) )
	{
		pass1_limit(statement, node, rse);
	}

	// Process boolean, if any

	if ( (node = input->nod_arg[e_qry_where]) )
	{
		++statement->req_in_where_clause;
		rse->nod_arg[e_rse_boolean] = pass1_node_psql(statement, node, false);
		--statement->req_in_where_clause;

		// AB: An aggregate pointing to it's own parent_context isn't
		// allowed, HAVING should be used instead
		if (pass1_found_aggregate(rse->nod_arg[e_rse_boolean],
				statement->req_scope_level, FIELD_MATCH_TYPE_EQUAL, true))
		{
			// Cannot use an aggregate in a WHERE clause, use HAVING instead
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_agg_where_err));
		}
	}

#ifdef DSQL_DEBUG
	if (DSQL_debug & 16) {
		dsql_trace("PASS1_rse input tree:");
		DSQL_pretty(input, 0);
	}
#endif

	// Process select list, if any. If not, generate one
	dsql_nod* selectList = input->nod_arg[e_qry_list];
	// First expand select list, this will expand nodes with asterisk.
	++statement->req_in_select_list;
	selectList = pass1_expand_select_list(statement, selectList, rse->nod_arg[e_rse_streams]);

	if ((flags & NOD_SELECT_EXPR_VALUE) && (!selectList || selectList->nod_count > 1))
	{
		// More than one column (or asterisk) is specified in column_singleton
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_count_mismatch));
	}

	// Pass select list
	rse->nod_arg[e_rse_items] =
		pass1_sel_list(statement, selectList, (viewFlags & NOD_SELECT_VIEW_FIELDS));
	--statement->req_in_select_list;

	// Process ORDER clause, if any
	if (order) {
		++statement->req_in_order_by_clause;
		rse->nod_arg[e_rse_sort] = pass1_sort(statement, order, selectList);
		--statement->req_in_order_by_clause;
	}

	// A GROUP BY, HAVING, or any aggregate function in the select list
	// will force an aggregate
	dsql_ctx* parent_context = NULL;
	dsql_nod* parent_rse = NULL;
	dsql_nod* aggregate = NULL;

	if (input->nod_arg[e_qry_group] ||
		input->nod_arg[e_qry_having] ||
		(rse->nod_arg[e_rse_items] && aggregate_found(statement, rse->nod_arg[e_rse_items])) ||
		(rse->nod_arg[e_rse_sort] && aggregate_found(statement, rse->nod_arg[e_rse_sort])))
	{

		// dimitr: don't allow WITH LOCK for aggregates

		if (update_lock) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("WITH LOCK"));
		}

		parent_context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
		parent_context->ctx_context = statement->req_context_number++;
		parent_context->ctx_scope_level = statement->req_scope_level;

		// When we're in a outer-join part mark context for it.
		if (statement->req_in_outer_join)
			parent_context->ctx_flags |= CTX_outer_join;
		parent_context->ctx_in_outer_join = statement->req_in_outer_join;

		aggregate = MAKE_node(nod_aggregate, e_agg_count);
		aggregate->nod_arg[e_agg_context] = (dsql_nod*) parent_context;
		aggregate->nod_arg[e_agg_rse] = rse;
		parent_rse = target_rse = MAKE_node(nod_rse, e_rse_count);
		parent_rse->nod_arg[e_rse_streams] = list = MAKE_node(nod_list, 1);
		list->nod_arg[0] = aggregate;

		if (rse->nod_arg[e_rse_first]) {
			parent_rse->nod_arg[e_rse_first] = rse->nod_arg[e_rse_first];
			rse->nod_arg[e_rse_first] = NULL;
		}
		if (rse->nod_arg[e_rse_skip]) {
			parent_rse->nod_arg[e_rse_skip] = rse->nod_arg[e_rse_skip];
			rse->nod_arg[e_rse_skip] = NULL;
		}

		statement->req_context->push(parent_context);
		// replace original contexts with parent context
		remap_streams_to_parent_context(rse->nod_arg[e_rse_streams], parent_context);
	}

	// Process GROUP BY clause, if any
	if (node = input->nod_arg[e_qry_group])
	{
		// if there are positions in the group by clause then replace them
		// by the (newly pass) items from the select_list
		++statement->req_in_group_by_clause;
		aggregate->nod_arg[e_agg_group] =
			pass1_group_by_list(statement, input->nod_arg[e_qry_group], selectList);
		--statement->req_in_group_by_clause;

		// AB: An field pointing to another parent_context isn't
		// allowed and GROUP BY items can't contain aggregates
		bool field;
		if (pass1_found_field(aggregate->nod_arg[e_agg_group],
				statement->req_scope_level, FIELD_MATCH_TYPE_LOWER, &field) ||
		    pass1_found_aggregate(aggregate->nod_arg[e_agg_group],
				statement->req_scope_level, FIELD_MATCH_TYPE_LOWER_EQUAL, true))
		{
			// Cannot use an aggregate in a GROUP BY clause
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_agg_group_err));
		}
	}

	// Parse a user-specified access PLAN
	rse->nod_arg[e_rse_plan] = pass1_node_psql(statement, input->nod_arg[e_qry_plan], false);

	// AB: Pass select-items for distinct operation again, because for
	// sub-selects a new context number should be generated
	if (input->nod_arg[e_qry_distinct])
	{
		if (update_lock) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  // Token unknown
					  Arg::Gds(isc_token_err) <<
					  Arg::Gds(isc_random) << Arg::Str("WITH LOCK"));
		}

		++statement->req_in_select_list;
		// ASF: We pass false to viewFields parameter here because these expressions are
		// generated inside the view body, and not in view fields.
		target_rse->nod_arg[e_rse_reduced] = pass1_sel_list(statement, selectList, false);
		--statement->req_in_select_list;

		// sort, group and distinct have the same limit for now
		if (selectList->nod_count > MAX_SORT_ITEMS)
		{
			// Cannot have more than 255 items in DISTINCT list.
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(isc_dsql_max_distinct_items));
		}
	}

	// Unless there was a parent, we're done
	if (!parent_context)
	{
		rse->nod_flags = flags;
		return rse;
	}

	// Reset context of select items to point to the parent stream

	parent_rse->nod_arg[e_rse_items] =
		remap_fields(statement, rse->nod_arg[e_rse_items], parent_context);
	rse->nod_arg[e_rse_items] = NULL;

	// AB: Check for invalid contructions inside selected-items list
	list = parent_rse->nod_arg[e_rse_items];
	{ // scope block
		const dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			if (invalid_reference(parent_context, *ptr, aggregate->nod_arg[e_agg_group], false, false))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Invalid expression in the select list
						  // (not contained in either an aggregate or the GROUP BY clause)
						  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("select list"));
			}
		}
	} // end scope block

	// Reset context of order items to point to the parent stream

	if (order)
	{
		parent_rse->nod_arg[e_rse_sort] =
			remap_fields(statement, rse->nod_arg[e_rse_sort], parent_context);
		rse->nod_arg[e_rse_sort] = NULL;

		// AB: Check for invalid contructions inside the ORDER BY clause
		list = target_rse->nod_arg[e_rse_sort];
		const dsql_nod* const* ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			if (invalid_reference(parent_context, *ptr, aggregate->nod_arg[e_agg_group], false, false))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Invalid expression in the ORDER BY clause
						  // (not contained in either an aggregate or the GROUP BY clause)
						  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("ORDER BY clause"));
			}
		}
	}

	// And, of course, reduction clauses must also apply to the parent

	if (input->nod_arg[e_qry_distinct])
	{
		parent_rse->nod_arg[e_rse_reduced] =
			remap_fields(statement, parent_rse->nod_arg[e_rse_reduced], parent_context);
	}

	// Process HAVING clause, if any

	if ( (node = input->nod_arg[e_qry_having]) )
	{
		++statement->req_in_having_clause;
		parent_rse->nod_arg[e_rse_boolean] = pass1_node_psql(statement, node, false);
		--statement->req_in_having_clause;

		parent_rse->nod_arg[e_rse_boolean] =
			remap_fields(statement, parent_rse->nod_arg[e_rse_boolean], parent_context);

		// AB: Check for invalid contructions inside the HAVING clause
		list = parent_rse->nod_arg[e_rse_boolean];
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			if (invalid_reference(parent_context, *ptr, aggregate->nod_arg[e_agg_group], false, false))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Invalid expression in the HAVING clause
						  // (neither an aggregate nor contained in the GROUP BY clause)
						  Arg::Gds(isc_dsql_agg_having_err) << Arg::Str("HAVING clause"));
			}
		}

#ifdef CHECK_HAVING
		if (aggregate)
		{
			if (invalid_reference(parent_rse->nod_arg[e_rse_boolean], aggregate->nod_arg[e_agg_group]))
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // invalid field reference
						  Arg::Gds(isc_field_ref_err));
			}
		}
#endif
	}

	parent_rse->nod_flags = flags;
	return parent_rse;
}


/**

 	pass1_searched_case

    @brief	Handle a reference to a searched case expression.


    @param statement
    @param input

 **/
static dsql_nod* pass1_searched_case( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(input->nod_arg[0], dsql_type_nod);

	dsql_nod* node = MAKE_node(nod_searched_case, 2);

	dsql_nod* list = input->nod_arg[0];

	// build boolean-expression list
	{ // scope block
		DsqlNodStack stack;
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr += 2)
		{
			pass1_put_args_on_stack(statement, *ptr, stack);
		}
		node->nod_arg[e_searched_case_search_conditions] = MAKE_list(stack);
	} // end scope block

	// build when_result list including else_result at the end
	// else_result is included for easy handling in MAKE_desc()
	{ // scope block
		DsqlNodStack stack;
		dsql_nod** ptr = list->nod_arg;
		const dsql_nod* const* const end = ptr + list->nod_count;
		for (++ptr; ptr < end; ptr += 2) {
			pass1_put_args_on_stack(statement, *ptr, stack);
		}
		pass1_put_args_on_stack(statement, input->nod_arg[1], stack);
		node->nod_arg[e_searched_case_results] = MAKE_list(stack);
	} // end scope block

	// Set describer for output node
	MAKE_desc(statement, &node->nod_desc, node, NULL);

	// Set parameter-types if parameters are there in the result nodes
	dsql_nod* case_results = node->nod_arg[e_searched_case_results];
	dsql_nod** ptr = case_results->nod_arg;
	for (const dsql_nod* const* const end = ptr + case_results->nod_count; ptr < end; ptr++)
	{
		set_parameter_type(statement, *ptr, node, false);
	}

	return node;
}


/**

 	pass1_sel_list

    @brief	Compile a select list.

    @param statement
    @param input

 **/
static dsql_nod* pass1_sel_list(CompiledStatement* statement, dsql_nod* input, bool viewFields)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	DsqlNodStack stack;
	dsql_nod** ptr = input->nod_arg;
	for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		if (viewFields)
			statement->req_hidden_vars_number = 0;

		DEV_BLKCHK(*ptr, dsql_type_nod);
		stack.push(pass1_node_psql(statement, *ptr, false));

		if (viewFields)
			statement->req_hidden_vars_number = 0;
	}
	dsql_nod* node = MAKE_list(stack);

	return node;
}


/**

 	pass1_simple_case

    @brief	Handle a reference to a simple case expression.


    @param statement
    @param input

 **/
static dsql_nod* pass1_simple_case( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(input->nod_arg[0], dsql_type_nod);

	dsql_nod* node = MAKE_node(nod_simple_case, 4);
	node->nod_count = 3;	// we do not want to reprocess e_simple_case_case_operand2 later

	// build case_operand node
	node->nod_arg[e_simple_case_case_operand] = PASS1_node(statement, input->nod_arg[0]);
	node->nod_arg[e_simple_case_case_operand2] = pass1_hidden_variable(
		statement, node->nod_arg[e_simple_case_case_operand]);

	// If a hidden variable was generated, it will be stored in e_simple_case_case_operand2 and
	// we'll use it in GEN. Otherwise, e_simple_case_case_operand2 is NULL and we should use the
	// original expression (e_simple_case_case_operand) in GEN.

	dsql_nod* list = input->nod_arg[1];

	// build when_operand list
	{ // scope block
		DsqlNodStack stack;
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ptr += 2)
		{
			pass1_put_args_on_stack(statement, *ptr, stack);
		}
		node->nod_arg[e_simple_case_when_operands] = MAKE_list(stack);
	} // end scope block

	// build when_result list including else_result at the end
	// else_result is included for easy handling in MAKE_desc()
	{ // scope block
		DsqlNodStack stack;
		dsql_nod** ptr = list->nod_arg;
		const dsql_nod* const* const end = ptr + list->nod_count;
		for (++ptr; ptr < end; ptr += 2) {
			pass1_put_args_on_stack(statement, *ptr, stack);
		}
		pass1_put_args_on_stack(statement, input->nod_arg[2], stack);
		node->nod_arg[e_simple_case_results] = MAKE_list(stack);
	} // end scope block

	// Check if there is a parameter in the case/when operand list
	bool setParameters = (node->nod_arg[e_simple_case_case_operand]->nod_type == nod_parameter);
	if (!setParameters)
	{
		list = node->nod_arg[e_simple_case_when_operands];
		dsql_nod** ptr = list->nod_arg;
		for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ++ptr)
		{
			if ((*ptr)->nod_type == nod_parameter)
			{
				setParameters = true;
				break;
			}
		}
	}
	// build list for making describe information from
	// case_operand and when_operands this is used for
	// setting parameter describers if used in this case.
	if (setParameters)
	{
		list = node->nod_arg[e_simple_case_when_operands];
		dsql_nod* node1 = MAKE_node(nod_list, list->nod_count + 1);

		{ // scope block
			int i = 0;
			node1->nod_arg[i++] = node->nod_arg[e_simple_case_case_operand];
			dsql_nod** ptr = list->nod_arg;
			for (const dsql_nod* const* const end = ptr + list->nod_count; ptr < end; ++ptr, ++i)
			{
				node1->nod_arg[i] = *ptr;
			}
			MAKE_desc_from_list(statement, &node1->nod_desc, node1, NULL, "CASE");
			// Set parameter describe information
			set_parameter_type(statement, node->nod_arg[e_simple_case_case_operand], node1, false);
		} // end scope block

		{ // scope block
			dsql_nod* simple_when = node->nod_arg[e_simple_case_when_operands];
			dsql_nod** ptr = simple_when->nod_arg;
			for (const dsql_nod* const* const end = ptr + simple_when->nod_count; ptr < end; ptr++)
			{
				set_parameter_type(statement, *ptr, node1, false);
			}
		} // end scope block

		// Clean up temporary used node
		delete node1;
	}

	// Set describer for output node
	MAKE_desc(statement, &node->nod_desc, node, NULL);
	// Set parameter describe information for evt. results parameters
	dsql_nod* simple_res = node->nod_arg[e_simple_case_results];
	dsql_nod** ptr = simple_res->nod_arg;
	for (const dsql_nod* const* const end = ptr + simple_res->nod_count; ptr < end; ptr++)
	{
		set_parameter_type(statement, *ptr, node, false);
	}

	return node;
}


/**

 	pass1_sort

    @brief	Process ORDER BY list, which may contain
			an ordinal or alias which references the
			select list.

    @param statement
    @param input
    @param selectList

 **/
static dsql_nod* pass1_sort( CompiledStatement* statement, dsql_nod* input, dsql_nod* selectList)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(selectList, dsql_type_nod);

	if (input->nod_type != nod_list) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  // invalid ORDER BY clause
				  Arg::Gds(isc_order_by_err));
	}

	if (input->nod_count > MAX_SORT_ITEMS)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_order_by_err) <<
				  // invalid ORDER BY clause, cannot sort on more than 255 items
				  Arg::Gds(isc_dsql_max_sort_items));
	}

	// Node is simply to be rebuilt -- just recurse merrily

	dsql_nod* node = MAKE_node(input->nod_type, input->nod_count);
	dsql_nod** ptr2 = node->nod_arg;

	for (int sortloop = 0; sortloop < input->nod_count; sortloop++)
	{
		DEV_BLKCHK(input->nod_arg[sortloop], dsql_type_nod);
		dsql_nod* node1 = input->nod_arg[sortloop];
		if (node1->nod_type != nod_order) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  // invalid ORDER BY clause
					  Arg::Gds(isc_order_by_err));
		}
		dsql_nod* node2 = MAKE_node(nod_order, e_order_count);
		node2->nod_arg[e_order_flag] = node1->nod_arg[e_order_flag]; // asc/desc flag
		node2->nod_arg[e_order_nulls] = node1->nod_arg[e_order_nulls]; // nulls first/last flag

		const dsql_str* collate = NULL;

		// get node of value to be ordered by
		node1 = node1->nod_arg[e_order_field];

		if (node1->nod_type == nod_collate) {
			collate = (dsql_str*) node1->nod_arg[e_coll_target];
			// substitute nod_collate with its argument (real value)
			node1 = node1->nod_arg[e_coll_source];
		}

		if (node1->nod_type == nod_field_name) {
			// check for alias or field node
			node1 = pass1_field(statement, node1, false, selectList);
		}
		else if (node1->nod_type == nod_constant && node1->nod_desc.dsc_dtype == dtype_long)
		{
			const ULONG position = node1->getSlong();
			if (position < 1 || !selectList || position > (ULONG) selectList->nod_count)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Invalid column position used in the ORDER BY clause
						  Arg::Gds(isc_dsql_column_pos_err) << Arg::Str("ORDER BY"));
			}
			// substitute ordinal with appropriate field
			node1 = pass1_node_psql(statement, selectList->nod_arg[position - 1], false);
		}
		else {
			node1 = pass1_node_psql(statement, node1, false);
		}

		if (collate) {
			// finally apply collation order, if necessary
			node1 = pass1_collate(statement, node1, collate);
		}

		// store actual value to be ordered by
		node2->nod_arg[e_order_field] = node1;
		*ptr2++ = node2;
	}

	return node;
}


/**

 	pass1_sys_function

    @brief	Handle a reference to a system defined function.


    @param statement
    @param input

 **/
static dsql_nod* pass1_sys_function(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsql_nod* node;

	// Don't look for UDF if the system function has a special syntax
	if (!(input->nod_flags & NOD_SPECIAL_SYNTAX))
	{
		const dsql_str* name = (dsql_str*) input->nod_arg[e_sysfunc_name];
		dsql_udf* userFunc = METD_get_function(statement, name);

		if (userFunc)
		{
			node = MAKE_node(nod_udf, 2);
			node->nod_arg[0] = (dsql_nod*) name;
			node->nod_arg[1] = input->nod_arg[e_sysfunc_args];

			return pass1_udf(statement, node);
		}
	}

	node = MAKE_node(input->nod_type, e_sysfunc_count);
	node->nod_arg[e_sysfunc_name] = input->nod_arg[e_sysfunc_name];
	node->nod_arg[e_sysfunc_args] = PASS1_node(statement, input->nod_arg[e_sysfunc_args]);

	if (node->nod_arg[e_sysfunc_args])
	{
		const SysFunction* const sf =
			SysFunction::lookup(((dsql_str*) node->nod_arg[e_sysfunc_name])->str_data);

		if (sf && sf->setParamsFunc)
		{
			Firebird::Array<dsc*> args;

			dsql_nod* in_args = node->nod_arg[e_sysfunc_args];
			fb_assert(in_args->nod_type == nod_list);

			for (unsigned int i = 0; i < in_args->nod_count; ++i)
			{
				dsql_nod* p = in_args->nod_arg[i];
				MAKE_desc(statement, &p->nod_desc, p, p);
				args.add(&p->nod_desc);
			}

			DSqlDataTypeUtil dataTypeUtil(statement);
			sf->setParamsFunc(&dataTypeUtil, sf, args.getCount(), args.begin());

			for (unsigned int j = 0; j < in_args->nod_count; ++j)
			{
				dsql_nod* p = in_args->nod_arg[j];
				set_parameter_type(statement, p, p, false);
			}
		}
	}

	return node;
}


/**

 	pass1_udf

    @brief	Handle a reference to a user defined function.


    @param statement
    @param input

 **/
static dsql_nod* pass1_udf( CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	const dsql_str* name = (dsql_str*) input->nod_arg[0];
	DEV_BLKCHK(name, dsql_type_str);
	dsql_udf* userFunc = METD_get_function(statement, name);
	if (!userFunc)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				  Arg::Gds(isc_dsql_function_err) <<
				  Arg::Gds(isc_random) << Arg::Str(name->str_data));
	}

	dsql_nod* node = MAKE_node(nod_udf, input->nod_count);
	node->nod_arg[0] = (dsql_nod*) userFunc;
	if (input->nod_count == 2) {
		DsqlNodStack stack;
		USHORT arg_pos = 0;
		pass1_udf_args(statement, input->nod_arg[1], userFunc, arg_pos, stack);
		node->nod_arg[1] = MAKE_list(stack);
	}

	return node;
}


/**

 	pass1_udf_args

    @brief	Handle references to function arguments.


    @param statement
    @param input
    @param udf
    @param arg_pos
    @param stack

 **/
static void pass1_udf_args(CompiledStatement* statement, dsql_nod* input,
	dsql_udf* userFunc, USHORT& arg_pos, DsqlNodStack& stack)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(userFunc, dsql_type_udf);

	if (!input)
		return;

	DEV_BLKCHK(input, dsql_type_nod);

	if (input->nod_type != nod_list)
	{
		dsql_nod* temp = PASS1_node(statement, input);
		if (arg_pos < userFunc->udf_arguments.getCount()) {
			dsql_nod temp2;
			temp2.nod_desc = userFunc->udf_arguments[arg_pos];
			set_parameter_type(statement, temp, &temp2, false);
		}
		else {
			// We should complain here in the future! The parameter is
			// out of bounds or the function doesn't declare input params.
		}
		stack.push(temp);
		arg_pos++;
		return;
	}

	dsql_nod** ptr = input->nod_arg;
	for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
	{
		pass1_udf_args(statement, *ptr, userFunc, arg_pos, stack);
	}
}


/**

 	pass1_union

    @brief	Handle a UNION of substreams, generating
 	a mapping of all the fields and adding an
 	implicit PROJECT clause to ensure that all
 	the records returned are unique.


    @param statement
    @param input
    @param order_list
	@param rows

 **/
static dsql_nod* pass1_union( CompiledStatement* statement, dsql_nod* input,
	dsql_nod* order_list, dsql_nod* rows, USHORT flags)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(order_list, dsql_type_nod);

	thread_db* tdbb = JRD_get_thread_data();

	// set up the rse node for the union.
	dsql_nod* union_rse = MAKE_node(nod_rse, e_rse_count);
	dsql_nod* union_node = union_rse->nod_arg[e_rse_streams] = MAKE_node(nod_union, input->nod_count);
	union_node->nod_flags = input->nod_flags;

	// generate a context for the union itself.
	dsql_ctx* union_context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());

	if (input->nod_flags & NOD_UNION_RECURSIVE) {
		union_context->ctx_context = statement->req_recursive_ctx_id;
	}
	else {
		union_context->ctx_context = statement->req_context_number++;
	}

	union_context->ctx_scope_level = statement->req_scope_level;
	// When we're in a outer-join part mark context for it.
	if (statement->req_in_outer_join) {
		union_context->ctx_flags |= CTX_outer_join;
	}
	union_context->ctx_in_outer_join = statement->req_in_outer_join;

	statement->req_context->push(union_context);

	// process all the sub-rse's.
	{ // scope block
		dsql_nod** uptr = union_node->nod_arg;
		const DsqlContextStack::const_iterator base(*statement->req_context);
		dsql_nod** ptr = input->nod_arg;
		for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ++ptr, ++uptr)
		{
			statement->req_scope_level++;
			*uptr = pass1_rse(statement, *ptr, NULL, NULL, NULL, 0);
			statement->req_scope_level--;
			while (*(statement->req_context) != base)
			{
				statement->req_union_context.push(statement->req_context->pop());
			}

			// Push recursive context after initial select has been processed.
			// Corresponding pop occurs in pass1_derived_table
			if ((input->nod_flags & NOD_UNION_RECURSIVE) && (ptr == input->nod_arg))
				statement->req_context->push(statement->req_recursive_ctx);
		}
	} // end scope block

	// generate the list of fields to select.
	dsql_nod* items = union_node->nod_arg[0]->nod_arg[e_rse_items];

	// loop through the list nodes, checking to be sure that they have the
	// same number of items

	for (int i = 1; i < union_node->nod_count; i++)
	{
		const dsql_nod* nod1 = union_node->nod_arg[i]->nod_arg[e_rse_items];
		if (items->nod_count != nod1->nod_count) {
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  // overload of msg
					  Arg::Gds(isc_dsql_count_mismatch));
		}
	}

	// Comment below belongs to the old code (way a union was handled).

	// SQL II, section 9.3, pg 195 governs which data types
	// are considered equivalent for a UNION
	// The following restriction is in some ways more restrictive
	// (cannot UNION CHAR with VARCHAR for instance)
	// (or cannot union CHAR of different lengths)
	// and in someways less restrictive
	// (SCALE is not looked at)
	// Workaround: use a direct CAST() statement in the SQL
	// statement to force desired datatype.

	// loop through the list nodes and cast whenever possible.
	dsql_nod* tmp_list = MAKE_node(nod_list, union_node->nod_count);
	for (int j = 0; j < items->nod_count; j++)
	{
		for (int i = 0; i < union_node->nod_count; i++) {
			dsql_nod* nod1 = union_node->nod_arg[i]->nod_arg[e_rse_items];
			MAKE_desc(statement, &nod1->nod_arg[j]->nod_desc, nod1->nod_arg[j], NULL);
			tmp_list->nod_arg[i] = nod1->nod_arg[j];

			// We look only at the items->nod_arg[] when creating the
			// output descriptors. Make sure that the sub-rses
			// descriptor flags are copied onto items->nod_arg[]->nod_desc.
			// Bug 65584.
			// -Sudesh 07/28/1999.
			if (i > 0) {
				if (nod1->nod_arg[j]->nod_desc.dsc_flags & DSC_nullable) {
					items->nod_arg[j]->nod_desc.dsc_flags |= DSC_nullable;
				}
			}
		}
		dsc desc;
		MAKE_desc_from_list(statement, &desc, tmp_list, NULL, "UNION");
		// Only mark upper node as a NULL node when all sub-nodes are NULL
		items->nod_arg[j]->nod_desc.dsc_flags &= ~DSC_null;
		items->nod_arg[j]->nod_desc.dsc_flags |= (desc.dsc_flags & DSC_null);
		for (int i = 0; i < union_node->nod_count; i++) {
			pass1_union_auto_cast(union_node->nod_arg[i], desc, j);
		}
	}
	items = union_node->nod_arg[0]->nod_arg[e_rse_items];

	// Create mappings for union.
	dsql_nod* union_items = MAKE_node(nod_list, items->nod_count);
	{ // scope block
		SSHORT count = 0;
		dsql_nod** uptr = items->nod_arg;
		dsql_nod** ptr = union_items->nod_arg;
		for (const dsql_nod* const* const end = ptr + union_items->nod_count; ptr < end; ptr++)
		{
		    dsql_nod* map_node = MAKE_node(nod_map, e_map_count);
			*ptr = map_node;
			map_node->nod_arg[e_map_context] = (dsql_nod*) union_context;
			dsql_map* map = FB_NEW(*tdbb->getDefaultPool()) dsql_map;
			map_node->nod_arg[e_map_map] = (dsql_nod*) map;

			// set up the dsql_map* between the sub-rses and the union context.
			map->map_position = count++;
			map->map_node = *uptr++;
			map->map_next = union_context->ctx_map;
			union_context->ctx_map = map;
		}
		union_rse->nod_arg[e_rse_items] = union_items;
	} // end scope block

	// Process ORDER clause, if any.
	if (order_list)
	{
		dsql_nod* sort = MAKE_node(nod_list, order_list->nod_count);
		dsql_nod** uptr = sort->nod_arg;
		dsql_nod** ptr = order_list->nod_arg;
		for (const dsql_nod* const* const end = ptr + order_list->nod_count; ptr < end; ptr++, uptr++)
		{
			dsql_nod* order1 = *ptr;
			const dsql_str* collate = 0;
			const dsql_nod* position = order1->nod_arg[e_order_field];

			if (position->nod_type == nod_collate) {
				collate = (dsql_str*) position->nod_arg[e_coll_target];
				position = position->nod_arg[e_coll_source];
			}

			if (position->nod_type != nod_constant || position->nod_desc.dsc_dtype != dtype_long)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  // invalid ORDER BY clause.
						  Arg::Gds(isc_order_by_err));
			}
			const SLONG number = position->getSlong();
			if (number < 1 || number > union_items->nod_count) {
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  // invalid ORDER BY clause.
						  Arg::Gds(isc_order_by_err));
			}

			// make a new order node pointing at the Nth item in the select list.
			dsql_nod* order2 = MAKE_node(nod_order, e_order_count);
			*uptr = order2;
			order2->nod_arg[e_order_field] = union_items->nod_arg[number - 1];
			order2->nod_arg[e_order_flag] = order1->nod_arg[e_order_flag];
			if (collate) {
				order2->nod_arg[e_order_field] =
					pass1_collate(statement, order2->nod_arg[e_order_field], collate);
			}
			order2->nod_arg[e_order_nulls] = order1->nod_arg[e_order_nulls];
		}
		union_rse->nod_arg[e_rse_sort] = sort;
	}

	if (rows)
	{
		pass1_limit(statement, rows, union_rse);
	}

	// PROJECT on all the select items unless UNION ALL was specified.
	if (!(input->nod_flags & NOD_UNION_ALL)) {
		union_rse->nod_arg[e_rse_reduced] = union_items;
	}

	union_rse->nod_flags = flags;
	return union_rse;
}


/**

 	pass1_union_auto_cast

    @brief	Auto cast types to the same type by the rules from
	MAKE_desc_from_list. SELECT X1 FROM .. UNION SELECT X2 FROM ..
	Items X1..Xn are collected together to make the cast-descriptor, this
	was done by the caller (param desc and input is the collection).
	Then is a cast generated (or reused) for every X item if it has
	another descriptor than the param desc.
	Position tells us which column-nr we are processing.

    @param input
    @param desc
    @param position
    @param in_select_list

 **/
static void pass1_union_auto_cast(dsql_nod* input, const dsc& desc,
	SSHORT position, bool in_select_list)
{
	DEV_BLKCHK(input, dsql_type_nod);

	switch (input->nod_type)
	{
		case nod_list:
		case nod_union:
			if (in_select_list)
			{
				if (position < 0 || position >= input->nod_count)
				{
					// Internal dsql error: column position out of range in pass1_union_auto_cast
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_command_err) <<
							  Arg::Gds(isc_dsql_auto_field_bad_pos));
				}
				else
				{
					dsql_nod* select_item = input->nod_arg[position];
					if ((select_item->nod_desc.dsc_dtype != desc.dsc_dtype) ||
						(select_item->nod_desc.dsc_length != desc.dsc_length) ||
						(select_item->nod_desc.dsc_scale != desc.dsc_scale) ||
						(select_item->nod_desc.dsc_sub_type != desc.dsc_sub_type))
					{

						// Because this select item has a different descriptor then
						// our finally descriptor CAST it.
						dsql_nod* cast_node = NULL;
						dsql_nod* alias_node = NULL;

						// Pick a existing cast if available else make a new one.
						if ((select_item->nod_type == nod_alias) &&
							(select_item->nod_arg[e_alias_value]) &&
							(select_item->nod_arg[e_alias_value]->nod_type == nod_cast))
						{
							cast_node = select_item->nod_arg[e_alias_value];
						}
						else if ((select_item->nod_type == nod_derived_field) &&
							(select_item->nod_arg[e_derived_field_value]) &&
							(select_item->nod_arg[e_derived_field_value]->nod_type == nod_cast))
						{
							cast_node = select_item->nod_arg[e_derived_field_value];
						}
						else if (select_item->nod_type == nod_cast) {
							cast_node = select_item;
						}
						else
						{
							thread_db* tdbb = JRD_get_thread_data();
							cast_node = MAKE_node(nod_cast, e_cast_count);
							dsql_fld* afield =
								FB_NEW(*tdbb->getDefaultPool()) dsql_fld(*tdbb->getDefaultPool());
							cast_node->nod_arg[e_cast_target] = (dsql_nod*) afield;
							// We want to leave the ALIAS node on his place, because a UNION
							// uses the select_items from the first sub-rse to determine the
							// columnname.
							if (select_item->nod_type == nod_alias) {
								cast_node->nod_arg[e_cast_source] = select_item->nod_arg[e_alias_value];
							}
							else if (select_item->nod_type == nod_derived_field) {
								cast_node->nod_arg[e_cast_source] = select_item->nod_arg[e_derived_field_value];
							}
							else {
								cast_node->nod_arg[e_cast_source] = select_item;
							}
							// When a cast is created we're losing our fieldname, thus
							// create an alias to keep it.
							const dsql_nod* name_node = select_item;
							while (name_node->nod_type == nod_map)
							{
								// skip all the nod_map nodes
								const dsql_map* map = (dsql_map*) name_node->nod_arg[e_map_map];
								name_node = map->map_node;
							}
							if (name_node->nod_type == nod_field)
							{
								dsql_fld* sub_field = (dsql_fld*) name_node->nod_arg[e_fld_field];
								// Create new node for alias and copy fieldname
								alias_node = MAKE_node(nod_alias, e_alias_count);
								// Copy fieldname to a new string.
								dsql_str* str_alias = FB_NEW_RPT(*tdbb->getDefaultPool(),
															sub_field->fld_name.length()) dsql_str;
								strcpy(str_alias->str_data, sub_field->fld_name.c_str());
								str_alias->str_length = sub_field->fld_name.length();
								alias_node->nod_arg[e_alias_alias] = (dsql_nod*) str_alias;
							}
						}

						dsql_fld* field = (dsql_fld*) cast_node->nod_arg[e_cast_target];
						// Copy the descriptor to a field, because the gen_cast
						// uses a dsql field type.
						field->fld_dtype = desc.dsc_dtype;
						field->fld_scale = desc.dsc_scale;
						field->fld_sub_type = desc.dsc_sub_type;
						field->fld_length = desc.dsc_length;
						field->fld_flags = (desc.dsc_flags & DSC_nullable) ? FLD_nullable : 0;
						if (desc.dsc_dtype <= dtype_any_text) {
							field->fld_ttype = desc.dsc_sub_type;
							field->fld_character_set_id = INTL_GET_CHARSET(&desc);
							field->fld_collation_id = INTL_GET_COLLATE(&desc);
						}
						else if (desc.dsc_dtype == dtype_blob) {
							field->fld_character_set_id = desc.dsc_scale;
							field->fld_collation_id = desc.dsc_flags >> 8;
						}

						// Finally copy the descriptors to the root nodes and swap
						// the necessary nodes.
						cast_node->nod_desc = desc;
						if (select_item->nod_desc.dsc_flags & DSC_nullable) {
							cast_node->nod_desc.dsc_flags |= DSC_nullable;
						}
						if (select_item->nod_type == nod_alias) {
							select_item->nod_arg[e_alias_value] = cast_node;
							select_item->nod_desc = desc;
						}
						else if (select_item->nod_type == nod_derived_field) {
							select_item->nod_arg[e_derived_field_value] = cast_node;
							select_item->nod_desc = desc;
						}
						else {
							// If a new alias was created for keeping original field-name
							// make the alias the "top" node.
							if (alias_node) {
								alias_node->nod_arg[e_alias_value] = cast_node;
								alias_node->nod_desc = cast_node->nod_desc;
								input->nod_arg[position] = alias_node;
							}
							else {
								input->nod_arg[position] = cast_node;
							}
						}
					}
				}
			}
			else
			{
				dsql_nod** ptr = input->nod_arg;
				for (const dsql_nod* const* const end = ptr + input->nod_count;
					ptr < end; ptr++)
				{
					pass1_union_auto_cast(*ptr, desc, position);
				}
			}
			break;

		case nod_rse:
			{
				dsql_nod* streams = input->nod_arg[e_rse_streams];
				pass1_union_auto_cast(streams, desc, position);
				if (streams->nod_type == nod_union) {
					// We're now in a UNION under a UNION so don't change the existing mappings.
					// Only replace the node where the map points to, because they could be changed.
					dsql_nod* union_items = input->nod_arg[e_rse_items];
					dsql_nod* sub_rse_items = streams->nod_arg[0]->nod_arg[e_rse_items];
					dsql_map* map = (dsql_map*) union_items->nod_arg[position]->nod_arg[e_map_map];
					map->map_node = sub_rse_items->nod_arg[position];
					union_items->nod_arg[position]->nod_desc = desc;
				}
				else {
					pass1_union_auto_cast(input->nod_arg[e_rse_items], desc, position, true);
				}
			}
			break;


		default:
			// Nothing
		;
	}
}


/**

 	pass1_update

    @brief	Process UPDATE statement.


    @param statement
    @param input

 **/
static dsql_nod* pass1_update(CompiledStatement* statement, dsql_nod* input, bool insert_or_update)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	const bool isUpdateSqlCompliant = !Config::getOldSetClauseSemantics();

	// Separate old and new context references

	Firebird::Array<dsql_nod*> org_values, new_values;

	dsql_nod* list = input->nod_arg[e_upd_statement];
	fb_assert(list->nod_type == nod_list);

	for (int i = 0; i < list->nod_count; ++i)
	{
		const dsql_nod* const assign = list->nod_arg[i];
		fb_assert(assign->nod_type == nod_assign);
		org_values.add(assign->nod_arg[e_asgn_value]);
		new_values.add(assign->nod_arg[e_asgn_field]);
	}

	dsql_nod** ptr;

	dsql_nod* cursor = input->nod_arg[e_upd_cursor];
	dsql_nod* relation = input->nod_arg[e_upd_relation];

	if (cursor && statement->isPsql())
	{
		dsql_nod* anode = MAKE_node(nod_modify_current, e_mdc_count);
		dsql_ctx* context = pass1_cursor_context(statement, cursor, relation);
		anode->nod_arg[e_mdc_context] = (dsql_nod*) context;

		if (isUpdateSqlCompliant)
		{
			// Process old context values
			statement->req_context->push(context);
			statement->req_scope_level++;
			for (ptr = org_values.begin(); ptr < org_values.end(); ++ptr)
			{
				*ptr = pass1_node_psql(statement, *ptr, false);
			}
			statement->req_scope_level--;
			statement->req_context->pop();
		}

		// Process relation
		anode->nod_arg[e_mdc_update] = pass1_node_psql(statement, relation, false);

		if (!isUpdateSqlCompliant)
		{
			// Process old context values
			for (ptr = org_values.begin(); ptr < org_values.end(); ++ptr)
			{
				*ptr = pass1_node_psql(statement, *ptr, false);
			}
		}

		// Process new context values
		for (ptr = new_values.begin(); ptr < new_values.end(); ++ptr)
		{
			*ptr = pass1_node_psql(statement, *ptr, false);
		}

		anode->nod_arg[e_mdc_return] = process_returning(statement, input->nod_arg[e_upd_return]);

		statement->req_context->pop();
		// Recreate list of assignments
		anode->nod_arg[e_mdc_statement] = list = MAKE_node(nod_list, list->nod_count);
		for (int i = 0; i < list->nod_count; ++i)
		{
			dsql_nod* assign = MAKE_node(nod_assign, e_asgn_count);
			assign->nod_arg[e_asgn_value] = org_values[i];
			assign->nod_arg[e_asgn_field] = new_values[i];
			list->nod_arg[i] = assign;
		}
		// We do not allow cases like UPDATE T SET f1 = v1, f2 = v2, f1 = v3...
		field_appears_once(anode->nod_arg[e_mdc_statement],
						   input->nod_arg[e_upd_statement],
						   false, "UPDATE");
		return anode;
	}

	statement->req_type = cursor ? REQ_UPDATE_CURSOR : REQ_UPDATE;

	dsql_nod* node = MAKE_node(nod_modify, e_mod_count);
	node->nod_arg[e_mod_update] = pass1_node_psql(statement, relation, false);
	dsql_ctx* mod_context = get_context(node->nod_arg[e_mod_update]);

	if (!isUpdateSqlCompliant)
	{
		// Process old context values
		for (ptr = org_values.begin(); ptr < org_values.end(); ++ptr)
		{
			*ptr = pass1_node_psql(statement, *ptr, false);
		}
	}

	// Process new context values
	for (ptr = new_values.begin(); ptr < new_values.end(); ++ptr)
	{
		*ptr = pass1_node_psql(statement, *ptr, false);
	}

	statement->req_context->pop();

	// Generate record selection expression

	dsql_nod* rse;
	if (cursor) {
		rse = pass1_cursor_reference(statement, cursor, relation);
	}
	else
	{
		rse = MAKE_node(nod_rse, e_rse_count);
		rse->nod_flags = (USHORT)(IPTR) input->nod_arg[e_upd_rse_flags];
		if (input->nod_arg[e_upd_return])
			rse->nod_flags |= NOD_SELECT_EXPR_SINGLETON;

		dsql_nod* temp = MAKE_node(nod_list, 1);
		rse->nod_arg[e_rse_streams] = temp;
		temp->nod_arg[0] = pass1_node_psql(statement, relation, false);
		dsql_ctx* old_context = get_context(temp->nod_arg[0]);

		if ( (temp = input->nod_arg[e_upd_boolean]) ) {
			rse->nod_arg[e_rse_boolean] = pass1_node_psql(statement, temp, false);
		}

		if ( (temp = input->nod_arg[e_upd_plan]) ) {
			rse->nod_arg[e_rse_plan] = pass1_node_psql(statement, temp, false);
		}

		if ( (temp = input->nod_arg[e_upd_sort]) ) {
			rse->nod_arg[e_rse_sort] = pass1_sort(statement, temp, NULL);
		}

		if ( (temp = input->nod_arg[e_upd_rows]) ) {
			pass1_limit(statement, temp, rse);
		}

		if (input->nod_arg[e_upd_return])
		{
			// ASF: We have the RSE context in the stack.
			// Then we change his name to "OLD".
			const TEXT* const save_alias = old_context->ctx_alias;
			const TEXT* const save_internal_alias = old_context->ctx_internal_alias;
			const USHORT save_flags = old_context->ctx_flags;

			old_context->ctx_alias = old_context->ctx_internal_alias =
				MAKE_cstring(OLD_CONTEXT)->str_data;
			old_context->ctx_flags |= CTX_system | CTX_returning;

			// ASF: Trick to make OLD.* nullable - CORE-2044.
			if (insert_or_update)
				old_context->ctx_flags |= CTX_outer_join;

			// push the modify context in the same scope level
			statement->req_context->push(mod_context);

			// clone the modify context and push with name "NEW" in a greater scope level
			dsql_ctx* new_context = FB_NEW(statement->req_pool) dsql_ctx(statement->req_pool);
			*new_context = *mod_context;
			new_context->ctx_scope_level = ++statement->req_scope_level;
			new_context->ctx_alias = new_context->ctx_internal_alias =
				MAKE_cstring(NEW_CONTEXT)->str_data;
			new_context->ctx_flags |= CTX_system | CTX_returning;
			statement->req_context->push(new_context);

			// Process the RETURNING with the stack (NEW, (modify, OLD)),
			// so unqualified or qualified by NEW or the table name fields be
			// resolved to the changed record.
			node->nod_arg[e_mod_return] = process_returning(statement, input->nod_arg[e_upd_return]);

			// restore the stack with only the RSE context
			--statement->req_scope_level;
			statement->req_context->pop();
			statement->req_context->pop();

			old_context->ctx_flags = save_flags;
			old_context->ctx_internal_alias = save_internal_alias;
			old_context->ctx_alias = save_alias;
		}
	}

	node->nod_arg[e_mod_source] = rse->nod_arg[e_rse_streams]->nod_arg[0];
	node->nod_arg[e_mod_rse] = rse;

	if (isUpdateSqlCompliant)
	{
		// Process old context values
		for (ptr = org_values.begin(); ptr < org_values.end(); ++ptr)
		{
			*ptr = pass1_node_psql(statement, *ptr, false);
		}
	}

	statement->req_context->pop();

	// Recreate list of assignments
	node->nod_arg[e_mod_statement] = list = MAKE_node(nod_list, list->nod_count);

	for (int j = 0; j < list->nod_count; ++j)
	{
		dsql_nod* const sub1 = org_values[j];
		dsql_nod* const sub2 = new_values[j];
		if (!set_parameter_type(statement, sub1, sub2, false))
		{
			set_parameter_type(statement, sub2, sub1, false);
		}
		dsql_nod* assign = MAKE_node(nod_assign, e_asgn_count);
		assign->nod_arg[e_asgn_value] = sub1;
		assign->nod_arg[e_asgn_field] = sub2;
		list->nod_arg[j] = assign;
	}

	// We do not allow cases like UPDATE T SET f1 = v1, f2 = v2, f1 = v3...
	field_appears_once(node->nod_arg[e_mod_statement],
					   input->nod_arg[e_upd_statement],
					   false, "UPDATE");

	set_parameters_name(node->nod_arg[e_mod_statement], node->nod_arg[e_mod_update]);

	if (!insert_or_update)
		node = nullify_returning(statement, node);

	return node;
}


/**

 	pass1_update_or_insert

    @brief	Process UPDATE OR INSERT statement.


    @param statement
    @param input

 **/
static dsql_nod* pass1_update_or_insert(CompiledStatement* statement, dsql_nod* input)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	if (!statement->isPsql())
		statement->req_flags |= REQ_dsql_upd_or_ins;

	const dsql_str* relation_name = (dsql_str*) input->nod_arg[e_upi_relation]->nod_arg[e_rpn_name];
	const dsql_str* base_name = relation_name;

	dsql_nod* values = input->nod_arg[e_upi_values];

	// build the INSERT node
	dsql_nod* insert = MAKE_node(nod_insert, e_ins_count);
	insert->nod_arg[e_ins_relation] = input->nod_arg[e_upi_relation];
	insert->nod_arg[e_ins_fields] = input->nod_arg[e_upi_fields];
	insert->nod_arg[e_ins_values] = values;
	insert->nod_arg[e_ins_return] = input->nod_arg[e_upi_return];
	insert = pass1_insert(statement, insert, true);

	// PASS1_statement will transform nod_insert to nod_store
	fb_assert(insert->nod_type == nod_store);

	dsql_ctx* context = (dsql_ctx*) insert->nod_arg[e_sto_relation]->nod_arg[e_rel_context];
	DEV_BLKCHK(context, dsql_type_ctx);

	dsql_rel* relation = context->ctx_relation;
	dsql_nod* fields = input->nod_arg[e_upi_fields];

	// if a field list isn't present, build one using the same
	// rules of INSERT INTO table VALUES ...
	if (!fields)
		fields = explode_fields(relation);

	// maintain a pair of view's field name / base field name
	MetaNamePairMap view_fields;

	if ((relation->rel_flags & REL_view) && !input->nod_arg[e_upi_matching])
	{
		dsql_rel* base_rel = METD_get_view_base(statement, relation_name->str_data, view_fields);

		// get the base table name if there is only one
		if (base_rel)
			base_name = MAKE_cstring(base_rel->rel_name.c_str());
		else
			ERRD_post(Arg::Gds(isc_upd_ins_with_complex_view));
	}

	dsql_nod* matching = input->nod_arg[e_upi_matching];
	nod_t equality_type;

	if (matching)
	{
		equality_type = nod_equiv;

		statement->req_context->push(context);
		statement->req_scope_level++;

		const dsql_nod* matching_fields = pass1_node_psql(statement, matching, false);

		statement->req_scope_level--;
		statement->req_context->pop();

		field_appears_once(matching_fields, matching, true, "UPDATE OR INSERT");
	}
	else
	{
		equality_type = nod_eql;

		matching = METD_get_primary_key(statement, base_name);

		if (!matching)
		{
			ERRD_post(Arg::Gds(isc_primary_key_required) << Arg::Str(base_name->str_data));
		}
	}

	// build a boolean to use in the UPDATE statement
	dsql_nod* match = NULL;
	USHORT match_count = 0;

	DsqlNodStack varStack;

	DsqlNodStack stack;
	dsql_nod** field_ptr = fields->nod_arg;
	dsql_nod** value_ptr = values->nod_arg;

	for (const dsql_nod* const* const field_end = field_ptr + fields->nod_count;
		 field_ptr < field_end; field_ptr++, value_ptr++)
	{
		DEV_BLKCHK(*field_ptr, dsql_type_nod);
		DEV_BLKCHK(*value_ptr, dsql_type_nod);

		dsql_nod* assign = MAKE_node(nod_assign, e_asgn_count);
		assign->nod_arg[e_asgn_value] = *value_ptr;
		assign->nod_arg[e_asgn_field] = *field_ptr;
		stack.push(assign);

		dsql_nod* temp = *value_ptr;
		dsql_nod* temp2 = insert->nod_arg[e_sto_statement]->nod_arg[field_ptr - fields->nod_arg]->nod_arg[1];
		set_parameter_type(statement, temp, temp2, false);

		fb_assert((*field_ptr)->nod_type == nod_field_name);

		// When relation is a view and MATCHING was not specified, field_name
		// stores the base field name that is what we should find in the primary
		// key of base table.
		Firebird::MetaName field_name;

		if ((relation->rel_flags & REL_view) && !input->nod_arg[e_upi_matching])
		{
			view_fields.get(
				Firebird::MetaName(((dsql_str*) (*field_ptr)->nod_arg[e_fln_name])->str_data),
				field_name);
		}
		else
			field_name = ((dsql_str*) (*field_ptr)->nod_arg[e_fln_name])->str_data;

		if (field_name.hasData())
		{
			dsql_nod** matching_ptr = matching->nod_arg;

			for (const dsql_nod* const* const matching_end = matching_ptr + matching->nod_count;
				 matching_ptr < matching_end; matching_ptr++)
			{
				DEV_BLKCHK(*matching_ptr, dsql_type_nod);
				fb_assert((*matching_ptr)->nod_type == nod_field_name);

				const Firebird::MetaName
					testField(((dsql_str*)(*matching_ptr)->nod_arg[e_fln_name])->str_data);

				if (testField == field_name)
				{
					++match_count;

					const size_t fieldPos = field_ptr - fields->nod_arg;
					dsql_nod*& expr = insert->nod_arg[e_sto_statement]->nod_arg[fieldPos]->nod_arg[0];
					dsql_nod* var = pass1_hidden_variable(statement, expr);

					if (var)
					{
						dsql_nod* varAssign = MAKE_node(nod_assign, e_asgn_count);
						varAssign->nod_arg[e_asgn_value] = expr->nod_arg[e_hidden_var_expr];
						varAssign->nod_arg[e_asgn_field] = expr->nod_arg[e_hidden_var_var];
						varStack.push(varAssign);

						assign->nod_arg[e_asgn_value] = expr = var;
					}
					else
						var = *value_ptr;

					dsql_nod* eql = MAKE_node(equality_type, 2);
					eql->nod_arg[0] = *field_ptr;
					eql->nod_arg[1] = var;

					if (match)
					{
						// It's a composed MATCHING. Build an AND.
						dsql_nod* and_node = MAKE_node(nod_and, 2);
						and_node->nod_arg[0] = match;
						and_node->nod_arg[1] = eql;
						match = and_node;
					}
					else
						match = eql;
				}
			}
		}
	}

	// check if implicit or explicit MATCHING is valid
	if (match_count != matching->nod_count)
	{
		if (input->nod_arg[e_upi_matching])
			ERRD_post(Arg::Gds(isc_upd_ins_doesnt_match_matching));
		else
		{
			ERRD_post(Arg::Gds(isc_upd_ins_doesnt_match_pk) << Arg::Str(base_name->str_data));
		}
	}

	// build the UPDATE node
	dsql_nod* update = MAKE_node(nod_update, e_upd_count);
	update->nod_arg[e_upd_relation] = input->nod_arg[e_upi_relation];
	update->nod_arg[e_upd_statement] = MAKE_list(stack);
	update->nod_arg[e_upd_boolean] = match;

	if (input->nod_arg[e_upi_return])
	{
		update->nod_arg[e_upd_rse_flags] = (dsql_nod*)(IPTR) NOD_SELECT_EXPR_SINGLETON;

		dsql_nod* store_ret = insert->nod_arg[e_sto_return];

		// nod_returning was already processed
		fb_assert(store_ret->nod_type == nod_list);

		// And we create an already processed RETURNING, because
		// nod_returning creates parameters and they're already
		// created by the INSERT statement.
		dsql_nod* update_ret = update->nod_arg[e_upd_return] =
			MAKE_node(nod_list, store_ret->nod_count);

		dsql_nod** src_ptr = input->nod_arg[e_upi_return]->nod_arg[e_ret_source]->nod_arg;
		dsql_nod** dst_ptr = store_ret->nod_arg;
		dsql_nod** ptr = update_ret->nod_arg;

		for (const dsql_nod* const* const end = ptr + update_ret->nod_count;
			ptr < end; src_ptr++, dst_ptr++, ptr++)
		{
			dsql_nod* temp = MAKE_node(nod_assign, e_asgn_count);
			temp->nod_arg[e_asgn_value] = *src_ptr;
			temp->nod_arg[e_asgn_field] = (*dst_ptr)->nod_arg[1];
			*ptr = temp;
		}
	}

	update = pass1_update(statement, update, true);

	// PASS1_statement will transform nod_update to nod_modify
	fb_assert(update->nod_type == nod_modify);

	// test if ROW_COUNT = 0
	dsql_nod* eql = MAKE_node(nod_eql, 2);
	eql->nod_arg[0] = MAKE_node(nod_internal_info, e_internal_info_count);
	eql->nod_arg[0]->nod_arg[e_internal_info] = MAKE_const_slong(internal_rows_affected);
	eql->nod_arg[1] = MAKE_const_slong(0);

	const ULONG save_req_flags = statement->req_flags;
	statement->req_flags |= REQ_block;	// to compile ROW_COUNT
	eql = PASS1_node(statement, eql);
	statement->req_flags = save_req_flags;

	// if (ROW_COUNT = 0) then INSERT
	dsql_nod* if_nod = MAKE_node(nod_if, e_if_count);
	if_nod->nod_arg[e_if_condition] = eql;
	if_nod->nod_arg[e_if_true] = insert;

	// build the temporary vars / UPDATE / IF nodes
	dsql_nod* list = MAKE_node(nod_list, 3);
	list->nod_arg[0] = MAKE_list(varStack);
	list->nod_arg[0]->nod_flags |= NOD_SIMPLE_LIST;
	list->nod_arg[1] = update;
	list->nod_arg[2] = if_nod;

	// if RETURNING is present, req_type is already REQ_EXEC_PROCEDURE
	if (!input->nod_arg[e_upi_return])
		statement->req_type = REQ_INSERT;

	return list;
}


/**
	resolve_variable_name

 **/
static dsql_nod* resolve_variable_name(const dsql_nod* var_nodes, const dsql_str* var_name)
{
	dsql_nod* const* ptr = var_nodes->nod_arg;
	dsql_nod* const* const end = ptr + var_nodes->nod_count;

	for (; ptr < end; ptr++)
	{
		dsql_nod* var_node = *ptr;
		if (var_node->nod_type == nod_variable)
		{
			const dsql_var* variable = (dsql_var*) var_node->nod_arg[e_var_variable];
			DEV_BLKCHK(variable, dsql_type_var);
			if (!strcmp(var_name->str_data, variable->var_name))
			{
				 return var_node;
			}
		}
	}

	return NULL;
}


/**

 	pass1_variable

    @brief	Resolve a variable name to an available variable.


    @param statement
    @param input

 **/
static dsql_nod* pass1_variable( CompiledStatement* statement, dsql_nod* input)
{
	// CVC: I commented this variable and its usage because it wasn't useful for
	// anything. I didn't delete it in case it's an implementation in progress
	// by someone.
	//SSHORT position;

	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	const dsql_str* var_name = 0;
	if (input->nod_type == nod_field_name)
	{
		if (input->nod_arg[e_fln_context])
		{
			if (statement->req_flags & REQ_trigger) // triggers only
				return pass1_field(statement, input, false, NULL);

			field_unknown(0, 0, input);
		}
		var_name = (dsql_str*) input->nod_arg[e_fln_name];
	}
	else
		var_name = (dsql_str*) input->nod_arg[e_vrn_name];

	DEV_BLKCHK(var_name, dsql_type_str);

	dsql_nod* var_nodes;
	if (statement->req_flags & REQ_procedure) // procedures and triggers
	{
		dsql_nod* procedure_node = statement->req_ddl_node;
		fb_assert(procedure_node);
		if (!(statement->req_flags & REQ_trigger)) // no, procedures only
		{
			// try to resolve variable name against input and output parameters
			var_nodes = procedure_node->nod_arg[e_prc_inputs];
			if (var_nodes)
			{
				//position = 0;
				dsql_nod** ptr = var_nodes->nod_arg;
				for (const dsql_nod* const* const end = ptr + var_nodes->nod_count; ptr < end; ptr++)
				{
					dsql_nod* var_node = *ptr;
					const dsql_var* variable = (dsql_var*) var_node->nod_arg[e_var_variable];
					DEV_BLKCHK(variable, dsql_type_var);
					if (!strcmp(var_name->str_data, variable->var_name))
					{
						return var_node;
					}
				}
			}
			var_nodes = procedure_node->nod_arg[e_prc_outputs];
			if (var_nodes)
			{
				//position = 0;
				dsql_nod** ptr = var_nodes->nod_arg;
				for (const dsql_nod* const* const end = ptr + var_nodes->nod_count; ptr < end; ptr++)
				{
					dsql_nod* var_node = *ptr;
					const dsql_var* variable = (dsql_var*) var_node->nod_arg[e_var_variable];
					DEV_BLKCHK(variable, dsql_type_var);
					if (!strcmp(var_name->str_data, variable->var_name))
					{
						return var_node;
					}
				}
			}
			var_nodes = procedure_node->nod_arg[e_prc_dcls];
		}
		else
			var_nodes = procedure_node->nod_arg[e_trg_actions]->nod_arg[e_trg_act_dcls];

		if (var_nodes)
		{
			// try to resolve variable name against local variables
			//position = 0;
			dsql_nod** ptr = var_nodes->nod_arg;
			for (const dsql_nod* const* const end = ptr + var_nodes->nod_count; ptr < end; ptr++)
			{
				dsql_nod* var_node = *ptr;
				if (var_node->nod_type == nod_variable)
				{
					const dsql_var* variable = (dsql_var*) var_node->nod_arg[e_var_variable];
					DEV_BLKCHK(variable, dsql_type_var);
					if (!strcmp(var_name->str_data, variable->var_name))
					{
						return var_node;
					}
				}
			}
		}
	}

	if (statement->req_blk_node)
	{
		dsql_nod* var_node;

		if (var_nodes = statement->req_blk_node->nod_arg[e_exe_blk_dcls])
		{
			if (var_node = resolve_variable_name(var_nodes, var_name))
				return var_node;
		}

		if (var_nodes = statement->req_blk_node->nod_arg[e_exe_blk_inputs])
		{
			if (var_node = resolve_variable_name(var_nodes, var_name))
				return var_node;
		}

		if (var_nodes = statement->req_blk_node->nod_arg[e_exe_blk_outputs])
		{
			if (var_node = resolve_variable_name(var_nodes, var_name))
				return var_node;
		}
	}

	// field unresolved
	// CVC: That's all [the fix], folks!

	if (var_name)
		field_unknown(0, var_name->str_data, input);
	else
		field_unknown(0, 0, input);

	return NULL;
}


/**

 	post_map

    @brief	Post an item to a map for a context.


    @param node
    @param context

 **/
static dsql_nod* post_map( dsql_nod* node, dsql_ctx* context)
{
	DEV_BLKCHK(node, dsql_type_nod);
	DEV_BLKCHK(context, dsql_type_ctx);

	thread_db* tdbb = JRD_get_thread_data();

	// Check to see if the item has already been posted

	int count = 0;
	dsql_map* map;
	for (map = context->ctx_map; map; map = map->map_next, ++count)
	{
		if (node_match(node, map->map_node, false))
			break;
	}

	if (!map)
	{
		map = FB_NEW(*tdbb->getDefaultPool()) dsql_map;
		map->map_position = (USHORT) count;
		map->map_next = context->ctx_map;
		context->ctx_map = map;
		map->map_node = node;
	}

	dsql_nod* new_node = MAKE_node(nod_map, e_map_count);
	new_node->nod_count = 0;
	new_node->nod_arg[e_map_context] = (dsql_nod*) context;
	new_node->nod_arg[e_map_map] = (dsql_nod*) map;
	new_node->nod_desc = node->nod_desc;

	return new_node;
}


/**

 	remap_field

    @brief	Called to map fields used in an aggregate-context
	after all pass1 calls (SELECT-, ORDER BY-lists).
    Walk completly through the given node 'field' and
	map the fields with same scope_level as the given context
	to the given context with the post_map function.


    @param statement
    @param field
    @param context
    @param current_level

 **/
static dsql_nod* remap_field(CompiledStatement* statement, dsql_nod* field,
	dsql_ctx* context, USHORT current_level)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(field, dsql_type_nod);
	DEV_BLKCHK(context, dsql_type_ctx);

	if (!field)
		return NULL;

	switch (field->nod_type)
	{

		case nod_alias:
			field->nod_arg[e_alias_value] =
				remap_field(statement, field->nod_arg[e_alias_value], context, current_level);
			return field;

		case nod_derived_field:
			{
				// If we got a field from a derived table we should not remap anything
				// deeper in the alias, but this "virtual" field should be mapped to
				// the given context (of course only if we're in the same scope-level).
				const USHORT lscope_level = (USHORT)(U_IPTR) field->nod_arg[e_derived_field_scope];
				if (lscope_level == context->ctx_scope_level) {
					return post_map(field, context);
				}

				if (context->ctx_scope_level < lscope_level) {
					field->nod_arg[e_derived_field_value] =
						remap_field(statement, field->nod_arg[e_derived_field_value],
									context, current_level);
				}
				return field;
			}

		case nod_field:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(field->nod_arg[e_fld_context]);
				if (lcontext->ctx_scope_level == context->ctx_scope_level) {
					return post_map(field, context);
				}

				return field;
			}

		case nod_map:
			{
				const dsql_ctx* lcontext = reinterpret_cast<dsql_ctx*>(field->nod_arg[e_map_context]);
				if (lcontext->ctx_scope_level != context->ctx_scope_level) {
					dsql_map* lmap = reinterpret_cast<dsql_map*>(field->nod_arg[e_map_map]);
					lmap->map_node = remap_field(statement, lmap->map_node, context,
												 lcontext->ctx_scope_level);
				}
				return field;
			}

		case nod_agg_count:
		case nod_agg_min:
		case nod_agg_max:
		case nod_agg_average:
		case nod_agg_total:
		case nod_agg_average2:
		case nod_agg_total2:
		case nod_agg_list:
			{
				USHORT ldeepest_level = statement->req_scope_level;
				USHORT lcurrent_level = current_level;
				if (aggregate_found2(statement, field, &lcurrent_level, &ldeepest_level, false))
				{
					if (statement->req_scope_level == ldeepest_level) {
						return post_map(field, context);
					}

					if (field->nod_count) {
						field->nod_arg[e_agg_function_expression] =
							remap_field(statement, field->nod_arg[e_agg_function_expression],
										context, current_level);
					}
					return field;
				}

				if (field->nod_count) {
					field->nod_arg[e_agg_function_expression] =
						remap_field(statement, field->nod_arg[e_agg_function_expression],
									context, current_level);
				}
				return field;
			}

		case nod_via:
			field->nod_arg[e_via_rse] =
				remap_field(statement, field->nod_arg[e_via_rse], context, current_level);
			field->nod_arg[e_via_value_1] = field->nod_arg[e_via_rse]->nod_arg[e_rse_items]->nod_arg[0];
			return field;

		case nod_rse:
			current_level++;
			field->nod_arg[e_rse_streams] =
				remap_field(statement, field->nod_arg[e_rse_streams], context, current_level);
			field->nod_arg[e_rse_boolean] =
				remap_field(statement, field->nod_arg[e_rse_boolean], context, current_level);
			field->nod_arg[e_rse_items] =
				remap_field(statement, field->nod_arg[e_rse_items], context, current_level);
			field->nod_arg[e_rse_sort] =
				remap_field(statement, field->nod_arg[e_rse_sort], context, current_level);
			current_level--;
			return field;

		case nod_simple_case:
		case nod_searched_case:
			{
				dsql_nod** ptr = field->nod_arg;
				for (const dsql_nod* const* const end = ptr + field->nod_count; ptr < end; ptr++)
				{
					*ptr = remap_field(statement, *ptr, context, current_level);
				}
				return field;
			}

		case nod_coalesce:
			// ASF: We had deliberately changed nod_count to 1, to not process the second list.
			// But we should remap its fields. CORE-2176
			field->nod_arg[0] = remap_field(statement, field->nod_arg[0], context, current_level);
			field->nod_arg[1] = remap_field(statement, field->nod_arg[1], context, current_level);
			return field;

		case nod_aggregate:
			field->nod_arg[e_agg_rse] =
				remap_field(statement, field->nod_arg[e_agg_rse], context, current_level);
			return field;

		case nod_order:
			field->nod_arg[e_order_field] =
				remap_field(statement, field->nod_arg[e_order_field], context, current_level);
			return field;

		case nod_or:
		case nod_and:
		case nod_not:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_gtr:
		case nod_geq:
		case nod_lss:
		case nod_leq:
		case nod_eql_any:
		case nod_neq_any:
		case nod_gtr_any:
		case nod_geq_any:
		case nod_lss_any:
		case nod_leq_any:
		case nod_eql_all:
		case nod_neq_all:
		case nod_gtr_all:
		case nod_geq_all:
		case nod_lss_all:
		case nod_leq_all:
		case nod_any:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_between:
		case nod_like:
		case nod_containing:
		case nod_similar:
		case nod_starting:
		case nod_exists:
		case nod_singular:
		case nod_missing:
		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_subtract2:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_internal_info:
		case nod_extract:
		case nod_strlen:
		case nod_list:
		case nod_join:
		case nod_join_inner:
		case nod_join_left:
		case nod_join_right:
		case nod_join_full:
			{
				dsql_nod** ptr = field->nod_arg;
				for (const dsql_nod* const* const end = ptr + field->nod_count; ptr < end; ptr++)
				{
					*ptr = remap_field(statement, *ptr, context, current_level);
				}
				return field;
			}

		case nod_cast:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_udf:
		case nod_sys_function:
			if (field->nod_count == 2) {
				field->nod_arg[1] = remap_field(statement, field->nod_arg[1], context, current_level);
			}
			return field;

		case nod_relation:
			{
				dsql_ctx* lrelation_context = reinterpret_cast<dsql_ctx*>(field->nod_arg[e_rel_context]);
				// Check if relation is a procedure
				if (lrelation_context->ctx_procedure) {
					// Remap the input parameters
					lrelation_context->ctx_proc_inputs =
						remap_field(statement, lrelation_context->ctx_proc_inputs, context, current_level);
				}
				return field;
			}

		case nod_derived_table:
			remap_field(statement, field->nod_arg[e_derived_table_rse], context, current_level);
			return field;

		case nod_hidden_var:
			field->nod_arg[e_hidden_var_expr] = remap_field(statement,
				field->nod_arg[e_hidden_var_expr], context, current_level);
			return field;

		case nod_dbkey:
			return post_map(field, context);

		default:
			return field;
	}
}


/**

 	remap_fields

    @brief	Remap fields inside a field list against
	an artificial context.


    @param statement
    @param fields
    @param context

 **/
static dsql_nod* remap_fields(CompiledStatement* statement, dsql_nod* fields, dsql_ctx* context)
{
	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(fields, dsql_type_nod);
	DEV_BLKCHK(context, dsql_type_ctx);

	if (fields->nod_type == nod_list)
	{
		for (int i = 0; i < fields->nod_count; i++)
		{
			fields->nod_arg[i] =
				remap_field(statement, fields->nod_arg[i], context, statement->req_scope_level);
		}
	}
	else {
		fields = remap_field(statement, fields, context, statement->req_scope_level);
	}

	return fields;
}


/**

 remap_streams_to_parent_context

    @brief	For each relation in the list, flag the relation's context
 	as having a parent context.  Be sure to handle joins
 	(Bug 6674).


    @param input
    @param parent_context

 **/
static void remap_streams_to_parent_context( dsql_nod* input, dsql_ctx* parent_context)
{
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(parent_context, dsql_type_ctx);

	switch (input->nod_type)
	{
	case nod_list:
		{
			dsql_nod** ptr = input->nod_arg;
			for (const dsql_nod* const* const end = ptr + input->nod_count; ptr < end; ptr++)
			{
				remap_streams_to_parent_context(*ptr, parent_context);
			}
		}
		break;

	case nod_relation:
		{
			dsql_ctx* context = (dsql_ctx*) input->nod_arg[e_rel_context];
			DEV_BLKCHK(context, dsql_type_ctx);
			context->ctx_parent = parent_context;
		}
		break;

	case nod_join:
		remap_streams_to_parent_context(input->nod_arg[e_join_left_rel], parent_context);
		remap_streams_to_parent_context(input->nod_arg[e_join_rght_rel], parent_context);
		break;

	case nod_derived_table:
		{
			dsql_nod* list = input->nod_arg[e_derived_table_rse]->nod_arg[e_rse_streams];
			remap_streams_to_parent_context(list, parent_context);
		}
		break;

	case nod_union:
		{
			dsql_nod** rse = input->nod_arg;
			for (const dsql_nod* const* const end = rse + input->nod_count; rse < end; rse++)
			{
				remap_streams_to_parent_context((*rse)->nod_arg[e_rse_streams], parent_context);
			}
		}
		break;

	case nod_aggregate:
		break;

	default:
		fb_assert(false);
		break;
	}
}


/**

 	resolve_context

    @brief	Attempt to resolve field against context.
 	Return first field in context if successful,
 	NULL if not.


    @param statement
    @param name
    @param qualifier
    @param context

 **/
static dsql_fld* resolve_context( CompiledStatement* statement, const dsql_str* qualifier,
	dsql_ctx* context, bool isCheckConstraint, bool resolveByAlias)
{
	// CVC: Warning: the second param, "name" was is not used anymore and
	// therefore it was removed. Thus, the local variable "table_name"
	// is being stripped here to avoid mismatches due to trailing blanks.

	DEV_BLKCHK(statement, dsql_type_req);
	DEV_BLKCHK(qualifier, dsql_type_str);
	DEV_BLKCHK(context, dsql_type_ctx);

	if ((statement->req_flags & REQ_returning_into) && (context->ctx_flags & CTX_returning))
		return NULL;

	dsql_rel* relation = context->ctx_relation;
	dsql_prc* procedure = context->ctx_procedure;
	if (!relation && !procedure) {
		return NULL;
	}

	// if there is no qualifier, then we cannot match against
	// a context of a different scoping level
	// AB: Yes we can, but the scope level where the field is has priority.
//	if (!qualifier && context->ctx_scope_level != statement->req_scope_level) {
//		return NULL;
//	}

	// AB: If this context is a system generated context as in NEW/OLD inside
	// triggers, the qualifier by the field is mandatory. While we can't
	// fall back from a higher scope-level to the NEW/OLD contexts without
	// the qualifier present.
	// An exception is a check-constraint that is allowed to reference fields
	// without the qualifier.
	if (!isCheckConstraint && (context->ctx_flags & CTX_system) && !qualifier) {
		return NULL;
	}

	const TEXT* table_name = NULL;
	if (context->ctx_internal_alias && resolveByAlias) {
		table_name = context->ctx_internal_alias;
	}
	// AB: For a check constraint we should ignore the alias if the alias
	// contains the "NEW" alias. This is because it is possible
	// to reference a field by the complete table-name as alias
	// (see EMPLOYEE table in examples for a example).
	if (isCheckConstraint && table_name)
	{
		// If a qualifier is present and it's equal to the alias then we've already the right table-name
		if (!(qualifier && !strcmp(qualifier->str_data, table_name)))
		{
			if (!strcmp(table_name, NEW_CONTEXT))
			{
				table_name = NULL;
			}
			else if (!strcmp(table_name, OLD_CONTEXT))
			{
				// Only use the OLD context if it is explicit used. That means the
				// qualifer should hold the "OLD" alias.
				return NULL;
			}
		}
	}
	if (table_name == NULL) {
		if (relation)
			table_name = relation->rel_name.c_str();
		else
			table_name = procedure->prc_name.c_str();
	}

	// If a context qualifier is present, make sure this is the proper context
	if (qualifier && strcmp(qualifier->str_data, table_name)) {
		return NULL;
	}

	// Lookup field in relation or procedure

	dsql_fld* field;
	if (relation) {
		field = relation->rel_fields;
	}
	else {
		field = procedure->prc_outputs;
	}

	return field;
}


// Resolve a field for JOIN USING purposes
static dsql_nod* resolve_using_field(CompiledStatement* statement, dsql_str* name, DsqlNodStack& stack,
	const dsql_nod* flawedNode, const TEXT* side, dsql_ctx*& ctx)
{
	dsql_nod* list = MAKE_list(stack);
	dsql_nod* node = pass1_lookup_alias(statement, name, list, false);

	if (!node)
	{
		Firebird::string qualifier;
		qualifier.printf("<%s side of USING>", side);
		field_unknown(qualifier.c_str(), name->str_data, flawedNode);
	}

	switch (node->nod_type)
	{
	case nod_derived_field:
		ctx = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_derived_field_context]);
		break;
	case nod_field:
		ctx = reinterpret_cast<dsql_ctx*>(node->nod_arg[e_fln_context]);
		break;
	case nod_alias:
		fb_assert(node->nod_count >= (int) e_alias_imp_join - 1);
		ctx = reinterpret_cast<ImplicitJoin*>(node->nod_arg[e_alias_imp_join])->visibleInContext;
		break;
	default:
		fb_assert(false);
	}

	return node;
}


/**

 	set_parameter_type

    @brief	Setup the datatype of a parameter.


    @param in_node
    @param node
    @param force_varchar

 **/
static bool set_parameter_type(CompiledStatement* statement, dsql_nod* in_node,
	dsql_nod* node, bool force_varchar)
{
	thread_db* tdbb = JRD_get_thread_data();
	Attachment* att = tdbb->getAttachment();

	DEV_BLKCHK(in_node, dsql_type_nod);
	DEV_BLKCHK(node, dsql_type_nod);

	if (in_node == NULL)
		return false;

	switch (in_node->nod_type)
	{
		case nod_parameter:
			{
				if (!node)
					in_node->nod_desc.makeNullString();
				else
				{
					MAKE_desc(statement, &in_node->nod_desc, node, NULL);

					if (att->att_charset != CS_NONE && att->att_charset != CS_BINARY)
					{
						const USHORT fromCharSet = in_node->nod_desc.getCharSet();
						const USHORT toCharSet = (fromCharSet == CS_NONE || fromCharSet == CS_BINARY) ?
							fromCharSet : att->att_charset;

						if (in_node->nod_desc.dsc_dtype <= dtype_any_text)
						{
							int diff = 0;
							switch (in_node->nod_desc.dsc_dtype)
							{
							case dtype_varying:
								diff = sizeof(USHORT);
								break;
							case dtype_cstring:
								diff = 1;
								break;
							}
							in_node->nod_desc.dsc_length -= diff;

							if (toCharSet != fromCharSet)
							{
								const USHORT fromCharSetBPC = METD_get_charset_bpc(statement, fromCharSet);
								const USHORT toCharSetBPC = METD_get_charset_bpc(statement, toCharSet);

								INTL_ASSIGN_TTYPE(&in_node->nod_desc, INTL_CS_COLL_TO_TTYPE(toCharSet,
									(fromCharSet == toCharSet ? INTL_GET_COLLATE(&in_node->nod_desc) : 0)));

								in_node->nod_desc.dsc_length =
									UTLD_char_length_to_byte_length(in_node->nod_desc.dsc_length / fromCharSetBPC, toCharSetBPC, diff);
							}

							in_node->nod_desc.dsc_length += diff;
						}
						else if (in_node->nod_desc.dsc_dtype == dtype_blob &&
							in_node->nod_desc.dsc_sub_type == isc_blob_text &&
							fromCharSet != CS_NONE && fromCharSet != CS_BINARY)
						{
							in_node->nod_desc.setTextType(toCharSet);
						}
					}
				}

				dsql_par* parameter = (dsql_par*) in_node->nod_arg[e_par_parameter];

				if (!parameter)
				{
					parameter = MAKE_parameter(statement->req_send, true, true,
											   (USHORT)(IPTR) in_node->nod_arg[e_par_index], NULL);
					in_node->nod_arg[e_par_parameter] = (dsql_nod*) parameter;
					in_node->nod_arg[e_par_index] = (dsql_nod*) (IPTR) parameter->par_index;
				}

				DEV_BLKCHK(parameter, dsql_type_par);
				parameter->par_desc = in_node->nod_desc;
				parameter->par_node = in_node;

				// Parameters should receive precisely the data that the user
				// passes in.  Therefore for text strings lets use varying
				// strings to insure that we don't add trailing blanks.

				// However, there are situations this leads to problems - so
				// we use the force_varchar parameter to prevent this
				// datatype assumption from occuring.

				if (force_varchar)
				{
					if (parameter->par_desc.dsc_dtype == dtype_text)
					{
						parameter->par_desc.dsc_dtype = dtype_varying;
						// The error msgs is inaccurate, but causing dsc_length
						// to be outsise range can be worse.
						if (parameter->par_desc.dsc_length > MAX_COLUMN_SIZE - sizeof(USHORT))
						{
							ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
									  //Arg::Gds(isc_dsql_datatype_err)
									  Arg::Gds(isc_imp_exc));
									  //Arg::Gds(isc_field_name) << Arg::Str(parameter->par_name)
						}

						parameter->par_desc.dsc_length += sizeof(USHORT);
					}
					else if (!parameter->par_desc.isText() && !parameter->par_desc.isBlob())
					{
						const USHORT toCharSetBPC = METD_get_charset_bpc(
							statement, att->att_charset);

						// The LIKE & similar parameters must be varchar type
						// strings - so force this parameter to be varchar
						// and take a guess at a good length for it.
						parameter->par_desc.dsc_dtype = dtype_varying;
						parameter->par_desc.dsc_length = LIKE_PARAM_LEN * toCharSetBPC +
							sizeof(USHORT);
						parameter->par_desc.dsc_sub_type = 0;
						parameter->par_desc.dsc_scale = 0;
						parameter->par_desc.setTextType(att->att_charset);
					}
				}
				return true;
			}

		case nod_cast:
			{
				dsql_nod* par_node = in_node->nod_arg[e_cast_source];
				dsql_fld* field = (dsql_fld*) in_node->nod_arg[e_cast_target];
				if (par_node->nod_type == nod_parameter) {
					dsql_par* parameter = (dsql_par*) par_node->nod_arg[e_par_parameter];
					if (parameter)
					{
						parameter->par_desc = par_node->nod_desc;
						parameter->par_node = par_node;
						MAKE_desc_from_field(&parameter->par_desc, field);
						return true;
					}
				}
				return false;
			}

		case nod_missing:
		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_substr:
		case nod_subtract:
		case nod_subtract2:
		case nod_trim:
		case nod_upcase:
		case nod_lowcase:
		case nod_extract:
		case nod_limit:
		case nod_rows:
		case nod_agg_list:
			{
				bool result = false;
				dsql_nod** ptr = in_node->nod_arg;
				for (const dsql_nod* const* const end = ptr + in_node->nod_count; ptr < end; ptr++)
				{
					result |= set_parameter_type(statement, *ptr, node, force_varchar);
				}
				return result;
			}

		case nod_hidden_var:
			return set_parameter_type(statement, in_node->nod_arg[e_hidden_var_expr], node,
									  force_varchar);

		default:
			return false;
	}
}


/**

 set_parameters_name

    @brief      Setup parameter parameters name.


    @param list_node
    @param rel_node

 **/
static void set_parameters_name( dsql_nod* list_node, const dsql_nod* rel_node)
{
	DEV_BLKCHK(list_node, dsql_type_nod);
	DEV_BLKCHK(rel_node, dsql_type_nod);

	const dsql_ctx* context = reinterpret_cast<dsql_ctx*>(rel_node->nod_arg[e_rel_context]);
	DEV_BLKCHK(context, dsql_type_ctx);
	const dsql_rel* relation = context->ctx_relation;

	dsql_nod** ptr = list_node->nod_arg;
	for (const dsql_nod* const* const end = ptr + list_node->nod_count; ptr < end; ptr++)
	{
		DEV_BLKCHK(*ptr, dsql_type_nod);
		if ((*ptr)->nod_type == nod_assign)
			set_parameter_name((*ptr)->nod_arg[e_asgn_value], (*ptr)->nod_arg[e_asgn_field], relation);
		else
			fb_assert(FALSE);
	}
}


/**

 set_parameter_name

    @brief      Setup parameter parameter name.
 	This function was added as a part of array data type
 	support for InterClient. It is	called when either
 	"insert" or "update" statements are parsed. If the
 	statements have input parameters, than the parameter
 	is assigned the name of the field it is being inserted
 	(or updated). The same goes to the name of a relation.
 	The names are assigned to the parameter only if the
 	field is of array data type.


    @param par_node
    @param fld_node
    @param relation

 **/
static void set_parameter_name( dsql_nod* par_node, const dsql_nod* fld_node, const dsql_rel* relation)
{
	DEV_BLKCHK(par_node, dsql_type_nod);
	DEV_BLKCHK(fld_node, dsql_type_nod);
	DEV_BLKCHK(relation, dsql_type_dsql_rel);

	if (!par_node)
		return;

	// Could it be something else ???
	fb_assert(fld_node->nod_type == nod_field);

	if (fld_node->nod_desc.dsc_dtype != dtype_array)
		return;

	switch (par_node->nod_type)
	{
	case nod_parameter:
		{
			dsql_par* parameter = (dsql_par*) par_node->nod_arg[e_par_parameter];
			DEV_BLKCHK(parameter, dsql_type_par);
			const dsql_fld* field = (dsql_fld*) fld_node->nod_arg[e_fld_field];
			DEV_BLKCHK(field, dsql_type_fld);
			parameter->par_name = field->fld_name.c_str();
			parameter->par_rel_name = relation->rel_name.c_str();
			return;
		}

	case nod_add:
	case nod_add2:
	case nod_concatenate:
	case nod_divide:
	case nod_divide2:
	case nod_multiply:
	case nod_multiply2:
	case nod_negate:
	case nod_substr:
	case nod_subtract:
	case nod_subtract2:
	case nod_trim:
	case nod_upcase:
	case nod_lowcase:
	case nod_extract:
	case nod_strlen:
	case nod_limit:
	case nod_rows:
		{
			dsql_nod** ptr = par_node->nod_arg;
			for (const dsql_nod* const* const end = ptr + par_node->nod_count; ptr < end; ptr++)
			{
				set_parameter_name(*ptr, fld_node, relation);
			}
			return;
		}

	case nod_hidden_var:
		set_parameter_name(par_node->nod_arg[e_hidden_var_expr], fld_node, relation);
		return;

	default:
		return;
	}
}


/**

 pass1_savepoint

    @brief      Add savepoint pair of nodes
				to statement having error handlers.


    @param statement
	@param node

 **/
static dsql_nod* pass1_savepoint(const CompiledStatement* statement, dsql_nod* node)
{
	if (statement->req_error_handlers) {
		dsql_nod* temp = MAKE_node(nod_list, 3);
		temp->nod_arg[0] = MAKE_node(nod_start_savepoint, 0);
		temp->nod_arg[1] = node;
		temp->nod_arg[2] = MAKE_node(nod_end_savepoint, 0);
		node = temp;
	}

	return node;
}


void CompiledStatement::addCTEs(dsql_nod* with)
{
	DEV_BLKCHK(with, dsql_type_nod);
	fb_assert(with->nod_type == nod_with);

	if (req_ctes.getCount()) {
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  // WITH clause can't be nested
				  Arg::Gds(isc_dsql_cte_nested_with));
	}

	if (with->nod_flags & NOD_UNION_RECURSIVE)
		req_flags |= REQ_CTE_recursive;

	const dsql_nod* list = with->nod_arg[0];
	const dsql_nod* const* end = list->nod_arg + list->nod_count;
	for (dsql_nod* const* cte = list->nod_arg; cte < end; cte++)
	{
		fb_assert((*cte)->nod_type == nod_derived_table);

		if (with->nod_flags & NOD_UNION_RECURSIVE) 
		{
			req_curr_ctes.push(*cte);
			PsqlChanger changer(this, false);
			req_ctes.add(pass1_recursive_cte(this, *cte));
			req_curr_ctes.pop();

			// Add CTE name into CTE aliases stack. It allows later to search for 
			// aliases of given CTE.
			const dsql_str* cte_name = (dsql_str*) (*cte)->nod_arg[e_derived_table_alias];
			addCTEAlias(cte_name);
		}
		else {
			req_ctes.add(*cte);
		}
	}
}


dsql_nod* CompiledStatement::findCTE(const dsql_str* name)
{
	for (size_t i = 0; i < req_ctes.getCount(); i++)
	{
		dsql_nod* cte = req_ctes[i];
		const dsql_str* cte_name = (dsql_str*) cte->nod_arg[e_derived_table_alias];

		if (name->str_length == cte_name->str_length &&
			strncmp(name->str_data, cte_name->str_data, cte_name->str_length) == 0)
		{
			return cte;
		}
	}

	return NULL;
}


void CompiledStatement::clearCTEs()
{
	req_flags &= ~REQ_CTE_recursive;
	req_ctes.clear();
	req_cte_aliases.clear();
}


void CompiledStatement::checkUnusedCTEs() const
{
	for (size_t i = 0; i < req_ctes.getCount(); i++)
	{
		const dsql_nod* cte = req_ctes[i];

		if (!(cte->nod_flags & NOD_DT_CTE_USED))
		{
			const dsql_str* cte_name = (dsql_str*) cte->nod_arg[e_derived_table_alias];

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_cte_not_used) << Arg::Str(cte_name->str_data));
		}
	}
}

// Returns false for hidden fields and true for non-hidden.
// For non-hidden, change "node" if the field is part of an
// implicit join.
bool dsql_ctx::getImplicitJoinField(const Firebird::MetaName& name, dsql_nod*& node)
{
	ImplicitJoin* impJoin;
	if (ctx_imp_join.get(name, impJoin))
	{
		if (impJoin->visibleInContext == this)
		{
			node = impJoin->value;
			return true;
		}

		return false;
	}

	return true;
}

#ifdef DSQL_DEBUG

#include <stdarg.h>

static void trace_line(const char* message, ...)
{
	char buffer[1024];
	va_list params;
	va_start(params, message);
	VSNPRINTF(buffer, sizeof(buffer), message, params);
	va_end(params);
	buffer[sizeof(buffer) - 1] = 0;
	gds__trace_raw(buffer);
}

/**

 	DSQL_pretty

    @brief	Pretty print a node tree.


    @param node
    @param column

 **/
void DSQL_pretty(const dsql_nod* node, int column)
{
	TEXT buffer[1024];

	TEXT* p = buffer;
	p += sprintf(p, "%p ", (void*) node);

	if (column) {
		USHORT l = column * 3;
		do {
			*p++ = ' ';
		} while (--l);
	}

	*p = 0;

	if (!node) {
		trace_line("%s *** null ***\n", buffer);
		return;
	}

	switch (node->getType())
	{
	case dsql_type_str:
		trace_line("%sSTRING: \"%s\"\n", buffer, ((dsql_str*) node)->str_data);
		return;

	case dsql_type_fld:
		trace_line("%sFIELD: %s\n", buffer, ((dsql_fld*) node)->fld_name.c_str());
		return;

	case dsql_type_sym:
		trace_line("%sSYMBOL: %s\n", buffer, ((dsql_sym*) node)->sym_string);
		return;

	case dsql_type_nod:
		break;

	default:
		trace_line("%sUNKNOWN BLOCK TYPE\n", buffer);
		return;
	}

	TEXT s[64];
	const dsql_str* string;
	Firebird::string verb;
	const dsql_nod* const* ptr = node->nod_arg;
	const dsql_nod* const* end = ptr + node->nod_count;
	Firebird::Array<dsql_nod*> subNodes;

	switch (node->nod_type)
	{
	case nod_abort:
		verb = "abort";
		break;
	case nod_agg_average:
		verb = "agg_average";
		break;
	case nod_agg_count:
		verb = "agg_count";
		break;
// count2
//	case nod_agg_distinct: verb = "agg_distinct";
//		break;
	case nod_agg_max:
		verb = "agg_max";
		break;
	case nod_agg_min:
		verb = "agg_min";
		break;
	case nod_agg_total:
		verb = "agg_total";
		break;
	case nod_agg_list:
		verb = "agg_list";
		break;
	case nod_add:
		verb = "add";
		break;
	case nod_alias:
		verb = "alias";
		break;
	case nod_ansi_all:
	case nod_all:
		verb = "all";
		break;
	case nod_and:
		verb = "and";
		break;
	case nod_ansi_any:
	case nod_any:
		verb = "any";
		break;
	case nod_array:
		verb = "array element";
		break;
	case nod_assign:
		verb = "assign";
		break;
	case nod_average:
		verb = "average";
		break;
	case nod_between:
		verb = "between";
		break;
	case nod_cast:
		verb = "cast";
		break;
	case nod_collate:
		verb = "collate";
		break;
	case nod_concatenate:
		verb = "concatenate";
		break;
	case nod_containing:
		verb = "containing";
		break;
	//case nod_count:
	//	verb = "count";
	//	break;
	case nod_current_date:
		verb = "current_date";
		break;
	case nod_current_time:
		verb = "current_time";
		break;
	case nod_current_timestamp:
		verb = "current_timestamp";
		break;
	case nod_cursor:
		verb = "cursor";
		break;
	case nod_dbkey:
		verb = "dbkey";
		break;
	case nod_rec_version:
		verb = "record_version";
		break;
	case nod_def_database:
		verb = "define database";
		break;
	case nod_def_field:
		verb = "define field";
		break;
	case nod_def_generator:
		verb = "define generator";
		break;
	case nod_def_filter:
		verb = "define filter";
		break;
	case nod_def_index:
		verb = "define index";
		break;
	case nod_def_relation:
		verb = "define relation";
		break;
	// CVC: New node redef_relation.
	case nod_redef_relation:
		verb = "redefine relation";
		break;
	case nod_def_view:
		verb = "define view";
		break;
	case nod_redef_view:
		verb = "redefine view";
		break;
	case nod_mod_view:
		verb = "modify view";
		break;
	case nod_replace_view:
		verb = "replace view";
		break;
	case nod_delete:
		verb = "delete";
		break;
	case nod_del_field:
		verb = "delete field";
		break;
	case nod_del_filter:
		verb = "delete filter";
		break;
	case nod_del_generator:
		verb = "delete generator";
		break;
	case nod_del_index:
		verb = "delete index";
		break;
	case nod_del_relation:
		verb = "delete relation";
		break;
	// CVC: New node del_view.
	case nod_del_view:
		verb = "delete view";
		break;
	case nod_def_procedure:
		verb = "define procedure";
		break;
	case nod_mod_procedure:
		verb = "modify procedure";
		break;
	case nod_replace_procedure:
		verb = "replace procedure";
		break;
	// CVC: New node redef_procedure.
	case nod_redef_procedure:
		verb = "redefine procedure";
		break;
	case nod_del_procedure:
		verb = "delete procedure";
		break;
	case nod_def_trigger:
		verb = "define trigger";
		break;
	case nod_mod_trigger:
		verb = "modify trigger";
		break;
	case nod_replace_trigger:
		verb = "replace trigger";
		break;
	case nod_redef_trigger:
		verb = "redefine trigger";
		break;
	case nod_del_trigger:
		verb = "delete trigger";
		break;
	case nod_divide:
		verb = "divide";
		break;
	case nod_eql_all:
	case nod_eql_any:
	case nod_eql:
		verb = "eql";
		break;
	case nod_equiv:
		verb = "equiv";
		break;
	case nod_erase:
		verb = "erase";
		break;
	case nod_execute:
		verb = "execute";
		break;
	case nod_exec_procedure:
		verb = "execute procedure";
		break;
	case nod_exec_block:
		verb = "execute block";
		break;
	case nod_exists:
		verb = "exists";
		break;
	case nod_extract:
		verb = "extract";
		break;
	case nod_flag:
		verb = "flag";
		break;
	case nod_foreign:
		verb = "foreign key";
		break;
	case nod_gen_id:
		verb = "gen_id";
		break;
	case nod_geq_all:
	case nod_geq_any:
	case nod_geq:
		verb = "geq";
		break;
	case nod_get_segment:
		verb = "get segment";
		break;
	case nod_grant:
		verb = "grant";
		break;
	case nod_gtr_all:
	case nod_gtr_any:
	case nod_gtr:
		verb = "gtr";
		break;
	case nod_insert:
		verb = "insert";
		break;
	case nod_internal_info:
		verb = "internal info";
		break;
	case nod_join:
		verb = "join";
		break;
	case nod_join_full:
		verb = "join_full";
		break;
	case nod_join_left:
		verb = "join_left";
		break;
	case nod_join_right:
		verb = "join_right";
		break;
	case nod_strlen:
		verb = "strlen";
		break;
	case nod_leq_all:
	case nod_leq_any:
	case nod_leq:
		verb = "leq";
		break;
	case nod_like:
		verb = "like";
		break;
	case nod_list:
		verb = "list";
		break;
	case nod_lss_all:
	case nod_lss_any:
	case nod_lss:
		verb = "lss";
		break;
	case nod_max:
		verb = "max";
		break;
	case nod_min:
		verb = "min";
		break;
	case nod_missing:
		verb = "missing";
		break;
	case nod_modify:
		verb = "modify";
		break;
	case nod_mod_database:
		verb = "modify database";
		break;
	case nod_mod_field:
		verb = "modify field";
		break;
	case nod_mod_relation:
		verb = "modify relation";
		break;
	case nod_multiply:
		verb = "multiply";
		break;
	case nod_negate:
		verb = "negate";
		break;
	case nod_neq_all:
	case nod_neq_any:
	case nod_neq:
		verb = "neq";
		break;
	case nod_not:
		verb = "not";
		break;
	case nod_null:
		verb = "null";
		break;
	case nod_or:
		verb = "or";
		break;
	case nod_order:
		verb = "order";
		break;
	case nod_primary:
		verb = "primary key";
		break;
	case nod_procedure_name:
		verb = "procedure name";
		break;
	case nod_put_segment:
		verb = "put segment";
		break;
	case nod_relation_name:
		verb = "relation name";
		break;
	case nod_rel_proc_name:
		verb = "rel/proc name";
		break;
	case nod_return:
		verb = "return";
		ptr = end;
		break;
	case nod_revoke:
		verb = "revoke";
		break;
	case nod_rse:
		verb = "rse";
		break;
	case nod_select:
		verb = "select";
		break;
	case nod_select_expr:
		verb = "select expr";
		break;
	case nod_similar:
		verb = "similar";
		break;
	case nod_starting:
		verb = "starting";
		break;
	case nod_store:
		verb = "store";
		break;
	case nod_substr:
		verb = "substr";
		break;
	case nod_subtract:
		verb = "subtract";
		break;
	case nod_trim:
		verb = "trim";
		break;
	case nod_total:
		verb = "total";
		break;
	case nod_update:
		verb = "update";
		break;
	case nod_union:
		verb = "union";
		break;
	case nod_unique:
		verb = "unique";
		break;
	case nod_upcase:
		verb = "upcase";
		break;
	case nod_lowcase:
		verb = "lowcase";
		break;
	case nod_singular:
		verb = "singular";
		break;
	case nod_user_name:
		verb = "user_name";
		break;
	// CVC: New node current_role.
	case nod_current_role:
		verb = "current_role";
		break;
	case nod_via:
		verb = "via";
		break;

	case nod_coalesce:
		verb = "coalesce";
		break;

	case nod_simple_case:
		verb = "simple_case";
		break;

	case nod_searched_case:
		verb = "searched_case";
		break;

	case nod_add2:
		verb = "add2";
		break;
	case nod_agg_total2:
		verb = "agg_total2";
		break;
	case nod_divide2:
		verb = "divide2";
		break;
	case nod_gen_id2:
		verb = "gen_id2";
		break;
	case nod_multiply2:
		verb = "multiply2";
		break;
	case nod_subtract2:
		verb = "subtract2";
		break;
	case nod_limit:
		verb = "limit";
		break;
	case nod_rows:
		verb = "rows";
		break;
	// IOL: missing node types
	case nod_on_error:
		verb = "on error";
		break;
	case nod_block:
		verb = "block";
		break;
	case nod_default:
		verb = "default";
		break;
	case nod_plan_expr:
		verb = "plan";
		break;
	case nod_index:
		verb = "index";
		break;
	case nod_index_order:
		verb = "order";
		break;
	case nod_plan_item:
		verb = "item";
		break;
	case nod_natural:
		verb = "natural";
		break;
	case nod_join_inner:
		verb = "join_inner";
		break;
	// SKIDDER: some more missing node types
	case nod_commit:
		verb = "commit";
		break;
	case nod_rollback:
		verb = "rollback";
		break;
	case nod_trans:
		verb = "trans";
		break;
	case nod_def_default:
		verb = "def_default";
		break;
	case nod_del_default:
		verb = "del_default";
		break;
	case nod_def_domain:
		verb = "def_domain";
		break;
	case nod_mod_domain:
		verb = "mod_domain";
		break;
	case nod_del_domain:
		verb = "del_domain";
		break;
	case nod_def_constraint:
		verb = "def_constraint";
		break;
/*
	case nod_def_trigger_msg:
		verb = "def_trigger_msg";
		break;
	case nod_mod_trigger_msg:
		verb = "mod_trigger_msg";
		break;
	case nod_del_trigger_msg:
		verb = "del_trigger_msg";
		break;
*/
	case nod_def_exception:
		verb = "def_exception";
		break;
	case nod_redef_exception:
		verb = "redef_exception";
		break;
	case nod_mod_exception:
		verb = "mod_exception";
		break;
	case nod_replace_exception:
		verb = "replace_exception";
		break;
	case nod_del_exception:
		verb = "del_exception";
		break;
	case nod_def_shadow:
		verb = "def_shadow";
		break;
	case nod_del_shadow:
		verb = "del_shadow";
		break;
	case nod_def_udf:
		verb = "def_udf";
		break;
	case nod_del_udf:
		verb = "del_udf";
		break;
	case nod_rel_constraint:
		verb = "rel_constraint";
		break;
	case nod_delete_rel_constraint:
		verb = "delete_rel_constraint";
		break;
	case nod_references:
		verb = "references";
		break;
	case nod_proc_obj:
		verb = "proc_obj";
		break;
	case nod_trig_obj:
		verb = "trig_obj";
		break;
	case nod_view_obj:
		verb = "view_obj";
		break;
	case nod_exit:
		verb = "exit";
		break;
	case nod_if:
		verb = "if";
		break;
	case nod_erase_current:
		verb = "erase_current";
		break;
	case nod_modify_current:
		verb = "modify_current";
		break;
	case nod_post:
		verb = "post";
		break;
	case nod_sqlcode:
		verb = "sqlcode";
		break;
	case nod_gdscode:
		verb = "gdscode";
		break;
	case nod_exception:
		verb = "exception";
		break;
	case nod_exception_stmt:
		verb = "exception_stmt";
		break;
	case nod_start_savepoint:
		verb = "start_savepoint";
		break;
	case nod_end_savepoint:
		verb = "end_savepoint";
		break;
	case nod_dom_value:
		verb = "dom_value";
		break;
	case nod_user_group:
		verb = "user_group";
		break;
	case nod_from:
		verb = "from";
		break;
	case nod_agg_average2:
		verb = "agg_average2";
		break;
	case nod_access:
		verb = "access";
		break;
	case nod_wait:
		verb = "wait";
		break;
	case nod_isolation:
		verb = "isolation";
		break;
	case nod_version:
		verb = "version";
		break;
	case nod_table_lock:
		verb = "table_lock";
		break;
	case nod_lock_mode:
		verb = "lock_mode";
		break;
	case nod_reserve:
		verb = "reserve";
		break;
	case nod_retain:
		verb = "retain";
		break;
	case nod_page_size:
		verb = "page_size";
		break;
	case nod_file_length:
		verb = "file_length";
		break;
	case nod_file_desc:
		verb = "file_desc";
		break;
	case nod_dfl_charset:
		verb = "dfl_charset";
		break;
	case nod_password:
		verb = "password";
		break;
	case nod_lc_ctype:
		verb = "lc_ctype";
		break;
	case nod_udf_return_value:
		verb = "udf_return_value";
		break;
	case nod_def_computed:
		verb = "def_computed";
		break;
	case nod_merge_plan:
		verb = "merge_plan";
		break;
	case nod_set_generator:
		verb = "set_generator";
		break;
	case nod_set_generator2:
		verb = "set_generator2";
		break;
	case nod_mod_index:
		verb = "mod_index";
		break;
	case nod_idx_active:
		verb = "idx_active";
		break;
	case nod_idx_inactive:
		verb = "idx_inactive";
		break;
	case nod_restrict:
		verb = "restrict";
		break;
	case nod_cascade:
		verb = "cascade";
		break;
	case nod_set_statistics:
		verb = "set_statistics";
		break;
	case nod_ref_upd_del:
		verb = "ref_upd_del";
		break;
	case nod_ref_trig_action:
		verb = "ref_trig_action";
		break;
	case nod_def_role:
		verb = "def_role";
		break;
	case nod_role_name:
		verb = "role_name";
		break;
	case nod_grant_admin:
		verb = "grant_admin";
		break;
	case nod_del_role:
		verb = "del_role";
		break;
	case nod_mod_domain_type:
		verb = "mod_domain_type";
		break;
	case nod_mod_field_name:
		verb = "mod_field_name";
		break;
	case nod_mod_field_type:
		verb = "mod_field_type";
		break;
	case nod_mod_field_pos:
		verb = "mod_field_pos";
		break;
	case nod_udf_param:
		verb = "udf_param";
		break;
	case nod_exec_sql:
		verb = "exec_sql";
		break;
	case nod_for_update:
		verb = "for_update";
		break;
	case nod_user_savepoint:
		verb = "user_savepoint";
		break;
	case nod_release_savepoint:
		verb = "release_savepoint";
		break;
	case nod_undo_savepoint:
		verb = "undo_savepoint";
		break;
	case nod_difference_file:
		verb = "difference_file";
		break;
	case nod_drop_difference:
		verb = "drop_difference";
		break;
	case nod_begin_backup:
		verb = "begin_backup";
		break;
	case nod_end_backup:
		verb = "end_backup";
		break;
	case nod_derived_table:
		verb = "derived_table";
		break;

	// CVC: This node seems obsolete.
	case nod_exec_into:
		verb = "exec_into";
		break;

	case nod_breakleave:
		verb = "breakleave";
		break;

	case nod_for_select:
		verb = "for_select";
		break;

	case nod_while:
		verb = "while";
		break;

	case nod_label:
		verb = "label";
		DSQL_pretty(node->nod_arg[e_label_name], column + 1);
		trace_line("%s   number %d\n", buffer,
			(int)(IPTR)node->nod_arg[e_label_number]);
		return;

	case nod_derived_field:
		verb = "derived_field";
		trace_line("%s%s\n", buffer, verb.c_str());
		DSQL_pretty(node->nod_arg[e_derived_field_value], column + 1);
		DSQL_pretty(node->nod_arg[e_derived_field_name], column + 1);
		trace_line("%s   scope %d\n", buffer, (USHORT)(U_IPTR)node->nod_arg[e_derived_field_scope]);
		return;

	case nod_aggregate:
		{
			verb = "aggregate";
			trace_line("%s%s\n", buffer, verb.c_str());
			const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_agg_context];
			trace_line("%s   context %d\n", buffer, context->ctx_context);
			dsql_map* map = context->ctx_map;
			if (map != NULL)
				trace_line("%s   map\n", buffer);
			while (map) {
				trace_line("%s      position %d\n", buffer, map->map_position);
				DSQL_pretty(map->map_node, column + 2);
				map = map->map_next;
			}
			DSQL_pretty(node->nod_arg[e_agg_group], column + 1);
			DSQL_pretty(node->nod_arg[e_agg_rse], column + 1);
			return;
		}

	case nod_constant:
		verb = "constant";
		if (node->nod_desc.dsc_address) {
			if (node->nod_desc.dsc_dtype == dtype_text)
				sprintf(s, "constant \"%s\"", node->nod_desc.dsc_address);
			else
				sprintf(s, "constant %"SLONGFORMAT, node->getSlong());
			verb = s;
		}
		break;

	case nod_field:
		{
			const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_fld_context];
			const dsql_rel* relation = context->ctx_relation;
			const dsql_prc* procedure = context->ctx_procedure;
			const dsql_fld* field = (dsql_fld*) node->nod_arg[e_fld_field];
			trace_line("%sfield %s.%s, context %d\n", buffer,
				(relation != NULL ?
					relation->rel_name.c_str() :
					(procedure != NULL ? procedure->prc_name.c_str() : "unknown_db_object")),
				field->fld_name.c_str(), context->ctx_context);
			return;
		}

	case nod_field_name:
		trace_line("%sfield name: \"", buffer);
		string = (dsql_str*) node->nod_arg[e_fln_context];
		if (string)
			trace_line("%s.", string->str_data);
		string = (dsql_str*) node->nod_arg[e_fln_name];
		if (string != 0) {
			trace_line("%s\"\n", string->str_data);
		}
		else {
			trace_line("%s\"\n", "*");
		}
		return;

	case nod_map:
		{
			verb = "map";
			trace_line("%s%s\n", buffer, verb.c_str());
			const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_map_context];
			trace_line("%s   context %d\n", buffer, context->ctx_context);
			for (const dsql_map* map = (dsql_map*) node->nod_arg[e_map_map]; map; map = map->map_next)
			{
				trace_line("%s   position %d\n", buffer, map->map_position);
				DSQL_pretty(map->map_node, column + 1);
			}
			return;
		}

	case nod_relation:
		{
			const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_rel_context];
			const dsql_rel* relation = context->ctx_relation;
			const dsql_prc* procedure = context->ctx_procedure;
			if ( relation != NULL ) {
				trace_line("%srelation %s, context %d\n",
					buffer, relation->rel_name.c_str(), context->ctx_context);
			}
			else if ( procedure != NULL ) {
				trace_line("%sprocedure %s, context %d\n",
					buffer, procedure->prc_name.c_str(), context->ctx_context);
			}
			else {
				trace_line("%sUNKNOWN DB OBJECT, context %d\n",
					buffer, context->ctx_context);
			}
			return;
		}

	case nod_variable:
		{
			const dsql_var* variable = (dsql_var*) node->nod_arg[e_var_variable];
			// Adding variable->var_variable_number to display, obviously something
			// is missing from the printf, and Im assuming this was it.
			// (anyway can't be worse than it was MOD 05-July-2002.
			trace_line("%svariable %s %d\n", buffer, variable->var_name, variable->var_variable_number);
			return;
		}

	case nod_var_name:
		trace_line("%svariable name: \"", buffer);
		string = (dsql_str*) node->nod_arg[e_vrn_name];
		trace_line("%s\"\n", string->str_data);
		return;

	case nod_parameter:
		if (node->nod_column) {
			trace_line("%sparameter: %d\n",	buffer, (USHORT)(IPTR)node->nod_arg[e_par_index]);
		}
		else {
			const dsql_par* param = (dsql_par*) node->nod_arg[e_par_parameter];
			trace_line("%sparameter: %d\n",	buffer, param->par_index);
		}
		return;

	case nod_udf:
		trace_line ("%sfunction: \"", buffer);
		// nmcc: how are we supposed to tell which type of nod_udf this is ??
		// CVC: The answer is that nod_arg[0] can be either the udf name or the
		// pointer to udf struct returned by METD_get_function, so we should resort
		// to the block type. The replacement happens in pass1_udf().
		switch (node->nod_arg[e_udf_name]->getType())
		{
		case dsql_type_udf:
			trace_line ("%s\"\n", ((dsql_udf*) node->nod_arg[e_udf_name])->udf_name.c_str());
			break;
		case dsql_type_str:
			string = (dsql_str*) node->nod_arg[e_udf_name];
			trace_line ("%s\"\n", string->str_data);
			break;
		default:
			trace_line ("%s\"\n", "<ERROR>");
			break;
		}
		ptr++;

		if (node->nod_count == 2) {
			DSQL_pretty (*ptr, column + 1);
		}
		return;

	case nod_cursor_open:
		verb = "cursor_open";
		break;
	case nod_cursor_fetch:
		verb = "cursor_fetch";
		break;
	case nod_cursor_close:
		verb = "cursor_close";
		break;
	case nod_fetch_seek:
		verb = "fetch_seek";
		break;

	case nod_param_val:
		verb = "param_val"; // do we need more here?
		break;

	case nod_query_spec:
		verb = "query_spec";
		break;

	case nod_comment:
		verb = "comment";
		break;

	case nod_mod_udf:
		verb = "mod_udf";
		break;

	case nod_def_collation:
		verb = "def_collation";
		break;

	case nod_del_collation:
		verb = "del_collation";
		break;

	case nod_collation_from:
		verb = "collation_from";
		break;

	case nod_collation_from_external:
		verb = "collation_from_external";
		break;

	case nod_collation_attr:
		verb = "collation_attr";
		break;

	case nod_collation_specific_attr:
		verb = "collation_specific_attr";
		break;

	case nod_returning:
		verb = "returning";
		break;

	case nod_tra_misc:
		verb = "tra_misc";
		break;

	case nod_lock_timeout:
		verb = "lock_timeout"; // maybe show the timeout value?
		break;

	case nod_src_info:
		{
			const int line = (int) (IPTR) (*ptr++);
			const int col = (int) (IPTR) (*ptr++);
			sprintf(s, "src_info: line %d, col %d", line, col);
		}
		verb = s;
		break;

	case nod_with:
		verb = "with";
		break;

	case nod_update_or_insert:
		verb = "update_or_insert";
		break;

	case nod_merge:
		verb = "merge";
		break;

	case nod_merge_when:
		verb = "merge_when";
		break;

	case nod_merge_update:
		verb = "merge_update";
		break;

	case nod_merge_insert:
		verb = "merge_insert";
		break;

	case nod_sys_function:
		trace_line("%ssystem function: \"", buffer);
		string = (dsql_str*) node->nod_arg[e_sysfunc_name];
		trace_line("%s\"\n", string->str_data);
		ptr++;

		if (node->nod_count == 2)
			DSQL_pretty(*ptr, column + 1);
		return;

	case nod_mod_role:
		verb = "mod_role";
		break;

	case nod_add_user:
		verb = "add_user";
		break;

	case nod_mod_user:
		verb = "mod_user";
		break;

	case nod_del_user:
		verb = "del_user";
		break;

	case nod_exec_stmt:
		verb = "exec_stmt";
		break;

	case nod_class_node:
		reinterpret_cast<Node*>(node->nod_arg[0])->print(verb, subNodes);
		ptr = subNodes.begin();
		end = subNodes.end();
		break;

	case nod_hidden_var:
		verb = "hidden_var";
		break;

	default:
		sprintf(s, "unknown type %d", node->nod_type);
		verb = s;
	}

	if (node->nod_desc.dsc_dtype)
	{
		trace_line("%s%s (%d,%d,%p)\n",
				buffer, verb.c_str(),
				node->nod_desc.dsc_dtype, node->nod_desc.dsc_length, node->nod_desc.dsc_address);
	}
	else
		trace_line("%s%s\n", buffer, verb.c_str());

	++column;

	while (ptr < end)
		DSQL_pretty(*ptr++, column);

	return;
}
#endif
