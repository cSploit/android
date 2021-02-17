/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-2011  Brian Bruns
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

#ifndef _tds_sysdep_public_h_
#define _tds_sysdep_public_h_

/* $Id: tds_sysdep_public.h.in,v 1.15 2011-08-08 07:27:57 freddy77 Exp $ */

#ifdef __cplusplus
extern "C"
{
#endif

/*
** This is where platform-specific changes need to be made.
*/
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <windows.h>
#define tds_sysdep_int16_type short	/* 16-bit int */
#define tds_sysdep_int32_type int	/* 32-bit int */
#define tds_sysdep_int64_type __int64	/* 64-bit int */
#define tds_sysdep_real32_type float	/* 32-bit real */
#define tds_sysdep_real64_type double	/* 64-bit real */
#if !defined(WIN64) && !defined(_WIN64)
#define tds_sysdep_intptr_type int      /* 32-bit int */
#else
#define tds_sysdep_intptr_type __int64  /* 64-bit int */
#endif
#endif				/* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) */

#ifndef tds_sysdep_int16_type
#define tds_sysdep_int16_type short	/* 16-bit int */
#endif				/* !tds_sysdep_int16_type */

#ifndef tds_sysdep_int32_type
#define tds_sysdep_int32_type int	/* 32-bit int */
#endif				/* !tds_sysdep_int32_type */

#ifndef tds_sysdep_int64_type
#define tds_sysdep_int64_type long long	/* 64-bit int */
#endif				/* !tds_sysdep_int64_type */

#ifndef tds_sysdep_real32_type
#define tds_sysdep_real32_type float	/* 32-bit real */
#endif				/* !tds_sysdep_real32_type */

#ifndef tds_sysdep_real64_type
#define tds_sysdep_real64_type double	/* 64-bit real */
#endif				/* !tds_sysdep_real64_type */

#ifndef tds_sysdep_intptr_type
#define tds_sysdep_intptr_type int
#endif				/* !tds_sysdep_intptr_type */

#if !defined(MSDBLIB) && !defined(SYBDBLIB)
#define SYBDBLIB 1
#endif
#if defined(MSDBLIB) && defined(SYBDBLIB)
#error MSDBLIB and SYBDBLIB cannot both be defined
#endif

#ifdef __cplusplus
}
#endif

#endif				/* _tds_sysdep_public_h_ */
