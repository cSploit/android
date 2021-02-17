/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 * Copyright (C) 2011  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef SQLFRONT_h
#define SQLFRONT_h

#include "./sybfront.h"

static const char rcsid_sqlfront_h[] = "$Id: sqlfront.h,v 1.10 2011-07-13 11:06:31 freddy77 Exp $";
static const void *const no_unused_sqlfront_h_warn[] = { rcsid_sqlfront_h, no_unused_sqlfront_h_warn };

typedef DBPROCESS * PDBPROCESS;
typedef LOGINREC  * PLOGINREC;
typedef DBCURSOR  * PDBCURSOR;

typedef       int  *	LPINT;
typedef       char *	LPSTR;
#if !defined(PHP_MSSQL_H) || !defined(PHP_MSSQL_API)
typedef       BYTE *	LPBYTE;
#endif
typedef       void *	LPVOID;
typedef const char *	LPCSTR;

typedef const LPINT          LPCINT;
#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *         LPCBYTE;
#endif
typedef       USHORT *       LPUSHORT;
typedef const LPUSHORT       LPCUSHORT;
typedef       DBINT *        LPDBINT;
typedef const LPDBINT        LPCDBINT;
typedef       DBBINARY *     LPDBBINARY;
typedef const LPDBBINARY     LPCDBBINARY;
typedef       DBDATEREC *    LPDBDATEREC;
typedef const LPDBDATEREC    LPCDBDATEREC;
typedef       DBDATETIME *   LPDBDATETIME;
typedef const LPDBDATETIME   LPCDBDATETIME;

#endif
