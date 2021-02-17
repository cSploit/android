/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		expand.cpp
 *	DESCRIPTION:	Expand syntax tree -- first phase of compiler
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
 */

#include "firebird.h"
#include <string.h>
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../qli/compile.h"
#include "../qli/exe.h"
#include "../qli/report.h"
#include "../qli/all_proto.h"
#include "../qli/comma_proto.h"
#include "../qli/compi_proto.h"
#include "../qli/err_proto.h"
#include "../qli/expan_proto.h"
#include "../qli/help_proto.h"
#include "../qli/meta_proto.h"
#include "../qli/show_proto.h"

using MsgFormat::SafeArg;


static bool compare_names(const qli_name*, const qli_symbol*);
static bool compare_symbols(const qli_symbol*, const qli_symbol*);
static qli_symbol* copy_symbol(const qli_symbol*);
static void declare_global(qli_fld*, qli_syntax*);
static qli_syntax* decompile_field(qli_fld*, qli_ctx*);
static qli_name* decompile_symbol(qli_symbol*);
static qli_nod* expand_assignment(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_any(qli_syntax*, qli_lls*);
static qli_nod* expand_boolean(qli_syntax*, qli_lls*);
static void expand_control_break(qli_brk**, qli_lls*);
static void expand_distinct(qli_nod*, qli_nod*);
static void expand_edit_string(qli_nod*, qli_print_item*);
static qli_nod* expand_erase(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_expression(qli_syntax*, qli_lls*);
static qli_nod* expand_field(qli_syntax*, qli_lls*, qli_syntax*);
static qli_nod* expand_for(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_function(qli_syntax*, qli_lls*);
static qli_nod* expand_group_by(qli_syntax*, qli_lls*, qli_ctx*);
static qli_nod* expand_modify(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_output(qli_syntax*, qli_lls*, qli_prt**);
static qli_nod* expand_print(qli_syntax*, qli_lls*, qli_lls*);
static qli_print_item* expand_print_item(qli_syntax*, qli_lls*);
static qli_nod* expand_print_list(qli_syntax*, qli_lls*);
static qli_nod* expand_report(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_restructure(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_rse(qli_syntax*, qli_lls**);
static qli_nod* expand_sort(qli_syntax*, qli_lls*, qli_nod*);
static qli_nod* expand_statement(qli_syntax*, qli_lls*, qli_lls*);
static qli_nod* expand_store(qli_syntax*, qli_lls*, qli_lls*);
static void expand_values(qli_syntax*, qli_lls*);
static qli_ctx* find_context(const qli_name*, qli_lls*);
static int generate_fields(qli_ctx*, qli_lls*, qli_syntax*);
static int generate_items(const qli_syntax*, qli_lls*, qli_lls*, qli_nod*);
static bool global_agg(const qli_syntax*, const qli_syntax*);
static bool invalid_nod_field(const qli_nod*, const qli_nod*);
static bool invalid_syn_field(const qli_syntax*, const qli_syntax*);
static qli_nod* make_and(qli_nod*, qli_nod*);
static qli_nod* make_assignment(qli_nod*, qli_nod*, qli_lls*);
static qli_nod* make_field(qli_fld*, qli_ctx*);
static qli_nod* make_list(qli_lls*);
static qli_nod* make_node(nod_t, USHORT);
static qli_nod* negate(qli_nod*);
static qli_nod* possible_literal(qli_syntax*, qli_lls*, bool);
static qli_nod* post_map(qli_nod*, qli_ctx*);
static qli_fld* resolve(qli_syntax*, qli_lls*, qli_ctx**);
static void resolve_really(qli_fld*, const qli_syntax*);

static qli_lls* global_output_stack;


qli_nod* EXP_expand( qli_syntax* node)
{
/**************************************
 *
 *	E X P _ e x p a n d
 *
 **************************************
 *
 * Functional description
 *	Expand a syntax tree into something richer and more complete.
 *
 **************************************/
	switch (node->syn_type)
	{
	case nod_commit:
	case nod_prepare:
	case nod_rollback:
		CMD_transaction(node);
		return NULL;

	case nod_copy_proc:
		CMD_copy_procedure(node);
		return NULL;

	case nod_declare:
		declare_global((qli_fld*) node->syn_arg[0], node->syn_arg[1]);
		return NULL;

	case nod_define:
		CMD_define_procedure(node);
		return NULL;

	case nod_delete_proc:
		CMD_delete_proc(node);
		return NULL;

	case nod_def_database:
	case nod_sql_database:
		MET_ready(node, true);
		return NULL;

	case nod_def_field:
		MET_define_field((qli_dbb*) node->syn_arg[0], (qli_fld*) node->syn_arg[1]);
		return NULL;

	case nod_def_index:
		MET_define_index(node);
		return NULL;

	case nod_def_relation:
		MET_define_relation((qli_rel*)node->syn_arg[0], (qli_rel*) node->syn_arg[1]);
		return NULL;

	case nod_del_relation:
		MET_delete_relation((qli_rel*)node->syn_arg[0]);
		return NULL;

	case nod_del_field:
		MET_delete_field((qli_dbb*)node->syn_arg[0], (qli_name*) node->syn_arg[1]);
		return NULL;

	case nod_del_index:
		MET_delete_index((qli_dbb*)node->syn_arg[0], (qli_name*) node->syn_arg[1]);
		return NULL;

	case nod_del_database:
		MET_delete_database((qli_dbb*)node->syn_arg[0]);
		return NULL;

	case nod_edit_proc:
		CMD_edit_proc(node);
		return NULL;
	case nod_extract:
		node->syn_arg[1] = (qli_syntax*) expand_output(node->syn_arg[1], 0, 0);
		CMD_extract(node);
		return NULL;

	case nod_finish:
		CMD_finish(node);
		return NULL;

	case nod_help:
		HELP_help(node);
		return NULL;

	case nod_mod_field:
		MET_modify_field((qli_dbb*)node->syn_arg[0], (qli_fld*) node->syn_arg[1]);
		return NULL;

	case nod_mod_relation:
		MET_modify_relation((qli_rel*) node->syn_arg[0], (qli_fld*) node->syn_arg[1]);
		return NULL;

	case nod_mod_index:
		MET_modify_index(node);
		return NULL;

	case nod_ready:
		MET_ready(node, false);
		return NULL;

	case nod_rename_proc:
		CMD_rename_proc(node);
		return NULL;

	case nod_set:
		CMD_set(node);
		return NULL;

	case nod_show:
		SHOW_stuff(node);
		return NULL;

	case nod_shell:
		CMD_shell(node);
		return NULL;

	case nod_sql_grant:
		MET_sql_grant(node);
		return NULL;

	case nod_sql_revoke:
		MET_sql_revoke(node);
		return NULL;

	case nod_sql_cr_table:
		MET_define_sql_relation((qli_rel*) node->syn_arg[0]);
		return NULL;

    //case nod_sql_cr_view:
	//	MET_sql_cr_view (node);
	//	GEN_release();
	//	return NULL;

	case nod_sql_al_table:
		MET_sql_alter_table((qli_rel*) node->syn_arg[0], (qli_fld*) node->syn_arg[1]);
		return NULL;
	} // end switch, no default case for error

	// If there are any variables, make up a context now

	global_output_stack = NULL;
	qli_lls* right = NULL;
	qli_lls* left = NULL;

	if (QLI_variables)
	{
		qli_ctx* context = (qli_ctx*) ALLOCD(type_ctx);
		context->ctx_type = CTX_VARIABLE;
		context->ctx_variable = QLI_variables;
		ALLQ_push((blk*) context, &right);
		ALLQ_push((blk*) context, &left);
	}

	qli_nod* expanded = expand_statement(node, right, left);
	if (!expanded)
		return NULL;

	while (global_output_stack)
	{
		qli_nod* output = (qli_nod*) ALLQ_pop(&global_output_stack);
		output->nod_arg[e_out_statement] = expanded;
		expanded = output;
	}

	return expanded;
}


static bool compare_names( const qli_name* name, const qli_symbol* symbol)
{
/**************************************
 *
 *	c o m p a r e _ n a m e s
 *
 **************************************
 *
 * Functional description
 *	Compare a name node to a symbol.  If they are equal, return true.
 *
 **************************************/
	if (!symbol)
		return false;

	const int l = name->nam_length;
	if (l != symbol->sym_length)
		return false;

	if (l)
		return memcmp(symbol->sym_string, name->nam_string, l) == 0;

	return true;
}


static bool compare_symbols( const qli_symbol* symbol1, const qli_symbol* symbol2)
{
/**************************************
 *
 *	c o m p a r e _ s y m b o l s
 *
 **************************************
 *
 * Functional description
 *	Compare two symbols (either may be 0).
 *
 **************************************/
	if (!symbol1 || !symbol2)
		return false;

	const int l = symbol1->sym_length;
	if (l != symbol2->sym_length)
		return false;

	if (l)
		return memcmp(symbol1->sym_string, symbol2->sym_string, l) == 0;

	return true;
}


static qli_symbol* copy_symbol( const qli_symbol* old)
{
/**************************************
 *
 *	c o p y _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *	Copy a symbol into the permanent pool.
 *
 **************************************/
	qli_symbol* new_sym = (qli_symbol*) ALLOCPV(type_sym, old->sym_length);
	new_sym->sym_length = old->sym_length;
	new_sym->sym_type = old->sym_type;
	new_sym->sym_string = new_sym->sym_name;
	strcpy(new_sym->sym_name, old->sym_name);

	return new_sym;
}


static void declare_global( qli_fld* variable, qli_syntax* field_node)
{
/**************************************
 *
 *	d e c l a r e _ g l o b a l
 *
 **************************************
 *
 * Functional description
 *	Copy a variable block into the permanent pool as a field.
 *	Resolve BASED_ON references.
 *	Allocate all strings off the field block.
 *
 **************************************/

	// If it's based_on, flesh it out & check datatype.

	if (field_node)
	{
		if (field_node->syn_type == nod_index)
			field_node = field_node->syn_arg[s_idx_field];
		resolve_really(variable, field_node);
		if (variable->fld_dtype == dtype_blob)
			IBERROR(137);		// Msg137 variables may not be based on blob fields.
	}

	// Get rid of any other variables of the same name

	qli_fld* field;
	for (qli_fld** ptr = &QLI_variables; field = *ptr; ptr = &field->fld_next)
		if (!strcmp(field->fld_name->sym_string, variable->fld_name->sym_string))
		{
			*ptr = field->fld_next;
			ALLQ_release((qli_frb*) field->fld_name);
			if (field->fld_query_name)
				ALLQ_release((qli_frb*) field->fld_query_name);
			ALLQ_release((qli_frb*) field);
			break;
		}

	// Next, copy temporary field block into permanent pool.  Fold edit_string
	//   query_header into main block to save space and complexity.

	const TEXT* q;
	USHORT l = variable->fld_length;
	if (q = variable->fld_edit_string)
		l += strlen(q);
	if (q = variable->fld_query_header)
		l += strlen(q);

	qli_fld* new_fld = (qli_fld*) ALLOCPV(type_fld, l);
	new_fld->fld_name = copy_symbol(variable->fld_name);
	new_fld->fld_dtype = variable->fld_dtype;
	new_fld->fld_length = variable->fld_length;
	new_fld->fld_scale = variable->fld_scale;
	new_fld->fld_sub_type = variable->fld_sub_type;
	new_fld->fld_sub_type_missing = variable->fld_sub_type_missing;
	new_fld->fld_flags = variable->fld_flags | FLD_missing;

	// Copy query_name, edit string, query header

	TEXT* p = (TEXT*) new_fld->fld_data + new_fld->fld_length;
	if (q = variable->fld_edit_string)
	{
		new_fld->fld_edit_string = p;
		while (*p++ = *q++);
	}
	if (variable->fld_query_name)
		new_fld->fld_query_name = copy_symbol(variable->fld_query_name);
	if (q = variable->fld_query_header)
	{
		new_fld->fld_query_header = p;
		while (*p++ = *q++);
	}

	// Link new variable into variable chain

	new_fld->fld_next = QLI_variables;
	QLI_variables = new_fld;
}


static qli_syntax* decompile_field( qli_fld* field, qli_ctx* context)
{
/**************************************
 *
 *	d e c o m p i l e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Take a perfectly good, completely compiled
 *	field block and regress to a qli_syntax node and
 *	and a qli_name block.
 *	(Needed to support SQL idiocies)
 *
 **************************************/
	const int args = context ? 2 : 1;

	qli_syntax* node = (qli_syntax*) ALLOCDV(type_syn, args);
	node->syn_type = nod_field;
	node->syn_count = args;

	qli_name* name = decompile_symbol(field->fld_name);
	node->syn_arg[0] = (qli_syntax*) name;

	if (context)
	{
		node->syn_arg[1] = node->syn_arg[0];
		if (context->ctx_symbol)
			name = decompile_symbol(context->ctx_symbol);
		else
			name = decompile_symbol(context->ctx_relation->rel_symbol);
		node->syn_arg[0] = (qli_syntax*) name;
	}

	return node;
}


static qli_name* decompile_symbol( qli_symbol* symbol)
{
/**************************************
 *
 *	d e c o m p i l e _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *	Turn a symbol back into a name
 *	(Needed to support SQL idiocies)
 *
 **************************************/
	const int l = symbol->sym_length;

	qli_name* name = (qli_name*) ALLOCDV(type_nam, l);
	name->nam_length = l;
	name->nam_symbol = symbol;
	if (l)
		memcpy(name->nam_string, symbol->sym_string, l);

	/*
	TEXT* p = name->nam_string;
	const TEXT* q = symbol->sym_string;

	if (l)
		do {
			const TEXT c = *q++;
			*p++ = c; //UPPER(c);

		} while (--l);
	*/

	return name;
}


static qli_nod* expand_assignment( qli_syntax* input, qli_lls* right, qli_lls* left)
{
/**************************************
 *
 *	e x p a n d _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Expand an assigment statement.  All in all, not too tough.
 *
 **************************************/
	qli_nod* node = make_node(input->syn_type, e_asn_count);
	qli_nod* to = expand_expression(input->syn_arg[s_asn_to], left);
	node->nod_arg[e_asn_to] = to;
	qli_nod* from = expand_expression(input->syn_arg[s_asn_from], right);
	node->nod_arg[e_asn_from] = from;

	if (to->nod_type == nod_field || to->nod_type == nod_variable)
	{
		qli_fld* field = (qli_fld*) to->nod_arg[e_fld_field];
		if (field->fld_flags & FLD_computed)
		{
			ERRQ_print_error(138, field->fld_name->sym_string);
			// Msg138 can't do assignment to computed field
		}
		if (from->nod_type == nod_prompt)
			from->nod_arg[e_prm_field] = to->nod_arg[e_fld_field];
		if (field->fld_validation)
			node->nod_arg[e_asn_valid] = expand_expression(field->fld_validation, left);
	}

	if (!node->nod_arg[e_asn_valid])
		--node->nod_count;

	return node;
}


static qli_nod* expand_any( qli_syntax* input, qli_lls* stack)
{
/**************************************
 *
 *	e x p a n d _ a n y
 *
 **************************************
 *
 * Functional description
 *	Expand an any expression.  This would be trivial were it not
 *	for a funny SQL case when an expression needs to be checked
 *	for existence.
 *
 **************************************/
	qli_nod* node = make_node(input->syn_type, e_any_count);
	node->nod_count = 0;
	qli_nod* rse = expand_rse(input->syn_arg[0], &stack);
	node->nod_arg[e_any_rse] = rse;

	if (input->syn_count >= 2 && input->syn_arg[1])
	{
		qli_nod* boolean = make_node(nod_missing, 1);
		boolean->nod_arg[0] = expand_expression(input->syn_arg[1], stack);
		qli_nod* negation = make_node(nod_not, 1);
		negation->nod_arg[0] = boolean;
		rse->nod_arg[e_rse_boolean] = make_and(rse->nod_arg[e_rse_boolean], negation);
	}

	return node;
}


static qli_nod* expand_boolean( qli_syntax* input, qli_lls* stack)
{
/**************************************
 *
 *	e x p a n d _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	// Make node and process arguments

	qli_nod* node = make_node(input->syn_type, input->syn_count);
	qli_nod** ptr = node->nod_arg;
	qli_nod* value = expand_expression(input->syn_arg[0], stack);
	*ptr++ = value;

	for (int i = 1; i < input->syn_count; i++, ptr++)
	{
		if (!(*ptr = possible_literal(input->syn_arg[i], stack, true)))
			*ptr = expand_expression(input->syn_arg[i], stack);
	}

	// Try to match any prompts against fields to determine prompt length

	if (value->nod_type != nod_field)
		return node;

	qli_fld* field = (qli_fld*) value->nod_arg[e_fld_field];
	ptr = &node->nod_arg[1];

	for (int i = 1; i < node->nod_count; i++, ptr++)
	{
		if ((*ptr)->nod_type == nod_prompt)
			(*ptr)->nod_arg[e_prm_field] = (qli_nod*) field;
	}

	return node;
}


static void expand_control_break( qli_brk** ptr, qli_lls* right)
{
/**************************************
 *
 *	e x p a n d _ c o n t r o l _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Work on a report writer control break.  This is called recursively
 *	to handle multiple breaks.
 *
 **************************************/
	qli_brk* list = NULL;

	qli_brk* control;
	while (control = *ptr)
	{
		*ptr = control->brk_next;
		control->brk_next = list;
		list = control;
		if (control->brk_field)
			control->brk_field = (qli_syntax*) expand_expression(control->brk_field, right);
		if (control->brk_line)
			control->brk_line = (qli_syntax*) expand_print_list(control->brk_line, right);
	}

	*ptr = list;
}


static void expand_distinct( qli_nod* rse, qli_nod* node)
{
/**************************************
 *
 *	e x p a n d _ d i s t i n c t
 *
 **************************************
 *
 * Functional description
 *	We have run into a distinct count.  Add a reduced
 *	clause to it's parent.
 *
 **************************************/
	if (rse->nod_arg[e_rse_reduced])
		return;

	qli_lls* stack = NULL;
	ALLQ_push((blk*) node, &stack);
	ALLQ_push(0, &stack);
	qli_nod* list = make_list(stack);
	rse->nod_arg[e_rse_reduced] = list;
	list->nod_count = 1;
}


static void expand_edit_string( qli_nod* node, qli_print_item* item)
{
/**************************************
 *
 *	e x p a n d _ e d i t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Default edit_string and query_header.
 *
 **************************************/
	switch (node->nod_type)
	{
	case nod_min:
	case nod_rpt_min:
	case nod_agg_min:
		if (!item->itm_query_header)
			item->itm_query_header = "MIN";

	case nod_total:
	case nod_running_total:
	case nod_rpt_total:
	case nod_agg_total:
		if (!item->itm_query_header)
			item->itm_query_header = "TOTAL";

	case nod_average:
	case nod_rpt_average:
	case nod_agg_average:
		if (!item->itm_query_header)
			item->itm_query_header = "AVG";

	case nod_max:
	case nod_rpt_max:
	case nod_agg_max:
		if (!item->itm_query_header)
			item->itm_query_header = "MAX";
		expand_edit_string(node->nod_arg[e_stt_value], item);
		return;

	case nod_count:
	case nod_running_count:
	case nod_rpt_count:
	case nod_agg_count:
		if (!item->itm_edit_string)
			item->itm_edit_string = "ZZZ,ZZZ,ZZ9";
		if (!item->itm_query_header)
			item->itm_query_header = "COUNT";
		break;

	case nod_map:
		{
			qli_map* map = (qli_map*) node->nod_arg[e_map_map];
			expand_edit_string(map->map_node, item);
		}
		return;

	case nod_field:
	case nod_variable:
		break;

	case nod_function:
		{
			qli_fun* function = (qli_fun*) node->nod_arg[e_fun_function];
			if (!item->itm_query_header)
				item->itm_query_header = function->fun_symbol->sym_string;
		}
		return;

	default:
		return;
	}

	// Handle fields

	qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];

	if (!item->itm_edit_string)
		item->itm_edit_string = field->fld_edit_string;

	if (!item->itm_query_header)
		if (!(item->itm_query_header = field->fld_query_header))
			item->itm_query_header = field->fld_name->sym_string;
}


static qli_nod* expand_erase( qli_syntax* input, qli_lls* right, qli_lls* /*left*/)
{
/**************************************
 *
 *	e x p a n d _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	qli_nod* loop = NULL;

	// If there is an rse, make up a FOR loop

	if (input->syn_arg[s_era_rse])
	{
		loop = make_node(nod_for, e_for_count);
		loop->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_era_rse], &right);
	}

	// Loop thru contexts counting them.
	int count = 0;
	qli_ctx* context = NULL;
	for (qli_lls* contexts = right; contexts; contexts = contexts->lls_next)
	{
		context = (qli_ctx*) contexts->lls_object;
		if (context->ctx_variable)
			continue;
		count++;
		if (context->ctx_rse)
			break;
	}

	if (count == 0)
		IBERROR(139);			// Msg139 no context for ERASE
	else if (count > 1)
		IBERROR(140);			// Msg140 can't erase from a join

	// Make up node big enough to hold fixed fields plus all contexts

	qli_nod* node = make_node(nod_erase, e_era_count);
	node->nod_arg[e_era_context] = (qli_nod*) context;

	if (!loop)
		return node;

	loop->nod_arg[e_for_statement] = node;

	return loop;
}


static qli_nod* expand_expression( qli_syntax* input, qli_lls* stack)
{
/**************************************
 *
 *	e x p a n d _ e x p r e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Expand an expression.
 *
 **************************************/
	qli_nod* node;
	qli_ctx* context;
	qli_syntax* value;

	switch (input->syn_type)
	{
	case nod_field:
		return expand_field(input, stack, 0);

	case nod_null:
	case nod_user_name:
		return make_node(input->syn_type, 0);

	case nod_any:
	case nod_unique:
		return expand_any(input, stack);

	case nod_max:
	case nod_min:
	case nod_count:
	case nod_average:
	case nod_total:
	case nod_from:

	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_count:
	case nod_rpt_average:
	case nod_rpt_total:

	case nod_running_total:
	case nod_running_count:
		node = make_node(input->syn_type, e_stt_count);
		if (value = input->syn_arg[s_stt_rse])
			node->nod_arg[e_stt_rse] = expand_rse(value, &stack);
		if (value = input->syn_arg[s_stt_value])
			node->nod_arg[e_stt_value] = expand_expression(value, stack);
		if (value = input->syn_arg[s_stt_default])
			node->nod_arg[e_stt_default] = expand_expression(value, stack);
		if (input->syn_arg[s_prt_distinct] && node->nod_arg[e_stt_rse] && node->nod_arg[e_stt_value])
		{
			expand_distinct(node->nod_arg[e_stt_rse], node->nod_arg[e_stt_value]);
		}
		// count2 next 2 lines go
		if (input->syn_type == nod_count)
			node->nod_arg[e_stt_value] = 0;
		return node;

	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_count:
	case nod_agg_average:
	case nod_agg_total:
		node = make_node(input->syn_type, e_stt_count);
		for (; stack; stack = stack->lls_next)
		{
			context = (qli_ctx*) stack->lls_object;
			if (context->ctx_type == CTX_AGGREGATE)
				break;
		}
		if (!stack)
			ERRQ_print_error(454);
			// could not resolve context for aggregate
/* count2
	if (value = input->syn_arg [s_stt_value])
	    {
	    node->nod_arg [e_stt_value] = expand_expression (value, stack->lls_next);
	    if (input->syn_arg [s_prt_distinct])
		expand_distinct (context->ctx_sub_rse, node->nod_arg [e_stt_value]);
	    }
*/
		if ((value = input->syn_arg[s_stt_value]) &&
			(input->syn_arg[s_prt_distinct] || (input->syn_type != nod_agg_count)))
		{
			node->nod_arg[e_stt_value] = expand_expression(value, stack->lls_next);
			if (input->syn_arg[s_prt_distinct] ||
				(input->syn_type == nod_agg_count && context->ctx_sub_rse))
			{
				expand_distinct(context->ctx_sub_rse, node->nod_arg[e_stt_value]);
			}
		}
		return post_map(node, context);

	case nod_index:
		value = input->syn_arg[s_idx_field];
		if (value->syn_type != nod_field)
			IBERROR(466);		// Msg466 Only fields may be subscripted
		return expand_field(value, stack, input->syn_arg[s_idx_subs]);

	case nod_list:
	case nod_upcase:
	case nod_lowcase:

	case nod_and:
	case nod_or:
	case nod_not:
	case nod_missing:
	case nod_add:
	case nod_subtract:
	case nod_multiply:
	case nod_divide:
	case nod_negate:
	case nod_concatenate:
	case nod_substr:
		break;

	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_leq:
	case nod_lss:
	case nod_between:
	case nod_matches:
	case nod_sleuth:
	case nod_like:
	case nod_starts:
	case nod_containing:
		return expand_boolean(input, stack);

	case nod_edit_blob:
		node = make_node(input->syn_type, e_edt_count);
		node->nod_count = 0;
		if (input->syn_arg[0])
		{
			node->nod_count = 1;
			node->nod_arg[0] = expand_expression(input->syn_arg[0], stack);
		}
		return node;

	case nod_format:
		node = make_node(input->syn_type, e_fmt_count);
		node->nod_count = 1;
		node->nod_arg[e_fmt_value] = expand_expression(input->syn_arg[s_fmt_value], stack);
		node->nod_arg[e_fmt_edit] = (qli_nod*) input->syn_arg[s_fmt_edit];
		return node;

	case nod_function:
		return expand_function(input, stack);

	case nod_constant:
		{
			node = make_node(input->syn_type, 0);
			qli_const* constant = (qli_const*) input->syn_arg[0];
			node->nod_desc = constant->con_desc;
		}
		return node;

	case nod_prompt:
		node = make_node(input->syn_type, e_prm_count);
		node->nod_arg[e_prm_prompt] = (qli_nod*) input->syn_arg[0];
		return node;

	case nod_star:
		{
			qli_name* name = (qli_name*) input->syn_arg[0];
			ERRQ_print_error(141, name->nam_string);
			// Msg141 can't be used when a single element is required
		}

	default:
		ERRQ_bugcheck(135);			// Msg135 expand_expression: not yet implemented
	}

	node = make_node(input->syn_type, input->syn_count);
	qli_nod** ptr = node->nod_arg;

	for (int i = 0; i < input->syn_count; i++)
		*ptr++ = expand_expression(input->syn_arg[i], stack);

	return node;
}


static qli_nod* expand_field( qli_syntax* input, qli_lls* stack, qli_syntax* subs)
{
/**************************************
 *
 *	e x p a n d _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Expand a field reference.  Error if it can't be resolved.
 *	If the field belongs to a group by SQL expression, make
 *	sure it goes there.
 *
 **************************************/
	qli_ctx* context;

	qli_fld* field = resolve(input, stack, &context);
	if (!field || (subs && (context->ctx_variable)))
	{
	    TEXT s[160];
		TEXT* p = s;
		const TEXT* const limit = p + sizeof(s) - 1;
		for (USHORT i = 0; i < input->syn_count; i++)
		{
			qli_name* name = (qli_name*) input->syn_arg[i];
			const TEXT* q = name->nam_string;
			USHORT l = name->nam_length;
			if (p < limit)
			{
				if (l)
					do {
						*p++ = *q++;
					} while (--l && p < limit);
				*p++ = '.';
			}
		}
		*--p = 0;
		if (field)
			ERRQ_print_error(467, s);
			// Msg467 "%s" is not a field and so may not be subscripted
		else
			ERRQ_print_error(142, s);
			// Msg142 "%s" is undefined or used out of context
	}

	qli_nod* node = make_field(field, context);
	if (subs)
		node->nod_arg[e_fld_subs] = expand_expression(subs, stack);

	qli_ctx* parent = NULL;
	qli_lls* save_stack = stack;
	for (; stack; stack = stack->lls_next)
	{
		parent = (qli_ctx*) stack->lls_object;
		if (parent->ctx_type == CTX_AGGREGATE)
			break;
	}

	if (!parent)
		return node;

	if (context->ctx_parent != parent)
	{
		// The parent context may be hidden because we are part of
		// a stream context.  Check out this possibility.

		for (; save_stack; save_stack = save_stack->lls_next)
		{
			qli_ctx* stream_context = (qli_ctx*) save_stack->lls_object;
			if (stream_context->ctx_type != CTX_STREAM ||
				stream_context->ctx_stream->nod_type != nod_rse)
			{
				continue;
			}

			qli_ctx** ptr = (qli_ctx**) stream_context->ctx_stream->nod_arg + e_rse_count;
			const qli_ctx* const* const end = ptr + stream_context->ctx_stream->nod_count;
			for (; ptr < end; ptr++)
				if (*ptr == context)
					break;
			if (ptr < end && stream_context->ctx_parent == parent)
				break;
		}

		if (!save_stack)
			return node;
	}

	return post_map(node, parent);
}


static qli_nod* expand_for( qli_syntax* input, qli_lls* right, qli_lls* left)
{
/**************************************
 *
 *	e x p a n d _ f o r
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	qli_nod* node = make_node(input->syn_type, e_for_count);
	node->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_for_rse], &right);
	node->nod_arg[e_for_statement] = expand_statement(input->syn_arg[s_for_statement], right, left);

	return node;
}


static qli_nod* expand_function( qli_syntax* input, qli_lls* stack)
{
/**************************************
 *
 *	e x p a n d _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Expand a functionn reference.
 *	For a field or expression reference
 *	tied to a relation in a database
 *	use only that database.  For a variable
 *      reference, use any database that matches.
 *
 **************************************/
	qli_symbol* symbol = 0;
	qli_fun* function = 0;
	qli_dbb* database = 0;

	qli_nod* node = make_node(input->syn_type, e_fun_count);
	node->nod_count = 1;
	qli_ctx* context;
	if (stack && (context = (qli_ctx*) stack->lls_object) && (context->ctx_type == CTX_RELATION))
	{
		if (context->ctx_primary)
			context = context->ctx_primary;
		database = context->ctx_relation->rel_database;
		for (symbol = (qli_symbol*) input->syn_arg[s_fun_function]; symbol;
			symbol = symbol->sym_homonym)
		{
			if (symbol->sym_type == SYM_function)
			{
				function = (qli_fun*) symbol->sym_object;
				if (function->fun_database == database)
					break;
			}
		}
	}
	else
		for (database = QLI_databases; database; database = database->dbb_next)
		{
			for (symbol = (qli_symbol*) input->syn_arg[s_fun_function]; symbol;
				 symbol = symbol->sym_homonym)
			{
				if (symbol->sym_type == SYM_function)
				{
					function = (qli_fun*) symbol->sym_object;
					if (function->fun_database == database)
						break;
				}
			}
			if (symbol)
				break;
		}


	if (!symbol)
	{
		symbol = (qli_symbol*) input->syn_arg[s_fun_function];
		ERRQ_error(412, SafeArg() << symbol->sym_string << database->dbb_filename);
	}

	node->nod_arg[e_fun_function] = (qli_nod*) function;

	node->nod_arg[e_fun_args] = expand_expression(input->syn_arg[s_fun_args], stack);

	return node;
}


static qli_nod* expand_group_by( qli_syntax* input, qli_lls* stack, qli_ctx* context)
{
/**************************************
 *
 *	e x p a n d _ g r o u p _ b y
 *
 **************************************
 *
 * Functional description
 *	Expand a GROUP BY clause.
 *
 **************************************/
	qli_nod* node = make_node(input->syn_type, input->syn_count);
	qli_nod** ptr2 = node->nod_arg;

	qli_syntax** ptr = input->syn_arg;
	for (const qli_syntax* const* const end = ptr + input->syn_count; ptr < end; ptr++, ptr2++)
	{
		*ptr2 = expand_expression(*ptr, stack);
		post_map(*ptr2, context);
	}

	return node;
}


static qli_nod* expand_modify( qli_syntax* input, qli_lls* right, qli_lls* left)
{
/**************************************
 *
 *	e x p a n d _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	qli_nod* loop = NULL;

	// If there is an rse, make up a FOR loop

	if (input->syn_arg[s_mod_rse])
	{
		loop = make_node(nod_for, e_for_count);
		loop->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_mod_rse], &right);
	}

    qli_lls* contexts;

	// Loop thru contexts counting them.
	USHORT count = 0;
	for (contexts = right; contexts; contexts = contexts->lls_next)
	{
		qli_ctx* context = (qli_ctx*) contexts->lls_object;
		if (context->ctx_variable)
			continue;
		++count;
		if (context->ctx_rse)
			break;
	}

	if (!count)
		IBERROR(148);			// Msg148 no context for modify

	// Make up node big enough to hold fixed fields plus all contexts

	qli_nod* node = make_node(nod_modify, (int) e_mod_count + count);
	node->nod_count = count;
	qli_nod** ptr = &node->nod_arg[e_mod_count];

	// Loop thru contexts augmenting left context

	for (contexts = right; contexts; contexts = contexts->lls_next)
	{
		qli_ctx* context = (qli_ctx*) contexts->lls_object;
		if (context->ctx_variable)
			continue;
		qli_ctx* new_context = (qli_ctx*) ALLOCD(type_ctx);
		*ptr++ = (qli_nod*) new_context;
		new_context->ctx_type = CTX_RELATION;
		new_context->ctx_source = context;
		new_context->ctx_symbol = context->ctx_symbol;
		new_context->ctx_relation = context->ctx_relation;
		ALLQ_push((blk*) new_context, &left);
		if (context->ctx_rse)
			break;
	}

	// Process sub-statement, list of fields, or, sigh, none of the above

	qli_syntax* syn_list;
	if (input->syn_arg[s_mod_statement])
		node->nod_arg[e_mod_statement] = expand_statement(input->syn_arg[s_mod_statement], right, left);
	else if (syn_list = input->syn_arg[s_mod_list])
	{
		qli_nod* list = make_node(nod_list, syn_list->syn_count);
		node->nod_arg[e_mod_statement] = list;

		ptr = list->nod_arg;
		qli_syntax** syn_ptr = syn_list->syn_arg;
		for (USHORT i = 0; i < syn_list->syn_count; i++, syn_ptr++)
		{
			*ptr++ = make_assignment(expand_expression((qli_syntax*) *syn_ptr, left),
									(qli_nod*) *syn_ptr, right);
		}
	}
	else
		IBERROR(149);			// Msg149 field list required for modify

	if (!loop)
		return node;

	loop->nod_arg[e_for_statement] = node;

	return loop;
}


static qli_nod* expand_output( qli_syntax* input, qli_lls* right, qli_prt** print)
{
/**************************************
 *
 *	e x p a n d _ o u t p u t
 *
 **************************************
 *
 * Functional description
 *	Handle the presence (or absence) of an output specification clause.
 *
 **************************************/
	if (print)
		*print = (qli_prt*) ALLOCD(type_prt);

	if (!input)
		return NULL;

	qli_nod* output = make_node(nod_output, e_out_count);
	ALLQ_push((blk*) output, &global_output_stack);

    qli_nod* node = possible_literal(input->syn_arg[s_out_file], right, false);
	if (!node)
		node = expand_expression(input->syn_arg[s_out_file], right);

	output->nod_arg[e_out_file] = node;
	output->nod_arg[e_out_pipe] = (qli_nod*) input->syn_arg[s_out_pipe];

	if (print)
		output->nod_arg[e_out_print] = (qli_nod*) * print;

	return output;
}


static qli_nod* expand_print( qli_syntax* input, qli_lls* right, qli_lls* /*left*/)
{
/**************************************
 *
 *	e x p a n d _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	qli_syntax* syn_rse = input->syn_arg[s_prt_rse];
	qli_lls* new_right = right;

	// If an output file or pipe is present, make up an output node

	qli_prt* print;
	expand_output(input->syn_arg[s_prt_output], right, &print);

	// If a record select expression is present, expand it and build a FOR
	// statement.

	qli_nod* loop = NULL;
	qli_nod* rse = NULL;
	if (syn_rse)
	{
		loop = make_node(nod_for, e_for_count);
		loop->nod_arg[e_for_rse] = rse = expand_rse(syn_rse, &new_right);
	}

	// If there were any print items, process them now.  Look first for things that
	// look like items, but are actually lists.  If there aren't items of any kind,
	// pick up all fields in the relations from the record selection expression.
	qli_lls* items = NULL;
	USHORT count = 0;
	qli_syntax* syn_list = input->syn_arg[s_prt_list];
	if (syn_list)
	{
        qli_syntax** sub = syn_list->syn_arg;
		for (const qli_syntax* const* const end = sub + syn_list->syn_count; sub < end; sub++)
		{
			const qli_syntax* syn_item;
			if (((*sub)->syn_type == nod_print_item) &&
				(syn_item = (*sub)->syn_arg[s_itm_value]) && (syn_item->syn_type == nod_star))
			{
				count += generate_items(syn_item, new_right, (qli_lls*) &items, rse);
			}
			else
			{
				ALLQ_push((blk*) expand_print_item(*sub, new_right), &items);
				count++;
			}
		}
	}
	else if (syn_rse && (syn_list = syn_rse->syn_arg[s_rse_reduced]))
	{
        qli_syntax** sub = syn_list->syn_arg;
		for (const qli_syntax* const* const end = sub + syn_list->syn_count; sub < end; sub += 2)
		{
			qli_print_item* item = (qli_print_item*) ALLOCD(type_itm);
			item->itm_type = item_value;
			item->itm_value = expand_expression(*sub, new_right);
			expand_edit_string(item->itm_value, item);
			ALLQ_push((blk*) item, &items);
			count++;
		}
	}
	else
		for (; new_right; new_right = new_right->lls_next)
		{
			qli_ctx* context = (qli_ctx*) new_right->lls_object;
			qli_rel* relation = context->ctx_relation;
			if (!relation || context->ctx_sub_rse)
				continue;
			for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
			{
				if ((field->fld_system_flag && field->fld_system_flag != relation->rel_system_flag) ||
					field->fld_flags & FLD_array)
				{
					continue;
				}
				qli_nod* node = make_field(field, context);
				if (rse && rse->nod_arg[e_rse_group_by] &&
					invalid_nod_field(node, rse->nod_arg[e_rse_group_by]))
				{
					continue;
				}
				qli_print_item* item = (qli_print_item*) ALLOCD(type_itm);
				item->itm_type = item_value;
				item->itm_value = make_field(field, context);
				expand_edit_string(item->itm_value, item);
				ALLQ_push((blk*) item, &items);
				count++;
			}
			if (rse = context->ctx_rse)
				break;
		}

	// If no print object showed up, complain!

	if (!count)
		IBERROR(150);			// Msg150 No items in print list

	// Build new print statement.  Unlike the syntax node, the print statement
	// has only print items in it.

	qli_nod* node = make_node(input->syn_type, e_prt_count);
	qli_nod* list = make_list(items);
	node->nod_arg[e_prt_list] = list;
	node->nod_arg[e_prt_output] = (qli_nod*) print;

	// If DISTINCT was requested, make up a reduced list.

	if (rse && input->syn_arg[s_prt_distinct])
	{
		qli_nod* reduced = make_node(nod_list, list->nod_count * 2);
		reduced->nod_count = 0;
		qli_nod** ptr = reduced->nod_arg;
		for (USHORT i = 0; i < list->nod_count; i++)
		{
			qli_print_item* item = (qli_print_item*) list->nod_arg[i];
			if (item->itm_value)
			{
				*ptr++ = item->itm_value;
				ptr++;
				reduced->nod_count++;
			}
		}
		if (reduced->nod_count)
			rse->nod_arg[e_rse_reduced] = reduced;
	}

	// If a FOR loop was generated, splice it in here.

	if (loop)
	{
		loop->nod_arg[e_for_statement] = node;
		node = loop;
		if (input->syn_arg[s_prt_order])
			rse->nod_arg[e_rse_sort] = expand_sort(input->syn_arg[s_prt_order], new_right, list);
	}

	return node;
}


static qli_print_item* expand_print_item( qli_syntax* syn_item, qli_lls* right)
{
/**************************************
 *
 *	e x p a n d _ p r i n t _ i t e m
 *
 **************************************
 *
 * Functional description
 *	Expand a print item.  A print item can either be a value or a format
 *	specifier.
 *
 **************************************/
	qli_print_item* item = (qli_print_item*) ALLOCD(type_itm);

	switch (syn_item->syn_type)
	{
	case nod_print_item:
		{
			item->itm_type = item_value;
			qli_syntax* syn_expr = syn_item->syn_arg[s_itm_value];
			qli_nod* node = item->itm_value = expand_expression(syn_expr, right);
			item->itm_edit_string = (TEXT*) syn_item->syn_arg[s_itm_edit_string];
			item->itm_query_header = (TEXT*) syn_item->syn_arg[s_itm_header];
			expand_edit_string(node, item);
			return item;
		}

	case nod_column:
		item->itm_type = item_column;
		break;

	case nod_tab:
		item->itm_type = item_tab;
		break;

	case nod_space:
		item->itm_type = item_space;
		break;

	case nod_skip:
		item->itm_type = item_skip;
		break;

	case nod_new_page:
		item->itm_type = item_new_page;
		break;

	case nod_column_header:
		item->itm_type = item_column_header;
		break;

	case nod_report_header:
		item->itm_type = item_report_header;
		break;

	}

	item->itm_count = (IPTR) syn_item->syn_arg[0];
	return item;
}


static qli_nod* expand_print_list( qli_syntax* input, qli_lls* stack)
{
/**************************************
 *
 *	e x p a n d _ p r i n t _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Expand a print list.
 *
 **************************************/
	qli_lls* items = NULL;
	qli_syntax** ptr = input->syn_arg;

	for (const qli_syntax* const* const end = ptr + input->syn_count; ptr < end; ptr++)
	{
		ALLQ_push((blk*) expand_print_item(*ptr, stack), &items);
	}

	return make_list(items);
}


static qli_nod* expand_report( qli_syntax* input, qli_lls* right, qli_lls* /*left*/)
{
/**************************************
 *
 *	e x p a n d _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Expand a report specification.
 *
 **************************************/
	qli_prt* print;

	// Start by processing record selection expression

	expand_output(input->syn_arg[s_prt_output], right, &print);
	qli_rpt* report = print->prt_report = (qli_rpt*) input->syn_arg[s_prt_list];

	if (!(print->prt_lines_per_page = report->rpt_lines))
		print->prt_lines_per_page = QLI_lines;

	if (!report->rpt_columns)
		report->rpt_columns = QLI_columns;

	qli_nod* loop = make_node(nod_report_loop, e_for_count);
	loop->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_prt_rse], &right);
	qli_nod* node = make_node(nod_report, e_prt_count);
	loop->nod_arg[e_for_statement] = node;

	node->nod_arg[e_prt_list] = (qli_nod*) report;
	node->nod_arg[e_prt_output] = (qli_nod*) print;

	// Process clauses where they exist

	expand_control_break(&report->rpt_top_rpt, right);
	expand_control_break(&report->rpt_top_page, right);
	expand_control_break(&report->rpt_top_breaks, right);

	qli_syntax* sub = (qli_syntax*) report->rpt_detail_line;
	if (sub)
		report->rpt_detail_line = expand_print_list(sub, right);

	expand_control_break(&report->rpt_bottom_breaks, right);
	expand_control_break(&report->rpt_bottom_page, right);
	expand_control_break(&report->rpt_bottom_rpt, right);

	return loop;
}


static qli_nod* expand_restructure( qli_syntax* input, qli_lls* right, qli_lls* /*left*/)
{
/**************************************
 *
 *	e x p a n d _ r e s t r u c t u r e
 *
 **************************************
 *
 * Functional description
 *	Transform a restructure statement into a FOR <rse> STORE.
 *
 **************************************/

	// Make a FOR loop to drive the restructure

	qli_nod* loop = make_node(nod_for, e_for_count);
	loop->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_asn_from], &right);

	// Make a STORE node.

	qli_nod* node = make_node(nod_store, e_sto_count);
	loop->nod_arg[e_for_statement] = node;
	qli_syntax* rel_node = input->syn_arg[s_asn_to];
	qli_ctx* context = (qli_ctx*) ALLOCD(type_ctx);
	node->nod_arg[e_sto_context] = (qli_nod*) context;
	context->ctx_type = CTX_RELATION;
	context->ctx_rse = (qli_nod*) -1;
	qli_rel* relation = context->ctx_relation = (qli_rel*) rel_node->syn_arg[s_rel_relation];

	// If we don't already know about the relation, find out now.

	if (!(relation->rel_flags & REL_fields))
		MET_fields(relation);

	// Match fields in target relation against fields in the input rse.  Fields
	// may match on either name or query name.

	qli_lls* stack = NULL;

	for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
		if (!(field->fld_flags & FLD_computed))
		{
			for (qli_lls* search = right; search; search = search->lls_next)
			{
				qli_ctx* ctx = (qli_ctx*) search->lls_object;

				// First look for an exact field name match

				qli_fld* fld;
				for (fld = ctx->ctx_relation->rel_fields; fld; fld = fld->fld_next)
				{
					if (compare_symbols(field->fld_name, fld->fld_name))
						break;
				}
				// Next try, target field name matching source query name

				if (!fld)
					for (fld = ctx->ctx_relation->rel_fields; fld; fld = fld->fld_next)
					{
						if (compare_symbols(field->fld_name, fld->fld_query_name))
							break;
					}
				// If nothing yet, look for any old match

				if (!fld)
					for (fld = ctx->ctx_relation->rel_fields; fld; fld = fld->fld_next)
					{
						if (compare_symbols(field-> fld_query_name, fld->fld_name) ||
							compare_symbols(field->fld_query_name, fld->fld_query_name))
						{
								break;
						}
					}

				if (fld)
				{
					qli_nod* assignment = make_node(nod_assign, e_asn_count);
					assignment->nod_count = e_asn_count - 1;
					assignment->nod_arg[e_asn_to] = make_field(field, context);
					assignment->nod_arg[e_asn_from] = make_field(fld, ctx);
					ALLQ_push((blk*) assignment, &stack);
					goto found_field;
				}

				if (ctx->ctx_rse)
					break;
			}
			found_field:;
		}

	node->nod_arg[e_sto_statement] = make_list(stack);

	return loop;
}


static qli_nod* expand_rse( qli_syntax* input, qli_lls** stack)
{
/**************************************
 *
 *	e x p a n d _ r s e
 *
 **************************************
 *
 * Functional description
 *	Expand a record selection expression, returning an updated context
 *	stack.
 *
 **************************************/
	qli_lls* old_stack = *stack;
	qli_lls* new_stack = *stack;
	qli_nod* boolean = NULL;
	qli_nod* node = make_node(input->syn_type, (int) e_rse_count + input->syn_count);
	node->nod_count = input->syn_count;
	qli_nod** ptr2 = &node->nod_arg[e_rse_count];

	// Decide whether or not this is a GROUP BY, real or imagined
	// If it is, disallow normal field type references

	qli_ctx* parent_context = NULL;
	qli_nod* parent_rse = NULL;

	if (input->syn_arg[s_rse_group_by] || input->syn_arg[s_rse_having])
		parent_context = (qli_ctx*) ALLOCD(type_ctx);
	qli_syntax* list = input->syn_arg[s_rse_list];
	if (list)
	{
		for (USHORT i = 0; i < list->syn_count; i++)
		{
			const qli_syntax* value = list->syn_arg[i];
			const qli_syntax* field = value->syn_arg[e_itm_value];
			if (!field)
				continue;

			if (global_agg(field, input->syn_arg[s_rse_group_by]))
			{
				if (!parent_context)
					parent_context = (qli_ctx*) ALLOCD(type_ctx);
			}
			else if (parent_context)
			{
				if (invalid_syn_field(field, input->syn_arg[s_rse_group_by]))
					IBERROR(451);
			}
		}
	}

	if (parent_context)
	{
		parent_context->ctx_type = CTX_AGGREGATE;
		parent_rse = make_node(nod_rse, e_rse_count + 1);
		parent_rse->nod_count = 1;
		parent_rse->nod_arg[e_rse_count] = (qli_nod*) parent_context;
		parent_context->ctx_sub_rse = node;
	}

	// Process the FIRST clause before the context gets augmented

	if (input->syn_arg[s_rse_first])
		node->nod_arg[e_rse_first] = expand_expression(input->syn_arg[e_rse_first], old_stack);

	// Process relations

	qli_syntax** ptr = input->syn_arg + s_rse_count;

	for (USHORT i = 0; i < input->syn_count; i++)
	{
		qli_syntax* rel_node = *ptr++;
		qli_syntax* over = *ptr++;
		qli_ctx* context = (qli_ctx*) ALLOCD(type_ctx);
		*ptr2++ = (qli_nod*) context;
		if (i == 0)
			context->ctx_rse = node;
		if (rel_node->syn_type == nod_rse)
		{
			context->ctx_type = CTX_STREAM;
			context->ctx_stream = expand_rse(rel_node, &new_stack);
		}
		else
		{
			context->ctx_type = CTX_RELATION;
			qli_rel* relation = context->ctx_relation = (qli_rel*) rel_node->syn_arg[s_rel_relation];
			if (!(relation->rel_flags & REL_fields))
				MET_fields(relation);
			qli_symbol* symbol = context->ctx_symbol = (qli_symbol*) rel_node->syn_arg[s_rel_context];
			if (symbol)
				symbol->sym_object = (BLK) context;
			if (over)
			{
				qli_lls* short_stack = NULL;
				ALLQ_push((blk*) context, &short_stack);
				for (USHORT j = 0; j < over->syn_count; j++)
				{
					qli_syntax* field = over->syn_arg[j];
					qli_nod* eql_node = make_node(nod_eql, 2);
					eql_node->nod_arg[0] = expand_expression(field, short_stack);
					eql_node->nod_arg[1] = expand_expression(field, new_stack);
					boolean = make_and(eql_node, boolean);
				}
				ALLQ_pop(&short_stack);
			}
		}
		ALLQ_push((blk*) context, &new_stack);
	}

	// Handle explicit boolean

	if (input->syn_arg[e_rse_boolean])
		boolean = make_and(boolean, expand_expression(input->syn_arg[e_rse_boolean], new_stack));

	// Handle implicit boolean from SQL xxx IN (yyy FROM relation)

	if (input->syn_arg[s_rse_outer])
	{
		qli_nod* eql_node = make_node((enum nod_t)(IPTR)input->syn_arg[s_rse_op], 2);
		eql_node->nod_arg[0] = expand_expression(input->syn_arg[s_rse_outer], old_stack);
		eql_node->nod_arg[1] = expand_expression(input->syn_arg[s_rse_inner], new_stack);
		if (input->syn_arg[s_rse_all_flag])
			eql_node = negate(eql_node);
		boolean = make_and(eql_node, boolean);
	}

	node->nod_arg[e_rse_boolean] = boolean;

	if (input->syn_arg[s_rse_sort])
	{
		qli_nod* temp = expand_sort(input->syn_arg[e_rse_sort], new_stack, 0);
		if (parent_rse)
			parent_rse->nod_arg[e_rse_sort] = temp;
		else
			node->nod_arg[e_rse_sort] = temp;
	}

	if (input->syn_arg[s_rse_reduced])
		node->nod_arg[e_rse_reduced] = expand_sort(input->syn_arg[e_rse_reduced], new_stack, 0);

	if (input->syn_arg[s_rse_group_by])
	{
		parent_rse->nod_arg[e_rse_group_by] =
			expand_group_by(input->syn_arg[s_rse_group_by], new_stack, parent_context);
	}

	node->nod_arg[e_rse_join_type] = (qli_nod*) input->syn_arg[s_rse_join_type];

	// If there is a parent context, set it up here

	*stack = new_stack;

	if (!parent_context)
		return node;

    qli_ctx* context = NULL;
	ptr2 = node->nod_arg + e_rse_count;
	for (const qli_nod* const* const end = ptr2 + node->nod_count; ptr2 < end; ptr2++)
	{
		context = (qli_ctx*) *ptr2;
		context->ctx_parent = parent_context;
	}

	if (!(parent_context->ctx_relation = context->ctx_relation))
		parent_context->ctx_stream = context->ctx_stream;
	ALLQ_push((blk*) parent_context, stack);

	if (input->syn_arg[s_rse_having])
		parent_rse->nod_arg[e_rse_having] = expand_expression(input->syn_arg[s_rse_having], *stack);

	return parent_rse;
}


static qli_nod* expand_sort( qli_syntax* input, qli_lls* stack, qli_nod* list)
{
/**************************************
 *
 *	e x p a n d _ s o r t
 *
 **************************************
 *
 * Functional description
 *	Expand a sort or reduced clause.  This is more than a little
 *	kludgy.  For pure undiscipled, pragmatic reasons, the count for
 *	a sort/ reduced clause for a syntax node is twice the number of
 *	actual keys.  For node nodes, however, the count is the accurate
 *	number of keys.  So be careful.
 *
 **************************************/
	qli_nod* node = make_node(nod_list, input->syn_count);
	node->nod_count = input->syn_count / 2;
	qli_nod** ptr = node->nod_arg;
	qli_syntax** syn_ptr = input->syn_arg;

	for (USHORT i = 0; i < node->nod_count; i++)
	{
		qli_syntax* expr = *syn_ptr++;
		if (expr->syn_type == nod_position)
		{
			const IPTR position = (IPTR) expr->syn_arg[0];
			if (!list || !position || position > list->nod_count)
				IBERROR(152);	// Msg152 invalid ORDER BY ordinal
			qli_print_item* item = (qli_print_item*) list->nod_arg[position - 1];
			*ptr++ = item->itm_value;
		}
		else
			*ptr++ = expand_expression(expr, stack);
		*ptr++ = (qli_nod*) * syn_ptr++;
	}

	return node;
}


static qli_nod* expand_statement( qli_syntax* input, qli_lls* right, qli_lls* left)
{
/**************************************
 *
 *	e x p a n d _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Expand a statement.
 *
 **************************************/
	qli_nod* node;
	qli_nod* (*routine) (qli_syntax*, qli_lls*, qli_lls*);

	switch (input->syn_type)
	{
	case nod_abort:
		node = make_node(input->syn_type, input->syn_count);
		if (input->syn_arg[0])
			node->nod_arg[0] = expand_expression(input->syn_arg[0], right);
		return node;

	case nod_assign:
		routine = expand_assignment;
		break;

	case nod_commit_retaining:
		{
			node = make_node(input->syn_type, input->syn_count);
			for (USHORT i = 0; i < input->syn_count; i++)
				node->nod_arg[i] = (qli_nod*) input->syn_arg[i];
			return node;
		}

	case nod_erase:
		routine = expand_erase;
		break;

	case nod_for:
		routine = expand_for;
		break;

	case nod_if:
		node = make_node(input->syn_type, input->syn_count);
		node->nod_arg[e_if_boolean] = expand_expression(input->syn_arg[s_if_boolean], right);
		node->nod_arg[e_if_true] = expand_statement(input->syn_arg[s_if_true], right, left);
		if (input->syn_arg[s_if_false])
			node->nod_arg[e_if_false] = expand_statement(input->syn_arg[s_if_false], right, left);
		else
			node->nod_count = 2;
		return node;

	case nod_modify:
		routine = expand_modify;
		break;

	case nod_print:
	case nod_list_fields:
		routine = expand_print;
		break;

	case nod_report:
		routine = expand_report;
		break;

	case nod_restructure:
		routine = expand_restructure;
		break;

	case nod_store:
		routine = expand_store;
		break;

	case nod_repeat:
		node = make_node(input->syn_type, input->syn_count);
		node->nod_arg[e_rpt_value] = expand_expression(input->syn_arg[s_rpt_value], left);
		node->nod_arg[e_rpt_statement] =
			expand_statement(input->syn_arg[s_rpt_statement], right, left);
		return node;

	case nod_list:
		{
			qli_syntax** syn_ptr = input->syn_arg;
			qli_lls* stack = NULL;
			for (USHORT i = 0; i < input->syn_count; i++)
			{
				qli_syntax* syn_node = *syn_ptr++;
				if (syn_node->syn_type == nod_declare)
				{
					qli_ctx* context = (qli_ctx*) ALLOCD(type_ctx);
					context->ctx_type = CTX_VARIABLE;
					qli_syntax* field_node = syn_node->syn_arg[1];
					if (field_node)
					{
						if (field_node->syn_type == nod_index)
							field_node = field_node->syn_arg[s_idx_field];
						resolve_really((qli_fld*) syn_node->syn_arg[0], field_node);
					}
					context->ctx_variable = (qli_fld*) syn_node->syn_arg[0];
					ALLQ_push((blk*) context, &right);
					ALLQ_push((blk*) context, &left);
				}
				else if (node = expand_statement(syn_node, right, left))
					ALLQ_push((blk*) node, &stack);
			}
			return make_list(stack);
		}

	case nod_declare:
		return NULL;

	default:
		ERRQ_bugcheck(136);			// Msg136 expand_statement: not yet implemented
	}

	return (*routine) (input, right, left);
}


static qli_nod* expand_store( qli_syntax* input, qli_lls* right, qli_lls* left)
{
/**************************************
 *
 *	e x p a n d _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Process, yea expand, on a mere STORE statement.  Make us
 *	something neat if nothing looks obvious.
 *
 **************************************/
	qli_nod* loop = NULL;

	// If there is an rse, make up a FOR loop

	if (input->syn_arg[s_sto_rse])
	{
		loop = make_node(nod_for, e_for_count);
		loop->nod_arg[e_for_rse] = expand_rse(input->syn_arg[s_sto_rse], &right);
	}

	qli_nod* node = make_node(input->syn_type, e_sto_count);

	qli_syntax* rel_node = input->syn_arg[s_sto_relation];
	qli_ctx* context = (qli_ctx*) ALLOCD(type_ctx);
	node->nod_arg[e_sto_context] = (qli_nod*) context;
	context->ctx_type = CTX_RELATION;
	context->ctx_rse = (qli_nod*) -1;
	qli_rel* relation = context->ctx_relation = (qli_rel*) rel_node->syn_arg[s_rel_relation];

	if (!(relation->rel_flags & REL_fields))
		MET_fields(relation);

	qli_symbol* symbol = context->ctx_symbol = (qli_symbol*) rel_node->syn_arg[s_rel_context];
	if (symbol)
		symbol->sym_object = (BLK) context;

	ALLQ_push((blk*) context, &left);

	//  If there are field and value lists, process them

	if (input->syn_arg[s_sto_values])
	{
		if (!input->syn_arg[s_sto_fields])
		{
			qli_lls* stack = NULL;
			for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
			{
				ALLQ_push((blk*) decompile_field(field, 0), &stack);
			}
			input->syn_arg[s_sto_fields] = (qli_syntax*) stack;
		}
		expand_values(input, right);
	}

	// Process sub-statement.  If there isn't one, make up a series of assignments.

	if (input->syn_arg[s_sto_statement])
	{
		qli_ctx* secondary = (qli_ctx*) ALLOCD(type_ctx);
		secondary->ctx_type = CTX_RELATION;
		secondary->ctx_primary = context;
		ALLQ_push((blk*) secondary, &right);
		node->nod_arg[e_sto_statement] =
			expand_statement(input->syn_arg[s_sto_statement], right, left);
	}
	else
	{
		qli_lls* stack = NULL;
		for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
		{
			if (field->fld_flags & FLD_computed)
				continue;
			if ((field->fld_system_flag && field->fld_system_flag != relation->rel_system_flag) ||
				(field->fld_flags & FLD_array))
			{
				continue;
			}
			qli_nod* assignment = make_assignment(make_field(field, context), 0, 0);
			ALLQ_push((blk*) assignment, &stack);
		}
		node->nod_arg[e_sto_statement] = make_list(stack);
	}

	if (!loop)
		return node;

	loop->nod_arg[e_for_statement] = node;

	return loop;
}


static void expand_values( qli_syntax* input, qli_lls* right)
{
/**************************************
 *
 *	e x p a n d _ v a l u e s
 *
 **************************************
 *
 * Functional description
 *	We've got a grungy SQL insert, and we have
 *	to make the value list match the field list.
 *	On the way in, we got the right number of
 *	fields.  Now all that's needed is the values
 *	and matching the two lists, and generating
 *	assignments.  If the input is from a select,
 *	things may be harder, and if there are wild cards
 *	things will be harder still.  Wild cards come in
 *	two flavors * and <context>.*.  The first is
 *	a nod_prompt, the second a nod_star.
 *
 **************************************/

	// fields have already been checked and expanded.  Just count them

	qli_lls* fields = (qli_lls*) input->syn_arg[s_sto_fields];
	qli_lls* stack;
	int field_count = 0;
	for (stack = fields; stack; stack = stack->lls_next)
		field_count++;

	// We're going to want the values in the order listed in the command

	qli_lls* values = (qli_lls*) input->syn_arg[s_sto_values];
	while (values)
		ALLQ_push(ALLQ_pop(&values), &stack);

	// now go through, count, and expand where needed

	int value_count = 0;
	while (stack)
	{
		qli_syntax* value = (qli_syntax*) ALLQ_pop(&stack);
		if (input->syn_arg[s_sto_rse] && value->syn_type == nod_prompt)
		{
			if (value->syn_arg[0] == 0)
			{
				qli_lls* temp = NULL;
				for (; right; right = right->lls_next)
					ALLQ_push(right->lls_object, &temp);

				while (temp)
				{
					qli_ctx* context = (qli_ctx*) ALLQ_pop(&temp);
					value_count +=
						generate_fields(context, (qli_lls*) &values, input->syn_arg[s_sto_rse]);
				}
			}
			else
				IBERROR(542);	// this was a prompting expression.  won't do at all
		}
		else if (input->syn_arg[s_sto_rse] && (value->syn_type == nod_star))
		{
			qli_ctx* context = find_context((const qli_name*) value->syn_arg[0], right);
			if (!context)
				IBERROR(154);	// Msg154 unrecognized context
			value_count += generate_fields(context, (qli_lls*) &values, input->syn_arg[s_sto_rse]);
		}
		else
		{
			ALLQ_push((blk*) value, &values);
			value_count++;
		}
	}

	// Make assignments from values to fields

	if (field_count != value_count)
		IBERROR(189);
		// Msg189 the number of values do not match the number of fields

	qli_syntax* list = (qli_syntax*) ALLOCDV(type_syn, value_count);
	list->syn_type = nod_list;
	list->syn_count = value_count;
	input->syn_arg[s_sto_statement] = list;
	qli_syntax** ptr = list->syn_arg + value_count;

	while (values)
	{
	    qli_syntax* assignment = (qli_syntax*) ALLOCDV(type_syn, s_asn_count);
		*--ptr = assignment;
		assignment->syn_type = nod_assign;
		assignment->syn_count = s_asn_count;
		assignment->syn_arg[s_asn_to] = (qli_syntax*) ALLQ_pop(&fields);
		assignment->syn_arg[s_asn_from] = (qli_syntax*) ALLQ_pop(&values);
	}
}


static qli_ctx* find_context( const qli_name* name, qli_lls* contexts)
{
/**************************************
 *
 *	f i n d _ c o n t e x t
 *
 **************************************
 *
 * Functional description
 *	We've got a context name and we need to return
 *	the context block implicated.
 *
 **************************************/
	for (; contexts; contexts = contexts->lls_next)
	{
		qli_ctx* context = (qli_ctx*) contexts->lls_object;
		const qli_rel* relation = context->ctx_relation;
		if (compare_names(name, relation->rel_symbol))
			return context;
		if (compare_names(name, context->ctx_symbol))
			return context;
	}
	return NULL;
}


static int generate_fields( qli_ctx* context, qli_lls* values, qli_syntax* rse)
{
/**************************************
 *
 *	g e n e r a t e _ f i e l d s
 *
 **************************************
 *
 * Functional description
 *	Expand an asterisk expression, which
 *	could be <relation>.* or <alias>.* or <context>.*
 *	into a list of non-expanded field blocks for
 *	input to a store or update.
 *
 **************************************/

	if (context->ctx_type == CTX_VARIABLE)
		return 0;
	if (context->ctx_type == CTX_AGGREGATE)
		return 0;

	qli_syntax* group_list = rse->syn_arg[s_rse_group_by];
	qli_rel* relation = context->ctx_relation;
	int count = 0;

	for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		if ((field->fld_system_flag && field->fld_system_flag != relation->rel_system_flag) ||
			(field->fld_flags & FLD_array))
		{
			continue;
		}
		qli_syntax* value = decompile_field(field, context);
		if (group_list && invalid_syn_field(value, group_list))
			continue;
		ALLQ_push((blk*) value, (qli_lls**) values);
		count++;
	}

	return count;
}


