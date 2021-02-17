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

RETCODE SQL_API SQLAllocConnect(
										 HENV   henv,
										 HDBC *   phdbc)
{
	env_t*   penv = henv;
	dbc_t*   pdbc;
	int   i;

	UNSET_ERROR( penv->herr );

	pdbc = *phdbc = (void*)MEM_ALLOC( sizeof(dbc_t) );

	if (! pdbc )
	{
		PUSHSQLERR( penv->herr, en_S1001 );
		return SQL_ERROR;
	}

	pdbc->next = penv->hdbc;
	penv->hdbc = pdbc;

	pdbc->henv = henv;
	pdbc->stmt= 0;
	pdbc->herr = 0;

	pdbc->hcndes = 0;

	return SQL_SUCCESS;
}


