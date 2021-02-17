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
 *   and PASS1_put_args_on_stack
 *
 * 2002.08.04 Arno Brinkman: Added ignore_cast as parameter to PASS1_node_match,
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
 *   Modified functions pass1_field, pass1_rse, copy_field, PASS1_sort.
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
#include "../dsql/Nodes.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../jrd/intl.h"
#include "../jrd/blr.h"
#include "../jrd/jrd.h"
#include "../jrd/constants.h"
#include "../jrd/intl_classes.h"
#include "../jrd/RecordSourceNodes.h"
#include "../dsql/DdlNodes.h"
#include "../dsql/StmtNodes.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/pass1_proto.h"
#include "../dsql/utld_proto.h"
#include "../dsql/DSqlDataTypeUtil.h"
#include "../common/dsc_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/thread_proto.h"
#include "../yvalve/why_proto.h"
#include "../jrd/SysFunction.h"
#include "../common/classes/array.h"
#include "../common/classes/auto.h"
#include "../common/utils_proto.h"
#include "../common/config/config.h"
#include "../common/StatusArg.h"

using namespace Jrd;
using namespace Firebird;


static string pass1_alias_concat(const string&, const string&);
static void pass1_expand_contexts(DsqlContextStack& contexts, dsql_ctx* context);
static ValueListNode* pass1_expand_select_list(DsqlCompilerScratch*, ValueListNode*, RecSourceListNode*);
static ValueListNode* pass1_group_by_list(DsqlCompilerScratch*, ValueListNode*, ValueListNode*);
static ValueExprNode* pass1_make_derived_field(thread_db*, DsqlCompilerScratch*, ValueExprNode*);
static RseNode* pass1_rse(DsqlCompilerScratch*, RecordSourceNode*, ValueListNode*, RowsClause*, bool, USHORT);
static RseNode* pass1_rse_impl(DsqlCompilerScratch*, RecordSourceNode*, ValueListNode*, RowsClause*, bool, USHORT);
static ValueListNode* pass1_sel_list(DsqlCompilerScratch*, ValueListNode*);
static RseNode* pass1_union(DsqlCompilerScratch*, UnionSourceNode*, ValueListNode*, RowsClause*, bool, USHORT);
static void pass1_union_auto_cast(DsqlCompilerScratch*, ExprNode*, const dsc&, size_t position);
static void remap_streams_to_parent_context(ExprNode*, dsql_ctx*);


AggregateFinder::AggregateFinder(DsqlCompilerScratch* aDsqlScratch, bool aWindow)
	: dsqlScratch(aDsqlScratch),
	  window(aWindow),
	  currentLevel(dsqlScratch->scopeLevel),
	  deepestLevel(0),
	  ignoreSubSelects(false)
{
}

bool AggregateFinder::find(DsqlCompilerScratch* dsqlScratch, bool window, ExprNode* node)
{
	AggregateFinder visitor(dsqlScratch, window);
	return visitor.visit(node);
}

bool AggregateFinder::visit(ExprNode* node)
{
	return node && node->dsqlAggregateFinder(*this);
}


Aggregate2Finder::Aggregate2Finder(USHORT aCheckScopeLevel, FieldMatchType aMatchType, bool aWindowOnly)
	: checkScopeLevel(aCheckScopeLevel),
	  matchType(aMatchType),
	  windowOnly(aWindowOnly),
	  currentScopeLevelEqual(true)
{
}

bool Aggregate2Finder::find(USHORT checkScopeLevel, FieldMatchType matchType, bool windowOnly,
	ExprNode* node)
{
	Aggregate2Finder visitor(checkScopeLevel, matchType, windowOnly);
	return visitor.visit(node);
}

bool Aggregate2Finder::visit(ExprNode* node)
{
	return node && node->dsqlAggregate2Finder(*this);
}


FieldFinder::FieldFinder(USHORT aCheckScopeLevel, FieldMatchType aMatchType)
	: checkScopeLevel(aCheckScopeLevel),
	  matchType(aMatchType),
	  field(false)
{
}

bool FieldFinder::find(USHORT checkScopeLevel, FieldMatchType matchType, ExprNode* node)
{
	FieldFinder visitor(checkScopeLevel, matchType);
	return visitor.visit(node);
}

bool FieldFinder::visit(ExprNode* node)
{
	return node && node->dsqlFieldFinder(*this);
}


InvalidReferenceFinder::InvalidReferenceFinder(const dsql_ctx* aContext, const ValueListNode* aList)
	: context(aContext),
	  list(aList),
	  insideOwnMap(false),
	  insideHigherMap(false)
{
	DEV_BLKCHK(list, dsql_type_nod);
}

bool InvalidReferenceFinder::find(const dsql_ctx* context, const ValueListNode* list, ExprNode* node)
{
	InvalidReferenceFinder visitor(context, list);
	return visitor.visit(node);
}

bool InvalidReferenceFinder::visit(ExprNode* node)
{
	DEV_BLKCHK(node, dsql_type_nod);

	if (!node)
		return false;

	bool invalid = false;

	// ASF: What we do in this function is the verification of all fields/dbkeys (or any parent
	// expression involving them) are present in the passed node list.
	// That makes valid:
	//   select n + 0 from table group by n			=> The n item is present in the list
	//   select n + 0 from table group by n + 0		=> The n + 0 item is present in the list
	// And makes invalid:
	//   select n + 1 from table group by n + 0		=> The n + 1 item is not present in the list

	if (list)
	{
		// Check if this node (with ignoring of CASTs) appears also
		// in the list of group by. If yes then it's allowed
		const NestConst<ValueExprNode>* ptr = list->items.begin();
		for (const NestConst<ValueExprNode>* const end = list->items.end(); ptr != end; ++ptr)
		{
			if (PASS1_node_match(node, *ptr, true))
				return false;
		}
	}

	return node->dsqlInvalidReferenceFinder(*this);
}


FieldRemapper::FieldRemapper(DsqlCompilerScratch* aDsqlScratch, dsql_ctx* aContext, bool aWindow,
			ValueListNode* aPartitionNode, ValueListNode* aOrderNode)
	: dsqlScratch(aDsqlScratch),
	  context(aContext),
	  window(aWindow),
	  partitionNode(aPartitionNode),
	  orderNode(aOrderNode),
	  currentLevel(dsqlScratch->scopeLevel)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);
	DEV_BLKCHK(context, dsql_type_ctx);
}

ExprNode* FieldRemapper::visit(ExprNode* node)
{
	return node ? node->dsqlFieldRemapper(*this) : NULL;
}


bool SubSelectFinder::visit(ExprNode* node)
{
	return node && node->dsqlSubSelectFinder(*this);
}


/**

 	PASS1_make_context

    @brief	Generate a context for a dsqlScratch.


    @param dsqlScratch
    @param relationNode

 **/
dsql_ctx* PASS1_make_context(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* relationNode)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	thread_db* const tdbb = JRD_get_thread_data();

	dsql_rel* relation = NULL;
	dsql_prc* procedure = NULL;

	// figure out whether this is a relation or a procedure
	// and give an error if it is neither

	MetaName relation_name;
	ProcedureSourceNode* procNode = NULL;
	RelationSourceNode* relNode = NULL;
	SelectExprNode* selNode = NULL;

	if ((procNode = relationNode->as<ProcedureSourceNode>()))
		relation_name = procNode->dsqlName.identifier;
	else if ((relNode = relationNode->as<RelationSourceNode>()))
		relation_name = relNode->dsqlName;
	else if ((selNode = relationNode->as<SelectExprNode>()))
		relation_name = selNode->alias.c_str();

	SelectExprNode* cte = NULL;

	if (selNode)
	{
		// No processing needed here for derived tables.
	}
	else if (procNode && (procNode->dsqlName.package.hasData() || procNode->sourceList))
	{
		if (procNode->dsqlName.package.isEmpty())
			procedure = dsqlScratch->getSubProcedure(procNode->dsqlName.identifier);

		if (!procedure)
		{
			procedure = METD_get_procedure(dsqlScratch->getTransaction(), dsqlScratch,
				procNode->dsqlName);
		}

		if (!procedure)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_dsql_procedure_err) <<
					  Arg::Gds(isc_random) <<
					  Arg::Str(procNode->dsqlName.toString()) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relationNode->line) <<
					  									   Arg::Num(relationNode->column));
		}
	}
	else if ((cte = dsqlScratch->findCTE(relation_name)))
		relationNode = cte;
	else
	{
		if (procNode && procNode->dsqlName.package.isEmpty())
			procedure = dsqlScratch->getSubProcedure(procNode->dsqlName.identifier);

		if (!procedure)
			relation = METD_get_relation(dsqlScratch->getTransaction(), dsqlScratch, relation_name);

		if (!relation && !procedure && procNode)
		{
			procedure = METD_get_procedure(dsqlScratch->getTransaction(),
				dsqlScratch, procNode->dsqlName);
		}

		if (!relation && !procedure)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_dsql_relation_err) <<
					  Arg::Gds(isc_random) << Arg::Str(relation_name) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relationNode->line) <<
					  									   Arg::Num(relationNode->column));
		}
	}

	if (procedure && !procedure->prc_out_count)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-84) <<
				  Arg::Gds(isc_dsql_procedure_use_err) << Arg::Str(procedure->prc_name.toString()) <<
				  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(relationNode->line) <<
				  									   Arg::Num(relationNode->column));
	}

	// Set up context block.
	dsql_ctx* context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
	context->ctx_relation = relation;
	context->ctx_procedure = procedure;

	if (selNode)
		context->ctx_context = USHORT(MAX_UCHAR) + 1 + dsqlScratch->derivedContextNumber++;
	else
		context->ctx_context = dsqlScratch->contextNumber++;

	context->ctx_scope_level = dsqlScratch->scopeLevel;

	// When we're in a outer-join part mark context for it.
	if (dsqlScratch->inOuterJoin)
		context->ctx_flags |= CTX_outer_join;

	context->ctx_in_outer_join = dsqlScratch->inOuterJoin;

	// find the context alias name, if it exists.
	string str;

	if ((procNode = relationNode->as<ProcedureSourceNode>()))
		str = procNode->alias;
	else if ((relNode = relationNode->as<RelationSourceNode>()))
		str = relNode->alias;
	else if ((selNode = relationNode->as<SelectExprNode>()))
	{
		str = selNode->alias;
		// ASF: In the case of a UNION contained in a CTE, selNode->querySpec will be a
		// UnionSourceNode instead of a RseNode. Since ctx_rse is a RseNode and is always accessed
		// as one, I'll leave this assignment here. It will be set in PASS1_derived_table anyway.
		///context->ctx_rse = selNode->querySpec;
	}

	if (str.hasData())
		context->ctx_internal_alias = str;

	if (dsqlScratch->aliasRelationPrefix.hasData() && !selNode)
	{
		if (str.hasData())
			str = pass1_alias_concat(dsqlScratch->aliasRelationPrefix, str);
		else
			str = pass1_alias_concat(dsqlScratch->aliasRelationPrefix, relation_name.c_str());
	}

	if (str.hasData())
	{
		context->ctx_alias = str;

		// check to make sure the context is not already used at this same
		// query level (if there are no subqueries, this checks that the
		// alias is not used twice in the dsqlScratch).
		for (DsqlContextStack::iterator stack(*dsqlScratch->context); stack.hasData(); ++stack)
		{
			const dsql_ctx* conflict = stack.object();

			if (conflict->ctx_scope_level != context->ctx_scope_level)
				continue;

			const TEXT* conflict_name;
			ISC_STATUS error_code;

			if (conflict->ctx_alias.hasData())
			{
				conflict_name = conflict->ctx_alias.c_str();
				error_code = isc_alias_conflict_err;
				// alias %s conflicts with an alias in the same dsqlScratch.
			}
			else if (conflict->ctx_procedure)
			{
				conflict_name = conflict->ctx_procedure->prc_name.identifier.c_str();
				error_code = isc_procedure_conflict_error;
				// alias %s conflicts with a procedure in the same dsqlScratch.
			}
			else if (conflict->ctx_relation)
			{
				conflict_name = conflict->ctx_relation->rel_name.c_str();
				error_code = isc_relation_conflict_err;
				// alias %s conflicts with a relation in the same dsqlScratch.
			}
			else
				continue;

			if (context->ctx_alias == conflict_name)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(error_code) << Arg::Str(conflict_name));
			}
		}
	}

	if (procedure)
	{
		USHORT count = 0;

		if (procNode->sourceList)
		{
			context->ctx_proc_inputs = Node::doDsqlPass(dsqlScratch, procNode->sourceList, false);
			count = context->ctx_proc_inputs->items.getCount();
		}

		if (count > procedure->prc_in_count ||
			count < procedure->prc_in_count - procedure->prc_def_count)
		{
			ERRD_post(Arg::Gds(isc_prcmismat) << Arg::Str(procNode->dsqlName.toString()));
		}

		if (count)
		{
			// Initialize this stack variable, and make it look like a node
			dsc desc_node;
			ValueListNode* inputList = context->ctx_proc_inputs;
			NestConst<ValueExprNode>* input = inputList->items.begin();

			for (dsql_fld* field = procedure->prc_inputs;
				 input != inputList->items.end();
				 ++input, field = field->fld_next)
			{
				DEV_BLKCHK(field, dsql_type_fld);
				MAKE_desc_from_field(&desc_node, field);
				PASS1_set_parameter_type(dsqlScratch, *input, &desc_node, false);
			}
		}
	}

	// push the context onto the dsqlScratch context stack
	// for matching fields against

	dsqlScratch->context->push(context);

	return context;
}


