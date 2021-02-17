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
#include "driver.h"

static char	sccsid[]
	= "@(#)NNSQL(NetNews SQL) v0.5, Copyright(c) 1995, 1996 by Ke Jin";

static int	yyunbindpar(yystmt_t* yystmt, int ipar);

void*	nnsql_allocyystmt(void* hcndes)
{
	yystmt_t*	yystmt;

	yystmt = (yystmt_t*)MEM_ALLOC(sizeof(yystmt_t));

	if( ! yystmt )
		return 0;

	MEM_SET(yystmt, 0, sizeof(yystmt_t));

	yystmt->hcndes = hcndes;

	return yystmt;
}

void	nnsql_close_cursor(void* hstmt)
{
	yystmt_t*	yystmt = hstmt;
	yyattr_t*	pattr;
	int		i;

	if( !hstmt )
		return;

	for(pattr = yystmt->pattr, i=0; pattr && i<=en_body; pattr++, i++)
	{
		pattr->stat = 0; /* not needed */
		pattr->wstat= 0;
		nntp_closeheader(pattr->nntp_hand);
		pattr->nntp_hand = 0;
	}
}

void	nnsql_dropyystmt(void* hstmt)
{
	yystmt_t*	yystmt = hstmt;
	int		i;

	if(! hstmt )
		return;

	/* allocated for any statement
	 */
	MEM_FREE( yystmt->texts_buf );
	MEM_FREE( yystmt->sqlexpr );

	/* allocated for SELECT statement
	 */
	MEM_FREE( yystmt->node_buf );
	MEM_FREE( yystmt->pcol );

	nnsql_close_cursor(hstmt);

	if( yystmt->pattr )
	{
		MEM_FREE( (yystmt->pattr)[en_body].value.location );
		MEM_FREE( yystmt->pattr );
	}

	/* allocated for yybindpar(). i.e. nnsql_put{date, str, num}()
	 */
	for(i=1;;i++)
	{
		if( yyunbindpar(yystmt, i) )
			break;
	}
	MEM_FREE( yystmt->ppar );

	/* allocated for INSERT statement
	 */
	MEM_FREE( yystmt->ins_heads );
	MEM_FREE( yystmt->ins_values);

	/* the statement object itself
	 */
	MEM_FREE( hstmt );
}

static	void	nnsql_resetyystmt(void* hstmt)
{
	yystmt_t*	yystmt = hstmt;
	int		i;

	if( ! hstmt )
		return;

	yystmt->type = en_stmt_alloc;

	/* clear result from any statement
	 */
	MEM_FREE( yystmt->sqlexpr );
	MEM_FREE( yystmt->texts_buf );
	yystmt->sqlexpr = 0;
	yystmt->texts_buf = 0;

	yystmt->table	= 0;

	yystmt->ncol	= 0;
	yystmt->npar	= 0;
	yystmt->count	= 0;

	/* reset the search tree (but not free the buffer of it)
	 */
	yystmt->srchtree = 0;
	yystmt->srchtreenum = 0;

	/* clear fetched result
	 */
	nnsql_close_cursor(hstmt);

	/* clear result of bindpar()
	 */
	for(i=1; ;i++ )
	{
		if( yyunbindpar(yystmt, i) )
			break;
	}

	/* clear result of insert statement
	 */
	MEM_FREE( yystmt->ins_heads );
	MEM_FREE( yystmt->ins_values );
	yystmt->ins_heads = 0;
	yystmt->ins_values= 0;
}

