/*
    ncpmount.c
    Copyright (C) 1995, 1997 by Volker Lendecke

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
		Initial release.

	0.01  1996, January 20		Steven N. Hirsch <hirsch@emba.uvm.edu>
		If the ncpfs support is not loaded and we are using kerneld to
		autoload modules, then we don't want to do it here.  I added
		a conditional which leaves out the test and load code.

		Even if we _do_ want ncpmount to load the module, passing a
		fully-qualified pathname to modprobe causes it to bypass a 
		path search.  This may lead to ncpfs.o not being found on
 		some systems.

	0.02  1998, November 28		Wolfram Pienkoss <wp@bszh.de>
		NLS added. Thanks Petr Vandrovec for the idea of charset select
		and many hints.

	0.03  1999, September 9		Petr Vandrovec <vandrove@vc.cvut.cz>
 		Added ttl option.
		Reverted back to exec("/sbin/nwmsg") for incomming messages. 
		This allows tknwmsg work again...

	0.04  1999, September 9		Paul Rensing <paulr@dragonsys.com>
		Added resetting behavior for SIGTERM. This fixes ncpmount from 
		perl scripts.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
		Replaced nds_get_tree_name with NWIsDSServer.

	1.01  2000, January 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changes for 32bit uids.
		
	1.02  2000, April 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed /sbin/nwmsg to $(sbindir)/nwmsg (recomended by
			MandrakeSoft).

	1.03  2000, April 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Silently ignore 'noauto' option, mount-2.10f (at least)
			passes it down to mount.ncp.

	1.04  2000, June 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add NCP/TCP support.

	1.05  2000, October 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added nfsextras support.
		Changed some constants to symbolic names (SA_*)

	1.06  2001, January 7		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Made all error exit codes differents to help debug Pam module

	1.07  2001, February 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added UNIX socket handling for global features.

	1.08  2001, September 23	Petr Vandrovec <vandrove@vc.cvut.cz>
		Fix multiple mounts when using UDP or TCP.

	1.09  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Verify uid/gid/owner arguments carefully. It should not be
		security problem, but who knows which program derives some 
		security information from file uid/gid...

	1.10  2002, February 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Erase password quickly from command line.
		Clear command line completely.
		Simpler command line handling.

	1.11  2002, April 19		Peter Schwindt <schwindt@ba-loerrach.de>
		Add option -A to the help text.

 */

#define NCP_OBSOLETE

#include "ncpm_common.h"
#include "mount_login.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ncp/ext/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/mount.h>
#include <mntent.h>
#include <ncp/kernel/ipx.h>
#include <ncp/nwclient.h>
#include <sys/ioctl.h>
#ifdef CONFIG_NATIVE_UNIX
#include <linux/un.h>
#endif

#include "private/libintl.h"
#define _(X) gettext(X)

#include "../lib/ncpi.h"

static void usage(void);
static void help(void);

void veprintf(const char* message, va_list ap) {
	vfprintf(stderr, message, ap);
}

static void opt_set_tree(struct ncp_mount_info* info, const char* param) {
	if (strlen(param) > MAX_TREE_NAME_CHARS) {
		errexit(65, _("Specified tree name `%s' is too long\n"), param);
	}
	info->tree = param;
}

static void opt_set_bindery(struct ncp_mount_info* info, unsigned int param) {
	info->force_bindery_login = param;
}

static void opt_set_volume(struct ncp_mount_info* info, const char* param) {
	info->volume = param;
}

static void opt_set_allow_multiple(struct ncp_mount_info* info, unsigned int param) {
	info->allow_multiple_connections = param;
}

static void opt_set_auth(struct ncp_mount_info* info, const char* param) {
	info->auth_src = param;
}

static struct optinfo opts[] = {
	{'T', "tree",		FN_STRING,	opt_set_tree,		0},
	{'b', "bindery",	FN_NOPARAM,	opt_set_bindery,	1},
	{'V', "volume",		FN_STRING,	opt_set_volume,		0},
	{'m', "multiple",	FN_NOPARAM,	opt_set_allow_multiple,	1},
	{'a', "auth",		FN_STRING,	opt_set_auth,		0},
	{0,   NULL,		0,		NULL,			0}
};

