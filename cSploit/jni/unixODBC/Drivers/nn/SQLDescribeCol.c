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

RETCODE SQL_API SQLDescribeCol(
										HSTMT hstmt,
										UWORD icol,
										UCHAR*   szColName,
										SWORD cbColNameMax,
										SWORD*   pcbColName,
										SWORD*   pfSqlType,
										UDWORD* pcbColDef,
										SWORD*   pibScale,
										SWORD*   pfNullable )
{
	stmt_t* pstmt = hstmt;
	char* colname;
	int   colnamelen, sqltype, precision, ncol;
	RETCODE retcode = SQL_SUCCESS;

	UNSET_ERROR( pstmt->herr );

	ncol = nnsql_getcolnum(pstmt->yystmt);

	if ( icol > (UWORD)(ncol - 1) )
	{
		PUSHSQLERR( pstmt->herr, en_S1002 );

		return SQL_ERROR;
	}


	colname = nnsql_getcolnamebyidx(
											 nnsql_column_descid(pstmt->yystmt, icol) );

	colnamelen = STRLEN(colname);

	if ( szColName )
	{
		if ( cbColNameMax < colnamelen + 1 )
		{
			colnamelen = cbColNameMax - 1;
			PUSHSQLERR( pstmt->herr, en_01004 );
			retcode = SQL_SUCCESS_WITH_INFO;
		}

		STRNCPY( szColName, colname, colnamelen);
		szColName[colnamelen] = 0;

		if ( pcbColName )
			*pcbColName = colnamelen;
	}

	if ( nnsql_isstrcol(pstmt->yystmt, icol) )
	{
		sqltype = SQL_LONGVARCHAR;
		precision = SQL_NO_TOTAL;
	}
	else if ( nnsql_isnumcol(pstmt->yystmt, icol) )
	{
		sqltype = SQL_INTEGER;
		precision = 10;
	}
	else if ( nnsql_isdatecol(pstmt->yystmt, icol) )
	{
		sqltype = SQL_DATE;
		precision = 10;
	}
	else
	{
		sqltype= 0;
		precision = SQL_NO_TOTAL;
	}

	if ( pfSqlType )
		*pfSqlType = sqltype;

	if ( pcbColDef )
		*pcbColDef = precision;

	if ( pfNullable )
		*pfNullable = nnsql_isnullablecol(pstmt->yystmt, icol);


	return retcode;
}


