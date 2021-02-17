/*
    ncplogin.c ncpmap.c and possibly ncpmount.c
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
        1.00 2001 February 26           Patrick Pollet <patrick.pollet@insa-lyon.fr>
            Initial release
            This program is based on ncpmount.c release 1.07 2001, February 25	by Petr Vandrovec <vandrove@vc.cvut.cz>
            Basically it substitute the -S server parameter with -T tree -X context options
            Source code of ncplogin AND ncpmap (AND eventually ncpmount) should stays the same.
            See the Makefile.in for compile flags.
                CFLAGS_ncplogin.o = -DNWMSG=\"$(sbindir)/nwmsg\"
                CFLAGS_ncpmap.o = -DNWMSG=\"$(sbindir)/nwmsg\" -DNCPMAP=1
                CFLAGS_ncpmount.o = -DNWMSG=\"$(sbindir)/nwmsg\" -DNCPMOUNT=1

      1.01 2001 March 10                Patrick Pollet <patrick.pollet@insa-lyon.fr>
             Added failed in all error messages for easier TCL/tk parsing
             Added -'E' option to echo mount_point in case of success (for TCL/Tk)
             Shuffled some blocks of code to mimize #ifdef #ifndef
             Corrected:#1135 (make_unique_dir= forgot to autocreate ~/ncp!
             Corrected:#1397 wrong list of possible arguments to getopt()
             Corrected:#1754 else was missing.
             Corrected:#2147 ncplogin cannot use NWDSWhoami since we are not yet authenticated
             Corrected:forgot to free context at various errors points
             Corrected:incomplete help ncplogin,ncpmap (-X missing)

     1.02 2001 March 12                 Patrick Pollet <patrick.pollet@insa-lyon.fr>
             Corrected #1668  info.resourcename was not set to SYS with -S option.
             Note that with ncplogin/ncpmap: if volume is already mounted
              the message already mounted:name of mount point will be printed and exitcode=0 (#2201)

     1.03 2001 March 16                 Patrick Pollet <patrick.pollet@insa-lyon.fr>
             Added Default user logic if missing from command line (ncplogin only in TREE mode)

     1.04 2001 August 11                Petr Vandrovec
              Modified ncpmount part to match ncpmount v 1.08

     1.05 2001 October 25               Patrick Pollet <patrick.pollet@insa-lyon.fr>
                Added a signal(SIGPIPE, SIG_IGN) in process_connexion to match ncpmount v 1.08
                In progress: ncpmap does not complains in case of multiple connexions and mount it twice
                (2 entries in /etc/mtab)???
		not a problem with ncplogin since it check that user is not currently authenticated ...

     1.06 2001 December 8	        Patrick Pollet <patrick.pollet@insa-lyon.fr>
		ncplogin with -S option did not searched for missing -U user argument in environnment/nwclient
		file if not supplied on the command line. Code moved from the -T case to all cases.

     1.07 2002 January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Apply checks on uid/gid/owner value, so normal user can no longer mount filesystem
		with someone else's uid.
		Convert fprintf(stderr, "failed:yyy"...) to eprintf("yyy"...). Now ncpmount and ncplogin
		use same .po entries.
		Convert eprintf() + exit() => errexit()

	1.08  2002, February 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fix core dump when no username is specified to ncplogin.

	1.09  2002, February 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Erase password quickly from command line.
		Clear command line completely.
		Simpler command line handling.

	1.10  2002, April 19		Peter Schwindt <schwindt@ba-loerrach.de>
		Add option -A to the help text.

	1.11  2002, May 12		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		On NFS mounted homes, local root user has normally no rights to peek in user's home, not speaking
		about autocreating there a mount point directory (unless NFS directories are exported with the no_root_squash
		option, which is quite unsafe !). So we autocreate mount point in a local /mnt/ncp/$USER directory.
		As usual this directory is autocreated and chmoded to 0700 ... To activate this, we added a -l command line option
		(for local mount).

 */

#define NCP_OBSOLETE

