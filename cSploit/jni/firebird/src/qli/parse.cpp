/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		parse.cpp
 *	DESCRIPTION:	Statement parser
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../qli/dtr.h"
#include "../qli/exe.h"			// This is only included to suppress a compiler warning
#include "../qli/parse.h"
#include "../qli/compile.h"
#include "../qli/report.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/hsh_proto.h"
#include "../qli/lex_proto.h"
#include "../qli/mov_proto.h"
#include "../qli/parse_proto.h"
#include "../qli/proc_proto.h"
#include "../common/gdsassert.h"
#include "../jrd/constants.h"

using MsgFormat::SafeArg;


#define KEYWORD(kw)		(QLI_token->tok_keyword == kw)
#define INT_CAST		(qli_syntax*) (IPTR)

static void check_end();
static void command_end();
static qli_dbb* get_dbb(qli_symbol*);
static qli_syntax* make_list(qli_lls*);
static qli_name* make_name();
static qli_const* make_numeric_constant(const TEXT*, USHORT);
static TEXT* make_string(const TEXT*, USHORT);
static qli_syntax* negate(qli_syntax*);
static kwwords next_keyword();
static qli_syntax* parse_abort();
static qli_syntax* parse_accept();
static qli_syntax* parse_add(USHORT *, bool *);
static qli_syntax* parse_and(USHORT *);
static qli_syntax* parse_assignment();
static qli_syntax* parse_boolean(USHORT *);
static qli_syntax* parse_copy();
static qli_dbb* parse_database();
static qli_syntax* parse_declare();
static qli_syntax* parse_define();
static qli_syntax* parse_def_index();
static qli_syntax* parse_def_relation();
static qli_syntax* parse_delete();
static qli_syntax* parse_drop();
static int parse_dtype(USHORT *, USHORT *);
static int parse_dtype_subtype();
static qli_syntax* parse_edit();
static TEXT* parse_edit_string();
static qli_syntax* parse_erase();
static qli_syntax* parse_extract();
static qli_fld* parse_field(bool);
static qli_syntax* parse_field_name(qli_syntax**);
static qli_syntax* parse_for();
static qli_syntax* parse_from(USHORT*, bool*);
static qli_syntax* parse_function();
static TEXT* parse_header();
static qli_syntax* parse_help();
static qli_syntax* parse_if();
static qli_syntax* parse_in(qli_syntax*, nod_t, bool);
static qli_syntax* parse_insert();
static nod_t parse_join_type();
static qli_syntax* parse_list_fields();
static qli_const* parse_literal();
static qli_syntax* parse_matches();
static void parse_matching_paren();
static qli_syntax* parse_modify();
static qli_syntax* parse_modify_index();
static qli_syntax* parse_modify_relation();
static qli_syntax* parse_multiply(USHORT*, bool*);
static qli_name* parse_name();
static qli_syntax* parse_not(USHORT *);
static int parse_ordinal();
static qli_syntax* parse_output();
static qli_syntax* parse_primitive_value(USHORT*, bool*);
static qli_syntax* parse_print_list();
static qli_syntax* parse_print();
static qli_syntax* parse_prompt();
static qli_filter* parse_qualified_filter();
static qli_func* parse_qualified_function();
static qli_proc* parse_qualified_procedure();
static qli_rel* parse_qualified_relation();
static qli_syntax* parse_ready(nod_t);
static qli_syntax* parse_relational(USHORT*);
static qli_syntax* parse_relation();
static qli_syntax* parse_rename();
static qli_syntax* parse_repeat();
static qli_syntax* parse_report();
static qli_syntax* parse_rse();
static qli_syntax* parse_select();
static qli_syntax* parse_set();
static qli_syntax* parse_shell();
static qli_syntax* parse_show();
static qli_syntax* parse_sort();
static qli_syntax* parse_sql_alter();
static qli_syntax* parse_sql_create();
static int parse_sql_dtype(USHORT* length, USHORT* scale, USHORT* precision, USHORT* sub_type);
static qli_fld* parse_sql_field();
static qli_syntax* parse_sql_grant_revoke(const nod_t type);
static qli_syntax* parse_sql_index_create(const bool, const bool);
static qli_syntax* parse_sql_joined_relation(); //(qli_syntax*);
static qli_syntax* parse_sql_join_clause(qli_syntax*);
static qli_syntax* parse_sql_table_create();
#ifdef NOT_USED_OR_REPLACED
static qli_syntax* parse_sql_view_create();
#endif
static qli_syntax* parse_sql_relation();
static qli_syntax* parse_sql_rse();
static qli_syntax* parse_sql_singleton_select();
static qli_syntax* parse_sql_subquery();
static qli_syntax* parse_statement();
static qli_syntax* parse_statistical();
static qli_syntax* parse_store();
static TEXT* parse_string();
static qli_symbol* parse_symbol();
static void parse_terminating_parens(USHORT*, USHORT*);
static qli_syntax* parse_transaction(nod_t);
static qli_syntax* parse_udf_or_field();
static qli_syntax* parse_update();
static qli_syntax* parse_value(USHORT*, bool*);
static bool potential_rse();
static qli_rel* resolve_relation(const qli_symbol*, qli_symbol*);
static qli_syntax* syntax_node(nod_t, USHORT);
static bool test_end();

struct gds_quad
{
	SLONG gds_quad_high;
	ULONG gds_quad_low;
};

/*
The following flags are:

	sql_flag	indicates whether we are parsing in a SQL environment.
			The flag is used to turn off automatic end-of-command
			recognition.

	else_count	indicates the depth of if/then/else's

	sw_report 	indicates whether we're in a report statement

	sw_statement	indicates that we're actively parsing a statement/command

	sw_sql_view	indicates parsing a SQL VIEW, so restrict the select.
*/

static int sql_flag, else_count, sw_report;
static bool sw_statement, sw_sql_view;
static int function_count;	// indicates the depth of UDF calls

struct nod_types
{
	kwwords nod_t_keyword;
	nod_t nod_t_node;
	nod_t nod_t_rpt_node;
	nod_t nod_t_sql_node;
};

static const nod_types statisticals[] =
{
	{ KW_MAX, nod_max, nod_rpt_max, nod_agg_max },
	{ KW_MIN, nod_min, nod_rpt_min, nod_agg_min },
	{ KW_COUNT, nod_count, nod_rpt_count, nod_agg_count },
	{ KW_AVERAGE, nod_average, nod_rpt_average, nod_agg_average },
	{ KW_TOTAL, nod_total, nod_rpt_total, nod_agg_total },
	{ KW_none, nod_any, nod_any, nod_any }
};

static const nod_t relationals[] =
{
	nod_eql, nod_neq, nod_leq, nod_lss, nod_gtr, nod_geq, nod_containing,
	nod_like, nod_starts, nod_missing, nod_between, nod_sleuth,
	nod_matches, nod_and, nod_or, nod_not, nod_any, nod_unique, nod_nothing
};


qli_syntax* PARQ_parse()
{
/**************************************
 *
 *	P A R Q _ p a r s e
 *
 **************************************
 *
 * Functional description
 *	Parse a single statement or command.
 *
 **************************************/
	sql_flag = else_count = sw_report = 0;
	sw_statement = true;

	switch (next_keyword())
	{
	case KW_COMMIT:
		return parse_transaction(nod_commit);

	case KW_COPY:
		return parse_copy();

	case KW_EXIT:
		return syntax_node(nod_exit, 0);

	case KW_EXTRACT:
		return parse_extract();

	case KW_QUIT:
		return syntax_node(nod_quit, 0);

	case KW_DELETE:
	case KW_DROP:
		{
			qli_syntax* node = parse_drop();
			if (node)
				return node;
			node = parse_delete();
			check_end();
			if (!PAR_match(KW_THEN))
				return node;
			qli_lls* stack = NULL;
			ALLQ_push((blk*) node, &stack);
			ALLQ_push((blk*) parse_statement(), &stack);
			return make_list(stack);
		}

	case KW_DEFINE:
		return parse_define();

	case KW_CREATE:
		return parse_sql_create();

	case KW_ALTER:
		return parse_sql_alter();

	case KW_EDIT:
		return parse_edit();

	case KW_FINISH:
		return parse_transaction(nod_finish);

	case KW_GRANT:
		return parse_sql_grant_revoke(nod_sql_grant);

	case KW_HELP:
		return parse_help();

	case KW_PREPARE:
		return parse_transaction(nod_prepare);

	case KW_READY:
		return parse_ready(nod_ready);

	case KW_RENAME:
		return parse_rename();

	case KW_REVOKE:
		return parse_sql_grant_revoke(nod_sql_revoke);

	case KW_ROLLBACK:
		return parse_transaction(nod_rollback);

	case KW_SET:
		return parse_set();

	case KW_SHELL:
		return parse_shell();

	case KW_SHOW:
		return parse_show();
	}

	return parse_statement();
}


bool PAR_match( kwwords keyword)
{
/**************************************
 *
 *	P A R _ m a t c h
 *
 **************************************
 *
 * Functional description
 *	Test the current token for a particular keyword.
 *	If so, advance the token stream.
 *
 **************************************/
	if (KEYWORD(keyword))
	{
		PAR_token();
		return true;
	}

	for (const qli_symbol* symbol = QLI_token->tok_symbol; symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_keyword && symbol->sym_keyword == keyword)
		{
			PAR_token();
			return true;
		}
	}

	return false;
}


void PAR_real()
{
/**************************************
 *
 *	P A R _ r e a l
 *
 **************************************
 *
 * Functional description
 *	Get a real (not EOL) token.
 *
 *	If the token is an end of line, get the next token.
 *      If the next token is a colon, start reading the
 *      procedure.
 *
 **************************************/
	while ((QLI_token->tok_type == tok_eol) || KEYWORD(KW_continuation))
		LEX_token();

	if (PAR_match(KW_COLON))
	{
		qli_dbb* database = parse_database();
		PRO_invoke(database, QLI_token->tok_string);
	}
}


void PAR_real_token()
{
/**************************************
 *
 *	P A R _ r e a l _ t o k e n
 *
 **************************************
 *
 * Functional description
 *	Composition of PAR_token followed by PAR_real.
 *
 **************************************/

	PAR_token();
	PAR_real();
}


void PAR_token()
{
/**************************************
 *
 *	P A R _ t o k e n
 *
 **************************************
 *
 * Functional description
 *	Get the next token.
 *	If it's a procedure invocation, handle it
 *      or complain and get rid of the evidence.
 *
 **************************************/
	for (;;)
	{
		LEX_token();
		if (!(KEYWORD(KW_continuation)) &&
			!(sw_statement && QLI_semi && QLI_token->tok_type == tok_eol))
		{
			break;
		}
	}

	if (PAR_match(KW_COLON))
	{
		if (!QLI_databases)
		{
			ERRQ_error_format(159);	// Msg159 no databases are ready
			ERRQ_pending();
			LEX_token();
		}
		else
		{
			qli_dbb* database = parse_database();
			PRO_invoke(database, QLI_token->tok_string);
		}
	}
}


static void check_end()
{
/**************************************
 *
 *	c h e c k _ e n d
 *
 **************************************
 *
 * Functional description
 *	Check for end of statement.  If it isn't complain bitterly.
 *
 **************************************/

	if (QLI_token->tok_type == tok_eol || KEYWORD(KW_SEMI) || KEYWORD(KW_THEN) ||
		(else_count && KEYWORD(KW_ELSE)))
	{
		sw_statement = false;
		return;
	}

	ERRQ_syntax(161);			// Msg161 end of statement
}


static void command_end()
{
/**************************************
 *
 *	c o m m a n d _ e n d
 *
 **************************************
 *
 * Functional description
 *	Check for end of command.  If it isn't complain bitterly.
 *
 **************************************/

	if (QLI_token->tok_type == tok_eol || KEYWORD(KW_SEMI))
	{
		sw_statement = false;
		return;
	}

	ERRQ_syntax(162);			// Msg162 end of command
}


static qli_dbb* get_dbb( qli_symbol* symbol)
{
/**************************************
 *
 *	g e t _ d b b
 *
 **************************************
 *
 * Functional description
 *	Find a database block from a symbol
 *	or its homonyms.
 *
 **************************************/

	for (; symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_database)
			return (qli_dbb*) symbol->sym_object;
	}

	return NULL;
}


static qli_syntax* make_list( qli_lls* stack)
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

	qli_syntax* node = syntax_node(nod_list, count);
	qli_syntax** ptr = &node->syn_arg[count];

	while (stack)
		*--ptr = (qli_syntax*) ALLQ_pop(&stack);

	return node;
}


static qli_name* make_name()
{
/**************************************
 *
 *	m a k e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a unique name for something
 *	(like a database) that needs one.
 *
 **************************************/
	SSHORT l;
	TEXT string[32];

	for (SSHORT i = 0; i < 1000; i++)
	{
		sprintf(string, "QLI_%d", i);
		if (i < 10)
			l = 5;
		else
			l = (i < 100) ? 6 : 7;
		if (!(HSH_lookup(string, l)))
			break;
	}

	qli_name* name = (qli_name*) ALLOCDV(type_nam, l);
	name->nam_length = l;
	TEXT* p = name->nam_string;
	const TEXT* q = string;

	if (l)
		do {
			const TEXT c = *q++;
			*p++ = UPPER(c);
		} while (--l);

	return name;
}


static qli_const* make_numeric_constant(const TEXT* string, USHORT length)
{
/**************************************
 *
 *	m a k e _ n u m e r i c _ c o n s t a n t
 *
 **************************************
 *
 * Functional description
 *	Build a constant block for a numeric
 *	constant.  Numeric constants are normally
 *	stored as long words, but especially large
 *	ones become text.  They ought to become
 *      double precision, one would think, but they'd
 *      have to be VAX style double precision which is
 *      more pain than gain.
 *
 **************************************/
	qli_const* constant;

	// If there are a reasonable number of digits, convert to binary

	if (length < 9)
	{
		constant = (qli_const*) ALLOCDV(type_con, sizeof(SLONG));
		constant->con_desc.dsc_dtype = dtype_long;
		constant->con_desc.dsc_length = sizeof(SLONG);
		constant->con_desc.dsc_address = constant->con_data;
		constant->con_desc.dsc_scale = MOVQ_decompose(string, length, (SLONG*) constant->con_data);
	}
	else
	{
		++length;
		constant = (qli_const*) ALLOCDV(type_con, length);
		constant->con_desc.dsc_dtype = dtype_text;
		constant->con_desc.dsc_length = length;
		constant->con_desc.dsc_address = constant->con_data;
		TEXT* p = (TEXT*) constant->con_desc.dsc_address;
		*p++ = '0';
		memcpy(p, string, --length);
	}
	QLI_validate_desc(constant->con_desc);

	return constant;
}


static TEXT* make_string(const TEXT* address, USHORT length)
{
/**************************************
 *
 *	m a k e _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Copy a string into a temporary string block.
 *
 **************************************/
	qli_str* string = (qli_str*) ALLOCDV(type_str, length);
	if (length)
		memcpy(string->str_data, address, length);

	return string->str_data;
}


static qli_syntax* negate( qli_syntax* expr)
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
	qli_syntax* node = syntax_node(nod_not, 1);
	node->syn_arg[0] = expr;

	return node;
}


static kwwords next_keyword()
{
/**************************************
 *
 *	n e x t _ k e y w o r d
 *
 **************************************
 *
 * Functional description
 *	Get a real token and return the keyword number.
 *
 **************************************/
	PAR_real();

	for (const qli_symbol* symbol = QLI_token->tok_symbol; symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_keyword)
			return (kwwords) symbol->sym_keyword;
	}

	return KW_none;
}


static qli_syntax* parse_abort()
{
/**************************************
 *
 *	p a r s e _ a b o r t
 *
 **************************************
 *
 * Functional description
 *	Parse an ABORT statement.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_abort, 1);

	if (KEYWORD(KW_SEMI))
		node->syn_count = 0;
	else
		node->syn_arg[0] = parse_value(0, 0);

	return node;
}


static qli_syntax* parse_accept()
{
/**************************************
 *
 *	p a r s e _ a c c e p t
 *
 **************************************
 *
 * Functional description
 *	Parse form update statement.
 *
 **************************************/
	IBERROR(484);				// FORMs not supported
	return 0;
}


static qli_syntax* parse_add( USHORT* paren_count, bool* bool_flag)
{
/**************************************
 *
 *	p a r s e _ a d d
 *
 **************************************
 *
 * Functional description
 *	Parse the lowest precedence operatrs, ADD and SUBTRACT.
 *
 **************************************/
	nod_t operatr;

	qli_syntax* node = parse_multiply(paren_count, bool_flag);

	while (true)
	{
		if (PAR_match(KW_PLUS))
			operatr = nod_add;
		else if (PAR_match(KW_MINUS))
			operatr = nod_subtract;
		else
			return node;
		qli_syntax* arg = node;
		node = syntax_node(operatr, 2);
		node->syn_arg[0] = arg;
		node->syn_arg[1] = parse_multiply(paren_count, bool_flag);
	}
}


static qli_syntax* parse_and( USHORT* paren_count)
{
/**************************************
 *
 *	p a r s e _ a n d
 *
 **************************************
 *
 * Functional description
 *	Parse an AND expression.
 *
 **************************************/
	qli_syntax* expr = parse_not(paren_count);

	/*
	while (*paren_count && KEYWORD (KW_RIGHT_PAREN))
	{
		parse_matching_paren();
		(*paren_count)--;
	}
	*/

	if (!PAR_match(KW_AND))
		return expr;

	qli_syntax* node = syntax_node(nod_and, 2);
	node->syn_arg[0] = expr;
	node->syn_arg[1] = parse_and(paren_count);

	return node;
}


