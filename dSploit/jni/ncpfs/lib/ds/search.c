/*
    search.c - NWDSSearch implementation
    Copyright (C) 1999, 2000  Petr Vandrovec

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

	1.00  2000, February 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.
	
	1.01  2000, February 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		NWDSExtSyncSearch added.

	1.02  2000, August 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Removed DS_RESOLVE_WALK_TREE from resolve calls.

	1.03  2001, June 13		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed internal bug assertion when invalid DN passed
		to NWDSSearch.

 */

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ncplib_i.h"
#include "nwnet_i.h"

static NWDSCCODE __NWDSSearchV3(
	NWCONN_HANDLE	conn,
	nuint32		nameFormat,
	nuint32*	lowlevelIH,
	NWObjectID	objectID,
	nuint32		scope,
	nuint32		unk1,
	nuint32		infoType,
	nuint32		dsiFlags,
	nuint32		allAttrs,
	Buf_T*		attr,
	Buf_T*		filter,
	NWObjectCount*	countObjectsSearched,
	Buf_T*		objectInfo) {
	NWDSCCODE err;
	NW_FRAGMENT rq_frag[3];
	nuint8 rq_b[36];
	static const nuint8 zero4[4] = { 0, 0, 0, 0};
	NW_FRAGMENT rp_frag[2];
	nuint8 rp_b[8];
	
	DSET_LH(rq_b, 0, 3);
	DSET_LH(rq_b, 4, nameFormat);
	DSET_LH(rq_b, 8, *lowlevelIH);
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, scope);
	DSET_LH(rq_b, 20, unk1);
	DSET_LH(rq_b, 24, infoType);
	DSET_LH(rq_b, 28, dsiFlags);
	DSET_LH(rq_b, 32, allAttrs);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 36;
	if (!allAttrs && attr) {
		size_t len;

		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attr, &len);
		rq_frag[1].fragSize = ROUNDPKT(len);
	} else {
		rq_frag[1].fragAddr.ro = zero4;
		rq_frag[1].fragSize = 4;
	}
	rq_frag[2].fragAddr.ro = NWDSBufRetrieve(filter, &rq_frag[2].fragSize);
	
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(objectInfo, &rp_frag[1].fragSize);
	
	err = NWCFragmentRequest(conn, DSV_SEARCH, 3, rq_frag, 2, rp_frag, NULL);
	if (!err) {
		*lowlevelIH = DVAL_LH(rp_b, 0);
		if (countObjectsSearched) {
			*countObjectsSearched = DVAL_LH(rp_b, 4);
		}
		NWDSBufPutSkip(objectInfo, rp_frag[1].fragSize);
	}
	return err;
}

static NWDSCCODE __NWDSSearchV4(
	NWCONN_HANDLE		conn,
	nuint32			nameFormat,
	nuint32*		lowlevelIH,
	NWObjectID		objectID,
	nuint32			scope,
	nuint32			unk1,
	nuint32			infoType,
	nuint32			dsiFlags,
	const TimeStamp_T*	timeStamp,
	nuint32			allAttrs,
	Buf_T*			attr,
	Buf_T*			filter,
	NWObjectCount*		countObjectsSearched,
	Buf_T*			objectInfo) {
	NWDSCCODE err;
	nuint8 rq_b[44];
	NW_FRAGMENT rq_frag[3];
	static const nuint8 zero4[4] = {0, 0, 0, 0};
	nuint8 rp_b[8];
	NW_FRAGMENT rp_frag[2];

	DSET_LH(rq_b, 0, 4);
	DSET_LH(rq_b, 4, nameFormat);
	DSET_LH(rq_b, 8, *lowlevelIH);
	DSET_HL(rq_b, 12, objectID);
	DSET_LH(rq_b, 16, scope);
	DSET_LH(rq_b, 20, unk1);
	DSET_LH(rq_b, 24, infoType);
	DSET_LH(rq_b, 28, dsiFlags);
	DSET_LH(rq_b, 32, timeStamp->wholeSeconds);
	WSET_LH(rq_b, 36, timeStamp->replicaNum);
	WSET_LH(rq_b, 38, timeStamp->eventID);
	DSET_LH(rq_b, 40, allAttrs);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 44;
	if (!allAttrs && attr) {
		size_t len;

		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attr, &len);
		rq_frag[1].fragSize = ROUNDPKT(len);
	} else {
		rq_frag[1].fragAddr.ro = zero4;
		rq_frag[1].fragSize = 4;
	}
	rq_frag[2].fragAddr.ro = NWDSBufRetrieve(filter, &rq_frag[2].fragSize);
	
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(objectInfo, &rp_frag[1].fragSize);

	err = NWCFragmentRequest(conn, DSV_SEARCH, 3, rq_frag, 2, rp_frag, NULL);
	if (!err) {
		*lowlevelIH = DVAL_LH(rp_b, 0);
		if (countObjectsSearched) {
			*countObjectsSearched = DVAL_LH(rp_b, 4);
		}
		NWDSBufPutSkip(objectInfo, rp_frag[1].fragSize);
	}
	return err;
}

