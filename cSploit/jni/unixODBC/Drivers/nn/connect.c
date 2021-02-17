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

void*	nnodbc_getenverrstack(void* henv)
{
	return ((env_t*)henv)->herr;
}

void*	nnodbc_getdbcerrstack(void* hdbc)
{
	return ((dbc_t*)hdbc)->herr;
}

void	nnodbc_pushdbcerr( void* hdbc, int code, char* msg )
{
	PUSHSYSERR( ((dbc_t*)hdbc)->herr, code, msg );
}

void*	nnodbc_getnntpcndes( void* hdbc )
{
	return ((dbc_t*)hdbc)->hcndes;
}



int	nnodbc_attach_stmt(void* hdbc, void* hstmt)
{
	dbc_t*		pdbc = hdbc;
	gstmt_t*	pstmt;

	pstmt = (gstmt_t*)MEM_ALLOC(sizeof(gstmt_t));

	if( ! pstmt )
	{
		PUSHSQLERR( pdbc->herr, en_S1001 );
		return SQL_ERROR;
	}

	pstmt->next = pdbc->stmt;
	pdbc->stmt  = pstmt;

	pstmt->hstmt= hstmt;
	pstmt->hdbc = pdbc;

	return SQL_SUCCESS;
}

int	nnodbc_detach_stmt(void* hdbc, void* hstmt)
{
	dbc_t*		pdbc = hdbc;
	gstmt_t*	pstmt;
	gstmt_t*	ptr;

	for(pstmt=pdbc->stmt;pstmt;pstmt = pstmt->next)
	{
		if(pstmt->hstmt == hstmt)
		{
			pdbc->stmt = pstmt->next;
			MEM_FREE(pstmt);
			return 0;
		}

		if(pstmt->next->hstmt == hstmt )
		{
			ptr = pstmt->next;
			pstmt->next = ptr->next;
			MEM_FREE(ptr);
			return 0;
		}
	}

	return -1;
}



char*			/* return new position in input str */
readtoken(
	char*	istr,		/* old position in input buf */
	char*	obuf )		/* token string ( if "\0", then finished ) */
{
	for(; *istr && *istr != '\n' ; istr ++ )
	{
		char c, nx;

		c = *(istr);

		if( c == ' ' || c == '\t' )
		{
			continue;
		}

		nx = *(istr + 1);

		*obuf = c;
		obuf ++;

		if( c == ';' || c == '=' )
		{
			istr ++;
			break;
		}

		if( nx == ' ' || nx == '\t' || nx == ';' || nx == '=' )
		{
			istr ++;
			break;
		}
	}

	*obuf = '\0';

	return istr;
}

#if	!defined(WINDOWS) && !defined(WIN32) && !defined(OS2)
# include	<pwd.h>
# define	UNIX_PWD
#endif

char*
getinitfile(char* buf, int size)
{
	int	i, j;
	char*	ptr;

	j = STRLEN("/odbc.ini") + 1;

	if( size < j )
	{
		return NULL;
	}

#if	!defined(UNIX_PWD)

	i = GetWindowsDirectory((LPSTR)buf, size );

	if( i == 0 || i > size - j )
	{
		return NULL;
	}

	sprintf( buf + i, "/odbc.ini");

	return buf;
#else
	ptr = (char*)getpwuid(getuid());

	if( ptr == NULL )
	{
		return NULL;
	}

	ptr = ((struct passwd*)ptr)->pw_dir;

	if( ptr == NULL || *ptr == '\0' )
	{
		ptr = "/home";
	}

	if( size < STRLEN(ptr) + j )
	{
		return NULL;
	}

	sprintf( buf, "%s%s", ptr, "/.odbc.ini");
	/* i.e. searching ~/.odbc.ini */
#endif

	return buf;
}

char*	getkeyvalbydsn(
		char*	dsn,
		int	dsnlen,
		char*	keywd,
		char*	value,
		int	size )