// Compile a record selection expression, bumping up the statement scope level everytime an rse is
// seen. The scope level controls parsing of aliases.
RseNode* PASS1_rse(DsqlCompilerScratch* dsqlScratch, SelectExprNode* input, bool updateLock)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);

	dsqlScratch->scopeLevel++;
	RseNode* node = pass1_rse(dsqlScratch, input, NULL, NULL, updateLock, 0);
	dsqlScratch->scopeLevel--;

	return node;
}


// Check for ambiguity in a field reference. The list with contexts where the field was found is
// checked and the necessary message is build from it.
void PASS1_ambiguity_check(DsqlCompilerScratch* dsqlScratch,
	const MetaName& name, const DsqlContextStack& ambiguous_contexts)
{
	// If there are no relations or only 1 there's no ambiguity, thus return.
	if (ambiguous_contexts.getCount() < 2)
		return;

	TEXT buffer[1024];
	USHORT loop = 0;

	buffer[0] = 0;
	TEXT* b = buffer;
	TEXT* p = NULL;

	for (DsqlContextStack::const_iterator stack(ambiguous_contexts); stack.hasData(); ++stack)
	{
		const dsql_ctx* context = stack.object();
		const dsql_rel* relation = context->ctx_relation;
		const dsql_prc* procedure = context->ctx_procedure;
		if (strlen(b) > (sizeof(buffer) - 50))
		{
			// Buffer full
			break;
		}
		// if this is the second loop add "and " before relation.
		if (++loop > 2)
			strcat(buffer, "and ");
		// Process relation when present.
		if (relation)
		{
			if (!(relation->rel_flags & REL_view))
				strcat(buffer, "table ");
			else
				strcat(buffer, "view ");
			strcat(buffer, relation->rel_name.c_str());
		}
		else if (procedure)
		{
			// Process procedure when present.
			strcat(b, "procedure ");
			strcat(b, procedure->prc_name.toString().c_str());
		}
		else
		{
			// When there's no relation and no procedure it's a derived table.
			strcat(b, "derived table ");
			if (context->ctx_alias.hasData())
				strcat(b, context->ctx_alias.c_str());
		}
		strcat(buffer, " ");
		if (!p)
			p = b + strlen(b);
	}

	if (p)
		*--p = 0;

	if (dsqlScratch->clientDialect >= SQL_DIALECT_V6)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
				  Arg::Gds(isc_dsql_ambiguous_field_name) << Arg::Str(buffer) << Arg::Str(++p) <<
				  Arg::Gds(isc_random) << name);
	}

	ERRD_post_warning(Arg::Warning(isc_sqlwarn) << Arg::Num(204) <<
					  Arg::Warning(isc_dsql_ambiguous_field_name) << Arg::Str(buffer) <<
																	 Arg::Str(++p) <<
					  Arg::Warning(isc_random) << name);
}


/**
	PASS1_check_unique_fields_names

	check fields (params, variables, cursors etc) names against
	sorted array
	if success, add them into array
 **/
void PASS1_check_unique_fields_names(StrArray& names, const CompoundStmtNode* fields)
{
	if (!fields)
		return;

	const NestConst<StmtNode>* ptr = fields->statements.begin();
	const NestConst<StmtNode>* const end = fields->statements.end();

	for (; ptr != end; ++ptr)
	{
		const char* name = NULL;

		const DeclareVariableNode* varNode;
		const DeclareCursorNode* cursorNode;

		if ((varNode = (*ptr)->as<DeclareVariableNode>()))
			name = varNode->dsqlDef->name.c_str();
		else if ((cursorNode = (*ptr)->as<DeclareCursorNode>()))
			name = cursorNode->dsqlName.c_str();
		else if ((*ptr)->as<DeclareSubProcNode>() || (*ptr)->as<DeclareSubFuncNode>())
			continue;

		fb_assert(name);

		size_t pos;
		if (!names.find(name, pos))
			names.insert(pos, name);
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
					  Arg::Gds(isc_dsql_duplicate_spec) << Arg::Str(name));
		}
	}
}


// Compose two booleans.
BoolExprNode* PASS1_compose(BoolExprNode* expr1, BoolExprNode* expr2, UCHAR blrOp)
{
	thread_db* tdbb = JRD_get_thread_data();

	fb_assert(blrOp == blr_and || blrOp == blr_or);

	if (!expr1)
		return expr2;

	if (!expr2)
		return expr1;

	return FB_NEW(*tdbb->getDefaultPool()) BinaryBoolNode(
		*tdbb->getDefaultPool(), blrOp, expr1, expr2);
}


// Report a field parsing recognition error.
void PASS1_field_unknown(const TEXT* qualifier_name, const TEXT* field_name,
	const ExprNode* flawed_node)
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
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(flawed_node->line) <<
					  									   Arg::Num(flawed_node->column));
		}
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-206) <<
					  Arg::Gds(isc_dsql_field_err) <<
					  Arg::Gds(isc_dsql_line_col_error) << Arg::Num(flawed_node->line) <<
					  									   Arg::Num(flawed_node->column));
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

	PASS1_node_match

    @brief	Compare two nodes for equality of value.

   [2002-08-04]--- Arno Brinkman
 	If ignoreMapCast is true and the node1 is
   type nod_cast or nod_map then PASS1_node_match is
   calling itselfs again with the node1
   CASTs source or map->node.
   This is for allow CAST to other datatypes
   without complaining that it's an unknown
   column reference. (Aggregate functions)


    @param node1
    @param node2
    @param ignoreMapCast

 **/
bool PASS1_node_match(const ExprNode* node1, const ExprNode* node2, bool ignoreMapCast)
{
	DEV_BLKCHK(node1, dsql_type_nod);
	DEV_BLKCHK(node2, dsql_type_nod);

	if (!node1 && !node2)
		return true;

	if (!node1 || !node2)
		return false;

	const CastNode* castNode1 = node1->as<CastNode>();

	if (ignoreMapCast && castNode1)
	{
		const CastNode* castNode2 = node2->as<CastNode>();

		// If node2 is also cast and same type continue with both sources.
		if (castNode2 &&
			castNode1->castDesc.dsc_dtype == castNode2->castDesc.dsc_dtype &&
			castNode1->castDesc.dsc_scale == castNode2->castDesc.dsc_scale &&
			castNode1->castDesc.dsc_length == castNode2->castDesc.dsc_length &&
			castNode1->castDesc.dsc_sub_type == castNode2->castDesc.dsc_sub_type)
		{
			return PASS1_node_match(castNode1->source, castNode2->source, ignoreMapCast);
		}

		return PASS1_node_match(castNode1->source, node2, ignoreMapCast);
	}

	const DsqlMapNode* mapNode1 = node1->as<DsqlMapNode>();

	if (ignoreMapCast && mapNode1)
	{
		const DsqlMapNode* mapNode2 = node2->as<DsqlMapNode>();

		if (mapNode2)
		{
			if (mapNode1->context != mapNode2->context)
				return false;

			return PASS1_node_match(mapNode1->map->map_node, mapNode2->map->map_node, ignoreMapCast);
		}

		return PASS1_node_match(mapNode1->map->map_node, node2, ignoreMapCast);
	}

	const DsqlAliasNode* aliasNode1 = node1->as<DsqlAliasNode>();
	const DsqlAliasNode* aliasNode2 = node2->as<DsqlAliasNode>();

	// We don't care about the alias itself but only about its field.
	if (aliasNode1 || aliasNode2)
	{
		if (aliasNode1 && aliasNode2)
			return PASS1_node_match(aliasNode1->value, aliasNode2->value, ignoreMapCast);

		if (aliasNode1)
			return PASS1_node_match(aliasNode1->value, node2, ignoreMapCast);

		if (aliasNode2)
			return PASS1_node_match(node1, aliasNode2->value, ignoreMapCast);
	}

	// Handle derived fields.

	const DerivedFieldNode* derivedField1 = node1->as<DerivedFieldNode>();
	const DerivedFieldNode* derivedField2 = node2->as<DerivedFieldNode>();

	if (derivedField1 || derivedField2)
	{
		if (derivedField1 && derivedField2)
		{
			if (derivedField1->context->ctx_context != derivedField2->context->ctx_context ||
				derivedField1->name != derivedField2->name)
			{
				return false;
			}

			return PASS1_node_match(derivedField1->value, derivedField2->value, ignoreMapCast);
		}

		if (derivedField1)
			return PASS1_node_match(derivedField1->value, node2, ignoreMapCast);

		if (derivedField2)
			return PASS1_node_match(node1, derivedField2->value, ignoreMapCast);
	}

	return node1->type == node2->type && node1->dsqlMatch(node2, ignoreMapCast);
}


/**

 	PASS1_cursor_name

    @brief	Find a cursor.


    @param dsqlScratch
    @param name
	@param mask
	@param existence_flag

 **/
