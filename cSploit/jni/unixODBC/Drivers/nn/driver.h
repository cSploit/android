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
#ifndef _H_DRIVER
#define _H_DRIVER

#include "nnconfig.h"
#include "isql.h"
#include "isqlext.h"
#include "convert.h"

#include "yystmt.h"
#include "yyerr.h"
#include "yyenv.h"

#include "hstmt.h"
#include "herr.h"

typedef struct GSTMT {
	void*	hdbc;
	void*	hstmt;
	struct GSTMT*
		next;
} gstmt_t;

typedef struct DBC {
	void*	hcndes;
	void*	henv;
	gstmt_t*
		stmt;
	void*	herr;

	struct DBC*
		next;
} dbc_t;

typedef struct {
	void*	hdbc;
	void*	herr;
} env_t;


typedef struct {
	int	bind;
	short	type;
	unsigned long
		coldef;
	short	scale;
	char*	userbuf;
	long	userbufsize;
	long*	pdatalen;

	int	ctype;
	int	sqltype;
	fptr_t	cvt;	/* c to sql type convert function */

	/* for SQLPutData() on SQL_CHAR, SQL_VARCHAR, SQL_LONGVARCHAR */
	char*	putdtbuf;
	int	putdtlen;
	int	need;
} param_t;

typedef struct {
	short	ctype;
	char*	userbuf;
	long	userbufsize;
	long*	pdatalen;

	long	offset; 		/* getdata offset */
} column_t;

typedef struct {
	void*		herr;
	void*		hdbc;
	column_t*	pcol;		/* user bound columns */
	param_t*	ppar;		/* user bound parameters */
	int		ndelay; 	/* number of delay parameters */
	void*		yystmt; 	/* sql layer object handle */

	int		refetch;	/* need refetch */

	int		putipar;
} stmt_t;

#include "connect.h"

#include "nnsql.h"
#include "nntp.h"


char*	getkeyvalbydsn(
		char*	dsn,
		int	dsnlen,
		char*	keywd,
		char*	value,
		int	size );

char*	getkeyvalinstr(
		char*	cnstr,
		int	cnlen,
		char*	keywd,
		char*	value,
		int	size );

#endif


