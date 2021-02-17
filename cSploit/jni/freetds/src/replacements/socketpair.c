/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2011 Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#if !defined(HAVE_SOCKETPAIR)

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include <freetds/tds.h>
#include "replacements.h"

TDS_RCSID(var, "$Id: socketpair.c,v 1.2 2012-03-04 11:34:22 freddy77 Exp $");

int
tds_socketpair(int domain, int type, int protocol, int sv[2])
{
	struct sockaddr_in sa, sa2;
	SOCKLEN_T addrlen;
	TDS_SYS_SOCKET s;

	if (!sv)
		return -1;

	/* create a listener */
	s = socket(AF_INET, type, 0);
	if (TDS_IS_SOCKET_INVALID(s))
		return -1;

	sv[1] = INVALID_SOCKET;

	sv[0] = socket(AF_INET, type, 0);
	if (TDS_IS_SOCKET_INVALID(sv[0]))
		goto Cleanup;

	/* bind to a random port */
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_port = 0;
	if (bind(s, (struct sockaddr*) &sa, sizeof(sa)) < 0)
		goto Cleanup;
	if (listen(s, 1) < 0)
		goto Cleanup;

	/* connect to kernel choosen port */
	addrlen = sizeof(sa);
	if (tds_getsockname(s, (struct sockaddr*) &sa, &addrlen) < 0)
		goto Cleanup;
	if (connect(sv[0], (struct sockaddr*) &sa, sizeof(sa)) < 0)
		goto Cleanup;
	addrlen = sizeof(sa2);
	sv[1] = tds_accept(s, (struct sockaddr*) &sa2, &addrlen);
	if (TDS_IS_SOCKET_INVALID(sv[1]))
		goto Cleanup;

	/* check proper connection */
	addrlen = sizeof(sa);
	if (tds_getsockname(sv[0], (struct sockaddr*) &sa, &addrlen) < 0)
		goto Cleanup;
	addrlen = sizeof(sa2);
	if (tds_getpeername(sv[1], (struct sockaddr*) &sa2, &addrlen) < 0)
		goto Cleanup;
	if (sa.sin_family != sa2.sin_family || sa.sin_port != sa2.sin_port
	    || sa.sin_addr.s_addr != sa2.sin_addr.s_addr)
		goto Cleanup;

	CLOSESOCKET(s);
	return 0;

Cleanup:
	CLOSESOCKET(s);
	CLOSESOCKET(sv[0]);
	CLOSESOCKET(sv[1]);
	return -1;
}

#endif
