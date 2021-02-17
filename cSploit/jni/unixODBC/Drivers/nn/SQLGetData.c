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

RETCODE SQL_API SQLGetData(
								  HSTMT hstmt,
								  UWORD icol,
								  SWORD fCType,
								  PTR   rgbValue,
								  SDWORD   cbValueMax,
								  SDWORD* pcbValue )
{
	stmt_t*  pstmt = hstmt;
	column_t*   pcol;
	int      ncol, flag = 0, clen = 0, len = 0;
	int      sqltype, dft_ctype, sqlstat;
	char*    ptr;
	char*    ret;
	fptr_t      cvt;

	UNSET_ERROR( pstmt->herr );

	ncol = nnsql_getcolnum(pstmt->yystmt);

	if ( icol >= (UWORD)ncol )
	{
		PUSHSQLERR( pstmt->herr, en_S1002 );

		return SQL_ERROR;
	}

	pcol = pstmt->pcol + icol;

	if ( pcol->offset == -1 )
		return SQL_NO_DATA_FOUND;

	if ( fCType == SQL_C_BOOKMARK )
		fCType = SQL_C_ULONG;

	switch (fCType)
	{
	case SQL_C_DEFAULT:
	case SQL_C_CHAR:
	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_USHORT:
	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
	case SQL_C_DATE:
		break;

	default:
		PUSHSQLERR( pstmt->herr, en_S1C00 );
		return SQL_ERROR;
	}

	if ( nnsql_isnullcol(pstmt->yystmt, icol) )
	{
		if ( pcbValue )
			*pcbValue = SQL_NULL_DATA;

		return SQL_SUCCESS;
	}

	if ( pcbValue )
		*pcbValue = 0L;

	if ( nnsql_isstrcol(pstmt->yystmt, icol) )
	{
		ptr = nnsql_getstr(pstmt->yystmt, icol) + pcol->offset;
		len = STRLEN(ptr) + 1;
		sqltype = SQL_CHAR;
		dft_ctype = SQL_C_CHAR;
	}
	else if ( nnsql_isnumcol(pstmt->yystmt, icol) )
	{
		ptr = (char*)nnsql_getnum(pstmt->yystmt, icol);
		sqltype = SQL_INTEGER;
		dft_ctype = SQL_C_LONG;
	}
	else if ( nnsql_isdatecol(pstmt->yystmt, icol) )
	{
		ptr = (char*)nnsql_getdate(pstmt->yystmt, icol);
		sqltype = SQL_DATE;
		dft_ctype = SQL_C_DATE;
	}
	else
		abort();

	if ( fCType == SQL_C_DEFAULT )
		fCType = dft_ctype;

	cvt = nnodbc_get_sql2c_cvt(sqltype, fCType);

	if ( ! cvt )
	{
		PUSHSQLERR(pstmt->herr, en_07006);

		return SQL_ERROR;
	}

	ret = cvt(ptr, rgbValue, cbValueMax, &clen);

	if ( ret )
	{
		if ( clen )
			sqlstat = en_22003;
		else
			sqlstat = en_22005;

		PUSHSQLERR( pstmt->herr, sqlstat );

		return SQL_ERROR;
	}

	if ( len && clen == cbValueMax )
	{
		flag = 1;
		pcol->offset += (clen - 1);
	}
	else
		pcol->offset = -1;

	if ( len && pcbValue )
		*pcbValue = len;	/* not 'clen' but 'len' */

	if ( flag )
	{
		PUSHSQLERR(pstmt->herr, en_01004 );

		return SQL_SUCCESS_WITH_INFO;
	}

	return SQL_SUCCESS;
}