DeclareCursorNode* PASS1_cursor_name(DsqlCompilerScratch* dsqlScratch, const MetaName& name,
	USHORT mask, bool existence_flag)
{
	DeclareCursorNode* cursor = NULL;

	if (name.isEmpty())
	{
		if (existence_flag)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
					  Arg::Gds(isc_dsql_cursor_err) <<
					  Arg::Gds(isc_dsql_cursor_invalid));
		}
		else
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
					  Arg::Gds(isc_dsql_decl_err) <<
					  Arg::Gds(isc_dsql_cursor_invalid));
		}
	}

	for (Array<DeclareCursorNode*>::iterator itr = dsqlScratch->cursors.begin();
		 itr != dsqlScratch->cursors.end();
		 ++itr)
	{
		cursor = *itr;
		if (cursor->dsqlName == name && (cursor->dsqlCursorType & mask))
			break;
		cursor = NULL;
	}

	if (!cursor && existence_flag)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_dsql_cursor_err) <<
				  Arg::Gds(isc_dsql_cursor_not_found) << name);
	}
	else if (cursor && !existence_flag)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_decl_err) <<
				  Arg::Gds(isc_dsql_cursor_exists) << name);
	}

	return cursor;
}


// Extract relation and procedure context and expand derived child contexts.
static void pass1_expand_contexts(DsqlContextStack& contexts, dsql_ctx* context)
{
	if (context->ctx_relation || context->ctx_procedure ||
		context->ctx_map || context->ctx_win_maps.hasData())
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


// Process derived table which is part of a from clause.
RseNode* PASS1_derived_table(DsqlCompilerScratch* dsqlScratch, SelectExprNode* input,
	const char* cte_alias)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	const string& alias = input->alias;

	// Create the context now, because we need to know it for the tables inside.
	dsql_ctx* const context = PASS1_make_context(dsqlScratch, input);

	// Save some values to restore after rse process.
	DsqlContextStack* const req_base = dsqlScratch->context;
	const string aliasRelationPrefix = dsqlScratch->aliasRelationPrefix;

	// Change context, because when we are processing the derived table rse
	// it may not reference to other streams in the same scope_level.
	DsqlContextStack temp;
	// Put special contexts (NEW/OLD) also on the stack
	for (DsqlContextStack::iterator stack(*dsqlScratch->context); stack.hasData(); ++stack)
	{
		dsql_ctx* local_context = stack.object();
		if ((local_context->ctx_scope_level < dsqlScratch->scopeLevel) ||
			(local_context->ctx_flags & CTX_system))
		{
			temp.push(local_context);
		}
	}

	dsql_ctx* baseContext = NULL;

	if (temp.hasData())
		baseContext = temp.object();

	dsqlScratch->context = &temp;
	dsqlScratch->aliasRelationPrefix = pass1_alias_concat(aliasRelationPrefix, alias);

	RecordSourceNode* query = input->querySpec;
	UnionSourceNode* unionQuery = query->as<UnionSourceNode>();
	RseNode* rse = NULL;
	const bool isRecursive = unionQuery && unionQuery->recursive;
	USHORT recursive_map_ctx = 0;

	if (isRecursive)
	{
		// Create dummy, non-recursive select dsqlScratch by doing a union of
		// one, non-recursive member. The dummy will be replaced at the end
		// of this function.

		RecordSourceNode* save = unionQuery->dsqlClauses->items.pop();
		unionQuery->recursive = false;

		dsql_ctx* baseUnionCtx = dsqlScratch->unionContext.hasData() ?
			dsqlScratch->unionContext.object() : NULL;

		// reserve extra context number for map's secondary context
		recursive_map_ctx = dsqlScratch->contextNumber++;

		dsqlScratch->recursiveCtxId = dsqlScratch->contextNumber;
		rse = pass1_union(dsqlScratch, unionQuery, NULL, NULL, false, 0);
		dsqlScratch->contextNumber = dsqlScratch->recursiveCtxId + 1;

		// recursive union always has exactly 2 members
		unionQuery->dsqlClauses->items.push(save);
		unionQuery->recursive = true;

		while (dsqlScratch->unionContext.hasData() &&
			   dsqlScratch->unionContext.object() != baseUnionCtx)
		{
			dsqlScratch->unionContext.pop();
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
		RseNode* queryNode = query->as<RseNode>();
		if (queryNode)
			foundSubSelect = SubSelectFinder::find(queryNode->dsqlSelectList);

		if (foundSubSelect)
		{
			UnionSourceNode* unionExpr = FB_NEW(pool) UnionSourceNode(pool);
			unionExpr->dsqlClauses = FB_NEW(pool) RecSourceListNode(pool, 1);
			unionExpr->dsqlClauses->items[0] = input;
			unionExpr->dsqlAll = true;
			rse = pass1_union(dsqlScratch, unionExpr, NULL, NULL, false, 0);
		}
		else
			rse = PASS1_rse(dsqlScratch, input, false);

		// Finish off by cleaning up contexts and put them into derivedContext
		// so create view (ddl) can deal with it.
		// Also add the used contexts into the childs stack.
		while (temp.hasData() && (temp.object() != baseContext))
		{
			dsql_ctx* childCtx = temp.pop();

			dsqlScratch->derivedContext.push(childCtx);
			context->ctx_childs_derived_table.push(childCtx);

			// Collect contexts that will be used for blr_derived_expr generation.
			pass1_expand_contexts(context->ctx_main_derived_contexts, childCtx);
		}

		while (temp.hasData())
			temp.pop();
	}

	context->ctx_rse = rse;

	// CVC: prepare a truncated alias for the derived table here
	// because we need it several times.
	TEXT aliasbuffer[100] = "";
	const TEXT* aliasname = aliasbuffer;
	if (alias.hasData())
	{
		int length = alias.length();
		if (length > 99)
		{
			length = 99;
			memcpy(aliasbuffer, alias.c_str(), length);
			aliasbuffer[length] = 0;
		}
		else
			aliasname = alias.c_str();
	}
	else
		aliasname = "<unnamed>";

	// If an alias-list is specified, process it.

	const bool ignoreColumnChecks =
		(input->dsqlFlags & RecordSourceNode::DFLAG_DT_IGNORE_COLUMN_CHECK);

	if (input->columns && input->columns->hasData())
	{
		// Do both lists have the same number of items?
		if (input->columns->getCount() != rse->dsqlSelectList->items.getCount())
		{
			// Column list by derived table %s [alias-name] has %s [more/fewer] columns
			// than the number of items.

			int errcode = isc_dsql_derived_table_less_columns;
			if (input->columns->getCount() > rse->dsqlSelectList->items.getCount())
				errcode = isc_dsql_derived_table_more_columns;

			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(errcode) << Arg::Str(aliasname));
		}

		// Generate derived fields and assign alias-name to them.
		for (size_t count = 0; count < input->columns->getCount(); ++count)
		{
			ValueExprNode* select_item = rse->dsqlSelectList->items[count];
			MAKE_desc(dsqlScratch, &select_item->nodDesc, select_item);

			// Make new derived field node.

			DerivedFieldNode* derivedField = FB_NEW(pool) DerivedFieldNode(pool,
				(*input->columns)[count], dsqlScratch->scopeLevel, select_item);
			derivedField->nodDesc = select_item->nodDesc;
			rse->dsqlSelectList->items[count] = derivedField;
		}
	}
	else
	{
		// For those select-items where no alias is specified try
		// to generate one from the field_name.
		for (size_t count = 0; count < rse->dsqlSelectList->items.getCount(); ++count)
		{
			ValueExprNode* select_item = pass1_make_derived_field(tdbb, dsqlScratch,
				rse->dsqlSelectList->items[count]);

			// Auto-create dummy column name for pass1_any()
			if (ignoreColumnChecks && !ExprNode::is<DerivedFieldNode>(select_item))
			{
				MAKE_desc(dsqlScratch, &select_item->nodDesc, select_item);

				// Construct dummy fieldname
				char fieldname[25];
				sprintf(fieldname, "f%"SIZEFORMAT, count);

				// Make new derived field node.

				DerivedFieldNode* derivedField = FB_NEW(pool) DerivedFieldNode(pool,
					fieldname, dsqlScratch->scopeLevel, select_item);
				derivedField->nodDesc = select_item->nodDesc;
				select_item = derivedField;
			}

			rse->dsqlSelectList->items[count] = select_item;
		}
	}

	size_t count;

	// Check if all root select-items have a derived field else show a message.
	for (count = 0; count < rse->dsqlSelectList->items.getCount(); ++count)
	{
		ValueExprNode* select_item = rse->dsqlSelectList->items[count];
		DerivedFieldNode* derivedField;

		if ((derivedField = ExprNode::as<DerivedFieldNode>(select_item)))
			derivedField->context = context;
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
	for (count = 0; count < rse->dsqlSelectList->items.getCount(); ++count)
	{
		const DerivedFieldNode* selectItem1 =
			rse->dsqlSelectList->items[count]->as<DerivedFieldNode>();

		for (size_t count2 = (count + 1); count2 < rse->dsqlSelectList->items.getCount(); ++count2)
		{
			const DerivedFieldNode* selectItem2 =
				rse->dsqlSelectList->items[count2]->as<DerivedFieldNode>();

			if (selectItem1->name == selectItem2->name)
			{
				// column %s was specified multiple times for derived table %s
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_derived_field_dup_name) << selectItem1->name <<
								aliasname);
			}
		}
	}

	// If we used a dummy rse before, replace it with the real one now.
	// We cannot do this earlier, because recursive processing needs a fully
	// developed context block.
	if (isRecursive)
	{
		dsql_ctx* const saveRecursiveCtx = dsqlScratch->recursiveCtx;
		dsqlScratch->recursiveCtx = context;
		dsqlScratch->context = &temp;

		const string* const* saveCteAlias = dsqlScratch->currCteAlias;
		dsqlScratch->resetCTEAlias(alias);

		rse = PASS1_rse(dsqlScratch, input, false);

		if (saveCteAlias)
			dsqlScratch->resetCTEAlias(**saveCteAlias);
		dsqlScratch->recursiveCtx = saveRecursiveCtx;

		// Finish off by cleaning up contexts and put them into derivedContext
		// so create view (ddl) can deal with it.
		// Also add the used contexts into the childs stack.
		while (temp.hasData() && (temp.object() != baseContext))
		{
			dsqlScratch->derivedContext.push(temp.object());
			context->ctx_childs_derived_table.push(temp.pop());
		}

		temp.clear();

		rse->dsqlSelectList = context->ctx_rse->dsqlSelectList;

		context->ctx_rse = rse;

		if (cte_alias)
			context->ctx_alias = cte_alias;

		dsqlScratch->context = req_base;

		// Mark union's map context as recursive and assign secondary context number to it.
		ValueListNode* items = rse->dsqlSelectList;
		ValueExprNode* map_item = items->items[0];
		DerivedFieldNode* derivedField;

		if ((derivedField = ExprNode::as<DerivedFieldNode>(map_item)))
			map_item = derivedField->value;

		dsql_ctx* map_context = ExprNode::as<DsqlMapNode>(map_item)->context;

		map_context->ctx_flags |= CTX_recursive;
		map_context->ctx_recursive = recursive_map_ctx;
	}

	// Restore our original values.
	dsqlScratch->context = req_base;
	dsqlScratch->aliasRelationPrefix = aliasRelationPrefix;

	rse->dsqlContext = context;

	return rse;
}