static int	yyfetch(void* hstmt, int wstat)
{
	int		i, j, nattr;
	yystmt_t*	yystmt = hstmt;

	if( ! hstmt || ! yystmt->pattr )
		return -1;

	for(i=en_article_num + 1, nattr=0; i<= en_body; i++)
	{
		yyattr_t*	pattr;
		yyattr_t*	tpattr;
		void*		hrh = 0;

		pattr = yystmt->pattr + i;

		if(i == en_body )
		{
			if( nattr )
				break;

			i = en_lines;	/* use lines to retrive article number */
			pattr = yystmt->pattr;
			nattr --;
		}

		if( pattr->stat && pattr->wstat == wstat )
		{
			char*	header;
			long	data;
			int	r;

			nattr++;

			if( ! (header = nnsql_getcolnamebyidx(i)) )
				return -1;

			if( !wstat && !hrh )
			{
				for(j = 0, tpattr = yystmt->pattr;
				    j < en_body;
				    j++)
				{
					tpattr++;

					if( tpattr->wstat )
					{
						hrh = tpattr->nntp_hand;
						break;
					}
				}

				if( !hrh && yystmt->pattr->wstat )
					hrh = yystmt->pattr->nntp_hand;
			}

			if( ! pattr->nntp_hand )
			{
				nnsql_getrange(yystmt, &(yystmt->artnum_min), &(yystmt->artnum_max));

				pattr->nntp_hand =
					nntp_openheader( yystmt->hcndes, header,
						&(yystmt->artnum_min), &(yystmt->artnum_max) );

				if( ! pattr->nntp_hand )
					return -1;
			}

			if( yystmt->artnum_max )
				r = nntp_fetchheader( pattr->nntp_hand,
					&((yystmt->pattr->value).number), &data, hrh );
			else
				r = 100;

			if( !r && !i )
			{
				data = (yystmt->pattr->value).number;
				if(data > yystmt->artnum_max)
					r = 100;
			}

			switch(r)
			{
				case 100:
					(yystmt->pattr->value).number = 0;
				case -1:
					nntp_closeheader(pattr->nntp_hand);
					pattr->nntp_hand = 0;
					return r;

				case 0:
					break;

				default:
					abort();
					break;
			}

			switch(i)
			{
				case en_lines:
					if( nattr )
						(pattr->value).number = (long)(data);
					else
						return 0;
					break;

				case en_date:
					nnsql_nndatestr2date((char*)data,
						&((pattr->value).date) );
					break;

				default:
					(pattr->value).location = (char*)(data);
					break;
			}
		}
	}

	return 0;
}

#include	<stdio.h>

int	nnsql_fetch(void* hstmt)
{
	int		r, i;
	yystmt_t*	pstmt = hstmt;
	yyattr_t*	pattr = pstmt->pattr + en_body;

	for(;;)
	{
		switch( pstmt->type )
		{
			case en_stmt_fetch_count:
				pstmt->type = en_stmt_alloc;
				return 100;

			case en_stmt_select:
				break;

			default:
				return -1;
		}

		r = yyfetch(hstmt, 1);

		switch(r)
		{
			case 100:
				for(i=1; i<pstmt->ncol; i++)
				{
					if( (pstmt->pcol + i)->iattr == en_sql_count )
					{
						pstmt->type = en_stmt_fetch_count;
						return 0;
					}
				}
				pstmt->type = en_stmt_alloc;
				return 100;

			case -1:
				pstmt->type = en_stmt_alloc;
				return -1;

			case 0:
				r = nnsql_srchtree_evl(hstmt);

				switch(r)
				{
					case -1:
						pstmt->type = en_stmt_alloc;
						return r;

					case 1:
						pstmt->count++;

						if( pstmt->ncol == 2
						 && (pstmt->pcol + 1)->iattr == en_sql_count )
							continue;

						if( (r = yyfetch(hstmt, 0)) == -1 )
						{
							pstmt->type = en_stmt_alloc;
							return -1;
						}

						if( pattr->stat )
						/* pattr has already init to point
						 * to the en_body attr
						 */
						{
							MEM_FREE( (pattr->value).location );

							(pattr->value).location =
								nntp_body( pstmt->hcndes,
								(pstmt->pattr->value).number, 0);
						}
						return 0;

					case 0:
						continue;

					default:
						abort();
						break;
				}
				break;

			default:
				abort();
				break;
		}

		break;
	}

	abort();

	return -1;
}

int	nnsql_opentable(void* hstmt, char* table)
{
	yystmt_t*	yystmt = hstmt;

	if(! hstmt )
		return -1;

	if( !table )
		table = yystmt->table;

	return nntp_group( yystmt->hcndes, table );
}

int	nnsql_yyunbindpar(void* yystmt, int ipar)
{
	return yyunbindpar(yystmt, ipar);
}

static	int	yyunbindpar(yystmt_t* yystmt, int ipar)
{
	int		i;
	yypar_t*	par;

	if( yystmt
	 || ipar <= 0
	 || ipar > MAX_PARAM_NUMBER
	 || yystmt->ppar )
		return -1;

	par = yystmt->ppar + ipar - 1;

	switch( par->type )
	{
		case -1:
		case en_nt_num:
		case en_nt_null:
			break;

		case en_nt_qstr:
			MEM_FREE( par->value.location );
			break;

		default:
			abort();
			break;
	}

	yystmt->ppar->type = -1;
	return 0;
}

