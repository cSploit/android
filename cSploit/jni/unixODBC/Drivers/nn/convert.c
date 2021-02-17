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

/* It looks silly to use a MEM_ALLOC() in function char2str(), etc,
 * for converting C data type into STRING SQL data type. Esspecially
 * for char2str(), simply return the C data buffer address seems can
 * do the same thing. However, all these 'to STRING' functions are
 * assumed to be invoked in SQLExecute(), etc. According to ODBC, after
 * that, user buffers are detached from the statement been executed.
 * For example, for a statement:
 *
 *  "SELECT FROM comp.databases WHERE subject LIKE ?"
 *
 * You may bind a SQL_C_CHAR data to it as the pattern for the
 * LIKE condition. After SQLExecute(), the user buffer should be
 * detached from the statement. You can change the data in that
 * buffer without worry about the SQLFetch() results will be
 * affected. If we don't use a separate buffer to keep the user
 * data at SQLExecute() time but only keep a pointer of user buffer,
 * then when user changes data in user buffer after SQLExecute()
 * but before or during SQLFetch()s, the SQLFetch()'s result will
 * be affected as a result of the change of LIKE condition.
 *
 * These allocated memory will be released by SQL layer.
 */

typedef DATE_STRUCT	odbc_date_t;

char*	char2str( char* buf, int size )
{
	char*	ptr;
	int	len;

	if( size < 0 )
		size = STRLEN(buf);

	ptr = (char*)MEM_ALLOC(size + 1);

	if( ! ptr )
		return (char*)(-1);

	STRNCPY(ptr, buf, size+1);
	ptr[size] = 0;

	return ptr;
}

char*	char2num( char* buf, int size)
{
	char	tbuf[16];

	if( size < 0 )
		size = strlen(buf);

	if( size>15 )
		size = 15;

	STRNCPY(tbuf, buf, size);

	tbuf[15] = 0;

	return (char*)atol(tbuf);
}

char*	char2date( char* buf, int size, date_t* dt)
{
	char	tbuf[16];

	if( size < 0 )
		size = strlen(buf);

	if( size > 15 )
		size = 15;

	STRNCPY( tbuf, buf, size);

	tbuf[15] = 0;

	if( nnsql_odbcdatestr2date(tbuf, dt) )
		return (char*)(-1);

	return (char*)dt;
}

char*	date2str( odbc_date_t* dt )
{
	char*	ptr;

	if( dt->year < 0 || dt->year > 9999
	 || dt->month< 1 || dt->month> 12
	 || dt->day  < 1 || dt->day  > 31
	 || !( ptr = (char*)MEM_ALLOC(12)) )
		return (char*)(-1);

	sprintf(ptr, "%04d-%02d-%02d", dt->year, dt->month, dt->day);

	return ptr;
}

char*	odate2date( odbc_date_t* odt, int size, date_t* dt )
{
	if( dt->year < 0 || dt->year > 9999
	 || dt->month< 1 || dt->month> 12
	 || dt->day  < 1 || dt->day  > 31 )
		return (char*)(-1);

	dt->year = odt->year;
	dt->month= odt->month;
	dt->day  = odt->day;

	return (char*)dt;
}

char*	tint2num( char* d )
{
	long	num = *d;

	return (char*)num;
}

char*	short2num( short* d )
{
	long	num = *d;

	return (char*)num;
}

char*	long2num( long* d )
{
	return (char*)*d;
}

char*	tint2str( char* d )
{
	int	c = *d;
	char*	ptr;

	if( ! (ptr = MEM_ALLOC(5)) )
		return (char*)(-1);

	sprintf(ptr, "%d", c);

	return ptr;
}

char*	short2str( short* d )
{
	int	c = *d;
	char*	ptr;

	if( ! (ptr = MEM_ALLOC(32)) )
		return (char*)(-1);

	sprintf(ptr, "%d", c);

	return ptr;
}

char*	long2str( long* d )
{
	long	l = *d;
	char*	ptr;

	if( ! (ptr = MEM_ALLOC(64)) )
		return (char*)(-1);

	sprintf(ptr, "%ld", l);

	return ptr;
}

static fptr_t	c2sql_cvt_tab[5][3] =
{
	char2str,  char2num, char2date,
	tint2str,  tint2num, 0,
	short2str, short2num,0,
	long2str,  long2num, 0,
	date2str,  0,	     odate2date
};

static	struct {
	int	ctype;
	int	idx;
} ctype_idx_tab[] =
{
	SQL_C_CHAR,	0,

	SQL_C_TINYINT,	1,
	SQL_C_STINYINT, 1,
	SQL_C_UTINYINT, 1,

	SQL_C_SHORT,	2,
	SQL_C_SSHORT,	2,
	SQL_C_USHORT,	2,

	SQL_C_LONG,	3,
	SQL_C_SLONG,	3,
	SQL_C_ULONG,	3,

	SQL_C_DATE,	4
};