#include "ncpm_common.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
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
#include <sys/ioctl.h>
#ifdef CONFIG_NATIVE_UNIX
#include <linux/un.h>
#endif

#ifndef NDS_SUPPORT
#	error "NDS_SUPPORT must be enabled for ncplogin/ncpmap"
#endif
#include <ncp/nwclient.h>

#include "private/libintl.h"
#define _(X) gettext(X)

#include "../lib/ncpi.h"
#include "mount_login.h"

static void usage(void);
static void help(void);

void veprintf(const char* message, va_list ap) {
	fprintf(stderr, "failed:");
	vfprintf(stderr, message, ap);
}

/*begin of "alternating code between ncplogin/ncpmap/ncpmount */
/*basically ncpmap has no user nor passord required
   thus all options regarding "user identity" (uid,gid, mount id ...) are marked off */
/*options of ncpmount irrelevant to both are marked off with #ifdef NCPMOUNT #endif */

/*ncplogin/ncpmap */
/*actually ncplogin does not care about this (it use -V SYS and -a internally) */
#ifdef NCPMAP
/* we will parse volume name later since in can be in NDS format */
static void opt_set_volume(struct ncp_mount_info* info, const char* param) {
	info->remote_path = param;
}

static void opt_set_auto_create(struct ncp_mount_info* info, unsigned int param) {
	info->autocreate = param;
}

struct optinfo extopts[] = {
	{'a', "autocreate",	FN_NOPARAM,	opt_set_auto_create,	1},/*not in ncplogin*/
	{'V', "volume",		FN_STRING,	opt_set_volume,		0},/*not in ncplogin*/
	{0,   NULL,		0,		NULL,			0}
};
#endif

        static int opt_set_volume_after_parsing_all_options(struct ncp_mount_info* info) {
                char *path;
		int e;

		if (info->root_path) {
			e = asprintf(&path, "%s/%s", info->remote_path, info->root_path);
		} else {
			e = asprintf(&path, "%s", info->remote_path);
		}
		if (e == -1) {
			errexit(84, _("Cannot allocate memory for path\n"));
		}
                /* I keep forgeting typing it in uppercase so let's do it here */
                str_upper(path);
                info->pathlen = e = ncp_path_to_NW_format(path, info->NWpath, sizeof(info->NWpath));
	        if (e < 0) {
		        errexit(18, _("Volume path `%s' is invalid: `%s'\n"), path, strerror(-e));
	        };
	        if (info->pathlen == 1) {
		        info->mdata.mounted_vol = "";
		        info->remote_path = "/";
			free(path);
	        } else if (info->NWpath[0] != 1) {
		        info->mdata.mounted_vol = "dummy";
			free(path);
	        } else if (strlen(path) > NCP_VOLNAME_LEN) {
		        errexit(19, _("Volume name `%s' is too long\n"), path);
	        } else {
		        info->mdata.mounted_vol = path;
	        }
                return 0;
        }


        /* new options ncplogin/ncpmap */
        static void opt_set_tree(struct ncp_mount_info* info, const char* param) {
		info->tree = param;
        }

        static void opt_set_root_path(struct ncp_mount_info* info, const char* param) {
	        info->root_path = param;
        }

        static void opt_set_name_context(struct ncp_mount_info* info, const char* param) {
                if (strlen(param) < sizeof(info->context))
	                strcpy(info->context, param);
                else{
	                errexit(19, _("Context name `%s' is too long\n"), param);
                }
        }

        static void opt_set_bcast_mode(struct ncp_mount_info* info, unsigned int param) {
	        info->bcastmode = param & 3;
        }

        static void opt_set_echo(struct ncp_mount_info* info, unsigned int param) {
	        info->echo_mnt_pnt = param;
        }

	static void opt_set_use_local_mnt_dir(struct ncp_mount_info* info, const char* param) {
	        info->use_local_mnt_dir = param ? param : "/mnt/ncp";
		info->autocreate = 1;  /* if -l present, force -a */
	}

