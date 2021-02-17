/*
    ncpumount - Disconnects from permanent ncp connections

    Copyright (c) 1995 by Volker Lendecke
    Copyright (c) 2001 by Patrick Pollet
    Copyright (c) 2002 by Petr Vandrovec

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

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
				
	0.00(ncplogout)  2001, Feb 27	Patrick Pollet
		Initial revision.

        1.00(ncplogout)  2001, March 10	Patrick Pollet
                Added test for installation as suidroot
                Added emission of word "failed" in every error line
                  to make parsing easier from TCL/tk
                Corrected #218: testing memory after malloc()
                Corrected #238: NWCXIsSameServer() should not be called in theServer=NULL)
                Corrected #264: should not ignore error from NWCXGetPermConnInfo()
                Corrected #365: if -a, no -T nor -S allowed
                                 no -T and -S option
                Corrected #368: use a #define instead of 125 128 ...

	1.02  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Merged ncpumount and ncplogout, polished...
		/etc/mtab~ lock is now always removed when failure occurs with lock held.
		Use lockf() on /etc/mtab~ like util-linux do, and retry lock attempt couple
		of times.

 */

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/errno.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>

#include <mntent.h>
#include <pwd.h>

#include <sched.h>

#include "private/libintl.h"

#define _(X) X

#ifndef MS_REC
#define MS_REC 16384
#endif
#ifndef MS_SLAVE
#define MS_SLAVE (1<<19)
#endif

static char *progname;
static int is_ncplogout = 0;

uid_t uid;

static void
usage(void)
{
	printf(_("usage: %s [options]\n"), progname);
	printf(_("usage: %s mount-point\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"
		 "       %s mount-point\n"), progname, progname);
	printf(_("\n"
	         "mount-point    Disconnect specified mount-point\n"
                 "-a             Disconnect all my connections (NDS and Bindery)\n"
                 "-S servername  Disconnect all connections (NDS and/or Bindery) to specified\n"
		 "                       server\n"
                 "-T treename    Disconnect all NDS connections to specified tree\n"
                 "-g             Disconnect all connections to Netware for all users! (only\n"
                 "                       root can use this flag)\n"
	         "-h             Print this help text\n"
	         "\n"));
}

static void veprintf(const char* message, va_list ap) {
	if (is_ncplogout) {
		fprintf(stderr, "failed:");
	}
	vfprintf(stderr, message, ap);
}

static void eprintf(const char* message, ...) {
	va_list ap;
	
	va_start(ap, message);
	veprintf(message, ap);
	va_end(ap);
}

/* Mostly copied from ncpm_common.c */
void block_sigs(void) {

	sigset_t mask, orig_mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGALRM); /* Need SIGALRM for ncpumount */

	if(setresuid(0, 0, uid) < 0) {
		eprintf("Failed to raise privileges.\n");
		exit(-1);
	}

	if(sigprocmask(SIG_SETMASK, &mask, &orig_mask) < 0) {
		eprintf("Blocking signals failed.\n");
		exit(-1);
	}
}

void unblock_sigs(void) {

	sigset_t mask, orig_mask;
	sigemptyset(&mask);

	if(setresuid(uid, uid, 0) < 0) {
		eprintf("Failed to drop privileges.\n");
		exit(-1);
	}

	if(sigprocmask(SIG_SETMASK, &mask, &orig_mask) < 0) {
		eprintf("Un-blocking signals failed.\n");
		exit(-1);
	}
}

static void alarmSignal(int sig) {
	(void)sig;
}

static void enableAlarm(void) {
	struct sigaction sa;
	
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = alarmSignal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGALRM, &sa, NULL);
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL);
}