static int	yybindpar(yystmt_t* yystmt, int ipar, long data, int type)
{
	int	i;

	ipar --;

	if( ! yystmt->ppar )
	{
		yystmt->ppar = (yypar_t*)MEM_ALLOC(
			sizeof(yypar_t)*MAX_PARAM_NUMBER);

		if( ! yystmt->ppar )
		{
			yystmt->errcode = -1;
			return -1;
		}

		for(i=0; i<MAX_PARAM_NUMBER; i++)
			(yystmt->ppar)[i].type = -1; /* unbind */
	}

	yyunbindpar( yystmt, ipar + 1);

	(yystmt->ppar)[ipar].type = type;

	switch(type)
	{
		case en_nt_null:
			return 0;

		case en_nt_qstr:
			(yystmt->ppar)[ipar].value.location = (char*)data;
			break;

		case en_nt_num:
			(yystmt->ppar)[ipar].value.number = (long)data;
			break;

		case en_nt_date:
			(yystmt->ppar)[ipar].value.date = *((date_t*)data);
			break;

		default:
			abort();
			break;
	}

	return 0;
}

int	nnsql_putnull(void* hstmt, int ipar )
{
	return yybindpar((yystmt_t*)hstmt, ipar, 0, en_nt_null);
}

int	nnsql_putstr(void* hstmt, int ipar, char* str)
{
	return yybindpar((yystmt_t*)hstmt, ipar, (long)str, en_nt_qstr);
}

int	nnsql_putnum(void* hstmt, int ipar, long num)
{
	return yybindpar((yystmt_t*)hstmt, ipar, num, en_nt_num);
}

int	nnsql_putdate(void* hstmt, int ipar, date_t* date)
{
	return yybindpar((yystmt_t*)hstmt, ipar, (long)(date), en_nt_date);
}

int	nnsql_max_param()
{
	return MAX_PARAM_NUMBER;
}

int	nnsql_max_column()
{
	return MAX_COLUMN_NUMBER;
}

static int	access_mode_chk( yystmt_t* pstmt )
{
	int	mode;

	pstmt->errcode = 0;
	mode = nntp_getaccmode(pstmt->hcndes);

	switch( pstmt->type )
	{
		case en_stmt_insert:
			if( mode < ACCESS_MODE_INSERT )
				pstmt->errcode = NO_INSERT_PRIVILEGE;
			break;

		case en_stmt_srch_delete:
			if( nnsql_strlike( pstmt->table, "%.test", 0, 0) )
			{
				if( mode < ACCESS_MODE_DELETE_TEST )
					pstmt->errcode = NO_DELETE_PRIVILEGE;
			}
			else
			{
				if( mode < ACCESS_MODE_DELETE_ANY )
					pstmt->errcode = NO_DELETE_ANY_PRIVILEGE;
			}
			if( nnsql_opentable(pstmt, 0) )
				return -1;
			break;

		case en_stmt_select:
			if( nnsql_opentable(pstmt, 0) )
				return -1;
			return 0;

		default:
			pstmt->errcode = -1;
			break;
	}

	if( ! pstmt->errcode && ! nntp_postok( pstmt->hcndes ) )
		pstmt->errcode = NO_POST_PRIVILEGE;

	if( pstmt->errcode )
	{
		nnsql_resetyystmt(pstmt);
		return -1;
	}

	return 0;
}

int	nnsql_prepare(void* hstmt, char* expr, int len)
{
	yyenv_t 	yyenv;
	yystmt_t*	pstmt = hstmt;
	int		r;

	if( ! hstmt || ! expr || len < 0 )
		return -1;

	nnsql_resetyystmt(hstmt);

	pstmt->errcode = -1;

	pstmt->sqlexpr = MEM_ALLOC(len + 1);

	if( ! pstmt->sqlexpr )
		return -1;

	pstmt->texts_buf = MEM_ALLOC( len + 1 );

	if( ! pstmt->texts_buf )
	{
		MEM_FREE( pstmt->sqlexpr );
		pstmt->sqlexpr = 0;

		return -1;
	}

	STRNCPY(pstmt->sqlexpr, expr, len);
	(pstmt->sqlexpr)[len] = 0;

	nnsql_yyinit(&yyenv, pstmt);

	if( nnsql_yyparse(&yyenv)
	 || access_mode_chk( pstmt ) )
	{
		nnsql_resetyystmt(pstmt);
		return -1;
	}

	return 0;
}

