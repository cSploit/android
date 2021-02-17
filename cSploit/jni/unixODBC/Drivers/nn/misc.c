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
#include	<nnconfig.h>

int	upper_strneq(
	char*	s1,
	char*	s2,
	int	n )
{
	int	i;
	char	c1, c2;

	for(i=0;i<n;i++)
	{
		c1 = s1[i];
		c2 = s2[i];

		if( c1 >= 'a' && c1 <= 'z' )
			c1 += ('A' - 'a');
		else if( c1 == '\n' )
			c1 = '\0';

		if( c2 >= 'a' && c2 <= 'z' )
			c2 += ('A' - 'a');
		else if( c2 == '\n' )
			c2 = '\0';

		if( (c1 - c2) || !c1 || !c2 )
			break;
	}

	return (int)!(c1 - c2);
}

int	nnodbc_conndialog()
{
	return -1;
}
