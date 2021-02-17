/*
    syntaxes.c
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

	1.00  2001, January 4		patrick pollet <patrick.pollet@insa-lyon.fr>
		Initial version.

	1.01  2001, January 16		patrick pollet <patrick.pollet@insa-lyon.fr>
                 Added NWDSGetSyntaxID.

	1.02  2001, March 9		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added DS_BITFIELD_EQUALS into definition table.

*/


#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "nwnet_i.h"

#include <errno.h>
#include <stdlib.h>

//internal
struct __Syntax_Info_T
{
   nuint32  ID;
   const wchar_t *defStr;
   const wchar_t *name;
   nuint16  flags;
};


/* do no trash the caller's stack by returning more than the usual !
   no length parameter are specified in the standard API calls ! */
#define MAX_RETURN_LENGTH MAX_SCHEMA_NAME_BYTES

static const struct __Syntax_Info_T syntaxesTable[] = {
/*****************************/
{SYN_UNKNOWN,		L"SYN_UNKNOWN",		L"Unknown",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_DIST_NAME,		L"SYN_DIST_NAME",	L"Distinguished Name",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_CE_STRING,		L"SYN_CE_STRING",	L"Case Exact String",
		DS_STRING|DS_SUPPORT_EQUALS|DS_SIZEABLE},
{SYN_CI_STRING,		L"SYN_CI_STRING",	L"Case Ignore String",
		DS_STRING|DS_SUPPORT_EQUALS|DS_IGNORE_CASE|DS_SIZEABLE},
{SYN_PR_STRING,		L"SYN_PR_STRING",	L"Printable String",
		DS_STRING|DS_SUPPORT_EQUALS|DS_ONLY_PRINTABLE|DS_SIZEABLE},
{SYN_NU_STRING,		L"SYN_NU_STRING",	L"Numeric String",
		DS_STRING|DS_SUPPORT_EQUALS|DS_IGNORE_SPACE|DS_ONLY_DIGITS|DS_SIZEABLE},
{SYN_CI_LIST,		L"SYN_CI_LIST",		L"Case Ignore List",
		DS_SUPPORT_EQUALS|DS_IGNORE_CASE},
{SYN_BOOLEAN,		L"SYN_BOOLEAN",		L"Boolean",
		DS_SINGLE_VALUED|DS_SUPPORT_EQUALS},
{SYN_INTEGER,		L"SYN_INTEGER",		L"Integer",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_SIZEABLE|DS_BITWISE_EQUALS},
{SYN_OCTET_STRING,	L"SYN_OCTET_STRING",	L"Octet String",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_SIZEABLE|DS_BITWISE_EQUALS},
{SYN_TEL_NUMBER,	L"SYN_TEL_NUMBER",	L"Telephone Number",
		DS_STRING|DS_SUPPORT_EQUALS|DS_IGNORE_SPACE|DS_IGNORE_DASH|DS_SIZEABLE},
{SYN_FAX_NUMBER,	L"SYN_FAX_NUMBER",	L"Facsimile Telephone Number",
		DS_SUPPORT_EQUALS|DS_IGNORE_SPACE|DS_IGNORE_DASH},
{SYN_NET_ADDRESS,	L"SYN_NET_ADDRESS",	L"Net Address",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_OCTET_LIST,	L"SYN_OCTET_LIST",	L"Octet List",
		DS_SUPPORT_EQUALS},
{SYN_EMAIL_ADDRESS,	L"SYN_EMAIL_ADDRESS",	L"EMail Address",
		DS_SUPPORT_EQUALS|DS_IGNORE_CASE|DS_IGNORE_SPACE},
{SYN_PATH,		L"SYN_PATH",		L"Path",
		DS_SUPPORT_EQUALS|DS_SIZEABLE},
{SYN_REPLICA_POINTER,	L"SYN_REPLICA_POINTER",	L"Replica Pointer",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_OBJECT_ACL,	L"SYN_OBJECT_ACL",	L"Object ACL",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_PO_ADDRESS,	L"SYN_PO_ADDRESS",	L"Postal Address",
		DS_SUPPORT_EQUALS|DS_IGNORE_CASE},
{SYN_TIMESTAMP,		L"SYN_TIMESTAMP",	L"Timestamp",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_CLASS_NAME,	L"SYN_CLASS_NAME",	L"Class Name",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_STREAM,		L"SYN_STREAM",		L"Stream",
		DS_SINGLE_VALUED},
{SYN_COUNTER,		L"SYN_COUNTER",		L"Counter",
		DS_SINGLE_VALUED|DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS},
{SYN_BACK_LINK,		L"SYN_BACK_LINK",	L"Back Link",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_TIME,		L"SYN_TIME",		L"Time",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_TYPED_NAME,	L"SYN_TYPED_NAME",	L"Typed Name",
		DS_SUPPORT_EQUALS|DS_BITWISE_EQUALS},
{SYN_HOLD,		L"SYN_HOLD",		L"Hold",
		DS_SUPPORT_EQUALS},
{SYN_INTERVAL,		L"SYN_INTERVAL",	L"Interval",
		DS_SUPPORT_ORDER|DS_SUPPORT_EQUALS|DS_SIZEABLE|DS_BITWISE_EQUALS},
{0,			NULL,			NULL,
		0}};


