/* 
    nwtime.c - Timesync calls
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

	1.00  2000, May 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version - NWGetFileServerUTCTime moved from nwcalls.c.

 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncplib_i.h"
#include "ncpcode.h"


NWDSCCODE __NWGetFileServerUTCTime(
			NWCONN_HANDLE	conn,
			nuint32*	wholeSeconds,
			nuint32*	fractionSeconds,
			nuint32*	flags,
			nuint32*	eventOffsetLo,
			nuint32*	eventOffsetHi,
			nuint32*	eventTime,
			nuint32*	d7
) {
	NW_FRAGMENT rp_frag;
	nuint8	rp_b[256];
	NWCCODE err;
	
	rp_frag.fragAddr.rw = rp_b;
	rp_frag.fragSize = sizeof(rp_b);
	err = NWRequestSimple(conn, NCPC_SFN(114,1), NULL, 0, &rp_frag);
	if (err)
		return err;
	if (rp_frag.fragSize < 4 * 7)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (wholeSeconds)
		*wholeSeconds = DVAL_LH(rp_b, 0);
	if (fractionSeconds)
		*fractionSeconds = DVAL_LH(rp_b, 4);
	if (flags)
		*flags = DVAL_LH(rp_b, 8);
	if (eventOffsetLo)
		*eventOffsetLo = DVAL_LH(rp_b, 12);
	if (eventOffsetHi)
		*eventOffsetHi = DVAL_LH(rp_b, 16);
	if (eventTime)
		*eventTime = DVAL_LH(rp_b, 20);
	if (d7)
		*d7 = DVAL_LH(rp_b, 24);
	return 0;
}

NWDSCCODE NWGetFileServerUTCTime(
			NWCONN_HANDLE	conn,
			nuint32*	timev
) {
	nuint32 flags;
	NWDSCCODE err;
	
	err = __NWGetFileServerUTCTime(conn, timev, NULL, &flags, NULL, NULL, NULL, NULL);
	if (err)
		return err;
	if (!(flags & 2))
		return ERR_TIME_NOT_SYNCHRONIZED;
	return 0;
}

NWDSCCODE __NWTimeGetVersion(
			NWCONN_HANDLE	conn,
			int		req,
			void*		buffer,
			size_t*		len,
			size_t		maxlen
) {
	NW_FRAGMENT rp_frag;
	NWCCODE err;
	
	if (!buffer)
		return ERR_NULL_POINTER;
	rp_frag.fragAddr.rw = buffer;
	rp_frag.fragSize = maxlen;
	err = NWRequestSimple(conn, NCPC_SFN(114,req), NULL, 0, &rp_frag);
	if (err)
		return err;
	*len = rp_frag.fragSize;
	return 0;
}
