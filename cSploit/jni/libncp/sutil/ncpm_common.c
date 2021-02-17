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

 */

#include "ncpm_common.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
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
#if MOUNT3
#include <utmp.h>
#include <syslog.h>
#endif
#ifdef CONFIG_NATIVE_UNIX
#include <linux/un.h>
#endif

#include <ncp/kernel/ncp.h>
#include <ncp/kernel/ncp_fs.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <sys/utsname.h>

#include "private/libintl.h"
#define _(X) gettext(X)

#include "../lib/ncpi.h"

#ifndef NR_OPEN
/* getrlimit... */
#define NR_OPEN 1024
#endif

#ifndef MS_MANDLOCK
#define MS_MANDLOCK	64
#endif
#ifndef MS_NOATIME
#define MS_NOATIME	1024
#endif
#ifndef MS_NODIRATIME
#define MS_NODIRATIME	2048
#endif

#include "ncpmount.h"
#include "mount_login.h"

#define ncp_array_size(x) (sizeof(x)/sizeof((x)[0]))

void eprintf(const char* message, ...) {
	va_list ap;
	
	va_start(ap, message);
	veprintf(message, ap);
	va_end(ap);
}

void errexit(int ecode, const char* message, ...) {
	va_list ap;
	
	va_start(ap, message);
	veprintf(message, ap);
	va_end(ap);
	exit(ecode);
}

void mycom_err(int err, const char* message, ...) {
	eprintf("%s: %s ", progname, strnwerror(err));
	if (message) {
		va_list ap;
		
		va_start(ap, message);
		vfprintf(stderr, message, ap);
		va_end(ap);
	}
	fprintf(stderr, "\n");
}

#ifdef MOUNT2
/* Returns 0 if the filesystem is in the kernel after this routine
   completes */
static int load_ncpfs(void)
{
	FILE *fs;
	char s[1024];
	char *p, *p1;
	pid_t pid;
	int status;

	/* Check if ncpfs is in the kernel */
	fs = fopen("/proc/filesystems", "r");

	if (fs == NULL)
	{
		perror(_("Error: \"/proc/filesystems\" could not be read:"));
		return -1;
	}
	p = NULL;
	while (!feof(fs))
	{
		p1 = fgets(s, sizeof(s), fs);
		if (p1)
		{
			p = strstr(s, "ncpfs");
			if (p)
			{
				break;
			}
		}
	}
	fclose(fs);

	if (p)
	{
		return 0;
	}
	/* system() function without signal handling, from Stevens */

	if ((pid = fork()) < 0)
	{
		return 1;
	} else if (pid == 0)
	{
		char *myenv[] = {
			"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
			NULL
		};
		/* child */
		execle("/sbin/modprobe", "modprobe", "ncpfs", NULL, myenv);
		_exit(127);	/* execl error */
	} else
	{
		/* parent */
		while (waitpid(pid, &status, 0) < 0)
		{
			if (errno != EINTR)
			{
				status = -1;
				break;
			}
		}
	}
	return status;
}
#endif	/* MOUNT2 */

static int getmountver(void) {
	struct utsname name;
	int maj, mid, rev;
	int ver;

	if (uname(&name)) {
		errexit(1, _("Cannot get kernel release\n"));
	}
	if (sscanf(name.release, "%d.%d.%d", &maj, &mid, &rev) != 3) {
		errexit(2, _("Cannot convert kernel release \"%s\" to number\n"), name.release);
	}
	ver = maj*0x10000 + mid*0x100 + rev;
	if (ver < 0x20100)
		return 2;
	if (ver < 0x20328)
		return 3;
	if (ver < 0x2051F)
		return 4;
	return 5;
}

/* Check whether user is allowed to mount on the specified mount point */
int mount_ok(struct stat *st)
{
	if (!S_ISDIR(st->st_mode))
	{
		errno = ENOTDIR;
		return -1;
	}
	if (suser()) {
		return 0;
	}
	if ((myuid != st->st_uid)
		|| ((st->st_mode & S_IRWXU) != S_IRWXU))
	{
		errno = EPERM;
		return -1;
	}
	return 0;
}

#ifdef MOUNT3
/*
 * This function changes the processes name as shown by the system.
 * Stolen from Marcin Dalecki's modald :-)
 */
struct {
	int	argc;
	char	**argv;
	struct {
		size_t len;
		char*  base;
	} arg, env;
} iv;

static void inststr(const char *src)
{
	/* stolen from the source to perl 4.036 (assigning to $0) */
	int count;

	strncpy(iv.arg.base, src, iv.arg.len);
	for (count = 1; count < iv.argc; count++) {
		iv.argv[count] = NULL;
	}
}

void verify_argv(int argc, char* argv[]) {
	char *ptr;
	int count;

	progname = argv[0];
	ptr = strrchr(progname, '/');
	if (ptr) {
		progname = ptr + 1;
	}

	iv.argv = argv;
	iv.argc = argc;

	ptr = argv[0] + strlen(argv[0]);
	for (count = 1; count < argc; count++) {
		if (argv[count] == ptr + 1) {
			ptr++;
			ptr += strlen(ptr);
		}
	}
	iv.arg.base = argv[0];
	iv.arg.len = ptr - argv[0];

	ptr = environ[0];
	if (ptr) {
		ptr += strlen(ptr);
		for (count = 1; environ[count]; count++) {
			if (environ[count] == ptr + 1) {
				ptr++;
				ptr += strlen(ptr);
			}
		}
	}
	iv.env.base = environ[0];
	iv.env.len = ptr - environ[0];
}

#else

void verify_argv(int argc, char* argv[]) {
	char *ptr;

	progname = argv[0];
	ptr = strrchr(progname, '/');
	if (ptr) {
		progname = ptr + 1;
	}
}

#endif 

static inline int ncpm_suser(void) {
	return setresuid(0, 0, myuid);
}

static int ncpm_normal(void) {
	int e;
	int v;

	e = errno;
	v = setresuid(myuid, myuid, 0);
	errno = e;
	return v;
}

void block_sigs(void) {

	sigset_t mask, orig_mask;
	sigfillset(&mask);

	if(sigprocmask(SIG_SETMASK, &mask, &orig_mask) < 0) {
		errexit(-1, _("Blocking signals failed.\n"));
	}
}

void unblock_sigs(void) {

	sigset_t mask, orig_mask;
	sigemptyset(&mask);

	if (sigprocmask(SIG_SETMASK, &mask, &orig_mask) < 0) {
		errexit(-1, _("Un-blocking signals failed.\n"));
	}
}

static int proc_ncpm_mount(const char* source, const char* target, const char* filesystem, unsigned long mountflags, const void* data) {
	int v;
	int e;

	if (ncpm_suser()) {
		return errno;
	}
	v = mount(source, target, filesystem, mountflags, data);
	if (ncpm_normal()) {
		/* We cannot handle this situation gracefully, so do what we can */

		e = errno;
		/* If mount suceeded, undo it */
		if (v) {
			umount(target);
		}
		errexit(88, _("Cannot relinquish superuser rights: %s\n"), strerror(e));
	}
	return v;
}

int proc_ncpm_umount(const char* target) {
	int v;

	if (ncpm_suser()) {
		return errno;
	}
	v = umount(target);
	if (ncpm_normal()) {
		errexit(89, _("Cannot relinquish superuser rights: %s\n"), strerror(errno));
	}
	return v;
}