static int generate_items(const qli_syntax* symbol, qli_lls* right, qli_lls* items, qli_nod* rse)
{
/**************************************
 *
 *	g e n e r a t e _ i t e m s
 *
 **************************************
 *
 * Functional description
 *	Expand an asterisk expression, which
 *	could be <relation>.* or <alias>.* or <context>.*
 *	into a list of reasonable print items.
 *
 *      If the original request included a group by,
 *	include only the grouping fields.
 *
 **************************************/
	qli_nod* group_list = rse ? rse->nod_arg[e_rse_group_by] : NULL;

	// first identify the relation or context

	const qli_name* name;
	if (symbol->syn_count == 1)
		name = (qli_name*) symbol->syn_arg[0];
	else
		IBERROR(153);
		// Msg153 asterisk expressions require exactly one qualifying context

	qli_ctx* context = find_context(name, right);
	if (!context)
		IBERROR(154);			// Msg154 unrecognized context

	qli_rel* relation = context->ctx_relation;
	int count = 0;
	for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
	{
		if ((field->fld_system_flag && field->fld_system_flag != relation->rel_system_flag) ||
			(field->fld_flags & FLD_array))
		{
			continue;
		}
		qli_nod* node = make_field(field, context);
		if (group_list && invalid_nod_field(node, group_list))
			continue;
		qli_print_item* item = (qli_print_item*) ALLOCD(type_itm);
		item->itm_type = item_value;
		item->itm_value = make_field(field, context);
		expand_edit_string(item->itm_value, item);
		ALLQ_push((blk*) item, (qli_lls**) items);
		++count;
	}

	return count;
}


