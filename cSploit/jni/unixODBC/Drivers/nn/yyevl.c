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

typedef struct {
	int	type;	/* can only be en_nt_qstr, en_nt_num and en_nt_null */

	union {
		char*	qstr;
		long	num;
		date_t	date;
	} value;
} leaf_t;

static int	getleaf(yystmt_t* yystmt, node_t* nd, leaf_t* lf)
{
	yypar_t*	par;
	yyattr_t*	attr;

	switch( nd->type )
	{
		case en_nt_attr:
			attr = yystmt->pattr + (nd->value).iattr;

			if( (nd->value).iattr == en_lines
			 || (nd->value).iattr == en_article_num )
			{
				lf->type = en_nt_num;
				(lf->value).num = (attr->value).number;
				break;
			}

			if( (nd->value).iattr == en_date )
			{
				if( (attr->value).date.day )
				{
					lf->type = en_nt_date;
					(lf->value).date =
						(attr->value).date;
				}
				else
					lf->type = en_nt_null;
				break;
			}

			if( (attr->value).location )
			{
				lf->type = en_nt_qstr;
				(lf->value).qstr =
					(attr->value).location;
			}
			else
				lf->type = en_nt_null;
			break;

		case en_nt_param:
			par = yystmt->ppar + (nd->value).ipar - 1;

			if( par->type == en_nt_null )
			{
				lf->type = en_nt_null;
				break;
			}

			if( par->type == en_nt_num )
			{
				lf->type = en_nt_num;
				(lf->value).num = (par->value).number;
				break;
			}

			if( par->type == en_nt_qstr)
			{
				if( (par->value).location )
				{
					lf->type = en_nt_qstr;

					(lf->value).qstr =
						(par->value).location;
					break;
				}

				lf->type = en_nt_null;
				break;
			}

			if( par->type == en_nt_date)
			{
				if( (par->value).date.day )
				{
					lf->type = en_nt_date;

					(lf->value).date =
						(par->value).date;
					break;
				}

				lf->type = en_nt_null;
			}

			return -1;

		case en_nt_num:
			lf->type = en_nt_num;
			(lf->value).num = (nd->value).num;
			break;

		case en_nt_qstr:
			lf->type = en_nt_qstr;

			if( (nd->value).qstr )
			{
				lf->type = en_nt_qstr;
				(lf->value).qstr = (nd->value).qstr;
				break;
			}

			lf->type = en_nt_null;
			break;

		case en_nt_date:
			lf->type = en_nt_date;

			(lf->value).date = (nd->value).date;
			break;

		case en_nt_null:
			lf->type = en_nt_null;
			break;

		default:
			return -1;
	}

	return 0;
}

#ifndef MAX_SIGNED_LONG
# define	MAX_SIGNED_LONG ((-1UL) >> 1)
#endif

# define	AREA		( MAX_SIGNED_LONG + 1UL )
# define	RANGE_ALL	{ 0, 1UL, MAX_SIGNED_LONG }
# define	RANGE_EMPTY	{ 1,  0UL, 0UL }
# define	ALL_RANGE(r)	{ r.flag = 0; \
				  r.min = 1UL; \
				  r.max = MAX_SIGNED_LONG; }
# define	EMPTY_RANGE(r)	{ r.flag = 1;\
				  r.min = r.max = 0UL; }
# define	IS_EMPTY_RANGE(r) \
				(((r.min == 0UL) && (r.max==0UL))? 1:0)
# define	RANGE_MIN(a, b) ((((unsigned long)(a)) < ((unsigned long)(b)))? a:b)
# define	RANGE_MAX(a, b) ((((unsigned long)(a)) > ((unsigned long)(b)))? a:b)

typedef struct {
	int		flag;
	unsigned long	min;
	unsigned long	max;
} range_t;

static	range_t range_and(range_t r1, range_t r2)
{
	range_t r = RANGE_EMPTY;

	if( !(r.flag = r1.flag || r2.flag) )
		return r;

	if( r1.min > r2.max
	 || r1.max < r2.min )
		return r;

	r.min = (RANGE_MAX(r1.min, r2.min))%AREA;
	r.max = (RANGE_MIN(r1.max, r2.max))%AREA;

	return r;
}

static	range_t range_or(range_t r1, range_t r2)
{
	range_t r = RANGE_ALL;

	if( !(r.flag = r1.flag || r2.flag) )
		return r;

	if( !(r1.max/AREA) || !(r2.max/AREA) )
	/* at least one of the rangion does not cross cell */
	{
		unsigned long	c1, c2;

		c1 = (r1.max/2) + (r1.min/2);	/* central of rangion 1 */
		c2 = (r2.max/2) + (r2.min/2);	/* central of rangion 2 */

		if( c1 > c2 && (c1 - c2) > (AREA/2) )
		{
			/* shift it to the second cell */
			r2.min += AREA;
			r2.max += AREA;
		}
		else
		if( c2 > c1 && (c2 - c1) > (AREA/2) )
		{
			/* shift it to the second cell */
			r1.min += AREA;
			r1.max += AREA;
		}
	}

	r.min = (RANGE_MIN(r1.min, r2.min))%AREA;
	r.max = (RANGE_MAX(r1.max, r2.max))%AREA;

	return r;
}

