/*
    iterhandle.c - wrappers around server's iteration handle
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

#include "iterhandle.h"

#include <stdlib.h>

#define BADSIGN	0x0BADDE5C
#define OKSIGN	0x600DDE5C

#define WRAPIH_LOCKED	0x00000001

static ncpt_mutex_t ihlock = NCPT_MUTEX_INITIALIZER;

static struct wrappedIterationHandle* ihfirst = NULL;
static struct wrappedIterationHandle* ihlast = NULL;

void* __NWDSIHInit(NWCONN_HANDLE conn,
		nuint32 iterHandle,
		nuint32 verb
) {
	struct wrappedIterationHandle* p;
	static nuint32 proposedID = 1;
	struct wrappedIterationHandle* ih;
	
	ih = (struct wrappedIterationHandle*)malloc(sizeof(*ih));
	if (!ih) {
		if (conn && iterHandle != NO_MORE_ITERATIONS)
			__NWDSCloseIterationV0(conn, iterHandle, verb);
		return NULL;
	}
	memset(ih, 0, sizeof(*ih));
	ih->conn = conn;
	if (conn)
		ncp_conn_use(conn);
	ih->iterHandle = iterHandle;
	ih->verb = verb;
/*	ih->next = NULL; */
/*	ih->prev = NULL; */
/*	ih->abort = NULL; */
	ih->sign = OKSIGN;
	ih->flags = WRAPIH_LOCKED;
/*	ih->data = NULL; */

	ncpt_mutex_lock(&ihlock);
	if (!ihlast) {
		ihlast = ihfirst = ih;
	} else {
		p = NULL;
		
		if (ihlast->id >= proposedID) {
			p = ihfirst;
l1:;
			for (; p && p->id < proposedID; p = p->next);
			if (p) {
				if (p->id == proposedID) {
					proposedID++;
					goto l1;
				}
			}
		}
		if (p) {
			ih->next = p->next;
			ih->prev = p;
			if (ih->next)
				ih->next->prev = p;
		} else {
			ih->prev = ihlast;
			ihlast->next = ih;
			ihlast = ih;
		}
	}
	ih->id = proposedID;
	proposedID++;
	if (proposedID >= 0xFFFF0000)
		proposedID = 1;
	ncpt_mutex_unlock(&ihlock);
	return ih;
}		

NWDSCCODE __NWDSIHDelete(struct wrappedIterationHandle* ih) {
	NWDSCCODE err;

	if (!(ih->flags & WRAPIH_LOCKED)) {
		fprintf(stderr, "libncp internal bug: wrapped handle unlocked in NWDSIHDelete\n");
		/* libncp BUG */
		return ERR_SYSTEM_ERROR;
	}
	if (ih->sign != OKSIGN) {
		fprintf(stderr, "libncp internal bug: invalid wrapped handle in NWDSIHDelete\n");
		/* libncp BUG */
		return ERR_SYSTEM_ERROR;
	}
	err = 0;
	if (ih->abort)
		ih->abort(ih);
	if (ih->conn) {
		if (ih->iterHandle != NO_MORE_ITERATIONS) {
			err = __NWDSCloseIterationV0(ih->conn, ih->iterHandle, ih->verb);
		}
		NWCCCloseConn(ih->conn);
		ih->conn = NULL;
	}
	ih->sign = BADSIGN;
	ncpt_mutex_lock(&ihlock);
	if (ih->prev)
		ih->prev->next = ih->next;
	if (ih->next)
		ih->next->prev = ih->prev;
	if (ihfirst == ih)
		ihfirst = ih->next;
	if (ihlast == ih)
		ihlast = ih->prev;
	ih->next = ih->prev = NULL;
	ncpt_mutex_unlock(&ihlock);
	free(ih);
	return err;
}

struct wrappedIterationHandle* __NWDSIHLookup(nuint32 id, nuint32 verb) {
	struct wrappedIterationHandle* p;

	ncpt_mutex_lock(&ihlock);
	for (p = ihfirst; p && p->id < id; p = p->next);
	if (p && p->sign == OKSIGN && p->id == id && p->verb == verb) {
		if (p->flags & WRAPIH_LOCKED) {
			/* threaded app BUG or libncp BUG */
			p = NULL;
		} else
			p->flags |= WRAPIH_LOCKED;
	} else {
		p = NULL;
	}
	ncpt_mutex_unlock(&ihlock);
	return p;
}

void __NWDSIHPut(struct wrappedIterationHandle* ih, nuint32* ihp) {
	if (!(ih->flags & WRAPIH_LOCKED)) {
		fprintf(stderr, "libncp internal bug: wrapped handle unlocked in NWDSIHPut\n");
		/* libncp BUG */
		return;
	}
	if (ihp)
		*ihp = ih->id;
	ncpt_mutex_lock(&ihlock);
	ih->flags &= ~WRAPIH_LOCKED;
	ncpt_mutex_unlock(&ihlock);
}

NWDSCCODE __NWDSIHUpdate(struct wrappedIterationHandle* ih, NWDSCCODE err,
		nuint32 lh, nuint32* iterHandle) {
	if (err || lh == NO_MORE_ITERATIONS) {
		__NWDSIHAbort(ih);
		if (iterHandle)
			*iterHandle = NO_MORE_ITERATIONS;
		return err;
	}
	ih->iterHandle = lh;
	__NWDSIHPut(ih, iterHandle);
	return 0;
}

NWDSCCODE __NWDSIHCreate(NWDSCCODE err, NWCONN_HANDLE conn, NWObjectID objID,
		nuint32 lh, nuint32 verb, nuint32* iterHandle) {
	struct wrappedIterationHandle* ih;
	
	if (err || lh == NO_MORE_ITERATIONS) {
		NWCCCloseConn(conn);
		if (iterHandle)
			*iterHandle = NO_MORE_ITERATIONS;
		return err;
	}
	if (!iterHandle) {
		__NWDSCloseIterationV0(conn, lh, verb);
		err = ERR_NULL_POINTER;
	} else {
		ih = __NWDSIHInit(conn, lh, verb);
		if (ih) {
			ih->objectID = objID;
			__NWDSIHPut(ih, iterHandle);
		} else {
			err = ERR_NOT_ENOUGH_MEMORY;
		}
	}
	NWCCCloseConn(conn);
	return err;
}
