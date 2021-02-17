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
#include <string.h>
#include "driver.h"

#include	"yyerr.ci"

int	nnsql_errcode(void* yystmt)
{
	if( ! yystmt )
		return -1;

	return ((yystmt_t*)yystmt)->errcode;
}

char*	nnsql_errmsg(void* yystmt)
{
	yystmt_t*	pstmt = yystmt;
	int		i, errcode;

	errcode = nnsql_errcode( yystmt );

	switch( errcode )
	{
		case 0:
			return nntp_errmsg(pstmt->hcndes);

		case -1:
			if( nntp_errcode(pstmt->hcndes) )
				return nntp_errmsg(pstmt->hcndes);
			else
				return strerror( errno );

		case PARSER_ERROR:
			return ((yystmt_t*)yystmt)->msgbuf;

		default:
			break;
	}

	for(i=0; i<sizeof(yy_errmsg)/sizeof(yy_errmsg[0]);i++)
	{
		if( yy_errmsg[i].errcode == errcode )
			return yy_errmsg[i].msg;
	}

	return 0;
}

int	nnsql_errpos(void* yystmt)
{
	return ((yystmt_t*)yystmt)->errpos;
}
