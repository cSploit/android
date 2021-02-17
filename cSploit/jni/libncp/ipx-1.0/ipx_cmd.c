/*
    ipx_cmd.c - IPX Compatibility mode tunnel
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2001, June 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Cleanup some compilation warnings.

 */
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ncp/ext/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if_arp.h>	/* TODO: linux/if_arp.h */
#include <ncp/kernel/ipx.h>
#include <ncp/kernel/if.h>
#include <ncp/kernel/types.h>
#include "netlink.h"

#include "private/libintl.h"
#define _(X) gettext(X)

#define NOVELL_SCMD_PORT	(2645)

/* Move them to configure... */
#ifndef AF_NETLINK
#define AF_NETLINK 16
#endif
#ifndef PF_NETLINK
#define PF_NETLINK AF_NETLINK
#endif

/* we are doing EthernetII... Any objections? */
struct {
	u_int16_t	unknown	__attribute__((packed));
	u_int8_t	dst[6];
	u_int8_t	src[6];
	u_int16_t	type	__attribute__((packed));
	u_int8_t	ipx[16384];
	} buffer;

static int getiflist(int fd, struct ifconf* ifc) {
	int i;
	
	for (i = 0; i < 32; i++) {
		int err;
		
		ifc->ifc_len = 65536;	/* ignored */
		ifc->ifc_req = NULL;
		
		err = ioctl(fd, SIOCGIFCONF, ifc);
		if (err < 0)
			continue;
			
		if (!ifc->ifc_len)
			return 0;
			
		ifc->ifc_req = (struct ifreq*)malloc(ifc->ifc_len);
		err = ioctl(fd, SIOCGIFCONF, ifc);
		if (!err)
			return 0;
		free(ifc->ifc_buf);
	}
	ifc->ifc_len = 0;
	ifc->ifc_req = NULL;
	return errno;		
}

static char* progname;

static int findtap(int fd_ipx, u_int32_t local) {
	int i;
	u_int8_t localhw[6];
	
	localhw[0] = 0x7E;
	localhw[1] = 0x01;
	memcpy(localhw+2, &local, 4);
	
	for (i = 0; i < 16; i++) {
		struct ifreq ifr;
		int err;
		
		sprintf(ifr.ifr_name, "tap%d", i);
		((struct sockaddr_ipx*)&ifr.ifr_addr)->sipx_type = IPX_FRAME_ETHERII;
		err = ioctl(fd_ipx, SIOCGIFHWADDR, &ifr);
		if (err >= 0) {
			if ((ifr.ifr_addr.sa_family == ARPHRD_ETHER) && !memcmp(ifr.ifr_addr.sa_data, localhw, 6)) {
				err = ioctl(fd_ipx, SIOCGIFADDR, &ifr);
				if (err >= 0) {
					if (!memcmp(((struct sockaddr_ipx*)&ifr.ifr_addr)->sipx_node, localhw, 6)) {
						return i;
					}
				}
			}
		}
	}
	return -EADDRNOTAVAIL;
}
	
static void usage(void) {
	fprintf(stderr, _("usage: %s -A migration_agent [-l local_ip]\n"), progname);
	return;
}

