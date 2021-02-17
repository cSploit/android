/*

   IPX support library

   Copyright (C) 1994, 1995  Ales Dryak <e-mail: A.Dryak@sh.cvut.cz>
   Copyright (C) 1996, Volker Lendecke <lendecke@namu01.gwdg.de>

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */
#ifndef __IPXUTIL_H__

#define __IPXUTIL_H__

#include <stdio.h>
#include <ncp/kernel/ipx.h>

#define IPX_MAX_ERROR	(255)
#define IPX_THIS_NET 	(0)
#define IPX_THIS_NODE	(ipx_this_node)
#define IPX_BROADCAST	(ipx_broadcast_node)
#define IPX_AUTO_PORT	(0)
#define IPX_USER_PTYPE	(0)
#define IPX_IS_INTERNAL (1)

typedef u_int8_t	IPXNode[6];
typedef u_int32_t	IPXNet;
typedef u_int16_t	IPXPort;
typedef u_int16_t	hop_t;
typedef u_int16_t	tick_t;

void ipx_print_node(IPXNode node);
void ipx_print_network(IPXNet net);
void ipx_print_port(IPXPort port);
void ipx_print_saddr(struct sockaddr_ipx *sipx);

void ipx_fprint_node(FILE * file, IPXNode node);
void ipx_fprint_network(FILE * file, IPXNet net);
void ipx_fprint_port(FILE * file, IPXPort port);
void ipx_fprint_saddr(FILE * file, struct sockaddr_ipx *sipx);

int ipx_sscanf_node(char *buf, IPXNode node);
int ipx_sscanf_net(char *buf, IPXNet * target);

void ipx_assign_node(IPXNode dest, IPXNode src);
int ipx_node_equal(IPXNode n1, IPXNode n2);

extern IPXNode ipx_this_node;
extern IPXNode ipx_broadcast_node;

extern char ipx_err_string[IPX_MAX_ERROR + 1];

#endif
