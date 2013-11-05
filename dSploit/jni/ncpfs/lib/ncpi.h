/*
    ncpi.h
    Copyright (C) 2001  Petr Vandrovec

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

	1.00  2001, February 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version.

 */

#ifndef __NCPI_H__
#define __NCPI_H__

#include <errno.h>

#ifndef ENOPKG
#define ENOPKG ENOSYS
#endif

#define NCPI_REQUEST_SIGNATURE	0x56614E61	/* VaNa */
#define NCPI_REPLY_SIGNATURE	0x76416E41	/* vAnA */

#define NCPI_OP_GETVERSION	0x00000000	/* get version */
#define NCPI_OP_GETBCASTSTATE	0x00000001	/* retrieve broadcast state */
#define NCPI_OP_SETBCASTSTATE	0x00000002

#define NCPE_BAD_COMMAND	ENOPKG		/* unknown/not implemented command */
#define NCPE_BAD_VALUE		EINVAL		/* invalid argument value */
#define NCPE_SHORT		5000

#endif	/* __NCPI_H__ */
