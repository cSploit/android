/*
    ncplib.c
    Copyright (C) 1995, 1996 by Volker Lendecke
    Copyright (C) 1996, 1997, 1998, 1999, 2000  Petr Vandrovec

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

	0.xx  1995-1999			Petr Vandrovec <vandrove@vc.cvut.cz>
					Paul Rensing <paulr@dragonsys.com>
					Wolfram Pienkoss <wp@bszh.de>
					Eloy A. Paris <>
					David Woodhouse <dave@imladris.demon.co.uk>
					Christian Groessler <cpg@aladdin.de>
					Martin Stover <>
					Rik Faith <faith@cs.unc.edu>
					Uwe Bones <bon@elektron.ikp.physik.th-darmstadt.de>
					Tomasz Babczynski <faster@dino.ict.pwr.wroc.pl>
					Guntram Blom <>
					Brian G. Reid <breid@tim.com>
					Jeff Buhrt <buhrt@iquest.net>
					Neil Turton <ndt1001@chu.cam.ac.uk>
					Tom C. Henderson <thenderson@tim.com>
					Mathew Lim <M.Lim@sp.ac.sg>
					Steven M. Hirsch <hirsch@emba.uvm.edu>
					Volker Lendecke
		Different contributuions. See ../Changes for details.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  1999, December  5		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ERR_RDN_TOO_LONG error description.
		Removed select() call from recv path.

	1.02  2000, January 13		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use IPX_SAP_SPECIFIC_QUERY first. Helps on misconfigured networks
		when there is no server answering get nearest service requests.

	1.03  2000, January 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changes for 32bit uids.

	1.04  2000, January 17		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fix bugs in RIP route search.

	1.05  2000, January 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed ncp_get_fs_info for 32bit uids.

	1.06  2000, May 14		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added support for upcoming /dev/ncp device.

	1.07  2000, May 24		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWLogoutFromFileServer.

	1.08  2000, May 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Modified ncp_open() to try server name as DNS too, so
		now you can use -S dns.name instead of -A dns.name. Fixes
		problem with pam_ncp_auth in pure-IP environment.

	1.09  2000, Jun 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NCP/TCP userspace support.

	1.10  2000, August 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added some error descriptions.

	1.11  2001, January 11		Patrick Pollet <Patrick.Pollet@cipcinsa.insa-lyon.fr>
		ncp_find_conn_spec2 should use passed 'uid' instead of doing 'getuid()'
		on its own. Could affect PHP ncp auth module and Apache ncp auth module.

	1.12  2001, March 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed ncp_scan_bindery_object() prototype: search_filter is const.
		Added ncp_next_conn.

	1.13  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Include strings.h for AIX, use waitpid instead of wait4, call
		ioctl(,NCP_*,) only when compiling with kernel support.

	1.14  2001, September 9		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fix exec_nwsfind to open child's stdin/stderr with O_RDWR
		instead of O_RDONLY.

	1.15  2001, September 23	Petr Vandrovec <vandrove@vc.cvut.cz>
		Always initialize unused fields of struct ncp_bindery_object
		to zero (in ncp_get_bindery_object_{id,name} and
		ncp_get_stations_logged_info).

	1.16  2001, September 23	Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ncp_change_object_security... How is it possible that
		nobody never evere noticed that this function occurs only
		in headers?

	1.17  2001, December 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Call waitpid() without WNOHANG. Otherwise we can leave zombies
		on SMP machines, and maybe even on UP.

	1.18  2002, January 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NULL pointer checks here and there.
		Added NCP reply size checks.

	1.19  2003, March 19		Patrick Pollet <Patrick.Pollet@cipcinsa.insa-lyon.fr>
		Modified ncp_find_permanent & ncp_find_conn_spec3 to fix
		ncp_initialize not accepting -S /mount/point.

 */

/* #define CONFIG_NATIVE_UNIX */
#undef CONFIG_NATIVE_UNIX
#define NCP_OBSOLETE

#if 0
#define ncp_dprintf(X...)	printf(X)
#else
#define ncp_dprintf(X...)
#endif
#include "config.h"
#include "ncplib_i.h"
/* due to NWVerifyObjectPassword... */
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncpsign.h"
#ifdef NDS_SUPPORT
int bindery_only = 0;
#include <ncp/ndslib.h>
#endif
#include <ncp/nwnet.h>

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <ncp/ext/socket.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <ncp/kernel/route.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
#include <mntent.h>
#endif
#include <pwd.h>
#include <sys/stat.h>
#include <stdarg.h>
#ifdef CONFIG_NATIVE_IP
#include <netdb.h>
#endif
#ifdef CONFIG_NATIVE_UNIX
#include <linux/un.h>
#endif
#include <sys/uio.h>

#include "private/ncp-new.h"
#include "ncpcode.h"
#include "cfgfile.h"

#include "private/libintl.h"
#define _(X) dgettext(NCPFS_PACKAGE, (X))
#define N_(X) (X)

#ifndef ENOPKG
#define ENOPKG	ENOSYS
#endif

#define NCP_DEFAULT_BUFSIZE	1024
#define NCP_MAX_BUFSIZE		1024

#ifdef SIGNATURES
int in_options = 2;
#else
int in_options = 0;
#endif

#if defined(ANDROID) || defined(__BIONIC__)
/* function provided by util/getpass.c
 * which is missing from the BIONIC libc
 */
char * getpass(const char *);
#endif

static ncpt_mutex_t conn_lock = NCPT_MUTEX_INITIALIZER;
static LIST_HEAD(conn_list);
ncpt_mutex_t nds_ring_lock = NCPT_MUTEX_INITIALIZER;

/* return number of bytes in packet */
static inline size_t
ncp_packet_size(struct ncp_conn *_conn)
{
	return _conn->current_point - _conn->packet - 6;
}

static long
ncp_negotiate_buffersize(struct ncp_conn *conn,
			  size_t size, size_t *target);

#ifdef SIGNATURES
static NWCCODE
ncp_negotiate_size_and_options(struct ncp_conn *conn,
			  size_t size, int options, 
			  size_t *ret_size, int *ret_options);
#endif

static long
 ncp_login_object(struct ncp_conn *conn,
		  const unsigned char *username,
		  int login_type,
		  const unsigned char *password);

static long
ncp_do_close(struct ncp_conn *conn);

void
str_upper(char *name)
{
	while (*name) {
		*name = toupper(*name);
		name = name + 1;
	}
}

#ifdef NCP_TRACE_ENABLE

static FILE *logfile = 0;

inline FILE *
get_logfile()
{
	if (!logfile)
		logfile = fopen("/var/log/ncplib.trc", "aw");

	return logfile;
}

void
__ncp_trace(char *module, int line, char *p,...)
{
	FILE *log = get_logfile();
	va_list ap;

	if (!log)
		return;

	fprintf(log, "%s(%d)->", module, line);

	va_start(ap, p);
	vfprintf(log, p, ap);
	va_end(ap);
	fprintf(log, "\n");
	fflush(log);
}

void
__dump_hex(const char *_msg, const unsigned char *_buf, size_t _len)
{
	static char sym[] = "0123456789ABCDEF";
	FILE *log = get_logfile();
	if (!log)
		return;

	fprintf(log, "len = %d:msg->%s", _len, _msg);
	fflush(log);
	for (; _len > 0; _len--, _buf++) {
		putc(sym[(*_buf) >> 4], log);
		putc(sym[(*_buf) & 0x0F], log);
	}
	putc('\n', log);
	fflush(log);
}
#endif /*def NCP_TRACE_ENABLE */

#if 0

static int debug_level = 5;
static FILE *logfile = stderr;

static void
dprintf(int level, char *p,...)
{
	va_list ap;

	if (level > debug_level) {
		return;
	}
	va_start(ap, p);
	vfprintf(logfile, p, ap);
	va_end(ap);
	fprintf(logfile, "\n");
	fflush(logfile);
}

#endif

/* I know it's terrible to include a .c file here, but I want to keep
   the file nwcrypt.c intact and separate for copyright reasons */
#include "nwcrypt.c"

void
ipx_fprint_node(FILE * file, IPXNode node)
{
	fprintf(file, "%02X%02X%02X%02X%02X%02X",
		(unsigned char) node[0],
		(unsigned char) node[1],
		(unsigned char) node[2],
		(unsigned char) node[3],
		(unsigned char) node[4],
		(unsigned char) node[5]
	    );
}

void
ipx_fprint_network(FILE * file, IPXNet net)
{
	fprintf(file, "%08X", (u_int32_t)ntohl(net));
}

void
ipx_fprint_port(FILE * file, IPXPort port)
{
	fprintf(file, "%04X", ntohs(port));
}

void
ipx_print_node(IPXNode node)
{
	ipx_fprint_node(stdout, node);
}

void
ipx_print_network(IPXNet net)
{
	ipx_fprint_network(stdout, net);
}

void
ipx_print_port(IPXPort port)
{
	ipx_fprint_port(stdout, port);
}

int
ipx_node_equal(CIPXNode n1, CIPXNode n2)
{
	return memcmp(n1, n2, IPX_NODE_LEN) == 0;
}

int
ipx_sscanf_node(char *buf, unsigned char node[6])
{
	int i;
	int n[6];

	if ((i = sscanf(buf, "%2x%2x%2x%2x%2x%2x",
			&(n[0]), &(n[1]), &(n[2]),
			&(n[3]), &(n[4]), &(n[5]))) != 6) {
		return i;
	}
	for (i = 0; i < 6; i++) {
		node[i] = n[i];
	}
	return 6;
}

#ifdef CONFIG_NATIVE_IPX
void
ipx_fprint_saddr(FILE * file, struct sockaddr_ipx *sipx)
{
	ipx_fprint_network(file, sipx->sipx_network);
	fprintf(file, ":");
	ipx_fprint_node(file, sipx->sipx_node);
	fprintf(file, ":");
	ipx_fprint_port(file, sipx->sipx_port);
}

void
ipx_print_saddr(struct sockaddr_ipx *sipx)
{
	ipx_fprint_saddr(stdout, sipx);
}

int
ipx_sscanf_saddr(char *buf, struct sockaddr_ipx *target)
{
	char *p;
	struct sockaddr_ipx addr;
	unsigned long sipx_network;

	addr.sipx_family = AF_IPX;
	addr.sipx_type = NCP_PTYPE;

	if (sscanf(buf, "%lx", &sipx_network) != 1) {
		return 1;
	}
	addr.sipx_network = htonl(sipx_network);
	if ((p = strchr(buf, ':')) == NULL) {
		return 1;
	}
	p += 1;
	if (ipx_sscanf_node(p, addr.sipx_node) != 6) {
		return 1;
	}
	if ((p = strchr(p, ':')) == NULL) {
		return 1;
	}
	p += 1;
	if (sscanf(p, "%hx", &addr.sipx_port) != 1) {
		return 1;
	}
	addr.sipx_port = htons(addr.sipx_port);
	*target = addr;
	return 0;
}
#endif	/* CONFIG_NATIVE_IPX */

NWCCODE
x_recvfrom(int sock, void *buf, int len, unsigned int flags,
	   struct sockaddr *sender, socklen_t* addrlen, int timeout,
	   size_t* rlen)
{
	int result;

restart:
	if (timeout >= 0) {
		struct pollfd pfd;
		
		pfd.fd = sock;
		pfd.events = POLLIN | POLLHUP;
		
		if ((result = poll(&pfd, 1, timeout)) == -1) {
			if (errno == EINTR) {
				timeout /= 2;
				goto restart;
			}
			return errno;
		}
		if (!(pfd.revents & (POLLIN | POLLHUP))) {
			return ETIMEDOUT;
		}
	}
	result = sender?recvfrom(sock, buf, len, flags,
				 sender, addrlen):
			recv(sock, buf, len, flags);
	if (result < 0)
		return errno;
	*rlen = result;
	return 0;
}

#ifdef CONFIG_NATIVE_IPX
static int
exec_nwsfind(const char* request[]) {
	int err;
	
	signal(SIGCHLD, SIG_DFL);
	err = fork();
	if (err < 0)
		return errno;
	if (err == 0) {
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_RDWR); /* fd 0 */
		dup2(0, 1);
		dup2(0, 2);
		request[0] = NWSFIND;
		execv(request[0], (char**)request);
		/* OK, something bad happen... Drop it to /dev/null */
		exit(127);
	} else {
		int rd;
		
		if (waitpid(err, &rd, 0) != err) {
			return -1;
		}
		if (!WIFEXITED(rd)) {
			return -1;
		}
		if (WEXITSTATUS(rd)) {
			return -1;
		}
	}
	return 0;
}

/* Used only by nwsfind */
int ipx_make_reachable_rip(const struct sockaddr_ipx* target);