static	range_t getrange(yystmt_t* yystmt, node_t* pnd)
{
	range_t r = RANGE_ALL, r1, r2;
	node_t* tpnd = 0;
	leaf_t	a, b;
	int	flag = 0;

	if( !pnd )
		return r;

	if( pnd->left
	 && pnd->left->type == en_nt_attr
	 && (pnd->left->value).iattr == en_article_num )
	{
		r.flag = 1;

		switch( pnd->type )
		{
			case en_nt_between:
				getleaf(yystmt, pnd->right->left,  &a);
				getleaf(yystmt, pnd->right->right, &b);

				if( a.type == en_nt_null
				 || b.type == en_nt_null )
					break;

				r.min = RANGE_MIN(a.value.num, b.value.num);
				r.max = RANGE_MAX(a.value.num, b.value.num);
				break;

			case en_nt_in:
				EMPTY_RANGE(r);

				for(tpnd = pnd->right; tpnd; tpnd=tpnd->right)
				{
					getleaf(yystmt, tpnd, &a);

					if( a.type == en_nt_null )
						continue;

					if( IS_EMPTY_RANGE(r) )
						r.min = r.max = a.value.num;
					else
					{
						r.min = RANGE_MIN(r.min, a.value.num);
						r.max = RANGE_MAX(r.max, a.value.num);
					}
				}
				break;

			case en_nt_cmpop:
				getleaf(yystmt, pnd->right, &a);

				if( a.type == en_nt_null )
					break;

				switch((pnd->value).cmpop)
				{
					case en_cmpop_eq:
						r.min = r.max = a.value.num;
						break;

					case en_cmpop_ne:
						r.min = (a.value.num + 1UL )%AREA;
						r.max = (a.value.num - 1UL + AREA)%AREA;
						break;

					case en_cmpop_gt:
						r.min = (a.value.num + 1UL)%AREA;
						break;

					case en_cmpop_lt:
						r.max = (a.value.num - 1UL + AREA)%AREA;
						r.min = (r.max)? 1UL:0UL;
						break;

					case en_cmpop_ge:
						r.min = a.value.num;
						break;

					case en_cmpop_le:
						r.max = a.value.num;
						r.min = (r.max)? 1UL:0UL;
						break;

					default:
						EMPTY_RANGE(r);
						break;
				}
				break;

			default:
				break;
		}

		return r;
	}

	if( pnd->type != en_nt_logop )
		return r;

	switch( (pnd->value).logop )
	{
		case en_logop_not:
			r1 = getrange(yystmt, pnd->right);
			if( r1.flag )
			{
				r.min = (r1.max + 1UL)%AREA;
				r.max = (r1.min - 1UL + AREA)%AREA;
			}
			break;

		case en_logop_or:
			flag = 1;
		case en_logop_and:
			r1 = getrange(yystmt, pnd->left);
			r2 = getrange(yystmt, pnd->right);

			if( !(r1.flag || r2.flag ) )
				break;

			if( r1.min > r1.max )
				r1.max = r1.max + AREA;

			if( r2.min > r2.max )
				r2.max = r2.max + AREA;

			if( flag )
				r = range_or ( r1, r2 );
			else
				r = range_and( r1, r2 );
			break;

		default:
			EMPTY_RANGE(r);
			break;
	}

	return r;
}

void	nnsql_getrange(void* hstmt, long* pmin, long* pmax)
{
	yystmt_t*	yystmt = hstmt;
	range_t 	r;

	r = getrange(hstmt, yystmt->srchtree);

	if( !r.flag )
	{
		*pmin = 1UL;
		*pmax = MAX_SIGNED_LONG;
	}
	else
	{
		*pmin = r.min;
		*pmax = r.max;
	}
}

static int	is_sql_null(yystmt_t* yystmt, node_t* a)
{
	leaf_t	lf;

	if( getleaf(yystmt, a, &lf) )
		return -1;

	return (lf.type == en_nt_null);
}

