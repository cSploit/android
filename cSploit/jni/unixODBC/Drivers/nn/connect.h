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
#ifndef _CONNECT_H
# define _CONNECT_H

extern int	nnodbc_attach_stmt(void* hdbc, void* hstmt);
extern int	nnodbc_detach_stmt(void* hdbc, void* hstmt);
extern void*	nnodbc_getenverrstack(void* henv);
extern void*	nnodbc_getdbcerrstack(void* hdbc);
extern void	nnodbc_pushdbcerr(void* hdbc, int code, char* msg);
extern void*	nnodbc_getnntpcndes(void* hdbc);

#endif