#ifdef MOUNT2
static int ncp_mount_v2(const char* mount_name, unsigned long flags, const struct ncp_mount_data_independent* data) {
	struct ncp_mount_data_v2 datav2;
	int err;

	if (data->serv_addr.sipx_family != AF_IPX) {
		return EPROTONOSUPPORT;
	}
	datav2.version    = NCP_MOUNT_VERSION_V2;
	datav2.ncp_fd     = data->ncp_fd;
	datav2.wdog_fd    = data->wdog_fd;
	datav2.message_fd = data->message_fd;
	datav2.mounted_uid = data->mounted_uid;
	memcpy(&datav2.serv_addr, &data->serv_addr, sizeof(datav2.serv_addr));
	strncpy(datav2.server_name, data->server_name, sizeof(datav2.server_name));
	strncpy(datav2.mount_point, data->mount_point, sizeof(datav2.mount_point));
	strncpy(datav2.mounted_vol, data->mounted_vol, sizeof(datav2.mounted_vol));
	datav2.time_out    = data->time_out;
	datav2.retry_count = data->retry_count;
	datav2.flags       = 0;
	datav2.flags      |= data->flags.mount_soft?NCP_MOUNT2_SOFT:0;
	datav2.flags      |= data->flags.mount_intr?NCP_MOUNT2_INTR:0;
	datav2.flags      |= data->flags.mount_strong?NCP_MOUNT2_STRONG:0;
	datav2.flags      |= data->flags.mount_no_os2?NCP_MOUNT2_NO_OS2:0;
	datav2.flags      |= data->flags.mount_no_nfs?NCP_MOUNT2_NO_NFS:0;
	if (data->flags.mount_extras || data->flags.mount_symlinks || data->flags.mount_nfs_extras) {
		return EINVAL;
	}
	datav2.uid         = data->uid;
	datav2.gid         = data->gid;
	if ((datav2.mounted_uid != data->mounted_uid) ||
	    (datav2.uid         != data->uid) ||
	    (datav2.gid         != data->gid)) {
	    	return EINVAL;
	}
        datav2.file_mode   = data->file_mode;
        datav2.dir_mode    = data->dir_mode;
	err = proc_ncpm_mount(mount_name, ".", "ncpfs", flags, (void*) &datav2);
	if (err)
		return errno;
	return 0;
}
#endif

#ifdef MOUNT3
static int 
process_connection (const struct ncp_mount_data_independent* mnt, int pipe_fd);

static void notify_slave(int info_fd, int version) {
	unsigned char p[12];
	
	DSET_HL(p, 0, 12);
	DSET_HL(p, 4,  1);
	DSET_HL(p, 8, version);
	write(info_fd, p, 12);
}

static int ncp_mount_v3(const char* mount_name, unsigned long flags, const struct ncp_mount_data_independent* data) {
	int pp[2];
	struct ncp_mount_data_v3 datav3;
	int err;

	datav3.version	   = NCP_MOUNT_VERSION_V3;
	datav3.ncp_fd	   = data->ncp_fd;
	datav3.mounted_uid = data->mounted_uid;
	strncpy(datav3.mounted_vol, data->mounted_vol, sizeof(datav3.mounted_vol));
	datav3.time_out    = data->time_out;
	datav3.retry_count = data->retry_count;
	datav3.flags       = 0;
	datav3.flags      |= data->flags.mount_soft?NCP_MOUNT3_SOFT:0;
	datav3.flags      |= data->flags.mount_intr?NCP_MOUNT3_INTR:0;
	datav3.flags      |= data->flags.mount_strong?NCP_MOUNT3_STRONG:0;
	datav3.flags      |= data->flags.mount_no_os2?NCP_MOUNT3_NO_OS2:0;
	datav3.flags      |= data->flags.mount_no_nfs?NCP_MOUNT3_NO_NFS:0;
	datav3.flags	  |= data->flags.mount_extras?NCP_MOUNT3_EXTRAS:0;
	datav3.flags	  |= data->flags.mount_symlinks?NCP_MOUNT3_SYMLINKS:0;
	datav3.flags	  |= data->flags.mount_nfs_extras?NCP_MOUNT3_NFS_EXTRAS:0;
	datav3.uid         = data->uid;
	datav3.gid         = data->gid;
        datav3.file_mode   = data->file_mode;
        datav3.dir_mode    = data->dir_mode;
	if ((datav3.mounted_uid != data->mounted_uid) ||
	    (datav3.uid         != data->uid) ||
	    (datav3.gid         != data->gid)) {
	    	return EINVAL;
	}
	connect(datav3.ncp_fd, (const struct sockaddr *)&data->serv_addr, sizeof(data->serv_addr));
	if (pipe(pp)) {
		errexit(64, _("Could not create pipe: %s\n"), strerror(errno));
	}
	datav3.wdog_pid = fork();
	if (datav3.wdog_pid < 0) {
		errexit(5, _("Could not fork: %s\n"), strerror(errno));
	}
	if (datav3.wdog_pid == 0) {
		/* Child */
		close(pp[1]);
		process_connection(data, pp[0]);
		exit(0);	/* Should not return from process_connection */
	}
	close(pp[0]);
	err=proc_ncpm_mount(mount_name, ".", "ncpfs", flags, (void*) &datav3);
	if (err) {
		err = errno;
		/* Mount unsuccesful so we have to kill daemon */
		/* If mount succeeds, kernel kills daemon      */
		close(pp[1]);
		kill(datav3.wdog_pid, SIGTERM);
		return err;
	}
	notify_slave(pp[1], NCP_MOUNT_VERSION_V3);
	close(pp[1]);
	return 0;
}

static int ncp_mount_v4(const char* mount_name, unsigned long flags, struct ncp_mount_data_independent* data, int trytext) {
	int pp[2];
	int err;
	unsigned int ncpflags;
	int wdog_pid;
	char mountopts[1024];

	ncpflags  = 0;
	ncpflags |= data->flags.mount_soft?NCP_MOUNT3_SOFT:0;
	ncpflags |= data->flags.mount_intr?NCP_MOUNT3_INTR:0;
	ncpflags |= data->flags.mount_strong?NCP_MOUNT3_STRONG:0;
	ncpflags |= data->flags.mount_no_os2?NCP_MOUNT3_NO_OS2:0;
	ncpflags |= data->flags.mount_no_nfs?NCP_MOUNT3_NO_NFS:0;
	ncpflags |= data->flags.mount_extras?NCP_MOUNT3_EXTRAS:0;
	ncpflags |= data->flags.mount_symlinks?NCP_MOUNT3_SYMLINKS:0;
	ncpflags |= data->flags.mount_nfs_extras?NCP_MOUNT3_NFS_EXTRAS:0;

	if (socketpair(PF_UNIX, SOCK_STREAM, 0, pp)) {
		errexit(64, _("Could not create pipe: %s\n"), strerror(errno));
	}

	connect(data->ncp_fd, (const struct sockaddr *)&data->serv_addr, sizeof(data->serv_addr));
	wdog_pid = fork();
	if (wdog_pid < 0) {
		errexit(6, _("Could not fork: %s\n"), strerror(errno));
	}
	if (wdog_pid == 0) {
		/* Child */
		close(pp[1]);
		process_connection(data, pp[0]);
		exit(0);	/* Should not return from process_connection */
	}
	close(pp[0]);
	if (trytext) {
		sprintf(mountopts, "version=%u,flags=%u,owner=%u,uid=%u,gid=%u,mode=%u,dirmode=%u,timeout=%u,retry=%u,wdogpid=%u,ncpfd=%u,infofd=%u",
			NCP_MOUNT_VERSION_V5, ncpflags, data->mounted_uid, data->uid, data->gid, data->file_mode,
			data->dir_mode, data->time_out, data->retry_count, wdog_pid, data->ncp_fd, pp[1]);
		err=proc_ncpm_mount(mount_name, ".", "ncpfs", flags, mountopts);
	} else {
		err=-1;
	}
	if (err) {
		struct ncp_mount_data_v4 datav4;

		datav4.version	   = NCP_MOUNT_VERSION_V4;
		datav4.ncp_fd	   = data->ncp_fd;
		datav4.mounted_uid = data->mounted_uid;
		datav4.time_out    = data->time_out;
		datav4.retry_count = data->retry_count;
		datav4.flags       = ncpflags;
		datav4.uid         = data->uid;
		datav4.gid         = data->gid;
	        datav4.file_mode   = data->file_mode;
        	datav4.dir_mode    = data->dir_mode;
		datav4.wdog_pid	   = wdog_pid;
		err = proc_ncpm_mount(mount_name, ".", "ncpfs", flags, (void*)&datav4);
		if (err) {
			err = errno;
			/* Mount unsuccesful so we have to kill daemon */
			/* If mount succeeds, kernel kills daemon      */
			close(pp[1]);
			kill(wdog_pid, SIGTERM);
			return err;
		}
		notify_slave(pp[1], NCP_MOUNT_VERSION_V4);
	} else {
		notify_slave(pp[1], NCP_MOUNT_VERSION_V5);
	}
	close(pp[1]);
	return 0;
}
#endif

