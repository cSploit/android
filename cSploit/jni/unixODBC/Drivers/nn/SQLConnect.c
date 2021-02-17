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

RETCODE SQL_API SQLConnect(
								  HDBC  hdbc,
								  UCHAR*   szDSN,
								  SWORD cbDSN,
								  UCHAR*   szUID,
								  SWORD cbUID,
								  UCHAR*   szAuthStr,
								  SWORD cbAuthStr)
{
	dbc_t*   pdbc = hdbc;
	char* ptr;
	char  buf[64];

	UNSET_ERROR( pdbc->herr );

	ptr = (char*)getkeyvalbydsn( (char*)szDSN, cbDSN, "Server",
										  buf, sizeof(buf));

	if ( ! ptr )
	{
		PUSHSQLERR( pdbc->herr, en_IM002 );

		return SQL_ERROR;
	}

	pdbc->hcndes = nntp_connect(ptr);

	if ( ! pdbc->hcndes )
	{
		PUSHSQLERR( pdbc->herr, en_08001 );
		PUSHSYSERR( pdbc->herr, errno, nntp_errmsg(0));

		return SQL_ERROR;
	}

	return SQL_SUCCESS;
}


