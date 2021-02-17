/*
    dsgetstat.c - NWDSGetNDSStatistics()
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

	1.00  2000, February 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version, copied from nwdsgethost demo.
		
	1.01  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Turned from example into real API

	1.02  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSResetNDSStatistics.

*/

#include <ncp/nwcalls.h>
#include "nwnet_i.h"
#include <string.h>

static NWDSCCODE __NWDSGetNDSStatistics(
		NWCONN_HANDLE	conn,
		nuint32		requestMask,
		size_t		statsInfoLen,
		NDSStatsInfo_T*	statsInfo
) {
	nuint8 rqbuffer[5];
	nuint32 rpbuffer[32];
	NW_FRAGMENT rp;
	NWDSCCODE dserr;

	BSET(rqbuffer, 0, DS_NCP_GET_DS_STATISTICS);
	DSET_LH(rqbuffer, 1, requestMask | 0x00000001); 
	
	rp.fragAddr.rw = rpbuffer;
	rp.fragSize = sizeof(rpbuffer);
	dserr = NWRequestSimple(conn, DS_NCP_VERB, rqbuffer, 5, &rp);
	if (!dserr) {
		size_t rplen;

		rplen = rp.fragSize;
		if (rplen < 4) {
			dserr = NWE_INVALID_NCP_PACKET_LENGTH;
		} else {
			size_t pos;
			int i;
			nuint32 flags;

			flags = DVAL_LH(rpbuffer, 0);
			statsInfo->statsVersion = flags;
			pos = 4;
			for (i = 1; i < 32; i++) {

				nuint32 val;
				
				if (flags & (1 << i)) {
					if (pos + 4 > rplen) {
						dserr = NWE_INVALID_NCP_PACKET_LENGTH;
						break;
					}
					val = DVAL_LH(rpbuffer, pos);
					pos += 4;
				} else
					val = 0;
				switch (i) {
#define endof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER + sizeof(((TYPE *)0)->MEMBER))
#define ISI(v,x) case v: if (endof(NDSStatsInfo_T, x) <= statsInfoLen) statsInfo->x = val; break
					ISI(1, noSuchEntry);
					ISI(2, localEntry);
					ISI(3, typeReferral);
					ISI(4, aliasReferral);
					ISI(5, requestCount);
					ISI(6, requestDataSize);
					ISI(7, replyDataSize);
					ISI(8, resetTime);
					ISI(9, transportReferral);
					ISI(10, upReferral);
					ISI(11, downReferral);
					/* 12..31 are reserved */
#undef ISI
				}
			}
		}
	}
	return dserr;
}
	
