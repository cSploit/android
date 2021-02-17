/*
 *  Copyright (c) 1999 Petr Vandrovec <vandrove@vc.cvut.cz>
 *            All Rights Reserved.
 *
 *  See file COPYING for details.
 */

#ifndef __IPXUTIL_H__
#define __IPXUTIL_H__

#define SSIPX_NONE	0
#define SSIPX_NETWORK	1
#define SSIPX_NODE	2
#define SSIPX_PORT	4

int
sscanf_ipx_addr(struct sockaddr_ipx *sipx, const char* addr, int flags);

#endif	/* __IPXUTIL_H__ */
