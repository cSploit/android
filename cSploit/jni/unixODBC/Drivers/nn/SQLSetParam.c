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

RETCODE SQL_API SQLSetParam (
									 HSTMT     hstmt,
									 UWORD     ipar,
									 SWORD     fCType,
									 SWORD     fSqlType,
									 UDWORD       cbColDef,
									 SWORD     ibScale,
									 PTR       rgbValue,
									 SDWORD FAR *pcbValue)
{
	return SQLBindParameter(hstmt,
									ipar,
									(SWORD)SQL_PARAM_INPUT_OUTPUT,
									fCType,
									fSqlType,
									cbColDef,
									ibScale,
									rgbValue,
									SQL_SETPARAM_VALUE_MAX,
									pcbValue );
}