void init_mount_info(struct ncp_mount_info *info) {
	mode_t um;

	myuid = getuid();
	myeuid = geteuid();

	if (myeuid == 0) {
		if (setreuid(-1, myuid)) {
			errexit(87, _("Cannot relinquish superuser rights: %s\n"), strerror(errno));
		}
	}

	memset(info, 0, sizeof(*info));

	info->version = -1;
	info->flags = MS_MGC_VAL;
	info->sig_level = -1;
	info->remote_path = "/";
	info->dentry_ttl = 0;
	info->NWpath[0] = 0;
	info->pathlen = 1;
	info->passwd_fd = ~0;

	info->mdata.mounted_uid = myuid;
	info->mdata.uid = myuid;
	info->mdata.gid = getgid();
	um = umask(0);
	umask(um);
	info->mdata.file_mode = (S_IRWXU | S_IRWXG | S_IRWXO) & ~um;
	info->mdata.dir_mode = 0;
	info->mdata.flags.mount_soft = 1;
	info->mdata.flags.mount_intr = 0;
	info->mdata.flags.mount_strong = 0;
	info->mdata.flags.mount_no_os2 = 0;
	info->mdata.flags.mount_no_nfs = 0;
	info->mdata.flags.mount_extras = 0;
	info->mdata.flags.mount_symlinks = 0;
	info->mdata.time_out = 60;
	info->mdata.retry_count = 5;
	info->mdata.mounted_vol = "";
	info->upcase_password = 1;
	info->bcastmode = NWCC_BCAST_PERMIT_ALL; /* all broadcasts by default */
	
	if (!suser()) {
		info->flags |= MS_NODEV | MS_NOEXEC | MS_NOSUID;
	}
}

int ncp_mount(const char* mount_name, struct ncp_mount_info* info) {
	if (info->version < 0) {
		info->version = getmountver();
	}
	switch (info->version) {
#ifdef MOUNT2
		case 2:
			/* Check if the ncpfs filesystem is in the kernel.  If not, attempt
			 * to load the ncpfs module */
			if (load_ncpfs()) {
				return ENXIO;
			}
			return ncp_mount_v2(mount_name, info->flags, &info->mdata);
#endif /* MOUNT2 */
#ifdef MOUNT3
		case 3:
			return ncp_mount_v3(mount_name, info->flags, &info->mdata);
		case 4:
			return ncp_mount_v4(mount_name, info->flags, &info->mdata, 0);
		case 5:
			return ncp_mount_v4(mount_name, info->flags, &info->mdata, 1);
#endif /* MOUNT3 */
		default:
			errexit(50, _("Unsupported mount protocol version %d\n"), info->version);
	}
}

int
ncp_mount_specific(struct ncp_conn* conn, int pathNS, const unsigned char* NWpath, int pathlen) {
	int result;

#ifdef NCP_IOC_SETROOT
	{
		struct ncp_setroot_ioctl sr;

		if (pathlen == 1) {
			sr.volNumber = -1;
			sr.name_space = -1;
			sr.dirEntNum = 0;
		} else {
			struct nw_info_struct2 dirinfo;

			if (pathNS < 0) {
				static const unsigned int nspcs[] = { NW_NS_NFS, NW_NS_LONG, NW_NS_DOS };
				unsigned int i = 0;
				
				do {
					result = ncp_ns_obtain_entry_info(conn, nspcs[i], SA_ALL, NCP_DIRSTYLE_NOHANDLE,
							0, 0, NWpath, pathlen, NW_NS_DOS, RIM_ALL,
							&dirinfo, sizeof(dirinfo));
				} while (result && ++i < ncp_array_size(nspcs));
			} else {
				result = ncp_ns_obtain_entry_info(conn, pathNS, SA_ALL, NCP_DIRSTYLE_NOHANDLE, 0, 0,
							NWpath, pathlen, NW_NS_DOS, RIM_ALL, &dirinfo, sizeof(dirinfo));
			}
			if (result) {
				return -ENOENT;
			}
			if (!(dirinfo.Attributes.Attributes & A_DIRECTORY)) {
				return -ENOTDIR;
			}
			sr.volNumber = dirinfo.Directory.volNumber;
			sr.name_space = NW_NS_DOS;
			sr.dirEntNum = DVAL_LH(&dirinfo.Directory.dirEntNum, 0);
		}
		if (ncpm_suser()) {
			return -errno;
		}
		result = ioctl(ncp_get_fid(conn), NCP_IOC_SETROOT, &sr);
		if (ncpm_normal()) {
			/* Just continue, otherwise we cannot unmount directory */
			return result ? -errno : -EPERM;
		}
		if (!result) {
			return 0;
		}
	}
#endif
	if ((pathlen != 1) && (*NWpath != 1)) {
#ifdef NCP_IOC_SETROOT
		eprintf(_("Remote directory is specified but kernel"
			  " does not support subdir mounts\n"));
#else
		eprintf(_("Remote directory is specified but ncpmount"
			  " does not support subdir mounts\n"));
#endif
		return -ENOPKG;
	}
	if (ncpm_suser()) {
		return -errno;
	}
	result = ioctl(ncp_get_fid(conn), NCP_IOC_CONN_LOGGED_IN, NULL);
	if (ncpm_normal()) {
		/* Just continue, otherwise we cannot umount directory */
		return result ? -errno : -EPERM;
	}
	if (result != 0) {
		return -errno;
	}
	return 0;
}

#ifdef MOUNT3
static void
msg_received (void)
{
	struct ncp_conn *conn;
	int err;

	err = fork();
	if (err > 0)
		return;
	if (err == 0)
		execl(NWMSG, NWMSG, mount_point, NULL);
	/* fork or exec failed; get rid of message */
	if (ncp_open_mount(mount_point, &conn) == 0) {
		char message[256];
		
		if (!ncp_get_broadcast_message(conn, message)) {
			/* drop message to wastebasket */
		}
		ncp_close(conn);
	}
	if (err == 0)
		exit(0);
}

static inline void confirm_wait(void) {
	NWCONN_HANDLE conn;
	
	if (ncp_open_mount(mount_point, &conn) == 0) {
		NWCancelWait(conn);
		ncp_close(conn);
	}
}

/* MSG_DONTWAIT defined and module can run only on 2.1.x kernel */
#if defined(MSG_DONTWAIT) && !defined(MOUNT2)
#define recvfrom_notm(fd,buf,ln,sender,addrlen) recvfrom(fd,buf,ln,MSG_DONTWAIT,sender,addrlen)
#else
static int recvfrom_notm(int fd, void* buf, size_t len, struct sockaddr* sender, socklen_t* addrlen) {
	int ret;
	int flg;

	flg = fcntl(fd, F_GETFL);
	if (flg == -1) return -1;
	fcntl(fd, F_SETFL, flg | O_NONBLOCK);
	ret = recvfrom(fd, buf, len, 0, sender, addrlen);
	fcntl(fd, F_SETFL, flg);
	return ret;
}
#endif /* MSG_DONTWAIT && !MOUNT2 */

static void 
process_msg_packet (int msg_fd)
{
	struct sockaddr_ipx sender;
	socklen_t addrlen = sizeof(sender);
	char buf[1024];

	if (recvfrom_notm(msg_fd, buf, sizeof(buf),
		     (struct sockaddr *) &sender, &addrlen) <= 0) {
		return;
	}
	msg_received();
}

