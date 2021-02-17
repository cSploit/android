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

RETCODE SQL_API SQLPutData(
								  HSTMT hstmt,
								  PTR   rgbValue,
								  SDWORD   cbValue)
{
	stmt_t*  pstmt = hstmt;
	param_t* ppar;
	char*    ptr;
	fptr_t      cvt;
	char*    data;
	date_t      dt;

	UNSET_ERROR( pstmt->herr );

	ppar = pstmt->ppar + pstmt->putipar - 1;

	if ( ppar->ctype == SQL_C_CHAR )
	{
		if ( cbValue == SQL_NULL_DATA )
			return SQL_SUCCESS;

		if ( cbValue == SQL_NTS )
			cbValue = STRLEN(rgbValue);

		if ( ! ppar->putdtbuf )
		{
			ppar->putdtbuf = (char*)MEM_ALLOC(cbValue + 1);
		}
		else if ( cbValue )
		{
			ppar->putdtbuf = (char*)MEM_REALLOC(
														  ppar->putdtbuf,
														  ppar->putdtlen + cbValue + 1);
		}

		if ( ! ppar->putdtbuf )
		{
			PUSHSQLERR(pstmt->herr, en_S1001);
			return SQL_ERROR;
		}

		ptr = ppar->putdtbuf + ppar->putdtlen;

		STRNCPY(ptr, rgbValue, cbValue);
		ptr[cbValue] = 0;

		ppar->putdtlen += cbValue;

		return SQL_SUCCESS;
	}

	cvt = ppar->cvt;
	data= cvt(ppar->putdtbuf, ppar->putdtlen, &dt);

	if ( data == (char*)(-1) )
	{
		PUSHSQLERR( pstmt->herr, en_S1000 );

		return SQL_ERROR;
	}

	sqlputdata( pstmt, pstmt->putipar, data );

	return SQL_SUCCESS;
}