int	nnsql_column_descid(void* hstmt, int icol)
{
	if( icol < 0 || icol > MAX_COLUMN_NUMBER )
		return -1;

	return (((yystmt_t*)hstmt)->pcol)[icol].iattr;
}

static int	do_insert(void* hstmt)
{
	yystmt_t*	pstmt = hstmt;
	char		*body, *head, *value;
	node_t* 	node;
	int		i, attridx;
	yypar_t*	par;
	int		subj_set = 0, from_set = 0;

	pstmt->count = 0;

	if( nntp_start_post( pstmt->hcndes)
	 || nntp_send_head( pstmt->hcndes,
		"X-Newsreader", "NetNews SQL Agent v0.5")
	 || nntp_send_head( pstmt->hcndes,
		"Newsgroups", pstmt->table ) )
		return -1;

	for(i=0; ;i++)
	{
		head = (pstmt->ins_heads)[i];

		if( ! head )
			break;

		if( ! *head )
			continue;

		attridx = nnsql_getcolidxbyname( head );

		switch( attridx )
		{
			case en_article_num:
			case en_x_newsreader:
			case en_newsgroups:
			case en_xref:
			case en_host:
			case en_date:
			case en_path:
			case en_lines:
			case en_msgid:	/* better to let srv set id */
				continue;

			case en_subject:
				subj_set = 1;
				break;

			case en_from:
				from_set = 1;
				break;

			case -1:	/* an extended header */
				break;

			default:	/* any normal header  */
				head = nnsql_getcolnamebyidx(attridx);
				/* use stand representation */
				break;
		}

		node = pstmt->ins_values + i;

		switch(node->type)
		{
			case en_nt_qstr:
				value = node->value.qstr;
				break;

			case en_nt_null:
				continue;

			case en_nt_param:
				par = pstmt->ppar + (node->value).ipar - 1;

				if(par->type != en_nt_qstr )
					continue;

				value = (par->value).location;
				break;

			default:
				continue;
		}

		if( attridx == en_body )
		{
			body = value;
			continue;
		}

		nntp_send_head(pstmt->hcndes, head, value);
	}

	if( !subj_set )
		nntp_send_head(pstmt->hcndes, "Subject", "(none)");

	if( !from_set )
		nntp_send_head(pstmt->hcndes, "From", "(none)");

	if( nntp_end_head (pstmt->hcndes)
	 || nntp_send_body(pstmt->hcndes, body)
	 || nntp_end_post (pstmt->hcndes) )
		return -1;

	pstmt->count = 1;

	return 0;
}

int	do_srch_delete(void* hstmt)
{
	int		r, i, rcnt;
	yystmt_t*	pstmt = hstmt;
	yyattr_t*	pattr = pstmt->pattr;

	pstmt->count = 0;

	for(;;)
	{
		r = yyfetch(hstmt, 1);

		switch(r)
		{
			case 100:
				pstmt->type = en_stmt_alloc;
				return 0;

			case -1:
				pstmt->type = en_stmt_alloc;
				return -1;

			case 0:
				r = nnsql_srchtree_evl(hstmt);

				switch(r)
				{
					case -1:
						pstmt->type = en_stmt_alloc;
						return -1;

					case 1:
						for(rcnt=0; r && rcnt<6; rcnt++)
						/* retry 6 times */
						{
							if(rcnt && pstmt->count )
								sleep(1 + rcnt);
							/* give server time to
							 * finish previous DELETE
							 */

							r = nntp_cancel(
								pstmt->hcndes,
								pstmt->table,
								pattr[en_sender].value.location,
								pattr[en_from].value.location,
								pattr[en_msgid].value.location);
						}

						if( r )
							return -1;

						pstmt->count++;
						continue;

					case 0:
						continue;

					default:
						abort();
						break;
				}
				break;

			default:
				abort();
				break;
		}

		break;
	}

	abort();

	return -1;
}

