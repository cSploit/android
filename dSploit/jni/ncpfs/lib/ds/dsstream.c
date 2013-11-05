/*
    dsstream.c
    Copyright (C) 2002  Petr Vandrovec

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

	1.00  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.
		
	1.01  2002, January 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Require writeable replica.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

static NWDSCCODE __NWDSOpenStreamV0(
		NWCONN_HANDLE	conn,
		nuint32		qflags,
		NWObjectID	objectID,
		Buf_T*		attrName,
		nuint32*	fileHandle,
		nuint32*	fileSize) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[2];
	NW_FRAGMENT rp_frag[1];
	unsigned char rq_b[12];
	unsigned char rp_b[8];

	DSET_LH(rq_b, 0, 0); 		/* version */
	DSET_LH(rq_b, 4, qflags);
	DSET_HL(rq_b, 8, objectID);
	
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = sizeof(rq_b);
	
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrName, &rq_frag[1].fragSize);
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	err = NWCFragmentRequest(conn, DSV_OPEN_STREAM, 2, rq_frag, 1, rp_frag, NULL);
	if (err)
		return err;
	if (rp_frag[0].fragSize < 8)
		return ERR_INVALID_SERVER_RESPONSE;
	*fileHandle = DVAL_LH(rp_b, 0);
	*fileSize = DVAL_LH(rp_b, 4);
	return 0;
}

NWDSCCODE __NWDSOpenStream(
		NWDSContextHandle ctx,
		const NWDSChar*	objectName,
		const NWDSChar* attrName,
		nflag32		flags,
		NWCONN_HANDLE*  rconn,
		char		fh[6],
		ncp_off64_t*	size) {
	NWDSCCODE err;
        char rq_b[DEFAULT_MESSAGE_LEN];
	NWCONN_HANDLE conn;
	NWObjectID objID;
        Buf_T bufAttrName;
	nuint32 fh32;
	nuint32 size32;

	if (!objectName || !attrName || !rconn || !fh) {
		return ERR_NULL_POINTER;
	}
        NWDSSetupBuf(&bufAttrName, rq_b, sizeof(rq_b));
	err = NWDSCtxBufString(ctx, &bufAttrName, attrName);
	if (err)
		return err;
	err = NWDSResolveName2(ctx, objectName, DS_RESOLVE_WRITEABLE, &conn, &objID);
	if (err)
		return err;
	err = __NWDSOpenStreamV0(conn, flags, objID, &bufAttrName, &fh32, &size32);
	if (!err) {
		ConvertToNWfromDWORD(fh32, fh);
		*rconn = conn;
		if (size) {
			*size = size32;
		}
	} else {
		NWCCCloseConn(conn);
	}
	return err;
}