/* read odbc init file to resolve the value of specified
 * key from named or defaulted dsn section
 */
{
	char	buf[1024];
	char	dsntk[SQL_MAX_DSN_LENGTH + 3] = { '[', '\0'  };
	char	token[1024];	/* large enough */
	FILE*	file;
	char	pathbuf[1024];
	char*	path;

#define DSN_NOMATCH	0
#define DSN_NAMED	1
#define DSN_DEFAULT	2

	int	dsnid = DSN_NOMATCH;
	int	defaultdsn = DSN_NOMATCH;

	if( dsn == NULL || *dsn == 0 )
	{
		dsn = "default";
		dsnlen = STRLEN(dsn);
	}

	if( dsnlen == SQL_NTS )
	{
		dsnlen = STRLEN(dsn);
	}

	if( dsnlen <= 0 || keywd == NULL || buf == 0 || size <= 0 )
	{
		return NULL;
	}

	if( dsnlen > sizeof(dsntk) - 2	)
	{
		return NULL;
	}

	STRNCAT( dsntk, dsn, dsnlen );
	STRCAT( dsntk, "]" );

	value[0] = 0;

	dsnlen = dsnlen + 2;

	path = getinitfile(pathbuf, sizeof(pathbuf));

	if( path == NULL )
	{
		return NULL;
	}

	file = (FILE*)fopen(path, "r");

	if( file == NULL )
	{
		return NULL;
	}

	for(;;)
	{
		char*	str;

		str = fgets(buf, sizeof(buf), file);

		if( str == NULL )
		{
			break;
		}

		if( *str == '[' )
		{
			if( upper_strneq(str, "[default]", STRLEN("[default]")) )
			{
				/* we only read first dsn default dsn
				 * section (as well as named dsn).
				 */
				if( defaultdsn == DSN_NOMATCH )
				{
					dsnid = DSN_DEFAULT;
					defaultdsn = DSN_DEFAULT;
				}
				else
				{
					dsnid = DSN_NOMATCH;
				}

				continue;
			}
			else if( upper_strneq( str, dsntk, dsnlen ) )
			{
				dsnid = DSN_NAMED;
			}
			else
			{
				dsnid = DSN_NOMATCH;
			}

			continue;
		}
		else if( dsnid == DSN_NOMATCH )
		{
			continue;
		}

		str = readtoken(str, token);

		if( upper_strneq( keywd, token, STRLEN(keywd)) )
		{
			str = readtoken(str, token);

			if( ! STREQ( token, "=") )
			/* something other than = */
			{
				continue;
			}

			str = readtoken(str, token);

			if( STRLEN(token) > size - 1)
			{
				break;
			}

			STRNCPY(value, token, size);
			/* copy the value(i.e. next token) to buf */

			if( dsnid != DSN_DEFAULT )
			{
				break;
			}
		}
	}

	fclose(file);

	return (*value)? value:NULL;
}

char*	getkeyvalinstr(
		char*	cnstr,
		int	cnlen,
		char*	keywd,
		char*	value,
		int	size )
{
	char	token[1024] = { '\0' };
	int	flag = 0;

	if( cnstr == NULL || value == NULL
	 || keywd == NULL || size < 1 )
	{
		return NULL;
	}

	if( cnlen == SQL_NTS )
	{
		cnlen = STRLEN (cnstr);
	}

	if( cnlen <= 0 )
	{
		return NULL;
	}

	for(;;)
	{
		cnstr = readtoken(cnstr, token);

		if( *token == '\0' )
		{
			break;
		}

		if( STREQ( token, ";" ) )
		{
			flag = 0;
			continue;
		}

		switch(flag)
		{
			case 0:
				if( upper_strneq(token, keywd, strlen(keywd)) )
				{
					flag = 1;
				}
				break;

			case 1:
				if( STREQ( token, "=" ) )
				{
					flag = 2;
				}
				break;

			case 2:
				if( size < strlen(token) + 1 )
				{
					return NULL;
				}

				STRNCPY( value, token, size );

				return value;

			default:
				break;
		}
	}

	return	NULL;
}



