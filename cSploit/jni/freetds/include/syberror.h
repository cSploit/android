/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
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

#ifndef _syberror_h_
#define _syberror_h_

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

static const char rcsid_syberror_h[] = "$Id: syberror.h,v 1.4 2004-10-28 12:42:12 freddy77 Exp $";
static const void *const no_unused_syberror_h_warn[] = { rcsid_syberror_h, no_unused_syberror_h_warn };

/* severity levels, gleaned from google */
#define EXINFO         1
#define EXUSER         2
#define EXNONFATAL     3
#define EXCONVERSION   4
#define EXSERVER       5
#define EXTIME         6
#define EXPROGRAM      7
#define EXRESOURCE     8
#define EXCOMM         9
#define EXFATAL       10
#define EXCONSISTENCY 11

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