NWDSCCODE NWDSReadSyntaxes (
	UNUSED(	NWDSContextHandle ctx),
	UNUSED(	nuint             infoType),
		nuint             allSyntaxes,
		Buf_T*            syntaxNames,
		nuint32*          iterHandle,
		Buf_T*            syntaxDefs) {
	if (!syntaxDefs)
		return ERR_NULL_POINTER;

	if (iterHandle && *iterHandle != NO_MORE_ITERATIONS)
		return EINVAL;

	NWDSBufStartPut(syntaxDefs,DSV_READ_SYNTAXES);
	if (allSyntaxes) {
		const struct __Syntax_Info_T* i;
		NWDSCCODE dserr;

		dserr = NWDSBufPutLE32(syntaxDefs, SYNTAX_COUNT);
		if (dserr)
			return dserr;
		for (i = syntaxesTable; i->name; i++) {
			dserr = NWDSBufPutLE32(syntaxDefs, i->ID);
			if (dserr)
				return dserr;
		}
	} else {
		nuint8* p;
		unsigned int i;
		nuint32 syntCnt;
		nuint32 cnt;
		NWDSCCODE dserr;

		if (!syntaxNames)
			return ERR_NULL_POINTER;

		cnt = 0;
		p = NWDSBufPutPtr(syntaxDefs, 4); /* keep the mark ! */
		if (!p)
			return ERR_BUFFER_FULL;

		/* rewind input Buffer !!! */
		NWDSBufFinishPut(syntaxNames);
		dserr = NWDSBufGetLE32(syntaxNames, &syntCnt); /* how much names he has put ? */
		if (dserr)
			return dserr;

		for (i = 0; i < syntCnt; i++) {
			const struct __Syntax_Info_T* j;
			wchar_t tmpStr[MAX_SCHEMA_NAME_CHARS+1];

			dserr = NWDSBufDN(syntaxNames, tmpStr, sizeof(tmpStr));
			if (dserr)
				return dserr;
			/* find a match in the table */
			for (j = syntaxesTable; j->name; j++) {
				if (!wcscasecmp(j->name, tmpStr)) {
					/* found: add the syntax number in buffer */
					dserr = NWDSBufPutLE32(syntaxDefs, j->ID);
					if (dserr)
						return dserr;
					cnt++;
					break;
				}
			}
			/* FIXME: What if value was not found?! */
		}
		DSET_LH(p, 0, cnt); /* final value of count */
	}
	NWDSBufFinishPut(syntaxDefs);
	return 0;
}



NWDSCCODE NWDSPutSyntaxName(
		NWDSContextHandle ctx,
		Buf_T*            buf,
		const NWDSChar*   syntaxName) {
	NWDSCCODE dserr;

	/* same code as PutClassItem except verb checking ;-) */
	if (!buf || !syntaxName)
		return ERR_NULL_POINTER;
	if (!(buf->bufFlags & NWDSBUFT_INPUT))
		return ERR_BAD_VERB;
	if (!buf->attrCountPtr)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_SYNTAXES:
			break;
		default:
			return ERR_BAD_VERB;
	}

	dserr = NWDSCtxBufString(ctx, buf, syntaxName);
	if (dserr)
		return dserr;
	DSET_LH(buf->attrCountPtr, 0, DVAL_LH(buf->attrCountPtr, 0) + 1);
	return 0;
}