int	nnsql_execute(void* hstmt)
{
	yystmt_t*	pstmt = hstmt;
	yypar_t*	par;
	int		i;

	par = pstmt->ppar;

	if( !par && pstmt->npar )
		return 99;

	for(i=0;i<pstmt->npar;i++)
	{
		par = pstmt->ppar + i;

		if(par->type == -1)
			return 99;
	}

	switch(pstmt->type)
	{
		case en_stmt_insert:
			return do_insert(hstmt);

		case en_stmt_srch_delete:
		case en_stmt_select:
			if( nnsql_srchtree_tchk(hstmt)
			 || nnsql_opentable(hstmt, 0) )
				break;

			if( pstmt->type == en_stmt_srch_delete )
				return do_srch_delete(hstmt);

			return 0;

		default:
			break;
	}

	return -1;
}

int	nnsql_isnullablecol( void* hstmt, int icol )
{
	switch( (((yystmt_t*)hstmt)->pcol + icol)->iattr )
	{
		case en_subject:
		case en_from:
		case en_body:
			return 0;

		default:
			break;
	}

	return 1;
}

int	nnsql_isnumcol(void* hstmt, int icol )
{
	switch( (((yystmt_t*)hstmt)->pcol + icol)->iattr )
	{
		case en_lines:
		case en_article_num:
		case en_sql_count:
		case en_sql_num:
			return 1;

		default:
			break;
	}

	return 0;
}

int	nnsql_isdatecol(void* hstmt, int icol)
{
	switch( (((yystmt_t*)hstmt)->pcol + icol)->iattr )
	{
		case en_date:
		case en_sql_date:
			return 1;

		default:
			break;
	}

	return 0;
}

long	nnsql_getnum(void* hstmt, int icol )
{
	yystmt_t*	pstmt = hstmt;
	yycol_t*	pcol;

	pcol = ((yystmt_t*)hstmt)->pcol + icol;

	switch( pcol->iattr )
	{
		case en_lines:
		case en_article_num:
			return ((pstmt->pattr + pcol->iattr)->value).number;

		case en_sql_num:
			return (pcol->value).num;

		case en_sql_count:
			return (pstmt->count);

		default:
			break;
	}

	return 0;
}

int	nnsql_isstrcol(void* hstmt, int icol )
{
	return ! nnsql_isnumcol(hstmt, icol) && ! nnsql_isdatecol(hstmt, icol);
}

char*	nnsql_getstr(void* hstmt, int icol )
{
	yystmt_t*	pstmt = hstmt;
	yycol_t*	pcol;

	pcol = ((yystmt_t*)hstmt)->pcol + icol;

	switch( pcol->iattr )
	{
		case en_lines:
		case en_article_num:
		case en_sql_count:
		case en_sql_num:
			return 0;

		case en_sql_qstr:
			return (pcol->value).qstr;

		default:
			break;
	}

	return ((pstmt->pattr + pcol->iattr)->value).location;
}

date_t* nnsql_getdate(void* hstmt, int icol )
{
	yystmt_t*	pstmt = hstmt;
	yycol_t*	pcol;

	pcol = ((yystmt_t*)hstmt)->pcol + icol;

	switch( pcol->iattr )
	{
		case en_date:
			return &(((pstmt->pattr + pcol->iattr)->value).date);

		case en_sql_date:
			return &((pcol->value).date);

		default:
			break;
	}

	return 0;
}


int	nnsql_iscountcol(void* hstmt, int icol )
{
	return ( (((yystmt_t*)hstmt)->pcol + icol)->iattr == en_sql_count );
}

int	nnsql_isnullcol(void* hstmt, int icol )
{
	yystmt_t*	pstmt = hstmt;
	yycol_t*	pcol;
	long		artnum;
	date_t* 	date;

	artnum = (pstmt->pattr->value).number;

	pcol = ((yystmt_t*)hstmt)->pcol + icol;

	switch( pcol->iattr )
	{
		case en_sql_count:
			return !!artnum;

		case en_sql_num:
		case en_sql_qstr:
		case en_sql_date:
		case en_lines:
		case en_article_num:
			return !artnum;

		case en_date:
			date = nnsql_getdate(hstmt, icol);
			return !artnum || !date || !date->day;

		default:
			break;
	}

	return !artnum || !nnsql_getstr(hstmt, icol);
}

int	nnsql_getcolnum(void* hstmt)
{
	return ((yystmt_t*)hstmt)->ncol;
}

int	nnsql_getparnum(void* hstmt)
{
	return ((yystmt_t*)hstmt)->npar;
}

int	nnsql_getrowcount(void* hstmt)
{
	return ((yystmt_t*)hstmt)->count;
}
