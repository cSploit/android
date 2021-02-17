/*
    ncptest.c
    Copyright (C) 1995 by Volker Lendecke

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

	0.00  1995			Volker Lendecke
		Initial revision.

	1.00  2000, October 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed some constants to symbolic names (SA_*).

	1.01  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include strings.h.

 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <ncp/ext/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <ncp/kernel/ipx.h>

#include <ncp/kernel/fs.h>
#include <ncp/kernel/ncp.h>
#include <ncp/kernel/ncp_fs.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;
static const char *volname = "SYS";
/*RYP: only on volume "SYS" has "PUBLIC/NLS"*/
#define TEST_DIRNAME "PUBLIC/NLS" 

static void
test_connlist(struct ncp_conn *conn)
{
	u_int8_t conn_list[256] =
	{0,};
	int no;

	ncp_get_connlist(conn, NCP_BINDERY_USER, "SUPERVISOR", &no,
			 conn_list);
	return;
}

static void __attribute__((unused))
test_send(struct ncp_conn *conn)
{
	u_int8_t conn_list[256] =
	{0,};
	int no;

	if (ncp_get_connlist(conn, NCP_BINDERY_USER, "ME", &no,
			     conn_list) != 0)
	{
		no = 0;
	}
	if (no > 0)
	{
		ncp_send_broadcast(conn, no, conn_list, "Hallo");
	}
	return;
}

static void
test_create(struct ncp_conn *conn)
{
	long result;
	struct nw_info_struct sys;
	struct nw_info_struct me;
	u_int8_t dir_handle;
	struct ncp_file_info new_file;

	printf(_("\ntest_create:\n"));
	if ((result = ncp_do_lookup(conn, NULL, volname, &sys)) != 0)
	{
		printf(_("Volume %s not found! error %ld.\n"), volname, result);
		return;
	}
	if (ncp_do_lookup(conn, &sys, "ME", &me) != 0)
	{
		printf(_("lookup public error\n"));
		return;
	}
	if (ncp_alloc_short_dir_handle(conn, &me, NCP_ALLOC_TEMPORARY,
				       &dir_handle) != 0)
	{
		printf(_("alloc_dir_handle error\n"));
		return;
	}
	if (ncp_create_file(conn, dir_handle, "BLUB.TXT", 0,
			    &new_file) != 0)
	{
		printf(_("create error\n"));
		return;
	}
	if (ncp_dealloc_dir_handle(conn, dir_handle) != 0)
	{
		printf(_("dealloc error\n"));
		return;
	}
	printf(_("test_create: Passed.\n"));
}

static int __attribute__((unused))
test_change(struct ncp_conn *conn)
{
	long result;
	unsigned char ncp_key[8];
	struct ncp_bindery_object user;

	if ((result = ncp_get_encryption_key(conn, ncp_key)) != 0)
	{
		return result;
	}
	if ((result = ncp_get_bindery_object_id(conn, 1,
						"ME", &user)) != 0)
	{
		return result;
	}
	if ((result = ncp_change_login_passwd(conn, &user, ncp_key,
					      "MEE", "ME")) != 0)
	{
		return result;
	}
	return 0;
}