NWDSCCODE NWDSGetSyntaxDef (
		NWDSContextHandle ctx,
		Buf_T*            buf,
		NWDSChar*         syntaxName,
		Syntax_Info_T*    syntaxDef) {
	NWDSCCODE dserr;
	nuint32 syntNum;

	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_SYNTAXES:
			break;
		default:
			return ERR_BAD_VERB;
	}
        /* read the next number in the buffer */
	dserr = NWDSBufGetLE32(buf, &syntNum);
	if (dserr)
		return dserr;
	if (syntNum >= SYNTAX_COUNT)
		return -1;
	if (syntaxName) {
		dserr = NWDSXlateToCtx(ctx, syntaxName, MAX_RETURN_LENGTH,
				syntaxesTable[syntNum].name, NULL);
		if (dserr)
			return dserr;
	}
	if (syntaxDef) {
		dserr = NWDSReadSyntaxDef(ctx, syntNum, syntaxDef);
		if (dserr)
			return dserr;
	}
	return 0;
}

NWDSCCODE NWDSGetSyntaxCount(
	UNUSED(	NWDSContextHandle ctx),
		Buf_T*            buf,
		NWObjectCount*    syntaxCount){
	NWDSCCODE dserr;
	nuint32 tmp;

	if (!buf)
		return ERR_NULL_POINTER;
	if (buf->bufFlags & NWDSBUFT_INPUT)
		return ERR_BAD_VERB;
	switch (buf->operation) {
		case DSV_READ_SYNTAXES:
			break;
		default:
			return ERR_BAD_VERB;
	}
	dserr = NWDSBufGetLE32(buf, &tmp);
	if (dserr)
		return dserr;
	if (syntaxCount)
		*syntaxCount = tmp;
	return 0;
}


NWDSCCODE NWDSReadSyntaxDef (
		NWDSContextHandle ctx,
		enum SYNTAX       syntaxID,
		Syntax_Info_T *   syntaxDef) {
	if (syntaxID >= SYNTAX_COUNT)
		return -1;
	if (!syntaxDef)
		return ERR_NULL_POINTER;

	syntaxDef->ID    =  syntaxesTable[syntaxID].ID;
	syntaxDef->flags =  syntaxesTable[syntaxID].flags;
	return NWDSXlateToCtx(ctx, syntaxDef->defStr, MAX_RETURN_LENGTH,
				syntaxesTable[syntaxID].defStr, NULL);
}


NWDSCCODE NWDSGetSyntaxID(
		NWDSContextHandle ctx,
		const NWDSChar*   attrName,
		enum SYNTAX*      syntaxID){
	NWDSCCODE dserr;
	Buf_T* ibuf;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	NWObjectCount cnt;
	Attr_Info_T ainfo;
	nuint32 ih = NO_MORE_ITERATIONS;

	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &ibuf);
	if (dserr)
		goto quit;
	dserr = NWDSInitBuf(ctx, DSV_READ_ATTR_DEF, ibuf);
	if (dserr)
		goto quitibuf;
	dserr = NWDSAllocBuf(size, &buf);
	if (dserr)
		goto quitbufibuf;
	dserr = NWDSPutClassName(ctx, ibuf, attrName);
	if (dserr)
		goto quitbufibuf;
	dserr = NWDSReadAttrDef(ctx, 1, 0, ibuf, &ih, buf);
	if (dserr)
		goto quitbufibuf;

	dserr = NWDSGetAttrCount(ctx, buf, &cnt);
	if (dserr)
		goto quitbufibuf;
	if (cnt != 1) {
		dserr = ERR_INVALID_SERVER_RESPONSE;
		goto quitbufibuf;
	}

	dserr = NWDSGetAttrDef(ctx, buf, NULL, &ainfo);
	if (dserr)
		goto quitbufibuf;
	if (syntaxID)
		*syntaxID = ainfo.attrSyntaxID;
quitbufibuf:;
	NWDSFreeBuf(buf);
quitibuf:;
	NWDSFreeBuf(ibuf);
quit:;
	return dserr;
}
