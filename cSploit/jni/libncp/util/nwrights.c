/*
    nwrights.c - Show effective rights for dir or file.
    Copyright (C) 1996 by Volker Lendecke
  
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

	0.00  1996			Volker Lendecke
		Initial revision.
		
	1.00  2000, May 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Modified to use ncp_perms_to_str.

	1.01  2000, October 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed some constants to symbolic names (SA_*, NCP_DIRSTYLE_*)
 */

#include <ncp/nwcalls.h>
#include <unistd.h>
#include <stdlib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options] file/directory\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "\n"
	       "file/directory\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct NWCCRootEntry root;
	struct ncp_conn *conn = NULL;
	const char *path = ".";
	char permbuf[11];
	long err;
	int result = 1;
	int opt;
	u_int16_t rights;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

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

	if (optind > argc)
	{
		usage();
		goto finished;
	}
	if (optind == argc - 1)
	{
		path = argv[optind];
	}
	if ((err = ncp_open_mount(path, &conn)) != 0)
	{
		com_err(argv[0], err, _("when initializing"));
		goto finished;
	}
	if ((err = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(root),
			&root)) != 0) {
		com_err(argv[0], err, _("when retrieving root entry"));
		goto finished;
	}
	if ((err = ncp_get_eff_directory_rights(conn, 0, 0, SA_ALL,
						root.volume,
						root.dirEnt,
						NULL,
						&rights)) != 0)
	{
		com_err(argv[0], err, _("when finding rights"));
		goto finished;
	}
	printf(_("Your effective rights for %s are: %s\n"),
	       path,
	       ncp_perms_to_str(permbuf, rights));

	if ((rights & NCP_PERM_SUPER) != 0)
	{
		printf(_("(S): You have SUPERVISOR rights\n"));
	}
	if ((rights & NCP_PERM_READ) != 0)
	{
		printf(_("(R): You may READ from files\n"));
	}
	if ((rights & NCP_PERM_WRITE) != 0)
	{
		printf(_("(W): You may WRITE to files\n"));
	}
	if ((rights & NCP_PERM_CREATE) != 0)
	{
		printf(_("(C): You may CREATE files\n"));
	}
	if ((rights & NCP_PERM_DELETE) != 0)
	{
		printf(_("(E): You may ERASE files\n"));
	}
	if ((rights & NCP_PERM_MODIFY) != 0)
	{
		printf(_("(M): You may MODIFY directory\n"));
	}
	if ((rights & NCP_PERM_SEARCH) != 0)
	{
		printf(_("(F): You may SCAN for files\n"));
	}
	if ((rights & NCP_PERM_OWNER) != 0)
	{
		printf(_("(A): You may change ACCESS control\n"));
	}
	result = 0;

      finished:
	ncp_close(conn);
	return result;
}