static void 
process_wdog_packet (int wdog_fd)
{
	struct sockaddr_ipx sender;
	socklen_t addrlen = sizeof(sender);
	char buf[2];

	if (recvfrom_notm(wdog_fd, buf, sizeof(buf),
		  (struct sockaddr *) &sender, &addrlen) < (int)sizeof(buf)) {
		return;
	}
	if (buf[1] != '?') {
		return;
	}
	buf[1] = 'Y';
	sendto(wdog_fd, buf, 2, 0, (struct sockaddr *) &sender, addrlen);
}

static void reapChld(UNUSED(int dummy)) {
	signal(SIGCHLD, reapChld);
	/* reap all exited childrens... */
	while (wait4(-1, NULL, WNOHANG, NULL) > 0) ;
}

/* permanent connection globals */

static int nwcc_bcast_state = NWCC_BCAST_PERMIT_ALL;
static const char* nwcc_mount_point;
static NWCONN_NUM nwcc_connnum;
static int mount_version_rcvd;
static int mount_version;

static void ncp_ctl_cmd(unsigned int cmd, const unsigned char* data, size_t datalen) {
	switch (cmd) {
		case 0:
			if (datalen < 10)
				break;
			if (nwcc_connnum != (NWCONN_NUM)(BVAL(data, 3) | (BVAL(data, 5) << 8)))
				break;
			switch (WVAL_HL(data, 0)) {
				case 0xBBBB:
					switch (BVAL(data, 9)) {
						case 0x21:
							msg_received();
							return;
						case 0x4C:
							confirm_wait();
							return;
					}
					break;
			}
			break;
		case 1:
			if (datalen != 4)
				break;
			mount_version = DVAL_HL(data, 0);
			mount_version_rcvd = 1;
			return;
		default:
			return;
	}
	{
		char xxx[1024]; /* "cmd=XXXXXXXXXXX, len=XXXXXXXXX, data:" + 3x300 chars */
		char* p;

		sprintf(xxx, "cmd=%u, len=%u, data:", cmd, datalen);
		p = xxx + strlen(xxx);
		if (datalen > 300) {
			datalen = 300;
		}
		while (datalen--) {
			sprintf(p, " %02X", *data++);
			p += 3;
		}
		syslog(LOG_INFO, "%s", xxx);
	}
}

static int prepare_unixfd(struct sockaddr_un* unix_addr, size_t unix_len) {
	int f;
	int unix_fd;

	unix_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (unix_fd < 0)
		return -1;
	if (bind(unix_fd, (struct sockaddr*)unix_addr, unix_len))
		goto quit;
	if (listen(unix_fd, 5))
		goto quit;
	f = fcntl(unix_fd, F_GETFL);
	if (f == -1)
		goto quit;
	if (fcntl(unix_fd, F_SETFL, f | O_NONBLOCK))
		goto quit;
	return unix_fd;
quit:;
	close(unix_fd);
	return -1;	
}

struct ncp_ctl_client {
	int fd;
	int istate;
	unsigned char* rcvptr;
	size_t received;
	size_t toreceive;
	unsigned char hdr[4];
};

static void ncp_ctl_init(struct ncp_ctl_client* clnt, int fd) {
	clnt->fd = fd;
	clnt->rcvptr = clnt->hdr;
	clnt->received = 0;
	clnt->toreceive = 4;
	clnt->istate = 0;
}

static void ncp_ctl_read(struct ncp_ctl_client* clnt) {
	unsigned int len;
	int r;
			
	r = read(clnt->fd, clnt->rcvptr + clnt->received, clnt->toreceive - clnt->received);
	if (r == 0) {
		goto abort;
	}
	if (r < 0) {
		return;
	}
	clnt->received += r;
	if (clnt->received < clnt->toreceive) {
		return;
	}
	switch (clnt->istate) {
		case 0:
			len = DVAL_HL(clnt->hdr, 0);
			if (len < 8) {
				goto abort;
			}
			if (len > 4096) {
				goto abort;
			}
			len -= 4;
			clnt->rcvptr = malloc(len);
			if (!clnt->rcvptr) {
				goto abort;
			}
			clnt->received = 0;
			clnt->toreceive = len;
			clnt->istate = 1;
			break;
		case 1:
			ncp_ctl_cmd(DVAL_HL(clnt->rcvptr, 0), clnt->rcvptr + 4, clnt->toreceive - 4);
			free(clnt->rcvptr);
			clnt->rcvptr = clnt->hdr;
			clnt->received = 0;
			clnt->toreceive = 4;
			clnt->istate = 0;
			break;
	}
	return;
abort:;
	close(clnt->fd);
	clnt->fd = -1;
}

struct ncp_ext_client {
	struct ncp_ext_client* next;
	int fd;
	int istate;
	struct ucred peercred;
	unsigned char* rcvptr;
	size_t received;
	size_t toreceive;
	unsigned char hdr[16];
	unsigned char* sndptr;
	size_t sent;
	size_t tosend;
};

static struct ncp_ext_client* accept_new_conn(int fd) {
	struct ncp_ext_client* n;
	struct sockaddr_un sun;
	socklen_t sunlen;
	int newfd;
	int f;

	sunlen = sizeof(sun);
	newfd = accept(fd, (struct sockaddr*)&sun, &sunlen);
	if (newfd < 0)
		return NULL;
	f = fcntl(newfd, F_GETFL);
	if (f == -1)
		goto quit;
	if (fcntl(newfd, F_SETFL, f | O_NONBLOCK))
		goto quit;
	n = malloc(sizeof(*n));
	if (!n)
		goto quit;
	memset(n, 0, sizeof(*n));
	n->fd = newfd;
	sunlen = sizeof(n->peercred);
	if (getsockopt(newfd, SOL_SOCKET, SO_PEERCRED, &n->peercred, &sunlen))
		goto quitfree;
	if (sunlen != sizeof(n->peercred))
		goto quitfree;
	/* for now only root/suid can do that... */
	if (n->peercred.uid)
		goto quitfree;
	return n;
quitfree:;
	free(n);
quit:;
	close(newfd);
	return NULL;
}

static void ncp_ext_destroy(struct ncp_ext_client* clnt) {
	if (clnt->rcvptr && clnt->rcvptr != clnt->hdr)
		free(clnt->rcvptr);
	if (clnt->sndptr && clnt->sndptr != clnt->hdr)
		free(clnt->sndptr);
	close(clnt->fd);
}

static int ncp_ext_init(struct ncp_ext_client* clnt) {
	clnt->rcvptr = clnt->hdr;
	clnt->received = 0;
	clnt->toreceive = 16;
	clnt->istate = 0;
	return 0;
}

static int ncp_ext_send(struct ncp_ext_client* clnt) {
	if (clnt->sndptr && clnt->sndptr != clnt->hdr)
		free(clnt->sndptr);
	return ncp_ext_init(clnt);
}

static int getconn(NWCONN_HANDLE* cn) {
	return ncp_open_mount(nwcc_mount_point, cn);
}

