/*
    bindctx.c - NWDSGetBinderyContext, NWDSReloadDS
    Copyright (C) 2000  Petr Vandrovec

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

	1.00  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version. NWDSGetBinderyContext and NWDSReloadDS.
		
*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

static NWDSCCODE __NWDSGetBinderyContext(
		NWCONN_HANDLE	conn,
		Buf_T*		binderyEmulationContext
) {
	nuint8 rqbuffer[1];
	NW_FRAGMENT rp;
	NWDSCCODE dserr;

	BSET(rqbuffer, 0, DS_NCP_BINDERY_CONTEXT);
	
	NWDSBufStartPut(binderyEmulationContext, DSV_GET_BINDERY_CONTEXTS);
	rp.fragAddr.rw = NWDSBufPutPtrLen(binderyEmulationContext,
				&rp.fragSize);
	dserr = NWRequestSimple(conn, DS_NCP_VERB, rqbuffer, 1, &rp);
	if (dserr)
		return dserr;
	NWDSBufPutSkip(binderyEmulationContext, rp.fragSize);
	NWDSBufFinishPut(binderyEmulationContext);
	return 0;
}
	
NWDSCCODE NWDSGetBinderyContext(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		NWDSChar*		binderyEmulationContext
) {
	NWDSCCODE dserr;
	char spare[4 + MAX_DN_BYTES];
	Buf_T buf;
	
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = __NWDSGetBinderyContext(conn, &buf);
	if (dserr)
		return dserr;
	return NWDSBufCtxString(ctx, &buf, binderyEmulationContext,
			MAX_DN_BYTES, NULL);
}

static NWDSCCODE __NWDSReloadDS(
		NWCONN_HANDLE		conn
) {
	nuint8 rqbuffer[1];
	nuint8 rpbuffer[8];
	NW_FRAGMENT rp;
	NWDSCCODE dserr;

	BSET(rqbuffer, 0, DS_NCP_RELOAD);
	
	rp.fragAddr.rw = rpbuffer;
	rp.fragSize = sizeof(rpbuffer);
	dserr = NWRequestSimple(conn, DS_NCP_VERB, rqbuffer, 1, &rp);
	if (dserr)
		return dserr;
	if (rp.fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	dserr = DVAL_LH(rpbuffer, 0);
	if (dserr < 0 && dserr > -256)
		dserr = 0x8900 - dserr;
	return dserr;
}

NWDSCCODE NWDSReloadDS(
		NWDSContextHandle	ctx,
		const NWDSChar*		serverName
) {
	NWCONN_HANDLE conn;
	NWDSCCODE dserr;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSReloadDS(conn);
	NWCCCloseConn(conn);
	return dserr;
}

