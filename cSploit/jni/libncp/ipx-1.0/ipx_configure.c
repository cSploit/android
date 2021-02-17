/*  Copyright (c) 1995-1996 Caldera, Inc.  All Rights Reserved.

 *  See file COPYING for details.
 */

#include <getopt.h>	/* must be before stdio/unistd, otherwise multiple declaration warning is shown */
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <ncp/kernel/ipx.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

struct option options[] =
{
	{ "auto_primary",	required_argument,	NULL, 'p'},
	{ "auto-primary",	required_argument,	NULL, 'p'},
	{ "auto_interface",	required_argument,	NULL, 'i'},
	{ "auto-interface",	required_argument,	NULL, 'i'},
	{ "help",		no_argument,		NULL, 'h'},
	{ NULL,			0,			NULL, 0}
};

static char *progname;

static void
usage(void)
{
	fprintf(stderr,
		_("Usage: %s --auto_primary=[on|off]\n"
                  "Usage: %s --auto_interface=[on|off]\n"
                  "Usage: %s --help\n"
                  "Usage: %s\n"), progname, progname, progname, progname);
}

static int
map_string_to_bool(char *opt)
{
/* TODO: YES/NO VALUE FROM LIBC */
	if ((strcasecmp(opt, "ON") == 0) ||
	    (strcasecmp(opt, "TRUE") == 0) ||
	    (strcasecmp(opt, "SET") == 0) ||
	    (strcasecmp(opt, "YES") == 0) ||
	    (strcasecmp(opt, "1") == 0))
	{
		return 1;
	} else if ((strcasecmp(opt, "OFF") == 0) ||
		   (strcasecmp(opt, "FALSE") == 0) ||
		   (strcasecmp(opt, "CLEAR") == 0) ||
		   (strcasecmp(opt, "NO") == 0) ||
		   (strcasecmp(opt, "0") == 0))
	{
		return 0;
	}
	return -1;
}

int
main(int argc, char **argv)
{
	int s;
	int result;
	int val;
	int got_auto_pri = 0;
	int got_auto_itf = 0;
	ipx_config_data data;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		int old_errno = errno;
		fprintf(stderr, _("%s: socket: %s\n"), progname, strerror(errno));
		if (old_errno == -EINVAL)
		{
			fprintf(stderr, _("Probably you have no IPX support in "
				"your kernel\n"));
		}
		exit(-1);
	}
	while ((result = getopt_long(argc, argv, "hi:p:", options,
				     NULL)) != -1)
	{
		switch (result)
		{
		case 'p':
			if (got_auto_pri)
				break;
			got_auto_pri++;

			val = map_string_to_bool(optarg);
			if (val < 0)
			{
				usage();
				exit(-1);
			}
			{
				unsigned char v = val;

				result = ioctl(s, SIOCAIPXPRISLT, &v);
			}
			if (result < 0)
			{
				fprintf(stderr, _("%s: ioctl: %s\n"), progname,
					strerror(errno));
				exit(-1);
			}
			break;
		case 'i':
			if (got_auto_itf)
				break;
			got_auto_itf++;

			val = map_string_to_bool(optarg);
			if (val < 0)
			{
				usage();
				exit(-1);
			}
			{
				unsigned char v = val;

				result = ioctl(s, SIOCAIPXITFCRT, &v);
			}
			if (result < 0)
			{
				fprintf(stderr, _("%s: ioctl: %s\n"), progname,
					strerror(errno));
				exit(-1);
			}
			break;
		case 'h':
			usage();
			break;
		}
	}
	result = ioctl(s, SIOCIPXCFGDATA, &data);
	if (result < 0)
	{
		fprintf(stderr, _("%s: ioctl: %s\n"), progname,
			strerror(errno));
		return -1;
	}
	if (argc == 1)
	{
		fprintf(stdout, _("Auto Primary Select is %s\n"
                                  "Auto Interface Create is %s\n"),
			(data.ipxcfg_auto_select_primary) ? _("ON") : _("OFF"),
			(data.ipxcfg_auto_create_interfaces) ? _("ON") : _("OFF"));
	}
	return 0;
}
