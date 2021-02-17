
/*  Copyright (c) 1995-1996 Caldera, Inc.  All Rights Reserved.

 *  See file COPYING for details.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <ncp/kernel/ipx.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <ncp/kernel/route.h>

#include "ipxutil.h"

#include "private/libintl.h"
#define _(X) gettext(X)

static struct rtentry rd;
static char *progname;

static void
usage(void)
{
	fprintf(stderr,
		_("Usage: %s add network(hex) router_network(hex) router_node(hex)\n"
		  "Usage: %s del network(hex)\n"),
		progname, progname);
}

static int
ipx_add_route(int argc, char **argv)
{
	/* Router */
	struct sockaddr_ipx *sr = (struct sockaddr_ipx *) &rd.rt_gateway;
	/* Target */
	struct sockaddr_ipx *st = (struct sockaddr_ipx *) &rd.rt_dst;
	int s;
	int result;
	int val;
	int i;

	if (argc < 2) {
		usage();
		return -1;
	}

	/* Network Number */
	if (sscanf_ipx_addr(st, argv[0], SSIPX_NETWORK) != SSIPX_NETWORK) {
		fprintf(stderr, _("%s: Invalid network number %s\n"),
			progname, argv[0]);
		return -1;
	}
	if ((st->sipx_network == htonl(0xffffffffL)) || 
	    (st->sipx_network == htonl(0L)))
	{
		fprintf(stderr, _("%s: Inappropriate network number %08lX\n"),
			progname, (unsigned long)ntohl(st->sipx_network));
		return -1;
	}
	rd.rt_flags = RTF_GATEWAY;

	/* Router Network Number */
	val = sscanf_ipx_addr(sr, argv[1], SSIPX_NETWORK|SSIPX_NODE);
	if (!(val & SSIPX_NETWORK)) {
		fprintf(stderr, _("%s: Invalid router address %s\n"),
			progname, argv[1]);
		return -1;
	}
	if (!(val & SSIPX_NODE)) {
		if ((argc < 3) || 
		    (sscanf_ipx_addr(sr, argv[2], SSIPX_NODE) != SSIPX_NODE)) {
			fprintf(stderr, _("%s: Invalid router node %s\n"),
				progname, argv[2]);
			return -1;
		}
	}
	if ((sr->sipx_network == htonl(0xffffffffL)) || 
	    (sr->sipx_network == htonl(0L)))
	{
		fprintf(stderr, _("%s: Inappropriate network number %08lX\n"),
			progname, (unsigned long)htonl(sr->sipx_network));
		exit(-1);
	}
	if ((memcmp(sr->sipx_node, "\0\0\0\0\0\0\0\0", IPX_NODE_LEN) == 0) ||
	(memcmp(sr->sipx_node, "\377\377\377\377\377\377", IPX_NODE_LEN) == 0))
	{
		char tmpnode[IPX_NODE_LEN*2+1];
		
		for (i = 0; i < IPX_NODE_LEN; i++)
			sprintf(tmpnode+i*2, "%02X", sr->sipx_node[i]);
		fprintf(stderr, _("%s: Node (%s) is invalid.\n"), progname, tmpnode);
		exit(-1);
	}
	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		exit(-1);
	}
	sr->sipx_family = st->sipx_family = AF_IPX;
	i = 0;
	do
	{
		result = ioctl(s, SIOCADDRT, &rd);
		i++;
	}
	while ((i < 5) && (result < 0) && (errno == EAGAIN));

	if (result == 0) {
		return 0;
	}

	switch (errno)
	{
	case ENETUNREACH:
		fprintf(stderr, _("%s: Router network (%08X) not reachable.\n"),
			progname, (u_int32_t)htonl(sr->sipx_network));
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname,
			strerror(errno));
		break;
	}
	return -1;
}

static int
ipx_del_route(int argc, char **argv)
{
	/* Router */
	struct sockaddr_ipx *sr = (struct sockaddr_ipx *) &rd.rt_gateway;
	/* Target */
	struct sockaddr_ipx *st = (struct sockaddr_ipx *) &rd.rt_dst;
	int s;
	int result;
	unsigned long netnum;

	if (argc != 1)
	{
		usage();
		return -1;
	}
	rd.rt_flags = RTF_GATEWAY;
	/* Network Number */
	if (sscanf_ipx_addr(st, argv[0], SSIPX_NETWORK) != SSIPX_NETWORK) {
		fprintf(stderr, _("%s: Invalid network number %s\n"),
			progname, argv[0]);
		return -1;
	}
	netnum = ntohl(st->sipx_network);
	if ((netnum == 0xffffffffL) || (netnum == 0L))
	{
		fprintf(stderr, _("%s: Inappropriate network number %08lX.\n"),
			progname, netnum);
		return -1;
	}

	st->sipx_family = sr->sipx_family = AF_IPX;
	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	result = ioctl(s, SIOCDELRT, &rd);
	if (result == 0) {
		return 0;
	}

	switch (errno)
	{
	case ENOENT:
		fprintf(stderr, _("%s: Route not found for network %08lX.\n"),
			progname, netnum);
		break;
	case EPERM:
		fprintf(stderr, _("%s: Network %08lX is directly connected.\n"),
			progname, netnum);
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname,
			strerror(errno));
		break;
	}
	return -1;
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];
	if (argc < 2)
	{
		usage();
		return -1;
	}
	if (strcasecmp(argv[1], "add") == 0)
	{
		return ipx_add_route(argc - 2, argv + 2);
	} else if (strcasecmp(argv[1], "del") == 0)
	{
		return ipx_del_route(argc - 2, argv + 2);
	} else {
		usage();
		return -1;
	}
}
