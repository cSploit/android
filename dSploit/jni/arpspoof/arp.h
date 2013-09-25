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
#ifndef __ARP_H
#define __ARP_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <libnet.h>
#include <netinet/if_ether.h>
#include <err.h>
#include <pcap.h>
#include <droid.h>

#define BROADCAST_MAC_ADDR "\xff\xff\xff\xff\xff\xff"
#define VOID_MAC_ADDR			 "\x00\x00\x00\x00\x00\x00"

/*
 * Convert Ethernet address in the standard hex-digits-and-colons to binary
 * representation.
 */
struct ether_addr *ether_aton( const char *asc );

int arp_lookup( in_addr_t address, struct ether_addr *ether, const char* ifname );
int arp_send( libnet_t *pnet, int op, unsigned char *sha, in_addr_t spa, unsigned char *tha, in_addr_t tpa );

#endif