struct search_referrals {
	struct search_referrals*	next;
	nuint32		referrals;
	char		data[0];
};

struct SearchIH {
	u_int16_t	iteration;
	nuint8*		entrystart;
	struct search_referrals*	namePtr;
};

static void __NWDSAbortSearchHandle(struct wrappedIterationHandle* ih) {
	struct SearchIH* searchIH;
	
	searchIH = (struct SearchIH*)ih->data;
	if (searchIH) {
		struct search_referrals* ptr;

		ptr = searchIH->namePtr;
		while (ptr) {
			struct search_referrals* tmp;

			tmp = ptr;
			ptr = ptr->next;
			free(tmp);
		}
		free(searchIH);
		ih->data = NULL;
	}
}

static NWDSCCODE UpdateSearchHandle(struct wrappedIterationHandle** pSearchIH, Buf_T* buffer, NWCONN_HANDLE conn, NWObjectID objectID, nuint32 lowlevelIH) {
	struct wrappedIterationHandle*	searchIH;
	nuint32				len;
	nuint32				referrals;
	void*				data;
	struct search_referrals**	end;
	struct search_referrals*	namePtr;
	NWDSCCODE err;
	
	/* length */
	err = NWDSBufGetLE32(buffer, &len);
	if (err)
		return err;
	if (len < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	err = NWDSBufGetLE32(buffer, &referrals);
	if (err)
		return err;
	data = NWDSBufGetPtr(buffer, len - 4);
	if (!data)
		return ERR_BUFFER_EMPTY;
	searchIH = *pSearchIH;
	if (!searchIH) {
		struct SearchIH* sih;
		
		sih = malloc(sizeof(*sih));
		if (!sih)
			return ERR_NOT_ENOUGH_MEMORY;
		searchIH = __NWDSIHInit(conn, lowlevelIH, DSV_SEARCH);
		if (!searchIH) {
			free(sih);
			return ERR_NOT_ENOUGH_MEMORY;
		}
		searchIH->abort = __NWDSAbortSearchHandle;
		searchIH->objectID = objectID;
		sih->namePtr = NULL;
		sih->entrystart = NULL;
		sih->iteration = 0;
		searchIH->data = sih;
	}
	if (referrals) {
		struct SearchIH* sih;
		
		sih = searchIH->data;
		end = &sih->namePtr;
		while ((namePtr = *end) != NULL)
			end = &namePtr->next;
		namePtr = malloc(sizeof(*namePtr) + len);
		if (!namePtr) {
			__NWDSIHAbort(searchIH);
			return ERR_NOT_ENOUGH_MEMORY;
		}
		*end = namePtr;

		namePtr->next = NULL;
		namePtr->referrals = referrals;
		memcpy(namePtr->data, data, len - 4);
	}
	*pSearchIH = searchIH;
	return 0;
}

static inline void NextNamePtr(struct SearchIH* searchIH) {
	struct search_referrals* currName;
		
	currName = searchIH->namePtr;
	if (--currName->referrals) {
		/* skip entry: 32bit info + name */
		searchIH->entrystart += 4;
		searchIH->entrystart += ROUNDPKT(4 + DVAL_LH(searchIH->entrystart, 0));
	} else {
		struct search_referrals* nextName;
			
		nextName = currName->next;
		searchIH->namePtr = nextName;
		searchIH->entrystart = NULL;
		free(currName);
	}
}

static NWDSCCODE GetCurrentName(struct SearchIH* searchIH, u_int32_t* objInfo, unicode** objName, size_t* objNameLen) {
	nuint8* curPos;

	curPos = searchIH->entrystart;
	if (!curPos) {
		struct search_referrals* namePtr;
		
		namePtr = searchIH->namePtr;
		if (!namePtr)
			return ERR_BUFFER_EMPTY;
		curPos = namePtr->data;
		searchIH->entrystart = curPos;
	}
	*objInfo = DVAL_LH(curPos, 0);
	/* FIXME: Check buffer overflow */
	*objNameLen = DVAL_LH(curPos, 4);
	*objName = (unicode*)(curPos + 8);
	NextNamePtr(searchIH);
	return 0;
}

static NWDSCCODE __SearchProtocol(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		NWObjectID		objectID,
		nint			scope,
		Buf_T*			filter,
		const TimeStamp_T*	timeStamp,
		nuint			infoType,
		nuint			allAttrs,
		Buf_T*			attrNames,
		nuint32*		lowlevelIH,
	UNUSED(	NWObjectCount		countObjectsToSearch),
		NWObjectCount*		countObjectsSearched,
		Buf_T*			objectInfo,
		nuint32			nameFormat) {
	nuint32 dsiFlags;
	nuint32 dckFlags;
	NWDSCCODE err;

	if (!filter)
		return ERR_NULL_POINTER;
	err = NWDSGetContext(ctx, DCK_FLAGS, &dckFlags);
	if (err)
		return err;
	dsiFlags = ctx->dck.dsi_flags;
	if (dckFlags & DCV_DEREF_BASE_CLASS)
		dsiFlags |= DSI_DEREFERENCE_BASE_CLASS;
	NWDSBufStartPut(objectInfo, DSV_SEARCH);
	NWDSBufSetDSIFlags(objectInfo, dsiFlags);
	if (allAttrs)
		attrNames = NULL;
	if (timeStamp) {
		err = __NWDSSearchV4(conn, nameFormat, lowlevelIH, objectID, scope, 0, infoType, 
			dsiFlags, timeStamp, allAttrs, attrNames, filter, 
			countObjectsSearched, objectInfo);
	} else {
		err = __NWDSSearchV3(conn, nameFormat, lowlevelIH, objectID, scope, 0, infoType,
			dsiFlags, allAttrs, attrNames, filter,
			countObjectsSearched, objectInfo);
	}
	NWDSBufFinishPut(objectInfo);
	/* Now we skip infoType value returned by server. Maybe we should check it for being
	   equal with passed infoType? */
	NWDSBufGetSkip(objectInfo, 4);
	return err;
}

static NWDSCCODE UpdateServerInSearchHandle(struct wrappedIterationHandle* searchIH, NWCONN_HANDLE conn, NWObjectID objectID) {
	searchIH->objectID = objectID;
	__NWDSIHSetConn(searchIH, conn);
	return 0;
}

NWDSCCODE NWDSExtSyncSearch(
		NWDSContextHandle	ctx,
		const NWDSChar*		baseObjectName,
		nint			scope,
		nuint			searchAliases,
		Buf_T*			filter,
		const TimeStamp_T*	timeStamp,
		nuint			infoType,
		nuint			allAttrs,
		Buf_T*			attrNames,
		nuint32*		iterHandle,
		NWObjectCount		countObjectsToSearch,
		NWObjectCount*		countObjectsSearched,
		Buf_T*			objectInfo) {
	NWCONN_HANDLE	conn;
	NWObjectID	objectID;
	nuint32		dckFlags;
	nuint32		nameForm;
	struct wrappedIterationHandle* currIH;
	NWDSCCODE	err;
	wchar_t*	revobjname = NULL;
	size_t		revobjnamelen = 0;
	nuint32		lowlevelIH;
	nuint32		le32;
	void*		bufptr;

	err = NWDSGetContext(ctx, DCK_FLAGS, &dckFlags);
	if (err)
		return err;
	nameForm = ctx->dck.name_form;
	if (dckFlags & DCV_TYPELESS_NAMES)
		nameForm |= 0x00000001;
	if (searchAliases)
		nameForm |= 0x00010000;
	if (*iterHandle == NO_MORE_ITERATIONS) {
		currIH = NULL;
		err = NWDSResolveName2(ctx, baseObjectName, DS_RESOLVE_READABLE, 
			&conn, &objectID);
		if (err)
			goto abandon_nc;
		lowlevelIH = NO_MORE_ITERATIONS;
	} else {
		struct SearchIH* sih;

		currIH = __NWDSIHLookup(*iterHandle, DSV_SEARCH);
		if (!currIH)
			return ERR_INVALID_HANDLE;
		sih = currIH->data;
		if (currIH->conn) {
			conn = currIH->conn;
			ncp_conn_use(conn);
			objectID = currIH->objectID;
			lowlevelIH = currIH->iterHandle;
		} else {
			sih->iteration++;
			while (1) {
				unicode*	objName;
				size_t		objNameLen;
				nuint32		isReal;

				err = GetCurrentName(sih, &isReal, &objName, &objNameLen);
				if (err) {
					err = 0;
					goto abandon_nc;
				}
				if (searchAliases && !isReal) {
					wchar_t wname[MAX_DN_CHARS+1];

					err = NWDSPtrDN(objName, objNameLen, wname, sizeof(wname));
					if (err)
						goto abandon_nc;
					wcsrev(wname);
					if (!revobjname) {
						revobjname = malloc(MAX_DN_BYTES);
						if (!revobjname) {
							err = ERR_NOT_ENOUGH_MEMORY;
							goto abandon_nc;
						}
						/* we should return empty string for [root] */
						err = NWDSGetCanonicalizedName(ctx, baseObjectName, revobjname);
						if (err) {
							goto abandon_nc;
						}
						wcsrev(revobjname);
						revobjnamelen = wcslen(revobjname);
						/* append dot at the end... */
						revobjname[revobjnamelen++] = '.';
					}
					if (!wcsncasecmp(revobjname, wname, revobjnamelen))
						continue;
				}
				err = __NWDSResolveName2u(ctx, objName, DS_RESOLVE_READABLE,
					&conn, &objectID);
				if (err)
					continue;
				lowlevelIH = NO_MORE_ITERATIONS;
				err = UpdateServerInSearchHandle(currIH, conn, objectID);
				if (err)
					goto abandon;
				break;
			}
		}
		/* check for DS_SEARCH_PARTITION? */
		if ((scope == DS_SEARCH_SUBORDINATES) && (sih->iteration))
			scope = DS_SEARCH_ENTRY;
	}
	err = __SearchProtocol(ctx, conn, objectID, scope, filter, timeStamp, infoType, allAttrs, attrNames, &lowlevelIH,
			countObjectsToSearch, countObjectsSearched, objectInfo, nameForm);
	if (err)
		goto abandon;
	bufptr = NWDSBufTell(objectInfo);
	err = NWDSBufSkipBuffer(objectInfo);
	if (err)
		goto abandon;
	/* retrieve external references count */
	err = NWDSBufPeekLE32(objectInfo, 4, &le32);
	if (err)
		goto abandon;
	if (le32 || (lowlevelIH != NO_MORE_ITERATIONS)) {
		err = UpdateSearchHandle(&currIH, objectInfo, conn, objectID, lowlevelIH);
		if (err)
			goto abandon;
	}
	/* Return back to objects buffer */
	NWDSBufSeek(objectInfo, bufptr);
	/* Retrieve objects buffer length */
	err = NWDSBufGetLE32(objectInfo, &le32);
	if (err)
		goto abandon;
	/* And modify object end so that we cannot overrun */
	objectInfo->dataend = objectInfo->curPos + le32;

	if (!currIH) {
		*iterHandle = NO_MORE_ITERATIONS;
	} else if (lowlevelIH == NO_MORE_ITERATIONS) {
		struct SearchIH* sih = currIH->data;

		if (!sih->namePtr) {
			*iterHandle = NO_MORE_ITERATIONS;
			__NWDSIHAbort(currIH);
		} else {
			__NWDSIHSetConn(currIH, NULL);
			__NWDSIHPut(currIH, iterHandle);
		}
	} else {
		currIH->iterHandle = lowlevelIH;
		__NWDSIHPut(currIH, iterHandle);
	}
	NWCCCloseConn(conn);	
	goto quit;
abandon:;
	NWCCCloseConn(conn);
abandon_nc:;
	if (currIH) {
		__NWDSIHUpdate(currIH, err, lowlevelIH, iterHandle);
	} else {
		*iterHandle = NO_MORE_ITERATIONS;
	}
	/* Rewind buffer */
	NWDSBufSeek(objectInfo, objectInfo->data);
	/* and put zero objects in */
	NWDSBufPutLE32(objectInfo, 0);
	NWDSBufFinishPut(objectInfo);
quit:;
	if (revobjname)
		free(revobjname);
	return err;
}

NWDSCCODE NWDSSearch(
		NWDSContextHandle	ctx,
		const NWDSChar*		baseObjectName,
		nint			scope,
		nuint			searchAliases,
		Buf_T*			filter,
		nuint			infoType,
		nuint			allAttrs,
		Buf_T*			attrNames,
		nuint32*		iterHandle,
		NWObjectCount		countObjectsToSearch,
		NWObjectCount*		countObjectsSearched,
		Buf_T*			objectInfo) {
	return NWDSExtSyncSearch(ctx, baseObjectName, scope, searchAliases, 
			filter, NULL, infoType, allAttrs, attrNames, 
			iterHandle, countObjectsToSearch, 
			countObjectsSearched, objectInfo);
}