static int ncp_ext_perform(struct ncp_ext_client* clnt, UNUSED(const unsigned char* data), UNUSED(size_t rqlen)) {
	NWCONN_HANDLE cn;
	int err;

	clnt->sndptr = clnt->hdr;
	clnt->tosend = 12;
	switch (DVAL_LH(clnt->rcvptr, 0)) {
		case NCPI_OP_GETVERSION:
			DSET_LH(clnt->hdr, 12,  1);	/* version 1 */
			clnt->tosend = 16;
			break;
		case NCPI_OP_GETBCASTSTATE:
			DSET_LH(clnt->hdr, 12,  nwcc_bcast_state);	/* state... */
			clnt->tosend = 16;
			break;
		case NCPI_OP_SETBCASTSTATE:
			{
				int op;

				if (clnt->toreceive < 8)
					return NCPE_SHORT;
				switch (op = DVAL_LH(clnt->rcvptr, 4)) {
					case NWCC_BCAST_PERMIT_ALL:
					case NWCC_BCAST_PERMIT_SYSTEM:
					case NWCC_BCAST_PERMIT_NONE:
						break;
					case NWCC_BCAST_PERMIT_POLL:
					default:
						return NCPE_BAD_VALUE;
				}
				err = getconn(&cn);
				if (err)
					return err;
				if (op == NWCC_BCAST_PERMIT_NONE || op == NWCC_BCAST_PERMIT_SYSTEM)
					err = __NWDisableBroadcasts(cn);
				else
					err = __NWEnableBroadcasts(cn);
				NWCCCloseConn(cn);
				if (err)
					return err;
				nwcc_bcast_state = op;
			}
			break;
		default:
			return NCPE_BAD_COMMAND;
	}
	DSET_LH(clnt->sndptr, 4, clnt->tosend);
	DSET_LH(clnt->sndptr, 8, 0);
	return 0;
}

static int ncp_ext_receive(struct ncp_ext_client* clnt) {
	switch (clnt->istate) {
		case 0:	{
				size_t ln;
				size_t rpln;
				unsigned char* p;
				int r;

				if (DVAL_LH(clnt->hdr, 0) != NCPI_REQUEST_SIGNATURE)
					return 1;	/* unknown flag, kill it */
				if (DVAL_LH(clnt->hdr, 8) != 1)
					return 1;	/* unknown version, kill it */
				rpln = DVAL_LH(clnt->hdr, 12);
				if (rpln < 12)
					return 1;
				ln = DVAL_LH(clnt->hdr, 4);
				if (ln < 20)
					return 1;
				if (ln > 4096)	/* FIX for largest possible command */
					return 1;
				p = (unsigned char*)malloc(ln - 16);
				if (!p)
					return 1;
				clnt->rcvptr = p;
				clnt->received = 0;
				clnt->toreceive = ln - 16;
				clnt->istate = 1;
				r = read(clnt->fd, clnt->rcvptr, clnt->toreceive);
				if (r == 0)
					return 1;
				if (r < 0)
					return 0;
				clnt->received += r;
				if (clnt->received < clnt->toreceive)
					return 0;
				if (clnt->received != clnt->toreceive)
					return 1;
			}
			/* FALLTHROUGH */
		case 1:	{
				int w;

				DSET_LH(clnt->hdr, 0, NCPI_REPLY_SIGNATURE);
				w = ncp_ext_perform(clnt, clnt->rcvptr, clnt->toreceive);
				if (w) {
					DSET_LH(clnt->hdr, 4, 12);
					DSET_LH(clnt->hdr, 8, NCPE_BAD_COMMAND);
					clnt->sndptr = clnt->hdr;
					clnt->tosend = 12;
				}
				if (clnt->rcvptr && clnt->rcvptr != clnt->hdr) {
					free(clnt->rcvptr);
					clnt->rcvptr = NULL;
				}
				clnt->sent = 0;
				w = write(clnt->fd, clnt->sndptr, clnt->tosend);
				if (w == 0)
					return 1;
				if (w < 0)
					return 0;
				clnt->sent += w;
				if (clnt->sent > clnt->tosend)
					return 1;
				if (clnt->sent == clnt->tosend)
					return ncp_ext_send(clnt);
				return 0;
			}
	}
	return 1;
}

static int 
process_connection (const struct ncp_mount_data_independent* mnt, int pipe_fd)
{
	int i;
	int result;
	int max;
	int ncp_fd = mnt->ncp_fd;
	int wdog_fd = mnt->wdog_fd;
	int msg_fd = mnt->message_fd;
	int unix_fd;
	size_t unix_len;
	struct sockaddr_un unix_addr;
	struct stat stb;
	struct timeval timeout;
	struct timeval* tmv = NULL;
	struct ncp_ext_client* clnts = NULL;
	struct ncp_ctl_client infoctl;

	chdir("/");
	setsid();
	inststr("ncpd");
	{
		sigset_t newsigset;
	
		signal(SIGCHLD, reapChld);
		signal(SIGTERM, SIG_DFL);
		signal(SIGPIPE, SIG_IGN);    /*added as per ncpmount 1.08 */
		sigemptyset(&newsigset);
		sigaddset(&newsigset, SIGTERM);
		sigaddset(&newsigset, SIGCHLD);
		sigprocmask(SIG_UNBLOCK, &newsigset, NULL);
	}

	for (i = 0; i < NR_OPEN; i++) {
		if ((i == ncp_fd) || (i == wdog_fd) || (i == msg_fd) || (i == pipe_fd)) {
			continue;
		}
		close(i);
	}
	/* Make something reasonable with descriptors 0, 1 and 2 */
	for (i = 0; i < 3; i++) {
		open("/dev/null", O_RDWR);
	}
	mount_version_rcvd = 0;
	ncp_ctl_init(&infoctl, pipe_fd);
	while (!mount_version_rcvd) {
		ncp_ctl_read(&infoctl);
		if (infoctl.fd < 0)
			return 1;
	}
	unix_fd = -1;
	if (!stat(mnt->mount_point, &stb)) {
		unix_len = offsetof(struct sockaddr_un, sun_path) + sprintf(unix_addr.sun_path, "%cncpfs.permanent.mount.%lu", 0, (unsigned long)stb.st_dev) + 1;
		unix_addr.sun_family = AF_UNIX;
		unix_fd = prepare_unixfd(&unix_addr, unix_len);
	}
	nwcc_mount_point = mnt->mount_point;
	
	{
		NWCONN_HANDLE conn;
		NWCCODE err;
	
		err = ncp_open_mount(mnt->mount_point, &conn);
		if (err)
			return 1;	/* Give up */
		err = NWGetConnectionNumber(conn, &nwcc_connnum);
		ncp_close(conn);
		if (err)
			return 1;	/* Give up */
	}
#if defined(CONFIG_NATIVE_IP) || defined(CONFIG_NATIVE_UNIX)
	if (mnt->nt_type == NT_UDP) {
		if (mount_version != 5) {
			timeout.tv_sec = 180;
			timeout.tv_usec = 0;
			tmv = &timeout;
			close(wdog_fd);
			close(msg_fd);
			wdog_fd = -1;
			msg_fd = -1;
		} else {
			close(wdog_fd);
			close(msg_fd);
			close(ncp_fd);
			wdog_fd = -1;
			msg_fd = -1;
			ncp_fd = -1;
		}
	} else if (mnt->nt_type == NT_TCP) {
		close(wdog_fd);
		close(msg_fd);
		close(ncp_fd);
		wdog_fd = -1;
		msg_fd = -1;
		ncp_fd = -1;
	} else
#endif /* CONFIG_NATIVE_IP || CONFIG_NATIVE_UNIX */
	{
		close(ncp_fd);
		ncp_fd = -1;
	}

	max = wdog_fd;
	if (max < msg_fd)
		max = msg_fd;
	if (max < unix_fd)
		max = unix_fd;
	if (max < infoctl.fd)
		max = infoctl.fd;
	max += 1;

	while (1) {
		fd_set rd;
		fd_set wr;
		struct ncp_ext_client* tmpc;
		struct ncp_ext_client** dtmpc;

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		if (wdog_fd >= 0)
			FD_SET(wdog_fd, &rd);
		if (msg_fd >= 0)
			FD_SET(msg_fd, &rd);
		if (unix_fd >= 0)
			FD_SET(unix_fd, &rd);
		if (infoctl.fd >= 0)
			FD_SET(infoctl.fd, &rd);
		for (tmpc = clnts; tmpc; tmpc = tmpc->next) {
			if (tmpc->toreceive != tmpc->received)
				FD_SET(tmpc->fd, &rd);
			if (tmpc->tosend != tmpc->sent)
				FD_SET(tmpc->fd, &wr);
		}
		if ((result = select(max, &rd, &wr, NULL, tmv)) == -1) {
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
				continue;
			exit(0);
		}
		if (wdog_fd >= 0 && FD_ISSET(wdog_fd, &rd)) {
			process_wdog_packet(wdog_fd);
		}
		if (msg_fd >= 0 && FD_ISSET(msg_fd, &rd)) {
			process_msg_packet(msg_fd);
		}
		if (unix_fd >= 0 && FD_ISSET(unix_fd, &rd)) {
			tmpc = accept_new_conn(unix_fd);
			if (tmpc) {
				if (ncp_ext_init(tmpc)) {
					ncp_ext_destroy(tmpc);
					free(tmpc);
				} else {
					tmpc->next = clnts;
					clnts = tmpc;
					if (max <= tmpc->fd)
						max = tmpc->fd + 1;
				}
			}
		}
		if (infoctl.fd >= 0 && FD_ISSET(infoctl.fd, &rd)) {
			ncp_ctl_read(&infoctl);
		}
		if (tmv && !tmv->tv_sec && !tmv->tv_usec) {
			unsigned char buf[]={0x3E, 0x3E, 0, 0, 0, 0, 0, 0, 0, 'Y'};
			
			buf[3] = buf[8] = nwcc_connnum&0xFF;
			buf[5] = buf[7] = (nwcc_connnum>>8)&0xFF;

			send(ncp_fd, buf, sizeof(buf), 0);
			tmv->tv_sec = 180;
		}
		dtmpc = &clnts;
		while (1) {
			int dead;

			tmpc = *dtmpc;
			if (!tmpc)
				break;
			dead = 0;
			if (FD_ISSET(tmpc->fd, &rd)) {
				int r;
				
				r = read(tmpc->fd, tmpc->rcvptr + tmpc->received, tmpc->toreceive);
				if (r == 0)
					dead = 1;
				else if (r > 0) {
					tmpc->received += r;
					if (tmpc->received > tmpc->toreceive)
						dead = 1;
					else if (tmpc->received == tmpc->toreceive) {
						dead = ncp_ext_receive(tmpc);
					}
				}
			}
			if (!dead && FD_ISSET(tmpc->fd, &wr)) {
				int w;
				
				w = write(tmpc->fd, tmpc->sndptr + tmpc->sent, tmpc->tosend);
				if (w == 0)
					dead = 1;
				else if (w > 0) {
					tmpc->sent += w;
					if (tmpc->sent > tmpc->tosend)
						dead = 1;
					else if (tmpc->sent == tmpc->tosend) {
						dead = ncp_ext_send(tmpc);
					}
				}
			}
			if (dead) {
				ncp_ext_destroy(tmpc);
				*dtmpc = tmpc->next;
				free(tmpc);
			} else {
				dtmpc = &tmpc->next;
			}
		}
	}
	return 0;
}
#endif /* MOUNT3 */

