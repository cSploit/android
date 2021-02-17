/*
    ipxdump.c
    Copyright 1996 Volker Lendecke, Goettingen, Germany
    Copyright 1999, 2001 Petr Vandrovec, Prague, Czech Republic

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Revision History:

	1.01	1999, December, 15	Petr Vandrovec <vandrove@vc.cvut.cz>
			Use sigaction instead of signal, so ^C works
			immediately and not after next frame comes
			(bsd vs. sysv signal() semantic bug)
			
	1.02	2001, July 15		Petr Vandrovec <vandrove@vc.cvut.cz>
			Add #include <stdlib> to fix some warnings.

 */

#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <ncp/ext/socket.h>
/* TBD */
#include <netinet/in.h>
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#else
#include <linux/if_ether.h>
#endif
#include <string.h>
#include <sys/ioctl.h>
#include <ncp/kernel/if.h>
#include <signal.h>
#include <stdlib.h>
/* TBD */
#include <netinet/ip.h>
/* TBD */
#include <netinet/tcp.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "ipxutil.h"

struct ipx_address
{
	IPXNet	net	__attribute__((packed));
	IPXNode	node	__attribute__((packed));
	IPXPort	sock	__attribute__((packed));
};

struct ipx_packet
{
	unsigned short ipx_checksum __attribute__((packed));
#define IPX_NO_CHECKSUM	0xFFFF
	unsigned short ipx_pktsize __attribute__((packed));
	unsigned char ipx_tctrl __attribute__((packed));
	unsigned char ipx_type __attribute__((packed));
#define IPX_TYPE_UNKNOWN	0x00
#define IPX_TYPE_RIP		0x01	/* may also be 0 */
#define IPX_TYPE_SAP		0x04	/* may also be 0 */
#define IPX_TYPE_SPX		0x05	/* Not yet implemented */
#define IPX_TYPE_NCP		0x11	/* $lots for docs on this (SPIT) */
#define IPX_TYPE_PPROP		0x14	/* complicated flood fill brdcast [Not supported] */
	struct ipx_address ipx_dest __attribute__((packed));
	struct ipx_address ipx_source __attribute__((packed));
};


void handle_frame(unsigned char *buf, int length, struct sockaddr *saddr);
void handle_ipx(const char *frame, unsigned char *buf);

static int filter = 0;
static int allframes = 0;
static IPXNode filter_node;

static const char* progname;

static void usage(void) {
	fprintf(stderr, "usage: %s [-r] [-d device] [node]\n", progname);
}

static int exit_request = 0;
static void
int_handler(int dummy)
{
	exit_request = 1;
	(void)dummy;
}

int
main(int argc, char *argv[])
{
	int sd;
	struct ifreq ifr, oldifr;
	const char *device = "eth0";
	struct sockaddr saddr;
	int sizeaddr;
	unsigned char buf[4096];
	int length;
	int opt;
	struct sigaction saint;

	progname = strrchr(argv[0], '/');
	if (progname)
		progname++;
	else
		progname = argv[0];

	saint.sa_handler = int_handler;
	saint.sa_flags = 0;
	sigemptyset(&saint.sa_mask);
	sigaction(SIGINT, &saint, NULL);

	while ((opt = getopt(argc, argv, "rd:h")) != -1) {
		switch (opt) {
			case 'r':allframes = 1; 
				 break;
			case 'd':device = optarg;
				 break;
			case 'h':usage();
				 return 5;
			case ':':;
			case '?':break;
			default: printf("?? unknown option returned by getopt\n");
				 break;
		}
	}
	if (optind < argc)
	{
		if (ipx_sscanf_node(argv[optind], filter_node) != 0)
		{
			usage();
			exit(1);
		}
		filter = 1;
	}
	setvbuf(stdout, NULL, _IOLBF, 0);
//	if ((sd = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ALL))) < 0)
	if ((sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("Can't get socket");
		fprintf(stderr, "You must run %s as root\n", progname);
		exit(1);
	}
	/* SET PROMISC */

	strcpy(oldifr.ifr_name, device);
	if (ioctl(sd, SIOCGIFFLAGS, &oldifr) < 0)
	{
		close(sd);
		perror("Can't get flags");
		exit(2);
	}
	/* This should be rewritten to cooperate with other net tools */
	ifr = oldifr;
	ifr.ifr_flags |= IFF_PROMISC;

	if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0)
	{
		close(sd);
		perror("Can't set flags");
		exit(3);
	}
	while (exit_request == 0)
	{
		/* This is the main data-gathering loop; keep it small
		   and fast */
		sizeaddr = sizeof(saddr);
		length = recvfrom(sd, buf, sizeof(buf), 0,
				  &saddr, &sizeaddr);
		if (length < 0)
			continue;
		if (allframes) {
			unsigned char* ptr = buf;
			if (filter) {
				if (memcmp(filter_node, buf, 6)&&memcmp(filter_node, buf+6, 6))
					continue;
			}
			printf("raweth ");
			while (length--) printf("%02X", *ptr++);
			printf("\n");
		} else {
			handle_frame(buf, length, &saddr);
		}
	}

	/* This should be rewritten to cooperate with other net tools */
	if (ioctl(sd, SIOCSIFFLAGS, &oldifr) < 0)
	{
		close(sd);
		perror("Can't set flags");
		exit(4);
	}
	close(sd);
	return 0;
}