NWDSCCODE NWDSGetNDSStatistics(
		NWDSContextHandle	ctx,
		const NWDSChar*		serverName,
		size_t			statsInfoLen,
		NDSStatsInfo_T*		statsInfo) {
	NWCONN_HANDLE conn;
	NWDSCCODE dserr;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSGetNDSStatistics(conn, 0x01FFFFFF, statsInfoLen, statsInfo);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSResetNDSStatistics(
		NWCONN_HANDLE		conn
) {
	static const nuint8 rqbuffer[1] = { DS_NCP_RESET_DS_COUNTERS };

	return NWRequestSimple(conn, DS_NCP_VERB, rqbuffer, 1, NULL);
}
	
NWDSCCODE NWDSResetNDSStatistics(
		NWDSContextHandle	ctx,
		const NWDSChar*		serverName
) {
	NWCONN_HANDLE conn;
	NWDSCCODE dserr;
	
	dserr = NWDSOpenConnToNDSServer(ctx, serverName, &conn);
	if (dserr)
		return dserr;
	dserr = __NWDSResetNDSStatistics(conn);
	NWCCCloseConn(conn);
	return dserr;	
}

static NWDSCCODE __NWDSGetDSVerInfo(
		NWCONN_HANDLE	conn,
		nuint32		version,
		nuint32		requestMask,
		Buf_T*		reply
) {
	nuint8 rqbuffer[9];
	NW_FRAGMENT rp;
	NWDSCCODE dserr;
	void* rpptr;
	size_t maxl;

	if (!reply)
		return ERR_NULL_POINTER;
	BSET(rqbuffer, 0, DS_NCP_PING);
	DSET_LH(rqbuffer, 1, version);
	DSET_LH(rqbuffer, 5, requestMask); 

	NWDSBufStartPut(reply, 0);
	rpptr = NWDSBufRetrievePtrAndLen(reply, &maxl);
	
	rp.fragAddr.rw = rpptr;
	rp.fragSize = maxl;
	dserr = NWRequestSimple(conn, DS_NCP_VERB, rqbuffer, 9, &rp);
	if (dserr)
		return dserr;
	if (rp.fragSize > maxl)
		return NWE_BUFFER_OVERFLOW;
	NWDSBufPutSkip(reply, rp.fragSize);
	NWDSBufFinishPut(reply);
#if 0
	{
		nuint8* p;

		for (p = reply->curPos; p < reply->dataend; p++)
			printf("%02X ", *p);
		printf("\n");
	}
#endif
	return 0;
}
	
NWDSCCODE NWDSGetDSVerInfo(
		NWCONN_HANDLE	conn,
		nuint32*	dsVersion,
		nuint32*	rootMostEntryDepth,
		char*		sapName,
		nuint32*	flags,
		wchar_t*	treeName
) {
	NWDSCCODE dserr;
	nuint8 buffer[1024];
	Buf_T buf;
	nuint32 rqflags = 0;
	nuint32 tmp;
	nuint32 version;

	if (rootMostEntryDepth)
		rqflags |= DSPING_DEPTH;
	else
		rootMostEntryDepth = &tmp;
	if (dsVersion)
		rqflags |= DSPING_BUILD_NUMBER;
	else
		dsVersion = &tmp;
	if (flags)
		rqflags |= DSPING_FLAGS;
	else
		flags = &tmp;
	if (sapName)
		rqflags |= DSPING_SAP_NAME;
	if (treeName)
		rqflags |= DSPING_TREE_NAME;

	version = 0;
	if (rqflags & ~(DSPING_SUPPORTED_FIELDS | DSPING_DEPTH | 
			DSPING_BUILD_NUMBER | DSPING_FLAGS | DSPING_SAP_NAME | 
			DSPING_TREE_NAME)) {
		version = 1;
	}
	NWDSSetupBuf(&buf, buffer, sizeof(buffer));
	dserr = __NWDSGetDSVerInfo(conn, version, rqflags, &buf);
	if (dserr)
		return dserr;
	dserr = NWDSBufGetLE32(&buf, &version);
	if (dserr)
		return dserr;
	switch (version) {
		case 0x00000009:
			{
				char asciiName[MAX_TREE_NAME_CHARS + 1];
				nuint32 asciiNameLen;
				
				dserr = NWDSBufGetLE32(&buf, &asciiNameLen);
				if (dserr)
					return dserr;
				if (asciiNameLen > sizeof(asciiName))
					return NWE_BUFFER_OVERFLOW;
				if (asciiNameLen) {
					dserr = NWDSBufGet(&buf, asciiName, asciiNameLen);
					if (dserr)
						return dserr;
					if (asciiName[asciiNameLen-1])
						return ERR_INVALID_SERVER_RESPONSE;
					while (asciiNameLen > 1 && asciiName[asciiNameLen - 2] == '_')
						asciiNameLen--;
				} else {
					asciiNameLen = 1;
				}
				asciiName[asciiNameLen - 1] = 0;
				dserr = NWDSBufGetLE32(&buf, rootMostEntryDepth);
				if (dserr)
					return dserr;
				dserr = NWDSBufGetLE32(&buf, dsVersion);
				if (dserr)
					return dserr;
				dserr = NWDSBufGetLE32(&buf, flags);
				if (dserr)
					return dserr;
				if (sapName)
					memcpy(sapName, asciiName, asciiNameLen);
				if (treeName) {
					unsigned char* src = asciiName;
					do {
						*treeName++ = *src++;
					} while (--asciiNameLen);
				}
			}
			return 0;
		case 0x0000000A:
			if (rqflags & DSPING_SUPPORTED_FIELDS) {
				dserr = NWDSBufGetLE32(&buf, &rqflags);
				if (dserr)
					return dserr;
			}
			if (rqflags & DSPING_DEPTH) {
				dserr = NWDSBufGetLE32(&buf, rootMostEntryDepth);
				if (dserr)
					return dserr;
			}
			if (rqflags & DSPING_BUILD_NUMBER) {
				dserr = NWDSBufGetLE32(&buf, dsVersion);
				if (dserr)
					return dserr;
			}
			if (rqflags & DSPING_FLAGS) {
				dserr = NWDSBufGetLE32(&buf, flags);
				if (dserr)
					return dserr;
			}
			if (rqflags & DSPING_SAP_NAME) {
				dserr = NWDSBufGetLE32(&buf, &tmp);
				if (dserr)
					return dserr;
				if (tmp > MAX_TREE_NAME_CHARS + 1)
					return NWE_BUFFER_OVERFLOW;
				if (tmp) {
					dserr = NWDSBufGet(&buf, sapName, tmp);
					if (dserr)
						return dserr;
					if (sapName[tmp-1])
						return ERR_INVALID_SERVER_RESPONSE;
				} else {
					sapName[0] = 0;
				}
			}
			if (rqflags & DSPING_TREE_NAME) {
				dserr = NWDSBufDN(&buf, treeName, (MAX_TREE_NAME_CHARS + 1) * sizeof(wchar_t));
				if (dserr)
					return dserr;
			}
			break;
		default:
			return ERR_INVALID_API_VERSION;
	}	
	return 0;	
}

