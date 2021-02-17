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

#include <config.h>
#include	<nnconfig.h>
#include	<yystmt.h>
#include	<yylex.h>
#include	<nncol.h>
#include	<nndate.h>
#include	<yyerr.h>

static int	getleaftype(yystmt_t* yystmt, node_t* nd)
{
	yypar_t*	par;
	yyattr_t*	attr;

	switch( nd->type )
	{
		case en_nt_attr:
			attr = yystmt->pattr + (nd->value).iattr;

			switch((nd->value).iattr)
			{
				case en_lines:
				case en_article_num:
				case en_sql_num:
				case en_sql_count:
					return en_nt_num;

				case en_date:
				case en_sql_date:
					return en_nt_date;

				default:
					break;
			}
			return en_nt_qstr;

		case en_nt_param:
			par = yystmt->ppar + (nd->value).ipar - 1;

			switch(par->type)
			{
				case en_nt_null:
				case en_nt_num:
				case en_nt_qstr:
				case en_nt_date:
					break;

				default:
					return -1;
			}
			return par->type;

		case en_nt_num:
		case en_nt_qstr:
		case en_nt_date:
		case en_nt_null:
			return nd->type;

		default:
			return -1;
	}
}

static int	cmp_tchk(yystmt_t* yystmt, node_t* a, node_t* b)
{
	int	ta, tb;

	ta = getleaftype( yystmt, a );
	tb = getleaftype( yystmt, b );

	if( ta == -1 || tb == -1 )
		return -1;

	if( ta == en_nt_date && tb == en_nt_qstr )
		return 0;

	if( ta != tb && ta != en_nt_null && tb != en_nt_null )
		return -1;

	return 0;
}

static	int	evl_like_tchk(yystmt_t* yystmt, node_t* a, node_t* b)
{
	int	ta, tb;

	ta = getleaftype(yystmt, a);
	tb = getleaftype(yystmt, b);

	if( tb == en_nt_null
	 || tb == en_nt_null )
		return 0;

	if( ta != en_nt_qstr
	 || tb != en_nt_qstr )
		return -1;

	return 0;
}

static int srchtree_tchk(yystmt_t* yystmt, node_t* node)
/* return -1: err or syserr, 0: ok */
{
	int	r, s, flag = 0;
	node_t* ptr;

	if( ! node )
		return 0;

	switch( node->type )
	{
		case en_nt_cmpop:
			return cmp_tchk(yystmt, node->left, node->right);

		case en_nt_logop:
			switch( (node->value).logop )
			{
				case en_logop_not:
					return srchtree_tchk(yystmt, node->right);

				case en_logop_and:
				case en_logop_or:
					r = srchtree_tchk(yystmt, node->left);
					s = srchtree_tchk(yystmt, node->right);

					if( r == -1 || s == -1 )
						return -1;

					return 0;

				default:
					abort();
					break;	/* just  for turn off the warning */
			}
			break;	/* just for turn off the warning */

		case en_nt_isnull:
			return 0;

		case en_nt_between:
			r = cmp_tchk(yystmt, node->left, node->right->left);
			s = cmp_tchk(yystmt, node->left, node->right->right);

			if( r == -1 || s == -1 )
				return -1;

			return 0;

		case en_nt_in:
			for(ptr=node->right; ptr; ptr=ptr->right)
			{
				r = cmp_tchk(yystmt, node->left, ptr);

				if( r == -1 )
					return -1;
			}
			return 0;

		case en_nt_caselike:
		case en_nt_like:
			return evl_like_tchk( yystmt, node->left, node->right);

		default:
			abort();
			break;	/* turn off the warning */
	}

	return -1;	/* just for turn off the warning */
}

int	nnsql_srchtree_tchk(void* hstmt)
{
	yystmt_t*	yystmt = hstmt;
	int		r;

	r = srchtree_tchk(yystmt, yystmt->srchtree);

	if( r )
		yystmt->errcode = TYPE_VIOLATION;

	return r;
}
