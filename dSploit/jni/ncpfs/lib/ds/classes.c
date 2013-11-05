/*
    classes.c
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

	1.00  2000, May 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version.

	1.01  2000, May 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetClassItem, NWDSGetClassItemCount.

	1.02  2000, May 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSBeginClassItem, NWDSDefineClass, NWDSPutClassItem,
			NWDSRemoveClassDef, NWDSListContainableClasses,
			NWDSModifyClassDef, NWDSReadAttrDef, NWDSGetAttrDef,
			NWDSDefineAttr, NWDSRemoveAttrDef.
*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"
#include <string.h>

#define CLASSDEF_ASN	0x0001
#define CLASSDEF_TIMES	0x0002
#define CLASSDEF_ITEMS	0x0004

static NWDSCCODE __NWDSReadClassDefV0(
		NWCONN_HANDLE		conn,
		nuint			infoType,
		nuint			allClasses,
		Buf_T*			classNames,
		nuint32*		iterHandle,
		Buf_T*			classDefs
) {
	static const nuint infoType2cmd[] = {
		0,
		CLASSDEF_ASN | CLASSDEF_ITEMS,
		CLASSDEF_ASN | CLASSDEF_ITEMS,
		CLASSDEF_ASN,
		CLASSDEF_ASN | CLASSDEF_ITEMS,
		CLASSDEF_ASN | CLASSDEF_TIMES | CLASSDEF_ITEMS
	};
	NW_FRAGMENT rq_frag[2];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[20];
	nuint8 rp_b[8];
	NWDSCCODE dserr;
	size_t rq_frags;
	
	if (!classDefs)
		return ERR_NULL_POINTER;
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, *iterHandle);
	DSET_LH(rq_b, 8, infoType);
	DSET_LH(rq_b, 12, allClasses);
	
	if (allClasses || !classNames) {
		DSET_LH(rq_b, 16, 0);
		rq_frag[0].fragSize = 20;
		rq_frags = 1;
	} else {
		if (classNames->operation != DSV_READ_CLASS_DEF)
			return ERR_BAD_VERB;
		rq_frag[0].fragSize = 16;
		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(classNames, &rq_frag[1].fragSize);
		rq_frags = 2;
	}
	rq_frag[0].fragAddr.ro = rq_b;
	NWDSBufStartPut(classDefs, DSV_READ_CLASS_DEF);
	if (infoType > 5)
		infoType = 1;
	classDefs->cmdFlags = infoType2cmd[infoType];
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(classDefs, &rp_frag[1].fragSize);
	dserr = NWCFragmentRequest(conn, DSV_READ_CLASS_DEF, rq_frags, rq_frag, 
			2, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[1].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	if (DVAL_LH(rp_b, 4) != infoType) {
		/* FIXME: __NWDSCloseIterationHandle(conn, DVAL_LH(rp_b, 0), DSV_READ_CLASS_DEF); */
		return ERR_INVALID_SERVER_RESPONSE;
	}
	*iterHandle = DVAL_LH(rp_b, 0);
	NWDSBufPutSkip(classDefs, rp_frag[1].fragSize);
	NWDSBufFinishPut(classDefs);
	return 0;
}

NWDSCCODE NWDSReadClassDef(
		NWDSContextHandle	ctx,
		nuint			infoType,
		nuint			allClasses,
		Buf_T*			classNames,
		nuint32*		iterHandle,
		Buf_T*			classDefs
) {
	NWCONN_HANDLE conn;
	nuint32 lh;
	NWDSCCODE dserr;
	struct wrappedIterationHandle* ih;
	
	if (*iterHandle == NO_MORE_ITERATIONS) {
		dserr = __NWDSGetConnection(ctx, &conn);
		if (dserr)
			return dserr;
		ih = NULL;
		lh = NO_MORE_ITERATIONS;	
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_READ_CLASS_DEF);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		lh = ih->iterHandle;
	}
	dserr = __NWDSReadClassDefV0(conn, infoType, allClasses, classNames, &lh, classDefs);
	if (ih)
		return __NWDSIHUpdate(ih, dserr, lh, iterHandle);
	return __NWDSIHCreate(dserr, conn, 0, lh, DSV_READ_CLASS_DEF, iterHandle);
}

