/*
 *	PROGRAM:	External Data Representation
 *	MODULE:		xdr.cpp
 *	DESCRIPTION:	GDS version of Sun's register XDR Package.
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <string.h>
#include "../remote/remote.h"
#include "../remote/xdr.h"
#include "../jrd/common.h"
#include "../remote/proto_proto.h"
#include "../remote/xdr_proto.h"
#include "../jrd/gds_proto.h"

#ifdef BURP
#include "../burp/misc_proto.h"
inline UCHAR* XDR_ALLOC(ULONG size)
{
	return MISC_alloc_burp(size);
}
inline void XDR_FREEA(void* block)
{
	MISC_free_burp(block);
}
#else // BURP
inline UCHAR* XDR_ALLOC(ULONG size)
{
	return (UCHAR*) gds__alloc((SLONG) size);
}
inline void XDR_FREEA(void* block)
{
	gds__free(block);
}
#endif // BURP

#ifdef DEBUG_XDR_MEMORY
inline void DEBUG_XDR_ALLOC(XDR* xdrs, const void* xdrvar, const void* addr, ULONG len)
{
	xdr_debug_memory(xdrs, XDR_DECODE, xdrvar, addr, len)
}
inline void DEBUG_XDR_FREE(XDR* xdrs, const void* xdrvar, const void* addr, ULONG len)
{
	xdr_debug_memory (xdrs, XDR_FREE, xdrvar, addr, (ULONG) len);
}
#else
inline void DEBUG_XDR_ALLOC(XDR*, const void*, const void*, ULONG)
{
}
inline void DEBUG_XDR_FREE(XDR*, const void*, const void*, ULONG)
{
}
#endif // DEBUG_XDR_MEMORY

// Sun's XDR documentation says this should be "MAXUNSIGNED", but
// for Firebird purposes, limiting strings to 65K is more than
// sufficient.
// This setting may be related to our max DSQL statement size.

const u_int MAXSTRING_FOR_WRAPSTRING	= 65535;


static XDR_INT mem_destroy(XDR*);
static bool_t mem_getbytes(XDR*, SCHAR*, u_int);
static bool_t mem_getlong(XDR*, SLONG*);
static u_int mem_getpostn(XDR*);
static caddr_t mem_inline(XDR*, u_int);
static bool_t mem_putbytes(XDR*, const SCHAR*, u_int);
static bool_t mem_putlong(XDR*, const SLONG*);
static bool_t mem_setpostn(XDR*, u_int);


static const XDR::xdr_ops mem_ops =
{
	mem_getlong,
	mem_putlong,
	mem_getbytes,
	mem_putbytes,
	mem_getpostn,
	mem_setpostn,
	mem_inline,
	mem_destroy
};

#define GETBYTES	 (*xdrs->x_ops->x_getbytes)
#define GETLONG		 (*xdrs->x_ops->x_getlong)
#define PUTBYTES	 (*xdrs->x_ops->x_putbytes)
#define PUTLONG		 (*xdrs->x_ops->x_putlong)

static SCHAR zeros[4] = { 0, 0, 0, 0 };


bool_t xdr_hyper( XDR* xdrs, void* pi64)
{
/**************************************
 *
 *	x d r _ h y p e r       ( n o n - S O L A R I S )
 *
 **************************************
 *
 * Functional description
 *	Map a 64-bit Integer from external to internal representation
 *      (or vice versa).
 *
 *      Enable this for all platforms except Solaris (since it is
 *      available in the XDR library on Solaris). This function (normally)
 *      would have been implemented in REMOTE/xdr.c. Since some system
 *      XDR libraries (HP-UX) do not implement this function, we have it
 *      in this module. At a later date, when the function is available
 *      on all platforms, we can start using the system-provided version.
 *
 *      Handles "swapping" of the 2 long's to be "Endian" sensitive.
 *
 **************************************/
	SLONG temp_long[2];

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		memcpy(temp_long, pi64, sizeof temp_long);
#ifndef WORDS_BIGENDIAN
		if ((*xdrs->x_ops->x_putlong) (xdrs, &temp_long[1]) &&
			(*xdrs->x_ops->x_putlong) (xdrs, &temp_long[0]))
		{
			return TRUE;
		}
#else
		if ((*xdrs->x_ops->x_putlong) (xdrs, &temp_long[0]) &&
			(*xdrs->x_ops->x_putlong) (xdrs, &temp_long[1]))
		{
			return TRUE;
		}
#endif
		return FALSE;

	case XDR_DECODE:
#ifndef WORDS_BIGENDIAN
		if (!(*xdrs->x_ops->x_getlong) (xdrs, &temp_long[1]) ||
			!(*xdrs->x_ops->x_getlong) (xdrs, &temp_long[0]))
		{
			return FALSE;
		}
