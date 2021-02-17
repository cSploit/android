/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2010  Craig A. Berry	craigberry@mac.com
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

/*
 * Definitions used by the VMSARG parsing and mapping routines.
 *
 * Based on VMSARG Version 2.0 by Tom Wade <t.wade@vms.eurokom.ei>
 *
 * Extensively revised for inclusion in FreeTDS by Craig A. Berry.
 *
 * From the VMSARG 2.0 documentation:
 *
 * The product is aimed at . . . people who are porting a package from 
 * Unix to VMS. This software is made freely available for inclusion in 
 * such products, whether they are freeware, public domain or commercial. 
 * No licensing is required.
 */

#if __CRTL_VER >= 70302000 && !defined(__VAX)
#define QUAL_LENGTH		(4000+1)
#define S_LENGTH		(4096+1)
#else
#define QUAL_LENGTH		(255+1)
#define S_LENGTH		(1024+1)
#endif

#define MAX_ARGS 255

/*	bit fields for arg flags.
*/

#define VARG_M_AFFIRM		 1
#define VARG_M_NEGATIVE		 2
#define VARG_M_KEYWORDS		 4
#define VARG_M_SEPARATOR	 8
#define VARG_M_DATE		16
#define VARG_M_APPEND		32
#define VARG_M_HELP		64

/*	bit fields for action flags.
*/

#define VARGACT_M_UPPER		1
#define VARGACT_M_LOWER		2
#define VARGACT_M_SPECIAL	4
#define VARGACT_M_ESCAPE	8
#define VARGACT_M_DOUBLE	16
#define VARGACT_M_IMAGE		32
#define VARGACT_M_SYMBOL	64
#define VARGACT_M_COMMAND	128
#define VARGACT_M_RETURN	256
#define VARGACT_M_PROTECT	512
#define VARGACT_M_UNIXARG	1024

#define VARGACT_M_PROTMASK	1+2+4+8+16
