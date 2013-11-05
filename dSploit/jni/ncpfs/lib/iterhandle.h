#ifndef __NCP_ITERHANDLE_H__
#define __NCP_ITERHANDLE_H__

/*
    iterhandle.h - wrappers around server's iteration handle
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

	1.00  2000, April 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.
	
 */

#include "ncplib_i.h"
#include "nwnet_i.h"

struct wrappedIterationHandle {
	nuint32		sign;
	struct wrappedIterationHandle* next;
	struct wrappedIterationHandle* prev;
	nuint32		id;
	NWCONN_HANDLE	conn;
	nuint32		iterHandle;
	nuint32		verb;
	nuint32		flags;
	NWObjectID	objectID;
	void		(*abort)(struct wrappedIterationHandle*);
	void*		data;
};

void* __NWDSIHInit(NWCONN_HANDLE conn, 
		nuint32 iterHandle, nuint32 verb);
NWDSCCODE __NWDSIHDelete(struct wrappedIterationHandle* ih);
static inline NWDSCCODE __NWDSIHAbort(struct wrappedIterationHandle* ih) {
	ih->iterHandle = NO_MORE_ITERATIONS;
	return __NWDSIHDelete(ih);
};
struct wrappedIterationHandle* __NWDSIHLookup(nuint32 id, nuint32 verb);
void __NWDSIHPut(struct wrappedIterationHandle* ih, 
		nuint32* iterHandle);
static inline void __NWDSIHSetConn(struct wrappedIterationHandle* ih, 
		NWCONN_HANDLE conn) {
	if (ih->conn != conn) {
		if (conn)
			ncp_conn_use(conn);
		if (ih->conn)
			NWCCCloseConn(ih->conn);
		ih->conn = conn;
	}
	ih->iterHandle = NO_MORE_ITERATIONS;
};
NWDSCCODE __NWDSIHUpdate(struct wrappedIterationHandle* ih,
		NWDSCCODE dserr, nuint32 lh, nuint32* iterHandle);
NWDSCCODE __NWDSIHCreate(NWDSCCODE dserr, NWCONN_HANDLE conn, NWObjectID objID,
		nuint32 lh, nuint32 verb, nuint32* iterHandle);

#endif	/* __NCP_ITERHANDLE_H__ */
