/**
    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>
	Enhanced for unixODBC (1999) by Peter Harvey <pharvey@codebydesign.com>
	
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/
#ifndef _YYSTMT_H
# define _YYSTMT_H

# define	MAX_COLUMN_NUMBER	32
# define	MAX_PARAM_NUMBER	32

# include	<yylex.h>
# include	<nndate.h>
# include	<nnsql.h>

typedef enum {
	en_nt_cmpop,	/* en_cmpop_eq, */
	en_nt_logop,	/* kwd_and, kwd_or, kwd_not */

	en_nt_attr,
	en_nt_qstr,
	en_nt_num,
	en_nt_date,
	en_nt_param,
	en_nt_null,

	en_nt_like,
	en_nt_between,
	en_nt_in,
	en_nt_caselike,
	en_nt_isnull
} node_type;

typedef struct tnode
{
	int	type;	/* compare operator,
			 * logical operator,
			 * column value,
			 * literal value,
			 * dummy parameter
			 */

	union {
		int	cmpop;	/* en_cmpop_eq, en_cmpop_ne, ... */
		int	logop;	/* kwd_or, kwd_and, kwd_not */
		int	iattr;	/* attribute index */
		int	ipar;	/* parameter idx */
		char*	qstr;
		long	num;
		date_t	date;
		char	esc;	/* escape for like pattern match */
	} value;

	struct tnode*	left;
	struct tnode*	right;
} node_t;

typedef struct {
	int		iattr;
	char*		table;		/* table name from yyparse */

	union {
		long	num;
		char*	qstr;
		date_t	date;
	} value;
} yycol_t;

typedef struct {
	int	stat;	/* 0 don't needed(either by select list
			   or by where tree) 1 needed */
	int	wstat;	/* 0 if don't need by where tree, 1 if need */
	int	article;/* article number */

	union {
		char*	location;
		long	number;
		date_t	date;
	} value;

	void*	nntp_hand;	/* create with nntp_openheader()
				 * freed  with nntp_closeheader()
				 * fetch  with nntp_fetchheader() */
} yyattr_t;

typedef struct
{
	int		type;	/* can only be en_nt_qstr,
				   en_nt_num and en_nt_null */

	union {
		char*	location;
		long	number;
		date_t	date;
	} value;
} yypar_t;

enum {
	en_stmt_alloc = 0,	/* only in allocated stat */
	en_stmt_select,
	en_stmt_insert,
	en_stmt_srch_delete,

	en_stmt_fetch_count = 100
};

typedef struct {
	void*		hcndes; 	/* nntp cndes */

	int		type;

	int		errcode;
	int		errpos;

	yycol_t*	pcol;		/* column descriptor */
	yyattr_t*	pattr;		/* fetched required column */
	yypar_t*	ppar;		/* user parameter area */

	char*		table;		/* table name from yyparse */

	int		ncol;
	int		npar;
	int		count;		/* num. of record has been fetched. */

	char*		sqlexpr;	/* point to sqlexpr in hstmt_t */
	char*		texts_buf;
	char		msgbuf[64];	/* buf to hold message string passed to
					 * nnsql_yyerror() */

	/* search tree */
	node_t* 	srchtree;		/* root of search tree */
	node_t* 	node_buf;	/* address of array */
	int		srchtreesize;	/* search tree array size */
	int		srchtreenum;	/* number of nodes in tree */

	/* insert headers and values */
	char**		ins_heads;
	node_t* 	ins_values;

	/* article-num range */
	long		artnum_min;
	long		artnum_max;
} yystmt_t;

#endif
