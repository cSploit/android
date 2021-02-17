/*
 *  Copyright (c) 1999 Petr Vandrovec <vandrove@vc.cvut.cz>
 *            All Rights Reserved.
 *
 *  See file COPYING for details.
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ncp/kernel/ipx.h>
#include <sys/types.h>
#include "ipxutil.h"

/* caller must fill sockaddr_ipx with default values... */
/* it parses string in form NETWORK:NODE/PORT into sockaddr_ipx structure */
/* you can modify parser by flags: SSIPX_NETWORK = network present
                                   SSIPX_NODE    = node present
				   SSIPX_PORT    = port present
   separators (:, /) are optional if preceeding parts are not expected */
/* return value: SSIPX_NONE if nothing usable was found, otherwise
   combination of SSIPX_* reports, what was found and successfully parsed */
int
sscanf_ipx_addr(struct sockaddr_ipx *sipx, const char* addr, int flags) {
	u_int32_t netnum;
	char* ptr;
	int retval = SSIPX_NONE;
	
	/* skip leading spaces... should we? */
	while (*addr && isspace(*addr))
		addr++;
	if (!*addr)
		return retval;	/* nothing found */
	if (flags & SSIPX_NETWORK) {
		netnum = strtoul(addr, &ptr, 16);
		if (ptr != addr) {
			sipx->sipx_network = htonl(netnum);
			retval |= SSIPX_NETWORK;	/* network was specified */
			addr = ptr;
		}
	}
	if (flags & SSIPX_NODE) {
		if ((*addr == ':') || !(flags & SSIPX_NETWORK)) {
			u_int16_t hi;
			u_int32_t lo;
			unsigned int c;
			const char* scn;
			/* node... */
			hi = 0;
			lo = 0;
			if (*addr == ':')
				addr++;
			scn = addr;
			while (isxdigit(c=*scn)) {
				scn++;
				hi = (hi << 4) | ((lo >> 28) & 0x0F);
				c -= '0';
				if (c > 9)
					c = (c + '0' - 'A' + 10) & 0x0F;
				lo = (lo << 4) | c;
			}
			if (addr != scn) {
				sipx->sipx_node[0] = hi >> 8;
				sipx->sipx_node[1] = hi;
				sipx->sipx_node[2] = lo >> 24;
				sipx->sipx_node[3] = lo >> 16;
				sipx->sipx_node[4] = lo >> 8;
				sipx->sipx_node[5] = lo;
				retval |= SSIPX_NODE;
				addr = scn;
			} /* else syntax error(node empty) */
		}
	}
	if (flags & SSIPX_PORT) {
		if ((*addr == '/') || (*addr == ':') || 
		    !(flags & (SSIPX_NETWORK|SSIPX_NODE))) {
			u_int16_t sock;
		
			/* socket... */
			if ((*addr == '/') || (*addr == ':'))
				addr++;
			sock = strtoul(addr, &ptr, 16);
			if (ptr != addr) {
				sipx->sipx_port = sock;
				retval |= SSIPX_PORT;
				addr = ptr;
			} /* else syntax error(socket empty) */
		}
	}
	/* if (*addr) syntax error(data after end of address) */
	return retval;
}