static qli_syntax* parse_assignment()
{
/**************************************
 *
 *	p a r s e _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Parse an assignment statement (or give an error).  The
 *	assignment statement can be either a simple assignment
 *	(field = value) or a restructure (relation = rse).
 *	If the assignment operator is missing,
 *	generate an "expected statement" error.
 *
 **************************************/
	qli_syntax* field = NULL;

	qli_syntax* node = syntax_node(nod_assign, s_asn_count);
	node->syn_arg[s_asn_to] = parse_field_name(&field);
	qli_name* name = (qli_name*) field->syn_arg[0];

	// If the next token is an equals sign, the statement really is an
	// assignment, and we're off the hook.

	if (!PAR_match(KW_EQUALS))
		ERRQ_print_error(156, name->nam_string);	// Msg156 expected statement, encountered %s

	// See if the "field name" is really a relation reference.  If so,
	// turn the assignment into a restructure.

	qli_rel* relation = NULL;
	if (field->syn_count == 1)
		relation = resolve_relation(0, name->nam_symbol);
	else if (field->syn_count == 2 && name->nam_symbol)
	{
		qli_name* name2 = (qli_name*) field->syn_arg[1];
		relation = resolve_relation(name->nam_symbol, name2->nam_symbol);
	}

	if (relation)
	{
		ALLQ_release((qli_frb*) field);
		node->syn_type = nod_restructure;
		node->syn_arg[s_asn_to] = field = syntax_node(nod_relation, s_rel_count);
		field->syn_arg[s_rel_relation] = (qli_syntax*) relation;
		node->syn_arg[s_asn_from] = parse_rse();
	}
	else
		node->syn_arg[s_asn_from] = parse_value(0, 0);

	return node;
}


static qli_syntax* parse_boolean( USHORT * paren_count)
{
/**************************************
 *
 *	p a r s e _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Parse a general boolean expression.  By precedence, handle an OR
 *	here.
 *
 **************************************/
	USHORT local_count;

	if (!paren_count)
	{
		local_count = 0;
		paren_count = &local_count;
	}

	qli_syntax* expr = parse_and(paren_count);

	/*
	while (*paren_count && KEYWORD (KW_RIGHT_PAREN))
	{
		parse_matching_paren();
		(*paren_count)--;
	}
	*/

	if (!PAR_match(KW_OR))
	{
		parse_terminating_parens(paren_count, &local_count);
		return expr;
	}

	qli_syntax* node = syntax_node(nod_or, 2);
	node->syn_arg[0] = expr;
	node->syn_arg[1] = parse_boolean(paren_count);
	parse_terminating_parens(paren_count, &local_count);

	return node;
}


static qli_syntax* parse_copy()
{
/**************************************
 *
 *	p a r s e _ c o p y
 *
 **************************************
 *
 * Functional description
 *	Parse a copy command, which copies
 *	one procedure to another.
 *
 **************************************/
	PAR_real_token();

	if (PAR_match(KW_PROCEDURE))
	{
		qli_syntax* node = syntax_node(nod_copy_proc, 2);
		node->syn_arg[0] = (qli_syntax*) parse_qualified_procedure();
		PAR_match(KW_TO);
		node->syn_arg[1] = (qli_syntax*) parse_qualified_procedure();
		return node;
	}

	ERRQ_print_error(157, QLI_token->tok_string);	// Msg157 Expected PROCEDURE encountered %s
	return NULL;
}


static qli_dbb* parse_database()
{
/**************************************
 *
 *	p a r s e _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Pick up a database for a meta-data or
 *	procedure update.  Return NULL if the
 *	token is not a database name.
 *
 **************************************/
	qli_symbol* db_symbol = QLI_token->tok_symbol;

	if (db_symbol && db_symbol->sym_type == SYM_database)
	{
		qli_dbb* database = (qli_dbb*) db_symbol->sym_object;
		PAR_real_token();
		if (!PAR_match(KW_DOT))
			ERRQ_syntax(158);	// Msg158 period in qualified name
		PAR_real();
		return database;
	}

	if (!QLI_databases)
		IBERROR(159);			// Msg159 no databases are ready

	return NULL;
}


static qli_syntax* parse_declare()
{
/**************************************
 *
 *	p a r s e _ d e c l a r e
 *
 **************************************
 *
 * Functional description
 *	Parse a variable  declaration.
 *
 **************************************/
	PAR_token();
	PAR_real();

	USHORT dtype = 0, length = 0, scale = 0;
	SSHORT sub_type = 0;
	SSHORT sub_type_missing = 1;
	qli_syntax* field_node = NULL;
	qli_symbol* query_name = NULL;
	const TEXT* edit_string = NULL;
	const TEXT* query_header = NULL;

	qli_symbol* name = parse_symbol();

	// if (global_flag)
	//	PAR_real();

	while (!KEYWORD(KW_SEMI) && !KEYWORD(KW_COMMA))
	{
		PAR_real();
		switch (QLI_token->tok_keyword)
		{
		case KW_SHORT:
		case KW_LONG:
		case KW_FLOAT:
		case KW_DOUBLE:
		case KW_DATE:
		case KW_CHAR:
		case KW_VARYING:
			if (dtype)
				ERRQ_syntax(164);	// Msg164 variable definition clause
			dtype = parse_dtype(&length, &scale);
			break;

		case KW_BLOB:
			IBERROR(160);		// Msg160 blob variables are not supported
			break;

		case KW_SUB_TYPE:
			sub_type = parse_dtype_subtype();
			sub_type_missing = 0;
			break;

		case KW_EDIT_STRING:
			PAR_token();
			if (QLI_token->tok_type != tok_quoted)
				ERRQ_syntax(163);	// Msg163 quoted edit string
			edit_string = make_string(QLI_token->tok_string + 1, QLI_token->tok_length - 2);
			PAR_token();
			break;

		case KW_QUERY_NAME:
			PAR_token();
			PAR_match(KW_IS);
			if (QLI_token->tok_type != tok_ident)
				ERRQ_syntax(199);	// Msg199 identifier
			query_name = parse_symbol();
			break;

		case KW_QUERY_HEADER:
			PAR_token();
			query_header = parse_header();
			break;

		case KW_BASED:
			PAR_token();
			PAR_match(KW_ON);
			field_node = parse_field_name(0);
			break;

		default:
			ERRQ_syntax(164);	// Msg164 variable definition clause
			break;
		}
	}

	qli_rel* relation = NULL;
	if (field_node && field_node->syn_count == 3)
	{
		qli_name* db_name = (qli_name*) field_node->syn_arg[0];
		qli_name* rel_name = (qli_name*) field_node->syn_arg[1];
		if (!db_name->nam_symbol)
			ERRQ_print_error(165, db_name->nam_string);
			// Msg165 %s is not a database

		relation = resolve_relation(db_name->nam_symbol, rel_name->nam_symbol);
		if (!relation)
		{
			ERRQ_print_error(166, SafeArg() << rel_name->nam_string << db_name->nam_string);
			// Msg166 %s is not a relation in database %s
		}
	}

	if (!dtype && !field_node)
		ERRQ_syntax(167);		// Msg167 variable data type
	if (field_node && (dtype || length || scale))
		IBERROR(168);			// Msg168 no datatype may be specified for a variable based on a field

	qli_syntax* node = syntax_node(nod_declare, 2);
	// Not global to this unit, misleading name "global..."
	qli_fld* global_variable = (qli_fld*) ALLOCDV(type_fld, length);
	node->syn_arg[0] = (qli_syntax*) global_variable;
	global_variable->fld_name = name;
	global_variable->fld_dtype = dtype;
	global_variable->fld_scale = scale;
	global_variable->fld_sub_type = sub_type;
	global_variable->fld_sub_type_missing = sub_type_missing;
	global_variable->fld_length = length;
	global_variable->fld_edit_string = edit_string;
	global_variable->fld_query_name = query_name;
	global_variable->fld_query_header = query_header;
	global_variable->fld_relation = relation;

	node->syn_arg[1] = field_node;

	check_end();

	return node;
}


static qli_syntax* parse_define()
{
/**************************************
 *
 *	p a r s e _ d e f i n e
 *
 **************************************
 *
 * Functional description
 *	Parse a DEFINE command.
 *	There are, of course, a whole class of define commands.
 *
 **************************************/
	PAR_real_token();

	if (PAR_match(KW_PROCEDURE))
	{
		PAR_real();
		qli_syntax* anode = syntax_node(nod_define, 1);
		anode->syn_arg[0] = (qli_syntax*) parse_qualified_procedure();
		return anode;
	}

	if (PAR_match(KW_FIELD))
	{
		PAR_real();
		qli_syntax* node = syntax_node(nod_def_field, 2);
		node->syn_arg[0] = (qli_syntax*) parse_database();
		node->syn_arg[1] = (qli_syntax*) parse_field(true);
		return node;
	}

	if (PAR_match(KW_RELATION))
		return parse_def_relation();

	if (KEYWORD(KW_DATABASE))
		return parse_ready(nod_def_database);

	if (PAR_match(KW_INDEX))
		return parse_def_index();

	ERRQ_syntax(169);			// Msg169 object type for DEFINE
	return NULL;
}


static qli_syntax* parse_def_index()
{
/**************************************
 *
 *	p a r s e _ d e f _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Parse a DEFINE INDEX command.
 *
 **************************************/
	PAR_real();
	qli_syntax* node = syntax_node(nod_def_index, s_dfi_count);
	node->syn_arg[s_dfi_name] = (qli_syntax*) parse_symbol();
	PAR_real();
	PAR_match(KW_FOR);

	if (!(node->syn_arg[s_dfi_relation] = (qli_syntax*) parse_qualified_relation()))
		ERRQ_syntax(170);		// Msg170 relation name

	PAR_real();

	while (true)
	{
		PAR_real();
		if (PAR_match(KW_UNIQUE))
			node->syn_flags |= s_dfi_flag_unique;
		else if (PAR_match(KW_DUPLICATE))
			node->syn_flags &= ~s_dfi_flag_unique;
		else if (PAR_match(KW_ACTIVE))
			node->syn_flags &= ~s_dfi_flag_inactive;
		else if (PAR_match(KW_INACTIVE))
			node->syn_flags |= s_dfi_flag_inactive;
		else if (PAR_match(KW_DESCENDING))
			node->syn_flags |= s_dfi_flag_descending;
		else if (PAR_match(KW_ASCENDING))
			node->syn_flags &= ~s_dfi_flag_descending;
		else
			break;
	}

	qli_lls* stack = NULL;
	for (;;)
	{
		ALLQ_push((blk*) parse_name(), &stack);
		if (!PAR_match(KW_COMMA))
			break;
	}
	node->syn_arg[s_dfi_fields] = make_list(stack);

	command_end();
	return node;
}


static qli_syntax* parse_def_relation()
{
/**************************************
 *
 *	p a r s e _ d e f _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a DEFINE RELATION command,
 *	which include the field definitions
 *	for a primitive relation definition
 *	or it may just reference another relation
 *	whose field definitions we will copy.
 *
 **************************************/
	PAR_real();
	qli_syntax* node = syntax_node(nod_def_relation, 2);
	qli_rel* relation = (qli_rel*) ALLOCD(type_rel);
	node->syn_arg[0] = (qli_syntax*) relation;
	relation->rel_database = parse_database();
	relation->rel_symbol = parse_symbol();
	PAR_real();

	if (PAR_match(KW_BASED))
	{
		PAR_real();
		PAR_match(KW_ON);
		PAR_real();
		PAR_match(KW_RELATION);
		PAR_real();
		relation = (qli_rel*) ALLOCD(type_rel);
		node->syn_arg[1] = (qli_syntax*) relation;
		relation->rel_database = parse_database();
		relation->rel_symbol = parse_symbol();
	}
	else
	{
		node->syn_arg[1] = NULL;
		qli_fld** ptr = &relation->rel_fields;
		for (;;)
		{
			PAR_match(KW_ADD);
			PAR_real();
			PAR_match(KW_FIELD);
			qli_fld* field = parse_field(false);
			*ptr = field;
			ptr = &field->fld_next;
			if (KEYWORD(KW_SEMI))
				break;
			if (!PAR_match(KW_COMMA))
				ERRQ_syntax(171);	// Msg171 comma between field definitions
		}
	}

	command_end();
	return node;
}


static qli_syntax* parse_delete()
{
/**************************************
 *
 *	p a r s e _ d e l e t e
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL DELETE statement.
 *	(DELETE PROCEDURE is parsed in parse_drop)
 *
 **************************************/
	++sql_flag;

	if (!PAR_match(KW_FROM))
		ERRQ_syntax(172);		// Msg172 FROM

	qli_syntax* node = syntax_node(nod_erase, s_era_count);
	qli_syntax* rse = syntax_node(nod_rse, (int) s_rse_count + 2);
	node->syn_arg[s_era_rse] = rse;

	rse->syn_count = 1;
	rse->syn_arg[s_rse_count] = parse_sql_relation();

	// Pick up boolean, if present

	if (PAR_match(KW_WITH))
		rse->syn_arg[s_rse_boolean] = parse_boolean(0);

	--sql_flag;

	return node;
}


static qli_syntax* parse_drop()
{
/**************************************
 *
 *	p a r s e _ d r o p
 *
 **************************************
 *
 * Functional description
 *	Parse a DDL DROP/DELETE command.  It it isn't one,
 *	just return NULL.
 *
 **************************************/
	qli_syntax* node;
	nod_t type;
	qli_dbb* database;
	SSHORT l;
	const TEXT* q;

	PAR_real_token();

	if (PAR_match(KW_RELATION) || PAR_match(KW_VIEW) || PAR_match(KW_TABLE))
	{
		node = syntax_node(nod_del_relation, 1);
		if (!(node->syn_arg[0] = (qli_syntax*) parse_qualified_relation()))
			ERRQ_syntax(173);	// Msg173 relation or view name
		return node;
	}

	switch (QLI_token->tok_keyword)
	{
	case KW_PROCEDURE:
		type = nod_delete_proc;
		break;

	case KW_INDEX:
		type = nod_del_index;
		break;

	case KW_FIELD:
		type = nod_del_field;
		break;

	case KW_DATABASE:
		LEX_filename();
		if (!(l = QLI_token->tok_length))
			ERRQ_error(429);	// Msg429 database file name required on DROP DATABASE
		q = QLI_token->tok_string;
		if (QLI_token->tok_type == tok_quoted)
		{
			l -= 2;
			q++;
		}
		database = (qli_dbb*) ALLOCDV(type_dbb, l);
		database->dbb_filename_length = l;
		memcpy(database->dbb_filename, q, l);
		PAR_token();

		// parse an optional user name and password if given

		for (;;)
		{
			if (PAR_match(KW_USER))
				database->dbb_user = parse_literal();
			else if (PAR_match(KW_PASSWORD))
				database->dbb_password = parse_literal();
			else
				break;
		}

		command_end();
		node = syntax_node(nod_del_database, 1);
		node->syn_arg[0] = (qli_syntax*) database;
		return node;

	default:
		return NULL;
	}

	PAR_real_token();
	node = syntax_node(type, 2);

	if (type == nod_delete_proc)
		node->syn_arg[0] = (qli_syntax*) parse_qualified_procedure();
	else
	{
		node->syn_arg[0] = (qli_syntax*) parse_database();
		node->syn_arg[1] = (qli_syntax*) parse_name();
	}

	return node;
}


static int parse_dtype( USHORT * length, USHORT * scale)
{
/**************************************
 *
 *	p a r s e _ d t y p e
 *
 **************************************
 *
 * Functional description
 *	Parse a datatype clause.
 *
 **************************************/
	USHORT dtype;

	const kwwords keyword = QLI_token->tok_keyword;
	PAR_token();
	*scale = 0;

	switch (keyword)
	{
	case KW_SHORT:
		*length = sizeof(SSHORT);
		dtype = dtype_short;
		break;

   	case KW_BIGINT:
		*length = sizeof(SINT64);
		dtype = dtype_int64;
		break;

	case KW_LONG:
		*length = sizeof(SLONG);
		dtype = dtype_long;
		break;

	case KW_FLOAT:
		*length = sizeof(float);
		return dtype_real;

	case KW_DOUBLE:
		*length = sizeof(double);
		return dtype_double;

	case KW_DATE:
		*length = sizeof(gds_quad);
		return dtype_timestamp;

	case KW_CHAR:
		dtype = dtype_text;
		break;

	case KW_VARYING:
		dtype = dtype_varying;
		break;

	case KW_BLOB:
		*length = sizeof(gds_quad);
		return dtype_blob;
	}

	switch (dtype)
	{
	case dtype_short:
	case dtype_long:
	case dtype_int64:
		if (PAR_match(KW_SCALE))
		{
			const bool m = PAR_match(KW_MINUS) ? true : false;
			*scale = parse_ordinal();
			if (m)
				*scale = -(*scale);
		}
		break;
	case dtype_text:
	case dtype_varying:
		{
			if (!PAR_match(KW_L_BRCKET) && !PAR_match(KW_LT))
				ERRQ_syntax(174);	// Msg174 "["

			USHORT l = parse_ordinal();
			if (dtype == dtype_varying)
				l += sizeof(SSHORT);
			*length = l;

			if (!PAR_match(KW_R_BRCKET) && !PAR_match(KW_GT))
				ERRQ_syntax(175);	// Msg175 "]"
		}
	}

	return dtype;
}


