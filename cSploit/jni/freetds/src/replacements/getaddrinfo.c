/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2013 Peter Deacon
 * Copyright (C) 2013 Ziglio Frediano
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

#if !defined(HAVE_GETADDRINFO)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <freetds/tds.h>
#include <freetds/sysdep_private.h>
#include "replacements.h"

/* Incomplete implementation, single ipv4 addr, service does not work, hints do not work */
int
tds_getaddrinfo(const char *node, const char *service, const struct tds_addrinfo *hints, struct tds_addrinfo **res)
{
	struct tds_addrinfo *addr;
	struct sockaddr_in *sin = NULL;
	struct hostent *host;
	in_addr_t ipaddr;
	char buffer[4096];
	struct hostent result;
	int h_errnop, port = 0;

	assert(node != NULL);

	if ((addr = (tds_addrinfo *) calloc(1, sizeof(struct tds_addrinfo))) == NULL)
		goto Cleanup;

	if ((sin = (struct sockaddr_in *) calloc(1, sizeof(struct sockaddr_in))) == NULL)
		goto Cleanup;

	addr->ai_addr = (struct sockaddr *) sin;
	addr->ai_addrlen = sizeof(struct sockaddr_in);
	addr->ai_family = AF_INET;

	if ((ipaddr = inet_addr(node)) == INADDR_NONE) {
		if ((host = tds_gethostbyname_r(node, &result, buffer, sizeof(buffer), &h_errnop)) == NULL)
			goto Cleanup;
		if (host->h_name)
			addr->ai_canonname = strdup(host->h_name);
		ipaddr = *(in_addr_t *) host->h_addr;
	}

	if (service) {
		port = atoi(service);
		if (!port)
			port = tds_getservice(service);
	}

	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ipaddr;
	sin->sin_port = htons(port);

	*res = addr;
	return 0;

Cleanup:
	if (addr != NULL)
		tds_freeaddrinfo(addr);

	return -1;
}

/* Incomplete implementation, ipv4 only, port does not work, flags do not work */
int
tds_getnameinfo(const struct sockaddr *sa, size_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags)
{
	struct sockaddr_in *sin = (struct sockaddr_in *) sa;

	if (sa->sa_family != AF_INET)
		return EAI_FAMILY;

	if (host == NULL || hostlen < 16)
		return EAI_OVERFLOW;

#if defined(AF_INET) && HAVE_INET_NTOP
	inet_ntop(AF_INET, &sin->sin_addr, host, hostlen);
#elif HAVE_INET_NTOA_R
	inet_ntoa_r(sin->sin_addr, host, hostlen);
#else
	tds_strlcpy(hostip, inet_ntoa(sin->sin_addr), hostlen);
#endif

	return 0;
}

void
tds_freeaddrinfo(struct tds_addrinfo *addr)
{
	assert(addr != NULL);
	free(addr->ai_canonname);
	free(addr->ai_addr);
	free(addr);
}

#endif