/**

 	pass1_expand_select_list

    @brief	Expand asterisk nodes into fields.


    @param dsqlScratch
    @param list
    @param streams

 **/
static ValueListNode* pass1_expand_select_list(DsqlCompilerScratch* dsqlScratch, ValueListNode* list,
	RecSourceListNode* streams)
{
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	ValueListNode* retList = FB_NEW(pool) ValueListNode(pool, 0u);

	if (list)
	{
		for (NestConst<ValueExprNode>* ptr = list->items.begin(); ptr != list->items.end(); ++ptr)
			PASS1_expand_select_node(dsqlScratch, *ptr, retList, true);
	}
	else
	{
		for (NestConst<RecordSourceNode>* ptr = streams->items.begin(); ptr != streams->items.end(); ++ptr)
			PASS1_expand_select_node(dsqlScratch, *ptr, retList, true);
	}

	return retList;
}

// Expand a select item node.
void PASS1_expand_select_node(DsqlCompilerScratch* dsqlScratch, ExprNode* node, ValueListNode* list,
	bool hide_using)
{
	RseNode* rseNode;
	ProcedureSourceNode* procNode;
	RelationSourceNode* relNode;
	FieldNode* fieldNode;

	if ((rseNode = ExprNode::as<RseNode>(node)))
	{
		ValueListNode* sub_items = rseNode->dsqlSelectList;

		if (sub_items)	// AB: Derived table support
		{
			NestConst<ValueExprNode>* ptr = sub_items->items.begin();

			for (const NestConst<ValueExprNode>* const end = sub_items->items.end(); ptr != end; ++ptr)
			{
				// Create a new alias else mappings would be mangled.
				NestConst<ValueExprNode> select_item = *ptr;

				// select-item should always be a derived field!

				DerivedFieldNode* derivedField;

				if (!(derivedField = select_item->as<DerivedFieldNode>()))
				{
					// Internal dsql error: alias type expected by PASS1_expand_select_node
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_command_err) <<
							  Arg::Gds(isc_dsql_derived_alias_select));
				}

				dsql_ctx* context = derivedField->context;
				DEV_BLKCHK(context, dsql_type_ctx);

				if (!hide_using || context->getImplicitJoinField(derivedField->name, select_item))
					list->add(select_item);
			}
		}
		else	// joins
		{
			RecSourceListNode* streamList = rseNode->dsqlStreams;

			for (NestConst<RecordSourceNode>* ptr = streamList->items.begin();
				 ptr != streamList->items.end();
				 ++ptr)
			{
				PASS1_expand_select_node(dsqlScratch, *ptr, list, true);
			}
		}
	}
	else if ((procNode = ExprNode::as<ProcedureSourceNode>(node)))
	{
		dsql_ctx* context = procNode->dsqlContext;

		if (context->ctx_procedure)
		{
			for (dsql_fld* field = context->ctx_procedure->prc_outputs; field; field = field->fld_next)
			{
				DEV_BLKCHK(field, dsql_type_fld);

				NestConst<ValueExprNode> select_item = NULL;
				if (!hide_using || context->getImplicitJoinField(field->fld_name, select_item))
				{
					if (!select_item)
						select_item = MAKE_field(context, field, NULL);
					list->add(select_item);
				}
			}
		}
	}
	else if ((relNode = ExprNode::as<RelationSourceNode>(node)))
	{
		dsql_ctx* context = relNode->dsqlContext;

		if (context->ctx_relation)
		{
			for (dsql_fld* field = context->ctx_relation->rel_fields; field; field = field->fld_next)
			{
				DEV_BLKCHK(field, dsql_type_fld);

				NestConst<ValueExprNode> select_item = NULL;
				if (!hide_using || context->getImplicitJoinField(field->fld_name, select_item))
				{
					if (!select_item)
						select_item = MAKE_field(context, field, NULL);
					list->add(select_item);
				}
			}
		}
	}
	else if ((fieldNode = ExprNode::as<FieldNode>(node)))
	{
		RecordSourceNode* recSource = NULL;
		ValueExprNode* value = fieldNode->internalDsqlPass(dsqlScratch, &recSource);

		if (recSource)
			PASS1_expand_select_node(dsqlScratch, recSource, list, false);
		else
			list->add(value);
	}
	else
	{
		fb_assert(node->kind == DmlNode::KIND_VALUE);
		list->add(static_cast<ValueExprNode*>(node));
	}
}


/**

 	pass1_group_by_list

    @brief	Process GROUP BY list, which may contain
			an ordinal or alias which references the
			select list.

    @param dsqlScratch
    @param input
    @param select_list

 **/
static ValueListNode* pass1_group_by_list(DsqlCompilerScratch* dsqlScratch, ValueListNode* input,
	ValueListNode* selectList)
{
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	if (input->items.getCount() > MAX_SORT_ITEMS) // sort, group and distinct have the same limit for now
	{
		// cannot group on more than 255 items
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_max_group_items));
	}

	ValueListNode* retList = FB_NEW(pool) ValueListNode(pool, 0u);

	NestConst<ValueExprNode>* ptr = input->items.begin();
	for (const NestConst<ValueExprNode>* const end = input->items.end(); ptr != end; ++ptr)
	{
		ValueExprNode* sub = (*ptr);
		ValueExprNode* frnode = NULL;
		FieldNode* field;
		LiteralNode* literal;

		if ((field = ExprNode::as<FieldNode>(sub)))
		{
			// check for alias or field node
			if (selectList && field->dsqlQualifier.isEmpty() && field->dsqlName.hasData())
			{
				// AB: Check first against the select list for matching column.
				// When no matches at all are found we go on with our
				// normal way of field name lookup.
				frnode = PASS1_lookup_alias(dsqlScratch, field->dsqlName, selectList, true);
			}

			if (!frnode)
				frnode = field->internalDsqlPass(dsqlScratch, NULL);
		}
		else if ((literal = ExprNode::as<LiteralNode>(sub)) && (literal->litDesc.dsc_dtype == dtype_long))
		{
			const ULONG position = literal->getSlong();

			if (position < 1 || !selectList || position > (ULONG) selectList->items.getCount())
			{
				// Invalid column position used in the GROUP BY clause
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_column_pos_err) << Arg::Str("GROUP BY"));
			}

			frnode = Node::doDsqlPass(dsqlScratch, selectList->items[position - 1], false);
		}
		else
			frnode = Node::doDsqlPass(dsqlScratch, *ptr, false);

		retList->add(frnode);
	}

	// Finally make the complete list.
	return retList;
}


// Process the limit clause (FIRST/SKIP/ROWS)
void PASS1_limit(DsqlCompilerScratch* dsqlScratch, NestConst<ValueExprNode> firstNode,
	NestConst<ValueExprNode> skipNode, RseNode* rse)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);
	DEV_BLKCHK(firstNode, dsql_type_nod);
	DEV_BLKCHK(skipNode, dsql_type_nod);

	dsc descNode;

	if (dsqlScratch->clientDialect <= SQL_DIALECT_V5)
		descNode.makeLong(0);
	else
		descNode.makeInt64(0);

	rse->dsqlFirst = Node::doDsqlPass(dsqlScratch, firstNode, false);
	PASS1_set_parameter_type(dsqlScratch, rse->dsqlFirst, &descNode, false);

	rse->dsqlSkip = Node::doDsqlPass(dsqlScratch, skipNode, false);
	PASS1_set_parameter_type(dsqlScratch, rse->dsqlSkip, &descNode, false);
}


// Lookup a matching item in the select list. Return node if found else return NULL.
// If more matches are found we raise ambiguity error.
ValueExprNode* PASS1_lookup_alias(DsqlCompilerScratch* dsqlScratch, const MetaName& name,
	ValueListNode* selectList, bool process)
{
	ValueExprNode* returnNode = NULL;
	NestConst<ValueExprNode>* ptr = selectList->items.begin();
	const NestConst<ValueExprNode>* const end = selectList->items.end();
	for (; ptr < end; ++ptr)
	{
		NestConst<ValueExprNode> matchingNode = NULL;
		ValueExprNode* node = *ptr;
		DsqlAliasNode* aliasNode;
		FieldNode* fieldNode;
		DerivedFieldNode* derivedField;

		if ((aliasNode = node->as<DsqlAliasNode>()))
		{
			if (aliasNode->name == name)
				matchingNode = node;
		}
		else if ((fieldNode = node->as<FieldNode>()))
		{
			if (fieldNode->dsqlField->fld_name == name.c_str())
				matchingNode = node;
		}
		else if ((derivedField = node->as<DerivedFieldNode>()))
		{
			if (derivedField->name == name)
				matchingNode = node;
		}

		if (matchingNode)
		{
			if (process)
				matchingNode = Node::doDsqlPass(dsqlScratch, matchingNode, false);

			if (returnNode)
			{
				// There was already a node matched, thus raise ambiguous field name error.
				TEXT buffer1[256];
				buffer1[0] = 0;

				if (returnNode->is<DsqlAliasNode>())
					strcat(buffer1, "an alias");
				else if (returnNode->is<FieldNode>())
					strcat(buffer1, "a field");
				else if (returnNode->is<DerivedFieldNode>())
					strcat(buffer1, "a derived field");
				else
					strcat(buffer1, "an item");

				TEXT buffer2[256];
				buffer2[0] = 0;

				if (matchingNode->is<DsqlAliasNode>())
					strcat(buffer2, "an alias");
				else if (matchingNode->is<FieldNode>())
					strcat(buffer2, "a field");
				else if (matchingNode->is<DerivedFieldNode>())
					strcat(buffer2, "a derived field");
				else
					strcat(buffer2, "an item");

				strcat(buffer2, " in the select list with name");

				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
						  Arg::Gds(isc_dsql_ambiguous_field_name) << Arg::Str(buffer1) <<
																	 Arg::Str(buffer2) <<
						  Arg::Gds(isc_random) << name);
			}

			returnNode = matchingNode;
		}
	}

	return returnNode;
}

/**

 	pass1_make_derived_field

    @brief	Create a derived field based on underlying expressions


    @param dsqlScratch
    @param tdbb
    @param select_item

 **/