static int parse_dtype_subtype()
{
/**************************************
 *
 *	p a r s e _ d t y p e _ s u b t y p e
 *
 **************************************
 *
 * Functional description
 *	Parse a sub-type definition, which can be any of
 *	SUB_TYPE {IS} [TEXT | FIXED | <n>]
 *
 *	Returns the numeric subtype value,
 *
 **************************************/
	// grab KW_SUB_TYPE

	PAR_token();
	PAR_match(KW_IS);
	if (PAR_match(KW_TEXT) || PAR_match(KW_FIXED))
		return 1;

	const int sign = PAR_match(KW_MINUS) ? -1 : 1;

	return (sign * parse_ordinal());
}


static qli_syntax* parse_edit()
{
/**************************************
 *
 *	p a r s e _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Parse an edit statement which can
 *	be any of  EDIT <procedure_name>
 *	           EDIT <n>
 *		   EDIT <*>
 *		   EDIT
 *
 *
 **************************************/
	LEX_token();

	// edit previous statements.  The top of the statment list
	// is this edit command, which we conveniently ignore.

	if (KEYWORD(KW_SEMI) || (QLI_token->tok_type == tok_number) || (KEYWORD(KW_ASTERISK)))
	{
	    qli_lls* statement_list = LEX_statement_list();
		if (!statement_list)
			IBERROR(176);		// Msg176 No statements issued yet

		if (PAR_match(KW_ASTERISK))
			LEX_edit(0, (IPTR) statement_list->lls_object);
		else
		{
		    int l = 0; // initialize, will catch changes in logic here.
			if (KEYWORD(KW_SEMI))
				l = 1;
			else if (QLI_token->tok_type == tok_number) // redundant for now
				l = parse_ordinal();

			qli_lls* start = statement_list;
			qli_lls* stop = start;
			while (l && start->lls_next)
			{
				--l;
				start = start->lls_next;
			}
			command_end();
			LEX_edit((IPTR) start->lls_object, (IPTR) stop->lls_object);
		}
	}
	else
	{
		const nod_t type = nod_edit_proc;
		qli_syntax* node = syntax_node(type, 2);
		node->syn_arg[0] = (qli_syntax*) parse_qualified_procedure();
		command_end();
		return node;
	}

	return NULL;
}


static TEXT* parse_edit_string()
{
/**************************************
 *
 *	p a r s e _ e d i t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Look for and parse a clause of the form:
 *	    USING <edit_string>
 *
 **************************************/

	if (!KEYWORD(KW_USING))
		return NULL;

	LEX_edit_string();

	return parse_string();
}


static qli_syntax* parse_erase()
{
/**************************************
 *
 *	p a r s e _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Parse an ERASE statement.  Erase can be any of the
 *	following:
 *
 *	ERASE [ALL] [OF <rse>]
 *
 **************************************/
	PAR_token();
	PAR_match(KW_ALL);
	PAR_match(KW_OF);
	qli_syntax* node = syntax_node(nod_erase, s_era_count);

	if (PAR_match(KW_ALL) || potential_rse())
	{
		PAR_match(KW_OF);
		node->syn_arg[s_era_rse] = parse_rse();
	}

	return node;
}


static qli_syntax* parse_extract()
{
/**************************************
 *
 *	p a r s e _ e x t r a c t
 *
 **************************************
 *
 * Functional description
 *	Parse a procedure extract statement.  Syntax is:
 *
 *	EXTRACT [ON <file>] proc [, ...] [ON <file> ]
 *
 **************************************/
	PAR_real_token();
	qli_syntax* node = syntax_node(nod_extract, 2);
	node->syn_arg[1] = parse_output();

	if (!PAR_match(KW_ALL))
	{
		qli_lls* stack = NULL;
		for (;;)
		{
			ALLQ_push((blk*) parse_qualified_procedure(), &stack);
			if (!PAR_match(KW_COMMA))
				break;
		}
		node->syn_arg[0] = make_list(stack);
	}

	if (!node->syn_arg[1] && !(node->syn_arg[1] = parse_output()))
		ERRQ_syntax(177);		// Msg177 "ON or TO"

	return node;
}


static qli_fld* parse_field( bool global_flag)
{
/**************************************
 *
 *	p a r s e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Parse a field description.
 *
 **************************************/
	PAR_real();

	USHORT dtype = 0, length = 0, scale = 0;
	SSHORT sub_type = 0;
	SSHORT sub_type_missing = 1;
	qli_symbol* query_name = NULL;
	qli_symbol* based_on = NULL;
	const TEXT* edit_string = NULL;
	const TEXT* query_header = NULL;
	qli_symbol* name = parse_symbol();

	if (global_flag)
		PAR_real();

	while (!KEYWORD(KW_SEMI) && !KEYWORD(KW_COMMA))
	{
		PAR_real();
		switch (QLI_token->tok_keyword)
		{
		case KW_SHORT:
		case KW_LONG:
		case KW_FLOAT:
		case KW_DOUBLE:
		case KW_DATE:
		case KW_CHAR:
		case KW_VARYING:
		case KW_BLOB:
			if (dtype)
				ERRQ_syntax(179);	// Msg179 field definition clause
			dtype = parse_dtype(&length, &scale);
			break;

		case KW_SUB_TYPE:
			sub_type = parse_dtype_subtype();
			sub_type_missing = 0;
			break;

		case KW_EDIT_STRING:
			PAR_token();
			if (QLI_token->tok_type != tok_quoted)
				ERRQ_syntax(178);	// Msg178 quoted edit string
			edit_string = make_string(QLI_token->tok_string + 1, QLI_token->tok_length - 2);
			PAR_token();
			break;

		case KW_QUERY_NAME:
			PAR_token();
			PAR_match(KW_IS);
			if (QLI_token->tok_type != tok_ident)
				ERRQ_syntax(199);	// Msg199 identifier
			query_name = parse_symbol();
			break;

		case KW_BASED:
			PAR_token();
			PAR_match(KW_ON);
			based_on = parse_symbol();
			break;

		default:
			ERRQ_syntax(179);	// Msg179 field definition clause
			break;
		}
	}

	qli_fld* field = (qli_fld*) ALLOCDV(type_fld, length);
	field->fld_name = name;
	field->fld_dtype = dtype;
	field->fld_scale = scale;
	field->fld_sub_type = sub_type;
	field->fld_sub_type_missing = sub_type_missing;
	field->fld_length = length;
	field->fld_edit_string = edit_string;
	field->fld_query_name = query_name;
	field->fld_query_header = query_header;
	if (!global_flag)
		field->fld_based = based_on;
	else if (based_on)
		IBERROR(180);			// Msg180 global fields may not be based on other fields

	return field;
}


static qli_syntax* parse_field_name( qli_syntax** fld_ptr)
{
/**************************************
 *
 *	p a r s e _ f i e l d _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Parse a qualified field name, or
 *	qualified * expression.
 *
 **************************************/
	qli_lls* stack = NULL;

	while (true)
	{
		if (PAR_match(KW_ASTERISK))
		{
			if (!stack)
				ERRQ_syntax(181);	// Msg181 field name or asterisk expression
			qli_syntax* afield = make_list(stack);
			afield->syn_type = nod_star;
			return afield;
		}
		ALLQ_push((blk*) parse_name(), &stack);
		if (!PAR_match(KW_DOT))
			break;
	}

	qli_syntax* field = make_list(stack);
	field->syn_type = nod_field;
	if (fld_ptr)
		*fld_ptr = field;
	if (!(PAR_match(KW_L_BRCKET)))
		return field;

	// Parse an array reference

	stack = NULL;
	for (;;)
	{
		ALLQ_push((blk*) parse_value(0, 0), &stack);
		if (PAR_match(KW_R_BRCKET))
			break;
		if (!PAR_match(KW_COMMA))
			ERRQ_syntax(183);	// Msg183 comma
	}

	qli_syntax* node = syntax_node(nod_index, s_idx_count);
	node->syn_arg[s_idx_field] = field;
	node->syn_arg[s_idx_subs] = make_list(stack);

	return node;
}


static qli_syntax* parse_for()
{
/**************************************
 *
 *	p a r s e _ f o r
 *
 **************************************
 *
 * Functional description
 *	Parse a FOR statement.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_for, s_for_count);
	node->syn_arg[s_for_rse] = parse_rse();
	node->syn_arg[s_for_statement] = parse_statement();

	return node;
}


static qli_syntax* parse_from( USHORT* paren_count, bool* bool_flag)
{
/**************************************
 *
 *	p a r s e _ f r o m
 *
 **************************************
 *
 * Functional description
 *	Parse either an explicit or implicit FIRST ... FROM statement.
 *
 **************************************/
	qli_syntax* value;

	PAR_real();

	if (PAR_match(KW_FIRST))
	{
		value = parse_primitive_value(0, 0);
		PAR_real();
		if (!PAR_match(KW_FROM))
			ERRQ_syntax(182);	// Msg182 FROM rse clause
	}
	else
	{
		value = parse_primitive_value(paren_count, bool_flag);
		if (sql_flag || !PAR_match(KW_FROM))
			return value;
	}

	qli_syntax* node = syntax_node(nod_from, s_stt_count);
	node->syn_arg[s_stt_value] = value;
	node->syn_arg[s_stt_rse] = parse_rse();

	if (PAR_match(KW_ELSE))
		node->syn_arg[s_stt_default] = parse_value(0, 0);

	return node;
}


static qli_syntax* parse_function()
{
/**************************************
 *
 *	p a r s e _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a function reference.
 *
 **************************************/
	function_count++;
	qli_syntax* node = syntax_node(nod_function, s_fun_count);
	node->syn_arg[s_fun_function] = (qli_syntax*) QLI_token->tok_symbol;
	node->syn_count = 1;
	PAR_token();
	qli_lls* stack = NULL;

	if (PAR_match(KW_LEFT_PAREN))
	{
		for (;;)
		{
			ALLQ_push((blk*) parse_value(0, 0), &stack);
			if (PAR_match(KW_RIGHT_PAREN))
				break;
			if (!PAR_match(KW_COMMA))
				ERRQ_syntax(183);	// Msg183 comma
		}
	}

	node->syn_arg[s_fun_args] = make_list(stack);
	function_count--;

	return node;
}


static TEXT* parse_header()
{
/**************************************
 *
 *	p a r s e _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Parse and store headers of the form:
 *		"quoted_string" [/ "more_string"]...
 *		or even the non-header -
 *
 **************************************/
	TEXT header[1024];

	TEXT* p = header;
	const TEXT* end = p + sizeof(header);

	while (true)
	{
		PAR_real();
		if ((QLI_token->tok_keyword != KW_MINUS) && (QLI_token->tok_type != tok_quoted))
		{
			ERRQ_syntax(184);	// Msg184 quoted header segment
		}
		const TEXT* q = QLI_token->tok_string;
		while (*q && p < end)
			*p++ = *q++;

		if (p == end && *q)
		    ERRQ_syntax(184); // Msg184 quoted header segment

		PAR_real_token();
		if (!PAR_match(KW_SLASH))
			break;
	}

	return make_string(header, p - header);
}


static qli_syntax* parse_help()
{
/**************************************
 *
 *	p a r s e _ h e l p
 *
 **************************************
 *
 * Functional description
 *	Parse HELP statement.  Unreasonable, but the masses
 *	must be appeased.  Bread, circuses, help.
 *
 **************************************/
	qli_lls* stack = NULL;
	PAR_token();

	while (!KEYWORD(KW_SEMI))
	{
		ALLQ_push((blk*) parse_name(), &stack);
		PAR_match(KW_COMMA);
	}

	qli_syntax* node = make_list(stack);
	node->syn_type = nod_help;
	command_end();

	return node;
}


static qli_syntax* parse_if()
{
/**************************************
 *
 *	p a r s e _ i f
 *
 **************************************
 *
 * Functional description
 *	Parse an IF THEN ELSE statement.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_if, s_if_count);
	node->syn_arg[s_if_boolean] = parse_boolean(0);
	PAR_real();
	PAR_match(KW_THEN);
	++else_count;
	node->syn_arg[s_if_true] = parse_statement();
	--else_count;

	if (PAR_match(KW_ELSE))
		node->syn_arg[s_if_false] = parse_statement();

	return node;
}


static qli_syntax* parse_in( qli_syntax* value, nod_t operatr, bool all_flag)
{
/**************************************
 *
 *	p a r s e _ i n
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL "IN" clause.  This can have two forms:
 *
 *		value IN (exp1, exp2...)
 *
 *		value IN (column <select expression>)
 *
 **************************************/
	PAR_real();

	if (!PAR_match(KW_LEFT_PAREN))
		ERRQ_syntax(185);		// Msg185 left parenthesis

	// Time to chose between two forms of the expression

	if (!PAR_match(KW_SELECT))
	{
		qli_syntax* node1 = syntax_node(operatr, 2);
		node1->syn_arg[0] = value;
		node1->syn_arg[1] = parse_primitive_value(0, 0);
		while (PAR_match(KW_COMMA))
		{
			qli_syntax* node2 = node1;
			node1 = syntax_node(nod_or, 2);
			node1->syn_arg[0] = node2;
			node1->syn_arg[1] = node2 = syntax_node(nod_eql, 2);
			node2->syn_arg[0] = value;
			node2->syn_arg[1] = parse_value(0, 0);
		}
		parse_matching_paren();
		return node1;
	}

	qli_syntax* value2 = parse_value(0, 0);

	// We have the "hard" -- an implicit ANY

	qli_syntax* rse = parse_sql_rse();
	parse_matching_paren();

	rse->syn_arg[s_rse_outer] = value;
	rse->syn_arg[s_rse_inner] = value2;
	rse->syn_arg[s_rse_op] = INT_CAST operatr;
	rse->syn_arg[s_rse_all_flag] = INT_CAST (all_flag ? TRUE: FALSE);

	// Finally, construct an ANY node

	qli_syntax* node = syntax_node(nod_any, 1);
	node->syn_arg[0] = rse;

	return all_flag ? negate(node) : node;
}


static qli_syntax* parse_insert()
{
/**************************************
 *
 *	p a r s e _ i n s e r t
 *
 **************************************
 *
 * Functional description
 *	Parse a STORE statement.
 *
 **************************************/
	++sql_flag;
	PAR_real_token();
	PAR_match(KW_INTO);
	qli_syntax* node = syntax_node(nod_store, s_sto_count);

	// Pick up relation name for insert

	node->syn_arg[s_sto_relation] = parse_sql_relation();

	// Pick up field list, provided one is present

	PAR_real();

	qli_lls* fields = NULL;
	if (PAR_match(KW_LEFT_PAREN))
	{
		while (true)
		{
			ALLQ_push((blk*) parse_field_name(0), &fields);
			if (PAR_match(KW_RIGHT_PAREN))
				break;
			if (!PAR_match(KW_COMMA))
				ERRQ_syntax(186);	// Msg186 comma or terminating right parenthesis
		}
	}

	// Pick up value list or SELECT statement

	PAR_real();

	bool select_flag;
	if (PAR_match(KW_VALUES))
	{
		select_flag = false;
		if (!PAR_match(KW_LEFT_PAREN))
			ERRQ_syntax(187);	// Msg187 left parenthesis
	}
	else if (PAR_match(KW_SELECT))
		select_flag = true;
	else
		ERRQ_syntax(188);		// Msg188 VALUES list or SELECT clause


	qli_lls* values = NULL;
	qli_lls* distinct = NULL;
	while (true)
	{
		if (distinct || PAR_match(KW_DISTINCT))
		{
			ALLQ_push((blk*) parse_value(0, 0), &distinct);
			ALLQ_push(distinct->lls_object, &values);
			ALLQ_push(0, &distinct);
		}
		else
			ALLQ_push((blk*) parse_value(0, 0), &values);
		if (!PAR_match(KW_COMMA))
			break;
	}

	if (select_flag)
		node->syn_arg[s_sto_rse] = parse_sql_rse();
	else
		parse_matching_paren();

	if (distinct)
		node->syn_arg[s_sto_rse]->syn_arg[s_rse_reduced] = make_list(distinct);

	node->syn_arg[s_sto_fields] = (qli_syntax*) fields;
	node->syn_arg[s_sto_values] = (qli_syntax*) values;

	--sql_flag;

	return node;
}


static nod_t parse_join_type()
{
/**************************************
 *
 *	p a r s e _ j o i n _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Parse a join type.
 *
 **************************************/
	nod_t operatr;

	if (PAR_match(KW_INNER))
		operatr = nod_join_inner;
	else if (PAR_match(KW_LEFT))
		operatr = nod_join_left;
	else if (PAR_match(KW_RIGHT))
		operatr = nod_join_right;
	else if (PAR_match(KW_FULL))
		operatr = nod_join_full;
	else if (PAR_match(KW_JOIN))
		return nod_join_inner;
	else
		return nod_nothing;

	if (operatr != nod_join_inner)
		PAR_match(KW_OUTER);

	if (!PAR_match(KW_JOIN))
		ERRQ_syntax(489);		// Msg489 JOIN

	return operatr;
}


