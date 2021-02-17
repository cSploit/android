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

void*	nnodbc_getstmterrstack(void* hstmt)
{
	return ((stmt_t*)hstmt)->herr;
}


int	nnodbc_sqlfreestmt(
		void*	hstmt,
		int	fOption)
{
	stmt_t* pstmt = hstmt;
	int	i, max;

	switch( fOption )
	{
		case SQL_DROP:
			nnodbc_detach_stmt(pstmt->hdbc, hstmt);

			/* free all resources */
			MEM_FREE(pstmt->pcol);
			MEM_FREE(pstmt->ppar);
			CLEAR_ERROR(pstmt->herr);
			MEM_FREE(hstmt);
			break;

		case SQL_CLOSE:
			nnsql_close_cursor(hstmt);
			break;

		case SQL_UNBIND:
			max = nnsql_max_column();

			for(i=0; pstmt->pcol && i < max + 1; i++ )
				(pstmt->pcol)[i].userbuf = 0;

			break;

		case SQL_RESET_PARAMS:
			max = nnsql_max_param();

			for(i=1;pstmt->ppar && i < max + 1; i++ )
			{
				nnsql_yyunbindpar( pstmt->yystmt, i);
				(pstmt->ppar)[i-1].bind = 0;
			}
			break;

		default:
			/* this error will be caught by driver manager */
			return SQL_ERROR;
	}

	return SQL_SUCCESS;
}

int   nnodbc_sqlprepare (
								void* hstmt,
								char* szSqlStr,
								int   cbSqlStr )
{
	stmt_t* pstmt = hstmt;
	int   len;

	len = (cbSqlStr == SQL_NTS )? STRLEN(szSqlStr):cbSqlStr;

	if ( nnsql_prepare(pstmt->yystmt, szSqlStr, len ) )
	{
		int   code;

		code = nnsql_errcode(pstmt->yystmt);

		if ( code == -1 )
			code = errno;

		PUSHSYSERR( pstmt->herr, code, nnsql_errmsg(pstmt->yystmt) );

		return SQL_ERROR;
	}

	return SQL_SUCCESS;
}




