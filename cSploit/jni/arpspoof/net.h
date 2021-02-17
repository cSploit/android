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
#ifndef __NET_H
#define __NET_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <libnet.h>
#include <netinet/if_ether.h>
#include <err.h>
#include <pcap.h>
#include <droid.h>
#include "hashmap.h"

typedef struct
{
	int 					address;
	unsigned char mac[6];
}
network_entry_t;

int 		 net_get_details( char *iface, int *netmask, int *ifaddr, int *nhosts );
void	   net_wake( char *iface, int nhosts, int ifaddr, int netmask, in_addr_t gateway );
Hashmap *net_get_mapping( char *iface, int nhosts, int ifaddr, int netmask, in_addr_t gateway, int *count );

#endif