static qli_syntax* parse_list_fields()
{
/**************************************
 *
 *	p a r s e _ l i s t _ f i e l d s
 *
 **************************************
 *
 * Functional description
 *	Parse a LIST statement.  LIST is like PRINT, but does vertical
 *	formatting.
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_list_fields, s_prt_count);
	PAR_token();

	if (!PAR_match(KW_ALL) && PAR_match(KW_DISTINCT))
		node->syn_arg[s_prt_distinct] = INT_CAST TRUE;

	if (node->syn_arg[s_prt_output] = parse_output())
		return node;

	if (test_end())
		return node;

	// If there is a potential record selection expression, there obviously
	// can't be a print list.  Get the rse.  Otherwise, pick up the print
	// list.

	if (potential_rse())
		node->syn_arg[s_prt_rse] = parse_rse();
	else
	{
		if (!test_end())
		{
			qli_lls* stack = NULL;
			for (;;)
			{
				qli_syntax* item = syntax_node(nod_print_item, s_itm_count);
				item->syn_arg[s_itm_value] = parse_value(0, 0);
				item->syn_arg[s_itm_edit_string] = (qli_syntax*) parse_edit_string();
				ALLQ_push((blk*) item, &stack);
				if (!PAR_match(KW_COMMA) && !PAR_match(KW_AND))
					break;
				PAR_real();
				if (PAR_match(KW_AND))
					PAR_real();
			}
			node->syn_arg[s_prt_list] = make_list(stack);
		}
		if (PAR_match(KW_OF))
			node->syn_arg[s_prt_rse] = parse_rse();
	}

	node->syn_arg[s_prt_output] = parse_output();

	return node;
}


static qli_const* parse_literal()
{
/**************************************
 *
 *	p a r s e _ l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Parse a literal expression.
 *
 **************************************/
	qli_const* constant;

	PAR_real();
	const UCHAR* q = (UCHAR*) QLI_token->tok_string;
	USHORT l = QLI_token->tok_length;

	if (QLI_token->tok_type == tok_quoted)
	{
		q++;
		l -= 2;
		constant = (qli_const*) ALLOCDV(type_con, l);
		constant->con_desc.dsc_dtype = dtype_text;
		UCHAR* p = constant->con_desc.dsc_address = constant->con_data;
		QLI_validate_desc(constant->con_desc);
		if (constant->con_desc.dsc_length = l)
			memcpy(p, q, l);
	}
	else if (QLI_token->tok_type == tok_number)
		constant = make_numeric_constant(QLI_token->tok_string, QLI_token->tok_length);
	else
		ERRQ_syntax(190);		// Msg190 value expression

	PAR_token();

	return constant;
}


static qli_syntax* parse_matches()
{
/**************************************
 *
 *	p a r s e _ m a t c h e s
 *
 **************************************
 *
 * Functional description
 *	Parse a matching expressing, including
 *	the preset matching language template.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_sleuth, 3);
	node->syn_arg[1] = parse_value(0, 0);
	if (PAR_match(KW_USING))
		node->syn_arg[2] = parse_value(0, 0);
	else if (QLI_matching_language)
	{
		qli_syntax* language = syntax_node(nod_constant, 1);
		node->syn_arg[2] = language;
		language->syn_arg[0] = (qli_syntax*) QLI_matching_language;
	}
	else
	{
		node->syn_type = nod_matches;
		node->syn_count = 2;
	}

	return node;
}


static void parse_matching_paren()
{
/**************************************
 *
 *	p a r s e _ m a t c h i n g _ p a r e n
 *
 **************************************
 *
 * Functional description
 *	Check for a trailing (right) parenthesis.  Complain if
 *	its not there.
 *
 **************************************/

	PAR_real();

	if (PAR_match(KW_RIGHT_PAREN))
		return;

	ERRQ_syntax(191);			// Msg191 right parenthesis
}


static qli_syntax* parse_modify()
{
/**************************************
 *
 *	p a r s e _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Parse a MODIFY statement.  Modify can be any of the
 *	following:
 *
 *	MODIFY [ALL] [<field> [, <field>]...] [OF <rse> ]
 *	MODIFY [ALL] USING <statement> [OF <rse>]
 *
 **************************************/
	PAR_token();

	// If this is a meta-data change, handle it elsewhere

	if (PAR_match(KW_INDEX))
		return parse_modify_index();

	if (PAR_match(KW_FIELD))
	{
		qli_syntax* anode = syntax_node(nod_mod_field, 1);
		anode->syn_arg[0] = (qli_syntax*) parse_database();
		anode->syn_arg[1] = (qli_syntax*) parse_field(true);
		return anode;
	}

	if (PAR_match(KW_RELATION))
		return parse_modify_relation();

	// Not a meta-data modification, just a simple data modify

	PAR_match(KW_ALL);
	qli_syntax* node = syntax_node(nod_modify, s_mod_count);

	if (PAR_match(KW_USING))
		node->syn_arg[s_mod_statement] = parse_statement();
	else if (!KEYWORD(KW_SEMI))
	{
		qli_lls* stack = NULL;
		while (true)
		{
			ALLQ_push((blk*) parse_field_name(0), &stack);
			if (!PAR_match(KW_COMMA))
				break;
		}
		node->syn_arg[s_mod_list] = make_list(stack);
	}

	if (PAR_match(KW_OF))
		node->syn_arg[s_mod_rse] = parse_rse();

	return node;
}


static qli_syntax* parse_modify_index()
{
/**************************************
 *
 *	p a r s e _ m o d i f y _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Parse a MODIFY INDEX statement.
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_mod_index, s_mfi_count);
	node->syn_arg[s_mfi_database] = (qli_syntax*) parse_database();
	node->syn_arg[s_mfi_name] = (qli_syntax*) parse_name();
	PAR_real();

	while (true)
	{
		if (PAR_match(KW_UNIQUE))
			node->syn_flags |= (s_dfi_flag_selectivity | s_dfi_flag_unique);
		else if (PAR_match(KW_DUPLICATE))
		{
			node->syn_flags |= s_dfi_flag_selectivity;
			node->syn_flags &= ~s_dfi_flag_unique;
		}
		else if (PAR_match(KW_INACTIVE))
			node->syn_flags |= (s_dfi_flag_activity | s_dfi_flag_inactive);
		else if (PAR_match(KW_ACTIVE))
		{
			node->syn_flags |= s_dfi_flag_activity;
			node->syn_flags &= ~s_dfi_flag_inactive;
		}
		else if (PAR_match(KW_DESCENDING))
			node->syn_flags |= (s_dfi_flag_order | s_dfi_flag_descending);
		else if (PAR_match(KW_ASCENDING))
		{
			node->syn_flags |= s_dfi_flag_order;
			node->syn_flags &= ~s_dfi_flag_inactive;
		}
		else if (PAR_match(KW_STATISTICS))
			node->syn_flags |= s_dfi_flag_statistics;
		else
			break;
	}

	if (!node->syn_flags)
		ERRQ_syntax(195);		// Msg195 index state option

	command_end();

	return node;
}


static qli_syntax* parse_modify_relation()
{
/**************************************
 *
 *	p a r s e _ m o d i f y _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a MODIFY RELATION statement.
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_mod_relation, 2);
	qli_rel* relation = parse_qualified_relation();
	node->syn_arg[0] = (qli_syntax*) relation;

	if (!relation)
		ERRQ_syntax(196);		// Msg196 relation name

	for (;;)
	{
		PAR_real();
		qli_fld* field;
		if (PAR_match(KW_ADD))
		{
			PAR_real();
			PAR_match(KW_FIELD);
			field = parse_field(false);
		}
		else if (PAR_match(KW_MODIFY))
		{
			PAR_real();
			PAR_match(KW_FIELD);
			field = parse_field(false);
			field->fld_flags |= FLD_modify;
		}
		else if (PAR_match(KW_DROP))
		{
			PAR_real();
			PAR_match(KW_FIELD);
			field = parse_field(false);
			field->fld_flags |= FLD_drop;
		}
		else
			ERRQ_syntax(197);	// Msg197 ADD, MODIFY, or DROP
		field->fld_next = (qli_fld*) node->syn_arg[1];
		node->syn_arg[1] = (qli_syntax*) field;
		if (KEYWORD(KW_SEMI))
			break;
		if (!PAR_match(KW_COMMA))
			ERRQ_syntax(198);	// Msg198 comma between field definitions
	}

	command_end();

	return node;
}


static qli_syntax* parse_multiply( USHORT * paren_count, bool* bool_flag)
{
/**************************************
 *
 *	p a r s e _ m u l t i p l y
 *
 **************************************
 *
 * Functional description
 *	Parse the operatrs * and /.
 *
 **************************************/
	nod_t operatr;

	qli_syntax* node = parse_from(paren_count, bool_flag);

	while (true)
	{
		if (PAR_match(KW_ASTERISK))
			operatr = nod_multiply;
		else if (PAR_match(KW_SLASH))
			operatr = nod_divide;
		else
			return node;
		qli_syntax* arg = node;
		node = syntax_node(operatr, 2);
		node->syn_arg[0] = arg;
		node->syn_arg[1] = parse_from(paren_count, bool_flag);
	}
}


static qli_name* parse_name()
{
/**************************************
 *
 *	p a r s e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Turn current token into a name and get the next token.
 *
 **************************************/
	PAR_real();

	const bool isQuoted =
		QLI_token->tok_type == tok_quoted && sql_flag && QLI_token->tok_string[0] == '"';
	if (QLI_token->tok_type != tok_ident && !isQuoted)
		ERRQ_syntax(199);		// Msg199 identifier

	SSHORT l = QLI_token->tok_length - 2 * int(isQuoted);
	qli_name* name = (qli_name*) ALLOCDV(type_nam, l);
	name->nam_length = l;
	name->nam_symbol = QLI_token->tok_symbol;
	const TEXT* q = QLI_token->tok_string + int(isQuoted);
	TEXT* p = name->nam_string;

	if (isQuoted)
		memcpy(p, q, l);
	else if (l)
		do {
			const TEXT c = *q++;
			*p++ = UPPER(c);

		} while (--l);

	PAR_token();

	return name;
}


static qli_syntax* parse_not( USHORT * paren_count)
{
/**************************************
 *
 *	p a r s e _ n o t
 *
 **************************************
 *
 * Functional description
 *	Parse a prefix NOT expression.
 *
 **************************************/

	PAR_real();

	if (!PAR_match(KW_NOT))
		return parse_relational(paren_count);

	return negate(parse_not(paren_count));
}


static int parse_ordinal()
{
/**************************************
 *
 *	p a r s e _ o r d i n a l
 *
 **************************************
 *
 * Functional description
 *	Pick up a simple number as a number.  This is
 *	used for SKIP [n], SPACE [n], COL n, and SQL
 *	positions.
 *
 **************************************/
	PAR_real();

	if (QLI_token->tok_type != tok_number)
		ERRQ_syntax(200);		// Msg200 positive number

	const int n = atoi(QLI_token->tok_string);
	if (n < 0)
		ERRQ_syntax(200);		// Msg200 positive number
	PAR_token();

	return n;
}


static qli_syntax* parse_output()
{
/**************************************
 *
 *	p a r s e _ o u t p u t
 *
 **************************************
 *
 * Functional description
 *	Parse an output clause the the absence thereof.
 *
 **************************************/
	USHORT flag;

	if (PAR_match(KW_ON))
		flag = FALSE;
	else if (PAR_match(KW_TO))
		flag = TRUE;
	else
		return NULL;

	qli_syntax* node = syntax_node(nod_output, s_out_count);
	node->syn_arg[s_out_file] = parse_value(0, 0);
	node->syn_arg[s_out_pipe] = INT_CAST flag;

	return node;
}


static qli_syntax* parse_primitive_value( USHORT* paren_count, bool* bool_flag)
{
/**************************************
 *
 *	p a r s e _ p r i m i t i v e _ v a l u e
 *
 **************************************
 *
 * Functional description
 *	Pick up a value expression.  This may be either a field reference
 *	or a constant value.
 *
 **************************************/
	USHORT local_count;

	if (!paren_count)
	{
		local_count = 0;
		paren_count = &local_count;
	}

	PAR_real();

	qli_syntax* node;

	switch (next_keyword())
	{
	case KW_LEFT_PAREN:
		PAR_token();
		(*paren_count)++;
		if (bool_flag && *bool_flag)
			node = parse_boolean(paren_count);
		else
			node = parse_value(paren_count, bool_flag);

		// if (*paren_count && KEYWORD (KW_RIGHT_PAREN))
		{
			parse_matching_paren();
			(*paren_count)--;
		}
		break;

	case KW_PLUS:
		PAR_token();
		return parse_primitive_value(paren_count, 0);

	case KW_MINUS:
		{
			PAR_token();
			qli_syntax* sub = parse_primitive_value(paren_count, 0);
			if (sub->syn_type == nod_constant)
			{
				qli_const* constant = (qli_const*) sub->syn_arg[0];
				UCHAR* p = constant->con_desc.dsc_address;
				switch (constant->con_desc.dsc_dtype)
				{
				case dtype_long:
					*(SLONG *) p = -*(SLONG *) p;
					return sub;
				case dtype_text:
					*p = '-';
					return sub;
				}
			}
			node = syntax_node(nod_negate, 1);
			node->syn_arg[0] = sub;
			break;
		}

	case KW_ASTERISK:
		node = parse_prompt();
		break;

	case KW_EDIT:
		PAR_token();
		node = syntax_node(nod_edit_blob, 1);
		if (!KEYWORD(KW_SEMI))
			node->syn_arg[0] = parse_value(0, 0);
		break;

	case KW_FORMAT:
		PAR_token();
		node = syntax_node(nod_format, s_fmt_count);
		node->syn_arg[s_fmt_value] = parse_value(0, 0);
		node->syn_arg[s_fmt_edit] = (qli_syntax*) parse_edit_string();
		break;

	case KW_NULL:
		PAR_token();
		node = syntax_node(nod_null, 0);
		break;

	case KW_USER_NAME:
		PAR_token();
		node = syntax_node(nod_user_name, 0);
		break;

	case KW_COUNT:
	case KW_MAX:
	case KW_MIN:
	case KW_AVERAGE:
	case KW_TOTAL:
		node = parse_statistical();
		break;

	case KW_RUNNING:
		if (function_count > 0)
			IBERROR(487);		// Msg487 Invalid argument for UDF
		PAR_real_token();
		node = syntax_node(nod_running_total, s_stt_count);
		if (PAR_match(KW_COUNT))
			node->syn_type = nod_running_count;
		else
		{
			PAR_match(KW_TOTAL);
			node->syn_arg[s_stt_value] = parse_value(0, 0);
		}
		break;

	case KW_SELECT:
		node = parse_sql_subquery();
		break;

	default:
		{
			const qli_symbol* symbol = QLI_token->tok_symbol;
			if (symbol && symbol->sym_type == SYM_function)
			{
				node = parse_function();
				break;
			}
			if (QLI_token->tok_type == tok_ident ||
				QLI_token->tok_type == tok_quoted && sql_flag && QLI_token->tok_string[0] == '"')
			{
				node = parse_field_name(0);
				break;
			}
			node = syntax_node(nod_constant, 1);
			node->syn_arg[0] = (qli_syntax*) parse_literal();
		}
	}

	return node;
}


static qli_syntax* parse_print_list()
{
/**************************************
 *
 *	p a r s e _ p r i n t _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Pick up a print item.  The syntax of a print item is:
 *
 *	<value> [ '[ <query_header> '] ] [USING <edit_string> ]
 *
 **************************************/
	qli_syntax* node;
	qli_lls* stack = NULL;

	while (true)
	{
	    nod_t op;
		if (PAR_match(KW_SKIP))
			op = nod_skip;
		else if (PAR_match(KW_SPACE))
			op = nod_space;
		else if (PAR_match(KW_TAB))
			op = nod_tab;
		else if (PAR_match(KW_COLUMN))
			op = nod_column;
		else if (PAR_match(KW_NEW_PAGE))
			op = nod_new_page;
		else if (PAR_match(KW_REPORT_HEADER))
			op = nod_report_header;
		else if (PAR_match(KW_COLUMN_HEADER))
			op = nod_column_header;
		else
		{
			op = nod_print_item;
			node = syntax_node(nod_print_item, s_itm_count);
			node->syn_arg[s_itm_value] = parse_value(0, 0);
			if (PAR_match(KW_LEFT_PAREN))
			{
				if (PAR_match(KW_MINUS))
					node->syn_arg[s_itm_header] = (qli_syntax*) "-";
				else
					node->syn_arg[s_itm_header] = (qli_syntax*) parse_header();
				parse_matching_paren();
			}
			node->syn_arg[s_itm_edit_string] = (qli_syntax*) parse_edit_string();
		}
		if (op != nod_print_item)
		{
			node = syntax_node(op, 1);
			node->syn_count = 0;
			node->syn_arg[0] = INT_CAST 1;
			if (op == nod_column || QLI_token->tok_type == tok_number)
				node->syn_arg[0] = INT_CAST parse_ordinal();
			if ((op == nod_skip) && ((IPTR) node->syn_arg[0] < 1))
				ERRQ_syntax(478);	// Msg478 number > 0
		}
		ALLQ_push((blk*) node, &stack);
		if (!PAR_match(KW_COMMA) && !PAR_match(KW_AND))
			break;
		PAR_real();
		if (PAR_match(KW_AND))
			PAR_real();
	}

	node = make_list(stack);

	return node;
}


