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

RETCODE SQL_API SQLGetConnectOption(HDBC hdbc, UWORD fOption, PTR pvParam)
{
	dbc_t*   pdbc = hdbc;
	UDWORD   opt;

	UNSET_ERROR( pdbc->herr );

	if ( fOption == SQL_ACCESS_MODE )
	{
		switch (nntp_getaccmode(pdbc->hcndes))
		{
		case ACCESS_MODE_SELECT:
			opt = SQL_MODE_READ_ONLY;
			break;

		case ACCESS_MODE_INSERT:
		case ACCESS_MODE_DELETE_TEST:
		case ACCESS_MODE_DELETE_ANY:
			opt = SQL_MODE_READ_WRITE;
			break;

		default:
			opt = SQL_MODE_DEFAULT;
			break;
		}

		if (pvParam)
			*((UDWORD*)pvParam) = opt;

		return SQL_SUCCESS;
	}

	PUSHSQLERR( pdbc->herr, en_S1C00 );

	return SQL_ERROR;
}

