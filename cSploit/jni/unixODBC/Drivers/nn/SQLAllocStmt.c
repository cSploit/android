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

RETCODE SQL_API SQLAllocStmt(
									 HDBC  hdbc,
									 HSTMT*   phstmt )
{
	void* hcndes;
	void* yystmt;
	stmt_t* pstmt;

	*phstmt = 0;

	hcndes = nnodbc_getnntpcndes(hdbc);

	if ( ! (yystmt = nnsql_allocyystmt(hcndes)) )
	{
		int   code = nnsql_errcode(hcndes);

		if ( code == -1 )
			code = errno;

		nnodbc_pushdbcerr( hdbc, code, nnsql_errmsg(hcndes));

		return SQL_ERROR;
	}

	pstmt = (stmt_t*)MEM_ALLOC(sizeof(stmt_t));

	if ( !pstmt )
	{
		nnsql_dropyystmt(yystmt);
		nnodbc_pushdbcerr(hdbc, en_S1001, 0);
		return SQL_ERROR;
	}

	if ( nnodbc_attach_stmt( hdbc, pstmt ) )
	{
		nnsql_dropyystmt(yystmt);
		MEM_FREE( pstmt );
		return SQL_ERROR;
	}

	pstmt->yystmt = yystmt;
	pstmt->herr = 0;
	pstmt->pcol = 0;
	pstmt->ppar = 0;
	pstmt->ndelay = 0;
	pstmt->hdbc = hdbc;
	pstmt->refetch = 0;
	pstmt->putipar = 0;

	*phstmt = pstmt;

	return SQL_SUCCESS;
}

