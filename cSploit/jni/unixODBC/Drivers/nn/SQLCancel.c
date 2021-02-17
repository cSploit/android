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

RETCODE SQL_API SQLCancel(
								 HSTMT hstmt)
{
	stmt_t*  pstmt = hstmt;
	param_t* ppar;
	int      i, npar;

	UNSET_ERROR( pstmt->herr );

	npar = nnsql_getparnum(pstmt->yystmt);

	for (i=1, ppar = pstmt->ppar; ppar && i < npar + 1; i++, ppar++ )
	{
		nnsql_yyunbindpar( pstmt->yystmt, i);
		MEM_FREE(ppar->putdtbuf);
		ppar->putdtbuf = 0;
		ppar->putdtlen = 0;
		ppar->need = 0;
	}

	pstmt->ndelay = 0;
	pstmt->putipar= 0;

	return SQL_SUCCESS;
}