static ValueExprNode* pass1_make_derived_field(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	ValueExprNode* select_item)
{
	MemoryPool& pool = *tdbb->getDefaultPool();
	DsqlAliasNode* aliasNode;
	SubQueryNode* subQueryNode;
	DsqlMapNode* mapNode;
	FieldNode* fieldNode;
	DerivedFieldNode* derivedField;

	if ((aliasNode = ExprNode::as<DsqlAliasNode>(select_item)))
	{
		// Create a derived field and ignore alias node.
		DerivedFieldNode* newField = FB_NEW(pool) DerivedFieldNode(pool,
			aliasNode->name, dsqlScratch->scopeLevel, aliasNode->value);
		newField->nodDesc = aliasNode->value->nodDesc;
		return newField;
	}
	else if ((subQueryNode = ExprNode::as<SubQueryNode>(select_item)))
	{
		// Try to generate derived field from sub-select
		ValueExprNode* derived_field = pass1_make_derived_field(tdbb, dsqlScratch,
			subQueryNode->value1);

		if ((derivedField = ExprNode::as<DerivedFieldNode>(derived_field)))
		{
			derivedField->value = select_item;
			return derived_field;
		}
	}
	else if ((mapNode = ExprNode::as<DsqlMapNode>(select_item)))
	{
		// Aggregate's have map on top.
		ValueExprNode* derived_field = pass1_make_derived_field(tdbb, dsqlScratch, mapNode->map->map_node);

		// If we had succesfully made a derived field node change it with orginal map.
		if ((derivedField = ExprNode::as<DerivedFieldNode>(derived_field)))
		{
			derivedField->value = select_item;
			derivedField->scope = dsqlScratch->scopeLevel;
			derived_field->nodDesc = select_item->nodDesc;
			return derived_field;
		}
	}
	else if ((fieldNode = ExprNode::as<FieldNode>(select_item)))
	{
		// Create a derived field and hook in.

		DerivedFieldNode* newField = FB_NEW(pool) DerivedFieldNode(pool,
			fieldNode->dsqlField->fld_name, dsqlScratch->scopeLevel, select_item);
		newField->nodDesc = fieldNode->nodDesc;
		return newField;
	}
	else if ((derivedField = ExprNode::as<DerivedFieldNode>(select_item)))
	{
		// Create a derived field that points to a derived field.

		DerivedFieldNode* newField = FB_NEW(pool) DerivedFieldNode(pool,
			derivedField->name, dsqlScratch->scopeLevel, select_item);
		newField->nodDesc = select_item->nodDesc;
		return newField;
	}

	return select_item;
}


// Prepare a relation name for processing. Allocate a new relation node.
RecordSourceNode* PASS1_relation(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* input)
{
	thread_db* tdbb = JRD_get_thread_data();

	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	dsql_ctx* context = PASS1_make_context(dsqlScratch, input);
	RecordSourceNode* node = NULL;

	if (context->ctx_relation)
	{
		RelationSourceNode* relNode = FB_NEW(*tdbb->getDefaultPool()) RelationSourceNode(
			*tdbb->getDefaultPool(), context->ctx_relation->rel_name);
		relNode->dsqlContext = context;
		return relNode;
	}
	else if (context->ctx_procedure)
	{
		ProcedureSourceNode* procNode = FB_NEW(*tdbb->getDefaultPool()) ProcedureSourceNode(
			*tdbb->getDefaultPool(), context->ctx_procedure->prc_name);
		procNode->dsqlContext = context;
		return procNode;
	}

	fb_assert(false);
	return NULL;
}


// Concatenate 2 input strings together for	a new alias string
// Note: Both input params can be empty.
static string pass1_alias_concat(const string& input1, const string& input2)
{
	string output;

	if (input1.hasData())
		output.append(input1);

	if (input2.hasData())
	{
		if (output.hasData())
			output.append(" ");
		output.append(input2);
	}

	return output;
}


// Wrapper for pass1_rse_impl. Substitute recursive CTE alias (if needed) and call pass1_rse_impl.
static RseNode* pass1_rse(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* input,
	ValueListNode* order, RowsClause* rows, bool updateLock, USHORT flags)
{
	string save_alias;
	RseNode* rseNode = input->as<RseNode>();
	const bool isRecursive = rseNode && (rseNode->dsqlFlags & RecordSourceNode::DFLAG_RECURSIVE);
	AutoSetRestore<USHORT> autoScopeLevel(&dsqlScratch->scopeLevel, dsqlScratch->scopeLevel);

	if (isRecursive)
	{
		fb_assert(dsqlScratch->recursiveCtx);
		save_alias = dsqlScratch->recursiveCtx->ctx_alias;

		dsqlScratch->recursiveCtx->ctx_alias = *dsqlScratch->getNextCTEAlias();

		// ASF: We need to reset the scope level to the same value found in the non-recursive
		// part of the query, to verify usage of aggregate functions correctly. See CORE-4322.
		dsqlScratch->scopeLevel = dsqlScratch->recursiveCtx->ctx_scope_level;
	}

	RseNode* ret = pass1_rse_impl(dsqlScratch, input, order, rows, updateLock, flags);

	if (isRecursive)
		dsqlScratch->recursiveCtx->ctx_alias = save_alias;

	return ret;
}


// Compile a record selection expression. The input node may either be a "select_expression"
// or a "list" (an implicit union) or a "query specification".
static RseNode* pass1_rse_impl(DsqlCompilerScratch* dsqlScratch, RecordSourceNode* input,
	ValueListNode* order, RowsClause* rows, bool updateLock, USHORT flags)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	SelectExprNode* selNode = input->as<SelectExprNode>();
	UnionSourceNode* unionNode = input->as<UnionSourceNode>();

	if (selNode)
	{
		WithClause* withClause = selNode->withClause;
		try
		{
			if (withClause)
				dsqlScratch->addCTEs(withClause);

			RseNode* ret = pass1_rse(dsqlScratch, selNode->querySpec, selNode->orderClause,
				selNode->rowsClause, updateLock, selNode->dsqlFlags);

			if (withClause)
			{
				dsqlScratch->checkUnusedCTEs();
				dsqlScratch->clearCTEs();
			}

			return ret;
		}
		catch (const Exception&)
		{
			if (withClause)
				dsqlScratch->clearCTEs();
			throw;
		}
	}
	else if (unionNode)
	{
		fb_assert(unionNode->dsqlClauses->items.hasData());
		return pass1_union(dsqlScratch, unionNode, order, rows, updateLock, flags);
	}

	RseNode* inputRse = input->as<RseNode>();
	fb_assert(inputRse);

	// Save the original base of the context stack and process relations

	RseNode* targetRse = FB_NEW(pool) RseNode(pool);
	RseNode* rse = targetRse;

	if (updateLock)
		rse->flags |= RseNode::FLAG_WRITELOCK;

	rse->dsqlStreams = Node::doDsqlPass(dsqlScratch, inputRse->dsqlFrom, false);
	RecSourceListNode* streamList = rse->dsqlStreams;

	{ // scope block
		RelationSourceNode* relNode;
		const dsql_rel* relation;

		if (updateLock &&
			(streamList->items.getCount() != 1 ||
				!(relNode = streamList->items[0]->as<RelationSourceNode>()) ||
				!(relation = relNode->dsqlContext->ctx_relation) ||
				(relation->rel_flags & (REL_view | REL_external))))
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_wlock_simple));
		}
	} // end scope block

	// Process LIMIT and/or ROWS, if any

	if ((inputRse->dsqlFirst || inputRse->dsqlSkip) && rows)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_firstskip_rows));
	}
	else if (rows)
		PASS1_limit(dsqlScratch, rows->length, rows->skip, rse);
	else if (inputRse->dsqlFirst || inputRse->dsqlSkip)
		PASS1_limit(dsqlScratch, inputRse->dsqlFirst, inputRse->dsqlSkip, rse);

	// Process boolean, if any

	if (inputRse->dsqlWhere)
	{
		++dsqlScratch->inWhereClause;
		rse->dsqlWhere = Node::doDsqlPass(dsqlScratch, inputRse->dsqlWhere, false);
		--dsqlScratch->inWhereClause;

		// AB: An aggregate pointing to it's own parent_context isn't
		// allowed, HAVING should be used instead
		if (Aggregate2Finder::find(dsqlScratch->scopeLevel, FIELD_MATCH_TYPE_EQUAL, false,
				rse->dsqlWhere))
		{
			// Cannot use an aggregate in a WHERE clause, use HAVING instead
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_agg_where_err));
		}
	}

#ifdef DSQL_DEBUG
	if (DSQL_debug & 16)
	{
		dsql_trace("PASS1_rse input tree:");
		//// TODO: Implement Node::print correctly.
	}
