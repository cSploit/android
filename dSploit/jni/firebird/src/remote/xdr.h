/*
 *	PROGRAM:	External Data Representation
 *	MODULE:		xdr.h
 *	DESCRIPTION:	GDS version of Sun's XDR Package.
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef REMOTE_XDR_H
#define REMOTE_XDR_H

#include "../jrd/common.h"

#include <sys/types.h>
#ifdef WIN_NT
#include <winsock2.h>
typedef	char* caddr_t;
#else // WIN_NT
#include <netinet/in.h>
#endif // WIN_NT

#if defined(__hpux) && !defined(ntohl)
// this include is only for HP 11i v2.
// ntohl is not defined in <netinet/in.h> when _XOPEN_SOURCE_EXTENDED
// is defined, even though ntohl is defined by POSIX
#include <arpa/inet.h>
#endif

typedef int XDR_INT;
typedef int bool_t;
//#ifndef enum_t
//#define enum_t	enum xdr_op
//#endif

#define xdr_getpostn(xdr)	((*(*xdr).x_ops->x_getpostn)(xdr)) // unused?
#define xdr_destroy(xdr)	(*(*xdr).x_ops->x_destroy)() // unused?


enum xdr_op { XDR_ENCODE = 0, XDR_DECODE = 1, XDR_FREE = 2 };

typedef struct xdr_t
{
	xdr_op x_op;			// operation; fast additional param
	struct xdr_ops
	{
		bool_t  (*x_getlong)(struct xdr_t*, SLONG*);			// get a long from underlying stream
		bool_t  (*x_putlong)(struct xdr_t*, const SLONG*);		// put a long to "
		bool_t  (*x_getbytes)(struct xdr_t*, SCHAR *, u_int);	// get some bytes from "
		bool_t  (*x_putbytes)(struct xdr_t*, const SCHAR*, u_int);	// put some bytes to "
		u_int   (*x_getpostn)(struct xdr_t*);			// returns bytes offset from beginning
		bool_t  (*x_setpostn)(struct xdr_t*, u_int);	// repositions position in stream
		caddr_t (*x_inline)(struct xdr_t*, u_int);		// buf quick ptr to buffered data
		XDR_INT (*x_destroy)(struct xdr_t*);			// free privates of this xdr_stream
	} const *x_ops;
	caddr_t	x_public;	// Users' data
	caddr_t	x_private;	// pointer to private data
	caddr_t	x_base;		// private used for position info
	int		x_handy;	// extra private word

public:
	xdr_t() :
		x_op(XDR_ENCODE), x_ops(0), x_public(0), x_private(0), x_base(0), x_handy(0)
	{ }
} XDR;

// Discriminated union crud

// CVC: Restore the old definition if some compilation failure happens.
//typedef bool_t			(*xdrproc_t)();
typedef bool_t          (*xdrproc_t)(xdr_t*, SCHAR*);
//#define NULL_xdrproc_t	((xdrproc_t) 0)

struct xdr_discrim
{
	xdr_op		value;
	xdrproc_t	proc;
};


#endif // REMOTE_XDR_H