int
main(int argc, char *argv[])
{
	struct ncp_mount_info info;
	struct stat st;
	char *mount_name;
#ifdef NDS_SUPPORT
	NWDSContextHandle ctx;
	NWCONN_HANDLE auth_conn;
#endif

	int result;

	long err;

	struct ncp_conn_spec spec;

	struct ncp_conn *conn;

	int opt;

#ifdef CONFIG_NATIVE_UNIX
	struct sockaddr_un server_un;
#endif

	char *tmp_mount;

	int mount_ncp = 0;
	int opt_n = 0;
	int opt_v = 0;

	verify_argv(argc, argv);

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	init_mount_info(&info);

	while ((opt = getopt(argc, argv, "a:CS:U:c:u:g:f:d:P:nh?vV:t:r:o:"
		"sN:y:p:bi:mA:T:"
#ifdef MOUNT2
		"2"
#endif
#ifdef MOUNT3
		"345"
#endif
	        )) != EOF)
	{
		char tmpopt[2];

		switch (opt)
		{
		case 'C':
		case 's':
		case 'b':
		case 'm':
			optarg = NULL;
		case 'a':
		case 'S':
		case 'T':
		case 'U':
		case 'c':
		case 'u':
		case 'g':
		case 'f':
		case 'd':
		case 'V':
		case 't':
		case 'r':
		case 'i':
		case 'A':
		case 'y':
		case 'p':
			tmpopt[0] = opt;
			tmpopt[1] = 0;
			proc_option(opts, &info, tmpopt, optarg);
			break;
		case 'P':
			if (!mount_ncp) opt_n = 0;	/* clear -n (nopasswd) */
			tmpopt[0] = opt;
			tmpopt[1] = 0;
			proc_option(opts, &info, tmpopt, optarg);
			break;
		case 'n':
			opt_n = 1;	/* no passwd or no update /etc/fstab */
			break;
		case 'h':
		case '?':
			help();
			exit(27);
		case 'v':
			opt_v = 1;	/* version or verbose */
			break;	
#ifdef MOUNT2
		case '2':
			info.version = 2;
			break;
#endif /* MOUNT2 */
#ifdef MOUNT3
		case '3':
			info.version = 3;
			break;
		case '4':
			info.version = 4;
			break;
		case '5':
			info.version = 5;
			break;
#endif /* MOUNT3 */
		case 'N':
			{
				char *inp;
				char *out;

				inp = optarg;

				while ((out = strtok(inp, ",;:"))!=NULL) {
					inp=NULL;
					if (!strcasecmp(out, "OS2"))
						proc_option(opts, &info, "noos2", NULL);
					else if (!strcasecmp(out, "LONG"))
						proc_option(opts, &info, "nolong", NULL);
					else if (!strcasecmp(out, "NFS"))
						proc_option(opts, &info, "nonfs", NULL);
					else {
						errexit(128, _("Unknown namespace \"%s\"\n"), out);
					}
				}
			};
			break;
		case 'o':
			{
				char* arg;

				arg = optarg;
				mount_ncp = 1;
				while (arg) {
					char* end;
					char* parm;

					arg += strspn(arg, " \t");
					if (!*arg) break;
					end = strchr(arg, ',');
					if (end) *end++ = 0;

					parm = strchr(arg, '=');
					if (parm) *parm++ = 0;

					proc_option(opts, &info, arg, parm);
					arg = end;
				};
			}
			break;
		default:
			usage();
			return -1;
		}
	}
	
	if (opt_n && !mount_ncp) {
		opt_n = 0;
		proc_option(opts, &info, "passwd", "");
	}
	if (opt_v && !mount_ncp) {
		fprintf(stderr, _("ncpfs version %s\n"), NCPFS_VERSION);
		exit(30);
	}
	if (optind < argc - 1) {
		char* s;
		char* u;

		s = strdup(argv[optind++]);
		if (s) {
			u = strchr(s, '/');
			if (u) *u++ = 0;
		} else
			u = NULL;
		if (!info.server && !info.tree) info.server = s;
		if (!info.user) info.user = u;
	}
	if (info.server && info.tree) {
		errexit(66, _("Both tree and server name were specified. It is not allowed.\n"));
	}
	if (myeuid != 0)
	{
		errexit(26, _("%s must be installed suid root\n"), progname);
	}
	if (optind != argc - 1)
	{
		usage();
		return -1;
	}

	realpath(argv[optind], mount_point);

	if (chdir(mount_point))
	{
		errexit(31, _("Could not change directory into mount target %s: %s\n"),
			mount_point, strerror(errno));
	}
	if (stat(".", &st) == -1)
	{
		errexit(31, _("Mount point %s does not exist: %s\n"),
			mount_point, strerror(errno));
	}
	if (mount_ok(&st) != 0)
	{
		errexit(32, _("Cannot to mount on %s: %s\n"),
			mount_point, strerror(errno));
	}

	get_passwd(&info);

	if (myuid != info.mdata.mounted_uid) {
		if (setreuid(info.mdata.mounted_uid, -1)) {
			errexit(76, _("Cannot impersonate as requested: %s\n"), strerror(errno));
		}
	}
#ifdef NDS_SUPPORT
	if (info.tree) {
		static char vol[64];
		static char serv[64];
		nuint32 ctxf;
		
		if (!info.volume) {
			errexit(70, _("You must specify NDS volume name if you specified tree name.\n"));
		}
		err = NWDSCreateContextHandle(&ctx);
		if (err) {
			mycom_err(err, _("in create context"));
			exit(68);
		}
		if (!NWDSGetContext(ctx, DCK_FLAGS, &ctxf)) {
			ctxf |= DCV_XLATE_STRINGS | DCV_DEREF_ALIASES | DCV_TYPELESS_NAMES | DCV_DEREF_BASE_CLASS;
			NWDSSetContext(ctx, DCK_FLAGS, &ctxf);
		}
		err = NWCCOpenConnByName(NULL, info.tree, NWCC_NAME_FORMAT_NDS_TREE,
			NWCC_OPEN_NEW_CONN, 
			NWCC_RESERVED, &conn);
		if (err) {
			NWDSFreeContext(ctx);
			mycom_err(err, _("in tree search"));
			exit(67);
		}
		err = nds_login_auth(conn, info.user, info.password);
		if (err) {
			NWCCCloseConn(conn);
			NWDSFreeContext(ctx);
			mycom_err(err, _("in nds login"));
			exit(68);
		}
		NWDSAddConnection(ctx, conn);
		err = NWCXGetNDSVolumeServerAndResourceName(ctx,
			info.volume, serv, sizeof(serv),
			vol, sizeof(vol));
		if (err) {
			NWDSFreeContext(ctx);
			NWCCCloseConn(conn);
			mycom_err(err, _("in volume search"));
			exit(69);
		}
		NWDSFreeContext(ctx);
		NWCCCloseConn(conn);
		info.server = serv;
		strcpy(info.NWpath + 2, vol);
		info.NWpath[0] = 1;
		info.NWpath[1] = strlen(vol);
		info.pathlen = strlen(vol) + 2;
	} else 
#endif
	{
		if (info.volume) {
			int ln;

			ln = ncp_path_to_NW_format(info.volume, info.NWpath, sizeof(info.NWpath));
			info.remote_path = info.volume;
			if (ln < 0) {
				errexit(18, _("Volume path `%s' is invalid: `%s'\n"), info.volume, strerror(-ln));
			};
			info.pathlen = ln;
			if (info.pathlen == 1) {
				info.mdata.mounted_vol = "";
				info.remote_path = "/";
			} else if (info.NWpath[0] != 1) {
				info.mdata.mounted_vol = "dummy";
			} else if (strlen(info.volume) > NCP_VOLNAME_LEN) {
				errexit(19, _("Volume name `%s' is too long\n"), info.volume);
			} else {
				info.mdata.mounted_vol = info.volume;
			}
		}
	}
	err = NWDSCreateContextHandle(&ctx);
	if (err) {
		mycom_err(err, _("in create context"));
		exit(113);
	}
	if (info.auth_src) {
		NWDSChar user[MAX_DN_BYTES];

		err = ncp_open_mount(info.auth_src, &auth_conn);
		if (err) {
			NWDSFreeContext(ctx);
			mycom_err(err, _("opening mount %s"), info.auth_src);
			exit(114);
		}
		NWDSAddConnection(ctx, auth_conn);
		err = NWDSWhoAmI(ctx, user);
		if (err) {
			NWDSFreeContext(ctx);
			NWCCCloseConn(auth_conn);
			mycom_err(err, _("retrieving user name"));
			exit(117);
		}
		memset(&spec, 0, sizeof(spec));
		strncpy(spec.server, info.server, sizeof(spec.server));
		strncpy(spec.user, user, sizeof(spec.user));
	} else {
		if ((err = ncp_find_conn_spec3(info.server, info.user, info.password, 1, 
				info.mdata.uid, 1, &spec))
		    != 0)
		{
			NWDSFreeContext(ctx);
			mycom_err(err, _("in find_conn_spec"));
			exit(33);
		}
		if (info.upcase_password != 0)
		{
			str_upper(spec.password);
		}
		auth_conn = NULL;
	}
	info.mdata.server_name = spec.server;

	if (info.mdata.dir_mode == 0)
	{
		info.mdata.dir_mode = info.mdata.file_mode;
		if ((info.mdata.dir_mode & S_IRUSR) != 0)
			info.mdata.dir_mode |= S_IXUSR;
		if ((info.mdata.dir_mode & S_IRGRP) != 0)
			info.mdata.dir_mode |= S_IXGRP;
		if ((info.mdata.dir_mode & S_IROTH) != 0)
			info.mdata.dir_mode |= S_IXOTH;
	}
#if defined(CONFIG_NATIVE_IP) || defined(CONFIG_NATIVE_UNIX)
	if (info.server_name) {
#if defined(CONFIG_NATIVE_UNIX)
		if (info.server_name[0] == '/') {
			server_un.sun_family = AF_UNIX;
			memcpy(server_un.sun_path, "\000ncpfs", 6);
			memcpy(&info.mdata.serv_addr, &server_un, 16);
		} else
#endif
#if defined(CONFIG_NATIVE_IP)		
		{
			nuint tran;

			if ((!info.allow_multiple_connections)&&
			    ((tmp_mount = ncp_find_permanent(&spec)) != NULL))
			{
				NWDSFreeContext(ctx);
				if (auth_conn) {
					NWCCCloseConn(auth_conn);
				}
				fprintf(stderr,
					_("You already have mounted server %s\nas user "
					"%s\non mount point %s\n"), spec.server, spec.user,
					tmp_mount);
				exit(35);
			}

			tran = NT_UDP;
			if (info.nt.valid && info.nt.value == NT_TCP) {
				tran = info.nt.value;
			}
			err = ncp_find_server_addr(&info.server_name, OT_FILE_SERVER, (struct sockaddr*)&info.mdata.serv_addr,
					sizeof(info.mdata.serv_addr), tran);
			if (err) {
				NWDSFreeContext(ctx);
				if (auth_conn) {
					NWCCCloseConn(auth_conn);
				}
				eprintf(_("Get host address `%s': %s\n"), info.server_name, strnwerror(err));
				exit(1);
			}
		}
#else
		{
			goto ncpipx;
		}
#endif		
	} else 
#endif	/* CONFIG_NATIVE_IP */
	{
#if defined(CONFIG_NATIVE_UNIX) && !defined(CONFIG_NATIVE_IP)
ncpipx:;
#endif
		if ((!info.allow_multiple_connections)&&
		    ((tmp_mount = ncp_find_permanent(&spec)) != NULL))
		{
			NWDSFreeContext(ctx);
			if (auth_conn) {
				NWCCCloseConn(auth_conn);
			}
			fprintf(stderr,
				_("You already have mounted server %s\nas user "
				"%s\non mount point %s\n"), spec.server, spec.user,
				tmp_mount);
			exit(35);
		}
		if ((err = ncp_find_fileserver(spec.server, (struct sockaddr*)&info.mdata.serv_addr, sizeof(info.mdata.serv_addr))) != 0)
		{
			NWDSFreeContext(ctx);
			if (auth_conn) {
				NWCCCloseConn(auth_conn);
			}
			mycom_err(err, _("when trying to find %s"),
				spec.server);
			exit(36);
		}
	}

	err = proc_buildconn(&info);
	if (err) {
		NWDSFreeContext(ctx);
		if (auth_conn) {
			NWCCCloseConn(auth_conn);
		}
		exit(err);
	}

	info.mdata.mount_point = mount_point;

	if (asprintf(&mount_name, "%s/%s", spec.server, spec.user) < 0) {
		NWDSFreeContext(ctx);
		if (auth_conn) {
			NWCCCloseConn(auth_conn);
		}
		errexit(85, _("Cannot allocate memory for mtab entry: %s\n"), strerror(ENOMEM));
	}

	result = ncp_mount(mount_name, &info);
	if (result)
	{
		free(mount_name);
		NWDSFreeContext(ctx);
		if (auth_conn) {
			NWCCCloseConn(auth_conn);
		}
		mycom_err(result, _("in mount(2)"));
		exit(51);
	}
	err = proc_aftermount(&info, &conn);
	if (err) {
		proc_ncpm_umount(info.mdata.mount_point);
		free(mount_name);
		NWDSFreeContext(ctx);
		if (auth_conn) {
			NWCCCloseConn(auth_conn);
		}
		exit(err);
	}
#ifdef NDS_SUPPORT
	if (info.auth_src != NULL) {
		err = NWDSAuthenticateConn(ctx, conn);
		if (err) {
			proc_ncpm_umount(info.mdata.mount_point);
			free(mount_name);
			NWDSFreeContext(ctx);
			if (auth_conn) {
				NWCCCloseConn(auth_conn);
			}
			mycom_err(err, _("in authenticate connection"));
			exit(115);
		}
	} else if (!info.force_bindery_login && NWIsDSServer(conn, NULL))
	{
		if ((err = nds_login_auth(conn, spec.user, spec.password)))
		{
			if (err != NWE_PASSWORD_EXPIRED) {
				mycom_err(err, _("in nds login"));
				fprintf(stderr, _("Login denied.\n"));
				ncp_close(conn);
				proc_ncpm_umount(mount_point);
				free(mount_name);
				NWDSFreeContext(ctx);
				if (auth_conn) {
					NWCCCloseConn(auth_conn);
				}
				exit(55);
			}
	    		fprintf(stderr, _("Your password has expired\n"));
		}
	}
	else
	{
#endif	
	if ((err = ncp_login_user(conn, spec.user, spec.password)) != 0)
	{
		struct nw_property p;
		struct ncp_prop_login_control *l
		= (struct ncp_prop_login_control *) &p;

		if (err != NWE_PASSWORD_EXPIRED)
		{
			mycom_err(err, _("in login"));
			fprintf(stderr, _("Login denied\n"));
			ncp_close(conn);
			proc_ncpm_umount(mount_point);
			free(mount_name);
			NWDSFreeContext(ctx);
			if (auth_conn) {
				NWCCCloseConn(auth_conn);
			}
			exit(56);
		}
		fprintf(stderr, _("Your password has expired\n"));

		if ((err = ncp_read_property_value(conn, NCP_BINDERY_USER,
						   spec.user, 1,
						   "LOGIN_CONTROL", &p)) == 0)
		{
			fprintf(stderr, _("You have %d login attempts left\n"),
				l->GraceLogins);
		}
	}
#ifdef NDS_SUPPORT
	}
#endif
	err = NWDSFreeContext(ctx);
	if (err) {
		ncp_close(conn);
		proc_ncpm_umount(mount_point);
		free(mount_name);
		if (auth_conn) {
			NWCCCloseConn(auth_conn);
		}
		mycom_err(err, _("in free context"));
		exit(116);
	}
	if (auth_conn) {
		NWCCCloseConn(auth_conn);
	}
	if ((err = ncp_mount_specific(conn, -1, info.NWpath, info.pathlen)) != 0) 
	{
		ncp_close(conn);
		proc_ncpm_umount(mount_point);
		free(mount_name);
		errexit(57, _("Cannot access path \"%s\": %s\n"), info.remote_path, strerror(-err));
	}
	ncp_close(conn);

	if (!opt_n) {
		block_sigs();
		add_mnt_entry(mount_name, mount_point, info.flags);
		unblock_sigs();
	}
	return 0;
}