NWDSCCODE NWDSGetClassDefCount(
	UNUSED(	NWDSContextHandle	ctx),
		Buf_T*			buf,
		NWObjectCount*		classDefCount
) {
	NWDSCCODE dserr;
	nuint32 tmp;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_CLASS_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufGetLE32(buf, &tmp);
	if (dserr)
		return dserr;
	if (classDefCount)
		*classDefCount = tmp;
	return 0;
}

NWDSCCODE NWDSGetClassDef(
		NWDSContextHandle	ctx,
		Buf_T*			buf,
		NWDSChar*		className,
		Class_Info_T*		classInfo
) {
	NWDSCCODE dserr;
	nuint32 tmp;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_CLASS_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufCtxString(ctx, buf, className, MAX_SCHEMA_NAME_BYTES, NULL);
	if (dserr)
		return dserr;
	if (buf->cmdFlags & CLASSDEF_ASN) {
		dserr = NWDSBufGetLE32(buf, &tmp);
		if (dserr)
			return dserr;
		if (classInfo) {
			classInfo->classFlags = tmp;

			dserr = NWDSBufGetLE32(buf, &tmp);
			if (dserr)
				return dserr;
			classInfo->asn1ID.length = tmp;
			if (tmp > MAX_ASN1_NAME)
				return NWE_BUFFER_OVERFLOW;
			dserr = NWDSBufGet(buf, classInfo->asn1ID.data, tmp);		
		} else {
			dserr = NWDSBufSkipBuffer(buf);
		}
		if (dserr)
			return dserr;
	}
	return dserr;
}

NWDSCCODE NWDSGetClassItemCount(
	UNUSED(	NWDSContextHandle	ctx),
		Buf_T*			buf,
		NWObjectCount*		itemCount
) {
	NWDSCCODE dserr;
	nuint32 tmp;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_CLASS_DEF:
		case DSV_LIST_CONTAINABLE_CLASSES:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufGetLE32(buf, &tmp);
	if (dserr)
		return dserr;
	if (itemCount)
		*itemCount = tmp;
	return 0;
}

NWDSCCODE NWDSGetClassItem(
		NWDSContextHandle	ctx,
		Buf_T*			buf,
		NWDSChar*		item
) {
	NWDSCCODE dserr;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_CLASS_DEF:
		case DSV_LIST_CONTAINABLE_CLASSES:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufCtxString(ctx, buf, item, MAX_SCHEMA_NAME_BYTES, NULL);
	if (dserr)
		return dserr;
	return 0;
}

NWDSCCODE NWDSPutClassItem(
		NWDSContextHandle	ctx,
		Buf_T*			buf,
		const NWDSChar*		item
) {
	NWDSCCODE dserr;

	if (!buf)
		return ERR_NULL_POINTER;
	if (!(buf->bufFlags & NWDSBUFT_INPUT))
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_DEFINE_CLASS:
			if (!buf->attrCountPtr)
				return ERR_BAD_VERB;
			break;
		/* DSV_READ_ATTR_DEF should not be there... NWDSPutAttrName 
		   should be used with NWDSReadAttrDef */
		case DSV_READ_ATTR_DEF:
		case DSV_READ_CLASS_DEF:
		case DSV_MODIFY_CLASS_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSCtxBufString(ctx, buf, item);
	if (dserr)
		return dserr;
	DSET_LH(buf->attrCountPtr, 0, DVAL_LH(buf->attrCountPtr, 0) + 1);
	return 0;
}
	
NWDSCCODE NWDSBeginClassItem(
	UNUSED(	NWDSContextHandle	ctx),
		Buf_T*			buf
) {
	NWDSCCODE dserr;
	nuint8* p;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (!(buf->bufFlags & NWDSBUFT_INPUT))
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_DEFINE_CLASS:
			break;
		default:
			return ERR_BAD_VERB;
	}
	p = buf->curPos;
	dserr = NWDSBufPutLE32(buf, 0);
	if (dserr)
		return dserr;
	buf->attrCountPtr = p;
	return 0;
}

static inline NWDSCCODE __NWDSFindSchemaServer(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE*		conn
) {
	NWObjectID id;
	
	return __NWDSResolveName2w(ctx, L"[Root]", 
			DS_RESOLVE_WRITEABLE | DS_RESOLVE_V0, conn, &id);
}

