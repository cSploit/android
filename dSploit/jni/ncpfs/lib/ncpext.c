/*
    ncpext.c
    Copyright (C) 2001  Petr Vandrovec

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

	1.00  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncplib_i.h"
#include "ncpcode.h"

#include <string.h>
#include <stdlib.h>

#define ncp_array_size(x) (sizeof(x)/sizeof((x)[0]))

NWCCODE
NWScanNCPExtensions(
		NWCONN_HANDLE conn,
		nuint32* iterHandle,
		char* name,
		nuint8* majorVersion,
		nuint8* minorVersion,
		nuint8* revision,
		nuint8 queryData[32]) {
	NWCCODE err;
	
	if (!iterHandle) {
		return NWE_PARAM_INVALID;
	}
	ncp_init_request_s(conn, 0);
	ncp_add_dword_lh(conn, *iterHandle);
	err = ncp_request(conn, 36);
	if (err) {
		ncp_unlock_conn(conn);
		return err;
	}
	if (conn->ncp_reply_size < 72) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*iterHandle = ncp_reply_dword_lh(conn, 0);
	if (majorVersion)
		*majorVersion = ncp_reply_byte(conn, 4);
	if (minorVersion)
		*minorVersion = ncp_reply_byte(conn, 5);
	if (revision)
		*revision = ncp_reply_byte(conn, 6);
	if (queryData)
		memcpy(queryData, ncp_reply_data(conn, 40), 32);
	if (name) {
		size_t namelen;
		
		namelen = ncp_reply_byte(conn, 7);
		if (namelen >= MAX_NCP_EXTENSION_NAME_BYTES) {
			ncp_unlock_conn(conn);
			return NWE_BUFFER_OVERFLOW;
		}
		memcpy(name, ncp_reply_data(conn, 8), namelen);
		name[namelen] = 0;
	}
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE
NWGetNumberNCPExtensions(
		NWCONN_HANDLE conn,
		nuint* exts) {
	NWCCODE err;
	u_int32_t num;
	NW_FRAGMENT rp;
	u_int32_t iterHandle;
	
	rp.fragAddr.rw = &num;
	rp.fragSize = sizeof(num);
	err = NWRequestSimple(conn, NCPC_SFN(36,3), NULL, 0, &rp);
	if (err) {
		if (err != NWE_NCP_NOT_SUPPORTED) {
			return err;
		}
		iterHandle = ~0;
		num = 0;
		while ((err = NWScanNCPExtensions(conn, &iterHandle, NULL, NULL, NULL, NULL, NULL)) == 0) {
			num++;
		}
		if (err != NWE_SERVER_FAILURE)
			return err;
	} else {
		if (rp.fragSize < 4)
			return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (exts)
		*exts = num;
	return 0;	
}

static NWCCODE
NWFragLen(size_t *total,
	  nuint cnt,
          const NW_FRAGMENT* frag)
{
	size_t len;
	
	if (cnt && !frag) {
		return ERR_NULL_POINTER;
	}
	len = 0;
	while (cnt--) {
		len += frag->fragSize;
		frag++;
	}
	*total = len;
	return 0;
}

typedef struct {
	nuint8* current;
	size_t space;
	int currentActive;
	nuint fragCount;
	NW_FRAGMENT* fragList;
} NWFragger;

static void
NWPushFragStart(NWFragger*   fragger,
                nuint        fragCount,
		NW_FRAGMENT* fragList)
{
	fragger->currentActive = 0;
	fragger->fragCount = fragCount;
	fragger->fragList = fragList;
}

static NWCCODE
NWPushFragPush(NWFragger*    fragger,
               const nuint8* data,
	       size_t        len)
{
	while (len) {
		if (!fragger->currentActive) {
			if (fragger->fragCount == 0) {
				return NWE_BUFFER_OVERFLOW;
			}
			fragger->current = fragger->fragList->fragAddr.rw;
			fragger->space = fragger->fragList->fragSize;
			fragger->currentActive = 1;
		}
		if (len >= fragger->space) {
			memcpy(fragger->current, data, fragger->space);
			len -= fragger->space;
			data += fragger->space;
			fragger->fragList++;
			fragger->fragCount--;
			fragger->currentActive = 0;
		} else {
			memcpy(fragger->current, data, len);
			fragger->current += len;
			fragger->space -= len;
			return 0;
		}
	}
	return 0;
}

static void
NWPushFragFinish(NWFragger*    fragger)
{
	if (fragger->currentActive) {
		fragger->fragList->fragSize -= fragger->space;
		fragger->fragList++;
		fragger->fragCount--;
	}
	while (fragger->fragCount--) {
		fragger->fragList->fragSize = 0;
		fragger->fragList++;
	}
}

NWCCODE
NWFragNCPExtensionRequest(
		NWCONN_HANDLE	conn,
		nuint32		NCPExtensionID,
		nuint		reqFragCount,
		NW_FRAGMENT*	reqFragList,
		nuint		replyFragCount,
		NW_FRAGMENT*	replyFragList)
{
	NWCCODE err;
	size_t reqLen;
	size_t rpLen;
	u_int16_t ver;

	err = NWFragLen(&reqLen, reqFragCount, reqFragList);
	if (err) {
		return err;
	}
	err = NWFragLen(&rpLen, replyFragCount, replyFragList);
	if (err) {
		return err;
	}
	err = NWGetFileServerVersion(conn, &ver);
	if (err) {
		return err;
	}
	if (reqLen < 523 && rpLen < 530 && (rpLen < 100 || ver > 0x030B)) {
		NW_FRAGMENT* rq;
		NW_FRAGMENT rp[2];
		u_int8_t rqhdr[8];
		u_int8_t rphdr[2];
		u_int8_t rpbuf[530];
		NWFragger fragger;
		
		rq = malloc(sizeof(NW_FRAGMENT) * (reqFragCount + 1));
		if (!rq) {
			return ERR_NOT_ENOUGH_MEMORY;
		}
		memcpy(rq + 1, reqFragList, sizeof(NW_FRAGMENT) * reqFragCount);
		rq->fragAddress = rqhdr;
		rq->fragSize = 8;

		WSET_HL(rqhdr, 0, reqLen + 6);
		DSET_LH(rqhdr, 2, NCPExtensionID);
		WSET_LH(rqhdr, 6, rpLen);
		
		rp[0].fragAddress = rphdr;
		rp[0].fragSize = 2;
		rp[1].fragAddress = rpbuf;
		rp[1].fragSize = 530;
		
		err = NWRequest(conn, 37, reqFragCount + 1, rq, 2, rp);
		free(rq);
		if (err) {
			return err;
		}

		NWPushFragStart(&fragger, replyFragCount, replyFragList);
		NWPushFragPush(&fragger, rpbuf, rp[1].fragSize);
		NWPushFragFinish(&fragger);
		return 0;
	} else {
		NW_FRAGMENT rq[2];
		u_int8_t rqhdr[20];
		u_int8_t rqbuf[519];
		u_int8_t rphdr[10];
		u_int8_t rpbuf[524];
		size_t flen;
		const u_int8_t* dptr;
		size_t dlen;
		NWFragger fragger;
		
		WSET_HL(rqhdr,  0, 0xFFFF);
		DSET_LH(rqhdr,  2, NCPExtensionID);
		DSET_LH(rqhdr,  6, 0);
//		WSET_LH(rqhdr, 10, 0);	//472
		DSET_LH(rqhdr, 12, reqLen);
		DSET_LH(rqhdr, 16, rpLen);
		
		rq[0].fragAddress = rqhdr;
		rq[0].fragSize = 20;
		
		flen = 511;

		if (reqLen) {
			dptr = reqFragList->fragAddr.ro;
			dlen = reqFragList->fragSize;
			reqFragList++;
		} else {
			dptr = NULL;
			dlen = 0;
		}
		do {
			NW_FRAGMENT rp[3];

			rq[1].fragSize = 0;
			if (reqLen) {
				if (dlen >= flen) {
					rq[1].fragAddr.ro = dptr;
					rq[1].fragSize = flen;
					dptr += flen;
					dlen -= flen;
					reqLen -= flen;
					if (dlen == 0 && reqLen) {
						dptr = reqFragList->fragAddress;
						dlen = reqFragList->fragSize;
						reqFragList++;
					}
				} else if (dlen == reqLen) {
					rq[1].fragAddr.ro = dptr;
					rq[1].fragSize = dlen;
					reqLen = 0;
				} else {
					u_int8_t* pptr;

					rq[1].fragAddress = pptr = rqbuf;
					rq[1].fragSize = 0;
					while (flen && reqLen) {
						if (dlen > flen) {
							memcpy(pptr, dptr, flen);
							dptr += flen;
							dlen -= flen;
							rq[1].fragSize += flen;
							reqLen -= flen;
							break;
						} else {
							memcpy(pptr, dptr, dlen);
							pptr += dlen;
							flen -= dlen;
							rq[1].fragSize += dlen;
							reqLen -= dlen;
							dptr = reqFragList->fragAddress;
							dlen = reqFragList->fragSize;
							reqFragList++;
						}
					}
				}
			}
			WSET_LH(rqhdr, 10, rq[1].fragSize);

			rp[0].fragAddress = rphdr;
			rp[0].fragSize = 10;
			rp[1].fragAddress = rpbuf;
			rp[1].fragSize = sizeof(rpbuf);

			err = NWRequest(conn, 37, 2, rq, 3, rp);
			if (err) {
				return err;
			}
			DSET_LH(rqhdr, 6, DVAL_LH(rphdr, 0));
			rq[0].fragSize = 12;
			flen = 519;
		} while (reqLen);

		{
			size_t tlen = DVAL_LH(rphdr, 6);
			if (rpLen > tlen) {
				rpLen = tlen;
			}
		}
		
		NWPushFragStart(&fragger, replyFragCount, replyFragList);
		while (rpLen) {
			size_t fragLen;

			fragLen = WVAL_LH(rphdr, 4);
			
			if (fragLen <= rpLen) {
				rpLen -= fragLen;
			} else {
				fragLen = rpLen;
				rpLen = 0;
			}
			err = NWPushFragPush(&fragger, rpbuf, fragLen);
			if (err) {
				return err;
			}
			if (rpLen) {
				NW_FRAGMENT rp[2];

				rp[0].fragAddress = rphdr;
				rp[0].fragSize = 6;
				rp[1].fragAddress = rpbuf;
				rp[1].fragSize = sizeof(rpbuf);

				err = NWRequest(conn, 37, 2, rq, 3, rp);
				if (err) {
					return err;
				}
				DSET_LH(rqhdr, 6, DVAL_LH(rphdr, 0));
			}
		}
		NWPushFragFinish(&fragger);
	}
	return 0; 
}

NWCCODE
NWNCPExtensionRequest(
		NWCONN_HANDLE	conn,
		nuint32		NCPExtensionID,
		const void*	requestData,
		size_t		requestDataLen,
		void*		replyData,
		size_t*		replyDataLen)
{
	NW_FRAGMENT rq;
	NW_FRAGMENT rp;
	NWCCODE err;

	rq.fragAddr.ro = requestData;
	rq.fragSize = requestDataLen;
	rp.fragAddr.rw = replyData;
	rp.fragSize = *replyDataLen;

	err = NWFragNCPExtensionRequest(conn, NCPExtensionID, 1, &rq, 1, &rp);
	if (!err) {
		*replyDataLen = rp.fragSize;
	}
	return err;
}