#else
		if (!(*xdrs->x_ops->x_getlong) (xdrs, &temp_long[0]) ||
			!(*xdrs->x_ops->x_getlong) (xdrs, &temp_long[1]))
		{
			return FALSE;
		}
#endif
		memcpy(pi64, temp_long, sizeof temp_long);
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}
	// TMN: added compiler silencier return FALSE.
	return FALSE;
}

bool_t xdr_bool( XDR* xdrs, bool_t* bp)
{
/**************************************
 *
 *	x d r _ b o o l
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = *bp;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*bp = (bool_t) temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_bytes(XDR* xdrs, SCHAR** bpp, u_int* lp, u_int maxlength)
{
/**************************************
 *
 *	x d r _ b y t e s
 *
 **************************************
 *
 * Functional description
 *	Encode, decode, or free a string.
 *
 **************************************/
	SLONG length;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		length = *lp;
		if (length > (SLONG) maxlength || !PUTLONG(xdrs, &length) || !PUTBYTES(xdrs, *bpp, length))
		{
			return FALSE;
		}
		if ((length = (4 - length) & 3) != 0)
			return PUTBYTES(xdrs, zeros, length);
		return TRUE;

	case XDR_DECODE:
		if (!*bpp)
		{
			*bpp = (SCHAR*) XDR_ALLOC((ULONG) (maxlength + 1));
			// FREE: via XDR_FREE call to this procedure
			if (!*bpp)			// NOMEM:
				return FALSE;
			DEBUG_XDR_ALLOC(xdrs, bpp, *bpp, (maxlength + 1));
		}
		if (!GETLONG(xdrs, &length) || length > (SLONG) maxlength || !GETBYTES(xdrs, *bpp, length))
		{
			return FALSE;
		}
		if ((length = (4 - length) & 3) != 0)
			return GETBYTES(xdrs, zeros, length);
		*lp = (u_int) length;
		return TRUE;

	case XDR_FREE:
		if (*bpp)
		{
			XDR_FREEA(*bpp);
			DEBUG_XDR_FREE(xdrs, bpp, *bpp, (maxlength + 1));
			*bpp = NULL;
		}
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_double(XDR* xdrs, double* ip)
{
/**************************************
 *
 *	x d r _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	union {
		double temp_double;
		SLONG temp_long[2];
	} temp;

	fb_assert(sizeof(double) == sizeof(temp));

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp.temp_double = *ip;
		if (PUTLONG(xdrs, &temp.temp_long[FB_LONG_DOUBLE_FIRST]) &&
			PUTLONG(xdrs, &temp.temp_long[FB_LONG_DOUBLE_SECOND]))
		{
			return TRUE;
		}
		return FALSE;

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp.temp_long[FB_LONG_DOUBLE_FIRST]) ||
			!GETLONG(xdrs, &temp.temp_long[FB_LONG_DOUBLE_SECOND]))
		{
			return FALSE;
		}
		*ip = temp.temp_double;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_enum(XDR* xdrs, xdr_op* ip)
{
/**************************************
 *
 *	x d r _ e n u m
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = (SLONG) *ip;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*ip = (xdr_op) temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_float(XDR* xdrs, float* ip)
{
/**************************************
 *
 *	x d r _ f l o a t
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		return PUTLONG(xdrs, reinterpret_cast<SLONG*>(ip));

	case XDR_DECODE:
#pragma FB_COMPILER_MESSAGE("BUGBUG! No way float* and SLONG* are compatible!")
		return GETLONG(xdrs, reinterpret_cast<SLONG*>(ip));

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_free(xdrproc_t proc, SCHAR* objp)
{
/**************************************
 *
 *	x d r _ f r e e
 *
 **************************************
 *
 * Functional description
 *	Perform XDR_FREE operation on an XDR structure
 *
 **************************************/
	XDR xdrs;

	xdrs.x_op = XDR_FREE;

	return (*proc)(&xdrs, objp);
}


bool_t xdr_int(XDR* xdrs, int* ip)
{
/**************************************
 *
 *	x d r _ i n t
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = *ip;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*ip = (int) temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_long(XDR* xdrs, SLONG* ip)
{
/**************************************
 *
 *	x d r _ l o n g
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		return PUTLONG(xdrs, ip);

	case XDR_DECODE:
		return GETLONG(xdrs, ip);

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_opaque(XDR* xdrs, SCHAR* p, u_int len)
{
/**************************************
 *
 *	x d r _ o p a q u e
 *
 **************************************
 *
 * Functional description
 *	Encode, decode, or free an opaque object.
 *
 **************************************/
	SCHAR trash[4];
	static const SCHAR filler[4] = { 0, 0, 0, 0 };

	const SSHORT l = (4 - len) & 3;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		if (!PUTBYTES(xdrs, p, len))
			return FALSE;
		if (l)
			return PUTBYTES(xdrs, filler, l);
		return TRUE;

	case XDR_DECODE:
		if (!GETBYTES(xdrs, p, len))
			return FALSE;
		if (l)
			return GETBYTES(xdrs, trash, l);
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_short(XDR* xdrs, SSHORT* ip)
{
/**************************************
 *
 *	x d r _ s h o r t
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = *ip;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*ip = (SSHORT) temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_string(XDR* xdrs, SCHAR** sp, u_int maxlength)
{
/**************************************
 *
 *	x d r _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Encode, decode, or free a string.
 *
 **************************************/
	SCHAR trash[4];
	static const SCHAR filler[4] = { 0, 0, 0, 0 };
	ULONG length;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		length = strlen(*sp);
		if (length > maxlength ||
			!PUTLONG(xdrs, reinterpret_cast<SLONG*>(&length)) ||
			!PUTBYTES(xdrs, *sp, length))
		{
			return FALSE;
		}
		if ((length = (4 - length) & 3) != 0)
			return PUTBYTES(xdrs, filler, length);
		return TRUE;

	case XDR_DECODE:
		if (!*sp)
		{
			*sp = (SCHAR*) XDR_ALLOC((ULONG) (maxlength + 1));
			// FREE: via XDR_FREE call to this procedure
			if (!*sp)			// NOMEM: return error
				return FALSE;
			DEBUG_XDR_ALLOC(xdrs, sp, *sp, (maxlength + 1));
		}
		if (!GETLONG(xdrs, reinterpret_cast<SLONG*>(&length)) ||
			length > maxlength || !GETBYTES(xdrs, *sp, length))
		{
			return FALSE;
		}
		(*sp)[length] = 0;
		if ((length = (4 - length) & 3) != 0)
			return GETBYTES(xdrs, trash, length);
		return TRUE;

	case XDR_FREE:
		if (*sp)
		{
			XDR_FREEA(*sp);
			DEBUG_XDR_FREE(xdrs, sp, *sp, (maxlength + 1));
			*sp = NULL;
		}
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_u_int(XDR* xdrs, u_int* ip)
{
/**************************************
 *
 *	x d r _ u _ i n t
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = *ip;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*ip = temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;

	default:
		return FALSE;
	}
}


bool_t xdr_u_long(XDR* xdrs, ULONG* ip)
{
/**************************************
 *
 *	x d r _ u _ l o n g
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		return PUTLONG(xdrs, reinterpret_cast<SLONG*>(ip));

	case XDR_DECODE:
		if (!GETLONG(xdrs, reinterpret_cast<SLONG*>(ip)))
			  return FALSE;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


bool_t xdr_u_short(XDR* xdrs, u_short* ip)
{
/**************************************
 *
 *	x d r _ u _ s h o r t
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/
	SLONG temp;

	switch (xdrs->x_op)
	{
	case XDR_ENCODE:
		temp = *ip;
		return PUTLONG(xdrs, &temp);

	case XDR_DECODE:
		if (!GETLONG(xdrs, &temp))
			return FALSE;
		*ip = (u_int) temp;
		return TRUE;

	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}


int xdr_union(	XDR*			xdrs,
				xdr_op*			dscmp,
				SCHAR*			unp,
				xdr_discrim*	choices,
				xdrproc_t		dfault)
{
/**************************************
 *
 *	x d r _ u n i o n
 *
 **************************************
 *
 * Functional description
 *	Descriminated union.  Yuckola.
 *
 **************************************/

	// TMN: Added temporary int to hold the enum_t, since an enum
	// can have any size.
	int enum_value = *dscmp;
	const bool_t bOK = xdr_int(xdrs, &enum_value);
	*dscmp = static_cast<xdr_op>(enum_value);

	if (!bOK)
	{
		return FALSE;
	}

	for (; choices->proc; ++choices)
	{
		if (*dscmp == choices->value)
		{
			return (*choices->proc)(xdrs, unp);
		}
	}

	if (dfault)
	{
		return (*dfault)(xdrs, unp);
	}

	return FALSE;
}


bool_t xdr_wrapstring(XDR* xdrs, SCHAR** strp)
{
/**************************************
 *
 *	x d r _ w r a p s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Map from external to internal representation (or vice versa).
 *
 **************************************/

	return xdr_string(xdrs, strp, MAXSTRING_FOR_WRAPSTRING);
}


int xdrmem_create(	XDR* xdrs, SCHAR* addr, u_int len, xdr_op x_op)
{
/**************************************
 *
 *	x d r m e m _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Initialize an "in memory" register XDR stream.
 *
 **************************************/

	xdrs->x_base = xdrs->x_private = addr;
	xdrs->x_handy = len;
	xdrs->x_ops = &mem_ops;
	xdrs->x_op = x_op;

	return TRUE;
}


static XDR_INT mem_destroy(XDR*)
{
/**************************************
 *
 *	m e m _ d e s t r o y
 *
 **************************************
 *
 * Functional description
 *	Destroy a stream.  A no-op.
 *
 **************************************/
	return TRUE;
}


static bool_t mem_getbytes(	XDR* xdrs, SCHAR* buff, u_int count)
{
/**************************************
 *
 *	m e m _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Get a bunch of bytes from a memory stream if it fits.
 *
 **************************************/
	const SLONG bytecount = count;

	if ((xdrs->x_handy -= bytecount) < 0)
	{
		xdrs->x_handy += bytecount;
		return FALSE;
	}

	if (bytecount)
	{
		memcpy(buff, xdrs->x_private, bytecount);
		xdrs->x_private += bytecount;
	}

	return TRUE;
}


static bool_t mem_getlong( XDR* xdrs, SLONG* lp)
{
/**************************************
 *
 *	m e m _ g e t l o n g
 *
 **************************************
 *
 * Functional description
 *	Fetch a longword into a memory stream if it fits.
 *
 **************************************/
	if ((xdrs->x_handy -= sizeof(SLONG)) < 0)
	{
		xdrs->x_handy += sizeof(SLONG);
		return FALSE;
	}

	SLONG* p = (SLONG*) xdrs->x_private;
	*lp = ntohl(*p++);
	xdrs->x_private = (SCHAR*) p;

	return TRUE;
}


SLONG getOperation(const void* data, size_t size)
{
/**************************************
 *
 *	g e t O p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Fetch an operation from buffer in network format
 *
 **************************************/
	if (size < sizeof(SLONG))
	{
		return op_void;
	}

	const SLONG* p = (SLONG*) data;
	return ntohl(*p);
}


static u_int mem_getpostn( XDR* xdrs)
{
/**************************************
 *
 *	m e m _ g e t p o s t n
 *
 **************************************
 *
 * Functional description
 *	Get the current position (which is also current length) from stream.
 *
 **************************************/

	return (u_int) (xdrs->x_private - xdrs->x_base);
}


static caddr_t mem_inline( XDR* xdrs, u_int bytecount)
{
/**************************************
 *
 *	m e m _  i n l i n e
 *
 **************************************
 *
 * Functional description
 *	Return a pointer to somewhere in the buffer.
 *
 **************************************/

	if (bytecount > (u_int) ((xdrs->x_private + xdrs->x_handy) - xdrs->x_base))
		return FALSE;

	return xdrs->x_base + bytecount;
}


static bool_t mem_putbytes(XDR* xdrs, const SCHAR* buff, u_int count)
{
/**************************************
 *
 *	m e m _ p u t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Put a bunch of bytes to a memory stream if it fits.
 *
 **************************************/
	const SLONG bytecount = count;

	if ((xdrs->x_handy -= bytecount) < 0)
	{
		xdrs->x_handy += bytecount;
		return FALSE;
	}

	if (bytecount)
	{
		memcpy(xdrs->x_private, buff, bytecount);
		xdrs->x_private += bytecount;
	}

	return TRUE;
}


static bool_t mem_putlong( XDR* xdrs, const SLONG* lp)
{
/**************************************
 *
 *	m e m _ p u t l o n g
 *
 **************************************
 *
 * Functional description
 *	Fetch a longword into a memory stream if it fits.
 *
 **************************************/
	if ((xdrs->x_handy -= sizeof(SLONG)) < 0)
	{
		xdrs->x_handy += sizeof(SLONG);
		return FALSE;
	}

	SLONG* p = (SLONG*) xdrs->x_private;
	*p++ = htonl(*lp);
	xdrs->x_private = (SCHAR*) p;

	return TRUE;
}


static bool_t mem_setpostn( XDR* xdrs, u_int bytecount)
{
/**************************************
 *
 *	m e m _ s e t p o s t n
 *
 **************************************
 *
 * Functional description
 *	Set the current position (which is also current length) from stream.
 *
 **************************************/
	const u_int length = (u_int) ((xdrs->x_private - xdrs->x_base) + xdrs->x_handy);

	if (bytecount > length)
		return FALSE;

	xdrs->x_handy = length - bytecount;
	xdrs->x_private = xdrs->x_base + bytecount;

	return TRUE;
}

