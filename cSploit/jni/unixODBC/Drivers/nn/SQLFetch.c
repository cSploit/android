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

RETCODE SQL_API SQLFetch( HSTMT hstmt )
{
	stmt_t*  pstmt = hstmt;
	column_t*   pcol = pstmt->pcol;
	int      ncol, i;
	long     len, clen;
	char*    ptr;
	int      sqltype, sqlstat, dft_ctype, flag = 0, err;
	fptr_t      cvt;
	char*    ret;

	UNSET_ERROR( pstmt->herr );

	ncol = nnsql_getcolnum(pstmt->yystmt);

	if ( !pstmt->refetch && (err = nnsql_fetch(pstmt->yystmt)) )
	{
		int   code;

		if ( err == 100 )
			return SQL_NO_DATA_FOUND;

		code = nnsql_errcode(pstmt->yystmt);

		if ( code == -1 )
			code = errno;

		PUSHSYSERR( pstmt->herr, code, nnsql_errmsg(pstmt->yystmt));

		return SQL_ERROR;
	}

	if ( !pcol )
	{
		int   max;

		max = nnsql_max_column();

		pcol = pstmt->pcol = (column_t*)MEM_ALLOC( sizeof(column_t)*(max+1) );

		if ( ! pcol )
		{
			PUSHSQLERR( pstmt->herr, en_S1001 );

			return SQL_ERROR;
		}

		MEM_SET(pcol, 0, sizeof(column_t)*(max+1) );

		return SQL_SUCCESS;
	}

	for (i=0;i<ncol;i++, pcol++)
	{
		len = clen = 0L;
		pcol->offset = 0;

		if ( ! pcol->userbuf )
			continue;

		if ( nnsql_isnullcol(pstmt->yystmt, i) )
		{
			if ( pcol->pdatalen )
				*(pcol->pdatalen) = SQL_NULL_DATA;
			continue;
		}

		if ( pcol->pdatalen )
			*(pcol->pdatalen ) = 0L;

		if ( nnsql_isstrcol(pstmt->yystmt, i) )
		{
			ptr = nnsql_getstr(pstmt->yystmt, i);
			len = STRLEN(ptr) + 1;
			sqltype = SQL_CHAR;
			dft_ctype = SQL_C_CHAR;
		}
		else if ( nnsql_isnumcol(pstmt->yystmt, i) )
		{
			ptr = (char*)nnsql_getnum(pstmt->yystmt, i);
			sqltype = SQL_INTEGER;
			dft_ctype = SQL_C_LONG;
		}
		else if ( nnsql_isdatecol(pstmt->yystmt, i) )
		{
			ptr = (char*)nnsql_getdate(pstmt->yystmt, i);
			sqltype = SQL_DATE;
			dft_ctype = SQL_C_DATE;
		}
		else
			abort();

		if ( pcol->ctype == SQL_C_DEFAULT )
			pcol->ctype = dft_ctype;

		cvt = nnodbc_get_sql2c_cvt(sqltype, pcol->ctype);

		if ( ! cvt )
		{
			pstmt->refetch = 1;

			PUSHSQLERR(pstmt->herr, en_07006);

			return SQL_ERROR;
		}

		ret = cvt(  ptr, pcol->userbuf, pcol->userbufsize, &clen);

		if ( ret )
		{
			pstmt->refetch = 1;

			if ( clen )
				sqlstat = en_22003;
			else
				sqlstat = en_22005;

			PUSHSQLERR( pstmt->herr, sqlstat );

			return SQL_ERROR;
		}

		if ( len && clen == len )
			flag = 1;

		if ( len && pcol->pdatalen )
			*(pcol->pdatalen) = clen;	/* not 'len' but 'clen' */
	}

	if ( flag )
	{
		PUSHSQLERR( pstmt->herr, en_01004 );

		return SQL_SUCCESS_WITH_INFO;
	}

	return SQL_SUCCESS;
}