static qli_syntax* parse_print()
{
/**************************************
 *
 *	p a r s e _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Parse the PRINT statement.  This is the richest and most general
 *	Datatrieve statement.  Hence this may get a bit long.  The syntax is:
 *
 *		[<item> [, <item>]] OF <rse>
 *	PRINT [ 			     ] [ON <file>]
 *		<rse>
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_print, s_prt_count);
	PAR_token();

	if (!PAR_match(KW_ALL) && PAR_match(KW_DISTINCT))
		node->syn_arg[s_prt_distinct] = INT_CAST TRUE;

	if (node->syn_arg[s_prt_output] = parse_output())
		return node;

	if (test_end())
		return node;

	// If there is a potential record selection expression, there obviously
	// can't be a print list.  Get the rse.  Otherwise, pick up the print
	// list.

	if (potential_rse())
		node->syn_arg[s_prt_rse] = parse_rse();
	else if (!KEYWORD(KW_USING))
	{
		if (!KEYWORD(KW_THEN) && !KEYWORD(KW_OF) && !KEYWORD(KW_ON))
			node->syn_arg[s_prt_list] = parse_print_list();
		if (PAR_match(KW_OF))
			node->syn_arg[s_prt_rse] = parse_rse();
	}

	if (!node->syn_arg[s_prt_list] && PAR_match(KW_USING))
	{
		IBERROR(484);			// FORMs not supported
	}
	else
		node->syn_arg[s_prt_output] = parse_output();

	return node;
}


static qli_syntax* parse_prompt()
{
/**************************************
 *
 *	p a r s e _ p r o m p t
 *
 **************************************
 *
 * Functional description
 *	Parse a prompt expression.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_prompt, 1);

	// If there is a period, get the prompt string and make a string node

	if (PAR_match(KW_DOT))
	{
		PAR_real();
		USHORT l = QLI_token->tok_length;
		const TEXT* q = QLI_token->tok_string;
		if (QLI_token->tok_type == tok_quoted)
		{
			q++;
			l -= 2;
		}
		node->syn_arg[0] = (qli_syntax*) make_string(q, l);
		PAR_token();
	}

	return node;
}


static qli_filter* parse_qualified_filter()
{
/**************************************
 *
 *	p a r s e _ q u a l i f i e d _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *	This token ought to be a filter, possibly qualified.
 *	Return a qualified filter block, containing the
 *	filter name in a qli_name block and the database in a
 *	qli_dbb block if a database was specified.  Somebody
 *	else will decide what to do if the database was not
 *	specified.
 *
 **************************************/
	qli_filter* filter = (qli_filter*) ALLOCD(type_qfl);
	filter->qfl_database = parse_database();
	filter->qfl_name = parse_name();
	return filter;
}



static qli_func* parse_qualified_function()
{
/**************************************
 *
 *	p a r s e _ q u a l i f i e d _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	This token ought to be a function, possibly qualified.
 *	Return a qualified function block, containing the
 *	function name in a qli_name block and the database in a
 *	qli_dbb block if a database was specified.  Somebody
 *	else will decide what to do if the database was not
 *	specified.
 *
 **************************************/
	qli_func* func = (qli_func*) ALLOCD(type_qfn);
	func->qfn_database = parse_database();
	func->qfn_name = parse_name();
	return func;
}


static qli_proc* parse_qualified_procedure()
{
/**************************************
 *
 *	p a r s e _ q u a l i f i e d _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	This token ought to be a procedure, possibly qualified.
 *	Return a qualified procedure block, containing the
 *	procedure name in a qli_name block and the database in a
 *	qli_dbb block if a database was specified.  Somebody
 *	else will decide what to do if the database was not
 *	specified.
 *
 **************************************/
	qli_proc* proc = (qli_proc*) ALLOCD(type_qpr);
	proc->qpr_database = parse_database();
	proc->qpr_name = parse_name();
	return proc;
}


static qli_rel* parse_qualified_relation()
{
/**************************************
 *
 *	p a r s e _ q u a l i f i e d _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Check for a relation name, possible qualified.  If there
 *	is a relation name, parse it and return the relation.  If
 *	not, return NULL.  Produce a syntax error only if there is
 *	a partially qualified name.
 *
 **************************************/
	PAR_real();

	// If the next token is a database symbol, take it as a qualifier

	qli_symbol* db_symbol = QLI_token->tok_symbol;
	if (db_symbol && db_symbol->sym_type == SYM_database)
	{
		PAR_real_token();
		if (!PAR_match(KW_DOT))
			ERRQ_syntax(202);	// Msg202 period in qualified relation name
		PAR_real();
		qli_rel* relation = resolve_relation(db_symbol, QLI_token->tok_symbol);
		if (relation)
		{
			PAR_token();
			return relation;
		}
		ERRQ_print_error(203, SafeArg() << QLI_token->tok_string << db_symbol->sym_string);
		// Msg203 %s is not a relation in database %s
	}

	qli_rel* relation = resolve_relation(0, QLI_token->tok_symbol);
	if (relation)
		PAR_token();

	return relation;
}


static qli_syntax* parse_ready( nod_t node_type)
{
/**************************************
 *
 *	p a r s e _ r e a d y
 *
 **************************************
 *
 * Functional description
 *	Parse a READY statement:
 *
 *	READY <filename> [AS <symbol>] [,...];
 *
 **************************************/
	qli_lls* stack = NULL;

	while (true)
	{
		LEX_filename();
		SSHORT l = QLI_token->tok_length;
		if (!l)
			ERRQ_error(204);
			// Msg204 database file name required on READY
		const TEXT* q = QLI_token->tok_string;
		if (QLI_token->tok_type == tok_quoted)
		{
			l -= 2;
			q++;
		}
		qli_dbb* database = (qli_dbb*) ALLOCDV(type_dbb, l);
		database->dbb_filename_length = l;
		memcpy(database->dbb_filename, q, l);
		PAR_token();

		switch (node_type)
		{
		case nod_def_database:
		case nod_ready:
			if (PAR_match(KW_AS))
			{
				qli_name* name = parse_name();
				database->dbb_symbol = (qli_symbol*) name;
				if (HSH_lookup(name->nam_string, name->nam_length))
					ERRQ_error(408, name->nam_string);
					// Database handle is not unique
			}
			else
				database->dbb_symbol = (qli_symbol*) make_name();
			break;
		case nod_sql_database:
			if (PAR_match(KW_PAGESIZE))
			{
				if (database->dbb_pagesize)
					ERRQ_syntax(390);	// Msg390 Multiple page size specifications
				if (!PAR_match(KW_EQUALS))
					ERRQ_syntax(396);	// Msg396 = (equals)
				database->dbb_pagesize = parse_ordinal();
			}
			database->dbb_symbol = (qli_symbol*) make_name();
			break;
		}

		for (;;)
		{
			if (PAR_match(KW_USER))
				database->dbb_user = parse_literal();
			else if (PAR_match(KW_PASSWORD))
				database->dbb_password = parse_literal();
			else
				break;
		}

		ALLQ_push((blk*) database, &stack);
		if (!KEYWORD(KW_COMMA) || (node_type == nod_sql_database))
			break;
	}

	command_end();
	qli_syntax* node = make_list(stack);
	node->syn_type = node_type;

	return node;
}


static qli_syntax* parse_relational( USHORT * paren_count)
{
/**************************************
 *
 *	p a r s e _ r e l a t i o n a l
 *
 **************************************
 *
 * Functional description
 *	Parse a relational expression.
 *
 **************************************/
	if (PAR_match(KW_ANY))
	{
		qli_syntax* anode = syntax_node(nod_any, 1);
		anode->syn_arg[0] = parse_rse();
		return anode;
	}

	nod_t operatr = nod_nothing;
	if (PAR_match(KW_EXISTS))
		operatr = nod_any;
	else if (PAR_match(KW_SINGULAR))
		operatr = nod_unique;

	if (operatr != nod_nothing)
	{
		PAR_real();
		if (PAR_match(KW_LEFT_PAREN))
		{
			PAR_real();
			if (PAR_match(KW_SELECT))
			{
				PAR_real();
				qli_syntax* node = syntax_node(operatr, 2);
				if (!PAR_match(KW_ASTERISK))
					node->syn_arg[1] = parse_value(0, 0);
				node->syn_arg[0] = parse_sql_rse();
				parse_matching_paren();
				return node;
			}
		}
		if (operatr == nod_any)
			ERRQ_syntax(205);	// Msg205 EXISTS (SELECT * <sql rse>)
		else
			ERRQ_syntax(488);	// Msg488 SINGULAR (SELECT * <sql rse>)
	}

	if (PAR_match(KW_UNIQUE))
	{
		qli_syntax* node = syntax_node(nod_unique, 1);
		node->syn_arg[0] = parse_rse();
		return node;
	}

    bool local_flag = true;
	qli_syntax* expr1 = parse_value(paren_count, &local_flag);
	if (KEYWORD(KW_RIGHT_PAREN))
		return expr1;

	const nod_t* rel_ops;

	if (KEYWORD(KW_SEMI))
	{
		for (rel_ops = relationals; *rel_ops != nod_nothing; rel_ops++)
		{
			if (expr1->syn_type == *rel_ops)
				return expr1;
		}
	}

	bool negation = false;
	qli_syntax* node = NULL;
	PAR_match(KW_IS);
	PAR_real();

	if (PAR_match(KW_NOT))
	{
		negation = true;
		PAR_real();
	}

	switch (next_keyword())
	{
	case KW_IN:
		PAR_token();
		node = parse_in(expr1, nod_eql, false);
		break;

	case KW_EQUALS:
	case KW_EQ:
		operatr = negation ? nod_neq : nod_eql;
		negation = false;
		break;

	case KW_NE:
		operatr = negation ? nod_eql : nod_neq;
		negation = false;
		break;

	case KW_GT:
		operatr = negation ? nod_leq : nod_gtr;
		negation = false;
		break;

	case KW_GE:
		operatr = negation ? nod_lss : nod_geq;
		negation = false;
		break;

	case KW_LE:
		operatr = negation ? nod_gtr : nod_leq;
		negation = false;
		break;

	case KW_LT:
		operatr = negation ? nod_geq : nod_lss;
		negation = false;
		break;

	case KW_CONTAINING:
		operatr = nod_containing;
		break;

	case KW_MATCHES:
		node = parse_matches();
		node->syn_arg[0] = expr1;
		operatr = node->syn_type;
		break;

	case KW_LIKE:
		PAR_token();
		node = syntax_node(nod_like, 3);
		node->syn_arg[0] = expr1;
		node->syn_arg[1] = parse_value(0, 0);
		if (PAR_match(KW_ESCAPE))
			node->syn_arg[2] = parse_value(0, 0);
		else
			node->syn_count = 2;
		break;

	case KW_STARTS:
		PAR_token();
		PAR_match(KW_WITH);
		node = syntax_node(nod_starts, 2);
		node->syn_arg[0] = expr1;
		node->syn_arg[1] = parse_value(0, 0);
		break;

	case KW_NULL:
	case KW_MISSING:
		PAR_token();
		node = syntax_node(nod_missing, 1);
		node->syn_arg[0] = expr1;
		break;

	case KW_BETWEEN:
		PAR_token();
		node = syntax_node(nod_between, 3);
		node->syn_arg[0] = expr1;
		node->syn_arg[1] = parse_value(0, 0);
		PAR_match(KW_AND);
		node->syn_arg[2] = parse_value(0, 0);
		break;

	default:
		for (rel_ops = relationals; *rel_ops != nod_nothing; rel_ops++)
		{
			if (expr1->syn_type == *rel_ops)
				return expr1;
		}
		ERRQ_syntax(206);		// Msg206 relational operatr
	}

	// If we haven't already built a node, it must be an ordinary binary operatr.
	// Build it.

	if (!node)
	{
		PAR_token();
		if (PAR_match(KW_ANY))
			return parse_in(expr1, operatr, false);
		if (PAR_match(KW_ALL))
			return parse_in(expr1, operatr, true);
		node = syntax_node(operatr, 2);
		node->syn_arg[0] = expr1;
		node->syn_arg[1] = parse_value(paren_count, &local_flag);
	}

	// If a negation remains to be handled, zap the node under a NOT.

	if (negation)
		node = negate(node);

	// If the node isn't an equality, we've done.  Since equalities can be
	// structured as implicit ORs, build them here.

	if (operatr != nod_eql)
		return node;

	// We have an equality operation, which can take a number of values.  Generate
	// implicit ORs

	while (PAR_match(KW_COMMA))
	{
		PAR_real();
		PAR_match(KW_OR);
		qli_syntax* or_node = syntax_node(nod_or, 2);
		or_node->syn_arg[0] = node;
		or_node->syn_arg[1] = node = syntax_node(nod_eql, 2);
		node->syn_arg[0] = expr1;
		node->syn_arg[1] = parse_value(paren_count, &local_flag);
		node = or_node;
	}

	return node;
}


static qli_syntax* parse_relation()
{
/**************************************
 *
 *	p a r s e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a relation expression.  Syntax is:
 *
 *	[ <context_variable> IN ] <relation>
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_relation, s_rel_count);

	// Token wasn't a relation name, maybe it's a context variable.

	if (!(node->syn_arg[s_rel_relation] = (qli_syntax*) parse_qualified_relation()))
	{
		qli_symbol* context = parse_symbol();
		node->syn_arg[s_rel_context] = (qli_syntax*) context;
		if (sql_flag || !PAR_match(KW_IN))
		{
			if (!QLI_databases)
				IBERROR(207);	// Msg207 a database has not been readied
			ERRQ_print_error(208, context->sym_string);
			// Msg208 expected \"relation_name\", encountered \"%s\"
		}
		if (!(node->syn_arg[s_rel_relation] = (qli_syntax*) parse_qualified_relation()))
		{
			ERRQ_syntax(209);	// Msg209 relation name
		}
	}

	return node;
}


static qli_syntax* parse_rename()
{
/**************************************
 *
 *	p a r s e _ r e n a m e
 *
 **************************************
 *
 * Functional description
 *	Parse a PROCEDURE rename statement.
 *
 **************************************/
	PAR_real_token();

	if (!PAR_match(KW_PROCEDURE))
		ERRQ_syntax(210);		// Msg210 PROCEDURE

	qli_syntax* node = syntax_node(nod_rename_proc, 2);
	node->syn_arg[0] = (qli_syntax*) parse_qualified_procedure();
	PAR_match(KW_TO);
	node->syn_arg[1] = (qli_syntax*) parse_qualified_procedure();

	return node;
}


static qli_syntax* parse_repeat()
{
/**************************************
 *
 *	p a r s e _ r e p e a t
 *
 **************************************
 *
 * Functional description
 *	Parse REPEAT statement.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_repeat, s_rpt_count);
	node->syn_arg[s_rpt_value] = parse_value(0, 0);
	node->syn_arg[s_rpt_statement] = parse_statement();

	return node;
}


static qli_syntax* parse_report()
{
/**************************************
 *
 *	p a r s e _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Parse a report specification.
 *
 **************************************/
	++sw_report;
	PAR_token();
	qli_rpt* report = (qli_rpt*) ALLOCD(type_rpt);
	qli_syntax* node = syntax_node(nod_report, s_prt_count);
	node->syn_arg[s_prt_list] = (qli_syntax*) report;

	// Pick up record select expression

	qli_syntax* rse = node->syn_arg[s_prt_rse] = parse_rse();
	node->syn_arg[s_prt_output] = parse_output();

	// Pick up report clauses

	bool top;

	for (;;)
	{
		PAR_real();
		if (PAR_match(KW_END_REPORT))
			break;

		switch (next_keyword())
		{
		case KW_PRINT:
			PAR_token();
			report->rpt_detail_line = (qli_nod*) parse_print_list();
			break;

		case KW_AT:
			PAR_token();
			if (PAR_match(KW_TOP))
				top = true;
			else if (PAR_match(KW_BOTTOM))
				top = false;
			else
				ERRQ_syntax(382);	// Msg382 TOP or BOTTOM
			PAR_match(KW_OF);
			if (PAR_match(KW_REPORT))
			{
				qli_brk* control = (qli_brk*) ALLOCD(type_brk);
				qli_brk** ptr = top ? &report->rpt_top_rpt : &report->rpt_bottom_rpt;
				control->brk_next = *ptr;
				*ptr = control;
				PAR_match(KW_PRINT);
				control->brk_line = parse_print_list();
			}
			else if (PAR_match(KW_PAGE))
			{
				qli_brk* control = (qli_brk*) ALLOCD(type_brk);
				qli_brk** ptr = top ? &report->rpt_top_page : &report->rpt_bottom_page;
				control->brk_next = *ptr;
				*ptr = control;
				PAR_match(KW_PRINT);
				control->brk_line = parse_print_list();
			}
			else
			{
				qli_brk** ptr = top ? &report->rpt_top_breaks : &report->rpt_bottom_breaks;
				if (!*ptr)
				{
					// control breaks should only be on sorted fields, set up list
					// of control breaks based on sorted fields and then add action (print)
					// items to that list.
					qli_syntax* flds = rse->syn_arg[s_rse_sort];
					if (!flds)
						ERRQ_syntax(383);	// Msg383 sort field
					qli_brk* tmpptr = *ptr;
					for (USHORT i = 0; i < flds->syn_count; i += 2)
					{
						qli_brk* control = (qli_brk*) ALLOCD(type_brk);
						control->brk_field = flds->syn_arg[i];
						control->brk_line = NULL;
						control->brk_statisticals = NULL;
						control->brk_next = tmpptr;
						tmpptr = control;
					}
					if (!top)
					{
						// reverse the 'at bottom' control break list as the
						// lower control breaks should be performed prior to the higher ones.
						qli_brk* control = 0;
						for (qli_brk* tmpptr1 = tmpptr->brk_next; tmpptr;)
						{
							tmpptr->brk_next = control;
							control = tmpptr;
							if (tmpptr = tmpptr1)
								tmpptr1 = tmpptr->brk_next;
						}
						tmpptr = control;
					}
					*ptr = tmpptr;
				}
				qli_syntax* qli_fld = parse_field_name(0);
				qli_brk* control;
				for (control = *ptr; control; control = control->brk_next)
				{
					qli_syntax* rse_fld = (qli_syntax*) control->brk_field;
					if (rse_fld->syn_type != qli_fld->syn_type)
						continue;
					// if number of field qualifiers on sort field and control field
					// are not equal test match of rightmost set
					const USHORT syn_count = MIN(rse_fld->syn_count, qli_fld->syn_count);
					USHORT srt_syn = 0, ctl_syn = 0;
					if (syn_count != rse_fld->syn_count)
						srt_syn = rse_fld->syn_count - syn_count;
					if (syn_count != qli_fld->syn_count)
						ctl_syn = qli_fld->syn_count - syn_count;
					USHORT i;
					for (i = 0; i < syn_count; i++)
					{
						const qli_name* name1 = (qli_name*) rse_fld->syn_arg[i + srt_syn];
						const qli_name* name2 = (qli_name*) qli_fld->syn_arg[i + ctl_syn];
						if (strcmp(name1->nam_string, name2->nam_string))
							break;
					}
					if (i == qli_fld->syn_count)
						break;
				}
				if (!control)
					ERRQ_syntax(383);	// Msg383 sort field
				PAR_match(KW_PRINT);
				control->brk_field = qli_fld;
				control->brk_line = parse_print_list();
			}
			break;

		case KW_SET:
			PAR_token();
			if (PAR_match(KW_COLUMNS))
			{
				PAR_match(KW_EQUALS);
				report->rpt_columns = parse_ordinal();
			}
			else if (PAR_match(KW_LINES))
			{
				PAR_match(KW_EQUALS);
				report->rpt_lines = parse_ordinal();
			}
			else if (PAR_match(KW_REPORT_NAME))
			{
				PAR_match(KW_EQUALS);
				report->rpt_name = parse_header();
			}
			else
				ERRQ_syntax(212);	// Msg212 report writer SET option
			break;

		default:
			ERRQ_syntax(213);	// Msg213 report item
		}
		PAR_match(KW_SEMI);
	}

	if (!node->syn_arg[s_prt_output])
		node->syn_arg[s_prt_output] = parse_output();

	check_end();
	--sw_report;

	return node;
}