static int __clearMtab (const char* mount_points[], unsigned int numEntries) {
// main logic from ncpumount.c
	struct mntent *mnt;
	FILE *mtab;
	FILE *new_mtab;

#define MOUNTED_TMP MOUNTED".tmp"

	if ((mtab = setmntent(MOUNTED, "r")) == NULL){
		eprintf(_("Can't open %s: %s\n"), MOUNTED,
			strerror(errno));
		return 1;
	}

	if ((new_mtab = setmntent(MOUNTED_TMP, "w")) == NULL){
		eprintf(_("Can't open %s: %s\n"), MOUNTED_TMP,
			strerror(errno));
		endmntent(mtab);
		return 1;
	}
	while ((mnt = getmntent(mtab)) != NULL)	{
		unsigned int i=0;
		int found=0;

		while (i<numEntries && !found) {
			found=!strcmp(mnt->mnt_dir, mount_points[i]);
			i++;
		}
		if (!found) {
			addmntent(new_mtab, mnt);
		}
	}

	endmntent(mtab);

	if (fchmod(fileno(new_mtab), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0){
		eprintf(_("Error changing mode of %s: %s\n"),
			MOUNTED_TMP, strerror(errno));
		return 1;
	}
	endmntent(new_mtab);

	if (rename(MOUNTED_TMP, MOUNTED) < 0){
		eprintf(_("Cannot rename %s to %s: %s\n"),
			MOUNTED, MOUNTED_TMP, strerror(errno));
		return 1;
	}
	return 0;
}

static int clearMtab (const char* mount_points[], unsigned int numEntries) {
	int fd;
	int err;
	int retries = 10;

	if (!numEntries)
		return 0; /* don't waste time ! */

	block_sigs();

	while ((fd = open(MOUNTED "~", O_RDWR | O_CREAT | O_EXCL, 0600)) == -1) {
		struct timespec tm;

		if (errno != EEXIST || retries == 0) {
			unblock_sigs();
			eprintf(_("Can't get %s~ lock file: %s\n"), MOUNTED, strerror(errno));
			return 1;
		}
		fd = open(MOUNTED "~", O_RDWR);
		if (fd != -1) {
			alarm(10);
			err = lockf(fd, F_LOCK, 0);
			alarm(0);
			close(fd);
			if (err) {
				unblock_sigs();
				eprintf(_("Can't lock lock file %s~: %s\n"), MOUNTED, _("Lock timed out"));
				return 1;
			}
			tm.tv_sec = 0;
			tm.tv_nsec = 20000000;
			nanosleep(&tm, NULL);
		}
		retries--;
	}
	alarm(1);
	lockf(fd, F_LOCK, 0);
	alarm(0);
	close(fd);

	err = __clearMtab(mount_points, numEntries);

	if ((unlink(MOUNTED "~") == -1) && (err == 0)){
		unblock_sigs();
		eprintf(_("Can't remove %s~"), MOUNTED);
		return 1;
	}
	unblock_sigs();
	return err;
}


int ncp_mnt_umount(const char *abs_mnt, const char *rel_mnt)
{
	if (umount(rel_mnt) != 0) {
		eprintf(_("Could not umount %s: %s\n"),
			abs_mnt, strerror(errno));
		return -1;
	}
	return 0;
}


static int check_is_mount_child(void *p)
{
	const char **a = p;
	const char *last = a[0];
	const char *mnt = a[1];
	int res;
	const char *procmounts = "/proc/mounts";
	int found;
	FILE *fp;
	struct mntent *entp;

	res = mount("", "/", "", MS_SLAVE | MS_REC, NULL);
	if (res == -1) {
		eprintf(_("Failed to mark mounts slave: %s\n"),
			strerror(errno));
		return 1;
	}

	res = mount(".", "/tmp", "", MS_BIND | MS_REC, NULL);
	if (res == -1) {
		eprintf(_("Failed to bind parent to /tmp: %s\n"),
			strerror(errno));
		return 1;
	}

	fp = setmntent(procmounts, "r");
	if (fp == NULL) {
		eprintf(_("Failed to open %s: %s\n"),
			procmounts, strerror(errno));
		return 1;
	}

	found = 0;
	while ((entp = getmntent(fp)) != NULL) {
		if (strncmp(entp->mnt_dir, "/tmp/", 5) == 0 &&
		    strcmp(entp->mnt_dir + 5, last) == 0) {
			found = 1;
			break;
		}
	}
	endmntent(fp);

	if (!found) {
		eprintf(_("%s not mounted\n"), mnt);
		return 1;
	}

	return 0;
}


static int check_is_mount(const char *last, const char *mnt)
{
	char buf[131072];
	pid_t pid, p;
	int status;
	const char *a[2] = { last, mnt };

	pid = clone(check_is_mount_child, buf + 65536, CLONE_NEWNS, (void *) a);
	if (pid == (pid_t) -1) {
		eprintf(_("Failed to clone namespace: %s\n"),
			strerror(errno));
		return -1;
	}
	p = waitpid(pid, &status, __WCLONE);
	if (p == (pid_t) -1) {
		eprintf(_("Waitpid failed: %s\n"),
			strerror(errno));
		return -1;
	}
	if (!WIFEXITED(status)) {
		eprintf(_("Child terminated abnormally (status %i)\n"),
			status);
		return -1;
	}
	if (WEXITSTATUS(status) != 0)
		return -1;

	return 0;
}


static int chdir_to_parent(char *copy, const char **lastp, int *currdir_fd)
{
	char *tmp;
	const char *parent;
	char buf[PATH_MAX];
	int res;

	tmp = strrchr(copy, '/');
	if (tmp == NULL || tmp[1] == '\0') {
		eprintf(_("Internal error: invalid abs path: <%s>\n"),
			copy);
		return -1;
	}
	if (tmp != copy) {
		*tmp = '\0';
		parent = copy;
		*lastp = tmp + 1;
	} else if (tmp[1] != '\0') {
		*lastp = tmp + 1;
		parent = "/";
	} else {
		*lastp = ".";
		parent = "/";
	}
	*currdir_fd = open(".", O_RDONLY);
	if (*currdir_fd == -1) {
		eprintf(_("Failed to open current directory: %s\n"),
			strerror(errno));
		return -1;
	}
	res = chdir(parent);
	if (res == -1) {
		eprintf(_("Failed to chdir to %s: %s\n"),
			parent, strerror(errno));
		return -1;
	}
	if (getcwd(buf, sizeof(buf)) == NULL) {
		eprintf(_("Failed to obtain current directory: %s\n"),
			strerror(errno));
		return -1;
	}
	if (strcmp(buf, parent) != 0) {
		eprintf(_("Mountpoint moved (%s -> %s)\n"),
			parent, buf);
		return -1;

	}

	return 0;
}


static int unmount_ncp(const char *mount_point)
{
	int currdir_fd = -1;
	char *copy;
	const char *last;
	int res;

	copy = strdup(mount_point);
	if (copy == NULL) {
		eprintf(_("Failed to allocate memory\n"));
		return -1;
	}
	res = chdir_to_parent(copy, &last, &currdir_fd);
	if (res == -1)
		goto out;
	res = check_is_mount(last, mount_point);
	if (res == -1)
		goto out;
	res = ncp_mnt_umount(mount_point, last);

out:
	free(copy);
	if (currdir_fd != -1) {
		fchdir(currdir_fd);
		close(currdir_fd);
	}

	return res;
}

static int
do_umount(const char *mount_point)
{
	int fid = open(mount_point, O_RDONLY, 0);
	uid_t mount_uid;
	int res;

	if (fid == -1) {
		eprintf(_("Invalid or unauthorized mountpoint %s\n"),
			mount_point);
		return -1;
	}
	if (ncp_get_mount_uid(fid, &mount_uid) != 0) {
		close(fid);
		eprintf(_("Invalid or unauthorized mountpoint %s\n"),
			mount_point);
		return -1;
	}
	if ((getuid() != 0) && (mount_uid != getuid()))	{
		close(fid);
		eprintf(_("You are not allowed to umount %s\n"),
			mount_point);
		return -1;
	}
	close(fid);
	res = unmount_ncp(mount_point);
	return res;
}





static NWCCODE processServer(NWCONN_HANDLE conn, const char *theServer,
			     const char* mount_points[], unsigned int * numEntries) {
/* close the connection if any error or just before trying do_umount().
   Caller does not have to do it */
	NWCCODE err;
	char buffer[PATH_MAX + 1];

	if (!conn) return 0; // already seen

	buffer[0]=0;
	err = NWCCGetConnInfo(conn,NWCC_INFO_MOUNT_POINT, sizeof(buffer), buffer);
	NWCCCloseConn(conn);
	if (err) {
		eprintf(_("NWCC_INFO_MOUNT_POINT failed: %s\n"), strnwerror(err));
		return -1;
	}
	/* Connection must be closed before doing do_umount, otherwise umount will fail */
	err = do_umount(buffer);
	if (!err) {
		char * aux=strdup(buffer);
		if (!aux) {
			eprintf(_("Not enough memory for strdup()\n"));
			return ENOMEM;
		}
		mount_points[(*numEntries)++]= aux;
		printf(_("Successfully logged out from %s\n"), theServer);
	}
	return err;
}

static NWCCODE processBindServers(NWCONN_HANDLE conns[], unsigned int curEntries,
				  const char * theServer,
				  const char* mount_points[], unsigned int * numEntries) {
	unsigned int i;

	for (i=0; i<curEntries; i++) {
		NWCONN_HANDLE theConn=conns[i];
		if (theConn) {
			processServer(theConn,theServer,mount_points,numEntries);
			/* NWCCCloseConn (theConn);
			   conn has already been closed just before umounting
			   It must be that way otherwise umount fails (device busy) */
			conns[i]=NULL; // don't see it again
		}
	}
	return 0;
}

static NWCCODE processTree(NWCONN_HANDLE conns[], unsigned int curEntries,
			   const char * theTree,
			   const char* mount_points[], unsigned int * numEntries){
/* cut off all connections to servers of that tree */
	char serverName[128];
	unsigned int i;
	int nbServers = 0;
	int nbFailures = 0;
	NWCCODE err = 0;

	for (i=0; i<curEntries; i++) {
		NWCONN_HANDLE theConn=conns[i];
		/* theTree NULL = all trees */
		if (theConn) {
			err= NWCCGetConnInfo(theConn, NWCC_INFO_SERVER_NAME, sizeof(serverName), serverName);
			if (!err) {
				if (!processServer(theConn, serverName, mount_points, numEntries))
					nbServers++;
				else
					nbFailures++;
			} else {
				NWCCCloseConn(theConn);
				nbFailures++;
			}
			/* conn has already been closed if error or just before umounting
			   It must be that way otherwise umount fails (device busy) */
			conns[i]=NULL;	/* don't see it again*/
		}
	}
	/*printf ("%d %d \n",nbServers,nbFailures);*/
	if (!nbFailures) {
		if (nbServers)
			if (theTree)
				printf(_("Successfully logged out from tree %s (%d server(s))\n"),theTree,nbServers);
			else
				printf(_("Successfully logged out from all trees.\n"));
		else
			if (theTree)
				eprintf(_("Not logged to tree %s\n"),theTree);
			else
				eprintf(_("Not logged in\n"));
	} else {
		if (theTree)
			eprintf(_("Unsuccessfully logged out from tree %s (%d server(s) Ok and %d failure(s))\n"),theTree,nbServers,nbFailures);
		else
			eprintf(_("Unsuccessfully logged out from all trees (%d server(s) Ok and %d failure(s))\n"),nbServers,nbFailures);
	}
	return err;
}

/* Make a canonical pathname from PATH.  Returns a freshly malloced string.
   It is up the *caller* to ensure that the PATH is sensible.  i.e.
   canonicalize ("/dev/fd0/.") returns "/dev/fd0" even though ``/dev/fd0/.''
   is not a legal pathname for ``/dev/fd0.''  Anything we cannot parse
   we return unmodified.   */
static inline const char* canonicalize(const char *path, char *buffer) {
	if (path && realpath(path, buffer))
		return buffer;
	return path;
}


static int old_ncpumount(int argc, char *argv[]) {
	int ret = 0;
	int idx;
	
	for (idx = 0; idx < argc; idx++) {
		char buffer[PATH_MAX + 1];
		const char* mount_point;

		mount_point = canonicalize(argv[idx], buffer);
		if (!mount_point) {
			fprintf(stderr, _("Invalid mount point: %s\n"), argv[idx]);
			ret = 1;
			continue;
		}
		if (do_umount(mount_point) != 0) {
			ret = 1;
			continue;
		}
		if (clearMtab(&mount_point, 1)) {
			ret = 1;
			continue;
		}
	}
	return ret;
}

int
main(int argc, char *argv[])
{
	long err;
	int opt;
	int allConns = 0;
	const char *serverName = NULL;
	const char *treeName = NULL;
	
	uid = getuid();

	progname = strrchr(argv[0], '/');
	if (progname) {
		progname++;
	} else {
		progname = argv[0];
	}
	if (!strcmp(progname, "ncplogout")) {
		is_ncplogout = 1;
	}
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	if (geteuid() != 0) {
		eprintf(_("%s must be installed suid root\n"), progname);
		exit(26);
	}

	enableAlarm();

	while ((opt = getopt(argc, argv, "h?ags:S:t:T:")) != EOF) {
		switch (opt) {
			case 'h':
			case '?':
				help();
				return 1;
			case 'a':
				allConns = 1;
				break;
			case 'g':
				if (uid == 0) {
					allConns = 1;
					uid = (uid_t)-1;
				} else {
					eprintf(_("Only super-user can use the -g flag\n"));
					exit(2);
				}
				break;
			case 't':
			case 'T':
				treeName = optarg;
				break;
			case 's':
			case 'S':
				serverName= optarg;
				break;
			default:
				usage();
				return 1;
		}
	}

	if (optind != argc) {
		if (!serverName && !treeName && !allConns) {
			return old_ncpumount(argc - optind, argv + optind);
		}
		usage();
		return 1;
	}
	if (!serverName && ! treeName && !allConns) {
		eprintf(_("You must specify either a server, a tree, a mount point, or all connections.\n"));
		exit(2);
	}
	if (allConns && (serverName || treeName)) {
		eprintf(_("You cannot specify a server or a tree with -a or -g option.\n"));
		exit(2);
	}
	if ((serverName && treeName)) {
		eprintf(_("You cannot specify both a server and a tree together.\n"));
		exit(2);
	}


	{
#define NUM_CONNS 128
		NWCONN_HANDLE conns[NUM_CONNS];
		int maxEntries=NUM_CONNS;
		int curEntries=0;
		const char * umountTable[NUM_CONNS];
		unsigned int mountEntries=0;

		if (treeName)
			err = NWCXGetPermConnListByTreeName(conns, maxEntries, &curEntries, uid, treeName);
		else if (serverName)
			err = NWCXGetPermConnListByServerName(conns, maxEntries, &curEntries, uid, serverName);
		else
			err = NWCXGetPermConnList(conns, maxEntries, &curEntries, uid);

		if (err) {
			eprintf(_("NWCXGetPermConnList failed: %s\n"), strnwerror(err));
			return err;
		}

		/*printf ("got %d \n",curEntries);*/
		if (curEntries) {
			/*printf ("----- NDS connections -----\n");*/
			if (treeName || allConns) {
				processTree(conns,curEntries,treeName,umountTable,&mountEntries);
			}
			/*printf ("----- Bindery connections -----\n");*/
			if (serverName || allConns) {
				processBindServers(conns,curEntries,serverName,umountTable,&mountEntries);
			}
			clearMtab(umountTable, mountEntries);
		} else if (treeName) {
			eprintf(_("No NCP connections to tree %s.\n"),treeName);
		} else if (serverName) {
			eprintf(_("No NCP connections to server %s.\n"),serverName);
		} else {
			eprintf(_("No NCP connections.\n"));
		}
	}
	return 0;
}
