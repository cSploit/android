/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IFINFO_H
#define IFINFO_H

#include <stdint.h>

#include "netdefs.h"

extern struct if_info {
  char     name[IFNAMSIZ];
  uint8_t  eth_addr[ETH_ALEN];
  uint32_t ip_subnet;
  uint32_t ip_addr;
  uint32_t ip_mask;
  uint32_t ip_broadcast;
  int index;
  uint32_t mtu;
} ifinfo;

int get_ifinfo(char *);

#endif
