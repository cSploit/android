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

RETCODE SQL_API SQLDriverConnect(
										  HDBC  hdbc,
										  HWND  hwnd,
										  UCHAR*   szConnStrIn,
										  SWORD cbConnStrIn,
										  UCHAR*   szConnStrOut,
										  SWORD cbConnStrOutMax,
										  SWORD*   pcbConnStrOut,
										  UWORD fDriverCompletion)
{
	dbc_t*   pdbc = hdbc;
	char  *dsn, *server;
	char  buf[64];
	int   sqlstat = en_00000;

	UNSET_ERROR( pdbc->herr );

	server = getkeyvalinstr((char*)szConnStrIn, cbConnStrIn,
									"Server", buf, sizeof(buf));

	if ( ! server )
	{
		dsn = getkeyvalinstr((char*)szConnStrIn, cbConnStrIn,
									"DSN", buf, sizeof(buf));

		if ( !dsn )
			dsn = "default";

		server = getkeyvalbydsn((char*)dsn, SQL_NTS, "Server",
										buf, sizeof(buf));
	}

	if ( ! server )
		buf[0] = 0;

	switch ( fDriverCompletion )
	{
	case SQL_DRIVER_NOPROMPT:
		break;

	case SQL_DRIVER_COMPLETE:
	case SQL_DRIVER_COMPLETE_REQUIRED:
		if ( ! server )
			break;
		/* to next case */
	case SQL_DRIVER_PROMPT:
		if ( nnodbc_conndialog( hwnd, buf, sizeof(buf)) )
		{
			sqlstat = en_IM008;
			break;
		}
		server = buf;
		break;

	default:
		sqlstat = en_S1110;
		break;
	}

	if ( sqlstat != en_00000 )
	{
		PUSHSQLERR( pdbc->herr, sqlstat );

		return SQL_ERROR;
	}

	if ( !server )
	{
		PUSHSYSERR( pdbc->herr, en_S1000,
						NNODBC_ERRHEAD "server name or address not specified" );

		return SQL_ERROR;
	}

	pdbc->hcndes = nntp_connect(server);

	if ( ! pdbc->hcndes )
	{
		PUSHSQLERR( pdbc->herr, en_08001 );
		PUSHSYSERR( pdbc->herr, errno, nntp_errmsg(0));

		return SQL_ERROR;
	}

	return SQL_SUCCESS;
}

