
/*  Copyright (c) 1995-1996 Caldera, Inc.  All Rights Reserved.

 *  See file COPYING for details.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
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
	fprintf(stderr,
		_("Usage: %s add [-p] device frame_type [net_number[:node]]\n"
		  "Usage: %s del device frame_type\n"
                  "Usage: %s delall\n"
                  "Usage: %s check device frame_type\n"),
		progname, progname, progname, progname);
}

struct frame_type
{
	const char *ft_name;
	unsigned char ft_val;
}
frame_types[] =
{
	{
		"802.2", IPX_FRAME_8022
	}
	,
#ifdef IPX_FRAME_TR_8022
	{
		"802.2TR", IPX_FRAME_TR_8022
	}
	,
#endif
	{
		"802.3", IPX_FRAME_8023
	}
	,
	{
		"SNAP", IPX_FRAME_SNAP
	}
	,
	{
		"EtherII", IPX_FRAME_ETHERII
	}
};

#define NFTYPES	(sizeof(frame_types)/sizeof(struct frame_type))

static int
lookup_frame_type(char *frame)
{
	size_t j;

	for (j = 0; (j < NFTYPES) &&
	     (strcasecmp(frame_types[j].ft_name, frame));
	     j++)
		;

	if (j != NFTYPES)
		return j;

	fprintf(stderr, _("%s: Frame type must be"), progname);
	for (j = 0; j < NFTYPES; j++)
	{
		fprintf(stderr, "%s%s",
			(j == NFTYPES - 1) ? _(" or ") : " ",
			frame_types[j].ft_name);
	}
	fprintf(stderr, ".\n");
	return -1;
}

static int
ipx_add_interface(int argc, char **argv)
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;
	int i, fti = 0;
	int c;

	sipx->sipx_special = IPX_SPECIAL_NONE;
	sipx->sipx_network = 0L;
	memset(sipx->sipx_node, 0, sizeof(sipx->sipx_node));
	sipx->sipx_type = IPX_FRAME_NONE;
	while ((c = getopt(argc, argv, "p")) > 0)
	{
		switch (c)
		{
		case 'p':
			sipx->sipx_special = IPX_PRIMARY;
			break;
		}
	}

	if (((i = (argc - optind)) < 2) || (i > 3))
	{
		usage();
		return -1;
	}
	for (i = optind; i < argc; i++)
	{
		switch (i - optind)
		{
		case 0:	/* Physical Device - Required */
			strcpy(id.ifr_name, argv[i]);
			break;
		case 1:	/* Frame Type - Required */
			fti = lookup_frame_type(argv[i]);
			if (fti < 0) {
				return -1;
			}
			sipx->sipx_type = frame_types[fti].ft_val;
			break;

		case 2:	/* Network Number - Optional */
			sscanf_ipx_addr(sipx, argv[i], SSIPX_NETWORK | SSIPX_NODE);
			if (sipx->sipx_network == htonl(0xffffffffL))
			{
				fprintf(stderr,
				   _("%s: Inappropriate network number %08lX\n"),
					progname, (unsigned long)ntohl(sipx->sipx_network));
				return -1;
			}
			break;
		}
	}

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		int old_errno = errno;
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		if (old_errno == -EINVAL)
		{
			fprintf(stderr, _("Probably you have no IPX support in "
				"your kernel\n"));
		}
		return -1;
	}
	i = 0;
	sipx->sipx_family = AF_IPX;
	sipx->sipx_action = IPX_CRTITF;
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
	case EPROTONOSUPPORT:
		fprintf(stderr, _("%s: Invalid frame type (%s).\n"),
			progname, frame_types[fti].ft_name);
		break;
	case ENODEV:
		fprintf(stderr, _("%s: No such device (%s).\n"), progname,
			id.ifr_name);
		break;
	case ENETDOWN:
		fprintf(stderr, _("%s: Requested device (%s) is down.\n"), progname,
			id.ifr_name);
		break;
	case EINVAL:
		fprintf(stderr, _("%s: Invalid device (%s).\n"), progname,
			id.ifr_name);
		break;
	case EAGAIN:
		fprintf(stderr,
			_("%s: Insufficient memory to create interface.\n"),
			progname);
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname, 
			strerror(errno));
		break;
	}
	close(s);
	return -1;
}

