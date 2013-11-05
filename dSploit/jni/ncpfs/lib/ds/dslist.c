/*
    dslist.c
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
		Added NWDSExtSyncList.

	1.01  2000, August 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Removed DS_RESOLVE_WALK_TREE from resolve calls.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

static void NWDSRewindBuf(Buf_T* buf) {
	buf->curPos = buf->data;
	buf->dataend = buf->allocend;
	buf->attrCountPtr = NULL;
	buf->valCountPtr = NULL;
}

static NWDSCCODE __NWDSListV1(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		NWObjectID objectID,
		nuint32 qflags,
		nuint32* iterHandle,
		nuint32 dsiflags,
		Buf_T* nameb,
		Buf_T* buffer
) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[2];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[20];
	nuint8 rp_b[4];
	nuint32 ctxflags;
		
	err = NWDSGetContext(ctx, DCK_FLAGS, &ctxflags);
	if (err)
		return err;
	if (ctxflags & DCV_TYPELESS_NAMES)
		qflags |= 0x0001;
	if (ctxflags & DCV_DEREF_BASE_CLASS) {
		qflags |= 0x0040;
		dsiflags |= DSI_DEREFERENCE_BASE_CLASS;
	}
	qflags |= ctx->dck.name_form << 1;

	DSET_LH(rq_b, 0, 1); 		/* version */
	DSET_LH(rq_b, 4, qflags);	/* ctxflag */
	DSET_LH(rq_b, 8, *iterHandle);	/* iterhandle */
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, dsiflags);
	
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 20;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(nameb, &rq_frag[1].fragSize);

	NWDSBufStartPut(buffer, DSV_LIST);
	NWDSBufSetDSIFlags(buffer, dsiflags);
	
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(buffer, &rp_frag[1].fragSize);
	err = NWCFragmentRequest(conn, DSV_LIST, 2, rq_frag, 2, rp_frag,
		NULL);
	if (err)
		return err;
	if (rp_frag[0].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	NWDSBufPutSkip(buffer, rp_frag[1].fragSize);
	NWDSBufFinishPut(buffer);
	*iterHandle = DVAL_LH(rp_b, 0);
	return err;
}

static NWDSCCODE __NWDSListV2(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		NWObjectID objectID,
		nuint32 qflags,
		nuint32* iterHandle,
		nuint32 dsiflags,
		Buf_T* nameb,
		const TimeStamp_T* ts,
		Buf_T* buffer
) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[3];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[20];
	nuint8 rq_b2[8];
	nuint8 rp_b[4];
	nuint32 ctxflags;
		
	err = NWDSGetContext(ctx, DCK_FLAGS, &ctxflags);
	if (err)
		return err;
	if (ctxflags & DCV_TYPELESS_NAMES)
		qflags |= 0x0001;
	if (ctxflags & DCV_DEREF_BASE_CLASS) {
		qflags |= 0x0040;
		dsiflags |= DSI_DEREFERENCE_BASE_CLASS;
	}
	qflags |= ctx->dck.name_form << 1;

	DSET_LH(rq_b, 0, 2); 		/* version */
	DSET_LH(rq_b, 4, qflags);	/* ctxflag */
	DSET_LH(rq_b, 8, *iterHandle);	/* iterhandle */
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, dsiflags);
	
	DSET_LH(rq_b2, 0, ts->wholeSeconds);
	WSET_LH(rq_b2, 4, ts->replicaNum);
	WSET_LH(rq_b2, 6, ts->eventID);

	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 20;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(nameb, &rq_frag[1].fragSize);
	rq_frag[2].fragAddr.ro = rq_b2;
	rq_frag[2].fragSize = 8;

	NWDSBufStartPut(buffer, DSV_LIST);
	NWDSBufSetDSIFlags(buffer, dsiflags);
	
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(buffer, &rp_frag[1].fragSize);
	err = NWCFragmentRequest(conn, DSV_LIST, 3, rq_frag, 2, rp_frag,
		NULL);
	if (err)
		return err;
	if (rp_frag[0].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	NWDSBufPutSkip(buffer, rp_frag[1].fragSize);
	NWDSBufFinishPut(buffer);
	*iterHandle = DVAL_LH(rp_b, 0);
	return err;
}

