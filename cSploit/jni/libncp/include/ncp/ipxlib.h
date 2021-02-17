/*
    ipxlib.h
    Copyright (C) 1995 by Volker Lendecke

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

	0.00  1995			Volker Lendecke
		Initial revision.

	1.00  2001, September 15	Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixes for SWIG processing. Unwind nested structs so names
		are defined here and not by SWIG.

 */

#ifndef _IPXLIB_H
#define _IPXLIB_H

#include <ncp/kernel/types.h>
#include <ncp/ncp.h>
#include <ncp/kernel/ipx.h>

typedef u_int32_t	IPXNet;
typedef u_int16_t	IPXPort;
typedef u_int8_t	IPXNode[IPX_NODE_LEN];
typedef const u_int8_t	CIPXNode[IPX_NODE_LEN];

#define IPX_USER_PTYPE (0x00)
#define IPX_RIP_PTYPE (0x01)
#define IPX_SAP_PTYPE (0x04)
#define IPX_AUTO_PORT (0x0000)
#define IPX_SAP_PORT  (0x0452)
#define IPX_RIP_PORT  (0x0453)

#define IPX_SAP_GENERAL_QUERY		(0x0001)
#define IPX_SAP_GENERAL_RESPONSE	(0x0002)
#define IPX_SAP_NEAREST_QUERY		(0x0003)
#define IPX_SAP_NEAREST_RESPONSE	(0x0004)
#define IPX_SAP_SPECIFIC_QUERY		(0x000E)
#define IPX_SAP_SPECIFIC_RESPONSE	(0x000F)
#define IPX_SAP_FILE_SERVER (0x0004)

struct sap_query
{
	u_int16_t	query_type;	/* net order */
	u_int16_t	server_type;	/* net order */
};

struct sap_server_ident
{
	u_int16_t	server_type __attribute__((packed));
	char		server_name[48];
	IPXNet		server_network __attribute__((packed));
#ifdef SWIG
	u_int8_t	server_node[6] __attribute__((packed));
#else
	IPXNode		server_node;
#endif
	IPXPort		server_port __attribute__((packed));
	u_int16_t	intermediate_network __attribute__((packed));
};

#define IPX_RIP_REQUEST (0x1)
#define IPX_RIP_RESPONSE (0x2)

struct ipx_rt_def {
	u_int32_t network __attribute__((packed));
	u_int16_t hops __attribute__((packed));
	u_int16_t ticks __attribute__((packed));
};

struct ipx_rip_packet
{
	u_int16_t		operation __attribute__((packed));
	struct ipx_rt_def	rt[1];
};

#ifdef SWIG
#define IPX_BROADCAST_NODE "\xff\xff\xff\xff\xff\xff"
#define IPX_THIS_NODE      "\0\0\0\0\0\0"
#else
#define IPX_BROADCAST_NODE ("\xff\xff\xff\xff\xff\xff")
#define IPX_THIS_NODE      ("\0\0\0\0\0\0")
#endif
#define IPX_THIS_NET (0)

#ifndef IPX_NODE_LEN
#define IPX_NODE_LEN (6)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void
 ipx_print_node(IPXNode node);
void
 ipx_print_network(IPXNet net);
void
 ipx_print_port(IPXPort port);
void
 ipx_fprint_node(FILE * file, IPXNode node);
void
 ipx_fprint_network(FILE * file, IPXNet net);
void
 ipx_fprint_port(FILE * file, IPXPort port);
int
 ipx_sscanf_node(char *buf, unsigned char node[IPX_NODE_LEN]);
void
 ipx_assign_node(IPXNode dest, CIPXNode src);
int
 ipx_node_equal(CIPXNode n1, CIPXNode n2);

#ifdef NCP_IPX_SUPPORT
void
ipx_print_saddr(struct sockaddr_ipx *sipx);
void
ipx_fprint_saddr(FILE * file, struct sockaddr_ipx *sipx);
int
ipx_sscanf_saddr(char* buf, struct sockaddr_ipx* sipx);
#endif

#ifdef __cplusplus
}
#endif

#endif				/* _IPXLIB_H */
