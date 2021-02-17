%{
/** source of nntp odbc sql parser -- yyparse.y

    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/

static	char	sccsid[]
	= "@(#)SQL parser for NNSQL(NetNews SQL), Copyright(c) 1995, 1996 by Ke Jin";

#include	<config.h>

#include	<nncol.h>
#include	<yyenv.h>
#include	<yystmt.h>
#include	<yylex.h>
#include	<yyerr.h>
#include	<nndate.h>

# ifdef YYLSP_NEEDED
#  undef YYLSP_NEEDED
# endif

#if defined(YYBISON) || defined(__YY_BISON__)
# define yylex(pyylval) 	nnsql_yylex(pyylval, pyyenv)
#else
# define yylex()		nnsql_yylex(&yylval, pyyenv)
#endif

#define yyparse(x)		nnsql_yyparse	(pyyenv)
#define yyerror(msg)		nnsql_yyerror (pyyenv, msg)
#define SETYYERROR(env, code)	{ env->pstmt->errcode = code; \
				  env->pstmt->errpos = env->errpos;}

typedef struct
{
	char*	schema_tab_name;
	char*	column_name;
} column_name_t;

static void	unpack_col_name(char* schema_tab_column_name, column_name_t* ptr);
static void*	add_node(yystmt_t* pstmt, node_t* node);
static void	srchtree_reloc(node_t* srchtree, int num);
static int	add_attr(yystmt_t* pstmt, int idx, int wstat);
static void*	add_all_attr(yystmt_t* pstmt);
static void*	add_news_attr(yystmt_t* pstmt);
static void*	add_xnews_attr(yystmt_t* pstmt);
static void*	add_column(yystmt_t* pstmt, yycol_t* pcol);
static void	nnsql_yyerror(yyenv_t* pyyenv, char* msg);
static int	table_check(yystmt_t* pstmt);
static int	column_name(yystmt_t* pstmt, char* name);
static void*	attr_name(yystmt_t* pstmt, char* name);
static int	add_ins_head(yystmt_t* pstmt, char* head, int idx);
static int	add_ins_value(yystmt_t* pstmt, node_t node, int idx);
static char*	get_unpacked_attrname(yystmt_t* pstmt, char* name);

#define 	ERROR_PTR		((void*)(-1L))
#define 	EMPTY_PTR		ERROR_PTR

%}

%pure_parser	/* forcing to generate reentrant parser */
%start	sql_stmt

%union {
	char*	qstring;
	char*	name;
	long	number;

	int	ipar;		/* parameter index */
	int	cmpop;		/* for comparsion operators */
	char	esc;
	int	flag;		/* for not_opt and case_opt */
	int	idx;

	void*	offset; 	/* actually, it is used as a 'int' offset */

	node_t node;		/* a node haven't add to tree */
}

%token	kwd_select
%token	kwd_all
%token	kwd_news
%token	kwd_xnews
%token	kwd_distinct
%token	kwd_count
%token	kwd_from
%token	kwd_where
%token	kwd_in
%token	kwd_between
%token	kwd_like
%token	kwd_escape
%token	kwd_group
%token	kwd_by
%token	kwd_having
%token	kwd_order
%token	kwd_for
%token	kwd_insert
%token	kwd_into
%token	kwd_values
%token	kwd_delete
%token	kwd_update
%token	kwd_create
%token	kwd_alter
%token	kwd_drop
%token	kwd_table
%token	kwd_column
%token	kwd_view
%token	kwd_index
%token	kwd_of
%token	kwd_current
%token	kwd_grant
%token	kwd_revoke
%token	kwd_is
%token	kwd_null
%token	kwd_call
%token	kwd_uncase
%token	kwd_case

%token	kwd_fn
%token	kwd_d

%token	<qstring>	QSTRING
%token	<number>	NUM
%token	<name>		NAME

%token	<ipar>		PARAM
%type	<esc>		escape_desc
%type	<flag>		not_opt
%type	<flag>		case_opt

%type	<name>		tab_name
%type	<offset>	search_condition
%type	<offset>	pattern
%type	<offset>	attr_name
%type	<offset>	value_list

%type	<idx>		ins_head_list
%type	<idx>		ins_value_list

%type	<node>		value
%type	<node>		ins_value

%type	<qstring>	ins_head

%left			kwd_or
%left			kwd_and
%nonassoc		kwd_not
%nonassoc <cmpop>	CMPOP

%%
sql_stmt
	: stmt_body ';' { YYACCEPT; }
	;

stmt_body
	:
	| kwd_select distinct_opt select_list kwd_from tab_list select_clauses
	  {
		if( ! table_check( pyyenv->pstmt ) )
		{
			SETYYERROR(pyyenv, NOT_SUPPORT_MULTITABLE_QUERY);
			YYABORT;
		}

		pyyenv->pstmt->type = en_stmt_select;
	  }
	| insert_stmt
	  {
		pyyenv->pstmt->type = en_stmt_insert;
	  }
	| srch_delete_stmt
	  {
		pyyenv->pstmt->type = en_stmt_srch_delete;
	  }
	| posi_delete_stmt
	| other_stmt
	  {
		pyyenv->pstmt->type = en_stmt_alloc;
		YYABORT;
	  }
	;

select_clauses
	: where_clause group_clause having_clause order_clause for_stmt
	;

for_stmt
	:
	| kwd_for kwd_update kwd_of col_name_list
	  {
		SETYYERROR(pyyenv, NOT_SUPPORT_UPDATEABLE_CURSOR) ;
		YYABORT;
	  }
	;

distinct_opt
	:
	| kwd_all
	| kwd_distinct
	  {
		SETYYERROR(pyyenv, NOT_SUPPORT_DISTINCT_SELECT);
		YYABORT;
	  }
	;

select_list
	:		{ if(add_all_attr(pyyenv->pstmt)) YYABORT; }
	| '*'		{ if(add_all_attr(pyyenv->pstmt)) YYABORT; }
	| news_hotlist	{ if(add_news_attr(pyyenv->pstmt)) YYABORT; }
	| news_xhotlist { if(add_xnews_attr(pyyenv->pstmt)) YYABORT; }
	| col_name_list { if(add_attr(pyyenv->pstmt, 0, 0))  YYABORT; }
	;

news_hotlist
	: kwd_news
	| kwd_news '(' ')'
	| '{' kwd_fn kwd_news '(' ')' '}'
	;

news_xhotlist
	: kwd_xnews
	| kwd_xnews '(' ')'
	| '{' kwd_fn kwd_xnews '(' ')' '}'
	;

col_name_list
	: col_name
	| col_name_list ',' col_name
	;

col_name
	: kwd_count count_sub_opt
	  {
		yycol_t col;

		col.iattr = en_sql_count;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;

	  }
	| NAME
	  { if( column_name( pyyenv->pstmt, $1 ) ) YYABORT; }
	| kwd_column '(' QSTRING ')'
	  { if( column_name( pyyenv->pstmt, $3 ) ) YYABORT; }
	| '{' kwd_fn kwd_column '(' QSTRING ')' '}'
	  { if( column_name( pyyenv->pstmt, $5 ) ) YYABORT; }
	| QSTRING
	  {
		yycol_t col;

		col.iattr = en_sql_qstr;
		col.value.qstr = $1;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  }
	| NUM
	  {
		yycol_t col;

		col.iattr = en_sql_num;
		col.value.num = $1;
		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  }
	| '{' kwd_d QSTRING '}'
	  {
		yycol_t col;

		col.iattr = en_sql_date;
		if( nnsql_odbcdatestr2date($3, &(col.value.date)) )
			YYABORT;

		col.table = 0;

		if( add_column(pyyenv->pstmt, &col) )
			YYABORT;
	  }
	| PARAM
	  {	SETYYERROR(pyyenv, VARIABLE_IN_SELECT_LIST); YYABORT; }
	;

count_sub_opt
	:
	| '(' ')'
	| '(' '*' ')'
	| '(' NAME ')'
	| '(' QSTRING ')'
	| '(' NUM ')'
	;

tab_list
	: tab_name { pyyenv->pstmt->table = $1; }
	| tab_list ',' tab_name
	  {
		if( ! pyyenv->pstmt->table )
			pyyenv->pstmt->table = $3;
		else if( !STREQ(pyyenv->pstmt->table, $3) )
		{
			SETYYERROR(pyyenv, NOT_SUPPORT_MULTITABLE_QUERY);
			YYABORT;
		}
	  }
	;

tab_name
	: NAME
	  { $$ = $1; }
	| kwd_table '(' QSTRING ')'
	  { $$ = $3; }
	| '{' kwd_fn kwd_table '(' QSTRING ')' '}'
	  { $$ = $5; }
	| PARAM
	  {
		SETYYERROR(pyyenv, VARIABLE_IN_TABLE_LIST);
		YYABORT;
	  }
	;

where_clause
	:
	  {
		pyyenv->pstmt->srchtree = 0;
	  }
	| kwd_where search_condition
	  {
		yystmt_t*	pstmt;
		long		offset;

		pstmt = pyyenv->pstmt;
		offset = (long)($2);

		pstmt->srchtree = pstmt->node_buf + offset;
		srchtree_reloc (	pstmt->node_buf, pstmt->srchtreenum);
	  }
	;

search_condition
	: '(' search_condition ')' { $$ = $2; }
	| kwd_not search_condition
	  {
		node_t	node;

		node.type = en_nt_logop;
		node.value.logop = en_logop_not;
		node.left = EMPTY_PTR;
		node.right= $2;

		if(($$ = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  }
	| search_condition kwd_or search_condition
	  {
		node_t	node;
		void*		p;

		node.type = en_nt_logop;
		node.value.logop = en_logop_or;
		node.left = $1;
		node.right= $3;

		if(($$ = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  }
	| search_condition kwd_and search_condition
	  {
		node_t	node;

		node.type = en_nt_logop;
		node.value.logop = en_logop_and;
		node.left = $1;
		node.right= $3;

		if(($$ = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  }
	| attr_name not_opt kwd_in '(' value_list ')'
	  {
		node_t	node;
		void*		ptr;

		node.type = en_nt_in;
		node.left = $1;
		node.right= $5;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( $2 )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right = ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		$$ = ptr;
	  }
	| attr_name not_opt kwd_between value kwd_and value
	  {
		node_t	node;
		void*		ptr;

		if( ((node.left = add_node(pyyenv->pstmt, &($4))) == ERROR_PTR)
		 || ((node.right= add_node(pyyenv->pstmt, &($6))) == ERROR_PTR)
		 || ((ptr	= add_node(pyyenv->pstmt, &node)) == ERROR_PTR) )
			YYABORT;

		node.type = en_nt_between;
		node.left = $1;
		node.right = ptr;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( $2 )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		$$ = ptr;
	  }
	| attr_name not_opt case_opt kwd_like pattern escape_desc
	  {
		node_t	node;
		void*		ptr;

		if( $3 )
			node.type = en_nt_caselike;
		else
			node.type = en_nt_like;
		node.value.esc = $6;
		node.left = $1;
		node.right= $5;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( $2 )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		$$ = ptr;
	  }
	| attr_name kwd_is not_opt kwd_null
	  {
		node_t	node;
		void*		ptr;

		node.type = en_nt_isnull;
		node.left = $1;
		node.right= EMPTY_PTR;

		if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;

		if( $3 )
		{
			node.type = en_nt_logop;
			node.value.logop = en_logop_not;
			node.left = EMPTY_PTR;
			node.right= ptr;

			if((ptr = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
				YYABORT;
		}

		$$ = ptr;
	  }
	| attr_name CMPOP value
	  {
		node_t	node;

		node.type = en_nt_cmpop;
		node.value.cmpop = $2;
		node.left = $1;

		if( ((node.right= add_node(pyyenv->pstmt, &($3))) == ERROR_PTR)
		 || (($$	= add_node(pyyenv->pstmt, &node)) == ERROR_PTR) )
			YYABORT;
	  }
	;

case_opt
	:
	  { $$ = 0; }
	| kwd_case
	  { $$ = 0; }
	| kwd_uncase
	  { $$ = 1; }
	| kwd_all kwd_case
	  { $$ = 1; }
	;

attr_name
	: NAME
	  { $$ = attr_name( pyyenv->pstmt, $1 ); if( $$ == ERROR_PTR ) YYABORT; }
	| kwd_column '(' QSTRING ')'
	  { $$ = attr_name( pyyenv->pstmt, $3 ); if( $$ == ERROR_PTR ) YYABORT; }
	| '{' kwd_fn kwd_column '(' QSTRING ')' '}'
	  { $$ = attr_name( pyyenv->pstmt, $5 ); if( $$ == ERROR_PTR ) YYABORT; }
	;

value_list
	: value
	  {
		if( ($$ = add_node( pyyenv->pstmt, &($1))) == ERROR_PTR )
			YYABORT;
	  }
	| value_list ',' value
	  {
		node_t	node;

		node = $3;
		node.left = EMPTY_PTR;
		node.right= $1;

		if( ($$ = add_node(pyyenv->pstmt, &node)) == ERROR_PTR )
			YYABORT;
	  }
	;

value
	: kwd_null
	  {
		node_t	node;

		node.type = en_nt_null;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		$$ = node;
	  }
	| PARAM
	  {
		node_t	node;

		node.type = en_nt_param,
		node.value.ipar = $1;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		pyyenv->pstmt->npar ++;

		$$ = node;
	  }
	| QSTRING
	  {
		node_t	node;

		node.type = en_nt_qstr;
		node.value.qstr = $1;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		$$ = node;
	  }
	| NUM
	  {
		node_t	node;

		node.type = en_nt_num;
		node.value.num = $1;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		$$ = node;
	  }
	| '{' kwd_d QSTRING '}'
	  {
		node_t	node;

		node.type = en_nt_date;
		if( nnsql_odbcdatestr2date($3, &(node.value.date)) )
			YYABORT;

		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		$$ = node;
	  }
	;

escape_desc
	:				{ $$ = 0; }
	| kwd_escape QSTRING		{ $$ = $2[0]; }
	| '{' kwd_escape  QSTRING '}'	{ $$ = $3[0]; }
	;

pattern
	: PARAM
	  {
		node_t	node;

		node.type = en_nt_param;
		node.value.ipar = $1;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		pyyenv->pstmt->npar ++;

		if(($$ = add_node(pyyenv->pstmt, &node)) == EMPTY_PTR)
			YYABORT;
	  }
	| QSTRING
	  {
		node_t	node;

		node.type = en_nt_qstr;
		node.value.qstr = $1;
		node.left = EMPTY_PTR;
		node.right= EMPTY_PTR;

		if(($$ = add_node(pyyenv->pstmt, &node)) == ERROR_PTR)
			YYABORT;
	  }
	;

not_opt
	:		{ $$ = 0; }
	| kwd_not	{ $$ = 1; }
	;

group_clause
	:
	| kwd_group kwd_by col_name_list
	  {
		SETYYERROR(pyyenv, NOT_SUPPORT_GROUP_CLAUSE);
		YYABORT;
	  }
	;

having_clause
	:
	| kwd_having search_condition
	  {
		SETYYERROR(pyyenv, NOT_SUPPORT_HAVING_CLAUSE);
		YYABORT;
	  }
	;

order_clause
	:
	| kwd_order kwd_by col_name_list
	  {
		SETYYERROR(pyyenv, NOT_SUPPORT_ORDER_CLAUSE);
		YYABORT;
	  }
	;

insert_stmt
	: kwd_insert kwd_into tab_name '(' ins_head_list ')' kwd_values '(' ins_value_list ')'
	  {
		int	i;
		char*	head = 0;

		if( $5 > $9 )
		{
			SETYYERROR(pyyenv, INSERT_VALUE_LESS);
			YYABORT;
		}

		if( $5 < $9 )
		{
			SETYYERROR(pyyenv, INSERT_VALUE_MORE);
			YYABORT;
		}

		for(i=0;;i++)
		{
			head = (pyyenv->pstmt->ins_heads)[i];

			if( ! head )
			{
				SETYYERROR(pyyenv, POST_WITHOUT_BODY);
				YYABORT;
			}

			if( nnsql_getcolidxbyname(head) == en_body )
				break;
		}

		if( add_ins_head(pyyenv->pstmt, 0, $5) == -1 )
			YYABORT;

		pyyenv->pstmt->table = $3;
		/* we will not check table(i.e. group)name here.
		 * According to RFC1036, it is totally legal to
		 * post an articl to a group which is not access
		 * able locally.
		 * table name here is actually newsgroups name.
		 * it should look like:
		 *	<group_name>,<group_name>,...
		 * Be aware, there are must no space between the
		 * ',' and the two <group_name>s. Also, group
		 * names are case sensitive.
		 */
	  }
	;

ins_head_list
	: ins_head
	  {
		if( ($$ = add_ins_head(pyyenv->pstmt, $1, 0)) == -1)
			YYABORT;
	  }
	| ins_head_list ',' ins_head
	  {
		if( ($$ = add_ins_head(pyyenv->pstmt, $3, $1))== -1)
			YYABORT;
	  }
	;

ins_head
	: NAME
	  { $$ = get_unpacked_attrname(pyyenv->pstmt, $1); }
	| kwd_column '(' QSTRING ')'
	  { $$ = get_unpacked_attrname(pyyenv->pstmt, $3); }
	| '{' kwd_fn kwd_column '(' QSTRING ')' '}'
	  { $$ = $5; }
	;

ins_value_list
	: ins_value
	  {
		if( ($$ = add_ins_value(pyyenv->pstmt, $1, 0))== -1)
			YYABORT;
	  }
	| ins_value_list ',' ins_value
	  {
		if( ($$ = add_ins_value(pyyenv->pstmt, $3, $1))==-1)
			YYABORT;
	  }
	;

ins_value
	: kwd_null
	  {
		$$.type = en_nt_null;
	  }
	| QSTRING
	  {
		$$.type = en_nt_qstr;
		$$.value.qstr = $1;
	  }
	| PARAM
	  {
		$$.type = en_nt_param;
		$$.value.ipar= $1;
	  }
	;

srch_delete_stmt
	: kwd_delete kwd_from tab_name where_clause
	  {
		pyyenv->pstmt->table = $3;

		if( add_attr( pyyenv->pstmt, en_sender, 1 )
		 || add_attr( pyyenv->pstmt, en_from, 1 )
		 || add_attr( pyyenv->pstmt, en_msgid, 1 ) )
			YYABORT;
	  }
	;

posi_delete_stmt
	: kwd_delete kwd_from tab_name kwd_where kwd_current kwd_of NAME
	  {
		SETYYERROR( pyyenv, NOT_SUPPORT_POSITION_DELETE );
		YYABORT;
	  }
	;

other_stmt
	: kwd_create	{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); }
	| kwd_alter	{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); }
	| kwd_drop	{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); }
	| kwd_update	{ SETYYERROR(pyyenv, NOT_SUPPORT_UPDATE); }
	| '{' kwd_call	{ SETYYERROR(pyyenv, NOT_SUPPORT_SPROC); }
	| '{' PARAM '=' kwd_call
			{ SETYYERROR(pyyenv, NOT_SUPPORT_SPROC); }
	| kwd_grant	{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); }
	| kwd_revoke	{ SETYYERROR(pyyenv, NOT_SUPPORT_DDL_DCL); }
	;
%%

static void	unpack_col_name(char* schema_tab_column_name, column_name_t* ptr)
{
	int	len, i;
	char	c;

	len = STRLEN(schema_tab_column_name);

	for(i=len; i; i--)
	{
		if( schema_tab_column_name[i-1] == '.' )
		{
			schema_tab_column_name[i-1] = 0;
			break;
		}
	}

	ptr->column_name = ( schema_tab_column_name + i );

	if( i )
		ptr->schema_tab_name = schema_tab_column_name;
	else
		ptr->schema_tab_name = schema_tab_column_name + len;
}

static char*	get_unpacked_attrname(yystmt_t* pstmt, char* name)
{
	column_name_t	attrname;

	unpack_col_name( name, &attrname );

	return attrname.column_name;
}

#define FILTER_CHUNK_SIZE	16

static	void*	add_node(yystmt_t* pstmt, node_t* node)
{
	int		i;
	node_t* 	srchtree;

	if (!pstmt->node_buf)
	{
		pstmt->node_buf = (node_t*)MEM_ALLOC(
			sizeof(node_t)*FILTER_CHUNK_SIZE);

		if( ! pstmt->node_buf )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		pstmt->srchtreesize = FILTER_CHUNK_SIZE;
		pstmt->srchtreenum  = 0;
	}

	if( pstmt->srchtreenum == pstmt->srchtreesize )
	{
		pstmt->node_buf = (node_t*)MEM_REALLOC(
			pstmt->node_buf,
			sizeof(node_t)*(pstmt->srchtreesize + FILTER_CHUNK_SIZE));

		if( ! pstmt->node_buf )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		pstmt->srchtreesize += FILTER_CHUNK_SIZE;
	}

	srchtree = pstmt->node_buf;

	srchtree[pstmt->srchtreenum] = *node;
	pstmt->srchtreenum ++;

	for(i=pstmt->srchtreenum;i<pstmt->srchtreesize;i++)
	{
		srchtree[i].left = EMPTY_PTR;
		srchtree[i].right= EMPTY_PTR;
	}

	return (void*)((long)(pstmt->srchtreenum - 1));
}

static	void	srchtree_reloc(node_t* buf, int num)
/* The 'where' srchtree is build on a re-allocateable array with addnode(). The purpose
 * of using an array rather than a link list is to easy the job of freeing it. Thus,
 * the left and right pointer of a node point is an offset to the array address not
 * the virtual address of the subtree. However, an evaluate function would be easy to
 * work by using virtual address of subtree rather than an offset. Because, in the first
 * case, a recrusive evaluate function only need to pass virtual address of (sub)tree
 * without worry about the offset of subtree. The purpose of this function, srchtree_reloc(),
 * is to convert all subnodes' offset into virtual address. It will called only once
 * after the whole tree has been built.
 */
{
	int	i;
	long offset;
	node_t* ptr = buf;

	for(i=0; ptr && i<num; ptr++, i++)
	{
		if( ptr->left == EMPTY_PTR )
			ptr->left = 0;
		else
		{
			offset = (long)(ptr->left);
			ptr->left = buf + offset;
		}

		if( ptr->right== EMPTY_PTR )
			ptr->right= 0;
		else
		{
			offset = (long)(ptr->right);
			ptr->right= buf+ offset;
		}
	}
}

static	int	add_attr(yystmt_t* pstmt, int idx, int wstat)
{
	if( ! pstmt->pattr )
	{
		pstmt->pattr = (yyattr_t*)MEM_ALLOC((en_body+1)*sizeof(yyattr_t));

		if( ! pstmt->pattr )
		{
			pstmt->errcode = -1;
			return -1;
		}

		MEM_SET(pstmt->pattr, 0, (en_body+1)*sizeof(yyattr_t));
	}

	(pstmt->pattr)[0].stat	= 1;
	(pstmt->pattr)[0].wstat = 1;
	(pstmt->pattr)[0].article = 0;
	(pstmt->pattr)[0].nntp_hand = 0;

	(pstmt->pattr)[idx].stat = 1;
	(pstmt->pattr)[idx].wstat |= wstat;

	return 0;
}

static void*	add_all_attr(yystmt_t* pstmt)
{
	int	i;
	yycol_t col;

	for(i=en_article_num + 1; i < en_body + 1; i++)
	{
		col.iattr = i;
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, i, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static const int	news_attr[] = {
	en_subject,
	en_date,
	en_from,
	en_organization,
	en_msgid,
	en_body
};

static void*	add_news_attr(yystmt_t* pstmt)
{
	int	i, n;
	yycol_t col;

	n = sizeof(news_attr)/sizeof(news_attr[0]);

	for(i=0; i<n ;i++)
	{
		col.iattr = news_attr[i];
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, col.iattr, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static const int	xnews_attr[] = {
	en_newsgroups,
	en_subject,
	en_date,
	en_from,
	en_organization,
	en_sender,
	en_msgid,
	en_summary,
	en_keywords,
	en_host,
	en_x_newsreader,
	en_body
};

static void*	add_xnews_attr(yystmt_t*	pstmt)
{
	int	i, n;
	yycol_t col;

	n = sizeof(xnews_attr)/sizeof(xnews_attr[0]);

	for(i=0; i<n ;i++)
	{
		col.iattr = xnews_attr[i];
		col.table = 0;

		if( add_column(pstmt, &col)
		 || add_attr  (pstmt, col.iattr, 0) )
			return ERROR_PTR;
	}

	return 0;
}

static void* add_column(yystmt_t* pstmt, yycol_t* column)
{
	yycol_t*	pcol;

	if( ! pstmt->pcol )
	{
		pstmt->pcol = (yycol_t*)MEM_ALLOC((MAX_COLUMN_NUMBER + 1)*sizeof(yycol_t));

		if( ! pstmt->pcol )
		{
			pstmt->errcode = -1;
			return ERROR_PTR;
		}

		MEM_SET( pstmt->pcol, 0, (MAX_COLUMN_NUMBER + 1)*sizeof(yycol_t));
	}

	if( ! pstmt->ncol )
	{
		pstmt->ncol = 1;
		pstmt->pcol->iattr = en_article_num;
		pstmt->pcol->table = 0;
	}

	if( pstmt->ncol > MAX_COLUMN_NUMBER + 1)
	{
		pstmt->errcode = TOO_MANY_COLUMNS;
		return ERROR_PTR;
	}

	pcol = pstmt->pcol + pstmt->ncol;
	pstmt->ncol++;

	*pcol = *column;

	return 0;
}

static void nnsql_yyerror(yyenv_t* pyyenv, char* msg)
{
	yystmt_t*	pstmt = pyyenv->pstmt;

	pstmt->errcode = PARSER_ERROR;
	pstmt->errpos  = pyyenv->errpos;

	sprintf(pstmt->msgbuf, NNSQL_ERRHEAD "%s", msg);
}

static	int	table_check(yystmt_t* pstmt)
{
	int	i;
	char	*table, *table1;

	table = pstmt->table;

	if( ! (table && *table) )
		return 0;

	for(i=1;pstmt->pcol && i<pstmt->ncol;i++)
	{
		table1 = (pstmt->pcol)[i].table;

		if( table1 && *table1 && !STREQ( table, table1 ) )
			return 0;
	}

	return 1;
}

static void*	attr_name(yystmt_t* pstmt, char* name)
{
	node_t	node;
	column_name_t	attrname;
	int		attridx;
	void*		offset;

	unpack_col_name( name, &attrname );

	attridx = nnsql_getcolidxbyname( attrname.column_name );
	if( attridx == -1 )
	{
		pstmt->errcode = INVALID_COLUMN_NAME;
		return ERROR_PTR;
	}

	if( attridx == en_body )
	{
		pstmt->errcode = UNSEARCHABLE_ATTR;
		return ERROR_PTR;
	}

	node.type = en_nt_attr;
	node.value.iattr =  attridx;
	node.left = EMPTY_PTR;
	node.right= EMPTY_PTR;

	if( (offset = add_node(pstmt, &node)) == ERROR_PTR )
		return ERROR_PTR;

	if( add_attr(pstmt, attridx, 1) )
		return ERROR_PTR;

	return offset;
}

static int	column_name(yystmt_t* pstmt, char* name)
{
	column_name_t	colname;
	int		colidx;
	yycol_t 	col;

	unpack_col_name( name, &colname );

	colidx = nnsql_getcolidxbyname( colname.column_name );
	if( colidx == -1 )
	{
		pstmt->errcode = INVALID_COLUMN_NAME;
		return -1;
	}

	col.iattr = colidx;
	col.table = colname.schema_tab_name;

	if( add_column(pstmt, &col)
	 || add_attr(pstmt, colidx, 0) )
		return -1;

	return 0;
}

static int	add_ins_head( yystmt_t* pstmt, char* head, int idx)
{
	if( !idx )
	{
		MEM_FREE(pstmt->ins_heads);
		pstmt->ins_heads = (char**)MEM_ALLOC( FILTER_CHUNK_SIZE * sizeof(char*));
	}
	else if( ! idx%FILTER_CHUNK_SIZE )
	{
		pstmt->ins_heads = (char**)MEM_REALLOC( pstmt->ins_heads,
			(idx+FILTER_CHUNK_SIZE)*sizeof(char*));
	}

	if( ! pstmt->ins_heads )
		return -1;

	(pstmt->ins_heads)[idx] = head;

	return idx + 1;
}

static int	add_ins_value( yystmt_t* pstmt, node_t node, int idx)
{
	if( !idx )
	{
		MEM_FREE(pstmt->ins_values)
		pstmt->ins_values = (node_t*)MEM_ALLOC( FILTER_CHUNK_SIZE * sizeof(node_t));
	}
	else if( ! idx%FILTER_CHUNK_SIZE )
	{
		pstmt->ins_values = (node_t*)MEM_REALLOC( pstmt->ins_values,
			(idx+FILTER_CHUNK_SIZE)*sizeof(node_t));
	}

	if( ! pstmt->ins_values )
		return -1;

	(pstmt->ins_values)[idx] = node;

	return idx + 1;
}
