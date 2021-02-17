/*
    dsqueue.c - Find print queue.
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
		Initial release, from nprint.c 2000/01/15 change.

 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "dsqueue.h"

#ifdef NDS_SUPPORT
NWDSCCODE try_nds_queue(NWCONN_HANDLE* conn, const char* name, NWObjectID* id) {
	NWDSContextHandle ctx;
	NWDSCCODE err;
	nuint32 flags;
	char host[MAX_DN_CHARS+1];
	NWCONN_HANDLE nconn;

	err = NWDSCreateContextHandle(&ctx);
	if (err)
		goto quit;
	err = NWDSAddConnection(ctx, *conn);
	if (err)
		goto freectx;
	err = NWDSGetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		goto freectx;
	flags |= DCV_TYPELESS_NAMES;
	err = NWDSSetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		goto freectx;
	/* if object does not have Host Server property, assume damaged queue...
	   We use provided server in that case... */
	err = NWDSGetObjectHostServerAddress(ctx, name, host, NULL);
	if (err)
		goto simple;
	/* if we cannot connect to target server, ignore it... */
	err = NWDSOpenConnToNDSServer(ctx, host, &nconn);
	if (err)
		goto simple;
	err = NWDSAuthenticateConn(ctx, nconn);
	if (err) {
		NWCCCloseConn(nconn);
		goto freectx;
	}
	/* swap connections to get to right server */
	NWCCCloseConn(*conn);
	*conn = nconn;
simple:;		
	err = NWDSMapNameToID(ctx, *conn, name, id);
freectx:;
	NWDSFreeContext(ctx);
quit:;
	return err;
}
#else
NWDSCCODE try_nds_queue(NWCONN_HANDLE* conn, const char* name, NWObjectID* id) {
	return NWE_NCP_NOT_SUPPORTED;
}
#endif

