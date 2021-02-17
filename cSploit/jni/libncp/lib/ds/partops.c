/*
    partops.c
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

	1.00  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version, NWDSAbortPartitionOperation.

	1.01  2000, April 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSJoinPartitions.
		Added NWDSListPartitions.
	
	1.02  2000, April 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSListPartitionsExtInfo.
		Added NWDSGetPartitionInfo.
		Added NWDSGetPartitionExtInfoPtr.
		Added NWDSGetPartitionExtInfo.
		Added NWDSSplitPartition.
		Added NWDSRemovePartition.
		
	1.03  2000, April 29		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSAddReplica.
		Added NWDSRemoveReplica.
		Added NWDSChangeReplicaType.

	1.04  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSPartitionReceiveAllUpdates.
		Added NWDSPartitionSendAllUpdates.
		Added NWDSSyncPartition.
		Added NWDSGetPartitionRoot.

	1.05  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSRepairTimeStamps.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"

static NWDSCCODE __NWDSAbortPartitionOperationV3(
		NWCONN_HANDLE	conn,
		nflag32		flags,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 3);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_ABORT_PARTITION_OPERATION, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSAbortPartitionOperation(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = __NWDSAbortPartitionOperationV3(conn, 0, partitionID);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSJoinPartitionsV0(
		NWCONN_HANDLE	conn,
		nflag32		flags,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_JOIN_PARTITIONS, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSJoinPartitions(
		NWDSContextHandle	ctx,
		const NWDSChar*		subordinatePartition,
		nflag32			flags
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, subordinatePartition, 
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = __NWDSJoinPartitionsV0(conn, flags, partitionID);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSSplitPartitionV0(
		NWCONN_HANDLE	conn,
		nflag32		flags,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_SPLIT_PARTITION, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSSplitPartition(
		NWDSContextHandle	ctx,
		const NWDSChar*		subordinatePartition,
		nflag32			flags
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, subordinatePartition, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = __NWDSSplitPartitionV0(conn, flags, partitionID);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSRemovePartitionV0(
		NWCONN_HANDLE	conn,
		nflag32		flags,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_REMOVE_PARTITION, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSRemovePartition(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = __NWDSRemovePartitionV0(conn, 0, partitionID);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSListPartitionsV0(
		NWCONN_HANDLE		conn,
		nuint32			qflags,
		nuint32*		iterHandle,
		Buf_T*			partitions
) {
	NW_FRAGMENT rq_frag[1];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[12];
	nuint8 rp_b[4];
	NWDSCCODE dserr;
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, qflags);
	DSET_LH(rq_b, 8, *iterHandle);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	NWDSBufStartPut(partitions, DSV_LIST_PARTITIONS);
	NWDSBufSetDSIFlags(partitions, DSP_PARTITION_DN | DSP_REPLICA_TYPE);
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(partitions, &rp_frag[1].fragSize);
	dserr = NWCFragmentRequest(conn, DSV_LIST_PARTITIONS, 1, rq_frag, 
			2, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[1].fragSize < 1)
		return ERR_INVALID_SERVER_RESPONSE;
	*iterHandle = DVAL_LH(rp_b, 0);
	NWDSBufPutSkip(partitions, rp_frag[1].fragSize);
	NWDSBufFinishPut(partitions);
	return 0;
}

static NWDSCCODE __NWDSListPartitionsV1(
		NWCONN_HANDLE		conn,
		nuint32			qflags,
		nflag32			dspFlags,
		nuint32*		iterHandle,
		Buf_T*			partitions
) {
	NW_FRAGMENT rq_frag[1];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[16];
	nuint8 rp_b[4];
	NWDSCCODE dserr;
	
	DSET_LH(rq_b, 0, 1);
	DSET_LH(rq_b, 4, qflags);
	DSET_LH(rq_b, 8, *iterHandle);
	DSET_LH(rq_b, 12, dspFlags);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 16;
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	NWDSBufStartPut(partitions, DSV_LIST_PARTITIONS);
	NWDSBufSetDSIFlags(partitions, dspFlags);
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(partitions, &rp_frag[1].fragSize);
	dserr = NWCFragmentRequest(conn, DSV_LIST_PARTITIONS, 1, rq_frag, 
			2, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[1].fragSize < 1)
		return ERR_INVALID_SERVER_RESPONSE;
	*iterHandle = DVAL_LH(rp_b, 0);
	NWDSBufPutSkip(partitions, rp_frag[1].fragSize);
	NWDSBufFinishPut(partitions);
	return 0;
}

NWDSCCODE NWDSListPartitionsExtInfo(
		NWDSContextHandle	ctx,
		nuint32*		iterHandle,
		const NWDSChar*		server,
		nflag32			dspFlags,
		Buf_T*			partitions
) {
	struct wrappedIterationHandle* ih = NULL;
	NWCONN_HANDLE conn;
	nuint32 lh;
	NWDSCCODE dserr;
	nuint32 qflags = 0;
	nuint32 ctxflags;
	
	dserr = NWDSGetContext(ctx, DCK_FLAGS, &ctxflags);
	if (dserr)
		return dserr;
	if (ctxflags & DCV_TYPELESS_NAMES)
		qflags |= 0x0001;
	qflags |= ctx->dck.name_form << 1;
	if (*iterHandle == NO_MORE_ITERATIONS) {
		dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
		if (dserr)
			return dserr;
		lh = NO_MORE_ITERATIONS;
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_LIST_PARTITIONS);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		lh = ih->iterHandle;
	}
	dserr = __NWDSListPartitionsV1(conn, qflags, dspFlags, &lh, partitions);
	if (dserr == ERR_INVALID_API_VERSION && dspFlags == (DSP_PARTITION_DN | DSP_REPLICA_TYPE)) {
		dserr = __NWDSListPartitionsV0(conn, qflags, &lh, partitions);
	}
	if (ih)
		return __NWDSIHUpdate(ih, dserr, lh, iterHandle);
	return __NWDSIHCreate(dserr, conn, 0, lh, DSV_LIST_PARTITIONS, iterHandle);
}

NWDSCCODE NWDSListPartitions(
		NWDSContextHandle	ctx,
		nuint32*		iterHandle,
		const NWDSChar*		server,
		Buf_T*			partitions
) {
	return NWDSListPartitionsExtInfo(ctx, iterHandle, server, 
		DSP_PARTITION_DN | DSP_REPLICA_TYPE, partitions);
}

NWDSCCODE NWDSGetPartitionExtInfoPtr(
	UNUSED( NWDSContextHandle	ctx),
		Buf_T*			partitions,
		char**			infoPtr,
		char**			infoPtrEnd
) {
	nuint32 fields;
	NWDSCCODE dserr;

	if (!partitions)
		return ERR_NULL_POINTER;
	if (partitions->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (partitions->operation) {
		case DSV_LIST_PARTITIONS:
			break;
		default:
			return ERR_BAD_VERB;
	}
	fields = partitions->dsiFlags;
	if (fields & DSP_OUTPUT_FIELDS) {
		*infoPtr = partitions->curPos;
		dserr = NWDSBufGetLE32(partitions, &fields);
		if (dserr)
			return dserr;
	} else {
		*infoPtr = partitions->curPos - 4;
		DSET_LH(partitions->curPos - 4, 0, fields);
	}
	if (fields & DSP_PARTITION_ID)
		NWDSBufGetSkip(partitions, 4);
	if (fields & DSP_REPLICA_STATE)
		NWDSBufGetSkip(partitions, 4);
	if (fields & DSP_MODIFICATION_TIMESTAMP)
		NWDSBufGetSkip(partitions, 8);
	if (fields & DSP_PURGE_TIME)
		NWDSBufGetSkip(partitions, 4);
	if (fields & DSP_LOCAL_PARTITION_ID)
		NWDSBufGetSkip(partitions, 4);
	if (fields & DSP_PARTITION_DN) {
		dserr = NWDSBufSkipBuffer(partitions);
		if (dserr)
			return dserr;
	}
	if (fields & DSP_REPLICA_TYPE)
		NWDSBufGetSkip(partitions, 4);
	if (fields & DSP_PARTITION_BUSY)
		NWDSBufGetSkip(partitions, 4);
	if (fields & 0x0200)
		NWDSBufGetSkip(partitions, 4);
	if (fields & 0xFFFFFC00)
		return NWE_PARAM_INVALID;
	if (partitions->curPos > partitions->dataend)
		return ERR_BUFFER_EMPTY;
	*infoPtrEnd = partitions->curPos;
	return 0;
}

NWDSCCODE NWDSGetPartitionExtInfo(
		NWDSContextHandle	ctx,
		char*			infoPtr,
		char*			infoPtrEnd,
		nflag32			infoFlag,
		size_t*			len,
		void*			data
) {
	Buf_T buf;
	nuint32 flags;
	nuint32 val;
	nuint32 bit;
	NWDSCCODE err;
	size_t tmplen;

	if (!infoPtr || !infoPtrEnd)
		return ERR_NULL_POINTER;
	if (infoPtr + 4 > infoPtrEnd)
		return NWE_PARAM_INVALID;
	if (!infoFlag || (infoFlag & (infoFlag - 1)))
		return NWE_PARAM_INVALID;
	if (!len)
		len = &tmplen;
	NWDSSetupBuf(&buf, infoPtr, infoPtrEnd - infoPtr);
	err = NWDSBufGetLE32(&buf, &flags);
	if (err)
		return err;
	if (!(flags & infoFlag))
		return NWE_PARAM_INVALID;
	val = flags;
	if (infoFlag == DSP_OUTPUT_FIELDS)
		goto bytes4;
	for (bit = DSP_PARTITION_ID; bit; bit <<= 1) {
		if (flags & bit) {
			if (bit == infoFlag) {
				switch (bit) {
					case DSP_PARTITION_ID:
						{
							NWObjectID id;
		
							err = NWDSBufGetID(&buf, &id);
							if (err)
								return err;
							if (data)
								*(NWObjectID*)data = id;
							*len = sizeof(NWObjectID);
						}
						return 0;
					case DSP_MODIFICATION_TIMESTAMP:
						{
							void* p;
		
							p = NWDSBufGetPtr(&buf, 8);
							if (!p)
								return ERR_BUFFER_EMPTY;
							if (data) {
								((TimeStamp_T*)data)->wholeSeconds = DVAL_LH(p, 0);
								((TimeStamp_T*)data)->replicaNum = WVAL_LH(p, 4);
								((TimeStamp_T*)data)->eventID = WVAL_LH(p, 6);
							}
							*len = sizeof(TimeStamp_T);
						}
						return 0;
					case DSP_PARTITION_DN:
						return NWDSBufCtxDN(ctx, &buf, data, len);
					default:;
						err = NWDSBufGetLE32(&buf, &val);
						if (err)
							return err;
					bytes4:;
						if (data)
							*(nuint32*)data = val;
						*len = sizeof(nuint32);
						return 0;
				}
			} else {
				switch (bit) {
					case DSP_MODIFICATION_TIMESTAMP:
						NWDSBufGetSkip(&buf, 8);
						break;
					case DSP_PARTITION_DN:
						err = NWDSBufSkipBuffer(&buf);
						if (err)
							return err;
						break;
					default:
						NWDSBufGetSkip(&buf, 4);
						break;
				}
			}
		}
	}
	return NWE_PARAM_INVALID;
}

NWDSCCODE NWDSGetPartitionInfo(
		NWDSContextHandle	ctx,
		Buf_T*			partitions,
		NWDSChar*		partitionName,
		nuint32*		replicaType
) {
	NWDSCCODE dserr;
	char* start;
	char* stop;
	
	dserr = NWDSGetPartitionExtInfoPtr(ctx, partitions, &start, &stop);
	if (dserr)
		return dserr;
	if (partitionName) {
		dserr = NWDSGetPartitionExtInfo(ctx, start, stop, 
				DSP_PARTITION_DN, NULL, partitionName);
		if (dserr)
			return dserr;
	}
	if (replicaType) {
		dserr = NWDSGetPartitionExtInfo(ctx, start, stop,
				DSP_REPLICA_TYPE, NULL, replicaType);
		if (dserr)
			return dserr;
	}
	return 0;
}

static NWDSCCODE __NWDSAddReplicaV0(
		NWCONN_HANDLE		conn,
		nflag32			flags,
		NWObjectID		partitionID,
		nuint32			replicaType,
		Buf_T*			serverDN
) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[16];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	DSET_LH(rq_b, 12, replicaType);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 16;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(serverDN, &rq_frag[1].fragSize);
	return NWCFragmentRequest(conn, DSV_ADD_REPLICA, 2, rq_frag, 
			0, NULL, NULL);
}

NWDSCCODE NWDSAddReplica(
		NWDSContextHandle	ctx,
		const NWDSChar*		server,
		const NWDSChar*		partitionRoot,
		nuint32			replicaType
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	Buf_T buf;
	char spare[4 + MAX_DN_BYTES];
		
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = NWDSCtxBufDN(ctx, &buf, server);
	if (!dserr) {
		dserr = __NWDSAddReplicaV0(conn, 0, partitionID, replicaType,
				&buf);
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSRemoveReplicaV0(
		NWCONN_HANDLE		conn,
		nflag32			flags,
		NWObjectID		partitionID,
		Buf_T*			serverDN
) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(serverDN, &rq_frag[1].fragSize);
	return NWCFragmentRequest(conn, DSV_REMOVE_REPLICA, 2, rq_frag, 
			0, NULL, NULL);
}

NWDSCCODE NWDSRemoveReplica(
		NWDSContextHandle	ctx,
		const NWDSChar*		server,
		const NWDSChar*		partitionRoot
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	Buf_T buf;
	char spare[4 + MAX_DN_BYTES];
		
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = NWDSCtxBufDN(ctx, &buf, server);
	if (!dserr) {
		dserr = __NWDSRemoveReplicaV0(conn, 0, partitionID, &buf);
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSChangeReplicaTypeV0(
		NWCONN_HANDLE		conn,
		nflag32			flags,
		NWObjectID		partitionID,
		nuint32			newReplicaType,
		Buf_T*			serverDN
) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[16];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	DSET_LH(rq_b, 12, newReplicaType);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 16;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(serverDN, &rq_frag[1].fragSize);
	return NWCFragmentRequest(conn, DSV_CHANGE_REPLICA_TYPE, 2, rq_frag, 
			0, NULL, NULL);
}

NWDSCCODE NWDSChangeReplicaType(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot,
		const NWDSChar*		server,
		nuint32			newReplicaType
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	Buf_T buf;
	char spare[4 + MAX_DN_BYTES];
		
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = NWDSCtxBufDN(ctx, &buf, server);
	if (!dserr) {
		dserr = __NWDSChangeReplicaTypeV0(conn, 0, partitionID, newReplicaType,
				&buf);
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSPartitionReceiveAllUpdatesV0(
		NWCONN_HANDLE	conn,
		NWObjectID	partitionID,
		NWObjectID	serverID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[16];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, 1);
	DSET_HL(rq_b, 8, partitionID);
	DSET_HL(rq_b, 12, serverID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 16;
	return NWCFragmentRequest(conn, DSV_PARTITION_FUNCTIONS, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSPartitionReceiveAllUpdates(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot,
		const NWDSChar*		serverName
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWObjectID serverID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = NWDSMapNameToID(ctx, conn, serverName, &serverID);
	if (!dserr) {
		dserr = __NWDSPartitionReceiveAllUpdatesV0(conn, partitionID, serverID);
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSPartitionSendAllUpdatesV0(
		NWCONN_HANDLE	conn,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, 1);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_PARTITION_FUNCTIONS, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSPartitionSendAllUpdates(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot,
		const NWDSChar*		serverName
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = NWDSMapNameToID(ctx, conn, partitionRoot, &partitionID);
	if (!dserr) {
		dserr = __NWDSPartitionSendAllUpdatesV0(conn, partitionID);
		if (!dserr) {
			dserr = NWDSSyncPartition(ctx, serverName, partitionRoot, 3);
		}
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSSyncPartitionV1(
		NWCONN_HANDLE	conn,
		nuint32		flags,
		nuint32		seconds,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[16];
	
	DSET_LH(rq_b, 0, 1);
	DSET_LH(rq_b, 4, flags);
	DSET_LH(rq_b, 8, seconds);
	DSET_HL(rq_b, 12, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 16;
	return NWCFragmentRequest(conn, DSV_SYNC_PARTITION, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSSyncPartition(
		NWDSContextHandle	ctx,
		const NWDSChar*		serverName,
		const NWDSChar*		partitionRoot,
		nuint32			seconds
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = NWDSMapNameToID(ctx, conn, partitionRoot, &partitionID);
	if (!dserr) {
		dserr = __NWDSSyncPartitionV1(conn, 0, seconds, partitionID);
	}
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSGetPartitionRootV0(
		NWCONN_HANDLE	conn,
		NWObjectID	objectID,
		NWObjectID*	partitionRootID
) {
	NW_FRAGMENT rq_frag[1];
	NW_FRAGMENT rp_frag[1];
	nuint8 rq_b[8];
	nuint8 rp_b[4];
	NWDSCCODE dserr;
	
	DSET_LH(rq_b, 0, 0);
	DSET_HL(rq_b, 4, objectID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 8;
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	dserr = NWCFragmentRequest(conn, DSV_PARTITION_FUNCTIONS, 
			1, rq_frag, 1, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[0].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	if (partitionRootID)
		*partitionRootID = DVAL_LH(rp_b, 0);
	return 0;
}

NWDSCCODE NWDSGetPartitionRoot(
		NWDSContextHandle	ctx,
		const NWDSChar*		objectName,
		NWDSChar*		partitionRoot
) {
	NWCONN_HANDLE conn;
	NWObjectID objectID;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	Buf_T buf;
	char spare[MAX_DN_BYTES + 4];
	
	if (!partitionRoot)
		return ERR_NULL_POINTER;
	dserr = NWDSResolveName2DR(ctx, objectName, 
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_READABLE,
			&conn, &objectID);
	if (dserr)
		return dserr;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = __NWDSReadObjectDSIInfo(ctx, conn, objectID, 
			DSI_PARTITION_ROOT_DN, &buf);
	if (!dserr)
		goto storeName;
	dserr = __NWDSGetPartitionRootV0(conn, objectID, &partitionID);
	if (dserr)
		goto quit;
	NWDSSetupBuf(&buf, spare, sizeof(spare));
	dserr = __NWDSReadObjectDSIInfo(ctx, conn, partitionID,
			DSI_ENTRY_DN, &buf);
	if (dserr)
		goto quit;
storeName:;
	dserr = NWDSBufCtxDN(ctx, &buf, partitionRoot, NULL);
quit:;
	NWCCCloseConn(conn);
	return dserr;
}

static NWDSCCODE __NWDSRepairTimeStampsV0(
		NWCONN_HANDLE	conn,
		nflag32		flags,
		NWObjectID	partitionID
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, flags);
	DSET_HL(rq_b, 8, partitionID);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_REPAIR_TIMESTAMPS, 
		1, rq_frag, 0, NULL, NULL);
}

NWDSCCODE NWDSRepairTimeStamps(
		NWDSContextHandle	ctx,
		const NWDSChar*		partitionRoot
) {
	NWCONN_HANDLE conn;
	NWObjectID partitionID;
	NWDSCCODE dserr;
	
	dserr = NWDSResolveName2DR(ctx, partitionRoot, 
			/* FIXME: master or read-write? */
			DS_RESOLVE_DEREF_ALIASES | DS_RESOLVE_MASTER,
			&conn, &partitionID);
	if (dserr)
		return dserr;
	dserr = __NWDSRepairTimeStampsV0(conn, 0, partitionID);
	NWCCCloseConn(conn);
	return dserr;	
}
