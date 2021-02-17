/*
    dsread.c
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
		Initial version, copied from nwnet.c
		Added NWDSListAttrsEffectiveRights.
		Added NWDSExtSyncRead.

	1.01  2000, August 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Removed DS_RESOLVE_WALK_TREE from resolve calls.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

NWDSCCODE __NWDSReadV1(
		NWCONN_HANDLE	conn,
		nuint32		qflags,
		NWObjectID	objectID,
		nuint		infoType,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterhandle,
		Buf_T*		subjectName,
		Buf_T*		objectInfo) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[3];
	NW_FRAGMENT rp_frag[2];
	unsigned char rq_b[28];
	unsigned char rp_b[8];
	size_t rq_frags;

	DSET_LH(rq_b, 0, 1); 		/* version */
	DSET_LH(rq_b, 4, qflags);		/* ctxflag, to do: 1,4,8 */
	DSET_LH(rq_b, 8, *iterhandle);	/* iterhandle */
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, infoType);
	DSET_LH(rq_b, 20, allAttrs);
	
	rq_frag[0].fragAddr.ro = rq_b;
	
	if (!allAttrs && attrNames) {
		rq_frag[0].fragSize = 24;
		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrNames, &rq_frag[1].fragSize);
		rq_frags = 2;
	} else {
		DSET_LH(rq_b, 24, 0);
		rq_frag[0].fragSize = 28;
		rq_frags = 1;
	}
	if (subjectName) {
		rq_frag[rq_frags].fragAddr.ro = NWDSBufRetrieve(subjectName, &rq_frag[rq_frags].fragSize);
		rq_frags++;
	}
	NWDSBufStartPut(objectInfo, DSV_READ);
	err = NWDSBufSetInfoType(objectInfo, infoType);
	if (err)
		return err;
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(objectInfo, &rp_frag[1].fragSize);
	err = NWCFragmentRequest(conn, DSV_READ, rq_frags, rq_frag, 2, rp_frag, NULL);
	if (err)
		return err;
	if (rp_frag[0].fragSize < 8)
		return ERR_INVALID_SERVER_RESPONSE;
	NWDSBufPutSkip(objectInfo, rp_frag[1].fragSize);
	NWDSBufFinishPut(objectInfo);
	*iterhandle = DVAL_LH(rp_b, 0);
	if (DVAL_LH(rp_b, 4) != infoType)
		return ERR_INVALID_SERVER_RESPONSE;
	return 0;
}

static NWDSCCODE __NWDSReadV2(
		NWCONN_HANDLE	conn,
		nuint32		qflags,
		NWObjectID	objectID,
		nuint		infoType,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterhandle,
		Buf_T*		subjectName,
		const TimeStamp_T* timeStamp,
		Buf_T*		objectInfo) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[4];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[28];
	nuint8 rq_b2[8];
	nuint8 rp_b[8];
	size_t rq_frags;

	DSET_LH(rq_b, 0, 2); 		/* version */
	DSET_LH(rq_b, 4, qflags);	/* ctxflag, to do: 1,4,8 */
	DSET_LH(rq_b, 8, *iterhandle);	/* iterhandle */
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, infoType);
	DSET_LH(rq_b, 20, allAttrs);
	
	DSET_LH(rq_b2, 0, timeStamp->wholeSeconds);
	WSET_LH(rq_b2, 4, timeStamp->replicaNum);
	WSET_LH(rq_b2, 6, timeStamp->eventID);

	rq_frag[0].fragAddr.ro = rq_b;
	
	if (!allAttrs && attrNames) {
		rq_frag[0].fragSize = 24;
		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrNames, &rq_frag[1].fragSize);
		rq_frags = 2;
	} else {
		DSET_LH(rq_b, 24, 0);
		rq_frag[0].fragSize = 28;
		rq_frags = 1;
	}
	if (subjectName) {
		rq_frag[rq_frags].fragAddr.ro = NWDSBufRetrieve(subjectName, &rq_frag[rq_frags].fragSize);
		rq_frags++;
	}

	rq_frag[rq_frags].fragAddr.ro = rq_b2;
	rq_frag[rq_frags].fragSize = 8;
	rq_frags++;

	NWDSBufStartPut(objectInfo, DSV_READ);
	err = NWDSBufSetInfoType(objectInfo, infoType);
	if (err)
		return err;
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(objectInfo, &rp_frag[1].fragSize);
	err = NWCFragmentRequest(conn, DSV_READ, rq_frags, rq_frag, 2, rp_frag, NULL);
	if (err)
		return err;
	if (rp_frag[0].fragSize < 8)
		return ERR_INVALID_SERVER_RESPONSE;
	NWDSBufPutSkip(objectInfo, rp_frag[1].fragSize);
	NWDSBufFinishPut(objectInfo);
	*iterhandle = DVAL_LH(rp_b, 0);
	if (DVAL_LH(rp_b, 4) != infoType)
		return ERR_INVALID_SERVER_RESPONSE;
	return 0;
}