#endif

	// Process select list, if any. If not, generate one
	ValueListNode* selectList = inputRse->dsqlSelectList;
	// First expand select list, this will expand nodes with asterisk.
	++dsqlScratch->inSelectList;
	selectList = pass1_expand_select_list(dsqlScratch, selectList, rse->dsqlStreams);

	if ((flags & RecordSourceNode::DFLAG_VALUE) &&
		(!selectList || selectList->items.getCount() > 1))
	{
		// More than one column (or asterisk) is specified in column_singleton
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_dsql_count_mismatch));
	}

	// Pass select list
	rse->dsqlSelectList = pass1_sel_list(dsqlScratch, selectList);
	--dsqlScratch->inSelectList;

	if (inputRse->dsqlFlags & RecordSourceNode::DFLAG_RECURSIVE)
	{
		if (Aggregate2Finder::find(dsqlScratch->scopeLevel, FIELD_MATCH_TYPE_EQUAL, false,
				rse->dsqlSelectList))
		{
			// Recursive member of CTE cannot use aggregate function
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_cte_recursive_aggregate));
		}
	}

	// Process ORDER clause, if any
	if (order)
	{
		++dsqlScratch->inOrderByClause;
		rse->dsqlOrder = PASS1_sort(dsqlScratch, order, selectList);
		--dsqlScratch->inOrderByClause;

		if (inputRse->dsqlFlags & RecordSourceNode::DFLAG_RECURSIVE)
		{
			if (Aggregate2Finder::find(dsqlScratch->scopeLevel, FIELD_MATCH_TYPE_EQUAL, false,
					rse->dsqlOrder))
			{
				// Recursive member of CTE cannot use aggregate function
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_cte_recursive_aggregate));
			}
		}
	}

	// A GROUP BY, HAVING, or any aggregate function in the select list
	// will force an aggregate
	dsql_ctx* parent_context = NULL;
	RseNode* parentRse = NULL;
	AggregateSourceNode* aggregate = NULL;

	if (inputRse->dsqlGroup ||
		inputRse->dsqlHaving ||
		(rse->dsqlSelectList && AggregateFinder::find(dsqlScratch, false, rse->dsqlSelectList)) ||
		(rse->dsqlOrder && AggregateFinder::find(dsqlScratch, false, rse->dsqlOrder)))
	{
		// dimitr: don't allow WITH LOCK for aggregates
		if (updateLock)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_wlock_aggregates));
		}

		parent_context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
		parent_context->ctx_scope_level = dsqlScratch->scopeLevel;

		// When we're in a outer-join part mark context for it.
		if (dsqlScratch->inOuterJoin)
			parent_context->ctx_flags |= CTX_outer_join;
		parent_context->ctx_in_outer_join = dsqlScratch->inOuterJoin;

		aggregate = FB_NEW(pool) AggregateSourceNode(pool);
		aggregate->dsqlContext = parent_context;
		aggregate->dsqlRse = rse;
		parentRse = targetRse = FB_NEW(pool) RseNode(pool);
		parentRse->dsqlStreams = (streamList = FB_NEW(pool) RecSourceListNode(pool, 1));
		streamList->items[0] = aggregate;

		if (rse->dsqlFirst)
		{
			parentRse->dsqlFirst = rse->dsqlFirst;
			rse->dsqlFirst = NULL;
		}

		if (rse->dsqlSkip)
		{
			parentRse->dsqlSkip = rse->dsqlSkip;
			rse->dsqlSkip = NULL;
		}

		dsqlScratch->context->push(parent_context);
		// replace original contexts with parent context
		remap_streams_to_parent_context(rse->dsqlStreams, parent_context);
	}

	// Process GROUP BY clause, if any
	if (inputRse->dsqlGroup)
	{
		// if there are positions in the group by clause then replace them
		// by the (newly pass) items from the select_list
		++dsqlScratch->inGroupByClause;
		aggregate->dsqlGroup = pass1_group_by_list(dsqlScratch, inputRse->dsqlGroup, selectList);
		--dsqlScratch->inGroupByClause;

		// AB: An field pointing to another parent_context isn't
		// allowed and GROUP BY items can't contain aggregates
		if (FieldFinder::find(dsqlScratch->scopeLevel, FIELD_MATCH_TYPE_LOWER, aggregate->dsqlGroup) ||
			Aggregate2Finder::find(dsqlScratch->scopeLevel, FIELD_MATCH_TYPE_LOWER_EQUAL,
				false, aggregate->dsqlGroup))
		{
			// Cannot use an aggregate in a GROUP BY clause
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_agg_group_err));
		}
	}

	// Parse a user-specified access PLAN
	if (inputRse->rse_plan)
	{
		PsqlChanger changer(dsqlScratch, false);
		rse->rse_plan = inputRse->rse_plan->dsqlPass(dsqlScratch);
	}

	// AB: Pass select-items for distinct operation again, because for
	// sub-selects a new context number should be generated
	if (inputRse->dsqlDistinct)
	{
		if (updateLock)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_wlock_conflict) << Arg::Str("DISTINCT"));
		}

		++dsqlScratch->inSelectList;
		targetRse->dsqlDistinct = pass1_sel_list(dsqlScratch, selectList);
		--dsqlScratch->inSelectList;

		// sort, group and distinct have the same limit for now
		if (selectList->items.getCount() > MAX_SORT_ITEMS)
		{
			// Cannot have more than 255 items in DISTINCT list.
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  Arg::Gds(isc_dsql_max_distinct_items));
		}
	}

	if (parent_context)
	{
		FieldRemapper remapper(dsqlScratch, parent_context, false);

		// Reset context of select items to point to the parent stream

		ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlSelectList, rse->dsqlSelectList);
		rse->dsqlSelectList = NULL;

		// AB: Check for invalid constructions inside selected-items list
		ValueListNode* valueList = parentRse->dsqlSelectList;

		{ // scope block
			NestConst<ValueExprNode>* ptr = valueList->items.begin();
			for (const NestConst<ValueExprNode>* const end = valueList->items.end(); ptr != end; ++ptr)
			{
				if (InvalidReferenceFinder::find(parent_context, aggregate->dsqlGroup, *ptr))
				{
					// Invalid expression in the select list
					// (not contained in either an aggregate or the GROUP BY clause)
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("select list"));
				}
			}
		} // end scope block

		// Reset context of order items to point to the parent stream

		if (order)
		{
			ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlOrder, rse->dsqlOrder);
			rse->dsqlOrder = NULL;

			// AB: Check for invalid contructions inside the ORDER BY clause
			ValueListNode* valueList = targetRse->dsqlOrder;
			NestConst<ValueExprNode>* ptr = valueList->items.begin();
			for (const NestConst<ValueExprNode>* const end = valueList->items.end(); ptr != end; ++ptr)
			{
				if (InvalidReferenceFinder::find(parent_context, aggregate->dsqlGroup, *ptr))
				{
					// Invalid expression in the ORDER BY clause
					// (not contained in either an aggregate or the GROUP BY clause)
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("ORDER BY clause"));
				}
			}
		}

		// And, of course, reduction clauses must also apply to the parent
		if (inputRse->dsqlDistinct)
			ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlDistinct);

		// Process HAVING clause, if any

		if (inputRse->dsqlHaving)
		{
			++dsqlScratch->inHavingClause;
			parentRse->dsqlWhere = Node::doDsqlPass(dsqlScratch, inputRse->dsqlHaving, false);
			--dsqlScratch->inHavingClause;

			ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlWhere);

			// AB: Check for invalid contructions inside the HAVING clause

			if (InvalidReferenceFinder::find(parent_context, aggregate->dsqlGroup,
					parentRse->dsqlWhere))
			{
				// Invalid expression in the HAVING clause
				// (neither an aggregate nor contained in the GROUP BY clause)
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_agg_having_err) << Arg::Str("HAVING clause"));
			}

			if (AggregateFinder::find(dsqlScratch, true, parentRse->dsqlWhere))
			{
				// Cannot use an aggregate in a WHERE clause, use HAVING instead
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_agg_where_err));
			}
		}

		rse = parentRse;

		parent_context->ctx_context = dsqlScratch->contextNumber++;
	}

	const bool sortWindow = rse->dsqlOrder && AggregateFinder::find(dsqlScratch, true, rse->dsqlOrder);

	// WINDOW functions
	if ((rse->dsqlSelectList && AggregateFinder::find(dsqlScratch, true, rse->dsqlSelectList)) ||
		sortWindow)
	{
		AutoSetRestore<bool> autoProcessingWindow(&dsqlScratch->processingWindow, true);

		parent_context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());
		parent_context->ctx_scope_level = dsqlScratch->scopeLevel;

		// When we're in a outer-join part mark context for it.
		if (dsqlScratch->inOuterJoin)
			parent_context->ctx_flags |= CTX_outer_join;
		parent_context->ctx_in_outer_join = dsqlScratch->inOuterJoin;

		AggregateSourceNode* window = FB_NEW(pool) AggregateSourceNode(pool);
		window->dsqlContext = parent_context;
		window->dsqlRse = rse;
		window->dsqlWindow = true;

		parentRse = targetRse = FB_NEW(pool) RseNode(pool);
		parentRse->dsqlStreams = streamList = FB_NEW(pool) RecSourceListNode(pool, 1);
		streamList->items[0] = window;

		if (rse->flags & RseNode::FLAG_WRITELOCK)
		{
			parentRse->flags |= RseNode::FLAG_WRITELOCK;
			rse->flags &= ~RseNode::FLAG_WRITELOCK;
		}

		if (rse->dsqlFirst)
		{
			parentRse->dsqlFirst = rse->dsqlFirst;
			rse->dsqlFirst = NULL;
		}

		if (rse->dsqlSkip)
		{
			parentRse->dsqlSkip = rse->dsqlSkip;
			rse->dsqlSkip = NULL;
		}

		dsqlScratch->context->push(parent_context);
		// replace original contexts with parent context
		remap_streams_to_parent_context(rse->dsqlStreams, parent_context);

		if (aggregate)
		{
			// Check for invalid contructions inside selected-items list
			ValueListNode* valueList = rse->dsqlSelectList;
			NestConst<ValueExprNode>* ptr = valueList->items.begin();
			for (const NestConst<ValueExprNode>* const end = valueList->items.end(); ptr != end; ++ptr)
			{
				if (InvalidReferenceFinder::find(parent_context, aggregate->dsqlGroup, *ptr))
				{
					// Invalid expression in the select list
					// (not contained in either an aggregate or the GROUP BY clause)
					ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("select list"));
				}
			}
		}

		FieldRemapper remapper(dsqlScratch, parent_context, true);
		ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlSelectList, rse->dsqlSelectList);

		// Remap the nodes to the partition context.
		for (size_t i = 0, mapCount = parent_context->ctx_win_maps.getCount(); i < mapCount; ++i)
		{
			PartitionMap* partitionMap = parent_context->ctx_win_maps[i];
			if (partitionMap->partition)
			{
				partitionMap->partitionRemapped = Node::doDsqlPass(dsqlScratch, partitionMap->partition);

				FieldRemapper remapper2(dsqlScratch, parent_context, true, partitionMap->partition,
					partitionMap->order);
				ExprNode::doDsqlFieldRemapper(remapper2, partitionMap->partitionRemapped);
			}
		}

		rse->dsqlSelectList = NULL;

		if (order)
		{
			if (aggregate)
			{
				// Check for invalid contructions inside the order-by list
				ValueListNode* valueList = rse->dsqlOrder;
				NestConst<ValueExprNode>* ptr = valueList->items.begin();
				for (const NestConst<ValueExprNode>* const end = valueList->items.end(); ptr != end; ++ptr)
				{
					if (InvalidReferenceFinder::find(parent_context, aggregate->dsqlGroup, *ptr))
					{
						// Invalid expression in the ORDER BY list
						// (not contained in either an aggregate or the GROUP BY clause)
						ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
								  Arg::Gds(isc_dsql_agg_column_err) << Arg::Str("ORDER BY list"));
					}
				}
			}

			ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlOrder, rse->dsqlOrder);
			rse->dsqlOrder = NULL;
		}

		// And, of course, reduction clauses must also apply to the parent
		if (rse->dsqlDistinct)
		{
			ExprNode::doDsqlFieldRemapper(remapper, parentRse->dsqlDistinct, rse->dsqlDistinct);
			rse->dsqlDistinct = NULL;
		}

		rse = parentRse;
	}

	rse->dsqlFlags = flags;

	return rse;
}


/**

 	pass1_sel_list

    @brief	Compile a select list.

    @param dsqlScratch
    @param input

 **/
static ValueListNode* pass1_sel_list(DsqlCompilerScratch* dsqlScratch, ValueListNode* input)
{
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	ValueListNode* retList = FB_NEW(pool) ValueListNode(pool, 0u);

	NestConst<ValueExprNode>* ptr = input->items.begin();
	for (const NestConst<ValueExprNode>* const end = input->items.end(); ptr != end; ++ptr)
		retList->add(Node::doDsqlPass(dsqlScratch, *ptr, false));

	return retList;
}