static qli_syntax* parse_rse()
{
/**************************************
 *
 *	p a r s e _ r s e
 *
 **************************************
 *
 * Functional description
 *	Parse a record selection expression.
 *
 **************************************/
	PAR_real();

	if (PAR_match(KW_ALL))
		PAR_real();

	qli_syntax* first = NULL;
	if (PAR_match(KW_FIRST))
		first = parse_value(0, 0);

	USHORT count = 0;
	qli_lls* stack = NULL;
	while (true)
	{
		count++;
		ALLQ_push((blk*) parse_relation(), &stack);
		qli_syntax* over = NULL;
		if (PAR_match(KW_OVER))
		{
			qli_lls* field_stack = NULL;
			while (true)
			{
				ALLQ_push((blk*) parse_field_name(0), &field_stack);
				if (!PAR_match(KW_COMMA))
					break;
			}
			over = make_list(field_stack);
		}
		ALLQ_push((blk*) over, &stack);
		if (!PAR_match(KW_CROSS))
			break;
	}

	qli_syntax* node = syntax_node(nod_rse, (int) s_rse_count + 2 * count);
	node->syn_count = count;
	node->syn_arg[s_rse_first] = first;
	qli_syntax** ptr = &node->syn_arg[(int) s_rse_count + 2 * count];

	while (stack)
		*--ptr = (qli_syntax*) ALLQ_pop(&stack);

	// Pick up various other clauses

	USHORT sw_with = 0;
	while (true)
	{
		if (PAR_match(KW_WITH))
		{
			if (!sw_with)
			{
				sw_with++;
				node->syn_arg[s_rse_boolean] = parse_boolean(0);
			}
			else
				IBERROR(384);	// Msg384 Too many WITHs
		}
		else if (PAR_match(KW_SORTED))
		{
			PAR_real();
			PAR_match(KW_BY);
			node->syn_arg[s_rse_sort] = parse_sort();
		}
		else if (PAR_match(KW_REDUCED))
		{
			PAR_real();
			PAR_match(KW_TO);
			node->syn_arg[s_rse_reduced] = parse_sort();
		}
		else if (PAR_match(KW_GROUP))
		{
			PAR_real();
			PAR_match(KW_BY);
			stack = NULL;
			while (true)
			{
				ALLQ_push((blk*) parse_udf_or_field(), &stack);
				if (!PAR_match(KW_COMMA))
					break;
			}
			node->syn_arg[s_rse_group_by] = make_list(stack);
		}
		else
			break;
	}

	return node;
}


static qli_syntax* parse_select()
{
/**************************************
 *
 *	p a r s e _ s e l e c t
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL select statement.
 *
 **************************************/
	++sql_flag;
	PAR_token();
	qli_syntax* node = syntax_node(nod_print, s_prt_count);

	if (!PAR_match(KW_ALL) && PAR_match(KW_DISTINCT))
		node->syn_arg[s_prt_distinct] = INT_CAST TRUE;

	// Get list of items

	if (!PAR_match(KW_ASTERISK))
	{
		qli_lls* stack = NULL;
		while (true)
		{
			qli_syntax* item = syntax_node(nod_print_item, s_itm_count);
			item->syn_arg[s_itm_value] = parse_value(0, 0);
			ALLQ_push((blk*) item, &stack);
			if (!PAR_match(KW_COMMA))
				break;
		}
		node->syn_arg[s_prt_list] = make_list(stack);
	}

    qli_syntax* rse = parse_sql_rse();
	node->syn_arg[s_prt_rse] = rse;
	rse->syn_arg[s_rse_list] = node->syn_arg[s_prt_list];

	if (PAR_match(KW_ORDER))
	{
		PAR_real();
		PAR_match(KW_BY);
		node->syn_arg[s_prt_order] = parse_sort();
	}

	--sql_flag;

	return node;
}


static qli_syntax* parse_set()
{
/**************************************
 *
 *	p a r s e _ s e t
 *
 **************************************
 *
 * Functional description
 *	Parse a SET statement.
 *
 **************************************/
	PAR_token();
	qli_lls* stack = NULL;
	USHORT count = 0;

	while (true)
	{
		PAR_real();
		U_IPTR value = TRUE;
		if (PAR_match(KW_NO))
		{
			value = FALSE;
			PAR_real();
		}
		enum set_t sw;

		switch (QLI_token->tok_keyword)
		{
		case KW_BLR:
			sw = set_blr;
			PAR_token();
			break;

		case KW_STATISTICS:
			sw = set_statistics;
			PAR_token();
			break;

		case KW_COLUMNS:
			sw = set_columns;
			PAR_token();
			PAR_match(KW_EQUALS);
			value = parse_ordinal();
			break;

		case KW_LINES:
			sw = set_lines;
			PAR_token();
			PAR_match(KW_EQUALS);
			value = parse_ordinal();
			break;

		case KW_SEMICOLON:
			sw = set_semi;
			PAR_token();
			break;

		case KW_ECHO:
			sw = set_echo;
			PAR_token();
			break;

		case KW_MATCHING_LANGUAGE:
			sw = set_matching_language;
			PAR_token();
			PAR_match(KW_EQUALS);
			if (value)
				value = (U_IPTR) parse_literal();
			break;

		case KW_PASSWORD:
			sw = set_password;
			PAR_token();
			value = (U_IPTR) parse_literal();
			break;

		case KW_PROMPT:
			sw = set_prompt;
			PAR_token();
			value = (U_IPTR) parse_literal();
			break;

		case KW_CONT_PROMPT:
			sw = set_continuation;
			PAR_token();
			value = (U_IPTR) parse_literal();
			break;

		case KW_USER:
			sw = set_user;
			PAR_token();
			value = (U_IPTR) parse_literal();
			break;

		case KW_COUNT:
			sw = set_count;
			PAR_token();
			break;

		case KW_CHAR:
			sw = set_charset;
			PAR_token();
			PAR_match(KW_SET);
			if (value)
			{
				// allow for NO
				PAR_match(KW_EQUALS);
				value = (U_IPTR) parse_name();
			}
			break;

		case KW_NAMES:
			sw = set_charset;
			PAR_token();
			if (value) {
				value = (U_IPTR) parse_name();
			}
			break;

#ifdef DEV_BUILD
		case KW_EXPLAIN:
			sw = set_explain;
			PAR_token();
			break;

		case KW_HEXOUT:
			sw = set_hex_output;
			PAR_token();
			break;
#endif

		default:
			ERRQ_syntax(214);	// Msg214 set option
		}
		ALLQ_push((blk*) sw, &stack);
		ALLQ_push((blk*) value, &stack);
		count++;
		if (!PAR_match(KW_COMMA))
			break;
	}

	command_end();
	qli_syntax* node = make_list(stack);
	node->syn_count = count;
	node->syn_type = nod_set;

	return node;
}


static qli_syntax* parse_shell()
{
/**************************************
 *
 *	p a r s e _ s h e l l
 *
 **************************************
 *
 * Functional description
 *	Parse SHELL command.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_shell, 1);

	if (!KEYWORD(KW_SEMI))
		node->syn_arg[0] = (qli_syntax*) parse_literal();

	command_end();

	return node;
}


static qli_syntax* parse_show()
{
/**************************************
 *
 *	p a r s e _ s h o w
 *
 **************************************
 *
 * Functional description
 *	Parse a SHOW statement.  The first
 *	group are individual things (one
 *	named relation, field, form, ...)
 *
 *	the second group are sets of things
 *	and can be qualified with a FOR
 *	[DATABASE] <handle>
 *
 **************************************/
	PAR_token();
	qli_lls* stack = NULL;
	USHORT count = 0;

	while (true)
	{
		PAR_real();
		BLK value = NULL;
		qli_symbol* symbol = QLI_token->tok_symbol;
		enum show_t sw;

		if (PAR_match(KW_ALL))
			sw = show_all;
		else if (PAR_match(KW_MATCHING_LANGUAGE))
			sw = show_matching_language;
		else if (PAR_match(KW_VERSION))
			sw = show_version;
		else if (PAR_match(KW_RELATION))
		{
			if (!(value = (BLK) parse_qualified_relation()))
				ERRQ_syntax(216);	// Msg216 relation name
			else
				sw = show_relation;
		}
		else if (PAR_match(KW_FILTER))
		{
			sw = show_filter;
			value = (BLK) parse_qualified_filter();
		}
		else if (PAR_match(KW_FUNCTION))
		{
			sw = show_function;
			value = (BLK) parse_qualified_function();
		}
		else if ((PAR_match(KW_DATABASES)) || (PAR_match(KW_READY)))
			sw = show_databases;
		else if (PAR_match(KW_DATABASE))
		{
			sw = show_database;
			if (value = (BLK) get_dbb(QLI_token->tok_symbol))
				PAR_token();
		}
		else if (PAR_match(KW_FIELD))
		{
			sw = show_field;
			value = (BLK) parse_field_name(0);
		}
		else if (PAR_match(KW_PROCEDURE))
		{
			sw = show_procedure;
			value = (BLK) parse_qualified_procedure();
		}
		else if (PAR_match(KW_VARIABLE))
		{
			sw = show_variable;
			value = (BLK) parse_name();
		}
		else if (PAR_match(KW_VARIABLES))
			sw = show_variables;
		else if (PAR_match(KW_FIELDS))
		{
			if (PAR_match(KW_FOR))
			{
				if (PAR_match(KW_DATABASE))
				{
					if (value = (BLK) get_dbb(QLI_token->tok_symbol))
						PAR_token();
					else
						ERRQ_syntax(221);	// Msg221 database name
					sw = show_db_fields;
				}
				else
				{
					PAR_match(KW_RELATION);
					if (!(value = (BLK) parse_qualified_relation()))
						ERRQ_syntax(218);	// Msg218 relation name
					else
						sw = show_relation;
				}
			}
			else
				sw = show_all;
		}
		else if (PAR_match(KW_INDICES))
		{
			sw = show_indices;
			if (PAR_match(KW_FOR))
				if (PAR_match(KW_DATABASE))
				{
					if (value = (BLK) get_dbb(QLI_token->tok_symbol))
						PAR_token();
					else
						ERRQ_syntax(221);	// Msg221 database name
					sw = show_db_indices;
				}
				else if (!(value = (BLK) parse_qualified_relation()))
					ERRQ_syntax(220);	// Msg220 relation name
		}
		else if (PAR_match(KW_SECURITY_CLASS))
		{
			sw = show_security_class;
			value = (BLK) parse_name();
		}
		else if (PAR_match(KW_TRIGGERS))
		{
			sw = show_triggers;
			if (PAR_match(KW_FOR))
			{
				if (PAR_match(KW_DATABASE))
				{
					if (value = (BLK) get_dbb(QLI_token->tok_symbol))
						PAR_token();
					else
						ERRQ_syntax(221);	// Msg221 database name
				}
				else
				{
					PAR_match(KW_RELATION);
					if (!(value = (BLK) parse_qualified_relation()))
						ERRQ_syntax(222);	// Msg222 relation_name
					sw = show_trigger;
				}
			}
		}
		else if (PAR_match(KW_RELATIONS))
			sw = show_relations;
		else if (PAR_match(KW_VIEWS))
			sw = show_views;
		else if (PAR_match(KW_SECURITY_CLASSES))
			sw = show_security_classes;
		else if (PAR_match(KW_SYSTEM))
		{
			if (PAR_match(KW_TRIGGERS))
				sw = show_system_triggers;
			else if (PAR_match(KW_RELATIONS) || QLI_token->tok_type == tok_eol ||
				KEYWORD(KW_SEMI) || KEYWORD(KW_FOR))
			{
				sw = show_system_relations;
			}
			else
				ERRQ_syntax(215);	// Msg215 RELATIONS or TRIGGERS
		}
		else if (PAR_match(KW_PROCEDURES))
			sw = show_procedures;
		else if (PAR_match(KW_FILTERS))
			sw = show_filters;
		else if (PAR_match(KW_FUNCTIONS))
			sw = show_functions;
		else if (PAR_match(KW_GLOBAL))
		{
			PAR_real();
			if (PAR_match(KW_FIELD))
			{
				sw = show_global_field;
				value = (BLK) parse_field_name(0);
			}
			else if (PAR_match(KW_FIELDS))
				sw = show_global_fields;
		}
		else if (symbol && symbol->sym_type == SYM_relation)
		{
			sw = show_relation;
			value = symbol->sym_object;
			PAR_token();
		}
		else if (value = (BLK) get_dbb(symbol))
		{
			sw = show_database;
			PAR_token();
			if (PAR_match(KW_DOT))
			{
				if (PAR_match(KW_RELATIONS))
					sw = show_relations;
				else if (PAR_match(KW_FIELDS))
					sw = show_db_fields;
				else if (PAR_match(KW_INDICES))
					sw = show_db_indices;
				else if (PAR_match(KW_SECURITY_CLASS))
					sw = show_security_class;
				else if (PAR_match(KW_TRIGGERS))
					sw = show_triggers;
				else if (PAR_match(KW_VIEWS))
					sw = show_views;
				else if (PAR_match(KW_SECURITY_CLASSES))
					sw = show_security_classes;
				else if (PAR_match(KW_SYSTEM))
				{
					if (PAR_match(KW_TRIGGERS))
						sw = show_system_triggers;
					else if (PAR_match(KW_RELATIONS) || QLI_token->tok_type == tok_eol ||
						KEYWORD(KW_SEMI) || KEYWORD(KW_FOR))
					{
						sw = show_system_relations;
					}
					else
						ERRQ_syntax(215);	// Msg215 RELATIONS or TRIGGERS
				}
				else if (PAR_match(KW_PROCEDURES))
					sw = show_procedures;
				else if (PAR_match(KW_FILTERS))
					sw = show_filters;
				else if (PAR_match(KW_FUNCTIONS))
					sw = show_functions;
				else if (PAR_match(KW_GLOBAL))
				{
					PAR_real();
					if (PAR_match(KW_FIELD))
					{
						sw = show_global_field;
						value = (BLK) parse_field_name(0);
					}
					else if (PAR_match(KW_FIELDS))
						sw = show_global_fields;
				}
				else
				{
					qli_rel* relation =
						resolve_relation(symbol, QLI_token->tok_symbol);
					if (relation)
					{
						sw = show_relation;
						value = relation->rel_symbol->sym_object;
						PAR_token();
					}
					else
					{
						sw = show_procedure;
						qli_proc* proc = (qli_proc*) ALLOCD(type_qpr);
						proc->qpr_database = (qli_dbb*) value;
						proc->qpr_name = parse_name();
						value = (BLK) proc;
					}
				}
			}
		}
		else
		{
			sw = show_procedure;
			value = (BLK) parse_qualified_procedure();
		}

		ALLQ_push((blk*) sw, &stack);
		if (!value && (sw == show_relations || sw == show_views ||sw == show_security_classes ||
			sw == show_system_triggers || sw == show_system_relations || sw == show_procedures ||
			sw == show_filters || sw == show_functions || sw == show_global_fields))
		{
			if (PAR_match(KW_FOR))
			{
				PAR_match(KW_DATABASE);
				if (value = (BLK) get_dbb(QLI_token->tok_symbol))
					PAR_token();
				else
					ERRQ_syntax(221);	// Msg221 database name
			}
		}
		ALLQ_push(value, &stack);
		count++;
		if (!PAR_match(KW_COMMA))
			break;
	}

	command_end();
	qli_syntax* node = make_list(stack);
	node->syn_count = count;
	node->syn_type = nod_show;

	return node;
}