static int
ipx_delall_interface(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;
	char buffer[80];
	char device[20];
	char frame_type[20];
	int fti;
	FILE *fp;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	fp = fopen("/proc/net/ipx_interface", "r");
	if (!fp) {
		fp = fopen("/proc/net/ipx/interface", "r");
	}
	if (fp == NULL)
	{
		close(s);
		fprintf(stderr,
			_("%s: Unable to open \"%s.\"\n"),
			progname,
			"/proc/net/ipx_interface");
		return -1;
	}
	fgets(buffer, 80, fp);
	while (fscanf(fp, "%s %s %s %s %s", buffer, buffer, buffer,
		      device, frame_type) == 5)
	{

		sipx->sipx_network = 0L;
		if (strcasecmp(device, "Internal") == 0)
		{
			sipx->sipx_special = IPX_INTERNAL;
		} else
		{
			sipx->sipx_special = IPX_SPECIAL_NONE;
			strcpy(id.ifr_name, device);
			fti = lookup_frame_type(frame_type);
			if (fti < 0)
				continue;
			sipx->sipx_type = frame_types[fti].ft_val;
		}

		sipx->sipx_action = IPX_DLTITF;
		sipx->sipx_family = AF_IPX;
		result = ioctl(s, SIOCSIFADDR, &id);
		if (result == 0)
			continue;
		switch (errno)
		{
		case EPROTONOSUPPORT:
			fprintf(stderr, _("%s: Invalid frame type (%s).\n"),
				progname, frame_type);
			break;
		case ENODEV:
			fprintf(stderr, _("%s: No such device (%s).\n"),
				progname, device);
			break;
		case EINVAL:
			fprintf(stderr, _("%s: No such IPX interface %s %s.\n"),
				progname, device, frame_type);
			break;
		default:
			fprintf(stderr, _("%s: ioctl: %s\n"), progname,
				strerror(errno));
			break;
		}
	}
	fclose(fp);
	close(s);
	return 0;
}

static int
ipx_del_interface(int argc, char **argv)
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;
	int fti;

	if (argc != 2)
	{
		usage();
		return -1;
	}
	sipx->sipx_network = 0L;
	sipx->sipx_special = IPX_SPECIAL_NONE;
	strcpy(id.ifr_name, argv[0]);
	fti = lookup_frame_type(argv[1]);
	if (fti < 0) {
		return -1;
	}
	sipx->sipx_type = frame_types[fti].ft_val;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	sipx->sipx_action = IPX_DLTITF;
	sipx->sipx_family = AF_IPX;
	result = ioctl(s, SIOCSIFADDR, &id);
	if (result == 0) {
		close(s);
		return 0;
	}

	switch (errno)
	{
	case EPROTONOSUPPORT:
		fprintf(stderr, _("%s: Invalid frame type (%s).\n"),
			progname, frame_types[fti].ft_name);
		break;
	case ENODEV:
		fprintf(stderr, _("%s: No such device (%s).\n"), progname,
			id.ifr_name);
		break;
	case EINVAL:
		fprintf(stderr, _("%s: No such IPX interface %s %s.\n"), progname,
			id.ifr_name, frame_types[fti].ft_name);
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname,
			strerror(errno));
		break;
	}
	close(s);
	return -1;
}

static int
ipx_check_interface(int argc, char **argv)
{
	struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &id.ifr_addr;
	int s;
	int result;
	int fti;

	if (argc != 2)
	{
		usage();
		return -1;
	}
	sipx->sipx_network = 0L;
	strcpy(id.ifr_name, argv[0]);
	fti = lookup_frame_type(argv[1]);
	if (fti < 0) {
		return -1;
	}
	sipx->sipx_type = frame_types[fti].ft_val;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		fprintf(stderr, _("%s: socket: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	sipx->sipx_family = AF_IPX;
	result = ioctl(s, SIOCGIFADDR, &id);
	if (result == 0)
	{
		close(s);
		printf(
			      _("IPX Address for (%s, %s) is %08X:%02X%02X%02X%02X%02X%02X.\n"),
			      argv[1], frame_types[fti].ft_name,
			      (u_int32_t)htonl(sipx->sipx_network), sipx->sipx_node[0],
			      sipx->sipx_node[1], sipx->sipx_node[2],
			      sipx->sipx_node[3], sipx->sipx_node[4],
			      sipx->sipx_node[5]);
		return 0;
	}
	switch (errno)
	{
	case EPROTONOSUPPORT:
		fprintf(stderr, _("%s: Invalid frame type (%s).\n"),
			progname, frame_types[fti].ft_name);
		break;
	case ENODEV:
		fprintf(stderr, _("%s: No such device (%s).\n"), progname,
			id.ifr_name);
		break;
	case EADDRNOTAVAIL:
		fprintf(stderr, _("%s: No such IPX interface %s %s.\n"), progname,
			id.ifr_name, frame_types[fti].ft_name);
		break;
	default:
		fprintf(stderr, _("%s: ioctl: %s\n"), progname, 
			strerror(errno));
		break;
	}
	close(s);
	return -1;
}

int
main(int argc, char **argv)
{
	int i;

	progname = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	if (argc < 2)
	{
		usage();
		return -1;
	}
	if (strcasecmp(argv[1], "add") == 0)
	{
		for (i = 1; i < (argc - 1); i++)
			argv[i] = argv[i + 1];
		return ipx_add_interface(argc - 1, argv);
	} else if (strcasecmp(argv[1], "delall") == 0)
	{
		return ipx_delall_interface(argc - 2, argv + 2);
	} else if (strcasecmp(argv[1], "del") == 0)
	{
		return ipx_del_interface(argc - 2, argv + 2);
	} else if (strcasecmp(argv[1], "check") == 0)
	{
		return ipx_check_interface(argc - 2, argv + 2);
	}
	usage();
	return -1;
}
