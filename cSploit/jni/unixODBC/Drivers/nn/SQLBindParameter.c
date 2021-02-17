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

RETCODE SQL_API SQLBindParameter(
										  HSTMT hstmt,
										  UWORD ipar,
										  SWORD fParamType,
										  SWORD fCType,
										  SWORD fSqlType,
										  UDWORD   cbColDef,
										  SWORD ibScale,
										  PTR   rgbValue,
										  SDWORD   cbValueMax,
										  SDWORD* pcbValue)
{
	param_t* ppar;
	int      i, max;
	stmt_t*  pstmt = hstmt;
	fptr_t      cvt;

	UNSET_ERROR( pstmt->herr );

	max = nnsql_max_param();

	if ( ipar > (UWORD)max )
	{
		PUSHSQLERR( pstmt->herr, en_S1093);

		return SQL_ERROR;
	}

	if ( fCType == SQL_C_DEFAULT )
	{
		switch ( fSqlType )
		{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
			fCType = SQL_C_CHAR;
			break;

		case SQL_TINYINT:
			fCType = SQL_C_STINYINT;
			break;

		case SQL_SMALLINT:
			fCType = SQL_C_SSHORT;
			break;

		case SQL_INTEGER:
			fCType = SQL_C_SLONG;
			break;

		case SQL_DATE:
			fCType = SQL_C_DATE;
			break;

		default:
			PUSHSQLERR( pstmt->herr, en_S1C00 );
			return SQL_ERROR;
		}
	}

	cvt = nnodbc_get_c2sql_cvt( fCType, fSqlType);

	if ( ! cvt )
	{
		PUSHSQLERR( pstmt->herr, en_07006 );

		return SQL_ERROR;
	}

	if ( ! pstmt->ppar )
	{
		int   i;

		pstmt->ppar = (param_t*)MEM_ALLOC( sizeof(param_t)*max );

		if ( ! pstmt->ppar )
		{
			PUSHSQLERR( pstmt->herr, en_S1001 );

			return SQL_ERROR;
		}

		ppar = pstmt->ppar;

		MEM_SET( ppar, 0, sizeof(param_t)*max );

		for (i=0; i< max; i++ )
		{
			ppar->bind = 0;
			ppar ++;
		}
	}

	ppar = pstmt->ppar + ipar - 1;

	ppar->bind  = 1;
	ppar->type  = fParamType;
	ppar->coldef   = cbColDef;
	ppar->scale = ibScale;
	ppar->userbuf  = rgbValue;
	ppar->userbufsize = cbValueMax;
	ppar->pdatalen = (long*) pcbValue;

	ppar->ctype = fCType;
	ppar->sqltype  = fSqlType;
	ppar->cvt   = cvt;

	return SQL_SUCCESS;
}