static qli_syntax* parse_sort()
{
/**************************************
 *
 *	p a r s e _ s o r t
 *
 **************************************
 *
 * Functional description
 *	Parse a sort list.
 *
 **************************************/
	USHORT direction = 0;
	bool sensitive = false;
	qli_lls* stack = NULL;

	while (true)
	{
		PAR_real();
		if (!sql_flag)
		{
			if (PAR_match(KW_ASCENDING))
			{
				direction = 0;
				continue;
			}
			if (PAR_match(KW_DESCENDING))
			{
				direction = 1;
				continue;
			}
			if (PAR_match(KW_EXACTCASE))
			{
				sensitive = false;
				continue;
			}
			if (PAR_match(KW_ANYCASE))
			{
				sensitive = true;
				continue;
			}
		}
		qli_syntax* node;
		if (sql_flag && QLI_token->tok_type == tok_number)
		{
			node = syntax_node(nod_position, 1);
			node->syn_arg[0] = INT_CAST parse_ordinal();
		}
		else
			node = parse_value(0, 0);
		if (sensitive)
		{
			qli_syntax* upcase = syntax_node(nod_upcase, 1);
			upcase->syn_arg[0] = node;
			ALLQ_push((blk*) upcase, &stack);
		}
		else
			ALLQ_push((blk*) node, &stack);
		if (sql_flag)
			if (PAR_match(KW_ASCENDING))
				direction = 0;
			else if (PAR_match(KW_DESCENDING))
				direction = 1;
		ALLQ_push((blk*) (IPTR) direction, &stack);
		if (!PAR_match(KW_COMMA))
			break;
	}

	return make_list(stack);
}


static qli_syntax* parse_sql_alter()
{
/**************************************
 *
 *	p a r s e _ s q l _ a l t e r
 *
 **************************************
 *
 * Functional description
 *	Parse the leading clauses of a SQL ALTER statement.
 *
 **************************************/
	PAR_real_token();

	if (!PAR_match(KW_TABLE))
		ERRQ_syntax(407);		// Msg407 TABLE

	qli_syntax* node = syntax_node(nod_sql_al_table, 2);
	qli_rel* relation = parse_qualified_relation();
	node->syn_arg[0] = (qli_syntax*) relation;

	for (;;)
	{
	    qli_fld* field;
		if (PAR_match(KW_ADD))
		{
			field = parse_sql_field();
			field->fld_flags |= FLD_add;
		}
		else if (PAR_match(KW_DROP))
		{
			field = parse_field(false);
			field->fld_flags |= FLD_drop;
		}
		else
			ERRQ_syntax(405);	// Msg405 ADD or DROP

		field->fld_next = (qli_fld*) node->syn_arg[1];
		node->syn_arg[1] = (qli_syntax*) field;

		if (!PAR_match(KW_COMMA))
			break;
	}

	command_end();
	return node;

}



static qli_syntax* parse_sql_create()
{
/**************************************
 *
 *	p a r s e _ s q l _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Parse the leading clauses of a SQL CREATE statement.
 *
 **************************************/
	PAR_real_token();

	if (KEYWORD(KW_DATABASE))
		return parse_ready(nod_sql_database);

	if (KEYWORD(KW_UNIQUE) || KEYWORD(KW_ASCENDING) || KEYWORD(KW_DESCENDING) || KEYWORD(KW_INDEX))
	{
		bool unique = false, descending = false;
		while (true)
		{
			if (PAR_match(KW_UNIQUE))
				unique = true;
			else if (PAR_match(KW_ASCENDING))
				descending = false;
			else if (PAR_match(KW_DESCENDING))
				descending = true;
			else if (PAR_match(KW_INDEX))
				return parse_sql_index_create(unique, descending);
			else
				ERRQ_syntax(389);	// Msg389 INDEX
		}
	}

	if (PAR_match(KW_TABLE))
		return parse_sql_table_create();

#ifdef NOT_USED_OR_REPLACED
	//if (PAR_match (KW_VIEW))
	//	return parse_sql_view_create();
#endif

	ERRQ_syntax(386);			// Msg386 object type for CREATE
	return NULL;
}


static int parse_sql_dtype( USHORT* length, USHORT* scale, USHORT* precision, USHORT* sub_type)
{
/**************************************
 *
 *	p a r s e _ s q l _ d t y p e
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL datatype clause.
 *
 **************************************/
	USHORT dtype = dtype_unknown;

	const kwwords keyword = QLI_token->tok_keyword;
	PAR_token();
	*scale = 0;
	*length = 1;
	*precision = 0;
	*sub_type = 0;

	switch (keyword)
	{
	case KW_DATE:
		*length = 8;
		return dtype_timestamp;

	case KW_CHAR:
		dtype = dtype_text;
		break;

	case KW_VARCHAR:
		dtype = dtype_varying;
		break;

	case KW_SMALLINT:
		*length = sizeof(SSHORT);
		return dtype_short;

	case KW_INTEGER:
		*length = sizeof(SLONG);
		return dtype_long;

	case KW_BIGINT:
		*length = sizeof(SINT64);
		return dtype_int64;

	case KW_REAL:
	case KW_FLOAT:
		*length = sizeof(float);
		dtype = dtype_real;
		break;

	case KW_LONG:
		if (!PAR_match(KW_FLOAT))
			ERRQ_syntax(388);	// Msg388 "FLOAT"
		*length = sizeof(double);
		dtype = dtype_double;
		break;

	case KW_DOUBLE:
		if (!PAR_match(KW_PRECISION))
			ERRQ_syntax(509);   // Msg509 "PRECISION"
		*length = sizeof(double);
		dtype = dtype_double;
		break;

	case KW_DECIMAL:
		*length = sizeof(SLONG);
		dtype = dtype_long;
		*sub_type = dsc_num_type_decimal;
		break;

	case KW_NUMERIC:
		*length = sizeof(SLONG);
		dtype = dtype_long;
		*sub_type = dsc_num_type_numeric;
		break;
	}

	// CVC: SQL doesn't accept arbitrary types with scale specification.
	//if (dtype == dtype_long || dtype == dtype_real || dtype == dtype_double)
	if (keyword == KW_DECIMAL || keyword == KW_NUMERIC)
	{
		if (PAR_match(KW_LEFT_PAREN))
		{
			const USHORT logLength = parse_ordinal();
			if (logLength < 1)
				ERRQ_syntax(512);  // Msg512 "Field length should be greater than zero"
			else if (logLength < 5)
			{
				*length = sizeof(SSHORT);
				dtype = dtype_short;
			}
			else if (logLength > 18)
				ERRQ_syntax(511);  // Msg511 "Field length exceeds allowed range"
			else if (logLength > 9)
			{
				*length = sizeof(SINT64);
				dtype = dtype_int64;
			}

			if (PAR_match(KW_COMMA))
			{
				const bool l = PAR_match(KW_MINUS) ? true : false;
				*scale = parse_ordinal();
				if (*scale > logLength)
					ERRQ_syntax(510);  // Msg510 "Field scale exceeds allowed range"

				if (l || *scale > 0) // We need to have it negative in system tables.
					*scale = -(*scale);
			}

			*precision = logLength;
			parse_matching_paren();
		}
	}
	else if (dtype == dtype_text || dtype == dtype_varying)
	{
		if (PAR_match(KW_LEFT_PAREN))
		{
			USHORT l = parse_ordinal();
			if (l > MAX_COLUMN_SIZE)
				ERRQ_syntax(511);  // Msg511 "Field length exceeds allowed range"
			if (dtype == dtype_varying)
			{
				if (l > MAX_COLUMN_SIZE - sizeof(SSHORT))
					ERRQ_syntax(511);  // Msg511 "Field length exceeds allowed range"
				l += sizeof(SSHORT);
			}
			*length = l;
			parse_matching_paren();
		}
	}

	return dtype;
}


static qli_fld* parse_sql_field()
{
/**************************************
 *
 *	p a r s e _ s q l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Parse a field description.
 *
 **************************************/
	PAR_real();

    USHORT dtype, length, scale, precision, sub_type;
	dtype = length = scale = precision = sub_type = 0;
	qli_symbol* name = parse_symbol();

	PAR_real();
	switch (QLI_token->tok_keyword)
	{
	case KW_DOUBLE:
		PAR_match(KW_PRECISION);
	case KW_NUMERIC:
	case KW_REAL:
	case KW_DATE:
	case KW_CHAR:
	case KW_VARCHAR:
	case KW_SMALLINT:
	case KW_INTEGER:
	case KW_FLOAT:
	case KW_LONG:
	case KW_DECIMAL:
	case KW_BIGINT:
		dtype = parse_sql_dtype(&length, &scale, &precision, &sub_type);
		break;

	default:
		ERRQ_syntax(179);		// Msg179 field definition clause
		break;
	}

	qli_fld* field = (qli_fld*) ALLOCDV(type_fld, length);
	field->fld_name = name;
	field->fld_dtype = dtype;
	field->fld_scale = scale;
	field->fld_length = length;
	field->fld_precision = precision;
	field->fld_sub_type = sub_type;

	if (PAR_match(KW_NOT))
		if (PAR_match(KW_NULL)) {
			field->fld_flags |= FLD_not_null;
		}
		else {
			ERRQ_syntax(393);	// Msg393 NULL
		}

	return field;
}


static qli_syntax* parse_sql_grant_revoke(const nod_t type)
{
/**************************************
 *
 *	p a r s e _ s q l _ g r a n t _ r e v o k e
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL GRANT/REVOKE statement.
 *
 **************************************/
	PAR_real_token();
	qli_syntax* node = syntax_node(type, s_grant_count);
	qli_lls* stack = NULL;
	USHORT privileges = 0;

	if (PAR_match(KW_ALL))
	{
		PAR_match(KW_PRIVILEGES);
		privileges |= PRV_all;
	}
	else
	{
		while (true)
		{
			PAR_real();
			if (PAR_match(KW_SELECT))
			{
				privileges |= PRV_select;
				continue;
			}
			if (PAR_match(KW_INSERT))
			{
				privileges |= PRV_insert;
				continue;
			}
			if (PAR_match(KW_DELETE))
			{
				privileges |= PRV_delete;
				continue;
			}
			if (PAR_match(KW_UPDATE))
			{
				privileges |= PRV_update;

				if (PAR_match(KW_COMMA))
					continue;

				if (KEYWORD(KW_ON))
					break;

				if (!PAR_match(KW_LEFT_PAREN))
					ERRQ_syntax(187);	// Msg187 left parenthesis

				do {
					if (KEYWORD(KW_SELECT) || KEYWORD(KW_INSERT) ||
						KEYWORD(KW_DELETE) || KEYWORD(KW_UPDATE))
					{
						break;
					}
					PAR_real();
					ALLQ_push((blk*) parse_name(), &stack);

				} while (PAR_match(KW_COMMA));

				if (!PAR_match(KW_RIGHT_PAREN))
					ERRQ_syntax(191);	// Msg191 right parenthesis

				continue;
			}

			if (!PAR_match(KW_COMMA))
				break;
		}
	}

	node->syn_arg[s_grant_fields] = make_list(stack);

	PAR_real();
	if (!PAR_match(KW_ON))
		ERRQ_syntax(397);		// Msg397 ON

	PAR_real();
	if (!(node->syn_arg[s_grant_relation] = (qli_syntax*) parse_qualified_relation()))
		ERRQ_syntax(170);		// Msg170 relation name

	if (type == nod_sql_grant)
	{
		if (!PAR_match(KW_TO))
			ERRQ_syntax(404);	// Msg404 TO
	}
	else
	{
		if (!PAR_match(KW_FROM))
			ERRQ_syntax(403);	// Msg403 FROM
	}

	stack = NULL;

	while (true)
	{
		PAR_real();
		ALLQ_push((blk*) parse_name(), &stack);
		if (!PAR_match(KW_COMMA))
			break;
	}

	node->syn_arg[s_grant_users] = make_list(stack);

	if (type == nod_sql_grant)
		if (PAR_match(KW_WITH))
		{
			PAR_real();
			if (!PAR_match(KW_GRANT))
				ERRQ_syntax(401);	// Msg401 GRANT
			PAR_match(KW_OPTION);
			privileges |= PRV_grant_option;
		}

	node->syn_arg[s_grant_privileges] = INT_CAST privileges;

	return node;
}


static qli_syntax* parse_sql_index_create(const bool unique, const bool descending)
{
/**************************************
 *
 *	p a r s e _ s q l _ i n d e x _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Parse the SQL CREATE INDEX statement.
 *
 **************************************/
	PAR_real();
	qli_syntax* node = syntax_node(nod_def_index, s_dfi_count);

	if (unique)
		node->syn_flags |= s_dfi_flag_unique;
	if (descending)
		node->syn_flags |= s_dfi_flag_descending;

	node->syn_arg[s_dfi_name] = (qli_syntax*) parse_symbol();

	PAR_real();
	if (!PAR_match(KW_ON))
		ERRQ_syntax(397);		// Msg397 ON

	if (!(node->syn_arg[s_dfi_relation] = (qli_syntax*) parse_qualified_relation()))
		ERRQ_syntax(170);		// Msg170 relation name

	PAR_real();

	if (!PAR_match(KW_LEFT_PAREN))
		ERRQ_syntax(185);		// Msg185 left parenthesis

	qli_lls* stack = NULL;

	for (;;)
	{
		ALLQ_push((blk*) parse_name(), &stack);
		if (PAR_match(KW_RIGHT_PAREN))
			break;
		if (!PAR_match(KW_COMMA))
			ERRQ_syntax(171);	// Msg171 comma between field definitions
	}

	node->syn_arg[s_dfi_fields] = make_list(stack);

	command_end();

	return node;
}


static qli_syntax* parse_sql_joined_relation() //( qli_syntax* prior_context)
{
/**************************************
 *
 *	p a r s e _ s q l _ j o i n e d _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a join relation clause.
 *
 **************************************/
	qli_syntax* left;

	if (PAR_match(KW_LEFT_PAREN))
	{
		left = parse_sql_joined_relation(); // (0)
		parse_matching_paren();
	}
	else if (!(left = parse_sql_relation()))
		return NULL;

	return parse_sql_join_clause(left);
}


static qli_syntax* parse_sql_join_clause( qli_syntax* left)
{
/**************************************
 *
 *	p a r s e _ s q l _ j o i n _ c l a u s e
 *
 **************************************
 *
 * Functional description
 *	Parse a join relation clause.
 *
 **************************************/
	const nod_t join_type = parse_join_type();
	if (join_type == nod_nothing)
		return left;

	qli_syntax* right = parse_sql_joined_relation(); //(left);
	if (!right)
		ERRQ_syntax(490);		// Msg490 joined relation clause

	if (!PAR_match(KW_ON))
		ERRQ_syntax(492);		// Msg492 ON

	qli_syntax* node = syntax_node(nod_rse, (int) s_rse_count + 2 * 2);
	node->syn_count = 2;
	node->syn_arg[s_rse_count] = left;
	node->syn_arg[s_rse_count + 2] = right;
	node->syn_arg[s_rse_join_type] = (qli_syntax*) join_type;
	node->syn_arg[s_rse_boolean] = parse_boolean(0);

	return parse_sql_join_clause(node);
}


static qli_syntax* parse_sql_table_create()
{
/**************************************
 *
 *	p a r s e _ s q l _ t a b l e _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Parse the SQL CREATE TABLE statement.
 *
 **************************************/
	PAR_real();
	qli_syntax* node = syntax_node(nod_sql_cr_table, 1);
	qli_rel* relation = (qli_rel*) ALLOCD(type_rel);
	node->syn_arg[0] = (qli_syntax*) relation;
	relation->rel_database = parse_database();
	relation->rel_symbol = parse_symbol();

	qli_fld** ptr = &relation->rel_fields;

	if (!PAR_match(KW_LEFT_PAREN))
		ERRQ_syntax(185);		// Msg185 left parenthesis

	PAR_real();

	for (;;)
	{
	    qli_fld* field = parse_sql_field();
		*ptr = field;
		ptr = &field->fld_next;
		if (PAR_match(KW_RIGHT_PAREN))
			break;
		if (!PAR_match(KW_COMMA))
			ERRQ_syntax(171);	// Msg171 comma between field definitions
	}

	command_end();

	return node;
}

#ifdef NOT_USED_OR_REPLACED
static qli_syntax* parse_sql_view_create()
{
/**************************************
 *
 *	p a r s e _ s q l _ v i e w _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Parse the SQL CREATE VIEW statement.
 *
 **************************************/
	PAR_real();

	sw_sql_view = true;
	qli_syntax* node = syntax_node(nod_sql_cr_view, s_crv_count);
	qli_lls* stack = NULL;

	qli_rel* relation = (qli_rel*) ALLOCD(type_rel);
	node->syn_arg[s_crv_name] = (qli_syntax*) relation;
	relation->rel_database = parse_database();
	relation->rel_symbol = parse_symbol();

	// if field list is present parse it and create corresponding field blocks

	if (PAR_match(KW_LEFT_PAREN))
	{
		for (;;)
		{
			ALLQ_push(parse_name(), &stack);
			if (PAR_match(KW_RIGHT_PAREN))
				break;
			if (!PAR_match(KW_COMMA))
				ERRQ_syntax(171);	// Msg171 comma between field definitions
		}
	}

	// node->syn_arg [s_crv_fields] = make_list (stack);

	if (!PAR_match(KW_AS))
		ERRQ_syntax(394);		// Msg394 As

	if (!KEYWORD(KW_SELECT))
		ERRQ_syntax(395);		// Msg395 Select

	node->syn_arg[s_crv_rse] = parse_select();

	sw_sql_view = false;

	return node;
}
#endif