static int check_name(const char *name)
{
	char *s;
	for (s = "\n\t\\"; *s; s++) {
		if (strchr(name, *s)) {
			return -1;
		}
	}
	return 0;
}

static const struct smntflags {
	unsigned int	flag;
	const char*	name;
		} mntflags[] = {
       {MS_NOEXEC,	"noexec"},
       {MS_NOSUID,	"nosuid"},
       {MS_NODEV,	"nodev"},
       {MS_SYNCHRONOUS,	"sync"},
       {MS_MANDLOCK,	"mand"},
       {MS_NOATIME,	"noatime"},
       {MS_NODIRATIME,	"nodiratime"},
       {0,		NULL}};

void add_mnt_entry(char* mount_name, char* mpnt, unsigned long flags) {
	const struct smntflags* sf;
	char mnt_opts[80];
	char* p;
	struct mntent ment;
	int fd;
	FILE* mtab;

	if (check_name(mount_name) == -1 || check_name(mpnt) == -1)
		errexit(107, _("Illegal character in mount entry\n"));

	ment.mnt_fsname = mount_name;
	ment.mnt_dir = mpnt;
	ment.mnt_type = (char*)"ncpfs";
	ment.mnt_opts = mnt_opts;
	ment.mnt_freq = 0;
	ment.mnt_passno = 0;

	p = mnt_opts;
	*p++ = 'r';
	*p++ = (flags & MS_RDONLY)?'o':'w';
	for (sf = mntflags; sf->flag; sf++) {
		if (flags & sf->flag) {
			*p++ = ',';
			strcpy(p, sf->name);
			p += strlen(p);
		}
	}
	*p = 0;

	if (ncpm_suser()) {
		errexit(91, _("Cannot switch to superuser: %s\n"), strerror(errno));
	}
	if ((fd = open(MOUNTED "~", O_RDWR | O_CREAT | O_EXCL, 0600)) == -1)
	{
		errexit(58, _("Can't get %s~ lock file\n"), MOUNTED);
	}
	close(fd);

	if ((mtab = setmntent(MOUNTED, "a+")) == NULL)
	{
		errexit(59, _("Can't open %s\n"), MOUNTED);
	}
	if (addmntent(mtab, &ment) == 1)
	{
		errexit(60, _("Can't write mount entry\n"));
	}
	if (fchmod(fileno(mtab), 0644) == -1)
	{
		errexit(61, _("Can't set perms on %s\n"), MOUNTED);
	}
	endmntent(mtab);

	if (unlink(MOUNTED "~") == -1)
	{
		errexit(62, _("Can't remove %s~\n"), MOUNTED);
	}
	if (ncpm_normal()) {
		errexit(90, _("Cannot relinquish superuser rights: %s\n"), strerror(EPERM));
	}
}

static int __proc_option(const struct optinfo* opts, struct ncp_mount_info* info, const char* opt, const char* param) {
	const struct optinfo* optr;

	for (optr = opts; optr->option; optr++) {
		if (!strcmp(optr->option, opt) || ((opt[0] == optr->shortopt) && (opt[1] == 0))) {
			if (param) {
				if (optr->flags & FN_STRING)
					((void (*)(struct ncp_mount_info*, const char*))(optr->proc))(info, param);
				else if (optr->flags & FN_INT) {
					unsigned int i;
					char* end;

					i = strtoul(param, &end, 10);
					if (end && *end && !isspace(*end)) {
						errexit(24, _("Value `%s' for option `%s' is not a number\n"), param, opt);
					}
					((void (*)(struct ncp_mount_info*, unsigned int))(optr->proc))(info, i);
				} else {
					fprintf(stderr, _("Ignoring unneeded value for option `%s'\n"), opt);
					((void (*)(struct ncp_mount_info*, unsigned int))(optr->proc))(info, optr->param);
				}
			} else {
				if (!(optr->flags & FN_NOPARAM)) {
					errexit(25, _("Required parameter for option `%s' missing\n"), opt);
				}
				((void (*)(struct ncp_mount_info*, unsigned int))(optr->proc))(info, optr->param);
			}
			return 0;
		}
	}
	return -1;
}

static void opt_set_flags(struct ncp_mount_info* info, unsigned int param) {
	info->flags |= param;
}

static void opt_clear_flags(struct ncp_mount_info* info, unsigned int param) {
	info->flags &= ~param;
}

static void opt_clear_flags_su(struct ncp_mount_info* info, unsigned int param) {
	if (info->flags & param) {
	        if (suser()) {
			info->flags &= ~param;
                	return;
	        }
        	errexit(82, _("You are not allowed to clear nosuid and nodev flags\n"));
	}
}

static void opt_clear_noexec(struct ncp_mount_info* info, unsigned int param) {
	if (info->flags & param) {
	        if (suser() || myuid == info->mdata.uid) {
			info->flags &= ~param;
                	return;
	        }
        	errexit(83, _("You are not allowed to clear noexec flag\n"));
	}
}

static void opt_remount(struct ncp_mount_info* info, unsigned int param) __attribute__((noreturn));
static void opt_remount(UNUSED(struct ncp_mount_info* info), UNUSED(unsigned int param)) {
	errexit(7, _("Remounting not supported, sorry\n"));
}

