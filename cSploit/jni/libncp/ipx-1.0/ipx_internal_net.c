/*  Copyright (c) 1995-1996 Caldera, Inc.  All Rights Reserved.

 *  See file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <ncp/kernel/ipx.h>
#include <ncp/kernel/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "ipxutil.h"

#include "private/libintl.h"
#define _(X) gettext(X)

static struct ifreq id;
static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("Usage: %s add net_number(hex) node(hex)\n"
                          "Usage: %s del\n"),
		progname, progname);
}

static int
ipx_add_internal_net(int argc, char **argv)
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;
	unsigned long netnum;
	int i;

	if (argc < 1)
	{
		usage();
		return -1;
	}
	i = sscanf_ipx_addr(sipx, argv[0], SSIPX_NETWORK|SSIPX_NODE);
	if (!(i & SSIPX_NETWORK)) {
		fprintf(stderr, _("%s: Invalid internal network address %s\n"),
			progname, argv[0]);
		return -1;
	}
	if (!(i & SSIPX_NODE)) {
		if (argc < 2)
			memcpy(sipx->sipx_node, "\0\0\0\0\0\1", IPX_NODE_LEN);
		else if (sscanf_ipx_addr(sipx, argv[1], SSIPX_NODE) != SSIPX_NODE) {
			fprintf(stderr, _("%s: Invalid internal network node %s\n"),
				progname, argv[1]);
			return -1;
		}
	}
	netnum = ntohl(sipx->sipx_network);
	if ((netnum == 0L) || (netnum == 0xffffffffL))
	{
		fprintf(stderr, _("%s: Inappropriate network number %08lX\n"),
			progname, netnum);
		return -1;
	}
	if ((memcmp(sipx->sipx_node, "\0\0\0\0\0\0\0\0", IPX_NODE_LEN) == 0) ||
	    (memcmp(sipx->sipx_node, "\377\377\377\377\377\377", IPX_NODE_LEN) == 0))
	{
		fprintf(stderr, _("%s: Node is invalid.\n"), progname);
		return -1;
	}
	sipx->sipx_type = IPX_FRAME_NONE;
	sipx->sipx_special = IPX_INTERNAL;
	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname, 
			strerror(errno));
		return -1;
	}
	sipx->sipx_family = AF_IPX;
	sipx->sipx_action = IPX_CRTITF;
	i = 0;
	do
	{
		result = ioctl(s, SIOCSIFADDR, &id);
		i++;
	}
	while ((i < 5) && (result < 0) && (errno == EAGAIN));

	if (result == 0) {
		return 0;
	}

	switch (errno)
	{
	case EEXIST:
		fprintf(stderr, _("%s: Primary network already selected.\n"),
			progname);
		break;
	case EADDRINUSE:
		fprintf(stderr, _("%s: Network number (%08X) already in use.\n"),
			progname, (u_int32_t)htonl(sipx->sipx_network));
		break;
	case EAGAIN:
		fprintf(stderr,
			_("%s: Insufficient memory to create internal net.\n"),
			progname);
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname,
			strerror(errno));
		break;
	}
	return -1;
}

static int
ipx_del_internal_net(int argc, char **argv __attribute__((unused)))
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;

	if (argc != 0)
	{
		usage();
		return -1;
	}
	sipx->sipx_network = 0L;
	sipx->sipx_special = IPX_INTERNAL;
	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	sipx->sipx_family = AF_IPX;
	sipx->sipx_action = IPX_DLTITF;
	result = ioctl(s, SIOCSIFADDR, &id);
	if (result == 0) {
		return 0;
	}

	switch (errno)
	{
	case ENOENT:
		fprintf(stderr, _("%s: No internal network configured.\n"), progname);
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
		return ipx_add_internal_net(argc - 2, argv + 2);
	} else if (strcasecmp(argv[1], "del") == 0)
	{
		return ipx_del_internal_net(argc - 2, argv + 2);
	}
	usage();
	return -1;
}