// Process ORDER BY list, which may contain an ordinal or alias which references the select list.
ValueListNode* PASS1_sort(DsqlCompilerScratch* dsqlScratch, ValueListNode* input, ValueListNode* selectList)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);
	DEV_BLKCHK(input, dsql_type_nod);
	DEV_BLKCHK(selectList, dsql_type_nod);

	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	if (!input)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  // invalid ORDER BY clause
				  Arg::Gds(isc_order_by_err));
	}

	if (input->items.getCount() > MAX_SORT_ITEMS)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_command_err) <<
				  Arg::Gds(isc_order_by_err) <<
				  // invalid ORDER BY clause, cannot sort on more than 255 items
				  Arg::Gds(isc_dsql_max_sort_items));
	}

	// Node is simply to be rebuilt -- just recurse merrily

	NestConst<ValueListNode> node = FB_NEW(pool) ValueListNode(pool, input->items.getCount());
	NestConst<ValueExprNode>* ptr2 = node->items.begin();

	for (size_t sortloop = 0; sortloop < input->items.getCount(); ++sortloop)
	{
		DEV_BLKCHK(input->items[sortloop], dsql_type_nod);
		NestConst<OrderNode> node1 = input->items[sortloop]->as<OrderNode>();
		if (!node1)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
					  Arg::Gds(isc_dsql_command_err) <<
					  // invalid ORDER BY clause
					  Arg::Gds(isc_order_by_err));
		}

		// get node of value to be ordered by
		NestConst<ValueExprNode> orderValue = node1->value;

		NestConst<CollateNode> collateNode = orderValue->as<CollateNode>();

		if (collateNode)
		{
			// substitute CollateNode with its argument (real value)
			orderValue = collateNode->arg;
		}

		FieldNode* field;
		LiteralNode* literal;

		if ((field = orderValue->as<FieldNode>()))
		{
			ValueExprNode* aliasNode = NULL;

			// check for alias or field node
			if (selectList && field->dsqlQualifier.isEmpty() && field->dsqlName.hasData())
			{
				// AB: Check first against the select list for matching column.
				// When no matches at all are found we go on with our
				// normal way of field name lookup.
				aliasNode = PASS1_lookup_alias(dsqlScratch, field->dsqlName, selectList, true);
			}

			orderValue = aliasNode ? aliasNode : field->internalDsqlPass(dsqlScratch, NULL);
		}
		else if ((literal = orderValue->as<LiteralNode>()) && literal->litDesc.dsc_dtype == dtype_long)
		{
			const ULONG position = literal->getSlong();

			if (position < 1 || !selectList || position > (ULONG) selectList->items.getCount())
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  // Invalid column position used in the ORDER BY clause
						  Arg::Gds(isc_dsql_column_pos_err) << Arg::Str("ORDER BY"));
			}

			// substitute ordinal with appropriate field
			orderValue = Node::doDsqlPass(dsqlScratch, selectList->items[position - 1], false);
		}
		else
			orderValue = Node::doDsqlPass(dsqlScratch, orderValue, false);

		if (collateNode)
		{
			// Finally apply collation order, if necessary.
			orderValue = CollateNode::pass1Collate(dsqlScratch, orderValue, collateNode->collation);
		}

		OrderNode* node2 = FB_NEW(pool) OrderNode(pool, orderValue);
		node2->descending = node1->descending;
		node2->nullsPlacement = node1->nullsPlacement;

		// store actual value to be ordered by
		*ptr2++ = node2;
	}

	return node;
}


// Handle a UNION of substreams, generating a mapping of all the fields and adding an implicit
// PROJECT clause to ensure that all the records returned are unique.
static RseNode* pass1_union(DsqlCompilerScratch* dsqlScratch, UnionSourceNode* input,
	ValueListNode* orderList, RowsClause* rows, bool updateLock, USHORT flags)
{
	DEV_BLKCHK(dsqlScratch, dsql_type_req);

	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	if (updateLock && !input->dsqlAll)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_dsql_wlock_conflict) << Arg::Str("UNION"));
	}

	// set up the rse node for the union.

	UnionSourceNode* unionSource = FB_NEW(pool) UnionSourceNode(pool);
	unionSource->dsqlAll = input->dsqlAll;
	unionSource->recursive = input->recursive;

	RseNode* unionRse = FB_NEW(pool) RseNode(pool);

	unionRse->dsqlStreams = FB_NEW(pool) RecSourceListNode(pool, 1);
	unionRse->dsqlStreams->items[0] = unionSource;
	unionSource->dsqlParentRse = unionRse;

	// generate a context for the union itself.
	dsql_ctx* union_context = FB_NEW(*tdbb->getDefaultPool()) dsql_ctx(*tdbb->getDefaultPool());

	if (input->recursive)
		union_context->ctx_context = dsqlScratch->recursiveCtxId;
	else
		union_context->ctx_context = dsqlScratch->contextNumber++;

	union_context->ctx_scope_level = dsqlScratch->scopeLevel;

	// When we're in a outer-join part mark context for it.
	if (dsqlScratch->inOuterJoin)
		union_context->ctx_flags |= CTX_outer_join;
	union_context->ctx_in_outer_join = dsqlScratch->inOuterJoin;

	dsqlScratch->context->push(union_context);

	unionSource->dsqlClauses = FB_NEW(pool) RecSourceListNode(pool,
		input->dsqlClauses->items.getCount());

	// process all the sub-rse's.
	{ // scope block
		NestConst<RecordSourceNode>* uptr = unionSource->dsqlClauses->items.begin();
		const DsqlContextStack::const_iterator base(*dsqlScratch->context);
		NestConst<RecordSourceNode>* ptr = input->dsqlClauses->items.begin();

		for (const NestConst<RecordSourceNode>* const end = input->dsqlClauses->items.end();
			 ptr != end;
			 ++ptr, ++uptr)
		{
			dsqlScratch->scopeLevel++;
			*uptr = pass1_rse(dsqlScratch, *ptr, NULL, NULL, updateLock, 0);
			dsqlScratch->scopeLevel--;

			if (updateLock)
				(*uptr)->as<RseNode>()->flags &= ~RseNode::FLAG_WRITELOCK;

			while (*(dsqlScratch->context) != base)
				dsqlScratch->unionContext.push(dsqlScratch->context->pop());

			// Push recursive context after initial select has been processed.
			// Corresponding pop occurs in pass1_derived_table
			if (input->recursive && (ptr == input->dsqlClauses->items.begin()))
				dsqlScratch->context->push(dsqlScratch->recursiveCtx);
		}
	} // end scope block

	// generate the list of fields to select.
	ValueListNode* items = unionSource->dsqlClauses->items[0]->as<RseNode>()->dsqlSelectList;

	// loop through the list nodes, checking to be sure that they have the
	// same number of items

	for (size_t i = 1; i < unionSource->dsqlClauses->items.getCount(); ++i)
	{
		const ValueListNode* nod1 = unionSource->dsqlClauses->items[i]->as<RseNode>()->dsqlSelectList;

		if (items->items.getCount() != nod1->items.getCount())
		{
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
	// Workaround: use a direct CAST() dsqlScratch in the SQL
	// dsqlScratch to force desired datatype.

	// loop through the list nodes and cast whenever possible.
	ValueListNode* tmp_list = FB_NEW(pool) ValueListNode(
		pool, unionSource->dsqlClauses->items.getCount());

	for (size_t j = 0; j < items->items.getCount(); ++j)
	{
		for (size_t i = 0; i < unionSource->dsqlClauses->items.getCount(); ++i)
		{
			ValueListNode* nod1 = unionSource->dsqlClauses->items[i]->as<RseNode>()->dsqlSelectList;
			MAKE_desc(dsqlScratch, &nod1->items[j]->nodDesc, nod1->items[j]);
			tmp_list->items[i] = nod1->items[j];

			// We look only at the items->nod_arg[] when creating the
			// output descriptors. Make sure that the sub-rses
			// descriptor flags are copied onto items->nod_arg[]->nodDesc.
			// Bug 65584.
			// -Sudesh 07/28/1999.
			if (i > 0)
			{
				if (nod1->items[j]->nodDesc.dsc_flags & DSC_nullable)
					items->items[j]->nodDesc.dsc_flags |= DSC_nullable;
			}
		}

		dsc desc;
		MAKE_desc_from_list(dsqlScratch, &desc, tmp_list, "UNION");
		// Only mark upper node as a NULL node when all sub-nodes are NULL
		items->items[j]->nodDesc.dsc_flags &= ~DSC_null;
		items->items[j]->nodDesc.dsc_flags |= (desc.dsc_flags & DSC_null);

		pass1_union_auto_cast(dsqlScratch, unionSource->dsqlClauses, desc, j);
	}

	items = unionSource->dsqlClauses->items[0]->as<RseNode>()->dsqlSelectList;

	// Create mappings for union.

	ValueListNode* union_items = FB_NEW(pool) ValueListNode(pool, items->items.getCount());

	{ // scope block
		USHORT count = 0;
		NestConst<ValueExprNode>* uptr = items->items.begin();
		NestConst<ValueExprNode>* ptr = union_items->items.begin();

		for (const NestConst<ValueExprNode>* const end = union_items->items.end(); ptr != end; ++ptr)
		{
			// Set up the dsql_map* between the sub-rses and the union context.
			dsql_map* map = FB_NEW(*tdbb->getDefaultPool()) dsql_map;
			map->map_position = count++;
			fb_assert(count != 0); // no wrap, please!
			map->map_node = *uptr++;
			map->map_next = union_context->ctx_map;
			map->map_partition = NULL;
			union_context->ctx_map = map;

		    *ptr = FB_NEW(pool) DsqlMapNode(pool, union_context, map);
		}

		unionRse->dsqlSelectList = union_items;
	} // end scope block

	// Process ORDER clause, if any.
	if (orderList)
	{
		ValueListNode* sort = FB_NEW(pool) ValueListNode(pool, orderList->items.getCount());
		NestConst<ValueExprNode>* uptr = sort->items.begin();
		NestConst<ValueExprNode>* ptr = orderList->items.begin();

		for (const NestConst<ValueExprNode>* const end = orderList->items.end();
			 ptr != end;
			 ++ptr, ++uptr)
		{
			OrderNode* order1 = (*ptr)->as<OrderNode>();
			const ValueExprNode* position = order1->value;
			const CollateNode* collateNode = position->as<CollateNode>();

			if (collateNode)
				position = collateNode->arg;

			const LiteralNode* literal = position->as<LiteralNode>();

			if (!literal || literal->litDesc.dsc_dtype != dtype_long)
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  // invalid ORDER BY clause.
						  Arg::Gds(isc_order_by_err));
			}

			const SLONG number = literal->getSlong();

			if (number < 1 || ULONG(number) > union_items->items.getCount())
			{
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  // invalid ORDER BY clause.
						  Arg::Gds(isc_order_by_err));
			}

			// make a new order node pointing at the Nth item in the select list.
			OrderNode* order2 = FB_NEW(pool) OrderNode(pool, union_items->items[number - 1]);
			*uptr = order2;
			order2->descending = order1->descending;

			if (collateNode)
			{
				order2->value = CollateNode::pass1Collate(dsqlScratch,
					order2->value, collateNode->collation);
			}

			order2->nullsPlacement = order1->nullsPlacement;
		}

		unionRse->dsqlOrder = sort;
	}

	if (rows)
		PASS1_limit(dsqlScratch, rows->length, rows->skip, unionRse);

	// PROJECT on all the select items unless UNION ALL was specified.
	if (!input->dsqlAll)
		unionRse->dsqlDistinct = union_items;

	if (updateLock)
		unionRse->flags |= RseNode::FLAG_WRITELOCK;

	unionRse->dsqlFlags = flags;

	return unionRse;
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

 **/