static bool global_agg( const qli_syntax* item, const qli_syntax* group_list)
{
/**************************************
 *
 *	g l o b a l _ a g g
 *
 **************************************
 *
 * Functional description
 *	We've got a print list item that may contain
 *	a sql global aggregate.  If it does, we're
 *	going to make the whole thing a degenerate
 *	group by.  Anyway.  Look for aggregates buried
 *	deep within printable things.
 *
 *	This recurses.  If it finds a mixture of normal
 *	and aggregates it complains.
 *
 **************************************/
	bool normal_field = false;
	bool aggregate = false;

	switch (item->syn_type)
	{
	case nod_agg_average:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:
	case nod_agg_count:
	case nod_running_total:
	case nod_running_count:
		return true;

	case nod_upcase:
	case nod_lowcase:
	case nod_add:
	case nod_subtract:
	case nod_multiply:
	case nod_divide:
	case nod_negate:
	case nod_concatenate:
	case nod_substr:
		{
			const qli_syntax* const* ptr = item->syn_arg;
			for (const qli_syntax* const* const end = ptr + item->syn_count; ptr < end; ptr++)
			{
				if ((*ptr)->syn_type == nod_constant)
					continue;
				if (global_agg(*ptr, group_list))
					aggregate = true;
				else if (!group_list || invalid_syn_field(*ptr, group_list))
					normal_field = true;
			}
		}

	default:
		break;
	}

	if (normal_field && aggregate)
		IBERROR(451);

	return aggregate;
}


