/*
    nwpasswd.c - Change a bindery object's password
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
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncp/ncplib.h>

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
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-O objectname  Object name to change, default username\n"
	       "-t type        Object type (decimal value)\n"
	       "\n"));
}


int
main(int argc, char *argv[])
{
	struct ncp_conn_spec spec;
	struct ncp_conn *conn;
	char *server = NULL;
	char *user_name = NULL;
	char *object_name = NULL;
	int object_type = NCP_BINDERY_USER;
	unsigned char ncp_key[8];
	struct ncp_bindery_object user;
	unsigned char buf_obj_name[50];
	long err;

	char *str;

	char oldpass[200], newpass1[200], newpass2[200];

	int opt;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	progname = argv[0];

	while ((opt = getopt(argc, argv, "h?S:U:O:t:")) != EOF)
	{
		switch (opt)
		{
		case 'S':
			server = optarg;
			break;
		case 'U':
			user_name = optarg;
			break;
		case 'O':
			object_name = optarg;
			break;
		case 't':
			object_type = atoi(optarg);
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
	err = ncp_find_conn_spec3(server, user_name, "",
				  1, getuid(), 0, &spec);

	if (err)
	{
		com_err(argv[0], err, _("trying to find server"));
		exit(1);
	}
	if (!object_name)
	{
		object_name = spec.user;
	} else
	{
		strcpy(buf_obj_name, object_name);
		object_name = buf_obj_name;
		str_upper(object_name);
	}
	spec.login_type = object_type;

	printf(_("Changing password for user %s on server %s\n"),
	       object_name, spec.server);

	if (object_name == spec.user)
	{
		str = getpass(_("Enter old password: "));
	} else
	{
		char sx[80];
		sprintf(sx, _("Enter password for %s: "), spec.user);
		str = getpass(sx);
	}
	if (strlen(str) >= sizeof(oldpass))
	{
		printf(_("Password too long\n"));
		exit(1);
	}
	strcpy(oldpass, str);

	str = getpass(_("Enter new password: "));
	if (strlen(str) >= sizeof(newpass1))
	{
		printf(_("Password too long\n"));
		exit(1);
	}
	strcpy(newpass1, str);

	str = getpass(_("Re-Enter new password: "));
	if (strlen(str) >= sizeof(newpass2))
	{
		printf(_("Password too long\n"));
		exit(1);
	}
	strcpy(newpass2, str);

	str_upper(oldpass);
	str_upper(newpass1);
	str_upper(newpass2);

	if (strcmp(newpass1, newpass2) != 0)
	{
		printf(_("You mistype the new password, try again\n"));
		exit(1);
	}
	strcpy(spec.password, oldpass);

	if ((conn = ncp_open(&spec, &err)) == NULL)
	{
		com_err(argv[0], err, _("when trying to open connection"));
		exit(1);
	}
	if (object_name != spec.user)
	{
		if (!(err = ncp_get_bindery_object_id(conn, 1, spec.user,
						      &user))
		    && !(err = ncp_login_user(conn, spec.user, oldpass)))
		{
			*oldpass = '\0';
		} else
		{
			com_err(argv[0], err, _("not own password"));
		}
	}
	if (((err = ncp_get_encryption_key(conn, ncp_key)) != 0)
	    || ((err = ncp_get_bindery_object_id(conn, 1, object_name,
						 &user)) != 0)
	    || ((err = ncp_change_login_passwd(conn, &user, ncp_key,
					       oldpass, newpass1)) != 0))
	{
		com_err(argv[0], err, _("trying to change password"));
	}
	ncp_close(conn);
	return 0;
}