NWDSCCODE NWDSExtSyncList(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const NWDSChar* className,
		const NWDSChar* subordinateName,
		nuint32* iterHandle,
		const TimeStamp_T* timeStamp,
		nuint onlyContainers,
		Buf_T* buffer
) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	nuint32 lh;
	struct wrappedIterationHandle *ih = NULL;
	nuint32 qflags;
	nuint8 spare[(4 + MAX_DN_BYTES) * 2];
	Buf_T nameb;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	qflags = onlyContainers ? 2 : 0;
	NWDSSetupBuf(&nameb, spare, sizeof(spare));
	if (subordinateName) {
		err = NWDSPutAttrVal_XX_STRING(ctx, &nameb, subordinateName);
	} else {
		err = NWDSBufPutLE32(&nameb, 0);
	}
	if (err)
		return err;
	if (className) {
		err = NWDSPutAttrVal_XX_STRING(ctx, &nameb, className);
	} else {
		err = NWDSBufPutLE32(&nameb, 0);
	}
	if (err)
		return err;
		
	if (*iterHandle == NO_MORE_ITERATIONS) {
		err = NWDSResolveName2(ctx, objectName, DS_RESOLVE_READABLE,
			&conn, &objID);
		if (err)
			return err;
		lh = NO_MORE_ITERATIONS;
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_LIST);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		lh = ih->iterHandle;
		objID = ih->objectID;
	}
	if (timeStamp) {
		err = __NWDSListV2(ctx, conn, objID, qflags, &lh,
			ctx->dck.dsi_flags, &nameb,
			timeStamp, buffer);
	} else {
		err = __NWDSListV1(ctx, conn, objID, qflags, &lh, 
			ctx->dck.dsi_flags, &nameb, 
			buffer);
	}
	if (ih) {
		return __NWDSIHUpdate(ih, err, lh, iterHandle);
	}
	return __NWDSIHCreate(err, conn, objID, lh, DSV_LIST, iterHandle);
}

static inline NWDSCCODE __NWDSListByClassAndName(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const NWDSChar* className,
		const NWDSChar* subordinateName,
		nuint32* iterHandle,
		nuint onlyContainers,
		Buf_T* buffer) {
	return NWDSExtSyncList(ctx, objectName, className, subordinateName,
		iterHandle, NULL, onlyContainers, buffer);
}

NWDSCCODE NWDSGetCountByClassAndName(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const NWDSChar* className,
		const NWDSChar* subordinateName,
		NWObjectCount* count
) {
	NWDSCCODE err;
	NWCONN_HANDLE conn;
	NWObjectID objID;
	nuint32 iterHandle;
	nuint32 lcnt;
	Buf_T* buf;
	nuint8 spare[(4 + MAX_DN_BYTES) * 2];
	Buf_T nameb;
		
	if (!count)
		return ERR_NULL_POINTER;
	NWDSSetupBuf(&nameb, spare, sizeof(spare));
	if (subordinateName)
		err = NWDSPutAttrVal_XX_STRING(ctx, &nameb, subordinateName);
	else
		err = NWDSBufPutLE32(&nameb, 0);
	if (err)
		return err;
	if (className)
		err = NWDSPutAttrVal_XX_STRING(ctx, &nameb, subordinateName);
	else
		err = NWDSBufPutLE32(&nameb, 0);
	if (err)
		return err;
	*count = 0;
	err = NWDSResolveName2(ctx, objectName, DS_RESOLVE_READABLE,
		&conn, &objID);
	if (err)
		return err;
	lcnt = 0;
	iterHandle = NO_MORE_ITERATIONS;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &buf);
	if (err)
		goto quit_handle;
	do {
		NWObjectCount cnt;

		NWDSRewindBuf(buf);
		err = __NWDSListV1(ctx, conn, objID, 0, &iterHandle, 0, &nameb,
			buf);
		if (err)
			goto quit;
		err = NWDSGetObjectCount(ctx, buf, &cnt);
		if (err)
			goto quit;
		lcnt += cnt;
	} while (iterHandle != NO_MORE_ITERATIONS);
quit:;
	*count = lcnt;
	NWDSFreeBuf(buf);
quit_handle:;
	NWCCCloseConn(conn);
	return err;
} 	

NWDSCCODE NWDSListByClassAndName(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		const NWDSChar* className,
		const NWDSChar* subordinateName,
		nuint32* iterHandle,
		Buf_T* buffer) {
	return __NWDSListByClassAndName(ctx, objectName, className, 
		subordinateName, iterHandle, 0, buffer);
}

NWDSCCODE NWDSList(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		nuint32* iterHandle,
		Buf_T* buffer) {
	return NWDSListByClassAndName(ctx, objectName, NULL, NULL, iterHandle, 
		buffer);
}

NWDSCCODE NWDSListContainers(
		NWDSContextHandle ctx,
		const NWDSChar* objectName,
		nuint32* iterHandle,
		Buf_T* buffer) {
	return __NWDSListByClassAndName(ctx, objectName, NULL, NULL,
		iterHandle, 1, buffer);
}
