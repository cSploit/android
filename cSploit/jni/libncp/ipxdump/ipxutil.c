/*
   IPX support library - general functions

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
#include <string.h>
#include <netinet/in.h>
#include "ipxutil.h"

void
ipx_fprint_node(FILE * file, IPXNode node)
{
	fprintf(file, "%02X%02X%02X%02X%02X%02X",
		(unsigned char) node[0],
		(unsigned char) node[1],
		(unsigned char) node[2],
		(unsigned char) node[3],
		(unsigned char) node[4],
		(unsigned char) node[5]
	    );
}

void
ipx_fprint_network(FILE * file, IPXNet net)
{
	fprintf(file, "%08lX", (unsigned long)net);
}

void
ipx_fprint_port(FILE * file, IPXPort port)
{
	fprintf(file, "%04X", port);
}

void
ipx_fprint_saddr(FILE * file, struct sockaddr_ipx *sipx)
{
	ipx_fprint_network(file, ntohl(sipx->sipx_network));
	fprintf(file, ":");
	ipx_fprint_node(file, sipx->sipx_node);
	fprintf(file, ":");
	ipx_fprint_port(file, ntohs(sipx->sipx_port));
}

void
ipx_print_node(IPXNode node)
{
	ipx_fprint_node(stdout, node);
}

void
ipx_print_network(IPXNet net)
{
	ipx_fprint_network(stdout, net);
}

void
ipx_print_port(IPXPort port)
{
	ipx_fprint_port(stdout, port);
}

void
ipx_print_saddr(struct sockaddr_ipx *sipx)
{
	ipx_fprint_saddr(stdout, sipx);
}

void
ipx_assign_node(IPXNode dest, IPXNode src)
{
	memcpy(dest, src, sizeof(IPXNode));
}

int
ipx_node_equal(IPXNode n1, IPXNode n2)
{
	return memcmp(n1, n2, sizeof(IPXNode)) == 0;
}

int
ipx_sscanf_node(char *buf, IPXNode node)
{
	int i;
	int n[6];

	if ((i = sscanf(buf, "%2x%2x%2x%2x%2x%2x",
			&(n[0]), &(n[1]), &(n[2]),
			&(n[3]), &(n[4]), &(n[5]))) != 6)
	{
		return -1;
	}
	for (i = 0; i < 6; i++)
	{
		node[i] = n[i];
	}
	return 0;
}

int
ipx_sscanf_net(char *buf, IPXNet * target)
{
	unsigned long net;
	
	if (sscanf(buf, "%8lX", &net) == 1)
	{
		*target = net;
		return 0;
	}
	return -1;
}

IPXNode ipx_this_node =
{0, 0, 0, 0, 0, 0};
IPXNode ipx_broadcast_node =
{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
char ipx_err_string[IPX_MAX_ERROR + 1] = "no error detected";