static void opt_set_mount_soft(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_soft = param;
}

static void opt_set_mount_timeout(struct ncp_mount_info* info, unsigned int param) {
	if ((param < 1) || (param > 900)) {
		errexit(8, _("Timeout must be between 1 and 900 secs inclusive\n"));
	}
	info->mdata.time_out = param;
}

static void opt_set_mount_retry(struct ncp_mount_info* info, unsigned int param) {
	if ((param < 1) || (param > 0x10000)) {
		errexit(9, _("Retry count must be between 1 and 65536 inclusive\n"));
	}
	info->mdata.retry_count = param;
}

static void opt_set_mount_strong(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_strong = param;
}

static void opt_set_mount_extras(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_extras = param;
}

static void opt_set_mount_symlinks(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_symlinks = param;
}

static void opt_set_mount_no_nfs(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_no_nfs = param;
}

static void opt_set_mount_no_os2(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_no_os2 = param;
}

static void opt_set_mount_nfs_extras(struct ncp_mount_info* info, unsigned int param) {
	info->mdata.flags.mount_nfs_extras = param;
}

static void opt_set_server(struct ncp_mount_info* info, const char* param) {
	if (strlen(param) >= sizeof(((const struct ncp_conn_spec*)param)->server)) {
		errexit(17, _("Specified server name `%s' is too long\n"), param);
	}
	info->server = param;
}

static void opt_set_server_name(struct ncp_mount_info* info, const char* param) {
	info->server_name = param;
}

static void opt_set_sig_level(struct ncp_mount_info* info, unsigned int param) {
	if (param > 3) {
		errexit(20, _("NCP signature level must be number between 0 and 3. You specified %u\n"), param);
	}
	info->sig_level = param;
}

static void opt_set_ttl(struct ncp_mount_info* info, unsigned int param) {
	if (param > 20000) {
		errexit(21, _("NCP cache time to live must be less than 20000 ms. You specified %u\n"), param);
	}
	info->dentry_ttl = param;
}

static void opt_set_file_mode(struct ncp_mount_info* info, const char* param) {
	char* end;

	info->mdata.file_mode = strtoul(param, &end, 8);
	if (end && *end && !isspace(*end)) {
		errexit(22, _("File mode `%s' is not valid octal number\n"), param);
	}
}

static void opt_set_dir_mode(struct ncp_mount_info* info, const char* param) {
	char* end;

	info->mdata.dir_mode = strtoul(param, &end, 8);
	if (end && *end && !isspace(*end)) {
		errexit(23, _("Directory mode `%s' is not valid octal number\n"), param);
	}
}

static void opt_set_iocharset(struct ncp_mount_info* info, const char* param) {
	if (strlen(param) >= sizeof(info->nls_cs.iocharset)) {
		errexit(128, _("I/O charset name `%s' is too long\n"), param);
	}
	strcpy(info->nls_cs.iocharset, param);
}

static void opt_set_codepage(struct ncp_mount_info* info, const char* param) {
	if (strlen(param) >= sizeof(info->nls_cs.codepage)) {
		errexit(128, _("Codepage name `%s' is too long\n"), param);
	}
	strcpy(info->nls_cs.codepage, param);
}

static void opt_not_implemented(UNUSED(struct ncp_mount_info* info), UNUSED(unsigned int param)) {
	/* noop */
}

static void opt_set_transport(struct ncp_mount_info* info, unsigned int param) {
	info->nt.value = param;
	info->nt.valid = 1;
}

static void opt_set_version(struct ncp_mount_info* info, unsigned int param) {
	info->version = param;
}

static struct optinfo globopts[] = {
	{0,   "ro",		FN_NOPARAM,	opt_set_flags,		MS_RDONLY},
	{0,   "rw",		FN_NOPARAM,	opt_clear_flags,	MS_RDONLY},
	{0,   "nosuid",		FN_NOPARAM,	opt_set_flags,		MS_NOSUID},
	{0,   "suid",		FN_NOPARAM,	opt_clear_flags_su,	MS_NOSUID},
	{0,   "nodev",		FN_NOPARAM,	opt_set_flags,		MS_NODEV},
	{0,   "dev",		FN_NOPARAM,	opt_clear_flags_su,	MS_NODEV},
	{0,   "noexec",		FN_NOPARAM,	opt_set_flags,		MS_NOEXEC},
	{0,   "exec",		FN_NOPARAM,	opt_clear_noexec,	MS_NOEXEC},
	{0,   "sync",		FN_NOPARAM,	opt_set_flags,		MS_SYNCHRONOUS},
	{0,   "async",		FN_NOPARAM,	opt_clear_flags,	MS_SYNCHRONOUS},
	{0,   "mand",		FN_NOPARAM,	opt_set_flags,		MS_MANDLOCK},
	{0,   "nomand",		FN_NOPARAM,	opt_clear_flags,	MS_MANDLOCK},
	{0,   "noatime",	FN_NOPARAM,	opt_set_flags,		MS_NOATIME},
	{0,   "atime",		FN_NOPARAM,	opt_clear_flags,	MS_NOATIME},
	{0,   "nodiratime",	FN_NOPARAM,	opt_not_implemented,	MS_NODIRATIME},	/* does not exist in 2.0 kernel */
	{0,   "remount",	FN_NOPARAM,	opt_remount,		0},
	{0,   "soft",		FN_NOPARAM,	opt_set_mount_soft,	1},
	{0,   "hard",		FN_NOPARAM,	opt_set_mount_soft,	0},
	{'t', "timeo",		FN_INT,		opt_set_mount_timeout,	60},
	{'t', "timeout",	FN_INT,		opt_set_mount_timeout,	60},
	{'r', "retry",		FN_INT,		opt_set_mount_retry,	5},
	{'S', "server",		FN_STRING,	opt_set_server,		0},
	{'s', "strong",		FN_NOPARAM,	opt_set_mount_strong,	1},
	{0,   "nostrong",	FN_NOPARAM,	opt_set_mount_strong,	0},
	{0,   "symlinks",	FN_NOPARAM,	opt_set_mount_symlinks,	1},
	{0,   "nosymlinks",	FN_NOPARAM,	opt_set_mount_symlinks,	0},
	{0,   "extras",		FN_NOPARAM,	opt_set_mount_extras,	1},
	{0,   "noextras",	FN_NOPARAM,	opt_set_mount_extras,	0},
	{0,   "nfsextras",	FN_NOPARAM,	opt_set_mount_nfs_extras, 1},
	{0,   "nonfsextras",	FN_NOPARAM,	opt_set_mount_nfs_extras, 0},
	{0,   "nonfs",		FN_NOPARAM,	opt_set_mount_no_nfs,	1},
	{0,   "noos2",		FN_NOPARAM,	opt_set_mount_no_os2,	1},
	{0,   "nolong",		FN_NOPARAM,	opt_set_mount_no_os2,	1},
	{'A', "ipserver",	FN_STRING,	opt_set_server_name,	0},
	{'i', "signature",	FN_INT,		opt_set_sig_level,	0},
	{'f', "mode",		FN_STRING,	opt_set_file_mode,	0},
	{'f', "filemode",	FN_STRING,	opt_set_file_mode,	0},
	{'d', "dirmode",	FN_STRING,	opt_set_dir_mode,	0},
	{'y', "iocharset",	FN_STRING,	opt_set_iocharset,	0},
	{'p', "codepage",	FN_STRING,	opt_set_codepage,	0},
	{0,   "ttl",            FN_INT,		opt_set_ttl,		0},
	{0,   "tcp",		FN_NOPARAM,	opt_set_transport,	NT_TCP},
	{0,   "udp",		FN_NOPARAM,	opt_set_transport,	NT_UDP},
	{0,   "ipx",		FN_NOPARAM,	opt_set_transport,	NT_IPX},
	{0,   "version",	FN_INT,		opt_set_version,	0},
	/* silently ignore "auto" and "noauto". They are used by 'mount -a',
	   but they are passed down to mount.ncp by bug in mount */
	{0,   "auto",		FN_NOPARAM,	opt_set_flags,		0},
	{0,   "noauto",		FN_NOPARAM,	opt_set_flags,		0},
	{0,   NULL,		0,		NULL,			0}
};

