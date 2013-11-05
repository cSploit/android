/*
    request.c - NWCFragmentRequest implementation
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
		Initial release

 */

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ncplib_i.h"
#include "nwnet_i.h"

NWDSCCODE NWCFragmentRequest(
		NWCONN_HANDLE conn,
		nuint32 verb,
		nuint numRq, const NW_FRAGMENT* rq, 
		nuint numRp, NW_FRAGMENT* rp,
		size_t *replyLen) {
	nuint i;
	size_t rqlen;
	size_t buflen;
	size_t rplen;
	size_t rpbuflen;
	
	rqlen = 0;
	for (i=numRq; i--; ) {
		rqlen += (rq+i)->fragSize;
	}
	buflen = ROUNDPKT(rqlen);
	rplen = 0;
	for (i = numRp; i--; ) {
		rplen += (rp+i)->fragSize;
	}
	rpbuflen = ROUNDPKT(rplen);
	{
		char buff[buflen + rpbuflen];
		char* ptr = buff;
		size_t rest;
		NWDSCCODE result;
		
		for (i = numRq; i--; rq++) {
			memcpy(ptr, rq->fragAddr.ro, rq->fragSize);
			ptr += rq->fragSize;
		}
		ptr = buff + buflen;
		result = ncp_send_nds_frag(conn, verb, buff, rqlen, ptr, rpbuflen, &rest);
		if (result)
			return result;
		if (replyLen)
			*replyLen = rest;
		for (i=numRp; i--; rp++) {
			size_t spc = rp->fragSize;
			if (spc > rest) {
				memcpy(rp->fragAddr.rw, ptr, rest);
				/* Do we want to modify fragList ? */
				/* I think so because of we want to propagate */
				/* reply length back to client */
				/* maybe we could do "return conn->ncp_reply_size" */
				/* instead of "return 0". */
				rp->fragSize = rest;
				rest = 0;
			} else {
				memcpy(rp->fragAddr.rw, ptr, spc);
				rest -= spc;
				ptr += spc;
			}
		}
	}
	return 0;
}
