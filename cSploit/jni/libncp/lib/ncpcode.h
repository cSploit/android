/*
    ncpcode.h
    Copyright (C) 1999  Petr Vandrovec

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


    Revision history:

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

 */

#ifndef __NCPCODE_H__
#define __NCPCODE_H__

#define	NCPC_SUBFUNCTION	0x10000

#define NCPC_SUBFN(X)		(X>>8)
#define	NCPC_FN(X)		((X) & 0xFF)

#define NCPC_SFN(FN,SFN)	(FN | ((SFN) << 8) | NCPC_SUBFUNCTION)

#endif	/* __NCPCODE_H__ */