int main(int argc, char* argv[]) {
	int fd_ip;
	struct sockaddr_in localIP;
	int status;
	int fd_netlink;
	struct sockaddr_nl localNET;
	int arg;
	int maxfd;
	fd_set rcvset;
	int opt;
	char *server = NULL;
	struct hostent* hent;
	u_int32_t serveraddr;
	u_int32_t local = htonl(INADDR_ANY);
	int local_ok;
	struct ifconf ifc;
	struct ifreq* curr;
	struct ifreq* stop;
	int tapnum;
	int fd_ipx;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	{
		char* i;
		
		i = strrchr(argv[0], '/');
		progname = i?i+1:argv[0];
	}	
	
	while ((opt = getopt(argc, argv, "A:l:")) != -1) {
		switch (opt) {
			case 'A':
				server = optarg;
				break;
			case 'l':
				{
					hent = gethostbyname(optarg);
					if (!hent) {
						fprintf(stderr, _("%s: %s: "),
							progname,
							optarg);
						herror("");
						exit(99);
					}
					if (hent->h_addrtype != AF_INET) {
						fprintf(stderr, _("%s: Address of %s is not IP\n"),
							progname,
							optarg);
						exit(99);
					}
					if (hent->h_length != 4) {
						fprintf(stderr, _("%s: Address of %s is not 4 bytes long\n"),
							progname,
							optarg);
						exit(99);
					}
					memcpy(&local, hent->h_addr, 4);
				}
				break;
			default:
				usage();
				exit(99);
		}
	}
	
	if (!server) {
		usage();
		exit(99);
	}
	
	hent = gethostbyname(server);
	if (!hent) {
		fprintf(stderr, _("%s: %s: "),
			progname,
			server);
		herror("");
		exit(99);
	}
	if (hent->h_addrtype != AF_INET) {
		fprintf(stderr, _("%s: Address of %s is not IP\n"),
			progname,
			server);
		exit(99);
	}
	if (hent->h_length != 4) {
		fprintf(stderr, _("%s: Address of %s is not 4 bytes long\n"),
			progname,
			server);
		exit(99);
	}
	memcpy(&serveraddr, hent->h_addr, 4);

	fd_ip = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd_ip == -1) {
		int err = errno;
		fprintf(stderr, _("%s: Cannot create UDP/IP socket: %s\n"),
			progname,
			strerror(err));
		return 1;
	}
	
	status = getiflist(fd_ip, &ifc);
	if (status) {
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot get list of local interfaces: %s\n"),
			progname,
			strerror(status));
		return 1;
	}
	curr = ifc.ifc_req;
	stop = (struct ifreq*)(((u_int8_t*)curr) + ifc.ifc_len);

	local_ok = 0;
	while (curr < stop) {
		if (curr->ifr_addr.sa_family == AF_INET) {
			u_int32_t net = ((struct sockaddr_in*)&curr->ifr_addr)->sin_addr.s_addr;
			if (((ntohl(net) & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT) != IN_LOOPBACKNET) {
				if (local == htonl(INADDR_ANY))
					local = net;
				if (local == net)
					local_ok = 1;
			}
		}
		curr++;
	}
	free(ifc.ifc_req);
	if (!local_ok) {
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot find local requested address\n"),
			progname);
		return 1;
	}
	
	/* TBD: enable SO_REUSEADDR? */
	localIP.sin_family = AF_INET;
	localIP.sin_addr.s_addr = htonl(INADDR_ANY);
	localIP.sin_port = htons(NOVELL_SCMD_PORT);
	status = bind(fd_ip, (struct sockaddr*)&localIP, sizeof(localIP));
	if (status) {
		int err = errno;
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot bind requested address to IP socket: %s\n"),
			progname,
			strerror(err));
		return 1;
	}

	/* OK, we have local IP address... Now find appropriate tap device */
	fd_ipx = socket(PF_IPX, SOCK_DGRAM, IPXPROTO_IPX);
	if (fd_ipx < 0) {
		int err = errno;
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot create IPX socket: %s\n"),
			progname,
			strerror(err));
		return 1;
	}
	tapnum = findtap(fd_ipx, local);
	if (tapnum < 0) {
		close(fd_ipx);
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot find ethertap interface: %s\n"),
			progname,
			strerror(-tapnum));
		return 1;
	}
	close(fd_ipx);
	
	fd_netlink = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_TAPBASE+tapnum);
	if (fd_netlink == -1) {
		int err = errno;
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot create NETLINK socket: %s\n"),
			progname,
			strerror(err));
		return 1;
	}
	localNET.nl_family = AF_NETLINK;
	localNET.nl_pid = serveraddr;
	localNET.nl_groups = ~0;
	status = bind(fd_netlink, (struct sockaddr*)&localNET, sizeof(localNET));
	if (status) {
		int err = errno;
		close(fd_netlink);
		close(fd_ip);
		fprintf(stderr, _("%s: Cannot bind requested address to NETLINK socket: %s\n"),
			progname,
			strerror(err));
		return 1;
	}
	arg = fcntl(fd_ip, F_GETFL);
	fcntl(fd_ip, F_SETFL, arg | O_NONBLOCK);
	arg = fcntl(fd_netlink, F_GETFL);
	fcntl(fd_netlink, F_SETFL, arg | O_NONBLOCK);
	maxfd = fd_ip;
	if (fd_netlink > maxfd)
		maxfd = fd_netlink;
	FD_ZERO(&rcvset);
	
	printf("Running...\n");
	
	while (1) {
		FD_SET(fd_ip, &rcvset);
		FD_SET(fd_netlink, &rcvset);
		status = select(maxfd+1, &rcvset, NULL, NULL, NULL);
		if (status > 0) {
			if (FD_ISSET(fd_ip, &rcvset)) {
				struct sockaddr_in remoteIP;
				socklen_t remoteIPlen = sizeof(remoteIP);
				status = recvfrom(fd_ip, buffer.ipx, sizeof(buffer.ipx), 0, (struct sockaddr*)&remoteIP, &remoteIPlen);	
				if (status > 0) {
					if (remoteIP.sin_port != htons(NOVELL_SCMD_PORT)) {
						/* invalid source port... faked? */
					} else if (remoteIP.sin_addr.s_addr != serveraddr) {
						/* invalid server... faked? */
					} else if (status < 2 + 2 + 1 + 1 + 12 + 12) {
						/* too short... */
					} else {
						int ipxLen = ntohs(*(u_int16_t*)(buffer.ipx+2));
						if (ipxLen > status) {
							/* too big... */
						} else {
							/* looks OK... */
							status += 2 + 6 + 6 + 2;
							
							buffer.unknown = 0;
							buffer.type = htons(0x8137);
							buffer.src[0] = 0x7E;
							buffer.src[1] = 0x01;
							memcpy(buffer.src+2, &remoteIP.sin_addr.s_addr, 4);
							buffer.dst[0] = 0x7E;
							buffer.dst[1] = 0x01;
							memcpy(buffer.dst+2, &local, 4);
							{
								struct sockaddr_nl remoteNETLINK;
								int e;
								
								remoteNETLINK.nl_family = AF_NETLINK;
								remoteNETLINK.nl_pid = 0;
								remoteNETLINK.nl_groups = 0;
								e = sendto(fd_netlink, &buffer, status, 0, (struct sockaddr*)&remoteNETLINK, sizeof(remoteNETLINK));
								if (e < 0)
									sendto(fd_netlink, &buffer, status, 0, (struct sockaddr*)&remoteNETLINK, sizeof(remoteNETLINK));
							}
						}
					}
				}
			}
			if (FD_ISSET(fd_netlink, &rcvset)) {
				struct sockaddr_in remoteNETLINK;
				socklen_t remoteNETLINKlen = sizeof(remoteNETLINK);
				status = recvfrom(fd_netlink, &buffer, sizeof(buffer), 0, (struct sockaddr*)&remoteNETLINK, &remoteNETLINKlen);	
				if (status > 0) {
					if (status < 2 + 6 + 6 + 2 + 2 + 2 + 1 + 1 + 12 + 12) {
						/* too short... faked? */
					} else if (buffer.type != htons(0x8137)) {
						/* invalid frame */
					} else {
						int ipxLen = ntohs(*(u_int16_t*)(buffer.ipx+2));
						status -= 2 + 6 + 6 + 2;
						if (ipxLen > status) {
							/* too big... */
						} else {
							/* looks OK... */
							{
								struct sockaddr_in remoteIP;
								int e;
								
								remoteIP.sin_family = AF_INET;
								remoteIP.sin_addr.s_addr = serveraddr;
								remoteIP.sin_port = htons(NOVELL_SCMD_PORT);
								e = sendto(fd_ip, buffer.ipx, status, 0, (struct sockaddr*)&remoteIP, sizeof(remoteIP));
								if (e < 0)
									sendto(fd_ip, buffer.ipx, status, 0, (struct sockaddr*)&remoteIP, sizeof(remoteIP));
							}
						}
					}
				}
			}
		}
	}
	close(fd_netlink);
	close(fd_ip);
	return 0;
}