int
ipx_make_reachable_rip(const struct sockaddr_ipx* target)
{
	IPXNet	network = target->sipx_network;
	struct rtentry rt_def;
	/* Router */
	struct sockaddr_ipx *sr = (struct sockaddr_ipx *) &rt_def.rt_gateway;
	/* Target */
	struct sockaddr_ipx *st = (struct sockaddr_ipx *) &rt_def.rt_dst;

	struct ipx_rip_packet rip;
	struct sockaddr_ipx addr;
	socklen_t addrlen;
	int sock;
	int opt;
	int res = -1;
	int i;
	int packets;

	memset(&rip, 0, sizeof(rip));

	sock = socket(PF_IPX, SOCK_DGRAM, IPXPROTO_IPX);

	if (sock == -1) {
		return errno;
	}
	opt = 1;
	/* Permit broadcast output */
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) != 0) {
		goto finished;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sipx_family = AF_IPX;
	addr.sipx_network = htonl(0x0);
	addr.sipx_port = htons(0x0);
	addr.sipx_type = IPX_RIP_PTYPE;

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
		goto finished;
	}
	addr.sipx_family = AF_IPX;
	addr.sipx_port = htons(IPX_RIP_PORT);
	addr.sipx_type = IPX_RIP_PTYPE;
	addr.sipx_network = htonl(0x0);
	ipx_assign_node(addr.sipx_node, IPX_BROADCAST_NODE);

	rip.operation = htons(IPX_RIP_REQUEST);
	rip.rt[0].network = network;

	if (sendto(sock, &rip, sizeof(rip), 0,
		   (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		goto finished;
	}
	packets = 3;
	do {
		NWCCODE err;
		size_t len;

		if (packets == 0) {
			goto finished;
		}
		addrlen = sizeof(struct sockaddr_ipx);

		err = x_recvfrom(sock, &rip, sizeof(rip), 0, (struct sockaddr*)sr, &addrlen, 1000,
				   &len);

		if (err || len < sizeof(rip)) {
			packets = packets - 1;
			continue;
		}
	} while (ntohs(rip.operation) != IPX_RIP_RESPONSE);

	if (rip.rt[0].network != network) {
		goto finished;
	}
	rt_def.rt_flags = RTF_GATEWAY;
	st->sipx_network = network;
	st->sipx_family = AF_IPX;
	sr->sipx_family = AF_IPX;

	i = 0;
	do {
		res = ioctl(sock, SIOCADDRT, &rt_def);
		i++;
	} while ((i < 5) && (res < 0) && (errno == EAGAIN));

      finished:
	close(sock);

	if (res) {
		return ENETUNREACH;
	}
	return 0;
}

static int
ipx_make_reachable_call(const struct sockaddr_ipx* target)
{
	char buf[40];
	const char *buf2[4];
	
	buf2[1] = "-a";
	buf2[2] = buf;
	buf2[3] = NULL;
    
	sprintf(buf, "%08x:%02x%02x%02x%02x%02x%02x:%04x",
		(u_int32_t)ntohl(target->sipx_network),
		((const unsigned char*)target->sipx_node)[0],
		((const unsigned char*)target->sipx_node)[1],
		((const unsigned char*)target->sipx_node)[2],
		((const unsigned char*)target->sipx_node)[3],
		((const unsigned char*)target->sipx_node)[4],
		((const unsigned char*)target->sipx_node)[5],
		ntohs(target->sipx_port));

	return exec_nwsfind(buf2) ? ENETUNREACH : 0;
}

static int ipx_make_reachable(const struct sockaddr_ipx* target) {
	if (geteuid() == 0)
		return ipx_make_reachable_rip(target);
	else
		return ipx_make_reachable_call(target);
}

void
ipx_assign_node(IPXNode dest, CIPXNode src)
{
	memcpy(dest, src, IPX_NODE_LEN);
}

static void run_wdog(struct ncp_conn *conn, int fd) {
	struct pollfd pfd[2];
	
	pfd[0].fd = conn->wdog_sock;
	pfd[0].events = POLLIN;
	pfd[1].fd = fd;
	pfd[1].events = POLLIN | POLLHUP;
	while (1) {
		switch (poll(pfd, 2, -1)) {
			case -1:
				if (errno != EINTR) {
					/* Commit suicide, error happened */
					return;
				}
				break;
			case 0:
				break;
			default:
				if (pfd[0].revents & POLLIN) {
					struct sockaddr_ipx sender;
					int sizeofaddr = sizeof(struct sockaddr_ipx);
					char buf[1024];
					size_t pktsize;
					NWCCODE err;
		
					err = x_recvfrom(pfd[0].fd, buf, sizeof(buf), 0,
					     (struct sockaddr*)&sender, &sizeofaddr, 120000, &pktsize);

					if (!err && pktsize >= 2 && buf[1] == '?') {
						buf[1] = 'Y';
						pktsize = sendto(pfd[0].fd, buf, 2, 0, (struct sockaddr *)&sender, sizeof(sender));
					}
				}
				if (pfd[0].revents & (POLLHUP | POLLNVAL)) {
					/* HUP or invalid? Both unexpected => die */
					return;
				}
				if (pfd[1].revents & POLLHUP) {
					/* Parent closed NCP connection => stop */
					return;
				}
				if (pfd[1].revents & POLLIN) {
					int i;
					char cmd;
					/* Parent wrote something to us */
					i = read(fd, &cmd, 1);
					if (i == 1) {
						switch (cmd) {
							case 'Q':	/* quit */
								return;
						}
					}
					/* Something wrong happened => die */
					return;
				}
				if (pfd[1].revents & POLLNVAL) {
					/* Parent either send us some data, or invalid => die */
					return;
				}
				break;
		}
	}
}

static int
install_wdog(struct ncp_conn *conn)
{
	int pid;
	int fildes[2];
	int i;

	if (socketpair(PF_UNIX, SOCK_STREAM, 0, fildes)) {
		return -1;
	}
	fcntl(fildes[0], F_SETFD, FD_CLOEXEC);
	fcntl(fildes[1], F_SETFD, FD_CLOEXEC);
	if ((pid = fork()) < 0) {
		close(fildes[0]);
		close(fildes[1]);
		return -1;
	}
	if (pid != 0) {
		int status;
		pid_t p;
		
		close(fildes[0]);
		/* Parent: wait for child to exit */
		p = waitpid(pid, &status, 0);
		if (p < 0) {
			close(fildes[1]);
			return -1;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status)) {
			close(fildes[1]);
			return -1;
		}
		conn->wdog_pipe = fildes[1];
		return 0;
	}
	for (i = 0; i < 1024; i++) {
		if (i == fildes[0] || i == conn->wdog_sock) {
			continue;
		}
		close(i);
	}
	if ((pid = fork()) < 0) {
		close(fildes[0]);
		exit(1);
	}
	if (pid != 0) {
		close(fildes[0]);
		exit(0);
	}
	chdir("/");
	run_wdog(conn, fildes[0]);
	/* wdog socket must be closed before wdog pipe */
	close(conn->wdog_sock);
	close(fildes[0]);
	exit(0);
}
#endif	/* CONFIG_NATIVE_IPX */

void
ncp_lock_conn(struct ncp_conn *conn)
{
	ncpt_mutex_lock(&conn->buffer_mutex);
	conn->lock++;
}

void
ncp_unlock_conn(struct ncp_conn *conn)
{
	assert_conn_locked(conn);
	conn->lock--;
	ncpt_mutex_unlock(&conn->buffer_mutex);
}

static struct ncp_conn *ncp_alloc_conn(void) {
	struct ncp_conn* conn;
	
	conn = (struct ncp_conn*)malloc(sizeof(*conn));
	if (conn) {
		memset(conn, 0, sizeof(*conn));
		ncpt_atomic_set(&conn->use_count, 1);
		ncpt_atomic_set(&conn->store_count, 0);
		INIT_LIST_HEAD(&conn->nds_ring);
//		conn->nds_conn = NULL;
		INIT_LIST_HEAD(&conn->conn_ring);
		ncpt_mutex_init(&conn->buffer_mutex);
		ncpt_mutex_init(&conn->serverInfo.mutex);
//		conn->serverInfo.valid = 0;
//		conn->connState = 0;
		conn->bcast_state = NWCC_BCAST_PERMIT_UNKNOWN;
		conn->global_fd = -1;
		conn->wdog_pipe = -1;
		ncpt_mutex_lock(&conn_lock);
		list_add(&conn->conn_ring, &conn_list);
		ncpt_mutex_unlock(&conn_lock);
	}
	return conn;
}

static inline unsigned int get_conn_from_reply(const void* hdr) {
	return BVAL(hdr, 3) | (BVAL(hdr, 5) << 8);
}

static NWCCODE
do_ncp_call(struct ncp_conn *conn, unsigned int cmd, unsigned int task, const unsigned char* request, size_t request_size)
{
	int result;
	int retries = 20;
	size_t len;
	NWCCODE err;
	unsigned char header[6];
	unsigned char signature[8];
	struct iovec io[3];
	struct msghdr msg;

	conn->sequence++;
	WSET_HL(header, 0, cmd);
	BSET(header, 2, conn->sequence);
	BSET(header, 3, conn->i.connection);
	BSET(header, 4, task);
	BSET(header, 5, conn->i.connection >> 8);
	
	io[0].iov_base = header;
	io[0].iov_len = 6;
	io[1].iov_base = ncp_const_cast(request);
	io[1].iov_len = request_size;
	io[2].iov_base = signature;
	io[2].iov_len = sign_packet(conn, request, request_size,
			cpu_to_le32(request_size + 6), signature);

	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_iov = io;
	msg.msg_iovlen = io[2].iov_len ? 3 : 2;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	
	conn->ncp_reply = conn->ncp_reply_buffer;

	while (retries > 0) {
		retries -= 1;

		result = sendmsg(conn->ncp_sock, &msg, 0);
		if (result < 0) {
			return errno;
		}
re_select:
		err = x_recv(conn->ncp_sock,
			     conn->ncp_reply, conn->ncp_reply_alloc,
			     0, 3000, &len);

		if (err) {
			if (err == ETIMEDOUT)
				continue;
			return err;
		}
		if (len < sizeof(struct ncp_reply_header*) || len > conn->ncp_reply_alloc) {
			continue;
		}
		if (WVAL_HL(conn->ncp_reply, 0) != NCP_REPLY) {
			goto re_select;
		}
		if (cmd != NCP_ALLOC_SLOT_REQUEST &&
		   (BVAL(conn->ncp_reply, 2) != conn->sequence
		     || get_conn_from_reply(conn->ncp_reply) != conn->i.connection))
		{
			goto re_select;
		}
		if (ncp_get_sign_active(conn) && cmd != NCP_DEALLOC_SLOT_REQUEST) {
			unsigned int hdrl;

			len -= 8;
			hdrl = conn->nt == NT_UDP ? 8 : 6;
			if (len < hdrl)
				goto re_select;
			if (sign_verify_reply(conn, conn->ncp_reply + hdrl, len - hdrl, cpu_to_le32(len), conn->ncp_reply + len)) {
				goto re_select;
			}
		}
		conn->ncp_reply_size = len - sizeof(struct ncp_reply_header);
		return 0;
	}
	return ETIMEDOUT;
}

static int do_tcp_rcv(int fd, unsigned char* data, size_t len) {
	while (len) {
		int rcv;
	
		rcv = recv(fd, data, len, 0);
		if (rcv < 0) {
			if (errno == EINTR)
				continue;
			return errno;
		}
		if (rcv == 0) {
			return ECONNABORTED;
		}
		if ((size_t)rcv > len) {
			return ECONNABORTED;
		}
		len -= rcv;
		data += rcv;
	}
	return 0;
}

static int do_tcp_rcv_skip(int fd, size_t len) {
	while (len) {
		int rcv;
		unsigned char dummy[1000];
	
		if (len > sizeof(dummy)) {
			rcv = recv(fd, dummy, sizeof(dummy), 0);
		} else {
			rcv = recv(fd, dummy, len, 0);
		}
		if (rcv < 0) {
			if (errno == EINTR)
				continue;
			return errno;
		}
		if (rcv == 0) {
			return ECONNABORTED;
		}
		if ((size_t)rcv > len) {
			return ECONNABORTED;
		}
		len -= rcv;
	}
	return 0;
}

static NWCCODE
do_ncp_tcp_call(struct ncp_conn *conn, unsigned int cmd, unsigned int task, const unsigned char *request, size_t request_size)
{
	unsigned char replyhdr[18];
	int result;
	unsigned char rqhdr[30];
	struct iovec io[2];
	struct msghdr msg;
	size_t replyhdrlen;
	size_t ln;
	struct ncp_reply_header *reply;
	size_t siglen;

	conn->sequence++;

	siglen = sign_packet(conn, request, request_size,
			cpu_to_be32(request_size + 30), rqhdr + 16);

	DSET_HL(rqhdr, 0, 0x446D6454);
	DSET_HL(rqhdr, 4, request_size + 16 + siglen + 6);
	DSET_HL(rqhdr, 8, 1);
	DSET_HL(rqhdr, 12, sizeof(conn->packet));
	
	WSET_HL(rqhdr, 16 + siglen + 0, cmd);
	BSET(rqhdr, 16 + siglen + 2, conn->sequence);
	BSET(rqhdr, 16 + siglen + 3, conn->i.connection);
	BSET(rqhdr, 16 + siglen + 4, task);
	BSET(rqhdr, 16 + siglen + 5, conn->i.connection >> 8);

	io[0].iov_base = rqhdr;
	io[0].iov_len = 16 + siglen + 6;
	io[1].iov_base = ncp_const_cast(request);
	io[1].iov_len = request_size;
	
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_iov = io;
	msg.msg_iovlen = 2;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
		
	result = sendmsg(conn->ncp_sock, &msg, MSG_NOSIGNAL);
	if (result < 0) {
		return errno;
	}
	if ((size_t)result != io[1].iov_len + io[0].iov_len) {
		return ECONNABORTED;
	}
	replyhdrlen = 8 + siglen + 2;
	while (1) {
		result = do_tcp_rcv(conn->ncp_sock, replyhdr, replyhdrlen);
		if (result) {
			return result;
		}
		if (DVAL_HL(replyhdr, 0) != 0x744E6350) {
			fprintf(stderr, "RecvUnk: %08X, %08X\n", DVAL_HL(replyhdr, 0), DVAL_HL(replyhdr, 4));
			return ECONNABORTED;
		}
		ln = DVAL_HL(replyhdr, 4) & 0x0FFFFFFF;
		if (ln < replyhdrlen) {
			return ECONNABORTED;
		}
		ln -= replyhdrlen;
		if (WVAL_HL(replyhdr, replyhdrlen - 2) == NCP_REPLY) {
			break;
		}
		result = do_tcp_rcv_skip(conn->ncp_sock, ln);
		if (result) {
			return result;
		}
	};
	if (ln < sizeof(*reply) - 2) {
		return ECONNABORTED;
	}
	if (ln > sizeof(conn->packet) - 2) {
		fprintf(stderr, "Too long reply: %u\n", ln);
		return ECONNABORTED;
	}
	result = do_tcp_rcv(conn->ncp_sock, conn->packet + 2, ln);
	if (result) {
		return result;
	}
	reply = (struct ncp_reply_header*)conn->packet;
	reply->type = htons(NCP_REPLY);
	if (cmd != NCP_ALLOC_SLOT_REQUEST) {
		if (reply->sequence != conn->sequence
		   || get_conn_from_reply(reply) != conn->i.connection) {
			/* Protocol violation... */
			return ECONNABORTED;
		}
	}
	ln += 2;
	if (ncp_get_sign_active(conn) && cmd != NCP_DEALLOC_SLOT_REQUEST) {
		if (sign_verify_reply(conn, conn->packet + 6, ln - 6, cpu_to_be32(ln + 16), replyhdr + 8)) {
			return ERR_INVALID_SIGNATURE;
		}
	}
	conn->ncp_reply_size = ln - sizeof(struct ncp_reply_header);
	conn->ncp_reply = conn->packet;
	return 0;
}

static inline void
fill_size(struct ncp_conn *_conn)
{
	if (_conn->has_subfunction != 0) {
		size_t sz = ncp_packet_size(_conn) - 2 - 1;
		WSET_HL(_conn->packet, 7, sz);
	}
}

static NWCCODE
ncp_mount_request(struct ncp_conn *conn, int function)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	struct ncp_ioctl_request request;
	int result;

	assert_conn_locked(conn);

	fill_size(conn);
	request.function = function;
	request.size = ncp_packet_size(conn) + 6;
	request.data = conn->packet;

	if ((result = ioctl(conn->mount_fid, NCP_IOC_NCPREQUEST, &request)) < 0) {
		return errno;
	}
	conn->ncp_reply_size = result - sizeof(struct ncp_reply_header);
	conn->ncp_reply = conn->packet;
	
	result = BVAL(conn->packet, 6);
	conn->conn_status = BVAL(conn->packet, 7);

	if (result && (conn->verbose != 0)) {
		ncp_printf(_("ncp_request_error: %d\n"), result);
	}
	return (result == 0) ? 0 : (NWE_SERVER_ERROR | result);
#else
	return ENOPKG;
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

