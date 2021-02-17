/*
    verunix.c - Verify ncpd unix socket implementation
    Copyright (C) 2001  Petr Vandrovec

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

	1.00  2001, February 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#include "../../lib/ncpi.h"

#include <sys/stat.h>
#include <sys/un.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] path\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] path\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "\n"));
}

static void get4(int fd, int fn) {
	unsigned char rq[256];
	int w, r;
	
	DSET_LH(rq, 0, NCPI_REQUEST_SIGNATURE);
	DSET_LH(rq, 4, 20);
	DSET_LH(rq, 8, 1);
	DSET_LH(rq, 12, 1234);
	DSET_LH(rq, 16, fn);
	w = write(fd, rq, 20);
	printf("Write: %d, %m\n", w);
	r = read(fd, rq, 20);
	if (r < 12) {
		printf("Read: Few data: %d, %m\n", r);
		return;
	}
	if (DVAL_LH(rq, 0) != NCPI_REPLY_SIGNATURE) {
		printf("Read: Bad signature\n");
		return;
	}
	r = DVAL_LH(rq, 4);
	printf("Read: %d bytes\n", r);
	for (w = 8; w < r; w += 4) {
		printf("DD %08X: %08X\n", w, DVAL_LH(rq, w));
	}
	printf("OK\n");
}

int main(int argc, char *argv[]) {
	const char* path;
	struct stat stb;
	int opt;
	int fd;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished;
		default:
			usage();
			goto finished;
		}
	}

	if (optind < argc)
		path = argv[optind++];
	else
		path = ".";
	if (stat(path, &stb)) {
		fprintf(stderr, _("Stat failed: %m\n"));
		return 111;
	}
	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, _("socket: %m\n"));
		return 110;
	}
	{
		struct sockaddr_un sun;
		socklen_t sunlen;
		
		sunlen = offsetof(struct sockaddr_un, sun_path) + sprintf(sun.sun_path, "%cncpfs.permanent.mount.%lu", 0, (unsigned long)stb.st_dev) + 1;
		sun.sun_family = AF_UNIX;
		if (connect(fd, (struct sockaddr*)&sun, sunlen)) {
			fprintf(stderr, _("connect: %m\n"));
			return 109;
		}
		get4(fd, 0);
		get4(fd, 1);
		get4(fd, 123456);
		close(fd);
	}
finished:;
	return 0;
}
