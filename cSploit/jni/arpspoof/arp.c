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
#include "arp.h"

static inline int xdigit (char c)
{
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
struct ether_addr *ether_aton_r (const char *asc, struct ether_addr * addr)
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

		addr->ether_addr_octet[i] = (unsigned char)((val0 << 4) + val1);

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
struct ether_addr *ether_aton (const char *asc)
{
	static struct ether_addr addr;
	return ether_aton_r(asc, &addr);
}

int arp_lookup( in_addr_t address, struct ether_addr *ether, const char* ifname )
{
	int num, type, flags;
	FILE *fp 			 = fopen( "/proc/net/arp", "r" );
	char ip[128]   = { 0x00 },
			 hwa[128]  = { 0x00 },
			 mask[128] = { 0x00 },
			 line[128] = { 0x00 },
			 dev[128]  = { 0x00 },
			*ipaddress = inet_ntoa( *( struct in_addr *)&address );

	if( !fp ) return -1;

	/*
	 * Bypass header, read one line.
	 */
	fgets( line, sizeof(line), fp );

	while( fgets( line, sizeof(line), fp ) )
	{
		/*
		 * All these strings can't overflow because fgets above reads limited amount of data
		 */
		num = sscanf( line, "%s 0x%x 0x%x %s %s %s\n", ip, &type, &flags, hwa, mask, dev );
		if( num < 4 )
			continue;
		/*
		 * Not on the same interface, skip.
		 */
		if( strcmp( dev, ifname ) != 0 )
			continue;
		/*
		 * Unknown or broadcast address, skip.
		 */
		if( strcmp( hwa, "00:00:00:00:00:00" ) == 0 || strcmp( hwa, "FF:FF:FF:FF:FF:FF" ) == 0 || strcmp( hwa, "ff:ff:ff:ff:ff:ff" ) == 0 )
			continue;
		/*
		 * Found!
		 */
		if( strcmp( ip, ipaddress ) == 0 )
		{
			memcpy( ether, (void const *)ether_aton( hwa ), sizeof( struct ether_addr ) );

			printf( "%s -> %s\n", ip, hwa );

			fclose( fp );

			return 0;
		}
	}

	fclose( fp );

	return -1;
}

int arp_send( libnet_t *pnet, int opcode, unsigned char *source_hardware, in_addr_t source_address, unsigned char *target_hardware, in_addr_t target_address )
{
	int retval = -1;

	if( source_hardware != NULL || ( source_hardware = (unsigned char *)libnet_get_hwaddr( pnet ) ) != NULL )
	{
		if( source_address != 0 || ( source_address = libnet_get_ipaddr4( pnet ) ) != -1 )
		{
			if( target_hardware == NULL )
				target_hardware = BROADCAST_MAC_ADDR;

			if( libnet_autobuild_arp( opcode, source_hardware, (unsigned char *)&source_address, target_hardware, (unsigned char *)&target_address, pnet ) != -1 )
			{
				if( libnet_build_ethernet( target_hardware, source_hardware, ETHERTYPE_ARP, NULL, 0, pnet, 0 ) != -1 )
				{
					retval = libnet_write( pnet );
					if( retval == -1 )
						printf( "error sending packet : %s\n", libnet_geterror( pnet ) );

					else
						printf
						(
								"sent arp %s from %s [ %x:%x:%x:%x:%x:%x ] to %s [ %x:%x:%x:%x:%x:%x ]\n",
								opcode == ARPOP_REQUEST ? "request" : "reply",
								inet_ntoa( *(struct in_addr *)&source_address ),
								source_hardware[0], source_hardware[1], source_hardware[2], source_hardware[3], source_hardware[4], source_hardware[5],
								inet_ntoa( *(struct in_addr *)&target_address ),
								target_hardware[0], target_hardware[1], target_hardware[2], target_hardware[3], target_hardware[4], target_hardware[5]
						);

					libnet_clear_packet( pnet );
				}
				else
					printf( "error building ethernet packet : %s\n", libnet_geterror( pnet ) );
			}
			else
				printf( "error building arp packet : %s\n", libnet_geterror( pnet ) );
		}
		else
			printf( "error obtaining source address : %s\n", libnet_geterror( pnet ) );
	}
	else
		printf( "error obtaining source hardware address : %s\n", libnet_geterror( pnet ) );

	return retval;
}

