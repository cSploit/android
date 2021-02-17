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

RETCODE SQL_API SQLBindCol(
								  HSTMT hstmt,
								  UWORD icol,
								  SWORD fCType,
								  PTR   rgbValue,
								  SDWORD   cbValueMax,
								  SDWORD* pcbValue )
{
	stmt_t*  pstmt = hstmt;
	column_t*   pcol;
	int      i, max;

	UNSET_ERROR( pstmt->herr );

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

	max = nnsql_max_column();

	if ( icol > (UWORD)max )
	{
		PUSHSQLERR( pstmt->herr, en_S1002 );

		return SQL_ERROR;
	}

	if ( ! pstmt->pcol )
	{
		if ( ! rgbValue )
			return SQL_SUCCESS;

		pstmt->pcol = (column_t*)MEM_ALLOC( sizeof(column_t)*(max+1) );

		if ( ! pstmt->pcol )
		{
			PUSHSQLERR( pstmt->herr, en_S1001 );

			return SQL_ERROR;
		}

		MEM_SET(pstmt->pcol, 0, sizeof(column_t)*(max+1) );
	}

	pcol = pstmt->pcol + icol;

	pcol->ctype = fCType;
	pcol->userbuf = rgbValue;
	pcol->userbufsize = cbValueMax;
	pcol->pdatalen = (long*) pcbValue;

	pcol->offset = 0;

	return SQL_SUCCESS;
}



