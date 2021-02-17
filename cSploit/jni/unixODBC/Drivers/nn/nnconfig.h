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
#ifndef _CONFIG_H
#define _CONFIG_H

# if	!defined(WINDOWS) && !defined(WIN32_SYSTEM)
#  define	_UNIX_

#  include	<stdlib.h>
#  include	<errno.h>
#  include	<sys/types.h>

#  define	MEM_ALLOC(size) (malloc((size_t)(size)))
#  define	MEM_FREE(ptr)	{if(ptr) free(ptr);}
#  define	MEM_REALLOC(ptr, size)	(ptr? realloc(ptr, size):malloc(size))

#  define	STRCPY(t, s)	(strcpy((char*)(t), (char*)(s)))
#  define	STRNCPY(t,s,n)	(strncpy((char*)(t), (char*)(s), (size_t)(n)))
#  define	STRCAT(t, s)	(strcat((char*)(t), (char*)(s)))
#  define	STRNCAT(t,s,n)	(strncat((char*)(t), (char*)(s), (size_t)(n)))
#  define	STRCMP(a, b)	strcmp((char*)(a), (char*)(b))
#  define	STRNCMP(a,b,n)	strncmp((char*)(a), (char*)(b), n)
#  define	STREQ(a, b)	(strcmp((char*)(a), (char*)(b)) == 0)
#  define	STRNEQ(a,b,n)	(strncmp((char*)(a), (char*)(b), n)==0)
#  define	STRLEN(str)	((str)? strlen((char*)(str)):0)

#  define	MEM_SET(p,c,n)	(memset((char*)p, (int)c, (int)n))

#  define	EXPORT
#  define	CALLBACK
#  define	FAR

   typedef	signed short	SSHORT;
   typedef	short		WORD;
   typedef	long		DWORD;

   typedef	WORD		WPARAM;
   typedef	DWORD		LPARAM;
   typedef	void*		HWND;
   typedef	int		BOOL;
# endif /* _UNIX_ */

/* for nntp driver */

# ifdef WINSOCK

#  define	SOCK_CLOSE(sd)			closesocket(sock)
#  define	SOCK_FDOPEN(fd, mode)		wsa_fdopen(fd, mode)
#  define	SOCK_FCLOSE(stream)		wsa_fclose(stream)
#  define	SOCK_FGETS(buf, size, stream)	wsa_fgets(buf, size, stream)
#  define	SOCK_FPUTS(str, stream) 	wsa_fputs(str, stream)
#  define	SOCK_FPRINTF			wsa_fprintf
#  define	SOCK_FFLUSH(stream)		wsa_fflush(stream)

# else

# include	<stdio.h>

#  define	SOCK_CLOSE(sd)			close(sd)
#  define	SOCK_FDOPEN(fd, mode)		fdopen(fd, mode)
#  define	SOCK_FCLOSE(stream)		fclose(stream)
#  define	SOCK_FGETS(buf, size, stream)	fgets(buf, size, stream)
#  define	SOCK_FPUTS(str, stream) 	fputs(str, stream)
#  define	SOCK_FPRINTF			fprintf
#  define	SOCK_FFLUSH(stream)		fflush(stream)

# endif

# if	defined(WINDOWS) || defined(WIN32_SYSTEM)

#  include	<windows.h>
#  include	<windowsx.h>

#  ifdef	_MSVC_
#   define	MEM_ALLOC(size) (fmalloc((size_t)(size)))
#   define	MEM_FREE(ptr)	((ptr)? ffree((PTR)(ptr)):0))
#   define	STRCPY(t, s)	(fstrcpy((char FAR*)(t), (char FAR*)(s)))
#   define	STRNCPY(t,s,n)	(fstrncpy((char FAR*)(t), (char FAR*)(s), (size_t)(n)))
#   define	STRLEN(str)	((str)? fstrlen((char FAR*)(str)):0)
#   define	STREQ(a, b)	(fstrcmp((char FAR*)(a), (char FAR*)(b) == 0)
#  endif

#  ifdef	_BORLAND_
#   define	MEM_ALLOC(size) (farmalloc((unsigned long)(size))
#   define	MEM_FREE(ptr)	((ptr)? farfree((void far*)(ptr)):0)
#   define	STRCPY(t, s)	(_fstrcpy((char FAR*)(t), (char FAR*)(s)))
#   define	STRNCPY(t,s,n)	(_fstrncpy((char FAR*)(t), (char FAR*)(s), (size_t)(n)))
#   define	STRLEN(str)	((str)? _fstrlen((char FAR*)(str)):0)
#   define	STREQ(a, b)	(_fstrcmp((char FAR*)(a), (char FAR*)(b) == 0)
#  endif

# endif /* WINDOWS */

#endif