static int	compare(yystmt_t* yystmt, node_t* a, node_t* b, int op)
{
	leaf_t	la, lb;
	int	diff, r;

	if( getleaf( yystmt, a, &la )
	 || getleaf( yystmt, b, &lb ) )
		return -1;

	if( la.type == en_nt_date && lb.type == en_nt_qstr )
	{
		lb.type = en_nt_date;

		if( nnsql_odbcdatestr2date(lb.value.qstr, &(lb.value.date)) )
			return -1;
	}

	if( la.type != lb.type
	 || la.type == en_nt_null
	 || lb.type == en_nt_null )
		return 0;

	switch( la.type )
	{
		case en_nt_qstr:
			diff = STRCMP(la.value.qstr, lb.value.qstr);
			break;

		case en_nt_num:
			diff = la.value.num - lb.value.num;
			break;

		case en_nt_date:
			diff = nnsql_datecmp(&(la.value.date), &(lb.value.date));
			break;

		default:
			abort();
			return -1;
	}

	switch(op)
	{
		case en_cmpop_eq:
			r = !diff;
			break;

		case en_cmpop_ne:
			r = !!diff;
			break;

		case en_cmpop_gt:
			r = (diff>0)?  1:0;
			break;

		case en_cmpop_ge:
			r = (diff>=0)? 1:0;
			break;

		case en_cmpop_lt:
			r = (diff<0)?  1:0;
			break;

		case en_cmpop_le:
			r = (diff<=0)? 1:0;
			break;

		default:
			r = -1;
			break;
	}

	return r;
}

static	int	ch_case_cmp(char a, char b)
{
	if( a >= 'a' && a <= 'z' )
		a += ('A' - 'a');

	if( b >= 'a' && b <= 'z' )
		b += ('A' - 'a');

	return (a - b);
}

static int strlike(
		char* str,
		char* pattern,
		char esc,
		int flag )	/* flag = 0: case sensitive */
{
	char	c, cp;

	for(;;str++, pattern++)
	{
		c = *str;
		cp= *pattern;

		if( esc && cp == esc )
		{
			cp = *pattern++;

			if( (!flag && (c - cp) )
			 || ( flag && ch_case_cmp(c, cp) ) )
				return 0;

			if( !c )
				return 1;

			continue;
		}

		switch(cp)
		{
			case 0:
				return !c;

			case '_':
				if(!c)
					return 0;
				break;

			case '%':
				if(! *(pattern + 1) )
					return 1;

				for(;*str;str++)
				{
					if(strlike(str,
						pattern + 1, esc, flag))
						return 1;
				}
				return 0;

			default:
				if( (!flag && (c - cp) )
				 || ( flag && ch_case_cmp(c, cp) ) )
					return 0;
				break;
		}
	}
}

int	nnsql_strlike(char* str, char* pattern, char esc, int flag)
{
	return strlike(str, pattern, esc, flag);
}

static	int	evl_like(yystmt_t* yystmt, node_t* a, node_t* b, char esc, int flag)
{
	leaf_t	la, lb;

	if( getleaf(yystmt, a, &la )
	 || getleaf(yystmt, b, &lb ) )
		return -1;

	if( la.type != en_nt_qstr
	 || lb.type != en_nt_qstr )
		return 0;

	return strlike(la.value.qstr, lb.value.qstr, esc, flag);
}

static int srchtree_evl(yystmt_t* yystmt, node_t* node)
/* return -1: syserr, 0: fail, 1: true */
{
	int	r, s, flag = 0;
	node_t* ptr;

	if( ! node )
		return 1;

	switch( node->type )
	{
		case en_nt_cmpop:
			return compare(yystmt, node->left, node->right,
					(node->value).cmpop);

		case en_nt_logop:
			switch( (node->value).logop )
			{
				case en_logop_not:
					r = srchtree_evl(yystmt, node->right);
					if( r == -1 )
						return -1;
					return ( ! r );

				case en_logop_and:
					flag = 1;
				case en_logop_or:
					r = srchtree_evl(yystmt, node->left);
					s = srchtree_evl(yystmt, node->right);

					if( r == -1 || s == -1 )
						return -1;

					if( flag )
						return ( r && s );
					else
						return ( r || s );

				default:
					abort();
					break;	/* just  for turn off the warning */
			}
			break;	/* just for turn off the warning */

		case en_nt_isnull:
			return is_sql_null(yystmt, node->left);

		case en_nt_between:
			r = compare(yystmt, node->left, node->right->left,  en_cmpop_ge);
			s = compare(yystmt, node->left, node->right->right, en_cmpop_le);

			if( r == -1 || s == -1 )
				return -1;

			return ( r && s );

		case en_nt_in:
			for(ptr=node->right; ptr; ptr=ptr->right)
			{
				r = compare(yystmt, node->left, ptr, en_cmpop_eq);

				if( r ) /* -1 or 1 */
					return r;
			}
			return 0;

		case en_nt_caselike:
			flag = 1;
		case en_nt_like:
			return evl_like( yystmt, node->left, node->right,
				(node->value).esc, flag );

		default:
			abort();
			break;	/* turn off the warning */
	}

	return -1;	/* just for turn off the warning */
}

int	nnsql_srchtree_evl(void* hstmt)
{
	yystmt_t*	yystmt = hstmt;

	return srchtree_evl(yystmt, yystmt->srchtree);
}