static NWCCODE
ncp_kernel_request(struct ncp_conn *conn, int function)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	struct ncp_ioc_request_reply request;
	int result;

	assert_conn_locked(conn);

	fill_size(conn);
	request.function = function;
	request.request.size = ncp_packet_size(conn) - 1;
	request.request.addr = conn->packet + sizeof(struct ncp_request_header);
	request.reply.size = sizeof(conn->packet) - sizeof(struct ncp_reply_header);
	request.reply.addr = conn->packet + sizeof(struct ncp_reply_header);

	if ((result = ioctl(conn->mount_fid, NCP_IOC_REQUEST_REPLY, &request)) < 0) {
		return errno;
	}
	conn->ncp_reply_size = request.reply.size - sizeof(struct ncp_reply_header);
	conn->ncp_reply = conn->packet;

	if (result && (conn->verbose != 0)) {
		ncp_printf(_("ncp_request_error: %d\n"), result);
	}
	return (result == 0) ? 0 : (NWE_SERVER_ERROR | result);
#else
	return ENOPKG;
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

static NWCCODE
ncp_temp_request(struct ncp_conn *conn, int function)
{
	NWCCODE err;

	assert_conn_locked(conn);

	BSET(conn->packet, 6, function);

	fill_size(conn);
	switch (conn->nt) {
		case NT_IPX:
		case NT_UDP:
			err = do_ncp_call(conn, NCP_REQUEST, 1, conn->packet + 6, ncp_packet_size(conn));
			break;
		case NT_TCP:
			err = do_ncp_tcp_call(conn, NCP_REQUEST, 1, conn->packet + 6, ncp_packet_size(conn));
			break;
		default:
			err = ECONNABORTED;
			break;
	}
	if (err) {
		return err;
	}
	err = BVAL(conn->ncp_reply, 6);
	conn->conn_status = BVAL(conn->ncp_reply, 7);

	if (err && (conn->verbose != 0)) {
		ncp_printf(_("ncp_request_error: %d\n"), err);
	}
	return (err == 0) ? 0 : (NWE_SERVER_ERROR | err);
}

NWCCODE
ncp_renegotiate_siglevel(NWCONN_HANDLE conn, size_t buffsize, int siglevel) {
	size_t neg_buffsize;
	int err;
	int newoptions;
	int options;

	if (ncp_get_sign_active(conn)) siglevel = 3;
	if (siglevel < 2)
		newoptions = 0;
	else
		newoptions = 2;
#ifdef SIGNATURES
	err = ncp_negotiate_size_and_options(conn, buffsize, newoptions, &neg_buffsize, &options);
	if (!err) {
		if ((options & 2) != newoptions) {
			if (siglevel == 3) {
				return NWE_SIGNATURE_LEVEL_CONFLICT;
			}
			if (siglevel != 0) {
				newoptions ^= 2;
				err = ncp_negotiate_size_and_options(conn, buffsize, newoptions, &neg_buffsize, &options);
				if (!err) {
					if ((options & 2) != newoptions) {
						return NWE_SIGNATURE_LEVEL_CONFLICT;
					}
				}
			}
		}
	}
	if (err) 
#endif	
	{
		if (siglevel == 3) {
			return NWE_SIGNATURE_LEVEL_CONFLICT;
		}
		err = ncp_negotiate_buffersize(conn, buffsize, &neg_buffsize);
		if (err) {
			return err;
		}
		options = 0;
	}
	if ((neg_buffsize < 512) || (neg_buffsize > NCP_PACKET_SIZE - 40))
		return NWE_REQUESTER_FAILURE;
	conn->i.buffer_size = neg_buffsize;
#ifdef SIGNATURES
	conn->sign_wanted = (options & 2) ? 1 : 0;
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	if (conn->is_connected == CONN_PERMANENT) {
		int cursign;

		if (ioctl(conn->mount_fid, NCP_IOC_SIGN_WANTED, &cursign)) {
			/* ncpfs does not support SIGN_WANTED -> current sign level = 0 */
			cursign = 0;
		}
		if (cursign) cursign = 1;
		if (cursign != conn->sign_wanted) {
			int newsign = conn->sign_wanted ? -1 : 0;

			err = ioctl(conn->mount_fid, NCP_IOC_SET_SIGN_WANTED, &newsign);
			if (err) {
				return errno;
			}
		}
	}
#endif  /* NCP_KERNEL_NCPFS_AVAILABLE */
#endif
	return 0;
	
}

static inline NWCCODE
ncp_negotiate_siglevel(struct ncp_conn *conn, size_t buffsize, int siglevel) {
#ifdef SIGNATURES
	conn->sign_active = 0;
	conn->sign_wanted = 0;
#endif
	return ncp_renegotiate_siglevel(conn, buffsize, siglevel);
}

long
ncp_renegotiate_connparam(struct ncp_conn *conn, int buffsize, int newoptions)
{
	if (newoptions & 2)
		newoptions = 2;
	else
		newoptions = 1;
	return ncp_renegotiate_siglevel(conn, buffsize, newoptions);
}

static void
move_conn_to_kernel(UNUSED(struct ncp_conn *conn)) {
#if 0
	int f;
	int e;
	struct ncp_ioc_newconn nc;
	
	f = open("/dev/ncp", O_RDWR);
	if (f == -1)
		return;
	nc.fd = conn->ncp_sock;
	nc.flags = 0;
	if (ncp_get_sign_wanted(conn))
		nc.flags |= NCP_FLAGS_SIGN_NEGOTIATED;
	nc.sequence = conn->sequence;
	nc.connection = conn->i.connection;
	e = ioctl(f, NCP_IOC_NEWCONN, &nc);
	if (e) {
		close(f);
		return;
	}
	close(conn->ncp_sock);
	conn->ncp_sock = -1;
	conn->is_connected = CONN_KERNELBASED;
	conn->mount_fid = f;
#endif
}

static NWCCODE ncp_finish_connect(struct ncp_conn *conn) {
	NWCCODE err;

	conn->sequence = 0;
	conn->i.connection = get_conn_from_reply(conn->ncp_reply);
	conn->is_connected = CONN_TEMPORARY;

	err = ncp_negotiate_siglevel(conn, NCP_DEFAULT_BUFSIZE, in_options);
	if (err) {
		return err;
	}
	conn->bcast_state = NWCC_BCAST_PERMIT_ALL;
	return 0;
}