static struct optinfo opts[] = {
	{'X', "namecontext",	FN_STRING,	opt_set_name_context,	1},/*unfortunate that C,c and N reserved*/
	{'T', "tree",		FN_STRING,	opt_set_tree,		0},
	{'R', "root",		FN_STRING,	opt_set_root_path,	0},
	{'B', "broadcast",	FN_INT,		opt_set_bcast_mode,	1},
	{'E', "echo",		FN_NOPARAM,	opt_set_echo,		1},
	{'l', "localmount",	FN_NOPARAM | FN_STRING,
						opt_set_use_local_mnt_dir, 0},
	{0,   NULL,		0,		NULL,			0}
};

static int make_unique_dir(struct ncp_mount_info* info, char * mpnt, size_t maxl,
		const char* p1, const char* p2) {
	struct stat st;
	size_t ln;
	char* curr;
	const struct passwd* pwd;

	if (!info->server || !info->NWpath || !info->NWpath[0]) return -1;

	pwd = getpwuid(info->mdata.uid);
	if (!pwd) return -1;

	if (!p2) {
		p2 = pwd->pw_name;
	}
	if (!p1) {
		p1 = pwd->pw_dir;
	} else {
		if (stat(p1, &st)) {
			if (mkdir(p1, 0711)) {
				return -1;
			}
		}
	}

	curr = mpnt;

	ln = snprintf(curr, maxl, "%s/%s", p1, p2);
	if (ln >= maxl) {
		return E2BIG;
	}
	if (stat(mpnt, &st)) {
		if (mkdir(mpnt, 0700) || chown(mpnt, pwd->pw_uid, pwd->pw_gid)) {
			return -1;
		}
	}
	curr[ln] = '/';
	curr += ln + 1;
	maxl -= ln + 1;

	ln = strlen(info->server);
	if (ln >= maxl) {
		return E2BIG;
	}
	memcpy(curr, info->server, ln);
	curr[ln] = 0;
	str_upper(curr);
	curr += ln;
	maxl -= ln;
	if (stat(mpnt, &st)){
		if (mkdir(mpnt,0700) || chown(mpnt, pwd->pw_uid, pwd->pw_gid)) {
			return -1;
		}
	}
	ln = info->NWpath[1];	/*length of NW fragments*/
	if (ln + 1 >= maxl) {
		return E2BIG;
	}
	*curr++ = '/';
	memcpy(curr, info->NWpath + 2, ln);
	curr[ln] = 0;
	str_upper(curr);
	if (stat(mpnt, &st)) {
		if (mkdir(mpnt, 0700) || chown(mpnt, pwd->pw_uid, pwd->pw_gid)) {
			return -1;
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	struct ncp_mount_info info;
	struct stat st;
	char *mount_name;

	int result;

	long err;

	char ntree[MAX_TREE_NAME_CHARS + 1];

#ifndef NCPMAP
	struct ncp_conn_spec spec;
#endif
	NWDSContextHandle ctx=NULL;
	struct ncp_conn *conn;

	int opt;

#ifdef CONFIG_NATIVE_UNIX
	struct sockaddr_un server_un;
#endif
	int mount_ncp = 0;
#ifndef NCPMAP
	int opt_n = 0;
#endif
	int opt_v = 0;

	verify_argv(argc, argv);

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	init_mount_info(&info);

	if (myeuid != 0)
	{
		errexit(26, _("%s must be installed suid root\n"), progname);
	}

/*ncplogin,ncpmap */
#ifdef NCPMAP
	/*same as ncpmount :minus all users,passwd , uids, guids
			   :plus  -R -a -T -X*/

	while ((opt = getopt(argc, argv, "S:f:d:h?vV:t:r:o:"
					 "sN:y:p:i:A:B:T:X:R:aEl"
#else
	/*same as ncpmount :plus -T, -X
			   :minus -b -m
	 ncplogin does not want -R -a */
	while ((opt = getopt(argc, argv, "CS:U:c:u:g:f:d:P:nh?vV:t:r:o:"
					 "sN:y:p:i:A:B:T:X:El"

#endif
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
#ifndef NCPMAP
		case 'C':
#else
		case 'a':
#endif
		case 's':
		case 'E':
		case 'l':

			optarg = NULL;
#ifndef NCPMAP
		case 'U':
		case 'c':
		case 'u':
		case 'g':
#else
		case 'V':
		case 'R':
#endif
		case 'S':
		case 'T':
		case 'X':
		case 'f':
		case 'd':
		case 't':
		case 'r':
		case 'i':
		case 'A':
		case 'y':
		case 'p':
		case 'B':
			tmpopt[0] = opt;
			tmpopt[1] = 0;
			proc_option(opts, &info, tmpopt, optarg);
			break;
#ifndef NCPMAP
		case 'P':
			if (!mount_ncp) opt_n = 0;	/* clear -n (nopasswd) */
			tmpopt[0] = opt;
			tmpopt[1] = 0;
			proc_option(opts, &info, tmpopt, optarg);
			break;
		case 'n':
			opt_n = 1;	/* no passwd or no update /etc/fstab */
			break;
#endif
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
			eprintf(_("invalid option: %c\n"),opt);
			usage();
			return -1;
		}
	}

#ifndef NCPMAP
	if (opt_n && !mount_ncp) {
		opt_n = 0;
		proc_option(opts, &info, "passwd", "");
	}
#endif
	if (opt_v && !mount_ncp) {
		fprintf(stderr, _("ncpfs version %s\n"), NCPFS_VERSION);
		exit(30);
	}
	/*ncplogin -a not allowed.
	ncpmap only :added autocreate feature as LinuxNeighborood GUI utility for samba client*/
#ifdef NCPMAP
	if (optind != argc - 1){
		if (!info.autocreate) {
			usage();
			return -1;
		}
		/* we will autocreate later since NCPMAP may have to resolve NDS volume name!*/
	} else {
		realpath(argv[optind], mount_point);
		info.autocreate = 0; /*turn it off user has provided a mount point*/
	}
#else
	if (optind != argc) {
		usage();
		return -1;
	}
	info.autocreate = 1; /*forced in ncplogin */
#endif

	if (myuid != info.mdata.mounted_uid) {
		if (setreuid(info.mdata.mounted_uid, -1)) {
			errexit(76, _("Cannot impersonate as requested: %s\n"), strerror(errno));
		}
	}


	/* processing new arguments */
	/* FIX ME: so far I do not consider info.server_name (IP name) nor bindery only */
#ifdef NCPMAP
	if (!info.remote_path) {
		errexit(100, _("You must specify a volume to mount using -V option.\n"));
	}
#endif
	if (info.server && info.tree) {
		errexit(101, _("Cannot have both -T tree and -S server options\n"));
	}

	/* if a server has been specified, get its NDS status and tree */
	if (info.server) {
		err = NWCCOpenConnByName (NULL,info.server,NWCC_NAME_FORMAT_BIND,0,NWCC_RESERVED,&conn);
		if (err) {
			errexit(102, _("Unable to open connection to %s.\n"),info.server);
		}
		if (!NWCXIsDSServer(conn, ntree)) { /* we have removed trailing ---*/
			NWCCCloseConn(conn);
#ifdef NCPMAP
			errexit(103, _("%s is not a NDS server, so background authentication will fail.\n"), info.server);
#else
			errexit(103, _("%s is not a NDS server, so NDS authentication will fail.\n"), info.server);
#endif
		}
		NWCCCloseConn(conn);
		err = NWDSCreateContextHandleMnt(&ctx, ntree);
		if (err) {
			errexit(104, _("Cannot create NDS context handle: %s\n"), strerror(err));
		}
#ifdef NCPMAP
		if (!NWCXIsDSAuthenticated(ctx)) {
			NWDSFreeContext(ctx);
			errexit(105, _("Server %s belong to tree %s and you are not authenticated to it.\n"), info.server, ntree);
		}
#else
		if (NWCXIsDSAuthenticated(ctx)) {
			NWDSFreeContext(ctx);
			errexit(105, _("Server %s belong to tree %s and you are already authenticated to it.\n"),info.server, ntree);
		}
		/* with ncplogin use SYS as volume name */
		strcpy(info.resourceName, "SYS");
		info.remote_path=info.resourceName;
#endif
		info.tree = ntree;
		/*in that case the volume name must be in bindery format */
	} else {
		/* try to get a default tree from env variable */
		/* we are using the same env names as the Caldera client (hope they won't mind)
		#define PREFER_TREE_ENV "NWCLIENT_PREFERRED_TREE"
		#define PREFER_CTX_ENV "NWCLIENT_DEFAULT_NAME_CONTEXT"
		*/
		int flags = 0;

		if (!info.tree) {
			NWCXGetPreferredDSTree(ntree, sizeof(ntree));
			info.tree = ntree;
		}

		err = NWDSCreateContextHandleMnt (&ctx,info.tree);
		if (err) {
			errexit(104, _("Cannot create NDS context handle: %s\n"),strnwerror(err));
		}
#ifdef NCPMAP
		if (!NWCXIsDSAuthenticated(ctx)) {
			NWDSFreeContext(ctx);
			errexit(105, _("You are not authenticated to tree %s.\n"),info.tree);
		}
#else
		if (NWCXIsDSAuthenticated(ctx)) {
			NWDSFreeContext(ctx);
			errexit(105, _("You are already authenticated to tree %s.\n"),info.tree);
		}
#endif

		err = NWDSGetContext(ctx, DCK_FLAGS, &flags);
		if (!err) {
			flags |= DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES | DCV_DEREF_ALIASES;
			err = NWDSSetContext(ctx, DCK_FLAGS, &flags);
		}

		if (err) {
			NWDSFreeContext(ctx);
			errexit(110, _("NWDSGetContext/NWDSSetContext (DCK_FLAGS) failed: %s.\n"), strnwerror(err));
		}
		/* no -X argument */
		if (!info.context[0])
			NWCXGetDefaultNameContext(info.tree,info.context,sizeof(info.context));
		/* got one */
		if (info.context[0]) {
			err = NWDSSetContext(ctx, DCK_NAME_CONTEXT, info.context);
			if (err) {
				errexit(106, _("NWDSSetContext(DCK_NAME_CTX) failed: %s\n"), strnwerror(err));
			}
		}

#ifdef NCPMAP
		/*resolve NDS volume name */

		if ((err=NWCXGetNDSVolumeServerAndResourceName(ctx,info.remote_path,
				info.serverName,sizeof(info.serverName),
				info.resourceName,sizeof(info.resourceName)))) {
			NWDSFreeContext(ctx);
			errexit(105, _("Cannot resolve volume name %s on tree %s (using context %s). Err:%s\n"),
				info.remote_path,info.tree,info.context[0]?info.context:"[Root]",strnwerror(err));
		} else {
			err = NWCXSplitNameAndContext(ctx,info.serverName,info.serverName,NULL);
			info.server=info.serverName;
			info.remote_path=info.resourceName;
		}
#else
		/* get a server name from that tree
		   NWCXAttchToTreeByName do search for a default server in env
		 */

		if ((err=NWCXAttachToTreeByName (&conn,info.tree))) {
			NWDSFreeContext(ctx);
			errexit(108, _("Cannot attach to tree %s. Err:%s\n"),
				info.tree,strnwerror(err));
		}
		if ((err=NWCCGetConnInfo(conn,NWCC_INFO_SERVER_NAME,sizeof(info.serverName),info.serverName))){
			NWCCCloseConn(conn);
			NWDSFreeContext(ctx);
			errexit(109, _("Cannot get server name from connection to tree %s. Err:%s\n"),
				info.tree,strnwerror(err));
		}

		/* use SYS volume that should always exist */
		strcpy(info.resourceName, "SYS");
		info.server = info.serverName;
		info.remote_path = info.resourceName;
		NWCCCloseConn(conn);
#endif
	}
/* v 1.06 make search of  DefaultUserName (ncplogin/ncpmount  only) moved here so it is called with both -T or -S options*/
#ifndef NCPMAP
	if (!info.user) {
		if (!NWCXGetDefaultUserName(info.tree,info.defaultUser,sizeof(info.defaultUser))) {
			info.user = info.defaultUser;
		} else {
			NWDSFreeContext(ctx);
			errexit(110, _("No user name found in cmd line nor in env\n"));
		}
	}
#endif
	/* just now do what ncpmount.c used to do where parsing -V option */
	err=opt_set_volume_after_parsing_all_options(&info);
	if (err) {
		NWDSFreeContext(ctx);
		exit(err);
	}

	/*printf ("Ok we will mount %s:%s:%s in %s\n",info.server,info.remote_path,info.root_path,mount_point);*/

	/* try autocreation of mount_point must be done AFTER NCPMAP specific part cause it is
	   only now that server and volume are correct*/
	if (info.autocreate) {
		/* printf ("try autocreate %s\n","");*/

		if (info.use_local_mnt_dir) {
			err = make_unique_dir(&info, mount_point, sizeof(mount_point), info.use_local_mnt_dir, NULL);
		} else {
			err = make_unique_dir(&info, mount_point, sizeof(mount_point), NULL, "ncp");
		}
		if (err) {
			NWDSFreeContext(ctx);
			errexit(-1, _("Could not autocreate mount point %s: %s\n"),mount_point, strerror(err));
		}
	}

	/* final checking it's time to do it for all ! */
	if (stat(mount_point, &st) == -1)
	{
		err = errno;
		NWDSFreeContext(ctx);
		errexit(31, _("Could not find mount point %s: %s\n"),
			mount_point, strerror(err));
	}
	if (mount_ok(&st) != 0)
	{
		err = errno;
		NWDSFreeContext(ctx);
		errexit(32, _("Cannot mount on %s: %s\n"), mount_point, strerror(err));
	}
	/* ncplogin: now that we have the server NAME in bindery format we can do it */
#ifndef NCPMAP

	/* no user/pwd in ncpmap */
	get_passwd(&info);

	if ((err = ncp_find_conn_spec3(info.server, info.user, info.password, 1, info.mdata.uid, info.allow_multiple_connections, &spec))
	    != 0)
	{
		mycom_err(err, _("in find_conn_spec"));
		exit(33);
	}
	/* I am afraid it's too late. ncp_find_conn_spec3 did it */
	if (info.upcase_password != 0)
	{
		str_upper(spec.password);
	}

#endif

#ifndef NCPMAP
	info.mdata.server_name = spec.server;
#endif
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
			tran = NT_UDP;
			if (info.nt.valid && info.nt.value == NT_TCP) {
				tran = info.nt.value;
			}
			err = ncp_find_server_addr(&info.server_name, OT_FILE_SERVER, (struct sockaddr*)&info.mdata.serv_addr,
					sizeof(info.mdata.serv_addr), tran);
			if (err) {
				eprintf(_("Get host address `%s': %s\n"), info.server_name, strnwerror(err));
				NWDSFreeContext(ctx);
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
#ifndef NCPMAP	/*ncpmount & ncplogin */
		if ((err = ncp_find_fileserver(spec.server, (struct sockaddr*)&info.mdata.serv_addr, sizeof(info.mdata.serv_addr))) != 0)
		{
			mycom_err(err, _("when trying to find %s"), spec.server);
			NWDSFreeContext(ctx);
			exit(36);
		}
#else

		if ((err = ncp_find_fileserver(info.server, (struct sockaddr*)&info.mdata.serv_addr, sizeof(info.mdata.serv_addr))) != 0)
		{
			mycom_err(err, _("when trying to find %s"), info.server);
			NWDSFreeContext(ctx);
			exit(36);
		}
#endif
	}

	err = proc_buildconn(&info);
	if (err) {
		NWDSFreeContext(ctx);
		exit(err);
	}

	info.mdata.mount_point = mount_point;
#ifndef NCPMAP
	if (asprintf(&mount_name, "%s/%s", spec.server, spec.user) < 0) {
		NWDSFreeContext(ctx);
		errexit(85, _("Cannot allocate memory for mtab entry: %s\n"), strerror(ENOMEM));
	}
#else
	{
		NWDSChar user[MAX_DN_BYTES];

		err = NWDSWhoAmI(ctx, user);
		if (err) {
			NWDSFreeContext(ctx);
			errexit(51, _("Cannot retrieve user identity: %s\n"), strnwerror(err));
		}
		err = NWCXSplitNameAndContext(ctx, user, user, NULL);
		if (err) {
			NWDSFreeContext(ctx);
			errexit(86, _("Cannot parse user name: %s\n"), strnwerror(err));
		}
		/* FIXME: str_upper is unwanted! NWCXSplitNameAndContext too, BTW... Just retrieve name from server and do 'NWDSAbbreviateName' on it */
		str_upper(user);
		if (asprintf(&mount_name, "%s/%s", info.server, user) < 0) {
			NWDSFreeContext(ctx);
			errexit(85, _("Cannot allocate memory for mtab entry: %s\n"), strerror(ENOMEM));
		}
	}
#endif
	result = ncp_mount(mount_name, &info);
	if (result)
	{
		if (result == EBUSY) {
			/* for TCL/tk to catch it*/
			fprintf(stderr, _("already mounted:%s\n"), mount_point);
		} else {
			mycom_err(result, _("failed in mount(2)"));
		}
		free(mount_name);
		NWDSFreeContext(ctx);
		/*exit code stays 0 for TCL/tk to be fixed...*/
		exit(0);
	}
	err = proc_aftermount(&info, &conn);
	if (err) {
		free(mount_name);
		NWDSFreeContext(ctx);
		proc_ncpm_umount(mount_point);
		exit(err);
	}
#ifndef NCPMAP
#ifdef NDS_SUPPORT
	if (!info.force_bindery_login && NWIsDSServer(conn, NULL))
	{
		if ((err = nds_login_auth(conn, spec.user, spec.password)))
		{
			if (err != NWE_PASSWORD_EXPIRED) {
				mycom_err(err, _("failed in nds login"));
				fprintf(stderr, _("Login denied.\n"));
				ncp_close(conn);
				proc_ncpm_umount(mount_point);
				free(mount_name);
				NWDSFreeContext(ctx);
				exit(55);
			}
			fprintf(stderr, _("Your password has expired\n"));
		}
	}
	else
#endif
	{
		if ((err = ncp_login_user(conn, spec.user, spec.password)) != 0) {
			struct nw_property p;
			struct ncp_prop_login_control *l = (struct ncp_prop_login_control *) &p;

			if (err != NWE_PASSWORD_EXPIRED) {
				mycom_err(err, _("in login"));
				fprintf(stderr, _("Login denied\n"));
				ncp_close(conn);
				proc_ncpm_umount(mount_point);
				free(mount_name);
				NWDSFreeContext(ctx);
				exit(56);
			}
			fprintf(stderr, _("Your password has expired\n"));

			if ((err = ncp_read_property_value(conn, NCP_BINDERY_USER,
							   spec.user, 1,
							   "LOGIN_CONTROL", &p)) == 0) {
				fprintf(stderr, _("You have %d login attempts left\n"),
					l->GraceLogins);
			}
		}
	}
#else	/*ncpmap only*/
	err = NWDSAddConnection(ctx, conn);
	if (err) {
		ncp_close(conn);
		proc_ncpm_umount(mount_point);
		free(mount_name);
		NWDSFreeContext(ctx);
		errexit(110, _("Cannot attach connection to context: %s\n"), strnwerror(err));
	}
	err = NWDSAuthenticateConn(ctx, conn);
	if (err) {
		ncp_close(conn);
		proc_ncpm_umount(mount_point);
		free(mount_name);
		NWDSFreeContext(ctx);
		errexit(112, _("Cannot authenticate connection: %s\n"), strnwerror(err));
	}
#endif

	err = NWSetBroadcastMode(conn,info.bcastmode); /*ignore error for now */

	if ((err = ncp_mount_specific(conn, -1, info.NWpath, info.pathlen)) != 0)
	{
		ncp_close(conn);
		proc_ncpm_umount(mount_point);
		NWDSFreeContext(ctx);
		errexit(57, _("Cannot access path \"%s\": %s\n"), info.remote_path, strerror(-err));
	}
	NWCCCloseConn(conn);
	NWDSFreeContext(ctx);
	/* ncpmap, ncplogin must write in /etc/mtab */
	{
		block_sigs();
		add_mnt_entry(mount_name, mount_point, info.flags);
		unblock_sigs();
	}
	free(mount_name);
	if (info.echo_mnt_pnt) {
		printf(_("mounted on:%s\n"),mount_point);
	}
	return 0;
}

static void usage(void)
{
#ifdef NCPMAP
	printf(_("usage: %s [options] mount-point\n"), progname);
#else
	printf(_("usage: %s [options]\n"), progname);
#endif
	printf(_("Try `%s -h' for more information\n"), progname);
}

static void
help(void)
{
	const char* str;
	const char* str2;

#ifdef NCPMAP
	printf(_("\n"
		 "usage: %s [options] mount-point\n"), progname);
#else
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
#endif
#ifdef NCPMAP
	str = _(" mount_point is optional if -a option specified\n");
#else
	str = "";
#endif
	printf(_("\n"
		 "%s"
		 "-T tree        Tree name to be used\n"
		 "-S server      Server name to be used\n"
		 "                 these two options are exclusive\n"
		 "-X name_ctx    Default name context to be used\n"
		 "-E             Echo value of final mount_point\n")
		 , str);

#ifndef NCPMAP
	printf(_("\n"
		 "-U username    Username sent to server\n"
		 "-u uid         uid the mounted files get\n"
		 "-g gid         gid the mounted files get\n"
		 "-c uid         uid to identify the connection to mount on\n"
		 "               Only makes sense for root\n"
		 "-C             Don't convert password to uppercase\n"
		 "-P password    Use this password\n"
		 "-n             Do not use any password\n"
		 "               If neither -P nor -n are given, you are\n"
		 "               asked for a password.\n")
	);
#else
	printf(_("\n"
		 "-V volume      Volume to mount\n"
		 "                 must be in bindery format if -S server\n"
		 "                 or in NDS format if -T tree\n"
		 "-R path        Path in volume to 'map root'\n")
	);
#endif
#ifdef SIGNATURES
	str = _("-i level       Signature level, 0=never, 1=supported, 2=preferred, 3=required\n");
#else
	str = "";
#endif
#ifdef NCPMAP
	str2 = _("-a             Autocreate mounting point if needed in ~/ncp/SERVER/VOLUME\n");
#else
	str2 = "";
#endif
	printf(_("\n"
               "-f mode        permission the files get (octal notation)\n"
	       "-d mode        permission the dirs get (octal notation)\n"

	       "-t time_out    Waiting time (in 1/100s) to wait for\n"
	       "               an answer from the server. Default: 60\n"
	       "-r retry_count Number of retry attempts. Default: 5\n"

	       "-s             Enable renaming/deletion of read-only files\n"
	       "-h             print this help text\n"
	       "-v             print ncpfs version number\n"
	       "%s"
	       "-N os2,nfs     Do not use specified namespaces on mounted volume\n"
	       "-y charset     character set used for input and display\n"
	       "-p codepage    codepage used on volume, including letters `cp'\n"
               "-B bcast       Broadcast mode =0 all 1= console 2= none (default=all)\n"
               "%s"
	       "-l             Autocreate mounting point if needed in /mnt/ncp/SERVER/VOLUME\n"
	       "\n"),
                str, str2);
}