static bool invalid_nod_field( const qli_nod* node, const qli_nod* list)
{
/**************************************
 *
 *	i n v a l i d _ n o d _ f i e l d
 *
 **************************************
 *
 * Functional description
 *
 *	Validate that an expanded field / context
 *	pair is in a specified list.  Thus is used
 *	in one instance to check that a simple field selected
 *	through a grouping rse is a grouping field -
 *	thus a valid field reference.
 *
 **************************************/
	if (!list)
		return true;

	bool invalid = false;

	if (node->nod_type == nod_field)
	{
		const qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
		const qli_ctx* context = (qli_ctx*) node->nod_arg[e_fld_context];
		const qli_nod* const* ptr = list->nod_arg;
		for (const qli_nod* const* const end = ptr + list->nod_count; ptr < end; ptr++)
		{
			if (field == (qli_fld*) (*ptr)->nod_arg[e_fld_field] &&
				context == (qli_ctx*) (*ptr)->nod_arg[e_fld_context])
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		const qli_nod* const* ptr = node->nod_arg;
		for (const qli_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
		{
			switch ((*ptr)->nod_type)
			{
			case nod_field:
			case nod_add:
			case nod_subtract:
			case nod_multiply:
			case nod_divide:
			case nod_negate:
			case nod_concatenate:
			case nod_substr:
			case nod_format:
			case nod_choice:
			case nod_function:
			case nod_upcase:
			case nod_lowcase:
				invalid |= invalid_nod_field(*ptr, list);
			}
		}
	}

	return invalid;
}


static bool invalid_syn_field( const qli_syntax* syn_node, const qli_syntax* list)
{
/**************************************
 *
 *	i n v a l i d _ s y n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Make sure an unexpanded simple field selected
 *	through a grouping rse is a grouping field -
 *	thus a valid field reference.  For the sake of
 *      argument, we'll match qualified to unqualified
 *	reference, but qualified reference must match
 *	completely.
 *
 *	One more thought.  If this miserable thing is
 *	a wild card, let it through and expand it
 *	correctly later.
 *
 **************************************/
	if (syn_node->syn_type == nod_star)
		return false;

	if (!list)
		return true;

	bool invalid = false;

	if (syn_node->syn_type == nod_field)
	{
		const qli_name* fctx = NULL;
		const qli_name* fname = (qli_name*) syn_node->syn_arg[0];
		if (syn_node->syn_count == 2)
		{
			fctx = fname;
			fname = (qli_name*) syn_node->syn_arg[1];
		}

		for (SSHORT count = list->syn_count; count;)
		{
			const qli_name* gctx = NULL;
			const qli_syntax* element = list->syn_arg[--count];
			const qli_name* gname = (qli_name*) element->syn_arg[0];
			if (element->syn_count == 2)
			{
				gctx = gname;
				gname = (qli_name*) element->syn_arg[1];
			}
			if (!strcmp(fname->nam_string, gname->nam_string))
			{
				if (!gctx || !fctx || !strcmp(fctx->nam_string, gctx->nam_string))
				{
					return false;
				}
			}
		}
		return true;
	}
	else
	{
		const qli_syntax* const* ptr = syn_node->syn_arg;
		for (const qli_syntax* const* const end = ptr + syn_node->syn_count; ptr < end; ptr++)
		{
			switch ((*ptr)->syn_type)
			{
			case nod_field:
			case nod_add:
			case nod_subtract:
			case nod_multiply:
			case nod_divide:
			case nod_negate:
			case nod_concatenate:
			case nod_substr:
			case nod_format:
			case nod_choice:
			case nod_function:
			case nod_upcase:
			case nod_lowcase:
				invalid |= invalid_syn_field(*ptr, list);
			}
		}
	}

	return invalid;
}


static qli_nod* make_and( qli_nod* expr1, qli_nod* expr2)
{
/**************************************
 *
 *	m a k e _ a n d
 *
 **************************************
 *
 * Functional description
 *	Combine two expressions, each possible null, into at most
 *	a single boolean.
 *
 **************************************/
	if (!expr1)
		return expr2;

	if (!expr2)
		return expr1;

	qli_nod* node = make_node(nod_and, 2);
	node->nod_arg[0] = expr1;
	node->nod_arg[1] = expr2;

	return node;
}


static qli_nod* make_assignment( qli_nod* target, qli_nod* initial, qli_lls* right)
{
/**************************************
 *
 *	m a k e _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Generate a prompt and assignment to a field.
 *
 **************************************/
	qli_fld* field = (qli_fld*) target->nod_arg[e_fld_field];
	qli_lls* stack = NULL;
	ALLQ_push((blk*) target->nod_arg[e_fld_context], &stack);

	qli_nod* prompt;

	if (field->fld_dtype == dtype_blob)
	{
		prompt = make_node(nod_edit_blob, e_edt_count);
		prompt->nod_count = 0;
		prompt->nod_arg[e_edt_name] = (qli_nod*) field->fld_name->sym_string;
		if (initial)
		{
			prompt->nod_count = 1;
			prompt->nod_arg[e_edt_input] = expand_expression((qli_syntax*) initial, right);
		}
	}
	else
	{
		prompt = make_node(nod_prompt, e_prm_count);
		prompt->nod_arg[e_prm_prompt] = (qli_nod*) field->fld_name->sym_string;
		prompt->nod_arg[e_prm_field] = (qli_nod*) field;
	}

	qli_nod* assignment = make_node(nod_assign, e_asn_count);
	assignment->nod_arg[e_asn_to] = target;
	assignment->nod_arg[e_asn_from] = prompt;

	if (field->fld_validation)
		assignment->nod_arg[e_asn_valid] = expand_expression(field->fld_validation, stack);
	else
		--assignment->nod_count;

	ALLQ_pop(&stack);

	return assignment;
}


static qli_nod* make_field( qli_fld* field, qli_ctx* context)
{
/**************************************
 *
 *	m a k e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Make a field block.  Not too tough.
 *
 **************************************/
	qli_nod* node = make_node(nod_field, e_fld_count);
	node->nod_count = 0;
	node->nod_arg[e_fld_field] = (qli_nod*) field;
	node->nod_arg[e_fld_context] = (qli_nod*) context;

	if (context->ctx_variable)
		node->nod_type = nod_variable;

	return node;
}


static qli_nod* make_list( qli_lls* stack)
{
/**************************************
 *
 *	m a k e _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Dump a stack of junk into a list node.  Best count
 *	them first.
 *
 **************************************/
	qli_lls* temp = stack;
	USHORT count = 0;

	while (temp)
	{
		count++;
		temp = temp->lls_next;
	}

	qli_nod* node = make_node(nod_list, count);
	qli_nod** ptr = &node->nod_arg[count];

	while (stack)
		*--ptr = (qli_nod*) ALLQ_pop(&stack);

	return node;
}


static qli_nod* make_node( nod_t type, USHORT count)
{
/**************************************
 *
 *	m a k e _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Allocate a node and fill in some basic stuff.
 *
 **************************************/
	qli_nod* node = (qli_nod*) ALLOCDV(type_nod, count);
	node->nod_type = type;
	node->nod_count = count;

	return node;
}


static qli_nod* negate( qli_nod* expr)
{
/**************************************
 *
 *	n e g a t e
 *
 **************************************
 *
 * Functional description
 *	Build negation of expression.
 *
 **************************************/
	qli_nod* node = make_node(nod_not, 1);
	node->nod_arg[0] = expr;

	return node;
}


static qli_nod* possible_literal(qli_syntax* input, qli_lls* stack, bool upper_flag)
{
/**************************************
 *
 *	p o s s i b l e _ l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Check to see if a value node is an unresolved name.  If so,
 *	transform it into a constant expression.  This is used to
 *	correct "informalities" in relational expressions.
 *
 **************************************/
	qli_ctx* context;

	// If the value isn't a field, is qualified, or can be resolved,
	// it doesn't qualify for conversion.  Return NULL.

	if (input->syn_type != nod_field || input->syn_count != 1 || resolve(input, stack, &context))
	{
		return NULL;
	}

	const qli_name* name = (qli_name*) input->syn_arg[0];
	USHORT l = name->nam_length;
	qli_const* constant = (qli_const*) ALLOCDV(type_con, l);
	constant->con_desc.dsc_dtype = dtype_text;
	constant->con_desc.dsc_length = l;
	constant->con_desc.dsc_address = constant->con_data;
	QLI_validate_desc(constant->con_desc);
	TEXT* p = (TEXT*) constant->con_data;
	const TEXT* q = name->nam_string;

	if (upper_flag)
	{
		if (l)
			do {
				const TEXT c = *q++;
				*p++ = UPPER(c);
			} while (--l);
	}
	else if (l)
		do {
			const TEXT c = *q++;
			*p++ = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;

		} while (--l);

	qli_nod* node = make_node(nod_constant, 0);
	node->nod_desc = constant->con_desc;

	return node;
}


static qli_nod* post_map( qli_nod* node, qli_ctx* context)
{
/**************************************
 *
 *	p o s t _ m a p
 *
 **************************************
 *
 * Functional description
 *	Post an item to a map for a context.
 *
 **************************************/
	qli_map* map;

	// Check to see if the item has already been posted

	for (map = context->ctx_map; map; map = map->map_next)
	{
		if (CMP_node_match(node, map->map_node))
			break;
	}

	if (!map)
	{
		map = (qli_map*) ALLOCD(type_map);
		map->map_next = context->ctx_map;
		context->ctx_map = map;
		map->map_node = node;
	}

	qli_nod* new_node = make_node(nod_map, e_map_count);
	new_node->nod_count = 0;
	new_node->nod_arg[e_map_context] = (qli_nod*) context;
	new_node->nod_arg[e_map_map] = (qli_nod*) map;
	new_node->nod_desc = node->nod_desc;

	return new_node;
}


static qli_fld* resolve( qli_syntax* node, qli_lls* stack, qli_ctx** out_context)
{
/**************************************
 *
 *	r e s o l v e
 *
 **************************************
 *
 * Functional description
 *	Resolve a field node against a context stack.  Return both the
 *	field block (by value)  and the corresponding context block (by
 *	reference).  Return NULL if field can't be resolved.
 *
 **************************************/
	qli_rel* relation;

	// Look thru context stack looking for a context that will resolve
	// all name segments.  If the context is a secondary context, require
	// that the context name be given explicitly (used for special STORE
	// context).

	qli_name** base = (qli_name**) node->syn_arg;

	for (; stack; stack = stack->lls_next)
	{
		qli_ctx* context = (qli_ctx*) stack->lls_object;
		*out_context = context;
		qli_name** ptr = base + node->syn_count;
		const qli_name* name = *--ptr;

		switch (context->ctx_type)
		{
		case CTX_VARIABLE:
			if (ptr == base)
				for (qli_fld* field = context->ctx_variable; field; field = field->fld_next)
				{
					if (compare_names(name, field->fld_name) ||
						compare_names(name, field->fld_query_name))
					{
						return field;
					}
				}
			break;

		case CTX_RELATION:
			if (context->ctx_primary)
			{
				*out_context = context = context->ctx_primary;
				if (!compare_names((qli_name*) node->syn_arg[0], context->ctx_symbol))
					break;
			}
			relation = context->ctx_relation;

			for (qli_fld* field = relation->rel_fields; field; field = field->fld_next)
				if (compare_names(name, field->fld_name) || compare_names(name, field->fld_query_name))
				{
					if (ptr == base)
						return field;

					name = *--ptr;

					if (compare_names(name, relation->rel_symbol))
					{
						if (ptr == base)
							return field;

						name = *--ptr;
					}

					if (compare_names(name, context->ctx_symbol))
					{
						if (ptr == base)
							return field;
					}
					break;
				}
			break;
		}
	}

	// We didn't resolve all name segments.  Let somebody else worry about it.

	return NULL;
}



static void resolve_really( qli_fld* variable, const qli_syntax* field_node)
{
/**************************************
 *
 *	r e s o l v e _ r e a l l y
 *
 **************************************
 *
 * Functional description
 *	Resolve a field reference entirely.
 *
 **************************************/

	// For ease, break down the syntax block.
	// It should contain at least one name; two names are a  potential ambiguity:
	// check for a qli_dbb (<db>.<glo_fld>), then for a rel (<rel>.<fld>).

	USHORT offset = field_node->syn_count;
	const qli_name* fld_name = (qli_name*) field_node->syn_arg[--offset];

	qli_name* rel_name = NULL;
	//qli_name* db_name = NULL;
	if (offset)
	{
		rel_name = (qli_name*) field_node->syn_arg[--offset];
		//if (offset)
		//	db_name = (qli_name*) field_node->syn_arg[--offset];
	}

    bool resolved = false;
    bool local = false;
    qli_fld* field = NULL;

	if (field_node->syn_count == 1)
		resolved = MET_declare(0, variable, fld_name);
	else if (field_node->syn_count == 2)
	{
		for (qli_symbol* symbol = rel_name->nam_symbol; symbol; symbol = symbol->sym_homonym)
		{
			if (symbol->sym_type == SYM_database)
			{
				qli_dbb* dbb = (qli_dbb*) symbol->sym_object;
				resolved = MET_declare(dbb, variable, fld_name);
				break;			// should be only one db in homonym list
			}
		}

		if (!resolved)
		{
			for (qli_dbb* dbb = QLI_databases; dbb && !resolved; dbb = dbb->dbb_next)
				for (qli_symbol* symbol = rel_name->nam_symbol; symbol; symbol = symbol->sym_homonym)
				{
					qli_rel* relation;
					if (symbol->sym_type == SYM_relation &&
						(relation = (qli_rel*) symbol->sym_object) && relation->rel_database == dbb)
					{
						if (!relation->rel_fields)
							MET_fields(relation);
						for (field = relation->rel_fields; field; field = field->fld_next)
						{
							resolved = local = compare_names(fld_name, field->fld_name);
							if (resolved)
								break;
						}
						break;	// should be only one rel in homonym list for each db
					}
				}
		}
	}
	else
	{
		qli_rel* relation = variable->fld_relation;
		if (!relation->rel_fields)
			MET_fields(relation);
		for (field = relation->rel_fields; field; field = field->fld_next)
		{
			resolved = local = compare_names(fld_name, field->fld_name);
			if (resolved)
				break;
		}
	}

	if (!resolved)
		IBERROR(155);
		// Msg155 field referenced in BASED ON can not be resolved against readied databases

	if (local)
	{
		variable->fld_dtype = field->fld_dtype;
		variable->fld_length = field->fld_length;
		variable->fld_scale = field->fld_scale;
		variable->fld_sub_type = field->fld_sub_type;
		variable->fld_sub_type_missing = field->fld_sub_type_missing;
		if (!variable->fld_edit_string)
			variable->fld_edit_string = field->fld_edit_string;
		if (!variable->fld_query_header)
			variable->fld_query_header = field->fld_query_header;
		if (!variable->fld_query_name)
			variable->fld_query_name = field->fld_query_name;
	}
}



