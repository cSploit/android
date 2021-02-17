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
#ifndef _YYENV_H
# define _YYENV_H

# include	<yystmt.h>

typedef struct
{
	int		extlevel;	/* {} block nest level	*/
	int		errpos; 	/* error   position	*/
	int		scanpos;	/* current position	*/
	char*		texts_bufptr;
	int		dummy_npar;	/* total number of dummy parameters */

	yystmt_t*	pstmt;
} yyenv_t;

/* it is unlikely we will exceed bison's default init stack depth YYINITDEPTH,
 * i.e. 200.  Thus, for system/compiler which not support alloca(), we simply
 * set it to null and make a bit larger init stack depth for more safty.
 */
# if	defined(__hpux) && !defined (alloca) && !defined(__GNUC__)
#  define	alloca(size)	(0)
#  define	YYINITDEPTH	(512)
# endif

#endif
