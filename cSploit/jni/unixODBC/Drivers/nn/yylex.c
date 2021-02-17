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

#include	<yyenv.h>
#include	<yystmt.h>
#include	<yylex.h>
#include	<yyparse.tab.h>

#include	"yylex.ci"

# define	YYERRCODE	256

#include	<stdio.h>

static int	getcmpopidxbyname(char* name)
{
	int	i, size;

	size = sizeof(cmpop_tab)/sizeof(cmpop_tab[0]);

	for(i=0; i<size; i++)
	{
		if(STREQ(name, cmpop_tab[i].name))
			return cmpop_tab[i].idx;
	}

	return YYERRCODE;
}

static int	getkeywdidxbyname(char* name)
{
	int	i, size;

	size = sizeof(keywd_tab)/sizeof(keywd_tab[0]);

	for(i=0; i<size; i++)
	{
		if(upper_strneq(name, keywd_tab[i].name, 12))
			return keywd_tab[i].idx;
	}

	return YYERRCODE;
}

static	int	getxkeywdidxbyname(char* name)
{
	int	i, size;

	size = sizeof(xkeywd_tab)/sizeof(xkeywd_tab[0]);

	for(i=0; i<size; i++)
	{
		if(upper_strneq(name, xkeywd_tab[i].name, 6))
			return xkeywd_tab[i].idx;
	}

	return YYERRCODE;
}

static char	popc(yyenv_t* penv)
{
	char c;

	c = (penv->pstmt->sqlexpr)[(penv->scanpos)];
	(penv->errpos) = (penv->scanpos);
	(penv->scanpos) ++;

	return c;
}

static	void	unputc(char c, yyenv_t* penv)
{
	(penv->errpos) = (--(penv->scanpos));

	if( (penv->pstmt->sqlexpr)[(penv->scanpos)] != c )
		(penv->pstmt->sqlexpr)[(penv->scanpos)] = c;
}

static long	getnum(yyenv_t* penv)
{
	long	a;
	char	c;
	int	i, errpos = (penv->scanpos);

	a = atol((penv->pstmt->sqlexpr) + (penv->scanpos));

	for(i=0;;i++)
	{
		c = popc(penv);

		if( ! isdigit(c) )
		{
			unputc(c, penv);
			break;
		}
	}

	(penv->errpos) = errpos;
	return a;
}

static	int	getqstring(char* buf, int len, yyenv_t* penv, char term)
{
	char	c, c1;
	int	i, errpos = (penv->scanpos);

	for(i=0; len == -1 || i<len; i++)
	{
		buf[i] = c = popc(penv);

		if( c == term )
		{
			c1 = popc(penv);

			if( c1 == term )
				continue;

			unputc(c1, penv);
			break;
		}

		if( !c || c == '\n' )
			return YYERRCODE;
	}

	buf[i] = 0;
	(penv->errpos) = errpos;

	return i;
}

static	int	getname(char* buf, int len, yyenv_t* penv)
{
	int	i = 0;
	char	c;
	int	errpos = (penv->scanpos);

	for(i=0; len == -1 || i<len; i++)
	{
		buf[i] = c = popc(penv);

		if( isalpha(c) || (i && isdigit(c)) )
			continue;
		else if( i )
		{
			char	c1;

			c1 = popc(penv);

			unputc(c1, penv);

			switch(c)
			{
				case '+':	/* a group name may use '+'
						   such as comp.lang.c++ */
				case '-':	/* a group name may use '-'
						   such as comp.client-server */
						/* However, the best way is
						   using double quato '"' for
						   such kind of group name */
				case '_':
					continue;

				case '.':
					if(isalpha(c1))
						continue;
					break;

				default:
					break;
			}
		}

		break;
	}

	buf[i] = 0;
	unputc(c, penv);
	(penv->errpos) = errpos;

	return i;
}

static	int	getcmpop(yyenv_t* penv)
{
	char	opname[3];
	char	c, c1;
	int	errpos = (penv->scanpos);

	opname[0] = c = popc(penv);
	opname[1] = c1= popc(penv);
	opname[2] = 0;

	if( c1 != '=' && c1 != '>' && c1 != '<' )
	{
		opname[1] = 0;
		unputc(c1, penv);
	}

	(penv->errpos) = errpos;

	return	getcmpopidxbyname(opname);
}

int	nnsql_yylex(YYSTYPE* pyylval, yyenv_t* penv)
{
	int		len, cmpop;
	char		c;

	do
	{ c = popc(penv); }
	while( c == ' ' || c == '\t' || c == '\n' );

	if( isalpha(c) )
	{
		int	keywdidx;

		unputc(c, penv);

		len = getname(penv->texts_bufptr, -1, penv);

		if( len == YYERRCODE )
			return len;

		if( penv->extlevel )
			keywdidx = getxkeywdidxbyname( penv->texts_bufptr );
		else
			keywdidx = YYERRCODE;

		if( keywdidx == YYERRCODE )
			keywdidx = getkeywdidxbyname( penv->texts_bufptr);

		if( keywdidx != YYERRCODE )
			return keywdidx;

		pyylval->name = penv->texts_bufptr;
		(penv->texts_bufptr) += (len + 1);

		return NAME;
	}

	if( isdigit(c) )
	{
		unputc(c, penv);

		pyylval->number = getnum(penv);

		return NUM;
	}

	switch( c )
	{
		case ';':
		case '\0':
			return ';';


		case '\'':
		case '"':
			len = getqstring(penv->texts_bufptr, -1, penv, c);

			if( len == YYERRCODE )
				return len;

			if( c == '\'' )
			{
				pyylval->qstring = penv->texts_bufptr;
				penv->texts_bufptr += (len + 1);
				return QSTRING;
			}

			pyylval->name = penv->texts_bufptr;
			(penv->texts_bufptr) += (len + 1);
			return NAME;

		case '=':
		case '<':
		case '>':
		case '!':
			unputc(c, penv);
			cmpop = getcmpop( penv );
			if(cmpop == YYERRCODE )
				return cmpop;
			pyylval->cmpop = cmpop;
			return CMPOP;

		case '?':
			pyylval->ipar = (++(penv->dummy_npar));
			return PARAM;

		case '{':
			penv->extlevel ++;
			break;

		case '}':
			penv->extlevel --;
			break;

		default:
			break;
	}

	return c;
}

void	nnsql_yyinit(yyenv_t* penv, yystmt_t* yystmt)
{
	penv->extlevel = 0;
	penv->dummy_npar = penv->errpos = penv->scanpos = 0;
	penv->pstmt = yystmt;
	penv->texts_bufptr = yystmt->texts_buf;
}
