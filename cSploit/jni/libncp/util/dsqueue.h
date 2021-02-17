/*
    dsqueue.h - Find print queue.
    Copyright (C) 2000-2001 by Petr Vandrovec

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
    
	1.00  2000, February 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

 */

#ifndef __DSQUEUE_H__

#include <ncp/nwnet.h>

NWDSCCODE try_nds_queue(NWCONN_HANDLE* conn, const char* name, NWObjectID* id);

#endif /* __DSQUEUE_H__ */