void proc_option(const struct optinfo* opts, struct ncp_mount_info* info, const char* opt, const char* param) {
	if (__proc_option(opts, info, opt, param) &&
	    __proc_option(extopts, info, opt, param) &&
	    __proc_option(globopts, info, opt, param)) {
		fprintf(stderr, _("Unknown option `%s', ignoring it\n"), opt);
	}
}

int proc_aftermount(const struct ncp_mount_info* info, NWCONN_HANDLE* pconn) {
	NWCONN_HANDLE conn;
	int err;

	if ((err = ncp_open_mount(info->mdata.mount_point, &conn)) != 0) {
		mycom_err(err, _("attempt to open mount point"));
		return 52;
	}

	if ((info->nls_cs.codepage[0] != 0) || (info->nls_cs.iocharset[0] != 0)) {
		int i;
		
		if (ncpm_suser()) {
			fprintf(stderr, _("Cannot switch to superuser: %s\n"), strerror(errno));
			return 90;
		}
		i = ioctl(ncp_get_fid(conn), NCP_IOC_SETCHARSETS,  &info->nls_cs);
		if (i && (errno == EINVAL)) {
			struct ncp_nls_ioctl_old old_nls;
			const char* p = info->nls_cs.codepage;
			
			/* skip `cp' */
			while (*p && !isdigit(*p)) p++;
			old_nls.codepage = strtoul(p, NULL, 0);
			strcpy(old_nls.iocharset, info->nls_cs.iocharset);
			i = ioctl(ncp_get_fid(conn), NCP_IOC_SETCHARSETS_OLD, &old_nls);
		}
		if (ncpm_normal()) {
			fprintf(stderr, _("Cannot relinquish superuser rights: %s\n"), strerror(-errno));
			return 91;
		}
		if (i) {
			if (errno == EINVAL || errno == ENOTTY) {
				fprintf(stderr, _("Your kernel does not support character mapping. You should upgrade to latest version.\n"));
			} else {
				perror(_("Warning: Unable to load NLS charsets"));
			}
		}
	}
	if (info->dentry_ttl) {
		err = ncp_set_dentry_ttl(conn, info->dentry_ttl);
		if (err == EINVAL) {
			fprintf(stderr, _("Your kernel does not support filename caching. You should upgrade to latest kernel version.\n"));
		} else if (err != 0) {
			fprintf(stderr, _("Warning: Cannot enable filename caching: %s\n"), strnwerror(err));
		}
	}
	
	if (info->sig_level >= 0) {
		size_t ln;
		
		err = NWCCGetConnInfo(conn, NWCC_INFO_MAX_PACKET_SIZE, sizeof(ln), &ln);
		if (!err)
			err = ncp_renegotiate_siglevel(conn, ln, info->sig_level);
		if (err) {
			ncp_close(conn);
			eprintf( _("Unable to negotiate requested security level: %s\n"), strnwerror(err));
			return 53;
		}
	}
	*pconn = conn;
	return 0;
}

int proc_buildconn(struct ncp_mount_info* info) {
	struct sockaddr_ipx addr;
	socklen_t addrlen;

#ifdef CONFIG_NATIVE_UNIX
	if (info->mdata.serv_addr.sipx_family == AF_UNIX) {
		info->mdata.ncp_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
		if (info->mdata.ncp_fd == -1) {
			mycom_err(errno, _("opening ncp_socket"));
                        return 37;
		}
		info->mdata.wdog_fd = -1;
		info->mdata.message_fd = -1;
		memset(&addr, 0, sizeof(addr));
		addr.sipx_family = AF_UNIX;
		addrlen = 2;
	} else
#endif
#ifdef CONFIG_NATIVE_IP
	if (info->mdata.serv_addr.sipx_family == AF_INET) {
		if (info->nt.valid && info->nt.value == NT_TCP) {
			info->mdata.ncp_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			info->mdata.nt_type = NT_TCP;
		} else if (info->nt.valid && info->nt.value != NT_UDP) {
			eprintf("%s: %s\n", progname, _("Invalid transport requested"));
			return 38;
		} else {
			info->mdata.ncp_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			info->mdata.nt_type = NT_UDP;
		}
		if (info->mdata.ncp_fd == -1) {
			mycom_err(errno, _("opening ncp_socket"));
			return 39;
		}
		info->mdata.wdog_fd = -1;
		info->mdata.message_fd = -1;
		memset(&addr, 0, sizeof(addr));
		addr.sipx_family = AF_INET;
		addrlen = 16;
	} else
#endif	/* CONFIG_NATIVE_IP */
#ifdef CONFIG_NATIVE_IPX
	if (info->mdata.serv_addr.sipx_family == AF_IPX) {
		if (info->nt.valid && info->nt.value != NT_IPX) {
			eprintf("%s: %s\n", progname, _("Invalid transport requested"));
			return 40;
		}
		info->mdata.nt_type = NT_IPX;
		info->mdata.ncp_fd = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
		if (info->mdata.ncp_fd == -1)
		{
			mycom_err(errno, _("opening ncp_socket"));
			return 41;
		}
		info->mdata.wdog_fd = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
		if (info->mdata.wdog_fd == -1)
		{
			eprintf(_("%s: Could not open wdog socket: %s\n"),
				progname,strerror(errno));
			return 42;
		}
		memset(&addr, 0, sizeof(addr));
		addr.sipx_family = AF_IPX;
		addr.sipx_type = NCP_PTYPE;
		addrlen = 16;
	} else
#endif	/* CONFIG_NATIVE_IPX */
	{
		eprintf(_("No transport available\n"));
		return 43;
	}
	if (bind(info->mdata.ncp_fd, (struct sockaddr *) &addr, addrlen) == -1)
	{
		eprintf(_("bind failed: %s\n"),	strerror(errno));
		fprintf(stderr,
			_("\nMaybe you want to use \n"
			  "ipx_configure --auto_interface=on --auto_primary=on\n"
			  "and try again after waiting a minute.\n\n"));
		return 44;

	}

#ifdef CONFIG_NATIVE_IPX
	if (info->mdata.serv_addr.sipx_family == AF_IPX) {
		addrlen = sizeof(addr);

		if (getsockname(info->mdata.ncp_fd, (struct sockaddr *) &addr, &addrlen) == -1)
		{
			perror(_("getsockname ncp socketfailed"));
			close(info->mdata.ncp_fd);
			close(info->mdata.wdog_fd);
			return 45;
		}
		addr.sipx_port = htons(ntohs(addr.sipx_port) + 1);

		if (bind(info->mdata.wdog_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		{
			eprintf(_("bind(wdog_sock, ): %s\n"),
				strerror(errno));
			return 46;
		}

		info->mdata.message_fd = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
		if (info->mdata.message_fd == -1)
		{
			eprintf(_("Could not open message socket: %s\n"),
				strerror(errno));
			return 47;
		}
		addr.sipx_port = htons(ntohs(addr.sipx_port) + 1);

		if (bind(info->mdata.message_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		{
			eprintf(_("bind(message_sock, ): %s\n"),
				strerror(errno));
			return 48;
		}
	}
#endif	/* CONFIG_NATIVE_IPX */
#ifdef CONFIG_NATIVE_UNIX
	if (info->mdata.serv_addr.sipx_family == AF_UNIX) {
		if (connect(info->mdata.ncp_fd, (struct sockaddr *) &info->mdata.serv_addr, 2 + 6)) {
			eprintf(_("connect(ncp_fd, ): %s\n"), strerror(errno));
			return 49;
		}
	}
#endif
	return 0;
}
