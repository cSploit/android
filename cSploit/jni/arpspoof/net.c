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
#include "net.h"

inline int address_hash( void* key ) {
	return (int)key;
}

inline bool address_hash_equals( void* keyA, void* keyB ) {
	return address_hash( keyA ) == address_hash( keyB );
}

int net_get_details( char *iface, int *netmask, int *ifaddr, int *nhosts )
{
	pcap_if_t   *ifaces = NULL,
						  *pcap_iface;
	pcap_addr_t *pcap_address;
	char 				 pcap_ebuf[ PCAP_ERRBUF_SIZE ];
	int 				 retval = -1;

	if( pcap_findalldevs( &ifaces, pcap_ebuf ) == 0 )
	{
		// loop each interface
		for( pcap_iface = ifaces; pcap_iface != NULL && *netmask == 0; pcap_iface = pcap_iface->next )
		{
			// found the interface we need
			if( strcmp( pcap_iface->name, iface ) == 0 )
			{
				// loop each interface address
				for( pcap_address = pcap_iface->addresses; pcap_address != NULL; pcap_address = pcap_address->next )
				{
					// found a suitable address ?
					if( pcap_address->addr->sa_family == AF_INET )
					{
						*ifaddr  = *( unsigned int *)&((struct sockaddr_in*)pcap_address->addr)->sin_addr;
						*netmask = *( unsigned int *)&((struct sockaddr_in*)pcap_address->netmask)->sin_addr;

						break;
					}
				}
			}
		}

		if( *netmask != 0 && *ifaddr != 0 )
		{
			*nhosts = ntohl( ~( *netmask ) );
			retval  = 0;
		}
	}

	return retval;
}

void net_wake( char *iface, int nhosts, int ifaddr, int netmask, in_addr_t gateway )
{
	int			           i, sd;
	in_addr_t          address;
	struct sockaddr_in sin;
	unsigned char      dummy[128] = {0};

	// force the arp cache to be populated
	for( i = 1; i <= nhosts; i++ )
	{
		address = ( gateway & netmask ) | htonl( i );
		// skip local address and gateway address
		if( address != ifaddr && address != gateway )
		{
			// send a dummy udp packet just to wake up the arp cache
			if( ( sd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) > 0 )
			{
				memset( &sin, 0, sizeof(sin) );

				sin.sin_family 		  = AF_INET;
				sin.sin_addr.s_addr = address;
				sin.sin_port 				= htons(67);

				sendto( sd, dummy, 128, 0, (struct sockaddr *)&sin, sizeof(sin) );

				close( sd );
			}
		}
	}

	// seems a fair amount of time to wait
	sleep( 1 );
}

Hashmap *net_get_mapping( char *iface, int nhosts, int ifaddr, int netmask, in_addr_t gateway, int *count )
{
	network_entry_t   *entry;
	Hashmap           *netmap = NULL;
	int			           i;
	in_addr_t          address;
	struct ether_addr  hardware;

	netmap = hashmapCreate( nhosts, address_hash, address_hash_equals );

	*count = 0;

	// precompute alive addresses
	for( i = 1; i <= nhosts; i++ )
	{
		address = ( gateway & netmask ) | htonl( i );
		// skip local address and gateway address
		if( address != ifaddr && address != gateway )
		{
			// skip endpoints unknown to arp and broadcasting address
			if( arp_lookup( address, &hardware, iface ) == 0 )
			{
				entry = ( network_entry_t * )malloc( sizeof( network_entry_t ) );

				memcpy( &entry->address, &address,  sizeof( address ) );
				memcpy( &entry->mac, 		 &hardware, sizeof( hardware ) );

				hashmapPut( netmap, (void *)address, entry );

				++(*count);
			}
		}
	}

	return netmap;
}