static qli_syntax* parse_sql_relation()
{
/**************************************
 *
 *	p a r s e _ s q l _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL relation clause.
 *
 **************************************/
	qli_syntax* node = syntax_node(nod_relation, s_rel_count);

	if (!(node->syn_arg[s_rel_relation] = (qli_syntax*) parse_qualified_relation()))
		ERRQ_syntax(223);		// Msg223 relation name

	if (!QLI_token->tok_symbol)
		node->syn_arg[s_rel_context] = (qli_syntax*) parse_symbol();

	return node;
}


static qli_syntax* parse_sql_rse()
{
/**************************************
 *
 *	p a r s e _ s q l _ r s e
 *
 **************************************
 *
 * Functional description
 *	Parse the trailing clauses of a SQL SELECT statement.
 *
 **************************************/
	qli_lls* stack = NULL;
	USHORT count = 0;
	PAR_real();

	if (!PAR_match(KW_FROM))
		ERRQ_syntax(224);		// Msg224 FROM clause

	// Parse FROM list of relations

	while (true)
	{
		count++;
		ALLQ_push((blk*) parse_sql_joined_relation(/*0*/), &stack);
		if (!PAR_match(KW_COMMA))
			break;
	}

	// Build a syntax node.  Since SQL doesn't support OVER, only every
	// other slot will be used in the RSE.

	qli_syntax* node = syntax_node(nod_rse, (int) s_rse_count + 2 * count);
	node->syn_count = count;
	qli_syntax** ptr = &node->syn_arg[(int) s_rse_count + 2 * count];

	while (stack)
	{
		--ptr;
		*--ptr = (qli_syntax*) ALLQ_pop(&stack);
	}

	if (PAR_match(KW_WITH))
		node->syn_arg[s_rse_boolean] = parse_boolean(0);

	if (PAR_match(KW_GROUP))
	{
		if (sw_sql_view)
			ERRQ_syntax(391);	// Msg391 No group by in view def
		PAR_real();
		PAR_match(KW_BY);
		stack = NULL;
		while (true)
		{
			ALLQ_push((blk*) parse_udf_or_field(), &stack);
			if (!PAR_match(KW_COMMA))
				break;
		}
		node->syn_arg[s_rse_group_by] = make_list(stack);
		if (PAR_match(KW_HAVING))
			node->syn_arg[s_rse_having] = parse_boolean(0);
	}

	return node;
}


static qli_syntax* parse_sql_singleton_select()
{
/**************************************
 *
 *	p a r s e _ s q l _ s i n g l e t o n _ s e l e c t
 *
 **************************************
 *
 * Functional description
 *	Finish parsing an SQL singleton select and
 *	turn it into a FIRST ... FROM --- not exactly
 *	kosher, but a start.
 *
 **************************************/
	qli_syntax* value = parse_primitive_value(0, 0);
	PAR_real();

	qli_syntax* node = syntax_node(nod_from, s_stt_count);
	node->syn_arg[s_stt_value] = value;

	node->syn_arg[s_stt_rse] = parse_sql_rse();
	--sql_flag; // The increment was done in parse_sql_subquery, the only caller.

	return node;
}


static qli_syntax* parse_sql_subquery()
{
/**************************************
 *
 *	p a r s e _ s q l _ s u b q u e r y
 *
 **************************************
 *
 * Functional description
 *	Parse an sql subquery that should
 *	return a single value.
 *
 **************************************/
	if (sw_sql_view)
		ERRQ_syntax(392);		// Msg392 No aggregates in view def

	PAR_token();

	const kwwords keyword = next_keyword();
	++sql_flag;

	const nod_types* ntypes;
	const nod_types* const endtypes = statisticals + FB_NELEM(statisticals);
	for (ntypes = statisticals; ntypes < endtypes; ntypes++)
	{
		if (ntypes->nod_t_keyword == KW_none)
			return parse_sql_singleton_select();
		if (ntypes->nod_t_keyword == keyword)
			break;
	}

	fb_assert(ntypes < endtypes);
	if (ntypes >= endtypes)
	    return NULL;

	PAR_token();
	qli_syntax* node = syntax_node(ntypes->nod_t_node, s_stt_count);

	PAR_match(KW_LEFT_PAREN);

	if (node->syn_type != nod_count || !PAR_match(KW_ASTERISK))
	{
		if (PAR_match(KW_DISTINCT))
			node->syn_arg[s_prt_distinct] = INT_CAST TRUE;
		node->syn_arg[s_stt_value] = parse_value(0, 0);
	}

	parse_matching_paren();

	node->syn_arg[s_stt_rse] = parse_sql_rse();
	--sql_flag;

	return node;
}


static qli_syntax* parse_statement()
{
/**************************************
 *
 *	p a r s e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Parse a statement.  (Set statement switch
 *	to true here as well as in PARQ_parse to
 *	avoid confusion with linked statements
 *	e.g. THEN conjuncts )
 *
 **************************************/
	qli_syntax* node;

	PAR_real();
	sw_statement = true;
	function_count = 0;

	switch (next_keyword())
	{
	case KW_ABORT:
		node = parse_abort();
		break;

	case KW_ACCEPT:
		node = parse_accept();
		break;

	case KW_COMMIT:
		node = parse_transaction(nod_commit_retaining);
		break;

	case KW_DECLARE:
		node = parse_declare();
		break;

	case KW_DELETE:
		PAR_match(KW_DELETE);
		node = parse_delete();
		break;

	case KW_ERASE:
		node = parse_erase();
		break;

	case KW_FOR:
		node = parse_for();
		break;

	case KW_IF:
		node = parse_if();
		break;

	case KW_INSERT:
		node = parse_insert();
		break;

	case KW_LIST:
		node = parse_list_fields();
		break;

	case KW_MODIFY:
		node = parse_modify();
		break;

	case KW_PRINT:
		node = parse_print();
		break;

	case KW_REPEAT:
		node = parse_repeat();
		break;

	case KW_REPORT:
		node = parse_report();
		break;

	case KW_SELECT:
		node = parse_select();
		break;

	case KW_STORE:
		node = parse_store();
		break;

	case KW_UPDATE:
		node = parse_update();
		break;

	case KW_BEGIN:
	    {
			qli_lls* stack = NULL;
			PAR_token();
			while (true)
			{
				PAR_real();
				if (PAR_match(KW_END))
					break;
				ALLQ_push((blk*) parse_statement(), &stack);
				PAR_match(KW_SEMI);
			}
			node = make_list(stack);
		}
		break;

	default:
		node = parse_assignment();
	}

	check_end();

	// Check for the "THEN" connective.  If found, make a list of statements.

	if (QLI_token->tok_type != tok_eol || (QLI_semi && !KEYWORD(KW_SEMI)))
		PAR_match(KW_SEMI);

	if (!PAR_match(KW_THEN))
		return node;

	qli_lls* stack = NULL;
	ALLQ_push((blk*) node, &stack);
	ALLQ_push((blk*) parse_statement(), &stack);

	return make_list(stack);
}


static qli_syntax* parse_statistical()
{
/**************************************
 *
 *	p a r s e _ s t a t i s t i c a l
 *
 **************************************
 *
 * Functional description
 *	Parse statistical expression.
 *
 **************************************/
	const kwwords keyword = next_keyword();
	PAR_token();

	const nod_types* ntypes;
	const nod_types* const endtypes = statisticals + FB_NELEM(statisticals);
	for (ntypes = statisticals; ntypes < endtypes; ntypes++)
		if (ntypes->nod_t_keyword == keyword)
			break;

	fb_assert(ntypes < endtypes);
	if (ntypes >= endtypes)
	    return NULL;

	// Handle SQL statisticals a little differently

	if (sql_flag)
	{
		qli_syntax* anode = syntax_node(ntypes->nod_t_sql_node, s_stt_count);
		if (!PAR_match(KW_LEFT_PAREN))
			ERRQ_syntax(227);	// Msg227 left parenthesis
		if (anode->syn_type != nod_agg_count || !PAR_match(KW_ASTERISK))
		{
			if (PAR_match(KW_DISTINCT))
				anode->syn_arg[s_prt_distinct] = INT_CAST TRUE;
			anode->syn_arg[s_stt_value] = parse_value(0, 0);
		}
		parse_matching_paren();
		return anode;
	}

	// Handle GDML statisticals

	qli_syntax* node = syntax_node(ntypes->nod_t_node, s_stt_count);

	if (node->syn_type != nod_count)
		node->syn_arg[s_stt_value] = parse_value(0, 0);

	if (!PAR_match(KW_OF))
	{
		if (sw_report)
		{
			if (function_count > 0)
				IBERROR(487);	// Msg487 Invalid argument for UDF
			node->syn_type = ntypes->nod_t_rpt_node;
			return node;
		}
		PAR_real();
		if (!PAR_match(KW_OF))
			ERRQ_syntax(228);	// Msg 228 OF
	}

	node->syn_arg[s_stt_rse] = parse_rse();

	return node;
}


static qli_syntax* parse_store()
{
/**************************************
 *
 *	p a r s e _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Parse a STORE statement.
 *
 **************************************/
	PAR_token();
	qli_syntax* node = syntax_node(nod_store, s_sto_count);
	node->syn_arg[s_sto_relation] = parse_relation();

	if (test_end())
		return node;

	PAR_match(KW_USING);

	node->syn_arg[s_sto_statement] = parse_statement();

	return node;
}


static TEXT* parse_string()
{
/**************************************
 *
 *	p a r s e _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Save the current token as a string, advance to the next
 *	token, and return a pointer to the string.
 *
 **************************************/
	TEXT* string = make_string(QLI_token->tok_string, QLI_token->tok_length);
	PAR_token();

	return string;
}


static qli_symbol* parse_symbol()
{
/**************************************
 *
 *	p a r s e _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *	Parse the next token as a context symbol.
 *
 **************************************/
	USHORT l = QLI_token->tok_length;
	qli_symbol* context = (qli_symbol*) ALLOCDV(type_sym, l);
	context->sym_type = SYM_context;
	context->sym_length = l;
	const TEXT* q = QLI_token->tok_string;
	context->sym_string = context->sym_name;
	TEXT* p = context->sym_name;

	if (l)
		do {
			const TEXT c = *q++;
			*p++ = UPPER(c);
		} while (--l);

	PAR_token();

	return context;
}


static void parse_terminating_parens( USHORT * paren_count, USHORT * local_count)
{
/**************************************
 *
 *	p a r s e _ t e r m i n a t i n g _ p a r e n s
 *
 **************************************
 *
 * Functional description
 *	Check for balancing parenthesis.  If missing, barf.
 *
 **************************************/

	if (*paren_count && paren_count == local_count)
		do {
			parse_matching_paren();
		} while (--(*paren_count));
}


static qli_syntax* parse_transaction( nod_t node_type)
{
/**************************************
 *
 *	p a r s e _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Parse the FINISH, COMMIT, ROLLBACK,
 *	and PREPARE commands and the COMMIT statement.
 *
 **************************************/
	qli_lls* stack = NULL;
	PAR_token();

	if (!KEYWORD(KW_SEMI))
		while (true)
		{
		    qli_symbol* symbol;
			for (symbol = QLI_token->tok_symbol; symbol; symbol = symbol->sym_homonym)
			{
				if (symbol->sym_type == SYM_database)
					break;
			}
			if (!symbol)
				ERRQ_syntax(229);	// Msg229 database name
			ALLQ_push(symbol->sym_object, &stack);
			PAR_token();
			if (!PAR_match(KW_COMMA))
				break;
		}

	command_end();
	qli_syntax* node = make_list(stack);
	node->syn_type = node_type;

	return node;
}


static qli_syntax* parse_udf_or_field()
{
/**************************************
 *
 *	p a r s e _ u d f _ o r _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Parse a function or field reference.
 *
 **************************************/
	const qli_symbol* symbol = QLI_token->tok_symbol;

	if (symbol && symbol->sym_type == SYM_function)
		return parse_function();

	return parse_field_name(0);
}


static qli_syntax* parse_update()
{
/**************************************
 *
 *	p a r s e _ u p d a t e
 *
 **************************************
 *
 * Functional description
 *	Parse a SQL UPDATE statement.
 *
 **************************************/
	++sql_flag;
	PAR_token();
	qli_syntax* node = syntax_node(nod_modify, s_mod_count);
	qli_syntax* rse = syntax_node(nod_rse, (int) s_rse_count + 2);
	node->syn_arg[s_mod_rse] = rse;
	rse->syn_count = 1;
	rse->syn_arg[s_rse_count] = parse_sql_relation();

	if (!PAR_match(KW_SET))
		ERRQ_syntax(230);		// Msg230 SET

	// Pick up assignments

	qli_lls* stack = NULL;

	while (true)
	{
		ALLQ_push((blk*) parse_assignment(), &stack);
		if (!PAR_match(KW_COMMA))
			break;
	}

	// Pick up boolean, if present

	if (PAR_match(KW_WITH))
		rse->syn_arg[s_rse_boolean] = parse_boolean(0);

	node->syn_arg[s_mod_statement] = make_list(stack);
	--sql_flag;

	return node;
}


static qli_syntax* parse_value( USHORT* paren_count, bool* bool_flag)
{
/**************************************
 *
 *	p a r s e _ v a l u e
 *
 **************************************
 *
 * Functional description
 *	Parse a general value expression.  In practice, this means parse the
 *	lowest precedence operator CONCATENATE.
 *
 **************************************/
	USHORT local_count;

	if (!paren_count)
	{
		local_count = 0;
		paren_count = &local_count;
	}

	bool local_flag;
	if (!bool_flag)
	{
		local_flag = false;
		bool_flag = &local_flag;
	}

	qli_syntax* node = parse_add(paren_count, bool_flag);

	while (true)
	{
		if (!PAR_match(KW_BAR))
		{
			parse_terminating_parens(paren_count, &local_count);
			return node;
		}
		qli_syntax* arg = node;
		node = syntax_node(nod_concatenate, 2);
		node->syn_arg[0] = arg;
		node->syn_arg[1] = parse_add(paren_count, bool_flag);
	}
}


static bool potential_rse()
{
/**************************************
 *
 *	p o t e n t i a l _ r s e
 *
 **************************************
 *
 * Functional description
 *	Test to see if the current token is likely (sic!) to be part of
 *	a record selection expression.
 *
 **************************************/
	for (const qli_symbol* symbol = QLI_token->tok_symbol; symbol; symbol = symbol->sym_homonym)
	{
		if ((symbol->sym_type == SYM_keyword && symbol->sym_keyword == KW_FIRST) ||
			symbol->sym_type == SYM_relation || symbol->sym_type == SYM_database)
		{
			return true;
		}
	}

	return false;
}


static qli_rel* resolve_relation( const qli_symbol* db_symbol, qli_symbol* relation_symbol)
{
/**************************************
 *
 *	r e s o l v e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Given symbols for a database and a relation (either may be null),
 *	resolve the relation.  If the relation can't be resolved, return
 *	NULL (don't error!).
 *
 **************************************/

	// If we don't recognize the relation, punt.

	if (!relation_symbol)
		return NULL;

	// If a database symbol is present, resolve the relation against the
	// the given database.

	if (db_symbol)			// && db_symbol->sym_type == SYM_database ?
	{
		for (; db_symbol; db_symbol = db_symbol->sym_homonym)
		{
			for (qli_symbol* temp = relation_symbol; temp; temp = temp->sym_homonym)
				if (temp->sym_type == SYM_relation)
				{
					qli_rel* relation = (qli_rel*) temp->sym_object;
					if (relation->rel_database == (qli_dbb*) db_symbol->sym_object)
						return relation;
				}
		}
		return NULL;
	}

	// No database qualifier, so search all databases.

	for (qli_dbb* dbb = QLI_databases; dbb; dbb = dbb->dbb_next)
	{
		for (qli_symbol* temp = relation_symbol; temp; temp = temp->sym_homonym)
			if (temp->sym_type == SYM_relation)
			{
				qli_rel* relation = (qli_rel*) temp->sym_object;
				if (relation->rel_database == dbb)
					return relation;
			}
	}

	return NULL;
}


static qli_syntax* syntax_node( nod_t type, USHORT count)
{
/**************************************
 *
 *	s y n t a x _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Allocate and initialize a syntax node of given type.
 *
 **************************************/
	qli_syntax* node = (qli_syntax*) ALLOCDV(type_syn, count);
	node->syn_type = type;
	node->syn_count = count;

	return node;
}


static bool test_end()
{
/**************************************
 *
 *	t e s t _ e n d
 *
 **************************************
 *
 * Functional description
 *	Test for end of a statement.  In specific, test for one of
 *	THEN, ELSE, ON, or a semi-colon.
 *
 **************************************/

	if (KEYWORD(KW_THEN) || KEYWORD(KW_ON) || KEYWORD(KW_ELSE) || KEYWORD(KW_SEMI))
	{
		return true;
	}

	return false;
}