static	struct {
	int	sqltype;
	int	idx;
} sqltype_idx_tab[] =
{
	SQL_CHAR,	0,
	SQL_VARCHAR,	0,
	SQL_LONGVARCHAR,0,

	SQL_TINYINT,	1,
	SQL_SMALLINT,	1,
	SQL_INTEGER,	1,

	SQL_DATE,	2
};

fptr_t	nnodbc_get_c2sql_cvt(int ctype, int sqltype)
{
	int	i, cidx = -1, sqlidx = -1;

	for(i=0; i< sizeof(ctype_idx_tab) / sizeof(ctype_idx_tab[ 0 ]); i++ )
	{
		if( ctype_idx_tab[i].ctype == ctype )
		{
			cidx = ctype_idx_tab[i].idx;
			break;
		}
	}

	if( cidx == -1 )
		return 0;

	for(i=0; i< sizeof(sqltype_idx_tab) / sizeof(sqltype_idx_tab[ 0 ]); i++ )
	{
		if( sqltype_idx_tab[i].sqltype == sqltype )
		{
			sqlidx = sqltype_idx_tab[i].idx;
			break;
		}
	}

	if( sqlidx == -1 )
		return 0;

	return	c2sql_cvt_tab[cidx][sqlidx];
}

static char*	str2char( char* ptr, char* buf, long size, long* psize)
{
	long	len;

	len = STRLEN(ptr) + 1;

	if( len > size )
		len = size;

	if( len )
	{
		STRNCPY( buf, ptr, len );
		buf[len - 1] = 0;
	}

	*psize = len;

	return 0;
}

static char*	str2tint( char* ptr, char* buf, long size, long* psize )
{
	unsigned long	a, b = (unsigned char)(-1);

	a = (unsigned long)atol(ptr);

	if( a > b )
	{
		*psize = a;
		return (char*)(-1);
	}

	*buf = (char)a;

	return 0;
}

static char*	str2short( char* ptr, short* buf, long size, long* psize)
{
	unsigned long a, b = (unsigned short)(-1);

	a = atoi(ptr);

	if( a > b )
	{
		*psize = a;
		return (char*)(-1);
	}

	*buf = (short)a;

	return 0;
}

static char*	str2long( char* ptr, long* buf)
{
	*buf = atol(ptr);
	return 0;
}

static char*	str2date( char* ptr, odbc_date_t* buf )
{
	date_t	dt;

	if( nnsql_nndatestr2date(ptr, &dt) )
		return (char*)(-1);

	buf->year = dt.year;
	buf->month= dt.month;
	buf->day  = dt.day;

	return 0;
}

static char*	num2char( long x, char* buf, long size, long* psize)
{
	char	tbuf[48];

	sprintf(tbuf, "%ld", x);

	*psize = STRLEN(buf) + 1;

	if(*psize > size )
		return (char*)(-1);

	STRCPY(buf, tbuf);

	return 0;
}

static char*	num2tint(long x, char* buf, long size, long* psize)
{
	unsigned long	a = x, b = (unsigned char)(-1);

	if( a > b )
	{
		*psize = x;
		return (char*)(-1);
	}

	*buf = (char)x;

	return 0;
}

static char*	num2short(long x, short* buf, long size, long* psize)
{
	unsigned long a = x, b = (unsigned short)(-1);

	if( a > b)
	{
		*psize = x;
		return (char*)(-1);
	}

	*buf = (short)x;

	return 0;
}

static char*	num2long(long x, long* buf )
{
	*buf = x;

	return 0;
}

static char*	date2char(date_t* dt, char* buf, long size, long* psize)
{
	*psize = 11;

	if(size < 11)
		return (char*)(-1);

	sprintf(buf, "%04d-%02d-%02d", dt->year, dt->month, dt->day);

	return 0;
}

static char*	date2odate(date_t* dt, odbc_date_t* buf)
{
	buf->year = dt->year;
	buf->month= dt->month;
	buf->day  = dt->day;

	return 0;
}

static	fptr_t	sql2c_cvt_tab[3][5] =
{
	str2char,  str2tint, str2short, str2long, str2date,
	num2char,  num2tint, num2short, num2long, 0,
	date2char, 0,	     0, 	0,	  date2odate
};

fptr_t	nnodbc_get_sql2c_cvt(int sqltype, int ctype)
{
	int	i, cidx = -1, sqlidx = -1;

	for(i=0; i< sizeof(ctype_idx_tab) / sizeof(ctype_idx_tab[ 0 ]); i++ )
	{
		if( ctype_idx_tab[i].ctype == ctype )
		{
			cidx = ctype_idx_tab[i].idx;
			break;
		}
	}

	if( cidx == -1 )
		return 0;

	for(i=0; i< sizeof(sqltype_idx_tab) / sizeof(sqltype_idx_tab[ 0 ]); i++ )
	{
		if( sqltype_idx_tab[i].sqltype == sqltype )
		{
			sqlidx = sqltype_idx_tab[i].idx;
			break;
		}
	}

	if( sqlidx == -1 )
		return 0;

	return	sql2c_cvt_tab[sqlidx][cidx];
}
