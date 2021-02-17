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

enum xdr_op { XDR_ENCODE = 0, XDR_DECODE = 1, XDR_FREE = 2 };

typedef struct xdr_t
{
	xdr_op x_op;			// operation; fast additional param
	struct xdr_ops
	{
		bool_t  (*x_getbytes)(struct xdr_t*, SCHAR *, u_int);	// get some bytes from "
		bool_t  (*x_putbytes)(struct xdr_t*, const SCHAR*, u_int);	// put some bytes to "
	} const *x_ops;
	caddr_t	x_public;	// Users' data
	caddr_t	x_private;	// pointer to private data
	caddr_t	x_base;		// private used for position info
	int		x_handy;	// extra private word
	bool	x_local;	// transmission is known to be local (bytes are in the host order)
#ifdef DEV_BUILD
	bool	x_client;	// set this flag to true if this is client port
#endif

public:
	xdr_t() :
		x_op(XDR_ENCODE), x_ops(0), x_public(0), x_private(0), x_base(0), x_handy(0),
		x_local(false)
#ifdef DEV_BUILD
		, x_client(false)
#endif
	{ }
} XDR;


#endif // REMOTE_XDR_H