static void
test_readdir(struct ncp_conn *conn)
{
	long	result;
	struct nw_info_struct sys;
	struct nw_info_struct blub;
	struct ncp_search_seq seq;
	struct nw_info_struct entry;

	printf(_("\ntest_readdir:\n"));
	if ((result = ncp_do_lookup(conn, NULL, volname, &sys)) != 0)
	{
		printf(_("Volume %s not found! error %ld.\n"), volname, result);
		return;
	}
	if (ncp_do_lookup(conn, &sys, "BLUB", &blub) != 0)
	{
		printf(_("lookup blub error\n"));
		return;
	}
	if (ncp_initialize_search(conn, &sys, NW_NS_DOS, &seq) != 0)
	{
		printf(_("init error\n"));
		return;
	}
	while (ncp_search_for_file_or_subdir(conn, &seq, &entry) == 0)
	{
		struct nw_info_struct nfs;
		printf(_("\tfound\t\t: %s\n"), entry.entryName);
		if (ncp_obtain_file_or_subdir_info(conn, NW_NS_DOS, NW_NS_NFS,
						   SA_ALL, RIM_ALL,
						   entry.volNumber,
						   entry.DosDirNum,
						   NULL,
						   &nfs) == 0)
		{
			printf(_("\tnfs name\t: %s\n"), nfs.entryName);
		}
		if (ncp_obtain_file_or_subdir_info(conn, NW_NS_DOS, NW_NS_OS2,
						   SA_ALL, RIM_ALL,
						   entry.volNumber,
						   entry.DosDirNum,
						   NULL,
						   &nfs) == 0)
		{
			printf(_("\tos2 name\t: %s\n"), nfs.entryName);
		}
	}
	if (strcasecmp (volname, "SYS") == 0)	/* test subdir search: */
	{	
		unsigned char	encpath[2000];
		int pathlen = ncp_path_to_NW_format (
				TEST_DIRNAME,
				(unsigned char*) encpath,
				sizeof (encpath));
		
		if (pathlen < 0)
		{
			printf(_("path translation error\n"));
			return;
		}
		printf (_("Search in subdir '"TEST_DIRNAME"' on volume '%s'\n"), volname);
		if (ncp_initialize_search2 (
			conn, &sys, NW_NS_DOS,
			encpath, pathlen,
			&seq) != 0)
		{
			printf(_("init search2 error\n"));
			return;
		}
		while (ncp_search_for_file_or_subdir(conn, &seq, &entry) == 0)
		{
			struct nw_info_struct xxx;
			printf(_("\tfound %s\t: %s\n"),
				(DVAL_LH(&entry.attributes, 0) & A_DIRECTORY ? "DIR " : "FILE"),
				entry.entryName);
			if (ncp_obtain_file_or_subdir_info (
				conn, NW_NS_DOS, NW_NS_NFS,
				SA_ALL, RIM_NAME,
				entry.volNumber,
				entry.dirEntNum, 
				NULL,
				&xxx) == 0)
			{
				printf (_("\tnfs name\t: %s\n"),
					xxx.entryName);
			}
			if (ncp_obtain_file_or_subdir_info (
				conn, NW_NS_DOS, NW_NS_OS2,
				SA_ALL, RIM_NAME,
				entry.volNumber,
				entry.dirEntNum,
				NULL,
				&xxx) == 0)
			{
				printf (_("\tos2 name\t: %s\n"),
					xxx.entryName);
			}
 		}
 	}
	printf(_("test_readdir: Passed.\n"));
}

static void
test_rights(struct ncp_conn *conn)
{
	long	result;
	struct nw_info_struct sys;
	struct nw_info_struct me;
	u_int16_t rights;

	printf(_("\ntest_rights:\n"));
	if ((result = ncp_do_lookup(conn, NULL, volname, &sys)) != 0)
	{
		printf(_("Volume %s not found! error %ld.\n"), volname, result);
		return;
	}
	if (ncp_do_lookup(conn, &sys, "ME", &me) != 0)
	{
		printf(_("lookup me error\n"));
		return;
	}
	if (ncp_get_eff_directory_rights(conn, 0, 0, SA_ALL,
					 sys.volNumber, sys.DosDirNum, NULL,
					 &rights) != 0)
	{
		printf(_("get sys rights error\n"));
		return;
	}
	printf(_("%s right: %4.4x\n"), volname, rights);

	if (ncp_get_eff_directory_rights(conn, 0, 0, SA_ALL,
					 me.volNumber, me.DosDirNum, NULL,
					 &rights) != 0)
	{
		printf(_("get me rights error\n"));
		return;
	}
	printf(_("me right: %4.4x\n"), rights);
	printf(_("test_rights: Passed.\n"));
	return;
}

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] pattern\n"), progname);
	return;
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
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-v volume      volume name\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	long err;
	int opt;
	int exit_code = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];
	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("in ncp_initialize"));
		return 1;
	}
	while ((opt = getopt(argc, argv, "h?Nv:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit_code = 1;
			goto finish;
		case 'v':
			volname = optarg;
			break;
		default:
			usage();
			exit_code = 1;
			goto finish;
		}
	}
	test_connlist(conn);
	test_create(conn);
	test_readdir(conn);
	test_rights(conn);
finish:	
	ncp_close(conn);
	return exit_code;
}
