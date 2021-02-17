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

#include	<nncol.h>
#include	"nncol.ci"

int	nnsql_getcolidxbyname( char* col_name )
{
	int	i;

	for( i= 0; nncol_info_tab[i].idx != en_sql_count; i++ )
	{
		if(upper_strneq( col_name, nncol_info_tab[i].name, 16))
			return nncol_info_tab[i].idx;
	}

	return -1;
}

char*	nnsql_getcolnamebyidx( int idx )
{
	int	i;

	if( nncol_info_tab[idx].idx == idx )
		return nncol_info_tab[idx].name;

	for(i=0; nncol_info_tab[i].idx != en_sql_count;i++)
	{
		if( nncol_info_tab[i].idx == idx )
			return nncol_info_tab[i].name;
	}

	return 0;
}

void*	nnsql_getcoldescbyidx( int idx )
{
	int	i;

	if( nncol_info_tab[idx].idx == idx )
		return &(nncol_info_tab[idx]);

	for(i=0; i<sizeof(nncol_info_tab)/sizeof(nncol_info_tab[0]); i++)
	{
		if( nncol_info_tab[i].idx == idx )
			return &(nncol_info_tab[i]);
	}

	return 0;
}

char*	nnsql_getcolnamebydesc( void* hdesc )
{
	col_desc_t*	desc = hdesc;

	return desc->name;
}

int	nnsql_getcoldatatypebydesc( void* hdesc )
{
	col_desc_t*	desc = hdesc;

	return desc->datatype;
}

int	nnsql_getcolnullablebydesc( void* hdesc )
{
	col_desc_t*	desc = hdesc;

	return desc->nullable;
}