static NWDSCCODE __NWDSDefineClassV0(
		NWCONN_HANDLE		conn,
		const Class_Info_T*	classInfo,
		Buf_T*			className,
		Buf_T*			classItems
) {
	NW_FRAGMENT rq_frag[4];
	nuint8 rq_b[8];
	nuint8 rq_b2[4 + MAX_ASN1_NAME];
	size_t asnlen;
	size_t asnlen2;
	
	if (!classInfo || !classItems)
		return ERR_NULL_POINTER;
	asnlen = classInfo->asn1ID.length;
	if (asnlen > MAX_ASN1_NAME)
		return NWE_BUFFER_OVERFLOW;
	DSET_LH(rq_b, 0, 0);	/* version */
	DSET_LH(rq_b, 4, classInfo->classFlags);
	DSET_LH(rq_b2, 0, asnlen);
	memcpy(rq_b2 + 4, classInfo->asn1ID.data, asnlen);
	asnlen2 = ROUNDPKT(asnlen);
	if (asnlen2 > asnlen)
		memset(rq_b2 + 4 + asnlen, 0, asnlen2 - asnlen);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 8;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(className, &rq_frag[1].fragSize);
	rq_frag[2].fragAddr.ro = rq_b2;
	rq_frag[2].fragSize = 4 + asnlen2;
	rq_frag[3].fragAddr.ro = NWDSBufRetrieve(classItems, &rq_frag[3].fragSize);
	
	return NWCFragmentRequest(conn, DSV_DEFINE_CLASS, 4, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSDefineClass(
		NWDSContextHandle	ctx,
		const NWDSChar*		className,
		const Class_Info_T*	classInfo,
		Buf_T*			classItems
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	nuint8 rq[4 + MAX_SCHEMA_NAME_BYTES];
	Buf_T rqb;
	
	NWDSSetupBuf(&rqb, rq, sizeof(rq));
	dserr = NWDSCtxBufString(ctx, &rqb, className);
	if (dserr)
		return dserr;
	dserr = __NWDSFindSchemaServer(ctx, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSDefineClassV0(conn, classInfo, &rqb, classItems);
	NWCCCloseConn(conn);
	return dserr;			
}

static NWDSCCODE __NWDSRemoveClassDefV0(
		NWCONN_HANDLE		conn,
		Buf_T*			className
) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[4];
	
	DSET_LH(rq_b, 0, 0);	/* version */

	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 4;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(className, &rq_frag[1].fragSize);
	
	return NWCFragmentRequest(conn, DSV_REMOVE_CLASS_DEF, 2, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSRemoveClassDef(
		NWDSContextHandle	ctx,
		const NWDSChar*		className
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	nuint8 rq[4 + MAX_SCHEMA_NAME_BYTES];
	Buf_T rqb;
	
	NWDSSetupBuf(&rqb, rq, sizeof(rq));
	dserr = NWDSCtxBufString(ctx, &rqb, className);
	if (dserr)
		return dserr;
	dserr = __NWDSFindSchemaServer(ctx, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSRemoveClassDefV0(conn, &rqb);
	NWCCCloseConn(conn);
	return dserr;
}

static NWDSCCODE __NWDSModifyClassDefV0(
		NWCONN_HANDLE		conn,
		Buf_T*			className,
		Buf_T*			optionalAttrs
) {
	NW_FRAGMENT rq_frag[3];
	nuint8 rq_b[4];

	if (!optionalAttrs)
		return ERR_NULL_POINTER;	
	if (optionalAttrs->operation != DSV_MODIFY_CLASS_DEF)
		return ERR_BAD_VERB;
	DSET_LH(rq_b, 0, 0);	/* version */

	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 4;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(className, &rq_frag[1].fragSize);
	rq_frag[2].fragAddr.ro = NWDSBufRetrieve(optionalAttrs, &rq_frag[2].fragSize);
	return NWCFragmentRequest(conn, DSV_MODIFY_CLASS_DEF, 3, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSModifyClassDef(
		NWDSContextHandle	ctx,
		const NWDSChar*		className,
		Buf_T*			optionalAttrs
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	nuint8 rq[4 + MAX_SCHEMA_NAME_BYTES];
	Buf_T rqb;
	
	NWDSSetupBuf(&rqb, rq, sizeof(rq));
	dserr = NWDSCtxBufString(ctx, &rqb, className);
	if (dserr)
		return dserr;
	dserr = __NWDSFindSchemaServer(ctx, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSModifyClassDefV0(conn, &rqb, optionalAttrs);
	NWCCCloseConn(conn);
	return dserr;
}

static NWDSCCODE __NWDSListContainableClassesV0(
		NWCONN_HANDLE		conn,
		nuint32*		iterHandle,
		NWObjectID		objID,
		Buf_T*			containableClasses
) {
	NW_FRAGMENT rq_frag[1];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[12];
	nuint8 rp_b[4];
	NWDSCCODE dserr;
	
	if (!containableClasses)
		return ERR_NULL_POINTER;
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, *iterHandle);
	DSET_HL(rq_b, 8, objID);
	
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	NWDSBufStartPut(containableClasses, DSV_LIST_CONTAINABLE_CLASSES);
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 4;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(containableClasses, &rp_frag[1].fragSize);
	dserr = NWCFragmentRequest(conn, DSV_LIST_CONTAINABLE_CLASSES, 
			1, rq_frag, 2, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[1].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	*iterHandle = DVAL_LH(rp_b, 0);
	NWDSBufPutSkip(containableClasses, rp_frag[1].fragSize);
	NWDSBufFinishPut(containableClasses);
	return 0;
}

NWDSCCODE NWDSListContainableClasses(
		NWDSContextHandle	ctx,
		const NWDSChar*		parentName,
		nuint32*		iterHandle,
		Buf_T*			containableClasses
) {
	NWCONN_HANDLE conn;
	NWObjectID objID;
	nuint32 lh;
	NWDSCCODE dserr;
	struct wrappedIterationHandle* ih;
	
	if (*iterHandle == NO_MORE_ITERATIONS) {
		dserr = NWDSResolveName2(ctx, parentName, 
				DS_RESOLVE_READABLE | DS_RESOLVE_V0, &conn,
				&objID);
		if (dserr)
			return dserr;
		ih = NULL;
		lh = NO_MORE_ITERATIONS;	
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_LIST_CONTAINABLE_CLASSES);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		objID = ih->objectID;
		lh = ih->iterHandle;
	}
	dserr = __NWDSListContainableClassesV0(conn, &lh, objID, containableClasses);
	if (ih)
		return __NWDSIHUpdate(ih, dserr, lh, iterHandle);
	return __NWDSIHCreate(dserr, conn, objID, lh, DSV_LIST_CONTAINABLE_CLASSES, iterHandle);
}

#define ATTRDEF_SYNTAXID	0x0001
#define ATTRDEF_TIMES		0x0002
static NWDSCCODE __NWDSReadAttrDefV0(
		NWCONN_HANDLE		conn,
		nuint32*		iterHandle,
		nuint			infoType,
		nuint			allAttrs,
		Buf_T*			attrNames,
		Buf_T*			attrDefs
) {
	static const nuint it2cmd[] = {
		0,
		ATTRDEF_SYNTAXID,
		ATTRDEF_SYNTAXID | ATTRDEF_TIMES
	};
	NW_FRAGMENT rq_frag[2];
	NW_FRAGMENT rp_frag[2];
	nuint8 rq_b[20];
	nuint8 rp_b[8];
	NWDSCCODE dserr;
	size_t rq_frags;

	if (!attrDefs)
		return ERR_NULL_POINTER;
	DSET_LH(rq_b, 0, 0);
	DSET_LH(rq_b, 4, *iterHandle);
	DSET_LH(rq_b, 8, infoType);
	DSET_LH(rq_b, 12, allAttrs);
	
	rq_frag[0].fragAddr.ro = rq_b;
	
	if (allAttrs || !attrNames) {
		DSET_LH(rq_b, 16, 0);
		rq_frag[0].fragSize = 20;
		rq_frags = 1;
	} else {
		if (attrNames->operation != DSV_READ_ATTR_DEF)
			return ERR_BAD_VERB;
		rq_frag[0].fragSize = 16;
		rq_frags = 2;
		rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrNames, &rq_frag[1].fragSize);
	}
	NWDSBufStartPut(attrDefs, DSV_READ_ATTR_DEF);
	if (infoType > 2)
		attrDefs->cmdFlags = ATTRDEF_SYNTAXID;
	else
		attrDefs->cmdFlags = it2cmd[infoType];
	rp_frag[0].fragAddr.rw = rp_b;
	rp_frag[0].fragSize = 8;
	rp_frag[1].fragAddr.rw = NWDSBufPutPtrLen(attrDefs, &rp_frag[1].fragSize);
	dserr = NWCFragmentRequest(conn, DSV_READ_ATTR_DEF, rq_frags, rq_frag, 
			2, rp_frag, NULL);
	if (dserr)
		return dserr;
	if (rp_frag[1].fragSize < 4)
		return ERR_INVALID_SERVER_RESPONSE;
	if (infoType != DVAL_LH(rp_b, 4))
		return ERR_INVALID_SERVER_RESPONSE;
	*iterHandle = DVAL_LH(rp_b, 0);
	NWDSBufPutSkip(attrDefs, rp_frag[1].fragSize);
	NWDSBufFinishPut(attrDefs);
	return 0;
}

NWDSCCODE NWDSReadAttrDef(
		NWDSContextHandle	ctx,
		nuint			infoType,
		nuint			allAttrs,
		Buf_T*			attrNames,
		nuint32*		iterHandle,
		Buf_T*			attrDefs
) {
	NWCONN_HANDLE conn;
	nuint32 lh;
	NWDSCCODE dserr;
	struct wrappedIterationHandle* ih;
	
	if (*iterHandle == NO_MORE_ITERATIONS) {
		dserr = __NWDSGetConnection(ctx, &conn);
		if (dserr)
			return dserr;
		ih = NULL;
		lh = NO_MORE_ITERATIONS;	
	} else {
		ih = __NWDSIHLookup(*iterHandle, DSV_READ_ATTR_DEF);
		if (!ih)
			return ERR_INVALID_HANDLE;
		conn = ih->conn;
		lh = ih->iterHandle;
	}
	dserr = __NWDSReadAttrDefV0(conn, &lh, infoType, allAttrs, attrNames, attrDefs);
	if (ih)
		return __NWDSIHUpdate(ih, dserr, lh, iterHandle);
	return __NWDSIHCreate(dserr, conn, 0, lh, DSV_READ_ATTR_DEF, iterHandle);
}

NWDSCCODE NWDSGetAttrDef(
		NWDSContextHandle	ctx,
		Buf_T*			buf,
		NWDSChar*		attrName,
		Attr_Info_T*		attrInfo
) {
	NWDSCCODE dserr;
	nuint32 tmp;
	
	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_ATTR_DEF:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufCtxString(ctx, buf, attrName, MAX_SCHEMA_NAME_BYTES, NULL);
	if (dserr)
		return dserr;
	if (buf->cmdFlags & ATTRDEF_SYNTAXID) {
		dserr = NWDSBufGetLE32(buf, &tmp);
		if (dserr)
			return dserr;
		if (attrInfo)
			attrInfo->attrFlags = tmp;
		dserr = NWDSBufGetLE32(buf, &tmp);
		if (dserr)
			return dserr;
		if (attrInfo)
			attrInfo->attrSyntaxID = tmp;
		dserr = NWDSBufGetLE32(buf, &tmp);
		if (dserr)
			return dserr;
		if (attrInfo)
			attrInfo->attrLower = tmp;
		dserr = NWDSBufGetLE32(buf, &tmp);
		if (dserr)
			return dserr;
		if (attrInfo)
			attrInfo->attrUpper = tmp;
		if (attrInfo) {
			dserr = NWDSBufGetLE32(buf, &tmp);
			if (dserr)
				return dserr;
			attrInfo->asn1ID.length = tmp;
			if (tmp > MAX_ASN1_NAME)
				return NWE_BUFFER_OVERFLOW;
			dserr = NWDSBufGet(buf, attrInfo->asn1ID.data, tmp);		
		} else {
			dserr = NWDSBufSkipBuffer(buf);
		}
		if (dserr)
			return dserr;
	} else if (attrInfo) {
		attrInfo->attrFlags = 0;
		attrInfo->attrSyntaxID = SYN_UNKNOWN;
		attrInfo->attrLower = 0;
		attrInfo->attrUpper = 0;
		attrInfo->asn1ID.length = 0;
	}
	return dserr;
}

static NWDSCCODE __NWDSDefineAttrV0(
		NWCONN_HANDLE		conn,
		const Attr_Info_T*	attrInfo,
		Buf_T*			attrName
) {
	NW_FRAGMENT rq_frag[3];
	nuint8 rq_b[8];
	nuint8 rq_b2[16 + MAX_ASN1_NAME];
	size_t asnlen;
	size_t asnlen2;
	
	if (!attrInfo)
		return ERR_NULL_POINTER;
	asnlen = attrInfo->asn1ID.length;
	if (asnlen > MAX_ASN1_NAME)
		return NWE_BUFFER_OVERFLOW;
	DSET_LH(rq_b, 0, 0);	/* version */
	DSET_LH(rq_b, 4, attrInfo->attrFlags);
	DSET_LH(rq_b2, 0, attrInfo->attrSyntaxID);
	DSET_LH(rq_b2, 4, attrInfo->attrLower);
	DSET_LH(rq_b2, 8, attrInfo->attrUpper);
	DSET_LH(rq_b2, 12, asnlen);
	memcpy(rq_b2 + 16, attrInfo->asn1ID.data, asnlen);
	asnlen2 = ROUNDPKT(asnlen);
	if (asnlen2 > asnlen)
		memset(rq_b2 + 4 + asnlen, 0, asnlen2 - asnlen);
	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 8;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrName, &rq_frag[1].fragSize);
	rq_frag[2].fragAddr.ro = rq_b2;
	rq_frag[2].fragSize = 16 + asnlen2;
	
	return NWCFragmentRequest(conn, DSV_DEFINE_ATTR, 3, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSDefineAttr(
		NWDSContextHandle	ctx,
		const NWDSChar*		attrName,
		const Attr_Info_T*	attrInfo
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	nuint8 rq[4 + MAX_SCHEMA_NAME_BYTES];
	Buf_T rqb;
	
	NWDSSetupBuf(&rqb, rq, sizeof(rq));
	dserr = NWDSCtxBufString(ctx, &rqb, attrName);
	if (dserr)
		return dserr;
	dserr = __NWDSFindSchemaServer(ctx, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSDefineAttrV0(conn, attrInfo, &rqb);
	NWCCCloseConn(conn);
	return dserr;			
}

static NWDSCCODE __NWDSRemoveAttrDefV0(
		NWCONN_HANDLE		conn,
		Buf_T*			attrName
) {
	NW_FRAGMENT rq_frag[2];
	nuint8 rq_b[4];
	
	DSET_LH(rq_b, 0, 0);	/* version */

	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 4;
	rq_frag[1].fragAddr.ro = NWDSBufRetrieve(attrName, &rq_frag[1].fragSize);
	
	return NWCFragmentRequest(conn, DSV_REMOVE_ATTR_DEF, 2, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSRemoveAttrDef(
		NWDSContextHandle	ctx,
		const NWDSChar*		attrName
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	nuint8 rq[4 + MAX_SCHEMA_NAME_BYTES];
	Buf_T rqb;
	
	NWDSSetupBuf(&rqb, rq, sizeof(rq));
	dserr = NWDSCtxBufString(ctx, &rqb, attrName);
	if (dserr)
		return dserr;
	dserr = __NWDSFindSchemaServer(ctx, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSRemoveAttrDefV0(conn, &rqb);
	NWCCCloseConn(conn);
	return dserr;
}

static NWDSCCODE __NWDSSyncSchemaV0(
		NWCONN_HANDLE		conn,
		nuint32			flags,
		nuint32a		timev
) {
	NW_FRAGMENT rq_frag[1];
	nuint8 rq_b[12];
	
	DSET_LH(rq_b, 0, 0);	/* version */
	DSET_LH(rq_b, 4, flags);
	DSET_LH(rq_b, 8, timev);

	rq_frag[0].fragAddr.ro = rq_b;
	rq_frag[0].fragSize = 12;
	return NWCFragmentRequest(conn, DSV_SYNC_SCHEMA, 1, rq_frag, 0, NULL, NULL);	
}

NWDSCCODE NWDSSyncSchema(
		NWDSContextHandle	ctx,
		const NWDSChar*		serverName,
		nuint32a		seconds
) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSSyncSchemaV0(conn, 0, seconds);
	NWCCCloseConn(conn);
	return dserr;
}

