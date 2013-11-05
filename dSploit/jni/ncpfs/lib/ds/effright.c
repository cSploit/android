/*
    effright.c
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

	1.00  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

static NWDSCCODE __NWDSGetEffectiveRightsV0(
		NWCONN_HANDLE	conn,
		NWObjectID	objectID,
		Buf_T*		subjectAndAttr,
		nuint32*	privileges
) {
	NW_FRAGMENT rq_frag[2];
	NW_FRAGMENT rp_frag[1];
	nuint8 rq_b[8];
	nuint8 rp_b[4];
	NWDSCCODE dserr;
	
	DSET_LH(rq_b, 0, 0);
	DSET_HL(rq_b, 4, objectID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 8;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(subjectAndAttr, &rq_frag[1].fragSize);
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	dserr = NWCFragmentRequest(conn, DSV_GET_EFFECTIVE_RIGHTS, 
		2, rq_frag, 1, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[0].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	if (privileges)
		*privileges = DVAL_LH(rp_b, 0);
	return 0;
}

NWDSCCODE NWDSGetEffectiveRights(
		NWDSContextHandle	ctx,
		const NWDSChar*		subjectName,
		const NWDSChar*		objectName,
		const NWDSChar*		attrName,
		nuint32*		privileges
) {
	NWCONN_HANDLE conn;
	NWObjectID objectID;
	NWDSCCODE dserr;
	Buf_T buf;
	char spare[(MAX_DN_BYTES + 4) * 2];
		
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = NWDSCtxBufDN(ctx, &buf, subjectName);
	if (dserr)
		return dserr;
	dserr = NWDSCtxBufString(ctx, &buf, attrName);
	if (dserr)
		return dserr;
	dserr = NWDSResolveName2(ctx, objectName, 
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_READABLE,
			&conn, &objectID);
	if (dserr)
		return dserr;
	dserr = __NWDSGetEffectiveRightsV0(conn, objectID, &buf, privileges);
	NWCCCloseConn(conn);
	return dserr;	
}