#ifdef CONFIG_NATIVE_IPX
static long
ncp_connect_ipx_addr(struct ncp_conn *conn, const struct sockaddr_ipx *target,
		 int wdog_needed)
{
	static const unsigned char zero = 0;
	struct sockaddr_ipx addr;
	socklen_t addrlen;

	int ncp_sock, wdog_sock;
	long err;

	conn->ncp_reply_buffer = (char*)malloc(NCP_PACKET_SIZE);
	if (!conn->ncp_reply_buffer) {
		return ENOMEM;
	}
	conn->ncp_reply_alloc = NCP_PACKET_SIZE;

	conn->is_connected = NOT_CONNECTED;
	conn->verbose = 0;

	if ((ncp_sock = socket(PF_IPX, SOCK_DGRAM, IPXPROTO_IPX)) == -1) {
		return errno;
	}
	if ((wdog_sock = socket(PF_IPX, SOCK_DGRAM, IPXPROTO_IPX)) == -1) {
		close(ncp_sock);
		return errno;
	}
	addr.sipx_family = AF_IPX;
	addr.sipx_port = htons(0x0);
	addr.sipx_type = NCP_PTYPE;
	addr.sipx_network = IPX_THIS_NET;
	ipx_assign_node(addr.sipx_node, IPX_THIS_NODE);

	addrlen = sizeof(addr);

	if ((bind(ncp_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	|| (getsockname(ncp_sock, (struct sockaddr *) &addr, &addrlen) == -1)) {
		int saved_errno = errno;
		close(ncp_sock);
		close(wdog_sock);
		return saved_errno;
	}
	addr.sipx_port = htons(ntohs(addr.sipx_port) + 1);

	if (bind(wdog_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		close(wdog_sock);
		return saved_errno;
	}
	conn->ncp_sock = ncp_sock;
	conn->wdog_sock = wdog_sock;

	conn->sequence = 0;
	conn->addr.ipx = *target;
	conn->nt = NT_IPX;

	if (connect(ncp_sock, (const struct sockaddr*)target, sizeof(*target)) == -1) {
		int saved_errno = errno;

		if ((saved_errno != ENETUNREACH) || ipx_make_reachable(target) 
		  || connect(ncp_sock, (const struct sockaddr*)target, sizeof(*target))) {;
			close(ncp_sock);
			close(wdog_sock);
			return saved_errno;
		}
	}

	conn->i.connection = ~0;

	if ((err = do_ncp_call(conn, NCP_ALLOC_SLOT_REQUEST, 1, &zero, 1)) != 0) {
		if ((err != ENETUNREACH)
		    || (ipx_make_reachable(target) != 0)
		    || ((err =
			 do_ncp_call(conn, NCP_ALLOC_SLOT_REQUEST, 1, &zero, 1)) != 0)) {
			close(ncp_sock);
			close(wdog_sock);
			return err;
		}
	}
	if (wdog_needed != 0) {
		install_wdog(conn);
	}

	err = ncp_finish_connect(conn);
	if (err) {
		return err;
	}
	move_conn_to_kernel(conn);
	return 0;
}
#endif	/* CONFIG_NATIVE_IPX */

#ifdef CONFIG_NATIVE_IP
static long
ncp_connect_in_addr(struct ncp_conn *conn, const struct sockaddr_in *target,
		    UNUSED(int wdog_needed))
{
	static const unsigned char zero = 0;
	struct sockaddr_in addr;
	socklen_t addrlen;

	int ncp_sock;
	long err;

	conn->ncp_reply_buffer = (char*)malloc(NCP_PACKET_SIZE);
	if (!conn->ncp_reply_buffer) {
		return ENOMEM;
	}
	conn->ncp_reply_alloc = NCP_PACKET_SIZE;

	conn->is_connected = NOT_CONNECTED;
	conn->verbose = 0;

	if ((ncp_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		return errno;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0x0);
	addr.sin_addr.s_addr = INADDR_ANY;

	addrlen = sizeof(addr);

	if (bind(ncp_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		return saved_errno;
	}

	conn->ncp_sock = ncp_sock;
	conn->wdog_sock = -1;

	conn->sequence = 0;
	memcpy(&conn->addr, target, sizeof(*target));

	conn->nt = NT_UDP;

	if (connect(ncp_sock, (const struct sockaddr*)target, sizeof(*target)) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		return saved_errno;
	}

	conn->i.connection = ~0;

	if ((err = do_ncp_call(conn, NCP_ALLOC_SLOT_REQUEST, 1, &zero, 1)) != 0) {
		/* ? request route ? */
		close(ncp_sock);
		return err;
	}

	err = ncp_finish_connect(conn);
	if (err) {
		return err;
	}
	move_conn_to_kernel(conn);
	return 0;
}

static long
ncp_connect_tcp_addr(struct ncp_conn *conn, const struct sockaddr_in *target,
		     UNUSED(int wdog_needed))
{
	static const unsigned char connreq[19] = { 1, 16, 0, 0 };
	struct sockaddr_in addr;
	socklen_t addrlen;

	int ncp_sock;
	long err;

	conn->is_connected = NOT_CONNECTED;
	conn->verbose = 0;

	if ((ncp_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		return errno;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0x0);
	addr.sin_addr.s_addr = INADDR_ANY;

	addrlen = sizeof(addr);

	if (bind(ncp_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		return saved_errno;
	}

	conn->ncp_sock = ncp_sock;
	conn->wdog_sock = -1;

	conn->sequence = 0;
	memcpy(&conn->addr, target, sizeof(*target));

	conn->nt = NT_TCP;

	if (connect(ncp_sock, (const struct sockaddr*)target, sizeof(*target)) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		return saved_errno;
	}

	conn->i.connection = ~0;
	
	if ((err = do_ncp_tcp_call(conn, NCP_ALLOC_SLOT_REQUEST, 1, connreq, sizeof(connreq))) != 0) {
		/* ? request route ? */
		close(ncp_sock);
		return err;
	}

	err = ncp_finish_connect(conn);
	if (err) {
		return err;
	}
	move_conn_to_kernel(conn);
	return 0;
}
#endif	/* CONFIG_NATIVE_IP */

#ifdef CONFIG_NATIVE_UNIX
static long
ncp_connect_un_addr(struct ncp_conn *conn, const struct sockaddr_un *target,
		 int wdog_needed)
{
	static const unsigned char zero = 0;
	struct sockaddr_un addr;

	int fd;
	int ncp_sock;
	long err;

	conn->is_connected = NOT_CONNECTED;
	conn->verbose = 0;

	if ((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) == -1) {
		return errno;
	}
	addr.sun_family = AF_UNIX;

	if (bind(fd, (struct sockaddr *) &addr, 2) == -1) {
		int saved_errno = errno;
		close(fd);
		return saved_errno;
	}
	
	memcpy(addr.sun_path, "\000ncpfs", 6);
	if (connect(fd, (struct sockaddr *) &addr, 8) == -1) {
		int saved_errno = errno;
		close(fd);
		return saved_errno;
	}
	{
		unsigned char buffer = 'Q';
		if (send(fd, &buffer, 1, MSG_DONTWAIT) != 1) {
			int saved_errno = errno;
			close(fd);
			return saved_errno;
		}
	}
	{
		unsigned char buffer[100];
		struct msghdr msg;
		struct cmsghdr* cmsg;
		struct iovec io[1];
		unsigned char ctrl[1024];
		int ret;
		
		io[0].iov_base = buffer;
		io[0].iov_len = sizeof(buffer);
		msg.msg_namelen = 0;
		msg.msg_name = NULL;
		msg.msg_iov = io;
		msg.msg_iovlen = 1;
		msg.msg_control = ctrl;
		msg.msg_controllen = sizeof(ctrl);
		
		ret = recvmsg(fd, &msg, 0);
		if (ret < 0) {
			int saved_errno = errno;
			close(fd);
			return saved_errno;
		}
		close(fd);
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level == SOL_SOCKET 
			 && cmsg->cmsg_type == SCM_RIGHTS 
			 && cmsg->cmsg_len >= CMSG_LEN(sizeof(int)))
			 	break;
		}
		if (!cmsg)
			return EACCES;
		ncp_sock = *(int*)(CMSG_DATA(cmsg));
		if (ncp_sock == -1)
			return EINVAL;
	}
#if 0
	{
		int val = 1;
		
		if (setsockopt(ncp_sock, SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) == -1) {
			int saved_errno = errno;
			close(ncp_sock);
			return saved_errno;
		}
	}
#endif

	conn->ncp_sock = ncp_sock;
	conn->wdog_sock = -1;

	conn->sequence = 0;
	memset(&conn->addr, 0, sizeof(conn->addr));

#if 0
	memcpy(addr.sun_path, "\000ncpfs", 6);
	if (connect(ncp_sock, (const struct sockaddr*)&addr, 8) == -1) {
		int saved_errno = errno;
		close(ncp_sock);
		return saved_errno;
	}
#endif

	conn->i.connection = ~0;

	if ((err = do_ncp_call(conn, NCP_ALLOC_SLOT_REQUEST, 1, &zero, 1)) != 0) {
		/* ? request route ? */
		close(ncp_sock);
		return err;
	}

	err = ncp_finish_connect(conn);
	if (err) {
		return err;
	}
	return 0;
}
#endif	/* CONFIG_NATIVE_UNIX */

static long
ncp_connect_addr(struct ncp_conn *conn, const struct sockaddr *target,
		 int wdog_needed, nuint transport) {
	switch (target->sa_family) {
#ifdef CONFIG_NATIVE_IPX
		case AF_IPX:
			if (transport == NT_IPX) {
				return ncp_connect_ipx_addr(conn, (const struct sockaddr_ipx*)target, wdog_needed);
			}
			break;
#endif	/* CONFIG_NATIVE_IPX */
#ifdef CONFIG_NATIVE_IP
		case AF_INET:
			switch (transport) {
				case NT_TCP:
					return ncp_connect_tcp_addr(conn, (const struct sockaddr_in*)target, wdog_needed);
				case NT_UDP:
					return ncp_connect_in_addr(conn, (const struct sockaddr_in*)target, wdog_needed);
			}
			break;
#endif	/* CONFIG_NATIVE_IP */
#ifdef CONFIG_NATIVE_UNIX
		case AF_UNIX:
			return ncp_connect_un_addr(conn, (const struct sockaddr_un*)target, wdog_needed);
#endif	/* CONFIG_NATIVE_UNIX */
		default:
			break;
	}
	return NWE_UNSUPPORTED_TRAN_TYPE;
}

static long
ncp_connect_any(NWCONN_HANDLE *conn, UNUSED(int wdog_needed))
{
	return NWCCOpenConnByName(NULL, NULL, NWCC_NAME_FORMAT_BIND, 0, NWCC_RESERVED, conn);
}

long 
ncp_find_fileserver(const char *server_name, struct sockaddr* addr, socklen_t len) {
	return ncp_find_server(&server_name, OT_FILE_SERVER, addr, len);
}

#ifdef NDS_SUPPORT
static NWCCODE
ncp_login_nds(struct ncp_conn* conn, const char* object_name, const char* password) {
	NWCCODE err;
	
	err = NWE_NCP_NOT_SUPPORTED;
	if (NWIsDSServer(conn, NULL)) {
		err = nds_login_auth(conn, object_name, password);
		if (!err) return 0;
		if (err == NWE_PASSWORD_EXPIRED) {
			fprintf(stderr, _("Your password has expired\n"));
			return 0;
		}
	}
	return err;
}
#endif

NWCCODE
ncp_login_conn(struct ncp_conn* conn, const char* object_name, NWObjectType object_type, const char* password) {
	NWCCODE err;
	char* auth;
	
	auth = cfgGetItem("Requester", "NetWare Protocol");
	if (auth) {
		char* ptr = auth;
		char* curr;
		
		err = NWE_UNSUPPORTED_AUTHENTICATOR;
		while ((curr = strsep(&ptr, " \t,")) != NULL) {
			if (!strcasecmp(curr, "BIND")) {
				err = ncp_login_object(conn, object_name, object_type, password);
#ifdef NDS_SUPPORT
			} else if (!strcasecmp(curr, "NDS")) {
				err = ncp_login_nds(conn, object_name, password);
#endif
			} else {
				/* Leave error code as is... */
			}
			if (!err) {
				break;
			}
		}
		free(auth);
	} else {
#ifdef NDS_SUPPORT
		err = ncp_login_nds(conn, object_name, password);
		if (!err) {
			return 0;
		}
#endif
		err = ncp_login_object(conn, object_name, object_type, password);
	}
	return err;
}

static long
ncp_open_temporary(NWCONN_HANDLE *conn, const char* server) {
	return NWCCOpenConnByName(NULL, server, NWCC_NAME_FORMAT_BIND, 0, NWCC_RESERVED, conn);
}

static long
ncp_open_temporary2(struct ncp_conn **conn,
		     const char* address) {
	/* Ask only for IPv4 name resolve... */
	return NWCCOpenConnByName(NULL, address, NWCC_NAME_FORMAT_BIND, 0, NWCC_RESERVED, conn);
}

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
static int ncp_get_fs_info(int fd, struct ncp_fs_info_v2* info) {
	int err;
	struct ncp_fs_info i;

	info->version = NCP_GET_FS_INFO_VERSION_V2;
	err = ioctl(fd, NCP_IOC_GET_FS_INFO_V2, info);
	if (err != -1) {
		/* Kernel uses dirEntNum as seen by protocol (as an opaque 4 byte entity), 
		   while userspace uses host byteorder */
		info->directory_id = DVAL_LH(&info->directory_id, 0);
		return err;
	}
	err = errno;
	if (err != EINVAL)
		return err;

	i.version = NCP_GET_FS_INFO_VERSION;
	err = ioctl(fd, NCP_IOC_GET_FS_INFO, &i);
	if (err != -1) {
		info->version = NCP_GET_FS_INFO_VERSION_V2;
		info->mounted_uid = i.mounted_uid;
		info->connection = i.connection;
		info->buffer_size = i.buffer_size;
		info->volume_number = i.volume_number;
		/* Kernel uses dirEntNum as seen by protocol (as an opaque 4 byte entity), 
		   while userspace uses host byteorder */
		info->directory_id = DVAL_LH(&i.directory_id, 0);
		info->dummy1 = info->dummy2 = info->dummy3 = 0;
		return err;
	}
	return errno; 
}
#endif

char *
ncp_find_permanent(const struct ncp_conn_spec *spec)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	FILE *mtab;
	struct ncp_conn_ent *conn_ent;
	char *result = NULL;
	int mount_fid;
	struct ncp_fs_info_v2 i;

	if ((mtab = fopen(MOUNTED, "r")) == NULL) {
		return NULL;
	}
	while ((conn_ent = ncp_get_conn_ent(mtab)) != NULL) {
		if (spec != NULL) {
			if (conn_ent->uid != spec->uid) {
				continue;
			}
			if (spec->server[0] == '/') {
				if (strcmp(conn_ent->mount_point, spec->server)) {
					continue;
				}
			} else {
				if (spec->server[0] &&
				    strcasecmp(conn_ent->server, spec->server)) {
					continue;
				}
				if (spec->user[0] &&
				    strcasecmp(conn_ent->user, spec->user)) {
				    	continue;
				}
			}
		}
		mount_fid = open(conn_ent->mount_point, O_RDONLY, 0);
		if (mount_fid < 0) {
			continue;
		}
		if (ncp_get_fs_info(mount_fid, &i)) {
			close(mount_fid);
			continue;
		}
		close(mount_fid);
		result = conn_ent->mount_point;
		break;
	}

	fclose(mtab);
	errno = (result == NULL) ? ENOENT : 0;
	return result;
#else
	return NULL;
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
#ifdef SIGNATURES
static void
ncp_sign_init_perm(struct ncp_conn *conn)
{
	if (ioctl(conn->mount_fid, NCP_IOC_SIGN_WANTED, 
	          &conn->sign_wanted) != 0)
		conn->sign_wanted = 0;
	conn->sign_active = 0;
}
#endif
#endif

static int
ncp_open_permanent(const struct ncp_conn_spec *spec, struct ncp_conn** conn)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	char *mount_point;

	if ((mount_point = ncp_find_permanent(spec)) == NULL) {
		return -1;
	}
	return ncp_open_mount(mount_point, conn);
#else
	return -1;
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

static NWCCODE
ncp_open_2(struct ncp_conn **conn, const struct ncp_conn_spec *spec, const char* address)
{
	struct ncp_conn *result;
	static int ncp_domain_bound = 0;
	NWCCODE err;
	
	if (!ncp_domain_bound) {
		bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
		ncp_domain_bound = 1;
	}

	if (ncp_open_permanent(spec, conn) == 0) {
		return 0;
	}

	if (spec) {
		err = NWE_REQUESTER_FAILURE;
		if (address) {
			err = ncp_open_temporary2(&result, address);
		}
		if (err)
			err = ncp_open_temporary(&result, spec->server);
	} else {
		err = ncp_connect_any(&result, 1);
	}
	if (err) {
		return err;
	}
	if (spec && (strlen(spec->user) != 0)) {
		err = ncp_login_conn(result, spec->user, spec->login_type, spec->password);
		if (err) {
			ncp_close(result);
			return err;
		}
		result->user = strdup(spec->user);
	}
	*conn = result;
	return 0;
}

struct ncp_conn *
ncp_open(const struct ncp_conn_spec *spec, long *err) {
	NWCCODE nwerr;
	struct ncp_conn* conn;
	
	nwerr = ncp_open_2(&conn, spec, NULL);
	*err = nwerr;
	if (nwerr)
		return NULL;
	return conn;
}

static int ncp_do_open_fd(int fd, struct ncp_conn** conn) {
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	struct ncp_conn *result;
	size_t klen;

	*conn = NULL;

	result = ncp_alloc_conn();
	if (result == NULL) {
		return ENOMEM;
	}
	result->mount_fid = fd;
	result->is_connected = CONN_PERMANENT;

	/* on kernels <= 2.2.10 & 2.3.8 directory_id can be set incorrectly */
	/* do stat(mount_point) before if you want to use it... */
	if (ncp_get_fs_info(result->mount_fid, &(result->i))) {
		free(result);
		return errno;
	}

#ifdef SIGNATURES
	ncp_sign_init_perm(result);
#endif	
	if (!ncp_get_private_key(result, NULL, &klen)) {
		if (klen > 10) {
			result->connState |= CONNECTION_AUTHENTICATED;
		}
	}
	result->bcast_state = NWCC_BCAST_PERMIT_UNKNOWN;
	*conn = result;
	return 0;
#else
	return ENOPKG;	/* better error code, anyone? */
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

int ncp_open_fd(int fd, struct ncp_conn** conn) {
	int fd2;
	int err;

	fd2 = dup(fd);
	if (fd2 == -1)
		return errno;
	err = ncp_do_open_fd(fd2, conn);
	if (err) 
		close(fd2);
	return err;
}

int
ncp_open_mount(const char *mount_point, struct ncp_conn** conn)
{
	int fd;
	int err;
	
	fd = open(mount_point, O_RDONLY, 0);
	if (fd == -1)
		return errno;
	err = ncp_do_open_fd(fd, conn);
	if (err) {
		close(fd);
		return err;
	}
	/* returning NULL is not critical */
	(*conn)->mount_point = strdup(mount_point);
	return 0;
}

static long
ncp_user_disconnect(struct ncp_conn *conn)
{
	static const unsigned char zero = 0;
	long result;

	switch (conn->nt) {
		case NT_IPX:
		case NT_UDP:
			result = do_ncp_call(conn, NCP_DEALLOC_SLOT_REQUEST, 1, &zero, 1);
			break;
		case NT_TCP:
			result = do_ncp_tcp_call(conn, NCP_DEALLOC_SLOT_REQUEST, 1, &zero, 1);
			break;
		default:
			result = ECONNABORTED;
			break;
	}
	if (result) {
		return result;
	}
	close(conn->ncp_sock);
	if (conn->wdog_sock != -1) {
		close(conn->wdog_sock);
	}

	if (conn->wdog_pipe != -1) {
		unsigned char dummy[1];
		int res;

		res = send(conn->wdog_pipe, "Q", 1, MSG_NOSIGNAL);
		/* If write failed, we should not wait for child */
		if (res == 1) {
			read(conn->wdog_pipe, dummy, 1);
		}
		close(conn->wdog_pipe);
	}
	return 0;
}

static long
ncp_do_close(struct ncp_conn *conn)
{
	long result;

	switch (conn->is_connected) {
	case CONN_PERMANENT:
		if (conn->global_fd != -1)
			close(conn->global_fd);
		result = close(conn->mount_fid);
		ncpt_mutex_lock(&nds_ring_lock);
		list_del(&conn->nds_ring);
		conn->nds_conn = NULL;
		ncpt_mutex_unlock(&nds_ring_lock);
		/* call NWDSReleaseDSConnection... */
		conn->state++;
		break;

	case CONN_TEMPORARY:
		if (conn->global_fd != -1)
			close(conn->global_fd);
		result = ncp_user_disconnect(conn);
		ncpt_mutex_lock(&nds_ring_lock);
		list_del(&conn->nds_ring);
		conn->nds_conn = NULL;
		ncpt_mutex_unlock(&nds_ring_lock);
		/* call NWDSReleaseDSConnection... */
		conn->state++;
		break;

	case CONN_KERNELBASED:
		if (conn->global_fd != -1)
			close(conn->global_fd);
		result = close(conn->mount_fid);
		ncpt_mutex_lock(&nds_ring_lock);
		list_del(&conn->nds_ring);
		conn->nds_conn = NULL;
		ncpt_mutex_unlock(&nds_ring_lock);
		/* call NWDSReleaseDSConnection... */
		conn->state++;
		break;

	case NOT_CONNECTED:
		result = 0;
		break;
		
	default:
		result = -1;
		break;
	}
	conn->is_connected = NOT_CONNECTED;

	if (ncpt_atomic_read(&conn->store_count))
		return 0;
	ncpt_mutex_lock(&conn_lock);
	list_del(&conn->conn_ring);
	ncpt_mutex_unlock(&conn_lock);
	if (conn->mount_point) {
		free(conn->mount_point);
		conn->mount_point = NULL;
	}
	if (conn->serverInfo.serverName) {
		free(conn->serverInfo.serverName);
		conn->serverInfo.serverName = NULL;
	}
	if (conn->user) {
		free(conn->user);
		conn->user = NULL;	/* To be safe */
	}
	if (conn->ncp_reply_buffer) {
		free(conn->ncp_reply_buffer);
		conn->ncp_reply_buffer = NULL;
		conn->ncp_reply_alloc = 0;
	}
	if (conn->private_key) {
		free(conn->private_key);
		conn->private_key = NULL;
		conn->private_key_len = 0;
	}
	ncpt_mutex_destroy(&conn->serverInfo.mutex);
	ncpt_mutex_destroy(&conn->buffer_mutex);
	free(conn);
	return result;
}

long
ncp_conn_release(struct ncp_conn *conn) {
	if (!ncpt_atomic_dec_and_test(&conn->store_count))
		return 0;
	if (ncpt_atomic_read(&conn->use_count))
		return 0;
	return ncp_do_close(conn);
}

long
ncp_close(struct ncp_conn *conn)
{
	if (conn == NULL) {
		return 0;
	}
	if (ncpt_atomic_read(&conn->use_count)) {
		if (!ncpt_atomic_dec_and_test(&conn->use_count))
			return 0;
		return ncp_do_close(conn);
	} else
		return NWE_REQUESTER_FAILURE;	/* buggg ! */
}

int 
ncp_get_mount_uid(int fid, uid_t* uid)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	unsigned long k2_uid;
	__kernel_uid_t k_uid;
	int err;

	err = ioctl(fid, NCP_IOC_GETMOUNTUID2, &k2_uid);
	if (!err) {
		*uid = k2_uid;
		return 0;
	}
	if (errno != -EINVAL)
		return err;
	err = ioctl(fid, NCP_IOC_GETMOUNTUID, &k_uid);
	if (err)
		return err;
	*uid = k_uid;
	return 0;
#else
	errno = ENOPKG;
	return -1;
#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */
}

struct ncp_conn_ent *
ncp_get_conn_ent(FILE * filep)
{
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	static struct ncp_conn_ent entry;
	static char server[2 * NCPFS_MAX_CFG_USERNAME];
	char *user;
	struct mntent *mnt_ent;
	int fid;

	memset(server, 0, sizeof(server));
	memset(&entry, 0, sizeof(entry));

	while ((mnt_ent = getmntent(filep)) != NULL) {
		if (strcmp(mnt_ent->mnt_type, "ncpfs") &&
		    strcmp(mnt_ent->mnt_type, "ncp")) {
			continue;
		}
		if (strlen(mnt_ent->mnt_fsname) >= sizeof(server)) {
			continue;
		}
		strcpy(server, mnt_ent->mnt_fsname);
		user = strchr(server, '/');
		if (user == NULL) {
			continue;
		}
		*user = '\0';
		user += 1;
		entry.user = user;
		if ((strlen(server) >= sizeof(entry.server))
		    || (strlen(mnt_ent->mnt_dir) >= sizeof(entry.mount_point))) {
			continue;
		}
		strcpy(entry.server, server);
		strcpy(entry.mount_point, mnt_ent->mnt_dir);

		fid = open(entry.mount_point, O_RDONLY, 0);

		if (fid == -1) {
			continue;
		}
		if (ncp_get_mount_uid(fid, &entry.uid) != 0) {
			close(fid);
			continue;
		}
		close(fid);
		return &entry;
	}
#else
	errno = ENOPKG;
#endif
	return NULL;
}

static struct ncp_conn_spec *
ncp_get_nwc_ent(FILE * nwc)
{
	static struct ncp_conn_spec spec;
	char line[512];
	int line_len;
	char *user;
	char *password;

	memset(&spec, 0, sizeof(spec));
	spec.uid = getuid();

	while (fgets(line, sizeof(line), nwc) != NULL) {
		if ((line[0] == '\n')
		    || (line[0] == '#')) {
			continue;
		}
		line_len = strlen(line);
		if (line[line_len - 1] == '\n') {
			line[line_len - 1] = '\0';
		}
		user = strchr(line, '/');
		password = strchr(user != NULL ? user : line, ' ');

		if (password != NULL) {
			*password = '\0';
			password += 1;
		}
		if (user != NULL) {
			*user = '\0';
			user += 1;
			if (strlen(user) >= sizeof(spec.user)) {
				continue;
			}
			strcpy(spec.user, user);
		}
		if (strlen(line) >= sizeof(spec.server)) {
			continue;
		}
		strcpy(spec.server, line);

		if (password != NULL) {
			while (*password == ' ') {
				password += 1;
			}

			if (strlen(password) >= sizeof(spec.password)) {
				continue;
			}
			strcpy(spec.password, password);
		}
		return &spec;
	}
	return NULL;
}

static NWCCODE
ncp_fopen_nwc(FILE** nwc)
{
	char path[MAXPATHLEN];
	char *home = NULL;
	struct stat st;
	FILE* f;

	home = getenv("HOME");
	if ((home == NULL)
	    || (strlen(home) + sizeof(NWCLIENT) + 2 > sizeof(path))) {
		return ENAMETOOLONG;
	}
	strcpy(path, home);
	strcat(path, "/");
	strcat(path, NWCLIENT);

	if (stat(path, &st) != 0) {
		return errno;
	}
        if (st.st_uid != getuid()) {
                return EACCES;
        }
	if ((st.st_mode & (S_IRWXO | S_IRWXG)) != 0) {
		return NCPLIB_INVALID_MODE;
	}
	f = fopen(path, "r");
	if (!f) {
		return errno;
	}
	*nwc = f;
	return 0;
}

NWCCODE
ncp_find_conn_spec3(const char *server, const char *user, const char *password,
		    int login_necessary, uid_t uid, int allow_multiple_conns,
		    struct ncp_conn_spec *spec)
{
	FILE *nwc;
	struct ncp_conn_spec *nwc_ent;

	if (!spec) {
		return ERR_NULL_POINTER;
	}
	memset(spec, 0, sizeof(*spec));
	spec->uid = uid;

	if (server != NULL) {
		if (strlen(server) >= sizeof(spec->server)) {
			return ENAMETOOLONG;
		}
		strcpy(spec->server, server);
	} else {
		if (ncp_fopen_nwc(&nwc)) {
			return NWE_SERVER_UNKNOWN;
		}
		nwc_ent = ncp_get_nwc_ent(nwc);
		fclose(nwc);

		if (nwc_ent == NULL) {
			return NWE_SERVER_NO_CONN;
		}
		strcpy(spec->server, nwc_ent->server);
		strcpy(spec->user, nwc_ent->user);
	}

	if (login_necessary == 0) {
		memset(spec->user, 0, sizeof(spec->user));
		memset(spec->password, 0, sizeof(spec->password));
		return 0;
	}
	if (user != NULL) {
		if (strlen(user) >= sizeof(spec->user)) {
			return ENAMETOOLONG;
		}
		strcpy(spec->user, user);
	}
	str_upper(spec->user);
	spec->login_type = NCP_BINDERY_USER;

	if (!allow_multiple_conns) {
		struct ncp_conn* conn;

		if (ncp_open_permanent(spec, &conn) == 0) {
			ncp_close(conn);
			if (login_necessary &&
			    !(conn->connState & CONNECTION_AUTHENTICATED)) {
				return NWE_USER_NO_NAME;
			}
			return 0;
		}
	}
	if (password != NULL) {
		if (strlen(password) >= sizeof(spec->password)) {
			return ENAMETOOLONG;
		}
		strcpy(spec->password, password);
	} else {
		if (!ncp_fopen_nwc(&nwc)) {
			while ((nwc_ent = ncp_get_nwc_ent(nwc)) != NULL) {
				if ((strcasecmp(spec->server,
						nwc_ent->server) != 0)
				    || ((spec->user[0] != '\0')
					&& (strcasecmp(spec->user,
						       nwc_ent->user) != 0))) {
					continue;
				}
				strcpy(spec->user, nwc_ent->user);
				strcpy(spec->password, nwc_ent->password);
				break;
			}
			fclose(nwc);
		}
	}

	if (spec->user[0] == 0) {
		if (login_necessary == 1)
			return NWE_USER_NO_NAME;
		spec->password[0] = 0;
		return 0;
	}
	if ((spec->password[0] == 0) && (password == NULL)) {
		char *pwd;
		if (!(isatty(0) && isatty(1))) {
			return NCPLIB_PASSWORD_REQUIRED;
		}
		printf(_("Logging into %s as %s\n"),
		       spec->server, spec->user);

		pwd = getpass(_("Password: "));
		if (strlen(pwd) >= sizeof(spec->password)) {
			return ENAMETOOLONG;
		}
		strcpy(spec->password, pwd);
	} else {
		if (strcmp(spec->password, NWC_NOPASSWORD) == 0) {
			spec->password[0] = '\0';
		}
	}

	str_upper(spec->server);
	str_upper(spec->user);
	str_upper(spec->password);
	return 0;
}

struct ncp_conn_spec *
ncp_find_conn_spec2(const char *server, const char *user, const char *password,
		   int login_necessary, uid_t uid, int allow_multiple_conns, long *err)
{
	static struct ncp_conn_spec spec;
	NWCCODE nwerr;
	
	nwerr = ncp_find_conn_spec3(server, user, password, login_necessary, uid,
				  allow_multiple_conns, &spec);
	*err = nwerr;
	if (!nwerr)
		return &spec;
	return NULL;
}

struct ncp_conn_spec *
ncp_find_conn_spec(const char *server, const char *user, const char *password,
		   int login_necessary, uid_t uid, long *err) {
	return ncp_find_conn_spec2(server, user, password, login_necessary,
				  uid, 0, err);
}

struct ncp_conn *
ncp_initialize_2(int *argc, char **argv, int login_necessary, 
		 int login_type, long *err, int required)
{
	const char *server = NULL;
	const char *user = NULL;
	const char *password = NULL;
	const char *address = NULL;
	struct ncp_conn_spec spec;
	struct ncp_conn *conn;
	int i = 1;
	NWCCODE nwerr;

	int get_argument(int arg_no, const char **target) {
		int count = 1;

		if (target != NULL) {
			if (arg_no + 1 >= *argc) {
				/* No argument to switch */
				errno = EINVAL;
				return -1;
			}
			*target = argv[arg_no + 1];
			count = 2;
		}
		/* Delete the consumed switch from the argument list
		   and decrement the argument count */
		while (count + arg_no < *argc) {
			argv[arg_no] = argv[arg_no + count];
			arg_no += 1;
		}
		*argc -= count;
		return 0;
	}

	*err = EINVAL;

	while (i < *argc) {
		if ((argv[i][0] != '-')
		    || (strlen(argv[i]) != 2)) {
			i += 1;
			continue;
		}
		switch (argv[i][1]) {
		case 'S':
			if (get_argument(i, &server) != 0) {
				return NULL;
			}
			continue;
		case 'U':
			if (get_argument(i, &user) != 0) {
				return NULL;
			}
			continue;
		case 'P':
			if (get_argument(i, &password) != 0) {
				return NULL;
			}
			if (password) {
				char* opw = (char*)password;
				password = strdup(password);
				memset(opw, 0, strlen(opw));
			}
			continue;
		case 'n':
			if (get_argument(i, NULL) != 0) {
				return NULL;
			}
			password = NWC_NOPASSWORD;
			continue;
#ifdef NDS_SUPPORT
		case 'b':
			if (get_argument(i, NULL) != 0) {
				return NULL;
			}
			bindery_only = 1;
			continue;
#endif
#ifdef CONFIG_NATIVE_IP
		case 'A':
			if (get_argument(i, &address) != 0) {
				return NULL;
			}
			continue;
#endif	/* CONFIG_NATIVE_IP */
		}
		i += 1;
	}

	if ((!required) && !(server || user || password || address))
		return NULL;
	nwerr = ncp_find_conn_spec3(server, user, password, login_necessary,
				  getuid(), 0, &spec);

	if (nwerr) {
		*err = nwerr;
		if (login_necessary == 1) {
			return NULL;
		}
		return ncp_open(NULL, err);
	}
	spec.login_type = login_type;

	if (login_necessary == 0) {
		spec.user[0] = '\0';
	}
	nwerr = ncp_open_2(&conn, &spec, address);
	*err = nwerr;
	if (nwerr)
		return NULL;
	return conn;
}

struct ncp_conn *
ncp_initialize_as(int *argc, char **argv, int login_necessary,
		  int login_type, long *err)
{
	return ncp_initialize_2(argc, argv, login_necessary,
		  login_type, err, 1);
}

struct ncp_conn *
ncp_initialize(int *argc, char **argv,
	       int login_necessary, long *err)
{
	return ncp_initialize_as(argc, argv, login_necessary,
				 NCP_BINDERY_USER, err);
}

long
ncp_request(struct ncp_conn *conn, int function)
{
	long result = ENOTCONN;
#ifdef NCP_TRACE_ENABLE
	{
		char *ppacket = conn->packet + sizeof(struct ncp_request_header);
		if (conn->has_subfunction) {
			NCP_TRACE("ncp_request: fn=%d subfn=%d", function, (int) ppacket[2]);
		} else {
			NCP_TRACE("ncp_request: fn=%d subfn?%d", function, (int) *ppacket);
		}
		DUMP_HEX("ncp_request: packet=", ppacket, ncp_packet_size(conn));
	}
#endif /*def NCP_TRACE_ENABLE */
	switch (conn->is_connected) {
	case CONN_PERMANENT:
		result = ncp_mount_request(conn, function);
		break;
	case CONN_TEMPORARY:
		result = ncp_temp_request(conn, function);
		break;
	case CONN_KERNELBASED:
		result = ncp_kernel_request(conn, function);
		break;
	default:
		break;
	}
#ifdef NCP_TRACE_ENABLE
	NCP_TRACE("ncp_request: reply %ld", result);
	if (result == 0) {
		DUMP_HEX("ncp_request->reply=",
			 conn->ncp_reply + sizeof(struct ncp_reply_header),
			 conn->ncp_reply_size);
	}
#endif /*def NCP_TRACE_ENABLE */
	return result;
}

/****************************************************************************/
/*                                                                          */
/* Helper functions                                                         */
/*                                                                          */
/****************************************************************************/

struct nw_time_buffer
{
	u_int8_t year;
	u_int8_t month;
	u_int8_t day;
	u_int8_t hour;
	u_int8_t minute;
	u_int8_t second;
	u_int8_t wday;
};

static time_t
nw_to_ctime(struct nw_time_buffer *source)
{
	struct tm u_time;

	memset(&u_time, 0, sizeof(u_time));
	u_time.tm_sec = source->second;
	u_time.tm_min = source->minute;
	u_time.tm_hour = source->hour;
	u_time.tm_mday = source->day;
	u_time.tm_mon = source->month - 1;
	u_time.tm_year = source->year;

	if (u_time.tm_year < 80) {
		u_time.tm_year += 100;
	}
	return mktime(&u_time);
}

void ncp_add_pstring(struct ncp_conn *conn, const char *s) {
	int len = strlen(s);
	assert_conn_locked(conn);
	if (len > 255) {
		ncp_printf(_("ncpfs: string too long: %s\n"), s);
		len = 255;
	}
	ncp_add_byte(conn, len);
	ncp_add_mem(conn, s, len);
	return;
}

void
ncp_init_request(struct ncp_conn *conn)
{
	ncp_lock_conn(conn);

	conn->current_point = conn->packet + sizeof(struct ncp_request_header);
	conn->has_subfunction = 0;
}

void
ncp_init_request_s(struct ncp_conn *conn, int subfunction)
{
	ncp_init_request(conn);
	ncp_add_word_lh(conn, 0);	/* preliminary size */

	ncp_add_byte(conn, subfunction);

	conn->has_subfunction = 1;
}

/* Here the ncp calls begin
 */

static long
ncp_negotiate_buffersize(struct ncp_conn *conn,
			 size_t size, size_t *target)
{
	NWCCODE err;
	nuint8 rq[2];
	nuint8 buf[2];
	NW_FRAGMENT rp;
	
	WSET_HL(rq, 0, size);
	rp.fragAddress = buf;
	rp.fragSize = 2;
	
	err = NWRequestSimple(conn, 33, rq, 2, &rp);
	if (err)
		return err;
	if (rp.fragSize < 2)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (target)
		*target = min(WVAL_HL(buf, 0), size);
	return 0;
}

#ifdef SIGNATURES
static NWCCODE ncp_negotiate_size_and_options(struct ncp_conn *conn,
			  size_t size, int options, 
			  size_t *ret_size, int *ret_options)
{
	long result;

	ncp_init_request(conn);
	if (options & 2) {
		ncp_add_word_hl(conn, size + 8);
	} else {
		ncp_add_word_hl(conn, size);
	}
	ncp_add_byte(conn, options);

	if ((result = ncp_request(conn, 0x61)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 5) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (ncp_reply_word_hl(conn, 0) == 0) 
		*ret_size = size;
	else 
		*ret_size = min(ncp_reply_word_hl(conn, 0), size);
	*ret_options = ncp_reply_byte(conn, 4);

	ncp_unlock_conn(conn);
	return 0;
}
#endif

long
ncp_get_file_server_description_strings(struct ncp_conn *conn,
					char target[512])
{
	NW_FRAGMENT rp;
	
	if (!target)
		return NWE_PARAM_INVALID;
		
	rp.fragAddress = target;
	rp.fragSize = 512;
	return NWRequestSimple(conn, NCPC_SFN(23,201), NULL, 0, &rp);
}

long
ncp_get_file_server_time(struct ncp_conn *conn, time_t * target)
{
	NWCCODE result;
	struct nw_time_buffer buf;
	NW_FRAGMENT rp;
	
	rp.fragAddress = &buf;
	rp.fragSize = sizeof(buf);
	result = NWRequestSimple(conn, 20, NULL, 0, &rp);
	if (result)
		return result;
	if (rp.fragSize < sizeof(buf))
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (target)
		*target = nw_to_ctime(&buf);
	return 0;
}

long
ncp_set_file_server_time(struct ncp_conn *conn, time_t * source)
{
	int year;
	struct tm *utime = localtime(source);
	nuint8 srvtime[6];
	
	year = utime->tm_year;
	if (year > 99) {
		year -= 100;
	}
	BSET(srvtime, 0, year);
	BSET(srvtime, 1, utime->tm_mon + 1);
	BSET(srvtime, 2, utime->tm_mday);
	BSET(srvtime, 3, utime->tm_hour);
	BSET(srvtime, 4, utime->tm_min);
	BSET(srvtime, 5, utime->tm_sec);
	
	return NWRequestSimple(conn, NCPC_SFN(23,202), srvtime, 6, NULL);
}

long
ncp_get_file_server_information(struct ncp_conn *conn,
				  struct ncp_file_server_info *target)
{
	long result;
	ncp_init_request_s(conn, 17);
	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	memcpy(target, ncp_reply_data(conn, 0), sizeof(*target));
	ncp_unlock_conn(conn);
	target->MaximumServiceConnections
	    = htons(target->MaximumServiceConnections);
	target->ConnectionsInUse
	    = htons(target->ConnectionsInUse);
	target->MaxConnectionsEverUsed
	    = htons(target->MaxConnectionsEverUsed);
	target->NumberMountedVolumes
	    = htons(target->NumberMountedVolumes);
	return 0;
}

NWCCODE
ncp_get_file_server_information_2(struct ncp_conn *conn,
				  struct ncp_file_server_info_2 *target, size_t starget)
{
	NWCCODE result;
	
	switch (starget) {
		case sizeof(struct ncp_file_server_info_2):
			break;
		default:
			return EINVAL;
	}
	ncp_init_request_s(conn, 17);
	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (target) {
		memcpy(target->ServerName, ncp_reply_data(conn, 0), 48);
		target->ServerName[48] = 0;
		target->FileServiceVersion = ncp_reply_byte(conn, 48);
		target->FileServiceSubVersion = ncp_reply_byte(conn, 49);
		target->MaximumServiceConnections = ncp_reply_word_hl(conn, 50);
		target->ConnectionsInUse = ncp_reply_word_hl(conn, 52);
		target->NumberMountedVolumes = ncp_reply_word_hl(conn, 54);
		target->Revision = ncp_reply_byte(conn, 56);
		target->SFTLevel = ncp_reply_byte(conn, 57);
		target->TTSLevel = ncp_reply_byte(conn, 58);
		target->MaxConnectionsEverUsed = ncp_reply_word_hl(conn, 59);
		target->AccountVersion = ncp_reply_byte(conn, 61);
		target->VAPVersion = ncp_reply_byte(conn, 62);
		target->QueueVersion = ncp_reply_byte(conn, 63);
		target->PrintVersion = ncp_reply_byte(conn, 64);
		target->VirtualConsoleVersion = ncp_reply_byte(conn, 65);
		target->RestrictionLevel = ncp_reply_byte(conn, 66);
		target->InternetBridge = ncp_reply_byte(conn, 67);
		target->MixedModePathFlag = ncp_reply_byte(conn, 68);
		target->LocalLoginInfoCcode = ncp_reply_byte(conn, 69);
		target->ProductMajorVersion = ncp_reply_word_hl(conn, 70);
		target->ProductMinorVersion = ncp_reply_word_hl(conn, 72);
		target->ProductRevisionVersion = ncp_reply_word_hl(conn, 74);
		target->OSLanguageID = ncp_reply_byte(conn, 76);
		target->_64BitOffsetsSupportedFlag = ncp_reply_byte(conn, 77);
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_connlist(struct ncp_conn *conn,
		 NWObjectType object_type, const char *object_name,
		 int *returned_no, u_int8_t conn_numbers[256])
{
	long result;
	unsigned int cnlen;

	if (!object_name || !returned_no || !conn_numbers) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 21);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	cnlen = ncp_reply_byte(conn, 0);
	if (conn->ncp_reply_size < 1 + cnlen) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}	
	*returned_no = cnlen;
	memcpy(conn_numbers, ncp_reply_data(conn, 1), cnlen);
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_stations_logged_info(struct ncp_conn *conn,
			     u_int32_t connection,
			     struct ncp_bindery_object *target,
			     time_t * login_time)
{
	long result;
	ncp_init_request_s(conn, 28);
	ncp_add_dword_lh(conn, connection);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 60) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (target) {
		memset(target, 0, sizeof(*target));
		target->object_id = ncp_reply_dword_hl(conn, 0);
		target->object_type = ncp_reply_word_hl(conn, 4);
		memcpy(target->object_name, ncp_reply_data(conn, 6),
	       		sizeof(target->object_name));
		target->object_flags = 0;
		target->object_security = 0;
		target->object_has_prop = 0;
	}
	if (login_time)
		*login_time = nw_to_ctime((struct nw_time_buffer *)
				  ncp_reply_data(conn, 54));
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_internet_address(struct ncp_conn *conn,
			 u_int32_t connection,
			 struct sockaddr *target,
			 u_int8_t * conn_type)
{
	long result;
	u_int8_t ct;
	union ncp_sockaddr* t2 = (union ncp_sockaddr*)target;
	
	if (!target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 26);
	ncp_add_dword_lh(conn, connection);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	memset(target, 0, sizeof(*target));
	ct = ncp_reply_byte(conn, 12);
	if (conn_type)
		*conn_type = ct;
	if (ct == 11) {	/* 11... why 11? Nobody knows... */
#ifdef NCP_IN_SUPPORT
		t2->inet.sin_family = AF_INET;
		memcpy(&(t2->inet.sin_addr.s_addr), ncp_reply_data(conn, 0), 4);
		/* I'm not sure about port... it is always 0 on Netwares I checked */
		memcpy(&(t2->inet.sin_port), ncp_reply_data(conn, 4), 2);
#endif
	} else {	/* 2... IPX, 3... internal network (doc lies Appletalk DDP) */
#ifdef NCP_IPX_SUPPORT
		t2->ipx.sipx_family = AF_IPX;
		memcpy(&(t2->ipx.sipx_network), ncp_reply_data(conn, 0), 4);
		memcpy(&(t2->ipx.sipx_node), ncp_reply_data(conn, 4), 6);
		memcpy(&(t2->ipx.sipx_port), ncp_reply_data(conn, 10), 2);
#endif
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_send_broadcast(struct ncp_conn *conn,
		   u_int8_t no_conn, const u_int8_t * connections,
		   const char *message)
{
	long result;

	if (!message || (no_conn && !connections)) {
		return ERR_NULL_POINTER;
	}
	if (strlen(message) > 58) {
		return NWE_SERVER_FAILURE;
	}
	ncp_init_request_s(conn, 0);
	ncp_add_byte(conn, no_conn);
	ncp_add_mem(conn, connections, no_conn);
	ncp_add_pstring(conn, message);

	result = ncp_request(conn, 21);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_send_broadcast2(struct ncp_conn *conn,
		    unsigned int conns, const unsigned int* connlist, 
		    const char* message)
{
	int i;
	long result;

	if (!message || (conns && !connlist)) {
		return ERR_NULL_POINTER;
	}
	i = strlen(message);
	if (i > 255) {
		return NWE_SERVER_FAILURE;
	}
	if (conns > 350)	/* max pkt len ~ 1KB */
				/* maybe do it by handshaked length ? */
		return NWE_SERVER_FAILURE;
		
	ncp_init_request_s(conn, 0x0A);
	ncp_add_word_lh(conn, conns);
	for (;conns; --conns)
		ncp_add_dword_lh(conn, *connlist++);
	ncp_add_byte(conn, i);
	ncp_add_mem(conn, message, i);
	result = ncp_request(conn, 0x15);
	ncp_unlock_conn(conn);
	return result;
}

/*
 * target is a 8-byte buffer
 */
long
ncp_get_encryption_key(struct ncp_conn *conn,
		       char *target)
{
	NW_FRAGMENT rp;
	NWCCODE err;
	
	if (!target)
		return NWE_PARAM_INVALID;
	rp.fragAddress = target;
	rp.fragSize = 8;
	err = NWRequestSimple(conn, NCPC_SFN(23,23), NULL, 0, &rp);
	if (err)
		return err;
	if (rp.fragSize < 8)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	return 0;
}

long
ncp_get_bindery_object_id(struct ncp_conn *conn,
			  NWObjectType object_type,
			  const char *object_name,
			  struct ncp_bindery_object *target)
{
	long result;
	
	if (!object_name || !target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 53);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 54) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	target->object_id = ncp_reply_dword_hl(conn, 0);
	target->object_type = ncp_reply_word_hl(conn, 4);
	memcpy(target->object_name, ncp_reply_data(conn, 6), 48);
	target->object_flags = 0;
	target->object_security = 0;
	target->object_has_prop = 0;
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_bindery_object_name(struct ncp_conn *conn,
			    u_int32_t object_id,
			    struct ncp_bindery_object *target)
{
	long result;
	
	if (!target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 54);
	ncp_add_dword_hl(conn, object_id);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	target->object_id = ncp_reply_dword_hl(conn, 0);
	target->object_type = ncp_reply_word_hl(conn, 4);
	memcpy(target->object_name, ncp_reply_data(conn, 6),
	       NCP_BINDERY_NAME_LEN);
	target->object_flags = 0;
	target->object_security = 0;
	target->object_has_prop = 0;
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE NWScanObject(NWCONN_HANDLE conn, const char *searchName,
		     NWObjectType searchType, NWObjectID *objID,
		     char objName[NCP_BINDERY_NAME_LEN + 1], NWObjectType *objType,
		     nuint8 *hasPropertiesFlag, nuint8 *objFlags,
		     nuint8 *objSecurity) {
	NWCCODE result;
	
	if (!searchName || !objID) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 55);
	ncp_add_dword_hl(conn, *objID);
	ncp_add_word_hl(conn, searchType);
	ncp_add_pstring(conn, searchName);
	/* fn: 23 , subfn: 55 */
	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	*objID = ncp_reply_dword_hl(conn, 0);
	if (objType) {
		*objType = ncp_reply_word_hl(conn, 4);
	}
	if (objName) {
		memcpy(objName, ncp_reply_data(conn, 6),
		       NCP_BINDERY_NAME_LEN);
		objName[NCP_BINDERY_NAME_LEN] = 0;
	}
	if (hasPropertiesFlag) {
		*hasPropertiesFlag = ncp_reply_byte(conn, 56);
	}
	if (objFlags) {
		*objFlags = ncp_reply_byte(conn, 54);
	}
	if (objSecurity) {
		*objSecurity = ncp_reply_byte(conn, 55);
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_scan_bindery_object(struct ncp_conn *conn,
			NWObjectID last_id, NWObjectType object_type, const char *search_string,
			struct ncp_bindery_object *target)
{
	NWCCODE result;
	nuint8 hasProperties, objFlags, objSecurity;
	
	if (!target) {
		return ERR_NULL_POINTER;
	}
	result = NWScanObject(conn, search_string, object_type, &last_id, 
			target->object_name, &object_type, &hasProperties,
			&objFlags, &objSecurity);
	if (!result) {
		target->object_id = last_id;
		target->object_type = object_type;
		target->object_flags = objFlags;
		target->object_security = objSecurity;
		target->object_has_prop = hasProperties;
	}
	return result;
}

long
ncp_change_object_security(struct ncp_conn *conn,
			   NWObjectType object_type,
			   const char *object_name,
			   u_int8_t new_object_security) {
	long result;
	
	if (!object_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 56);
	ncp_add_byte(conn, new_object_security);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	
	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_create_bindery_object(struct ncp_conn *conn,
			  NWObjectType object_type,
			  const char *object_name,
			  u_int8_t object_security,
			  u_int8_t object_status)
{
	long result;
	
	if (!object_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 50);
	ncp_add_byte(conn, object_status);
	ncp_add_byte(conn, object_security);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}


long
ncp_delete_bindery_object(struct ncp_conn *conn,
			  NWObjectType object_type,
			  const char *object_name)
{
	long result;
	
	if (!object_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 51);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
NWReadPropertyValue(NWCONN_HANDLE conn, const char *objName,
		    NWObjectType objType, const char *propertyName,
		    unsigned int segmentNum, nuint8 *segmentData,
		    nuint8 *moreSegments, nuint8 *flags) {
	NWCCODE result;
	
	if (!objName || !propertyName) {
		return ERR_NULL_POINTER;
	}
	if (segmentNum >= 256) {
		return NWE_PARAM_INVALID;
	}
	ncp_init_request_s(conn, 61);
	ncp_add_word_hl(conn, objType);
	ncp_add_pstring(conn, objName);
	ncp_add_byte(conn, segmentNum);
	ncp_add_pstring(conn, propertyName);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 130) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (segmentData) {
		memcpy(segmentData, ncp_reply_data(conn, 0), 128);
	}
	if (moreSegments) {
		*moreSegments = ncp_reply_byte(conn, 128);
	}
	if (flags) {
		*flags = ncp_reply_byte(conn, 129);
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_read_property_value(struct ncp_conn *conn,
			NWObjectType object_type, const char *object_name,
			int segment, const char *prop_name,
			struct nw_property *target)
{
	nuint8 moreSegments;
	nuint8 flags;
	NWCCODE result;
	
	if (!target) {
		return ERR_NULL_POINTER;
	}
	result = NWReadPropertyValue(conn, object_name, object_type, prop_name,
			segment, target->value, &moreSegments, &flags);
	if (!result) {
		target->more_flag = moreSegments;
		target->property_flag = flags;
	}
	return result;
}

long
ncp_scan_property(struct ncp_conn *conn,
		  NWObjectType object_type, const char *object_name,
		  u_int32_t last_id, const char *search_string,
		  struct ncp_property_info *property_info)
{
	long result;
	
	if (!object_name || !search_string || !property_info) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 60);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_dword_hl(conn, last_id);
	ncp_add_pstring(conn, search_string);

	if ((result = ncp_request(conn, 23)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 24) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	memcpy(property_info->property_name, ncp_reply_data(conn, 0), 16);
	property_info->property_flags = ncp_reply_byte(conn, 16);
	property_info->property_security = ncp_reply_byte(conn, 17);
	property_info->search_instance = ncp_reply_dword_hl(conn, 18);
	property_info->value_available_flag = ncp_reply_byte(conn, 22);
	property_info->more_properties_flag = ncp_reply_byte(conn, 23);
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_add_object_to_set(struct ncp_conn *conn,
		      NWObjectType object_type, const char *object_name,
		      const char *property_name,
		      NWObjectType member_type,
		      const char *member_name)
{
	long result;
	
	if (!object_name || !property_name || !member_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 65);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, property_name);
	ncp_add_word_hl(conn, member_type);
	ncp_add_pstring(conn, member_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_change_property_security(struct ncp_conn *conn,
			     NWObjectType object_type, const char *object_name,
			     const char *property_name,
			     u_int8_t property_security)
{
	long result;
	
	if (!object_name || !property_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 59);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_byte(conn, property_security);
	ncp_add_pstring(conn, property_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_create_property(struct ncp_conn *conn,
		    NWObjectType object_type, const char *object_name,
		    const char *property_name,
		    u_int8_t property_flags, u_int8_t property_security)
{
	long result;
	
	if (!object_name || !property_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 57);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_byte(conn, property_flags);
	ncp_add_byte(conn, property_security);
	ncp_add_pstring(conn, property_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_delete_object_from_set(struct ncp_conn *conn,
			   NWObjectType object_type, const char *object_name,
			   const char *property_name,
			   NWObjectType member_type,
			   const char *member_name)
{
	long result;
	
	if (!object_name || !property_name || !member_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 66);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, property_name);
	ncp_add_word_hl(conn, member_type);
	ncp_add_pstring(conn, member_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_delete_property(struct ncp_conn *conn,
		    NWObjectType object_type, const char *object_name,
		    const char *property_name)
{
	long result;
	
	if (!object_name || !property_name) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 58);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, property_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_write_property_value(struct ncp_conn *conn,
			 NWObjectType object_type, const char *object_name,
			 const char *property_name,
			 u_int8_t segment,
			 const struct nw_property *property_value)
{
	long result;
	
	if (!object_name || !property_name || !property_value) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 62);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_byte(conn, segment);
	ncp_add_byte(conn, property_value->more_flag);
	ncp_add_pstring(conn, property_name);
	ncp_add_mem(conn, property_value->value, 128);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_get_big_ncp_max_packet_size(struct ncp_conn *conn,
				u_int16_t proposed_max_size,
				u_int8_t proposed_security_flag,
				u_int16_t * accepted_max_size,
				u_int16_t * echo_socket,
				u_int8_t * accepted_security_flag)
{
	long result;
	ncp_init_request(conn);
	ncp_add_word_hl(conn, proposed_max_size);
	ncp_add_byte(conn, proposed_security_flag);

	if ((result = ncp_request(conn, 97)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 5) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (accepted_max_size)
		*accepted_max_size = ncp_reply_word_hl(conn, 0);
	if (echo_socket)
		*echo_socket = ncp_reply_word_hl(conn, 2);
	if (accepted_security_flag)
		*accepted_security_flag = ncp_reply_byte(conn, 4);
	ncp_unlock_conn(conn);
	return 0;
}

#if 0
static long
ncp_verify_object_password(struct ncp_conn *conn,
		           const char *objName, NWObjectType objType,
                           const char *objPassword)
{
	long result;
	ncp_init_request_s(conn, 63);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, passwd);
	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}
#endif

static long
ncp_keyed_verify_password(struct ncp_conn *conn,
			  const struct ncp_bindery_object *object,
			  const unsigned char *key,
			  const unsigned char *passwd)
{
	dword tmpID = htonl(object->object_id);
	unsigned char buf[128];
	unsigned char encrypted[8];
	long result;

	if (!passwd) {
		return ERR_NULL_POINTER;
	}
	shuffle((byte *) & tmpID, passwd, strlen(passwd), buf);
	nw_encrypt(key, buf, encrypted);

	ncp_init_request_s(conn, 74);
	ncp_add_mem(conn, encrypted, 8);
	ncp_add_word_hl(conn, object->object_type);
	ncp_add_pstring(conn, object->object_name);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE NWVerifyObjectPassword(NWCONN_HANDLE conn,
			       const char* objName,
			       NWObjectType objType,
			       const char* objPassword) {
	NWCCODE result;
	u_int8_t ncp_key[8];
	struct ncp_bindery_object user;

	if ((result = ncp_get_encryption_key(conn, ncp_key)) != 0) {
#if 0
		if (!ncp_conn_trusted(conn))
			result = ncp_verify_object_password(conn, objName, 
						objType, objPassword);
#endif						  
		return result;
	}
	result = ncp_get_bindery_object_id(conn, objType, objName, &user);
	if (result)
		return result;
	return ncp_keyed_verify_password(conn, &user, ncp_key, objPassword);
}

static NWCCODE ncp_sign_stop(NWCONN_HANDLE conn);

NWCCODE NWLogoutFromFileServer(NWCONN_HANDLE conn)
{
	NWCCODE result;

	ncp_init_request(conn);

	result = ncp_request(conn, 25);
	if (result == 0) {
		conn->user_id = 0;
		conn->user_id_valid = 1;
		conn->state++;
		conn->connState &= ~(CONNECTION_AUTHENTICATED | CONNECTION_LICENSED);

		result = ncp_sign_stop(conn);
	}
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_login_encrypted(struct ncp_conn *conn,
		    const struct ncp_bindery_object *object,
		    const unsigned char *key,
		    const unsigned char *passwd)
{
	dword tmpID;
	unsigned char buf[128];
	unsigned char encrypted[8];
	long result;

	if (!passwd || !key || !object) {
		return ERR_NULL_POINTER;
	}

	tmpID = htonl(object->object_id);
	shuffle((byte *) & tmpID, passwd, strlen(passwd), buf);
	nw_encrypt(key, buf, encrypted);

	ncp_init_request_s(conn, 24);
	ncp_add_mem(conn, encrypted, 8);
	ncp_add_word_hl(conn, object->object_type);
	ncp_add_pstring(conn, object->object_name);

	result = ncp_request(conn, 23);
	if ((result == 0) || (result == NWE_PASSWORD_EXPIRED)) {
		long result2;

		/* we must not set user_id here, we do not know it.
		   We know special `password ID' here only. */
		conn->user_id_valid = 0;
		conn->state++;
		conn->connState |= CONNECTION_AUTHENTICATED | CONNECTION_LICENSED;

		memcpy(buf + 16, key, 8);
		sign_init(buf, buf);
		result2 = ncp_sign_start(conn, buf);
		if (result2)
			result = result2;
	}
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_login_unencrypted(struct ncp_conn *conn,
		      NWObjectType object_type, const char *object_name,
		      const unsigned char *passwd)
{
	long result;

	if (!object_name || !passwd) {
		return ERR_NULL_POINTER;
	}

	ncp_init_request_s(conn, 20);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, passwd);
	result = ncp_request(conn, 23);
	if ((result == 0) || (result == NWE_PASSWORD_EXPIRED)) {
		/* we do not have user_id here... */
		conn->user_id_valid = 0;
		conn->state++;
		conn->connState |= CONNECTION_AUTHENTICATED | CONNECTION_LICENSED;
	}
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_change_login_passwd(struct ncp_conn *conn,
			const struct ncp_bindery_object *object,
			const unsigned char *key,
			const unsigned char *oldpasswd,
			const unsigned char *newpasswd)
{
	long id;
	unsigned char cryptkey[8];
	unsigned char newpwd[16];	/* new passwd as stored by server */
	unsigned char oldpwd[16];	/* old passwd as stored by server */
	unsigned char len;
	long result;

	if (!object || !key || !oldpasswd || !newpasswd) {
		return ERR_NULL_POINTER;
	}
	id = htonl(object->object_id);
	memcpy(cryptkey, key, 8);
	shuffle((byte *) & id, oldpasswd, strlen(oldpasswd), oldpwd);
	shuffle((byte *) & id, newpasswd, strlen(newpasswd), newpwd);
	nw_encrypt(cryptkey, oldpwd, cryptkey);
	newpassencrypt(oldpwd, newpwd, newpwd);
	newpassencrypt(oldpwd + 8, newpwd + 8, newpwd + 8);
	if ((len = strlen(newpasswd)) > 63) {
		len = 63;
	}
	len = ((len ^ oldpwd[0] ^ oldpwd[1]) & 0x7f) | 0x40;

	ncp_init_request_s(conn, 75);
	ncp_add_mem(conn, cryptkey, 8);
	ncp_add_word_hl(conn, object->object_type);
	ncp_add_pstring(conn, object->object_name);
	ncp_add_byte(conn, len);
	ncp_add_mem(conn, newpwd, 16);
	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_login_user(struct ncp_conn *conn,
	       const unsigned char *username,
	       const unsigned char *password)
{
	return ncp_login_object(conn, username, NCP_BINDERY_USER, password);
}

static long
ncp_login_object(struct ncp_conn *conn,
		 const unsigned char *username,
		 int login_type,
		 const unsigned char *password)
{
	long result;
	unsigned char ncp_key[8];
	struct ncp_bindery_object user;

	if ((result = ncp_get_encryption_key(conn, ncp_key)) != 0) {
		return ncp_login_unencrypted(conn, login_type, username,
					     password);
	}
	if ((result = ncp_get_bindery_object_id(conn, login_type,
						username, &user)) != 0) {
		return result;
	}
	if ((result = ncp_login_encrypted(conn, &user,
					  ncp_key, password)) != 0) {
		struct nw_property p;
		struct ncp_prop_login_control *l
		= (struct ncp_prop_login_control *) &p;

		if (result != NWE_PASSWORD_EXPIRED)
		{
			return result;
		}
		fprintf(stderr, _("Your password has expired\n"));

		if ((result = ncp_read_property_value(conn, NCP_BINDERY_USER,
						      username, 1,
						      "LOGIN_CONTROL",
						      &p)) == 0) {
			fprintf(stderr, _("You have %d login attempts left\n"),
				l->GraceLogins);
		}
	}
	return 0;
}

static NWCCODE
ncp_sign_stop(UNUSED(NWCONN_HANDLE conn))
{
#ifdef SIGNATURES
	return 0;
#if 0
	NWCCODE result = 0;

	printf("Conn %p, %d\n", conn, conn->sign_active);
	if (conn->sign_active) {
		conn->sign_active = 0;
		switch (conn->is_connected) {
			case NOT_CONNECTED:
				break;
			case CONN_TEMPORARY:
				break;
			case CONN_PERMANENT:
				/* Oops. We cannot turn off active signatures */
				if (ioctl(conn->mount_fid, NCP_IOC_SIGN_INIT, NULL))
					result = EOPNOTSUPP;
				break;
			case CONN_KERNELBASED:
				/* Oops. We cannot turn off active signatures */
				if (ioctl(conn->mount_fid, NCP_IOC_SIGN_INIT, NULL))
					result = EOPNOTSUPP;
				break;
		}
	}
	return result;
#endif
#else
	return 0;
#endif
}

long
ncp_sign_start(struct ncp_conn *conn, const char *sign_root)
{
#ifdef SIGNATURES
	static const char init_last[16]=
			   {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
	                    0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
	if (ncp_get_sign_wanted(conn)) {
		memcpy(conn->sign_data.sign_root, sign_root, 8);
		memcpy(conn->sign_data.sign_last, init_last, 16);
		conn->sign_active = 1;
		switch (conn->is_connected) {
			case NOT_CONNECTED:
				break;
			case CONN_TEMPORARY:
				break;
			case CONN_PERMANENT:
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
				if (ioctl(conn->mount_fid, NCP_IOC_SIGN_INIT, 
					&conn->sign_data))
#endif				  
					return NWE_SIGNATURE_LEVEL_CONFLICT;
				break;
			case CONN_KERNELBASED:
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
				if (ioctl(conn->mount_fid, NCP_IOC_SIGN_INIT,
					&conn->sign_data))
#endif
					return NWE_SIGNATURE_LEVEL_CONFLICT;
				break;
		}
	}
#endif
	return 0;
}

#ifdef NDS_SUPPORT
long
ncp_send_nds_frag(struct ncp_conn *conn,
		  int ndsverb,
		  const char *inbuf, size_t inbuflen,
		  char *outbuf, size_t outbufsize, size_t *outbuflen)
{
	long result;
	size_t  sizeleft, i;
	size_t	maxdatasize = 514;
	int first = 1;
	int firstReply = 1;
	int fraghnd = -1;
	int32_t	ndsCode = -399;
	size_t	replyLen = 0;
	size_t	fragLen;

	if (inbuflen && !inbuf) {
		return ERR_NULL_POINTER;
	}
	if (outbuflen) *outbuflen = 0;
	do {
		sizeleft = maxdatasize;
		ncp_init_request(conn);
		ncp_add_byte(conn, 2);
		ncp_add_dword_lh(conn, fraghnd);
		if (first) {
			ncp_add_dword_lh(conn, maxdatasize - 8);
			ncp_add_dword_lh(conn, inbuflen + 12);
			ncp_add_dword_lh(conn, 0);
			ncp_add_dword_lh(conn, ndsverb);
			ncp_add_dword_lh(conn, outbufsize);
			sizeleft -= 25;
			first = 0;
		}
		else
			sizeleft -= 5;
		i = (sizeleft > inbuflen) ? inbuflen : sizeleft;
		if (i) ncp_add_mem(conn, inbuf, i);
		inbuflen -= i;
		inbuf += i;
		if ((result = ncp_request(conn, 0x68)) != 0) {
			ncp_unlock_conn(conn);
			ncp_dprintf(_("Error in ncp_request\n"));
			return result;
		}
		fragLen = ncp_reply_dword_lh(conn, 0);
		if (fragLen < 4) {
			ncp_unlock_conn(conn);
			ncp_dprintf(_("Fragment too short\n"));
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		fraghnd = ncp_reply_dword_lh(conn, 4);
		fragLen -= 4;
		if (fragLen) {
			int hdr;

			if (firstReply) {
				ndsCode = ncp_reply_dword_lh(conn, 8);
				hdr = 12;
				fragLen -= 4;
				firstReply = 0;
			} else {
				hdr = 8;
			}
			if (fragLen > outbufsize) {
				ncp_unlock_conn(conn);
				ncp_dprintf(_("Fragment too large, len=%d, max=%d\n"), fragLen, outbufsize);
				return NWE_BUFFER_OVERFLOW;
			}
			if (outbuf) {
				memcpy(outbuf, ncp_reply_data(conn, hdr), fragLen);
				outbuf += fragLen;
			}
			replyLen += fragLen;
		} else {
			/* if reply len == 0 then we must have something to transmit */
			/* otherwise it can cause endless loop */
			if ((fraghnd != -1) && (inbuflen == 0)) {
				ncp_unlock_conn(conn);
				ncp_dprintf(_("Why next fragment?\n"));
				return NWE_SERVER_FAILURE;
			}
		}
		ncp_unlock_conn(conn);
		if (fraghnd != -1) {
			ncp_dprintf(_("Fragmented\n"));
		}
	} while (fraghnd != -1);
	if (inbuflen || firstReply) {
		ncp_dprintf(_("InBufLen after request=%d, FirstReply=%d\n"), inbuflen, firstReply);
		return NWE_SERVER_FAILURE;
	}
	if (outbuflen) *outbuflen = replyLen;
	if (ndsCode != 0) {
		ncp_dprintf(_("NDS error %d\n"), ndsCode);
		if ((ndsCode < 0) && (ndsCode > -256))
			return -ndsCode | NWE_SERVER_ERROR;
		else
			return ndsCode;
	}
	return 0;
}

long
ncp_send_nds(struct ncp_conn *conn, int fn,
	     const char *data_in, size_t data_in_len, 
	     char *data_out, size_t data_out_max, size_t *data_out_len)
{
	size_t i;
	long err;

	ncp_init_request(conn);
	ncp_add_byte(conn, fn);
	if (data_in) ncp_add_mem(conn, data_in, data_in_len);
	if (!(err = ncp_request(conn, 0x68))) {
		i = conn->ncp_reply_size;
		if (i > data_out_max) i = data_out_max;
		if (data_out)
			memcpy(data_out, ncp_reply_data(conn, 0), i);
		if (data_out_len) *data_out_len = i;
	}
	else
		if (data_out_len) *data_out_len = 0;
	ncp_unlock_conn(conn);
	return err;
}

long
ncp_change_conn_state(struct ncp_conn *conn, int new_state)
{
	nuint8 ns[4];
	
	DSET_LH(ns, 0, new_state);
	return NWRequestSimple(conn, NCPC_SFN(23,29), ns, 4, NULL);
}
#endif	/* NDS_SUPPORT */

static int
ncp_attach_by_addr_tran(const struct sockaddr *target, nuint transport, struct ncp_conn** result) {
	struct ncp_conn *conn;
	int err;

	if (!result) {
		return ERR_NULL_POINTER;
	}
	*result = NULL;
	conn = ncp_alloc_conn();
	if (!conn) {
		return ENOMEM;
	}
	err = ncp_connect_addr(conn, target, 1, transport);
	if (err) {
		ncp_close(conn);
		return err;
	}
	*result = conn;
	return 0;
}

NWCCODE NWCCOpenConnBySockAddr(const struct sockaddr* tran,
		enum NET_ADDRESS_TYPE atran, nuint openState,
		nuint reserved, NWCONN_HANDLE* conn) {
	if (reserved != NWCC_RESERVED)
		return NWE_PARAM_INVALID;
	/* not yet supported */
	if (openState & NWCC_OPEN_PUBLIC)
		return NWE_PARAM_INVALID;
	return ncp_attach_by_addr_tran(tran, atran, conn);
}

static int
ncp_attach_by_addr(const struct sockaddr *target, struct ncp_conn** result) 
{
	nuint transport;
	
	if (!result || !target) {
		return ERR_NULL_POINTER;
	}
	switch (target->sa_family) {
		case AF_IPX:
			transport = NT_IPX;
			break;
		case AF_INET:
			if (getenv("NCP_OVER_TCP")) {
				transport = NT_TCP;
			} else {
				transport = NT_UDP;
			}
			break;
		case AF_UNIX:
			transport = NT_TCP;
			break;
		default:
			return EAFNOSUPPORT;
	}
	return ncp_attach_by_addr_tran(target, transport, result);
}

struct ncp_conn *
ncp_open_addr(const struct sockaddr *target, long *err) {
	struct ncp_conn *conn;

	if (!err) {
		return NULL;
	}
	*err = ncp_attach_by_addr(target, &conn);
	return conn;
}

long
ncp_get_broadcast_message(struct ncp_conn *conn, char message[256])
{
	long result;
	int length;

	if (!message) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 0x0B);
	if ((result = ncp_request(conn, 0x15)) != 0) {
		ncp_unlock_conn(conn);
		ncp_init_request_s(conn, 0x01);
		if ((result = ncp_request(conn, 0x15)) != 0) {
			ncp_unlock_conn(conn);
			return result;
		}
	}
	length = ncp_reply_byte(conn, 0);
	message[length] = 0;
	memcpy(message, ncp_reply_data(conn, 1), length);
	ncp_unlock_conn(conn);
	return 0;
}

int
ncp_get_conn_type(struct ncp_conn* conn) {
	if (conn) {
		switch (conn->is_connected) {
			case CONN_PERMANENT:
					return NCP_CONN_PERMANENT;
			case CONN_TEMPORARY:
					return NCP_CONN_TEMPORARY;
			case CONN_KERNELBASED:
					return NCP_CONN_KERNELBASED;
			default:;
		}
	}
	return NCP_CONN_INVALID;
}

int
ncp_get_conn_number(struct ncp_conn* conn) {
	int type;
	
	type = ncp_get_conn_type(conn);
	if (conn == NCP_CONN_INVALID)
		return -ENOTCONN;
	return conn->i.connection;
}

int ncp_get_fid(struct ncp_conn* conn) {
	if (ncp_get_conn_type(conn) != NCP_CONN_PERMANENT)
		return -1;
	return conn->mount_fid;
}

NWCCODE
ncp_get_dentry_ttl(struct ncp_conn* conn, unsigned int* ttl) {
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	int fd = ncp_get_fid(conn);
	u_int32_t kernel_ttl;
	
	if (fd == -1)
		return NWE_REQUESTER_FAILURE;
	if (ioctl(fd, NCP_IOC_GETDENTRYTTL, &kernel_ttl)) {
		if (errno != EINVAL)
			return errno;
		kernel_ttl = 0;	/* old kernel... */
	}
	if (ttl)
		*ttl = kernel_ttl;
	return 0;
#else
	return NWE_REQUESTER_FAILURE;
#endif
}

NWCCODE
ncp_set_dentry_ttl(struct ncp_conn* conn, unsigned int ttl) {
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	int fd = ncp_get_fid(conn);
	u_int32_t kernel_ttl;
	
	if (fd == -1)
		return NWE_REQUESTER_FAILURE;
	kernel_ttl = ttl;
	if (ioctl(fd, NCP_IOC_SETDENTRYTTL, &kernel_ttl))
		return errno;
	return 0;
#else
	return NWE_REQUESTER_FAILURE;
#endif
}

static NWCCODE
ncp_get_private_key_perm(struct ncp_conn* conn, void* pk, size_t* pk_len) {
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	int fd = ncp_get_fid(conn);
	struct ncp_privatedata_ioctl npi;
	
	if (fd == -1)
		return NWE_REQUESTER_FAILURE;
	if (!pk_len) {
		return ERR_NULL_POINTER;
	}
	npi.data = pk;
	if (pk)
		npi.len = *pk_len;
	else
		npi.len = 0;

	if (ioctl(fd, NCP_IOC_GETPRIVATEDATA, &npi))
		return errno;
	*pk_len = npi.len;
	return 0;
#else
	return NWE_REQUESTER_FAILURE;
#endif
}

static NWCCODE
ncp_get_private_key_temp(struct ncp_conn* conn, void* pk, size_t* pk_len) {
	NWCCODE err = 0;
	
	ncp_lock_conn(conn);
	if (pk) {
		size_t maxln = *pk_len;
		if (maxln > conn->private_key_len) {
			maxln = conn->private_key_len;
		}
		memcpy(pk, conn->private_key, maxln);
	}
	*pk_len = conn->private_key_len;
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE
ncp_get_private_key(struct ncp_conn* conn, void* pk, size_t* pk_len) {
	switch (ncp_get_conn_type(conn)) {
		case NCP_CONN_TEMPORARY:
			return ncp_get_private_key_temp(conn, pk, pk_len);
		default:
			return ncp_get_private_key_perm(conn, pk, pk_len);
	}
}

static NWCCODE
ncp_set_private_key_perm(struct ncp_conn* conn, const void* pk, size_t pk_len) {
#ifdef NCP_KERNEL_NCPFS_AVAILABLE
	int fd = ncp_get_fid(conn);
	struct ncp_privatedata_ioctl npi;
	
	if (fd == -1)
		return NWE_REQUESTER_FAILURE;
	if (pk_len && !pk)
		return ERR_NULL_POINTER;
	npi.data = (void*)pk;
	npi.len = pk_len;
	if (ioctl(fd, NCP_IOC_SETPRIVATEDATA, &npi))
		return errno;
	return 0;
#else
	return NWE_REQUESTER_FAILURE;
#endif
}

static NWCCODE
ncp_set_private_key_temp(struct ncp_conn* conn, const void* pk, size_t pk_len) {
	void *keydata;
	void *oldkeydata;
	
	keydata = malloc(pk_len);
	if (!keydata)
		return ENOMEM;
	mlock(keydata, pk_len);
	memcpy(keydata, pk, pk_len);

	ncp_lock_conn(conn);	
	oldkeydata = conn->private_key;
	conn->private_key = keydata;
	conn->private_key_len = pk_len;
	ncp_unlock_conn(conn);
	
	free(oldkeydata);
	return 0;
}

NWCCODE
ncp_set_private_key(struct ncp_conn* conn, const void* pk, size_t pk_len) {
	switch (ncp_get_conn_type(conn)) {
		case NCP_CONN_TEMPORARY:
			return ncp_set_private_key_temp(conn, pk, pk_len);
		default:
			return ncp_set_private_key_perm(conn, pk, pk_len);
	}
}

NWCCODE
ncp_next_conn(NWCONN_HANDLE conn, NWCONN_HANDLE* next_conn) {
	struct list_head* h;

	if (!next_conn) {
		return ERR_NULL_POINTER;
	}
	ncpt_mutex_lock(&conn_lock);
	for (h = conn ? conn->conn_ring.next : conn_list.next; h != &conn_list; h = h->next) {
		NWCONN_HANDLE c2 = list_entry(h, struct ncp_conn, conn_ring);
		
		if (c2->is_connected == NOT_CONNECTED)
			continue;
		if (ncpt_atomic_read(&c2->use_count)) {
			/* FIXME. use_count has to be redone - either
			   with conditional store, or with mutex. Code 
			   never expected that library can return 
			   connection handle from internal list */
			ncpt_atomic_inc(&c2->use_count);
			ncpt_mutex_unlock(&conn_lock);
			*next_conn = c2;
			return 0;
		}
	}
	ncpt_mutex_unlock(&conn_lock);
	return NWE_REQUESTER_FAILURE;
}

#ifdef NCP_DEBUG
static FILE* ncp_do_FILE = NULL;
static const char* ncp_do_filename = NULL;
static size_t ncp_do_filename_len = 0;
static pid_t ncp_do_lastpid = 0;
static ncpt_mutex_t ncp_do_lock = NCPT_MUTEX_INITIALIZER;

static void ncp_do_printf(const char* fmt, ...) {
	va_list ap;

	ncpt_mutex_lock(&ncp_do_lock);
	if (ncp_do_filename) {
		pid_t cpid;
		
		cpid = getpid();
		if (cpid != ncp_do_lastpid) {
			char* nfn;

			if (ncp_do_FILE) {
				fclose(ncp_do_FILE);
				ncp_do_FILE = NULL;
			}
			nfn = alloca(ncp_do_filename_len + 20);
			memcpy(nfn, ncp_do_filename, ncp_do_filename_len);
			sprintf(nfn + ncp_do_filename_len, ".%u", cpid);
			ncp_do_FILE = fopen(nfn, "wt");
			if (ncp_do_FILE) {
				ncp_do_lastpid = cpid;
			}
		}
	}
	if (ncp_do_FILE) {
		va_start(ap, fmt);
		vfprintf(ncp_do_FILE, fmt, ap);
		va_end(ap);
	}
	ncpt_mutex_unlock(&ncp_do_lock);
}

static void ncp_do_init(void) {
	ncp_do_FILE = stdout;
	if (getuid() == geteuid()) {
		const char* ncp_do;
		
		ncp_do = getenv("NCP_DEBUG_OUTPUT");
		if (ncp_do) {
			ncp_do_filename_len = strlen(ncp_do);
			ncp_do_filename = ncp_do;
		}
	}
}

static void ncp_do_fini(void) {
	ncpt_mutex_lock(&ncp_do_lock);
	if (ncp_do_FILE && ncp_do_filename) {
		fclose(ncp_do_FILE);
		ncp_do_FILE = NULL;
	}
	ncpt_mutex_unlock(&ncp_do_lock);
}

static void ncp_debug_help(void) __attribute((noreturn));

static void ncp_debug_help(void) {
	fprintf(stderr, "Valid options for the NCP_DEBUG environment variable are:\n"
	                "\n"
			"  cleaup      List NCP connections opened at exit\n"
			"  all         All options above\n"
			"  help        Display this help text\n"
			"\n"
			"To direct the debugging output into a file instead of standard output\n"
			"a filename can be specified using the NCP_DEBUG_OUTPUT environment variable.\n");
	exit(248);
}

static void ncp_debug_usage(int len, const char* ncpdebug) {
	fprintf(stderr, "warning: NCP debug option `%*.*s\' unknown: try NCP_DEBUG=help\n", len, len, ncpdebug);
}

static int ncp_debug_show_cleanup = 0;

static void ncp_init(void) __attribute__((constructor));

static void ncp_init(void) {
	const char* ncpdebug;
	size_t ln;
	
	ncpdebug = getenv("NCP_DEBUG");
	if (!ncpdebug) {
		return;
	}
	while (*ncpdebug) {
		ln = strcspn(ncpdebug, ",:;");
		if (ln) {
			if (ln == 4 && !memcmp(ncpdebug, "help", 4)) {
				ncp_debug_help();
			} else if (ln == 7 && !memcmp(ncpdebug, "cleanup", 7)) {
				ncp_debug_show_cleanup = 1;
			} else if (ln == 3 && !memcmp(ncpdebug, "all", 3)) {
				ncp_debug_show_cleanup = 1;
			} else {
				ncp_debug_usage(ln, ncpdebug);
			}
		}
		ncpdebug += ln;
		ln = strspn(ncpdebug, ",:;");
		ncpdebug += ln;
	}
	ncp_do_init();
}

static void ncp_fini(void) __attribute__((destructor));

static void ncp_fini(void) {
	struct list_head* h;
	unsigned int cnt = 0;
	
	if (ncp_debug_show_cleanup) {
		ncpt_mutex_lock(&conn_lock);
		for (h = conn_list.next; h != &conn_list; h = h->next) {
			char fsname[NCP_BINDERY_NAME_LEN + 1];
			NWCCODE err;
			NWCONN_HANDLE c2 = list_entry(h, struct ncp_conn, conn_ring);
		
			ncp_do_printf("Connection %p: store count=%u, use count=%u\n",
				c2, ncpt_atomic_read(&c2->store_count), ncpt_atomic_read(&c2->use_count));
			ncp_do_printf("  NDS connection: %p\n", c2->nds_conn);
			ncp_do_printf("  User: %s\n", c2->user);
			err = NWGetFileServerName(c2, fsname);
			if (err)
				sprintf(fsname, "<%s>", strnwerror(err));
			ncp_do_printf("  Server name: %s\n", fsname);
			cnt++;
		}
		ncpt_mutex_unlock(&conn_lock);
		ncp_do_printf("Found %u connections alive on exit!\n", cnt);
	}
	ncp_do_fini();
}
#endif