static void usage(void)
{
	printf(_("usage: %s [options] mount-point\n"), progname);
	printf(_("Try `%s -h' for more information\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options] mount-point\n"), progname);
	/* printf() is macro in glibc2.2... */
	fprintf(stdout,
	     _("\n"
	       "-S server      Server name to be used\n"
	       "-A dns_name    DNS server name to be used when mounting over TCP or UDP\n"
	       "-U username    Username sent to server\n"
	       "-V volume      Volume to mount, for NFS re-export\n"
	       "-u uid         uid the mounted files get\n"
	       "-g gid         gid the mounted files get\n"
	       "-f mode        permission the files get (octal notation)\n"
	       "-d mode        permission the dirs get (octal notation)\n"
	       "-c uid         uid to identify the connection to mount on\n"
	       "               Only makes sense for root\n"
	       "-t time_out    Waiting time (in 1/100s) to wait for\n"
	       "               an answer from the server. Default: 60\n"
	       "-r retry_count Number of retry attempts. Default: 5\n"
	       "-C             Don't convert password to uppercase\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "               If neither -P nor -n are given, you are\n"
	       "               asked for a password.\n"
	       "-s             Enable renaming/deletion of read-only files\n"
	       "-h             print this help text\n"
	       "-v             print ncpfs version number\n"
	       "%s%s"
	       "-m             Allow multiple logins to server\n"
	       "-N os2,nfs     Do not use specified namespaces on mounted volume\n"
	       "-y charset     character set used for input and display\n"
	       "-p codepage    codepage used on volume, including letters `cp'\n"
	       "\n"),
#ifdef NDS_SUPPORT
	       _("-b             Force bindery login to NDS servers\n")
#else
	       ""
#endif /* NDS_SUPPORT */
	       ,
#ifdef SIGNATURES
	       _("-i level       Signature level, 0=never, 1=supported, 2=preferred, 3=required\n")
#else
	       ""
#endif /* SIGNATURES */
	       );
}
