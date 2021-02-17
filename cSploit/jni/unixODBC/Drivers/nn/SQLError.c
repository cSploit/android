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

RETCODE SQL_API SQLError(
								HENV  henv,
								HDBC  hdbc,
								HSTMT hstmt,
								UCHAR*   szSqlStat,
								SDWORD* pNativeCode,
								UCHAR*   szMsg,
								SWORD cbMsgMax,
								SWORD*   pcbMsg )
{
	void* herr;
	char* ststr;

	if ( hstmt )
		herr = nnodbc_getstmterrstack(hstmt);
	else if ( hdbc )
		herr = nnodbc_getdbcerrstack(hdbc);
	else if ( henv )
		herr = nnodbc_getenverrstack(henv);

	if ( nnodbc_errstkempty(herr) )
		return SQL_NO_DATA_FOUND;

	ststr = nnodbc_getsqlstatstr(herr);

	if (!ststr)
		ststr = "S1000";

	if ( szSqlStat )
		STRCPY( szSqlStat, ststr );

	if ( pNativeCode )
		*pNativeCode = nnodbc_getnativcode(herr);

	if ( szMsg )
	{
		char  buf[128];
		char* msg;

		msg = nnodbc_getsqlstatmsg(herr);

		if ( !msg )
			msg = nnodbc_getnativemsg( herr );

		if ( !msg )
			msg = "(null)";

		sprintf(buf, NNODBC_ERRHEAD "%s", msg);

		STRNCPY( szMsg, buf, cbMsgMax );
		szMsg[cbMsgMax-1]=0;

		if ( pcbMsg )
			*pcbMsg = STRLEN(szMsg);
	}
	else if (pcbMsg)
		*pcbMsg = 0;

	nnodbc_poperr(herr);

	return SQL_SUCCESS;
}

