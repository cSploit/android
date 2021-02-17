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

#include	<nndate.h>

static char* month_name[] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static int	nndate2date(char* str, date_t* date)
{
	int	i;
	char	buf[4];
	date_t	dt;

	if( STRLEN(str) < STRLEN("yyyy m d") )
		return -1;

	sscanf(str, "%d %s %d", &(dt.day), buf, &(dt.year));

	if( dt.year < 100 && dt.year > 0 )
		dt.year += 1900;

	if( dt.day <= 0 || dt.day > 31 )
		return -1;

	if(i = atoi(buf))
	{
		dt.month = i;

		if( i <= 0 || i > 12 )
			return -1;

		*date = dt;
		date->month = i;

		return 0;
	}

	for(i=0;i<12;i++)
	{
		if( upper_strneq(buf, month_name[i], 3) )
		{
			*date = dt;
			date->month = i+1;

			return 0;
		}
	}

	return -1;
}

int	nnsql_nndatestr2date(char* str, date_t* date)
/* convert 'dd mm year' or 'week, dd mm year' to date struct */
{
	int	r;
	date_t	dt;

	if( !str )
	{
		if(date)
			date->day = 0;

		return 0;
	}

	if(atoi(str))
		r = nndate2date(str, &dt);	/* a 'dd mm year' string */
	else
		r = nndate2date(str+5, &dt);	/* a 'week, dd mm year' string */

	if( r )
		dt.day = 0;

	if( date )
		*date = dt;

	return r;
}

int	nnsql_datecmp(date_t* a, date_t* b)
{
	int	r;

	if( (r = a->year - b->year) )
		return r;

	if( (r = a->month- b->month) )
		return r;

	return (a->day - b->day);
}

int	nnsql_odbcdatestr2date(char* str, date_t* date)
/* convert 'yyyy-m-d', 'yyyy-mm-dd' or 'yyyy-mmm-dd' or even 'yyyy/mm/dd'
 * string to date struct */
{
	date_t	dt;
	int	i;

	if( ! str )
	{
		if( date )
			date->day = 0;

		return 0;
	}

	if( STRLEN(str) < STRLEN("yyyy-m-d") )
	{
		if( date )
			date->day = 0;

		return -1;
	}

	dt.day = 0;

	dt.year = atoi(str);	str += 5;

	dt.month = atoi(str);

	if( dt.month < 0 || dt.month > 12 )
	{
		if(date)
			date->day = 0;

		return -1;
	}

	if(! dt.month)			/* jan to dec */
	{
		for(i=0;i<12;i++)
		{
			if( upper_strneq(str, month_name[i], 3) )
			{
				dt.month = i+1;
				break;
			}
		}

		if( ! dt.month )
		{
			if(date)
				date->day = 0;

			return -1;
		}

		str += 4;
	}
	else
	{
		if( *str == '0' || dt.month > 9 )	/* 01 to 12 */
			str += 3;
		else					/* 1 to 9 */
			str += 2;
	}

	dt.day = atoi(str);

	if( dt.day <= 0 || dt.day > 31 )
	{
		if(date)
			date->day = 0;

		return -1;
	}

	if(date)
		*date = dt;

	return 0;
}