static NWDSCCODE __NWDSRead(
		NWDSContextHandle ctx,
		const NWDSChar*	objectName,
		nuint		infoType,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterHandle,
		Buf_T*		subjectName,
		const TimeStamp_T* timeStamp,
		Buf_T*		objectInfo) {
	NWDSCCODE err;
	nuint32 ctxflags;
	nuint32 qflags = 0;
	struct wrappedIterationHandle* ih = NULL;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	nuint32 lh;

	if (attrNames && attrNames->operation != DSV_READ)
		return ERR_BAD_VERB;
	err = NWDSGetContext(ctx, DCK_FLAGS, &ctxflags);
	if (err)
		return err;
	if (ctxflags & DCV_TYPELESS_NAMES)
		qflags |= 0x0001;
	qflags |= ctx->dck.name_form << 1;
	if (*iterHandle == NO_MORE_ITERATIONS) {
		err = NWDSResolveName2(ctx, objectName, DS_RESOLVE_READABLE, 
			&conn, &objID);
		if (err)
			return err;
		lh = NO_MORE_ITERATIONS;
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_READ);
		if (!ih)
			return ERR_INVALID_HANDLE;
		lh = ih->iterHandle;
		objID = ih->objectID;
		conn = ih->conn;
	}
	if (timeStamp)
		err = __NWDSReadV2(conn, qflags, objID, infoType, allAttrs,
				attrNames, &lh, subjectName, timeStamp,
				objectInfo);
	else
		err = __NWDSReadV1(conn, qflags, objID, infoType, allAttrs, 
				attrNames, &lh, subjectName, objectInfo);
	if (ih) {
		return __NWDSIHUpdate(ih, err, lh, iterHandle);
	}
	return __NWDSIHCreate(err, conn, objID, lh, DSV_READ, iterHandle);
}

NWDSCCODE NWDSExtSyncRead(
		NWDSContextHandle ctx,
		const NWDSChar*	objectName,
		nuint		infoType,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterHandle,
		const TimeStamp_T* timeStamp,
		Buf_T*		objectInfo
) {
	if (infoType == DS_EFFECTIVE_PRIVILEGES)
		return ERR_INVALID_REQUEST;
	return __NWDSRead(ctx, objectName, infoType, allAttrs, attrNames,
			iterHandle, NULL, timeStamp, objectInfo);
}

NWDSCCODE NWDSRead(
		NWDSContextHandle ctx,
		const NWDSChar*	objectName,
		nuint		infoType,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterHandle,
		Buf_T*		objectInfo
) {
	return NWDSExtSyncRead(ctx, objectName, infoType, allAttrs, attrNames,
			iterHandle, NULL, objectInfo);
}

NWDSCCODE NWDSListAttrsEffectiveRights(
		NWDSContextHandle ctx,
		const NWDSChar*	objectName,
		const NWDSChar* subjectName,
		nuint		allAttrs,
		Buf_T*		attrNames,
		nuint32*	iterHandle,
		Buf_T*		privilegeInfo
) {
	Buf_T buf;
	nuint8 spare[4 + MAX_DN_BYTES];
	NWDSCCODE dserr;

	if (!subjectName)
		return ERR_NULL_POINTER;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = NWDSCtxBufDN(ctx, &buf, subjectName);
	if (dserr)
		return dserr;
	return __NWDSRead(ctx, objectName, DS_EFFECTIVE_PRIVILEGES,
			allAttrs, attrNames, iterHandle, &buf, NULL,
			privilegeInfo);	
}