void
handle_ipx(const char *frame, unsigned char *buf)
{
	int i;
	struct ipx_packet *h = (struct ipx_packet *) buf;
	struct sockaddr_ipx s_addr;
	struct sockaddr_ipx d_addr;
	int length = ntohs(h->ipx_pktsize);


	memset(&s_addr, 0, sizeof(s_addr));
	memset(&d_addr, 0, sizeof(d_addr));

	memcpy(s_addr.sipx_node, h->ipx_source.node, sizeof(s_addr.sipx_node));
	s_addr.sipx_port = h->ipx_source.sock;
	s_addr.sipx_network = h->ipx_source.net;

	memcpy(d_addr.sipx_node, h->ipx_dest.node, sizeof(d_addr.sipx_node));
	d_addr.sipx_port = h->ipx_dest.sock;
	d_addr.sipx_network = h->ipx_dest.net;

	if (filter != 0)
	{
		if ((memcmp(filter_node, s_addr.sipx_node,
			    sizeof(filter_node)) != 0)
		    && (memcmp(filter_node, d_addr.sipx_node,
			       sizeof(filter_node)) != 0))
		{
			/* Not for us */
			return;
		}
	}
	printf("%s ", frame);

	for (i = 0; i < length; i++)
	{
		printf("%2.2X", buf[i]);
	}
	printf("\n");
	if (!isatty(STDOUT_FILENO))
	{
		fflush(stdout);
	}
}

void
handle_other(unsigned char *buf, int length, struct sockaddr *saddr)
{
	struct ethhdr *eth = (struct ethhdr *) buf;
	unsigned char *p = &(buf[sizeof(struct ethhdr)]);

	if (ntohs(eth->h_proto) < 1536)
	{
		/* This is a magic hack to spot IPX packets. Older
		 * Novell breaks the protocol design and runs IPX over
		 * 802.3 without an 802.2 LLC layer. We look for FFFF
		 * which isnt a used 802.2 SSAP/DSAP. This won't work
		 * for fault tolerant netware but does for the rest.
		 */

		if (*(unsigned short *) p == 0xffff)
		{
			handle_ipx("802.3", p);
			return;
		}
		if ((*(unsigned short *) p == htons(0xe0e0))
		    && (p[2] == 0x03))
		{
			handle_ipx("802.2", p + 3);
			return;
		}
		if (memcmp(p, "\252\252\003\000\000\000\201\067", 8) == 0)
		{
			handle_ipx("snap", p + 8);
			return;
		}
	}
}

void
handle_frame(unsigned char *buf, int length, struct sockaddr *saddr)
{
	/* Ethernet packet type ID field */
	unsigned short packet_type = ((struct ethhdr *) buf)->h_proto;
	switch (htons(packet_type))
	{
	case ETH_P_IPX:
		handle_ipx("EtherII", &(buf[sizeof(struct ethhdr)]));
		break;
	default:
		handle_other(buf, length, saddr);
		break;
	}
}
