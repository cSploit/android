/*
    nwauth.c - Check a user/passwd against a NetWare server
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

	1.00  2002, January 11		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add '-D' option (D stands for daemon). There are some
		nwauth users who do not read manpages. These users
		have to add '-D' to nwauth invocation. (Hint: if your
		PAM/PHP/HTTP authentication module invokes nwauth, you
		need -D option).

 */

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

extern int bindery_only;

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf("\n");
	printf(_("usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-t type        Object type (decimal value)\n"
	       "-P password    User password\n"
	       "-b             Bindery only mode\n"
	       "-D             Daemon authentication, ignore process uid\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn_spec spec;
	struct ncp_conn *conn;
	char *server = NULL;
	char *object_name = NULL;
	char *object_pwd = NULL;
	int object_type = NCP_BINDERY_USER;
	long err;
	int prterr;
	
	char *str;

	int opt;
	int daemon = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	prterr = isatty(0);
	
	while ((opt = getopt(argc, argv, "h?S:U:t:P:bD")) != EOF)
	{
		switch (opt)
		{
		case 'S':
			server = optarg;
			break;
		case 'U':
			object_name = optarg;
			break;
		case 't':
			object_type = atoi(optarg);
			break;
		case 'P':
			object_pwd = optarg;
			break;
		case 'b':
#ifdef NDS_SUPPORT
			bindery_only = 1;
#endif
			break;
		case 'D':
			daemon = 1;
			break;
		case 'h':
		case '?':
			help();
			exit(1);
		default:
			usage();
			exit(1);
		}
	}

	err = ncp_find_conn_spec3(server, object_name, "",
				  1, daemon ? ~0U : getuid(), 0, &spec);

	if (err)
	{
		if (prterr)
			com_err(argv[0], err, _("when trying to find server"));
		exit(1);
	}
	if ((err = NWCCOpenConnByName(NULL, spec.server, NWCC_NAME_FORMAT_BIND, NWCC_OPEN_NEW_CONN, NWCC_RESERVED, &conn)) != 0)
	{
		if (prterr)
			com_err(argv[0], err, _("when trying to find server"));
		exit(1);
	}
	memset(spec.password, 0, sizeof(spec.password));

	if (object_pwd) {
		size_t l = strlen(object_pwd);
		if (l >= sizeof(spec.password)) {
			ncp_close(conn);
			if (prterr)
				fprintf(stderr, _("Password too long\n"));
			exit(1);
		}
		memcpy(spec.password, object_pwd, l+1);
	} else if (isatty(0)) {
		str = getpass(_("Enter password: "));
		if (strlen(str) >= sizeof(spec.password))
		{
			ncp_close(conn);
			printf(_("Password too long\n"));
			exit(1);
		}
		strcpy(spec.password, str);
	} else {
		fgets(spec.password, sizeof(spec.password), stdin);
	}

	str_upper(spec.password);

	if ((err = ncp_login_conn(conn, spec.user, object_type, spec.password)) != 0)
	{
		ncp_close(conn);
		if (prterr)
			com_err(argv[0], err, _("when trying to open connection"));
		exit(1);
	}
	ncp_close(conn);
	return 0;
}
