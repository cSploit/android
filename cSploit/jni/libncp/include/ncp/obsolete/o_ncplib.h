/*
    ncplib.h
    Copyright (C) 1995, 1996 by Volker Lendecke
    Copyright (C) 1997-2001  Petr Vandrovec

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#ifndef _OBSOLETE_NCPLIB_H
#define _OBSOLETE_NCPLIB_H

#ifndef _NCPLIB_H
#error "Do not include this file directly, compile your project with -DNCP_OBSOLETE instead"
#endif

#ifdef __cplusplus
extern "C" {
#endif

long
ncp_renegotiate_connparam(NWCONN_HANDLE conn, int buffsize, int options);

#ifdef __cplusplus
}
#endif

#endif	/* _NCPLIB_H */