static void pass1_union_auto_cast(DsqlCompilerScratch* dsqlScratch, ExprNode* input,
	const dsc& desc, size_t position)
{
	thread_db* tdbb = JRD_get_thread_data();

	RecSourceListNode* recSourceList;
	RseNode* rseNode;
	UnionSourceNode* unionNode;

	if ((recSourceList = input->as<RecSourceListNode>()))
	{
		NestConst<RecordSourceNode>* ptr = recSourceList->items.begin();
		for (const NestConst<RecordSourceNode>* const end = recSourceList->items.end(); ptr != end; ++ptr)
			pass1_union_auto_cast(dsqlScratch, *ptr, desc, position);
	}
	else if ((rseNode = input->as<RseNode>()) && !rseNode->dsqlExplicitJoin &&
		!rseNode->dsqlContext)	// not derived table
	{
		pass1_union_auto_cast(dsqlScratch, rseNode->dsqlStreams, desc, position);

		if (rseNode->dsqlStreams->items.getCount() == 1 &&
			(unionNode = rseNode->dsqlStreams->items[0]->as<UnionSourceNode>()) &&
			unionNode->dsqlParentRse == rseNode)
		{
			// We're now in a UNION under a UNION so don't change the existing mappings.
			// Only replace the node where the map points to, because they could be changed.
			ValueListNode* sub_rse_items =
				unionNode->dsqlClauses->items[0]->as<RseNode>()->dsqlSelectList;
			dsql_map* map = rseNode->dsqlSelectList->items[position]->as<DsqlMapNode>()->map;
			map->map_node = sub_rse_items->items[position];
			rseNode->dsqlSelectList->items[position]->nodDesc = desc;
		}
		else
		{
			ValueListNode* list = rseNode->dsqlSelectList;

			if (position >= list->items.getCount())
			{
				// Internal dsql error: column position out of range in pass1_union_auto_cast
				ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
						  Arg::Gds(isc_dsql_command_err) <<
						  Arg::Gds(isc_dsql_auto_field_bad_pos));
			}
			else
			{
				ValueExprNode* select_item = list->items[position];
				MAKE_desc(dsqlScratch, &select_item->nodDesc, select_item);

				if (select_item->nodDesc.dsc_dtype != desc.dsc_dtype ||
					select_item->nodDesc.dsc_length != desc.dsc_length ||
					select_item->nodDesc.dsc_scale != desc.dsc_scale ||
					select_item->nodDesc.dsc_sub_type != desc.dsc_sub_type)
				{
					// Because this select item has a different descriptor then
					// our finally descriptor CAST it.
					CastNode* castNode = NULL;
					DsqlAliasNode* newAliasNode = NULL;
					DsqlAliasNode* aliasNode;
					DerivedFieldNode* derivedField;

					// Pick a existing cast if available else make a new one.
					if ((aliasNode = ExprNode::as<DsqlAliasNode>(select_item)) &&
						aliasNode->value && (castNode = aliasNode->value->as<CastNode>()))
					{
					}
					else if ((derivedField = ExprNode::as<DerivedFieldNode>(select_item)) &&
						(castNode = derivedField->value->as<CastNode>()))
					{
					}
					else if ((castNode = ExprNode::as<CastNode>(select_item)))
					{
					}
					else
					{
						castNode = FB_NEW(*tdbb->getDefaultPool()) CastNode(
							*tdbb->getDefaultPool());

						castNode->dsqlField = FB_NEW(*tdbb->getDefaultPool()) dsql_fld(
							*tdbb->getDefaultPool());

						// We want to leave the ALIAS node on his place, because a UNION
						// uses the select_items from the first sub-rse to determine the
						// columnname.
						if ((aliasNode = ExprNode::as<DsqlAliasNode>(select_item)))
							castNode->source = aliasNode->value;
						else if ((derivedField = ExprNode::as<DerivedFieldNode>(select_item)))
							castNode->source = derivedField->value;
						else
							castNode->source = select_item;

						// When a cast is created we're losing our fieldname, thus
						// create an alias to keep it.
						const ValueExprNode* name_node = select_item;
						const DsqlMapNode* mapNode;

						while ((mapNode = ExprNode::as<DsqlMapNode>(name_node)))
						{
							// Skip all the DsqlMapNodes.
							name_node = mapNode->map->map_node;
						}

						const FieldNode* fieldNode;

						if ((fieldNode = ExprNode::as<FieldNode>(name_node)))
						{
							// Create new node for alias and copy fieldname.
							newAliasNode = FB_NEW(*tdbb->getDefaultPool()) DsqlAliasNode(
								*tdbb->getDefaultPool(), fieldNode->dsqlField->fld_name, NULL);
							// The alias value will be assigned a bit later.
						}
					}

					dsql_fld* field = castNode->dsqlField;
					// Copy the descriptor to a field, because the gen_cast
					// uses a dsql field type.
					field->dtype = desc.dsc_dtype;
					field->scale = desc.dsc_scale;
					field->subType = desc.dsc_sub_type;
					field->length = desc.dsc_length;
					field->flags = (desc.dsc_flags & DSC_nullable) ? FLD_nullable : 0;

					if (desc.dsc_dtype <= dtype_any_text)
					{
						field->textType = desc.dsc_sub_type;
						field->charSetId = INTL_GET_CHARSET(&desc);
						field->collationId = INTL_GET_COLLATE(&desc);
					}
					else if (desc.dsc_dtype == dtype_blob)
					{
						field->charSetId = desc.dsc_scale;
						field->collationId = desc.dsc_flags >> 8;
					}

					// Finally copy the descriptors to the root nodes and swap
					// the necessary nodes.
					castNode->nodDesc = desc;

					if (select_item->nodDesc.dsc_flags & DSC_nullable)
						castNode->nodDesc.dsc_flags |= DSC_nullable;

					if ((aliasNode = ExprNode::as<DsqlAliasNode>(select_item)))
					{
						aliasNode->value = castNode;
						aliasNode->value->nodDesc = desc;
						select_item->nodDesc = desc;
					}
					else if ((derivedField = ExprNode::as<DerivedFieldNode>(select_item)))
					{
						derivedField->value = castNode;
						derivedField->value->nodDesc = desc;
						select_item->nodDesc = desc;
					}
					else
					{
						// If a new alias was created for keeping original field-name
						// make the alias the "top" node.
						if (newAliasNode)
						{
							newAliasNode->value = castNode;
							newAliasNode->value->nodDesc = castNode->nodDesc;
							list->items[position] = newAliasNode;
						}
						else
							list->items[position] = castNode;
					}
				}
			}
		}
	}
	else if ((unionNode = input->as<UnionSourceNode>()))
	{
		recSourceList = unionNode->dsqlClauses;

		for (NestConst<RecordSourceNode>* ptr = recSourceList->items.begin();
			 ptr != recSourceList->items.end();
			 ++ptr)
		{
			pass1_union_auto_cast(dsqlScratch, *ptr, desc, position);
		}
	}
}


// Post an item to a map for a context.
DsqlMapNode* PASS1_post_map(DsqlCompilerScratch* dsqlScratch, ValueExprNode* node, dsql_ctx* context,
	ValueListNode* partitionNode, ValueListNode* orderNode)
{
	DEV_BLKCHK(node, dsql_type_nod);
	DEV_BLKCHK(context, dsql_type_ctx);

	thread_db* tdbb = JRD_get_thread_data();

	PartitionMap* partitionMap = NULL;
	dsql_map* map = NULL;

	if (dsqlScratch->processingWindow)
	{
		partitionMap = context->getPartitionMap(dsqlScratch, partitionNode, orderNode);
		map = partitionMap->map;
	}
	else
		map = context->ctx_map;

	USHORT count = 0;

	while (map)
	{
		if (PASS1_node_match(node, map->map_node, false))
			break;

		++count;
		map = map->map_next;
	}

	if (!map)
	{
		dsql_map** next = partitionMap ? &partitionMap->map : &context->ctx_map;

		if (*next)
		{
			while (*(next = &(*next)->map_next))
				;
		}

		map = *next = FB_NEW(*tdbb->getDefaultPool()) dsql_map;
		map->map_position = count;
		map->map_node = node;
		map->map_partition = partitionMap;
	}

	MAKE_desc(dsqlScratch, &node->nodDesc, node);

	return FB_NEW(*tdbb->getDefaultPool()) DsqlMapNode(*tdbb->getDefaultPool(), context, map);
}


// For each relation in the list, flag the relation's context as having a parent context.
// Be sure to handle joins (Bug 6674).
static void remap_streams_to_parent_context(ExprNode* input, dsql_ctx* parent_context)
{
	DEV_BLKCHK(parent_context, dsql_type_ctx);

	RecSourceListNode* listNode;
	ProcedureSourceNode* procNode;
	RelationSourceNode* relNode;
	RseNode* rseNode;
	UnionSourceNode* unionNode;

	if ((listNode = input->as<RecSourceListNode>()))
	{
		NestConst<RecordSourceNode>* ptr = listNode->items.begin();
		for (const NestConst<RecordSourceNode>* const end = listNode->items.end(); ptr != end; ++ptr)
			remap_streams_to_parent_context(*ptr, parent_context);
	}
	else if ((procNode = input->as<ProcedureSourceNode>()))
	{
		DEV_BLKCHK(procNode->dsqlContext, dsql_type_ctx);
		procNode->dsqlContext->ctx_parent = parent_context;
	}
	else if ((relNode = input->as<RelationSourceNode>()))
	{
		DEV_BLKCHK(relNode->dsqlContext, dsql_type_ctx);
		relNode->dsqlContext->ctx_parent = parent_context;
	}
	else if ((rseNode = input->as<RseNode>()))
		remap_streams_to_parent_context(rseNode->dsqlStreams, parent_context);
	else if ((unionNode = input->as<UnionSourceNode>()))
		remap_streams_to_parent_context(unionNode->dsqlClauses, parent_context);
	else
		fb_assert(input->as<AggregateSourceNode>());
}


// Setup the datatype of a parameter.
bool PASS1_set_parameter_type(DsqlCompilerScratch* dsqlScratch, ValueExprNode* inNode,
	const dsc* desc, bool force_varchar)
{
	return inNode && inNode->setParameterType(dsqlScratch, desc, force_varchar);
}

// Setup the datatype of a parameter.
bool PASS1_set_parameter_type(DsqlCompilerScratch* dsqlScratch, ValueExprNode* inNode,
	ValueExprNode* node, bool force_varchar)
{
	if (!inNode)
		return false;

	MAKE_desc(dsqlScratch, &node->nodDesc, node);
	return inNode->setParameterType(dsqlScratch, &node->nodDesc, force_varchar);
}


// Returns false for hidden fields and true for non-hidden.
// For non-hidden, change "node" if the field is part of an
// implicit join.
bool dsql_ctx::getImplicitJoinField(const MetaName& name, NestConst<ValueExprNode>& node)
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

// Returns (creating, if necessary) the PartitionMap of a given partition (that may be NULL).
PartitionMap* dsql_ctx::getPartitionMap(DsqlCompilerScratch* dsqlScratch, ValueListNode* partitionNode,
	ValueListNode* orderNode)
{
	thread_db* tdbb = JRD_get_thread_data();

	PartitionMap* partitionMap = NULL;

	for (Array<PartitionMap*>::iterator i = ctx_win_maps.begin();
		 !partitionMap && i != ctx_win_maps.end();
		 ++i)
	{
		if (PASS1_node_match((*i)->partition, partitionNode, false) &&
			PASS1_node_match((*i)->order, orderNode, false))
		{
			partitionMap = *i;
		}
	}

	if (!partitionMap)
	{
		partitionMap = FB_NEW(*tdbb->getDefaultPool()) PartitionMap(partitionNode, orderNode);
		ctx_win_maps.add(partitionMap);
		partitionMap->context = dsqlScratch->contextNumber++;
	}

	return partitionMap;
}
