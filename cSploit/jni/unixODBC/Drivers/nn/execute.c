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

int	sqlputdata (
		stmt_t* 	pstmt,
		int		ipar,
		char*		data )
{
	param_t*	ppar;

	ppar = pstmt->ppar + ipar - 1;

	switch( ppar->sqltype )
	{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
			if( ! data )
				nnsql_putnull(pstmt->yystmt, ipar );
			else
				nnsql_putstr(pstmt->yystmt, ipar, (char*)data);
			break;

		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
			nnsql_putnum(pstmt->yystmt, ipar, (long)data);
			break;

		case SQL_DATE:
			if(!data )
				nnsql_putnull(pstmt->yystmt, ipar );
			else
				nnsql_putdate(pstmt->yystmt, ipar, (date_t*)data);
			break;

		default:
			return -1;
	}

	return 0;
}

int	sqlexecute (
	stmt_t*   pstmt)
{
	param_t*	ppar = pstmt->ppar;
	int		i, npar, flag = 0;
	long		csize;

	pstmt->refetch = 0;
	pstmt->ndelay  = 0;

	npar = nnsql_getparnum(pstmt->yystmt);

	for( i = 0; ppar && i< npar; i++ )
	/* general check */
	{
		ppar = pstmt->ppar + i;

		if( ! ppar->bind )
		{
			PUSHSQLERR( pstmt->herr, en_07001 );
			return SQL_ERROR;
		}

		if( ( ! ppar->userbuf && ppar->pdatalen )
		 || (	ppar->userbuf && ppar->pdatalen
		    && *ppar->pdatalen < 0 && *ppar->pdatalen != SQL_NTS ) )
		{
			 if( *ppar->pdatalen
			  || *ppar->pdatalen == (long)SQL_NULL_DATA
			  || *ppar->pdatalen == (long)SQL_DATA_AT_EXEC
			  || *ppar->pdatalen <= (long)SQL_LEN_DATA_AT_EXEC(0) )
				continue;

			PUSHSQLERR( pstmt->herr, en_S1090 );

			return SQL_ERROR;
		}
	}

	for( i = 0; ppar && i< npar; i++ )
	{
		char*	(*cvt)();
		date_t	date;
		char*	data;

		ppar = pstmt->ppar + i;

		if( ! ppar->pdatalen )
			csize = 0;
		else
			csize = *(ppar->pdatalen);

		if( csize == (long)SQL_NULL_DATA )
		{
			nnsql_putnull(pstmt->yystmt, i+1);
			continue;
		}

		if( csize == (long)SQL_DATA_AT_EXEC
		 || csize <= (long)SQL_LEN_DATA_AT_EXEC(0) )
		/* delated parameter */
		{
			pstmt->ndelay ++;
			ppar->need = 1;

			continue;
		}

		cvt = ppar->cvt;
		data= cvt(ppar->userbuf, csize, &date );

		if( data == (char*)(-1) )
		{
			PUSHSQLERR(pstmt->herr, en_S1000 );

			return SQL_ERROR;
		}

		sqlputdata(pstmt, i+1, data);
	}

	if( pstmt->ndelay )
		return SQL_NEED_DATA;

	if( nnsql_execute(pstmt->yystmt) )
	{
		int	code;

		code = nnsql_errcode( pstmt->yystmt );

		if( code == -1 )
			code = errno;

		PUSHSYSERR( pstmt->herr, code, nnsql_errmsg(pstmt->yystmt));

		return SQL_ERROR;
	}

	if( ! nnsql_getcolnum(pstmt->yystmt)
	 && nnsql_getrowcount(pstmt->yystmt) > 1 )
	{
		PUSHSQLERR( pstmt->herr, en_01S04);

		return SQL_SUCCESS_WITH_INFO;
	}

	return SQL_SUCCESS;
}






