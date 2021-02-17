/*
    rsynt.c - Read SyntaxID of specified attribute
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

	1.00  2000, July 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision, cloned from readadef.c.

*/

#include "rsynt.h"

NWDSCCODE MyNWDSReadSyntaxID(NWDSContextHandle ctx, NWDSChar* attr, enum SYNTAX* synt) {
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
	dserr = NWDSPutClassName(ctx, ibuf, attr);
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
	if (synt)
		*synt = ainfo.attrSyntaxID;
quitbufibuf:;
	NWDSFreeBuf(buf);
quitibuf:;
	NWDSFreeBuf(ibuf);
quit:;
	return dserr;
}
	
